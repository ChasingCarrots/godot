#include "register_types.h"

#include "core/object/class_db.h"
#include "synced_animation_graph.h"

void initialize_blendalot_animgraph_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<SyncedAnimationGraph>();
	ClassDB::register_class<SyncedAnimationNode>();
	ClassDB::register_class<SyncedBlendTree>();
	ClassDB::register_class<AnimationSamplerNode>();
	ClassDB::register_class<AnimationBlend2Node>();
}

void uninitialize_blendalot_animgraph_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// Nothing to do here in this example.
}