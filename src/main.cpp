#include <iostream>
#include <vector>
#include <string>
#include <cstdio>      // For fopen, fread
#include <unistd.h>    // For sleep()
#include "arena.h"
#include "tensor.h"
#include "loader.h"    // Now links correctly
#include "tui.h"
#include "sys/stat.h>"

size_t get_file_size(const std::string& filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // ---------------------------------------------------------
    // 1. HEADLESS MODE (Agent/Help)
    // ---------------------------------------------------------
    if (argc > 1) {
        std::string arg1 = argv[1];

        if (arg1 == "--json" || arg1 == "--capabilities" || arg1 == "agent_capabilities") {
            tui_print_json_help();
            return 0;
        }

        if (arg1 == "--help" || arg1 == "-h") {
            std::cout << "Maxine Tensor Editor (v1.0)\n";
            std::cout << "Usage: ./maxine_tensor [file] [d] [h] [w]\n";
            std::cout << "  --json : Output capabilities for AI agents.\n";
            return 0;
        }
    }

    // ---------------------------------------------------------
    // 2. INITIALIZATION
    // ---------------------------------------------------------
    Arena memory;
    arena_init(&memory, 1024 * 1024 * 1024); // 1GB

    Tensor t = {};
    std::string active_file = "gradient_3x8x8.bin"; 

    // ---------------------------------------------------------
    // 3. LOAD DATA
    // ---------------------------------------------------------
    if (argc >= 2) {
        active_file = argv[1];
        size_t fsize = get_file_size(active_file);
        if (fsize > 0) {
            arena_size = fsize + (64 * 1024 * 1024); // Add 64MB buffer
            std::cout << ">> Detected file size: " << (fsize / 1024 * 1024) << "MB\n";
            std::cout << ">> Adjusted arena size to " << (arena_size / 1024 * 1024) << " MB for file loading.\n";
            
        }
    }
    if (argc >= 5) {
        // PATH A: Command Line Loading (Manual Safety Load)
        active_file = argv[1];
        try {
            size_t d = std::stoul(argv[2]);
            size_t h = std::stoul(argv[3]);
            size_t w = std::stoul(argv[4]);
            
            t = tensor_create(&memory, {d, h, w});
            
            // We manual load here to avoid loader.cpp's exit(1) on failure
            FILE* f = fopen(active_file.c_str(), "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                size_t fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                
                size_t expected = t.size * sizeof(float);
                if (fsize >= expected) {
                    fread(t.data, sizeof(float), t.size, f);
                    std::cout << ">> Loaded " << active_file << "\n";
                } else {
                    std::cout << ">> Warning: File smaller than expected. Zero-padding.\n";
                    fread(t.data, 1, fsize, f); // Read partial
                }
                fclose(f);
            } else {
                std::cout << ">> File not found. Created empty tensor.\n";
            }
            sleep(1); 

        } catch (...) {
            std::cout << "Error: Invalid dimensions.\n";
            arena_free(&memory);
            return 1;
        }
    } 
    else {
        // PATH B: Default / Demo Mode
        // We use your loader.cpp here. 
        // NOTE: If gradient_3x8x8.bin is missing, loader.cpp will exit(1)!
        
        // Check if file exists first to prevent loader crash
        FILE* check = fopen(active_file.c_str(), "rb");
        if (check) {
            fclose(check);
            // Cast to int because loader.h takes initializer_list<int>
            t = load_binary_tensor(&memory, active_file, {3, 8, 8});
            std::cout << "Data loaded! Starting Editor...\n";
            sleep(1);
        } else {
            std::cout << ">> Default file missing. Starting empty.\n";
            t = tensor_create(&memory, {3, 8, 8});
            sleep(1);
        }
    }

    // ---------------------------------------------------------
    // 4. LAUNCH INTERFACE
    // ---------------------------------------------------------
    tui_loop(&memory, t, active_file);

    arena_free(&memory);
    return 0;
}
