#pragma once

#include "blendalot_animation_node.h"

#include "core/object/callable_mp.h"

class BLTBlendTree : public BLTAnimationNode {
	GDCLASS(BLTBlendTree, BLTAnimationNode);

public:
	enum ConnectionError {
		CONNECTION_OK,
		CONNECTION_ERROR_GRAPH_ALREADY_INITIALIZED,
		CONNECTION_ERROR_NO_SOURCE_NODE,
		CONNECTION_ERROR_NO_TARGET_NODE,
		CONNECTION_ERROR_PARENT_EXISTS,
		CONNECTION_ERROR_TARGET_PORT_NOT_FOUND,
		CONNECTION_ERROR_TARGET_PORT_ALREADY_CONNECTED,
		CONNECTION_ERROR_CONNECTION_CREATES_LOOP,
		CONNECTION_ERROR_CONNECTION_NOT_FOUND
	};

	/**
	 * @class BLTBlendTreeGraph
	 * Helper class that is used to build runtime blend trees and also to validate connections.
	 */
	struct BLTBlendTreeGraph {
		struct NodeConnectionInfo {
			int parent_node_index = -1;
			HashSet<int> input_subtree_node_indices; // Contains all nodes down to the tree leaves that influence this node.
			LocalVector<int> connected_child_node_index_at_port; // Contains for each input port the index of the node that is connected to it.

			NodeConnectionInfo() = default;

			explicit NodeConnectionInfo(const BLTAnimationNode *node) {
				parent_node_index = -1;
				for (int i = 0; i < node->get_input_count(); i++) {
					connected_child_node_index_at_port.push_back(-1);
				}
			}

			void apply_node_mapping(const LocalVector<int> &old_to_new_mapping) {
				// Map connected node indices
				for (unsigned int j = 0; j < connected_child_node_index_at_port.size(); j++) {
					int connected_node_index = connected_child_node_index_at_port[j];
					connected_child_node_index_at_port[j] = connected_node_index == -1 ? -1 : old_to_new_mapping[connected_node_index];
				}

				// Map connected subtrees
				HashSet<int> old_indices(input_subtree_node_indices);
				input_subtree_node_indices.clear();
				for (int old_index : old_indices) {
					input_subtree_node_indices.insert(old_index == -1 ? -1 : old_to_new_mapping[old_index]);
				}
			}

			void _print_subtree() const {
				String result = vformat("subtree node indices #%d: ", input_subtree_node_indices.size());
				bool is_first = true;
				for (int index : input_subtree_node_indices) {
					if (is_first) {
						result += vformat("%d", index);
						is_first = false;
					} else {
						result += vformat(", %d", index);
					}
				}
				print_line(result);
			}
		};

		LocalVector<Ref<BLTAnimationNode>> nodes; // All added nodes
		LocalVector<NodeConnectionInfo> node_connection_info;
		LocalVector<BLTBlendTreeConnection> connections;

		BLTBlendTreeGraph();

		Ref<BLTAnimationNode> get_output_node();
		int find_node_index(const Ref<BLTAnimationNode> &node) const;
		int find_node_index_by_name(const StringName &name) const;
		void _print_graph() const;
		void _print_graph_timeinfo() const;

		void sort_nodes_and_references();
		LocalVector<int> get_sorted_node_indices();
		void sort_nodes_recursive(int node_index, LocalVector<int> &result);
		void add_index_and_update_subtrees_recursive(int node_index, int node_parent_index);
		void remove_subtree_and_update_subtrees_recursive(int node, const HashSet<int> &removed_subtree_indices);

		void add_node(const Ref<BLTAnimationNode> &node);
		bool remove_node(const Ref<BLTAnimationNode> &node);

		ConnectionError is_connection_valid(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, StringName target_port_name) const;
		ConnectionError add_connection(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name);
		int find_connection_index(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name) const;
		ConnectionError remove_connection(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name);
	};

private:
	BLTBlendTreeGraph tree_graph;
	bool tree_initialized = false;
	bool is_evaluation_order_dirty = true;
	GraphEvaluationContext *_graph_evaluation_context = nullptr;

	void sort_nodes() {
		_node_runtime_data.clear();
		tree_graph.sort_nodes_and_references();
	}

	void setup_runtime_data() {
		// Add nodes and allocate runtime data
		for (uint32_t i = 0; i < tree_graph.nodes.size(); i++) {
			const Ref<BLTAnimationNode> node = tree_graph.nodes[i];

			NodeRuntimeData node_runtime_data;
			for (int ni = 0; ni < node->get_input_count(); ni++) {
				node_runtime_data.input_data.push_back(nullptr);
			}

			node_runtime_data.output_data = nullptr;
			_node_runtime_data.push_back(node_runtime_data);
		}

		// Populate runtime data (only now is this.nodes populated to retrieve the nodes)
		for (uint32_t i = 0; i < tree_graph.nodes.size(); i++) {
			Ref<BLTAnimationNode> node = tree_graph.nodes[i];
			NodeRuntimeData &node_runtime_data = _node_runtime_data[i];

			for (int port_index = 0; port_index < node->get_input_count(); port_index++) {
				const int connected_node_index = tree_graph.node_connection_info[i].connected_child_node_index_at_port[port_index];
				if (connected_node_index == -1) {
					node_runtime_data.input_nodes.push_back(nullptr);
				} else {
					node_runtime_data.input_nodes.push_back(tree_graph.nodes[connected_node_index]);
				}
			}
		}
	}

	void update_node_evaluation_order() {
		sort_nodes();
		setup_runtime_data();

		is_evaluation_order_dirty = false;
	}

protected:
	static void _bind_methods();
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _get(const StringName &p_name, Variant &r_value) const;
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	Vector2 graph_offset;

	struct NodeRuntimeData {
		LocalVector<Ref<BLTAnimationNode>> input_nodes;
		LocalVector<AnimationData *> input_data;
		AnimationData *output_data = nullptr;
	};
	LocalVector<NodeRuntimeData> _node_runtime_data;

	void set_graph_offset(const Vector2 &p_graph_offset) {
		graph_offset = p_graph_offset;
	}

	Vector2 get_graph_offset() const {
		return graph_offset;
	}

	int find_node_index(const Ref<BLTAnimationNode> &node) const {
		return tree_graph.find_node_index(node);
	}

	int find_node_index_by_name(const StringName &p_name) const {
		return tree_graph.find_node_index_by_name(p_name);
	}

	void add_node(const Ref<BLTAnimationNode> &node) {
		tree_graph.add_node(node);
		node->node_path = vformat("%s/%s", node_path, node->get_name());

		if (_graph_evaluation_context != nullptr) {
			node->initialize(*_graph_evaluation_context);
		}

		node->connect(SNAME("node_changed"), callable_mp(this, &BLTBlendTree::_tree_node_changed));
	}

	void remove_node(const Ref<BLTAnimationNode> &node) {
		if (tree_graph.remove_node(node)) {
			_node_changed();
		}
	}

	TypedArray<StringName> get_node_names_as_typed_array() const {
		Vector<StringName> vec;
		for (const Ref<BLTAnimationNode> &node : tree_graph.nodes) {
			vec.push_back(node->get_name());
		}

		TypedArray<StringName> typed_arr;
		typed_arr.resize(vec.size());
		for (uint32_t i = 0; i < vec.size(); i++) {
			typed_arr[i] = vec[i];
		}
		return typed_arr;
	}

	Ref<BLTAnimationNode> get_node(const StringName &node_name) const {
		int node_index = tree_graph.find_node_index_by_name(node_name);

		if (node_index >= 0) {
			return tree_graph.nodes[node_index];
		}

		return nullptr;
	}

	Ref<BLTAnimationNode> get_node_by_index(int node_index) const {
		if (node_index < 0 || node_index > static_cast<int>(tree_graph.nodes.size())) {
			return nullptr;
		}

		return tree_graph.nodes[node_index];
	}

	Ref<BLTAnimationNode> get_output_node() const {
		return tree_graph.nodes[0];
	}

	ConnectionError is_connection_valid(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name) {
		return tree_graph.is_connection_valid(source_node, target_node, target_port_name);
	}

	ConnectionError add_connection(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name) {
		ConnectionError result = tree_graph.add_connection(source_node, target_node, target_port_name);
		if (result == CONNECTION_OK) {
			is_evaluation_order_dirty = true;
			_node_changed();
		}

		return result;
	}

	ConnectionError remove_connection(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name) {
		ConnectionError result = tree_graph.remove_connection(source_node, target_node, target_port_name);

		if (result == CONNECTION_OK) {
			is_evaluation_order_dirty = true;
			_node_changed();
		}

		return result;
	}

	Array get_connections_as_array() const {
		Array result;
		for (const BLTBlendTreeConnection &connection : tree_graph.connections) {
			result.push_back(connection.source_node);
			result.push_back(connection.target_node);
			result.push_back(connection.target_port_name);
		}

		return result;
	}

	void _tree_node_changed(const StringName &node_name) {
		_node_changed();
	}

	// overrides from BLTAnimationNode
	bool initialize(GraphEvaluationContext &context) override {
		GodotProfileZone("BLTBlendTree::initialize");

		tree_initialized = false;

		if (!BLTAnimationNode::initialize(context)) {
			return false;
		}

		_graph_evaluation_context = &context;

		if (is_evaluation_order_dirty) {
			update_node_evaluation_order();
		}

		const HashSet<int> &output_subtree = tree_graph.node_connection_info[0].input_subtree_node_indices;

		if (output_subtree.size() == 1) {
			context.validation_messages.push_back(vformat("Invalid node %s: Nothing connected to output node in node.", node_path));
			return false;
		}

		for (unsigned int i = 0; i < tree_graph.nodes.size(); i++) {
			const Ref<BLTAnimationNode> &node = tree_graph.nodes[i];

			// Re-apply node name as names of parents may have changed.
			node->node_path = vformat("%s/%s", node_path, node->get_name());

			// Initialize, but skip validation of nodes that are not part of the active tree.
			if (!output_subtree.has(i)) {
				node->initialize(context);
				continue;
			}

			if (!node->initialize(context)) {
				return false;
			}

			const NodeRuntimeData &node_runtime_data = _node_runtime_data[i];

			for (const Ref<BLTAnimationNode> &input_node : node_runtime_data.input_nodes) {
				if (!input_node.is_valid()) {
					return false;
				}
			}
		}

		tree_initialized = true;

		return true;
	}

	void
	activate_inputs(const LocalVector<Ref<BLTAnimationNode>> &input_nodes) override {
		GodotProfileZone("BLTBlendTree::activate_inputs");

		if (tree_graph.nodes.size() == 1) {
			return;
		}

		tree_graph.nodes[0]->active = true;
		tree_graph.nodes[0]->node_time_info.is_synced = node_time_info.is_synced;

		for (uint32_t i = 0; i < tree_graph.nodes.size(); i++) {
			const Ref<BLTAnimationNode> &node = tree_graph.nodes[i];

			if (!node->active) {
				continue;
			}

			const NodeRuntimeData &node_runtime_data = _node_runtime_data[i];
			node->activate_inputs(node_runtime_data.input_nodes);
		}
	}

	void calculate_sync_track(const LocalVector<Ref<BLTAnimationNode>> &input_nodes) override {
		GodotProfileZone("BLTBlendTree::calculate_sync_track");

		for (uint32_t i = tree_graph.nodes.size() - 1; i > 0; i--) {
			const Ref<BLTAnimationNode> &node = tree_graph.nodes[i];

			if (!node->active) {
				continue;
			}

			const NodeRuntimeData &node_runtime_data = _node_runtime_data[i];

			node->calculate_sync_track(node_runtime_data.input_nodes);

			if (i == 1) {
				node_time_info = node->node_time_info;
			}
		}
	}

	void update_time(double p_delta) override {
		GodotProfileZone("BLTBlendTree::update_time");

		BLTAnimationNode::update_time(p_delta);

		tree_graph.nodes[0]->node_time_info.delta = node_time_info.delta;
		tree_graph.nodes[0]->node_time_info.position = node_time_info.position;
		tree_graph.nodes[0]->node_time_info.sync_position = node_time_info.sync_position;

		for (uint32_t i = 1; i < tree_graph.nodes.size(); i++) {
			const Ref<BLTAnimationNode> &node = tree_graph.nodes[i];

			if (!node->active) {
				continue;
			}

			const BLTBlendTreeGraph::NodeConnectionInfo &node_connection_info = tree_graph.node_connection_info[i];
			const Ref<BLTAnimationNode> &node_parent = tree_graph.nodes[node_connection_info.parent_node_index];

			if (node->node_time_info.is_synced) {
				node->update_time(node_parent->node_time_info.sync_position);
			} else {
				node->update_time(node_parent->node_time_info.delta);
			}
		}
	}

	void evaluate(GraphEvaluationContext &context, const LocalVector<AnimationData *> &input_datas, AnimationData &output_data) override {
		GodotProfileZone("BLTBlendTree::evaluate");

		for (uint32_t i = tree_graph.nodes.size() - 1; i > 0; i--) {
			const Ref<BLTAnimationNode> &node = tree_graph.nodes[i];

			if (!node->active) {
				continue;
			}

			NodeRuntimeData &node_runtime_data = _node_runtime_data[i];

			// Populate the inputs
			for (unsigned int j = 0; j < node_runtime_data.input_data.size(); j++) {
				int child_node_index = tree_graph.node_connection_info[i].connected_child_node_index_at_port[j];
				if (tree_graph.nodes[child_node_index]->active) {
					node_runtime_data.input_data[j] = _node_runtime_data[child_node_index].output_data;
				} else {
					node_runtime_data.input_data[j] = nullptr;
				}
			}

			// Set output pointer
			if (i == 1) {
				// We're emitting to the Output node, therefore we can just copy the data to the output AnimationData.
				node_runtime_data.output_data = &output_data;
				node->active = false;
			} else {
				node_runtime_data.output_data = context.animation_data_allocator.allocate();
			}

			node->evaluate(context, node_runtime_data.input_data, *node_runtime_data.output_data);

			// All inputs have been consumed and can now be freed and nodes deactivated.
			for (unsigned int j = 0; j < node_runtime_data.input_data.size(); j++) {
				int child_node_index = tree_graph.node_connection_info[i].connected_child_node_index_at_port[j];
				if (tree_graph.nodes[child_node_index]->active) {
					context.animation_data_allocator.free(_node_runtime_data[child_node_index].output_data);
					tree_graph.nodes[j]->active = false;
				} else {
					node_runtime_data.input_data[j] = nullptr;
				}
			}
		}
	}

	void get_child_nodes(List<Ref<BLTAnimationNode>> *r_child_nodes) const override {
		for (const Ref<BLTAnimationNode> &node : tree_graph.nodes) {
			r_child_nodes->push_back(node.ptr());
		}
	}
};

VARIANT_ENUM_CAST(BLTBlendTree::ConnectionError)
