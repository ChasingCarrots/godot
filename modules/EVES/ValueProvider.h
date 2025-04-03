#ifndef VALUEPROVIDER_H
#define VALUEPROVIDER_H

#include "ValueCollection.h"

namespace eves {


class ValueProvider {

private:
	LocalVector<ValueProvider*> _inheritsFromValueProvider;

public:
	enum ApplyValueBehaviour {
		Add,
		Replace
	};

	void ApplyToValueCollection(ValueCollection *collection);
	void RemoveFromValueCollection(ValueCollection *collection);

};

} //namespace eves

#endif //VALUEPROVIDER_H
