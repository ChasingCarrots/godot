#include "ConstraintSolution.h"

#include "core/object/class_db.h"

StringName ConstraintSolution::get_node_element_id(int p_i) const {
	ERR_FAIL_INDEX_V(p_i, _nodes.size(), StringName());
	return _nodes[p_i].element_id;
}

Transform3D ConstraintSolution::get_node_transform(int p_i) const {
	ERR_FAIL_INDEX_V(p_i, _nodes.size(), Transform3D());
	return _nodes[p_i].transform;
}

TypedArray<StringName> ConstraintSolution::get_node_tags(int p_i) const {
	TypedArray<StringName> out;
	ERR_FAIL_INDEX_V(p_i, _nodes.size(), out);
	for (const StringName &t : _nodes[p_i].tags) {
		out.append(t);
	}
	return out;
}

PackedInt32Array ConstraintSolution::get_node_connections(int p_i) const {
	PackedInt32Array out;
	ERR_FAIL_INDEX_V(p_i, _nodes.size(), out);
	for (int c : _nodes[p_i].connections) {
		out.push_back(c);
	}
	return out;
}

TypedArray<Dictionary> ConstraintSolution::get_nodes() const {
	TypedArray<Dictionary> out;
	for (int i = 0; i < _nodes.size(); i++) {
		const Node &n = _nodes[i];
		Dictionary d;
		d["element_id"] = n.element_id;
		d["transform"] = n.transform;
		d["tags"] = get_node_tags(i);
		d["connections"] = get_node_connections(i);
		out.append(d);
	}
	return out;
}

TypedArray<Dictionary> ConstraintSolution::get_rule_stats() const {
	TypedArray<Dictionary> out;
	for (const RuleStat &s : _rule_stats) {
		Dictionary d;
		d["rule_index"] = s.rule_index;
		d["kind"] = s.kind;
		d["label"] = s.label;
		d["rejections"] = s.rejections;
		d["completion_failures"] = s.completion_failures;
		out.append(d);
	}
	return out;
}

void ConstraintSolution::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_success"), &ConstraintSolution::is_success);
	ClassDB::bind_method(D_METHOD("get_steps"), &ConstraintSolution::get_steps);
	ClassDB::bind_method(D_METHOD("get_failure_reason"), &ConstraintSolution::get_failure_reason);
	ClassDB::bind_method(D_METHOD("get_node_count"), &ConstraintSolution::get_node_count);
	ClassDB::bind_method(D_METHOD("get_node_element_id", "index"), &ConstraintSolution::get_node_element_id);
	ClassDB::bind_method(D_METHOD("get_node_transform", "index"), &ConstraintSolution::get_node_transform);
	ClassDB::bind_method(D_METHOD("get_node_tags", "index"), &ConstraintSolution::get_node_tags);
	ClassDB::bind_method(D_METHOD("get_node_connections", "index"), &ConstraintSolution::get_node_connections);
	ClassDB::bind_method(D_METHOD("get_nodes"), &ConstraintSolution::get_nodes);

	ClassDB::bind_method(D_METHOD("get_backtracks"), &ConstraintSolution::get_backtracks);
	ClassDB::bind_method(D_METHOD("get_candidates_evaluated"), &ConstraintSolution::get_candidates_evaluated);
	ClassDB::bind_method(D_METHOD("get_candidates_accepted"), &ConstraintSolution::get_candidates_accepted);
	ClassDB::bind_method(D_METHOD("get_rule_stats"), &ConstraintSolution::get_rule_stats);
}
