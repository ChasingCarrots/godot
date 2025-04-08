#include "CommunicationLineSystem.h"

#include <core/io/marshalls.h>

const int COMMUNICATION_LINE_CHANNEL = 7;

CommunicationLineSystem* CommunicationLineSystem::_global_coms = nullptr;

void CommunicationLineSystem::_bind_methods() {
	ClassDB::bind_static_method("CommunicationLineSystem", D_METHOD("set_global_communication_line_system", "system"),
		&CommunicationLineSystem::set_global_communication_line_system);
	ClassDB::bind_static_method("CommunicationLineSystem", D_METHOD("get_global_communication_line_system"),
		&CommunicationLineSystem::get_global_communication_line_system);
	ClassDB::bind_method(D_METHOD("grab_communication_line", "string_id"), &CommunicationLineSystem::grab_communication_line);
	ClassDB::bind_method(D_METHOD("get_number_of_communication_lines"), &CommunicationLineSystem::get_number_of_communication_lines);
	ClassDB::bind_method(D_METHOD("get_communication_line", "index"), &CommunicationLineSystem::get_communication_line);
}

void CommunicationLineSystem::_notification(int p_notification) {
	switch(p_notification) {
		case NOTIFICATION_ENTER_TREE:

		break;
		case NOTIFICATION_READY:
			_ready();
		break;
		case NOTIFICATION_EXIT_TREE:
			if (_scene_multiplayer.is_valid()) {
				_scene_multiplayer->disconnect("peer_packet", callable_mp(this, &CommunicationLineSystem::on_packet_received));
				_scene_multiplayer->disconnect("peer_connected", callable_mp(this, &CommunicationLineSystem::on_new_peer_connected));
				_scene_multiplayer->disconnect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::on_peer_disconnected));
			}
		break;
	}
}

void CommunicationLineSystem::_ready() {
	_receive_buffer.instantiate();
	_send_buffer.instantiate();
	_scene_multiplayer = get_multiplayer();
	_scene_multiplayer->connect("peer_packet", callable_mp(this, &CommunicationLineSystem::on_packet_received));
	_scene_multiplayer->connect("peer_connected", callable_mp(this, &CommunicationLineSystem::on_new_peer_connected));
	_scene_multiplayer->connect("peer_disconnected", callable_mp(this, &CommunicationLineSystem::on_peer_disconnected));
}

void CommunicationLineSystem::on_new_peer_connected(int multiplayer_id) {
	if (!_communication_lines.is_empty() && _scene_multiplayer->is_server()) {
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
}

void CommunicationLineSystem::on_peer_disconnected(int multiplayer_id) {
	for (auto &line : _communication_lines) {
		line->peer_disconnected(multiplayer_id);
	}
}

void CommunicationLineSystem::on_packet_received(int from_multiplayer_id, const PackedByteArray &packet) {
	_receive_buffer->clear();
	_receive_buffer->set_data_array(packet);
	CommunicationLinePacketTypes packet_type = static_cast<CommunicationLinePacketTypes>(_receive_buffer->get_u8());
	switch (packet_type) {
		case CommunicationLinePacketTypes::CreateSingleLine: {
			StringName string_id = _receive_buffer->get_string();
			uint16_t int_id = _receive_buffer->get_u16();
			if (_scene_multiplayer->is_server()) {
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
				Vector<int> peer_ids = _scene_multiplayer->get_peer_ids();
				// we don't have this line, so we'll just create it.
				Ref<CommunicationLine> new_communication_line;
				new_communication_line.instantiate();
				new_communication_line->_communication_line_system = this;
				new_communication_line->_my_peer_info.set_multiplayer_id(_scene_multiplayer->get_unique_id());
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
					Vector<int> peer_ids = _scene_multiplayer->get_peer_ids();
					// we don't have this line, so we'll just create it.
					Ref<CommunicationLine> new_communication_line;
					new_communication_line.instantiate();
					new_communication_line->_communication_line_system = this;
					new_communication_line->_my_peer_info.set_multiplayer_id(_scene_multiplayer->get_unique_id());
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

void CommunicationLineSystem::send_packet_to_peer(const PackedByteArray &bytes, int to_peer_id, MultiplayerPeer::TransferMode mode) const {
	_scene_multiplayer->send_bytes(bytes, to_peer_id, mode, COMMUNICATION_LINE_CHANNEL);
}

void CommunicationLineSystem::send_packet_to_server(const PackedByteArray &bytes, MultiplayerPeer::TransferMode mode) const {
	if (_scene_multiplayer->is_server()) {
		print_error("send_packet_to_server: we ARE the server! Handle this case directly...");
		return;
	}
	_scene_multiplayer->send_bytes(bytes, 1, mode, COMMUNICATION_LINE_CHANNEL);
}

Ref<CommunicationLine> CommunicationLineSystem::grab_communication_line(const StringName &string_id) {
	for (auto& line : _communication_lines) {
		if (line->_string_id == string_id) {
			return line;
		}
	}
	Vector<int> peer_ids = _scene_multiplayer->get_peer_ids();
	// we don't have this line, so we'll just create it.
	Ref<CommunicationLine> new_communication_line;
	new_communication_line.instantiate();
	new_communication_line->_communication_line_system = this;
	new_communication_line->_my_peer_info.set_multiplayer_id(_scene_multiplayer->get_unique_id());
	new_communication_line->_string_id = string_id;
	new_communication_line->create_unconnected_peers(peer_ids);
	_communication_lines.append(new_communication_line);

	if (_scene_multiplayer->is_server()) {
		// we are the server, so we can directly set the int id
		new_communication_line->_int_id = _next_line_id++;
		// and send the creation info to everybody else
		_send_buffer->clear();
		_send_buffer->put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CreateSingleLine));
		_send_buffer->put_string(string_id);
		_send_buffer->put_u16(new_communication_line->_int_id);
		for (int to_id : peer_ids) {
			if (to_id == 1) { continue; }
			send_packet_to_peer(_send_buffer->get_data_array(), to_id, MultiplayerPeer::TRANSFER_MODE_RELIABLE);
		}
	}
	else {
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