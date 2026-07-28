CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -flto
LDFLAGS = -lncurses
SRC = src/main.c src/editor.c src/input.c src/buffer.c src/file.c src/clipboard.c src/syntax.c src/ui.c src/actions.c
OBJ = $(SRC:.c=.o)
TARGET = arise
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
CONFIGDIR = $(HOME)/.config/arise

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "ARISE NEXT installed to $(DESTDIR)$(BINDIR)/$(TARGET)"
	@echo "Run 'arise' to start editing."

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	@echo "ARISE NEXT uninstalled from $(DESTDIR)$(BINDIR)/$(TARGET)"

.PHONY: all clean install uninstall
