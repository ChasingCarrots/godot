#ifndef NETWORKCONDITIONSIMULATOR_H
#define NETWORKCONDITIONSIMULATOR_H

#include "core/math/random_pcg.h"
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "core/variant/dictionary.h"

#include <scene/main/multiplayer_peer.h>

#include <atomic>

// Packet conditioner for CommunicationLineSystem, driven from a test harness:
// adds one-way latency, jitter and loss to CLS traffic and can black out
// individual peers to simulate a link going down.
//
// Inert until configured. is_active() is one relaxed atomic load and every hot
// path checks it first, so an unconfigured simulator costs a single load per
// packet and nothing else.
//
// Latency/jitter/loss apply to EGRESS only. Both endpoints run their own
// simulator, so a round trip between two peers that each add 50 ms measures
// ~100 ms of ping, and one-directional conditions are expressed by configuring
// only one side. Blackout instead drops BOTH directions for the affected peer,
// which is what a dead link looks like from either end.
//
// Delayed packets keep their FIFO order per (peer, channel) on reliable
// channels: the CLS protocol relies on reliable-ordered delivery, and
// reordering it would produce bug reports the network cannot actually cause.
// Unreliable channels are free to reorder under jitter.
class NetworkConditionSimulator {
public:
	// A packet held back by the latency queue, waiting to be put on the peer.
	struct Packet {
		uint64_t release_time_ms = 0;
		uint64_t order = 0; // tie-break so equal release times stay FIFO
		int peer_id = 0;
		int channel = 0;
		MultiplayerPeer::TransferMode mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE;
		PackedByteArray data;
	};

	NetworkConditionSimulator() { _rng.seed(DEFAULT_SEED); }

	_FORCE_INLINE_ bool is_active() const { return _active.load(std::memory_order_relaxed); }

	void configure(int p_latency_ms, int p_jitter_ms, float p_packet_loss);
	void set_blackout_all(bool p_enabled);
	void set_peer_blackout(int p_peer_id, bool p_enabled);
	// Back to fully inert: clears the conditions, the blackout set and the queue.
	void reset();
	// Drops held-back packets without touching the configuration.
	void clear_queue();
	void forget_peer(int p_peer_id);

	bool is_blacked_out(int p_peer_id) const;

	// False when the packet must not reach the peer right now, because it was
	// either dropped or queued for later release by take_due_outgoing().
	bool admit_outgoing(int p_peer_id, int p_channel, MultiplayerPeer::TransferMode p_mode, const PackedByteArray &p_data);
	bool admit_incoming(int p_peer_id);

	// Appends every queued packet whose release time has passed, in release order.
	void take_due_outgoing(uint64_t p_now_ms, LocalVector<Packet> &r_out);

	Dictionary get_status() const;

private:
	static constexpr uint64_t DEFAULT_SEED = 0x5eed1337;

	std::atomic<bool> _active{ false };
	std::atomic<uint32_t> _latency_ms{ 0 };
	std::atomic<uint32_t> _jitter_ms{ 0 };
	std::atomic<float> _loss{ 0.0f };
	std::atomic<bool> _blackout_all{ false };
	// Lets is_blacked_out() skip the lock while no per-peer blackout exists.
	std::atomic<bool> _has_peer_blackout{ false };

	std::atomic<uint64_t> _dropped_outgoing{ 0 };
	std::atomic<uint64_t> _dropped_incoming{ 0 };
	std::atomic<uint64_t> _delayed{ 0 };

	mutable BinaryMutex _mutex; // guards everything below
	HashSet<int> _blackout_peers;
	LocalVector<Packet> _queue; // sorted by (release_time_ms, order)
	HashMap<uint64_t, uint64_t> _last_release; // peer/channel key -> last reliable release time
	uint64_t _order = 0;
	RandomPCG _rng;

	static _FORCE_INLINE_ uint64_t channel_key(int p_peer_id, int p_channel) {
		return (static_cast<uint64_t>(static_cast<uint32_t>(p_peer_id)) << 32) | static_cast<uint32_t>(p_channel);
	}
	// _mutex must be held.
	void update_active();
	void insert_sorted(Packet &&p_packet);
};

#endif //NETWORKCONDITIONSIMULATOR_H
