#ifndef TERMINAL_H
#define TERMINAL_H

#include "../lib/editor.h"

void disableRaw();
void enableRaw();
void die(const char* str);
int editorReadKey();
int getWindowSize(int* rows, int* cols);
int getCursorPosition(int* rows, int* cols);

#endif