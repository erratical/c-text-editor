#include "../lib/file_io.h"
#include "../lib/editor.h"
#include "../lib/terminal.h"
#include "../lib/const.h"
#include "../lib/output.h"
#include "../lib/input.h"
#include "../lib/syntax.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

/**
 * @brief Manages file text opening by reading file and calling editorAppendRow, skipping any escape characters.
 * @param fileName The file to be read. If file not provided, simply display welcome screen.
*/
void editorOpen(char *fileName)
{
    free(editor.fileName);
    editor.fileName = strdup(fileName);

    editorSelectSyntaxHighlight();

    FILE *fp = fopen(fileName, "r");
    if (!fp) die("fopen");
    
    char *line = NULL;
    ssize_t linelen;
    size_t linecap = 0;

    while ((linelen = getline(&line, &linecap, fp)) != -1)
    {
        while (linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
            linelen--;

        editorInsertRow(editor.numrows, line, linelen);
    }
    free(line);
    fclose(fp);

    editor.unsaved = 0;
}

/**
 * @brief Reallocates memory for editor.row to increase `erow` by 1, 
 * @brief and sets editor row attributes `size` and `chars` to be the read string/line.
 * @param s The line read from the file that will be copied to `editor.row[index].chars`
 * @param len Length of line.
*/
void editorInsertRow(int at, char *s, size_t len)
{
    if (at < 0 || at > editor.numrows) return;

    editor.row = realloc(editor.row, sizeof(erow) * (editor.numrows + 1));
    memmove(&editor.row[at + 1], &editor.row[at], sizeof(erow) * (editor.numrows - at));
    for (int j = at + 1; j <= editor.numrows; j++) editor.row[j].idx++;

    editor.row[at].idx = at;

    editor.row[at].size = len;
    editor.row[at].chars = malloc(len + 1);
    memcpy(editor.row[at].chars, s, len);
    editor.row[at].chars[len] = '\0';

    editor.row[at].rsize = 0;
    editor.row[at].render = NULL;
    editor.row[at].hl = NULL;
    editor.row[at].hl_open_comment = 0;
    editorUpdateRow(&editor.row[at]);

    editor.numrows++;
    editor.unsaved++;
}

/**
 * @brief Properly renders the row, and counts tab spaces.
 * @param row The `erow *` that converts `row->chars` '\t' into 8 spaces into `row->render`.
*/
void editorUpdateRow(erow *row)
{
    int tabs = 0;

    // count tabs in a row
    for (int i = 0; i < row->size; i++)
        if (row->chars[i] == '\t') tabs++;
    
    free(row->render);
    row->render = malloc(row->size + tabs*(TAB_STOP - 1) + 1);

    int idx = 0;

    // copy string
    for (int j = 0; j < row->size; j++)
    {  
        if (row->chars[j] == '\t')
        {
            row->render[idx++] = ' ';
            while (idx % TAB_STOP != 0) row->render[idx++] = ' ';
        }
        else 
        {
            row->render[idx++] = row->chars[j];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;

    editorUpdateSyntax(row);
}

/**
 * @brief Handles cursor action with tabs.
*/
int editorRowCxToRx(erow *row, int cx) // teleports to skip tabs
{
    int rx = 0;
    
    for (int j = 0; j < cx; j++)
    {
        if (row->chars[j] == '\t')
            rx += (TAB_STOP-1) - (rx % TAB_STOP);
        rx++;
    }

    return rx;
}

int editorRowRxToCx(erow *row, int rx)
{
    int cur_rx = 0;
    int cx;

    for (cx = 0; cx < row->size; cx++)
    {
        if (row->chars[cx] == '\t')
            cur_rx += (TAB_STOP - 1) - (cur_rx % TAB_STOP);
        cur_rx++;

        if (cur_rx > rx) return cx;
    }

    return cx;
}

/**
 * @brief Reallocates a row and moves the memory to make space for `int c`.
 * @param row (type `erow *`) Pointer to the row being modified.
 * @param at (type `int`) The index in `row->raws` to be inserted.
 * @param c (type `int`) The character to be inserted.
*/
void editorRowInsertChar(erow *row, int at, int c)
{
    if (at < 0 || at > row->size) at = row->size;

    row->chars = realloc(row->chars, row->size + 2);

    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);

    row->size++;

    row->chars[at] = c;

    editorUpdateRow(row);
    editor.unsaved++;
}

/**
 * @brief Converts all of text rows into a singular string each separated by `\n`.
 * @param buflen (type `int *`) Points to the length of the entire file.
 * @returns `buf` - (type `char *`) All of the rows converted to string form.
*/
char *editorRowsToString(int *buflen) 
{
    int totalLen = 0;
    
    for (int i = 0; i < editor.numrows; i++)
    {
        totalLen += editor.row[i].size + 1;
    }

    *buflen = totalLen;

    char *buf = (char *)malloc(totalLen * sizeof(char));
    char *p = buf;

    // MODIFY CODE: String appending.
    for (int j = 0; j < editor.numrows; j++)
    {
        memcpy(p, editor.row[j].chars, editor.row[j].size);
        p += editor.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

/**
 * @brief Gets the string from `editorRowsToString` and saves it into a file.
 * @note The normal way of truncating a file would be to add a `O_TRUNC` flag to `open()`
 * @note However, the approach of using `ftruncate` would be safer, since it simply "crops" out
 * @note additional data (if previous file size is larger than length) or adds more bytes 
 * @note (if previous file size is smaller than length). When using `open()` with a `O_TRUNC`
 * @note flag, the whole file is erased first before writing. This is dangerous if the `write`
 * @note call fails, you'd end up with an empty file.
*/
void editorSave()
{
    if (editor.fileName == NULL) 
    {
        editor.fileName = editorPrompt("Save as (Ctrl-X to cancel): %s", NULL);
        if (editor.fileName == NULL) 
        {
            editorSetStatusMessage("Save aborted.");
            return;
        }
        editorSelectSyntaxHighlight();
    }

    int len;
    char *buf = editorRowsToString(&len);

    // `O_CREAT` tells `open()` to create a new file if file does not exist.
    // `O_RDWR` opens the file for reading and writing.
    // `0644` are the permissions for the text file. Owner gets read and write, everyone else read only.
    int fd = open(editor.fileName, O_RDWR | O_CREAT, 0644);

    if (fd != -1)
    {
    // Set the file size of `fd` to `len`
        if (ftruncate(fd, len) != -1)
        {
            if (write(fd, buf, len) == len)
            {
                close(fd);
                free(buf);

                editor.unsaved = 0;
                editorSetStatusMessage("\x1b[32;1mSave success.\x1b[m \x1b[1m%d bytes\x1b[m written to disk.", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editorSetStatusMessage("Save failed. I/O error: %s", strerror(errno));
}

/**
 * @brief Similar logic to `editorRowInsertChar`, but no need for reallocation.
 * @param row (type `erow *`) Pointer to the row to be modified.
 * @param at (type `int`) Index of the character to be deleted.
*/
void editorRowDelChar(erow *row, int at)
{
    if (at < 0 || at >= row->size) return;
    
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);

    row->size--;

    editorUpdateRow(row);

    editor.unsaved++;
}

/**
 * @brief Frees `row->render` and `row->chars`.
 * @param row (type `erow *`) Pointer to the row to be freed.
*/
void editorFreeRow(erow *row)
{
    free(row->render);
    free(row->chars);
    free(row->hl);
}

/**
 * @brief Frees the row at a given index and moves all the rows that came after it.
 * @param at (type `int`) Index of the row to be deleted.
*/
void editorDelRow(int at)
{
    if (at < 0 || at >= editor.numrows) return;
    editorFreeRow(&editor.row[at]);

    memmove(&editor.row[at], &editor.row[at + 1], sizeof(erow) * (editor.numrows - at - 1));
    for (int j = at; j < editor.numrows - 1; j++) editor.row[j].idx--;

    editor.numrows--;
    editor.unsaved++;
}

/**
 * @brief Reallocates `row->chars` and appends string to the row.
 * @param row (type `erow *`) The row to be appended on.
 * @param s (type `char *`) The string to be appended.
 * @param len (type `size_t`) Length of the string.
*/
void editorRowAppendString(erow *row, char *s, size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);

    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);

    editor.unsaved++;
}
