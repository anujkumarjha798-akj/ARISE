#include "arise.h"
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

extern Editor editor;

static int str_case_cmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static void generate_temp_filename(char *buf, size_t size, const char *filename) {
    time_t t = time(NULL);
    snprintf(buf, size, "%s.tmp.%ld", filename, t);
}

void detect_filetype(Editor *e) {
    if (!e || !e->filename) {
        e->filetype = strdup("text");
        detect_indentation(e);
        return;
    }
    
    char *ext = strrchr(e->filename, '.');
    if (!ext) {
        char *filename = strrchr(e->filename, '/');
        if (filename) filename++;
        else filename = e->filename;
        
        if (strcmp(filename, "Dockerfile") == 0) {
            e->filetype = strdup("dockerfile");
        } else if (strcmp(filename, "Makefile") == 0) {
            e->filetype = strdup("makefile");
        } else if (strcmp(filename, "Vagrantfile") == 0) {
            e->filetype = strdup("ruby");
        } else {
            e->filetype = strdup("text");
        }
        detect_indentation(e);
        return;
    }
    
    struct {
        char *ext;
        char *type;
    } filetypes[] = {
        {".c", "c"}, {".h", "c"}, {".cpp", "cpp"}, {".hpp", "cpp"},
        {".cc", "cpp"}, {".cxx", "cpp"}, {".go", "go"}, {".py", "python"},
        {".pyw", "python"}, {".sh", "bash"}, {".bash", "bash"},
        {".zsh", "zsh"}, {".fish", "fish"}, {".json", "json"},
        {".yaml", "yaml"}, {".yml", "yaml"}, {".md", "markdown"},
        {".markdown", "markdown"}, {".rs", "rust"}, {".java", "java"},
        {".js", "javascript"}, {".ts", "typescript"}, {".jsx", "javascript"},
        {".tsx", "typescript"}, {".php", "php"}, {".lua", "lua"},
        {".rb", "ruby"}, {".html", "html"}, {".css", "css"},
        {".xml", "xml"}, {".sql", "sql"}, {".toml", "toml"},
        {".ini", "ini"}, {".cfg", "ini"}, {".conf", "nginx"},
        {".tf", "terraform"}, {".swift", "swift"}, {".kt", "kotlin"},
        {".scala", "scala"}, {".dart", "dart"}, {".r", "r"},
        {".pl", "perl"}, {".pm", "perl"}, {".vim", "vim"},
        {NULL, NULL}
    };
    
    for (int i = 0; filetypes[i].ext; i++) {
        if (str_case_cmp(ext, filetypes[i].ext) == 0) {
            e->filetype = strdup(filetypes[i].type);
            detect_indentation(e);
            return;
        }
    }
    
    e->filetype = strdup("text");
    detect_indentation(e);
}

void open_file(Editor *e, const char *filename) {
    if (!e || !filename) return;
    
    struct stat st;
    int stat_result = stat(filename, &st);
    
    if (stat_result == 0) {
        if (S_ISDIR(st.st_mode)) {
            snprintf(e->message, MESSAGE_SIZE, 
                     "Error: '%s' is a directory, not a file", filename);
            new_file(e);
            return;
        }
        
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            snprintf(e->message, MESSAGE_SIZE, 
                     "Error: Cannot read file '%s'", filename);
            new_file(e);
            return;
        }
        
        for (int i = 0; i < e->num_lines; i++) {
            free(e->lines[i].chars);
            free(e->lines[i].tokens);
        }
        
        if (e->filename) free(e->filename);
        if (e->filetype) free(e->filetype);
        
        e->filename = strdup(filename);
        e->num_lines = 0;
        
        char line_buffer[MAX_LINE_LENGTH];
        while (fgets(line_buffer, MAX_LINE_LENGTH, fp)) {
            size_t len = strlen(line_buffer);
            if (len > 0 && line_buffer[len - 1] == '\n') {
                line_buffer[len - 1] = '\0';
                len--;
            }
            if (len > 0 && line_buffer[len - 1] == '\r') {
                line_buffer[len - 1] = '\0';
                len--;
            }
            
            if (e->num_lines >= e->capacity) {
                e->capacity *= 2;
                Line *new_lines = realloc(e->lines, sizeof(Line) * e->capacity);
                if (!new_lines) {
                    fclose(fp);
                    return;
                }
                e->lines = new_lines;
            }
            
            e->lines[e->num_lines].chars = strdup(line_buffer);
            e->lines[e->num_lines].length = len;
            e->lines[e->num_lines].capacity = len + 1;
            e->lines[e->num_lines].tokens = NULL;
            e->lines[e->num_lines].num_tokens = 0;
            e->lines[e->num_lines].token_capacity = 0;
            e->lines[e->num_lines].tokens_valid = false;
            e->num_lines++;
        }
        
        fclose(fp);
        detect_filetype(e);
        
        if (e->num_lines == 0) {
            e->lines[0].chars = malloc(MAX_LINE_LENGTH);
            if (!e->lines[0].chars) return;
            e->lines[0].chars[0] = '\0';
            e->lines[0].length = 0;
            e->lines[0].capacity = MAX_LINE_LENGTH;
            e->lines[0].tokens = NULL;
            e->lines[0].num_tokens = 0;
            e->lines[0].token_capacity = 0;
            e->lines[0].tokens_valid = false;
            e->num_lines = 1;
        }
        
        e->modified = false;
        e->has_selection = false;
        e->cursor_row = 0;
        e->cursor_col = 0;
        e->sel_anchor_row = 0;
        e->sel_anchor_col = 0;
        
        snprintf(e->message, MESSAGE_SIZE, "Opened: %s (%d lines, %s)", 
                 filename, e->num_lines, e->filetype);
        return;
    }
    
    if (errno == ENOENT) {
        char *last_slash = strrchr(filename, '/');
        if (last_slash) {
            char *parent = strdup(filename);
            if (!parent) {
                new_file(e);
                return;
            }
            parent[last_slash - filename] = '\0';
            
            struct stat parent_st;
            if (stat(parent, &parent_st) != 0 || !S_ISDIR(parent_st.st_mode)) {
                snprintf(e->message, MESSAGE_SIZE, 
                         "Error: Directory '%s' does not exist", parent);
                free(parent);
                new_file(e);
                return;
            }
            free(parent);
        }
        
        FILE *fp = fopen(filename, "w");
        if (!fp) {
            snprintf(e->message, MESSAGE_SIZE, 
                     "Error: Cannot create file '%s'", filename);
            new_file(e);
            return;
        }
        fclose(fp);
        chmod(filename, 0644);
        
        if (e->filename) free(e->filename);
        if (e->filetype) free(e->filetype);
        
        e->filename = strdup(filename);
        e->num_lines = 1;
        e->cursor_row = 0;
        e->cursor_col = 0;
        e->scroll_row = 0;
        e->scroll_col = 0;
        e->modified = false;
        e->has_selection = false;
        e->sel_anchor_row = 0;
        e->sel_anchor_col = 0;
        
        e->lines[0].chars = malloc(MAX_LINE_LENGTH);
        if (!e->lines[0].chars) return;
        e->lines[0].chars[0] = '\0';
        e->lines[0].length = 0;
        e->lines[0].capacity = MAX_LINE_LENGTH;
        e->lines[0].tokens = NULL;
        e->lines[0].num_tokens = 0;
        e->lines[0].token_capacity = 0;
        e->lines[0].tokens_valid = false;
        
        detect_filetype(e);
        
        snprintf(e->message, MESSAGE_SIZE, "Created new file: %s (%s)", 
                 filename, e->filetype);
        return;
    }
    
    snprintf(e->message, MESSAGE_SIZE, "Error: Cannot access '%s'", filename);
    new_file(e);
}

void new_file(Editor *e) {
    if (!e) return;
    
    for (int i = 0; i < e->num_lines; i++) {
        free(e->lines[i].chars);
        free(e->lines[i].tokens);
    }
    free(e->filename);
    free(e->filetype);
    
    e->filename = NULL;
    e->filetype = strdup("text");
    e->num_lines = 1;
    e->cursor_row = 0;
    e->cursor_col = 0;
    e->scroll_row = 0;
    e->scroll_col = 0;
    e->modified = false;
    e->has_selection = false;
    e->sel_anchor_row = 0;
    e->sel_anchor_col = 0;
    
    e->lines[0].chars = malloc(MAX_LINE_LENGTH);
    if (!e->lines[0].chars) return;
    e->lines[0].chars[0] = '\0';
    e->lines[0].length = 0;
    e->lines[0].capacity = MAX_LINE_LENGTH;
    e->lines[0].tokens = NULL;
    e->lines[0].num_tokens = 0;
    e->lines[0].token_capacity = 0;
    e->lines[0].tokens_valid = false;
    
    detect_indentation(e);
    e->modified = false;
    strcpy(e->message, "New file created");
}

// Save file - returns true if successful
bool save_file(Editor *e) {
    if (!e || !e->filename) return false;
    return save_file_as(e, e->filename);
}

bool save_file_as(Editor *e, const char *filename) {
    if (!e || !filename) return false;
    
    char tmp_name[1024];
    generate_temp_filename(tmp_name, sizeof(tmp_name), filename);
    
    for (int i = 0; tmp_name[i]; i++) {
        if (tmp_name[i] == ' ' || tmp_name[i] == '\t' || tmp_name[i] == '\n') {
            tmp_name[i] = '_';
        }
    }
    
    FILE *fp = fopen(tmp_name, "w");
    if (!fp) {
        snprintf(e->message, MESSAGE_SIZE, "Cannot save file: %s", filename);
        return false;
    }
    
    for (int i = 0; i < e->num_lines; i++) {
        if (e->lines[i].chars) {
            fwrite(e->lines[i].chars, 1, e->lines[i].length, fp);
        }
        if (i < e->num_lines - 1) fputc('\n', fp);
    }
    
    fclose(fp);
    chmod(tmp_name, 0644);
    
    if (rename(tmp_name, filename) != 0) {
        unlink(tmp_name);
        snprintf(e->message, MESSAGE_SIZE, "Cannot save file: %s", filename);
        return false;
    }
    
    if (e->filename) free(e->filename);
    e->filename = strdup(filename);
    detect_filetype(e);
    e->modified = false;
    snprintf(e->message, MESSAGE_SIZE, "Saved: %s (%d lines)", filename, e->num_lines);
    return true;
}
