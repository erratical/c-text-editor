#include "../lib/editor.h"
#include "../lib/syntax.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void editorUpdateSyntax(erow *row)
{
    row->hl = realloc(row->hl, row->rsize);

    // sets the string row->hl to sd "0000000000000" by default
    memset(row->hl, HL_NORMAL, row->rsize);

    if (editor.syntax == NULL) return;

    char **keywords = editor.syntax->keywords;

    char *mcs = editor.syntax->multiline_comment_start;
    char *mce = editor.syntax->multiline_comment_end;
    char *scs = editor.syntax->single_line_comment_start;

    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;
    int scs_len = scs ? strlen(scs) : 0;

    int prev_sep = 1;
    int in_string = 0;
    int in_comment = (row->idx > 0 && editor.row[row->idx - 1].hl_open_comment);

    int ch = 0;
    while (ch < row->rsize)
    {
        char c = row->render[ch];
        unsigned char prev_hl = (ch > 0) ? row->hl[ch - 1] : HL_NORMAL;

        if (mcs_len && mce_len && !in_string)
        {
            if (in_comment)
            {
                row->hl[ch] = HL_MLCOMMENT;

                // end of comment
                if (!strncmp(&row->render[ch], mce, mce_len))
                {
                    memset(&row->hl[ch], HL_MLCOMMENT, mce_len);
                    ch += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                }
                else
                {
                    ch++;
                    continue;
                }
            }
            else if (!strncmp(&row->render[ch], mcs, mcs_len))
            {
                memset(&row->hl[ch], HL_MLCOMMENT, mcs_len);
                ch += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        if (editor.syntax->flags & HL_HIGHLIGHT_STRINGS)
        {
            if (in_string) 
            {
                row->hl[ch] = HL_STRING;
                if (c == '\\' && (ch + 1 < row->size))
                {
                    row->hl[ch + 1] = HL_STRING;
                    ch += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                ch++;
                prev_sep = 1;
                continue;
            }
            else 
            {
                if (c == '"' || c == '\'')
                {
                    in_string = c;
                    row->hl[ch] = HL_STRING;
                    ch++;
                    continue;
                }
            }
        }

        if (scs_len && !in_string && !in_comment)
        {
            if (!strncmp(&row->render[ch], scs, scs_len))
            {
                memset(&row->hl[ch], HL_COMMENT, row->rsize - ch);
                ch += row->rsize - ch;
            }
        }

        if (editor.syntax->flags & HL_HIGHLIGHT_NUMBERS)
        {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || 
            (c == '.' && prev_hl == HL_NUMBER))
            {
                row->hl[ch] = HL_NUMBER;
                ch++;
                prev_sep = 0;
                continue;
            } 
        }

        // KEYWORDS
        if (prev_sep)
        {
            int j;
            for (j = 0; keywords[j]; j++)
            {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|'; //boolean
                
                if (kw2) klen--; // example: "int|"

                // this is how you compare substrings, strncmp, and the length.
                if (!strncmp(&row->render[ch], keywords[j], klen) &&
                isSeparator(row->render[ch + klen]))
                {
                    memset(&row->hl[ch], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    ch += klen;
                    break;
                }
            }
            if (keywords[j] != NULL)
            {
                prev_sep = 0;
                continue;
            }
        }

        prev_sep = isSeparator(c);
        ch++;  
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;

    // recursive call
    if (changed && (row->idx + 1) < editor.numrows)
        editorUpdateSyntax(&editor.row[row->idx + 1]);
}

int editorSyntaxToColor(int hl) 
{
    switch (hl)
    {
        case HL_COMMENT: 
        case HL_MLCOMMENT: return 64;
        case HL_KEYWORD1: return 170;
        case HL_KEYWORD2: return 33;
        case HL_STRING: return 136;
        case HL_NUMBER: return 193;
        case HL_MATCH: return 36;
        default: return 255;
    }
}

int isSeparator(int c)
{
    return isspace(c) || c == '\0' || (strchr(",.()+-/*=~%<>[];", c) != NULL);
}

// sets up editor.syntax
void editorSelectSyntaxHighlight()
{
    editor.syntax = NULL;

    if (editor.fileName == NULL) return;

    char *ext = strchr(editor.fileName, '.');

    for (unsigned int j = 0; j < HLDB_ENTRIES; j++)
    {
        struct editorSyntax *s = &HDLB[j];
        unsigned int i = 0;
        while(s->fileMatch[i])
        {
            int is_ext = (s->fileMatch[i][0] == '.');
            
            if ((is_ext && ext && !strcmp(ext, s->fileMatch[i])) ||
                ((!is_ext) && strstr(editor.fileName, s->fileMatch[i])))
                {
                    editor.syntax = s;

                    int fileRow;

                    for (fileRow = 0; fileRow < editor.numrows; fileRow++)
                    {
                        editorUpdateSyntax(&editor.row[fileRow]);
                    }

                    return;
                }
            i++;
        }
    }
}
