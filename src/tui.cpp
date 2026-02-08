#include "tensor.h"
#include "input.h"
#include "loader.h" 
#include "arena.h"    
#include <iostream>
#include <iomanip>    
#include <cctype>     
#include <limits>     
#include <string>
#include <vector>
#include <sstream>    
#include <cmath>      
#include <fstream>
#include <cstring>    

// --- ANSI COLORS ---
const std::string ANSI_RED_BOLD = "\033[1;31m"; 
const std::string ANSI_YELLOW   = "\033[33m";   
const std::string ANSI_CYAN     = "\033[36m";   
const std::string ANSI_GRAY     = "\033[90m";   
const std::string ANSI_INVERT   = "\033[7m";
const std::string ANSI_RESET    = "\033[0m";
const std::string ANSI_CLEAR    = "\033[2J\033[1;1H";

struct HelpEntry {
	std::string command;
	std::string args;
	std::string desc;
	std::string example;
};

// The database of Knowledge
const std::vector<HelpEntry> HELP_DB = {
// --- FILE OPERATIONS ---
    {"new",    "d h w",       "Creates a new empty tensor (resizes memory).",   ":new 3 64 64"},
    {"open",   "file d h w",  "Resizes memory AND loads a binary file.",        ":open dump.bin 1 128 128"},
    {"load",   "file",        "Loads binary into CURRENT shape (no resize).",   ":load weights.bin"},
    {"export", "file",        "Saves current layer to CSV format.",             ":export layer_1.csv"},
    {"import", "file",        "Overwrites current layer from CSV file.",        ":import layer_1.csv"},
    
    // --- NAVIGATION & DIAGNOSTICS ---
    {"goto",   "l r c",       "Teleports cursor/camera to coordinates.",        ":goto 0 500 120"},
    {"health", "",            "Scans layer for NaNs, Infs, and Dead neurons.",  ":health"},
    {"hist",   "",            "Plots ASCII histogram of value distribution.",   ":hist"},
    {"stats",  "",            "Shows Min, Max, and Mean of current layer.",     ":stats"},
    {"diff",   "file",        "Loads a comparison file (Ghost) for diffing.",   ":diff checkpoint.bin"},

    // --- MATH & EDITING ---
    {"clip",   "min max",     "Clamps all values to a specific range.",         ":clip -1.0 1.0"},
    {"norm",   "",            "Normalizes layer to 0.0 - 1.0 range.",           ":norm"},
    {"zero",   "",            "Sets all values in current layer to 0.0.",       ":zero"},
    {"fill",   "val",         "Sets all values in current layer to 'val'.",     ":fill 3.14"},
    {"relu",   "",            "Applies ReLU activation (max(0, x)).",           ":relu"},
    {"sigmoid","",            "Applies Sigmoid activation (1 / 1+e^-x).",       ":sigmoid"},
    
    // --- META ---
    {"help",   "[cmd]",       "Shows this list or details for a command.",      ":help goto"},
    {"agent",  "",            "Dumps capabilities as JSON (Machine Readable).", ":agent_capabilities"}
};
void show_help_screen(bool as_json = false) {
	std::cout << ANSI_CLEAR;

	// Agent View (Machine Readable)
	if (as_json) {
		std::cout << "{\n \"tools\": [\n";
		for(size_t i = 0; i < HELP_DB.size(); i++) {
			const auto& h = HELP_DB[i];
			std::cout << "    { \"name\": \"" << h.command << "\", "
				  << "\"args\": \"" << h.args << "\", "
				  << "\"description\": \"" << h.desc << "\" }";
			if (i < HELP_DB.size() - 1) std::cout << ",";
			std::cout << "\n";
		}
		std::cout << "  ]\n}\n";
	}

	// Human View (Pretty table)
	else {
		std::cout << ANSI_INVERT << " MAXINE HELP SYSTEM " << ANSI_RESET << "\n\n";
		std::cout << std::left << std::setw(10) << "COMMAND"
			  << std::left << std::setw(15) << "ARGS"
			  << "DESCRIPTION" << "\n";
		std::cout << "---------------------------------------------------------\n";

		for (const auto& h : HELP_DB) {
			std::cout << ANSI_YELLOW << std::setw(10) << h.command << ANSI_RESET
				  << std::setw(15) << h.args
				  << h.desc << "\n";
		}
		std::cout << "\n" << ANSI_CYAN << "Type ':help [command]' for examples." << ANSI_RESET << "\n";
	}
	std::cout << "\n(Press ENTER to return)";
	std::cin.get();
}
// Headless mode: prints json and exits
void tui_print_json_help() {
	// Note: We do not use ANSI_CLEAR here because agent want raw text
	std::cout << "{\n \"program\": \"Maxine Tensor Editor\",\n";
	std::cout << "  \"version\": \"1.0.0\",\n";
	std::cout << "  \"capabilities\": [\n";

	for(size_t i = 0; i < HELP_DB.size(); i++) {
		const auto& h = HELP_DB[i];
		std::cout << "    { ";
		std::cout << "\"comannd\": \"" << h.command << "\", ";
		std::cout << "\"args\": \"" << h.args << "\", ";
		std::cout << "\"description\": \"" << h.desc << "\"";
		std::cout << " }";

		if (i < HELP_DB.size() - 1) std::cout << ",";
		std::cout << "\n";
	}
	std::cout << " ]\n}\n";
}

// --- RENDER VIEW ---
// CHANGED: int -> size_t for all coordinates
void render_view(Tensor& t, Tensor& t_ghost, size_t layer, size_t cur_row, size_t cur_col, 
                 size_t scroll_row, size_t scroll_col, bool show_ascii, bool show_diff) {
    std::cout << ANSI_CLEAR;
    
    // Viewport settings remain int because screen size is small
    const size_t VIEW_HEIGHT = 20; 
    const size_t VIEW_WIDTH = 10; 

    // Calculate bounds
    size_t rows = t.shape[1];
    size_t cols = t.shape[2];
    
    size_t end_row = scroll_row + VIEW_HEIGHT;
    if (end_row > rows) end_row = rows;
    
    size_t end_col = scroll_col + VIEW_WIDTH;
    if (end_col > cols) end_col = cols;

    size_t total_layers = t.shape[0];

    // --- HEADER ---
    std::cout << "MAXINE TENSOR EDITOR | Layer " << layer << "/" << (total_layers - 1);
    std::cout << (show_ascii ? " [ASCII]" : " [FLOAT]");
    if (show_diff) std::cout << " [DIFF MODE]";
    std::cout << "\nPos: [" << layer << ", " << cur_row << ", " << cur_col << "]";
    std::cout << "  View: " << scroll_row << "-" << end_row << " | " << scroll_col << "-" << end_col << "\n";
    std::cout << "------------------------------------------\n";

    // --- COLUMN HEADERS ---
    std::cout << "     "; 
    for(size_t x = scroll_col; x < end_col; x++) {
        if (x == cur_col) std::cout << ANSI_INVERT;
        // Cast to char for printing letters (A, B, C...)
        // Note: This will look weird if you scroll past 26 columns, but that's a UI problem, not a memory one.
        std::cout << std::setw(8) << (char)('A' + (x % 26)) << " "; 
        if (x == cur_col) std::cout << ANSI_RESET;
    }
    std::cout << "\n";

    // --- GRID LOOP ---
    for(size_t y = scroll_row; y < end_row; y++) {
        // Row Number
        if (y == cur_row) std::cout << ANSI_INVERT;
        std::cout << std::setw(3) << y << " ";
        if (y == cur_row) std::cout << ANSI_RESET;
        std::cout << "|";
        
        for(size_t x = scroll_col; x < end_col; x++) {
            float val = tensor_get(t, layer, y, x);
            float display_val = val;

            // --- DIFF LOGIC ---
            if (show_diff && t_ghost.data != nullptr) {
                float ref_val = tensor_get(t_ghost, layer, y, x);
                display_val = val - ref_val; 
            }

            bool is_selected = (y == cur_row && x == cur_col);
            
            if (is_selected) {
                std::cout << ANSI_INVERT;
            } else {
                if (!show_ascii) {
                    if (show_diff) {
                        if (display_val > 0.001f) std::cout << ANSI_RED_BOLD;
                        else if (display_val < -0.001f) std::cout << ANSI_CYAN;
                        else std::cout << ANSI_GRAY;
                    } else {
                        if (val > 100.0f)      std::cout << ANSI_RED_BOLD;
                        else if (val > 0.0f)   std::cout << ANSI_YELLOW;
                        else if (val < 0.0f)   std::cout << ANSI_CYAN;
                        else                   std::cout << ANSI_GRAY;
                    }
                }
            }
            
            if (show_ascii) {
                char c = static_cast<char>(val);
                if (std::isprint(c)) std::cout << "   '" << c << "'  ";
                else std::cout << "   .    ";
            } else {
                std::cout << std::fixed << std::setprecision(2) << std::setw(8) << display_val << " ";
            }
            
            std::cout << ANSI_RESET;
        }
        std::cout << "\n";
    }

    // --- CONTROLS ---
    std::cout << "\n[WASD] Scroll/Move | [TAB] ASCII/DIFF | [:new d h w] Resize | [:open file d h w] Smart Load\n>> "; 
}

// --- COMMAND PROCESSOR ---
// CHANGED: int& current_layer -> size_t& current_layer
void process_command(Arena* a, Tensor& t, Tensor& t_ghost, bool& ghost_loaded, 
                     size_t& current_layer,
		     size_t& cur_row, size_t& cur_col,
		     size_t& scroll_row, size_t& scroll_col,
		     const std::string& cmd_line) {

    std::stringstream ss(cmd_line);

    std::string action;
    ss >> action; 
    
    // Dimensions are now size_t
    size_t rows = t.shape[1];
    size_t cols = t.shape[2];

    // COMMAND: :new
    if (action == "new" || action == "resize") {
        size_t d, h, w; // Changed to size_t
        if (ss >> d >> h >> w) {
            if (d < 1 || h < 1 || w < 1) {
                std::cout << "\n>> Error: Dimensions must be > 0\n(Press Enter)";
                std::cin.get();
                return;
            }
            arena_reset(a);
            t = tensor_create(a, {d, h, w});
            
            if (t.data == nullptr) {
                 std::cout << "\n>> CRITICAL ERROR: Out of Memory!\n(Press Enter)";
                 t = tensor_create(a, {1,1,1}); // Recovery
            } else {
                 std::memset(t.data, 0, t.size * sizeof(float)); 
                 current_layer = 0; 
                 std::cout << "\n>> Created new Tensor: [" << d << ", " << h << ", " << w << "]\n";
            }
            std::cout << "  (Press ENTER)" << std::flush;
            std::cin.get();
            return; 
        } else {
            // THE FIX FOR SILENT FAILURE
            std::cout << "\n>> Error: Invalid arguments. Usage: :new [d] [h] [w]\n(Press Enter)";
            std::cin.get();
        }
    }
    else if(action == "goto" || action == "jump" || action == "g") {
	size_t l, r, c;
	if (ss >> l >> r >> c) {
		// 1. Boundary checks
		if (l >= t.shape[0]) l = t.shape[0] - 1;
		if (r >= t.shape[1]) r = t.shape[1] - 1;
		if (c >= t.shape[2]) c = t.shape[2] - 1;

		// 2. Update Cursor
		current_layer = l;
		cur_row = r;
		cur_col = c;
		// 3. Center the Camera (Quality of life)
		// We need access scroll/scroll_col variables.
		scroll_row = (cur_row > 10) ? cur_row - 10 : 0;
		scroll_col = (cur_col > 5)  ? cur_col - 5  : 0;

		std::cout << "\n>> Jumped to [" << l << ", " << r << ", " << c << "]\n";
	} else {
		std::cout << "\n>> Usage: :goto [layer][row][col]\n";
	}
	std::cout << "(Press Enter)";
	std::cin.get();
}

    // COMMAND: :load
    else if (action == "load") {
        std::string fname;
        if (ss >> fname) {
            std::ifstream file(fname, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                std::cout << "\n>> Error: File not found.\n(Press Enter)";
                std::cin.get();
                return;
            }
            size_t filesize = file.tellg();
            size_t expected = t.size * sizeof(float);

            if(filesize != expected) {
                std::cout << "\n>> Error: Size Mismatch!\n";
                std::cout << "   File: " << filesize << " bytes\n";
                std::cout << "   Tensor: " << expected << " bytes\n(Press Enter)";
            } else {
                file.seekg(0);
                file.read(reinterpret_cast<char*>(t.data), filesize);
                std::cout << "\n>> Loaded " << fname << "\n(Press Enter)";
            }
            file.close();
            std::cin.get();
        }
    }

    // COMMAND: :relu
    else if (action == "relu") {
        for(size_t y = 0; y < rows; y++){
            for(size_t x = 0; x < cols; x++) {
                float& val = tensor_get(t, current_layer, y, x);
                if (val < 0) val = 0; 
            }
        }
    }

    // COMMAND: :zero
    else if (action == "zero") {
        for(size_t y = 0; y < rows; y++) {
            for(size_t x = 0; x < cols; x++) {
                tensor_get(t, current_layer, y, x) = 0.0f;
            }
        }
    }

    // COMMAND: :fill
    else if (action == "fill") {
        float val;
        if (ss >> val) {
            for(size_t y = 0; y < rows; y++) {
                for(size_t x = 0; x < cols; x++) {
                    tensor_get(t, current_layer, y, x) = val;
                }
            }
        }
    }

    // COMMAND: :sigmoid
    else if (action == "sigmoid") {
        for (size_t y = 0; y < rows; y++) {
            for (size_t x = 0; x < cols; x++) {
                float& val = tensor_get(t, current_layer, y, x);
                val = 1.0f / (1.0f + std::exp(-val));
            } 
        }
    }

    // COMMAND: :stats
    else if (action == "stats") {
        float min_val = 1e9; 
        float max_val = -1e9;
        double sum = 0;
        size_t count = 0; // Changed to size_t

        for (size_t y = 0; y < rows; y++) {
            for (size_t x = 0; x < cols; x++) {
                float val = tensor_get(t, current_layer, y, x);
                if (val < min_val) min_val = val;
                if (val > max_val) max_val = val;
                sum += val;
                count++;
            }
        }

        if (count > 0) {
            float mean = sum / count;
            std::cout << "\n>> Stats: Min=" << min_val
                      << " Max=" << max_val
                      << " Mean=" << mean;
            std::cout << "  (Press ENTER to continue)" << std::flush;
            std::cin.get();
        }
    }

    // COMMAND: :export
    else if (action == "export") {
        std::string fname;
        if (!(ss >> fname)) {
            fname = "layer_" + std::to_string(current_layer) + ".csv";
        }

        std::ofstream file(fname);
        if (file.is_open()) {
            file << "# Maxine Tensor Dump\n";
            file << "# Shape: [" << t.shape[0] << ", " << t.shape[1] << ", " << t.shape[2] << "]\n";
            file << "# Layer Index: " << current_layer << "\n";
            for (size_t x = 0; x < cols; x++) {
                file << (char)('A' + (x%26)) << (x < cols - 1 ? "," : "");
            }
            file << "\n";

            for (size_t y = 0; y < rows; y++) {
                for (size_t x = 0; x < cols; x++) {
                    float val = tensor_get(t, current_layer, y, x);
                    file << val;
                    if (x < cols - 1) file << ",";
                }
                file << "\n";
            }
            file.close();

            std::cout << "\n>> Exported Layer " << current_layer << " to " << fname << "\n";
            std::cout << "  (Press ENTER)" << std::flush;
            std::cin.get();
        } 
    }

    // COMMAND: :import
    else if (action == "import") {
        std::string fname;
        if(ss >> fname) {
            std::ifstream file(fname);
            if (!file.is_open()) {
                std::cout << "\n>> Error: File not found: " << fname << "\n(Press Enter)" << std::flush;
                std::cin.get();
                return;
            }
            std::string line;
            size_t row_idx = 0; // size_t

            while (std::getline(file, line) && row_idx < rows) {
                if (line.empty() || line[0] == '#') continue;
                if (std::isalpha(line[0])) continue;

                std::stringstream lineStream(line);
                std::string cell;
                size_t col_idx = 0; // size_t

                while (std::getline(lineStream, cell, ',') && col_idx < cols) {
                    try {
                        float val = std::stof(cell); 
                        tensor_get(t, current_layer, row_idx, col_idx) = val;
                    } catch (...) {}
                    col_idx++;
                }
                row_idx++;
            }
            file.close();

            std::cout << "\n>> Imported " << fname << " into Layer " << current_layer << ".\n";
            std::cout << "  (Press ENTER)" << std::flush;
            std::cin.get();
        }
    }

    // COMMAND: :open
    else if (action == "open") {
        std::string fname;
        size_t d, h, w; // size_t
        if (ss >> fname >> d >> h >> w) {
            arena_reset(a);
            t = tensor_create(a, {d, h, w});
            if (t.data == nullptr) {
                 std::cout << "\n>> Error: OOM during open!\n(Press Enter)";
                 t = tensor_create(a, {1,1,1});
                 std::cin.get();
                 return;
            }
            std::memset(t.data, 0, t.size * sizeof(float)); 
            current_layer = 0;

            std::ifstream file(fname, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                size_t filesize = file.tellg();
                if (filesize <= t.size * sizeof(float)) {
                    file.seekg(0);
                    file.read(reinterpret_cast<char*>(t.data), filesize);
                    std::cout << "\n>> Opened " << fname << " as [" << d << "x" << h << "x" << w << "]\n(Press Enter)";             
                } else {
                    std::cout << "\n>> Error: File too big for specified shape!\n(Press Enter)";
                }
                file.close();
            } else {
                std::cout << "\n>> Error: File not found (but resized anyway).\n(Press Enter)";
            }
            std::cin.get();
        }
    }

    // COMMAND: :clip
    else if (action == "clip") {
        float min_val, max_val;
        if (ss >> min_val >> max_val) {
            for(size_t i = 0; i < t.size; i++) {
                if (t.data[i] < min_val) t.data[i] = min_val;
                if (t.data[i] > max_val) t.data[i] = max_val;
            }
            std::cout << "\n>> Clipped values between " << min_val << " and " << max_val << ".\n";
            std::cout << "  (Press ENTER)" << std::flush;
            std::cin.get();
        }
    }

    // COMMAND: :norm
    else if (action == "norm") {
        float min_v = 1e9, max_v = -1e9;
        for(size_t i = 0; i < t.size; i++) {
            if (t.data[i] < min_v) min_v = t.data[i];
            if (t.data[i] > max_v) max_v = t.data[i];
        }

        float range = max_v - min_v;
        if (range == 0) range = 1.0f;

        for(size_t i = 0; i < t.size; i++) {
            t.data[i] = (t.data[i] - min_v) / range;
        }

        std::cout << "\n>> Normalized to 0.0 - 1.0 range.\n";
        std::cout << "  (Press ENTER)" << std::flush;
        std::cin.get();
    }
    // Command: :help 
    else if (action == "help" || action == "?") {
	std::string topic;
	
	// Did they type ":help goto"?
	if (ss >> topic) {
		bool found = false;
		for(const auto& h : HELP_DB) {
			if (h.command == topic) {
				std::cout << "\n>> HELP: " << ANSI_YELLOW << ":" << h.command << ANSI_RESET << "\n";
				std::cout << "  Usage:  :" << h.command << " " << h.args << "\n";
				std::cout << "  Effect:  " << h.desc << "\n";
				std::cout << "  Example: " << ANSI_CYAN << h.example << ANSI_RESET << "\n";
				found = true;
				break;
			}
		}
		if (!found) std::cout << "\n>> Unkown command '" << topic << "'.\n";
		std::cout << "(Press Enter)";
		std::cin.get();
	} else {
		show_help_screen(false);
	}
    } else if (action == "agent_capabilities") {
	show_help_screen(true); // dumps json
    }

    // COMMAND: :diff
    else if (action == "diff") {
        std::string fname;
        if (ss >> fname) {
            std::ifstream file(fname, std::ios::binary | std::ios::ate);
            if(!file.is_open()) {
                std::cout << "\n>> Error: File not found.\n(Press Enter)";
                std::cin.get();
                return;
            }

            size_t filesize = file.tellg();
            if (filesize != t.size * sizeof(float)) {
                std::cout << "\n>> Error: Size mismatch! Diff file must match current dimensions.\n(Press Enter)";
                std::cin.get();
                return;
            } else {
                if (t_ghost.data == nullptr) {
                    t_ghost = tensor_create(a, {t.shape[0], t.shape[1], t.shape[2]});
                }
                file.seekg(0);
                file.read(reinterpret_cast<char*>(t_ghost.data), filesize);
                ghost_loaded = true;

                std::cout << "\n>> Loaded Comparison file: " << fname << "\n";
                std::cout << "    (Press TAB to toggle Diff View)\n(Press Enter)";
            }
            file.close();
            std::cin.get();
        }
    }
    else if (action == "health" || action == "scan") {
	size_t nan_count = 0;
	size_t inf_count = 0;
	size_t zero_count = 0;
	size_t tiny_count = 0;

	float max_val = -1e9f;
	float min_val = 1e9f;

	// Threshhold's for warnings
	const float EXPLOSION_THRESHOLD = 100.0f; // Warn if > 100
	const float VANISHING_THRESHOLD = 1e-7f;  // Warning if < 0.0000001 (but not 0)
	
		// SCAN LOOP
		for (size_t y = 0; y < rows; y++) {
			for (size_t x = 0; x < cols; x++) {
				float val = tensor_get(t, current_layer, y, x);

				if (std::isnan(val)) nan_count++;
				if (std::isinf(val)) inf_count++;
				if (val == 0.0f) zero_count++;
				if (val != 0.0f && std::abs(val) < VANISHING_THRESHOLD) {
					tiny_count++;
				}
				if (val > max_val) max_val = val;
				if (val < min_val) min_val = val;
			}
		}

		size_t total_cells = rows * cols;
		float zero_percent = (float)zero_count / total_cells * 100.0f;

		// Report card
		std::cout << "\n>> HEALTH REPORT (Layer " << current_layer << ")\n";
		std::cout << "-------------------------------------------------\n";

		// 1. Critical Checks
		if (nan_count > 0) std::cout << ANSI_RED_BOLD << "[FAIL] Found " << nan_count << "NaN!\n" << ANSI_RESET;
		else std::cout << ANSI_CYAN << "[PASS] Values within normal range.\n";
		if (inf_count > 0) std::cout << ANSI_RED_BOLD << "[FAIL] Found " << inf_count << "Infs!\n" << ANSI_RESET;
		else std::cout << ANSI_CYAN << "[PASS] Values within normal range.\n";

		// 2. Value checks
		if (max_val > EXPLOSION_THRESHOLD || min_val < -EXPLOSION_THRESHOLD)
			std::cout << ANSI_YELLOW << "[WARN] Large value detected! (Max: " << max_val <<")\n" << ANSI_RESET;
		else std::cout << "[PASS] Values within normal range.\n";

		// 3. Sparsity Check
		if (zero_percent > 90.0f) std::cout << ANSI_YELLOW << "[WARN] High Sparsity: " << std::fixed << std::setprecision(1) << zero_percent << "% Zero's (Dead Layer?)\n" << ANSI_RESET;
		else std::cout << "[INFO] Sparsity: " << std::fixed << std::setprecision(1) << zero_percent << "%\n";

		// 4. Vanishing Gradient Check
		if (tiny_count > 0)
			std::cout << ANSI_YELLOW << "[WARN] Vanishing Gradients: " << tiny_count << " values are extremely small (< 1e-7)\n" << ANSI_RESET;


		std::cout << "----------------------------------------------------\n";
		std::cout << "  (Press Enter)";
		std::cin.get();
	}

// ... inside process_command ...

    // COMMAND: :hist
    // Effect: Draws an ASCII Histogram of the data distribution
    else if (action == "hist") {
        // 1. Find Min/Max
        float min_v = 1e9f;
        float max_v = -1e9f;
        for(size_t y=0; y<rows; y++) {
            for(size_t x=0; x<cols; x++) {
                float v = tensor_get(t, current_layer, y, x);
                if (v < min_v) min_v = v;
                if (v > max_v) max_v = v;
            }
        }

        if (min_v >= max_v) {
             std::cout << "\n>> Histogram: Flat value (" << min_v << ")\n(Press Enter)";
             std::cin.get();
             // We can't plot a flat line, so exit
             return; 
        }

        // 2. Create Buckets
        const int BINS = 10;
        int counts[BINS] = {0};
        float range = max_v - min_v;
        float step = range / BINS;

        for(size_t y=0; y<rows; y++) {
            for(size_t x=0; x<cols; x++) {
                float v = tensor_get(t, current_layer, y, x);
                int bucket = (int)((v - min_v) / step);
                if (bucket >= BINS) bucket = BINS - 1; // Include max_v in last bin
                counts[bucket]++;
            }
        }

        // 3. Draw the Chart
        std::cout << "\n>> DISTRIBUTION (Layer " << current_layer << ")\n";
        std::cout << "------------------------------------------------\n";
        
        // Find max count to normalize bar height
        int max_count = 0;
        for(int i=0; i<BINS; i++) if(counts[i] > max_count) max_count = counts[i];

        for(int i=0; i<BINS; i++) {
            float bin_start = min_v + (i * step);
            float bin_end = min_v + ((i+1) * step);
            
            // Normalize bar length to max 30 characters
            int bar_len = 0;
            if (max_count > 0) {
                bar_len = (int)((float)counts[i] / max_count * 30.0f);
            }
            
            // Print Range (e.g., "-0.50 .. -0.20 |")
            std::cout << std::fixed << std::setprecision(2) << std::setw(6) << bin_start 
                      << " .. " << std::setw(6) << bin_end << " | ";
            
            // Print Bar
            std::cout << ANSI_YELLOW; 
            for(int k=0; k<bar_len; k++) std::cout << "#";
            std::cout << ANSI_RESET;
            
            // Print Count
            std::cout << " (" << counts[i] << ")\n";
        }
        std::cout << "------------------------------------------------\n(Press Enter)";
        std::cin.get();
    }
    
    // CATCH-ALL FOR TYPOS
    else {
        std::cout << "\n>> Error: Unknown command '" << action << "'\n";
        std::cout << "   (Press ENTER)" << std::flush;
        std::cin.get();
    }
} 



// --- MAIN LOOP ---
void tui_loop(Arena* a, Tensor& t, const std::string& filename) {
    // CHANGED: int -> size_t
    size_t cur_layer = 0;
    size_t cur_row = 0;
    size_t cur_col = 0;
    
    size_t scroll_row = 0;
    size_t scroll_col = 0;

    const size_t VIEW_HEIGHT = 20; 
    const size_t VIEW_WIDTH = 10; 
    
    bool show_ascii = false; 
    bool running = true;
    bool show_diff = false; 
    
    Tensor t_ghost = {}; 
    bool ghost_loaded = false;

    enable_raw_mode();

    while(running) {
        // CHANGED: int -> size_t
        size_t max_layers = t.shape[0];
        size_t max_rows = t.shape[1];
        size_t max_cols = t.shape[2];

        // --- Camera Logic ---
        if (cur_row < scroll_row) scroll_row = cur_row;
        if (cur_row >= scroll_row + VIEW_HEIGHT) scroll_row = cur_row - VIEW_HEIGHT + 1;
        if (cur_col < scroll_col) scroll_col = cur_col;
        if (cur_col >= scroll_col + VIEW_WIDTH) scroll_col = cur_col - VIEW_WIDTH + 1;

        render_view(t, t_ghost, cur_layer, cur_row, cur_col, 
                    scroll_row, scroll_col, show_ascii, show_diff);
        
        char cmd = get_keypress();

        switch(cmd) {
            case 'x': running = false; break;
            
            case ':': 
            {
                disable_raw_mode();
                std::cout << "\n>> Command: :"; 
                std::string cmd_input;
                std::getline(std::cin, cmd_input);

                if (!cmd_input.empty()) {
                    process_command(a, t, t_ghost, ghost_loaded, cur_layer,
				    cur_row, cur_col, scroll_row, scroll_col,
				    cmd_input);
                    
                    if (cur_row >= t.shape[1]) cur_row = t.shape[1] - 1;
                    if (cur_col >= t.shape[2]) cur_col = t.shape[2] - 1;
                }
                enable_raw_mode();
                break;
            }
            
            case 'w': if (cur_row > 0) cur_row--; break;
            case 's': if (cur_row < max_rows - 1) cur_row++; break;
            case 'a': if (cur_col > 0) cur_col--; break;
            case 'd': if (cur_col < max_cols - 1) cur_col++; break;
            
            case 'e': if (cur_layer < max_layers - 1) cur_layer++; break;
            case 'q': if (cur_layer > 0) cur_layer--; break;

            case '\t': 
                if (!show_ascii && !show_diff) show_ascii = true;
                else if (show_ascii) { show_ascii = false; show_diff = true; } 
                else { show_diff = false; }
                break;

            case 'S': 
            {
                disable_raw_mode();
                save_binary_tensor(t, filename);
                std::cout << "Press any key to return...";
                getchar(); 
                enable_raw_mode();
                break;
            }

            case '\r':
            case '\n': 
            {
                disable_raw_mode();
                std::cout << "\n>> Enter new value: ";
                float new_val;
                if (std::cin >> new_val) {
                    tensor_get(t, cur_layer, cur_row, cur_col) = new_val;
                } else {
                    std::cin.clear(); 
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
                }
                enable_raw_mode();
                break;
            }
        }
    }
    
    disable_raw_mode();
}
