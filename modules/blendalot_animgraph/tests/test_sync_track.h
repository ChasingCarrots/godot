#pragma once

#include "../sync_track.h"

#include "tests/test_macros.h"

namespace TestBlendalotAnimationGraph {

TEST_CASE("[Blendalot][SyncTrack] Basic") {
	SyncTrack track_a;
	track_a.num_intervals = 2;
	track_a.duration = 2.0;
	track_a.interval_start_ratio[0] = 0.f;
	track_a.interval_duration_ratio[0] = 0.7;
	track_a.interval_start_ratio[1] = 0.7f;
	track_a.interval_duration_ratio[1] = 0.3;

	SyncTrack track_b;
	track_b.num_intervals = 2;
	track_b.duration = 1.5;
	track_b.interval_start_ratio[0] = 0.0f;
	track_b.interval_duration_ratio[0] = 0.6;
	track_b.interval_start_ratio[1] = 0.6f;
	track_b.interval_duration_ratio[1] = 0.4;

	WHEN("Calculating sync time of track_B at 0.5 duration") {
		float sync_time_at_0_75 =
				track_b.calc_sync_from_abs_time(0.5 * track_b.duration);
		REQUIRE(sync_time_at_0_75 == doctest::Approx(0.83333));
	}

	WHEN("Calculating sync time of track_B at 0.6 duration") {
		float sync_time_at_0_6 =
				track_b.calc_sync_from_abs_time(0.6 * track_b.duration);
		REQUIRE(sync_time_at_0_6 == doctest::Approx(1.0));
	}

	WHEN("Calculating sync time of track_B at 0.7 duration") {
		float sync_time_at_0_7 =
				track_b.calc_sync_from_abs_time(0.7 * track_b.duration);
		REQUIRE(sync_time_at_0_7 == doctest::Approx(1.25));
	}

	WHEN("Calculating sync time of track_B at 0.0 duration") {
		float sync_time_at_1_0 =
				track_b.calc_sync_from_abs_time(0.0 * track_b.duration);
		REQUIRE(sync_time_at_1_0 == doctest::Approx(0.0));
	}

	WHEN("Calculating sync time of track_B at 1.0 duration") {
		float sync_time_at_1_0 =
				track_b.calc_sync_from_abs_time(0.9999 * track_b.duration);
		REQUIRE(sync_time_at_1_0 == doctest::Approx(2.0).epsilon(0.001f));
	}

	WHEN("Calculating ratio from sync time on track_A at 0.83333") {
		float ratio = track_a.calc_ratio_from_sync_time(0.83333333);
		REQUIRE(ratio == doctest::Approx(0.5833333));
	}

	WHEN("Calculating ratio from sync time on track_A at 0.83333") {
		float ratio = track_a.calc_ratio_from_sync_time(1.25);
		REQUIRE(ratio == doctest::Approx(0.775));
	}

	WHEN("Blending two synctracks with weight 0.") {
		SyncTrack blended = SyncTrack::blend(0.f, track_a, track_b);

		blended.duration = track_a.duration;
		blended.interval_start_ratio[0] = 0.0;
		for (int i = 0; i < track_a.num_intervals; i++) {
			CHECK(blended.interval_duration_ratio[i] == track_a.interval_duration_ratio[i]);
		}
	}

	WHEN("Blending two synctracks with weight 1.") {
		SyncTrack blended = SyncTrack::blend(1.f, track_a, track_b);

		blended.duration = track_b.duration;
		blended.interval_start_ratio[0] = 0.0;
		for (int i = 0; i < track_b.num_intervals; i++) {
			CHECK(blended.interval_duration_ratio[i] == track_b.interval_duration_ratio[i]);
		}
	}
}

TEST_CASE("[Blendalot][SyncTrack] Create Sync Track from markers") {
	SyncTrack track = SyncTrack::create_from_markers(2.0f, { 0.9f, 0.2f });

	WHEN("Querying Ratios") {
		CHECK(track.interval_start_ratio[0] == doctest::Approx(0.45f));
		CHECK(track.interval_duration_ratio[0] == doctest::Approx(0.65f));

		CHECK(track.interval_start_ratio[1] == doctest::Approx(0.1f));
		CHECK(track.interval_duration_ratio[1] == doctest::Approx(0.35f));

		WHEN("Querying ratio at sync time at 0.001") {
			float ratio = track.calc_ratio_from_sync_time(0.0001f);
			CHECK(ratio == doctest::Approx(0.45).epsilon(0.001));
		}

		WHEN("Querying ratio at sync time at 0.9999") {
			float ratio = track.calc_ratio_from_sync_time(0.9999f);
			CHECK(ratio == doctest::Approx(0.1).epsilon(0.001));
		}

		WHEN("Querying ratio at sync time at 1.001") {
			float ratio = track.calc_ratio_from_sync_time(1.0001f);
			CHECK(ratio == doctest::Approx(0.1).epsilon(0.001));
		}

		WHEN("Querying ratio at sync time at 1.9999") {
			float ratio = track.calc_ratio_from_sync_time(1.9999f);
			CHECK(ratio == doctest::Approx(0.45).epsilon(0.001));
		}
	}

	WHEN("Querying SyncTime from Absolute Time") {
		WHEN("Querying absolute time at 0.9001s") {
			float sync_time = track.calc_sync_from_abs_time(0.9001f);
			CHECK(sync_time == doctest::Approx(0.0).epsilon(0.001));
		}

		WHEN("Querying absolute time at 0.2001s") {
			float sync_time = track.calc_sync_from_abs_time(0.2001f);
			CHECK(sync_time == doctest::Approx(1.0).epsilon(0.001));
		}

		WHEN("Querying absolute time at 0.8999s") {
			float sync_time = track.calc_sync_from_abs_time(0.8999f);
			CHECK(sync_time == doctest::Approx(1.999).epsilon(0.001));
		}

		WHEN("Querying absolute time at 1.9999s") {
			float sync_time = track.calc_sync_from_abs_time(1.9999f);
			CHECK(sync_time == doctest::Approx(0.84615384).epsilon(0.001));
		}
	}
}

TEST_CASE("[Blendalot][SyncTrack] Sync Track blending") {
	SyncTrack track_a = SyncTrack::create_from_markers(2.0, { 0., 0.6, 1.8 });
	SyncTrack track_b = SyncTrack::create_from_markers(1.5f, { 1.05, 1.35, 0.3 });

	WHEN("Calculating A's durations") {
		CHECK(track_a.interval_duration_ratio[0] == doctest::Approx(0.3));
		CHECK(track_a.interval_duration_ratio[1] == doctest::Approx(0.6));
		CHECK(track_a.interval_duration_ratio[2] == doctest::Approx(0.1));
	}

	WHEN("Calculating B's durations") {
		CHECK(track_b.interval_duration_ratio[0] == doctest::Approx(0.2));
		CHECK(track_b.interval_duration_ratio[1] == doctest::Approx(0.3));
		CHECK(track_b.interval_duration_ratio[2] == doctest::Approx(0.5));
	}

	WHEN("Blending two synctracks with weight 0.") {
		SyncTrack blended = SyncTrack::blend(0.f, track_a, track_b);

		blended.duration = track_a.duration;
		blended.interval_start_ratio[0] = 0.0;
		for (int i = 0; i < track_a.num_intervals; i++) {
			CHECK(blended.interval_duration_ratio[i] == track_a.interval_duration_ratio[i]);
		}
	}

	WHEN("Blending two synctracks with weight 1.") {
		SyncTrack blended = SyncTrack::blend(1.f, track_a, track_b);

		blended.duration = track_b.duration;
		blended.interval_start_ratio[0] = 0.0;
		for (int i = 0; i < track_b.num_intervals; i++) {
			CHECK(blended.interval_duration_ratio[i] == track_b.interval_duration_ratio[i]);
		}
	}

	WHEN("Blending with weight 0.2") {
		float weight = 0.2f;
		SyncTrack blended = SyncTrack::blend(weight, track_a, track_b);

		REQUIRE(
				blended.duration == (1.0f - weight) * track_a.duration + weight * track_b.duration);
		REQUIRE(
				blended.interval_start_ratio[0] == 0.0);
		REQUIRE(
				blended.interval_duration_ratio[1] == (1.0f - weight) * (track_a.interval_duration_ratio[1]) + weight * (track_b.interval_duration_ratio[1]));
		REQUIRE(
				blended.interval_duration_ratio[2] == (1.0f - weight) * (track_a.interval_duration_ratio[2]) + weight * (track_b.interval_duration_ratio[2]));
	}

	WHEN("Inverted blending with weight 0.2") {
		float weight = 0.2f;
		SyncTrack blended = SyncTrack::blend(weight, track_b, track_a);

		REQUIRE(
				blended.duration == (1.0f - weight) * track_b.duration + weight * track_a.duration);
		REQUIRE(
				blended.interval_start_ratio[0] == 0.0);
		REQUIRE(
				blended.interval_duration_ratio[1] == (1.0f - weight) * (track_b.interval_duration_ratio[1]) + weight * (track_a.interval_duration_ratio[1]));
		REQUIRE(
				blended.interval_duration_ratio[2] == (1.0f - weight) * (track_b.interval_duration_ratio[2]) + weight * (track_a.interval_duration_ratio[2]));
	}
}

TEST_CASE("[Blendalot][SyncTrack] Sync Track blending non-matching interval count") {
	SyncTrack track_a = SyncTrack::create_from_markers(2.0, { 0., 0.6, 1.8 });
	SyncTrack track_b = SyncTrack::create_from_markers(1.5f, { 1.05 });

	WHEN("Blending two synctracks with weight 0.") {
		SyncTrack blended = SyncTrack::blend(0.f, track_a, track_b);

		blended.duration = track_a.duration;
		blended.interval_start_ratio[0] = 0.0;
		for (int i = 0; i < track_a.num_intervals; i++) {
			CHECK(blended.interval_duration_ratio[i] == track_a.interval_duration_ratio[i]);
		}
	}
	WHEN("Blending two synctracks with weight 1.") {
		SyncTrack blended = SyncTrack::blend(1.f, track_a, track_b);

		blended.duration = track_b.duration;
		blended.interval_start_ratio[0] = 0.0;
		for (int i = 0; i < track_b.num_intervals; i++) {
			CHECK(blended.interval_duration_ratio[i] == track_b.interval_duration_ratio[i]);
		}
	}

	WHEN("Blending with weight 0.2") {
		float weight = 0.2f;
		SyncTrack blended = SyncTrack::blend(weight, track_a, track_b);

		float track_a_repeats = static_cast<float>(blended.num_intervals / track_a.num_intervals);
		float track_b_repeats = static_cast<float>(blended.num_intervals / track_b.num_intervals);

		CHECK(
				blended.duration == doctest::Approx(2.5));
		CHECK(
				blended.interval_start_ratio[0] == 0.0);
		CHECK(
				blended.interval_duration_ratio[0] == doctest::Approx((1.0 - weight) * track_a.interval_duration_ratio[0] / track_a_repeats + weight * track_b.interval_duration_ratio[0] / track_b_repeats));
		CHECK(
				blended.interval_duration_ratio[1] == doctest::Approx((1.0 - weight) * track_a.interval_duration_ratio[1] / track_a_repeats + weight * track_b.interval_duration_ratio[0] / track_b_repeats));
		CHECK(
				blended.interval_duration_ratio[2] == doctest::Approx((1.0 - weight) * track_a.interval_duration_ratio[2] / track_a_repeats + weight * track_b.interval_duration_ratio[0] / track_b_repeats));
	}
}

} //namespace TestBlendalotAnimationGraph