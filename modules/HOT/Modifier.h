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
	StringName _modifiedType;
	StringName _modifierName = "Unnamed Modifier";
	Variant _gameObject;
	Vector<StringName> _modifierCategories;
	mutable uint32_t _hash = 0;

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	~Modifier();

	static Modifier *create(const StringName &modifiedType, const Variant &gameObject) {
		Modifier *mod = memnew(Modifier);
		mod->_init(modifiedType, gameObject);
		return mod;
	}
	void _init(const StringName &modifiedType, const Variant &gameObject);

	void setModifierName(const StringName &modifierName) {
		_modifierName = modifierName;
		_hash = 0;
	}
	StringName getModifierName() {
		return _modifierName;
	}

	uint32_t hash() const {
		if (_hash != 0)
			return _hash;
		_hash = _modifiedType.hash();
		_hash *= 7919;
		_hash ^= hash_murmur3_one_float(_additiveMod);
		_hash *= 7717;
		_hash ^= hash_murmur3_one_float(_multiplierMod);
		_hash *= 7753;
		for (const auto &s : _modifierCategories) {
			_hash *= 6121;
			_hash ^= s.hash();
		}
		return _hash;
	}

	void allowCategories(TypedArray<StringName> categories);
	void setAdditiveMod(float additive) {
		_additiveMod = additive;
		_hash = 0;
	}
	void setMultiplierMod(float multiplier) {
		_multiplierMod = multiplier;
		_hash = 0;
	}

	bool isRelevant(const StringName &modifiedType, const PackedStringArray &categories) const {
		PROFILE_FUNCTION_NAMED("Modifier::isRelevant");
		if (modifiedType != _modifiedType)
			return false;
		if (_modifierCategories.is_empty())
			return true;
		for (int i = 0; i < _modifierCategories.size(); ++i) {
			if (categories.has(_modifierCategories[i]))
				return true;
		}
		return false;
	}
	float getAdditiveModifier() {
		return _additiveMod;
	}
	float getMultiplierModifier() {
		return _multiplierMod;
	}
};

#endif //GODOT_SOURCE_MODIFIER_H
