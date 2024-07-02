#include <stdlib.h>
#include "../lib/buffer.h"
#include <string.h>

/**
 * @brief Similar to strcat, realloc memory to append char `s` to buffer `ab`
 * @param ab Type `struct abuf` in which to be appended with.
 * @param s Type `const char*` string to append.
 * @param len The length of the string to append.
 * @return None
*/
void abAppend(struct abuf *ab, const char *s, int len)
{
    // Reallocate memory to fit both ab->b and s
    char* new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;

    // Copy string `s` into end of new
    memcpy(&new[ab->len], s, len);

    // Update `ab->b` and `ab->len`
    ab->b = new;
    ab->len += len;
}

/**
 * @brief Frees the string buffer from `struct abuf`
 * @param ab Type `struct abuf`
 * @return None
*/
void abFree(struct abuf *ab)
{
    free(ab->b);
}