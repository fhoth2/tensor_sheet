#pragma once
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>

// Global to store original settins
static struct termios original_termios;

inline void enable_raw_mode() {
	tcgetattr(STDIN_FILENO, &original_termios);
	struct termios raw = original_termios;
	// Disable cononical mode (line buffering) and echo
	raw.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

inline void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

// Linux magic to turn off "line Buffering" so we catch keys instantly
inline char get_keypress() {
	char ch;
	// Read 1 byte. read() is safer than getchar() in raw mode
	if (read(STDIN_FILENO, &ch, 1) == -1) return 0;
	return ch;
}
