#pragma once

#include "../blendalot_animation_graph.h"
#include "../blendalot_state_machine.h"

#include "core/object/class_db.h"
#include "scene/animation/animation_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

struct StateMachineFixture {
	Node *character_node;
	Skeleton3D *skeleton_node;
	AnimationPlayer *player_node;

	int hip_bone_index = -1;

	Ref<Animation> test_animation_a;
	Ref<Animation> test_animation_b;
	Ref<Animation> test_animation_c;
	Ref<Animation> test_animation_sync_a;
	Ref<Animation> test_animation_sync_b;

	Ref<AnimationLibrary> animation_library;

	BLTAnimationGraph *animation_graph;
	StateMachineFixture() {
		BLTAnimationGraph *scene_animation_graph = dynamic_cast<BLTAnimationGraph *>(SceneTree::get_singleton()->get_root()->find_child("SyncedAnimationGraphFixtureTestNode", true, false));

		if (scene_animation_graph == nullptr) {
			setup_test_scene();
		}

		assign_scene_variables();
	}

	void setup_test_scene() {
		character_node = memnew(Node);
		character_node->set_name("CharacterNode");
		SceneTree::get_singleton()->get_root()->add_child(character_node);

		skeleton_node = memnew(Skeleton3D);
		skeleton_node->set_name("Skeleton");
		character_node->add_child(skeleton_node);

		skeleton_node->add_bone("Root");
		hip_bone_index = skeleton_node->add_bone("Hips");

		player_node = memnew(AnimationPlayer);
		player_node->set_name("AnimationPlayer");

		setup_animations();

		SceneTree::get_singleton()->get_root()->add_child(player_node);

		animation_graph = memnew(BLTAnimationGraph);
		animation_graph->set_name("SyncedAnimationGraphFixtureTestNode");
		SceneTree::get_singleton()->get_root()->add_child(animation_graph);

		animation_graph->set_animation_player(player_node->get_path());
		animation_graph->set_skeleton(skeleton_node->get_path());
	}

	void setup_animations() {
		test_animation_a = memnew(Animation);
		int track_index = test_animation_a->add_track(Animation::TYPE_POSITION_3D);
		CHECK(track_index == 0);
		test_animation_a->track_insert_key(track_index, 0.0, Vector3(0., 0., 0.));
		test_animation_a->track_insert_key(track_index, 1.0, Vector3(1., 2., 3.));
		test_animation_a->track_set_path(track_index, NodePath(vformat("%s:%s", skeleton_node->get_path().get_concatenated_names(), "Hips")));
		test_animation_a->set_loop_mode(Animation::LOOP_LINEAR);

		animation_library.instantiate();
		animation_library->add_animation("TestAnimationA", test_animation_a);

		test_animation_b = memnew(Animation);
		track_index = test_animation_b->add_track(Animation::TYPE_POSITION_3D);
		CHECK(track_index == 0);
		test_animation_b->track_insert_key(track_index, 0.0, Vector3(0., 0., 0.));
		test_animation_b->track_insert_key(track_index, 1.0, Vector3(2., 4., 6.));
		test_animation_b->track_set_path(track_index, NodePath(vformat("%s:%s", skeleton_node->get_path().get_concatenated_names(), "Hips")));
		test_animation_b->set_loop_mode(Animation::LOOP_LINEAR);

		animation_library->add_animation("TestAnimationB", test_animation_b);

		test_animation_c = memnew(Animation);
		track_index = test_animation_c->add_track(Animation::TYPE_POSITION_3D);
		CHECK(track_index == 0);
		test_animation_c->track_insert_key(track_index, 0.0, Vector3(0., 0., 0.));
		test_animation_c->track_insert_key(track_index, 3.0, Vector3(2., 4., 6.));
		test_animation_c->track_set_path(track_index, NodePath(vformat("%s:%s", skeleton_node->get_path().get_concatenated_names(), "Hips")));
		test_animation_c->set_loop_mode(Animation::LOOP_LINEAR);

		animation_library->add_animation("TestAnimationC", test_animation_c);

		test_animation_sync_a = memnew(Animation);
		track_index = test_animation_sync_a->add_track(Animation::TYPE_POSITION_3D);
		CHECK(track_index == 0);
		test_animation_sync_a->track_insert_key(track_index, 0.0, Vector3(0., 0., 0.));
		test_animation_sync_a->track_insert_key(track_index, 0.4, Vector3(1., 2., 3.));
		test_animation_sync_a->set_length(2.0);
		test_animation_sync_a->track_set_path(track_index, NodePath(vformat("%s:%s", skeleton_node->get_path().get_concatenated_names(), "Hips")));
		test_animation_sync_a->add_marker("0", 0.0);
		test_animation_sync_a->add_marker("1", 0.4);
		test_animation_sync_a->track_set_interpolation_type(track_index, Animation::INTERPOLATION_LINEAR);
		test_animation_sync_a->set_loop_mode(Animation::LOOP_LINEAR);

		animation_library->add_animation("TestAnimationSyncA", test_animation_sync_a);

		test_animation_sync_b = memnew(Animation);
		track_index = test_animation_sync_b->add_track(Animation::TYPE_POSITION_3D);
		CHECK(track_index == 0);
		test_animation_sync_b->track_insert_key(track_index, 0.1, Vector3(2., 4., 6.));
		test_animation_sync_b->track_insert_key(track_index, 0.2, Vector3(0., 0., 0.));
		test_animation_sync_b->set_length(1.0);
		test_animation_sync_b->track_set_path(track_index, NodePath(vformat("%s:%s", skeleton_node->get_path().get_concatenated_names(), "Hips")));
		test_animation_sync_b->add_marker("1", 0.1);
		test_animation_sync_b->add_marker("0", 0.2);
		test_animation_sync_b->track_set_interpolation_type(track_index, Animation::INTERPOLATION_LINEAR);
		test_animation_sync_b->set_loop_mode(Animation::LOOP_LINEAR);

		animation_library->add_animation("TestAnimationSyncB", test_animation_sync_b);

		player_node->add_animation_library("animation_library", animation_library);
	}

	void assign_scene_variables() {
		animation_graph = dynamic_cast<BLTAnimationGraph *>(SceneTree::get_singleton()->get_root()->find_child("SyncedAnimationGraphFixtureTestNode", true, false));
		REQUIRE(animation_graph);
		character_node = (SceneTree::get_singleton()->get_root()->find_child("CharacterNode", true, false));
		REQUIRE(character_node != nullptr);
		skeleton_node = dynamic_cast<Skeleton3D *>((SceneTree::get_singleton()->get_root()->find_child("Skeleton", true, false)));
		REQUIRE(skeleton_node != nullptr);
		player_node = dynamic_cast<AnimationPlayer *>((SceneTree::get_singleton()->get_root()->find_child("AnimationPlayer", true, false)));
		REQUIRE(player_node != nullptr);

		skeleton_node->reset_bone_poses();
		hip_bone_index = skeleton_node->find_bone("Hips");
		REQUIRE(hip_bone_index > -1);

		animation_library = player_node->get_animation_library("animation_library");
		REQUIRE(animation_library.is_valid());

		test_animation_a = animation_library->get_animation("TestAnimationA");
		REQUIRE(test_animation_a.is_valid());
		test_animation_b = animation_library->get_animation("TestAnimationB");
		REQUIRE(test_animation_b.is_valid());
		test_animation_sync_a = animation_library->get_animation("TestAnimationSyncA");
		REQUIRE(test_animation_sync_a.is_valid());
		test_animation_sync_b = animation_library->get_animation("TestAnimationSyncB");
		REQUIRE(test_animation_sync_b.is_valid());
	}
};

namespace TestBlendalotStateMachine {

TEST_CASE_FIXTURE(StateMachineFixture, "[SceneTree][Blendalot][StateMachine] Non-synced evaluation") {
	Ref<BLTStateMachine> state_machine;
	state_machine.instantiate();

	Ref<BLTAnimationNodeSampler> animation_sampler_node_a;
	animation_sampler_node_a.instantiate();
	animation_sampler_node_a->animation_name = "animation_library/TestAnimationA";

	Ref<BLTAnimationNodeSampler> animation_sampler_node_b;
	animation_sampler_node_b.instantiate();
	animation_sampler_node_b->animation_name = "animation_library/TestAnimationB";
	state_machine->add_state(animation_sampler_node_a);
	state_machine->add_state(animation_sampler_node_b);

	Ref<BLTStateMachineTransition> transition;
	transition.instantiate();
	state_machine->add_transition(animation_sampler_node_a, animation_sampler_node_b, transition);
	transition->set_use_sync(false);

	state_machine->initialize(animation_graph->get_context());
	GraphEvaluationContext &graph_context = animation_graph->get_context();

	// Perform evaluation
	double delta = 0.1;

	graph_context.graph_process_delta_time = delta;
	AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
	state_machine->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->update_time(delta);
	state_machine->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	CHECK(transition->get_transition_time() == 0);
	CHECK(animation_sampler_node_a->node_time_info.position == doctest::Approx(0.1));

	// activate transition
	transition->set_transition_duration(0.2);
	transition->force_transition(true);

	// Perform evaluation
	delta = 0.1;
	graph_context.graph_process_delta_time = delta;
	state_machine->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->update_time(delta);
	state_machine->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	CHECK(transition->get_transition_time() == 0.1);
	CHECK(animation_sampler_node_a->node_time_info.position == doctest::Approx(0.2));
	CHECK(animation_sampler_node_b->node_time_info.position == doctest::Approx(0.1));

	// Perform evaluation to the end of the transition. This should already skip update of the previous state.
	delta = 0.1;
	graph_context.graph_process_delta_time = delta;
	state_machine->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->update_time(delta);
	state_machine->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	CHECK(transition->get_transition_time() == 0.2);
	CHECK(state_machine->get_active_transition().is_null());

	CHECK(animation_sampler_node_a->node_time_info.position == doctest::Approx(0.2));
	CHECK(animation_sampler_node_b->node_time_info.position == doctest::Approx(0.2));

	// Perform another evaluation that should only update the new active state.
	delta = 0.1;
	graph_context.graph_process_delta_time = delta;
	state_machine->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->update_time(delta);
	state_machine->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	CHECK(transition->get_transition_time() == 0.2);
	CHECK(state_machine->get_active_transition().is_null());

	CHECK(animation_sampler_node_a->node_time_info.position == doctest::Approx(0.2));
	CHECK(animation_sampler_node_b->node_time_info.position == doctest::Approx(0.3));
}

TEST_CASE_FIXTURE(StateMachineFixture, "[SceneTree][Blendalot][StateMachine] StateMachine modification") {
	Ref<BLTStateMachine> state_machine;
	state_machine.instantiate();

	Ref<BLTAnimationNodeSampler> sampler_a;
	sampler_a.instantiate();
	sampler_a->set_name("Sampler A");
	sampler_a->set_animation("animation_library/TestAnimationA");

	Ref<BLTAnimationNodeSampler> sampler_b;
	sampler_b.instantiate();
	sampler_b->set_name("Sampler B");
	sampler_b->set_animation("animation_library/TestAnimationB");

	Ref<BLTAnimationNodeSampler> sampler_c;
	sampler_c.instantiate();
	sampler_c->set_name("Sampler C");
	sampler_c->set_animation("animation_library/TestAnimationC");

	state_machine->add_state(sampler_a);
	state_machine->add_state(sampler_b);
	state_machine->add_state(sampler_c);

	state_machine->add_state(state_machine);

	animation_graph->set_root_animation_node(state_machine);

	state_machine->initialize(animation_graph->get_context());
	GraphEvaluationContext &graph_context = animation_graph->get_context();

	REQUIRE(state_machine->find_state_index_by_name("Sampler A") == 0);
	REQUIRE(state_machine->find_state_index_by_name("Sampler B") == 1);
	REQUIRE(state_machine->find_state_index_by_name("Sampler C") == 2);

	REQUIRE(state_machine->get_state_names_as_typed_array().size() == 3);

	Ref<BLTStateMachineTransition> transition_a_b;
	transition_a_b.instantiate();
	CHECK(BLTStateMachine::TRANSITION_OK == state_machine->add_transition(sampler_a, sampler_b, transition_a_b));
	CHECK(BLTStateMachine::TRANSITION_ERROR_ALREADY_EXISTS == state_machine->add_transition(sampler_a, sampler_b, transition_a_b));
	CHECK(transition_a_b == state_machine->get_transition_by_index(state_machine->find_transition_index(sampler_a, sampler_b)));

	Ref<BLTStateMachineTransition> transition_b_a;
	transition_b_a.instantiate();
	CHECK(BLTStateMachine::TRANSITION_OK == state_machine->add_transition(sampler_b, sampler_a, transition_b_a));
	CHECK(transition_b_a == state_machine->get_transition_by_index(state_machine->find_transition_index(sampler_b, sampler_a)));

	Ref<BLTStateMachineTransition> transition_c_a;
	transition_c_a.instantiate();
	CHECK(BLTStateMachine::TRANSITION_OK == state_machine->add_transition(sampler_c, sampler_a, transition_c_a));
	int transition_c_a_index = state_machine->find_transition_index(sampler_c, sampler_a);

	// When removing b -> a the index of c -> a should change but the transition should still be available.
	state_machine->remove_transition(sampler_b, sampler_a);
	CHECK(state_machine->find_transition_index(sampler_b, sampler_a) == -1);
	int transition_c_a_index_new = state_machine->find_transition_index(sampler_c, sampler_a);
	CHECK(transition_c_a_index_new != -1);
	CHECK(transition_c_a_index_new != transition_c_a_index);

	// When removing Sampler B the transition a -> b should be removed automatically
	state_machine->remove_state(sampler_b);
	CHECK(state_machine->find_transition_index(sampler_a, sampler_b) == -1);
	CHECK(state_machine->find_state_index_by_name("Sampler A") == 0);
	CHECK(state_machine->find_state_index_by_name("Sampler B") == -1);
	CHECK(state_machine->find_state_index_by_name("Sampler C") == 1);

	// Removing it again should not crash when evaluating graph.
	CHECK(state_machine->get_entry_state() == sampler_a);
	// No evaluation has happened, therefore active state is null.
	CHECK(state_machine->get_active_state().is_null());
	// Perform evaluation
	double delta = 0.1;
	graph_context.graph_process_delta_time = delta;
	AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
	state_machine->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->update_time(delta);
	state_machine->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	CHECK(state_machine->get_active_state() == sampler_a);

	// Remove entry and active state. This should set the entry state to the first available state.
	state_machine->remove_state(sampler_a);

	CHECK(state_machine->get_entry_state() == sampler_c);
	CHECK(state_machine->get_active_state() == sampler_c);

	// Next evaluation
	state_machine->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->update_time(delta);
	state_machine->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	CHECK(state_machine->get_entry_state() == sampler_c);
	CHECK(state_machine->get_active_state() == sampler_c);
}

TEST_CASE_FIXTURE(StateMachineFixture, "[SceneTree][Blendalot][StateMachine] Embedded state machines") {
	Ref<BLTStateMachine> embedded_state_machine;
	embedded_state_machine.instantiate();

	Ref<BLTStateMachine> state_machine;
	state_machine.instantiate();

	Ref<BLTAnimationNodeSampler> animation_sampler_node_a;
	animation_sampler_node_a.instantiate();
	animation_sampler_node_a->animation_name = "animation_library/TestAnimationA";

	embedded_state_machine->add_state(animation_sampler_node_a);
	state_machine->add_state(embedded_state_machine);

	animation_graph->set_root_animation_node(state_machine);

	state_machine->initialize(animation_graph->get_context());
	GraphEvaluationContext &graph_context = animation_graph->get_context();

	// Perform evaluation
	double delta = 0.1;

	graph_context.graph_process_delta_time = delta;
	AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
	state_machine->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
	state_machine->update_time(delta);
	state_machine->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

	CHECK(animation_sampler_node_a->node_time_info.position == doctest::Approx(0.1));
}

} //namespace TestBlendalotStateMachine