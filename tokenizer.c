#include "tokenizer.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Funzione helper per leggere il prossimo carattere dal file
static int advance(Tokenizer* tokenizer) {
    tokenizer->cursor = fgetc(tokenizer->stream);
    return tokenizer->cursor;
}

static void skip_whitespace(Tokenizer* tokenizer) {
    while (tokenizer->cursor == ' ' || tokenizer->cursor == '\t' || tokenizer->cursor == '\n' ||
           tokenizer->cursor == '\r') {
        advance(tokenizer);  // Avanza finché trova spazi
    }
}

void init_tokenizer(Tokenizer* tokenizer, FILE* fptr) {
    tokenizer->stream = fptr;
    advance(tokenizer);
}

static bool match_keyword(Tokenizer* tokenizer, const char* expected) {
    // expected è la parte Rimanente della parola (es. "rue" per "true")
    for (int i = 0; expected[i] != '\0'; i++) {
        if (advance(tokenizer) != expected[i]) {
            return false;
        }
    }
    advance(tokenizer);  // Avanziamo oltre l'ultimo carattere della parola
    return true;
}

static Token read_number(Tokenizer* tokenizer) {
    Token token;
    char buf[256];
    size_t len = 0;

// Helper locale per avanzare e contemporaneamente salvare nel buffer
#define CONSUME()                                                        \
    do {                                                                 \
        if (len < sizeof(buf) - 1) buf[len++] = (char)tokenizer->cursor; \
        advance(tokenizer);                                              \
    } while (0)

    // 1. Gestione segno o zero iniziale
    if (tokenizer->cursor == '0') {
        CONSUME();
        if (isdigit(tokenizer->cursor)) {
            token.type = TOKEN_ERROR;
            token.value = NULL;
            return token;
        }
    } else if (tokenizer->cursor == '-') {
        CONSUME();
        if (!isdigit(tokenizer->cursor)) {
            token.type = TOKEN_ERROR;
            token.value = NULL;
            return token;
        }
        while (isdigit(tokenizer->cursor)) {
            CONSUME();
        }
    } else {
        while (isdigit(tokenizer->cursor)) {
            CONSUME();
        }
    }

    // 2. Decimali
    if (tokenizer->cursor == '.') {
        CONSUME();
        if (!isdigit(tokenizer->cursor)) {
            token.type = TOKEN_ERROR;
            token.value = NULL;
            return token;
        }
        while (isdigit(tokenizer->cursor)) {
            CONSUME();
        }
    }

    // 3. Esponente
    if (tokenizer->cursor == 'e' || tokenizer->cursor == 'E') {
        CONSUME();
        if (tokenizer->cursor == '+' || tokenizer->cursor == '-') {
            CONSUME();
        }
        if (!isdigit(tokenizer->cursor)) {
            token.type = TOKEN_ERROR;
            token.value = NULL;
            return token;
        }
        while (isdigit(tokenizer->cursor)) {
            CONSUME();
        }
    }

#undef CONSUME

    buf[len] = '\0';
    token.type = TOKEN_NUMBER;
    token.value = strdup(buf);  // Salva il testo del numero (es. "123.45")
    return token;
}

Token next_token(Tokenizer* tokenizer) {
    // 1. Ignora gli spazi vuoti accumulati prima del token
    skip_whitespace(tokenizer);

    Token token;
    token.value = NULL;  // Default

    // 2. Gestione Fine File (EOF)
    if (tokenizer->cursor == EOF) {
        token.type = TOKEN_EOF;
        return token;
    }

    // 3. Riconosci il token in base al carattere attuale
    switch (tokenizer->cursor) {
        case '{':
            token.type = TOKEN_LEFT_BRACE;
            advance(tokenizer);  // MANGIA il carattere '{' per preparare il cursor al
                                 // prossimo giro
            return token;

        case '}':
            token.type = TOKEN_RIGHT_BRACE;
            advance(tokenizer);  // MANGIA il carattere '}'
            return token;

        case '[':
            token.type = TOKEN_LEFT_BRACKET;
            advance(tokenizer);  // MANGIA il carattere '[' per preparare il cursor al
                                 // prossimo giro
            return token;

        case ']':
            token.type = TOKEN_RIGHT_BRACKET;
            advance(tokenizer);  // MANGIA il carattere ']'
            return token;

        case ':':
            token.type = TOKEN_COLON;
            advance(tokenizer);
            return token;

        case ',':
            token.type = TOKEN_COMMA;
            advance(tokenizer);
            return token;

        case '"': {
            advance(tokenizer);  // Saltiamo la virgoletta iniziale

            size_t capacity = 128;  // Capacità iniziale piccola per non sprecare RAM
            size_t len = 0;
            char* buf = (char*)malloc(capacity);
            if (!buf) {
                token.type = TOKEN_ERROR;
                token.value = NULL;
                return token;
            }

            while (tokenizer->cursor != '"') {
                if (tokenizer->cursor == EOF || (unsigned char)tokenizer->cursor < 32) {
                    free(buf);
                    token.type = TOKEN_ERROR;
                    token.value = NULL;
                    return token;
                }

                char c = (char)tokenizer->cursor;

                // Gestione escape '\'
                if (c == '\\') {
                    advance(tokenizer);
                    switch (tokenizer->cursor) {
                        case '"':
                            c = '"';
                            break;
                        case '\\':
                            c = '\\';
                            break;
                        case '/':
                            c = '/';
                            break;
                        case 'b':
                            c = '\b';
                            break;
                        case 'f':
                            c = '\f';
                            break;
                        case 'n':
                            c = '\n';
                            break;
                        case 'r':
                            c = '\r';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        default:
                            free(buf);
                            token.type = TOKEN_ERROR;
                            token.value = NULL;
                            return token;
                    }
                }

                // Se il buffer è pieno (considerando anche il '\0' finale), raddoppiamo la
                // capacità!
                if (len + 1 >= capacity) {
                    size_t new_cap = capacity * 2;
                    char* new_buf = (char*)realloc(buf, new_cap);
                    if (!new_buf) {
                        free(buf);
                        token.type = TOKEN_ERROR;
                        token.value = NULL;
                        return token;
                    }
                    buf = new_buf;
                    capacity = new_cap;
                }

                buf[len++] = c;
                advance(tokenizer);
            }

            advance(tokenizer);  // Saltiamo la virgoletta di chiusura '"'

            buf[len] = '\0';  // Terminatore di stringa
            token.type = TOKEN_STRING;
            token.value =
                buf;  // Passiamo direttamente il buffer allocato (senza bisogno di strdup!)
            return token;
        }

        case 't':
            token.type = match_keyword(tokenizer, "rue") ? TOKEN_TRUE : TOKEN_ERROR;
            return token;

        case 'f':
            token.type = match_keyword(tokenizer, "alse") ? TOKEN_FALSE : TOKEN_ERROR;
            return token;

        case 'n':
            token.type = match_keyword(tokenizer, "ull") ? TOKEN_NULL : TOKEN_ERROR;
            return token;

        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return read_number(tokenizer);

        default:
            token.type = TOKEN_ERROR;
            advance(tokenizer);
            return token;
    }
}
