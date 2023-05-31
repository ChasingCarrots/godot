#include "ModifiedValues.h"

#include "core/profiling.h"

void ModifiedIntValue::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init", "baseVal", "modifierName", "gameObject", "rankModifier"), &ModifiedIntValue::_init);
	ClassDB::bind_method(D_METHOD("initAsAdditiveOnly", "modifierName", "gameObject", "rankModifier"), &ModifiedIntValue::_initAsAdditiveOnly);
	ClassDB::bind_method(D_METHOD("setModifierCategories", "categories"), &ModifiedIntValue::setModifierCategories);
	ClassDB::bind_method(D_METHOD("getModifierCategories"), &ModifiedIntValue::getModifierCategories);
	ClassDB::bind_method(D_METHOD("updateManually"), &ModifiedIntValue::updateManually);
	ClassDB::bind_method(D_METHOD("updateModifier", "mod"), &ModifiedIntValue::updateModifier);
	ClassDB::bind_method(D_METHOD("Value"), &ModifiedIntValue::Value);
	ClassDB::bind_method(D_METHOD("BaseValue"), &ModifiedIntValue::BaseValue);
	ClassDB::bind_method(D_METHOD("ModifiedBy"), &ModifiedIntValue::ModifiedBy);
	ADD_SIGNAL(MethodInfo("ValueUpdated", PropertyInfo(Variant::INT, "oldValue"), PropertyInfo(Variant::INT, "newValue")));
}

void ModifiedIntValue::_init(int baseVal, String modifierName, Variant gameObject, Callable rankModifier) {
	PROFILE_FUNCTION()
	_gameObjectChecker = gameObject;
	if (!IsGameObjectValid()) {
		print_error(String("Error: ModifiedIntValue init received an invalid gameObject! ModifierName: ")+modifierName);
		_gameObject = nullptr;
		return;
	}
	_gameObject = static_cast<GameObject*>(gameObject.get_validated_object());
	_baseValue = baseVal;
	_currentModifiedValue = baseVal;
	_modifiedBy = modifierName;
	// should be possible to call init multiple times, to change values...
	if(!_gameObject->is_connected("ModifierUpdated", callable_mp(this, &ModifiedIntValue::updateModifier)))
		_gameObject->connect("ModifierUpdated", callable_mp(this, &ModifiedIntValue::updateModifier));
	_rankModifier = rankModifier;
	updateModifier(modifierName);
}

void ModifiedIntValue::_initAsAdditiveOnly(String modifierName, Variant gameObject, Callable rankModifier) {
	_type = ModifiedValueType::AdditiveOnly;
	_init(0, modifierName, gameObject, rankModifier);
}

void ModifiedIntValue::setModifierCategories(TypedArray<String> categories) {
	_modifierCategories = categories;
	updateManually();
}

void ModifiedIntValue::updateManually() {
	updateModifier(_modifiedBy);
}

void ModifiedIntValue::updateModifier(String mod) {
	if (_gameObject == nullptr || !IsGameObjectValid()) {
		_gameObject = nullptr;
		return;
	}

	if(mod == _modifiedBy || mod == "ALL") {
		PROFILE_FUNCTION()
		int oldValue = _currentModifiedValue;
		switch (_type) {
			case ModifiedValueType::NormalCalculation:
				_currentModifiedValue = Math::ceil((float)_gameObject->calculateModifiedValue(_modifiedBy, _baseValue, _modifierCategories) - 0.001f);
				break;
			case ModifiedValueType::AdditiveOnly:
				_currentModifiedValue = Math::ceil((float)_gameObject->getAdditiveModifier(_modifiedBy, _modifierCategories) - 0.001f);
				break;
			case ModifiedValueType::MultiplicativeOnly:
				_currentModifiedValue = _gameObject->getMultiplicativeModifier(_modifiedBy, _modifierCategories);
				break;
		}
		if(oldValue != _currentModifiedValue)
			emit_signal("ValueUpdated", oldValue, _currentModifiedValue);
	}
}

int ModifiedIntValue::Value() {
	if(_rankModifier.is_null())
		return _currentModifiedValue;
	return Math::round((float)_currentModifiedValue * (float)_rankModifier.callv({}));
}



void ModifiedFloatValue::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init", "baseVal", "modifierName", "gameObject", "rankModifier"), &ModifiedFloatValue::_init);
	ClassDB::bind_method(D_METHOD("initAsAdditiveOnly", "modifierName", "gameObject", "rankModifier"), &ModifiedFloatValue::_initAsAdditiveOnly);
	ClassDB::bind_method(D_METHOD("initAsMultiplicativeOnly", "modifierName", "gameObject", "rankModifier"), &ModifiedFloatValue::_initAsMultiplicativeOnly);
	ClassDB::bind_method(D_METHOD("setModifierCategories", "categories"), &ModifiedFloatValue::setModifierCategories);
	ClassDB::bind_method(D_METHOD("getModifierCategories"), &ModifiedFloatValue::getModifierCategories);
	ClassDB::bind_method(D_METHOD("updateManually"), &ModifiedFloatValue::updateManually);
	ClassDB::bind_method(D_METHOD("updateModifier", "mod"), &ModifiedFloatValue::updateModifier);
	ClassDB::bind_method(D_METHOD("Value"), &ModifiedFloatValue::Value);
	ClassDB::bind_method(D_METHOD("BaseValue"), &ModifiedFloatValue::BaseValue);
	ClassDB::bind_method(D_METHOD("ModifiedBy"), &ModifiedFloatValue::ModifiedBy);
	ADD_SIGNAL(MethodInfo("ValueUpdated", PropertyInfo(Variant::FLOAT, "oldValue"), PropertyInfo(Variant::FLOAT, "newValue")));
}

void ModifiedFloatValue::_init(float baseVal, String modifierName, Variant gameObject, Callable rankModifier) {
	PROFILE_FUNCTION()
	_gameObjectChecker = gameObject;
	if (!IsGameObjectValid()) {
		print_error(String("Error: ModifiedFloatValue init received an invalid gameObject! ModifierName: ")+modifierName);
		_gameObject = nullptr;
		return;
	}
	_gameObject = static_cast<GameObject*>(gameObject.get_validated_object());
	_baseValue = baseVal;
	_currentModifiedValue = baseVal;
	_modifiedBy = modifierName;
	// should be possible to call init multiple times, to change values...
	if(!_gameObject->is_connected("ModifierUpdated", callable_mp(this, &ModifiedFloatValue::updateModifier)))
		_gameObject->connect("ModifierUpdated", callable_mp(this, &ModifiedFloatValue::updateModifier));
	_rankModifier = rankModifier;
	updateModifier(modifierName);
}

void ModifiedFloatValue::_initAsMultiplicativeOnly(String modifierName, Variant gameObject, Callable rankModifier) {
	_type = ModifiedValueType::MultiplicativeOnly;
	_init(0, modifierName, gameObject, rankModifier);
}

void ModifiedFloatValue::_initAsAdditiveOnly(String modifierName, Variant gameObject, Callable rankModifier) {
	_type = ModifiedValueType::AdditiveOnly;
	_init(0, modifierName, gameObject, rankModifier);
}

void ModifiedFloatValue::setModifierCategories(TypedArray<String> categories) {
	_modifierCategories = categories;
	updateManually();
}

void ModifiedFloatValue::updateManually() {
	updateModifier(_modifiedBy);
}

void ModifiedFloatValue::updateModifier(String mod) {
	if (_gameObject == nullptr || !IsGameObjectValid()) {
		_gameObject = nullptr;
		return;
	}

	if(mod == _modifiedBy || mod == "ALL") {
		PROFILE_FUNCTION()
		float oldValue = _currentModifiedValue;
		switch (_type) {
			case ModifiedValueType::NormalCalculation:
				_currentModifiedValue = _gameObject->calculateModifiedValue(_modifiedBy, _baseValue, _modifierCategories);
				break;
			case ModifiedValueType::AdditiveOnly:
				_currentModifiedValue = _gameObject->getAdditiveModifier(_modifiedBy, _modifierCategories);
				break;
			case ModifiedValueType::MultiplicativeOnly:
				_currentModifiedValue = _gameObject->getMultiplicativeModifier(_modifiedBy, _modifierCategories);
				break;
		}
		if(oldValue != _currentModifiedValue)
			emit_signal("ValueUpdated", oldValue, _currentModifiedValue);
	}
}

float ModifiedFloatValue::Value() {
	if(_rankModifier.is_null())
		return _currentModifiedValue;
	return _currentModifiedValue * (float)_rankModifier.callv({});
}
