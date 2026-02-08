cmake_minimum_required(VERSION 3.18)

# --- 1. FORCE COMPILER (Keep this, it worked) ---
if(EXISTS "/usr/bin/nvcc")
    set(CMAKE_CUDA_COMPILER /usr/bin/nvcc)
elseif(EXISTS "/usr/local/cuda/bin/nvcc")
    set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
endif()

project(TensorSheet LANGUAGES CXX)

# --- 2. Enable CUDA Language ---
include(CheckLanguage)
check_language(CUDA)

if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    message(STATUS ">> CUDA Compiler Found: ${CMAKE_CUDA_COMPILER}")
    add_compile_definitions(ENABLE_CUDA)
    set(CMAKE_CUDA_ARCHITECTURES native)
    set(CUDA_SOURCES src/gpu_ops.cu)
    
    # --- 3. MANUAL LIBRARY LINKING (The Fix) ---
    # Instead of asking for "CUDA::cudart", we find the file ourselves.
    # You verified this path exists in your 'ls' output.
    find_library(CUDART_LIB cudart 
                 PATHS /usr/local/cuda/lib64 
                 NO_DEFAULT_PATH)
                 
    message(STATUS ">> Found CUDART Library: ${CUDART_LIB}")

else()
    message(STATUS ">> Falling back to CPU/SSE mode.")
    set(CUDA_SOURCES "")
endif()

# --- 4. Build Setup ---
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -mavx2 -mfma -Wall -Wextra)
endif()

add_executable(maxine_tensor
    src/main.cpp
    src/arena.cpp
    src/tensor.cpp
    src/tui.cpp
    src/loader.cpp
    ${CUDA_SOURCES}
)

# --- 5. Manual Include & Link ---
target_include_directories(maxine_tensor PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    /usr/local/cuda/include  # Manually add the CUDA headers
)

if(CMAKE_CUDA_COMPILER)
    # Link the file we found in step 3
    target_link_libraries(maxine_tensor PRIVATE ${CUDART_LIB})
endif()
