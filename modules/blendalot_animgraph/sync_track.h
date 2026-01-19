#pragma once

#include "core/templates/local_vector.h"

#include <cassert>
#include <cmath>

/** @class SyncTrack used for synced animation blending.
 *
 *  A SyncTrack consists of multiple sync intervals that are adjacent to each other.
 *
 *  Important definitions:
 *
 *  - Absolute Time: time within an animation duration in seconds.
 *  - Ratio: time relative to the SyncTrack duration, e.g. 0.5 corresponds to 50% of the duration.
 *  - SyncTime is a floating point value where the integer parts defines the sync interval and the
 *    fractional part the fraction within the interval. I.e. a SyncTime of 5.332 means it is ~33%
 *    through interval 5.
 *
 *  A sync interval is defined by a ratio of the starting point and the ratio of the interval's
 *  duration. Blended SyncTracks always have their first interval start at t = 0.0s.
 */
struct SyncTrack {
	static constexpr int cSyncTrackMaxIntervals = 8;

	SyncTrack() :
			duration(0.f), num_intervals(1) {
		for (int i = 0; i < cSyncTrackMaxIntervals; i++) {
			interval_start_ratio[i] = 0.f;
			interval_duration_ratio[i] = 0.f;
		}

		interval_duration_ratio[0] = 1.0f;
	}

	float duration = 1.0;
	int num_intervals = 1;
	float interval_start_ratio
			[cSyncTrackMaxIntervals]; //< Starting time of interval in absolute time.
	float interval_duration_ratio[cSyncTrackMaxIntervals]; //<

	float calc_sync_from_abs_time(float abs_time) const {
		for (int i = 0; i < num_intervals; i++) {
			float query_abs_time = abs_time;
			float interval_start = interval_start_ratio[i] * duration;
			float interval_end =
					interval_start + interval_duration_ratio[i] * duration;

			if (query_abs_time < interval_start) {
				query_abs_time += duration;
			}
			if (query_abs_time >= interval_start && query_abs_time < interval_end) {
				return float(i) + (query_abs_time - interval_start) / (interval_end - interval_start);
			}
		}

		assert(false && "Invalid absolute time");
		return -1.f;
	}

	double calc_ratio_from_sync_time(double sync_time) const {
		float interval_ratio = fmod(sync_time, 1.0f);
		int interval = int(sync_time - interval_ratio);

		return fmod(
				interval_start_ratio[interval] + interval_duration_ratio[interval] * interval_ratio,
				1.0f);
	}

	bool operator==(const SyncTrack &other) const {
		bool result = duration == other.duration && num_intervals == other.num_intervals;

		if (!result) {
			return false;
		}

		for (int i = 0; i < num_intervals; i++) {
			if ((fabsf(interval_start_ratio[i] - other.interval_start_ratio[i]) > 1.0e-5) || (fabsf(interval_duration_ratio[i] - other.interval_duration_ratio[i]) > 1.0e-5)) {
				return false;
			}
		}

		return true;
	}

	/** Constructs SyncTrack from markers.
	 *
	 * Markers are specified in absolute time and must be >= 0 and <= duration. They define the
	 * start of the interval. The last marker is implicitly the (possibly looped) first marker.
	 */
	static SyncTrack create_from_markers(
			float duration,
			const LocalVector<float> &markers) {
		assert(markers.size() > 0);
		assert(markers.size() < cSyncTrackMaxIntervals);

		SyncTrack result;
		result.duration = duration;
		result.num_intervals = markers.size();

		for (unsigned int i = 0; i < markers.size(); i++) {
			assert(markers[i] >= 0.f && markers[i] <= duration);

			int end_index = i == (markers.size() - 1) ? 0 : i + 1;

			float interval_start = markers[i];
			float interval_end = markers[end_index];

			if (interval_end == interval_start) {
				interval_end = interval_start + duration;
			} else if (interval_end < interval_start) {
				interval_end += duration;
			}

			result.interval_start_ratio[i] = interval_start / duration;
			result.interval_duration_ratio[i] =
					(interval_end - interval_start) / duration;
		}

		return result;
	}

	/** Creates a blended SyncTrack from two input SyncTracks
	 *
	 * \note the first interval will always start at sync time 0, i.e. interval_start_ratio[0] = 0.0.
	 */
	static SyncTrack
	blend(float weight, const SyncTrack &track_A, const SyncTrack &track_B) {
		assert(track_A.num_intervals == track_B.num_intervals);

		SyncTrack result;
		result.num_intervals = track_A.num_intervals;

		result.duration =
				(1.0f - weight) * track_A.duration + weight * track_B.duration;

		result.interval_start_ratio[0] = 0.f;

		for (int i = 0; i < result.num_intervals; i++) {
			float interval_duration_A = track_A.interval_duration_ratio[i];
			float interval_duration_B = track_B.interval_duration_ratio[i];
			result.interval_duration_ratio[i] =
					(1.0f - weight) * interval_duration_A + weight * interval_duration_B;

			if (i < cSyncTrackMaxIntervals) {
				result.interval_start_ratio[i + 1] =
						result.interval_start_ratio[i] + result.interval_duration_ratio[i];
				if (result.interval_start_ratio[i + 1] > 1.0f) {
					result.interval_start_ratio[i + 1] =
							fmodf(result.interval_start_ratio[i + 1], 1.0f);
				}
			}
		}

		assert(result.num_intervals < cSyncTrackMaxIntervals);

		return result;
	}
};