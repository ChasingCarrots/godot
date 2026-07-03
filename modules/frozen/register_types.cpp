#include "register_types.h"

#include "CommunicationLine.h"
#include "CommunicationLineSystem.h"
#include "FutureValue.h"
#include "CompositeNode.h"
#include "CompositeNodeValue.h"
#include "CompositeNodeModules.h"
#include "SynchronizedArray.h"
#include "VoxelGrid.h"
#include "ConstraintElement.h"
#include "ConstraintRule.h"
#include "ConstraintProblem.h"
#include "ConstraintSolution.h"
#include "ConstraintSolver.h"


void initialize_frozen_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<CommunicationLineSystem>();
	ClassDB::register_class<CommunicationLine>();
	ClassDB::register_class<CommunicationCallWithAnswer>();
	ClassDB::register_class<FutureValue>();
	ClassDB::register_class<CompositeNode>();
	ClassDB::register_class<CompositeNodeModule>();
	ClassDB::register_class<CompositeNodeModule3D>();
	ClassDB::register_class<CompositeNodeModuleCharacterBody3D>();
	ClassDB::register_class<CompositeNodeModuleArea3D>();
	ClassDB::register_class<CompositeNodeValue>();
	ClassDB::register_class<SynchronizedArray>();
	ClassDB::register_class<VoxelGrid>();

	// Finite-domain constraint solver for procedural generation.
	ClassDB::register_class<ConstraintInterface>();
	ClassDB::register_class<ConstraintElement>();
	ClassDB::register_abstract_class<ConstraintRule>();
	ClassDB::register_class<ConstraintCountByTag>();
	ClassDB::register_class<ConstraintRequiresTagBefore>();
	ClassDB::register_class<ConstraintTagConnectsToTag>();
	ClassDB::register_class<ConstraintMaxConsecutiveTag>();
	ClassDB::register_class<ConstraintMaxAdjacentByTag>();
	ClassDB::register_class<ConstraintConnectionCountByTag>();
	ClassDB::register_class<ConstraintNeighborTagAllowed>();
	ClassDB::register_class<ConstraintMaxRepeatsByElementId>();
	ClassDB::register_class<ConstraintNoRepeatNeighborElement>();
	ClassDB::register_class<ConstraintLeafCountByTag>();
	ClassDB::register_class<ConstraintTagCompatibility>();
	ClassDB::register_class<ConstraintTagReachability>();
	ClassDB::register_class<ConstraintGeometry>();
	ClassDB::register_class<ConstraintCallback>();
	ClassDB::register_class<ConstraintSlot>();
	ClassDB::register_class<ConstraintProblem>();
	ClassDB::register_class<ConstraintSolution>();
	ClassDB::register_class<ConstraintSolver>();
}

void uninitialize_frozen_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}