#include "Statistics.h"

#include "GameObject.h"
#include <unordered_map>
#include <core/profiling.h>

struct WeaponIndexData {
	int TotalDamageDealt;
	float DamageLast30Seconds;
	float Max30SecondDamage;
};
std::unordered_map<int, WeaponIndexData> GlobalWeaponData;
using WeaponDataIter = std::unordered_map<int, WeaponIndexData>::iterator;
std::unordered_map<int, int> GlobalHealingData;
float TotalDamageLast30Seconds;
float Max30SecondDamage;
// don't access anything on this pointer, it is only there to compare a gameobject to!
GameObject* Player;

void Stats::_bind_methods() {
	ClassDB::bind_static_method("Stats", D_METHOD("ResetStats", "playerGameObject"), &Stats::ResetStats);
	ClassDB::bind_static_method("Stats", D_METHOD("UpdateStats", "delta"), &Stats::UpdateStats);
	ClassDB::bind_static_method("Stats", D_METHOD("AddDamageEvent", "sourceGameObject", "targetGameObject", "damageAmount", "weaponIndex"), &Stats::AddDamageEvent);
	ClassDB::bind_static_method("Stats", D_METHOD("AddHealEvent", "targetGameObject", "healAmount", "weaponIndex"), &Stats::AddHealEvent);
	ClassDB::bind_static_method("Stats", D_METHOD("GetDPS"), &Stats::GetDPS);
	ClassDB::bind_static_method("Stats", D_METHOD("GetDamagingWeaponIndices"), &Stats::GetDamagingWeaponIndices);
	ClassDB::bind_static_method("Stats", D_METHOD("GetTotalDamageOfWeapon", "weaponIndex"), &Stats::GetTotalDamageOfWeapon);
	ClassDB::bind_static_method("Stats", D_METHOD("GetMaxDPSOfWeapon", "weaponIndex"), &Stats::GetMaxDPSOfWeapon);
	ClassDB::bind_static_method("Stats", D_METHOD("GetHealingWeaponIndices"), &Stats::GetHealingWeaponIndices);
	ClassDB::bind_static_method("Stats", D_METHOD("GetTotalHealingOfWeapon", "weaponIndex"), &Stats::GetTotalHealingOfWeapon);
}

void Stats::ResetStats(GameObject* playerGameObject) {
	GlobalWeaponData.clear();
	GlobalHealingData.clear();
	TotalDamageLast30Seconds = 0;
	Max30SecondDamage = 0;
	Player = playerGameObject;
}

void Stats::UpdateStats(float delta) {
	PROFILE_FUNCTION()
	for(auto& weaponData : GlobalWeaponData) {
		weaponData.second.DamageLast30Seconds -= weaponData.second.DamageLast30Seconds * delta / 30.0f;
	}
	TotalDamageLast30Seconds -= TotalDamageLast30Seconds * delta / 30.0f;
}

void Stats::AddDamageEvent(GameObject *sourceNode, GameObject *targetNode, int damageAmount, int weaponIndex) {
	PROFILE_FUNCTION()
	// we only want to count damage done by the player, for now...
	if(sourceNode == nullptr)
		return;

	auto rootSource = sourceNode->get_rootSourceGameObject();
	if(rootSource != Player)
		return;

	WeaponDataIter weaponData = GlobalWeaponData.find(weaponIndex);
	if(weaponData == GlobalWeaponData.end())
		weaponData = GlobalWeaponData.insert({weaponIndex, {}}).first;

	weaponData->second.TotalDamageDealt += damageAmount;
	weaponData->second.DamageLast30Seconds += damageAmount;
	weaponData->second.Max30SecondDamage = std::max(weaponData->second.Max30SecondDamage, weaponData->second.DamageLast30Seconds);

	TotalDamageLast30Seconds += damageAmount;
	Max30SecondDamage = std::max(Max30SecondDamage, TotalDamageLast30Seconds);
}

void Stats::AddHealEvent(GameObject *targetNode, int healAmount, int weaponIndex) {
	PROFILE_FUNCTION()
	// only count healing for the player.
	if(targetNode != Player)
		return;

	auto healingDataIter = GlobalHealingData.find(weaponIndex);
	if(healingDataIter == GlobalHealingData.end())
		healingDataIter = GlobalHealingData.insert({weaponIndex, {}}).first;

	healingDataIter->second += healAmount;
}

float Stats::GetDPS() {
	return TotalDamageLast30Seconds / 30.0f;
}

Array Stats::GetDamagingWeaponIndices() {
	Array allWeaponIndices;
	for(const auto weaponData : GlobalWeaponData)
		allWeaponIndices.append(weaponData.first);
	return allWeaponIndices;
}

int Stats::GetTotalDamageOfWeapon(int weaponIndex) {
	WeaponDataIter weaponData = GlobalWeaponData.find(weaponIndex);
	if(weaponData == GlobalWeaponData.end()) {
		// print_error(String("There are no statistics for this weapon index! weaponIndex=")+ itos(weaponIndex));
		return 0;
	}
	return weaponData->second.TotalDamageDealt;
}

float Stats::GetMaxDPSOfWeapon(int weaponIndex) {
	WeaponDataIter weaponData = GlobalWeaponData.find(weaponIndex);
	if(weaponData == GlobalWeaponData.end()) {
		// print_error(String("There are no damage statistics for this weapon index! weaponIndex=")+ itos(weaponIndex));
		return 0;
	}
	return weaponData->second.Max30SecondDamage / 30.0f;
}

Array Stats::GetHealingWeaponIndices() {
	Array allWeaponIndices;
	for(const auto healingData : GlobalHealingData)
		allWeaponIndices.append(healingData.first);
	return allWeaponIndices;
}

int Stats::GetTotalHealingOfWeapon(int weaponIndex) {
	auto healingDataIter = GlobalHealingData.find(weaponIndex);
	if(healingDataIter == GlobalHealingData.end()) {
		// print_error(String("There are no healing statistics for this weapon index! weaponIndex=")+ itos(weaponIndex));
		return 0;
	}
	return healingDataIter->second;
}
