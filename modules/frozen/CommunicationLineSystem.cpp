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
	_chunk_sender.instantiate();
	_chunk_receiver.instantiate();
	_chunk_receiver->initialize(this);
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
			if (_multiplayer_peer.is_valid()) {
				_multiplayer_peer->disconnect("peer_connected", callable_mp(this, &CommunicationLineSystem::on_new_peer_connected));
				_multiplayer_peer->disconnect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::on_peer_disconnected));
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
		poll();
	}

	if (_chunk_receiver.is_valid()) {
		_chunk_receiver->process();
	}

	// poll() may have torn down the peer, so re-check before doing connection tracking.
	if (_multiplayer_peer.is_valid() && last_connection_status == MultiplayerPeer::CONNECTION_CONNECTED) {
		if (_closing) {
			// We are gracefully leaving: keep polling (done above) so the disconnect
			// packet flushes, then tear down once the grace period elapsed - or earlier
			// if every peer already acknowledged by dropping us.
			uint64_t now = OS::get_singleton()->get_ticks_msec();
			if (_connected_peers.is_empty() || now - _close_requested_time > CLOSE_GRACE_MS) {
				_multiplayer_peer->close();
				clear_multiplayer_peer();
				_closing = false;
				emit_signal(SNAME("connection_closed"));
			}
		} else {
			_time_since_last_ping += p_time;
			if (_time_since_last_ping * 1000.0 >= static_cast<double>(PING_INTERVAL_MS)) {
				_time_since_last_ping = 0.0;
				send_pings();
			}
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
		// clear_multiplayer_peer() disconnects the old peer's signals for us.
		clear_multiplayer_peer();
	}

	_multiplayer_peer = p_peer;

	_chunk_sender->initialize(p_peer);

	int peer_id = 0;
	if(p_peer.is_valid()) {
		peer_id = p_peer->get_unique_id();
	}

	//Our communication lines need the new peer id
	for (auto cl : _communication_lines){
		cl->_my_peer_info.set_multiplayer_id(peer_id);
	}

	if (_multiplayer_peer.is_valid()) {
		_multiplayer_peer->connect("peer_connected", callable_mp(this, &CommunicationLineSystem::on_new_peer_connected));
		_multiplayer_peer->connect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::on_peer_disconnected));
	}
	update_status();
}

void CommunicationLineSystem::on_new_peer_connected(int multiplayer_id) {
	PeerConnection peer_connection;
	peer_connection.last_packet_time = OS::get_singleton()->get_ticks_msec();
	_connected_peers.insert(multiplayer_id, peer_connection);

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
	_connected_peers.erase(multiplayer_id);

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
				new_communication_line->_my_peer_info.set_multiplayer_id(_multiplayer_peer->get_unique_id());
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
					new_communication_line->_my_peer_info.set_multiplayer_id(_multiplayer_peer->get_unique_id());
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

	new_communication_line->_my_peer_info.set_multiplayer_id(_multiplayer_peer->get_unique_id());

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

Error CommunicationLineSystem::poll() {
	GodotProfileFunction();
	if (_multiplayer_peer == nullptr){
		return OK;
	}

	update_status();
	if (last_connection_status == MultiplayerPeer::CONNECTION_DISCONNECTED) {
		return OK;
	}

	_multiplayer_peer->poll();

	update_status();
	if (last_connection_status != MultiplayerPeer::CONNECTION_CONNECTED) {
		// We might be still connecting, or polling might have resulted in a disconnection.
		return OK;
	}
	uint64_t time = OS::get_singleton()->get_ticks_msec();
	while (_multiplayer_peer->get_available_packet_count()) {
		int sender = _multiplayer_peer->get_packet_peer();
		int channel = _multiplayer_peer->get_packet_channel();
		const uint8_t *packet;
		int len;

		Error err = _multiplayer_peer->get_packet(&packet, len);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Error getting packet! %d", err));

		_remote_sender_id = sender;

		// Any packet from a known peer counts as a sign of life for timeout detection.
		if (HashMap<int, PeerConnection>::Iterator it = _connected_peers.find(sender)) {
			it->value.last_packet_time = OS::get_singleton()->get_ticks_msec();
		}

		if (channel == COMMUNICATION_LINE_CHANNEL_PING) {
			handle_ping_packet(sender, packet, len);
		} else if (channel == COMMUNICATION_LINE_CHANNEL_CONTROL) {
			handle_control_packet(sender, packet, len);
		} else {
			process_packet(sender, packet, len);
		}

		_remote_sender_id = 0;

		update_status();
		if (last_connection_status != MultiplayerPeer::CONNECTION_CONNECTED) { // It's possible that processing a packet might have resulted in a disconnection, so check here.
			return OK;
		}
		uint64_t now = OS::get_singleton()->get_ticks_msec();
		if (now-time > 100) {
			print_line("CommunicationLineSystem is taking long to process incoming packets! Continuing next frame.");
			break;
		}
	}

	update_status();
	if (last_connection_status != MultiplayerPeer::CONNECTION_CONNECTED) { // Signals might have triggered disconnection.
		return OK;
	}

	return OK;
}

void CommunicationLineSystem::clear_multiplayer_peer() {
	last_connection_status = MultiplayerPeer::CONNECTION_DISCONNECTED;

	if (_multiplayer_peer.is_valid()) {
		const Callable on_connected = callable_mp(this, &CommunicationLineSystem::on_new_peer_connected);
		if (_multiplayer_peer->is_connected("peer_connected", on_connected)) {
			_multiplayer_peer->disconnect("peer_connected", on_connected);
		}
		const Callable on_disconnected = callable_mp(this, &CommunicationLineSystem::on_peer_disconnected);
		if (_multiplayer_peer->is_connected("peer_disconnected", on_disconnected)) {
			_multiplayer_peer->disconnect("peer_disconnected", on_disconnected);
		}
	}
	_multiplayer_peer = nullptr;

	//We will still have communication lines and need to reset them
	for (auto cl : _communication_lines){
		cl->reset_communication_line();
		cl->update_own_communication_state(CommunicationLine::CommunicationState::WaitingForConnection);
	}

	_connected_peers.clear();
	_time_since_last_ping = 0.0;
	_closing = false;
}

void CommunicationLineSystem::update_status() {
	MultiplayerPeer::ConnectionStatus status = _multiplayer_peer.is_valid() ? _multiplayer_peer->get_connection_status() : MultiplayerPeer::CONNECTION_DISCONNECTED;
	if (last_connection_status != status) {
		if (status == MultiplayerPeer::CONNECTION_DISCONNECTED) {
			if (last_connection_status == MultiplayerPeer::CONNECTION_CONNECTING) {
				emit_signal(SNAME("connection_failed"));
			} else {
				emit_signal(SNAME("server_disconnected"));
			}
			clear_multiplayer_peer();
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
	ERR_FAIL_COND_MSG(_multiplayer_peer.is_null(), "Trying to send a raw packet while no multiplayer peer is active.");
	ERR_FAIL_COND_MSG(_multiplayer_peer->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED, "Trying to send a raw packet via a multiplayer peer which is not connected.");

	int channel = COMMUNICATION_LINE_CHANNEL_UNRELIABLE;

	if (mode == MultiplayerPeer::TRANSFER_MODE_RELIABLE) {
		channel = COMMUNICATION_LINE_CHANNEL_RELIABLE;
	}

	_multiplayer_peer->set_transfer_channel(channel);
	_multiplayer_peer->set_transfer_mode(mode);

	if (to > 0) {
		ERR_FAIL_COND(!_connected_peers.has(to));
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
		for (const KeyValue<int, PeerConnection> &E : _connected_peers) {
			const int pid = E.key;
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
	ERR_FAIL_COND_V_MSG(_multiplayer_peer.is_null(), Vector<int>(), "No multiplayer peer is assigned. Assume no peers are connected.");

	Vector<int> ret;
	for (const KeyValue<int, PeerConnection> &E : _connected_peers) {
		ret.push_back(E.key);
	}

	return ret;
}

void CommunicationLineSystem::send_internal_packet(const int to, const PackedByteArray &packet, const MultiplayerPeer::TransferMode mode, const int channel) const {
	ERR_FAIL_COND_MSG(packet.is_empty(), "Trying to send an empty internal packet.");
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

	_send_buffer->clear();
	_send_buffer->put_u8(static_cast<uint8_t>(PingPacketType::Request));
	_send_buffer->put_u32(static_cast<uint32_t>(now));
	const PackedByteArray request = _send_buffer->get_data_array();

	for (KeyValue<int, PeerConnection> &E : _connected_peers) {
		PeerConnection &peer = E.value;

		// Sample the previous ping for the packet-loss EWMA: if it is still
		// outstanding, it was very likely lost (an unreliable ping/pong round-trip
		// should comfortably fit within one PING_INTERVAL_MS).
		const float lost = peer.awaiting_pong ? 1.0f : 0.0f;
		peer.packet_loss += (lost - peer.packet_loss) * 0.1f;

		peer.awaiting_pong = true;
		peer.last_ping_sent_time = now;
		send_internal_packet(E.key, request, MultiplayerPeer::TRANSFER_MODE_UNRELIABLE, COMMUNICATION_LINE_CHANNEL_PING);
	}
}

void CommunicationLineSystem::handle_ping_packet(const int from, const uint8_t *packet, const int packet_len) {
	GodotProfileFunction();
	_receive_buffer->clear();
	{
		PackedByteArray data;
		data.resize(packet_len);
		memcpy(data.ptrw(), packet, packet_len);
		_receive_buffer->set_data_array(data);
	}

	const PingPacketType ping_type = static_cast<PingPacketType>(_receive_buffer->get_u8());
	switch (ping_type) {
		case PingPacketType::Request: {
			// Echo the requester's timestamp back unchanged and append our own clock
			// so the requester can estimate the round-trip time and our clock offset.
			const uint32_t requester_ticks = _receive_buffer->get_u32();
			_send_buffer->clear();
			_send_buffer->put_u8(static_cast<uint8_t>(PingPacketType::Response));
			_send_buffer->put_u32(requester_ticks);
			_send_buffer->put_u32(static_cast<uint32_t>(OS::get_singleton()->get_ticks_msec()));
			send_internal_packet(from, _send_buffer->get_data_array(), MultiplayerPeer::TRANSFER_MODE_UNRELIABLE, COMMUNICATION_LINE_CHANNEL_PING);
		} break;
		case PingPacketType::Response: {
			HashMap<int, PeerConnection>::Iterator it = _connected_peers.find(from);
			if (!it) {
				return; // peer disconnected in the meantime
			}
			PeerConnection &peer = it->value;

			const uint32_t requester_ticks = _receive_buffer->get_u32();
			const uint32_t responder_ticks = _receive_buffer->get_u32();
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

void CommunicationLineSystem::handle_control_packet(const int from, const uint8_t *packet, const int packet_len) {
	GodotProfileFunction();
	if (packet_len < 1) {
		return;
	}
	const ControlPacketType control_type = static_cast<ControlPacketType>(packet[0]);
	switch (control_type) {
		case ControlPacketType::Disconnect: {
			// The peer told us it is leaving the mesh. Drop it right away instead of
			// waiting for the timeout. disconnect_peer() triggers the multiplayer
			// peer's own "peer_disconnected" signal -> on_peer_disconnected() cleanup.
			if (_connected_peers.has(from)) {
				_multiplayer_peer->disconnect_peer(from);
			}
		} break;
	}
}

void CommunicationLineSystem::check_peer_timeouts() {
	const uint64_t now = OS::get_singleton()->get_ticks_msec();

	// Collect first: disconnect_peer() mutates _connected_peers via on_peer_disconnected().
	Vector<int> timed_out;
	for (const KeyValue<int, PeerConnection> &E : _connected_peers) {
		if (now - E.value.last_packet_time > PEER_TIMEOUT_MS) {
			timed_out.push_back(E.key);
		}
	}

	for (const int peer_id : timed_out) {
		print_line(vformat("[%d] Peer %d timed out after %d ms without a packet.", get_local_multiplayer_id(), peer_id, static_cast<int>(PEER_TIMEOUT_MS)));
		emit_signal(SNAME("peer_timed_out"), peer_id);
		if (_multiplayer_peer.is_valid()) {
			_multiplayer_peer->disconnect_peer(peer_id);
		}
	}
}

int CommunicationLineSystem::get_peer_ping(const int peer_id) const {
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, -1, vformat("Peer %d is not connected.", peer_id));
	return static_cast<int>(it->value.ping_ms);
}

float CommunicationLineSystem::get_peer_jitter(const int peer_id) const {
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, -1.0f, vformat("Peer %d is not connected.", peer_id));
	return it->value.jitter_ms;
}

float CommunicationLineSystem::get_peer_packet_loss(const int peer_id) const {
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, -1.0f, vformat("Peer %d is not connected.", peer_id));
	return it->value.packet_loss;
}

int CommunicationLineSystem::get_peer_clock_offset(const int peer_id) const {
	HashMap<int, PeerConnection>::ConstIterator it = _connected_peers.find(peer_id);
	ERR_FAIL_COND_V_MSG(!it, 0, vformat("Peer %d is not connected.", peer_id));
	return static_cast<int>(it->value.clock_offset_ms);
}

bool CommunicationLineSystem::close_connection() {
	GodotProfileFunction();
	if (_multiplayer_peer.is_null() || _multiplayer_peer->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED) {
		return false;
	}
	if (_closing) {
		return true; // already closing, return true to signal that the caller can wait for connection_closed
	}

	// Tell every peer we are leaving so they drop us immediately. Sent reliably so
	// it is not lost; _process() keeps polling for CLOSE_GRACE_MS so it can flush.
	PackedByteArray disconnect_packet;
	disconnect_packet.push_back(static_cast<uint8_t>(ControlPacketType::Disconnect));
	for (const KeyValue<int, PeerConnection> &E : _connected_peers) {
		send_internal_packet(E.key, disconnect_packet, MultiplayerPeer::TRANSFER_MODE_RELIABLE, COMMUNICATION_LINE_CHANNEL_CONTROL);
	}

	_closing = true;
	_close_requested_time = OS::get_singleton()->get_ticks_msec();

	return true;
}
