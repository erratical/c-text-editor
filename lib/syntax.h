#ifndef SYNTAX_H
#define SYNTAX_H

#include "../lib/const.h"
#include <stdlib.h>

enum editorHighlight
{
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

typedef struct erow erow;

void editorUpdateSyntax(erow *row);
int editorSyntaxToColor(int hl);
int isSeparator(int c);
void editorSelectSyntaxHighlight();

#endif