#ifndef EVES_VALUECOLLECTION_H
#define EVES_VALUECOLLECTION_H

#include "BaseValue.h"

#include <signals.hpp>
#include <unordered_map>
#include <core/string/ustring.h>

namespace eves {

class ValueCollection {
public:
	void AddNamedValue(const String &name, BaseValue *value) {
		_namedValues.emplace(std::make_pair(
				name.hash64(),
				ValueData(
						value,
						value->OnValueChanged.connect([this, name](BaseValue *valueParam) {
							if (const auto signalIter = _valueChangedSignals.find(name.hash64()); signalIter != _valueChangedSignals.end()) {
								signalIter->second(valueParam);
							}
						}),
						value->OnValueDeleted.connect([this, name](BaseValue *) {
							// automatically remove values, when they get deleted!
							RemoveNamedValue(name);
						}))));
		// directly trigger the value changed signal, just in case anybody already listens!
		if (const auto signalIter = _valueChangedSignals.find(name.hash64()); signalIter != _valueChangedSignals.end()) {
			signalIter->second(value);
		}
	}

	void RemoveNamedValue(const String &name) {
		if (const auto valueIter = _namedValues.find(name.hash64()); valueIter != _namedValues.end()) {
			_namedValues.erase(valueIter);
		}
	}

	void ClearAllValues() {
		_namedValues.clear();
	}

	template<typename F>
	auto ConnectToValueChangedSignal(const String& valueName, F&& functor) {
		return _valueChangedSignals[valueName.hash64()].connect(functor);
	}

	bool GetValue(const String &name, float& value) {
		if (const auto namedValueIter = _namedValues.find(name.hash64()); namedValueIter != _namedValues.end()) {
			value = namedValueIter->second.ValuePointer->GetValue();
			return true;
		}
		return false;
	}

	BaseValue* GetBaseValue(const String& name) {
		if (const auto namedValueIter = _namedValues.find(name.hash64()); namedValueIter != _namedValues.end()) {
			return namedValueIter->second.ValuePointer;
		}
		return nullptr;
	}

	bool HasValue(const String& name) const {
		return _namedValues.find(name.hash64()) != _namedValues.end();
	}

private:
	struct ValueData {
		ValueData(BaseValue *valuePointer, const fteng::connection_raw &valueUpdatedConnection, const fteng::connection_raw &valueRemovedConnection) :
				ValuePointer(valuePointer),
				ValueUpdatedConnection(valueUpdatedConnection),
				ValueRemovedConnection(valueRemovedConnection) {}
		BaseValue *ValuePointer;
		// the connections are saved here, so that they get severed,
		// as soon as the ValueData is removed!
		fteng::connection ValueUpdatedConnection;
		fteng::connection ValueRemovedConnection;
	};
	// can't use Godot's OAHashMap here, because the signals lib (fteng::...) has deleted
	// their copy constructors and assignment operators on almost all of their classes and
	// Godot's containers use those all over the place.
	std::unordered_map<uint64_t, ValueData> _namedValues;
	std::unordered_map<uint64_t, fteng::signal<void(BaseValue* value)>> _valueChangedSignals;
};

} //namespace eves

#endif //EVES_VALUECOLLECTION_H
