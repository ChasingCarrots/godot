#include "register_types.h"

#include <core/object/class_db.h>
#include "Locator.h"
#include "LocatorSystem.h"

void initialize_HOT_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<LocatorSystem>();
	ClassDB::register_class<Locator>();
}

void uninitialize_HOT_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}