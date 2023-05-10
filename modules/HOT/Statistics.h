#ifndef GODOT_SOURCE_STATISTICS_H
#define GODOT_SOURCE_STATISTICS_H

#include <core/object/ref_counted.h>

class GameObject;

class Stats : public Object {
	GDCLASS(Stats, Object)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:

	// those are all just static methods, because there is only one central Stats and
	// having an object would only cost performance without benefit (however slight it might be...)

	static void ResetStats(GameObject* playerGameObject);

	static void UpdateStats(float delta);

	static void AddDamageEvent(GameObject* sourceNode, GameObject* targetNode, int damageAmount, int weaponIndex);

	static void AddHealEvent(GameObject* targetNode, int healAmount, int weaponIndex);

	static float GetDPS();

	static Array GetDamagingWeaponIndices();

	static int GetTotalDamageOfWeapon(int weaponIndex);

	static float GetMaxDPSOfWeapon(int weaponIndex);

	static Array GetHealingWeaponIndices();

	static int GetTotalHealingOfWeapon(int weaponIndex);

};


#endif //GODOT_SOURCE_STATISTICS_H
