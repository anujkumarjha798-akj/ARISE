#include "arise.h"

Editor editor;

void init_dracula_theme(void) {
    init_pair(ARISE_PAIR_BG, DRACULA_FG, DRACULA_BG);
    init_pair(ARISE_PAIR_KEYWORD, DRACULA_KEYWORD, DRACULA_BG);
    init_pair(ARISE_PAIR_FUNC, DRACULA_FUNC, DRACULA_BG);
    init_pair(ARISE_PAIR_VAR, DRACULA_VAR, DRACULA_BG);
    init_pair(ARISE_PAIR_TYPE, DRACULA_TYPE, DRACULA_BG);
    init_pair(ARISE_PAIR_NUM, DRACULA_NUM, DRACULA_BG);
    init_pair(ARISE_PAIR_STRING, DRACULA_STRING, DRACULA_BG);
    init_pair(ARISE_PAIR_COMMENT, DRACULA_COMMENT, DRACULA_BG);
    init_pair(ARISE_PAIR_CONST, DRACULA_CONST, DRACULA_BG);
    init_pair(ARISE_PAIR_OPERATOR, DRACULA_OPERATOR, DRACULA_BG);
    init_pair(ARISE_PAIR_PREPROC, DRACULA_PREPROC, DRACULA_BG);
    init_pair(ARISE_PAIR_MACRO, DRACULA_MACRO, DRACULA_BG);
    init_pair(ARISE_PAIR_LINENUM, DRACULA_LINENUM, DRACULA_BG);
    init_pair(ARISE_PAIR_CURLINENUM, DRACULA_KEYWORD, DRACULA_CURLINE);
    init_pair(ARISE_PAIR_CURLINE, DRACULA_FG, DRACULA_CURLINE);
    init_pair(ARISE_PAIR_SELECTION, DRACULA_FG, DRACULA_SELECT);
    init_pair(ARISE_PAIR_STATUSBAR, DRACULA_STATFG, DRACULA_STATUSBG);
    init_pair(ARISE_PAIR_MSGBAR, DRACULA_STATFG, DRACULA_STATUSBG);
    init_pair(ARISE_PAIR_MATCHBRACKET, DRACULA_MATCHFG, DRACULA_MATCHBG);
    init_pair(ARISE_PAIR_RB1, DRACULA_RB1, DRACULA_BG);
    init_pair(ARISE_PAIR_RB2, DRACULA_RB2, DRACULA_BG);
    init_pair(ARISE_PAIR_RB3, DRACULA_RB3, DRACULA_BG);
    init_pair(ARISE_PAIR_RB4, DRACULA_RB4, DRACULA_BG);
    init_pair(ARISE_PAIR_RB5, DRACULA_RB5, DRACULA_BG);
    init_pair(ARISE_PAIR_RB6, DRACULA_RB6, DRACULA_BG);
    init_pair(ARISE_PAIR_RB7, DRACULA_RB7, DRACULA_BG);
    init_pair(ARISE_PAIR_RB8, DRACULA_RB8, DRACULA_BG);
    init_pair(ARISE_PAIR_INDENT, DRACULA_INDENT, DRACULA_BG);
}

static void ensure_config_dir(void) {
    const char *home = getenv("HOME");
    if (!home) return;
    
    char config_path[1024];
    snprintf(config_path, sizeof(config_path), "%s/.config", home);
    mkdir(config_path, 0755);
    
    snprintf(config_path, sizeof(config_path), "%s/.config/arise", home);
    mkdir(config_path, 0755);
}

void init_editor(Editor *e) {
    if (!e) return;
    
    ensure_config_dir();
    
    e->lines = malloc(sizeof(Line) * 256);
    if (!e->lines) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }
    e->num_lines = 1;
    e->capacity = 256;
    e->filename = NULL;
    e->filetype = strdup("text");
    e->modified = false;
    e->read_only = false;
    e->cursor_row = 0;
    e->cursor_col = 0;
    e->scroll_row = 0;
    e->scroll_col = 0;
    e->running = true;
    e->clipboard = NULL;
    e->show_line_numbers = true;
    e->show_indent_guides = true;
    e->tab_size = TAB_SIZE;
    e->use_spaces = true;
    strcpy(e->encoding, "UTF-8");
    e->has_selection = false;
    e->mouse_dragging = false;
    e->sel_anchor_row = 0;
    e->sel_anchor_col = 0;
    
    memset(e->message, 0, sizeof(e->message));
    memset(e->status, 0, sizeof(e->status));
    
    // 🔥 NEW: Default indentation config
    e->indent_config.indent_size = 2;
    e->indent_config.use_spaces = true;
    e->indent_config.auto_indent = true;
    e->indent_config.auto_format_on_save = true;
    e->indent_config.indent_chars = "  ";
    
    e->lines[0].chars = malloc(MAX_LINE_LENGTH);
    if (!e->lines[0].chars) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }
    e->lines[0].chars[0] = '\0';
    e->lines[0].length = 0;
    e->lines[0].capacity = MAX_LINE_LENGTH;
    e->lines[0].tokens = NULL;
    e->lines[0].num_tokens = 0;
    e->lines[0].token_capacity = 0;
    e->lines[0].tokens_valid = false;
}

void cleanup_editor(Editor *e) {
    if (!e) return;
    
    for (int i = 0; i < e->num_lines; i++) {
        free(e->lines[i].chars);
        free(e->lines[i].tokens);
    }
    free(e->lines);
    free(e->filename);
    free(e->filetype);
    free(e->clipboard);
    
    endwin();
}

void init_ncurses(void) {
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    start_color();
    use_default_colors();
    
    init_dracula_theme();
    
    mousemask(BUTTON1_PRESSED | BUTTON1_RELEASED | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    
    getmaxyx(stdscr, editor.screen_rows, editor.screen_cols);
}

void run_editor(Editor *e) {
    while (e->running) {
        highlight_syntax(e);
        draw_editor(e);
        int ch = getch();
        handle_keypress(e, ch);
    }
}
