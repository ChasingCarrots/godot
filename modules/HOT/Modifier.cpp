#include "Modifier.h"

#include "GameObject.h"

void Modifier::_bind_methods() {
	ClassDB::bind_static_method("Modifier", D_METHOD("create", "modifierName", "gameObject"), &Modifier::create);
	ClassDB::bind_method(D_METHOD("init", "modifiedType", "gameObject"), &Modifier::_init);
	ClassDB::bind_method(D_METHOD("setName", "name"), &Modifier::setModifierName);
	ClassDB::bind_method(D_METHOD("getName"), &Modifier::getModifierName);
	ClassDB::bind_method(D_METHOD("allowCategories", "categories"), &Modifier::allowCategories);
	ClassDB::bind_method(D_METHOD("setAdditiveMod", "additiveMod"), &Modifier::setAdditiveMod);
	ClassDB::bind_method(D_METHOD("setMultiplierMod", "multiplierMod"), &Modifier::setMultiplierMod);
	ClassDB::bind_method(D_METHOD("getAdditiveMod"), &Modifier::getAdditiveModifier);
	ClassDB::bind_method(D_METHOD("getMultiplierMod"), &Modifier::getMultiplierModifier);
}

void Modifier::_init(String modifierName, GameObject *gameObject) {
	_modifiedType = modifierName;
	_gameObject = gameObject;
	_gameObject->registerModifier(this);
}

Modifier::~Modifier() {
	if(_gameObject != nullptr) {
		_gameObject->unregisterModifier(this);
		_gameObject = nullptr;
	}
}

void Modifier::allowCategories(TypedArray<String> categories) {
	_modifierCategories = categories;
}

