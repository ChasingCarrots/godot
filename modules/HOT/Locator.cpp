#include "Locator.h"
#include "LocatorSystem.h"

void Locator::_notification(int p_notification) {
	switch(p_notification) {
		case NOTIFICATION_ENTER_TREE:
			_enter_tree();
			break;
		case NOTIFICATION_EXIT_TREE:
			_exit_tree();
			break;
		case NOTIFICATION_DRAW:
			_draw();
			break;
	}
}

void Locator::_enter_tree() {
	if(!_currentlyActive) {
		LocatorSystem::LocatorEnteredTree(this);
		_currentlyActive = true;
	}
}

void Locator::_exit_tree() {
	if(_currentlyActive) {
		LocatorSystem::LocatorExitedTree(this);
		_currentlyActive = false;
	}
}

void Locator::_draw() {
    if(!Engine::get_singleton()->is_editor_hint())
        return;
    draw_circle(Vector2(0,0), _radius, Color(0.8f, 0.6f, 1, 0.5f));
}

void Locator::_bind_methods() {
    ClassDB::bind_static_method("Locator", D_METHOD("is_tool"), &Locator::is_tool);

	ClassDB::bind_method(D_METHOD("SetLocatorActive", "active"), &Locator::SetLocatorActive);

    ClassDB::bind_method(D_METHOD("SetRadius", "radius"), &Locator::SetRadius);
    ClassDB::bind_method(D_METHOD("GetRadius"), &Locator::GetRadius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Radius"), "SetRadius", "GetRadius");

    ClassDB::bind_method(D_METHOD("SetLocatorPoolName", "locatorPoolName"), &Locator::SetLocatorPoolName);
    ClassDB::bind_method(D_METHOD("GetLocatorPoolName"), &Locator::GetLocatorPoolName);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "LocatorPoolName"), "SetLocatorPoolName", "GetLocatorPoolName");

    ClassDB::bind_method(D_METHOD("SetCurrentCell", "currentCell"), &Locator::SetCurrentCell);
    ClassDB::bind_method(D_METHOD("GetCurrentCell"), &Locator::GetCurrentCell);
    ADD_PROPERTY(PropertyInfo(
            Variant::VECTOR2I,
            "_currentCell",
            PROPERTY_HINT_NONE,
            "",
            PROPERTY_USAGE_NO_EDITOR), "SetCurrentCell", "GetCurrentCell");
}

void Locator::SetLocatorActive(bool active) {
	if(active == _currentlyActive)
		return;

	if(active)
		LocatorSystem::LocatorEnteredTree(this);
	else
		LocatorSystem::LocatorExitedTree(this);
	_currentlyActive = active;
}