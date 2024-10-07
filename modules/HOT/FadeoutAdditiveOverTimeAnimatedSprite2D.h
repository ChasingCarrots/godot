#ifndef FADEOUTADDITIVEOVERTIMEANIMATEDSPRITE2D_H
#define FADEOUTADDITIVEOVERTIMEANIMATEDSPRITE2D_H

#include <scene/2d/animated_sprite_2d.h>

class FadeoutAdditiveOverTimeAnimatedSprite2D : public AnimatedSprite2D {
  GDCLASS(FadeoutAdditiveOverTimeAnimatedSprite2D, AnimatedSprite2D)

protected:
  // Required entry point that the API calls to bind our class to Godot.
  static void _bind_methods();
  void _ready();
  void _exit_tree();
  void _notification(int p_notification);

  float FadeEase;
  bool UseFixedLifetime = true;
  float Lifetime = 1.0f;

  Color _start_color;
  Color _end_color;
  Ref<ShaderMaterial> _material_as_shadermat;
  float _last_updated_factor = 999;
public:
  static void updateAll(float delta);
  static void clearAll();

  void _on_bullet_on_time_reduced(float remainingTimeFactor);

  [[nodiscard]] float GetFadeEase() const { return FadeEase; }
  void SetFadeEase(float fadeEase) { FadeEase = fadeEase; }
  [[nodiscard]] bool GetUseFixedLifetime() const { return UseFixedLifetime; }
  void SetUseFixedLifetime(bool useFixedLifetime) { UseFixedLifetime = useFixedLifetime; }
  [[nodiscard]] float GetLifetime() const { return Lifetime; }
  void SetLifetime(float lifetime) { Lifetime = lifetime; }
};



#endif //FADEOUTADDITIVEOVERTIMEANIMATEDSPRITE2D_H
