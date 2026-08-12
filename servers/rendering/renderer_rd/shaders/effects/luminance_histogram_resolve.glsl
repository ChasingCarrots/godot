#[compute]

#version 450

#VERSION_DEFINES

#define BIN_COUNT 256

layout(local_size_x = BIN_COUNT, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0, std430) restrict buffer Histogram {
	uint bins[];
}
histogram;

layout(r32f, set = 1, binding = 0) uniform restrict writeonly image2D dest_luminance;

layout(set = 2, binding = 0) uniform sampler2D prev_luminance;

layout(push_constant, std430) uniform Params {
	float low_percent;
	float high_percent;
	float log_min;
	float log_range;
	float min_clamp;
	float max_clamp;
	float exposure_adjust;
	uint set_immediate;
}
params;

shared uint counts[BIN_COUNT];

void main() {
	uint t = gl_LocalInvocationIndex;
	counts[t] = histogram.bins[t];

	groupMemoryBarrier();
	barrier();

	// A single workgroup, so the trim walk is 256 serial iterations on one thread.
	// That is cheaper than the synchronization a parallel scan would need here.
	if (t != 0u) {
		return;
	}

	float total = 0.0;
	for (int i = 0; i < BIN_COUNT; i++) {
		total += float(counts[i]);
	}

	float prev = texelFetch(prev_luminance, ivec2(0, 0), 0).r; // 1 pixel previous exposure.

	if (total <= 0.0) {
		imageStore(dest_luminance, ivec2(0, 0), vec4(prev));
		return;
	}

	// Discard the darkest low_percent and the brightest (1 - high_percent) of the
	// pixel population, then average what remains in log space. Outliers stop
	// mattering because they are counted by population, not by magnitude. A bin
	// straddling a cut point contributes only the part of it inside the window.
	float lo = total * params.low_percent;
	float hi = total * params.high_percent;

	float accum = 0.0;
	float kept = 0.0;
	float cum = 0.0;

	for (int i = 0; i < BIN_COUNT; i++) {
		float c = float(counts[i]);
		float overlap = min(cum + c, hi) - max(cum, lo);
		cum += c;

		if (overlap <= 0.0) {
			continue;
		}

		// Position of this bin within the metering range, in 0..1.
		float e = (i == 0) ? 0.0 : float(i - 1) / float(BIN_COUNT - 2);
		accum += overlap * e;
		kept += overlap;
	}

	float lum = prev;
	if (kept > 0.0) {
		lum = exp2(params.log_min + (accum / kept) * params.log_range);
	}

	if (params.set_immediate == 0u) {
		lum = prev + (lum - prev) * params.exposure_adjust;
	}
	lum = clamp(lum, params.min_clamp, params.max_clamp);

	imageStore(dest_luminance, ivec2(0, 0), vec4(lum));
}
