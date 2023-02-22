#include "register_types.h"

#include <core/object/class_db.h>
#include "Locator.h"
#include "LocatorSystem.h"
#include "GameObject.h"
#include "ModifiedValues.h"

void initialize_HOT_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<LocatorSystem>();
	ClassDB::register_class<Locator>();
	ClassDB::register_class<GameObject>();
	ClassDB::register_class<ModifiedIntValue>();
	ClassDB::register_class<ModifiedFloatValue>();
}

void uninitialize_HOT_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}