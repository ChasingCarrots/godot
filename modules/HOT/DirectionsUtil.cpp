#include "DirectionsUtil.h"

#include <core/math/math_funcs.h>
#include <cstdlib>

const float DIRECTION_INCREMENT_ANGLE = 22.5f;
const int DIRECTION_COUNT = 16;

void DirectionsUtil::_bind_methods() {
	BIND_ENUM_CONSTANT(S);
	BIND_ENUM_CONSTANT(SSE);
	BIND_ENUM_CONSTANT(SE);
	BIND_ENUM_CONSTANT(SEE);
	BIND_ENUM_CONSTANT(E);
	BIND_ENUM_CONSTANT(NEE);
	BIND_ENUM_CONSTANT(NE);
	BIND_ENUM_CONSTANT(NNE);
	BIND_ENUM_CONSTANT(N);
	BIND_ENUM_CONSTANT(NNW);
	BIND_ENUM_CONSTANT(NW);
	BIND_ENUM_CONSTANT(NWW);
	BIND_ENUM_CONSTANT(W);
	BIND_ENUM_CONSTANT(SWW);
	BIND_ENUM_CONSTANT(SW);
	BIND_ENUM_CONSTANT(SSW);

	ClassDB::bind_static_method("DirectionsUtil", D_METHOD("get_opposite_direction", "direction"), &DirectionsUtil::get_opposite_direction);
	ClassDB::bind_static_method("DirectionsUtil", D_METHOD("get_angle_between_directions", "dirA", "dirB"), &DirectionsUtil::get_angle_between_directions);
	ClassDB::bind_static_method("DirectionsUtil", D_METHOD("get_direction_from_vector", "vector"), &DirectionsUtil::get_direction_from_vector);
	ClassDB::bind_static_method("DirectionsUtil", D_METHOD("get_direction_string_from_vector", "vector", "only_eight_dirs"), &DirectionsUtil::get_direction_string_from_vector, DEFVAL(false));
}

DirectionsUtil::Dir DirectionsUtil::get_opposite_direction(DirectionsUtil::Dir direction) {
	return (DirectionsUtil::Dir)Math::wrapi(direction+8, DirectionsUtil::Dir::S, DirectionsUtil::Dir::SSW);
}

float DirectionsUtil::get_angle_between_directions(DirectionsUtil::Dir dirA, DirectionsUtil::Dir dirB) {
	int leftTurn = ((int)dirA - (int)dirB) + 1;
	if((int)dirA < (int)dirB) leftTurn += DIRECTION_COUNT;
	int rightTurn = ((int)dirB - (int)dirA) + 1;
	if((int)dirA > (int)dirB) rightTurn += DIRECTION_COUNT;
	return (float)std::min(leftTurn, rightTurn) * DIRECTION_INCREMENT_ANGLE;
}

DirectionsUtil::Dir DirectionsUtil::get_direction_from_vector(Vector2 vector) {
	float angle = Math::rad_to_deg(Math::atan2(vector.x, vector.y));
	return (DirectionsUtil::Dir)Math::wrapi(
			(int)Math::round(angle / DIRECTION_INCREMENT_ANGLE),
			0, DIRECTION_COUNT);
}

String DirectionsUtil::get_direction_string_from_vector(Vector2 vector, bool only_eight_dirs) {
	auto dir_enum = DirectionsUtil::get_direction_from_vector(vector);
	if(only_eight_dirs && (int)dir_enum % 2 > 0)
		dir_enum = (DirectionsUtil::Dir)(dir_enum - 1);
	switch (dir_enum) {
		case S:
			return "S";
		case SSE:
			return "SSE";
		case SE:
			return "SE";
		case SEE:
			return "SEE";
		case E:
			return "E";
		case NEE:
			return "NEE";
		case NE:
			return "NE";
		case NNE:
			return "NNE";
		case N:
			return "N";
		case NNW:
			return "NNW";
		case NW:
			return "NW";
		case NWW:
			return "NWW";
		case W:
			return "W";
		case SWW:
			return "SWW";
		case SW:
			return "SW";
		case SSW:
			return "SSW";
	}
	return "";
}
