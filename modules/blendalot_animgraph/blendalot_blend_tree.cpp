#include "blendalot_blend_tree.h"

#include "core/object/class_db.h"

BLTBlendTree::BLTBlendTreeGraph::BLTBlendTreeGraph() {
	Ref<BLTAnimationNodeOutput> output_node;
	output_node.instantiate();
	output_node->set_name("Output");
	add_node(output_node);
}

Ref<BLTAnimationNode> BLTBlendTree::BLTBlendTreeGraph::get_output_node() {
	return nodes[0];
}

int BLTBlendTree::BLTBlendTreeGraph::find_node_index(const Ref<BLTAnimationNode> &node) const {
	for (uint32_t i = 0; i < nodes.size(); i++) {
		if (nodes[i] == node) {
			return i;
		}
	}

	return -1;
}

int BLTBlendTree::BLTBlendTreeGraph::find_node_index_by_name(const StringName &name) const {
	for (uint32_t i = 0; i < nodes.size(); i++) {
		if (nodes[i]->get_name() == name) {
			return i;
		}
	}

	return -1;
}

void BLTBlendTree::BLTBlendTreeGraph::add_node(const Ref<BLTAnimationNode> &node) {
	StringName node_base_name = node->get_name();
	if (node_base_name.is_empty()) {
		node_base_name = node->get_class_name();
	}
	node->set_name(node_base_name);

	int number_suffix = 1;
	while (find_node_index_by_name(node->get_name()) != -1) {
		node->set_name(vformat("%s %d", node_base_name, number_suffix));
		number_suffix++;
	}

	nodes.push_back(node);

	NodeConnectionInfo connection_info(node.ptr());
	connection_info.input_subtree_node_indices.insert(nodes.size() - 1);
	node_connection_info.push_back(connection_info);
}

bool BLTBlendTree::BLTBlendTreeGraph::remove_node(const Ref<BLTAnimationNode> &node) {
	if (node == get_output_node()) {
		// Output node not allowed to be removed
		return false;
	}

	int removed_node_index = find_node_index(node);
	assert(removed_node_index >= 0);

	// Remove all connections to and from this node
	for (int i = static_cast<int>(connections.size()) - 1; i >= 0; i--) {
		if (connections[i].source_node == node || connections[i].target_node == node) {
			remove_connection(connections[i].source_node, connections[i].target_node, connections[i].target_port_name);
		}
	}

	// Remove the data directly related to this node
	node_connection_info.remove_at(removed_node_index);
	nodes.remove_at(removed_node_index);

	// Ensure all indices are cleaned up.
	for (NodeConnectionInfo &connection_info : node_connection_info) {
		for (unsigned int j = 0; j < connection_info.connected_child_node_index_at_port.size(); j++) {
			if (connection_info.connected_child_node_index_at_port[j] > removed_node_index) {
				connection_info.connected_child_node_index_at_port[j] = connection_info.connected_child_node_index_at_port[j] - 1;
			}
		}

		if (connection_info.parent_node_index > removed_node_index) {
			connection_info.parent_node_index--;
		}

		// Map connected subtrees
		HashSet<int> old_indices(connection_info.input_subtree_node_indices);
		connection_info.input_subtree_node_indices.clear();
		for (int old_index : old_indices) {
			if (old_index > removed_node_index) {
				connection_info.input_subtree_node_indices.insert(old_index - 1);
			} else {
				connection_info.input_subtree_node_indices.insert(old_index);
			}
		}
	}

	return true;
}

void BLTBlendTree::BLTBlendTreeGraph::sort_nodes_and_references() {
	LocalVector<int> sorted_node_indices = get_sorted_node_indices();
	LocalVector<int> old_to_new_mapping;

	LocalVector<Ref<BLTAnimationNode>> sorted_nodes;
	LocalVector<NodeConnectionInfo> old_node_connection_info(node_connection_info);
	for (unsigned int i = 0; i < sorted_node_indices.size(); i++) {
		int node_index = sorted_node_indices[i];
		sorted_nodes.push_back(nodes[node_index]);
		node_connection_info[i] = old_node_connection_info[node_index];
		old_to_new_mapping.push_back(sorted_node_indices.find(i));
	}

	nodes = sorted_nodes;

	for (NodeConnectionInfo &connection_info : node_connection_info) {
		if (connection_info.parent_node_index != -1) {
			connection_info.parent_node_index = old_to_new_mapping[connection_info.parent_node_index];
		}
		connection_info.apply_node_mapping(old_to_new_mapping);
	}
}

void BLTBlendTree::BLTBlendTreeGraph::_print_graph() const {
	for (unsigned int i = 0; i < nodes.size(); i++) {
		print_line(vformat("Subtree of node %s (id %d, parent %d):", nodes[i]->get_name(), i, node_connection_info[i].parent_node_index));
		node_connection_info[i]._print_subtree();
	}
}

void BLTBlendTree::BLTBlendTreeGraph::_print_graph_timeinfo() const {
	for (unsigned int i = 0; i < nodes.size(); i++) {
		print_line(vformat("Node %s (id %d, parent %d):", nodes[i]->get_name(), i, node_connection_info[i].parent_node_index));
		print_line("  position:      %f", nodes[i]->node_time_info.position);
		print_line("  sync_position: %f", nodes[i]->node_time_info.sync_position);
		print_line("  duration:      %f", nodes[i]->node_time_info.sync_track.duration);
	}
}

LocalVector<int> BLTBlendTree::BLTBlendTreeGraph::get_sorted_node_indices() {
	LocalVector<int> result;

	sort_nodes_recursive(0, result);
	result.reverse();

	HashSet<int> connected_node_indices;
	for (int node_index : result) {
		connected_node_indices.insert(node_index);
	}

	// Ensure that nodes that are not reachable from the root node are still added to
	// the sorted nodes indices.
	for (Ref<BLTAnimationNode> &node : nodes) {
		int node_index = find_node_index(node);
		if (!connected_node_indices.has(node_index)) {
			result.push_back(node_index);
		}
	}

	return result;
}

void BLTBlendTree::BLTBlendTreeGraph::sort_nodes_recursive(int node_index, LocalVector<int> &result) {
	for (int input_node_index : node_connection_info[node_index].connected_child_node_index_at_port) {
		if (input_node_index >= 0) {
			sort_nodes_recursive(input_node_index, result);
		}
	}
	result.push_back(node_index);
}

void BLTBlendTree::BLTBlendTreeGraph::add_index_and_update_subtrees_recursive(int node_index, int node_parent_index) {
	if (node_parent_index == -1) {
		return;
	}

	node_connection_info[node_parent_index].input_subtree_node_indices.insert(node_index);

	for (int index : node_connection_info[node_index].input_subtree_node_indices) {
		node_connection_info[node_parent_index].input_subtree_node_indices.insert(index);
	}

	add_index_and_update_subtrees_recursive(node_parent_index, node_connection_info[node_parent_index].parent_node_index);
}

void BLTBlendTree::BLTBlendTreeGraph::remove_subtree_and_update_subtrees_recursive(int node_index, const HashSet<int> &removed_subtree_indices) {
	NodeConnectionInfo &connection_info = node_connection_info[node_index];

	for (int subtree_node_index : removed_subtree_indices) {
		connection_info.input_subtree_node_indices.erase(subtree_node_index);
	}

	if (connection_info.parent_node_index != -1) {
		remove_subtree_and_update_subtrees_recursive(connection_info.parent_node_index, removed_subtree_indices);
	}
}

BLTBlendTree::ConnectionError BLTBlendTree::BLTBlendTreeGraph::is_connection_valid(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, StringName target_port_name) const {
	int source_node_index = find_node_index(source_node);
	if (source_node_index == -1) {
		print_error("Cannot connect nodes: source node not found.");
		return CONNECTION_ERROR_NO_SOURCE_NODE;
	}

	if (node_connection_info[source_node_index].parent_node_index != -1) {
		print_error("Cannot connect node: source node already has a parent.");
		return CONNECTION_ERROR_PARENT_EXISTS;
	}

	int target_node_index = find_node_index(target_node);
	if (target_node_index == -1) {
		print_error("Cannot connect nodes: target node not found.");
		return CONNECTION_ERROR_NO_TARGET_NODE;
	}

	Vector<StringName> target_inputs = target_node->get_input_names();

	if (!target_inputs.has(target_port_name)) {
		print_error("Cannot connect nodes: target port not found.");
		return CONNECTION_ERROR_TARGET_PORT_NOT_FOUND;
	}

	int target_input_port_index = target_node->get_input_index(target_port_name);
	if (node_connection_info[target_node_index].connected_child_node_index_at_port[target_input_port_index] != -1) {
		print_error("Cannot connect node: target port already connected");
		return CONNECTION_ERROR_TARGET_PORT_ALREADY_CONNECTED;
	}

	if (node_connection_info[source_node_index].input_subtree_node_indices.has(target_node_index)) {
		print_error("Cannot connect node: connection would create loop.");
		return CONNECTION_ERROR_CONNECTION_CREATES_LOOP;
	}

	return CONNECTION_OK;
}

BLTBlendTree::ConnectionError BLTBlendTree::BLTBlendTreeGraph::add_connection(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name) {
	ConnectionError result = is_connection_valid(source_node, target_node, target_port_name);
	if (result != CONNECTION_OK) {
		return result;
	}

	int source_node_index = find_node_index(source_node);
	int target_node_index = find_node_index(target_node);
	int target_input_port_index = target_node->get_input_index(target_port_name);

	node_connection_info[source_node_index].parent_node_index = target_node_index;
	node_connection_info[target_node_index].connected_child_node_index_at_port[target_input_port_index] = source_node_index;
	connections.push_back(BLTBlendTreeConnection{ source_node, target_node, target_port_name });

	add_index_and_update_subtrees_recursive(source_node_index, target_node_index);

	return CONNECTION_OK;
}

int BLTBlendTree::BLTBlendTreeGraph::find_connection_index(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name) const {
	for (uint32_t i = 0; i < connections.size(); i++) {
		if (connections[i].source_node == source_node && connections[i].target_node == target_node && connections[i].target_port_name == target_port_name) {
			return i;
		}
	}

	return -1;
}

BLTBlendTree::ConnectionError BLTBlendTree::BLTBlendTreeGraph::remove_connection(const Ref<BLTAnimationNode> &source_node, const Ref<BLTAnimationNode> &target_node, const StringName &target_port_name) {
	int source_node_index = find_node_index(source_node);
	NodeConnectionInfo &connection_info = node_connection_info[source_node_index];

	int connection_index = find_connection_index(source_node, target_node, target_port_name);
	if (connection_index < 0 || connection_info.parent_node_index != -1) {
		NodeConnectionInfo &parent_connection_info = node_connection_info[connection_info.parent_node_index];
		parent_connection_info.input_subtree_node_indices.erase(source_node_index);
		parent_connection_info.connected_child_node_index_at_port[target_node->get_input_index(target_port_name)] = -1;

		remove_subtree_and_update_subtrees_recursive(connection_info.parent_node_index, connection_info.input_subtree_node_indices);

		connection_info.parent_node_index = -1;

		connections.remove_at(static_cast<uint32_t>(connection_index));
	} else {
		return CONNECTION_ERROR_CONNECTION_NOT_FOUND;
	}

	return CONNECTION_OK;
}

void BLTBlendTree::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_node", "animation_node"), &BLTBlendTree::add_node);
	ClassDB::bind_method(D_METHOD("remove_node", "animation_node"), &BLTBlendTree::remove_node);
	ClassDB::bind_method(D_METHOD("get_node", "node_name"), &BLTBlendTree::get_node);
	ClassDB::bind_method(D_METHOD("get_output_node"), &BLTBlendTree::get_output_node);
	ClassDB::bind_method(D_METHOD("get_node_names"), &BLTBlendTree::get_node_names_as_typed_array);

	ClassDB::bind_method(D_METHOD("is_connection_valid", "source_node", "target_node", "target_port_name"), &BLTBlendTree::is_connection_valid);
	ClassDB::bind_method(D_METHOD("add_connection", "source_node", "target_node", "target_port_name"), &BLTBlendTree::add_connection);
	ClassDB::bind_method(D_METHOD("remove_connection", "source_node", "target_node", "target_port_name"), &BLTBlendTree::remove_connection);
	ClassDB::bind_method(D_METHOD("get_connections"), &BLTBlendTree::get_connections_as_array);

	ClassDB::bind_method(D_METHOD("set_graph_offset", "graph_offset"), &BLTBlendTree::set_graph_offset);
	ClassDB::bind_method(D_METHOD("get_graph_offset"), &BLTBlendTree::get_graph_offset);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "graph_offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_graph_offset", "get_graph_offset");

	BIND_CONSTANT(CONNECTION_OK);
	BIND_CONSTANT(CONNECTION_ERROR_GRAPH_ALREADY_INITIALIZED);
	BIND_CONSTANT(CONNECTION_ERROR_NO_SOURCE_NODE);
	BIND_CONSTANT(CONNECTION_ERROR_NO_TARGET_NODE);
	BIND_CONSTANT(CONNECTION_ERROR_PARENT_EXISTS);
	BIND_CONSTANT(CONNECTION_ERROR_TARGET_PORT_NOT_FOUND);
	BIND_CONSTANT(CONNECTION_ERROR_TARGET_PORT_ALREADY_CONNECTED);
	BIND_CONSTANT(CONNECTION_ERROR_CONNECTION_CREATES_LOOP);
}

void BLTBlendTree::_get_property_list(List<PropertyInfo> *p_list) const {
	for (const Ref<BLTAnimationNode> &node : tree_graph.nodes) {
		String prop_name = node->get_name();
		if (prop_name != "Output") {
			p_list->push_back(PropertyInfo(Variant::OBJECT, "nodes/" + prop_name + "/node", PROPERTY_HINT_RESOURCE_TYPE, "AnimationNode", PROPERTY_USAGE_NO_EDITOR));
		}
		p_list->push_back(PropertyInfo(Variant::VECTOR2, "nodes/" + prop_name + "/graph_offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
	}

	p_list->push_back(PropertyInfo(Variant::ARRAY, "node_connections", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
}

bool BLTBlendTree::_get(const StringName &p_name, Variant &r_value) const {
	String prop_name = p_name;
	if (prop_name.begins_with("nodes/")) {
		String node_name = prop_name.get_slicec('/', 1);
		String what = prop_name.get_slicec('/', 2);
		int node_index = find_node_index_by_name(node_name);

		if (what == "node") {
			if (node_index != -1) {
				r_value = tree_graph.nodes[node_index];
				return true;
			}
		}

		if (what == "graph_offset") {
			if (node_index != -1) {
				r_value = tree_graph.nodes[node_index]->position;
				return true;
			}
		}
	} else if (prop_name == "node_connections") {
		Array conns;
		conns.resize(tree_graph.connections.size() * 3);

		int idx = 0;
		for (const BLTBlendTreeConnection &connection : tree_graph.connections) {
			conns[idx * 3 + 0] = connection.target_node->get_name();
			conns[idx * 3 + 1] = connection.target_node->get_input_index(connection.target_port_name);
			conns[idx * 3 + 2] = connection.source_node->get_name();
			idx++;
		}

		r_value = conns;
		return true;
	}

	return false;
}

bool BLTBlendTree::_set(const StringName &p_name, const Variant &p_value) {
	String prop_name = p_name;
	if (prop_name.begins_with("nodes/")) {
		String node_name = prop_name.get_slicec('/', 1);
		String what = prop_name.get_slicec('/', 2);

		if (what == "node") {
			Ref<BLTAnimationNode> anode = p_value;
			if (anode.is_valid()) {
				anode->set_name(node_name);
				add_node(anode);
			}
			return true;
		}

		if (what == "graph_offset") {
			int node_index = find_node_index_by_name(node_name);
			if (node_index > -1) {
				tree_graph.nodes[node_index]->position = p_value;
			}
			return true;
		}
	} else if (prop_name == "node_connections") {
		Array conns = p_value;
		ERR_FAIL_COND_V(conns.size() % 3 != 0, false);

		for (int i = 0; i < conns.size(); i += 3) {
			int target_node_index = find_node_index_by_name(conns[i]);
			int target_node_port_index = conns[i + 1];
			int source_node_index = find_node_index_by_name(conns[i + 2]);

			Ref<BLTAnimationNode> target_node = tree_graph.nodes[target_node_index];
			Vector<StringName> target_input_names = target_node->get_input_names();

			add_connection(tree_graph.nodes[source_node_index], target_node, target_input_names[target_node_port_index]);
		}
		return true;
	}

	return false;
}