#include "../lib/editor.h"
#include "../lib/terminal.h"
#include "../lib/syntax.h"
#include <stdlib.h>

char *C_HL_extensions[] = {".c", ".h", ".cpp", NULL};
char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", "NULL|", NULL
};

struct editorSyntax HDLB[] = {
    // C FILE TYPE
    {  
        "c", // filetype
        C_HL_extensions, //filematch
        C_HL_keywords, //keywords
        "//", //single_line_comment_start
        "/*", //multiline_comment_start
        "*/", //multiline_comment_end
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS, //flags

    },
};

unsigned int HLDB_ENTRIES = sizeof(HDLB) / sizeof(HDLB[0]);

/**
 * @brief Initializes the global editor variable attributes and editor's screensize based on terminal dimensions by calling getWindowSize.
 * @param None
 * @return None
*/
void initEditor()
{
    editor.cx = 0; 
    editor.cy = 0;
    editor.rx = 0;
    editor.rowoff = 0;
    editor.coloff = 0;
    editor.numrows = 0;
    editor.row = NULL;
    editor.fileName = NULL;
    editor.statusmsg[0] = '\0';
    editor.statusmsg_time = 0;
    editor.unsaved = 0;
    editor.syntax = NULL;

    if (getWindowSize(&editor.screenRows, &editor.screenCols) == -1) die("getWindowSize");
    editor.screenRows -= 2;
}