#ifndef ENTITY_H
#define ENTITY_H

#include "ValueCollection.h"

namespace eves {

typedef uint64_t EntityID;
constexpr EntityID InvalidEntityID = 0;

class Entity {
public:
	explicit Entity(EntityID id) : _entityID(id) {}
	EntityID GetID() const { return _entityID; }
	ValueCollection* GetValueCollection() { return &_floatValues; }
private:
	EntityID _entityID = InvalidEntityID;
	ValueCollection _floatValues;
};

} //namespace eves

#endif //ENTITY_H
