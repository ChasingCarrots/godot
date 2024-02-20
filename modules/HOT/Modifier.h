#ifndef GODOT_SOURCE_MODIFIER_H
#define GODOT_SOURCE_MODIFIER_H

#include "core/variant/typed_array.h"

#include <core/object/ref_counted.h>
#include <core/profiling.h>
#include <core/templates/oa_hash_map.h>

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
	uint32_t _modifierCategoriesHash = 0;

	struct RelevanceLists {
		HashSet<uint32_t> Relevant;
		HashSet<uint32_t> Irrelevant;
	};
	static OAHashMap<uint32_t, RelevanceLists> CategoriesRelevance;
	static bool AreCategoriesRelevant(uint32_t ownHash, const PackedStringArray& ownCategories,
		uint32_t otherHash, const PackedStringArray& otherCategories)
	{
		PROFILE_FUNCTION();
		if (RelevanceLists *lists = CategoriesRelevance.lookup_ptr(ownHash); lists != nullptr) {
			if(lists->Relevant.has(otherHash))
				return true;
			if(lists->Irrelevant.has(otherHash))
				return false;
			for (const auto& ownString : ownCategories) {
				if (otherCategories.has(ownString)) {
					lists->Relevant.insert(otherHash);
					return true;
				}
			}
			lists->Irrelevant.insert(otherHash);
			return false;
		}
		RelevanceLists newLists;
		bool isRelevant = false;
		for (const auto& ownString : ownCategories) {
			if (otherCategories.has(ownString)) {
				isRelevant = true;
				newLists.Relevant.insert(otherHash);
				break;
			}
		}
		if(!isRelevant) newLists.Irrelevant.insert(otherHash);
		CategoriesRelevance.insert(ownHash, newLists);
		return isRelevant;
	}

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

	static uint32_t CalculateCategoriesHash(const PackedStringArray& categories) {
		// we specifically don't want the order to matter, so we'll just add the hashes
		uint32_t hash = 0;
		for(const auto& s : categories)
			hash += s.hash();
		return hash;
	}

	bool isRelevant(const String& modifiedType, uint32_t categoriesHash, const PackedStringArray& categories) const {
		PROFILE_FUNCTION("Modifier::isRelevant");
		if(modifiedType != _modifiedType)
			return false;
		if(_modifierCategoriesHash == categoriesHash || _modifierCategoriesHash == 0)
			return true;
		if(categoriesHash == 0)
			return false;
		return AreCategoriesRelevant(_modifierCategoriesHash, _modifierCategories,
			categoriesHash, categories);
	}
	float getAdditiveModifier()  {
		return _additiveMod;
	}
	float getMultiplierModifier() {
		return _multiplierMod;
	}
};

#endif //GODOT_SOURCE_MODIFIER_H
