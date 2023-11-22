#ifndef GODOT_CLONE_SPRITEANIMATIONCONTROL_H
#define GODOT_CLONE_SPRITEANIMATIONCONTROL_H

#include <scene/2d/animated_sprite_2d.h>
#include "DirectionsUtil.h"
#include "SafeObjectPointer.h"

class SpriteAnimationControl : public AnimatedSprite2D {
	GDCLASS(SpriteAnimationControl, AnimatedSprite2D)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();
	void _notification(int p_notification);

	float AnimationSpeed = 1;
	float WalkAnimationThreshold = 200;
	bool FlashOnDamage = false;
	float FlashIntensity = 1;
	bool ReactToAttack = false;
	bool FlippedSprites = false;
	bool DirectionLess = false;

	int DirectionSourceIdle = 0;
	int DirectionSourceMove = 0;
	bool UpperBodyLowerBodySetup = false;
	NodePath CustomFacingProviderPath;
	SafeObjectPointer<Node> CustomFacingProvider;

	String IdleAnimationName = "idle";
	String WalkAnimationName = "walk";
	TypedArray<String> AttackAnimationNames;
	String InitialAnimation;


	void set_AnimationSpeed(float animSpeed) { AnimationSpeed = animSpeed; }
	float get_AnimationSpeed() { return AnimationSpeed; }
	void set_WalkAnimationThreshold(float walkAnimationThreshold) { WalkAnimationThreshold = walkAnimationThreshold; }
	float get_WalkAnimationThreshold() { return WalkAnimationThreshold; }
	void set_FlashOnDamage(bool flashOnDamage) { FlashOnDamage = flashOnDamage; }
	bool get_FlashOnDamage() { return FlashOnDamage; }
	void set_FlashIntensity(float flashIntensity) { FlashIntensity = flashIntensity; }
	float get_FlashIntensity() { return FlashIntensity; }
	void set_ReactToAttack(bool reactToAttack) { ReactToAttack = reactToAttack; }
	bool get_ReactToAttack() { return ReactToAttack; }
	void set_FlippedSprites(bool flippedSprites) { FlippedSprites = flippedSprites; }
	bool get_FlippedSprites() { return FlippedSprites; }
	void set_DirectionLess(bool directionLess) { DirectionLess = directionLess; }
	bool get_DirectionLess() { return DirectionLess; }
	void set_DirectionSourceIdle(int directionSourceIdle) { DirectionSourceIdle = directionSourceIdle; }
	int get_DirectionSourceIdle() { return DirectionSourceIdle; }
	void set_DirectionSourceMove(int directionSourceMove) { DirectionSourceMove = directionSourceMove; }
	int get_DirectionSourceMove() { return DirectionSourceMove; }
	void set_UpperBodyLowerBodySetup(bool upperBodyLowerBodySetup) { UpperBodyLowerBodySetup = upperBodyLowerBodySetup; }
	bool get_UpperBodyLowerBodySetup() { return UpperBodyLowerBodySetup; }
	void set_CustomFacingProvider(const NodePath& customFacingProviderPath) { CustomFacingProviderPath = customFacingProviderPath; }
	NodePath get_CustomFacingProvider() { return CustomFacingProviderPath; }
	void set_IdleAnimationName(String idleAnimationName) { IdleAnimationName = idleAnimationName; }
	String get_IdleAnimationName() { return IdleAnimationName; }
	void set_WalkAnimationName(String walkAnimationName) { WalkAnimationName = walkAnimationName; }
	String get_WalkAnimationName() { return WalkAnimationName; }
	void set_AttackAnimationNames(TypedArray<String> attackAnimationNames) { AttackAnimationNames = attackAnimationNames; }
	TypedArray<String> get_AttackAnimationNames() { return AttackAnimationNames; }
	void set_InitialAnimation(String initialAnimation) { InitialAnimation = initialAnimation; }
	String get_InitialAnimation() { return InitialAnimation; }


	DirectionsUtil::Dir directionState = DirectionsUtil::S;
	bool movementState = false;
	SafeObjectPointer<Node> velocityProvider;
	SafeObjectPointer<Node> facingProvider;
	SafeObjectPointer<Node> attackSpeedProvider;
	int current_attack_animation_index = -1;

	SafeObjectPointer<AnimatedSprite2D> shadow;
	int flash_timer = 0;

	// state of currently played animation
	String animationState = "idle";
	float secondsPerFrame = 1.0f / 15.0f;
	int frameCount = 1;
	float playbackSpeed = 1;
	bool playsBackwards = false;
	float frameTime = 0;

	Ref<ShaderMaterial> shaderMaterial;


public:
	static Ref<Shader> outlineshader;
	static float FlashModulation;
	static void set_flashmodulation(float newmodulation) { FlashModulation = newmodulation; }

	void _ready();
	void _process(float delta);
	void _process_animation(float delta);
	void _set_animation(String animationName, bool startAtFrameZero);
	void set_sprite_direction(Vector2 direction);
	void set_sprite_animation_state(String animationName, bool startAtFrameZero, bool backwards = false);
	void update_animation_state(bool startAtFrameZero);
	void update_animation_state_unflipped(bool startAtFrameZero);
	void update_animation_state_flipped(bool startAtFrameZero);
	void on_damage_received(int _amount, Node* _source, int _weapon_index);
	void update_flash();
	void on_attack_triggered(int attack_index);
	void on_animation_finished();
	void teleport_out_effect(float duration);
	void teleport_in_effect(float duration);
	void set_outline(Color color);
};

#endif //GODOT_CLONE_SPRITEANIMATIONCONTROL_H
