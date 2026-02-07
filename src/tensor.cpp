#include "tensor.h"
#include "arena.h"
#include <iostream>

Tensor tensor_create(Arena* a, std::vector<size_t> shape) {
	Tensor t = {}; // Zero out everyting first
	
	t.ndim = (int)shape.size();

	// 1. Fill shape & Default remaining dims to 1 
	for (size_t i = 0; i < 3; i++) {
		if (i < shape.size()) {
			t.shape[i] = shape[i];
		} else {
			t.shape[i] = 1;
		}
	}

	// 2. calculate strides (Row-major layout)
	// stride[2] (Width) moves 1 spot
	// stride[1] (Height) moves Width spots
	// stride[0] (depth) moves Width * Height spots
	t.strides[2] = 1;
	t.strides[1] = t.shape[2];
	t.strides[0] = t.shape[1] * t.shape[2];

	// 3. calculate Total size
	t.size = t.shape[0] * t.shape[1] * t.shape[2];

	// 4. Allocate Memory 
	// NOTE: We cast the size to bytes
	t.data = (float*)arena_alloc(a, t.size * sizeof(float));

	if (t.data == nullptr) {
		t.size = 0;  // if Memory full Invaildate
	}

	return t;
}

// ACCESSOR: Get a value using 64-bit indices
float& tensor_get(Tensor& t, size_t z, size_t y, size_t x) {
	// Formula index = (z * stride_z) + (y * stride_y) + (x * stride_x)
	size_t index = (z * t.strides[0]) + (y * t.strides[1]) + (x * t.strides[2]);
	return t.data[index];
}

