#ifndef STATISTICSVALUEDATA_H
#define STATISTICSVALUEDATA_H

#include <core/object/ref_counted.h>
#include <core/variant/typed_array.h>

class StatisticsValueData : public RefCounted {
	GDCLASS(StatisticsValueData, RefCounted)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	enum StatisticsTypes {
		PlayerBaseStats,
		AbilityStats
	};
	enum StatisticsFormatTypes {
		FlatValue,
		Percentage,
		Speed,
		SpeedInv,
		Distance,
		Force,
		SpeedDist,
		Float,
		Angle,
		Time,
		Multiplier,
		Rotation
	};
	enum StatisticsSpecialInfoTypes {
		None,
		Defense,
		Block,
		Attacks
	};

	String DisplayName;
	String StatisticsCategory;
	String ModifierKey;
	float BaseValue;
	float FinalValue;
	TypedArray<String> Categories;
	StatisticsFormatTypes FormatType;
	StatisticsSpecialInfoTypes SpecialInfoType;

	[[nodiscard]] String GetDisplayName() const { return DisplayName; }
	void SetDisplayName(const String &displayName) { DisplayName = displayName; }
	[[nodiscard]] String GetStatisticsCategory() const { return StatisticsCategory; }
	void SetStatisticsCategory(const String &statisticsCategory) { StatisticsCategory = statisticsCategory; }
	[[nodiscard]] String GetModifierKey() const { return ModifierKey; }
	void SetModifierKey(const String &modifierKey) { ModifierKey = modifierKey; }
	[[nodiscard]] float GetBaseValue() const { return BaseValue; }
	void SetBaseValue(float baseValue) { BaseValue = baseValue; }
	[[nodiscard]] float GetFinalValue() const { return FinalValue; }
	void SetFinalValue(float finalValue) { FinalValue = finalValue; }
	[[nodiscard]] TypedArray<String> GetCategories() const { return Categories; }
	void SetCategories(const TypedArray<String> &categories) { Categories = categories; }
	[[nodiscard]] StatisticsFormatTypes GetFormatType() const { return FormatType; }
	void SetFormatType(StatisticsFormatTypes formatType) { FormatType = formatType; }
	[[nodiscard]] StatisticsSpecialInfoTypes GetSpecialInfoType() const { return SpecialInfoType; }
	void SetSpecialInfoType(StatisticsSpecialInfoTypes specialInfoType) { SpecialInfoType = specialInfoType; }

	static Ref<StatisticsValueData> createFromModifiedValue(Variant modifiedValue, const String& displayName, const String& statsCategory, StatisticsFormatTypes format, StatisticsSpecialInfoTypes specialInfo);
	static Ref<StatisticsValueData> createNew(const String& displayName, const String& statsCategory, const String& modifierKey, float baseVal, float finalVal, const TypedArray<String>& categories, StatisticsFormatTypes format, StatisticsSpecialInfoTypes specialInfo);
};

VARIANT_ENUM_CAST(StatisticsValueData::StatisticsFormatTypes);
VARIANT_ENUM_CAST(StatisticsValueData::StatisticsSpecialInfoTypes);
VARIANT_ENUM_CAST(StatisticsValueData::StatisticsTypes);

#endif //STATISTICSVALUEDATA_H
