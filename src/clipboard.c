#include "arise.h"
#include "clipboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Editor editor;

static char *run_command(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    size_t capacity = 4096;
    size_t length = 0;
    char *result = malloc(capacity);
    if (!result) {
        pclose(fp);
        return NULL;
    }
    result[0] = '\0';
    
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t blen = strlen(buffer);
        if (length + blen + 1 > capacity) {
            capacity *= 2;
            char *new_result = realloc(result, capacity);
            if (!new_result) {
                free(result);
                pclose(fp);
                return NULL;
            }
            result = new_result;
        }
        strcpy(result + length, buffer);
        length += blen;
    }
    
    int status = pclose(fp);
    if (status != 0 && length == 0) {
        free(result);
        return NULL;
    }
    
    return result;
}

static char *paste_osc52(void) {
    char *osc52 = getenv("OSC52_PASTE");
    if (osc52 && strlen(osc52) > 0) {
        return strdup(osc52);
    }
    return NULL;
}

static bool copy_osc52(const char *text) {
    if (!text) return false;
    printf("\033]52;c;%s\007", text);
    fflush(stdout);
    return true;
}

static bool copy_wl_clipboard(const char *text) {
    FILE *fp = popen("wl-copy", "w");
    if (!fp) return false;
    fwrite(text, 1, strlen(text), fp);
    int status = pclose(fp);
    return (status == 0);
}

static char *paste_wl_clipboard(void) {
    return run_command("wl-paste 2>/dev/null");
}

static bool copy_xclip(const char *text) {
    FILE *fp = popen("xclip -selection clipboard", "w");
    if (!fp) return false;
    fwrite(text, 1, strlen(text), fp);
    int status = pclose(fp);
    return (status == 0);
}

static char *paste_xclip(void) {
    return run_command("xclip -selection clipboard -o 2>/dev/null");
}

static bool copy_xsel(const char *text) {
    FILE *fp = popen("xsel --clipboard --input", "w");
    if (!fp) return false;
    fwrite(text, 1, strlen(text), fp);
    int status = pclose(fp);
    return (status == 0);
}

static char *paste_xsel(void) {
    return run_command("xsel --clipboard --output 2>/dev/null");
}

static bool copy_pbcopy(const char *text) {
    FILE *fp = popen("pbcopy", "w");
    if (!fp) return false;
    fwrite(text, 1, strlen(text), fp);
    int status = pclose(fp);
    return (status == 0);
}

static char *paste_pbpaste(void) {
    return run_command("pbpaste 2>/dev/null");
}

void copy_to_system_clipboard(const char *text) {
    if (!text) return;
    
    if (getenv("SSH_TTY") || getenv("SSH_CLIENT") || getenv("SSH_CONNECTION")) {
        if (copy_osc52(text)) return;
    }
    
    if (getenv("WAYLAND_DISPLAY")) {
        if (copy_wl_clipboard(text)) return;
    }
    
    if (getenv("DISPLAY")) {
        if (copy_xclip(text)) return;
        if (copy_xsel(text)) return;
    }
    
    if (copy_pbcopy(text)) return;
    copy_osc52(text);
}

char *paste_from_system_clipboard(void) {
    char *text = NULL;
    
    text = paste_osc52();
    if (text) return text;
    
    if (getenv("WAYLAND_DISPLAY")) {
        text = paste_wl_clipboard();
        if (text) return text;
    }
    
    if (getenv("DISPLAY")) {
        text = paste_xclip();
        if (text) return text;
        text = paste_xsel();
        if (text) return text;
    }
    
    text = paste_pbpaste();
    if (text) return text;
    
    return NULL;
}

static void insert_raw_text(Editor *e, const char *text) {
    if (!e || !text) return;
    
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            Line *cur = &e->lines[e->cursor_row];
            if (!cur->chars) return;
            
            if (e->num_lines >= e->capacity - 1) {
                e->capacity *= 2;
                Line *new_lines = realloc(e->lines, sizeof(Line) * e->capacity);
                if (!new_lines) return;
                e->lines = new_lines;
            }
            
            for (int j = e->num_lines; j > e->cursor_row + 1; j--) {
                e->lines[j] = e->lines[j - 1];
            }
            
            int rest_len = cur->length - e->cursor_col;
            e->lines[e->cursor_row + 1].chars = malloc(MAX_LINE_LENGTH);
            e->lines[e->cursor_row + 1].capacity = MAX_LINE_LENGTH;
            e->lines[e->cursor_row + 1].tokens = NULL;
            e->lines[e->cursor_row + 1].num_tokens = 0;
            e->lines[e->cursor_row + 1].token_capacity = 0;
            e->lines[e->cursor_row + 1].tokens_valid = false;
            
            if (rest_len > 0) {
                memcpy(e->lines[e->cursor_row + 1].chars, &cur->chars[e->cursor_col], rest_len);
            }
            e->lines[e->cursor_row + 1].length = rest_len;
            e->lines[e->cursor_row + 1].chars[rest_len] = '\0';
            
            cur->chars[e->cursor_col] = '\0';
            cur->length = e->cursor_col;
            cur->tokens_valid = false;
            
            e->cursor_row++;
            e->cursor_col = 0;
            e->num_lines++;
        } else if (text[i] != '\r') {
            Line *line = &e->lines[e->cursor_row];
            if (!line->chars) return;
            
            if (line->length >= line->capacity - 1) {
                line->capacity *= 2;
                char *new_chars = realloc(line->chars, line->capacity);
                if (!new_chars) return;
                line->chars = new_chars;
            }
            
            if (e->cursor_col < 0) e->cursor_col = 0;
            if (e->cursor_col > line->length) e->cursor_col = line->length;
            
            memmove(&line->chars[e->cursor_col + 1], &line->chars[e->cursor_col], 
                    line->length - e->cursor_col + 1);
            line->chars[e->cursor_col] = text[i];
            line->length++;
            e->cursor_col++;
            line->tokens_valid = false;
        }
    }
    
    e->modified = true;
}

// Read raw text from document buffer ONLY - no line numbers, no UI
static char *get_buffer_text(Editor *e) {
    if (!e || e->num_lines == 0) return strdup("");
    
    size_t total = 0;
    for (int i = 0; i < e->num_lines; i++) {
        if (e->lines[i].chars) {
            total += e->lines[i].length;
        }
        total += 1;
    }
    
    char *text = malloc(total + 1);
    if (!text) return strdup("");
    text[0] = '\0';
    
    for (int i = 0; i < e->num_lines; i++) {
        if (e->lines[i].chars) {
            strncat(text, e->lines[i].chars, total - strlen(text));
        }
        if (i < e->num_lines - 1) {
            strncat(text, "\n", total - strlen(text));
        }
    }
    
    return text;
}

// Read raw text for the current selection ONLY, straight from the text
// buffer - no line numbers, no gutter spacing, no UI ever enters this.
static char *get_selection_text(Editor *e, int r1, int c1, int r2, int c2) {
    if (!e) return strdup("");
    
    size_t total = 0;
    for (int i = r1; i <= r2; i++) {
        Line *line = &e->lines[i];
        int start = (i == r1) ? c1 : 0;
        int end = (i == r2) ? c2 : line->length;
        if (end > line->length) end = line->length;
        if (start < end) total += (size_t)(end - start);
        if (i < r2) total += 1;
    }
    
    char *text = malloc(total + 1);
    if (!text) return strdup("");
    text[0] = '\0';
    size_t used = 0;
    
    for (int i = r1; i <= r2; i++) {
        Line *line = &e->lines[i];
        int start = (i == r1) ? c1 : 0;
        int end = (i == r2) ? c2 : line->length;
        if (end > line->length) end = line->length;
        
        if (start < end && line->chars) {
            memcpy(text + used, &line->chars[start], end - start);
            used += (end - start);
        }
        if (i < r2) {
            text[used++] = '\n';
        }
    }
    text[used] = '\0';
    
    return text;
}

void copy_selection(Editor *e) {
    if (!e) return;
    
    int r1, c1, r2, c2;
    char *content;
    if (selection_get_range(e, &r1, &c1, &r2, &c2)) {
        content = get_selection_text(e, r1, c1, r2, c2);
    } else {
        content = get_buffer_text(e);
    }
    
    if (e->clipboard) free(e->clipboard);
    e->clipboard = strdup(content);
    
    copy_to_system_clipboard(content);
    free(content);
    
    snprintf(e->message, MESSAGE_SIZE, "Copied to clipboard");
}

void cut_selection(Editor *e) {
    if (!e) return;
    
    if (selection_active(e)) {
        copy_selection(e);
        selection_delete_range(e);
        e->modified = true;
        snprintf(e->message, MESSAGE_SIZE, "Cut to clipboard");
        return;
    }
    
    copy_selection(e);
    
    for (int i = 0; i < e->num_lines; i++) {
        free(e->lines[i].chars);
        free(e->lines[i].tokens);
    }
    
    e->num_lines = 1;
    e->cursor_row = 0;
    e->cursor_col = 0;
    
    e->lines[0].chars = malloc(MAX_LINE_LENGTH);
    if (!e->lines[0].chars) return;
    e->lines[0].chars[0] = '\0';
    e->lines[0].length = 0;
    e->lines[0].capacity = MAX_LINE_LENGTH;
    e->lines[0].tokens = NULL;
    e->lines[0].num_tokens = 0;
    e->lines[0].token_capacity = 0;
    e->lines[0].tokens_valid = false;
    
    e->modified = true;
    snprintf(e->message, MESSAGE_SIZE, "Cut to clipboard");
}

void paste_clipboard(Editor *e) {
    if (!e) return;
    
    char *sys_text = paste_from_system_clipboard();
    char *text_to_paste = NULL;
    
    if (sys_text && strlen(sys_text) > 0) {
        text_to_paste = sys_text;
    } else if (e->clipboard && strlen(e->clipboard) > 0) {
        text_to_paste = e->clipboard;
    }
    
    if (!text_to_paste) {
        snprintf(e->message, MESSAGE_SIZE, "Clipboard is empty");
        return;
    }
    
    insert_raw_text(e, text_to_paste);
    
    if (sys_text) free(sys_text);
    
    snprintf(e->message, MESSAGE_SIZE, "Pasted from clipboard");
}
