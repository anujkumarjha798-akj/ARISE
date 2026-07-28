#include "arise.h"
#include <strings.h>

extern Editor editor;

static void autocomplete_word(Editor *e) {
    if (!e || e->cursor_row < 0 || e->cursor_row >= e->num_lines) return;
    
    Line *line = &e->lines[e->cursor_row];
    if (!line->chars || e->cursor_col == 0) return;
    
    int start = e->cursor_col - 1;
    while (start > 0 && isalnum(line->chars[start - 1])) {
        start--;
    }
    if (start == e->cursor_col) return;
    
    char prefix[256];
    int prefix_len = e->cursor_col - start;
    if (prefix_len >= 256) return;
    strncpy(prefix, &line->chars[start], prefix_len);
    prefix[prefix_len] = '\0';
    
    char best_match[256] = {0};
    int best_match_len = 0;
    
    for (int i = 0; i < e->num_lines; i++) {
        Line *l = &e->lines[i];
        if (!l->chars) continue;
        
        int pos = 0;
        while (pos < l->length) {
            while (pos < l->length && !isalnum(l->chars[pos]) && l->chars[pos] != '_') {
                pos++;
            }
            int word_start = pos;
            while (pos < l->length && (isalnum(l->chars[pos]) || l->chars[pos] == '_')) {
                pos++;
            }
            int word_len = pos - word_start;
            
            if (word_len == prefix_len && strncmp(&l->chars[word_start], prefix, prefix_len) == 0) {
                continue;
            }
            
            if (word_len > prefix_len && 
                strncasecmp(&l->chars[word_start], prefix, prefix_len) == 0) {
                
                if (best_match_len == 0 || word_len < best_match_len) {
                    strncpy(best_match, &l->chars[word_start], word_len);
                    best_match[word_len] = '\0';
                    best_match_len = word_len;
                }
            }
        }
    }
    
    if (best_match_len > prefix_len) {
        for (int i = 0; i < prefix_len; i++) {
            if (e->cursor_col > 0) {
                e->cursor_col--;
                delete_char(e);
            }
        }
        for (int i = 0; i < best_match_len; i++) {
            insert_char(e, best_match[i]);
        }
        snprintf(e->message, MESSAGE_SIZE, "Autocomplete: %s", best_match);
    } else {
        snprintf(e->message, MESSAGE_SIZE, "No match found for: %s", prefix);
    }
}

// Save confirmation dialog
SaveAction show_save_confirmation(Editor *e) {
    if (!e) return SAVE_ACTION_CANCEL;
    
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    int dialog_height = 8;
    int dialog_width = 50;
    int start_y = (rows - dialog_height) / 2;
    int start_x = (cols - dialog_width) / 2;
    
    WINDOW *dialog = newwin(dialog_height, dialog_width, start_y, start_x);
    box(dialog, 0, 0);
    
    wattron(dialog, A_BOLD);
    mvwprintw(dialog, 1, (dialog_width - 18) / 2, " Unsaved Changes ");
    wattroff(dialog, A_BOLD);
    
    mvwprintw(dialog, 3, 2, "Save changes before closing?");
    mvwprintw(dialog, 5, 5, "[Y] Yes  [N] No  [C] Cancel");
    wrefresh(dialog);
    
    int ch = wgetch(dialog);
    delwin(dialog);
    
    for (int i = start_y; i < start_y + dialog_height; i++) {
        move(i, start_x);
        for (int j = 0; j < dialog_width; j++) {
            addch(' ');
        }
    }
    refresh();
    
    switch (ch) {
        case 'y':
        case 'Y':
        case 13:
            return SAVE_ACTION_SAVE;
        case 'n':
        case 'N':
            return SAVE_ACTION_DONT_SAVE;
        case 'c':
        case 'C':
        case 27:
        default:
            return SAVE_ACTION_CANCEL;
    }
}

// Handle file close with confirmation
bool handle_file_close(Editor *e) {
    if (!e) return true;
    
    if (!e->modified) {
        return true;
    }
    
    SaveAction action = show_save_confirmation(e);
    
    switch (action) {
        case SAVE_ACTION_SAVE:
            if (e->filename) {
                if (save_file(e)) {
                    snprintf(e->message, MESSAGE_SIZE, "File saved and closed");
                    return true;
                } else {
                    snprintf(e->message, MESSAGE_SIZE, "Save failed! File not closed");
                    return false;
                }
            } else {
                snprintf(e->message, MESSAGE_SIZE, "Save As not implemented yet");
                return false;
            }
            
        case SAVE_ACTION_DONT_SAVE:
            snprintf(e->message, MESSAGE_SIZE, "Changes discarded");
            return true;
            
        case SAVE_ACTION_CANCEL:
        default:
            snprintf(e->message, MESSAGE_SIZE, "Close cancelled");
            return false;
    }
}

void handle_keypress(Editor *e, int ch) {
    memset(e->message, 0, sizeof(e->message));
    
    switch (ch) {
        case CTRL('q'):
            if (handle_file_close(e)) {
                e->running = false;
            }
            break;
            
        case CTRL('n'):
            if (handle_file_close(e)) {
                new_file(e);
            }
            break;
            
        case CTRL('a'):
            selection_select_all(e);
            break;
            
        case 3: // Ctrl+C
            copy_selection(e);
            break;
            
        case CTRL('x'):
            cut_selection(e);
            break;
            
        case CTRL('v'):
            if (selection_active(e)) {
                selection_delete_range(e);
            }
            paste_clipboard(e);
            break;
            
        case CTRL('d'):
            duplicate_line(e);
            break;
            
        case CTRL('l'):
            goto_line(e);
            break;
            
        case 0x1f:
            toggle_comment(e);
            break;
            
        case KEY_BACKSPACE:
        case 127:
            if (selection_active(e)) {
                selection_delete_range(e);
            } else {
                backspace_char(e);
            }
            break;
            
        case KEY_DC:
            if (selection_active(e)) {
                selection_delete_range(e);
            } else {
                delete_char(e);
            }
            break;
            
        case '\t':
            if (selection_active(e)) {
                selection_delete_range(e);
            } else {
                Line *line = &e->lines[e->cursor_row];
                if (e->cursor_col > 0 && isalnum(line->chars[e->cursor_col - 1])) {
                    autocomplete_word(e);
                } else {
                    insert_tab(e);
                }
            }
            break;
            
        case KEY_BTAB:
            remove_tab(e);
            break;
            
        case '\n':
        case '\r':
            if (selection_active(e)) {
                selection_delete_range(e);
            }
            insert_newline(e);
            break;
            
        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_HOME:
        case KEY_END:
        case KEY_PPAGE:
        case KEY_NPAGE:
            selection_clear(e);
            move_cursor(e, ch);
            break;
            
        case KEY_SLEFT:
            selection_start(e);
            move_cursor(e, KEY_LEFT);
            break;
            
        case KEY_SRIGHT:
            selection_start(e);
            move_cursor(e, KEY_RIGHT);
            break;
            
        case KEY_SR:
            selection_start(e);
            move_cursor(e, KEY_UP);
            break;
            
        case KEY_SF:
            selection_start(e);
            move_cursor(e, KEY_DOWN);
            break;
            
        case KEY_SHOME:
            selection_start(e);
            move_cursor(e, KEY_HOME);
            break;
            
        case KEY_SEND:
            selection_start(e);
            move_cursor(e, KEY_END);
            break;
            
        case KEY_MOUSE:
            handle_mouse_event(e);
            break;
            
        default:
            if (ch >= 32 && ch <= 126) {
                if (selection_active(e)) {
                    selection_delete_range(e);
                }
                insert_char(e, ch);
                e->modified = true;
            }
            break;
    }
}

void handle_mouse_event(Editor *e) {
    MEVENT event;
    if (getmouse(&event) != OK) return;
    if (!e || e->num_lines == 0) return;
    
    int gutter_width = editor_gutter_width(e);
    int visible_rows = e->screen_rows - 2;
    if (visible_rows < 0) visible_rows = 0;
    
    bool press = (event.bstate & BUTTON1_PRESSED) != 0;
    bool drag = (event.bstate & REPORT_MOUSE_POSITION) != 0;
    bool release = (event.bstate & BUTTON1_RELEASED) != 0;
    
    if (!press && !drag && !release) return;
    if (event.y < 0 || event.y >= visible_rows) return;
    
    int row = e->scroll_row + event.y;
    if (row < 0) row = 0;
    if (row >= e->num_lines) row = e->num_lines - 1;
    
    int col = event.x - gutter_width;
    if (col < 0) col = 0;
    if (col > e->lines[row].length) col = e->lines[row].length;
    
    if (press) {
        e->cursor_row = row;
        e->cursor_col = col;
        selection_clear(e);
        e->mouse_dragging = true;
    } else if (drag && e->mouse_dragging) {
        selection_start(e);
        e->cursor_row = row;
        e->cursor_col = col;
    }
    
    if (release) {
        e->mouse_dragging = false;
    }
}

void move_cursor(Editor *e, int key) {
    if (!e || e->num_lines == 0) return;
    
    switch (key) {
        case KEY_UP:
            if (e->cursor_row > 0) e->cursor_row--;
            break;
        case KEY_DOWN:
            if (e->cursor_row < e->num_lines - 1) e->cursor_row++;
            break;
        case KEY_LEFT:
            if (e->cursor_col > 0) {
                e->cursor_col--;
            } else if (e->cursor_row > 0) {
                e->cursor_row--;
                if (e->cursor_row >= 0 && e->cursor_row < e->num_lines) {
                    e->cursor_col = e->lines[e->cursor_row].length;
                }
            }
            break;
        case KEY_RIGHT:
            if (e->cursor_row < e->num_lines && 
                e->cursor_col < e->lines[e->cursor_row].length) {
                e->cursor_col++;
            } else if (e->cursor_row < e->num_lines - 1) {
                e->cursor_row++;
                e->cursor_col = 0;
            }
            break;
        case KEY_HOME:
            e->cursor_col = 0;
            break;
        case KEY_END:
            if (e->cursor_row >= 0 && e->cursor_row < e->num_lines) {
                e->cursor_col = e->lines[e->cursor_row].length;
            }
            break;
        case KEY_PPAGE:
            e->cursor_row -= (e->screen_rows - 3);
            if (e->cursor_row < 0) e->cursor_row = 0;
            break;
        case KEY_NPAGE:
            e->cursor_row += (e->screen_rows - 3);
            if (e->cursor_row >= e->num_lines) e->cursor_row = e->num_lines - 1;
            break;
    }
    
    if (e->cursor_row < 0) e->cursor_row = 0;
    if (e->cursor_row >= e->num_lines) e->cursor_row = e->num_lines - 1;
    
    if (e->cursor_row >= 0 && e->cursor_row < e->num_lines) {
        if (e->lines[e->cursor_row].chars) {
            if (e->cursor_col < 0) e->cursor_col = 0;
            if (e->cursor_col > e->lines[e->cursor_row].length) {
                e->cursor_col = e->lines[e->cursor_row].length;
            }
        } else {
            e->cursor_col = 0;
        }
    }
    
    if (e->cursor_row < e->scroll_row) {
        e->scroll_row = e->cursor_row;
    }
    if (e->cursor_row >= e->scroll_row + (e->screen_rows - 2)) {
        e->scroll_row = e->cursor_row - (e->screen_rows - 3);
    }
    if (e->scroll_row < 0) e->scroll_row = 0;
}
