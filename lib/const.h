#ifndef CONST_H
#define CONST_H

#define VERSION "0.0.1"
#define AUTHOR "John Michael C. Magpantay"
#define DSTART "Wednesday, 12 June 2024"
#define TITLE "Text Editor (Pending)"
#define DFIN "Tuesday, 2 July 2024"

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#define TAB_STOP 8
#define LEFT_BOUND 5

#define LN_OFFSET 6

#define QUIT_CONFIRMATION 3

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

#endif