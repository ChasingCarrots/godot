#include "CommunicationLineSystem.h"

#include "CommunicationLine.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/profiling/profiling.h"

constexpr int COMMUNICATION_LINE_CHANNEL_RELIABLE = 1; //Documentation mentions that channel 0 works as 3 separate channels (https://docs.godotengine.org/en/stable/classes/class_multiplayerpeer.html#class-multiplayerpeer-property-transfer-channel)
constexpr int COMMUNICATION_LINE_CHANNEL_UNRELIABLE = 2;
constexpr int COMMUNICATION_LINE_CHANNEL_PING = 3;
constexpr int COMMUNICATION_LINE_CHANNEL_CONTROL = 4;

constexpr uint64_t PING_INTERVAL_MS = 1000; // how often we ping each connected peer
constexpr uint64_t PEER_TIMEOUT_MS = 60000; // no packet for this long -> peer is considered gone
constexpr uint64_t CLOSE_GRACE_MS = 300; // time we keep polling after close_connection() so the disconnect packet flushes

CommunicationLineSystem* CommunicationLineSystem::_global_coms = nullptr;

CommunicationLineSystem::CommunicationLineSystem() {
	_receive_buffer.instantiate();
	_send_buffer.instantiate();
	_thread_receive_buffer.instantiate();
	_thread_send_buffer.instantiate();
	_chunk_sender.instantiate();
	_chunk_receiver.instantiate();
	_chunk_receiver->initialize(this);
}

CommunicationLineSystem::~CommunicationLineSystem() {
	_stop_poll_thread();
}

void CommunicationLineSystem::_bind_methods() {
	ClassDB::bind_static_method("CommunicationLineSystem", D_METHOD("set_global_communication_line_system", "system"),
		&CommunicationLineSystem::set_global_communication_line_system);
	ClassDB::bind_static_method("CommunicationLineSystem", D_METHOD("get_global_communication_line_system"),
		&CommunicationLineSystem::get_global_communication_line_system);

	ClassDB::bind_method(D_METHOD("grab_communication_line", "string_id"), &CommunicationLineSystem::grab_communication_line);
	ClassDB::bind_method(D_METHOD("remove_communication_line", "communication_line"), &CommunicationLineSystem::remove_communication_line);
	ClassDB::bind_method(D_METHOD("remove_all_communication_lines"), &CommunicationLineSystem::remove_all_communication_lines);
	ClassDB::bind_method(D_METHOD("get_number_of_communication_lines"), &CommunicationLineSystem::get_number_of_communication_lines);
	ClassDB::bind_method(D_METHOD("get_communication_line", "index"), &CommunicationLineSystem::get_communication_line);
	ClassDB::bind_method(D_METHOD("set_multiplayer_peer", "peer"), &CommunicationLineSystem::set_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("get_multiplayer_peer"), &CommunicationLineSystem::get_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("get_remote_sender_id"), &CommunicationLineSystem::get_remote_sender_id);
	ClassDB::bind_method(D_METHOD("get_connected_peer_ids"), &CommunicationLineSystem::get_connected_peer_ids);
	ClassDB::bind_method(D_METHOD("get_peer_ping", "peer_id"), &CommunicationLineSystem::get_peer_ping);
	ClassDB::bind_method(D_METHOD("get_peer_jitter", "peer_id"), &CommunicationLineSystem::get_peer_jitter);
	ClassDB::bind_method(D_METHOD("get_peer_packet_loss", "peer_id"), &CommunicationLineSystem::get_peer_packet_loss);
	ClassDB::bind_method(D_METHOD("get_peer_clock_offset", "peer_id"), &CommunicationLineSystem::get_peer_clock_offset);
	ClassDB::bind_method(D_METHOD("close_connection"), &CommunicationLineSystem::close_connection);
	ClassDB::bind_method(D_METHOD("disconnect_peer", "peer_id"), &CommunicationLineSystem::disconnect_peer);
	ClassDB::bind_method(D_METHOD("lock_multiplayer_peer"), &CommunicationLineSystem::lock_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("unlock_multiplayer_peer"), &CommunicationLineSystem::unlock_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("get_local_multiplayer_id"), &CommunicationLineSystem::get_local_multiplayer_id);
	ClassDB::bind_method(D_METHOD("set_server_id", "id"), &CommunicationLineSystem::set_server_id);
	ClassDB::bind_method(D_METHOD("get_server_id"), &CommunicationLineSystem::get_server_id);
	ClassDB::bind_method(D_METHOD("is_server"), &CommunicationLineSystem::is_server);
	ClassDB::bind_method(D_METHOD("clear_multiplayer_peer"), &CommunicationLineSystem::clear_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("initialize_server"), &CommunicationLineSystem::initialize_server);

	ADD_SIGNAL(MethodInfo("peer_connected", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("peer_disconnected", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("peer_timed_out", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("connected_to_server"));
	ADD_SIGNAL(MethodInfo("connection_failed"));
	ADD_SIGNAL(MethodInfo("server_disconnected"));
	ADD_SIGNAL(MethodInfo("connection_closed"));
}

void CommunicationLineSystem::_notification(int p_notification) {
	switch(p_notification) {
		case NOTIFICATION_ENTER_TREE:

		break;
		case NOTIFICATION_PROCESS:
			if (!Engine::get_singleton()->is_editor_hint()) {
				_process(get_process_delta_time());
			}
		break;
		case NOTIFICATION_READY:
			_ready();
		break;
		case NOTIFICATION_EXIT_TREE:
			_stop_poll_thread();
			if (_multiplayer_peer.is_valid()) {
				_multiplayer_peer->disconnect("peer_connected", callable_mp(this, &CommunicationLineSystem::_on_peer_connected_trampoline));
				_multiplayer_peer->disconnect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::_on_peer_disconnected_trampoline));
			}
		break;
	}
}

void CommunicationLineSystem::_ready() {
	set_process(true);
	set_process_mode(PROCESS_MODE_ALWAYS);
}

void CommunicationLineSystem::_process(double p_time) {
	if (_multiplayer_peer.is_valid()) {
		// update_status() tears down (and clears the queues) if the poll thread saw
		// a disconnect, so the drain below only runs on a healthy peer.
		update_status();
		_drain_network_events();
	}

	if (_chunk_receiver.is_valid()) {
		_chunk_receiver->process();
	}

	// Resolve any calls whose answer never arrived, so a lost answer can't leak its call id.
	uint64_t process_now = OS::get_singleton()->get_ticks_msec();
	for (const Ref<CommunicationLine> &communication_line : _communication_lines) {
		if (communication_line.is_valid()) {
			communication_line->process_pending_calls(process_now);
		}
	}

	// Draining events may have torn down the peer, so re-check before doing connection tracking.
	if (_multiplayer_peer.is_valid() && last_connection_status == MultiplayerPeer::CONNECTION_CONNECTED) {
		if (_closing) {
			// We are gracefully leaving: the poll thread keeps the peer serviced so
			// the disconnect packet flushes, then we tear down once the grace period
			// elapsed - or earlier if every peer already acknowledged by dropping us.
			bool peers_empty;
			{
				MutexLock lock(_connected_peers_mutex);
				peers_empty = _connected_peers.is_empty();
			}
			uint64_t now = OS::get_singleton()->get_ticks_msec();
			if (peers_empty || now - _close_requested_time > CLOSE_GRACE_MS) {
				{
					// Only lock around close(): clear_multiplayer_peer() joins the
					// poll thread, which would deadlock if we still held the mutex.
					MutexLock lock(_peer_mutex);
					_multiplayer_peer->close();
					_cached_status.store(_multiplayer_peer->get_connection_status());
				}
				clear_multiplayer_peer();
				_closing = false;
				emit_signal(SNAME("connection_closed"));
			}
		} else {
			// Pings are sent from the poll thread; timeout detection stays here
			// because it emits script-visible signals.
			check_peer_timeouts();
		}
	}
	else if (_closing) {
		// make sure that the connection_closed signal is emmitted
		// when someone is waiting for it.
		uint64_t now = OS::get_singleton()->get_ticks_msec();
		if ( now - _close_requested_time > CLOSE_GRACE_MS) {
			_closing = false;
			emit_signal(SNAME("connection_closed"));
		}
	}
}

void CommunicationLineSystem::set_multiplayer_peer(const Ref<MultiplayerPeer> &p_peer) {
	if (p_peer == _multiplayer_peer) {
		return; // Nothing to do
	}

	ERR_FAIL_COND_MSG(p_peer.is_valid() && p_peer->get_connection_status() == MultiplayerPeer::CONNECTION_DISCONNECTED,
			"Supplied MultiplayerPeer must be connecting or connected.");

	if (_multiplayer_peer.is_valid()) {
		// clear_multiplayer_peer() stops the poll thread and disconnects the old
		// peer's signals for us.
		clear_multiplayer_peer();
	}

	// No poll thread is running here, so the peer can be accessed directly.
	_multiplayer_peer = p_peer;

	_chunk_sender->initialize(p_peer);

	int peer_id = 0;
	if(p_peer.is_valid()) {
		peer_id = p_peer->get_unique_id();
	}
	_cached_unique_id = peer_id;
	_cached_status.store(p_peer.is_valid() ? p_peer->get_connection_status() : MultiplayerPeer::CONNECTION_DISCONNECTED);

	//Our communication lines need the new peer id
	for (auto cl : _communication_lines){
		cl->_my_peer_info.set_multiplayer_id(peer_id);
	}

	if (_multiplayer_peer.is_valid()) {
		_multiplayer_peer->connect("peer_connected", callable_mp(this, &CommunicationLineSystem::_on_peer_connected_trampoline));
		_multiplayer_peer->connect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::_on_peer_disconnected_trampoline));
	}
	update_status();

	if (_multiplayer_peer.is_valid()) {
		_start_poll_thread();
	}
}

void CommunicationLineSystem::on_new_peer_connected(int multiplayer_id) {
	PeerConnection peer_connection;
	peer_connection.last_packet_time = OS::get_singleton()->get_ticks_msec();
	{
		MutexLock lock(_connected_peers_mutex);
		_connected_peers.insert(multiplayer_id, peer_connection);
	}

	if (!_communication_lines.is_empty() && is_server()) {
		// we send the new peer all the current communication lines, so they are up to speed!
		_send_buffer->clear();
		_send_buffer->put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CreateMultipleLines));
		// the line count is sent as a single byte; more than 255 lines would silently truncate.
		DEV_ASSERT(_communication_lines.size() <= 255);
		_send_buffer->put_u8(_communication_lines.size());
		for (const auto &line : _communication_lines) {
			_send_buffer->put_string(line->_string_id);
			_send_buffer->put_u16(line->_int_id);
		}
		send_packet_to_peer(_send_buffer->get_data_array(), multiplayer_id, MultiplayerPeer::TRANSFER_MODE_RELIABLE);
	}

	// now we update all our lines with the new peer, which will
	// also update the new peer of our state of the line.
	for (auto &line : _communication_lines) {
		line->new_peer_connected(multiplayer_id);
	}

	emit_signal(SNAME("peer_connected"), multiplayer_id);
}

void CommunicationLineSystem::on_peer_disconnected(int multiplayer_id) {
	{
		MutexLock lock(_connected_peers_mutex);
		_connected_peers.erase(multiplayer_id);
	}

	for (auto &line : _communication_lines) {
		line->peer_disconnected(multiplayer_id);
	}

	if (multiplayer_id == _server_id){
		emit_signal(SNAME("server_disconnected"));
	} else {
		emit_signal(SNAME("peer_disconnected"), multiplayer_id);
	}
}

void CommunicationLineSystem::on_packet_received(int from_multiplayer_id, const PackedByteArray &packet) {
	GodotProfileFunction();
	_receive_buffer->clear();
	_receive_buffer->set_data_array(packet);
	CommunicationLinePacketTypes packet_type = static_cast<CommunicationLinePacketTypes>(_receive_buffer->get_u8());
	switch (packet_type) {
		case CommunicationLinePacketTypes::CreateSingleLine: {
			StringName string_id = _receive_buffer->get_string();
			uint16_t int_id = _receive_buffer->get_u16();
			if (is_server()) {
				// we are the server and another client created this communication line.
				// that means we have to create it for us, assign it an id and send the
				// creation message to all clients (including the sender, so that they get
				// the id as well!)
				for (auto& line : _communication_lines) {
					if (line->_string_id == string_id) {
						// we already have that line created, that also means we already
						// have sent the creation message for it to all peers! -> do nothing
						return;
					}
				}
				// calling grab_communication_line as a server does exactly what we want here
				grab_communication_line(string_id);
			}
			else {
				// we are not the server, so this message has to have come from the server.
				for (auto& line : _communication_lines) {
					if (line->_string_id == string_id) {
						// we already have the line, so we probably asked the server to create
						// it and have to set the int id.
						line->_int_id = int_id;
						if (line->_my_peer_info.get_communication_state() == CommunicationLine::NotConnected) {
							// the line is not initialized on our side, so we can't set it to "open"
							line->update_own_communication_state(CommunicationLine::ConnectedClosed);
						} else {
							line->update_own_communication_state(CommunicationLine::ConnectedOpen);
						}
						return;
					}
				}
				Vector<int> peer_ids = get_connected_peer_ids();
				// we don't have this line, so we'll just create it.
				Ref<CommunicationLine> new_communication_line;
				new_communication_line.instantiate();
				new_communication_line->_communication_line_system = this;
				new_communication_line->_my_peer_info.set_multiplayer_id(get_local_multiplayer_id());
				new_communication_line->_string_id = string_id;
				new_communication_line->_int_id = int_id;
				new_communication_line->create_unconnected_peers(peer_ids);
				_communication_lines.append(new_communication_line);
				// the state is "closed" because the line is not initialized on our side
				new_communication_line->update_own_communication_state(CommunicationLine::ConnectedClosed);
			}
		}
		break;
		case CommunicationLinePacketTypes::CreateMultipleLines: {
			uint8_t number_of_lines = _receive_buffer->get_u8();
			for (uint8_t i = 0; i < number_of_lines; i++) {
				StringName string_id = _receive_buffer->get_string();
				uint16_t int_id = _receive_buffer->get_u16();
				bool line_exists = false;
				for (auto& line : _communication_lines) {
					if (line->_string_id == string_id) {
						// we already have the line, so we probably asked the server to create
						// it and have to set the int id.
						line->_int_id = int_id;
						if (line->_my_peer_info.get_communication_state() == CommunicationLine::NotConnected) {
							// the line is not initialized on our side, so we can't set it to "open"
							line->update_own_communication_state(CommunicationLine::ConnectedClosed);
						} else {
							line->update_own_communication_state(CommunicationLine::ConnectedOpen);
						}
						line_exists = true;
						break;
					}
				}
				if (!line_exists) {
					Vector<int> peer_ids = get_connected_peer_ids();
					// we don't have this line, so we'll just create it.
					Ref<CommunicationLine> new_communication_line;
					new_communication_line.instantiate();
					new_communication_line->_communication_line_system = this;
					new_communication_line->_my_peer_info.set_multiplayer_id(get_local_multiplayer_id());
					new_communication_line->_string_id = string_id;
					new_communication_line->_int_id = int_id;
					new_communication_line->create_unconnected_peers(peer_ids);
					_communication_lines.append(new_communication_line);
					// the state is "closed" because the line is not initialized on our side
					new_communication_line->update_own_communication_state(CommunicationLine::ConnectedClosed);
				}
			}
		}
		break;
		case CommunicationLinePacketTypes::SendDataChunk: {
			_chunk_receiver->receive_chunk_data(from_multiplayer_id, _receive_buffer);
			break;
		}
		default: {
			// this is a packet for a specific communication line. so we forward it:
			uint16_t line_id = _receive_buffer->get_u16();
			for (auto& line : _communication_lines) {
				if (line->_int_id == line_id) {
					line->on_packet_received(packet_type, from_multiplayer_id, _receive_buffer);
					break;
				}
			}
		}
		break;
	}
}

void CommunicationLineSystem::send_packet_to_peer(const PackedByteArray &bytes, const int to_peer_id, const MultiplayerPeer::TransferMode mode) const {
	send_to_peer(to_peer_id, bytes, mode);
}

void CommunicationLineSystem::send_packet_to_server(const PackedByteArray &bytes, const MultiplayerPeer::TransferMode mode) const {
	if (is_server()) {
		print_error("send_packet_to_server: we ARE the server! Handle this case directly...");
		return;
	}
	send_to_peer(_server_id, bytes, mode);
}

Ref<CommunicationLine> CommunicationLineSystem::grab_communication_line(const StringName &string_id) {
	GodotProfileFunction();
	for (auto &line : _communication_lines) {
		if (line->_string_id == string_id) {
			return line;
		}
	}
	// we don't have this line, so we'll just create it.
	Ref<CommunicationLine> new_communication_line;
	new_communication_line.instantiate();
	new_communication_line->_communication_line_system = this;
	new_communication_line->_string_id = string_id;
	_communication_lines.append(new_communication_line);

	//If we have no MultiplayerPeer we are currently offline and can directly return the communication line
	if (_multiplayer_peer == nullptr) {
		return new_communication_line;
	}

	new_communication_line->_my_peer_info.set_multiplayer_id(get_local_multiplayer_id());

	if (is_server()) {
		Vector<int> peer_ids = get_connected_peer_ids();
		new_communication_line->create_unconnected_peers(peer_ids);
		// we are the server, so we can directly set the int id
		new_communication_line->_int_id = _next_line_id++;
		// and send the creation info to everybody else
		_send_buffer->clear();
		_send_buffer->put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CreateSingleLine));
		_send_buffer->put_string(string_id);
		_send_buffer->put_u16(new_communication_line->_int_id);
		for (int to_id : peer_ids) {
			if (to_id == get_server_id()) {
				continue;
			}
			send_packet_to_peer(_send_buffer->get_data_array(), to_id, MultiplayerPeer::TRANSFER_MODE_RELIABLE);
		}
	} else {
		// we are just a client, so we request the creation (and the int id) from the server:
		_send_buffer->clear();
		_send_buffer->put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CreateSingleLine));
		_send_buffer->put_string(string_id);
		// the int id is 0 until the server created one for us
		_send_buffer->put_u16(0);
		send_packet_to_server(_send_buffer->get_data_array(), MultiplayerPeer::TRANSFER_MODE_RELIABLE);
	}

	return new_communication_line;
}

void CommunicationLineSystem::set_server_id(const int id) {
	GodotProfileFunction();
	_server_id = id;

	print_line("[", get_local_multiplayer_id(), "] Server ID set to: ", id);
	if (id != get_local_multiplayer_id()) {
		GodotProfileZone("connected_to_server_signal");
		emit_signal(SNAME("connected_to_server"));
	}
}

void CommunicationLineSystem::initialize_server() {
	GodotProfileFunction();
	set_server_id(get_local_multiplayer_id());

	_next_line_id = 1;

	for (auto cl : _communication_lines){
		cl->update_own_communication_state(CommunicationLine::CommunicationState::ConnectedOpen);
		cl->_int_id = _next_line_id;
		_next_line_id++;
	}
}

void CommunicationLineSystem::remove_all_communication_lines() {
	_communication_lines.clear();
}

void CommunicationLineSystem::remove_communication_line(const Ref<CommunicationLine> removed_cl) {
	_communication_lines.erase(removed_cl);
}

void CommunicationLineSystem::_poll_thread_func(void *p_self) {
	Thread::set_name("CommunicationLineSystem");
	CommunicationLineSystem *self = static_cast<CommunicationLineSystem *>(p_self);
	while (!self->_poll_thread_exit.load(std::memory_order_relaxed)) {
		self->_poll_iteration();
		// ~1ms granularity is far below network jitter; no need to spin.
		OS::get_singleton()->delay_usec(1000);
	}
}

void CommunicationLineSystem::_poll_iteration() {
	GodotProfileFunction();
	MutexLock lock(_peer_mutex);
	if (_multiplayer_peer.is_null()) {
		return;
	}

	MultiplayerPeer::ConnectionStatus status = _multiplayer_peer->get_connection_status();
	_cached_status.store(status);
	if (status == MultiplayerPeer::CONNECTION_DISCONNECTED) {
		return;
	}

	// ENet emits peer_connected/peer_disconnected from inside poll(), i.e. on
	// this thread; the trampolines turn them into queued events.
	_multiplayer_peer->poll();

	status = _multiplayer_peer->get_connection_status();
	_cached_status.store(status);
	if (status != MultiplayerPeer::CONNECTION_CONNECTED) {
		// We might be still connecting, or polling might have resulted in a disconnection.
		return;
	}

	while (_multiplayer_peer->get_available_packet_count() > 0) {
		int sender = _multiplayer_peer->get_packet_peer();
		int channel = _multiplayer_peer->get_packet_channel();

		const uint8_t *packet;
		int len;
		Error err = _multiplayer_peer->get_packet(&packet, len);
		if (err != OK) {
			ERR_PRINT(vformat("Error getting packet! %d", err));
			break;
		}

		// Any packet from a known peer is a sign of life for timeout detection.
		// Done here so peers don't look stale when the main thread stalls.
		{
			MutexLock peers_lock(_connected_peers_mutex);
			if (HashMap<int, PeerConnection>::Iterator it = _connected_peers.find(sender)) {
				it->value.last_packet_time = OS::get_singleton()->get_ticks_msec();
			}
		}

		// Ping and control packets are latency-sensitive and independent of game
		// state, so they are handled right here on the poll thread.
		if (channel == COMMUNICATION_LINE_CHANNEL_PING) {
			handle_ping_packet(sender, packet, len);
			continue;
		}
		if (channel == COMMUNICATION_LINE_CHANNEL_CONTROL && _handle_control_packet_threaded(sender, packet, len)) {
			continue;
		}

		NetworkEvent event;
		event.type = NetworkEvent::Type::Packet;
		event.peer_id = sender;
		event.channel = channel;
		event.data.resize(len);
		memcpy(event.data.ptrw(), packet, len);
		_push_network_event(std::move(event));
	}

	// Ping cadence runs here so RTT/jitter/loss stats stay live - and remote peers
	// don't sample phantom loss - while the main thread stalls.
	if (!_closing.load()) {
		uint64_t now = OS::get_singleton()->get_ticks_msec();
		if (now - _last_ping_send_time_ms >= PING_INTERVAL_MS) {
			_last_ping_send_time_ms = now;
			send_pings();
		}
	}
}

void CommunicationLineSystem::_start_poll_thread() {
	if (_poll_thread.is_started()) {
		return;
	}
	_last_ping_send_time_ms = OS::get_singleton()->get_ticks_msec(); // first ping after one full interval
	_poll_thread_exit.store(false);
	Thread::Settings settings;
	settings.priority = Thread::Priority::PRIORITY_HIGH;
	_poll_thread.start(_poll_thread_func, this, settings);
}

void CommunicationLineSystem::_stop_poll_thread() {
	if (!_poll_thread.is_started()) {
		return;
	}
	// Callers must not hold _peer_mutex here: the thread needs it to finish.
	_poll_thread_exit.store(true);
	_poll_thread.wait_to_finish();
}

void CommunicationLineSystem::_push_network_event(NetworkEvent &&p_event) {
	MutexLock lock(_event_queue_mutex);
	_event_queue.push_back(std::move(p_event));
}

void CommunicationLineSystem::_on_peer_connected_trampoline(int multiplayer_id) {
	// Runs on whichever thread the peer emits from; only touch the event queue.
	NetworkEvent event;
	event.type = NetworkEvent::Type::PeerConnected;
	event.peer_id = multiplayer_id;
	_push_network_event(std::move(event));
}

void CommunicationLineSystem::_on_peer_disconnected_trampoline(int multiplayer_id) {
	NetworkEvent event;
	event.type = NetworkEvent::Type::PeerDisconnected;
	event.peer_id = multiplayer_id;
	_push_network_event(std::move(event));
}

void CommunicationLineSystem::_drain_network_events() {
	GodotProfileFunction();
	if (_multiplayer_peer.is_null()) {
		return;
	}

	uint64_t time = OS::get_singleton()->get_ticks_msec();
	while (true) {
		if (_pending_index >= _pending_events.size()) {
			_pending_events.clear();
			_pending_index = 0;
			MutexLock lock(_event_queue_mutex);
			if (_event_queue.is_empty()) {
				break;
			}
			_pending_events = std::move(_event_queue); // move-assign resets _event_queue
		}

		// Move the event out: handlers can tear down the peer, which clears
		// _pending_events underneath us.
		NetworkEvent event = std::move(_pending_events[_pending_index]);
		_pending_index++;

		switch (event.type) {
			case NetworkEvent::Type::PeerConnected:
				on_new_peer_connected(event.peer_id);
				break;
			case NetworkEvent::Type::PeerDisconnected:
				on_peer_disconnected(event.peer_id);
				break;
			case NetworkEvent::Type::Packet: {
				if (last_connection_status != MultiplayerPeer::CONNECTION_CONNECTED) {
					break; // Packets are only processed while connected, as before.
				}
				_remote_sender_id = event.peer_id;

				// Ping packets never reach this queue (handled on the poll thread);
				// control packets land here only when the thread deferred them.
				if (event.channel == COMMUNICATION_LINE_CHANNEL_CONTROL) {
					handle_control_packet(event.peer_id, event.data.ptr(), event.data.size());
				} else {
					on_packet_received(event.peer_id, event.data);
				}

				_remote_sender_id = 0;
			} break;
		}

		update_status();
		if (_multiplayer_peer.is_null()) {
			// Processing the event tore the peer down; the queues are already cleared.
			return;
		}

		uint64_t now = OS::get_singleton()->get_ticks_msec();
		if (now - time > 100) {
			print_line("CommunicationLineSystem is taking long to process incoming packets! Continuing next frame.");
			break;
		}
	}
}

void CommunicationLineSystem::clear_multiplayer_peer() {
	// Stop the poll thread first; afterwards the peer is ours alone.
	// (Callers must not hold _peer_mutex, the thread needs it to finish.)
	_stop_poll_thread();

	last_connection_status = MultiplayerPeer::CONNECTION_DISCONNECTED;
	_cached_status.store(MultiplayerPeer::CONNECTION_DISCONNECTED);
	_cached_unique_id = 0;

	if (_multiplayer_peer.is_valid()) {
		const Callable on_connected = callable_mp(this, &CommunicationLineSystem::_on_peer_connected_trampoline);
		if (_multiplayer_peer->is_connected("peer_connected", on_connected)) {
			_multiplayer_peer->disconnect("peer_connected", on_connected);
		}
		const Callable on_disconnected = callable_mp(this, &CommunicationLineSystem::_on_peer_disconnected_trampoline);
		if (_multiplayer_peer->is_connected("peer_disconnected", on_disconnected)) {
			_multiplayer_peer->disconnect("peer_disconnected", on_disconnected);
		}
	}
	_multiplayer_peer = nullptr;

	// Drop events from the old peer so they cannot leak into the next session.
	{
		MutexLock lock(_event_queue_mutex);
		_event_queue.clear();
	}
	_pending_events.clear();
	_pending_index = 0;

	//We will still have communication lines and need to reset them
	for (auto cl : _communication_lines){
		cl->reset_communication_line();
		cl->update_own_communication_state(CommunicationLine::CommunicationState::WaitingForConnection);
	}

	{
		MutexLock lock(_connected_peers_mutex);
		_connected_peers.clear();
	}
	_last_ping_send_time_ms = 0;
	_closing = false;
}

void CommunicationLineSystem::update_status() {
	// The peer's live status is published by the poll thread; reading the cache
	// keeps this main-thread hot path lock-free.
	MultiplayerPeer::ConnectionStatus status = _multiplayer_peer.is_valid() ? static_cast<MultiplayerPeer::ConnectionStatus>(_cached_status.load()) : MultiplayerPeer::CONNECTION_DISCONNECTED;
	if (last_connection_status != status) {
		if (status == MultiplayerPeer::CONNECTION_DISCONNECTED) {
			if (last_connection_status == MultiplayerPeer::CONNECTION_CONNECTING) {
				emit_signal(SNAME("connection_failed"));
			} else {
				emit_signal(SNAME("server_disconnected"));
			}
			clear_multiplayer_peer();
		} else if (status == MultiplayerPeer::CONNECTION_CONNECTED) {
			// Refresh the cached id in case the peer only settled on it while connecting.
			MutexLock lock(_peer_mutex);
			if (_multiplayer_peer.is_valid()) {
				_cached_unique_id = _multiplayer_peer->get_unique_id();
			}
		}
		last_connection_status = status;
	}
}

void CommunicationLineSystem::process_packet(const int from, const uint8_t *packet, const int packet_len) {
	Vector<uint8_t> out;
	out.resize(packet_len);
	{
		uint8_t *w = out.ptrw();
		memcpy(&w[0], &packet[0], packet_len);
	}

	on_packet_received(from, out);
}

void CommunicationLineSystem::send_to_peer(const int to, const PackedByteArray &packet, const MultiplayerPeer::TransferMode mode) const {
	GodotProfileFunction();
	ERR_FAIL_COND_MSG(packet.is_empty(), "Trying to send an empty packet.");

	// The transfer channel/mode/target setters and put_packet form one stateful
	// sequence on the peer; hold the lock for the whole send so the poll thread
	// cannot interleave. Also covers ChunkSender::send_as_chunk below.
	MutexLock lock(_peer_mutex);
	ERR_FAIL_COND_MSG(_multiplayer_peer.is_null(), "Trying to send a raw packet while no multiplayer peer is active.");
	ERR_FAIL_COND_MSG(_multiplayer_peer->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED, "Trying to send a raw packet via a multiplayer peer which is not connected.");

	int channel = COMMUNICATION_LINE_CHANNEL_UNRELIABLE;

	if (mode == MultiplayerPeer::TRANSFER_MODE_RELIABLE) {
		channel = COMMUNICATION_LINE_CHANNEL_RELIABLE;
	}

	_multiplayer_peer->set_transfer_channel(channel);
	_multiplayer_peer->set_transfer_mode(mode);

	if (to > 0) {
		{
			MutexLock peers_lock(_connected_peers_mutex);
			ERR_FAIL_COND(!_connected_peers.has(to));
		}
		if (packet.size() > _multiplayer_peer->get_max_packet_size()) {
			if (mode == MultiplayerPeer::TRANSFER_MODE_RELIABLE){
				_chunk_sender->send_as_chunk(to, packet);
			} else {
				print_error(vformat("Sending chunks over %d bytes unreliable is not supported!", _multiplayer_peer->get_max_packet_size()));
			}
			return;
		}

		_multiplayer_peer->set_target_peer(to);
		_multiplayer_peer->put_packet(packet.ptr(), packet.size());
	} else {
		// Snapshot first: put_packet is a peer call and must not happen while
		// holding _connected_peers_mutex.
		Vector<int> targets;
		{
			MutexLock peers_lock(_connected_peers_mutex);
			for (const KeyValue<int, PeerConnection> &E : _connected_peers) {
				targets.push_back(E.key);
			}
		}
		for (const int pid : targets) {
			if (packet.size() > _multiplayer_peer->get_max_packet_size()) {
				if (mode == MultiplayerPeer::TRANSFER_MODE_RELIABLE){
					_chunk_sender->send_as_chunk(pid, packet);
				} else {
					print_error(vformat("Sending chunks over %d bytes unreliable is not supported!", _multiplayer_peer->get_max_packet_size()));
				}
				continue;
			}

			_multiplayer_peer->set_target_peer(pid);
			_multiplayer_peer->put_packet(packet.ptr(), packet.size());
		}
	}
}

Vector<int> CommunicationLineSystem::get_connected_peer_ids() const {
	if(_multiplayer_peer.is_null()) {
		return Vector<int>();
	}

	Vector<int> ret;
	MutexLock lock(_connected_peers_mutex);
	for (const KeyValue<int, PeerConnection> &E : _connected_peers) {
		ret.push_back(E.key);
	}

	return ret;
}

void CommunicationLineSystem::send_internal_packet(const int to, const PackedByteArray &packet, const MultiplayerPeer::TransferMode mode, const int channel) const {
	ERR_FAIL_COND_MSG(packet.is_empty(), "Trying to send an empty internal packet.");

	MutexLock lock(_peer_mutex);
	ERR_FAIL_COND_MSG(_multiplayer_peer.is_null(), "Trying to send an internal packet while no multiplayer peer is active.");
	ERR_FAIL_COND_MSG(_multiplayer_peer->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED, "Trying to send an internal packet via a multiplayer peer which is not connected.");
	// Ping/control packets are tiny by design, so chunking (as in send_to_peer) is never needed here.
	ERR_FAIL_COND_MSG(packet.size() > _multiplayer_peer->get_max_packet_size(), "Internal packet exceeds the maximum packet size.");

	_multiplayer_peer->set_transfer_channel(channel);
	_multiplayer_peer->set_transfer_mode(mode);
	_multiplayer_peer->set_target_peer(to);
	_multiplayer_peer->put_packet(packet.ptr(), packet.size());
}

void CommunicationLineSystem::send_pings() {
	GodotProfileFunction();
	const uint64_t now = OS::get_singleton()->get_ticks_msec();

	_thread_send_buffer->clear();
	_thread_send_buffer->put_u8(static_cast<uint8_t>(PingPacketType::Request));
	_thread_send_buffer->put_u32(static_cast<uint32_t>(now));
	const PackedByteArray request = _thread_send_buffer->get_data_array();

	// Update stats under the peers lock, but send only after releasing it:
	// sending is a peer call and must not happen while holding the leaf lock.
	Vector<int> ping_targets;
	{
		MutexLock peers_lock(_connected_peers_mutex);
		for (KeyValue<int, PeerConnection> &E : _connected_peers) {
			PeerConnection &peer = E.value;

			// Sample the previous ping for the packet-loss EWMA: if it is still
			// outstanding, it was very likely lost (an unreliable ping/pong round-trip
			// should comfortably fit within one PING_INTERVAL_MS).
			const float lost = peer.awaiting_pong ? 1.0f : 0.0f;
			peer.packet_loss += (lost - peer.packet_loss) * 0.1f;

			peer.awaiting_pong = true;
			peer.last_ping_sent_time = now;
			ping_targets.push_back(E.key);
		}
	}

	for (const int peer_id : ping_targets) {
		send_internal_packet(peer_id, request, MultiplayerPeer::TRANSFER_MODE_UNRELIABLE, COMMUNICATION_LINE_CHANNEL_PING);
	}
}

void CommunicationLineSystem::handle_ping_packet(const int from, const uint8_t *packet, const int packet_len) {
	GodotProfileFunction();
	_thread_receive_buffer->clear();
	{
		PackedByteArray packet_data;
		packet_data.resize(packet_len);
		memcpy(packet_data.ptrw(), packet, packet_len);
		_thread_receive_buffer->set_data_array(packet_data);
	}

	const PingPacketType ping_type = static_cast<PingPacketType>(_thread_receive_buffer->get_u8());
	switch (ping_type) {
		case PingPacketType::Request: {
			// Echo the requester's timestamp back unchanged and append our own clock
			// so the requester can estimate the round-trip time and our clock offset.
			// Answering here (instead of after a main-thread round trip) keeps the
			// measured RTT honest even when the main thread stalls.
			const uint32_t requester_ticks = _thread_receive_buffer->get_u32();
			_thread_send_buffer->clear();
			_thread_send_buffer->put_u8(static_cast<uint8_t>(PingPacketType::Response));
			_thread_send_buffer->put_u32(requester_ticks);
			_thread_send_buffer->put_u32(static_cast<uint32_t>(OS::get_singleton()->get_ticks_msec()));
			send_internal_packet(from, _thread_send_buffer->get_data_array(), MultiplayerPeer::TRANSFER_MODE_UNRELIABLE, COMMUNICATION_LINE_CHANNEL_PING);
		} break;
		case PingPacketType::Response: {
			MutexLock peers_lock(_connected_peers_mutex); // pure stats update, no peer calls below
			HashMap<int, PeerConnection>::Iterator it = _connected_peers.find(from);
			if (!it) {
				return; // peer disconnected in the meantime
			}
			PeerConnection &peer = it->value;

			const uint32_t requester_ticks = _thread_receive_buffer->get_u32();
			const uint32_t responder_ticks = _thread_receive_buffer->get_u32();
			const uint64_t now = OS::get_singleton()->get_ticks_msec();

			// Round-trip time. The send timestamp travels inside the packet, so this
			// stays correct even if responses arrive out of order.
			const uint32_t rtt = static_cast<uint32_t>(now) - requester_ticks;

			// Jitter: EWMA of how much this RTT deviates from the last measurement.
			int64_t deviation = static_cast<int64_t>(rtt) - static_cast<int64_t>(peer.ping_ms);
			if (deviation < 0) {
				deviation = -deviation;
			}
			peer.jitter_ms += (static_cast<float>(deviation) - peer.jitter_ms) * 0.125f;

			peer.ping_ms = rtt;

			// Estimate the remote clock at "now": its reply timestamp plus the time
			// the reply spent travelling back (~ half the round-trip).
			const int64_t est_remote_now = static_cast<int64_t>(responder_ticks) + static_cast<int64_t>(rtt) / 2;
			peer.clock_offset_ms = est_remote_now - static_cast<int64_t>(now);

			peer.awaiting_pong = false;
		} break;
	}
}

bool CommunicationLineSystem::_handle_control_packet_threaded(const int from, const uint8_t *packet, const int packet_len) {
	if (packet_len < 1) {
		return true; // malformed, drop
	}
	const ControlPacketType control_type = static_cast<ControlPacketType>(packet[0]);
	switch (control_type) {
		case ControlPacketType::Disconnect: {
			// The peer told us it is leaving; drop it now instead of waiting for the
			// timeout. disconnect_peer() makes the peer emit its own "peer_disconnected"
			// on a later poll -> queued event -> on_peer_disconnected() on the main thread.
			bool known_peer;
			{
				MutexLock peers_lock(_connected_peers_mutex);
				known_peer = _connected_peers.has(from);
			}
			if (known_peer) {
				_multiplayer_peer->disconnect_peer(from);
			}
			return true;
		}
	}
	// Unknown to the thread: queue it so handle_control_packet() runs on the
	// main thread. Future control types that need main-thread state go there.
	return false;
}

void CommunicationLineSystem::handle_control_packet(const int from, const uint8_t *packet, const int packet_len) {
	GodotProfileFunction();
	if (packet_len < 1) {
		return;
	}
	// Only control messages that _handle_control_packet_threaded() deferred end
	// up here; currently every known type is handled on the poll thread.
	print_error(vformat("Received unhandled control packet type %d from peer %d.", packet[0], from));
}

void CommunicationLineSystem::check_peer_timeouts() {
	const uint64_t now = OS::get_singleton()->get_ticks_msec();

	// Collect first: disconnect_peer() mutates _connected_peers via on_peer_disconnected().
	Vector<int> timed_out;
	{
		MutexLock lock(_connected_peers_mutex);
		for (const KeyValue<int, PeerConnection> &E : _connected_peers) {
			if (now - E.value.last_packet_time > PEER_TIMEOUT_MS) {
				timed_out.push_back(E.key);
			}
		}
	}

	for (const int peer_id : timed_out) {
		print_line(vformat("[%d] Peer %d timed out after %d ms without a packet.", get_local_multiplayer_id(), peer_id, static_cast<int>(PEER_TIMEOUT_MS)));
		emit_signal(SNAME("peer_timed_out"), peer_id);
		if (_multiplayer_peer.is_valid()) {
			disconnect_peer(peer_id);
		}
	}
}

void CommunicationLineSystem::disconnect_peer(const int peer_id) {
	MutexLock lock(_peer_mutex);
	ERR_FAIL_COND_MSG(_multiplayer_peer.is_null(), "Trying to disconnect a peer while no multiplayer peer is active.");
	_multiplayer_peer->disconnect_peer(peer_id);
}

int CommunicationLineSystem::get_peer_ping(const int peer_id) const {
	MutexLock lock(_connected_peers_mutex); // stats are written by the poll thread
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, -1, vformat("Peer %d is not connected.", peer_id));
	return static_cast<int>(it->value.ping_ms);
}

float CommunicationLineSystem::get_peer_jitter(const int peer_id) const {
	MutexLock lock(_connected_peers_mutex); // stats are written by the poll thread
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, -1.0f, vformat("Peer %d is not connected.", peer_id));
	return it->value.jitter_ms;
}

float CommunicationLineSystem::get_peer_packet_loss(const int peer_id) const {
	MutexLock lock(_connected_peers_mutex); // stats are written by the poll thread
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, -1.0f, vformat("Peer %d is not connected.", peer_id));
	return it->value.packet_loss;
}

int CommunicationLineSystem::get_peer_clock_offset(const int peer_id) const {
	MutexLock lock(_connected_peers_mutex); // stats are written by the poll thread
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, 0, vformat("Peer %d is not connected.", peer_id));
	return static_cast<int>(it->value.clock_offset_ms);
}

bool CommunicationLineSystem::close_connection() {
	GodotProfileFunction();
	if (_multiplayer_peer.is_null() || last_connection_status != MultiplayerPeer::CONNECTION_CONNECTED) {
		return false;
	}
	if (_closing) {
		return true; // already closing, return true to signal that the caller can wait for connection_closed
	}

	// Tell every peer we are leaving so they drop us immediately. Sent reliably so
	// it is not lost; the poll thread keeps servicing the peer for CLOSE_GRACE_MS
	// so it can flush.
	PackedByteArray disconnect_packet;
	disconnect_packet.push_back(static_cast<uint8_t>(ControlPacketType::Disconnect));
	// Snapshot first: sending is a peer call and must not happen while holding
	// _connected_peers_mutex.
	for (const int peer_id : get_connected_peer_ids()) {
		send_internal_packet(peer_id, disconnect_packet, MultiplayerPeer::TRANSFER_MODE_RELIABLE, COMMUNICATION_LINE_CHANNEL_CONTROL);
	}

	_closing = true;
	_close_requested_time = OS::get_singleton()->get_ticks_msec();

	return true;
}
