#ifndef EVES_SUMVALUE_H
#define EVES_SUMVALUE_H

#include "BaseValue.h"

namespace eves {

class SumValue : public BaseValue {
public:
	void AddValueToSum(BaseValue *value) {
		_sumValues.emplace_back(
				value,
				value->OnValueDeleted.connect([this](eves::BaseValue *value) {
					RemoveValueFromSum(value);
				}),
				value->OnValueChanged.connect([this](eves::BaseValue *) {
					SetValueDirty();
				}));
		SetValueDirty();
	}

	void RemoveValueFromSum(BaseValue *value) {
		auto valIter = _sumValues.begin();
		while (valIter != _sumValues.end()) {
			if (valIter->ValuePointer == value) {
				_sumValues.erase(valIter);
				SetValueDirty();
				return;
			}
			++valIter;
		}
	}

protected:
	struct ValueData {
		ValueData(BaseValue *valuePointer, const fteng::connection_raw& changeSignalConnection, const fteng::connection_raw& removeSignalConnection) :
				ValuePointer(valuePointer),
				ValueUpdatedConnection(changeSignalConnection),
				ValueRemovedConnection(removeSignalConnection) {}
		BaseValue *ValuePointer;
		// the connections are saved here, so that they get disconnected,
		// as soon as that ValueData is removed!
		fteng::connection ValueUpdatedConnection;
		fteng::connection ValueRemovedConnection;
	};
	std::vector<ValueData> _sumValues;

	void RecalculateCachedValue() override {
		float sum = 0;
		for (auto &value : _sumValues) {
			sum += value.ValuePointer->GetValue();
		}
		CacheValue(sum);
	}
};

} //namespace eves

#endif //EVES_SUMVALUE_H
