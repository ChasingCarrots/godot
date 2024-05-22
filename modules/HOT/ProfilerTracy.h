#ifndef PROFILERTRACY_H
#define PROFILERTRACY_H
#include <core/object/class_db.h>
#include <core/object/object.h>
#include <core/os/thread.h>
#include <core/os/time.h>

class ProfilerTracy : public Object {
	GDCLASS(ProfilerTracy, Object);

	static Thread profilingThread;

protected:
	static void _bind_methods();

public:
	static void startProfiling();
};

#endif //PROFILERTRACY_H
