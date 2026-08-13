# Nome dell'eseguibile
TARGET = ccjq

# Compilatore e Flag base
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11
LDFLAGS =

# Flag specifici per Release e Debug
RELEASE_CFLAGS = -O2
DEBUG_CFLAGS   = -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined
DEBUG_LDFLAGS  = -fsanitize=address -fsanitize=undefined

# Sorgenti e Oggetti
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# ----------------------------------------------------------------------
# Target Principali
# ----------------------------------------------------------------------

# 1. Build standard (Release)
all: CFLAGS += $(RELEASE_CFLAGS)
all: $(TARGET)

# 2. Build di Debug: esegue prima un 'clean' e aggiunge i flag di debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: LDFLAGS += $(DEBUG_LDFLAGS)
debug: clean $(TARGET)

# Link dell'eseguibile
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilazione dei singoli file sorgente
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Esecuzione dei test
test: $(TARGET)
	@chmod +x test.sh
	./test.sh

# Pulizia
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all debug clean test
