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

void Modifier::_init(const StringName &modifiedType, const Variant &gameObject) {
	_hash = 0;
	_modifiedType = modifiedType;
	if (gameObject.get_type() != Variant::OBJECT)
		return;
	Object* gameObjectAsPtr = gameObject.get_validated_object();
	if (gameObjectAsPtr == nullptr)
		return;
	if (!gameObjectAsPtr->is_class("GameObject"))
		return;
	_gameObject = gameObject;
	static_cast<GameObject*>(gameObjectAsPtr)->registerModifier(this);
}

Modifier::~Modifier() {
	if (_gameObject.get_type() != Variant::OBJECT)
		return;
	Object* gameObjectAsPtr = _gameObject.get_validated_object();
	if (gameObjectAsPtr == nullptr)
		return;
	if (!gameObjectAsPtr->is_class("GameObject"))
		return;
	static_cast<GameObject*>(gameObjectAsPtr)->unregisterModifier(this);
}

void Modifier::allowCategories(TypedArray<StringName> categories) {
	_modifierCategories.resize(categories.size());
	for (int i = 0; i < categories.size(); ++i) {
		_modifierCategories.set(i, categories[i]);
	}
	_hash = 0;
}

