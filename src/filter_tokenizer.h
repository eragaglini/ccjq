#pragma once
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    FILTER_TOKEN_DOT,            // .
    FILTER_TOKEN_LEFT_BRACKET,   // [
    FILTER_TOKEN_RIGHT_BRACKET,  // ]
    FILTER_TOKEN_IDENTIFIER,     // nome del campo (es. "types", "name")
    FILTER_TOKEN_STRING,         // stringa con apici (es. "key-with-dash")
    FILTER_TOKEN_NUMBER,         // indice numerico (es. 0, 12)
    FILTER_TOKEN_PIPE,           // |
    FILTER_TOKEN_OPTIONAL, // '?'
    FILTER_TOKEN_EOF,            // fine filtro
    FILTER_TOKEN_ERROR           // errore di sintassi
} FilterTokenType;

typedef struct {
    FilterTokenType type;
    char* value;             // Stringa allocata per IDENTIFIER/STRING o NULL per simboli singoli
} FilterToken;

typedef struct {
    const char* filter_str;
    size_t cursor;
} FilterTokenizer;

void init_filter_tokenizer(FilterTokenizer* tokenizer, const char* filter_str);
FilterToken next_filter_token(FilterTokenizer* tokenizer);
char filter_tokenizer_peek(const FilterTokenizer* tokenizer);
