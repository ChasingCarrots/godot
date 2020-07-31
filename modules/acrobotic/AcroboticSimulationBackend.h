#ifndef GODOT_ACROBOTICSIMULATIONBACKEND_H
#define GODOT_ACROBOTICSIMULATIONBACKEND_H

#include <core/reference.h>
#include <Simulation/Simulation.h>

class AcroboticSimulationBackend : public Reference {
	GDCLASS(AcroboticSimulationBackend, Reference);

protected:
	static void _bind_methods();

public:
	void test();

private:
	AcroboticBackend::Simulation simulation;
};

#endif //GODOT_ACROBOTICSIMULATIONBACKEND_H
