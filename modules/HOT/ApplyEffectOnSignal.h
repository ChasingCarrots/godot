#ifndef APPLYEFFECTONSIGNAL_H
#define APPLYEFFECTONSIGNAL_H


#include "GameObjectComponents.h"
#include "StatisticsValueData.h"

class ApplyEffectOnSignal : public GameObjectComponent {
	GDCLASS(ApplyEffectOnSignal, GameObjectComponent)

protected:
  // Required entry point that the API calls to bind our class to Godot.
  static void _bind_methods();

  void _notification(int p_notification);
  void _ready();

private:
  Ref<PackedScene> EffectNode;
  String SignalName;

public:
  void _on_signal(GameObject *target, GameObject *source);

	[[nodiscard]] Ref<PackedScene> GetEffectNode() const { return EffectNode; }
	void SetEffectNode(const Ref<PackedScene> &effectNode) { EffectNode = effectNode; }
	[[nodiscard]] String GetSignalName() const { return SignalName; }
	void SetSignalName(String signalName) { SignalName = signalName; }
};



#endif //APPLYEFFECTONSIGNAL_H
