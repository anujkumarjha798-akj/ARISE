#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "arise.h"

void copy_to_system_clipboard(const char *text);
char *paste_from_system_clipboard(void);

#endif
