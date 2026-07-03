#include "ConstraintElement.h"

#include "core/object/class_db.h"

void ConstraintInterface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_type", "type"), &ConstraintInterface::set_type);
	ClassDB::bind_method(D_METHOD("get_type"), &ConstraintInterface::get_type);
	ClassDB::bind_method(D_METHOD("set_anchor", "anchor"), &ConstraintInterface::set_anchor);
	ClassDB::bind_method(D_METHOD("get_anchor"), &ConstraintInterface::get_anchor);
	ClassDB::bind_method(D_METHOD("set_required", "required"), &ConstraintInterface::set_required);
	ClassDB::bind_method(D_METHOD("get_required"), &ConstraintInterface::get_required);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "type"), "set_type", "get_type");
	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "anchor"), "set_anchor", "get_anchor");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "required"), "set_required", "get_required");
}

void ConstraintElement::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "id"), &ConstraintElement::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &ConstraintElement::get_id);
	ClassDB::bind_method(D_METHOD("set_tags", "tags"), &ConstraintElement::set_tags);
	ClassDB::bind_method(D_METHOD("get_tags"), &ConstraintElement::get_tags);
	ClassDB::bind_method(D_METHOD("set_tags_needed", "tags"), &ConstraintElement::set_tags_needed);
	ClassDB::bind_method(D_METHOD("get_tags_needed"), &ConstraintElement::get_tags_needed);
	ClassDB::bind_method(D_METHOD("set_excluded_by_tags", "tags"), &ConstraintElement::set_excluded_by_tags);
	ClassDB::bind_method(D_METHOD("get_excluded_by_tags"), &ConstraintElement::get_excluded_by_tags);
	ClassDB::bind_method(D_METHOD("set_interfaces", "interfaces"), &ConstraintElement::set_interfaces);
	ClassDB::bind_method(D_METHOD("get_interfaces"), &ConstraintElement::get_interfaces);
	ClassDB::bind_method(D_METHOD("set_geometry_points", "points"), &ConstraintElement::set_geometry_points);
	ClassDB::bind_method(D_METHOD("get_geometry_points"), &ConstraintElement::get_geometry_points);
	ClassDB::bind_method(D_METHOD("set_weight", "weight"), &ConstraintElement::set_weight);
	ClassDB::bind_method(D_METHOD("get_weight"), &ConstraintElement::get_weight);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "id"), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tags", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_tags", "get_tags");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "tags_needed", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_tags_needed", "get_tags_needed");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "excluded_by_tags", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_excluded_by_tags", "get_excluded_by_tags");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "interfaces", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("ConstraintInterface")),
			"set_interfaces", "get_interfaces");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "geometry_points"), "set_geometry_points", "get_geometry_points");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");
}
