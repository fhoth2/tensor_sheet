#include "arena.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#endif

void arena_init(Arena* a, size_t size_bytes) {
	a->capacity = size_bytes;
	a->offset = 0;

	#ifdef ENABLE_CUDA
		// UNIFIED MEMORY: The Holy Grail.
		// This pointer works on the CPU *and* the RTX 5070 without manual copying.
		cudaError_t err = cudaMallocManaged((void**)&a->base_ptr, size_bytes);

		if (err != cudaSuccess) {
			std::cerr << "!! CUDA Alloc Failed: " << cudaGetErrorString(err) << "\n";
			exit(1);
		}
		std::cout << ">> Arena Allocated " << (size_bytes / 1024 / 1024) << "MB (Unified GPU Memory)\n";
	#else
		// CPU FALLBACK: Standard aligned RAM.
		// We align to 64 bytes for AVX-512/AVX2 cache line friendliness.
		a->base_ptr = (uint8_t*)std::aligned_alloc(64, size_bytes);

		if (!a->base_ptr) {
			std::cerr << "!! CPU Alloc Failed\n";
			exit(1);
		}
		std::cout << ">> Arena Allocated " << (size_bytes / 1024 / 1024) << "MB (CPU RAM)\n";
	#endif
}

void arena_free(Arena* a) {
	#ifdef ENABLE_CUDA
		cudaFree(a->base_ptr);
	#else
		free(a->base_ptr);
	#endif
	a->base_ptr = nullptr;
	a->capacity = 0;
	a->offset = 0;
}

void arena_reset(Arena* a) {
	// Instant Wipe
	a->offset = 0;
}

void* arena_alloc(Arena* a, size_t size_bytes) {
	// 1. Align the allocation pointer to 8 bytes (keeps CPU happy)
	size_t alignment = 8;
	size_t current_addr = (size_t)(a->base_ptr + a->offset);
	size_t padding = (alignment - (current_addr % alignment)) % alignment;

	if (a->offset + padding + size_bytes > a->capacity) {
		std::cerr << "!! Arena Out of Memory!\n";
		return nullptr;
	}

	a->offset += padding;
	void* ptr = a->base_ptr + a->offset;
	a->offset += size_bytes;

	return ptr;
}
