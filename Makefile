# Compilatore e Flag base
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c11
LDFLAGS =

# Flag specifici per Release e Debug
RELEASE_CFLAGS = -O2
DEBUG_CFLAGS   = -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined
DEBUG_LDFLAGS  = -fsanitize=address -fsanitize=undefined

# Cartelle
SRCDIR   = src
TESTSDIR = tests
BUILDDIR = build
BINDIR   = bin

# Eseguibili
TARGET      = $(BINDIR)/ccjq
TEST_TARGET = $(BINDIR)/test

# File Sorgente e Oggetto Applicazione
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Oggetti per i test (esclude main.o per evitare doppio main())
APP_OBJECTS  = $(filter-out $(BUILDDIR)/main.o, $(OBJS))
TEST_SOURCES = $(wildcard $(TESTSDIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TESTSDIR)/%.c=$(BUILDDIR)/test_%.o)

# ----------------------------------------------------------------------
# Target Principali
# ----------------------------------------------------------------------

# 1. Build standard (Release)
all: CFLAGS += $(RELEASE_CFLAGS)
all: $(TARGET)

# 2. Build di Debug (con AddressSanitizer)
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: LDFLAGS += $(DEBUG_LDFLAGS)
debug: clean $(TARGET) $(TEST_TARGET)

# Link dell'eseguibile principale
$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilazione sorgenti applicazione in build/
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

# ----------------------------------------------------------------------
# Test
# ----------------------------------------------------------------------

# Link dell'eseguibile di unit test
$(TEST_TARGET): $(TEST_OBJECTS) $(APP_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRCDIR) -o $@ $^ $(LDFLAGS)

# Compilazione sorgenti di test in build/
$(BUILDDIR)/test_%.o: $(TESTSDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

# Esegue solo gli unit test
unit_tests: $(TEST_TARGET)
	@./$(TEST_TARGET)

# Esegue solo i test di integrazione
integration_tests: $(TARGET)
	@chmod +x test.sh
	./test.sh

# Esegue tutti i test in sequenza
test: unit_tests integration_tests

# ----------------------------------------------------------------------
# Pulizia
# ----------------------------------------------------------------------
clean:
	rm -rf $(BUILDDIR) $(BINDIR)

.PHONY: all debug clean test unit_tests integration_tests
