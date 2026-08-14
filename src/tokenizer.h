#pragma once
#include <stdio.h>

typedef enum {
    TOKEN_LEFT_BRACE,     // {
    TOKEN_RIGHT_BRACE,    // }
    TOKEN_LEFT_BRACKET,   // [
    TOKEN_RIGHT_BRACKET,  // ]
    TOKEN_COLON,          // :
    TOKEN_COMMA,          // ,
    TOKEN_STRING,         // "stringa"
    TOKEN_NUMBER,         // 123.45
    TOKEN_TRUE,           // true
    TOKEN_FALSE,          // false
    TOKEN_NULL,           // null
    TOKEN_EOF,            // Fine file
    TOKEN_ERROR           // Errore di sintassi
} TokenType;

typedef struct {
    TokenType type;
    char* value;
} Token;

typedef struct {
    /* data */
    FILE* stream;
    int cursor;
} Tokenizer;

Token next_token(Tokenizer* tokenizer);

void init_tokenizer(Tokenizer* tokenizer, FILE* fptr);
