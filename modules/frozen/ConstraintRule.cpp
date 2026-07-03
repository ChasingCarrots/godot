#include "ConstraintRule.h"

#include "core/object/class_db.h"

void ConstraintRule::_bind_methods() {
	BIND_ENUM_CONSTANT(KIND_NONE);
	BIND_ENUM_CONSTANT(KIND_COUNT_BY_TAG);
	BIND_ENUM_CONSTANT(KIND_REQUIRES_TAG_BEFORE);
	BIND_ENUM_CONSTANT(KIND_TAG_CONNECTS_TO_TAG);
	BIND_ENUM_CONSTANT(KIND_MAX_CONSECUTIVE_TAG);
	BIND_ENUM_CONSTANT(KIND_MAX_ADJACENT_BY_TAG);
	BIND_ENUM_CONSTANT(KIND_CONNECTION_COUNT_BY_TAG);
	BIND_ENUM_CONSTANT(KIND_NEIGHBOR_TAG_ALLOWED);
	BIND_ENUM_CONSTANT(KIND_MAX_REPEATS_BY_ELEMENT_ID);
	BIND_ENUM_CONSTANT(KIND_NO_REPEAT_NEIGHBOR_ELEMENT);
	BIND_ENUM_CONSTANT(KIND_LEAF_COUNT_BY_TAG);
	BIND_ENUM_CONSTANT(KIND_TAG_COMPATIBILITY);
	BIND_ENUM_CONSTANT(KIND_GEOMETRY);
	BIND_ENUM_CONSTANT(KIND_CALLBACK);
	BIND_ENUM_CONSTANT(KIND_TAG_REACHABILITY);

	ClassDB::bind_method(D_METHOD("get_kind"), &ConstraintRule::get_kind);
}

void ConstraintCountByTag::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &ConstraintCountByTag::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &ConstraintCountByTag::get_tag);
	ClassDB::bind_method(D_METHOD("set_min_count", "min_count"), &ConstraintCountByTag::set_min_count);
	ClassDB::bind_method(D_METHOD("get_min_count"), &ConstraintCountByTag::get_min_count);
	ClassDB::bind_method(D_METHOD("set_max_count", "max_count"), &ConstraintCountByTag::set_max_count);
	ClassDB::bind_method(D_METHOD("get_max_count"), &ConstraintCountByTag::get_max_count);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_count"), "set_min_count", "get_min_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_count"), "set_max_count", "get_max_count");
}

void ConstraintRequiresTagBefore::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &ConstraintRequiresTagBefore::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &ConstraintRequiresTagBefore::get_tag);
	ClassDB::bind_method(D_METHOD("set_required_before", "tags"), &ConstraintRequiresTagBefore::set_required_before);
	ClassDB::bind_method(D_METHOD("get_required_before"), &ConstraintRequiresTagBefore::get_required_before);
	ClassDB::bind_method(D_METHOD("set_as_stack", "as_stack"), &ConstraintRequiresTagBefore::set_as_stack);
	ClassDB::bind_method(D_METHOD("get_as_stack"), &ConstraintRequiresTagBefore::get_as_stack);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "required_before", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_required_before", "get_required_before");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "as_stack"), "set_as_stack", "get_as_stack");
}

void ConstraintTagConnectsToTag::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag_x", "tag_x"), &ConstraintTagConnectsToTag::set_tag_x);
	ClassDB::bind_method(D_METHOD("get_tag_x"), &ConstraintTagConnectsToTag::get_tag_x);
	ClassDB::bind_method(D_METHOD("set_tag_y", "tag_y"), &ConstraintTagConnectsToTag::set_tag_y);
	ClassDB::bind_method(D_METHOD("get_tag_y"), &ConstraintTagConnectsToTag::get_tag_y);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag_x"), "set_tag_x", "get_tag_x");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag_y"), "set_tag_y", "get_tag_y");
}

void ConstraintMaxConsecutiveTag::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &ConstraintMaxConsecutiveTag::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &ConstraintMaxConsecutiveTag::get_tag);
	ClassDB::bind_method(D_METHOD("set_max_consecutive", "max_consecutive"), &ConstraintMaxConsecutiveTag::set_max_consecutive);
	ClassDB::bind_method(D_METHOD("get_max_consecutive"), &ConstraintMaxConsecutiveTag::get_max_consecutive);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_consecutive"), "set_max_consecutive", "get_max_consecutive");
}

void ConstraintMaxAdjacentByTag::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &ConstraintMaxAdjacentByTag::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &ConstraintMaxAdjacentByTag::get_tag);
	ClassDB::bind_method(D_METHOD("set_max_adjacent", "max_adjacent"), &ConstraintMaxAdjacentByTag::set_max_adjacent);
	ClassDB::bind_method(D_METHOD("get_max_adjacent"), &ConstraintMaxAdjacentByTag::get_max_adjacent);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_adjacent"), "set_max_adjacent", "get_max_adjacent");
}

void ConstraintConnectionCountByTag::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &ConstraintConnectionCountByTag::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &ConstraintConnectionCountByTag::get_tag);
	ClassDB::bind_method(D_METHOD("set_min_connections", "min_connections"), &ConstraintConnectionCountByTag::set_min_connections);
	ClassDB::bind_method(D_METHOD("get_min_connections"), &ConstraintConnectionCountByTag::get_min_connections);
	ClassDB::bind_method(D_METHOD("set_max_connections", "max_connections"), &ConstraintConnectionCountByTag::set_max_connections);
	ClassDB::bind_method(D_METHOD("get_max_connections"), &ConstraintConnectionCountByTag::get_max_connections);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_connections"), "set_min_connections", "get_min_connections");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_connections"), "set_max_connections", "get_max_connections");
}

void ConstraintNeighborTagAllowed::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &ConstraintNeighborTagAllowed::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &ConstraintNeighborTagAllowed::get_tag);
	ClassDB::bind_method(D_METHOD("set_allowed_neighbor_tags", "tags"), &ConstraintNeighborTagAllowed::set_allowed_neighbor_tags);
	ClassDB::bind_method(D_METHOD("get_allowed_neighbor_tags"), &ConstraintNeighborTagAllowed::get_allowed_neighbor_tags);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "allowed_neighbor_tags", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_allowed_neighbor_tags", "get_allowed_neighbor_tags");
}

void ConstraintMaxRepeatsByElementId::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_element_id", "element_id"), &ConstraintMaxRepeatsByElementId::set_element_id);
	ClassDB::bind_method(D_METHOD("get_element_id"), &ConstraintMaxRepeatsByElementId::get_element_id);
	ClassDB::bind_method(D_METHOD("set_max_repeats", "max_repeats"), &ConstraintMaxRepeatsByElementId::set_max_repeats);
	ClassDB::bind_method(D_METHOD("get_max_repeats"), &ConstraintMaxRepeatsByElementId::get_max_repeats);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "element_id"), "set_element_id", "get_element_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_repeats"), "set_max_repeats", "get_max_repeats");
}

void ConstraintNoRepeatNeighborElement::_bind_methods() {
}

void ConstraintLeafCountByTag::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &ConstraintLeafCountByTag::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &ConstraintLeafCountByTag::get_tag);
	ClassDB::bind_method(D_METHOD("set_min_count", "min_count"), &ConstraintLeafCountByTag::set_min_count);
	ClassDB::bind_method(D_METHOD("get_min_count"), &ConstraintLeafCountByTag::get_min_count);
	ClassDB::bind_method(D_METHOD("set_max_count", "max_count"), &ConstraintLeafCountByTag::set_max_count);
	ClassDB::bind_method(D_METHOD("get_max_count"), &ConstraintLeafCountByTag::get_max_count);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_count"), "set_min_count", "get_min_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_count"), "set_max_count", "get_max_count");
}

void ConstraintTagCompatibility::_bind_methods() {
}

void ConstraintGeometry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_bounds", "bounds"), &ConstraintGeometry::set_bounds);
	ClassDB::bind_method(D_METHOD("get_bounds"), &ConstraintGeometry::get_bounds);
	ClassDB::bind_method(D_METHOD("set_voxel_size", "voxel_size"), &ConstraintGeometry::set_voxel_size);
	ClassDB::bind_method(D_METHOD("get_voxel_size"), &ConstraintGeometry::get_voxel_size);
	ClassDB::bind_method(D_METHOD("set_spacing_grow", "spacing_grow"), &ConstraintGeometry::set_spacing_grow);
	ClassDB::bind_method(D_METHOD("get_spacing_grow"), &ConstraintGeometry::get_spacing_grow);
	ClassDB::bind_method(D_METHOD("set_out_of_bounds_occupied", "value"), &ConstraintGeometry::set_out_of_bounds_occupied);
	ClassDB::bind_method(D_METHOD("get_out_of_bounds_occupied"), &ConstraintGeometry::get_out_of_bounds_occupied);

	ADD_PROPERTY(PropertyInfo(Variant::AABB, "bounds"), "set_bounds", "get_bounds");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "voxel_size"), "set_voxel_size", "get_voxel_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "spacing_grow"), "set_spacing_grow", "get_spacing_grow");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "out_of_bounds_occupied"), "set_out_of_bounds_occupied", "get_out_of_bounds_occupied");
}

void ConstraintCallback::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_callable", "callable"), &ConstraintCallback::set_callable);
	ClassDB::bind_method(D_METHOD("get_callable"), &ConstraintCallback::get_callable);

	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "callable"), "set_callable", "get_callable");
}

void ConstraintTagReachability::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_link_prefix", "link_prefix"), &ConstraintTagReachability::set_link_prefix);
	ClassDB::bind_method(D_METHOD("get_link_prefix"), &ConstraintTagReachability::get_link_prefix);
	ClassDB::bind_method(D_METHOD("set_required_tags", "tags"), &ConstraintTagReachability::set_required_tags);
	ClassDB::bind_method(D_METHOD("get_required_tags"), &ConstraintTagReachability::get_required_tags);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "link_prefix"), "set_link_prefix", "get_link_prefix");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "required_tags", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_required_tags", "get_required_tags");
}
