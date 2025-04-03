#ifndef EVES_FLATVALUE_H
#define EVES_FLATVALUE_H

#include "BaseValue.h"
#include <core/object/class_db.h>
#include <core/object/object.h>

namespace eves {

class FlatValue : public BaseValue {
public:
	void SetValue(float newValue) {
		if (newValue == _cachedValue)
			return;
		CacheValue(newValue);
		if(_overrides.empty())
			OnValueChanged(this);
	}
};

} //namespace eves

#endif //EVES_FLATVALUE_H
