#include "ApplyEffectOnSignal.h"

#include "Health.h"
#include "LocatorSystem.h"

#include <core/profiling.h>
#include <core/variant/variant_utility.h>

void ApplyEffectOnSignal::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_EffectNode", "effectNode"), &ApplyEffectOnSignal::SetEffectNode);
	ClassDB::bind_method(D_METHOD("get_EffectNode"), &ApplyEffectOnSignal::GetEffectNode);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "EffectScene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_EffectNode", "get_EffectNode");

	ClassDB::bind_method(D_METHOD("set_SignalName", "signalName"), &ApplyEffectOnSignal::SetSignalName);
	ClassDB::bind_method(D_METHOD("get_SignalName"), &ApplyEffectOnSignal::GetSignalName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "SignalName"), "set_SignalName", "get_SignalName");

	ClassDB::bind_method(D_METHOD("_on_signal", "target", "source"), &ApplyEffectOnSignal::_on_signal);
}

void ApplyEffectOnSignal::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_ENTER_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
		break;
	}
}

void ApplyEffectOnSignal::_ready() {
	PROFILE_FUNCTION()
	init_gameobject_component();
	if (_gameObject.is_valid()) {
		_gameObject->connectToSignal(SignalName, callable_mp(this, &ApplyEffectOnSignal::_on_signal));
	}
}

void ApplyEffectOnSignal::_on_signal(GameObject *target, GameObject *source) {
	PROFILE_FUNCTION()
	target->add_effect(EffectNode.ptr(), source);
}
