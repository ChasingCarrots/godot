#include "FadeoutAdditiveOverTimeSprite2D.h"

#include "GameObject.h"

#include <core/variant/variant_utility.h>

static const float ONLY_UPDATE_EVERY_FACTOR = 1.0f / 15.0f;

struct InstanceData {
	FadeoutAdditiveOverTimeSprite2D *Instance;
	Ref<ShaderMaterial> Material;
	Color StartColor;
	Color EndColor;
	float Lifetime;
	float RemainingLifetime;
	float LastUpdatedFactor;
	float FadeEase;
};
static LocalVector<InstanceData> Instances;

void FadeoutAdditiveOverTimeSprite2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_FadeEase", "fadeEase"), &FadeoutAdditiveOverTimeSprite2D::SetFadeEase);
	ClassDB::bind_method(D_METHOD("get_FadeEase"), &FadeoutAdditiveOverTimeSprite2D::GetFadeEase);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "FadeEase"), "set_FadeEase", "get_FadeEase");

	ClassDB::bind_method(D_METHOD("set_UseFixedLifetime", "useFixedLifetime"), &FadeoutAdditiveOverTimeSprite2D::SetUseFixedLifetime);
	ClassDB::bind_method(D_METHOD("get_UseFixedLifetime"), &FadeoutAdditiveOverTimeSprite2D::GetUseFixedLifetime);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "UseFixedLifetime"), "set_UseFixedLifetime", "get_UseFixedLifetime");

	ClassDB::bind_method(D_METHOD("set_Lifetime", "lifetime"), &FadeoutAdditiveOverTimeSprite2D::SetLifetime);
	ClassDB::bind_method(D_METHOD("get_Lifetime"), &FadeoutAdditiveOverTimeSprite2D::GetLifetime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Lifetime"), "set_Lifetime", "get_Lifetime");

	ClassDB::bind_method(D_METHOD("_on_bullet_on_time_reduced", "remainingTimeFactor"), &FadeoutAdditiveOverTimeSprite2D::_on_bullet_on_time_reduced);

	ClassDB::bind_static_method("FadeoutAdditiveOverTimeSprite2D", D_METHOD("updateAll", "delta"), &FadeoutAdditiveOverTimeSprite2D::updateAll);
	ClassDB::bind_static_method("FadeoutAdditiveOverTimeSprite2D", D_METHOD("clearAll"), &FadeoutAdditiveOverTimeSprite2D::clearAll);
}

void FadeoutAdditiveOverTimeSprite2D::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
		break;
		case NOTIFICATION_EXIT_TREE:
			_exit_tree();
		break;
	}
}

void FadeoutAdditiveOverTimeSprite2D::_ready() {
	PROFILE_FUNCTION()
	if(GameObject::Global()->call("is_world_ready"))
		set_material(GameObject::World()->get("Materials").call("get_material_duplicate", get_material()));
	else
		set_material(get_material()->duplicate());
	_material_as_shadermat = get_material();
	_start_color = _material_as_shadermat->get_shader_parameter("modulate_color");
	_end_color = _start_color;
	_end_color.a = 0;

	if(UseFixedLifetime) {
		InstanceData myData;
		myData.Instance = this;
		myData.RemainingLifetime = Lifetime;
		myData.Lifetime = Lifetime;
		myData.LastUpdatedFactor = 999;
		myData.StartColor = _start_color;
		myData.EndColor = _end_color;
		myData.Material = _material_as_shadermat;
		myData.FadeEase = FadeEase;
		Instances.push_back(myData);
	}
}

void FadeoutAdditiveOverTimeSprite2D::_exit_tree() {
	PROFILE_FUNCTION()
	if(UseFixedLifetime) {
		for(int i=0; i<Instances.size(); i++) {
			if(Instances[i].Instance == this) {
				Instances.remove_at_unordered(i);
				break;
			}
		}
	}
	if(_material_as_shadermat.is_valid()) {
		_material_as_shadermat->set_shader_parameter("modulate_color", _start_color);
		GameObject::World()->get("Materials").call("return_material", _material_as_shadermat);
	}
}

void FadeoutAdditiveOverTimeSprite2D::updateAll(float delta) {
	PROFILE_FUNCTION()
	for(auto& instance : Instances) {
		float factor = instance.RemainingLifetime / instance.Lifetime;
		instance.RemainingLifetime -= delta;

		// every skipped shader update is a win, since no _redraw_callback happens then!
		if(instance.LastUpdatedFactor - factor > ONLY_UPDATE_EVERY_FACTOR) {
			instance.LastUpdatedFactor = factor;
			instance.Material->set_shader_parameter("modulate_color",
				instance.EndColor.lerp(instance.StartColor, (float)VariantUtilityFunctions::ease(factor, instance.FadeEase)));
		}
	}
}

void FadeoutAdditiveOverTimeSprite2D::clearAll() {
	Instances.clear();
}

void FadeoutAdditiveOverTimeSprite2D::_on_bullet_on_time_reduced(float remainingTimeFactor) {
	if(_last_updated_factor - remainingTimeFactor > ONLY_UPDATE_EVERY_FACTOR) {
		_last_updated_factor = remainingTimeFactor;
		_material_as_shadermat->set_shader_parameter("modulate_color",
				_end_color.lerp(_start_color, (float)VariantUtilityFunctions::ease(remainingTimeFactor, FadeEase)));
	}
}

