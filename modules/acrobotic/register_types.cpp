/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "AcroboticSimulationBackend.h"

void register_acrobotic_types() {
	ClassDB::register_class<AcroboticSimulationBackend>();
}

void unregister_acrobotic_types() {

}