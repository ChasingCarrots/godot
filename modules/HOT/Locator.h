#ifndef HOTEXTENSION_LOCATOR_H
#define HOTEXTENSION_LOCATOR_H

#include <scene/2d/node_2d.h>

class Locator : public Node2D {
    GDCLASS(Locator, Node2D)

protected:
    // Required entry point that the API calls to bind our class to Godot.
    static void _bind_methods();
	void _notification(int p_notification);
    static inline bool is_tool() { return true; }

    float _radius = 10;
    String _locatorPoolName;
    Vector2i _currentCell;

public:
    void _enter_tree();

    void _exit_tree();

    void _draw();

    inline void SetRadius(float radius) {
        _radius = radius;
        queue_redraw();
    }
    inline float GetRadius() { return _radius; };

    inline void SetLocatorPoolName(String locatorPoolName){
        _locatorPoolName = locatorPoolName;
    }
    inline String GetLocatorPoolName(){
        return _locatorPoolName;
    }

    inline void SetCurrentCell(Vector2i currentCell) {
        _currentCell = currentCell;
    }
    inline Vector2i GetCurrentCell() {
        return _currentCell;
    }

};


#endif //HOTEXTENSION_LOCATOR_H
