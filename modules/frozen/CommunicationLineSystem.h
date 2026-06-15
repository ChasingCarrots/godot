#ifndef COMMUNICATIONLINESYSTEM_H
#define COMMUNICATIONLINESYSTEM_H

#include "CommunicationLine.h"
#include "NetworkChunking.h"
#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/profiling/profiling.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

#include <scene/main/node.h>

#include <atomic>

class CommunicationLineSystem : public Node {
	GDCLASS(CommunicationLineSystem, Node)

public:
	// Sub-types for packets sent on the dedicated ping/control channels.
	enum class PingPacketType : uint8_t { Request, Response };
	enum class ControlPacketType : uint8_t { Disconnect };

	// Per-connection tracking info for a single connected peer.
	struct PeerConnection {
		uint32_t ping_ms = 0;            // last measured round-trip time
		float jitter_ms = 0.0f;          // EWMA of |rtt - smoothed_rtt|
		float packet_loss = 0.0f;        // EWMA, 0..1
		int64_t clock_offset_ms = 0;     // estimated (remote_now - local_now)
		uint64_t last_ping_sent_time = 0;
		uint64_t last_packet_time = 0;   // any packet from this peer; used for timeout
		bool awaiting_pong = false;      // a ping is outstanding (packet-loss sampling)
	};

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _notification(int p_notification);

private:
	friend CommunicationLine;
	friend ChunkReceiver; //We need access to process packet in ChunkReceiver

	// An event observed by the poll thread, handed to the main thread in order.
	// One FIFO for connects, packets and disconnects so that a peer's packets are
	// always processed after its connect event and before its disconnect event.
	struct NetworkEvent {
		enum class Type : uint8_t { PeerConnected, PeerDisconnected, Packet };
		Type type = Type::Packet;
		int peer_id = 0;
		int channel = 0; // Packet only
		PackedByteArray data; // Packet only
	};

	uint32_t _next_line_id = 1;
	Vector<Ref<CommunicationLine>> _communication_lines;
	Ref<StreamPeerBuffer> _receive_buffer;
	Ref<StreamPeerBuffer> _send_buffer;
	// Poll-thread-owned counterparts: ping/control packets are handled on the
	// poll thread and must not share the main thread's scratch buffers.
	Ref<StreamPeerBuffer> _thread_receive_buffer;
	Ref<StreamPeerBuffer> _thread_send_buffer;

	Ref<MultiplayerPeer> _multiplayer_peer;
	// The poll thread pumps the peer so the connection stays healthy (ACKs,
	// keepalives) while the main thread stalls; packet processing and signal
	// emission stay on the main thread via _event_queue.
	// Lock order: _peer_mutex -> {_event_queue_mutex | _connected_peers_mutex}.
	// The inner two are leaves: never held while calling into the peer or taking
	// another lock, and never nested with each other.
	Thread _poll_thread;
	mutable Mutex _peer_mutex; // serializes ALL access to _multiplayer_peer
	std::atomic<bool> _poll_thread_exit = false;
	Mutex _event_queue_mutex;
	LocalVector<NetworkEvent> _event_queue; // filled by the poll thread (and trampolines)
	LocalVector<NetworkEvent> _pending_events; // main-thread only: events taken from the queue
	uint32_t _pending_index = 0;
	std::atomic<int> _cached_status = MultiplayerPeer::CONNECTION_DISCONNECTED;
	int _cached_unique_id = 0; // peers don't change their id, cached for lock-free reads
	Ref<ChunkSender> _chunk_sender;
	Ref<ChunkReceiver> _chunk_receiver;
	MultiplayerPeer::ConnectionStatus last_connection_status = MultiplayerPeer::CONNECTION_DISCONNECTED;

	// Protected by _connected_peers_mutex (innermost leaf lock): the poll thread
	// writes ping/loss stats and last_packet_time, the main thread inserts/erases
	// on connect/disconnect. NEVER call into the peer or take another lock while
	// holding it.
	HashMap<int, PeerConnection> _connected_peers;
	mutable BinaryMutex _connected_peers_mutex;

	uint64_t _last_ping_send_time_ms = 0; // poll-thread only
	std::atomic<bool> _closing = false; // written on the main thread, read by the poll thread (ping gate)
	uint64_t _close_requested_time = 0;

	int _remote_sender_id = 0;
	int _server_id = -1;

	void _ready();
	void _process(double p_time);
	void on_new_peer_connected(int multiplayer_id);
	void on_peer_disconnected(int multiplayer_id);
	void on_packet_received(int from_multiplayer_id, const PackedByteArray &packet);
	void send_packet_to_peer(const PackedByteArray &bytes, int peer_id, MultiplayerPeer::TransferMode mode) const;
	void send_packet_to_server(const PackedByteArray &bytes, MultiplayerPeer::TransferMode mode) const;
	static void _poll_thread_func(void *p_self);
	void _poll_iteration();
	void _start_poll_thread();
	void _stop_poll_thread();
	void _push_network_event(NetworkEvent &&p_event);
	// Signal trampolines: peer signals can fire on the poll thread (ENet emits
	// from inside poll()) or the main thread; these only queue an event, the real
	// handlers run on drain.
	void _on_peer_connected_trampoline(int multiplayer_id);
	void _on_peer_disconnected_trampoline(int multiplayer_id);
	void _drain_network_events();
	void clear_multiplayer_peer();
	void update_status();
	// Main-thread only: used by ChunkReceiver to feed reassembled chunks back in.
	void process_packet(int from, const uint8_t *packet, int packet_len);
	// Poll-thread only (with _peer_mutex held): answers ping requests and updates stats.
	void handle_ping_packet(int from, const uint8_t *packet, int packet_len);
	// Poll-thread only (with _peer_mutex held). Returns true when the message was
	// fully handled; false queues the packet so handle_control_packet() runs it
	// on the main thread (for future control types that need main-thread state).
	bool _handle_control_packet_threaded(int from, const uint8_t *packet, int packet_len);
	// Main-thread side of control handling: only sees messages the poll thread deferred.
	void handle_control_packet(int from, const uint8_t *packet, int packet_len);
	// Poll-thread only (with _peer_mutex held), driven by _last_ping_send_time_ms.
	void send_pings();
	void check_peer_timeouts();
	void send_to_peer(int to, const PackedByteArray &packet, MultiplayerPeer::TransferMode mode) const;
	void send_internal_packet(int to, const PackedByteArray &packet, MultiplayerPeer::TransferMode mode, int channel) const;
	Ref<MultiplayerPeer> get_multiplayer_peer() { return _multiplayer_peer; }

	static CommunicationLineSystem *_global_coms;

public:
	CommunicationLineSystem();
	~CommunicationLineSystem();

	static CommunicationLineSystem *get_global_communication_line_system() { return _global_coms; }
	static void set_global_communication_line_system(CommunicationLineSystem *system) { _global_coms = system; }
	int get_remote_sender_id() const { return _remote_sender_id; }

	Ref<CommunicationLine> grab_communication_line(const StringName &id);
	void remove_communication_line(const Ref<CommunicationLine> removed_cl);
	void remove_all_communication_lines();

	// access to all lines mostly for debug visualization purposes:
	int get_number_of_communication_lines() const { return _communication_lines.size(); }
	Ref<CommunicationLine> get_communication_line(int index) const {
		if (index < 0 || index >= _communication_lines.size()) {
			return {};
		}
		return _communication_lines[index];
	}
	void set_multiplayer_peer(const Ref<MultiplayerPeer> &p_peer);

	// Disconnects a peer on the multiplayer peer, synchronized with the poll
	// thread. Script must use this instead of calling disconnect_peer() on the
	// peer object directly.
	void disconnect_peer(int peer_id);

	// For script code that has to mutate the live peer directly (e.g.
	// add_mesh_peer): hold this lock around the call so it cannot race the poll
	// thread. _peer_mutex is recursive, so calling CLS methods while holding it
	// is safe, but keep the locked section minimal.
	void lock_multiplayer_peer() { _peer_mutex.lock(); }
	void unlock_multiplayer_peer() { _peer_mutex.unlock(); }

	Vector<int> get_connected_peer_ids() const;

	// Per-connection tracking, queryable from GDScript.
	int get_peer_ping(int peer_id) const;
	float get_peer_jitter(int peer_id) const;
	float get_peer_packet_loss(int peer_id) const;
	int get_peer_clock_offset(int peer_id) const;

	// Gracefully leaves the mesh: tells every peer we are disconnecting so they
	// drop us immediately, then closes the local peer and emits "connection_closed".
	bool close_connection();

	void initialize_server();
	
	void set_server_id(int id);
	
	int get_server_id() const {
		return _server_id;
	}
	bool is_server() const {
		if (_multiplayer_peer == nullptr) { return false; }

		return get_local_multiplayer_id() == _server_id;
	}
	int get_local_multiplayer_id() const {
		GodotProfileFunction();
		if (_multiplayer_peer == nullptr) { return -1; }

		return _cached_unique_id;
	};
};

#endif //COMMUNICATIONLINESYSTEM_H
