// `levenshtein.c` - levenshtein
// MIT licensed.
// Copyright (c) 2015 Titus Wormer <tituswormer@gmail.com>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Returns a size_t, depicting the difference between `a` and `b`.
// See <http://en.wikipedia.org/wiki/Levenshtein_distance> for more information.
static size_t levenshtein_private(const char *a, const size_t length, const char *b, const size_t bLength) {
	size_t *cache = (size_t*)calloc(length, sizeof(size_t));
	size_t index = 0;
	size_t bIndex = 0;
	size_t distance;
	size_t bDistance;
	size_t result = std::max( length, bLength );
	char code;

	// Shortcut optimizations / degenerate cases.
	if (a == b) {
		return 0;
	}

	if (length == 0) {
		return bLength;
	}

	if (bLength == 0) {
		return length;
	}

	// initialize the vector.
	while (index < length) {
		cache[index] = index + 1;
		index++;
	}

	// Loop.
	while (bIndex < bLength) {
		code = b[bIndex];
		result = distance = bIndex++;
		index = SIZE_MAX;

		while (++index < length) {
			bDistance = code == a[index] ? distance : distance + 1;
			distance = cache[index];

			cache[index] = result = distance > result
			? bDistance > result
				? result + 1
				: bDistance
			: bDistance > distance
				? distance + 1
				: bDistance;
		}
	}

	free(cache);

	return result;
}

static size_t levenshtein( const std::string & a, const std::string & b )
{
	return levenshtein_private( a.c_str(), a.size(), b.c_str(), b.size() );
}
