#ifndef CONSTRAINT_RULE_H
#define CONSTRAINT_RULE_H

#include "core/io/resource.h"
#include "core/math/aabb.h"
#include "core/string/string_name.h"
#include "core/templates/vector.h"
#include "core/variant/callable.h"
#include "core/variant/typed_array.h"

// Base class for declarative, editor-authored constraints. A rule is pure
// configuration; at the start of solve() the solver inspects its Kind and
// "compiles" it into the internal (Tier-B) narrowing machinery. Subclasses add
// their own typed parameters. Designers can also subclass via ConstraintCallback
// for arbitrary GDScript predicates without touching C++.
class ConstraintRule : public Resource {
	GDCLASS(ConstraintRule, Resource)

public:
	enum Kind {
		KIND_NONE,
		KIND_COUNT_BY_TAG,
		KIND_REQUIRES_TAG_BEFORE,
		KIND_TAG_CONNECTS_TO_TAG,
		KIND_MAX_CONSECUTIVE_TAG,
		KIND_MAX_ADJACENT_BY_TAG,
		KIND_CONNECTION_COUNT_BY_TAG,
		KIND_NEIGHBOR_TAG_ALLOWED,
		KIND_MAX_REPEATS_BY_ELEMENT_ID,
		KIND_NO_REPEAT_NEIGHBOR_ELEMENT,
		KIND_LEAF_COUNT_BY_TAG,
		KIND_TAG_COMPATIBILITY,
		KIND_GEOMETRY,
		KIND_CALLBACK,
		KIND_TAG_REACHABILITY,
		KIND_TAG_IMPLIES_TAG,
	};

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const { return KIND_NONE; }
};

VARIANT_ENUM_CAST(ConstraintRule::Kind);

// "NumberOfElementsWithTag(Tag, MinNumber, MaxNumber)" -- the count of placed
// elements carrying `tag` must be within [min_count, max_count]. max_count < 0
// means unbounded.
class ConstraintCountByTag : public ConstraintRule {
	GDCLASS(ConstraintCountByTag, ConstraintRule)

	StringName _tag;
	int _min_count = 0;
	int _max_count = -1;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_COUNT_BY_TAG; }

	void set_tag(const StringName &p_tag) { _tag = p_tag; }
	StringName get_tag() const { return _tag; }
	void set_min_count(int p_v) { _min_count = p_v; }
	int get_min_count() const { return _min_count; }
	void set_max_count(int p_v) { _max_count = p_v; }
	int get_max_count() const { return _max_count; }
};

// "ElementWithTagNeedsTagsBefore" -- an element carrying `tag` may only be placed
// if the required tags already appear on its ancestor/connection path back to the
// start. When `as_stack` is true the required tags must appear in order along the
// path (a tag stack).
class ConstraintRequiresTagBefore : public ConstraintRule {
	GDCLASS(ConstraintRequiresTagBefore, ConstraintRule)

	StringName _tag;
	Vector<StringName> _required_before;
	bool _as_stack = false;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_REQUIRES_TAG_BEFORE; }

	void set_tag(const StringName &p_tag) { _tag = p_tag; }
	StringName get_tag() const { return _tag; }

	void set_required_before(const TypedArray<StringName> &p_tags) {
		_required_before.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_required_before.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_required_before() const {
		TypedArray<StringName> out;
		for (const StringName &t : _required_before) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_required_before_vector() const { return _required_before; }

	void set_as_stack(bool p_v) { _as_stack = p_v; }
	bool get_as_stack() const { return _as_stack; }
};

// An element carrying `tag_x` must be connected (adjacent) to at least one
// element carrying `tag_y`.
class ConstraintTagConnectsToTag : public ConstraintRule {
	GDCLASS(ConstraintTagConnectsToTag, ConstraintRule)

	StringName _tag_x;
	StringName _tag_y;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_TAG_CONNECTS_TO_TAG; }

	void set_tag_x(const StringName &p_v) { _tag_x = p_v; }
	StringName get_tag_x() const { return _tag_x; }
	void set_tag_y(const StringName &p_v) { _tag_y = p_v; }
	StringName get_tag_y() const { return _tag_y; }
};

// "MaxConsecutiveConstraint" -- along any path from the start element down to a
// leaf, at most `max_consecutive` elements carrying `tag` may appear in an
// unbroken run. GROW-only (it is a property of the growth path).
class ConstraintMaxConsecutiveTag : public ConstraintRule {
	GDCLASS(ConstraintMaxConsecutiveTag, ConstraintRule)

	StringName _tag;
	int _max_consecutive = 1;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_MAX_CONSECUTIVE_TAG; }

	void set_tag(const StringName &p_v) { _tag = p_v; }
	StringName get_tag() const { return _tag; }
	void set_max_consecutive(int p_v) { _max_consecutive = p_v; }
	int get_max_consecutive() const { return _max_consecutive; }
};

// "MaxAdjacentConstraint" -- an element carrying `tag` may be connected to at
// most `max_adjacent` neighbors (its node degree in the connection graph).
class ConstraintMaxAdjacentByTag : public ConstraintRule {
	GDCLASS(ConstraintMaxAdjacentByTag, ConstraintRule)

	StringName _tag;
	int _max_adjacent = 1;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_MAX_ADJACENT_BY_TAG; }

	void set_tag(const StringName &p_v) { _tag = p_v; }
	StringName get_tag() const { return _tag; }
	void set_max_adjacent(int p_v) { _max_adjacent = p_v; }
	int get_max_adjacent() const { return _max_adjacent; }
};

// The number of connected neighbors (graph degree) of every element carrying
// `tag` must lie within [min_connections, max_connections]. max_connections < 0
// means unbounded. This generalizes ConstraintMaxAdjacentByTag by adding a lower
// bound: e.g. a START room with exactly one connection (1, 1), a TRANSITION with
// exactly two (2, 2), a MAIN hall with at least two (2, -1). The max bound is a
// forward check during growth; the min bound is verified when a layout completes.
class ConstraintConnectionCountByTag : public ConstraintRule {
	GDCLASS(ConstraintConnectionCountByTag, ConstraintRule)

	StringName _tag;
	int _min_connections = 0;
	int _max_connections = -1;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_CONNECTION_COUNT_BY_TAG; }

	void set_tag(const StringName &p_v) { _tag = p_v; }
	StringName get_tag() const { return _tag; }
	void set_min_connections(int p_v) { _min_connections = p_v; }
	int get_min_connections() const { return _min_connections; }
	void set_max_connections(int p_v) { _max_connections = p_v; }
	int get_max_connections() const { return _max_connections; }
};

// "RoomToRoomConstraint" -- every neighbor of an element carrying `tag` must
// itself carry at least one of `allowed_neighbor_tags`. Distinct from
// ConstraintTagConnectsToTag, which is an existence ("at least one neighbor")
// rule; this one restricts *which* tags are permitted as neighbors. An empty
// allowed list forbids any neighbor for a `tag` element.
class ConstraintNeighborTagAllowed : public ConstraintRule {
	GDCLASS(ConstraintNeighborTagAllowed, ConstraintRule)

	StringName _tag;
	Vector<StringName> _allowed_neighbor_tags;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_NEIGHBOR_TAG_ALLOWED; }

	void set_tag(const StringName &p_v) { _tag = p_v; }
	StringName get_tag() const { return _tag; }

	void set_allowed_neighbor_tags(const TypedArray<StringName> &p_tags) {
		_allowed_neighbor_tags.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_allowed_neighbor_tags.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_allowed_neighbor_tags() const {
		TypedArray<StringName> out;
		for (const StringName &t : _allowed_neighbor_tags) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_allowed_neighbor_tags_vector() const { return _allowed_neighbor_tags; }
};

// "MaxRepeatsByElementId" -- variety/anti-repeat. The number of placed nodes that
// instance a given element template (its `id`) must not exceed `max_repeats`.
// An empty `element_id` applies the cap to *every* id independently (no single
// template may appear more than `max_repeats` times). max_repeats < 0 = unbounded.
// Unlike ConstraintCountByTag this keys off the element template, not a tag.
class ConstraintMaxRepeatsByElementId : public ConstraintRule {
	GDCLASS(ConstraintMaxRepeatsByElementId, ConstraintRule)

	StringName _element_id;
	int _max_repeats = -1;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_MAX_REPEATS_BY_ELEMENT_ID; }

	void set_element_id(const StringName &p_v) { _element_id = p_v; }
	StringName get_element_id() const { return _element_id; }
	void set_max_repeats(int p_v) { _max_repeats = p_v; }
	int get_max_repeats() const { return _max_repeats; }
};

// "NoRepeatNeighborElement" -- variety/anti-repeat. No connection (edge) may join
// two nodes that instance the same element template, which prevents obvious
// tiling of identical rooms. Parameterless.
class ConstraintNoRepeatNeighborElement : public ConstraintRule {
	GDCLASS(ConstraintNoRepeatNeighborElement, ConstraintRule)

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_NO_REPEAT_NEIGHBOR_ELEMENT; }
};

// "LeafCountByTag" -- graph-shape. The number of leaf (dead-end) nodes carrying
// `tag` must lie within [min_count, max_count]. An empty `tag` counts all leaves.
// max_count < 0 = unbounded. A "leaf" is mode-specific: in GROW it is a node with
// zero children (a dead-end of the grown tree); in FIXED it is a node whose
// connection-graph degree is <= 1. Checked when a layout completes.
class ConstraintLeafCountByTag : public ConstraintRule {
	GDCLASS(ConstraintLeafCountByTag, ConstraintRule)

	StringName _tag;
	int _min_count = 0;
	int _max_count = -1;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_LEAF_COUNT_BY_TAG; }

	void set_tag(const StringName &p_v) { _tag = p_v; }
	StringName get_tag() const { return _tag; }
	void set_min_count(int p_v) { _min_count = p_v; }
	int get_min_count() const { return _min_count; }
	void set_max_count(int p_v) { _max_count = p_v; }
	int get_max_count() const { return _max_count; }
};

// "TagCompatibility" -- opt-in tag-pool compatibility. When present, activates
// enforcement of every element's `tags_needed` and `excluded_by_tags` against the
// result's tag pool R = ambient_tags ∪ {tags of every placed element}. A placed
// element is valid only if all of its `tags_needed` are in R and none of its
// `excluded_by_tags` are in R. Parameterless: the data lives on the elements (and
// ConstraintProblem.ambient_tags); this rule just turns the check on. Both modes.
class ConstraintTagCompatibility : public ConstraintRule {
	GDCLASS(ConstraintTagCompatibility, ConstraintRule)

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_TAG_COMPATIBILITY; }
};

// "TagReachability" -- global connectivity over a tag hypergraph. Tags whose name
// begins with `link_prefix` are graph nodes; every placed element unions all of the
// link tags it carries into one component. Every tag in `required_tags` must end up
// in a single shared component (all transitively connected through placed elements).
// A required tag that no placed element carries stays an isolated singleton and fails
// the check -- which is how it also forces elements to be chosen. 0 or 1 required
// tags trivially pass. Purely value-set based (ignores the connection graph), so it
// works in both GROW and FIXED; checked when a layout completes.
class ConstraintTagReachability : public ConstraintRule {
	GDCLASS(ConstraintTagReachability, ConstraintRule)

	StringName _link_prefix;
	Vector<StringName> _required_tags;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_TAG_REACHABILITY; }

	void set_link_prefix(const StringName &p_v) { _link_prefix = p_v; }
	StringName get_link_prefix() const { return _link_prefix; }

	void set_required_tags(const TypedArray<StringName> &p_tags) {
		_required_tags.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_required_tags.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_required_tags() const {
		TypedArray<StringName> out;
		for (const StringName &t : _required_tags) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_required_tags_vector() const { return _required_tags; }
};

// "TagImpliesTag" -- conditional presence. If ANY placed element carries `tag`, then at least one
// placed element must carry `implied_tag`. Directional on purpose: it models a dependent part and
// the anchor it needs (a ramp and the tower it lands on), so the anchor stays free to appear alone.
// Unlike ConstraintTagConnectsToTag this ignores adjacency entirely -- it is about what exists in the
// result, not what touches what. Forward-pruned in FIXED, checked on completion in both modes.
class ConstraintTagImpliesTag : public ConstraintRule {
	GDCLASS(ConstraintTagImpliesTag, ConstraintRule)

	StringName _tag;
	StringName _implied_tag;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_TAG_IMPLIES_TAG; }

	void set_tag(const StringName &p_v) { _tag = p_v; }
	StringName get_tag() const { return _tag; }
	void set_implied_tag(const StringName &p_v) { _implied_tag = p_v; }
	StringName get_implied_tag() const { return _implied_tag; }
};

// Opt-in geometric overlap rule backed by VoxelGrid. When present, each placed
// element's `geometry_points` are voxelized and rejected if they overlap already
// occupied space (excluding the parent it connects to). When absent, generation
// is purely topological.
class ConstraintGeometry : public ConstraintRule {
	GDCLASS(ConstraintGeometry, ConstraintRule)

	AABB _bounds;
	float _voxel_size = 1.0f;
	int _spacing_grow = 0;
	bool _out_of_bounds_occupied = true;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_GEOMETRY; }

	void set_bounds(const AABB &p_v) { _bounds = p_v; }
	AABB get_bounds() const { return _bounds; }
	void set_voxel_size(float p_v) { _voxel_size = p_v; }
	float get_voxel_size() const { return _voxel_size; }
	void set_spacing_grow(int p_v) { _spacing_grow = p_v; }
	int get_spacing_grow() const { return _spacing_grow; }
	void set_out_of_bounds_occupied(bool p_v) { _out_of_bounds_occupied = p_v; }
	bool get_out_of_bounds_occupied() const { return _out_of_bounds_occupied; }
};

// Escape hatch: a GDScript Callable invoked to accept/reject a candidate
// placement. It receives a Dictionary describing the candidate (see
// ConstraintSolver docs) and must return a bool (true = allowed).
//
// GROW-only: callbacks are not evaluated in TOPOLOGY_FIXED (the solver warns at
// compile time if one is present there).
//
// THREADING: the Callable is invoked synchronously on whatever thread runs
// solve(). If you solve() on a WorkerThreadPool thread, the callback MUST be
// thread-safe -- it must not touch the scene tree, servers, or other main-thread
// -only state. Marshal such work yourself (e.g. cache it before solving).
class ConstraintCallback : public ConstraintRule {
	GDCLASS(ConstraintCallback, ConstraintRule)

	Callable _callable;

protected:
	static void _bind_methods();

public:
	virtual Kind get_kind() const override { return KIND_CALLBACK; }

	void set_callable(const Callable &p_v) { _callable = p_v; }
	Callable get_callable() const { return _callable; }
};

#endif // CONSTRAINT_RULE_H
