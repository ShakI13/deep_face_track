// ref: https://codereview.stackexchange.com/questions/177973/softmax-function-implementation
#pragma once

#include <cmath>
#include <iterator>
#include <functional>
#include <numeric>
#include <type_traits>
#include <vector>

template <typename It>
void softmax(It beg, It end)
{
	using VType = typename std::iterator_traits<It>::value_type;

	static_assert(std::is_floating_point<VType>::value,
		"Softmax function only applicable for floating types");

	auto max_ele{ *std::max_element(beg, end) };

	std::transform(
		beg,
		end,
		beg,
		[&](VType x) { return std::exp(x - max_ele); });

	VType exptot = std::accumulate(beg, end, 0.0);

	std::transform(
		beg,
		end,
		beg,
		std::bind2nd(std::divides<VType>(), exptot));
}

void softmax(std::vector<float>& values)
{
	softmax(values.begin(), values.end());
}