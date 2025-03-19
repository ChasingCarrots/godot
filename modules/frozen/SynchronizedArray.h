#ifndef SYNCHRONIZED_ARRAY_H
#define SYNCHRONIZED_ARRAY_H

#include "CommunicationLine.h"

class SynchronizedArray : public RefCounted {
    GDCLASS(SynchronizedArray, RefCounted)

public:
    enum ElementChangeType {
        Added,
        Removed,
        Changed
    };

private:
    Ref<CommunicationLine> _communication_line;
    Array _array;

    StringName SYNC_ALL_FUNCTION;
    StringName APPEND_FUNCTION;
    StringName INSERT_FUNCTION;
    StringName REMOVE_AT_FUNCTION;
    StringName CLEAR_FUNCTION;
    StringName CHANGE_AT_FUNCTION;

    // Iterator state
    int iter_current_index;

protected:
    static void _bind_methods();

public:
    SynchronizedArray();
    static Ref<SynchronizedArray> create(Ref<CommunicationLine> communication_line, String prefix);

    // Signal
    void emit_element_changed(ElementChangeType change_type, Variant value, int index);

    // Remote functions
    void remote_sync_all(int sender_id, int number_of_elements, PackedByteArray elements_buffer, int uncompressed_size);
    void remote_append(int sender_id, Variant value);
    void remote_insert(int sender_id, int at_index, Variant value);
    void remote_remove_at(int sender_id, int at_index);
    void remote_clear(int sender_id);
    void remote_change_at(int sender_id, int at_index, Variant value);

    // Public methods
    void sync_all(int to_peer_id);
    int size() const;
    bool is_empty() const;
    Variant get_element(int index) const;
    void append(Variant value);
    void insert(int at_index, Variant value);
    void erase(Variant value);
    void remove_at(int at_index);
    void clear();
    void change_at(int at_index, Variant new_value);

    // Iterable implementation
    bool _iter_init(Variant arg);
    bool _iter_next(Variant arg);
    Variant _iter_get(Variant arg);
};

VARIANT_ENUM_CAST(SynchronizedArray::ElementChangeType)

#endif // SYNCHRONIZED_ARRAY_H