#ifndef CONSTRAINT_PROBLEM_H
#define CONSTRAINT_PROBLEM_H

#include "ConstraintElement.h"
#include "ConstraintRule.h"
#include "core/io/resource.h"
#include "core/math/transform_3d.h"
#include "core/string/string_name.h"
#include "core/templates/vector.h"
#include "core/variant/typed_array.h"

// A fixed slot for TOPOLOGY_FIXED problems: a placement with an optional tag
// filter restricting which catalog elements may occupy it (empty = any element).
class ConstraintSlot : public Resource {
	GDCLASS(ConstraintSlot, Resource)

	Transform3D _transform;
	Vector<StringName> _allowed_tags;

protected:
	static void _bind_methods();

public:
	void set_transform(const Transform3D &p_v) { _transform = p_v; }
	Transform3D get_transform() const { return _transform; }

	void set_allowed_tags(const TypedArray<StringName> &p_tags) {
		_allowed_tags.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_allowed_tags.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_allowed_tags() const {
		TypedArray<StringName> out;
		for (const StringName &t : _allowed_tags) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_allowed_tags_vector() const { return _allowed_tags; }
};

// The full, editor-authored description of a generation problem: the element
// catalog, the constraint rules, the topology mode and its parameters. Passed to
// ConstraintSolver::solve(). All naming is generic; nothing here is specific to
// rooms/dungeons.
class ConstraintProblem : public Resource {
	GDCLASS(ConstraintProblem, Resource)

public:
	enum TopologyMode {
		// Grow an unbounded graph by attaching elements at open interfaces
		// starting from `start_element` (tree, no cycles).
		TOPOLOGY_GROW,
		// Assign a value to each of a fixed set of `slots` connected by a
		// predefined (possibly cyclic) graph in `connections`.
		TOPOLOGY_FIXED,
	};

private:
	TopologyMode _topology_mode = TOPOLOGY_GROW;
	Vector<Ref<ConstraintElement>> _elements;
	Vector<Ref<ConstraintRule>> _rules;

	// GROW parameters.
	StringName _start_element;
	int _min_elements = 0;
	int _max_elements = -1; // -1 == unbounded

	// FIXED parameters.
	Vector<Ref<ConstraintSlot>> _slots;
	PackedInt32Array _connections; // flat pairs: [a0,b0, a1,b1, ...]

	// Tags considered present from the start of the solve, seeding the tag pool
	// that ConstraintTagCompatibility checks tags_needed/excluded_by_tags against.
	Vector<StringName> _ambient_tags;

	// Search budget / determinism.
	int _seed = 0;
	int _max_steps = 1000000;
	int _time_budget_ms = 0; // 0 == no wall-clock limit

protected:
	static void _bind_methods();

public:
	void set_topology_mode(TopologyMode p_v) { _topology_mode = p_v; }
	TopologyMode get_topology_mode() const { return _topology_mode; }

	void set_elements(const TypedArray<ConstraintElement> &p_v) {
		_elements.clear();
		for (int i = 0; i < p_v.size(); i++) {
			_elements.push_back(p_v[i]);
		}
	}
	TypedArray<ConstraintElement> get_elements() const {
		TypedArray<ConstraintElement> out;
		for (const Ref<ConstraintElement> &e : _elements) {
			out.append(e);
		}
		return out;
	}
	const Vector<Ref<ConstraintElement>> &get_elements_vector() const { return _elements; }

	void set_rules(const TypedArray<ConstraintRule> &p_v) {
		_rules.clear();
		for (int i = 0; i < p_v.size(); i++) {
			_rules.push_back(p_v[i]);
		}
	}
	TypedArray<ConstraintRule> get_rules() const {
		TypedArray<ConstraintRule> out;
		for (const Ref<ConstraintRule> &r : _rules) {
			out.append(r);
		}
		return out;
	}
	const Vector<Ref<ConstraintRule>> &get_rules_vector() const { return _rules; }

	void set_start_element(const StringName &p_v) { _start_element = p_v; }
	StringName get_start_element() const { return _start_element; }

	void set_min_elements(int p_v) { _min_elements = p_v; }
	int get_min_elements() const { return _min_elements; }
	void set_max_elements(int p_v) { _max_elements = p_v; }
	int get_max_elements() const { return _max_elements; }

	void set_slots(const TypedArray<ConstraintSlot> &p_v) {
		_slots.clear();
		for (int i = 0; i < p_v.size(); i++) {
			_slots.push_back(p_v[i]);
		}
	}
	TypedArray<ConstraintSlot> get_slots() const {
		TypedArray<ConstraintSlot> out;
		for (const Ref<ConstraintSlot> &s : _slots) {
			out.append(s);
		}
		return out;
	}
	const Vector<Ref<ConstraintSlot>> &get_slots_vector() const { return _slots; }

	void set_connections(const PackedInt32Array &p_v) { _connections = p_v; }
	PackedInt32Array get_connections() const { return _connections; }

	void set_ambient_tags(const TypedArray<StringName> &p_tags) {
		_ambient_tags.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_ambient_tags.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_ambient_tags() const {
		TypedArray<StringName> out;
		for (const StringName &t : _ambient_tags) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_ambient_tags_vector() const { return _ambient_tags; }

	void set_seed(int p_v) { _seed = p_v; }
	int get_seed() const { return _seed; }
	void set_max_steps(int p_v) { _max_steps = p_v; }
	int get_max_steps() const { return _max_steps; }
	void set_time_budget_ms(int p_v) { _time_budget_ms = p_v; }
	int get_time_budget_ms() const { return _time_budget_ms; }
};

VARIANT_ENUM_CAST(ConstraintProblem::TopologyMode);

#endif // CONSTRAINT_PROBLEM_H
