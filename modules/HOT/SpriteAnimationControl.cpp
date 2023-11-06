#include "SpriteAnimationControl.h"

#include <core/profiling.h>
#include <core/math/math_funcs.h>
#include <scene/animation/tween.h>
#include "GameObject.h"

float SpriteAnimationControl::FlashModulation = 4;
Ref<Shader> SpriteAnimationControl::outlineshader;

void SpriteAnimationControl::_bind_methods() {
	ClassDB::bind_static_method("SpriteAnimationControl", D_METHOD("set_flashmodulation", "newmodulation"), &SpriteAnimationControl::set_flashmodulation);

	ClassDB::bind_method(D_METHOD("set_AnimationSpeed", "animationSpeed"), &SpriteAnimationControl::set_AnimationSpeed);
	ClassDB::bind_method(D_METHOD("get_AnimationSpeed"), &SpriteAnimationControl::get_AnimationSpeed);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "AnimationSpeed"), "set_AnimationSpeed", "get_AnimationSpeed");
	ClassDB::bind_method(D_METHOD("set_WalkAnimationThreshold", "WalkAnimationThreshold"), &SpriteAnimationControl::set_WalkAnimationThreshold);
	ClassDB::bind_method(D_METHOD("get_WalkAnimationThreshold"), &SpriteAnimationControl::get_WalkAnimationThreshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "WalkAnimationThreshold"), "set_WalkAnimationThreshold", "get_WalkAnimationThreshold");
	ClassDB::bind_method(D_METHOD("set_FlashOnDamage", "FlashOnDamage"), &SpriteAnimationControl::set_FlashOnDamage);
	ClassDB::bind_method(D_METHOD("get_FlashOnDamage"), &SpriteAnimationControl::get_FlashOnDamage);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "FlashOnDamage"), "set_FlashOnDamage", "get_FlashOnDamage");
	ClassDB::bind_method(D_METHOD("set_FlashIntensity", "FlashIntensity"), &SpriteAnimationControl::set_FlashIntensity);
	ClassDB::bind_method(D_METHOD("get_FlashIntensity"), &SpriteAnimationControl::get_FlashIntensity);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "FlashIntensity"), "set_FlashIntensity", "get_FlashIntensity");
	ClassDB::bind_method(D_METHOD("set_ReactToAttack", "ReactToAttack"), &SpriteAnimationControl::set_ReactToAttack);
	ClassDB::bind_method(D_METHOD("get_ReactToAttack"), &SpriteAnimationControl::get_ReactToAttack);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ReactToAttack"), "set_ReactToAttack", "get_ReactToAttack");
	ClassDB::bind_method(D_METHOD("set_FlippedSprites", "FlippedSprites"), &SpriteAnimationControl::set_FlippedSprites);
	ClassDB::bind_method(D_METHOD("get_FlippedSprites"), &SpriteAnimationControl::get_FlippedSprites);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "FlippedSprites"), "set_FlippedSprites", "get_FlippedSprites");
	ClassDB::bind_method(D_METHOD("set_DirectionLess", "DirectionLess"), &SpriteAnimationControl::set_DirectionLess);
	ClassDB::bind_method(D_METHOD("get_DirectionLess"), &SpriteAnimationControl::get_DirectionLess);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "DirectionLess"), "set_DirectionLess", "get_DirectionLess");
	ClassDB::bind_method(D_METHOD("set_DirectionSourceIdle", "DirectionSourceIdle"), &SpriteAnimationControl::set_DirectionSourceIdle);
	ClassDB::bind_method(D_METHOD("get_DirectionSourceIdle"), &SpriteAnimationControl::get_DirectionSourceIdle);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "DirectionSourceIdle"), "set_DirectionSourceIdle", "get_DirectionSourceIdle");
	ClassDB::bind_method(D_METHOD("set_DirectionSourceMove", "DirectionSourceMove"), &SpriteAnimationControl::set_DirectionSourceMove);
	ClassDB::bind_method(D_METHOD("get_DirectionSourceMove"), &SpriteAnimationControl::get_DirectionSourceMove);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "DirectionSourceMove"), "set_DirectionSourceMove", "get_DirectionSourceMove");
	ClassDB::bind_method(D_METHOD("set_UpperBodyLowerBodySetup", "UpperBodyLowerBodySetup"), &SpriteAnimationControl::set_UpperBodyLowerBodySetup);
	ClassDB::bind_method(D_METHOD("get_UpperBodyLowerBodySetup"), &SpriteAnimationControl::get_UpperBodyLowerBodySetup);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "UpperBodyLowerBodySetup"), "set_UpperBodyLowerBodySetup", "get_UpperBodyLowerBodySetup");
	ClassDB::bind_method(D_METHOD("set_CustomFacingProvider", "CustomFacingProvider"), &SpriteAnimationControl::set_CustomFacingProvider);
	ClassDB::bind_method(D_METHOD("get_CustomFacingProvider"), &SpriteAnimationControl::get_CustomFacingProvider);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "CustomFacingProvider", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node"), "set_CustomFacingProvider", "get_CustomFacingProvider");
	ClassDB::bind_method(D_METHOD("set_IdleAnimationName", "IdleAnimationName"), &SpriteAnimationControl::set_IdleAnimationName);
	ClassDB::bind_method(D_METHOD("get_IdleAnimationName"), &SpriteAnimationControl::get_IdleAnimationName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "IdleAnimationName"), "set_IdleAnimationName", "get_IdleAnimationName");
	ClassDB::bind_method(D_METHOD("set_WalkAnimationName", "WalkAnimationName"), &SpriteAnimationControl::set_WalkAnimationName);
	ClassDB::bind_method(D_METHOD("get_WalkAnimationName"), &SpriteAnimationControl::get_WalkAnimationName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "WalkAnimationName"), "set_WalkAnimationName", "get_WalkAnimationName");
	ClassDB::bind_method(D_METHOD("set_AttackAnimationNames", "AttackAnimationNames"), &SpriteAnimationControl::set_AttackAnimationNames);
	ClassDB::bind_method(D_METHOD("get_AttackAnimationNames"), &SpriteAnimationControl::get_AttackAnimationNames);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "AttackAnimationNames", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("String")), "set_AttackAnimationNames", "get_AttackAnimationNames");
	ClassDB::bind_method(D_METHOD("set_InitialAnimation", "InitialAnimation"), &SpriteAnimationControl::set_InitialAnimation);
	ClassDB::bind_method(D_METHOD("get_InitialAnimation"), &SpriteAnimationControl::get_InitialAnimation);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "InitialAnimation"), "set_InitialAnimation", "get_InitialAnimation");

	ClassDB::bind_method(D_METHOD("_process_animation", "delta"), &SpriteAnimationControl::_process_animation);
	ClassDB::bind_method(D_METHOD("_set_animation", "animationName", "startAtFrameZero"), &SpriteAnimationControl::_set_animation);
	ClassDB::bind_method(D_METHOD("set_sprite_direction", "direction"), &SpriteAnimationControl::set_sprite_direction);
	ClassDB::bind_method(D_METHOD("set_sprite_animation_state", "animationName", "startAtFrameZero", "backwards"), &SpriteAnimationControl::set_sprite_animation_state, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("update_animation_state", "startAtFrameZero"), &SpriteAnimationControl::update_animation_state);
	ClassDB::bind_method(D_METHOD("update_animation_state_unflipped", "startAtFrameZero"), &SpriteAnimationControl::update_animation_state_unflipped);
	ClassDB::bind_method(D_METHOD("update_animation_state_flipped", "startAtFrameZero"), &SpriteAnimationControl::update_animation_state_flipped);
	ClassDB::bind_method(D_METHOD("on_damage_received", "_amount", "_source", "_weapon_index"), &SpriteAnimationControl::on_damage_received);
	ClassDB::bind_method(D_METHOD("update_flash"), &SpriteAnimationControl::update_flash);
	ClassDB::bind_method(D_METHOD("on_attack_triggered", "attack_index"), &SpriteAnimationControl::on_attack_triggered);
	ClassDB::bind_method(D_METHOD("on_animation_finished"), &SpriteAnimationControl::on_animation_finished);
	ClassDB::bind_method(D_METHOD("teleport_out_effect", "duration"), &SpriteAnimationControl::teleport_out_effect);
	ClassDB::bind_method(D_METHOD("teleport_in_effect", "duration"), &SpriteAnimationControl::teleport_in_effect);
	ClassDB::bind_method(D_METHOD("set_outline", "color"), &SpriteAnimationControl::set_outline);

	ADD_SIGNAL(MethodInfo("animation_cycle_finished"));
}

void SpriteAnimationControl::_notification(int p_notification) {
	switch(p_notification) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				_ready();
			break;
		case NOTIFICATION_PROCESS:
			_process(get_process_delta_time());
			break;
	}
}

void SpriteAnimationControl::_ready() {
	PROFILE_FUNCTION();

	if(!outlineshader.is_valid())
		outlineshader = ResourceLoader::load("res://Shaders/OutlinedSpriteShader.gdshader");
	shaderMaterial = get_material()->duplicate();
	set_material(shaderMaterial);

	if(!CustomFacingProviderPath.is_empty())
		CustomFacingProvider = get_node(CustomFacingProviderPath);

	GameObject* parentGameObj = GameObject::getGameObjectInParents(this);
	if(parentGameObj != nullptr) {
		velocityProvider = parentGameObj->getChildNodeWithMethod("get_targetVelocity");
		if(CustomFacingProvider.is_valid()) facingProvider = CustomFacingProvider;
		else facingProvider = parentGameObj->getChildNodeWithMethod("get_facingDirection");
		attackSpeedProvider = parentGameObj->getChildNodeWithMethod("get_attackSpeedFactor");
		parentGameObj->connectToSignal("ReceivedDamage", Callable(this, "on_damage_received"));
		parentGameObj->connectToSignal("AttackTriggered", Callable(this, "on_attack_triggered"));
		parentGameObj->connectToSignal("StartTeleport", Callable(this, "teleport_out_effect"));
		parentGameObj->connectToSignal("EndTeleport", Callable(this, "teleport_in_effect"));
	}
	pause();

	if(!InitialAnimation.is_empty())
		set_sprite_animation_state(InitialAnimation, true);

	if(has_node(NodePath("shadow"))){
		shadow = Object::cast_to<AnimatedSprite2D>(get_node(NodePath("shadow")));
		if(shadow.is_valid()) {
			if(FlippedSprites) shadow->set_flip_h(is_flipped_h());
			shadow->pause();
		}
	}

	set_process(true);
}

void SpriteAnimationControl::_process(float delta) {
	PROFILE_FUNCTION();
	_process_animation(delta);
	update_flash();

	Vector2 targetVel;
	Vector2 facingVector;
	Vector2 spriteDirection;

	if(velocityProvider.is_valid())
		targetVel = velocityProvider->callv("get_targetVelocity", {});
	if(facingProvider.is_valid())
		facingVector = facingProvider->callv("get_facingDirection", {});

	if(animationState == IdleAnimationName){
		if(DirectionSourceIdle == 0) spriteDirection = targetVel;
		else spriteDirection = facingVector;
	}
	else if(animationState == WalkAnimationName) {
		if(DirectionSourceMove == 0) spriteDirection = targetVel;
		else spriteDirection = facingVector;
	}
	else {
		spriteDirection = facingVector;
	}

	if(velocityProvider.is_valid()) {
		bool new_movement_state = ((Vector2)velocityProvider->callv("get_velocity", {})).length_squared() >= WalkAnimationThreshold;
		if(UpperBodyLowerBodySetup) {
			DirectionsUtil::Dir facingDir = DirectionsUtil::get_direction_from_vector(facingVector);
			DirectionsUtil::Dir velocityDir = DirectionsUtil::get_direction_from_vector(targetVel);
			if(new_movement_state &&
					DirectionsUtil::get_angle_between_directions(facingDir, velocityDir) > 90)
			{
				spriteDirection *= -1;
				playsBackwards = true;
			}
			else {
				playsBackwards = false;
			}
			set_sprite_animation_state(new_movement_state ? WalkAnimationName : IdleAnimationName, false);
			movementState = new_movement_state;
		}
		else if(new_movement_state != movementState) {
			set_sprite_animation_state(new_movement_state ? WalkAnimationName : IdleAnimationName, false);
			movementState = new_movement_state;
		}
	}
	set_sprite_direction(spriteDirection);
}

void SpriteAnimationControl::_process_animation(float delta) {
	PROFILE_FUNCTION();
	frameTime += delta * playbackSpeed * AnimationSpeed;
	if(frameTime >= secondsPerFrame) {
		frameTime -= secondsPerFrame;
		if(playsBackwards)
			set_frame(Math::wrapi(get_frame() - 1, 0, frameCount));
		else
			set_frame(Math::wrapi(get_frame() + 1, 0, frameCount));
		if(get_frame() == 0) on_animation_finished();
		if(shadow.is_valid()) {
			shadow->set_frame(get_frame());
			if(FlippedSprites) shadow->set_flip_h(is_flipped_h());
		}
	}
}

void SpriteAnimationControl::_set_animation(String animationName, bool startAtFrameZero) {
	PROFILE_FUNCTION();
	auto sprite_frames = get_sprite_frames();
	if(!sprite_frames.is_valid())
		return;
	if(!sprite_frames->has_animation(animationName))
		return;
	int currentFrame = get_frame();
	set_animation(animationName);
	if(shadow.is_valid()) {
		shadow->set_animation(animationName);
		shadow->set_frame(currentFrame);
		if (FlippedSprites)
			shadow->set_flip_h(is_flipped_h());
	}
	frameCount = sprite_frames->get_frame_count(animationName);
	secondsPerFrame = 1.0f / sprite_frames->get_animation_speed(animationName);
	if(startAtFrameZero) {
		if (playsBackwards) {
			set_frame(frameCount - 1);
			frameTime = secondsPerFrame;
		} else {
			set_frame(0);
			frameTime = 0.0;
		}
	}
	else
		set_frame(currentFrame);
}

void SpriteAnimationControl::set_sprite_direction(Vector2 direction) {
	PROFILE_FUNCTION();
	if(DirectionLess) {
		directionState = DirectionsUtil::Dir::S;
		update_animation_state(false);
		return;
	}
	DirectionsUtil::Dir newDirection = DirectionsUtil::get_direction_from_vector(direction);
	if(newDirection != directionState) {
		directionState = newDirection;
		update_animation_state(false);
	}
}

void SpriteAnimationControl::set_sprite_animation_state(String animationName, bool startAtFrameZero, bool backwards) {
	PROFILE_FUNCTION();
	if(ReactToAttack) {
		if(current_attack_animation_index >= 0 && !AttackAnimationNames.has(animationName))
			return;
	}
	if(animationName != animationState) {
		playsBackwards = backwards;
		animationState = animationName;
		update_animation_state(startAtFrameZero);
	}
}

void SpriteAnimationControl::update_animation_state(bool startAtFrameZero) {
	if(FlippedSprites) update_animation_state_flipped(startAtFrameZero);
	else update_animation_state_unflipped(startAtFrameZero);
}

void SpriteAnimationControl::update_animation_state_unflipped(bool startAtFrameZero) {
	PROFILE_FUNCTION();
	switch(directionState) {
		case DirectionsUtil::Dir::S: _set_animation(animationState+"_S", startAtFrameZero); break;
		case DirectionsUtil::Dir::SSE: _set_animation(animationState+"_SSE", startAtFrameZero); break;
		case DirectionsUtil::Dir::SE: _set_animation(animationState+"_SE", startAtFrameZero); break;
		case DirectionsUtil::Dir::SEE: _set_animation(animationState+"_SEE", startAtFrameZero); break;
		case DirectionsUtil::Dir::E: _set_animation(animationState+"_E", startAtFrameZero); break;
		case DirectionsUtil::Dir::NEE: _set_animation(animationState+"_NEE", startAtFrameZero); break;
		case DirectionsUtil::Dir::NE: _set_animation(animationState+"_NE", startAtFrameZero); break;
		case DirectionsUtil::Dir::NNE: _set_animation(animationState+"_NNE", startAtFrameZero); break;
		case DirectionsUtil::Dir::N: _set_animation(animationState+"_N", startAtFrameZero); break;
		case DirectionsUtil::Dir::NNW: _set_animation(animationState+"_NNW", startAtFrameZero); break;
		case DirectionsUtil::Dir::NW: _set_animation(animationState+"_NW", startAtFrameZero); break;
		case DirectionsUtil::Dir::NWW: _set_animation(animationState+"_NWW", startAtFrameZero); break;
		case DirectionsUtil::Dir::W: _set_animation(animationState+"_W", startAtFrameZero); break;
		case DirectionsUtil::Dir::SWW: _set_animation(animationState+"_SWW", startAtFrameZero); break;
		case DirectionsUtil::Dir::SW: _set_animation(animationState+"_SW", startAtFrameZero); break;
		case DirectionsUtil::Dir::SSW: _set_animation(animationState+"_SSW", startAtFrameZero); break;
	}
}

void SpriteAnimationControl::update_animation_state_flipped(bool startAtFrameZero) {
	PROFILE_FUNCTION();
	switch(directionState) {
		case DirectionsUtil::Dir::S:
			_set_animation(animationState+"_S", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::SSE:
			_set_animation(animationState+"_SSE", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::SE:
			_set_animation(animationState+"_SE", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::SEE:
			_set_animation(animationState+"_SEE", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::E:
			_set_animation(animationState+"_E", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::NEE:
			_set_animation(animationState+"_NEE", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::NE:
			_set_animation(animationState+"_NE", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::NNE:
			_set_animation(animationState+"_NNE", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::N:
			_set_animation(animationState+"_N", startAtFrameZero);
			set_flip_h(false);
			break;
		case DirectionsUtil::Dir::NNW:
			_set_animation(animationState+"_NNE", startAtFrameZero);
			set_flip_h(true);
			break;
		case DirectionsUtil::Dir::NW:
			_set_animation(animationState+"_NE", startAtFrameZero);
			set_flip_h(true);
			break;
		case DirectionsUtil::Dir::NWW:
			_set_animation(animationState+"_NEE", startAtFrameZero);
			set_flip_h(true);
			break;
		case DirectionsUtil::Dir::W:
			_set_animation(animationState+"_E", startAtFrameZero);
			set_flip_h(true);
			break;
		case DirectionsUtil::Dir::SWW:
			_set_animation(animationState+"_SEE", startAtFrameZero);
			set_flip_h(true);
			break;
		case DirectionsUtil::Dir::SW:
			_set_animation(animationState+"_SE", startAtFrameZero);
			set_flip_h(true);
			break;
		case DirectionsUtil::Dir::SSW:
			_set_animation(animationState+"_SSE", startAtFrameZero);
			set_flip_h(true);
			break;
	}
}

void SpriteAnimationControl::on_damage_received(int _amount, Node *_source, int _weapon_index) {
	if(FlashOnDamage) {
		flash_timer = 4;
		shaderMaterial->set_shader_parameter("flash_modifier", FlashIntensity * FlashModulation);
	}
}

void SpriteAnimationControl::update_flash() {
	if(flash_timer > 0) {
		flash_timer -= 1;
		if(flash_timer == 0) {
			shaderMaterial->set_shader_parameter("flash_modifier", 0.0f);
		}
	}
}

void SpriteAnimationControl::on_attack_triggered(int attack_index) {
	PROFILE_FUNCTION();
	if(!ReactToAttack || attack_index < 0)
		return;
	if(attack_index == current_attack_animation_index || AttackAnimationNames.size() <= attack_index)
		return;
	if(attackSpeedProvider.is_valid())
		playbackSpeed = attackSpeedProvider->callv("get_attackSpeedFactor", {});
	set_sprite_animation_state(AttackAnimationNames[attack_index], true);
	current_attack_animation_index = attack_index;
}

void SpriteAnimationControl::on_animation_finished() {
	PROFILE_FUNCTION();
	emit_signal("animation_cycle_finished");
	if(!AttackAnimationNames.is_empty() && current_attack_animation_index >= 0) {
		current_attack_animation_index = -1;
		playbackSpeed = 1.0f;
		if(movementState)
			animationState = WalkAnimationName;
		else
			animationState = IdleAnimationName;
		update_animation_state(true);
	}
}

void SpriteAnimationControl::teleport_out_effect(float duration) {
	Ref<Tween> tween = create_tween();
	tween->tween_property(this, NodePath("scale"), Vector2(0.1f, 1.2f), duration)->from(Vector2(1,1));
}

void SpriteAnimationControl::teleport_in_effect(float duration) {
	Ref<Tween> tween = create_tween();
	tween->tween_property(this, NodePath("scale"), Vector2(1,1), duration)->from(Vector2(0.1f, 1.2f));
}

void SpriteAnimationControl::set_outline(Color color) {
	if(shaderMaterial->get_shader() != outlineshader)
		shaderMaterial->set_shader(outlineshader);
	shaderMaterial->set_shader_parameter("line_color", color);
}

