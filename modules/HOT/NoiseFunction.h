#ifndef NOISEFUNCTION_H
#define NOISEFUNCTION_H

#include <cstdint> // Ensure uint32_t is available

const uint32_t BIG_PRIME = 126247697;

inline uint32_t GenerateNoise(uint32_t position) {
	// basically taken from https://www.youtube.com/watch?v=LWFzPP8ZbdU&t=2422s

	uint32_t mangled = (uint32_t) position;
	mangled *= BIG_PRIME;
	mangled += 35189;
	mangled *= mangled;
	mangled ^= (mangled >> 13);
	return mangled;
}

#endif //NOISEFUNCTION_H