#include "ModifiedValues.h"


void ModifiedIntValue::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init", "baseVal", "modifierName", "gameObject", "rankModifier"), &ModifiedIntValue::_init);
	ClassDB::bind_method(D_METHOD("setModifierCategories", "categories"), &ModifiedIntValue::setModifierCategories);
	ClassDB::bind_method(D_METHOD("updateManually"), &ModifiedIntValue::updateModifier);
	ClassDB::bind_method(D_METHOD("updateModifier", "mod"), &ModifiedIntValue::updateModifier);
	ClassDB::bind_method(D_METHOD("Value"), &ModifiedIntValue::Value);
	ClassDB::bind_method(D_METHOD("BaseValue"), &ModifiedIntValue::BaseValue);
	ClassDB::bind_method(D_METHOD("ModifiedBy"), &ModifiedIntValue::ModifiedBy);
	ADD_SIGNAL(MethodInfo("ValueUpdated", PropertyInfo(Variant::INT, "oldValue"), PropertyInfo(Variant::INT, "newValue")));
}

void ModifiedIntValue::_init(int baseVal, String modifierName, GameObject *gameObject, Callable rankModifier) {
	_baseValue = baseVal;
	_currentModifiedValue = baseVal;
	_modifiedBy = modifierName;
	_gameObject = gameObject;
	_gameObject->connect("ModifierUpdated", callable_mp(this, &ModifiedIntValue::updateModifier));
	_rankModifier = rankModifier;
	updateModifier(modifierName);
}

void ModifiedIntValue::setModifierCategories(TypedArray<String> categories) {
	_modifierCategories = categories;
	updateManually();
}

void ModifiedIntValue::updateManually() {
	updateModifier(_modifiedBy);
}

void ModifiedIntValue::updateModifier(String mod) {
	if(mod == _modifiedBy || mod == "ALL") {
		int oldValue = _currentModifiedValue;
		_currentModifiedValue = Math::ceil((float)_gameObject->calculateModifiedValue(_modifiedBy, _baseValue, _modifierCategories) - 0.001f);
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
	ClassDB::bind_method(D_METHOD("setModifierCategories", "categories"), &ModifiedFloatValue::setModifierCategories);
	ClassDB::bind_method(D_METHOD("updateManually"), &ModifiedFloatValue::updateModifier);
	ClassDB::bind_method(D_METHOD("updateModifier", "mod"), &ModifiedFloatValue::updateModifier);
	ClassDB::bind_method(D_METHOD("Value"), &ModifiedFloatValue::Value);
	ClassDB::bind_method(D_METHOD("BaseValue"), &ModifiedFloatValue::BaseValue);
	ClassDB::bind_method(D_METHOD("ModifiedBy"), &ModifiedFloatValue::ModifiedBy);
	ADD_SIGNAL(MethodInfo("ValueUpdated", PropertyInfo(Variant::FLOAT, "oldValue"), PropertyInfo(Variant::FLOAT, "newValue")));
}

void ModifiedFloatValue::_init(float baseVal, String modifierName, GameObject *gameObject, Callable rankModifier) {
	_baseValue = baseVal;
	_currentModifiedValue = baseVal;
	_modifiedBy = modifierName;
	_gameObject = gameObject;
	_gameObject->connect("ModifierUpdated", callable_mp(this, &ModifiedFloatValue::updateModifier));
	_rankModifier = rankModifier;
	updateModifier(modifierName);
}

void ModifiedFloatValue::setModifierCategories(TypedArray<String> categories) {
	_modifierCategories = categories;
	updateManually();
}

void ModifiedFloatValue::updateManually() {
	updateModifier(_modifiedBy);
}

void ModifiedFloatValue::updateModifier(String mod) {
	if(mod == _modifiedBy || mod == "ALL") {
		int oldValue = _currentModifiedValue;
		_currentModifiedValue = _gameObject->calculateModifiedValue(_modifiedBy, _baseValue, _modifierCategories);
		emit_signal("ValueUpdated", oldValue, _currentModifiedValue);
	}
}

float ModifiedFloatValue::Value() {
	if(_rankModifier.is_null())
		return _currentModifiedValue;
	return _currentModifiedValue * (float)_rankModifier.callv({});
}
