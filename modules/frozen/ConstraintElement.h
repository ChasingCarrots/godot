#ifndef CONSTRAINT_ELEMENT_H
#define CONSTRAINT_ELEMENT_H

#include "core/io/resource.h"
#include "core/math/transform_3d.h"
#include "core/string/string_name.h"
#include "core/templates/vector.h"
#include "core/variant/typed_array.h"

// A connection point on a ConstraintElement. Two interfaces can connect if
// their `type` matches. `anchor` is the local-space frame used to compute the
// world transform of an attached element (GROW mode). `required` interfaces must
// be connected for a solution to be complete; optional ones may be left capped.
class ConstraintInterface : public Resource {
	GDCLASS(ConstraintInterface, Resource)

	StringName _type;
	Transform3D _anchor;
	bool _required = false;

protected:
	static void _bind_methods();

public:
	void set_type(const StringName &p_type) { _type = p_type; }
	StringName get_type() const { return _type; }

	void set_anchor(const Transform3D &p_anchor) { _anchor = p_anchor; }
	Transform3D get_anchor() const { return _anchor; }

	void set_required(bool p_required) { _required = p_required; }
	bool get_required() const { return _required; }
};

// A template in the problem catalog -- one possible value a slot can take.
// Carries generic `tags` (StringNames), connection `interfaces`, optional
// `geometry_points` (local space, only used when a ConstraintGeometry rule is
// present) and a selection `weight`.
class ConstraintElement : public Resource {
	GDCLASS(ConstraintElement, Resource)

	StringName _id;
	Vector<StringName> _tags;
	Vector<StringName> _tags_needed;
	Vector<StringName> _excluded_by_tags;
	Vector<Ref<ConstraintInterface>> _interfaces;
	PackedVector3Array _geometry_points;
	float _weight = 1.0f;

protected:
	static void _bind_methods();

public:
	void set_id(const StringName &p_id) { _id = p_id; }
	StringName get_id() const { return _id; }

	void set_tags(const TypedArray<StringName> &p_tags) {
		_tags.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_tags.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_tags() const {
		TypedArray<StringName> out;
		for (const StringName &t : _tags) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_tags_vector() const { return _tags; }

	// Tags that must be present somewhere in the result for this element to be
	// valid (satisfied by ambient tags or any co-placed element's tags). Only
	// enforced when a ConstraintTagCompatibility rule is present.
	void set_tags_needed(const TypedArray<StringName> &p_tags) {
		_tags_needed.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_tags_needed.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_tags_needed() const {
		TypedArray<StringName> out;
		for (const StringName &t : _tags_needed) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_tags_needed_vector() const { return _tags_needed; }

	// Tags whose presence anywhere in the result makes this element invalid.
	// Only enforced when a ConstraintTagCompatibility rule is present.
	void set_excluded_by_tags(const TypedArray<StringName> &p_tags) {
		_excluded_by_tags.clear();
		for (int i = 0; i < p_tags.size(); i++) {
			_excluded_by_tags.push_back(p_tags[i]);
		}
	}
	TypedArray<StringName> get_excluded_by_tags() const {
		TypedArray<StringName> out;
		for (const StringName &t : _excluded_by_tags) {
			out.append(t);
		}
		return out;
	}
	const Vector<StringName> &get_excluded_by_tags_vector() const { return _excluded_by_tags; }

	void set_interfaces(const TypedArray<ConstraintInterface> &p_interfaces) {
		_interfaces.clear();
		for (int i = 0; i < p_interfaces.size(); i++) {
			_interfaces.push_back(p_interfaces[i]);
		}
	}
	TypedArray<ConstraintInterface> get_interfaces() const {
		TypedArray<ConstraintInterface> out;
		for (const Ref<ConstraintInterface> &iface : _interfaces) {
			out.append(iface);
		}
		return out;
	}
	const Vector<Ref<ConstraintInterface>> &get_interfaces_vector() const { return _interfaces; }

	void set_geometry_points(const PackedVector3Array &p_points) { _geometry_points = p_points; }
	PackedVector3Array get_geometry_points() const { return _geometry_points; }

	void set_weight(float p_weight) { _weight = p_weight; }
	float get_weight() const { return _weight; }
};

#endif // CONSTRAINT_ELEMENT_H
