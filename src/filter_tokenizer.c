#include "filter_tokenizer.h"

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Table look-up per classificazione rapida O(1) dei caratteri
static const unsigned char type[UCHAR_MAX + 1u] = {
    ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1, ['G'] = 1, ['H'] = 1,
    ['I'] = 1, ['J'] = 1, ['K'] = 1, ['L'] = 1, ['M'] = 1, ['N'] = 1, ['O'] = 1, ['P'] = 1,
    ['Q'] = 1, ['R'] = 1, ['S'] = 1, ['T'] = 1, ['U'] = 1, ['V'] = 1, ['W'] = 1, ['X'] = 1,
    ['Y'] = 1, ['Z'] = 1, ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1,
    ['g'] = 1, ['h'] = 1, ['i'] = 1, ['j'] = 1, ['k'] = 1, ['l'] = 1, ['m'] = 1, ['n'] = 1,
    ['o'] = 1, ['p'] = 1, ['q'] = 1, ['r'] = 1, ['s'] = 1, ['t'] = 1, ['u'] = 1, ['v'] = 1,
    ['w'] = 1, ['x'] = 1, ['y'] = 1, ['z'] = 1,
    ['_'] = 1,  // Supporto per identificatori snake_case
    ['"'] = 1, ['['] = 2, [']'] = 3, ['.'] = 4, ['|'] = 5, ['0'] = 6, ['1'] = 6, ['2'] = 6,
    ['3'] = 6, ['4'] = 6, ['5'] = 6, ['6'] = 6, ['7'] = 6, ['8'] = 6, ['9'] = 6, ['?'] = 7, ['\0'] = 8,};

static void advance_filter_tokenizer(FilterTokenizer* tokenizer) { tokenizer->cursor++; }

static char get_current_char(FilterTokenizer* tokenizer) {
    return tokenizer->filter_str[tokenizer->cursor];
}

static void skip_whitespace(FilterTokenizer* tokenizer) {
    char current = get_current_char(tokenizer);
    while (current == ' ' || current == '\t' || current == '\n' || current == '\r') {
        advance_filter_tokenizer(tokenizer);
        current = get_current_char(tokenizer);
    }
}

void init_filter_tokenizer(FilterTokenizer* tokenizer, const char* filter_str) {
    tokenizer->filter_str = filter_str;
    tokenizer->cursor = 0;
}

static FilterToken read_index(FilterTokenizer* tokenizer) {
    FilterToken token;
    char buf[256];
    size_t len = 0;
    char current = get_current_char(tokenizer);

#define CONSUME()                                                                  \
    do {                                                                           \
        if (len < sizeof(buf) - 1) buf[len++] = (char)get_current_char(tokenizer); \
        advance_filter_tokenizer(tokenizer);                                       \
        current = get_current_char(tokenizer);                                     \
    } while (0)

    if (current == '0') {
        CONSUME();
        if (isdigit((unsigned char)current)) {
            fprintf(stderr, "Errore: lo zero non può essere seguito da un numero!\n");
            token.type = FILTER_TOKEN_ERROR;
            token.value = NULL;
            return token;
        }
    } else {
        while (isdigit((unsigned char)current)) {
            CONSUME();
        }
    }

#undef CONSUME

    buf[len] = '\0';
    token.type = FILTER_TOKEN_NUMBER;
    token.value = strdup(buf);
    return token;
}

FilterToken next_filter_token(FilterTokenizer* tokenizer) {
    skip_whitespace(tokenizer);

    FilterToken token;
    token.value = NULL;
    char current = get_current_char(tokenizer);

    switch (type[(unsigned char)current]) {
        case 2:
            token.type = FILTER_TOKEN_LEFT_BRACKET;
            advance_filter_tokenizer(tokenizer);
            return token;

        case 3:
            token.type = FILTER_TOKEN_RIGHT_BRACKET;
            advance_filter_tokenizer(tokenizer);
            return token;

        case 4:
            token.type = FILTER_TOKEN_DOT;
            advance_filter_tokenizer(tokenizer);
            return token;

        case 5:
            token.type = FILTER_TOKEN_PIPE;
            advance_filter_tokenizer(tokenizer);
            return token;

        case 1: {
            bool is_quoted_str = (current == '"');
            if (is_quoted_str) {
                advance_filter_tokenizer(tokenizer);
            }

            size_t capacity = 128;
            size_t len = 0;
            char* buf = (char*)malloc(capacity);
            if (!buf) {
                fprintf(stderr, "Errore: memoria insufficiente!\n");
                token.type = FILTER_TOKEN_ERROR;
                token.value = NULL;
                return token;
            }

            current = get_current_char(tokenizer);
            while (((isalnum((unsigned char)current) || current == '_') && !is_quoted_str) ||
                   (current != '"' && is_quoted_str)) {
                if (is_quoted_str && (current == '\0' || (unsigned char)current < 32)) {
                    free(buf);
                    fprintf(stderr, "Errore: virgoletta '\"' non chiusa!\n");
                    token.type = FILTER_TOKEN_ERROR;
                    token.value = NULL;
                    return token;
                }

                if (is_quoted_str && current == '\\') {
                    advance_filter_tokenizer(tokenizer);
                    current = get_current_char(tokenizer);
                    switch (current) {
                        case '"':
                            current = '"';
                            break;
                        case '\\':
                            current = '\\';
                            break;
                        case '/':
                            current = '/';
                            break;
                        case 'b':
                            current = '\b';
                            break;
                        case 'f':
                            current = '\f';
                            break;
                        case 'n':
                            current = '\n';
                            break;
                        case 'r':
                            current = '\r';
                            break;
                        case 't':
                            current = '\t';
                            break;
                        default:
                            free(buf);
                            token.type = FILTER_TOKEN_ERROR;
                            token.value = NULL;
                            return token;
                    }
                }

                if (len + 1 >= capacity) {
                    size_t new_cap = capacity * 2;
                    char* new_buf = (char*)realloc(buf, new_cap);
                    if (!new_buf) {
                        free(buf);
                        token.type = FILTER_TOKEN_ERROR;
                        token.value = NULL;
                        return token;
                    }
                    buf = new_buf;
                    capacity = new_cap;
                }

                buf[len++] = current;
                advance_filter_tokenizer(tokenizer);
                current = get_current_char(tokenizer);
            }

            if (is_quoted_str) {
                advance_filter_tokenizer(tokenizer);
            }

            buf[len] = '\0';
            token.type = FILTER_TOKEN_STRING;
            token.value = buf;
            return token;
        }

        case 6:
            return read_index(tokenizer);

        case 7:
            token.type = FILTER_TOKEN_OPTIONAL;
            advance_filter_tokenizer(tokenizer);
            return token;

        case 8:
            token.type = FILTER_TOKEN_EOF;
            return token;

        default:
            fprintf(stderr, "Carattere '%c' non riconosciuto!\n", current);
            token.type = FILTER_TOKEN_ERROR;
            advance_filter_tokenizer(tokenizer);
            return token;
    }
}

char filter_tokenizer_peek(const FilterTokenizer* tokenizer) {
    if (tokenizer->filter_str == NULL) {
        return '\0';
    }
    return tokenizer->filter_str[tokenizer->cursor];
}
