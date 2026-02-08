#pragma once
#include "arena.h"
#include <initializer_list>
#include <vector>
#include <cstddef> // Required for size_t

#define MAX_DIMS 4 // Support up to 4 dimenstions (e.g., Batch, Layer, Row, Col)

struct Tensor {
	float* data;			// POinter to the start of data in the Arena
	size_t shape[MAX_DIMS];		// Size of each dimension (e.g., [3, 10, 10])
	size_t strides[MAX_DIMS];		// How many steps to jump in flat memory to move 1 unit
	int ndim;			// Current number of dimensions (e.g., 3)
	size_t size;			// total number of elements
};

// Create a new tensor in the arena
Tensor tensor_create(Arena* a, std::vector<size_t> shape);

/*
// get the value at a specific N-dimensional coordinate
// We use a variadic template so you can call t(o, 1, 4)
template <typename... Args>
float& tensor_get(Tensor& t, Args... indices) {
	int idx_list[] = { static_cast<int>(indices)... };

	// Calculate the flat index: index = z*stride[0] + y*stride[i] * x*stride[2]
	size_t flat_index = 0;
	for (int i = 0; i < t.ndim; ++i) {
		flat_index += idx_list[i] * t.strides[i];
	}

	return t.data[flat_index];
}*/

float& tensor_get(Tensor& t, size_t z, size_t y, size_t x);
