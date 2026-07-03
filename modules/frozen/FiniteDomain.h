#ifndef FINITE_DOMAIN_H
#define FINITE_DOMAIN_H

#include "core/templates/local_vector.h"
#include "core/typedefs.h"

// Tier-B internal type for the constraint solver: a dynamically sized bit set
// with value semantics, used as the finite domain of a variable (the set of
// still-possible values) and also for tag membership masks.
//
// The width (number of representable values) is the number of element templates
// in the problem's catalog (or the number of interned tags for tag-id sets), so
// there is no 64-value limit -- the backing store grows in 64-bit words.
//
// Bit-wise operations follow the recommendation in GameAIPro2 Ch.26 (§26.9.1):
// intersection is AND, set-subtract is AND-NOT, "is unique" is a single-bit test.
struct FiniteDomain {
	LocalVector<uint64_t> words;

	static _FORCE_INLINE_ uint32_t _word_count(uint32_t p_num_values) {
		return (p_num_values + 63u) >> 6;
	}

	static _FORCE_INLINE_ uint32_t _popcount64(uint64_t v) {
		v = v - ((v >> 1) & 0x5555555555555555ULL);
		v = (v & 0x3333333333333333ULL) + ((v >> 2) & 0x3333333333333333ULL);
		v = (v + (v >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
		return (uint32_t)((v * 0x0101010101010101ULL) >> 56);
	}

	static _FORCE_INLINE_ uint32_t _ctz64(uint64_t v) {
		// Lowest set bit index; v must be non-zero.
		uint32_t n = 0;
		while ((v & 1ULL) == 0) {
			v >>= 1;
			n++;
		}
		return n;
	}

	// Size the domain to hold p_num_values values, all clear or (p_fill) all set.
	void resize_values(uint32_t p_num_values, bool p_fill) {
		const uint32_t n = _word_count(p_num_values);
		words.resize(n);
		const uint64_t fill = p_fill ? ~0ULL : 0ULL;
		for (uint32_t i = 0; i < n; i++) {
			words[i] = fill;
		}
		// Mask off the unused high bits of the last word so popcount/equality are exact.
		if (p_fill && n > 0) {
			const uint32_t rem = p_num_values & 63u;
			if (rem != 0) {
				words[n - 1] = ~0ULL >> (64u - rem);
			}
		}
	}

	_FORCE_INLINE_ bool test(uint32_t i) const {
		return (words[i >> 6] >> (i & 63u)) & 1ULL;
	}
	_FORCE_INLINE_ void set(uint32_t i) {
		words[i >> 6] |= (1ULL << (i & 63u));
	}
	_FORCE_INLINE_ void unset(uint32_t i) {
		words[i >> 6] &= ~(1ULL << (i & 63u));
	}
	void clear() {
		for (uint32_t i = 0; i < words.size(); i++) {
			words[i] = 0ULL;
		}
	}

	bool is_empty() const {
		for (uint32_t i = 0; i < words.size(); i++) {
			if (words[i] != 0ULL) {
				return false;
			}
		}
		return true;
	}

	uint32_t count() const {
		uint32_t c = 0;
		for (uint32_t i = 0; i < words.size(); i++) {
			c += _popcount64(words[i]);
		}
		return c;
	}

	// True if exactly one bit is set (the variable is decided).
	bool is_unique() const {
		uint32_t seen = 0;
		for (uint32_t i = 0; i < words.size(); i++) {
			const uint64_t w = words[i];
			if (w != 0ULL) {
				if (w & (w - 1ULL)) {
					return false; // more than one bit in this word
				}
				if (++seen > 1) {
					return false;
				}
			}
		}
		return seen == 1;
	}

	// Index of the lowest set value, or -1 if empty.
	int first_value() const {
		for (uint32_t i = 0; i < words.size(); i++) {
			if (words[i] != 0ULL) {
				return (int)(i * 64u + _ctz64(words[i]));
			}
		}
		return -1;
	}

	// this &= other ; returns true if any bit was removed.
	bool intersect_with(const FiniteDomain &o) {
		bool changed = false;
		for (uint32_t i = 0; i < words.size(); i++) {
			const uint64_t before = words[i];
			words[i] = before & o.words[i];
			changed |= (words[i] != before);
		}
		return changed;
	}

	// this &= ~other ; returns true if any bit was removed.
	bool subtract(const FiniteDomain &o) {
		bool changed = false;
		for (uint32_t i = 0; i < words.size(); i++) {
			const uint64_t before = words[i];
			words[i] = before & ~o.words[i];
			changed |= (words[i] != before);
		}
		return changed;
	}

	// True if this set shares at least one value with other.
	bool intersects(const FiniteDomain &o) const {
		for (uint32_t i = 0; i < words.size(); i++) {
			if (words[i] & o.words[i]) {
				return true;
			}
		}
		return false;
	}

	// True if every value of this set is also in other (this is a subset of other).
	bool is_subset_of(const FiniteDomain &o) const {
		for (uint32_t i = 0; i < words.size(); i++) {
			if (words[i] & ~o.words[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator==(const FiniteDomain &o) const {
		if (words.size() != o.words.size()) {
			return false;
		}
		for (uint32_t i = 0; i < words.size(); i++) {
			if (words[i] != o.words[i]) {
				return false;
			}
		}
		return true;
	}
	bool operator!=(const FiniteDomain &o) const { return !(*this == o); }

	// Append every set value's index to p_out (cleared first).
	void collect_values(LocalVector<int> &p_out) const {
		p_out.clear();
		for (uint32_t i = 0; i < words.size(); i++) {
			uint64_t w = words[i];
			while (w != 0ULL) {
				const uint32_t b = _ctz64(w);
				p_out.push_back((int)(i * 64u + b));
				w &= (w - 1ULL); // clear lowest set bit
			}
		}
	}
};

#endif // FINITE_DOMAIN_H
