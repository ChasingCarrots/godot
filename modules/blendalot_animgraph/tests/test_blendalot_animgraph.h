#pragma once

#include "../blendalot_animation_graph.h"
#include "../blendalot_blend_tree.h"
#include "../blendalot_state_machine.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "scene/animation/animation_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

struct BlendTreeFixture {
	Node *character_node;
	Skeleton3D *skeleton_node;
	AnimationPlayer *player_node;

	int hip_bone_index = -1;

	Ref<Animation> test_animation_a;
	Ref<Animation> test_animation_b;
	Ref<Animation> test_animation_c;
	Ref<Animation> test_animation_sync_a;
	Ref<Animation> test_animation_sync_b;
	Ref<Animation> test_animation_sync_c;

	Ref<AnimationLibrary> animation_library;

	BLTAnimationGraph *animation_graph;
	BlendTreeFixture() {
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

		test_animation_sync_c = memnew(Animation);
		track_index = test_animation_sync_c->add_track(Animation::TYPE_POSITION_3D);
		CHECK(track_index == 0);
		test_animation_sync_c->track_insert_key(track_index, 0.3, Vector3(0., 2., 3.));
		test_animation_sync_c->track_insert_key(track_index, 0.8, Vector3(-10., 0., -2.));
		test_animation_sync_c->set_length(1.5);
		test_animation_sync_c->track_set_path(track_index, NodePath(vformat("%s:%s", skeleton_node->get_path().get_concatenated_names(), "Hips")));
		test_animation_sync_c->add_marker("0", 0.3);
		test_animation_sync_c->add_marker("1", 0.8);
		test_animation_sync_c->track_set_interpolation_type(track_index, Animation::INTERPOLATION_LINEAR);
		test_animation_sync_c->set_loop_mode(Animation::LOOP_LINEAR);

		animation_library->add_animation("TestAnimationSyncC", test_animation_sync_c);

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

namespace TestBlendalotAnimationGraph {

TEST_CASE("[Blendalot][BlendTree] Test BlendTree construction") {
	BLTBlendTree::BLTBlendTreeGraph tree_constructor;

	Ref<BLTAnimationNodeSampler> animation_sampler_node0;
	animation_sampler_node0.instantiate();
	animation_sampler_node0->set_name("Sampler0");
	tree_constructor.add_node(animation_sampler_node0);

	Ref<BLTAnimationNodeSampler> animation_sampler_node1;
	animation_sampler_node1.instantiate();
	animation_sampler_node1->set_name("Sampler1");
	tree_constructor.add_node(animation_sampler_node1);

	Ref<BLTAnimationNodeSampler> animation_sampler_node2;
	animation_sampler_node2.instantiate();
	animation_sampler_node2->set_name("Sampler2");
	tree_constructor.add_node(animation_sampler_node2);

	Ref<BLTAnimationNodeBlend2> node_blend0;
	node_blend0.instantiate();
	node_blend0->set_name("Blend0");
	tree_constructor.add_node(node_blend0);

	Ref<BLTAnimationNodeBlend2> node_blend1;
	node_blend1.instantiate();
	node_blend1->set_name("Blend1");
	tree_constructor.add_node(node_blend1);

	// Tree
	// Sampler0 -\
	// Sampler1 -+ Blend0 -\
	// Sampler2 -----------+ Blend1 - Output

	CHECK(BLTBlendTree::CONNECTION_OK == tree_constructor.add_connection(animation_sampler_node0, node_blend0, "Input0"));

	// Ensure that subtree is properly updated
	int sampler0_index = tree_constructor.find_node_index(animation_sampler_node0);
	int blend0_index = tree_constructor.find_node_index(node_blend0);
	CHECK(tree_constructor.node_connection_info[blend0_index].input_subtree_node_indices.has(sampler0_index));

	// Connect blend0 to blend1
	CHECK(BLTBlendTree::CONNECTION_OK == tree_constructor.add_connection(node_blend0, node_blend1, "Input0"));

	// Creating a loop must fail
	CHECK(BLTBlendTree::CONNECTION_ERROR_CONNECTION_CREATES_LOOP == tree_constructor.add_connection(node_blend1, node_blend0, "Input1"));

	// Correct connection of Sampler1 to Blend0
	CHECK(BLTBlendTree::CONNECTION_OK == tree_constructor.add_connection(animation_sampler_node1, node_blend0, "Input1"));

	// Connecting to an already connected port must fail
	CHECK(BLTBlendTree::CONNECTION_ERROR_TARGET_PORT_ALREADY_CONNECTED == tree_constructor.add_connection(animation_sampler_node2, node_blend0, "Input0"));

	// Ensure that subtree is properly updated
	int sampler1_index = tree_constructor.find_node_index(animation_sampler_node0);
	int blend1_index = tree_constructor.find_node_index(node_blend1);
	CHECK(tree_constructor.node_connection_info[blend1_index].input_subtree_node_indices.has(sampler1_index));
	CHECK(tree_constructor.node_connection_info[blend1_index].input_subtree_node_indices.has(sampler0_index));
	CHECK(tree_constructor.node_connection_info[blend1_index].input_subtree_node_indices.has(blend0_index));

	// Perform remaining connections
	CHECK(BLTBlendTree::CONNECTION_OK == tree_constructor.add_connection(node_blend1, tree_constructor.get_output_node(), "Output"));
	CHECK(BLTBlendTree::CONNECTION_OK == tree_constructor.add_connection(animation_sampler_node2, node_blend1, "Input1"));

	// Output node must have all nodes in its subtree:
	CHECK(tree_constructor.node_connection_info[0].input_subtree_node_indices.has(1));
	CHECK(tree_constructor.node_connection_info[0].input_subtree_node_indices.has(2));
	CHECK(tree_constructor.node_connection_info[0].input_subtree_node_indices.has(3));
	CHECK(tree_constructor.node_connection_info[0].input_subtree_node_indices.has(4));
	CHECK(tree_constructor.node_connection_info[0].input_subtree_node_indices.has(5));

	tree_constructor.sort_nodes_and_references();

	// Check that for node i all input nodes have a node index j >= i (i is part of the subtree)
	for (unsigned int i = 0; i < tree_constructor.nodes.size(); i++) {
		for (int input_index : tree_constructor.node_connection_info[i].input_subtree_node_indices) {
			CHECK(input_index >= i);
		}
	}
}

TEST_CASE_FIXTURE(BlendTreeFixture, "[SceneTree][Blendalot] Test AnimationData blending") {
	AnimationData data_t0;
	data_t0.allocate_track_values(test_animation_a, skeleton_node);
	data_t0.sample_from_animation(test_animation_a, skeleton_node, 0.0);

	AnimationData data_t1;
	data_t1.allocate_track_values(test_animation_a, skeleton_node);
	data_t1.sample_from_animation(test_animation_a, skeleton_node, 1.0);

	AnimationData data_t0_5;
	data_t0_5.allocate_track_values(test_animation_a, skeleton_node);
	data_t0_5.sample_from_animation(test_animation_a, skeleton_node, 0.5);

	AnimationData data_blended = data_t0;
	data_blended.blend(data_t1, 0.5);

	REQUIRE(data_blended.has_same_tracks(data_t0_5));
	for (const KeyValue<Animation::TrackCacheID, size_t> &K : data_blended.value_buffer_offset) {
		AnimationData::TrackValue *blended_value = data_blended.get_value<AnimationData::TrackValue>(K.key);
		AnimationData::TrackValue *data_t0_5_value = data_t0_5.get_value<AnimationData::TrackValue>(K.key);
		CHECK(*blended_value == *data_t0_5_value);
	}

	// And also check that values do not match
	data_blended = data_t0;
	data_blended.blend(data_t1, 0.3);

	REQUIRE(data_blended.has_same_tracks(data_t0_5));
	for (const KeyValue<Animation::TrackCacheID, size_t> &K : data_blended.value_buffer_offset) {
		AnimationData::TrackValue *blended_value = data_blended.get_value<AnimationData::TrackValue>(K.key);
		AnimationData::TrackValue *data_t0_5_value = data_t0_5.get_value<AnimationData::TrackValue>(K.key);
		CHECK(*blended_value != *data_t0_5_value);
	}
}

TEST_CASE_FIXTURE(BlendTreeFixture, "[SceneTree][Blendalot] SyncedAnimationGraph evaluation with an AnimationSampler as root node") {
	Ref<BLTAnimationNodeSampler> animation_sampler_node;
	animation_sampler_node.instantiate();
	animation_sampler_node->animation_name = "animation_library/TestAnimationA";

	animation_graph->set_root_animation_node(animation_sampler_node);

	Vector3 hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

	CHECK(hip_bone_position.x == doctest::Approx(0.0));
	CHECK(hip_bone_position.y == doctest::Approx(0.0));
	CHECK(hip_bone_position.z == doctest::Approx(0.0));

	SceneTree::get_singleton()->process(0.01);

	hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

	CHECK(hip_bone_position.x == doctest::Approx(0.01));
	CHECK(hip_bone_position.y == doctest::Approx(0.02));
	CHECK(hip_bone_position.z == doctest::Approx(0.03));
}

TEST_CASE_FIXTURE(BlendTreeFixture, "[SceneTree][Blendalot][BlendTree] BlendTree evaluation with a AnimationSamplerNode connected to the output") {
	Ref<BLTBlendTree> synced_blend_tree_node;
	synced_blend_tree_node.instantiate();

	Ref<BLTAnimationNodeSampler> animation_sampler_node;
	animation_sampler_node.instantiate();
	animation_sampler_node->animation_name = "animation_library/TestAnimationA";

	synced_blend_tree_node->add_node(animation_sampler_node);
	REQUIRE(BLTBlendTree::CONNECTION_OK == synced_blend_tree_node->add_connection(animation_sampler_node, synced_blend_tree_node->get_output_node(), "Output"));

	synced_blend_tree_node->initialize(animation_graph->get_context());

	animation_graph->set_root_animation_node(synced_blend_tree_node);

	Vector3 hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

	CHECK(hip_bone_position.x == doctest::Approx(0.0));
	CHECK(hip_bone_position.y == doctest::Approx(0.0));
	CHECK(hip_bone_position.z == doctest::Approx(0.0));

	SceneTree::get_singleton()->process(0.01);

	hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

	CHECK(hip_bone_position.x == doctest::Approx(0.01));
	CHECK(hip_bone_position.y == doctest::Approx(0.02));
	CHECK(hip_bone_position.z == doctest::Approx(0.03));
}

TEST_CASE_FIXTURE(BlendTreeFixture, "[SceneTree][Blendalot][BlendTree][Blend2Node] BlendTree evaluation with a Blend2Node connected to the output") {
	Ref<BLTBlendTree> synced_blend_tree_node;
	synced_blend_tree_node.instantiate();

	// TestAnimationA
	Ref<BLTAnimationNodeSampler> animation_sampler_node_a;
	animation_sampler_node_a.instantiate();
	animation_sampler_node_a->animation_name = "animation_library/TestAnimationA";

	synced_blend_tree_node->add_node(animation_sampler_node_a);

	// TestAnimationB
	Ref<BLTAnimationNodeSampler> animation_sampler_node_b;
	animation_sampler_node_b.instantiate();
	animation_sampler_node_b->animation_name = "animation_library/TestAnimationB";

	synced_blend_tree_node->add_node(animation_sampler_node_b);

	// Blend2
	Ref<BLTAnimationNodeBlend2> blend2_node;
	blend2_node.instantiate();
	blend2_node->set_name("Blend2");
	blend2_node->blend_weight = 0.5;
	blend2_node->sync = false;

	synced_blend_tree_node->add_node(blend2_node);

	// Connect nodes
	Vector<StringName> blend2_inputs = blend2_node->get_input_names();
	REQUIRE(BLTBlendTree::CONNECTION_OK == synced_blend_tree_node->add_connection(animation_sampler_node_a, blend2_node, blend2_inputs[0]));
	REQUIRE(BLTBlendTree::CONNECTION_OK == synced_blend_tree_node->add_connection(animation_sampler_node_b, blend2_node, blend2_inputs[1]));
	REQUIRE(BLTBlendTree::CONNECTION_OK == synced_blend_tree_node->add_connection(blend2_node, synced_blend_tree_node->get_output_node(), "Output"));

	synced_blend_tree_node->initialize(animation_graph->get_context());

	int blend2_node_index = synced_blend_tree_node->find_node_index(blend2_node);
	const BLTBlendTree::NodeRuntimeData &blend2_runtime_data = synced_blend_tree_node->_node_runtime_data[blend2_node_index];

	CHECK(blend2_runtime_data.input_nodes[0] == animation_sampler_node_a);
	CHECK(blend2_runtime_data.input_nodes[1] == animation_sampler_node_b);

	animation_graph->set_root_animation_node(synced_blend_tree_node);

	SUBCASE("Perform default evaluation") {
		Vector3 hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(0.0));
		CHECK(hip_bone_position.y == doctest::Approx(0.0));
		CHECK(hip_bone_position.z == doctest::Approx(0.0));

		SceneTree::get_singleton()->process(0.5);

		hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(0.75));
		CHECK(hip_bone_position.y == doctest::Approx(1.5));
		CHECK(hip_bone_position.z == doctest::Approx(2.25));
	}

	SUBCASE("Evaluate tree such that animations get looped") {
		Vector3 hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(0.0));
		CHECK(hip_bone_position.y == doctest::Approx(0.0));
		CHECK(hip_bone_position.z == doctest::Approx(0.0));

		SceneTree::get_singleton()->process(1.2);

		hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(0.3));
		CHECK(hip_bone_position.y == doctest::Approx(0.6));
		CHECK(hip_bone_position.z == doctest::Approx(0.9));
	}

	SUBCASE("Inputs are deactivated when blend weight is near 0 or 1") {
		GraphEvaluationContext &graph_context = animation_graph->get_context();
		AnimationData *graph_output = graph_context.animation_data_allocator.allocate();

		// Blend weight 0
		blend2_node->blend_weight = 0.;
		synced_blend_tree_node->initialize(graph_context);
		synced_blend_tree_node->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());

		CHECK(animation_sampler_node_a->active == true);
		CHECK(animation_sampler_node_b->active == false);

		synced_blend_tree_node->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		synced_blend_tree_node->update_time(0.1);
		synced_blend_tree_node->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		// Blend weight 1
		blend2_node->blend_weight = 1.;
		synced_blend_tree_node->initialize(graph_context);
		synced_blend_tree_node->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());

		CHECK(animation_sampler_node_a->active == false);
		CHECK(animation_sampler_node_b->active == true);

		synced_blend_tree_node->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		synced_blend_tree_node->update_time(0.1);
		synced_blend_tree_node->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);
	}

	SUBCASE("Evaluate synced blend") {
		animation_sampler_node_a->animation_name = "animation_library/TestAnimationSyncA";
		animation_sampler_node_b->animation_name = "animation_library/TestAnimationSyncB";
		blend2_node->sync = true;
		synced_blend_tree_node->initialize(animation_graph->get_context());

		REQUIRE(animation_graph->get_root_animation_node().ptr() == synced_blend_tree_node.ptr());

		// By blending both animations we get a SyncTrack of duration 1.5s with the following
		// intervals:
		//   0: 0.825s
		//   1: 0.675s
		// By updating by 0s we get to the start of interval 0, an update by 0.65s to the start of interval 1, and
		// another update by 0.85 to the start again to interval 0.
		SceneTree::get_singleton()->process(0.);

		Vector3 hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(0.0));
		CHECK(hip_bone_position.y == doctest::Approx(0.0));
		CHECK(hip_bone_position.z == doctest::Approx(0.0));

		// By updating again by 0.825s we get loop to the start of the 0th interval where
		// both TrackValues are zero.
		SceneTree::get_singleton()->process(0.825);

		hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(1.5));
		CHECK(hip_bone_position.y == doctest::Approx(3.0));
		CHECK(hip_bone_position.z == doctest::Approx(4.5));

		// By updating again by 0.675s we loop to the start of the 0th interval where both
		// TrackValues are zero.
		SceneTree::get_singleton()->process(0.675);

		hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(0.0));
		CHECK(hip_bone_position.y == doctest::Approx(0.0));
		CHECK(hip_bone_position.z == doctest::Approx(0.0));
	}

	SUBCASE("Evaluate synced blend followed by a node deactivation due to weight = 1.0f") {
		animation_sampler_node_a->animation_name = "animation_library/TestAnimationSyncA";
		animation_sampler_node_b->animation_name = "animation_library/TestAnimationSyncB";
		blend2_node->sync = true;
		synced_blend_tree_node->initialize(animation_graph->get_context());

		REQUIRE(animation_graph->get_root_animation_node().ptr() == synced_blend_tree_node.ptr());

		blend2_node->blend_weight = 0.1f;
		// Interval durations:
		//   0:  0.45
		//   1: 1.45s

		// Update to just after the switch to interval 1
		SceneTree::get_singleton()->process(0.51301);
		CHECK(animation_sampler_node_a->node_time_info.sync_position == doctest::Approx(1.0));
		CHECK(animation_sampler_node_a->node_time_info.position == doctest::Approx(0.4));
		CHECK(animation_sampler_node_b->node_time_info.sync_position == doctest::Approx(1.0));
		CHECK(animation_sampler_node_b->node_time_info.position == doctest::Approx(0.1));

		blend2_node->blend_weight = 1.0f;
		// Interval durations:
		//   0: 0.9
		//   1: 0.1

		// An infinitesimal update should not change much in the sampling position.
		SceneTree::get_singleton()->process(0.0001);
		CHECK(animation_sampler_node_b->node_time_info.sync_position == doctest::Approx(1.000).epsilon(0.01));
		CHECK(animation_sampler_node_b->node_time_info.position == doctest::Approx(0.100).epsilon(0.01));

		// Update to near the end of interval 1
		SceneTree::get_singleton()->process(0.0998);
		CHECK(animation_sampler_node_b->node_time_info.sync_position == doctest::Approx(1.999).epsilon(0.01));
		CHECK(animation_sampler_node_b->node_time_info.position == doctest::Approx(0.2).epsilon(0.01));
	}

	SUBCASE("Save, load and evaluate the SyncedBlendTree") {
		// Test saving and loading of the blend tree to a resource
		ResourceSaver::save(synced_blend_tree_node, "synced_blend_tree_node.tres");

		REQUIRE(ClassDB::class_exists("BLTAnimationNodeSampler"));

		// Load blend tree
		Ref<BLTBlendTree> loaded_synced_blend_tree = ResourceLoader::load("synced_blend_tree_node.tres");
		REQUIRE(loaded_synced_blend_tree.is_valid());

		Ref<BLTAnimationNodeBlend2> loaded_blend2_node = loaded_synced_blend_tree->get_node_by_index(loaded_synced_blend_tree->find_node_index_by_name("Blend2"));
		REQUIRE(loaded_blend2_node.is_valid());
		CHECK(loaded_blend2_node->sync == false);
		CHECK(loaded_blend2_node->blend_weight == blend2_node->blend_weight);

		loaded_synced_blend_tree->initialize(animation_graph->get_context());
		animation_graph->set_root_animation_node(loaded_synced_blend_tree);

		// Re-evaluate using a different time. All animation samplers will start again from 0.
		SceneTree::get_singleton()->process(0.2);

		Vector3 hip_bone_position = skeleton_node->get_bone_global_pose(hip_bone_index).origin;

		CHECK(hip_bone_position.x == doctest::Approx(0.3));
		CHECK(hip_bone_position.y == doctest::Approx(0.6));
		CHECK(hip_bone_position.z == doctest::Approx(0.9));
	}
}

TEST_CASE_FIXTURE(BlendTreeFixture, "[SceneTree][Blendalot][BlendTreeGraph][ChangeConnectivity] BlendTreeGraph with various nodes and connections that are removed") {
	BLTBlendTree::BLTBlendTreeGraph blend_tree_graph;

	// TestAnimationA
	Ref<BLTAnimationNodeSampler> animation_sampler_node_a;
	animation_sampler_node_a.instantiate();
	animation_sampler_node_a->set_name("SamplerNodeA");
	animation_sampler_node_a->animation_name = "animation_library/TestAnimationA";

	blend_tree_graph.add_node(animation_sampler_node_a);

	// TestAnimationB
	Ref<BLTAnimationNodeSampler> animation_sampler_node_b;
	animation_sampler_node_b.instantiate();
	animation_sampler_node_b->set_name("SamplerNodeB");
	animation_sampler_node_b->animation_name = "animation_library/TestAnimationB";

	blend_tree_graph.add_node(animation_sampler_node_b);

	// TestAnimationB
	Ref<BLTAnimationNodeSampler> animation_sampler_node_c;
	animation_sampler_node_c.instantiate();
	animation_sampler_node_c->set_name("SamplerNodeC");
	animation_sampler_node_c->animation_name = "animation_library/TestAnimationC";

	blend_tree_graph.add_node(animation_sampler_node_c);

	// Blend2A
	Ref<BLTAnimationNodeBlend2> blend2_node_a;
	blend2_node_a.instantiate();
	blend2_node_a->set_name("Blend2A");
	blend2_node_a->blend_weight = 0.5;
	blend2_node_a->sync = false;

	blend_tree_graph.add_node(blend2_node_a);

	// Blend2B
	Ref<BLTAnimationNodeBlend2> blend2_node_b;
	blend2_node_b.instantiate();
	blend2_node_b->set_name("Blend2A");
	blend2_node_b->blend_weight = 0.5;
	blend2_node_b->sync = false;

	blend_tree_graph.add_node(blend2_node_b);

	// Connect nodes: Subgraph Output, Blend2A, SamplerA
	REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_graph.add_connection(blend2_node_a, blend_tree_graph.get_output_node(), "Output"));
	REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_graph.add_connection(animation_sampler_node_a, blend2_node_a, "Input0"));

	// Connect nodes: Subgraph Blend2A, SamplerB, SamplerC
	REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_graph.add_connection(animation_sampler_node_b, blend2_node_b, "Input0"));
	REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_graph.add_connection(animation_sampler_node_c, blend2_node_b, "Input1"));

	SUBCASE("Add and remove a connection") {
		HashSet<int> subgraph_output_initial(blend_tree_graph.node_connection_info[0].input_subtree_node_indices);
		HashSet<int> subgraph_blend2a_initial(blend_tree_graph.node_connection_info[blend_tree_graph.find_node_index(blend2_node_a)].input_subtree_node_indices);
		HashSet<int> subgraph_blend2b_initial(blend_tree_graph.node_connection_info[blend_tree_graph.find_node_index(blend2_node_b)].input_subtree_node_indices);

		// Add and remove connection
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_graph.add_connection(blend2_node_b, blend2_node_a, "Input1"));
		blend_tree_graph.remove_connection(blend2_node_b, blend2_node_a, "Input1");
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_graph.is_connection_valid(blend2_node_b, blend2_node_a, "Input1"));

		// Check that we have the same subgraphs as before the connection
		CHECK(subgraph_output_initial == blend_tree_graph.node_connection_info[0].input_subtree_node_indices);
		CHECK(subgraph_blend2a_initial == blend_tree_graph.node_connection_info[blend_tree_graph.find_node_index(blend2_node_a)].input_subtree_node_indices);
		CHECK(subgraph_blend2b_initial == blend_tree_graph.node_connection_info[blend_tree_graph.find_node_index(blend2_node_b)].input_subtree_node_indices);

		// Check that the connection is not present anymore.
		for (const BLTBlendTreeConnection &connection : blend_tree_graph.connections) {
			bool connection_equals_removed_connection = connection.source_node == blend2_node_b && connection.target_node == blend2_node_a && connection.target_port_name == "Input1";
			CHECK(connection_equals_removed_connection == false);
		}
	}

	SUBCASE("Remove a node") {
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_graph.add_connection(blend2_node_b, blend2_node_a, "Input1"));

		int animation_sampler_node_b_index_pre_remove = blend_tree_graph.find_node_index(animation_sampler_node_b);
		int blend2_node_a_index_pre_remove = blend_tree_graph.find_node_index(blend2_node_a);
		int blend2_node_b_index_pre_remove = blend_tree_graph.find_node_index(blend2_node_b);

		CHECK(blend_tree_graph.node_connection_info[0].input_subtree_node_indices.size() == 6);
		CHECK(blend_tree_graph.node_connection_info[blend2_node_a_index_pre_remove].input_subtree_node_indices.size() == 5);

		SUBCASE("Removing the output node does nothing") {
			int num_nodes = blend_tree_graph.nodes.size();
			int num_connections = blend_tree_graph.connections.size();
			CHECK(blend_tree_graph.remove_node(blend_tree_graph.get_output_node()) == false);
			CHECK(blend_tree_graph.connections.size() == num_connections);
			CHECK(blend_tree_graph.nodes.size() == num_nodes);
		}

		SUBCASE("Remove a node with no children") {
			blend_tree_graph.remove_node(animation_sampler_node_a);

			for (const BLTBlendTreeConnection &connection : blend_tree_graph.connections) {
				bool is_connection_with_removed_node = connection.source_node == animation_sampler_node_a || connection.target_node == animation_sampler_node_a;
				CHECK(is_connection_with_removed_node == false);
			}

			int animation_sampler_node_b_index_post_remove = blend_tree_graph.find_node_index(animation_sampler_node_b);
			int blend2_node_a_index_post_remove = blend_tree_graph.find_node_index(blend2_node_a);
			int blend2_node_b_index_post_remove = blend_tree_graph.find_node_index(blend2_node_b);

			CHECK(blend_tree_graph.find_node_index(animation_sampler_node_a) == -1);
			CHECK(blend2_node_b_index_post_remove == blend2_node_b_index_pre_remove - 1);
			CHECK(animation_sampler_node_b_index_post_remove == animation_sampler_node_b_index_pre_remove - 1);

			CHECK(blend_tree_graph.node_connection_info[0].input_subtree_node_indices.size() == 5);
			CHECK(blend_tree_graph.node_connection_info[blend2_node_a_index_post_remove].input_subtree_node_indices.size() == 4);
			CHECK(blend_tree_graph.node_connection_info[blend2_node_a_index_post_remove].connected_child_node_index_at_port[0] == -1);
			CHECK(blend_tree_graph.node_connection_info[blend2_node_a_index_post_remove].connected_child_node_index_at_port[1] == blend2_node_b_index_post_remove);
			CHECK(blend_tree_graph.node_connection_info[blend2_node_b_index_post_remove].input_subtree_node_indices.has(blend2_node_b_index_post_remove));
			CHECK(blend_tree_graph.node_connection_info[blend2_node_b_index_post_remove].input_subtree_node_indices.has(animation_sampler_node_b_index_post_remove));
		}

		SUBCASE("Remove a node with parent and children") {
			int num_nodes = blend_tree_graph.nodes.size();
			blend_tree_graph.remove_node(blend2_node_a);
			blend_tree_graph.sort_nodes_and_references();

			CHECK(blend_tree_graph.nodes.size() == num_nodes - 1);

			for (const BLTBlendTreeConnection &connection : blend_tree_graph.connections) {
				bool is_connection_with_removed_node = connection.source_node == blend2_node_a || connection.target_node == blend2_node_a;
				CHECK(is_connection_with_removed_node == false);
			}

			int animation_sampler_node_b_index_post_remove = blend_tree_graph.find_node_index(animation_sampler_node_b);
			int animation_sampler_node_c_index_post_remove = blend_tree_graph.find_node_index(animation_sampler_node_c);
			int blend2_node_b_index_post_remove = blend_tree_graph.find_node_index(blend2_node_b);

			CHECK(blend_tree_graph.find_node_index(blend2_node_a) == -1);
			CHECK(blend2_node_b_index_post_remove == blend2_node_b_index_pre_remove - 1);
			CHECK(animation_sampler_node_b_index_post_remove == animation_sampler_node_b_index_pre_remove);

			CHECK(blend_tree_graph.node_connection_info[0].input_subtree_node_indices.size() == 1);
			CHECK(blend_tree_graph.node_connection_info[blend2_node_b_index_post_remove].input_subtree_node_indices.size() == 3);
			CHECK(blend_tree_graph.node_connection_info[blend2_node_b_index_post_remove].input_subtree_node_indices.has(blend2_node_b_index_post_remove));
			CHECK(blend_tree_graph.node_connection_info[blend2_node_b_index_post_remove].input_subtree_node_indices.has(animation_sampler_node_b_index_post_remove));
			CHECK(blend_tree_graph.node_connection_info[blend2_node_b_index_post_remove].input_subtree_node_indices.has(animation_sampler_node_c_index_post_remove));
		}
	}

	SUBCASE("Check evaluation of graph with modified connections") {
		Ref<BLTBlendTree> blend_tree_node;
		blend_tree_node.instantiate();
		blend_tree_node->add_node(animation_sampler_node_a);
		blend_tree_node->add_node(animation_sampler_node_b);
		blend_tree_node->add_node(animation_sampler_node_c);
		blend_tree_node->add_node(blend2_node_a);

		animation_graph->set_root_animation_node(blend_tree_node);
		GraphEvaluationContext &graph_context = animation_graph->get_context();
		CHECK(blend_tree_node->initialize(graph_context) == false);

		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(animation_sampler_node_a, blend_tree_node->get_output_node(), "Output"));
		CHECK(blend_tree_node->initialize(graph_context) == true);

		AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
		blend_tree_node->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree_node->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree_node->update_time(0.825);
		blend_tree_node->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);
		CHECK(animation_sampler_node_a->active == false);

		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(animation_sampler_node_b, blend2_node_a, "Input0"));
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(animation_sampler_node_c, blend2_node_a, "Input1"));

		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->remove_connection(animation_sampler_node_a, blend_tree_node->get_output_node(), "Output"));

		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(blend2_node_a, blend_tree_node->get_output_node(), "Output"));
		CHECK(blend_tree_node->initialize(graph_context) == true);

		blend_tree_node->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree_node->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree_node->update_time(0.825);
		blend_tree_node->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);
	}

	SUBCASE("Check node removal when evaluation order of nodes changes.") {
		Ref<BLTBlendTree> blend_tree_node;
		blend_tree_node.instantiate();

		// Add nodes such that blend2 node has index 1
		blend_tree_node->add_node(blend2_node_a);
		blend_tree_node->add_node(animation_sampler_node_a);
		blend_tree_node->add_node(animation_sampler_node_b);
		blend_tree_node->add_node(animation_sampler_node_c);

		// Connect samplers b and c to blend2_node_a
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(animation_sampler_node_b, blend2_node_a, "Input0"));
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(animation_sampler_node_c, blend2_node_a, "Input1"));

		// But then connect sampler a to the output => Reordering with sampler a now at index 1
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(animation_sampler_node_a, blend_tree_node->get_output_node(), "Output"));

		// Remove the sampler a from output => Invalid graph
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->remove_connection(animation_sampler_node_a, blend_tree_node->get_output_node(), "Output"));

		// Connect blend2_node_a to output => Reordering with blend2_node at index 1
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->add_connection(blend2_node_a, blend_tree_node->get_output_node(), "Output"));

		// Remove sampler c => should not cause issues but used to crash due to invalid parent index
		REQUIRE(BLTBlendTree::CONNECTION_OK == blend_tree_node->remove_connection(animation_sampler_node_c, blend2_node_a, "Input1"));
	}
}

TEST_CASE_FIXTURE(BlendTreeFixture, "[SceneTree][Blendalot][BlendTreeGraph][EmbeddedBlendTree] BlendTree with an embedded BlendTree subgraph") {
	// Embedded BlendTree
	Ref<BLTBlendTree> embedded_blend_tree;
	embedded_blend_tree.instantiate();

	// TestAnimationB
	Ref<BLTAnimationNodeSampler> animation_sampler_node_b;
	animation_sampler_node_b.instantiate();

	embedded_blend_tree->add_node(animation_sampler_node_b);
	embedded_blend_tree->add_connection(animation_sampler_node_b, embedded_blend_tree->get_output_node(), "Output");

	Ref<BLTBlendTree> blend_tree;
	blend_tree.instantiate();

	// Blend2
	Ref<BLTAnimationNodeBlend2> blend2;
	blend2.instantiate();
	blend2->set_name("Blend2");
	blend_tree->add_node(blend2);

	// TestAnimationA
	Ref<BLTAnimationNodeSampler> animation_sampler_node_a;
	animation_sampler_node_a.instantiate();

	blend_tree->add_node(animation_sampler_node_a);

	blend_tree->add_node(embedded_blend_tree);

	blend_tree->add_connection(animation_sampler_node_a, blend2, "Input0");
	blend_tree->add_connection(embedded_blend_tree, blend2, "Input1");
	blend_tree->add_connection(blend2, blend_tree->get_output_node(), "Output");

	SUBCASE("Perform regular blend") {
		animation_sampler_node_b->animation_name = "animation_library/TestAnimationB";
		animation_sampler_node_a->animation_name = "animation_library/TestAnimationA";
		blend2->blend_weight = 0.5;
		blend2->sync = false;

		// Trigger initialization
		animation_graph->set_root_animation_node(blend_tree);
		GraphEvaluationContext &graph_context = animation_graph->get_context();
		REQUIRE(blend_tree->initialize(graph_context));

		// Perform evaluation
		AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
		blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->update_time(0.1);
		blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		// Check values
		AnimationData::TransformTrackValue *hip_transform_value = graph_output->get_value<AnimationData::TransformTrackValue>(test_animation_a->get_tracks()[0]->get_unique_id());
		CHECK(hip_transform_value->loc[0] == doctest::Approx(0.15));
		CHECK(hip_transform_value->loc[1] == doctest::Approx(0.3));
		CHECK(hip_transform_value->loc[2] == doctest::Approx(0.45));
	}
	SUBCASE("Perform synced blend") {
		animation_sampler_node_b->animation_name = "animation_library/TestAnimationSyncA";
		animation_sampler_node_a->animation_name = "animation_library/TestAnimationSyncB";
		blend2->blend_weight = 0.5;
		blend2->sync = true;

		// Trigger initialization
		animation_graph->set_root_animation_node(blend_tree);
		GraphEvaluationContext &graph_context = animation_graph->get_context();
		REQUIRE(blend_tree->initialize(graph_context));

		// Perform evaluation
		AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
		blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->update_time(0.825);
		blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		// Check values
		AnimationData::TransformTrackValue *hip_transform_value = graph_output->get_value<AnimationData::TransformTrackValue>(test_animation_a->get_tracks()[0]->get_unique_id());
		CHECK(hip_transform_value->loc[0] == doctest::Approx(1.5));
		CHECK(hip_transform_value->loc[1] == doctest::Approx(3.0));
		CHECK(hip_transform_value->loc[2] == doctest::Approx(4.5));
	}
}

TEST_CASE_FIXTURE(BlendTreeFixture, "[SceneTree][Blendalot][BlendTree][StateMachine] BlendTree with an embedded StateMachine subgraph") {
	Ref<BLTBlendTree> blend_tree;
	blend_tree.instantiate();

	// Blend2
	Ref<BLTAnimationNodeBlend2> blend2;
	blend2.instantiate();
	blend2->set_name("Blend2");
	blend_tree->add_node(blend2);

	// TestAnimationA
	Ref<BLTAnimationNodeSampler> animation_sampler_node_a;
	animation_sampler_node_a.instantiate();
	animation_sampler_node_a->set_name("AnimA");

	blend_tree->add_node(animation_sampler_node_a);

	// Embedded StateMachine
	Ref<BLTStateMachine> embedded_state_machine;
	embedded_state_machine.instantiate();
	embedded_state_machine->set_name("StateMachine");

	// TestAnimationB
	Ref<BLTAnimationNodeSampler> animation_sampler_node_b;
	animation_sampler_node_b.instantiate();
	animation_sampler_node_b->set_name("AnimB");

	// TestAnimationC
	Ref<BLTAnimationNodeSampler> animation_sampler_node_c;
	animation_sampler_node_c.instantiate();
	animation_sampler_node_c->set_name("AnimC");

	embedded_state_machine->add_state(animation_sampler_node_b);
	embedded_state_machine->add_state(animation_sampler_node_c);
	Ref<BLTStateMachineTransition> transition_b_c;
	transition_b_c.instantiate();
	embedded_state_machine->add_transition(animation_sampler_node_b, animation_sampler_node_c, transition_b_c);

	Ref<BLTStateMachineTransition> transition_c_b;
	transition_c_b.instantiate();
	embedded_state_machine->add_transition(animation_sampler_node_c, animation_sampler_node_b, transition_c_b);

	blend_tree->add_node(embedded_state_machine);

	blend_tree->add_connection(animation_sampler_node_a, blend2, "Input0");
	blend_tree->add_connection(embedded_state_machine, blend2, "Input1");
	blend_tree->add_connection(blend2, blend_tree->get_output_node(), "Output");

	SUBCASE("Blending Sampler and State Machine") {
		animation_sampler_node_a->animation_name = "animation_library/TestAnimationSyncA";
		animation_sampler_node_b->animation_name = "animation_library/TestAnimationSyncB";
		animation_sampler_node_c->animation_name = "animation_library/TestAnimationSyncC";
		blend2->blend_weight = 0.5;
		blend2->sync = true;

		// Trigger initialization
		animation_graph->set_root_animation_node(blend_tree);
		GraphEvaluationContext &graph_context = animation_graph->get_context();
		REQUIRE(blend_tree->initialize(graph_context));
		AnimationData *graph_output = graph_context.animation_data_allocator.allocate();

		graph_context.graph_process_delta_time = 0.1;
		blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		// The blend2 node has syncing enabled, therefore all active nodes should have their is_synced flag to be enabled.
		CHECK(animation_sampler_node_a->node_time_info.is_synced);
		CHECK(embedded_state_machine->node_time_info.is_synced);
		CHECK(embedded_state_machine->get_active_state()->node_time_info.is_synced);
		blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		CHECK_EQ(embedded_state_machine->node_time_info.sync_track.duration, animation_sampler_node_b->node_time_info.sync_track.duration);
		CHECK_EQ(animation_sampler_node_b->node_time_info.sync_track.duration, 1.0);
		CHECK_EQ(blend2->node_time_info.sync_track.duration, 1.5);

		blend_tree->update_time(graph_context.graph_process_delta_time);
		blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		CHECK_EQ(blend2->node_time_info.sync_track.duration, 1.5);
		CHECK_EQ(blend2->node_time_info.sync_position, embedded_state_machine->node_time_info.sync_position);
		CHECK_EQ(blend2->node_time_info.sync_position, animation_sampler_node_a->node_time_info.sync_position);
		CHECK_EQ(blend2->node_time_info.sync_position, animation_sampler_node_b->node_time_info.sync_position);

		SUBCASE("Activate unsynced transition") {
			// Activate transition
			transition_b_c->set_transition_duration(0.2);
			transition_b_c->sync = false;
			animation_graph->set("parameters/StateMachine/transition/AnimB_to_AnimC/activate", true);

			// To ensure that the is_synced flag is properly set we mess with the state here.
			animation_sampler_node_a->node_time_info.is_synced = false;
			embedded_state_machine->node_time_info.is_synced = false;
			embedded_state_machine->get_active_state()->node_time_info.is_synced = false;

			blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
			CHECK(animation_sampler_node_a->node_time_info.is_synced);
			CHECK(embedded_state_machine->node_time_info.is_synced);
			CHECK(embedded_state_machine->get_active_state()->node_time_info.is_synced);
			CHECK(embedded_state_machine->get_previous_state()->node_time_info.is_synced);
			blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
			blend_tree->update_time(graph_context.graph_process_delta_time);
			blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

			CHECK(embedded_state_machine->node_time_info.is_synced);
			CHECK(animation_sampler_node_b->node_time_info.is_synced);
			CHECK(animation_sampler_node_c->node_time_info.is_synced);

			CHECK_EQ(animation_sampler_node_a->node_time_info.sync_position, blend2->node_time_info.sync_position);
			CHECK_EQ(animation_sampler_node_b->node_time_info.sync_position, blend2->node_time_info.sync_position);
			CHECK_EQ(animation_sampler_node_c->node_time_info.sync_position, blend2->node_time_info.sync_position);
		}

		SUBCASE("Activate synced transition whith non-synced Blend2 in parent graph.") {
			blend2->sync = false;

			// Activate transition
			transition_b_c->set_transition_duration(0.2);
			transition_b_c->sync = true;
			animation_graph->set("parameters/StateMachine/transition/AnimB_to_AnimC/activate", true);

			blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
			CHECK(!embedded_state_machine->node_time_info.is_synced);
			CHECK(animation_sampler_node_b->node_time_info.is_synced);
			CHECK(animation_sampler_node_c->node_time_info.is_synced);
			blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
			blend_tree->update_time(graph_context.graph_process_delta_time);
			blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);
		}

		graph_context.animation_data_allocator.free(graph_output);
	}

	SUBCASE("Non-synchronized transition") {
		animation_sampler_node_a->animation_name = "animation_library/TestAnimationA";
		animation_sampler_node_b->animation_name = "animation_library/TestAnimationB";
		animation_sampler_node_c->animation_name = "animation_library/TestAnimationC";
		blend2->blend_weight = 1.0;
		blend2->sync = false;

		// Trigger initialization
		animation_graph->set_root_animation_node(blend_tree);
		GraphEvaluationContext &graph_context = animation_graph->get_context();
		REQUIRE(blend_tree->initialize(graph_context));
		AnimationData *graph_output = graph_context.animation_data_allocator.allocate();

		// Perform evaluation
		graph_context.graph_process_delta_time = 0.1;
		blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->update_time(graph_context.graph_process_delta_time);
		blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		CHECK(animation_sampler_node_a->node_time_info.position == 0.0);
		CHECK(animation_sampler_node_b->node_time_info.position == 0.1);
		CHECK(animation_sampler_node_c->node_time_info.position == 0.0);

		// Activate transition
		transition_b_c->set_transition_duration(0.2);
		transition_b_c->sync = false;
		animation_graph->set("parameters/StateMachine/transition/AnimB_to_AnimC/activate", true);

		// and evaluation
		blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->update_time(graph_context.graph_process_delta_time);
		blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		CHECK(animation_sampler_node_a->node_time_info.position == 0.0);
		CHECK(animation_sampler_node_b->node_time_info.position == 0.2);
		CHECK(animation_sampler_node_c->node_time_info.position == 0.1);
		CHECK(transition_b_c->blend_weight == doctest::Approx(0.5));

		graph_context.animation_data_allocator.free(graph_output);
	}

	SUBCASE("Synchronized transition") {
		animation_sampler_node_a->animation_name = "animation_library/TestAnimationSyncA";
		animation_sampler_node_b->animation_name = "animation_library/TestAnimationSyncB";
		animation_sampler_node_c->animation_name = "animation_library/TestAnimationSyncC";
		blend2->blend_weight = 1.0;
		blend2->sync = false;

		// Trigger initialization
		animation_graph->set_root_animation_node(blend_tree);
		GraphEvaluationContext &graph_context = animation_graph->get_context();
		REQUIRE(blend_tree->initialize(graph_context));

		// Perform evaluation to the end of sync interval 0 of TestAnimationSyncB
		graph_context.graph_process_delta_time = 0.1;
		AnimationData *graph_output = graph_context.animation_data_allocator.allocate();
		blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		blend_tree->update_time(graph_context.graph_process_delta_time);
		blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		CHECK(animation_sampler_node_a->node_time_info.position == 0.0);
		CHECK(animation_sampler_node_b->node_time_info.position == 0.1);
		CHECK(animation_sampler_node_c->node_time_info.position == 0.0);

		// Activate transition
		transition_b_c->set_transition_duration(0.2);
		transition_b_c->sync = true;
		animation_graph->set("parameters/StateMachine/transition/AnimB_to_AnimC/activate", true);

		// And perform another evaluation

		// After the activation the transition weight must have been updated and animation C must have
		// been time warped to the same sync location as animation B. In addition both animations must
		// be marked as synced from the transition.
		blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
		CHECK_EQ(transition_b_c->blend_weight, 0.5);
		CHECK_EQ(transition_b_c->node_time_info.sync_position, 1.0);
		CHECK_EQ(animation_sampler_node_b->node_time_info.is_synced, true);
		CHECK_EQ(animation_sampler_node_c->node_time_info.is_synced, true);

		// SyncTrack update: the blended sync track must be
		blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
		CHECK_EQ(transition_b_c->node_time_info.position, doctest::Approx(0.6166666 * 1.25));
		CHECK_EQ(transition_b_c->node_time_info.sync_track.duration, doctest::Approx(1.25));
		CHECK_EQ(transition_b_c->node_time_info.sync_track.interval_duration_ratio[0], doctest::Approx(0.6166666));
		CHECK_EQ(transition_b_c->node_time_info.sync_track.interval_duration_ratio[1], doctest::Approx(0.3833333));

		// Time update: we are 0.1s into the second interval.
		blend_tree->update_time(graph_context.graph_process_delta_time);
		CHECK_EQ(transition_b_c->node_time_info.sync_position, doctest::Approx(1.0 + 0.1 / (0.38333 * 1.25)).epsilon(0.0001));
		CHECK_EQ(animation_sampler_node_b->node_time_info.sync_position, transition_b_c->node_time_info.sync_position);
		CHECK_EQ(animation_sampler_node_c->node_time_info.sync_position, transition_b_c->node_time_info.sync_position);

		// Actual evaluation
		blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

		WHEN("Triggering an update right at the end of the transition") {
			graph_context.graph_process_delta_time = 0.099999;
			blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
			blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
			blend_tree->update_time(graph_context.graph_process_delta_time);
			blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);

			// Transition must have been deactivated.
			CHECK(embedded_state_machine->get_active_transition().is_null());
			CHECK_EQ(transition_b_c->blend_weight, doctest::Approx(1.0).epsilon(0.0001));
			CHECK(!animation_sampler_node_c->node_time_info.is_synced);

			// Make another update just past the transition
			double sampler_c_position_end_of_transition = animation_sampler_node_c->node_time_info.position;

			graph_context.graph_process_delta_time = 0.0001;
			blend_tree->activate_inputs(LocalVector<Ref<BLTAnimationNode>>());
			blend_tree->calculate_sync_track(LocalVector<Ref<BLTAnimationNode>>());
			blend_tree->update_time(graph_context.graph_process_delta_time);

			CHECK_EQ(animation_sampler_node_c->node_time_info.position, doctest::Approx(sampler_c_position_end_of_transition).epsilon(0.001));

			blend_tree->evaluate(graph_context, LocalVector<AnimationData *>(), *graph_output);
		}

		graph_context.animation_data_allocator.free(graph_output);
	}
}

} //namespace TestBlendalotAnimationGraph