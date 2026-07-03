#ifndef CONSTRAINT_SOLUTION_H
#define CONSTRAINT_SOLUTION_H

#include "core/object/ref_counted.h"
#include "core/math/transform_3d.h"
#include "core/string/string_name.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"

// Result of ConstraintSolver::solve(). Holds the placed elements (the chosen
// template, its world transform, its tags, and the indices of the nodes it
// connects to) plus diagnostics. Read-only from GDScript; the solver fills it in.
class ConstraintSolution : public RefCounted {
	GDCLASS(ConstraintSolution, RefCounted)

public:
	struct Node {
		StringName element_id;
		Transform3D transform;
		Vector<StringName> tags;
		Vector<int> connections;
	};

	// Per-rule solve statistics, attributed back to the authored ConstraintRule.
	struct RuleStat {
		int rule_index = 0; // position in the problem's rules array
		int kind = 0; // ConstraintRule::Kind
		StringName label; // tag(s) the rule targets, for a readable report
		int rejections = 0; // candidate placements this rule vetoed
		int completion_failures = 0; // finished layouts this rule invalidated
	};

private:
	bool _success = false;
	int _steps = 0;
	String _failure_reason;
	Vector<Node> _nodes;

	int _backtracks = 0;
	int _candidates_evaluated = 0;
	int _candidates_accepted = 0;
	Vector<RuleStat> _rule_stats;

protected:
	static void _bind_methods();

public:
	// --- solver-facing mutators (not bound to GDScript) ---
	void set_success(bool p_v) { _success = p_v; }
	void set_steps(int p_v) { _steps = p_v; }
	void set_failure_reason(const String &p_v) { _failure_reason = p_v; }
	int add_node(const StringName &p_id, const Transform3D &p_xform, const Vector<StringName> &p_tags) {
		Node n;
		n.element_id = p_id;
		n.transform = p_xform;
		n.tags = p_tags;
		_nodes.push_back(n);
		return _nodes.size() - 1;
	}
	void add_connection(int p_a, int p_b) {
		_nodes.write[p_a].connections.push_back(p_b);
		_nodes.write[p_b].connections.push_back(p_a);
	}
	void set_search_stats(int p_backtracks, int p_evaluated, int p_accepted) {
		_backtracks = p_backtracks;
		_candidates_evaluated = p_evaluated;
		_candidates_accepted = p_accepted;
	}
	void add_rule_stat(int p_index, int p_kind, const StringName &p_label, int p_rejections, int p_completion_failures) {
		RuleStat s;
		s.rule_index = p_index;
		s.kind = p_kind;
		s.label = p_label;
		s.rejections = p_rejections;
		s.completion_failures = p_completion_failures;
		_rule_stats.push_back(s);
	}

	// --- GDScript API ---
	bool is_success() const { return _success; }
	int get_steps() const { return _steps; }
	String get_failure_reason() const { return _failure_reason; }
	int get_node_count() const { return _nodes.size(); }
	StringName get_node_element_id(int p_i) const;
	Transform3D get_node_transform(int p_i) const;
	TypedArray<StringName> get_node_tags(int p_i) const;
	PackedInt32Array get_node_connections(int p_i) const;
	TypedArray<Dictionary> get_nodes() const;

	// --- statistics API ---
	int get_backtracks() const { return _backtracks; }
	int get_candidates_evaluated() const { return _candidates_evaluated; }
	int get_candidates_accepted() const { return _candidates_accepted; }
	TypedArray<Dictionary> get_rule_stats() const;
};

#endif // CONSTRAINT_SOLUTION_H
