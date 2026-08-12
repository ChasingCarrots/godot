#[compute]

#version 450

#VERSION_DEFINES

#define BLOCK_SIZE 16
#define BIN_COUNT 256

layout(local_size_x = BLOCK_SIZE, local_size_y = BLOCK_SIZE, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform sampler2D source_texture;

layout(set = 1, binding = 0, std430) restrict buffer Histogram {
	uint bins[];
}
histogram;

layout(push_constant, std430) uniform Params {
	ivec2 source_size;
	float min_luminance;
	float log_min;
	float inv_log_range;
	float pad[3];
}
params;

shared uint tmp_bins[BIN_COUNT];

void main() {
	// One thread per bin, which is why the block is 16x16.
	uint t = gl_LocalInvocationIndex;
	tmp_bins[t] = 0u;

	groupMemoryBarrier();
	barrier();

	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	if (all(lessThan(pos, params.source_size))) {
		vec3 v = texelFetch(source_texture, pos, 0).rgb;
		float lum = dot(v, vec3(0.2126, 0.7152, 0.0722));

		// Bin 0 collects everything at or below the bottom of the metering range,
		// which keeps log2() away from -inf. Bins 1..255 span the range evenly in
		// log space.
		uint bin = 0u;
		if (lum > params.min_luminance) {
			float e = (log2(lum) - params.log_min) * params.inv_log_range;
			bin = uint(clamp(e, 0.0, 1.0) * float(BIN_COUNT - 2)) + 1u;
		}
		atomicAdd(tmp_bins[bin], 1u);
	}

	groupMemoryBarrier();
	barrier();

	// Merge this workgroup's bins into the global histogram. Most bins are empty
	// for any given block, so skipping those saves the bulk of the global atomics.
	if (tmp_bins[t] > 0u) {
		atomicAdd(histogram.bins[t], tmp_bins[t]);
	}
}
