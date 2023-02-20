#ifndef HOTEXTENSION_LOCATORSYSTEM_H
#define HOTEXTENSION_LOCATORSYSTEM_H

#include "Locator.h"

#include <core/object/ref_counted.h>
#include <core/math/random_number_generator.h>

#include <vector>


class LocatorSystem : public RefCounted
{
    GDCLASS(LocatorSystem, RefCounted)

private:
    std::vector<Locator*> _tempLocators;
    RandomNumberGenerator _random;
protected:
    // Required entry point that the API calls to bind our class to Godot.
    static void _bind_methods();

    void Update();

public:
    LocatorSystem();

    Array GetLocatorsInCircle(String poolName, Vector2 center, float radius);
    Array GetGameObjectsInCircle(String poolName, Vector2 center, float radius);
    int CountLocatorsInCircle(String poolName, Vector2 center, float radius);

    Array GetLocatorsOutsideCircle(String poolName, Vector2 center, float radius);
    Array GetGameObjectsOutsideCircle(String poolName, Vector2 center, float radius);
    int CountLocatorsOutsideCircle(String poolName, Vector2 center, float radius);

    Array GetLocatorsInCircleMotion(String poolName, Vector2 center, float radius, Vector2 motion);
    Array GetGameObjectsInCircleMotion(String poolName, Vector2 center, float radius, Vector2 motion);
    int CountLocatorsInCircleMotion(String poolName, Vector2 center, float radius, Vector2 motion);

    Array GetLocatorsInRectangle(String poolName, float minX, float maxX, float minY, float maxY);
    Array GetGameObjectsInRectangle(String poolName, float minX, float maxX, float minY, float maxY);
    int CountLocatorsInRectangle(String poolName, float minX, float maxX, float minY, float maxY);

    Array GetLocatorsOutsideRectangle(String poolName, float minX, float maxX, float minY, float maxY);
    Array GetGameObjectsOutsideRectangle(String poolName, float minX, float maxX, float minY, float maxY);
    int CountLocatorsOutsideRectangle(String poolName, float minX, float maxX, float minY, float maxY);

    Locator* GetRandomLocatorInPool(String poolName);

    static void LocatorEnteredTree(Locator* locator);
    static void LocatorExitedTree(Locator* locator);
};


#endif //HOTEXTENSION_LOCATORSYSTEM_H
