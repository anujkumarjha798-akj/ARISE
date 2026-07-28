#ifndef ARISE_H
#define ARISE_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <time.h>

#define CTRL(x) ((x) & 0x1f)
#define MAX_LINES 100000
#define MAX_LINE_LENGTH 4096
#define TAB_SIZE 4
#define LINE_NUMBER_WIDTH 6
#define RAINBOW_LEVELS 8
#define MESSAGE_SIZE 512

typedef enum {
    SAVE_ACTION_SAVE,
    SAVE_ACTION_DONT_SAVE,
    SAVE_ACTION_CANCEL
} SaveAction;

typedef struct {
    int indent_size;
    bool use_spaces;
    bool auto_indent;
    bool auto_format_on_save;
    const char *indent_chars;
} IndentConfig;

#define DRACULA_BG       234
#define DRACULA_FG       189
#define DRACULA_KEYWORD  212
#define DRACULA_FUNC     84
#define DRACULA_VAR      189
#define DRACULA_TYPE     117
#define DRACULA_NUM      141
#define DRACULA_STRING   228
#define DRACULA_COMMENT  61
#define DRACULA_CONST    212
#define DRACULA_OPERATOR 212
#define DRACULA_LINENUM  243
#define DRACULA_CURLINE  236
#define DRACULA_SELECT   60
#define DRACULA_STATUSBG 61
#define DRACULA_STATFG   189
#define DRACULA_MATCHBG  236
#define DRACULA_MATCHFG  84
#define DRACULA_RB1 212
#define DRACULA_RB2 84
#define DRACULA_RB3 228
#define DRACULA_RB4 117
#define DRACULA_RB5 141
#define DRACULA_RB6 212
#define DRACULA_RB7 189
#define DRACULA_RB8 61
#define DRACULA_INDENT   60
#define DRACULA_PREPROC  141
#define DRACULA_MACRO    212

enum {
    ARISE_PAIR_BG = 1,
    ARISE_PAIR_KEYWORD,
    ARISE_PAIR_FUNC,
    ARISE_PAIR_VAR,
    ARISE_PAIR_TYPE,
    ARISE_PAIR_NUM,
    ARISE_PAIR_STRING,
    ARISE_PAIR_COMMENT,
    ARISE_PAIR_CONST,
    ARISE_PAIR_OPERATOR,
    ARISE_PAIR_LINENUM,
    ARISE_PAIR_CURLINE,
    ARISE_PAIR_SELECTION,
    ARISE_PAIR_STATUSBAR,
    ARISE_PAIR_MATCHBRACKET,
    ARISE_PAIR_RB1,
    ARISE_PAIR_RB2,
    ARISE_PAIR_RB3,
    ARISE_PAIR_RB4,
    ARISE_PAIR_RB5,
    ARISE_PAIR_RB6,
    ARISE_PAIR_RB7,
    ARISE_PAIR_RB8,
    ARISE_PAIR_INDENT,
    ARISE_PAIR_PREPROC,
    ARISE_PAIR_MACRO,
    ARISE_PAIR_CURLINENUM,
    ARISE_PAIR_MSGBAR
};

typedef enum {
    TOKEN_NORMAL,
    TOKEN_KEYWORD,
    TOKEN_TYPE,
    TOKEN_FUNCTION,
    TOKEN_VARIABLE,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_CONSTANT,
    TOKEN_OPERATOR,
    TOKEN_PREPROCESSOR,
    TOKEN_MACRO,
    TOKEN_BRACKET,
    TOKEN_SPECIAL
} TokenType;

typedef struct {
    TokenType type;
    int start;
    int length;
} Token;

typedef struct {
    char *chars;
    int length;
    int capacity;
    Token *tokens;
    int num_tokens;
    int token_capacity;
    bool tokens_valid;
} Line;

typedef struct {
    Line *lines;
    int num_lines;
    int capacity;
    char *filename;
    char *filetype;
    bool modified;
    bool read_only;
    int cursor_row;
    int cursor_col;
    int scroll_row;
    int scroll_col;
    int screen_rows;
    int screen_cols;
    bool running;
    char *clipboard;
    char message[MESSAGE_SIZE];
    char status[256];
    bool show_line_numbers;
    bool show_indent_guides;
    int tab_size;
    bool use_spaces;
    char encoding[32];
    bool has_selection;
    bool mouse_dragging;
    int sel_anchor_row;
    int sel_anchor_col;
    IndentConfig indent_config;
} Editor;

extern Editor editor;

// editor.c
void init_editor(Editor *e);
void cleanup_editor(Editor *e);
void init_ncurses(void);
void run_editor(Editor *e);
void init_dracula_theme(void);

// input.c
void handle_keypress(Editor *e, int ch);
void move_cursor(Editor *e, int key);
void handle_mouse_event(Editor *e);

// buffer.c
void insert_char(Editor *e, char c);
void delete_char(Editor *e);
void backspace_char(Editor *e);
void insert_newline(Editor *e);
void insert_tab(Editor *e);
void remove_tab(Editor *e);
void selection_clear(Editor *e);
void selection_start(Editor *e);
bool selection_active(Editor *e);
bool selection_get_range(Editor *e, int *r1, int *c1, int *r2, int *c2);
void selection_select_all(Editor *e);
void selection_delete_range(Editor *e);

// indentation functions
void detect_indentation(Editor *e);
int get_current_indent(Editor *e, int row);
int calculate_indent(Editor *e, int row);
void auto_indent_line(Editor *e, int row);
void fix_indentation(Editor *e);
void format_buffer(Editor *e);

// save confirmation
SaveAction show_save_confirmation(Editor *e);
bool handle_file_close(Editor *e);

// file.c
void open_file(Editor *e, const char *filename);
void new_file(Editor *e);
bool save_file(Editor *e);
bool save_file_as(Editor *e, const char *filename);
void detect_filetype(Editor *e);

// clipboard.c
void copy_selection(Editor *e);
void cut_selection(Editor *e);
void paste_clipboard(Editor *e);

// syntax.c
void tokenize_line(Editor *e, int row);
void highlight_syntax(Editor *e);

// ui.c
void draw_editor(Editor *e);
void handle_resize(Editor *e);
void draw_status_bar(Editor *e);
void draw_line_numbers(Editor *e, int row, int screen_row);
int editor_gutter_width(Editor *e);

// actions.c
void toggle_comment(Editor *e);
void duplicate_line(Editor *e);
void move_line_up(Editor *e);
void move_line_down(Editor *e);
void goto_line(Editor *e);

#endif
