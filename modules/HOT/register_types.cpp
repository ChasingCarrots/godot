#include "register_types.h"

#include <core/object/class_db.h>
#include "Locator.h"
#include "LocatorSystem.h"
#include "GameObject.h"
#include "ModifiedValues.h"
#include "Modifier.h"
#include "Statistics.h"
#include "LocatorbasedColliderHelper.h"

void initialize_HOT_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<LocatorSystem>();
	ClassDB::register_class<Locator>();
	ClassDB::register_class<GameObject>();
	ClassDB::register_class<ModifiedIntValue>();
	ClassDB::register_class<ModifiedFloatValue>();
	ClassDB::register_class<Modifier>();
	ClassDB::register_class<Stats>();
	ClassDB::register_class<LocatorbasedColliderHelper>();
}

void uninitialize_HOT_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}