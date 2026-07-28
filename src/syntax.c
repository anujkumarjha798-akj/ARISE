#include "arise.h"

extern Editor editor;

static const char *c_keywords[] = {
    "if", "else", "for", "while", "do", "switch", "case", "default",
    "break", "continue", "return", "goto", "sizeof", "typedef",
    "struct", "union", "enum", "static", "extern", "register", "volatile",
    "const", "inline", "restrict", "auto", "signed", "unsigned",
    NULL
};

static const char *c_types[] = {
    "int", "long", "float", "double", "char", "void", "bool",
    "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "ssize_t", "ptrdiff_t", "wchar_t",
    NULL
};

static const char *c_constants[] = {
    "NULL", "nullptr", "true", "false", "TRUE", "FALSE",
    "EOF", "EXIT_SUCCESS", "EXIT_FAILURE",
    NULL
};

static const char *python_keywords[] = {
    "if", "elif", "else", "for", "while", "break", "continue",
    "return", "def", "class", "import", "from", "as", "try",
    "except", "finally", "raise", "with", "yield", "lambda",
    "pass", "assert", "del", "global", "nonlocal", "in", "is",
    "not", "and", "or", "True", "False", "None",
    NULL
};

static const char *go_keywords[] = {
    "if", "else", "for", "range", "switch", "case", "default",
    "return", "func", "type", "struct", "interface", "map",
    "chan", "go", "select", "defer", "package", "import",
    "var", "const", "break", "continue", "fallthrough",
    "goto", "make", "new", "nil",
    NULL
};

void init_syntax_keywords(void) {
}

bool is_keyword(const char *word, int len) {
    if (!word || len <= 0 || len > 32) return false;
    
    char buffer[33];
    strncpy(buffer, word, len);
    buffer[len] = '\0';
    
    const char **keywords = c_keywords;
    char *ext = editor.filetype;
    
    if (ext) {
        if (strcmp(ext, "python") == 0) keywords = python_keywords;
        else if (strcmp(ext, "go") == 0) keywords = go_keywords;
    }
    
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(buffer, keywords[i]) == 0) return true;
    }
    return false;
}

bool is_type(const char *word, int len) {
    if (!word || len <= 0 || len > 32) return false;
    
    char buffer[33];
    strncpy(buffer, word, len);
    buffer[len] = '\0';
    
    for (int i = 0; c_types[i]; i++) {
        if (strcmp(buffer, c_types[i]) == 0) return true;
    }
    return false;
}

static bool is_constant(const char *word, int len) {
    if (!word || len <= 0 || len > 32) return false;
    
    char buffer[33];
    strncpy(buffer, word, len);
    buffer[len] = '\0';
    
    for (int i = 0; c_constants[i]; i++) {
        if (strcmp(buffer, c_constants[i]) == 0) return true;
    }
    return false;
}

static void add_token(Line *line, TokenType type, int start, int length) {
    if (!line) return;
    
    if (line->num_tokens >= line->token_capacity - 1) {
        line->token_capacity *= 2;
        line->tokens = realloc(line->tokens, sizeof(Token) * line->token_capacity);
        if (!line->tokens) return;
    }
    
    line->tokens[line->num_tokens].type = type;
    line->tokens[line->num_tokens].start = start;
    line->tokens[line->num_tokens].length = length;
    line->num_tokens++;
}

void tokenize_line(Editor *e, int row) {
    if (!e || row < 0 || row >= e->num_lines) return;
    
    Line *line = &e->lines[row];
    if (!line->chars || line->length == 0) {
        line->num_tokens = 0;
        line->tokens_valid = true;
        return;
    }
    
    line->num_tokens = 0;
    if (!line->tokens) {
        line->tokens = malloc(sizeof(Token) * 64);
        line->token_capacity = 64;
    }
    
    int i = 0;
    bool in_single_line_comment = false;
    bool in_multi_line_comment = false;
    bool in_string_double = false;
    bool in_string_single = false;
    int string_start = 0;
    
    while (i < line->length) {
        char c = line->chars[i];
        
        if (row > 0 && e->lines[row-1].tokens_valid) {
            Line *prev = &e->lines[row-1];
            if (prev->num_tokens > 0) {
                Token *last = &prev->tokens[prev->num_tokens - 1];
                if (last->type == TOKEN_COMMENT && 
                    last->start + last->length > prev->length) {
                    in_multi_line_comment = true;
                }
            }
        }
        
        if (in_multi_line_comment) {
            if (c == '*' && i + 1 < line->length && line->chars[i+1] == '/') {
                add_token(line, TOKEN_COMMENT, 0, i + 2);
                in_multi_line_comment = false;
                i += 2;
                continue;
            }
            i++;
            continue;
        }
        
        if (in_string_double) {
            if (c == '\\' && i + 1 < line->length) {
                i += 2;
                continue;
            }
            if (c == '"') {
                add_token(line, TOKEN_STRING, string_start, i - string_start + 1);
                in_string_double = false;
                i++;
                continue;
            }
            i++;
            continue;
        }
        
        if (in_string_single) {
            if (c == '\\' && i + 1 < line->length) {
                i += 2;
                continue;
            }
            if (c == '\'') {
                add_token(line, TOKEN_STRING, string_start, i - string_start + 1);
                in_string_single = false;
                i++;
                continue;
            }
            i++;
            continue;
        }
        
        if (in_single_line_comment) {
            add_token(line, TOKEN_COMMENT, i, line->length - i);
            break;
        }
        
        if (c == '/' && i + 1 < line->length && line->chars[i+1] == '/') {
            in_single_line_comment = true;
            add_token(line, TOKEN_COMMENT, i, line->length - i);
            break;
        }
        
        if (c == '/' && i + 1 < line->length && line->chars[i+1] == '*') {
            int start = i;
            i += 2;
            while (i < line->length) {
                if (line->chars[i] == '*' && i + 1 < line->length && 
                    line->chars[i+1] == '/') {
                    add_token(line, TOKEN_COMMENT, start, i - start + 2);
                    i += 2;
                    goto next;
                }
                i++;
            }
            add_token(line, TOKEN_COMMENT, start, line->length - start);
            in_multi_line_comment = true;
            break;
        }
        
        if (c == '"') {
            in_string_double = true;
            string_start = i;
            i++;
            continue;
        }
        if (c == '\'') {
            in_string_single = true;
            string_start = i;
            i++;
            continue;
        }
        
        if (c == '#' && (i == 0 || line->chars[i-1] == '\n')) {
            int start = i;
            while (i < line->length && !isspace(line->chars[i])) i++;
            add_token(line, TOKEN_PREPROCESSOR, start, i - start);
            continue;
        }
        
        if (isdigit(c) || (c == '.' && i + 1 < line->length && isdigit(line->chars[i+1]))) {
            int start = i;
            bool is_hex = (c == '0' && i + 1 < line->length && 
                          (line->chars[i+1] == 'x' || line->chars[i+1] == 'X'));
            bool is_bin = (c == '0' && i + 1 < line->length && 
                          (line->chars[i+1] == 'b' || line->chars[i+1] == 'B'));
            
            if (is_hex || is_bin) i += 2;
            
            while (i < line->length && (isxdigit(line->chars[i]) || 
                   line->chars[i] == '.' || line->chars[i] == 'x' || 
                   line->chars[i] == 'X' || line->chars[i] == 'f' ||
                   line->chars[i] == 'u' || line->chars[i] == 'l')) {
                i++;
            }
            add_token(line, TOKEN_NUMBER, start, i - start);
            continue;
        }
        
        if (isalpha(c) || c == '_') {
            int start = i;
            while (i < line->length && (isalnum(line->chars[i]) || line->chars[i] == '_')) i++;
            int len = i - start;
            
            if (i < line->length && line->chars[i] == '(') {
                add_token(line, TOKEN_FUNCTION, start, len);
            } else if (is_keyword(&line->chars[start], len)) {
                add_token(line, TOKEN_KEYWORD, start, len);
            } else if (is_type(&line->chars[start], len)) {
                add_token(line, TOKEN_TYPE, start, len);
            } else if (is_constant(&line->chars[start], len)) {
                add_token(line, TOKEN_CONSTANT, start, len);
            } else if (isupper(line->chars[start])) {
                add_token(line, TOKEN_MACRO, start, len);
            } else {
                add_token(line, TOKEN_VARIABLE, start, len);
            }
            continue;
        }
        
        if (strchr("+-*/%=<>!&|^~.:;,", c)) {
            int start = i;
            i++;
            if (i < line->length && strchr("=+-*/<>!&|", line->chars[i])) i++;
            add_token(line, TOKEN_OPERATOR, start, i - start);
            continue;
        }
        
        if (strchr("()[]{}<>", c)) {
            add_token(line, TOKEN_BRACKET, i, 1);
            i++;
            continue;
        }
        
        add_token(line, TOKEN_NORMAL, i, 1);
        i++;
        
        next:;
    }
    
    line->tokens_valid = true;
}

void highlight_syntax(Editor *e) {
    if (!e) return;
    
    for (int i = e->scroll_row; i < e->num_lines && i - e->scroll_row < e->screen_rows - 2; i++) {
        if (!e->lines[i].tokens_valid) {
            tokenize_line(e, i);
        }
    }
}
