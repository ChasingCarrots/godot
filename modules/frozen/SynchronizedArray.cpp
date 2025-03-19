#include "SynchronizedArray.h"

#include <core/io/compression.h>

void SynchronizedArray::_bind_methods() {
	BIND_ENUM_CONSTANT(Added);
	BIND_ENUM_CONSTANT(Removed);
	BIND_ENUM_CONSTANT(Changed);

    // Bind methods
    ClassDB::bind_method(D_METHOD("sync_all", "to_peer_id"), &SynchronizedArray::sync_all);
    ClassDB::bind_method(D_METHOD("size"), &SynchronizedArray::size);
    ClassDB::bind_method(D_METHOD("is_empty"), &SynchronizedArray::is_empty);
    ClassDB::bind_method(D_METHOD("get_element", "index"), &SynchronizedArray::get_element);
    ClassDB::bind_method(D_METHOD("append", "value"), &SynchronizedArray::append);
    ClassDB::bind_method(D_METHOD("insert", "at_index", "value"), &SynchronizedArray::insert);
    ClassDB::bind_method(D_METHOD("erase", "value"), &SynchronizedArray::erase);
    ClassDB::bind_method(D_METHOD("remove_at", "at_index"), &SynchronizedArray::remove_at);
    ClassDB::bind_method(D_METHOD("clear"), &SynchronizedArray::clear);
    ClassDB::bind_method(D_METHOD("change_at", "at_index", "new_value"), &SynchronizedArray::change_at);
	ClassDB::bind_method(D_METHOD("_iter_init", "v"), &SynchronizedArray::_iter_init);
	ClassDB::bind_method(D_METHOD("_iter_next", "v"), &SynchronizedArray::_iter_next);
	ClassDB::bind_method(D_METHOD("_iter_get", "v"), &SynchronizedArray::_iter_get);


	ClassDB::bind_static_method("SynchronizedArray", D_METHOD("create", "communication_line", "prefix"),
		&SynchronizedArray::create);

    // Bind signal
    ADD_SIGNAL(MethodInfo("ElementChanged",
        PropertyInfo(Variant::INT, "change_type", PROPERTY_HINT_ENUM, "Added,Removed,Changed"),
        PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT),
        PropertyInfo(Variant::INT, "index")));
}

SynchronizedArray::SynchronizedArray() {
    // Default constructor required by Godot, but we won't initialize anything here
}

Ref<SynchronizedArray> SynchronizedArray::create(Ref<CommunicationLine> communication_line, String prefix) {
    Ref<SynchronizedArray> instance;
    instance.instantiate();

    instance->SYNC_ALL_FUNCTION = prefix + "_sync";
    instance->APPEND_FUNCTION = prefix + "_append";
    instance->INSERT_FUNCTION = prefix + "_insert";
    instance->REMOVE_AT_FUNCTION = prefix + "_remove_at";
    instance->CLEAR_FUNCTION = prefix + "_clear";
    instance->CHANGE_AT_FUNCTION = prefix + "_change_at";

    instance->_communication_line = communication_line;

    // Register functions with CommunicationLine
	Array params;
	params.append(CommunicationLine::U16);
	params.append(CommunicationLine::Bytes);
	params.append(CommunicationLine::U16);
    instance->_communication_line->add_function_definition(instance->SYNC_ALL_FUNCTION,
        callable_mp(instance.ptr(), &SynchronizedArray::remote_sync_all),
        params, CommunicationLine::None,
        MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
	params.append(CommunicationLine::VariantType);
	instance->_communication_line->add_function_definition(instance->APPEND_FUNCTION,
        callable_mp(instance.ptr(), &SynchronizedArray::remote_append),
        params, CommunicationLine::None,
        MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
	params.append(CommunicationLine::U16);
	params.append(CommunicationLine::VariantType);
	instance->_communication_line->add_function_definition(instance->INSERT_FUNCTION,
        callable_mp(instance.ptr(), &SynchronizedArray::remote_insert),
        params, CommunicationLine::None,
        MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
	params.append(CommunicationLine::U16);
    instance->_communication_line->add_function_definition(instance->REMOVE_AT_FUNCTION,
        callable_mp(instance.ptr(), &SynchronizedArray::remote_remove_at),
        params, CommunicationLine::None,
        MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
    instance->_communication_line->add_function_definition(instance->CLEAR_FUNCTION,
        callable_mp(instance.ptr(), &SynchronizedArray::remote_clear),
        params, CommunicationLine::None,
        MultiplayerPeer::TRANSFER_MODE_RELIABLE);

	params.clear();
	params.append(CommunicationLine::U16);
	params.append(CommunicationLine::VariantType);
	instance->_communication_line->add_function_definition(instance->CHANGE_AT_FUNCTION,
        callable_mp(instance.ptr(), &SynchronizedArray::remote_change_at),
        params, CommunicationLine::None,
        MultiplayerPeer::TRANSFER_MODE_RELIABLE);

    return instance;
}

void SynchronizedArray::emit_element_changed(ElementChangeType change_type, Variant value, int index) {
    emit_signal("ElementChanged", (int)change_type, value, index);
}

void SynchronizedArray::remote_sync_all(int sender_id, int number_of_elements, PackedByteArray elements_buffer, int uncompressed_size) {
    if (uncompressed_size > 0) {
    	PackedByteArray dataPackage;
    	Compression::Mode mode = Compression::MODE_FASTLZ;

    	int64_t buffer_size = uncompressed_size;
    	dataPackage.resize(buffer_size);
    	int result = Compression::decompress(dataPackage.ptrw(), buffer_size, elements_buffer.ptr(), elements_buffer.size(), mode);
    	result = result >= 0 ? result : 0;
    	dataPackage.resize(result);
        elements_buffer = dataPackage;
    }
    Ref<StreamPeerBuffer> buf;
    buf.instantiate();
    buf->set_data_array(elements_buffer);

    for (int i = 0; i < number_of_elements; i++) {
        Variant value = buf->get_var();
        _array.append(value);
        emit_element_changed(Added, value, _array.size() - 1);
    }
}

void SynchronizedArray::remote_append(int sender_id, Variant value) {
    _array.append(value);
    emit_element_changed(Added, value, _array.size() - 1);
}

void SynchronizedArray::remote_insert(int sender_id, int at_index, Variant value) {
    if (at_index < 0 || at_index > _array.size()) {
        print_error("SynchronizedArray received an insert request with an index out of bounds. Appending the value instead...");
        at_index = _array.size();
    }
    _array.insert(at_index, value);
    emit_element_changed(Added, value, at_index);
}

void SynchronizedArray::remote_remove_at(int sender_id, int at_index) {
    if (at_index < 0 || at_index >= _array.size()) {
        print_error("SynchronizedArray received a remove_at request with an index out of bounds.");
        return;
    }
    Variant value = _array[at_index];
    _array.remove_at(at_index);
    emit_element_changed(Removed, value, at_index);
}

void SynchronizedArray::remote_clear(int sender_id) {
    for (int i = 0; i < _array.size(); i++) {
        emit_element_changed(Removed, _array[i], i);
    }
    _array.clear();
}

void SynchronizedArray::remote_change_at(int sender_id, int at_index, Variant value) {
    if (at_index < 0 || at_index >= _array.size()) {
        print_error("SynchronizedArray received a change_at request with an index out of bounds.");
        return;
    }
    _array[at_index] = value;
    emit_element_changed(Changed, value, at_index);
}

void SynchronizedArray::sync_all(int to_peer_id) {
    if (_array.is_empty()) {
        return;
    }
    Ref<StreamPeerBuffer> stream;
    stream.instantiate();
    for (int i = 0; i < _array.size(); i++) {
        stream->put_var(_array[i]);
    }

    PackedByteArray packet_byte_array = stream->get_data_array();
    int uncompressed_size = packet_byte_array.size();
    if (uncompressed_size > 50) {
    	PackedByteArray compressed;
    	Compression::Mode mode = Compression::MODE_FASTLZ;
    	compressed.resize(Compression::get_max_compressed_buffer_size(uncompressed_size, mode));
    	int result = Compression::compress(compressed.ptrw(), packet_byte_array.ptr(), uncompressed_size, mode);
    	result = result >= 0 ? result : 0;
    	compressed.resize(result);
    	packet_byte_array = compressed;
    } else {
        uncompressed_size = 0;
    }
	Array params;
	params.append(_array.size());
	params.append(packet_byte_array);
	params.append(uncompressed_size);
    _communication_line->call_function_on_peer(SYNC_ALL_FUNCTION,
        params, to_peer_id);
}

int SynchronizedArray::size() const {
    return _array.size();
}

bool SynchronizedArray::is_empty() const {
    return _array.is_empty();
}

Variant SynchronizedArray::get_element(int index) const {
    if (index >= 0 && index < _array.size()) {
        return _array[index];
    }
    return Variant();
}

void SynchronizedArray::append(Variant value) {
    _array.append(value);
	Array params;
	params.append(value);
    _communication_line->call_function_on_peers(APPEND_FUNCTION, params);
}

void SynchronizedArray::insert(int at_index, Variant value) {
    if (at_index < 0) at_index = 0;
    if (at_index >= _array.size()) {
        append(value);
        return;
    }
    _array.insert(at_index, value);
	Array params;
	params.append(at_index);
	params.append(value);
    _communication_line->call_function_on_peers(INSERT_FUNCTION, params);
}

void SynchronizedArray::erase(Variant value) {
    int index = _array.find(value);
    if (index != -1) {
        remove_at(index);
    }
}

void SynchronizedArray::remove_at(int at_index) {
    if (at_index < 0 || at_index >= _array.size()) {
        print_error("SynchronizedArray.remove_at called with an index out of bounds.");
        return;
    }
    _array.remove_at(at_index);
	Array params;
	params.append(at_index);
    _communication_line->call_function_on_peers(REMOVE_AT_FUNCTION, params);
}

void SynchronizedArray::clear() {
    _array.clear();
    _communication_line->call_function_on_peers(CLEAR_FUNCTION, Array());
}

void SynchronizedArray::change_at(int at_index, Variant new_value) {
    if (at_index < 0 || at_index >= _array.size()) {
        print_error("SynchronizedArray.change_at called with an index out of bounds.");
        return;
    }
    _array[at_index] = new_value;
	Array params;
	params.append(at_index);
	params.append(new_value);
    _communication_line->call_function_on_peers(CHANGE_AT_FUNCTION, params);
}

bool SynchronizedArray::_iter_init(Variant arg) {
    iter_current_index = 0;
    return !_array.is_empty();
}

bool SynchronizedArray::_iter_next(Variant arg) {
    iter_current_index++;
    return iter_current_index < _array.size();
}

Variant SynchronizedArray::_iter_get(Variant arg) {
    return _array[iter_current_index];
}