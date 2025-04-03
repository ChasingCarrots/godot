#include "register_types.h"

#include "ValueWrappers.h"
#include "ValueCollection.h"
#include "Entity.h"
#include "System.h"
#include "World.h"
#include "EntityTemplate.h"


void initialize_EVES_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<eves::FlatValueResource>();
	ClassDB::register_class<eves::SumValueResource>();
	ClassDB::register_class<eves::System>();
	ClassDB::register_class<eves::SystemResource>();
	ClassDB::register_class<eves::TemplateValuesFromSystemResource>();
	ClassDB::register_class<eves::EntityFlatValueContainer>();
	ClassDB::register_class<eves::EntitySumValueContainer>();
}

void uninitialize_EVES_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
