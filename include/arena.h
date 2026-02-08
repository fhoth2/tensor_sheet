#pragma once
#include <cstddef>
#include <cstdint>


// linear memory arena.
// It grabs one huge chunk of memory at start up and hands out slices.
struct Arena {
	uint8_t* base_ptr;	// The start of our memory block
	size_t capacity;	// Total size available
	size_t offset;		// Current allocation position
};

// Intitialize the areana with a specific size (e.g., 1GB)
void arena_init(Arena* a, size_t size_bytes);

// Destroy/Free the arena
void arena_free(Arena* a);

// Reset the arena (wipes all data instantly by resetting offset)
void arena_reset(Arena* a);

// Allocate a block of memory from the arena
void* arena_alloc(Arena* a, size_t size_bytes);

// Helper: Allocate a specific type (Template for convenience)
template <typename T>
T* arena_alloc_array(Arena* a, size_t count) {
	return (T*)arena_alloc(a, count * sizeof(T));
}
