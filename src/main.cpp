#include <iostream>
#include "arena.h"
#include "tensor.h"
#include "loader.h"
#include <unistd.h>

// Forward declaration of our TUI loop
void tui_loop(Arena* a,Tensor& t, const std::string& filename);

int main() {
	std::cout << "Maxine Tensor Engine Initializing...\n";

	// 1. Create the Arena
	Arena memory;
	arena_init(&memory, 1024 * 1024 * 10); // 10MB
	
	// 2. Load Real Data
	// We know the shape is [3, 8, 8] because we set it inthe python script.
	// In the future, this would be a command line argument
	std::string datafile = "../gradient_3x8x8.bin";
	Tensor real_data = load_binary_tensor(&memory, datafile, {3,8,8});

	std::cout << "Data loaded! Starting Editor...\n";
	sleep(1);



	// 4. Launch the interface
	tui_loop(&memory, real_data, datafile);


	// 5. Cleanup
	arena_free(&memory);
	return 0;
}
