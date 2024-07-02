/***************************************************************************//**

  @file         text-editor.c

  @author       John Michael Magpantay

  @date         Wednesday, 12 June 2024 (Start)

  @date         Tuesday, 2 July 2024 (Finish)

  @brief        Simple Text Editor in C

*******************************************************************************/
#include "lib/buffer.h"
#include "lib/const.h"
#include "lib/editor.h"
#include "lib/input.h"
#include "lib/output.h"
#include "lib/terminal.h"
#include "lib/file_io.h"
#include "lib/syntax.h"
 
int main(int argc, char *argv[])
{
	enableRaw();
	initEditor();

  if (argc >= 2)
    {
      editorOpen(argv[1]);

    }

    editorSetStatusMessage("HELP: Ctrl-Q to quit | Ctrl-S to save | Ctrl-F to find"); 

    while (1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }
}
// text edit
