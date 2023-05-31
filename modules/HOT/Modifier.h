#ifndef GODOT_SOURCE_MODIFIER_H
#define GODOT_SOURCE_MODIFIER_H

#include "core/variant/typed_array.h"
#include <core/object/ref_counted.h>

class GameObject;

class Modifier : public RefCounted {
	GDCLASS(Modifier, RefCounted)

	friend GameObject;

	float _additiveMod = 0;
	float _multiplierMod = 0;
	String _modifiedType;
	String _modifierName = "Unnamed Modifier";
	Variant _gameObject;
	TypedArray<String> _modifierCategories;

	~Modifier();

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	static Modifier* create(String modifiedType, Variant gameObject) {
		Modifier* mod = memnew(Modifier);
		mod->_init(modifiedType, gameObject);
		return mod;
	}
	void _init(String modifiedType, Variant gameObject);

	void setModifierName(String modifierName) {
		_modifierName = modifierName;
	}
	String getModifierName() {
		return _modifierName;
	}

	void allowCategories(TypedArray<String> categories);
	void setAdditiveMod(float additive) {
		_additiveMod = additive;
	}
	void setMultiplierMod(float multiplier) {
		_multiplierMod = multiplier;
	}

	bool isRelevant(String modifiedType, TypedArray<String> categories) {
		if(modifiedType != _modifiedType)
			return false;
		if(_modifierCategories.is_empty())
			return true;
		for (int i = 0; i < _modifierCategories.size(); ++i) {
			if (categories.has(_modifierCategories[i]))
				return true;
		}
		return false;
	}
	float getAdditiveModifier()  {
		return _additiveMod;
	}
	float getMultiplierModifier() {
		return _multiplierMod;
	}
};

#endif //GODOT_SOURCE_MODIFIER_H
