#ifndef COMMUNICATIONLINE_H
#define COMMUNICATIONLINE_H

#include <core/io/stream_peer.h>
#include <core/object/ref_counted.h>
#include <scene/main/multiplayer_peer.h>

class CommunicationLineSystem;
class CommunicationLinePeer;

enum class CommunicationLinePacketTypes {
    CreateSingleLine = 66,
    CreateMultipleLines = 77,
    UpdateLinePeerState = 88,
    UpdateLinePeerBits = 99,
	UpdateLinePeerStateAndBits = 111,
	CallRemoteFunction = 122,
	CallRemoteFunctionExpectAnswer = 123,
	AnswerToFunctionCall = 133
};

class CommunicationLinePeer {
private:
    int _multiplayer_id = 0;
    uint8_t _peer_bits = 0;
    // can't use the actual enum here, because it has to be part
    // of the CommunicationLine class (so that it is correctly exposed to godot)
    int _communication_state = 0;

public:
    bool check_peer_bits(uint8_t set_bitmask, uint8_t unset_bitmask) const {
        return (_peer_bits & set_bitmask) == set_bitmask &&
            (_peer_bits & unset_bitmask) == 0;
    }
    bool set_peer_bits(uint8_t set_bitmask) {
    	uint8_t old_peer_bits = _peer_bits;
        _peer_bits |= set_bitmask;
    	return old_peer_bits != _peer_bits;
    }
    bool unset_peer_bits(uint8_t unset_bitmask) {
    	uint8_t old_peer_bits = _peer_bits;
        _peer_bits &= ~unset_bitmask;
    	return old_peer_bits != _peer_bits;
    }
	bool reset_peer_bits(uint8_t bits) {
    	if (_peer_bits == bits) { return false; }
	    _peer_bits = bits;
    	return true;
    }

    [[nodiscard]] int get_peer_bits() const { return _peer_bits; }
    [[nodiscard]] int get_multiplayer_id() const { return _multiplayer_id; }
    void set_multiplayer_id(int multiplayer_id) { _multiplayer_id = multiplayer_id; }
    [[nodiscard]] int get_communication_state() const { return _communication_state; }
    bool set_communication_state(int state) {
    	if (_communication_state == state) { return false; }
	    _communication_state = state;
    	return true;
    }
};

class CommunicationCallWithAnswer : public RefCounted {
	GDCLASS(CommunicationCallWithAnswer, RefCounted)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	struct CallPeerAnswer {
		int MultiplayerID = -1;
		Variant Answer;
		int TimeToAnswer;
	};

	int FunctionIndex = 0;
	uint8_t FunctionCallNumber = 0;
	int TicksAtSendTime = 0;
	Vector<CallPeerAnswer> PeerAnswers;

	int get_number_of_answers() const { return PeerAnswers.size(); }
	int get_peer_id(int index) const { return PeerAnswers[index].MultiplayerID; }
	Variant get_answer(int index) const { return PeerAnswers[index].Answer; }
	int get_time_to_answer(int index) const { return PeerAnswers[index].TimeToAnswer; }
};


class CommunicationLine : public RefCounted {
	GDCLASS(CommunicationLine, RefCounted)

public:
	enum CommunicationState {
		NotConnected,
		WaitingForConnection,
		ConnectedClosed,
		ConnectedOpen
	};

	enum ParamType {
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
		Vector3Type,

		Bytes,
		StringType,
		VariantType
	};

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

private:
	friend CommunicationLineSystem;
	CommunicationLineSystem *_communication_line_system = nullptr;
	StringName _string_id;
	uint16_t _int_id = 0;

	CommunicationLinePeer _my_peer_info;
	Vector<CommunicationLinePeer> _other_peers;
	StreamPeerBuffer _send_buffer;

	struct CommunicationFunction {
		StringName Name;
		Callable FunctionCallable;
		Vector<ParamType> Parameters;
		MultiplayerPeer::TransferMode Mode;
		ParamType ExpectedAnswer = ParamType::None;
	};
	Vector<CommunicationFunction> _communication_functions;
	uint8_t _next_call_id = 0;
	Vector<Ref<CommunicationCallWithAnswer>> _communication_calls_waiting_for_answer;

	void create_unconnected_peers(const Vector<int> &peers);
	void new_peer_connected(int peer_id);
	void update_own_communication_state(CommunicationState state);
	void on_packet_received(CommunicationLinePacketTypes packet_type, int from_multiplayer_id, Ref<StreamPeerBuffer> &packet);

	void fill_send_buffer_with_value(ParamType value_type, const Variant &value);
	int fill_send_buffer_with_function_parameters(const StringName & function_name, const Array & parameters);

public:
	void finish_initialization_and_open_line();

	void add_function_definition(const StringName &function_name, Callable callable, Array parameter_types, ParamType expected_answer, MultiplayerPeer::TransferMode mode);
	void call_function_on_peers(const StringName& function_name, Array parameters, uint8_t only_on_peers_with_bits_set = 0, uint8_t only_on_peers_with_bits_unset = 0);
	void call_function_on_peer(const StringName& function_name, Array parameters, int peer_id);

	Ref<CommunicationCallWithAnswer> call_function_on_peers_expect_answer(
		const StringName& function_name, Array parameters,
		uint8_t only_on_peers_with_bits_set = 0, uint8_t only_on_peers_with_bits_unset = 0);
	Ref<CommunicationCallWithAnswer> call_function_on_peer_expect_answer(
		const StringName& function_name, Array parameters, int peer_id);

	StringName get_string_id() const {
		return _string_id;
	}
	uint16_t get_int_id() const {
		return _int_id;
	}

	CommunicationState get_local_peer_state() const {
		return static_cast<CommunicationState>(_my_peer_info.get_communication_state());
	}
	CommunicationState get_peer_state(int peer_id) const;

	bool check_local_peer_bits(uint8_t set_bitmask, uint8_t unset_bitmask) const {
		return (_my_peer_info.get_peer_bits() & set_bitmask) == set_bitmask &&
		    (_my_peer_info.get_peer_bits() & unset_bitmask) == 0;
	}
	uint8_t get_local_peer_bits() const { return _my_peer_info.get_peer_bits(); }
	uint8_t get_peer_bits(int peer_id) const;

	void set_local_peer_bits(uint8_t bit);
	void unset_local_peer_bits(uint8_t bit);
};

VARIANT_ENUM_CAST(CommunicationLine::CommunicationState);
VARIANT_ENUM_CAST(CommunicationLine::ParamType);

#endif //COMMUNICATIONLINE_H
