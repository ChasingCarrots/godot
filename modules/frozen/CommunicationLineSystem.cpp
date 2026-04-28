#include "CommunicationLineSystem.h"

#include "CommunicationLine.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/profiling/profiling.h"

constexpr int COMMUNICATION_LINE_CHANNEL_RELIABLE = 1; //Documentation mentions that channel 0 works as 3 separate channels (https://docs.godotengine.org/en/stable/classes/class_multiplayerpeer.html#class-multiplayerpeer-property-transfer-channel)
constexpr int COMMUNICATION_LINE_CHANNEL_UNRELIABLE = 2;

CommunicationLineSystem* CommunicationLineSystem::_global_coms = nullptr;

CommunicationLineSystem::CommunicationLineSystem() {
}

void CommunicationLineSystem::_bind_methods() {
	ClassDB::bind_static_method("CommunicationLineSystem", D_METHOD("set_global_communication_line_system", "system"),
		&CommunicationLineSystem::set_global_communication_line_system);
	ClassDB::bind_static_method("CommunicationLineSystem", D_METHOD("get_global_communication_line_system"),
		&CommunicationLineSystem::get_global_communication_line_system);

	ClassDB::bind_method(D_METHOD("grab_communication_line", "string_id"), &CommunicationLineSystem::grab_communication_line);
	ClassDB::bind_method(D_METHOD("remove_communication_line", "communication_line"), &CommunicationLineSystem::remove_communication_line);
	ClassDB::bind_method(D_METHOD("remove_all_communication_line"), &CommunicationLineSystem::remove_all_communication_lines);
	ClassDB::bind_method(D_METHOD("get_number_of_communication_lines"), &CommunicationLineSystem::get_number_of_communication_lines);
	ClassDB::bind_method(D_METHOD("get_communication_line", "index"), &CommunicationLineSystem::get_communication_line);
	ClassDB::bind_method(D_METHOD("set_multiplayer_peer", "peer"), &CommunicationLineSystem::set_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("get_multiplayer_peer"), &CommunicationLineSystem::get_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("get_remote_sender_id"), &CommunicationLineSystem::get_remote_sender_id);
	ClassDB::bind_method(D_METHOD("get_connected_peer_ids"), &CommunicationLineSystem::get_connected_peer_ids);
	ClassDB::bind_method(D_METHOD("set_server_id", "id"), &CommunicationLineSystem::set_server_id);
	ClassDB::bind_method(D_METHOD("get_server_id"), &CommunicationLineSystem::get_server_id);
	ClassDB::bind_method(D_METHOD("is_server"), &CommunicationLineSystem::is_server);
	ClassDB::bind_method(D_METHOD("clear_multiplayer_peer"), &CommunicationLineSystem::clear_multiplayer_peer);
	ClassDB::bind_method(D_METHOD("initialize_server"), &CommunicationLineSystem::initialize_server);

	ADD_SIGNAL(MethodInfo("peer_connected", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("peer_disconnected", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("connected_to_server"));
	ADD_SIGNAL(MethodInfo("connection_failed"));
	ADD_SIGNAL(MethodInfo("server_disconnected"));
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
	_receive_buffer.instantiate();
	_send_buffer.instantiate();
	_chunk_sender.instantiate();
	_chunk_receiver.instantiate();
	_chunk_receiver->initialize(this);

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
}

void CommunicationLineSystem::set_multiplayer_peer(const Ref<MultiplayerPeer> &p_peer) {
	if (p_peer == _multiplayer_peer) {
		return; // Nothing to do
	}

	ERR_FAIL_COND_MSG(p_peer.is_valid() && p_peer->get_connection_status() == MultiplayerPeer::CONNECTION_DISCONNECTED,
			"Supplied MultiplayerPeer must be connecting or connected.");

	if (_multiplayer_peer.is_valid()) {
		_multiplayer_peer->disconnect("peer_connected", callable_mp(this, &CommunicationLineSystem::on_new_peer_connected));
		_multiplayer_peer->disconnect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::on_peer_disconnected));
		clear_multiplayer_peer();
	}

	_multiplayer_peer = p_peer;

	_chunk_sender->initialize(p_peer);

	//Our communication lines need the new peer id
	for (auto cl : _communication_lines){
		cl->_my_peer_info.set_multiplayer_id(_multiplayer_peer->get_unique_id());
	}

	if (_multiplayer_peer.is_valid()) {
		_multiplayer_peer->connect("peer_connected", callable_mp(this, &CommunicationLineSystem::on_new_peer_connected));
		_multiplayer_peer->connect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::on_peer_disconnected));
	}
	update_status();
}

void CommunicationLineSystem::on_new_peer_connected(int multiplayer_id) {
	_connected_peers.insert(multiplayer_id);

	if (!_communication_lines.is_empty() && is_server()) {
		// we send the new peer all the current communication lines, so they are up to speed!
		_send_buffer->clear();
		_send_buffer->put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CreateMultipleLines));
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

void CommunicationLineSystem::initialize_server() {
	GodotProfileFunction();
	set_server_id(get_local_multiplayer_id());

	_next_line_id = 0;

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
		const uint8_t *packet;
		int len;

		Error err = _multiplayer_peer->get_packet(&packet, len);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Error getting packet! %d", err));

		_remote_sender_id = sender;
		process_packet(sender, packet, len);
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
	_multiplayer_peer = nullptr;

	//We will still have communication lines and need to reset them
	for (auto cl : _communication_lines){
		cl->reset_communication_line();
		cl->update_own_communication_state(CommunicationLine::CommunicationState::WaitingForConnection);
	}

	_connected_peers.clear();
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
		for (const int &pid : _connected_peers) {
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
	for (const int &E : _connected_peers) {
		ret.push_back(E);
	}

	return ret;
}
