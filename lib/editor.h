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
    int cx, cy; // cursor position in terminal
    int rx; // rendered position in text 
    int rowoff; // row offset (vertical scroll)
    int coloff; // column offset (horizontal scroll)
    int screenRows; 
    int screenCols;
    int numrows; // total number of rows
    erow *row; // editor row
    char *fileName; 
    char statusmsg[80]; // status bar message
    time_t statusmsg_time; // timeout 
    struct editorSyntax *syntax; 
    struct termios originalTermios; // terminal attributes
    int unsaved; // modified flag
};

extern struct editorSyntax HDLB[]; // database for syntaxing
extern unsigned int HLDB_ENTRIES; // size of HDLB

struct editorConfig editor; // `GLOBAL`

void initEditor();

#endif