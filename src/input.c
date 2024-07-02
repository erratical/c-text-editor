#include "../lib/input.h"
#include "../lib/const.h"
#include "../lib/editor.h"
#include "../lib/terminal.h"
#include "../lib/file_io.h"
#include "../lib/edit_op.h"
#include "../lib/output.h"
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief Handles special keypresses for editor functions.
 * @param None
 * @return None
*/
void editorProcessKeypress()
{
    static int quitConfirmation = QUIT_CONFIRMATION;

    int c = editorReadKey();

    switch (c)
    {
        // Enter Key
        case '\r':
            editorInsertNewLine();
            break;

        // Exit Key 'Ctrl + Q'
        case CTRL_KEY('q'):
            if (editor.unsaved && quitConfirmation > 0)
            {
                editorSetStatusMessage("Warning! File has unsaved changes."
                "Press Ctrl-Q %d more times to quit.", quitConfirmation);
                quitConfirmation--;
                return;
            }
            system("clear");
            exit(0);
            break;
        
        // Save
        case CTRL_KEY('s'):
            editorSave();
            break;
        
        case CTRL_KEY('f'):
            editorFind();

        // Movement Keys
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            editorMoveCursor(c);
            break;
        
        case HOME_KEY:
            editor.cx = 0;
            break;
        
        case END_KEY:
            if (editor.cy < editor.numrows)
                editor.cx = editor.row[editor.cy].size;
            break;
        
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;
        
        // Escape Key
        case CTRL_KEY('l'):
        case '\x1b':
            // TODO
            break;
        
        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP)
                {
                    editor.cy = editor.rowoff;
                }
                else if (c == PAGE_DOWN)
                {
                    editor.cy = editor.rowoff + editor.screenRows - 1;
                    if (editor.cy > editor.numrows) editor.cy = editor.numrows;
                }
                int times = editor.screenRows;
                while (times--)
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
        
        default:
            editorInsertChar(c);
    }

    quitConfirmation = QUIT_CONFIRMATION;
}

/**
 * @brief Handles keypresses for moving cursor.
 * @param key Key pressed.
 * @return None
*/
void editorMoveCursor(int key)
{
    // point row to the row in the file where the cursor is, unless cursor is beyond file, then NULL
    erow *row = (editor.cy >= editor.numrows) ? NULL : &editor.row[editor.cy];
    // this disables us to move when empty file

    switch (key) {
        case ARROW_LEFT:
            if (editor.cx != 0)
            {
                editor.cx--;
            }
            else if (editor.cy > 0)
            {
                editor.cy--;
                editor.cx = editor.row[editor.cy].size;
            }
            
            break;

        case ARROW_DOWN:
            if (editor.cy < editor.numrows)
            {
                editor.cy++;
            }
            break;

        case ARROW_UP:
            if (editor.cy < 1) break; 
            editor.cy--;
            break;

        case ARROW_RIGHT:
            if (row && (editor.cx) < row->size)
            {
                editor.cx++;
            } 
            else if (row && (editor.cx) == row->size)
            {
                editor.cy++;
                editor.cx = 0;
            }
            break;
    }

    row = (editor.cy >= editor.numrows) ? NULL : &editor.row[editor.cy];
    int rowlen = row ? row->size : 0; // if row exists, rowlen = rowsize, otherwise 0
    if (editor.cx > rowlen) editor.cx = rowlen; // correct x position if cursor is beyond line
}

char *editorPrompt(char *prompt, void (*callback)(char *, int))
{
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;

    buf[0] = '\0';
    
    while (1)
    {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();

        // Allow for backspacing
        if ( c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE)
        {
            if (buflen != 0) buf[--buflen] = '\0';
        }
        // Escape Key, cancel save as
        else if (c == CTRL_KEY('x'))
        {
            editorSetStatusMessage("");
            if (callback) callback(buf, c);
            free(buf);
            return NULL;
        }
        // Enter Key, save as file name
        else if (c == '\r')
        {
            if (buflen != 0)
            {
                editorSetStatusMessage("");
                if (callback) callback(buf, c);
                return buf;
            }
        }
        // Normal character keys
        else if (!iscntrl(c) && c < 128)
        {
            if (buflen == bufsize - 1)
            {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }

            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
        
        // Incremental Search
        if (callback) callback(buf, c);
    }
}