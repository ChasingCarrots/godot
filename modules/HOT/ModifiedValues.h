#ifndef GODOT_SOURCE_MODIFIEDVALUES_H
#define GODOT_SOURCE_MODIFIEDVALUES_H

#include "GameObject.h"
#include <core/object/ref_counted.h>

enum class ModifiedValueType {
	NormalCalculation,
	AdditiveOnly,
	MultiplicativeOnly
};

class ModifiedIntValue : public RefCounted {
	GDCLASS(ModifiedIntValue, RefCounted)

	ModifiedValueType _type = ModifiedValueType::NormalCalculation;
	int _baseValue = 0;
	int _currentModifiedValue = 0;
	String _modifiedBy;
	GameObject* _gameObject = nullptr;
	Variant _gameObjectChecker;
	inline bool IsGameObjectValid() {
		if (_gameObjectChecker.get_type() != Variant::OBJECT)
			return false;
		Object *validatedObject = _gameObjectChecker.get_validated_object();
		if (validatedObject == nullptr)
			return false;
		if (!validatedObject->is_class("GameObject"))
			return false;
		return true;
	}
	Callable _rankModifier;
	TypedArray<String> _modifierCategories;

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	void _init(int baseVal, String modifierName, Variant gameObject, Callable rankModifier);
	void _initAsAdditiveOnly(String modifierName, Variant gameObject, Callable rankModifier);
	void setModifierCategories(TypedArray<String> categories);
	TypedArray<String> getModifierCategories() { return _modifierCategories.duplicate(); }
	void updateManually();
	void updateModifier(String mod);
	int Value();
	int BaseValue() { return _baseValue; }
	String ModifiedBy() { return _modifiedBy; }
};

class ModifiedFloatValue : public RefCounted {
	GDCLASS(ModifiedFloatValue, RefCounted)

	ModifiedValueType _type = ModifiedValueType::NormalCalculation;
	float _baseValue = 0;
	float _currentModifiedValue = 0;
	String _modifiedBy;
	GameObject* _gameObject = nullptr;
	Variant _gameObjectChecker;
	inline bool IsGameObjectValid() {
		if (_gameObjectChecker.get_type() != Variant::OBJECT)
			return false;
		Object *validatedObject = _gameObjectChecker.get_validated_object();
		if (validatedObject == nullptr)
			return false;
		if (!validatedObject->is_class("GameObject"))
			return false;
		return true;
	}
	Callable _rankModifier;
	TypedArray<String> _modifierCategories;

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	void _init(float baseVal, String modifierName, Variant gameObject, Callable rankModifier);
	void _initAsAdditiveOnly(String modifierName, Variant gameObject, Callable rankModifier);
	void _initAsMultiplicativeOnly(String modifierName, Variant gameObject, Callable rankModifier);
	void setModifierCategories(TypedArray<String> categories);
	TypedArray<String> getModifierCategories() { return _modifierCategories.duplicate(); }
	void updateManually();
	void updateModifier(String mod);
	float Value();
	float BaseValue() { return _baseValue; }
	String ModifiedBy() { return _modifiedBy; }
};

#endif //GODOT_SOURCE_MODIFIEDVALUES_H
