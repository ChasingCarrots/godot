#ifndef FUTUREVALUE_H
#define FUTUREVALUE_H

#include <core/object/ref_counted.h>


class FutureValue : public RefCounted {
	GDCLASS(FutureValue, RefCounted)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("has_value"), &FutureValue::has_value);
		ClassDB::bind_method(D_METHOD("get_value"), &FutureValue::get_value);
		ClassDB::bind_method(D_METHOD("set_value", "value"), &FutureValue::set_value);

		ADD_SIGNAL(MethodInfo("ValueWasSet", PropertyInfo(Variant::NIL, "value")));
	}

	bool _value_was_set = false;
	Variant _value;
public:
	// This function is only for C++, so that we can connect lambdas
	std::function<void(Variant)> ValueWasSetCallback;

	bool has_value() const { return _value_was_set; }
	Variant get_value() const { return _value; }
	void set_value(Variant value) {
		_value_was_set = true;
		_value = value;
		emit_signal("ValueWasSet", _value);
		if (ValueWasSetCallback) {
			ValueWasSetCallback(value);
		}
	}
};



#endif //FUTUREVALUE_H
