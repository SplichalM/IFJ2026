CC = gcc

CFLAGS = -std=gnu11 -Wall -Wextra -pedantic -g
CPPFLAGS = -Iinclude -I. -DIFJ_ENABLE_SEMANTICS

SOURCES = \
    src/main.c \
    src/lexer/main.c \
    src/parser/parser.c \
    src/parser/precedence.c \
    src/parser/stack.c \
    src/parser/ast.c \
    symtable.c \
    $(wildcard src/semantic/*.c)

HEADERS = $(wildcard include/*.h) symtable.h
TARGET = ifj26

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@

clean:
ifeq ($(OS),Windows_NT)
	if exist "$(TARGET)" del /Q "$(TARGET)"
	if exist "$(TARGET).exe" del /Q "$(TARGET).exe"
else
	$(RM) $(TARGET)
endif