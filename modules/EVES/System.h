#ifndef SYSTEM_H
#define SYSTEM_H

#include "BaseValue.h"
#include "FlatValue.h"
#include "SumValue.h"
#include "Entity.h"
#include "ValueWrappers.h"

#include <core/io/resource.h>
#include <core/templates/oa_hash_map.h>
#include <core/variant/typed_array.h>

namespace eves {

class EntityFlatValueContainer;
class EntitySumValueContainer;

class System : public RefCounted {
	GDCLASS(System, RefCounted)

	friend EntityFlatValueContainer;
	friend EntitySumValueContainer;
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	~System();
	EntityFlatValueContainer *CreateEntityFlatValueContainer(const String& valueName);
	EntitySumValueContainer *CreateEntitySumValueContainer(const String& valueName);
	void AddEntityToSystem(Entity* entity);
	void RemoveEntityFromSystem(Entity* entity);

private:
	PagedAllocator<FlatValue> _flatValueAllocator;
	PagedAllocator<SumValue> _sumValueAllocator;
	LocalVector<EntityFlatValueContainer*> _entityFlatValueContainers;
	LocalVector<EntitySumValueContainer*> _entitySumValueContainers;
};



class SystemResource : public Resource {
	GDCLASS(SystemResource, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

	TypedArray<FloatTemplate> _staticTemplateFloatValues;
	TypedArray<FloatTemplate> _dynamicTemplateFloatValues;
public:

};





class EntityFlatValueContainer : public Object {
	GDCLASS(EntityFlatValueContainer, Object)
	friend System;
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_value_resource_for_entity", "entity_id"), &EntityFlatValueContainer::GetValueResourceForEntity);
		ClassDB::bind_method(D_METHOD("get_value_for_entity", "entity_id"), &EntityFlatValueContainer::GetValueForEntity);
		ClassDB::bind_method(D_METHOD("set_value_for_entity", "entity_id", "value"), &EntityFlatValueContainer::SetValueForEntity);
		ClassDB::bind_method(D_METHOD("increment_value_for_entity", "entity_id", "increment_value"), &EntityFlatValueContainer::IncrementValueForEntity);
	}

private:
	void Initialize(System *system, const String& valueName) {
		_system = system;
		_valueName = valueName;
	}

	void Uninitialize() {
		auto iter = _entityValues.iter();
		while(iter.valid) {
			_system->_flatValueAllocator.free(*iter.value);
			iter = _entityValues.next_iter(iter);
		}
		_entityValues.clear();
		_system = nullptr;
	}

	FlatValue* AllocValueForEntity(Entity* entity) {
		ERR_FAIL_COND_V_MSG(_entityValues.has(entity->GetID()), nullptr, "Entity already has a valid BaseValue");
		FlatValue* flatValue = _system->_flatValueAllocator.alloc();
		entity->GetValueCollection()->AddNamedValue(_valueName, flatValue);
		_entityValues.insert(entity->GetID(), flatValue);
		return flatValue;
	}

	void FreeValueForEntity(Entity* entity) {
		if (FlatValue * foundValue; _entityValues.lookup(entity->GetID(), foundValue)) {
			_system->_flatValueAllocator.free(foundValue);
			_entityValues.remove(entity->GetID());
		}
		else
			ERR_FAIL_MSG("Entity does not have a value to free.");
	}

public:
	FlatValue *GetValuePointerForEntity(EntityID entityID) const {
		if (FlatValue * foundValue; _entityValues.lookup(entityID, foundValue))
			return foundValue;
		ERR_FAIL_V_MSG(nullptr, "No value for entity saved");
	}

	float GetValueForEntity(EntityID entityID) const {
		if (FlatValue * foundValue; _entityValues.lookup(entityID, foundValue))
			return foundValue->GetValue();
		ERR_FAIL_V_MSG(0, "No value for entity saved");
	}

	Ref<FlatValueResource> GetValueResourceForEntity(EntityID entityID) const {
		if (FlatValue * foundValue; _entityValues.lookup(entityID, foundValue)) {
			Ref<FlatValueResource> flatValueRes;
			flatValueRes.instantiate();
			flatValueRes->InitAsNonOwner(foundValue);
			return flatValueRes;
		}
		ERR_FAIL_V_MSG(nullptr, "No value for entity saved");
	}

	void SetValueForEntity(EntityID entityID, float setValue) const {
		if (FlatValue * foundValue; _entityValues.lookup(entityID, foundValue))
			foundValue->SetValue(setValue);
		else
			ERR_FAIL_MSG("No value for entity saved");
	}
	
	void IncrementValueForEntity(EntityID entityID, float increment) const {
		if(increment == 0)
			return;
		if (FlatValue * foundValue; _entityValues.lookup(entityID, foundValue))
			foundValue->SetValue(foundValue->GetValue() + increment);
		else
			ERR_FAIL_MSG("No value for entity saved");
	}
private:
	String _valueName;
	System* _system;
	OAHashMap<EntityID,FlatValue*> _entityValues;
};



class EntitySumValueContainer : public Object {
	GDCLASS(EntitySumValueContainer, Object)
	friend System;
protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_value_resource_for_entity", "entity_id"), &EntitySumValueContainer::GetValueResourceForEntity);
		ClassDB::bind_method(D_METHOD("get_value_for_entity", "entity_id"), &EntitySumValueContainer::GetValueForEntity);
	}

private:
	void Initialize(System *system, const String& valueName) {
		_system = system;
		_valueName = valueName;
	}

	void Uninitialize() {
		auto iter = _entityValues.iter();
		while(iter.valid) {
			_system->_sumValueAllocator.free(*iter.value);
			iter = _entityValues.next_iter(iter);
		}
		_entityValues.clear();
		_system = nullptr;
	}

	SumValue* AllocValueForEntity(Entity *entity) {
		ERR_FAIL_COND_V_MSG(_entityValues.has(entity->GetID()), nullptr, "EntityID already has a valid BaseValue");
		SumValue* sumValue = _system->_sumValueAllocator.alloc();
		entity->GetValueCollection()->AddNamedValue(_valueName, sumValue);
		_entityValues.insert(entity->GetID(), sumValue);
		return sumValue;
	}

	void FreeValueForEntity(Entity *entity) {
		if (SumValue * foundValue; _entityValues.lookup(entity->GetID(), foundValue)) {
			_system->_sumValueAllocator.free(foundValue);
			_entityValues.remove(entity->GetID());
		}
		else
			ERR_FAIL_MSG("Entity does not have a value to free.");
	}

public:
	SumValue *GetValuePointerForEntity(EntityID entityID) const {
		if (SumValue * foundValue; _entityValues.lookup(entityID, foundValue))
			return foundValue;
		ERR_FAIL_V_MSG(nullptr, "No value for entity saved");
	}

	float GetValueForEntity(EntityID entityID) const {
		if (SumValue * foundValue; _entityValues.lookup(entityID, foundValue))
			return foundValue->GetValue();
		ERR_FAIL_V_MSG(0, "No value for entity saved");
	}

	Ref<SumValueResource> GetValueResourceForEntity(EntityID entityID) const {
		if (SumValue * foundValue; _entityValues.lookup(entityID, foundValue)) {
			Ref<SumValueResource> sumValueRes;
			sumValueRes.instantiate();
			sumValueRes->InitAsNonOwner(foundValue);
			return sumValueRes;
		}
		ERR_FAIL_V_MSG(nullptr, "No value for entity saved");
	}
private:
	String _valueName;
	System* _system;
	OAHashMap<EntityID,SumValue*> _entityValues;
};

} //namespace eves

#endif //SYSTEM_H
