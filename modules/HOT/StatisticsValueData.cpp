#include "StatisticsValueData.h"

#include "ModifiedValues.h"

void StatisticsValueData::_bind_methods() {
	BIND_ENUM_CONSTANT(PlayerBaseStats);
	BIND_ENUM_CONSTANT(AbilityStats);

	BIND_ENUM_CONSTANT(FlatValue);
	BIND_ENUM_CONSTANT(Percentage);
	BIND_ENUM_CONSTANT(Speed);
	BIND_ENUM_CONSTANT(SpeedInv);
	BIND_ENUM_CONSTANT(Distance);
	BIND_ENUM_CONSTANT(Force);
	BIND_ENUM_CONSTANT(SpeedDist);
	BIND_ENUM_CONSTANT(Float);
	BIND_ENUM_CONSTANT(Angle);
	BIND_ENUM_CONSTANT(Time);
	BIND_ENUM_CONSTANT(Multiplier);
	BIND_ENUM_CONSTANT(Rotation);

	BIND_ENUM_CONSTANT(None);
	BIND_ENUM_CONSTANT(Defense);
	BIND_ENUM_CONSTANT(Block);
	BIND_ENUM_CONSTANT(Attacks);

	ClassDB::bind_static_method("StatisticsValueData", D_METHOD("createFromModifiedValue", "modifiedValue", "displayName", "statsCategory", "format", "specialInfo"), &StatisticsValueData::createFromModifiedValue);
	ClassDB::bind_static_method("StatisticsValueData", D_METHOD("createNew", "displayName", "statsCategory", "modifierKey", "baseVal", "finalVal", "categories", "format", "specialInfo"), &StatisticsValueData::createNew);

	ClassDB::bind_method(D_METHOD("set_DisplayName", "displayName"), &StatisticsValueData::SetDisplayName);
	ClassDB::bind_method(D_METHOD("get_DisplayName"), &StatisticsValueData::GetDisplayName);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "DisplayName"), "set_DisplayName", "get_DisplayName");
	ClassDB::bind_method(D_METHOD("set_StatisticsCategory", "statisticsCategory"), &StatisticsValueData::SetStatisticsCategory);
	ClassDB::bind_method(D_METHOD("get_StatisticsCategory"), &StatisticsValueData::GetStatisticsCategory);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "StatisticsCategory"), "set_StatisticsCategory", "get_StatisticsCategory");
	ClassDB::bind_method(D_METHOD("set_ModifierKey", "modifierKey"), &StatisticsValueData::SetModifierKey);
	ClassDB::bind_method(D_METHOD("get_ModifierKey"), &StatisticsValueData::GetModifierKey);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ModifierKey"), "set_ModifierKey", "get_ModifierKey");
	ClassDB::bind_method(D_METHOD("set_BaseValue", "baseValue"), &StatisticsValueData::SetBaseValue);
	ClassDB::bind_method(D_METHOD("get_BaseValue"), &StatisticsValueData::GetBaseValue);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BaseValue"), "set_BaseValue", "get_BaseValue");
	ClassDB::bind_method(D_METHOD("set_FinalValue", "finalValue"), &StatisticsValueData::SetFinalValue);
	ClassDB::bind_method(D_METHOD("get_FinalValue"), &StatisticsValueData::GetFinalValue);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "FinalValue"), "set_FinalValue", "get_FinalValue");
	ClassDB::bind_method(D_METHOD("set_Categories", "categories"), &StatisticsValueData::SetCategories);
	ClassDB::bind_method(D_METHOD("get_Categories"), &StatisticsValueData::GetCategories);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "Categories", PROPERTY_HINT_ARRAY_TYPE, vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "String")), "set_Categories", "get_Categories");
	ClassDB::bind_method(D_METHOD("set_FormatType", "formatType"), &StatisticsValueData::SetFormatType);
	ClassDB::bind_method(D_METHOD("get_FormatType"), &StatisticsValueData::GetFormatType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "FormatType", PROPERTY_HINT_ENUM, "FlatValue,Percentage,Speed,SpeedInv,Distance,Force,SpeedDist,Float,Angle,Time,Multiplier,Rotation"), "set_FormatType", "get_FormatType");
	ClassDB::bind_method(D_METHOD("set_SpecialInfoType", "specialInfoType"), &StatisticsValueData::SetSpecialInfoType);
	ClassDB::bind_method(D_METHOD("get_SpecialInfoType"), &StatisticsValueData::GetSpecialInfoType);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "SpecialInfoType", PROPERTY_HINT_ENUM, "None,Defense,Block,Attacks"), "set_SpecialInfoType", "get_SpecialInfoType");
}

Ref<StatisticsValueData> StatisticsValueData::createFromModifiedValue(Variant modifiedValue, const String &displayName, const String &statsCategory, StatisticsFormatTypes format, StatisticsSpecialInfoTypes specialInfo) {
	if(modifiedValue.is_null()) {
		print_error("StatisticsValueData::createFromModifiedValue called with a null value!");
		return {};
	}
	Object* modifiedValueAsObject = modifiedValue.get_validated_object();
	if(modifiedValueAsObject == nullptr) {
		print_error("StatisticsValueData::createFromModifiedValue needs either a ModifiedIntValue or a ModifiedFloatValue as its first parameter!");
		return {};
	}
	{
		ModifiedFloatValue* modifiedFloatValue = dynamic_cast<ModifiedFloatValue*>(modifiedValueAsObject);
		if(modifiedFloatValue != nullptr) {
			return createNew(
				displayName,
				statsCategory,
				modifiedFloatValue->ModifiedBy(),
				modifiedFloatValue->BaseValue(),
				modifiedFloatValue->Value(),
				modifiedFloatValue->getModifierCategories(),
				format,
				specialInfo);
		}
	}
	{
		ModifiedIntValue* modifiedIntValue = dynamic_cast<ModifiedIntValue*>(modifiedValueAsObject);
		if(modifiedIntValue != nullptr) {
			return createNew(
				displayName,
				statsCategory,
				modifiedIntValue->ModifiedBy(),
				modifiedIntValue->BaseValue(),
				modifiedIntValue->Value(),
				modifiedIntValue->getModifierCategories(),
				format,
				specialInfo);
		}
	}
	print_line("StatisticsValueData::createFromModifiedValue needs either a ModifiedIntValue or a ModifiedFloatValue as its first parameter, but got: ", modifiedValueAsObject->get_class_name());
	return {};
}

Ref<StatisticsValueData> StatisticsValueData::createNew(const String &displayName, const String &statsCategory, const String &modifierKey, float baseVal, float finalVal, const TypedArray<String> &categories, StatisticsFormatTypes format, StatisticsSpecialInfoTypes specialInfo) {
	Ref<StatisticsValueData> newStatsValueData;
	newStatsValueData.instantiate();
	newStatsValueData->DisplayName = displayName;
	newStatsValueData->StatisticsCategory = statsCategory;
	newStatsValueData->ModifierKey = modifierKey;
	newStatsValueData->BaseValue = baseVal;
	newStatsValueData->FinalValue = finalVal;
	newStatsValueData->Categories = categories;
	newStatsValueData->FormatType = format;
	newStatsValueData->SpecialInfoType = specialInfo;
	return newStatsValueData;
}
