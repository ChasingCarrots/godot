#ifndef COMMUNICATIONLINESYSTEM_H
#define COMMUNICATIONLINESYSTEM_H

#include "CommunicationLine.h"
#include "core/profiling/profiling.h"

#include <scene/main/node.h>

class CommunicationLineSystem : public Node {
	GDCLASS(CommunicationLineSystem, Node)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _notification(int p_notification);

private:
	friend CommunicationLine;
	// Ref<SceneMultiplayer> _scene_multiplayer;
	uint32_t _next_line_id = 1;
	Vector<Ref<CommunicationLine>> _communication_lines;
	Ref<StreamPeerBuffer> _receive_buffer;
	Ref<StreamPeerBuffer> _send_buffer;

	Ref<MultiplayerPeer> _multiplayer_peer;
	MultiplayerPeer::ConnectionStatus last_connection_status = MultiplayerPeer::CONNECTION_DISCONNECTED;

	HashSet<int> _connected_peers;

	int _remote_sender_id = 0;
	int _server_id = -1;

	void _ready();
	void _process(double p_time);
	void on_new_peer_connected(int multiplayer_id);
	void on_peer_disconnected(int multiplayer_id);
	void on_packet_received(int from_multiplayer_id, const PackedByteArray &packet);
	void send_packet_to_peer(const PackedByteArray &bytes, int peer_id, MultiplayerPeer::TransferMode mode) const;
	void send_packet_to_server(const PackedByteArray &bytes, MultiplayerPeer::TransferMode mode) const;
	Error poll();
	void clear_multiplayer_peer();
	void update_status();
	void process_packet(int from, const uint8_t *packet, int packet_len);
	void send_to_peer(int to, const PackedByteArray &packet, MultiplayerPeer::TransferMode mode) const;
	Ref<MultiplayerPeer> get_multiplayer_peer() { return _multiplayer_peer; }

	static CommunicationLineSystem *_global_coms;

public:
	CommunicationLineSystem();

	static CommunicationLineSystem *get_global_communication_line_system() { return _global_coms; }
	static void set_global_communication_line_system(CommunicationLineSystem *system) { _global_coms = system; }
	int get_remote_sender_id() const { return _remote_sender_id; }

	Ref<CommunicationLine> grab_communication_line(const StringName &id);
	void remove_communication_line(const Ref<CommunicationLine> removed_cl);

	// access to all lines mostly for debug visualization purposes:
	int get_number_of_communication_lines() const { return _communication_lines.size(); }
	Ref<CommunicationLine> get_communication_line(int index) const {
		if (index < 0 || index >= _communication_lines.size()) {
			return {};
		}
		return _communication_lines[index];
	}
	void set_multiplayer_peer(const Ref<MultiplayerPeer> &p_peer);
	
	Vector<int> get_connected_peer_ids() const;
	
	void initialize_server();
	
	void set_server_id(const int id) {
		GodotProfileFunction();
		_server_id = id;

		print_line("[", get_local_multiplayer_id() ,"] Server ID set to: ", id);
		if (id != get_local_multiplayer_id()){
			GodotProfileZone("connected_to_server_signal");
			emit_signal(SNAME("connected_to_server"));
		}
	}
	
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
		
		return _multiplayer_peer->get_unique_id();
	};
};

#endif //COMMUNICATIONLINESYSTEM_H
