#include "../lib/editor.h"
#include "../lib/edit_op.h"
#include "../lib/file_io.h"
#include "../lib/input.h"
#include "../lib/syntax.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @brief Inserts character in editor by calling `editorRowInsertChar` in cursor's x position.
 * @param c (type `int`) Character to be inserted.
*/
void editorInsertChar(int c)
{
    if (editor.cy == editor.numrows) editorInsertRow(editor.numrows, "", 0);

    editorRowInsertChar(&editor.row[editor.cy], editor.cx, c);
    editor.cx++;
}

/**
 * @brief Calls `editorRowDelChar` if `cx` is not at start of line, else appends string and deletes row.
*/
void editorDelChar()
{
    if (editor.cy == editor.numrows) return;
    if (editor.cy == 0 & editor.cx == 0) return;

    erow *row = &editor.row[editor.cy];

    if (editor.cx > 0)
    {
        editorRowDelChar(row, editor.cx - 1);
        editor.cx--;
    }
    else
    {
        // go to end of prev row
        editor.cx = editor.row[editor.cy - 1].size;
        editorRowAppendString(&editor.row[editor.cy - 1], row->chars, row->size);
        editorDelRow(editor.cy);
        editor.cy--;
    }
}

/**
 * @brief On ENTER keypress, creates a new row if `cx` is at start of line,
 * @brief Else, creates a new row and splits the row where `cx` was.
*/
void editorInsertNewLine()
{
    if (editor.cx == 0)
    {
        editorInsertRow(editor.cy, "", 0);
    }
    else
    {
        erow *row = &editor.row[editor.cy];
        editorInsertRow(editor.cy + 1, &row->chars[editor.cx], row->size - editor.cx);
        row = &editor.row[editor.cy];
        row->size = editor.cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    editor.cy++;
    editor.cx = 0;
}

/**
 * @brief Enabled with Ctrl-F, it saves cursor position and calls `editorPrompt` to
 * @brief which passes `editorFindCallback` as its callback function for incremental search.
*/
void editorFind()
{
    int saved_cx = editor.cx;
    int saved_cy = editor.cy;
    int saved_coloff = editor.coloff;
    int saved_rowoff = editor.rowoff;

    char *query = editorPrompt("Search: %s (Ctrl-X/Arrows/Enter)", editorFindCallback);
    if (query) 
    {
        free(query);
    }
    else 
    {
        editor.cx = saved_cx;
        editor.cy = saved_cy;
        editor.coloff = saved_coloff;
        editor.rowoff = saved_rowoff;
    }
}

/**
 * @brief Used in `editorFindCallback` to find previous match. Reverses a string.
 * @param str String to reverse.
*/
void reverse_string(char* str) {
    if (str == NULL) return;
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[(len - 1) - i];
        str[(len - 1) - i] = temp;
    }
}

/**
 * @brief Main function for finding a substring within the document. 
 * @brief Uses `strstr` to find an instance of substring within the line and saves
 * @brief previous match with `static` variables.
 * @param query Substring to be matched within document.
 * @param key Last key pressed.
*/
void editorFindCallback(char *query, int key)
{
    static int last_match = -1; // index of the row last matched on, or -1 if no last match
    static int direction = 1; // 1: search forward, -1: search backward
    static int in_line_position = 0; // position within the current line
    static int searching_inline = 0; // flag to indicate if we're searching within the same line

    static int saved_hl_line;
    static char* saved_hl = NULL;

    if (saved_hl)
    {
        memcpy(editor.row[saved_hl_line].hl, saved_hl, editor.row[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == CTRL_KEY('x')) 
    {
        last_match = -1;
        direction = 1;
        in_line_position = 0;
        searching_inline = 0;
        return;
    }
    else if (key == ARROW_RIGHT || key == ARROW_DOWN)
    {   
        direction = 1;
        searching_inline = 1;
    }
    else if (key == ARROW_LEFT || key == ARROW_UP)
    {
        direction = -1;
        searching_inline = 1;
    }
    else // when pressing any key, such as when searching
    {
        last_match = -1;
        direction = 1;
        in_line_position = 0;
        searching_inline = 0;
    }

    if (last_match == -1) {
        direction = 1;
        in_line_position = 0;
        searching_inline = 0;
    }
    
    int currentRow = last_match;
    int i;

    // Actual Search
    for (i = 0; i < editor.numrows; i++)
    {
        if (!searching_inline) {
            currentRow += direction;
            
            // Wrap around
            if (currentRow == -1) 
            {
                currentRow = editor.numrows - 1;
            }
            else if (currentRow == editor.numrows) currentRow = 0;
        }

        erow *row = &editor.row[currentRow];
        char *line_start = row->render;
        char *match = line_start;

        if (searching_inline) {
            // If searching inline, adjust the starting position
            if (direction == 1) {
                match += in_line_position + 1;  // Start after the current match
            } else {
                // Backward search
                char reversed_line[row->rsize + 1];
                char reversed_query[strlen(query) + 1];

                strcpy(reversed_line, line_start);
                strcpy(reversed_query, query);

                reverse_string(reversed_line);
                reverse_string(reversed_query);

                char *reverse_match = strstr(&reversed_line[row->rsize - in_line_position - 1], reversed_query);
                
                if (reverse_match) {
                match = line_start + (row->rsize - (reverse_match - reversed_line) - strlen(query));
                } 
                else match = NULL;

                if (match == NULL) {
                    // If no match found before current position move to previous
                    direction = -1;
                    searching_inline = 0;
                    continue;
                }
            }
        }

        match = strstr(match, query);

        if (match)
        {
            last_match = currentRow;
            editor.cy = currentRow;

            // index of where first char is
            editor.cx = editorRowRxToCx(row, match - line_start);
            editor.rowoff = editor.numrows;
            in_line_position = match - line_start;
            searching_inline = 0;  // Reset for next search

            saved_hl_line = currentRow;
            saved_hl = malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[in_line_position], HL_MATCH, strlen(query));

            break;
        }
        else if (searching_inline)
        {
            // If searching inline and no match found, move to next/previous line
            searching_inline = 0;
            continue;
        }
    }

    // If all rows are searched and found no match, reset
    if (i == editor.numrows)
    {
        last_match = -1;
        in_line_position = 0;
        searching_inline = 0;
    }
}
