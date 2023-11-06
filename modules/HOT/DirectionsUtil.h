#ifndef GODOT_CLONE_DIRECTIONSUTIL_H
#define GODOT_CLONE_DIRECTIONSUTIL_H

#include <scene/main/node.h>

class DirectionsUtil : public Object {
	GDCLASS(DirectionsUtil, Object)

protected:
	// Required entry point that the API calls to bind our class to Godot.
	static void _bind_methods();

public:
	enum Dir {
		S = 0,
		SSE = 1,
		SE = 2,
		SEE = 3,
		E = 4,
		NEE = 5,
		NE = 6,
		NNE = 7,
		N = 8,
		NNW = 9,
		NW = 10,
		NWW = 11,
		W = 12,
		SWW = 13,
		SW = 14,
		SSW = 15
	};

	static Dir get_opposite_direction(Dir direction);
	static float get_angle_between_directions(Dir dirA, Dir dirB);
	static Dir get_direction_from_vector(Vector2 vector);
	static String get_direction_string_from_vector(Vector2 vector, bool only_eight_dirs = false);
};

VARIANT_ENUM_CAST(DirectionsUtil::Dir);

#endif //GODOT_CLONE_DIRECTIONSUTIL_H
