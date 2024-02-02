#ifndef STATICVALUEHELPER_H
#define STATICVALUEHELPER_H

#include <core/object/class_db.h>
#include <core/object/object.h>
#include <core/templates/oa_hash_map.h>

class StaticValueHelper : public Object {
	GDCLASS(StaticValueHelper, Object)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_static_method("StaticValueHelper", D_METHOD("get_value", "key"), &StaticValueHelper::get_value);
		ClassDB::bind_static_method("StaticValueHelper", D_METHOD("set_value", "key", "value"), &StaticValueHelper::set_value);
		ClassDB::bind_static_method("StaticValueHelper", D_METHOD("has_value", "key"), &StaticValueHelper::has_value);
	}

	static OAHashMap<String, Variant> _staticValues;

public:
	inline static Variant get_value(const String& key) {
		Variant var;
		_staticValues.lookup(key, var);
		return var;
	}
	inline static void set_value(const String& key, const Variant &var) {
		_staticValues.set(key, var);
	}
	inline static bool has_value(const String& key) {
		return _staticValues.has(key);
	}
};



#endif //STATICVALUEHELPER_H
