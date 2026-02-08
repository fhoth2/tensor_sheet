#pragma once
#include "tensor.h"
#include <string>
#include <initializer_list>

// Loads raw binary float data from a file directly into the Arena.
// you must provide the expected shape, as raw files don't store it.
Tensor load_binary_tensor(Arena* a, const std::string& filename, std::initializer_list<int> shape);

void save_binary_tensor(Tensor& t, const std::string& filename);

