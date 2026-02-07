import numpy as np

# Define Shape: [3 Layers, 8 Rows, 8 Cols]
# We make it slightly bigger to test the TUI rendering.
D, H, W = 3, 8, 8

# Create a 3D grid representing coordinates
z, y, x = np.ogrid[:D, :H, :W]

# Generate a pattern: values increase based on their X, Y, and Z position
# this creates a nice gradient effect across layers and rows.
data = (z * 100) + (y * 10) + x

# IMPORTANT: Convert to float32. Our C++ engine expects 4-byte floats.
data = data.astype(np.float32)

print(f"Generate Data Shape: {data.shape}")
print(f"Example - Layer 0 (Top left): {data[0,0,0]}")
print(f"Example - Layer 1 (Top left): {data[1,0,0]}")
print(f"Example - Layer 2 (Bottom Right): {data[2,7,7]}")

# SAVE AS RAW BINARY.
#.tofile() writes the raw bytes to disk without any headers or metadata.
filename = "gradient_3x8x8.bin"
data.tofile(filename)
print(f"\n>> Saved raw binary to: {filename}")
print(f">> Expected file size {D*H*W * 4} bytes")
