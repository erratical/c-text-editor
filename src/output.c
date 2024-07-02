#include "../lib/output.h"
#include "../lib/buffer.h"
#include "../lib/const.h"
#include "../lib/editor.h"
#include "../lib/file_io.h"
#include "../lib/syntax.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/**
 * @brief Uses escape sequences to refresh screen.
 * @param None
 * @return None
*/
void editorRefreshScreen()
{
    editorScroll();
    
    struct abuf ab = ABUF_INIT;

    // Hide cursor
    abAppend(&ab, "\x1b[?25l", 6);

    // Move cursor to top left
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buf[32];

    // if file exists, offset cursor to make space for line numbers
    if (editor.numrows) 
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (editor.cy - editor.rowoff) + 1, (editor.rx - editor.coloff) + 1 + LN_OFFSET);
    else
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (editor.cy - editor.rowoff) + 1, (editor.rx - editor.coloff) + 1);

    abAppend(&ab, buf, strlen(buf));

    // Show cursor
    abAppend(&ab, "\x1b[?25h", 6);

    // Executes command
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/**
 * @brief Main function for outputting file content and welcome messsage.
 * @param ab Type `struct abuf *` 
 * @return None
*/
void editorDrawRows(struct abuf *ab)
{
    for (int y = 0; y < editor.screenRows; y++)
    {
        int fileRow = y + editor.rowoff;
        if (fileRow >= editor.numrows)
        {
            // NO FILE INPUT
            if (editor.numrows == 0 && y == (editor.screenRows / 3))
            {
            // VERSION TEXT
                char welcome[50];
                snprintf(welcome, sizeof(welcome),"Text editor -- version %s", VERSION);
                editorCenteredText(welcome, ab);
            } 
            // AUTHOR TEXT
            else if (editor.numrows == 0 && y == (editor.screenRows / 3) + 1)
            {
                editorCenteredText("github.com/erratical", ab);
            } 
            else
            {
            abAppend(ab, "\x1b[1;92m~\x1b[m", 12);
            }
        }
        else 
        {   
            int fileLineLen = snprintf(NULL, 0, "\x1b[1;32m[%.3d]\x1b[m ", fileRow);
            char fileLine[fileLineLen + 1];
            snprintf(fileLine, sizeof(fileLine), "\x1b[1;32m[%.3d]\x1b[m ", fileRow);

            abAppend(ab, fileLine, fileLineLen + 6);

            int lineLen = editor.row[fileRow].rsize - editor.coloff;

            char *line = &editor.row[fileRow].render[editor.coloff];
            unsigned char *hl = &editor.row[fileRow].hl[editor.coloff];
            int currentColor = -1;

            if (lineLen + fileLineLen > editor.screenCols) lineLen = editor.screenCols - fileLineLen + LN_OFFSET + 4;

            for (int ch = 0; ch < lineLen; ch++)
            {
                if (iscntrl(line[ch]))
                {
                    char sym = (line[ch] <= 26) ? '@' + line[ch] : '?';
                    abAppend(ab, "\x1b[7m", 4);
                    abAppend(ab, &sym, 1);
                    abAppend(ab, "\x1b[m", 3);

                    if (currentColor != -1)
                    {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", currentColor);
                        abAppend(ab, buf, clen);
                    }
                }
                else if (hl[ch] == HL_NORMAL)
                {
                    if (currentColor != -1)
                    {
                        abAppend(ab, "\x1b[m", 3);
                        currentColor = -1;
                    }
                    abAppend(ab, &line[ch], 1);
                }
                else 
                {
                    int color = editorSyntaxToColor(hl[ch]);
                    if (color != currentColor)
                    {
                        currentColor = color;
                        char buf[16];
                        int colorLen = snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", color);
                        abAppend(ab, buf, colorLen);
                    }
                    abAppend(ab, &line[ch], 1);
                }
            }
            abAppend(ab, "\x1b[m", 3);
        }
        // Clear line to the right of cursor
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

/**
 * @brief Takes in a string and the buffer structure and adds padding to center in terminal.
 * @param s String to center and append to buffer.
 * @param ab The pointer to the `struct abuf` to append to.
 * @return None
*/
void editorCenteredText(const char* s, struct abuf *ab)
{
    int len = strlen(s);

    if (len > editor.screenCols) len = editor.screenCols;

    int padding = (editor.screenCols - len) / 2;

    if (padding)
    {
        abAppend(ab, "\x1b[1;92m~\x1b[m", 12);
        padding--;
    }
    while (padding--) abAppend(ab, " ", 1);

    abAppend(ab, s, len);

}

/**
 * @brief Manages editor attributes `rx`, `rowoff`, `coloff`
 * @note `rx` Handles tab stop cursor positioning.
 * @note `rowoff` Stores the offset of the cursor from the maximum rendered `screenRows`
 * @note `coloff` Stores the offset of the cursor from the maximum rendered `screenCols`
*/
void editorScroll()
{
    editor.rx = 0;

    if (editor.cy < editor.numrows)
    {
        editor.rx = editorRowCxToRx(&editor.row[editor.cy], editor.cx);
    }
    
    // Scrolling up
    if (editor.cy < editor.rowoff)
    {
        editor.rowoff = editor.cy;
    }

    // Scrolling down
    if (editor.cy >= editor.rowoff + editor.screenRows)
    {
        // How much distance cursor y is from the maximum rendered screen rows
        editor.rowoff = editor.cy - editor.screenRows + 1;
    }

    if (editor.rx < editor.coloff)
    {
        editor.coloff = editor.rx;
    }

    if (editor.rx >= editor.coloff + (editor.screenCols - (LN_OFFSET + 1)))
    {
        editor.coloff = editor.rx - (editor.screenCols - (LN_OFFSET + 1)) + 1;
    }
}

/**
 * @brief Draws the status bar at the bottom of the editor
 * @param ab Append buffer to update the stream
 * @note `<esc>[m` is a command to set text attributes to terminal
 * @note `[m` - Default
 * @note `[7m` - Inverted colors (White background, Black foreground)
 * @note `[1m` - Bold
*/
void editorDrawStatusBar(struct abuf *ab)
{
    // '^[7m' switches to inverted colors
    abAppend(ab, "\x1b[7m", 4); 

    char status[80], rstatus[80];

    abAppend(ab, "\x1b[1m", 4); 

    int len = snprintf(
        status, sizeof(status),
        "[OPN] %.20s - %d lines %s",
        editor.fileName ? editor.fileName : "NO FILE",
        editor.numrows,
        editor.unsaved ? "(modified)" : ""
    );

    int rlen = (editor.cy + 1) <= editor.numrows ? 
        snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", (editor.syntax ? editor.syntax->fileType : "no ft"), editor.cy + 1, editor.numrows) : 
        //snprintf(rstatus, sizeof(rstatus), "CX: %d, CY: %d", editor.cx, editor.cy) : 
        snprintf(rstatus, sizeof(rstatus), "END OF FILE");

    if (rlen > editor.screenCols) rlen = editor.screenCols;
    if (len > editor.screenCols) len = editor.screenCols;
    abAppend(ab, status, len);

    // span the whole screen width
    while (len < editor.screenCols)
    {
        if (editor.screenCols - len == rlen)
        {
            abAppend(ab, rstatus, rlen);
            break;
        } 
        else
        {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab,"\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

/**
 * @brief Variadic function that emulates printf's multiparameter formating to store to `editor.statusmsg`
 * @param fmt String input
*/
void editorSetStatusMessage(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(editor.statusmsg, sizeof(editor.statusmsg), fmt, ap);
    va_end(ap);

    // stores CURRENT time to statusmsg_time => time(NULL)
    editor.statusmsg_time = time(NULL);
}

/**
 * @brief Displays helpful tips and information about editor.
*/
void editorDrawMessageBar(struct abuf *ab)
{
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(editor.statusmsg);

    if (msglen > editor.screenCols) msglen = editor.screenCols;

    if (msglen && time(NULL) - editor.statusmsg_time < 5)
    {
        abAppend(ab, editor.statusmsg, msglen);
    }
}