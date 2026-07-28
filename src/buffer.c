#include "arise.h"
#include <strings.h>   // 🔥 ADDED - strncasecmp ke liye

extern Editor editor;
static bool auto_closing = false;

// 🔥 Detect filetype and set indentation
void detect_indentation(Editor *e) {
    if (!e || !e->filetype) return;
    
    IndentConfig *cfg = &e->indent_config;
    cfg->use_spaces = true;
    cfg->auto_indent = true;
    cfg->auto_format_on_save = true;
    
    if (strcmp(e->filetype, "yaml") == 0 || 
        strcmp(e->filetype, "yml") == 0) {
        cfg->indent_size = 2;
        cfg->indent_chars = "  ";
    }
    else if (strcmp(e->filetype, "python") == 0) {
        cfg->indent_size = 4;
        cfg->indent_chars = "    ";
    }
    else if (strcmp(e->filetype, "json") == 0) {
        cfg->indent_size = 2;
        cfg->indent_chars = "  ";
    }
    else if (strcmp(e->filetype, "c") == 0 || 
             strcmp(e->filetype, "cpp") == 0 ||
             strcmp(e->filetype, "java") == 0) {
        cfg->indent_size = 4;
        cfg->indent_chars = "    ";
    }
    else if (strcmp(e->filetype, "javascript") == 0 ||
             strcmp(e->filetype, "typescript") == 0 ||
             strcmp(e->filetype, "html") == 0 ||
             strcmp(e->filetype, "css") == 0) {
        cfg->indent_size = 2;
        cfg->indent_chars = "  ";
    }
    else if (strcmp(e->filetype, "terraform") == 0) {
        cfg->indent_size = 2;
        cfg->indent_chars = "  ";
    }
    else if (strcmp(e->filetype, "go") == 0) {
        cfg->indent_size = 1;
        cfg->indent_chars = "\t";
        cfg->use_spaces = false;
    }
    else {
        cfg->indent_size = 2;
        cfg->indent_chars = "  ";
    }
}

// 🔥 Get current line indentation
int get_current_indent(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines) return 0;
    Line *line = &e->lines[row];
    if (!line->chars) return 0;
    
    int indent = 0;
    while (indent < line->length && line->chars[indent] == ' ') {
        indent++;
    }
    return indent;
}

// 🔥 Calculate indent for next line
int calculate_indent(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines) return 0;
    
    Line *line = &e->lines[row];
    if (!line->chars) return 0;
    
    int current_indent = get_current_indent(e, row);
    int indent_size = e->indent_config.indent_size;
    
    // Check if line is empty (just spaces)
    int i = 0;
    while (i < line->length && (line->chars[i] == ' ' || line->chars[i] == '\t')) {
        i++;
    }
    
    // If line is empty, use previous line's indent
    if (i >= line->length) {
        return current_indent;
    }
    
    char *content = line->chars + i;
    int content_len = line->length - i;
    
    // YAML: Check for list items or map keys
    if (strcmp(e->filetype, "yaml") == 0 || strcmp(e->filetype, "yml") == 0) {
        // If line is a list item, next line should be 2 spaces more
        if (content_len > 0 && content[0] == '-') {
            return current_indent + indent_size;
        }
        // If line ends with ':', next line should indent
        if (content_len > 0 && content[content_len - 1] == ':') {
            return current_indent + indent_size;
        }
        // If line contains a key-value pair, next line same indent
        return current_indent;
    }
    
    // Python: Check for block start keywords
    if (strcmp(e->filetype, "python") == 0) {
        const char *block_keywords[] = {
            "def", "class", "if", "elif", "else", "for", "while",
            "try", "except", "finally", "with", "async",
            NULL
        };
        
        for (int k = 0; block_keywords[k] != NULL; k++) {
            int kw_len = strlen(block_keywords[k]);
            if (content_len >= kw_len && 
                strncasecmp(content, block_keywords[k], kw_len) == 0) {
                // Check if line ends with ':'
                if (content_len > 0 && content[content_len - 1] == ':') {
                    return current_indent + indent_size;
                }
            }
        }
        return current_indent;
    }
    
    // C/C++/Java/JavaScript: Check for opening braces
    if (strcmp(e->filetype, "c") == 0 || strcmp(e->filetype, "cpp") == 0 ||
        strcmp(e->filetype, "java") == 0 || strcmp(e->filetype, "javascript") == 0 ||
        strcmp(e->filetype, "typescript") == 0) {
        
        int brace_count = 0;
        for (int j = 0; j < line->length; j++) {
            if (line->chars[j] == '{') {
                brace_count++;
            }
            if (line->chars[j] == '}') {
                brace_count--;
            }
        }
        
        if (brace_count > 0) {
            return current_indent + indent_size;
        }
        
        // Check for keywords like if, for, while
        const char *block_keywords[] = {
            "if", "else", "for", "while", "do", "switch", "try", "catch",
            NULL
        };
        
        for (int k = 0; block_keywords[k] != NULL; k++) {
            int kw_len = strlen(block_keywords[k]);
            if (content_len >= kw_len && 
                strncasecmp(content, block_keywords[k], kw_len) == 0) {
                // Check if line ends with '{' or has '{' somewhere
                for (int j = 0; j < line->length; j++) {
                    if (line->chars[j] == '{') {
                        return current_indent + indent_size;
                    }
                }
            }
        }
        return current_indent;
    }
    
    // Dockerfile
    if (strcmp(e->filetype, "dockerfile") == 0) {
        const char *block_keywords[] = {
            "RUN", "CMD", "ENTRYPOINT", "FROM", "ENV", "ARG", "LABEL",
            NULL
        };
        
        for (int k = 0; block_keywords[k] != NULL; k++) {
            int kw_len = strlen(block_keywords[k]);
            if (content_len >= kw_len && 
                strncasecmp(content, block_keywords[k], kw_len) == 0) {
                return current_indent + indent_size;
            }
        }
        return current_indent;
    }
    
    return current_indent;
}

// 🔥 Auto-indent a line
void auto_indent_line(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines) return;
    if (row == 0) return;
    
    int target_indent = calculate_indent(e, row - 1);
    if (target_indent < 0) target_indent = 0;
    
    Line *line = &e->lines[row];
    if (!line->chars) return;
    
    // Count current spaces
    int current_indent = 0;
    while (current_indent < line->length && line->chars[current_indent] == ' ') {
        current_indent++;
    }
    
    // If already correctly indented, do nothing
    if (current_indent == target_indent) return;
    
    // Move content to new position
    int content_len = line->length - current_indent;
    char *content = malloc(content_len + 1);
    if (!content) return;
    memcpy(content, &line->chars[current_indent], content_len);
    content[content_len] = '\0';
    
    // Create new line with proper indent
    char *new_chars = malloc(target_indent + content_len + 1);
    if (!new_chars) {
        free(content);
        return;
    }
    
    // Fill with spaces
    for (int i = 0; i < target_indent; i++) {
        new_chars[i] = ' ';
    }
    memcpy(new_chars + target_indent, content, content_len);
    new_chars[target_indent + content_len] = '\0';
    
    free(line->chars);
    line->chars = new_chars;
    line->length = target_indent + content_len;
    line->tokens_valid = false;
    
    free(content);
}

// 🔥 Fix indentation for entire file
void fix_indentation(Editor *e) {
    if (!e) return;
    
    // Detect filetype and set config
    detect_indentation(e);
    
    for (int i = 1; i < e->num_lines; i++) {
        auto_indent_line(e, i);
    }
    
    e->modified = true;
    snprintf(e->message, MESSAGE_SIZE, "Indentation fixed (%d lines)", e->num_lines);
}

// 🔥 Format entire buffer
void format_buffer(Editor *e) {
    if (!e) return;
    fix_indentation(e);
    snprintf(e->message, MESSAGE_SIZE, "Buffer formatted");
}

// 🔥 Check if line is blank
bool is_blank_line(Line *line) {
    if (!line || !line->chars) return true;
    for (int i = 0; i < line->length; i++) {
        if (line->chars[i] != ' ' && line->chars[i] != '\t') {
            return false;
        }
    }
    return true;
}

// 🔥 Check if line starts a block
bool is_block_start(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines) return false;
    Line *line = &e->lines[row];
    if (!line->chars) return false;
    
    char *content = line->chars;
    int len = line->length;
    
    // Check for '{' in C-like languages
    if (strcmp(e->filetype, "c") == 0 || strcmp(e->filetype, "cpp") == 0 ||
        strcmp(e->filetype, "java") == 0 || strcmp(e->filetype, "javascript") == 0 ||
        strcmp(e->filetype, "typescript") == 0) {
        for (int i = 0; i < len; i++) {
            if (content[i] == '{') return true;
        }
    }
    
    // Python: check for ':'
    if (strcmp(e->filetype, "python") == 0) {
        for (int i = 0; i < len; i++) {
            if (content[i] == ':') return true;
        }
    }
    
    // YAML: check for ':' or '-'
    if (strcmp(e->filetype, "yaml") == 0 || strcmp(e->filetype, "yml") == 0) {
        for (int i = 0; i < len; i++) {
            if (content[i] == ':' || content[i] == '-') return true;
        }
    }
    
    return false;
}

// 🔥 Check if line ends a block
bool is_block_end(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines) return false;
    Line *line = &e->lines[row];
    if (!line->chars) return false;
    
    // Check for '}' in C-like languages
    if (strcmp(e->filetype, "c") == 0 || strcmp(e->filetype, "cpp") == 0 ||
        strcmp(e->filetype, "java") == 0 || strcmp(e->filetype, "javascript") == 0 ||
        strcmp(e->filetype, "typescript") == 0) {
        for (int i = 0; i < line->length; i++) {
            if (line->chars[i] == '}') return true;
        }
    }
    
    return false;
}

// 🔥 Get line indent (spaces only)
int get_line_indent(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines) return 0;
    return get_current_indent(e, row);
}

// 🔥 Get next line indent
int get_next_indent(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines - 1) return 0;
    return get_current_indent(e, row + 1);
}

void insert_char(Editor *e, char c) {
    if (!e || e->num_lines == 0) return;
    if (e->cursor_row < 0 || e->cursor_row >= e->num_lines) return;
    
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
    line->chars[e->cursor_col] = c;
    line->length++;
    e->cursor_col++;
    e->modified = true;
    
    line->tokens_valid = false;
    
    if (!auto_closing) {
        char closing = 0;
        switch (c) {
            case '(': closing = ')'; break;
            case '[': closing = ']'; break;
            case '{': closing = '}'; break;
            case '<': closing = '>'; break;
            case '"': closing = '"'; break;
            case '\'': closing = '\''; break;
            case '`': closing = '`'; break;
        }
        
        if (closing) {
            auto_closing = true;
            insert_char(e, closing);
            e->cursor_col--;
            auto_closing = false;
        }
    }
}

void delete_char(Editor *e) {
    if (!e || e->num_lines == 0) return;
    if (e->cursor_row < 0 || e->cursor_row >= e->num_lines) return;
    
    Line *line = &e->lines[e->cursor_row];
    if (!line->chars) return;
    
    if (e->cursor_col < line->length) {
        memmove(&line->chars[e->cursor_col], &line->chars[e->cursor_col + 1],
                line->length - e->cursor_col);
        line->length--;
        line->tokens_valid = false;
        e->modified = true;
    } else if (e->cursor_row < e->num_lines - 1) {
        Line *next = &e->lines[e->cursor_row + 1];
        if (!next->chars) return;
        
        int new_len = line->length + next->length;
        char *new_chars = realloc(line->chars, new_len + 1);
        if (!new_chars) return;
        line->chars = new_chars;
        
        memcpy(&line->chars[line->length], next->chars, next->length);
        line->length = new_len;
        line->chars[line->length] = '\0';
        line->tokens_valid = false;
        
        free(next->chars);
        free(next->tokens);
        
        for (int i = e->cursor_row + 1; i < e->num_lines - 1; i++) {
            e->lines[i] = e->lines[i + 1];
        }
        e->num_lines--;
        e->modified = true;
    }
}

void backspace_char(Editor *e) {
    if (!e) return;
    
    if (e->cursor_col > 0 && e->cursor_col < e->lines[e->cursor_row].length) {
        Line *line = &e->lines[e->cursor_row];
        char left = line->chars[e->cursor_col - 1];
        char right = line->chars[e->cursor_col];
        
        if ((left == '(' && right == ')') ||
            (left == '[' && right == ']') ||
            (left == '{' && right == '}') ||
            (left == '<' && right == '>') ||
            (left == '"' && right == '"') ||
            (left == '\'' && right == '\'') ||
            (left == '`' && right == '`')) {
            memmove(&line->chars[e->cursor_col], &line->chars[e->cursor_col + 1],
                    line->length - e->cursor_col);
            line->length--;
            line->tokens_valid = false;
            e->modified = true;
        }
    }
    
    if (e->cursor_col > 0) {
        e->cursor_col--;
        delete_char(e);
    } else if (e->cursor_row > 0) {
        e->cursor_col = e->lines[e->cursor_row - 1].length;
        e->cursor_row--;
        delete_char(e);
    }
}

static void adjust_scroll(Editor *e) {
    if (!e) return;
    
    int visible_rows = e->screen_rows - 2;
    if (visible_rows <= 0) return;
    
    if (e->cursor_row < e->scroll_row) {
        e->scroll_row = e->cursor_row;
    }
    
    if (e->cursor_row >= e->scroll_row + visible_rows) {
        e->scroll_row = e->cursor_row - visible_rows + 1;
    }
    
    if (e->scroll_row < 0) e->scroll_row = 0;
    int max_scroll = e->num_lines - visible_rows;
    if (max_scroll < 0) max_scroll = 0;
    if (e->scroll_row > max_scroll) e->scroll_row = max_scroll;
}

void insert_newline(Editor *e) {
    if (!e || e->num_lines == 0) return;
    if (e->cursor_row < 0 || e->cursor_row >= e->num_lines) return;
    
    if (e->num_lines >= e->capacity - 1) {
        e->capacity *= 2;
        Line *new_lines = realloc(e->lines, sizeof(Line) * e->capacity);
        if (!new_lines) return;
        e->lines = new_lines;
    }
    
    Line *cur = &e->lines[e->cursor_row];
    if (!cur->chars) return;
    
    for (int i = e->num_lines; i > e->cursor_row + 1; i--) {
        e->lines[i] = e->lines[i - 1];
    }
    
    e->lines[e->cursor_row + 1].chars = malloc(MAX_LINE_LENGTH);
    if (!e->lines[e->cursor_row + 1].chars) return;
    e->lines[e->cursor_row + 1].capacity = MAX_LINE_LENGTH;
    e->lines[e->cursor_row + 1].tokens = NULL;
    e->lines[e->cursor_row + 1].num_tokens = 0;
    e->lines[e->cursor_row + 1].token_capacity = 0;
    e->lines[e->cursor_row + 1].tokens_valid = false;
    
    int rest_length = cur->length - e->cursor_col;
    if (rest_length > 0) {
        memcpy(e->lines[e->cursor_row + 1].chars, &cur->chars[e->cursor_col], rest_length);
    }
    e->lines[e->cursor_row + 1].length = rest_length;
    e->lines[e->cursor_row + 1].chars[rest_length] = '\0';
    
    cur->chars[e->cursor_col] = '\0';
    cur->length = e->cursor_col;
    cur->tokens_valid = false;
    
    // 🔥 Smart auto-indentation for new line
    detect_indentation(e);
    int indent = calculate_indent(e, e->cursor_row);
    if (indent < 0) indent = 0;
    
    if (indent > 0) {
        char *indent_str = malloc(indent + 1);
        if (!indent_str) return;
        memset(indent_str, ' ', indent);
        indent_str[indent] = '\0';
        
        memmove(&e->lines[e->cursor_row + 1].chars[indent], 
                e->lines[e->cursor_row + 1].chars, rest_length + 1);
        memcpy(e->lines[e->cursor_row + 1].chars, indent_str, indent);
        e->lines[e->cursor_row + 1].length += indent;
        free(indent_str);
    }
    
    e->cursor_row++;
    e->cursor_col = indent;
    e->num_lines++;
    e->modified = true;
    
    adjust_scroll(e);
}

void selection_clear(Editor *e) {
    if (!e) return;
    e->has_selection = false;
    e->sel_anchor_row = e->cursor_row;
    e->sel_anchor_col = e->cursor_col;
}

void selection_start(Editor *e) {
    if (!e) return;
    if (!e->has_selection) {
        e->sel_anchor_row = e->cursor_row;
        e->sel_anchor_col = e->cursor_col;
    }
    e->has_selection = true;
}

bool selection_active(Editor *e) {
    if (!e || !e->has_selection) return false;
    return (e->sel_anchor_row != e->cursor_row || e->sel_anchor_col != e->cursor_col);
}

bool selection_get_range(Editor *e, int *r1, int *c1, int *r2, int *c2) {
    if (!selection_active(e)) return false;
    
    int ar = e->sel_anchor_row, ac = e->sel_anchor_col;
    int cr = e->cursor_row, cc = e->cursor_col;
    
    if (ar < cr || (ar == cr && ac < cc)) {
        *r1 = ar; *c1 = ac; *r2 = cr; *c2 = cc;
    } else {
        *r1 = cr; *c1 = cc; *r2 = ar; *c2 = ac;
    }
    return true;
}

void selection_select_all(Editor *e) {
    if (!e || e->num_lines == 0) return;
    e->sel_anchor_row = 0;
    e->sel_anchor_col = 0;
    e->cursor_row = e->num_lines - 1;
    e->cursor_col = e->lines[e->cursor_row].length;
    e->has_selection = true;
}

void selection_delete_range(Editor *e) {
    int r1, c1, r2, c2;
    if (!e || !selection_get_range(e, &r1, &c1, &r2, &c2)) return;
    
    if (r1 == r2) {
        Line *line = &e->lines[r1];
        int len = c2 - c1;
        if (len > 0) {
            memmove(&line->chars[c1], &line->chars[c2], line->length - c2 + 1);
            line->length -= len;
            line->tokens_valid = false;
        }
    } else {
        Line *first = &e->lines[r1];
        Line *last = &e->lines[r2];
        int keep_last_len = last->length - c2;
        int new_len = c1 + keep_last_len;
        
        if (new_len + 1 > first->capacity) {
            int new_cap = new_len + 1;
            char *new_chars = realloc(first->chars, new_cap);
            if (!new_chars) return;
            first->chars = new_chars;
            first->capacity = new_cap;
        }
        
        memcpy(&first->chars[c1], &last->chars[c2], keep_last_len);
        first->length = new_len;
        first->chars[new_len] = '\0';
        first->tokens_valid = false;
        
        for (int i = r1 + 1; i <= r2; i++) {
            free(e->lines[i].chars);
            free(e->lines[i].tokens);
        }
        
        int shift = r2 - r1;
        for (int i = r1 + 1; i < e->num_lines - shift; i++) {
            e->lines[i] = e->lines[i + shift];
        }
        e->num_lines -= shift;
    }
    
    e->cursor_row = r1;
    e->cursor_col = c1;
    e->has_selection = false;
    e->sel_anchor_row = r1;
    e->sel_anchor_col = c1;
    e->modified = true;
    
    adjust_scroll(e);
}

void insert_tab(Editor *e) {
    if (!e) return;
    detect_indentation(e);
    
    int size = e->indent_config.indent_size;
    if (e->indent_config.use_spaces) {
        for (int i = 0; i < size; i++) {
            insert_char(e, ' ');
        }
    } else {
        insert_char(e, '\t');
    }
}

void remove_tab(Editor *e) {
    if (!e) return;
    
    Line *line = &e->lines[e->cursor_row];
    if (!line->chars) return;
    
    int spaces = 0;
    int col = e->cursor_col - 1;
    while (col >= 0 && line->chars[col] == ' ') {
        spaces++;
        col--;
    }
    
    if (spaces == 0) return;
    
    detect_indentation(e);
    int remove = (spaces % e->indent_config.indent_size == 0) ? 
                  e->indent_config.indent_size : (spaces % e->indent_config.indent_size);
    if (remove > e->cursor_col) remove = e->cursor_col;
    
    e->cursor_col -= remove;
    for (int i = 0; i < remove; i++) {
        delete_char(e);
    }
}
