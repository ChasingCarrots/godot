#include "register_types.h"

#include "CommunicationLine.h"
#include "CommunicationLineSystem.h"
#include "FutureValue.h"
#include "CompositeNode.h"


void initialize_frozen_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<CommunicationLineSystem>();
	ClassDB::register_class<CommunicationLine>();
	ClassDB::register_class<CommunicationCallWithAnswer>();
	ClassDB::register_class<FutureValue>();
	ClassDB::register_class<CompositeNode>();
}

void uninitialize_frozen_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}