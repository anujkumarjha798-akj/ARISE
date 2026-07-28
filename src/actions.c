#include "arise.h"

extern Editor editor;

void toggle_comment(Editor *e) {
    if (!e) return;
    
    Line *line = &e->lines[e->cursor_row];
    if (!line->chars) return;
    
    char *comment = "// ";
    if (e->filetype) {
        if (strcmp(e->filetype, "python") == 0 || 
            strcmp(e->filetype, "bash") == 0 ||
            strcmp(e->filetype, "yaml") == 0 ||
            strcmp(e->filetype, "ruby") == 0) {
            comment = "# ";
        }
    }
    
    int comment_len = strlen(comment);
    
    if (strncmp(line->chars, comment, comment_len) == 0) {
        memmove(line->chars, line->chars + comment_len, line->length - comment_len + 1);
        line->length -= comment_len;
        if (e->cursor_col >= comment_len) e->cursor_col -= comment_len;
    } else {
        memmove(line->chars + comment_len, line->chars, line->length + 1);
        memcpy(line->chars, comment, comment_len);
        line->length += comment_len;
        e->cursor_col += comment_len;
    }
    
    line->tokens_valid = false;
    e->modified = true;
}

void duplicate_line(Editor *e) {
    if (!e || e->num_lines >= e->capacity - 1) return;
    
    e->capacity *= 2;
    Line *new_lines = realloc(e->lines, sizeof(Line) * e->capacity);
    if (!new_lines) return;
    e->lines = new_lines;
    
    for (int i = e->num_lines; i > e->cursor_row + 1; i--) {
        e->lines[i] = e->lines[i - 1];
    }
    
    Line *src = &e->lines[e->cursor_row];
    Line *dst = &e->lines[e->cursor_row + 1];
    
    dst->chars = strdup(src->chars);
    dst->length = src->length;
    dst->capacity = src->capacity;
    dst->tokens = NULL;
    dst->num_tokens = 0;
    dst->token_capacity = 0;
    dst->tokens_valid = false;
    
    e->num_lines++;
    e->cursor_row++;
    e->modified = true;
}

void move_line_up(Editor *e) {
    if (!e || e->cursor_row <= 0) return;
    
    Line temp = e->lines[e->cursor_row];
    e->lines[e->cursor_row] = e->lines[e->cursor_row - 1];
    e->lines[e->cursor_row - 1] = temp;
    e->cursor_row--;
    e->modified = true;
}

void move_line_down(Editor *e) {
    if (!e || e->cursor_row >= e->num_lines - 1) return;
    
    Line temp = e->lines[e->cursor_row];
    e->lines[e->cursor_row] = e->lines[e->cursor_row + 1];
    e->lines[e->cursor_row + 1] = temp;
    e->cursor_row++;
    e->modified = true;
}

void goto_line(Editor *e) {
    if (!e) return;
    snprintf(e->message, sizeof(e->message), "Go to line (1-%d): ", e->num_lines);
}
