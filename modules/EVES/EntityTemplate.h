#ifndef ENTITYTEMPLATE_H
#define ENTITYTEMPLATE_H

#include "System.h"

namespace eves {


class TemplateValuesFromSystemResource : public Resource {
	GDCLASS(TemplateValuesFromSystemResource, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_belongs_to_system", "system"), &TemplateValuesFromSystemResource::SetBelongsToSystem);
		ClassDB::bind_method(D_METHOD("get_belongs_to_system"), &TemplateValuesFromSystemResource::GetBelongsToSystem);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "BelongsToSystem", PROPERTY_HINT_RESOURCE_TYPE, "SystemResource"), "set_belongs_to_system", "get_belongs_to_system");
	}

	Ref<SystemResource> _belongsToSystem;
	TypedArray<FlatValueResource> _staticValues;
public:
	Ref<SystemResource> GetBelongsToSystem() const { return _belongsToSystem; }
	void SetBelongsToSystem(Ref<SystemResource> belongsToSystem) { _belongsToSystem = belongsToSystem; }
};




class EntityTemplate : public Resource {
	GDCLASS(EntityTemplate, Resource);

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods() {

	}

	TypedArray<TemplateValuesFromSystemResource> _systemResources;
public:

};

}

#endif //ENTITYTEMPLATE_H
