#pragma once
#include <string>
#include "tensor.h"
#include "arena.h"

// The main interactive loop 
void tui_loop(Arena* a, Tensor& t, const std::string& filename);

// The headless helper dump
void tui_print_json_help();
