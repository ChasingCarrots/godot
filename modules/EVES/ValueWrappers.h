#ifndef VALUEWRAPPER_H
#define VALUEWRAPPER_H

#include "FlatValue.h"
#include "SumValue.h"

#include <core/io/resource.h>

namespace eves {

class FlatValueResource : public Resource {
	GDCLASS(FlatValueResource, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("is_value_valid"), &FlatValueResource::IsValueValid);
		ClassDB::bind_method(D_METHOD("get_value"), &FlatValueResource::GetValue);
		ClassDB::bind_method(D_METHOD("set_value"), &FlatValueResource::SetValue);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Value"), "set_value", "get_value");
	}

public:
	explicit FlatValueResource() {
		// start out with the pointer pointing to our own flatvalue,
		// so that this wrapper can be used as is right away.
		_myFlatValuePointer = &_ownedFlatValue;
	}

	void InitAsNonOwner(FlatValue* flatValue) {
		// when this is called (only possible from C++!), that essentially
		// overrides the OwnedFlatValue and turns this into a
		// godot interface for that FlatValue.
		_myFlatValuePointer = flatValue;
		// invalidate this whole resource when this pointed-to value gets deleted!
		_invalidatePointerConnection = _myFlatValuePointer->OnValueDeleted
			.connect<&FlatValueResource::InvalidatePointer>(this);
	}

	bool IsValueValid() const {
		return _myFlatValuePointer != nullptr;
	}

	float GetValue() const {
		ERR_FAIL_COND_V_MSG(_myFlatValuePointer == nullptr, 0, "This FlatValueResource points to an invalid FlatValue and must not be used anymore");
		return _myFlatValuePointer->GetValue();
	}

	void SetValue(float newValue) const {
		ERR_FAIL_COND_MSG(_myFlatValuePointer == nullptr, "This FlatValueResource points to an invalid FlatValue and must not be used anymore");
		_myFlatValuePointer->SetValue(newValue);
	}

	BaseValue* GetValuePointer() const {
		return _myFlatValuePointer;
	}

private:
	FlatValue* _myFlatValuePointer = nullptr;

	FlatValue _ownedFlatValue;

	fteng::connection _invalidatePointerConnection;
	void InvalidatePointer(BaseValue*) {
		_myFlatValuePointer = nullptr;
	}
};



class SumValueResource : public Resource {
	GDCLASS(SumValueResource, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("is_value_valid"), &SumValueResource::IsValueValid);
		ClassDB::bind_method(D_METHOD("get_value"), &SumValueResource::GetValue);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Value"), "", "get_value");
		ClassDB::bind_method(D_METHOD("add_value_to_sum", "value_resource"), &SumValueResource::AddValueResourceToSum);
	}

public:
	explicit SumValueResource() {
		// start out with the pointer pointing to our own SumValue,
		// so that this wrapper can be used as is right away.
		_mySumValuePointer = &_ownedSumValue;
	}

	void InitAsNonOwner(SumValue* SumValue) {
		// when this is called (only possible from C++!), that essentially
		// overrides the OwnedSumValue and turns this into a
		// godot interface for that SumValue.
		_mySumValuePointer = SumValue;
		// invalidate this whole resource when this pointed-to value gets deleted!
		_invalidatePointerConnection = _mySumValuePointer->OnValueDeleted
			.connect<&SumValueResource::InvalidatePointer>(this);
	}

	bool IsValueValid() const {
		return _mySumValuePointer != nullptr;
	}

	float GetValue() const {
		ERR_FAIL_COND_V_MSG(_mySumValuePointer == nullptr, 0, "This SumValueResource points to an invalid SumValue and must not be used anymore");
		return _mySumValuePointer->GetValue();
	}

	void AddValuePointerToSum(BaseValue* valuePointer) const {
		ERR_FAIL_COND_MSG(_mySumValuePointer == nullptr, "This SumValueResource points to an invalid SumValue and must not be used anymore");
		_mySumValuePointer->AddValueToSum(valuePointer);
	}

	void AddValueResourceToSum(Variant valueResource) const {
		ERR_FAIL_COND_MSG(_mySumValuePointer == nullptr, "This SumValueResource points to an invalid SumValue and must not be used anymore");
		auto flatValue = cast_to<FlatValueResource>(valueResource);
		if(flatValue != nullptr) {
			_mySumValuePointer->AddValueToSum(flatValue->GetValuePointer());
			return;
		}
		auto sumValue = Object::cast_to<SumValueResource>(valueResource);
		if(sumValue != nullptr) {
			_mySumValuePointer->AddValueToSum(sumValue->GetValuePointer());
			return;
		}
		print_error("SumValueResource::AddValueResourceToSum: the passed valueResource is no known ValueResource");
	}

	BaseValue* GetValuePointer() const {
		return _mySumValuePointer;
	}

private:
	SumValue* _mySumValuePointer = nullptr;

	SumValue _ownedSumValue;

	fteng::connection _invalidatePointerConnection;
	void InvalidatePointer(BaseValue*) {
		_mySumValuePointer = nullptr;
	}
};


class NamedFlatValueResource : public Resource {
	GDCLASS(NamedFlatValueResource, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_value_name"), &NamedFlatValueResource::GetValueName);
		ClassDB::bind_method(D_METHOD("set_value_name"), &NamedFlatValueResource::SetValueName);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "ValueName"), "set_value_name", "get_value_name");
		ClassDB::bind_method(D_METHOD("get_value"), &NamedFlatValueResource::GetValue);
		ClassDB::bind_method(D_METHOD("set_value"), &NamedFlatValueResource::SetValue);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "Value", PROPERTY_HINT_RESOURCE_TYPE, "FlatValueResource"), "set_value", "get_value");
	}

	String ValueName;
	Ref<FlatValueResource> Value;
public:
	String GetValueName() const { return ValueName; }
	void SetValueName(const String &valueName) { ValueName = valueName; }
	Ref<FlatValueResource> GetValue() const { return Value; }
	void SetValue(const Ref<FlatValueResource> &value) { Value = value; }
};

class NamedSumValueResource : public Resource {
	GDCLASS(NamedSumValueResource, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_value_name"), &NamedSumValueResource::GetValueName);
		ClassDB::bind_method(D_METHOD("set_value_name"), &NamedSumValueResource::SetValueName);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "ValueName"), "set_value_name", "get_value_name");
		ClassDB::bind_method(D_METHOD("get_value"), &NamedSumValueResource::GetValue);
		ClassDB::bind_method(D_METHOD("set_value"), &NamedSumValueResource::SetValue);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "Value", PROPERTY_HINT_RESOURCE_TYPE, "SumValueResource"), "set_value", "get_value");
	}

	String ValueName;
	Ref<SumValueResource> Value;
public:
	String GetValueName() const { return ValueName; }
	void SetValueName(const String &valueName) { ValueName = valueName; }
	Ref<SumValueResource> GetValue() const { return Value; }
	void SetValue(const Ref<SumValueResource> &value) { Value = value; }
};

class FloatTemplate : public Resource {
	GDCLASS(FloatTemplate, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
	}

	String _valueName;
	float _defaultValue;
public:
	String GetValueName() const { return _valueName; }
	void SetValueName(const String &valueName) { _valueName = valueName; }
	float GetDefaultValue() const { return _defaultValue; }
	void SetDefaultValue(float defaultValue) { _defaultValue = defaultValue; }
};

} //namespace eves

#endif //VALUEWRAPPER_H
