#ifndef GODOT_SOURCE_MODIFIER_H
#define GODOT_SOURCE_MODIFIER_H

#include "core/variant/typed_array.h"

#include <core/object/ref_counted.h>
#include <core/profiling.h>

class GameObject;

class Modifier : public RefCounted {
	GDCLASS(Modifier, RefCounted)

	friend GameObject;

	float _additiveMod = 0;
	float _multiplierMod = 0;
	String _modifiedType;
	String _modifierName = "Unnamed Modifier";
	Variant _gameObject;
	PackedStringArray _modifierCategories;
	mutable uint32_t _hash = 0;

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	~Modifier();

	static Modifier* create(String modifiedType, Variant gameObject) {
		Modifier* mod = memnew(Modifier);
		mod->_init(modifiedType, gameObject);
		return mod;
	}
	void _init(String modifiedType, Variant gameObject);

	void setModifierName(String modifierName) {
		_modifierName = modifierName;
		_hash = 0;
	}
	String getModifierName() {
		return _modifierName;
	}

	uint32_t hash() const {
		if(_hash != 0)
			return _hash;
		_hash = _modifiedType.hash();
		_hash += 6763 * hash_murmur3_one_float(_additiveMod);
		_hash += 7489 * hash_murmur3_one_float(_multiplierMod);
		// we specifically don't want the order to matter, so we'll just use xor
		for(const auto& s : _modifierCategories)
			_hash ^= s.hash();
		return _hash;
	}

	void allowCategories(TypedArray<String> categories);
	void setAdditiveMod(float additive) {
		_additiveMod = additive;
		_hash = 0;
	}
	void setMultiplierMod(float multiplier) {
		_multiplierMod = multiplier;
		_hash = 0;
	}

	bool isRelevant(const String& modifiedType, const PackedStringArray& categories) const {
		PROFILE_FUNCTION("Modifier::isRelevant");
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
