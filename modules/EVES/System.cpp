#include "System.h"

namespace eves {

void System::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_flat_value_container_for_entities"), &System::CreateEntityFlatValueContainer);
	ClassDB::bind_method(D_METHOD("create_sum_value_container_for_entities"), &System::CreateEntityFlatValueContainer);
}

System::~System() {
	for (uint32_t i = 0; i < _entityFlatValueContainers.size(); ++i) {
		_entityFlatValueContainers[i]->Uninitialize();
		memfree(_entityFlatValueContainers[i]);
	}
	_entityFlatValueContainers.clear();
	for (uint32_t i = 0; i < _entitySumValueContainers.size(); ++i) {
		_entitySumValueContainers[i]->Uninitialize();
		memfree(_entitySumValueContainers[i]);
	}
	_entitySumValueContainers.clear();
}

EntityFlatValueContainer *System::CreateEntityFlatValueContainer(const String& valueName) {
	EntityFlatValueContainer* newContainer = memnew(EntityFlatValueContainer);
	newContainer->Initialize(this, valueName);
	_entityFlatValueContainers.push_back(newContainer);
	return newContainer;
}

EntitySumValueContainer *System::CreateEntitySumValueContainer(const String &valueName) {
	EntitySumValueContainer *newContainer = memnew(EntitySumValueContainer);
	newContainer->Initialize(this, valueName);
	_entitySumValueContainers.push_back(newContainer);
	return newContainer;
}

void System::AddEntityToSystem(Entity *entity) {
	for (uint32_t i = 0; i < _entityFlatValueContainers.size(); ++i) {
		_entityFlatValueContainers[i]->AllocValueForEntity(entity);
	}
	for (uint32_t i = 0; i < _entitySumValueContainers.size(); ++i) {
		_entitySumValueContainers[i]->AllocValueForEntity(entity);
	}
}

void System::RemoveEntityFromSystem(Entity *entity) {
	for (uint32_t i = 0; i < _entityFlatValueContainers.size(); ++i) {
		_entityFlatValueContainers[i]->FreeValueForEntity(entity);
	}
	for (uint32_t i = 0; i < _entitySumValueContainers.size(); ++i) {
		_entitySumValueContainers[i]->FreeValueForEntity(entity);
	}
}

void SystemResource::_bind_methods() {

}

} //namespace eves
