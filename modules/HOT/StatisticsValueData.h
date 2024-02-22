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
	enum StatisticsDisplayFlags {
		NoFlags = 0,
		NoBaseValue = 1 << 0,
		NoMultiplierValue = 1 << 1,
		NegativeValueIsPositiveEffect = 1 << 2
	};

	String DisplayName;
	String StatisticsCategory;
	TypedArray<String> ModifierKeys;
	float BaseValue = 0;
	float FinalValue = 0;
	TypedArray<String> Categories;
	StatisticsFormatTypes FormatType;
	StatisticsSpecialInfoTypes SpecialInfoType;
	StatisticsDisplayFlags DisplayFlags = NoFlags;

	[[nodiscard]] String GetDisplayName() const { return DisplayName; }
	void SetDisplayName(const String &displayName) { DisplayName = displayName; }
	[[nodiscard]] String GetStatisticsCategory() const { return StatisticsCategory; }
	void SetStatisticsCategory(const String &statisticsCategory) { StatisticsCategory = statisticsCategory; }
	[[nodiscard]] TypedArray<String> GetModifierKeys() const { return ModifierKeys; }
	void SetModifierKeys(const TypedArray<String> &modifierKeys) { ModifierKeys = modifierKeys; }
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
	[[nodiscard]] StatisticsDisplayFlags GetDisplayFlags() const { return DisplayFlags; }
	void SetDisplayFlags(StatisticsDisplayFlags display_flags) { DisplayFlags = display_flags; }

	static Ref<StatisticsValueData> createFromModifiedValue(Variant modifiedValue, const String& displayName, const String& statsCategory, StatisticsFormatTypes format, StatisticsSpecialInfoTypes specialInfo, StatisticsDisplayFlags display_flags = NoFlags);
	static Ref<StatisticsValueData> createNew(const String& displayName, const String& statsCategory, Variant modifierKeys, float baseVal, float finalVal, const TypedArray<String>& categories, StatisticsFormatTypes format, StatisticsSpecialInfoTypes specialInfo, StatisticsDisplayFlags display_flags = NoFlags);
};

VARIANT_ENUM_CAST(StatisticsValueData::StatisticsFormatTypes);
VARIANT_ENUM_CAST(StatisticsValueData::StatisticsSpecialInfoTypes);
VARIANT_ENUM_CAST(StatisticsValueData::StatisticsTypes);
VARIANT_ENUM_CAST(StatisticsValueData::StatisticsDisplayFlags);

#endif //STATISTICSVALUEDATA_H
