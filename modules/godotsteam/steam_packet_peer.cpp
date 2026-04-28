//================================================================================================//
// GodotSteam - steam_packet_peer.cpp
//================================================================================================//
//
// Copyright (c) 2017-Current | Chris Ridenour, Ryan Leverenz, GP Garcia, and Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//================================================================================================//


#include "steam_packet_peer.h"


SteamPacketPeer::SteamPacketPeer() {
	state = STATE_NONE;
	steam_id = 0;
	connection_handle = k_HSteamNetConnection_Invalid;
	peer_id = 0;
	last_packet = nullptr;
	configured_lanes = SteamProjectSettings::get_max_channels();
}

SteamPacketPeer::~SteamPacketPeer() {
	if (last_packet) {
		SteamAPI_SteamNetworkingMessage_t_Release(last_packet);
		last_packet = nullptr;
	}
	for (List<SteamNetworkingMessage_t *>::Element *E = packet_queue.front(); E; E = E->next()) {
		SteamAPI_SteamNetworkingMessage_t_Release(E->get());
	}
	packet_queue.clear();
}

void SteamPacketPeer::set_steam_id(uint64_t p_steam_id) {
	steam_id = p_steam_id;
}

uint64_t SteamPacketPeer::get_steam_id() const { return steam_id; }

void SteamPacketPeer::set_connection_handle(HSteamNetConnection p_handle) {
	connection_handle = p_handle;
	SteamAPI_ISteamNetworkingSockets_ConfigureConnectionLanes(
			SteamAPI_SteamNetworkingSockets_SteamAPI(), 
			connection_handle, configured_lanes, nullptr, nullptr
			);
}

HSteamNetConnection SteamPacketPeer::get_connection_handle() const {
	return connection_handle;
}

void SteamPacketPeer::set_peer_id(uint32_t p_peer_id) {
	peer_id = p_peer_id;
	SteamAPI_ISteamNetworkingSockets_SetConnectionUserData(
			SteamAPI_SteamNetworkingSockets_SteamAPI(),
			connection_handle, p_peer_id
			);
}

uint32_t SteamPacketPeer::get_peer_id() const { return peer_id; }

void SteamPacketPeer::set_state(PeerState p_state) { state = p_state; }

SteamPacketPeer::PeerState SteamPacketPeer::get_state() const { return state; }

Error SteamPacketPeer::send(int p_channel, const uint8_t *p_data, int p_size, int p_flags) {
	ERR_FAIL_COND_V(state != STATE_CONNECTED, ERR_UNAVAILABLE);

	ERR_FAIL_COND_V(p_data == nullptr || p_size <= 0, ERR_INVALID_PARAMETER);

	if (unlikely(p_channel >= (configured_lanes - 1))) {
		WARN_PRINT(vformat("Peer is only set up to use %d channels (0-%d). Please explicitly set"
				"the number of channels in your Project Settings. Defaulting to channel 0.",
				configured_lanes, configured_lanes - 1)
				);
		p_channel = 0;
	}

	SteamNetworkingMessage_t *packet = SteamAPI_ISteamNetworkingUtils_AllocateMessage(
			SteamAPI_SteamNetworkingUtils_SteamAPI(), p_size
			);
	ERR_FAIL_COND_V(!packet, ERR_CANT_CREATE);

	packet->m_conn = connection_handle;
	packet->m_nFlags = p_flags;
	packet->m_idxLane = (uint16)p_channel;
	memcpy(packet->m_pData, p_data, p_size);

	SteamNetworkingMessage_t *const messages[1] = {packet};
	SteamAPI_ISteamNetworkingSockets_SendMessages(
			SteamAPI_SteamNetworkingSockets_SteamAPI(), 1, messages, nullptr
			);

	return OK;
}

Error SteamPacketPeer::ping(uint32_t p_peer_id) {
	ERR_FAIL_COND_V(state != STATE_CONNECTED, ERR_UNAVAILABLE);

	PeerIDPacket data = PeerIDPacket();
	data.peer_id = p_peer_id;

	SteamNetworkingMessage_t *packet = SteamAPI_ISteamNetworkingUtils_AllocateMessage(
			SteamAPI_SteamNetworkingUtils_SteamAPI(), sizeof(PeerIDPacket)
			);
	ERR_FAIL_COND_V(!packet, ERR_CANT_CREATE);

	packet->m_conn = connection_handle;
	packet->m_nFlags = k_nSteamNetworkingSend_ReliableNoNagle;
	memcpy(packet->m_pData, (void *)&data, sizeof(PeerIDPacket));

	SteamNetworkingMessage_t *const messages[1] = {packet};
	SteamAPI_ISteamNetworkingSockets_SendMessages(
			SteamAPI_SteamNetworkingSockets_SteamAPI(), 1, messages, nullptr
			);

	return OK;
}

Error SteamPacketPeer::process_ping(SteamNetworkingMessage_t *p_packet) {
	ERR_FAIL_COND_V(state != STATE_CONNECTED && state != STATE_CONNECTING &&
			state != STATE_FINDING_ROUTE, ERR_UNAVAILABLE
			);
	ERR_FAIL_COND_V(p_packet->m_cbSize != sizeof(PeerIDPacket), ERR_INVALID_DATA);

	PeerIDPacket *data = (PeerIDPacket *)p_packet->m_pData;
	set_peer_id(data->peer_id);

	return OK;
}

Error SteamPacketPeer::get_packet(const uint8_t **r_buffer, int &r_buffer_size) {
	if (packet_queue.is_empty()) {
		return ERR_UNAVAILABLE;
	}

	if (last_packet != nullptr) {
		SteamAPI_SteamNetworkingMessage_t_Release(last_packet);
		last_packet = nullptr;
	}

	last_packet = packet_queue.front()->get();
	packet_queue.pop_front();

	*r_buffer = (uint8_t *)last_packet->GetData();
	r_buffer_size = last_packet->GetSize();

	return OK;
}

Error SteamPacketPeer::put_packet(const uint8_t *p_buffer, int p_buffer_size) {
	// Default channel and transfer mode for put_packet
	return send(0, p_buffer, p_buffer_size, k_nSteamNetworkingSend_Reliable);
}

int SteamPacketPeer::get_max_packet_size() const {
	// Steam networking sockets max message size
	return k_cbMaxSteamNetworkingSocketsMessageSizeSend; // 512 KB
}

int SteamPacketPeer::get_available_packet_count() const {
	return packet_queue.size();
}

void SteamPacketPeer::queue_packet(SteamNetworkingMessage_t *p_packet) {
	packet_queue.push_back(p_packet);
}

void SteamPacketPeer::disconnect_peer(bool p_force) {
	if (state == STATE_NONE || state == STATE_DEAD) {
		return;
	}

	SteamAPI_ISteamNetworkingSockets_CloseConnection(
			SteamAPI_SteamNetworkingSockets_SteamAPI(), connection_handle, 0,
			"Disconnect requested by application.", !p_force
			);

	if (p_force) {
		state = STATE_NONE;
	} else {
		state = STATE_LINGER;
	}
}

void SteamPacketPeer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_steam_id"),
			&SteamPacketPeer::get_steam_id);
	ClassDB::bind_method(D_METHOD("get_peer_id"),
			&SteamPacketPeer::get_peer_id);
	ClassDB::bind_method(D_METHOD("disconnect_peer", "force"),
			&SteamPacketPeer::disconnect_peer, DEFVAL(false));

	BIND_ENUM_CONSTANT(STATE_NONE);
	BIND_ENUM_CONSTANT(STATE_CONNECTING);
	BIND_ENUM_CONSTANT(STATE_FINDING_ROUTE);
	BIND_ENUM_CONSTANT(STATE_CONNECTED);
	BIND_ENUM_CONSTANT(STATE_CLOSED_BY_PEER);
	BIND_ENUM_CONSTANT(STATE_PROBLEM_DETECTED_LOCALLY);
	BIND_ENUM_CONSTANT(STATE_FIN_WAIT);
	BIND_ENUM_CONSTANT(STATE_LINGER);
	BIND_ENUM_CONSTANT(STATE_DEAD);
}
