#ifndef EDITOR_H
#define EDITOR_H

#include "../lib/syntax.h"
#include <termios.h>
#include <time.h>

typedef struct erow // editor row
{
    int idx;
    int size;
    int rsize;
    char* chars;
    char* render;
    unsigned char *hl;
    int hl_open_comment;
} erow;

struct editorSyntax 
{
    char *fileType;
    char **fileMatch;
    char **keywords;
    char *single_line_comment_start;
    char *multiline_comment_start;
    char *multiline_comment_end;
    int flags;
};

struct editorConfig 
{
    int cx, cy; // CURSOR POSITION
    int rx; // RENDERED POSITION (tabs)
    int rowoff; // ROW OFFSET (vertical scroll)
    int coloff; // COL OFFSET (horizontal scroll)
    int screenRows; 
    int screenCols;
    int numrows; 
    erow *row; // editor row
    char *fileName;
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;
    struct termios originalTermios;
    int unsaved;
};

extern struct editorSyntax HDLB[];
extern unsigned int HLDB_ENTRIES;

struct editorConfig editor; // `GLOBAL`

void initEditor();

#endif