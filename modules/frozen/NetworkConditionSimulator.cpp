#include "NetworkConditionSimulator.h"

#include "core/os/os.h"
#include "core/variant/array.h"

void NetworkConditionSimulator::configure(const int p_latency_ms, const int p_jitter_ms, const float p_packet_loss) {
	_latency_ms.store(static_cast<uint32_t>(MAX(0, p_latency_ms)), std::memory_order_relaxed);
	_jitter_ms.store(static_cast<uint32_t>(MAX(0, p_jitter_ms)), std::memory_order_relaxed);
	_loss.store(CLAMP(p_packet_loss, 0.0f, 1.0f), std::memory_order_relaxed);
	MutexLock lock(_mutex);
	update_active();
}

void NetworkConditionSimulator::set_blackout_all(const bool p_enabled) {
	_blackout_all.store(p_enabled, std::memory_order_relaxed);
	MutexLock lock(_mutex);
	if (p_enabled) {
		// A blackout drops packets rather than holding them, so anything already
		// waiting would surface as a burst the moment the link comes back.
		_queue.clear();
		_last_release.clear();
	}
	update_active();
}

void NetworkConditionSimulator::set_peer_blackout(const int p_peer_id, const bool p_enabled) {
	MutexLock lock(_mutex);
	if (p_enabled) {
		_blackout_peers.insert(p_peer_id);
		for (uint32_t i = _queue.size(); i > 0; i--) {
			if (_queue[i - 1].peer_id == p_peer_id) {
				_queue.remove_at(i - 1);
			}
		}
	} else {
		_blackout_peers.erase(p_peer_id);
	}
	_has_peer_blackout.store(!_blackout_peers.is_empty(), std::memory_order_relaxed);
	update_active();
}

void NetworkConditionSimulator::reset() {
	_latency_ms.store(0, std::memory_order_relaxed);
	_jitter_ms.store(0, std::memory_order_relaxed);
	_loss.store(0.0f, std::memory_order_relaxed);
	_blackout_all.store(false, std::memory_order_relaxed);
	_has_peer_blackout.store(false, std::memory_order_relaxed);
	_dropped_outgoing.store(0, std::memory_order_relaxed);
	_dropped_incoming.store(0, std::memory_order_relaxed);
	_delayed.store(0, std::memory_order_relaxed);
	MutexLock lock(_mutex);
	_blackout_peers.clear();
	_queue.clear();
	_last_release.clear();
	_order = 0;
	_rng.seed(DEFAULT_SEED);
	update_active();
}

void NetworkConditionSimulator::clear_queue() {
	MutexLock lock(_mutex);
	_queue.clear();
	_last_release.clear();
}

void NetworkConditionSimulator::forget_peer(const int p_peer_id) {
	MutexLock lock(_mutex);
	_blackout_peers.erase(p_peer_id);
	_has_peer_blackout.store(!_blackout_peers.is_empty(), std::memory_order_relaxed);
	for (uint32_t i = _queue.size(); i > 0; i--) {
		if (_queue[i - 1].peer_id == p_peer_id) {
			_queue.remove_at(i - 1);
		}
	}
	for (int channel = 0; channel < 8; channel++) {
		_last_release.erase(channel_key(p_peer_id, channel));
	}
	update_active();
}

bool NetworkConditionSimulator::is_blacked_out(const int p_peer_id) const {
	if (_blackout_all.load(std::memory_order_relaxed)) {
		return true;
	}
	if (!_has_peer_blackout.load(std::memory_order_relaxed)) {
		return false;
	}
	MutexLock lock(_mutex);
	return _blackout_peers.has(p_peer_id);
}

bool NetworkConditionSimulator::admit_outgoing(const int p_peer_id, const int p_channel, const MultiplayerPeer::TransferMode p_mode, const PackedByteArray &p_data) {
	if (is_blacked_out(p_peer_id)) {
		_dropped_outgoing.fetch_add(1, std::memory_order_relaxed);
		return false;
	}

	const float loss = _loss.load(std::memory_order_relaxed);
	const uint32_t latency = _latency_ms.load(std::memory_order_relaxed);
	const uint32_t jitter = _jitter_ms.load(std::memory_order_relaxed);
	if (loss <= 0.0f && latency == 0 && jitter == 0) {
		return true;
	}

	MutexLock lock(_mutex);
	if (p_mode == MultiplayerPeer::TRANSFER_MODE_UNRELIABLE && loss > 0.0f && _rng.randf() < loss) {
		_dropped_outgoing.fetch_add(1, std::memory_order_relaxed);
		return false;
	}
	if (latency == 0 && jitter == 0) {
		return true;
	}

	uint64_t release = OS::get_singleton()->get_ticks_msec() + latency;
	if (jitter > 0) {
		release += static_cast<uint64_t>(_rng.randf() * static_cast<float>(jitter));
	}
	// Reliable channels must not be reordered: the receiver hands packets to the
	// protocol in arrival order, and a permuted reliable stream is a failure mode
	// the real network cannot produce.
	if (p_mode != MultiplayerPeer::TRANSFER_MODE_UNRELIABLE) {
		const uint64_t key = channel_key(p_peer_id, p_channel);
		if (uint64_t *last = _last_release.getptr(key)) {
			release = MAX(release, *last);
			*last = release;
		} else {
			_last_release.insert(key, release);
		}
	}

	Packet packet;
	packet.release_time_ms = release;
	packet.order = _order++;
	packet.peer_id = p_peer_id;
	packet.channel = p_channel;
	packet.mode = p_mode;
	packet.data = p_data;
	insert_sorted(std::move(packet));
	_delayed.fetch_add(1, std::memory_order_relaxed);
	return false;
}

bool NetworkConditionSimulator::admit_incoming(const int p_peer_id) {
	if (!is_blacked_out(p_peer_id)) {
		return true;
	}
	_dropped_incoming.fetch_add(1, std::memory_order_relaxed);
	return false;
}

void NetworkConditionSimulator::take_due_outgoing(const uint64_t p_now_ms, LocalVector<Packet> &r_out) {
	MutexLock lock(_mutex);
	uint32_t due = 0;
	while (due < _queue.size() && _queue[due].release_time_ms <= p_now_ms) {
		due++;
	}
	if (due == 0) {
		return;
	}
	for (uint32_t i = 0; i < due; i++) {
		r_out.push_back(std::move(_queue[i]));
	}
	// The queue is sorted, so the due packets are a prefix: shift the rest down once.
	const uint32_t remaining = _queue.size() - due;
	for (uint32_t i = 0; i < remaining; i++) {
		_queue[i] = std::move(_queue[i + due]);
	}
	_queue.resize(remaining);
	// Draining the last held-back packet may be what makes the simulator inert again.
	update_active();
}

Dictionary NetworkConditionSimulator::get_status() const {
	Dictionary status;
	status["active"] = is_active();
	status["latency_ms"] = static_cast<int>(_latency_ms.load(std::memory_order_relaxed));
	status["jitter_ms"] = static_cast<int>(_jitter_ms.load(std::memory_order_relaxed));
	status["packet_loss"] = _loss.load(std::memory_order_relaxed);
	status["blackout_all"] = _blackout_all.load(std::memory_order_relaxed);
	status["dropped_outgoing"] = static_cast<int64_t>(_dropped_outgoing.load(std::memory_order_relaxed));
	status["dropped_incoming"] = static_cast<int64_t>(_dropped_incoming.load(std::memory_order_relaxed));
	status["delayed"] = static_cast<int64_t>(_delayed.load(std::memory_order_relaxed));

	Array blacked_out;
	{
		MutexLock lock(_mutex);
		for (const int peer_id : _blackout_peers) {
			blacked_out.push_back(peer_id);
		}
		status["queued"] = static_cast<int>(_queue.size());
	}
	blacked_out.sort();
	status["blackout_peers"] = blacked_out;
	return status;
}

void NetworkConditionSimulator::update_active() {
	_active.store(_latency_ms.load(std::memory_order_relaxed) > 0 ||
						_jitter_ms.load(std::memory_order_relaxed) > 0 ||
						_loss.load(std::memory_order_relaxed) > 0.0f ||
						_blackout_all.load(std::memory_order_relaxed) ||
						!_blackout_peers.is_empty() ||
						!_queue.is_empty(),
			std::memory_order_relaxed);
}

void NetworkConditionSimulator::insert_sorted(Packet &&p_packet) {
	uint32_t index = _queue.size();
	while (index > 0) {
		const Packet &previous = _queue[index - 1];
		if (previous.release_time_ms < p_packet.release_time_ms ||
				(previous.release_time_ms == p_packet.release_time_ms && previous.order < p_packet.order)) {
			break;
		}
		index--;
	}
	_queue.insert(index, std::move(p_packet));
	// A non-empty queue has to keep the simulator active, or the last packets
	// would never be flushed after the conditions are cleared.
	_active.store(true, std::memory_order_relaxed);
}
