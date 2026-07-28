// src/main.c
#include "arise.h"

int main(int argc, char *argv[]) {
    Editor editor;
    init_editor(&editor);
    
    if (argc > 1) {
        open_file(&editor, argv[1]);
    }
    
    init_ncurses();
    run_editor(&editor);
    cleanup_editor(&editor);
    
    return 0;
}
