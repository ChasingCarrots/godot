#ifndef COMPOSITENODE_H
#define COMPOSITENODE_H

#include "CommunicationLine.h"
#include "FutureValue.h"
#include "SafeObjectPointer.h"
#include "SynchronizedArray.h"

#include <scene/3d/node_3d.h>

class CompositeNode : public Node3D {
	GDCLASS(CompositeNode, Node3D)
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _notification(int p_notification);

public:
	enum DataSynchronizationMode {
		OnChange,
		LowFrequency,
		HighFrequency
	};
	enum DataSynchronizationType {
		None,

		U8,
		U16,
		U32,
		U64,
		S8,
		S16,
		S32,
		S64,

		HalfFloat,
		Float,
		Double,

		Vector2Type,
		Vector3Type
	};

protected:
	// exports:
	NodePath ParentCompositeNode;
	Vector<StringName> ForwardDataToParentCompositeNode;
	Vector<StringName> ForwardCallbacksToParentCompositeNode;

	SafeObjectPointer<CompositeNode> _parentCompositeNode;
	bool has_parent_composite_node() {
		if (_parentCompositeNode.is_valid()) { return true; }
		if (!ParentCompositeNode.is_empty()) {
			_parentCompositeNode.set(cast_to<CompositeNode>(get_node(ParentCompositeNode)));
			return _parentCompositeNode.is_valid();
		}
		return false;
	};

	Ref<CommunicationLine> _communication_line;
	PackedByteArray _send_buffer;
	Vector<StringName> _temp_stringnames;
	int _initialized_authority_id = -1;

	struct DataValue {
		Variant Value;
		Vector<Callable> DataUpdatedCallbacks;
		Vector<StringName> PartOfSums;
	};
	struct DataSynchronizationSettings {
		int DataID;
		StringName DataName;
		DataSynchronizationType SyncType;
		bool Paused = false;
	};
	struct DataSumSettings {
		StringName SumDataName;
		Vector<StringName> SumComponentsDataNames;
		PackedFloat32Array SumComponentsValues;
	};

	struct DataSynchronizationSettingsComparator {
		bool operator()(const DataSynchronizationSettings &a, const DataSynchronizationSettings &b) const {
			return static_cast<String>(a.DataName) < static_cast<String>(b.DataName);
		}
	};

	// the default way of comparing StringName just uses the assigned pointer
	// and that order will be different on every machine! we need a reliable
	// way to reproduce the same order everytime.
	struct StringNameComparatorMultiplayerCompatible {
		bool operator()(const StringName &a, const StringName &b) const {
			return static_cast<String>(a) < static_cast<String>(b);
		}
	};

	void update_sum_component(DataSumSettings &sum_settings, StringName component_name, float new_value);

	AHashMap<StringName, Callable> _functions;
	AHashMap<StringName, Vector<Callable>> _callbacks;
	AHashMap<StringName, DataValue> _data;

	AHashMap<StringName, DataSynchronizationSettings> _sync_data_on_change;
	Vector<StringName> _sync_data_on_change_sorting;
	Vector<DataSynchronizationSettings> _sync_data_low_freq;
	Vector<DataSynchronizationSettings> _sync_data_high_freq;
	AHashMap<StringName, DataSumSettings> _sums;
	AHashMap<StringName, StringName> _linear_movement_with_velocity;

	const float LOW_FREQUENCY_TIMING = 1.0f / 5.0f;
	const float HIGH_FREQUENCY_TIMING = 1.0f / 30.0f;
	const int BYTE_SIZE = 8;

	float _next_low_freq_gametime = 0.0f;
	float _next_high_freq_gametime = 0.0f;

	static Vector<CompositeNode*> _all_composite_nodes;
	static constexpr uint16_t INVALID_ID = 0;
	static uint16_t _next_composite_node_id;
	uint16_t _composite_ID = INVALID_ID;

	void _enter_tree();
	void _exit_tree();
	void _ready();
	void _process(float delta);

	void init_authority(int authority_player_id);
	void init_authority_rpc(int sender_id, int authority_player_id);
	void set_composite_id_rpc(int sender_id, uint16_t composite_id);

	void _peer_state_changed(int peer_id, CommunicationLine::CommunicationState new_state);

	void _complete_sync_package(int sender_id, const PackedByteArray & package, int size_uncompressed);
	void _sendHighFrequencyData();
	void _sendLowFrequencyData();
	void _sendOnChangeData(const DataSynchronizationSettings& sync_setting, Variant value, int skip_multiplayer_peer = -1);
	void _updateHighFrequencyData(int sender_id, const PackedByteArray & dataPackage);
	void _updateLowFrequencyData(int sender_id, const PackedByteArray & dataPackage);
	void _updateSingleOnChangeData(int sender_id, const PackedByteArray & dataPackage);

	int _setDataFromPackage(StringName dataName, const PackedByteArray &dataPackage, int offset, DataSynchronizationType data_type, bool skipCallbacks = false);
	int _encodeDataToPackage(Variant value, PackedByteArray & dataPackage, int offset, DataSynchronizationType data_type);

	void _updateDataWithLinearMovement(StringName dataName, StringName movementDataName, float delta);
	Variant _callFunctionOnAuthorityRPC(int _from_peer_id, StringName functionName, const Array& parameters);

	void _authoritySetDataRPC(int from_peer_id, int dataID, Variant value);

	PackedStringArray get_data_value_names() const {
		PackedStringArray names;
		for (const auto& iter : _data) {
			names.push_back(iter.key);
		}
		return names;
	}
	PackedStringArray get_function_names() const {
		PackedStringArray names;
		for (const auto& iter : _functions) {
			names.push_back(iter.key);
		}
		return names;
	}
	PackedStringArray get_callback_names() const {
		PackedStringArray names;
		for (const auto& iter : _callbacks) {
			names.push_back(iter.key);
		}
		return names;
	}

	String get_data_value_debug_string(StringName name) const;
	String get_callback_debug_string(StringName name) const;
	String get_function_debug_string(StringName name) const;
public:
	static float GameTimeServerOffset;
	static void SetGameTimeServerOffset(float offset) { GameTimeServerOffset = offset; }
	static float GameTime() { return (OS::get_singleton()->get_ticks_msec() + GameTimeServerOffset) / 1000.0f; }
	static CompositeNode *GetCompositeNodeInParents(Node* node);
	static int GetNumberOfExistingCompositeNodes() { return _all_composite_nodes.size(); };
	static CompositeNode* GetExistingCompositeNode(int index) { return _all_composite_nodes[index]; }
	static CompositeNode* GetCompositeNodeByID(int id) {
		for (auto node : _all_composite_nodes) {
			if (node->get_composite_id() == id) {
				return node;
			}
		}
		return nullptr;
	}

	void InitializeAsAuthority();
	Ref<CommunicationLine> GetCommunicationLine();
	bool IsServer();
	int GetServerID();
	bool IsAuthority();
	int GetAuthorityID();
	int GetLocalMultiplayerID();

	void SynchronizeAllToSingleClient(int client_multiplayer_id);
	void SetupDataMultiplayerSynchronization(StringName dataName, DataSynchronizationMode syncMode, DataSynchronizationType dataType);
	void SetupDataMultiplayerSynchronizationWithLinearMovement(StringName dataName, StringName velocityDataName, DataSynchronizationMode syncMode, DataSynchronizationType dataType);

	bool HasFunction(StringName functionName) const { return _functions.has(functionName); }
	bool RegisterFunction(StringName functionName, Callable callable);
	void UnregisterFunction(StringName functionName, const Callable &callable);
	Variant CallFunction(StringName functionName, const Array &parameters);
	Ref<FutureValue> CallFunctionOnAuthority(StringName functionName, const Array &parameters);

    bool HasCallback(StringName callbackName);
	void RegisterCallback(StringName callbackName, const Callable &callable);
	void UnregisterCallback(StringName callbackName, const Callable &callable);
	void CallCallbacks(StringName callbackName, const Array& parameters);

	bool HasData(StringName dataName);
	void SetData(StringName dataName, Variant value, bool skipCallbacks = false, int skipMultiplayerPeer = -1, bool increment = false);
	void SetDataOnAuthority(StringName dataName, Variant value);
	void PauseData(StringName dataName);
	void UnpauseData(StringName dataName);

	void RegisterDataUpdatedCallback(StringName dataName, Callable callable, bool callIfDataExists=false);
	void UnregisterDataUpdatedCallback(StringName dataName, Callable callable);
	Variant GetData(StringName dataName);
	void AddDataToSumDefinition(StringName sumDataName, StringName componentDataName, float initialValue);

	Ref<SynchronizedArray> CreateSynchronizedArray(StringName variableName);

	[[nodiscard]] NodePath get_parent_composite_node() const { return ParentCompositeNode;	}
	void set_parent_composite_node(NodePath parent_composite_node_path) { ParentCompositeNode = parent_composite_node_path; }
	[[nodiscard]] TypedArray<StringName> get_forward_data_to_parent_composite_node() {
		// we can't directly use Vector<StringName> (not supported by godot), so we'll have to convert here...
		// shouldn't be a big deal, since this is only used for the @exports in the editor...
		TypedArray<StringName> str_array;
		for (const auto& str_name : ForwardDataToParentCompositeNode) { str_array.append(str_name); }
		return str_array;
	}
	void set_forward_data_to_parent_composite_node(TypedArray<StringName> forward_data_to_parent_composite_node) {
		ForwardDataToParentCompositeNode.clear();
		for (const auto& s : forward_data_to_parent_composite_node) { ForwardDataToParentCompositeNode.append(s); }
	}
	[[nodiscard]] TypedArray<StringName> get_forward_callbacks_to_parent_composite_node() {
		TypedArray<StringName> str_array;
		for (const auto& str_name : ForwardCallbacksToParentCompositeNode) { str_array.append(str_name); }
		return str_array;
	}
	void set_forward_callbacks_to_parent_composite_node(TypedArray<StringName> forward_callbacks_to_parent_composite_node) {
		ForwardCallbacksToParentCompositeNode.clear();
		for (const auto &s : forward_callbacks_to_parent_composite_node) {
			ForwardCallbacksToParentCompositeNode.append(s);
		}
	}
	[[nodiscard]] uint16_t get_composite_id() const { return _composite_ID; }
	void set_composite_id(uint16_t id) { _composite_ID = id; }
};

VARIANT_ENUM_CAST(CompositeNode::DataSynchronizationMode);
VARIANT_ENUM_CAST(CompositeNode::DataSynchronizationType);

#endif //COMPOSITENODE_H
