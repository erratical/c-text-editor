#include "../lib/terminal.h"
#include "../lib/const.h"
#include "../lib/input.h"
#include "../lib/editor.h"

#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Sets terminal attribute to disable 'ECHO'
 * @param None
 * @return None
*/
void enableRaw()
{
    if (tcgetattr(STDIN_FILENO, &(editor.originalTermios)) == -1) die("tcgetattr");

    atexit(disableRaw);

    // Initialize termios variable
    struct termios raw = editor.originalTermios;

    // Turning off Ctrl-Q and Ctrl-Z with 'IXON'
    // Differentiating '\r\ and '\n' with 'ICRNL'
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // Turning off output processing such as '\n' into '\r\n' with 'OPOST'
    raw.c_oflag &= ~(OPOST);

    // Modify 'raw' attributes, disabling 'ECHO' and 'ICANON' mode
    // Turning off 'canonical mode' enables for the input to be read byte-by-byte
    // Turning off Ctrl-C and Ctrl-Z with 'ISIG'
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cflag |= (CS8);

    // Set the minimum number of bytes to be read to 0
    raw.c_cc[VMIN] = 0;

    // Set time to wait before read() returns; to 10ms
    raw.c_cc[VTIME] = 10;

    // Save 'raw' attributes into terminal
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr"); 
}

/**
 * @brief Resets terminal attributes back to normal
 * @param None
 * @return None
*/
void disableRaw()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &(editor.originalTermios)) == -1) die("tcsetattr");
}

/**
 * @brief Display error message and exit the program with failure.
 * @param str The context in which the error occurred.
*/
void die(const char* str)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    // Custom string `str` to be printed before actual error.
    perror(str);
    exit(1);
}

/**
 * @brief Function for reading single keypresses. The different key press constants are mapped here.
 * @return The character read.
*/
int editorReadKey()
{
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    // Escape sequences
    if (c == '\x1b')
    {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[')
        {
            if (seq[1] >= 0 && seq[1] <= 9)
            {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~')
                {
                    switch(seq[1])
                    {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }
            // Ctrl-A, Ctrl-B, Ctrl-C, Ctrl-D 
            // are synonymous to arrow keys
            else 
            {
                switch(seq[1])
                {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'F': return END_KEY;
                    case 'H': return HOME_KEY;
                }
            }    
        } 
        // '<esc>O'
        else if (seq[0] == 'O') 
        {
            switch(seq[1])
            {
                case 'F': return END_KEY;
                case 'H': return HOME_KEY;
            }
        }
    
        return '\x1b';
    }

    // Normal key presses
    else 
    {
        return c;
    }
}

/**
 * @brief Get the current terminal window size.
 * @param rows Integer pointer to number of rows
 * @param cols Integer pointer to number of cols
 * @return 0 on success, -1 on failure (if window size value failed to be obtained or erroneous value)
*/
int getWindowSize(int* rows, int* cols)
{
    struct winsize ws;

    // If ioctl has failed, manually position cursor to the bottom right
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        
        return getCursorPosition(rows, cols);
    }
    
    else 
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/**
 * @brief Performs a terminal command '<esc> n' to obtain cursor position and parses it
 * @param rows Integer pointer to number of rows
 * @param cols Integer pointer to number of cols
 * @return 0 if successful parsing, -1 if not
*/
int getCursorPosition(int* rows, int* cols)
{
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    // Parse the request into a string 'buf'
    //<rows>;<cols>R
    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }

    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}