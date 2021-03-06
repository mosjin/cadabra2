#pragma once
#include <stdlib.h>
#include <chrono>
#include <random>
#include <vector>

namespace cadabra
{
	namespace detail
	{
		inline unsigned int get_seed()
		{
			auto tp = std::chrono::system_clock::now();
			return static_cast<unsigned long>(tp.time_since_epoch().count());
		}
	}

	inline uint64_t generate_uuid()
	{
		static bool seeded = false;
		static std::mt19937 rng;
		static std::uniform_int_distribution<uint64_t> uni(1);

		if (!seeded) {
			rng.seed(detail::get_seed());
			seeded = true;
		}

		return uni(rng);
	}
}