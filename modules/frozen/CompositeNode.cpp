#include "CompositeNode.h"

#include "CommunicationLineSystem.h"

#include <core/io/marshalls.h>

const char* DataSynchronizationModeStr[] = {
	"OnChange",
	"LowFrequency",
	"HighFrequency"
};
const char* DataSynchronizationTypeStr[] = {
	"None",
	"U8",
	"U16",
	"U32",
	"U64",
	"S8",
	"S16",
	"S32",
	"S64",
	"HalfFloat",
	"Float",
	"Double",
	"Vector2Type",
	"Vector3Type"
};

float CompositeNode::GameTimeServerOffset = 0;
Vector<CompositeNode*> CompositeNode::_all_composite_nodes;

String CompositeNode::get_data_value_debug_string(StringName name) const {
	String t;
	const DataValue *di = _data.lookup_ptr(name);
	if (!di) {
		return t;
	}

	t = vformat("%s = %s\nVariant type: %s\n", name, di->Value, Variant::get_type_name(di->Value.get_type()));

	const CompositeNode::DataSynchronizationSettings *sync = _sync_data_on_change.lookup_ptr(name);
	if (sync) {
		t += vformat("Multiplayer SyncOnChange (%s)\n", DataSynchronizationTypeStr[sync->SyncType]);
	}

	for (const auto &lf : _sync_data_low_freq) {
		if (lf.DataName == name) {
			t += vformat("Multiplayer SyncLowFreq (%s)\n", DataSynchronizationTypeStr[lf.SyncType]);
		}
	}

	for (const auto &hf : _sync_data_high_freq) {
		if (hf.DataName == name) {
			t += vformat("Multiplayer SyncHighFreq (%s)\n", DataSynchronizationTypeStr[hf.SyncType]);
		}
	}

	const DataSumSettings* sum = _sums.lookup_ptr(name);
	if (sum) {
		t += "Sum Definition:\n  ";
		bool first = true;
		for (const StringName &sum_component_valuename : sum->SumComponentsDataNames) {
			if (!first) {
				t += " + ";
			}
			first = false;
			t += sum_component_valuename;
		}
		t += "\n";
	}

	if (!di->DataUpdatedCallbacks.is_empty()) {
		t += "DataUpdatedCallbacks:\n";
		for (const Callable &cb : di->DataUpdatedCallbacks) {
			t += vformat("  %s.%s\n", ((String)cb.get_object()->get_class_name()).ascii().ptr(), ((String)cb.get_method()).ascii().ptr());
		}
	}

	return t;
}

String CompositeNode::get_callback_debug_string(StringName name) const {
	String t;
	const Vector<Callable> *callbacks = _callbacks.lookup_ptr(name);
	if (!callbacks) {
		return t;
	}
	t = "Registered Callbacks:\n";
	for (const auto& cb : *callbacks) {
		t += vformat("  %s.%s\n", ((String)cb.get_object()->get_class_name()).ascii().ptr(), ((String)cb.get_method()).ascii().ptr());
	}
	return t;
}

String CompositeNode::get_function_debug_string(StringName name) const {
	String t;
	const Callable* cb = _functions.lookup_ptr(name);
	if (!cb) {
		return t;
	}
	t = "Function Callback:\n";
	t += vformat("  %s.%s\n", ((String)cb->get_object()->get_class_name()).ascii().ptr(), ((String)cb->get_method()).ascii().ptr());
	return t;
}

void CompositeNode::_bind_methods() {
	BIND_ENUM_CONSTANT(OnChange);
	BIND_ENUM_CONSTANT(LowFrequency);
	BIND_ENUM_CONSTANT(HighFrequency);

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


	ClassDB::bind_method(D_METHOD("set_ParentCompositeNode", "parentCompositeNode"),
		&CompositeNode::set_parent_composite_node);
	ClassDB::bind_method(D_METHOD("get_ParentCompositeNode"),
		&CompositeNode::get_parent_composite_node);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "ParentCompositeNode", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "CompositeNode"),
		"set_ParentCompositeNode", "get_ParentCompositeNode");

	ClassDB::bind_method(D_METHOD("set_ForwardDataToParentCompositeNode", "forwardData"),
		&CompositeNode::set_forward_data_to_parent_composite_node);
	ClassDB::bind_method(D_METHOD("get_ForwardDataToParentCompositeNode"),
		&CompositeNode::get_forward_callbacks_to_parent_composite_node);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ForwardDataToParentCompositeNode", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
		"set_ForwardDataToParentCompositeNode", "get_ForwardDataToParentCompositeNode");

	ClassDB::bind_method(D_METHOD("set_ForwardCallbacksToParentCompositeNode", "forwardCallbacks"),
		&CompositeNode::set_forward_callbacks_to_parent_composite_node);
	ClassDB::bind_method(D_METHOD("get_ForwardCallbacksToParentCompositeNode"),
		&CompositeNode::get_forward_callbacks_to_parent_composite_node);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ForwardCallbacksToParentCompositeNode", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
		"set_ForwardCallbacksToParentCompositeNode", "get_ForwardCallbacksToParentCompositeNode");

	ClassDB::bind_static_method("CompositeNode", D_METHOD("SetGameTimeServerOffset", "offset"),
	    &CompositeNode::SetGameTimeServerOffset);
	ClassDB::bind_static_method("CompositeNode", D_METHOD("GetCompositeNodeInParents", "node"),
	    &CompositeNode::GetCompositeNodeInParents);
	ClassDB::bind_static_method("CompositeNode", D_METHOD("GetNumberOfExistingCompositeNodes"),
	    &CompositeNode::GetNumberOfExistingCompositeNodes);
	ClassDB::bind_static_method("CompositeNode", D_METHOD("GetExistingCompositeNode", "index"),
		&CompositeNode::GetExistingCompositeNode);
	ClassDB::bind_method(D_METHOD("InitializeAsAuthority"),
	    &CompositeNode::InitializeAsAuthority);
	ClassDB::bind_method(D_METHOD("GetCommunicationLine"),
	    &CompositeNode::GetCommunicationLine);
	ClassDB::bind_method(D_METHOD("SynchronizeAllToSingleClient", "client_multiplayer_id"),
	    &CompositeNode::SynchronizeAllToSingleClient);
	ClassDB::bind_method(D_METHOD("SetupDataMultiplayerSynchronization", "dataName", "syncMode", "dataType"),
	    &CompositeNode::SetupDataMultiplayerSynchronization);
	ClassDB::bind_method(D_METHOD("SetupDataMultiplayerSynchronizationWithLinearMovement", "dataName", "velocityDataName", "syncMode", "dataType"),
	    &CompositeNode::SetupDataMultiplayerSynchronizationWithLinearMovement);
	ClassDB::bind_method(D_METHOD("RegisterFunction", "functionName", "callable"),
	    &CompositeNode::RegisterFunction);
	ClassDB::bind_method(D_METHOD("UnregisterFunction", "functionName", "callable"),
	    &CompositeNode::UnregisterFunction);
	ClassDB::bind_method(D_METHOD("CallFunction", "functionName", "parameters"),
	    &CompositeNode::CallFunction);
	ClassDB::bind_method(D_METHOD("CallFunctionOnAuthority", "functionName", "parameters"),
	    &CompositeNode::CallFunctionOnAuthority);
	ClassDB::bind_method(D_METHOD("RegisterCallback", "callbackName", "callable"),
	    &CompositeNode::RegisterCallback);
	ClassDB::bind_method(D_METHOD("UnregisterCallback", "callbackName", "callable"),
	    &CompositeNode::UnregisterCallback);
	ClassDB::bind_method(D_METHOD("CallCallbacks", "callbackName", "parameters"),
	    &CompositeNode::CallCallbacks);
	ClassDB::bind_method(D_METHOD("HasData", "dataName"),
	    &CompositeNode::HasData);
	ClassDB::bind_method(D_METHOD("SetData", "dataName", "value", "skipCallbacks", "skipMultiplayerPeer", "increment"),
	    &CompositeNode::SetData, DEFVAL(false), DEFVAL(-1), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("SetDataOnAuthority", "dataName", "value"),
	    &CompositeNode::SetDataOnAuthority);
	ClassDB::bind_method(D_METHOD("RegisterDataUpdatedCallback", "dataName", "callable", "callIfDataExists"),
	    &CompositeNode::RegisterDataUpdatedCallback, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("UnregisterDataUpdatedCallback", "dataName", "callable"),
	    &CompositeNode::UnregisterDataUpdatedCallback);
	ClassDB::bind_method(D_METHOD("GetData", "dataName"),
	    &CompositeNode::GetData);
	ClassDB::bind_method(D_METHOD("AddDataToSumDefinition", "sumDataName", "componentDataName", "initialValue"),
	    &CompositeNode::AddDataToSumDefinition);
	ClassDB::bind_method(D_METHOD("CreateSynchronizedArray", "variableName"),
		&CompositeNode::CreateSynchronizedArray);

	ClassDB::bind_method(D_METHOD("get_data_value_debug_string", "dataName"),
		&CompositeNode::get_data_value_debug_string);
	ClassDB::bind_method(D_METHOD("get_callback_debug_string", "callbackName"),
		&CompositeNode::get_callback_debug_string);
	ClassDB::bind_method(D_METHOD("get_function_debug_string", "functionName"),
		&CompositeNode::get_function_debug_string);
	ClassDB::bind_method(D_METHOD("get_data_value_names"),
		&CompositeNode::get_data_value_names);
	ClassDB::bind_method(D_METHOD("get_function_names"),
		&CompositeNode::get_function_names);
	ClassDB::bind_method(D_METHOD("get_callback_names"),
		&CompositeNode::get_callback_names);

	ClassDB::bind_method(D_METHOD("_sendLowFrequencyData"),
		&CompositeNode::_sendLowFrequencyData);
	ClassDB::bind_method(D_METHOD("_sendHighFrequencyData"),
		&CompositeNode::_sendHighFrequencyData);
}

void CompositeNode::_notification(int p_notification) {
	switch(p_notification) {
		case NOTIFICATION_ENTER_TREE:
			if (!Engine::get_singleton()->is_editor_hint()) {
				_enter_tree();
			}
			break;
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint()) {
				_ready();
			}
		break;
		case NOTIFICATION_PROCESS:
			if (!Engine::get_singleton()->is_editor_hint()) {
				_process(get_process_delta_time());
			}
		break;
		case NOTIFICATION_EXIT_TREE:
			if (!Engine::get_singleton()->is_editor_hint()) {
				_exit_tree();
			}
		break;
	}
}

void CompositeNode::update_sum_component(DataSumSettings &sum_settings, StringName component_name, float new_value) {
	float sum = 0;
	for (int i=0; i < sum_settings.SumComponentsDataNames.size(); i++) {
		if (sum_settings.SumComponentsDataNames[i] != component_name) {
			sum += sum_settings.SumComponentsValues[i];
		}
		else {
			sum_settings.SumComponentsValues.set(i, new_value);
			sum += new_value;
		}
	}
	SetData(sum_settings.SumDataName, sum);
}

void CompositeNode::_enter_tree() {
	_all_composite_nodes.append(this);
}

void CompositeNode::_exit_tree() {
	_all_composite_nodes.erase(this);
}

void CompositeNode::_ready() {
	if (!ParentCompositeNode.is_empty()) {
		_parentCompositeNode.set(cast_to<CompositeNode>(get_node(ParentCompositeNode)));
	}

	// this will initialize the line, if it hasn't already been initalized through a child
	GetCommunicationLine();
	_communication_line->finish_initialization_and_open_line();
	_communication_line->connect("PeerCommunicationStateChanged", callable_mp(this, &CompositeNode::_peer_state_changed));

	// only process when initialized via init_authority as the authority
	set_process(false);

	if (_parentCompositeNode.is_valid()) {
		_parentCompositeNode->RegisterCallback("init_authority", callable_mp(this, &CompositeNode::init_authority));
		_parentCompositeNode->RegisterCallback("SynchronizeAllToSingleClient", callable_mp(this, &CompositeNode::SynchronizeAllToSingleClient));
	}
}

void CompositeNode::_process(float delta) {
	float game_time = GameTime();
	if (game_time >= _next_high_freq_gametime) {
		while (game_time >= _next_high_freq_gametime) {
			_next_high_freq_gametime += HIGH_FREQUENCY_TIMING;
		}
		if (is_multiplayer_authority() && !_sync_data_high_freq.is_empty()) {
			// send the high frequency data at the end of the frame
			call_deferred("_sendHighFrequencyData");
		}
	}

	if (game_time >= _next_low_freq_gametime) {
		while (game_time >= _next_low_freq_gametime) {
			_next_low_freq_gametime += LOW_FREQUENCY_TIMING;
		}
		if (is_multiplayer_authority() && !_sync_data_low_freq.is_empty()) {
			// send the low frequency data at the end of the frame
			call_deferred("_sendLowFrequencyData");
		}
	}

}

void CompositeNode::init_authority(int authority_player_id) {
	init_authority_rpc(0, authority_player_id);
}

void CompositeNode::init_authority_rpc(int sender_id, int authority_player_id) {
    // with the communication_line system we really don't need the
    // authority set to a specific id (will be propagated by the peer bits)
    // but there are a lot of places that check the authority via "is_multiplayer_authority"
    // and so we'll set it for the time being.
	set_multiplayer_authority(authority_player_id);
	if ( is_multiplayer_authority() ) {
		set_process(true);
		// 1 is the bit we defined in the godot project for "authority"
		// (E.CommunicationLineBits.Authority)
		GetCommunicationLine()->set_local_peer_bits(1);
	}
	Array params;
	params.append(authority_player_id);
	CallCallbacks("init_authority", params);
}

void CompositeNode::_peer_state_changed(int peer_id, CommunicationLine::CommunicationState new_state) {
	if (new_state == CommunicationLine::ConnectedOpen &&
	_communication_line->check_local_peer_bits(1, 0)) {

		int authority_id = get_multiplayer_authority();

		Array params;
		params.append(authority_id);
		_communication_line->call_function_on_peer("init_authority_rpc", params, peer_id);

		SynchronizeAllToSingleClient(peer_id);
	}

}

void CompositeNode::_complete_sync_package(int sender_id, const PackedByteArray &package, int size_uncompressed) {
	PackedByteArray dataPackage;
	Compression::Mode mode = Compression::MODE_FASTLZ;

	int64_t buffer_size = size_uncompressed;
	dataPackage.resize(buffer_size);
	int result = Compression::decompress(dataPackage.ptrw(), buffer_size, package.ptr(), package.size(), mode);
	result = result >= 0 ? result : 0;
	dataPackage.resize(result);

	_temp_stringnames.clear();

	float delta_since_sent = GameTime() - decode_float(dataPackage.ptr());
	int offset = 4;

	for (const auto& on_change_data_key : _sync_data_on_change_sorting) {
		bool skipCallbacks = (delta_since_sent > 0) && _linear_movement_with_velocity.has(on_change_data_key);
		auto on_change_config = _sync_data_on_change.lookup_ptr(on_change_data_key);
		DataSynchronizationType data_type = on_change_config->SyncType;
		offset += _setDataFromPackage(on_change_data_key, dataPackage, offset, data_type, skipCallbacks);
	}

	for (const auto &sync_setting : _sync_data_high_freq) {
		bool skipCallbacks = (delta_since_sent > 0) && _linear_movement_with_velocity.has(sync_setting.DataName);
		offset += _setDataFromPackage(sync_setting.DataName, dataPackage, offset, sync_setting.SyncType, skipCallbacks);
		if (skipCallbacks) {
			_temp_stringnames.append(sync_setting.DataName);
		}
	}

	for (const auto &sync_setting : _sync_data_low_freq) {
		bool skipCallbacks = (delta_since_sent > 0) && _linear_movement_with_velocity.has(sync_setting.DataName);
		offset += _setDataFromPackage(sync_setting.DataName, dataPackage, offset, sync_setting.SyncType, skipCallbacks);
		if (skipCallbacks) {
			_temp_stringnames.append(sync_setting.DataName);
		}
	}

	if (delta_since_sent > 0) {
		for (const auto &dataWithLinearMovement : _temp_stringnames) {
			const auto* velocity_data_name = _linear_movement_with_velocity.lookup_ptr(dataWithLinearMovement);
			_updateDataWithLinearMovement(dataWithLinearMovement, *velocity_data_name, delta_since_sent);
		}
	}

}

void CompositeNode::_sendHighFrequencyData() {
	_send_buffer.clear();
	int offset = 0;

	offset += _encodeDataToPackage(GameTime(), _send_buffer, offset, DataSynchronizationType::Float);

	for (const auto &sync_setting : _sync_data_high_freq) {
		offset += _encodeDataToPackage(_data.lookup_ptr(sync_setting.DataName)->Value, _send_buffer, offset, sync_setting.SyncType);
	}

	Array params;
	params.append(_send_buffer);
	GetCommunicationLine()->call_function_on_peers("_updateHighFrequencyData", params);

}

void CompositeNode::_sendLowFrequencyData() {
	_send_buffer.clear();
	int offset = 0;

	offset += _encodeDataToPackage(GameTime(), _send_buffer, offset, DataSynchronizationType::Float);

	for (const auto &sync_setting : _sync_data_low_freq) {
		offset += _encodeDataToPackage(_data.lookup_ptr(sync_setting.DataName)->Value, _send_buffer, offset, sync_setting.SyncType);
	}

	Array params;
	params.append(_send_buffer);
	GetCommunicationLine()->call_function_on_peers("_updateLowFrequencyData", params);
}

void CompositeNode::_sendOnChangeData(const DataSynchronizationSettings &sync_setting, Variant value, int skip_multiplayer_peer) {
	_send_buffer.clear();
	_send_buffer.append(sync_setting.DataID);

	int offset = 1;

	if (_linear_movement_with_velocity.has(sync_setting.DataName)) {
		_send_buffer.resize(5);
		encode_float(GameTime(), _send_buffer.ptrw()+offset);
		offset += 4;
	}

	_encodeDataToPackage(value, _send_buffer, offset, sync_setting.SyncType);

	Array params;
	params.append(_send_buffer);
	if (skip_multiplayer_peer == -1) {
		GetCommunicationLine()->call_function_on_peers("_updateSingleOnChangeData", params);
	} else {
		int local_peer_id = get_multiplayer()->get_unique_id();
		for (int peer : get_multiplayer()->get_peer_ids()) {
			if (peer == skip_multiplayer_peer || peer == local_peer_id) {
				continue;
			}
			GetCommunicationLine()->call_function_on_peer("_updateSingleOnChangeData", params, peer);
		}
	}
}

void CompositeNode::_updateHighFrequencyData(int sender_id, const PackedByteArray &dataPackage) {
	_temp_stringnames.clear();

	float delta_since_sent = GameTime() - decode_float(dataPackage.ptr());
	int offset = 4;

	for (const auto &sync_setting : _sync_data_high_freq) {
		bool skipCallbacks = delta_since_sent > 0 && _linear_movement_with_velocity.has(sync_setting.DataName);
		offset += _setDataFromPackage(sync_setting.DataName, dataPackage, offset, sync_setting.SyncType, skipCallbacks);
		if (skipCallbacks) {
			_temp_stringnames.append(sync_setting.DataName);
		}
	}

	if (delta_since_sent > 0) {
		for (const StringName &dataWithLinearMovement : _temp_stringnames) {
			_updateDataWithLinearMovement(dataWithLinearMovement, *_linear_movement_with_velocity.lookup_ptr(dataWithLinearMovement), delta_since_sent);
		}
	}
}

void CompositeNode::_updateLowFrequencyData(int sender_id, const PackedByteArray &dataPackage) {
	_temp_stringnames.clear();

	float delta_since_sent = GameTime() - decode_float(dataPackage.ptr());
	int offset = 4;

	for (const auto &sync_setting : _sync_data_low_freq) {
		bool skipCallbacks = delta_since_sent > 0 && _linear_movement_with_velocity.has(sync_setting.DataName);
		offset += _setDataFromPackage(sync_setting.DataName, dataPackage, offset, sync_setting.SyncType, skipCallbacks);
		if (skipCallbacks) {
			_temp_stringnames.append(sync_setting.DataName);
		}
	}

	if (delta_since_sent > 0) {
		for (const StringName &dataWithLinearMovement : _temp_stringnames) {
			_updateDataWithLinearMovement(dataWithLinearMovement, *_linear_movement_with_velocity.lookup_ptr(dataWithLinearMovement), delta_since_sent);
		}
	}
}

void CompositeNode::_updateSingleOnChangeData(int sender_id, const PackedByteArray &dataPackage) {
	uint8_t data_id = dataPackage[0];
	String data_name = _sync_data_on_change_sorting[data_id];
	int offset = 1;

	bool has_linear_movement = _linear_movement_with_velocity.has(data_name);
	float delta = 0.0f;

	if (has_linear_movement) {
		delta = GameTime() - decode_float(dataPackage.ptr() + 1);
		offset += 4;
	}

	DataSynchronizationType data_type = _sync_data_on_change.lookup_ptr(data_name)->SyncType;
	_setDataFromPackage(data_name, dataPackage, offset, data_type, has_linear_movement && delta > 0);

	if (has_linear_movement && delta > 0) {
		_updateDataWithLinearMovement(data_name, *_linear_movement_with_velocity.lookup_ptr(data_name), delta);
	}

}

int CompositeNode::_setDataFromPackage(StringName data_name, const PackedByteArray &dataPackage, int offset, DataSynchronizationType data_type, bool skipCallbacks) {
	switch (data_type) {
    case U8:
        SetData(data_name, dataPackage[offset], skipCallbacks);
        return 1;
    case U16:
        SetData(data_name, decode_uint16(dataPackage.ptr() + offset), skipCallbacks);
        return 2;
    case U32:
        SetData(data_name, decode_uint32(dataPackage.ptr() + offset), skipCallbacks);
        return 4;
    case U64:
        SetData(data_name, decode_uint64(dataPackage.ptr() + offset), skipCallbacks);
        return 8;
    case S8:
        SetData(data_name, static_cast<int8_t>(dataPackage[offset]), skipCallbacks);
        return 1;
    case S16:
        SetData(data_name, static_cast<int16_t>(decode_uint16(dataPackage.ptr() + offset)), skipCallbacks);
        return 2;
    case S32:
        SetData(data_name, static_cast<int32_t>(decode_uint32(dataPackage.ptr() + offset)), skipCallbacks);
        return 4;
    case S64:
        SetData(data_name, static_cast<int64_t>(decode_uint64(dataPackage.ptr() + offset)), skipCallbacks);
        return 8;
    case HalfFloat:
        SetData(data_name, decode_half(dataPackage.ptr() + offset), skipCallbacks);
        return 2;
    case Float:
        SetData(data_name, decode_float(dataPackage.ptr() + offset), skipCallbacks);
        return 4;
    case Double:
        SetData(data_name, decode_double(dataPackage.ptr() + offset), skipCallbacks);
        return 8;
    case Vector2Type:
        SetData(data_name, Vector2(
            decode_float(dataPackage.ptr() + offset),
            decode_float(dataPackage.ptr() + offset + 4)
        ), skipCallbacks);
        return 8;
    case Vector3Type:
        SetData(data_name, Vector3(
            decode_float(dataPackage.ptr() + offset),
            decode_float(dataPackage.ptr() + offset + 4),
            decode_float(dataPackage.ptr() + offset + 8)
        ), skipCallbacks);
        return 12;
    default:
        print_error("_setDataFromPackage received invalid data_type: " + String::num_int64(data_type));
        return 1;
}
}

int CompositeNode::_encodeDataToPackage(Variant value, PackedByteArray &dataPackage, int offset, DataSynchronizationType data_type) {
	if (value.get_type() == Variant::NIL) {
	    if (data_type == DataSynchronizationType::Vector2Type) value = Vector2(0, 0);
	    else if (data_type == DataSynchronizationType::Vector3Type) value = Vector3(0, 0, 0);
	    else value = 0;
	}

	switch (data_type) {
	    case DataSynchronizationType::U8:
	    case DataSynchronizationType::S8:
	        dataPackage.append(value);
	        return 1;
	    case DataSynchronizationType::U16:
	    case DataSynchronizationType::S16:
	        dataPackage.resize(dataPackage.size() + 2);
	        encode_uint16(value, dataPackage.ptrw() + offset);
	        return 2;
	    case DataSynchronizationType::U32:
	    case DataSynchronizationType::S32:
	        dataPackage.resize(dataPackage.size() + 4);
	        encode_uint32(value, dataPackage.ptrw() + offset);
	        return 4;
	    case DataSynchronizationType::U64:
	    case DataSynchronizationType::S64:
	        dataPackage.resize(dataPackage.size() + 8);
	        encode_uint64(value, dataPackage.ptrw() + offset);
	        return 8;
	    case DataSynchronizationType::HalfFloat:
	        dataPackage.resize(dataPackage.size() + 2);
	        encode_half(value, dataPackage.ptrw() + offset);
	        return 2;
	    case DataSynchronizationType::Float:
	        dataPackage.resize(dataPackage.size() + 4);
	        encode_float(value, dataPackage.ptrw() + offset);
	        return 4;
	    case DataSynchronizationType::Double:
	        dataPackage.resize(dataPackage.size() + 8);
	        encode_double(value, dataPackage.ptrw() + offset);
	        return 8;
	    case DataSynchronizationType::Vector2Type: {
		    dataPackage.resize(dataPackage.size() + 8);
	    	Vector2 v2 = value;
	    	encode_float(v2.x, dataPackage.ptrw() + offset);
	    	encode_float(v2.y, dataPackage.ptrw() + offset + 4);
	    	return 8;
	    }
	    case DataSynchronizationType::Vector3Type: {
		    dataPackage.resize(dataPackage.size() + 12);
	    	Vector3 v3 = value;
	    	encode_float(v3.x, dataPackage.ptrw() + offset);
	    	encode_float(v3.y, dataPackage.ptrw() + offset + 4);
	    	encode_float(v3.z, dataPackage.ptrw() + offset + 8);
	    	return 12;
	    }
	    default:
	        print_error("_encodeDataToPackage received invalid data_type: " + String::num_int64(data_type));
	        return 1;
	}
}

void CompositeNode::_updateDataWithLinearMovement(StringName dataName, StringName movementDataName, float delta) {
	auto data_value = _data.lookup_ptr(dataName);
	auto movement_data_value = _data.lookup_ptr(movementDataName);

	if (data_value && movement_data_value) {
		switch (movement_data_value->Value.get_type()) {
			case Variant::FLOAT:
				data_value->Value = (float)data_value->Value + delta * (float)movement_data_value->Value;
			break;
			case Variant::VECTOR3:
				data_value->Value = (Vector3)data_value->Value + delta * (Vector3)movement_data_value->Value;
			break;
			case Variant::VECTOR2:
				data_value->Value = (Vector2)data_value->Value + delta * (Vector2)movement_data_value->Value;
			break;
			default:
				print_error("CompositeNode::_updateDataWithLinearMovement can only handle float, Vector2 or Vector3 at the moment.");
				return;
		}

		for (const auto &cb : data_value->DataUpdatedCallbacks) {
			cb.call(data_value->Value);
		}
	}

}

Variant CompositeNode::_callFunctionOnAuthorityRPC(int _from_peer_id, StringName functionName, const Array &parameters) {
	Variant return_value = CallFunction(functionName, parameters);
	return return_value;
}

void CompositeNode::_authoritySetDataRPC(int from_peer_id, int dataID, Variant value) {
	if (dataID > _sync_data_on_change_sorting.size()) {
		return;
	}
	StringName dataName = _sync_data_on_change_sorting[dataID];
	// we exclude the sender of this RPC from the update, since it already set
	// the data locally...
	SetData(dataName, value, false, from_peer_id);
}

CompositeNode *CompositeNode::GetCompositeNodeInParents(Node *node) {
	while(node != nullptr) {
		CompositeNode* currentNodeComposite = dynamic_cast<CompositeNode*>(node);
		if(currentNodeComposite != nullptr) {
			return currentNodeComposite;
		}
		node = node->get_parent();
	}
	return nullptr;
}

void CompositeNode::InitializeAsAuthority() {
	int my_id = get_multiplayer()->get_unique_id();
	// We are the authority! Tell the others:
	GetCommunicationLine()->set_local_peer_bits(1);
	init_authority(my_id);
	Array params;
	params.append(my_id);
	_communication_line->call_function_on_peers("init_authority_rpc", params);
}

Ref<CommunicationLine> CompositeNode::GetCommunicationLine() {
	if (_communication_line.is_valid()) {
		return _communication_line;
	}
	CommunicationLineSystem* coms = CommunicationLineSystem::get_global_communication_line_system();
	if (coms == nullptr) {
		return {};
	}
	_communication_line = coms->grab_communication_line(get_path().get_concatenated_names());

	Array params;  // Temporary array for parameters

	params.clear();
	params.append(CommunicationLine::U64);
	_communication_line->add_function_definition("init_authority_rpc", callable_mp(this, &CompositeNode::init_authority_rpc), params, CommunicationLine::None, MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
	params.append(CommunicationLine::Bytes);
	params.append(CommunicationLine::U16);
	_communication_line->add_function_definition("_complete_sync_package", callable_mp(this, &CompositeNode::_complete_sync_package), params, CommunicationLine::None, MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
	params.append(CommunicationLine::Bytes);
	_communication_line->add_function_definition("_updateHighFrequencyData", callable_mp(this, &CompositeNode::_updateHighFrequencyData), params, CommunicationLine::None, MultiplayerPeer::TRANSFER_MODE_UNRELIABLE);

	params.clear();
	params.append(CommunicationLine::Bytes);
	_communication_line->add_function_definition("_updateLowFrequencyData", callable_mp(this, &CompositeNode::_updateLowFrequencyData), params, CommunicationLine::None, MultiplayerPeer::TRANSFER_MODE_UNRELIABLE);

	params.clear();
	params.append(CommunicationLine::Bytes);
	_communication_line->add_function_definition("_updateSingleOnChangeData", callable_mp(this, &CompositeNode::_updateSingleOnChangeData), params, CommunicationLine::None, MultiplayerPeer::TRANSFER_MODE_UNRELIABLE);

	params.clear();
	params.append(CommunicationLine::StringType);
	params.append(CommunicationLine::VariantType);
	_communication_line->add_function_definition("_callFunctionOnAuthorityRPC", callable_mp(this, &CompositeNode::_callFunctionOnAuthorityRPC), params, CommunicationLine::VariantType, MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
	params.append(CommunicationLine::U8);
	params.append(CommunicationLine::VariantType);
	_communication_line->add_function_definition("_authoritySetDataRPC", callable_mp(this, &CompositeNode::_authoritySetDataRPC), params, CommunicationLine::None, MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	return _communication_line;
}

void CompositeNode::SynchronizeAllToSingleClient(int client_multiplayer_id) {
	_send_buffer.clear();
	int offset = 0;

	offset += _encodeDataToPackage(GameTime(), _send_buffer, offset, DataSynchronizationType::Float);

	for (const auto& on_change_data_key : _sync_data_on_change_sorting) {
		// This data might never have been set!
		Variant send_value;
		DataValue* data_value = _data.lookup_ptr(on_change_data_key);
		if (data_value != nullptr) {
			send_value = data_value->Value;
		}
		auto on_change_config = _sync_data_on_change.lookup_ptr(on_change_data_key);
		offset += _encodeDataToPackage(send_value, _send_buffer, offset, on_change_config->SyncType);
	}

	for (const auto &sync_setting : _sync_data_high_freq) {
		// This data might never have been set!
		Variant send_value;
		DataValue* data_value = _data.lookup_ptr(sync_setting.DataName);
		if (data_value != nullptr) {
			send_value = data_value->Value;
		}
		offset += _encodeDataToPackage(send_value, _send_buffer, offset, sync_setting.SyncType);
	}

	for (const auto &sync_setting : _sync_data_low_freq) {
		// This data might never have been set!
		Variant send_value;
		DataValue* data_value = _data.lookup_ptr(sync_setting.DataName);
		if (data_value != nullptr) {
			send_value = data_value->Value;
		}
		offset += _encodeDataToPackage(send_value, _send_buffer, offset, sync_setting.SyncType);
	}

	int size_uncompressed = _send_buffer.size();
	PackedByteArray compressed;
	if (size_uncompressed > 0) {
		Compression::Mode mode = Compression::MODE_FASTLZ;
		compressed.resize(Compression::get_max_compressed_buffer_size(size_uncompressed, mode));
		int result = Compression::compress(compressed.ptrw(), _send_buffer.ptr(), size_uncompressed, mode);
		result = result >= 0 ? result : 0;
		compressed.resize(result);
	}

	Array params;
	params.append(compressed);
	params.append(size_uncompressed);
	GetCommunicationLine()->call_function_on_peer("_complete_sync_package", params, client_multiplayer_id);

	Array callback_params;
	callback_params.append(client_multiplayer_id);
	CallCallbacks("SynchronizeAllToSingleClient", callback_params);

}

void CompositeNode::SetupDataMultiplayerSynchronization(StringName dataName, DataSynchronizationMode syncMode, DataSynchronizationType dataType) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
	    _parentCompositeNode->SetupDataMultiplayerSynchronization(dataName, syncMode, dataType);
	    return;
	}

	// Quick sanity check! These should not be registered multiple times!
	ERR_FAIL_COND_MSG(_sync_data_on_change.has(dataName), vformat("CompositeNode.SetupDataMultiplayerSynchronization called for an already existing DataName: %s", dataName));
	for (const auto &lf : _sync_data_low_freq) {
	    ERR_FAIL_COND_MSG (lf.DataName == dataName, vformat("CompositeNode.SetupDataMultiplayerSynchronization called for an already existing DataName: %s", dataName));
	}
	for (const auto &hf : _sync_data_high_freq) {
	    ERR_FAIL_COND_MSG (hf.DataName == dataName, vformat("CompositeNode.SetupDataMultiplayerSynchronization called for an already existing DataName: %s", dataName));
	}

	// Create new sync settings
	DataSynchronizationSettings newSyncSettings;
	newSyncSettings.DataName = dataName;
	newSyncSettings.SyncType = dataType;

	switch (syncMode) {
	    case DataSynchronizationMode::OnChange:
	        _sync_data_on_change.insert(dataName, newSyncSettings);
	        _sync_data_on_change_sorting.append(dataName);
	        _sync_data_on_change_sorting.sort_custom<StringNameComparatorMultiplayerCompatible>();
	        for (int i = 0; i < _sync_data_on_change_sorting.size(); i++) {
	            _sync_data_on_change.lookup_ptr(_sync_data_on_change_sorting[i])->DataID = i;
	        }
	        break;

	    case DataSynchronizationMode::LowFrequency:
	        // DataID is not needed here, the order of the array is the only important thing
	        newSyncSettings.DataID = -1;
	        _sync_data_low_freq.append(newSyncSettings);
	        _sync_data_low_freq.sort_custom<DataSynchronizationSettingsComparator>();
	        break;

	    case DataSynchronizationMode::HighFrequency:
	        // DataID is not needed here, the order of the array is the only important thing
	        newSyncSettings.DataID = -1;
	        _sync_data_high_freq.append(newSyncSettings);
	        _sync_data_high_freq.sort_custom<DataSynchronizationSettingsComparator>();
	        break;
	}

}

void CompositeNode::SetupDataMultiplayerSynchronizationWithLinearMovement(StringName dataName, StringName velocityDataName, DataSynchronizationMode syncMode, DataSynchronizationType dataType) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
		_parentCompositeNode->SetupDataMultiplayerSynchronizationWithLinearMovement(dataName, velocityDataName, syncMode, dataType);
		return;
	}

	SetupDataMultiplayerSynchronization(dataName, syncMode, dataType);
	SetupDataMultiplayerSynchronization(velocityDataName, syncMode, dataType);
	_linear_movement_with_velocity.insert(dataName, velocityDataName);

}

bool CompositeNode::RegisterFunction(StringName functionName, Callable callable) {
	if (_functions.has(functionName)) {
		return false;
	}
	_functions.set(functionName, callable);
	return true;
}

void CompositeNode::UnregisterFunction(StringName functionName, const Callable &callable) {
	Callable* function_callable = _functions.lookup_ptr(functionName);
	if (function_callable != nullptr && *function_callable == callable) {
		_functions.remove(functionName);
	}
}

Variant CompositeNode::CallFunction(StringName functionName, const Array &parameters) {
	if (Callable *function_callable = _functions.lookup_ptr(functionName); function_callable != nullptr) {
		return function_callable->callv(parameters);
	}
	return Variant();
}

Ref<FutureValue> CompositeNode::CallFunctionOnAuthority(StringName functionName, const Array &parameters) {
	Ref f = memnew(FutureValue);

	if (GetCommunicationLine()->check_local_peer_bits(1, 0)) {
		// We are the authority, so call the function directly and return the value
		f->set_value(CallFunction(functionName, parameters));
	} else {
		Array params;
		params.append(functionName);
		params.append(parameters);
		Ref<CommunicationCallWithAnswer> call_object = _communication_line->call_function_on_peers_expect_answer(
			"_callFunctionOnAuthorityRPC", params, 1);

		call_object->AnswerReceivedCallback = [f](auto _call_object) {
			if (f.is_valid()) {
				// The authority bit should really only be set on one peer, so we just take the "first" answer
				f->set_value(_call_object->get_answer(0));
			}
		};
	}

	return f;
}

void CompositeNode::RegisterCallback(StringName callbackName, const Callable &callable) {
	if (Vector<Callable> *callback_callbacks = _callbacks.lookup_ptr(callbackName); callback_callbacks != nullptr) {
		callback_callbacks->append(callable);
	} else {
		Vector<Callable> callbacks_arr;
		callbacks_arr.append(callable);
		_callbacks.insert(callbackName, callbacks_arr);
	}
}

void CompositeNode::UnregisterCallback(StringName callbackName, const Callable &callable) {
	if (Vector<Callable> *callback_callbacks = _callbacks.lookup_ptr(callbackName); callback_callbacks != nullptr) {
		callback_callbacks->erase(callable);
	}
}

void CompositeNode::CallCallbacks(StringName callbackName, const Array &parameters) {
	if (Vector<Callable> *callback_callbacks = _callbacks.lookup_ptr(callbackName); callback_callbacks != nullptr) {
		for (auto& callback_callback : *callback_callbacks) {
			callback_callback.callv(parameters);
		}
	}
	if (_parentCompositeNode.is_valid() && ForwardCallbacksToParentCompositeNode.has(callbackName)) {
		_parentCompositeNode->CallCallbacks(callbackName, parameters);
	}
}

bool CompositeNode::HasData(StringName dataName) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
		return _parentCompositeNode->HasData(dataName);
	}
	return _data.has(dataName);
}

void CompositeNode::SetData(StringName dataName, Variant value, bool skipCallbacks, int skipMultiplayerPeer, bool increment) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
		_parentCompositeNode->SetData(dataName, value);
		return;
	}

	if (DataValue *data_value = _data.lookup_ptr(dataName); data_value != nullptr) {
		if (!increment) {
			data_value->Value = value;
		} else {
			switch (data_value->Value.get_type()) {
				case Variant::INT:
					data_value->Value = (int)data_value->Value + (int)value; break;
				case Variant::FLOAT:
					data_value->Value = (float)data_value->Value + (float)value; break;
				case Variant::VECTOR2:
					data_value->Value = (Vector2)data_value->Value + (Vector2)value; break;
				case Variant::VECTOR3:
					data_value->Value = (Vector3)data_value->Value + (Vector3)value; break;
				default: break;
			}
		}

		if (!skipCallbacks) {
			for (int i = 0; i < data_value->DataUpdatedCallbacks.size(); i++) {
				data_value->DataUpdatedCallbacks[i].call(data_value->Value);
			}
			// we'll also update the sums here!
			for (auto& sum_data_name : data_value->PartOfSums) {
				DataSumSettings* sum_settings = _sums.lookup_ptr(sum_data_name);
				if (sum_settings == nullptr) {
					continue;
				}
				update_sum_component(*sum_settings, dataName, data_value->Value);
			}
		}
	} else {
		DataValue new_data_value;
		new_data_value.Value = value;
		_data.insert(dataName, new_data_value);
	}

	if (is_multiplayer_authority()) {
		DataSynchronizationSettings* sync_config = _sync_data_on_change.lookup_ptr(dataName);
		if (sync_config != nullptr) {
			_sendOnChangeData(*sync_config, value, skipMultiplayerPeer);
		}
	}
}

void CompositeNode::SetDataOnAuthority(StringName dataName, Variant value) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
		_parentCompositeNode->SetDataOnAuthority(dataName, value);
		return;
	}
	// we'll set the data locally, so that the change gets reflected here right away
	SetData(dataName, value);
	// we need to sync to the authority, only when we are NOT the authority
	// (otherwise the syncing is already done by SetData)
	if (!is_multiplayer_authority()) {
		DataSynchronizationSettings* sync_config = _sync_data_on_change.lookup_ptr(dataName);
		ERR_FAIL_COND_MSG(sync_config == nullptr, "CompositeNode::SetDataOnAuthority can only be called for data that has OnChange Synchronization Settings set up.");
		int authority_peer_id = get_multiplayer_authority();
		Array params;
		params.append(sync_config->DataID);
		params.append(value);
		GetCommunicationLine()->call_function_on_peer("_authoritySetDataRPC", params, authority_peer_id);
	}
}

void CompositeNode::RegisterDataUpdatedCallback(StringName dataName, Callable callable, bool callIfDataExists) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
		_parentCompositeNode->RegisterDataUpdatedCallback(dataName, callable, callIfDataExists);
		return;
	}
	DataValue *data_value = _data.lookup_ptr(dataName);
	if (data_value == nullptr) {
		_data.insert(dataName, {});
		data_value = _data.lookup_ptr(dataName);
	}

	data_value->DataUpdatedCallbacks.append(callable);

	if (callIfDataExists && data_value->Value.get_type() != Variant::NIL) {
		callable.call(data_value->Value);
	}
}

void CompositeNode::UnregisterDataUpdatedCallback(StringName dataName, Callable callable) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
		_parentCompositeNode->UnregisterDataUpdatedCallback(dataName, callable);
		return;
	}
	if (DataValue *data_value = _data.lookup_ptr(dataName); data_value != nullptr) {
		data_value->DataUpdatedCallbacks.erase(callable);
	}
}

Variant CompositeNode::GetData(StringName dataName) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(dataName)) {
		return _parentCompositeNode->GetData(dataName);
	}
	if (DataValue *data_value = _data.lookup_ptr(dataName); data_value != nullptr) {
		return data_value->Value;
	}
	return Variant();
}

void CompositeNode::AddDataToSumDefinition(StringName sumDataName, StringName componentDataName, float initial_value) {
	if (_parentCompositeNode.is_valid() && ForwardDataToParentCompositeNode.has(componentDataName)) {
		_parentCompositeNode->AddDataToSumDefinition(sumDataName, componentDataName, initial_value);
		return;
	}

	DataSumSettings *sum_settings = _sums.lookup_ptr(sumDataName);
	if (sum_settings == nullptr) {
		DataSumSettings new_settings;
		new_settings.SumDataName = sumDataName;
		_sums.insert(sumDataName, new_settings);
		sum_settings = _sums.lookup_ptr(sumDataName);
	}

	sum_settings->SumComponentsDataNames.append(componentDataName);
	sum_settings->SumComponentsValues.append(initial_value);
	update_sum_component(*sum_settings, componentDataName, initial_value);

	// the data value might not yet exist, at this point!
	// but we have to add the sumDataName to its "PartOfSums",
	// so that the sums get updated in the SetData function.
	DataValue *data_value = _data.lookup_ptr(componentDataName);
	if (data_value == nullptr) {
		_data.insert(componentDataName, {});
		data_value = _data.lookup_ptr(componentDataName);
	}
	data_value->PartOfSums.append(sumDataName);
}

Ref<SynchronizedArray> CompositeNode::CreateSynchronizedArray(StringName variableName) {
	return SynchronizedArray::create(GetCommunicationLine(), variableName);
}
