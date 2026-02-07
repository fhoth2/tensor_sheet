#include "loader.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <numeric> // For std::accumulate
#include <unistd.h>


Tensor load_binary_tensor(Arena* a, const std::string& filename, std::initializer_list<int> shape_list) {
	// 1. Calculate expected total elements and size
	size_t total_elements = 1;
	for (int dim : shape_list) {
		total_elements *= dim;
	}
	size_t expected_bytes = total_elements * sizeof(float);

	// 2. Open the file in Binary mode, starting at the end (ate) to check size
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "!! Failed to open file: " << filename << "\n";
		exit(1);
	}

	// 3. Verify file size matches expected shape
	std::streamsize file_size = file.tellg();
	if ((size_t)file_size != expected_bytes) {
		std::cerr << "!! Error: File size mismatch.\n";
		std::cerr << "Expected: " << expected_bytes << "bytes based on shape.\n";
		std::cerr << "Found: " << file_size << " bytes in file.\n";
		exit(1);
	}

	// 4. Seek back to the beginning
	file.seekg(0, std::ios::beg);

	// 5. Allocate memory in the Arena
	// This will be Unified Memory if CUDA IS ON!
	float* data_ptr = arena_alloc_array<float>(a, total_elements);

	std::cout << ">> Loading " << file_size << " bytes from " << filename << "into Arena...\n";

	// 6. Read directly into arena memory. No intermdiate buffers.
	if (!file.read(reinterpret_cast<char*>(data_ptr), file_size)) {
		std::cerr << "!! Error reading file data.\n";
		exit(1);
	}
	file.close();

	// 7. Create the tensor view aroudn the existing memory
	// We manually contruct it because we already have the pointer
	Tensor t;
	t.data = data_ptr;
	t.size = total_elements;
	t.ndim = shape_list.size();
	int i = 0;
	for (int dim : shape_list) t.shape[i++] = dim;

	// Calculate strides (same logic as before)
	int current_stride = 1;
	for (int j = t.ndim - 1; j >= 0; --j) {
		t.strides[j] = current_stride;
		current_stride *= t.shape[j];
	}
	return t;
}

void save_binary_tensor(Tensor& t, const std::string& filename) {
	std::ofstream file(filename, std::ios::binary | std::ios::trunc);

	if(!file.is_open()) {
		std::cerr << "!!Error: Could not opend file for writing: " << filename << "\n";
		return;
	}

	// Calculate total bytes: elements * 4 bytes (sizeof float)
	size_t total_bytes = t.size * sizeof(float);

	std::cout << "\n>> Saving " << total_bytes << " bytes to " << filename << "...\n";
	
	// The Magic: dump the raw memory directly to the disk
	file.write(reinterpret_cast<const char*>(t.data), total_bytes);

	if (!file) {
		std::cerr << "!! Error: Write failed!\n";
	} else {
		std::cout << ">> Save Complete!\n";
	}
	sleep(1);
	file.close();
}
