#include "ConstraintProblem.h"

#include "core/object/class_db.h"

void ConstraintSlot::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_transform", "transform"), &ConstraintSlot::set_transform);
	ClassDB::bind_method(D_METHOD("get_transform"), &ConstraintSlot::get_transform);
	ClassDB::bind_method(D_METHOD("set_allowed_tags", "tags"), &ConstraintSlot::set_allowed_tags);
	ClassDB::bind_method(D_METHOD("get_allowed_tags"), &ConstraintSlot::get_allowed_tags);

	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "transform"), "set_transform", "get_transform");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "allowed_tags", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_allowed_tags", "get_allowed_tags");
}

void ConstraintProblem::_bind_methods() {
	BIND_ENUM_CONSTANT(TOPOLOGY_GROW);
	BIND_ENUM_CONSTANT(TOPOLOGY_FIXED);

	ClassDB::bind_method(D_METHOD("set_topology_mode", "mode"), &ConstraintProblem::set_topology_mode);
	ClassDB::bind_method(D_METHOD("get_topology_mode"), &ConstraintProblem::get_topology_mode);
	ClassDB::bind_method(D_METHOD("set_elements", "elements"), &ConstraintProblem::set_elements);
	ClassDB::bind_method(D_METHOD("get_elements"), &ConstraintProblem::get_elements);
	ClassDB::bind_method(D_METHOD("set_rules", "rules"), &ConstraintProblem::set_rules);
	ClassDB::bind_method(D_METHOD("get_rules"), &ConstraintProblem::get_rules);
	ClassDB::bind_method(D_METHOD("set_start_element", "id"), &ConstraintProblem::set_start_element);
	ClassDB::bind_method(D_METHOD("get_start_element"), &ConstraintProblem::get_start_element);
	ClassDB::bind_method(D_METHOD("set_min_elements", "count"), &ConstraintProblem::set_min_elements);
	ClassDB::bind_method(D_METHOD("get_min_elements"), &ConstraintProblem::get_min_elements);
	ClassDB::bind_method(D_METHOD("set_max_elements", "count"), &ConstraintProblem::set_max_elements);
	ClassDB::bind_method(D_METHOD("get_max_elements"), &ConstraintProblem::get_max_elements);
	ClassDB::bind_method(D_METHOD("set_slots", "slots"), &ConstraintProblem::set_slots);
	ClassDB::bind_method(D_METHOD("get_slots"), &ConstraintProblem::get_slots);
	ClassDB::bind_method(D_METHOD("set_connections", "connections"), &ConstraintProblem::set_connections);
	ClassDB::bind_method(D_METHOD("get_connections"), &ConstraintProblem::get_connections);
	ClassDB::bind_method(D_METHOD("set_ambient_tags", "tags"), &ConstraintProblem::set_ambient_tags);
	ClassDB::bind_method(D_METHOD("get_ambient_tags"), &ConstraintProblem::get_ambient_tags);
	ClassDB::bind_method(D_METHOD("set_seed", "seed"), &ConstraintProblem::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &ConstraintProblem::get_seed);
	ClassDB::bind_method(D_METHOD("set_max_steps", "steps"), &ConstraintProblem::set_max_steps);
	ClassDB::bind_method(D_METHOD("get_max_steps"), &ConstraintProblem::get_max_steps);
	ClassDB::bind_method(D_METHOD("set_time_budget_ms", "ms"), &ConstraintProblem::set_time_budget_ms);
	ClassDB::bind_method(D_METHOD("get_time_budget_ms"), &ConstraintProblem::get_time_budget_ms);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "topology_mode", PROPERTY_HINT_ENUM, "Grow,Fixed"),
			"set_topology_mode", "get_topology_mode");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "elements", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("ConstraintElement")),
			"set_elements", "get_elements");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rules", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("ConstraintRule")),
			"set_rules", "get_rules");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "start_element"), "set_start_element", "get_start_element");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_elements"), "set_min_elements", "get_min_elements");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_elements"), "set_max_elements", "get_max_elements");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "slots", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("ConstraintSlot")),
			"set_slots", "get_slots");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "connections"), "set_connections", "get_connections");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "ambient_tags", PROPERTY_HINT_TYPE_STRING, String::num(Variant::STRING_NAME) + ":"),
			"set_ambient_tags", "get_ambient_tags");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_steps"), "set_max_steps", "get_max_steps");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "time_budget_ms"), "set_time_budget_ms", "get_time_budget_ms");
}
