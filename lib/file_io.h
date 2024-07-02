#ifndef FILE_IO_H
#define FILE_IO_H

#include <sys/types.h>
#include "../lib/editor.h"

void editorOpen(char *fileName);
void editorInsertRow(int at, char *s, size_t len);
void editorUpdateRow(erow *row);
int editorRowCxToRx(erow *row, int cx);
int editorRowRxToCx(erow *row, int rx);
void editorRowInsertChar(erow *row, int at, int c);
void editorInsertChar(int c);
char *editorRowsToString(int *buflen);
void editorSave();
void editorRowDelChar(erow *row, int at);
void editorFreeRow(erow *row);
void editorDelRow(int at);
void editorRowAppendString(erow *row, char *s, size_t len);

#endif