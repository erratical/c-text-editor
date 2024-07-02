#ifndef OUTPUT_H
#define OUTPUT_H

#include "../lib/editor.h"
#include "../lib/buffer.h"
#include <stdarg.h>


void editorRefreshScreen();
void editorDrawRows(struct abuf *ab);
void editorCenteredText(const char *s, struct abuf *ab);
void editorScroll();
void editorDrawStatusBar(struct abuf *ab);
void editorSetStatusMessage(const char *fmt, ...);
void editorDrawMessageBar(struct abuf *ab);

#endif