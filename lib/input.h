#ifndef INPUT_H
#define INPUT_H

#include "../lib/editor.h"

enum editorKey 
{
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  END_KEY,
  HOME_KEY,
  PAGE_UP,
  PAGE_DOWN
};


void editorProcessKeypress();
void editorMoveCursor(int key);
char *editorPrompt(char *prompt, void (*callback)(char *, int));

#endif