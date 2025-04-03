#ifndef EVES_BASEVALUE_H
#define EVES_BASEVALUE_H

#include <core/templates/local_vector.h>
#include <algorithm>
#include <signals.hpp>

namespace eves {

enum class ValueTypes {
	FlatValue,
	SumValue,
	ExpressionValue
};

class BaseValue {
public:
	BaseValue(BaseValue const &) = delete;
	BaseValue() = default;

	virtual ~BaseValue() {
		OnValueDeleted(this);
	}

	fteng::signal<void(BaseValue *Value)> OnValueChanged;
	fteng::signal<void(BaseValue *Value)> OnValueDeleted;

	float GetValue() {
		if(!_overrides.empty())
			return _overrides.back().OverriddenBy->GetValue();

		if (_isDirty)
			RecalculateCachedValue();
		return _cachedValue;
	}

	void AddOverride(BaseValue* baseValue) {
		// we have to check for circular overrides!
		if(baseValue->HasOverride(this) || HasOverride(baseValue)) {
			ERR_FAIL_MSG("Can't add BaseValue override: would be a circular override.");
			return;
		}
		_overrides.emplace_back(
			baseValue,
			baseValue->OnValueDeleted.connect([this](BaseValue * baseValue) { RemoveOverride(baseValue); })
		);
		_overrideUpdatedConnection = baseValue->OnValueChanged.connect([this](BaseValue* baseValue) { OnValueChanged(this); });
		OnValueChanged(this);
	}

	void RemoveOverride(BaseValue *baseValue) {
		auto overrideIter = _overrides.begin();
		while(overrideIter != _overrides.end()) {
			if(overrideIter->OverriddenBy == baseValue) {
				bool wasCurrentlyActiveOverride = overrideIter == _overrides.end() - 1;
				_overrides.erase(overrideIter);
				if(wasCurrentlyActiveOverride) {
					_overrideUpdatedConnection.disconnect();
					if(!_overrides.empty()) {
						BaseValue* nextActiveOverride = _overrides.back().OverriddenBy;
						_overrideUpdatedConnection = nextActiveOverride->
							OnValueChanged.connect([this](BaseValue* baseValue) { OnValueChanged(this); });
					}
					OnValueChanged(this);
				}
				return;
			}
			++overrideIter;
		}
	}

	bool HasOverride(BaseValue* baseValue) const {
		if(baseValue == this)
			return true;
		for(const auto& override : _overrides) {
			if(override.OverriddenBy == baseValue || override.OverriddenBy->HasOverride(baseValue))
				return true;
		}
		return false;
	}

protected:
	bool _isDirty = true;
	float _cachedValue = 0;

	struct OverrideData {
		BaseValue* OverriddenBy;
		fteng::connection OverriddenRemovedConnection;
		OverrideData(BaseValue *overriddenBy, const fteng::connection_raw &overriddenRemovedConnection) :
				OverriddenBy(overriddenBy),
				OverriddenRemovedConnection(overriddenRemovedConnection) {}
	};
	std::vector<OverrideData> _overrides;
	fteng::connection _overrideUpdatedConnection;

	// can't really be abstract, because of the way the godot objectdb works...
	virtual void RecalculateCachedValue() {}

	void SetValueDirty() {
		_isDirty = true;
		if(_overrides.empty())
			OnValueChanged(this);
	}

	void CacheValue(float cachedValue) {
		_isDirty = false;
		_cachedValue = cachedValue;
	}
};

} //namespace eves

#endif //EVES_BASEVALUE_H