#include "arise.h"

extern Editor editor;

int editor_gutter_width(Editor *e) {
    if (!e) return 0;
    return e->show_line_numbers ? LINE_NUMBER_WIDTH : 0;
}

void draw_editor(Editor *e) {
    if (!e) return;
    
    getmaxyx(stdscr, e->screen_rows, e->screen_cols);
    
    int gutter_width = editor_gutter_width(e);
    int text_start_col = gutter_width;
    
    int sel_r1 = 0, sel_c1 = 0, sel_r2 = 0, sel_c2 = 0;
    bool has_sel = selection_get_range(e, &sel_r1, &sel_c1, &sel_r2, &sel_c2);
    int visible_rows = e->screen_rows - 2;
    if (visible_rows < 0) visible_rows = 0;
    
    bkgd(COLOR_PAIR(ARISE_PAIR_BG));
    erase();
    
    for (int screen_row = 0; screen_row < visible_rows; screen_row++) {
        int line_index = e->scroll_row + screen_row;
        bool is_current = (line_index < e->num_lines && line_index == e->cursor_row);
        bool valid_line = (line_index < e->num_lines);
        
        if (e->show_line_numbers) {
            attron(COLOR_PAIR(ARISE_PAIR_LINENUM));
            for (int x = 0; x < gutter_width - 1; x++) {
                mvaddch(screen_row, x, ' ');
            }
            attroff(COLOR_PAIR(ARISE_PAIR_LINENUM));
            
            attron(COLOR_PAIR(ARISE_PAIR_INDENT));
            mvaddch(screen_row, gutter_width - 1, ' ');
            attroff(COLOR_PAIR(ARISE_PAIR_INDENT));
            
            if (valid_line) {
                char num[16];
                snprintf(num, sizeof(num), "%*d", gutter_width - 1, line_index + 1);
                
                if (is_current) {
                    attron(COLOR_PAIR(ARISE_PAIR_CURLINENUM) | A_BOLD);
                } else {
                    attron(COLOR_PAIR(ARISE_PAIR_LINENUM) | A_DIM);
                }
                mvprintw(screen_row, 0, "%s", num);
                if (is_current) {
                    attroff(COLOR_PAIR(ARISE_PAIR_CURLINENUM) | A_BOLD);
                } else {
                    attroff(COLOR_PAIR(ARISE_PAIR_LINENUM) | A_DIM);
                }
            }
        }
        
        int text_pair = is_current ? ARISE_PAIR_CURLINE : ARISE_PAIR_BG;
        attron(COLOR_PAIR(text_pair));
        mvhline(screen_row, text_start_col, ' ', e->screen_cols - text_start_col);
        attroff(COLOR_PAIR(text_pair));
        
        if (!valid_line) continue;
        
        Line *line = &e->lines[line_index];
        if (!line || !line->chars) continue;
        
        if (e->show_indent_guides) {
            int spaces = 0;
            while (spaces < line->length && line->chars[spaces] == ' ') spaces++;
            for (int j = e->indent_config.indent_size; j < spaces; j += e->indent_config.indent_size) {
                int guide_col = text_start_col + j;
                if (guide_col < e->screen_cols) {
                    attron(COLOR_PAIR(ARISE_PAIR_INDENT));
                    mvaddch(screen_row, guide_col, '|');
                    attroff(COLOR_PAIR(ARISE_PAIR_INDENT));
                }
            }
        }
        
        if (line->tokens_valid && line->num_tokens > 0) {
            for (int j = 0; j < line->num_tokens; j++) {
                Token *tok = &line->tokens[j];
                int col = text_start_col + tok->start;
                
                if (col >= e->screen_cols) break;
                
                int pair_id = text_pair;
                int attr = 0;
                
                switch (tok->type) {
                    case TOKEN_KEYWORD:
                        pair_id = ARISE_PAIR_KEYWORD;
                        attr = A_BOLD;
                        break;
                    case TOKEN_FUNCTION:
                        pair_id = ARISE_PAIR_FUNC;
                        break;
                    case TOKEN_VARIABLE:
                        pair_id = ARISE_PAIR_VAR;
                        break;
                    case TOKEN_TYPE:
                        pair_id = ARISE_PAIR_TYPE;
                        break;
                    case TOKEN_NUMBER:
                        pair_id = ARISE_PAIR_NUM;
                        break;
                    case TOKEN_STRING:
                        pair_id = ARISE_PAIR_STRING;
                        break;
                    case TOKEN_COMMENT:
                        pair_id = ARISE_PAIR_COMMENT;
                        attr = A_DIM;
                        break;
                    case TOKEN_CONSTANT:
                        pair_id = ARISE_PAIR_CONST;
                        attr = A_BOLD;
                        break;
                    case TOKEN_OPERATOR:
                        pair_id = ARISE_PAIR_OPERATOR;
                        break;
                    case TOKEN_PREPROCESSOR:
                        pair_id = ARISE_PAIR_PREPROC;
                        attr = A_BOLD;
                        break;
                    case TOKEN_MACRO:
                        pair_id = ARISE_PAIR_MACRO;
                        break;
                    case TOKEN_BRACKET: {
                        int depth = 0;
                        if (strchr("([{<", line->chars[tok->start])) {
                            for (int k = 0; k <= j; k++) {
                                if (line->tokens[k].type == TOKEN_BRACKET) {
                                    char bk = line->chars[line->tokens[k].start];
                                    if (strchr("([{<", bk)) depth++;
                                }
                            }
                        } else {
                            for (int k = 0; k <= j; k++) {
                                if (line->tokens[k].type == TOKEN_BRACKET) {
                                    char bk = line->chars[line->tokens[k].start];
                                    if (strchr("([{<", bk)) depth++;
                                    else depth--;
                                }
                            }
                        }
                        depth = (depth - 1) % RAINBOW_LEVELS;
                        if (depth < 0) depth = 0;
                        pair_id = ARISE_PAIR_RB1 + depth;
                        break;
                    }
                    default:
                        break;
                }
                
                attron(COLOR_PAIR(pair_id) | attr);
                
                int len = tok->length;
                if (col + len > e->screen_cols) len = e->screen_cols - col;
                if (len > 0) {
                    mvprintw(screen_row, col, "%.*s", len, &line->chars[tok->start]);
                }
                attroff(COLOR_PAIR(pair_id) | attr);
            }
        } else {
            attron(COLOR_PAIR(text_pair));
            if (line->length > 0) {
                int len = line->length;
                int max_len = e->screen_cols - text_start_col;
                if (len > max_len) len = max_len;
                if (len > 0) {
                    mvprintw(screen_row, text_start_col, "%.*s", len, line->chars);
                }
            }
            attroff(COLOR_PAIR(text_pair));
        }
        
        if (has_sel && line_index >= sel_r1 && line_index <= sel_r2) {
            int start_col = (line_index == sel_r1) ? sel_c1 : 0;
            int end_col = (line_index == sel_r2) ? sel_c2 : line->length;
            if (end_col > line->length) end_col = line->length;
            
            if (start_col < end_col) {
                int screen_start = text_start_col + start_col;
                int width = end_col - start_col;
                if (screen_start < e->screen_cols) {
                    if (screen_start + width > e->screen_cols) {
                        width = e->screen_cols - screen_start;
                    }
                    if (width > 0) {
                        mvchgat(screen_row, screen_start, width, A_NORMAL,
                                ARISE_PAIR_SELECTION, NULL);
                    }
                }
            } else if (line_index != sel_r2 && text_start_col < e->screen_cols) {
                mvchgat(screen_row, text_start_col, 1, A_NORMAL,
                        ARISE_PAIR_SELECTION, NULL);
            }
        }
    }
    
    draw_status_bar(e);
    
    attron(COLOR_PAIR(ARISE_PAIR_MSGBAR));
    mvhline(e->screen_rows - 1, 0, ' ', e->screen_cols);
    if (strlen(e->message) > 0) {
        attron(A_BOLD);
        mvprintw(e->screen_rows - 1, 0, "%-*s", e->screen_cols, e->message);
        attroff(A_BOLD);
    } else {
        mvprintw(e->screen_rows - 1, 0, "%-*s", e->screen_cols, 
                 "Ctrl+Q:Quit(With Save)  Ctrl+C:Copy  Ctrl+V:Paste  Ctrl+A:Select All");
    }
    attroff(COLOR_PAIR(ARISE_PAIR_MSGBAR));
    
    int cursor_screen_row = e->cursor_row - e->scroll_row;
    int cursor_screen_col = e->cursor_col + text_start_col;
    
    if (cursor_screen_row < 0) cursor_screen_row = 0;
    if (cursor_screen_row >= visible_rows) cursor_screen_row = visible_rows - 1;
    if (cursor_screen_col < text_start_col) cursor_screen_col = text_start_col;
    if (cursor_screen_col >= e->screen_cols) cursor_screen_col = e->screen_cols - 1;
    
    move(cursor_screen_row, cursor_screen_col);
    curs_set(1);
    refresh();
}

void draw_line_numbers(Editor *e, int row, int screen_row) {
    (void)e;
    (void)row;
    (void)screen_row;
}

void draw_status_bar(Editor *e) {
    if (!e) return;
    
    attron(COLOR_PAIR(ARISE_PAIR_STATUSBAR) | A_REVERSE);
    mvhline(e->screen_rows - 2, 0, ' ', e->screen_cols);
    
    char left[256];
    snprintf(left, sizeof(left), " %s%s %s | Indent: %d spaces",
             e->filename ? e->filename : "[No Name]",
             e->modified ? " [+]" : "",
             e->read_only ? "[RO]" : "",
             e->indent_config.indent_size);
    
    char right[128];
    snprintf(right, sizeof(right), "%s | Ln %d, Col %d | Dracula ",
             e->filetype ? e->filetype : "text",
             e->cursor_row + 1,
             e->cursor_col + 1);
    
    mvprintw(e->screen_rows - 2, 0, "%s", left);
    if (e->screen_cols > (int)strlen(right) + 1) {
        mvprintw(e->screen_rows - 2, e->screen_cols - strlen(right), "%s", right);
    }
    
    attroff(COLOR_PAIR(ARISE_PAIR_STATUSBAR) | A_REVERSE);
}

void handle_resize(Editor *e) {
    if (!e) return;
    endwin();
    refresh();
    initscr();
    init_dracula_theme();
    
    getmaxyx(stdscr, e->screen_rows, e->screen_cols);
    
    for (int i = 0; i < e->num_lines; i++) {
        e->lines[i].tokens_valid = false;
    }
}
