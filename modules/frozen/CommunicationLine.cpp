#include "CommunicationLine.h"

#include "CommunicationLineSystem.h"

#include <functional>

void CommunicationCallWithAnswer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_number_of_answers"), &CommunicationCallWithAnswer::get_number_of_answers);
	ClassDB::bind_method(D_METHOD("get_peer_id", "index"), &CommunicationCallWithAnswer::get_peer_id);
	ClassDB::bind_method(D_METHOD("get_answer", "index"), &CommunicationCallWithAnswer::get_answer);
	ClassDB::bind_method(D_METHOD("get_time_to_answer", "index"), &CommunicationCallWithAnswer::get_time_to_answer);

	ADD_SIGNAL(MethodInfo("AnswerReceived"));
}

void CommunicationLine::_bind_methods() {
	BIND_ENUM_CONSTANT(NotConnected);
	BIND_ENUM_CONSTANT(WaitingForConnection);
	BIND_ENUM_CONSTANT(ConnectedClosed);
	BIND_ENUM_CONSTANT(ConnectedOpen);

	BIND_ENUM_CONSTANT(None);
	BIND_ENUM_CONSTANT(U8);
	BIND_ENUM_CONSTANT(U16);
	BIND_ENUM_CONSTANT(U32);
	BIND_ENUM_CONSTANT(U64);
	BIND_ENUM_CONSTANT(S8);
	BIND_ENUM_CONSTANT(S16);
	BIND_ENUM_CONSTANT(S32);
	BIND_ENUM_CONSTANT(S64);
	BIND_ENUM_CONSTANT(HalfFloat);
	BIND_ENUM_CONSTANT(Float);
	BIND_ENUM_CONSTANT(Double);
	BIND_ENUM_CONSTANT(Vector2Type);
	BIND_ENUM_CONSTANT(Vector3Type);
	BIND_ENUM_CONSTANT(Bytes);
	BIND_ENUM_CONSTANT(StringType);
	BIND_ENUM_CONSTANT(VariantType);

	ClassDB::bind_method(D_METHOD("finish_initialization_and_open_line"), &CommunicationLine::finish_initialization_and_open_line);
	ClassDB::bind_method(D_METHOD("get_peer_state", "multiplayer_peer_id"), &CommunicationLine::get_peer_state);
	ClassDB::bind_method(D_METHOD("get_local_peer_state"), &CommunicationLine::get_local_peer_state);
	ClassDB::bind_method(D_METHOD("get_string_id"), &CommunicationLine::get_string_id);
	ClassDB::bind_method(D_METHOD("get_int_id"), &CommunicationLine::get_int_id);
	ClassDB::bind_method(D_METHOD("get_local_peer_bits"), &CommunicationLine::get_local_peer_bits);
	ClassDB::bind_method(D_METHOD("check_local_peer_bits", "bits_set", "bits_not_set"), &CommunicationLine::check_local_peer_bits, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_local_peer_bits", "set_bits"), &CommunicationLine::set_local_peer_bits);
	ClassDB::bind_method(D_METHOD("unset_local_peer_bits", "unset_bits"), &CommunicationLine::unset_local_peer_bits);
	ClassDB::bind_method(D_METHOD("get_peer_bits", "multiplayer_peer_id"), &CommunicationLine::get_peer_bits);
	ClassDB::bind_method(D_METHOD("add_function_definition", "function_name", "callable", "parameter_types", "answer_parameter_type", "transfer_mode"), &CommunicationLine::add_function_definition);
	ClassDB::bind_method(D_METHOD("call_function_on_peers", "function_name", "parameters", "only_on_peers_with_bits_set", "only_on_peers_with_bits_unset"), &CommunicationLine::call_function_on_peers, DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("call_function_on_peer", "function_name", "parameters", "peer_id"), &CommunicationLine::call_function_on_peer);
	ClassDB::bind_method(D_METHOD("call_function_on_peers_expect_answer", "function_name", "parameters", "only_on_peers_with_bits_set", "only_on_peers_with_bits_unset"), &CommunicationLine::call_function_on_peers_expect_answer, DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("call_function_on_peer_expect_answer", "function_name", "parameters", "peer_id"), &CommunicationLine::call_function_on_peer_expect_answer);

	ADD_SIGNAL(MethodInfo("PeerCommunicationStateChanged", PropertyInfo(Variant::INT, "peer_multiplayer_id"), PropertyInfo(Variant::INT, "new_state")));
	ADD_SIGNAL(MethodInfo("CommunicationStateChanged", PropertyInfo(Variant::INT, "new_state")));
	ADD_SIGNAL(MethodInfo("PeerBitsChanged", PropertyInfo(Variant::INT, "peer_multiplayer_id"), PropertyInfo(Variant::INT, "new_bits"), PropertyInfo(Variant::INT, "old_bits")));
}

void CommunicationLine::create_unconnected_peers(const Vector<int> &peers) {
	for (int peer_id : peers) {
		if (peer_id == _my_peer_info.get_multiplayer_id()) {
			continue;
		}
		bool already_there = false;
		for (auto &peer : _other_peers) {
			if (peer.get_multiplayer_id() == peer_id) {
				// we already have that peer
				already_there = true;
				break;
			}
		}
		if (!already_there) {
			CommunicationLinePeer new_peer;
			new_peer.set_multiplayer_id(peer_id);
			_other_peers.push_back(new_peer);
		}
	}
}

void CommunicationLine::new_peer_connected(int peer_id) {
	for (auto &peer : _other_peers) {
		if (peer.get_multiplayer_id() == peer_id) {
			// we already have that peer
			return;
		}
	}
	CommunicationLinePeer new_peer;
	new_peer.set_multiplayer_id(peer_id);
	_other_peers.push_back(new_peer);
	// also update the peer of our state and bits
	_send_buffer.clear();
	_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::UpdateLinePeerStateAndBits));
	_send_buffer.put_u16(_int_id);
	_send_buffer.put_u8(static_cast<uint8_t>(_my_peer_info.get_communication_state()));
	_send_buffer.put_u8(_my_peer_info.get_peer_bits());
	_communication_line_system->send_packet_to_peer(_send_buffer.get_data_array(), peer_id, MultiplayerPeer::TRANSFER_MODE_RELIABLE);
}

void CommunicationLine::update_own_communication_state(CommunicationState state) {
	if (_my_peer_info.get_communication_state() == state) {
		return;
	}
	_my_peer_info.set_communication_state(state);
	if (!_other_peers.is_empty()) {
		_send_buffer.clear();
		_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::UpdateLinePeerState));
		_send_buffer.put_u16(_int_id);
		_send_buffer.put_u8(static_cast<uint8_t>(state));
		for (auto &peer : _other_peers) {
			_communication_line_system->send_packet_to_peer(_send_buffer.get_data_array(), peer.get_multiplayer_id(), MultiplayerPeer::TRANSFER_MODE_RELIABLE);
		}
	}
	emit_signal("CommunicationStateChanged", state);
}

Variant get_value_from_buffer(StreamPeerBuffer * packet, CommunicationLine::ParamType parameter_type) {
	switch (parameter_type) {
		case CommunicationLine::None: break;
		case CommunicationLine::U8: return packet->get_u8();
		case CommunicationLine::U16: return packet->get_u16();
		case CommunicationLine::U32: return packet->get_u32();
		case CommunicationLine::U64: return packet->get_u64();
		case CommunicationLine::S8: return packet->get_8();
		case CommunicationLine::S16: return packet->get_16();
		case CommunicationLine::S32: return packet->get_32();
		case CommunicationLine::S64: return packet->get_64();
		case CommunicationLine::HalfFloat: return packet->get_half();
		case CommunicationLine::Float: return packet->get_float();
		case CommunicationLine::Double: return packet->get_double();
		case CommunicationLine::Vector2Type: {
			Vector2 v2;
			v2.x = packet->get_float();
			v2.y = packet->get_float();
			return v2;
		}
		case CommunicationLine::Vector3Type: {
			Vector3 v3;
			v3.x = packet->get_float();
			v3.y = packet->get_float();
			v3.z = packet->get_float();
			return v3;
		}
		case CommunicationLine::Bytes: {
			PackedByteArray ba;
			int length = packet->get_u16();
			ba.resize(length);
			for (int i=0; i<length; i++) {
				ba.set(i, packet->get_u8());
			}
			return ba;
		}
		case CommunicationLine::StringType: return packet->get_string();
		case CommunicationLine::VariantType: return packet->get_var();
	}
	return {};
}
void CommunicationLine::on_packet_received(CommunicationLinePacketTypes packet_type, int from_multiplayer_id, Ref<StreamPeerBuffer> &packet) {
	switch (packet_type) {
		case CommunicationLinePacketTypes::UpdateLinePeerState: {
			CommunicationState state = static_cast<CommunicationState>(packet->get_u8());
			for (auto &peer : _other_peers) {
				if (peer.get_multiplayer_id() == from_multiplayer_id) {
					if (peer.set_communication_state(state)) {
						if (state == ConnectedOpen) {
							// the connection to this peer is newly established, lets send them our own state
							_send_buffer.clear();
							_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::UpdateLinePeerStateAndBits));
							_send_buffer.put_u16(_int_id);
							_send_buffer.put_u8(static_cast<uint8_t>(_my_peer_info.get_communication_state()));
							_send_buffer.put_u8(_my_peer_info.get_peer_bits());
							_communication_line_system->send_packet_to_peer(_send_buffer.get_data_array(), from_multiplayer_id, MultiplayerPeer::TRANSFER_MODE_RELIABLE);
						}
						emit_signal("PeerCommunicationStateChanged", from_multiplayer_id, state);
					}
					return;
				}
			}
			// we don't even have this peer in our list, yet!
			CommunicationLinePeer new_peer;
			new_peer.set_multiplayer_id(from_multiplayer_id);
			new_peer.set_communication_state(state);
			_other_peers.push_back(new_peer);
			// also update the peer of our state
			_send_buffer.clear();
			_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::UpdateLinePeerStateAndBits));
			_send_buffer.put_u16(_int_id);
			_send_buffer.put_u8(static_cast<uint8_t>(_my_peer_info.get_communication_state()));
			_send_buffer.put_u8(_my_peer_info.get_peer_bits());
			_communication_line_system->send_packet_to_peer(_send_buffer.get_data_array(), from_multiplayer_id, MultiplayerPeer::TRANSFER_MODE_RELIABLE);
		}
		break;
		case CommunicationLinePacketTypes::UpdateLinePeerBits: {
			uint8_t bits = packet->get_u8();
			for (auto &peer : _other_peers) {
				if (peer.get_multiplayer_id() == from_multiplayer_id) {
					uint8_t bits_before = peer.get_peer_bits();
					if (peer.reset_peer_bits(bits)) {
						emit_signal("PeerBitsChanged", from_multiplayer_id, bits, bits_before);
					}
					return;
				}
			}
		}
		break;
		case CommunicationLinePacketTypes::UpdateLinePeerStateAndBits: {
			CommunicationState state = static_cast<CommunicationState>(packet->get_u8());
			uint8_t bits = packet->get_u8();
			for (auto &peer : _other_peers) {
				if (peer.get_multiplayer_id() == from_multiplayer_id) {
					if (peer.set_communication_state(state)) {
						emit_signal("PeerCommunicationStateChanged", from_multiplayer_id, state);
					}
					uint8_t bits_before = peer.get_peer_bits();
					if (peer.reset_peer_bits(bits)) {
						emit_signal("PeerBitsChanged", from_multiplayer_id, bits, bits_before);
					}
					return;
				}
			}
		}
		break;
		case CommunicationLinePacketTypes::CallRemoteFunction:
		case CommunicationLinePacketTypes::CallRemoteFunctionExpectAnswer: {
			int function_index = packet->get_u8();
			// TODO: check if index is valid
			const CommunicationFunction& function = _communication_functions[function_index];
			Vector<Variant> parameters;
			// we always start with the sender id. that is also to ensure that the gdscript functions
			// do not get called locally (at least without willingly putting in a fake id)
			parameters.append(from_multiplayer_id);
			for (int param_i=0; param_i < function.Parameters.size(); param_i++) {
				CommunicationLine::ParamType parameter_type = function.Parameters[param_i];
				if (parameter_type == None) {
					continue;
				}
				parameters.push_back(get_value_from_buffer(packet.ptr(), parameter_type));
			}
			// the way callp expects the parameters is weird: it wants an array of pointers to variants
			const Variant *args[10];
			for (int param_i=0; param_i < parameters.size(); param_i++) {
				args[param_i] = &parameters.ptr()[param_i];
			}
			Variant return_value;
			Callable::CallError call_error;
			function.FunctionCallable.callp(args, parameters.size(), return_value, call_error);
			if (call_error.error != Callable::CallError::CALL_OK) {
				print_error(vformat("CommunicationLine::CallRemoteFunction %s call_error: %s", function.Name, Variant::get_callable_error_text(function.FunctionCallable, args, parameters.size(), call_error)));
			}
			else if (packet_type == CommunicationLinePacketTypes::CallRemoteFunctionExpectAnswer) {
				uint8_t call_id = packet->get_u8();
				_send_buffer.clear();
				_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::AnswerToFunctionCall));
				_send_buffer.put_u16(_int_id);
				_send_buffer.put_u8(call_id);
				fill_send_buffer_with_value(function.ExpectedAnswer, return_value);
				_communication_line_system->send_packet_to_peer(_send_buffer.get_data_array(), from_multiplayer_id, MultiplayerPeer::TRANSFER_MODE_RELIABLE);
			}
		}
		break;
		case CommunicationLinePacketTypes::AnswerToFunctionCall: {
			uint8_t call_id = packet->get_u8();
			int call_index = 0;
			for (auto& call : _communication_calls_waiting_for_answer) {
				if (call->FunctionCallNumber != call_id) {
					++call_index;
					continue;
				}
				const CommunicationFunction& function = _communication_functions[call->FunctionIndex];
				Variant return_value = get_value_from_buffer(packet.ptr(), function.ExpectedAnswer);
				bool all_peers_answered = true;
				for (auto& peer_answer : call->PeerAnswers) {
					if (peer_answer.MultiplayerID == from_multiplayer_id) {
						peer_answer.Answer = return_value;
						peer_answer.TimeToAnswer = OS::get_singleton()->get_ticks_msec() - call->TicksAtSendTime;
					}
					else if (peer_answer.TimeToAnswer == -1) {
						all_peers_answered = false;
					}
				}
				if (all_peers_answered) {
					call->emit_signal("AnswerReceived");
					if (call->AnswerReceivedCallback) {
						call->AnswerReceivedCallback(call.ptr());
					}
					_communication_calls_waiting_for_answer.remove_at(call_index);
				}
				return;
			}
			print_error("CommunicationLine::AnswerToFunctionCall: Answer with the sent ID not found!");
		}
		break;
		default: // this is just so that there is no compiler warning...
		break;
	}
}

void CommunicationLine::finish_initialization_and_open_line() {
	switch (_my_peer_info.get_communication_state()) {
		case NotConnected:
			if (_communication_line_system->is_server()) {
				// we as the server can directly open the connection
				update_own_communication_state(ConnectedOpen);
			} else {
				update_own_communication_state(WaitingForConnection);
			}
			break;
		case ConnectedClosed:
			update_own_communication_state(ConnectedOpen);
			break;
	}
}

void CommunicationLine::add_function_definition(const StringName &function_name, Callable callable, Array parameter_types, CommunicationLine::ParamType expected_answer, MultiplayerPeer::TransferMode mode) {
	// TODO: check if function already exists
	CommunicationFunction new_function;
	new_function.Name = function_name;
	new_function.FunctionCallable = callable;
	for (auto &parameter : parameter_types) {
		if (parameter.is_num()) {
			new_function.Parameters.append(static_cast<ParamType>((int)parameter));
		}
	}
	new_function.ExpectedAnswer = expected_answer;
	new_function.Mode = mode;
	_communication_functions.push_back(new_function);
}

void CommunicationLine::fill_send_buffer_with_value(CommunicationLine::ParamType value_type, const Variant &value) {
	switch (value_type) {
		case U8:
			_send_buffer.put_u8(value);
			break;
		case U16:
			_send_buffer.put_u16(value);
			break;
		case U32:
			_send_buffer.put_u32(value);
			break;
		case U64:
			_send_buffer.put_u64(value);
			break;
		case S8:
			_send_buffer.put_8(value);
			break;
		case S16:
			_send_buffer.put_16(value);
			break;
		case S32:
			_send_buffer.put_32(value);
			break;
		case S64:
			_send_buffer.put_64(value);
			break;
		case HalfFloat:
			_send_buffer.put_half(value);
			break;
		case Float:
			_send_buffer.put_float(value);
			break;
		case Double:
			_send_buffer.put_double(value);
			break;
		case Vector2Type: {
			Vector2 v2 = value;
			_send_buffer.put_float(v2.x);
			_send_buffer.put_float(v2.y);
		} break;
		case Vector3Type: {
			Vector3 v3 = value;
			_send_buffer.put_float(v3.x);
			_send_buffer.put_float(v3.y);
			_send_buffer.put_float(v3.z);
		} break;
		case Bytes: {
			// TODO: check if the size is too large for u16!
			PackedByteArray ba = value;
			_send_buffer.put_u16(static_cast<uint16_t>(ba.size()));
			_send_buffer.put_data(ba.ptr(), ba.size());
		} break;
		case StringType:
			_send_buffer.put_string(value);
			break;
		case VariantType:
			_send_buffer.put_var(value);
			break;
		default:
			break;
	}
}

int CommunicationLine::fill_send_buffer_with_function_parameters(const StringName &function_name, const Array &parameters) {
	for (int function_i = 0; function_i < _communication_functions.size(); function_i++) {
		const CommunicationFunction &function = _communication_functions[function_i];
		if (function.Name != function_name) {
			continue;
		}
		if (parameters.size() != function.Parameters.size()) {
			print_error(vformat("Error calling function %s: function definition has %i parameters, but is called with %i parameters.", function_name, function.Parameters.size(), parameters.size()));
			return false;
		}
		_send_buffer.put_u8(function_i);
		for (int param_i = 0; param_i < function.Parameters.size(); param_i++) {
			ParamType parameter_type = function.Parameters[param_i];
			if (parameter_type == None) {
				continue;
			}
			Variant parameter = parameters[param_i];
			// TODO: should we check for the right Variant type here?
			fill_send_buffer_with_value(parameter_type, parameter);
		}
		return function_i;
	}
	return -1;
}

void CommunicationLine::call_function_on_peers(const StringName &function_name, Array parameters, uint8_t only_on_peers_with_bits_set, uint8_t only_on_peers_with_bits_unset) {
	bool has_qualifying_peer = false;
	for (const auto &peer : _other_peers) {
		if (peer.get_communication_state() == ConnectedOpen && peer.check_peer_bits(only_on_peers_with_bits_set, only_on_peers_with_bits_unset)) {
			has_qualifying_peer = true;
			break;
		}
	}
	if (!has_qualifying_peer) {
		// nobody is there to send the function call to!
		return;
	}

	_send_buffer.clear();
	_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CallRemoteFunction));
	_send_buffer.put_u16(_int_id);
	if (int function_index = fill_send_buffer_with_function_parameters(function_name, parameters); function_index != -1) {
		const auto& function = _communication_functions[function_index];
		Vector<uint8_t> bytes = _send_buffer.get_data_array();
		for (auto &peer : _other_peers) {
			if (peer.get_communication_state() != ConnectedOpen || !peer.check_peer_bits(only_on_peers_with_bits_set, only_on_peers_with_bits_unset)) {
				continue;
			}
			_communication_line_system->send_packet_to_peer(bytes, peer.get_multiplayer_id(), function.Mode);
		}
	} else {
		print_error(vformat("CommunicationLine::call_function_on_peers: failed to send function %s", function_name));
	}
}

void CommunicationLine::call_function_on_peer(const StringName &function_name, Array parameters, int peer_id) {
	// with the direct peer sending function, we don't check for the correct state,
	// so that this function can be used while initializing and in a whole queue of
	// messages.
	bool peer_found = false;
	for (const auto &peer : _other_peers) {
		if (peer.get_multiplayer_id() == peer_id) {
			peer_found = true;
			break;
		}
	}
	if (!peer_found) {
		return;
	}

	_send_buffer.clear();
	_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CallRemoteFunction));
	_send_buffer.put_u16(_int_id);
	if (int function_index = fill_send_buffer_with_function_parameters(function_name, parameters); function_index != -1) {
		const auto& function = _communication_functions[function_index];
		Vector<uint8_t> bytes = _send_buffer.get_data_array();
		_communication_line_system->send_packet_to_peer(bytes, peer_id, function.Mode);
	} else {
		print_error(vformat("CommunicationLine::call_function_on_peers: failed to send function %s", function_name));
	}
}

Ref<CommunicationCallWithAnswer> CommunicationLine::call_function_on_peer_expect_answer(const StringName &function_name, Array parameters, int peer_id) {
	bool peer_found = false;
	for (const auto &peer : _other_peers) {
		if (peer.get_multiplayer_id() == peer_id) {
			peer_found = true;
			break;
		}
	}
	if (!peer_found) {
		return {};
	}

	_send_buffer.clear();
	_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CallRemoteFunctionExpectAnswer));
	_send_buffer.put_u16(_int_id);
	if (int function_index = fill_send_buffer_with_function_parameters(function_name, parameters); function_index != -1) {
		const auto& function = _communication_functions[function_index];
		uint8_t call_id = _next_call_id++;
		_send_buffer.put_u8(call_id);
		Ref<CommunicationCallWithAnswer> call_with_answer;
		call_with_answer.instantiate();
		call_with_answer->FunctionIndex = function_index;
		call_with_answer->FunctionCallNumber = call_id;
		call_with_answer->TicksAtSendTime = OS::get_singleton()->get_ticks_msec();
		Vector<uint8_t> bytes = _send_buffer.get_data_array();
		_communication_line_system->send_packet_to_peer(bytes, peer_id, function.Mode);
		// add an answer object for every peer we sent the call to, so that we can
		// trigger the signal when every peer has sent their answer.
		call_with_answer->PeerAnswers.push_back({peer_id, {}, -1});
		_communication_calls_waiting_for_answer.append(call_with_answer);
		return call_with_answer;
	}
	else {
		print_error(vformat("CommunicationLine::call_function_on_peers: failed to send function %s", function_name));
	}
	return {};
}

Ref<CommunicationCallWithAnswer> CommunicationLine::call_function_on_peers_expect_answer(const StringName &function_name, Array parameters, uint8_t only_on_peers_with_bits_set, uint8_t only_on_peers_with_bits_unset) {
	bool has_qualifying_peer = false;
	for (const auto &peer : _other_peers) {
		if (peer.get_communication_state() == ConnectedOpen && peer.check_peer_bits(only_on_peers_with_bits_set, only_on_peers_with_bits_unset)) {
			has_qualifying_peer = true;
			break;
		}
	}
	if (!has_qualifying_peer) {
		// nobody is there to send the function call to!
		return {};
	}

	_send_buffer.clear();
	_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::CallRemoteFunctionExpectAnswer));
	_send_buffer.put_u16(_int_id);
	if (int function_index = fill_send_buffer_with_function_parameters(function_name, parameters); function_index != -1) {
		const auto& function = _communication_functions[function_index];
		uint8_t call_id = _next_call_id++;
		_send_buffer.put_u8(call_id);
		Ref<CommunicationCallWithAnswer> call_with_answer;
		call_with_answer.instantiate();
		call_with_answer->FunctionIndex = function_index;
		call_with_answer->FunctionCallNumber = call_id;
		call_with_answer->TicksAtSendTime = OS::get_singleton()->get_ticks_msec();
		Vector<uint8_t> bytes = _send_buffer.get_data_array();
		for (auto &peer : _other_peers) {
			if (peer.get_communication_state() != ConnectedOpen || !peer.check_peer_bits(only_on_peers_with_bits_set, only_on_peers_with_bits_unset)) {
				continue;
			}
			_communication_line_system->send_packet_to_peer(bytes, peer.get_multiplayer_id(), function.Mode);
			// add an answer object for every peer we sent the call to, so that we can
			// trigger the signal when every peer has sent their answer.
			call_with_answer->PeerAnswers.push_back({peer.get_multiplayer_id(), {}, -1});
		}
		_communication_calls_waiting_for_answer.append(call_with_answer);
		return call_with_answer;
	}
	else {
		print_error(vformat("CommunicationLine::call_function_on_peers: failed to send function %s", function_name));
	}
	return {};
}

CommunicationLine::CommunicationState CommunicationLine::get_peer_state(int peer_id) const {
	for (auto &peer : _other_peers) {
		if (peer.get_multiplayer_id() == peer_id) {
			return static_cast<CommunicationState>(peer.get_communication_state());
		}
	}
	return NotConnected;
}

inline uint8_t CommunicationLine::get_peer_bits(int peer_id) const {
	for (auto &peer : _other_peers) {
		if (peer.get_multiplayer_id() == peer_id) {
			return peer.get_peer_bits();
		}
	}
	return 0;
}

inline void CommunicationLine::set_local_peer_bits(uint8_t bit) {
	if (_my_peer_info.set_peer_bits(bit)) {
		_send_buffer.clear();
		_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::UpdateLinePeerBits));
		_send_buffer.put_u16(_int_id);
		_send_buffer.put_u8(static_cast<uint8_t>(_my_peer_info.get_peer_bits()));
		for (auto &peer : _other_peers) {
			_communication_line_system->send_packet_to_peer(_send_buffer.get_data_array(), peer.get_multiplayer_id(), MultiplayerPeer::TRANSFER_MODE_RELIABLE);
		}
	}
}

inline void CommunicationLine::unset_local_peer_bits(uint8_t bit) {
	if (_my_peer_info.unset_peer_bits(bit)) {
		_send_buffer.clear();
		_send_buffer.put_u8(static_cast<uint8_t>(CommunicationLinePacketTypes::UpdateLinePeerBits));
		_send_buffer.put_u16(_int_id);
		_send_buffer.put_u8(static_cast<uint8_t>(_my_peer_info.get_peer_bits()));
		for (auto &peer : _other_peers) {
			_communication_line_system->send_packet_to_peer(_send_buffer.get_data_array(), peer.get_multiplayer_id(), MultiplayerPeer::TRANSFER_MODE_RELIABLE);
		}
	}
}
