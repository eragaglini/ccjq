#include "filter_tokenizer.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Funzione helper per leggere il prossimo carattere dal file
static void advance_filter_tokenizer(FilterTokenizer* tokenizer_ptr) { tokenizer_ptr->cursor++; }

static char get_current_char(FilterTokenizer* tokenizer_ptr) {
    return tokenizer_ptr->filter_str[tokenizer_ptr->cursor];
}

static void skip_whitespace(FilterTokenizer* tokenizer_ptr) {
    char current = get_current_char(tokenizer_ptr);
    while (current == ' ' || current == '\t' || current == '\n' || current == '\r') {
        advance_filter_tokenizer(tokenizer_ptr);  // Avanza finché trova spazi
        current = get_current_char(tokenizer_ptr);
    }
}

void init_filter_tokenizer(FilterTokenizer* tokenizer_ptr, const char* filter_str) {
    tokenizer_ptr->filter_str = filter_str;
    // a differenza del tokenizer per il JSON, dove potevamo fare fgetc, qui il cursore è
    // l'indice della filter string del tokenizer, pertanto lo inizializziamo a 0 qui e lo
    // avanzeremo solo successivamente
    tokenizer_ptr->cursor = 0;
    // advance_filter_tokenizer(tokenizer_ptr);
}

static FilterToken read_index(FilterTokenizer* tokenizer_ptr) {
    FilterToken token;
    char buf[256];
    size_t len = 0;
    char current = get_current_char(tokenizer_ptr);

// Helper locale per avanzare e contemporaneamente salvare nel buffer
#define CONSUME()                                                                      \
    do {                                                                               \
        if (len < sizeof(buf) - 1) buf[len++] = (char)get_current_char(tokenizer_ptr); \
        advance_filter_tokenizer(tokenizer_ptr);                                       \
        current = get_current_char(tokenizer_ptr);                                     \
    } while (0)

    // 1. Gestione segno o zero iniziale
    if (current == '0') {
        CONSUME();
        if (isdigit(current)) {
            fprintf(stderr, "Lo zero non può essere seguito da un numero!\n");
            token.type = FILTER_TOKEN_ERROR;
            token.value = NULL;
            return token;
        }
    }
    else {
        while (isdigit(current)) {
            CONSUME();
        }
    }

#undef CONSUME

    buf[len] = '\0';
    token.type = FILTER_TOKEN_NUMBER;
    token.value = strdup(buf);  // Salva il testo dell'indice (es. "0", "1", "2")
    return token;
}

FilterToken next_filter_token(FilterTokenizer* tokenizer_ptr) {
    // 1. Ignora gli spazi vuoti accumulati prima del token
    skip_whitespace(tokenizer_ptr);

    FilterToken token;
    token.value = NULL;  // Default
    char current = get_current_char(tokenizer_ptr);

    // 2. Gestione Fine File (EOF)
    if (current == EOF) {
        token.type = FILTER_TOKEN_EOF;
        return token;
    }


    // 3. Riconosci il token in base al carattere attuale
    switch (current) {
        case '[':
            token.type = FILTER_TOKEN_LEFT_BRACKET;
            advance_filter_tokenizer(tokenizer_ptr);  // MANGIA il carattere '[' per preparare il
                                                      // cursor al prossimo giro
            return token;

        case ']':
            token.type = FILTER_TOKEN_RIGHT_BRACKET;
            advance_filter_tokenizer(tokenizer_ptr);  // MANGIA il carattere ']'
            return token;

        case '.':
            token.type = FILTER_TOKEN_DOT;
            advance_filter_tokenizer(tokenizer_ptr);  // MANGIA il carattere '.'
            return token;

        case '|':
            token.type = FILTER_TOKEN_PIPE;
            advance_filter_tokenizer(tokenizer_ptr);  // MANGIA il carattere '|'
            return token;

        case '"': {
            advance_filter_tokenizer(tokenizer_ptr);  // Saltiamo la virgoletta iniziale

            size_t capacity = 128;  // Capacità iniziale piccola per non sprecare RAM
            size_t len = 0;
            char* buf = (char*)malloc(capacity);
            if (!buf) {
                fprintf(stderr, "Attenzione! Memoria insufficiente!\n");
                token.type = FILTER_TOKEN_ERROR;
                token.value = NULL;
                return token;
            }
            // advance_filter_tokenizer(tokenizer_ptr);
            // prendiamo il primo vero carattere, es.: 'n' in 'name'
            current = get_current_char(tokenizer_ptr);
            while (current != '"') {
                // se la virgoletta non viene chiusa, dobbiamo ritornare un errore
                if (current == '\0' || (unsigned char)current < 32) {
                    free(buf);
                    fprintf(stderr,"Attenzione! '\"' non è mai stata chiusa!\n");
                    token.type = FILTER_TOKEN_ERROR;
                    token.value = NULL;
                    return token;
                }

                // Gestione escape '\'
                if (current == '\\') {
                    advance_filter_tokenizer(tokenizer_ptr);
                    current = get_current_char(tokenizer_ptr);
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


                // Se il buffer è pieno (considerando anche il '\0' finale), raddoppiamo la
                // capacità!
                if (len + 1 >= capacity) {
                    size_t new_cap = capacity * 2;
                    char* new_buf = (char*)realloc(buf, new_cap);
                    if (!new_buf) {
                        fprintf(stderr, "Attenzione! Memoria insufficiente!\n");
                        free(buf);
                        token.type = FILTER_TOKEN_ERROR;
                        token.value = NULL;
                        return token;
                    }
                    buf = new_buf;
                    capacity = new_cap;
                }
                // andiamo a scrivere all'indice len il carattere corrente
                buf[len++] = current;
                // avanza, es.: 'a' in 'name'
                advance_filter_tokenizer(tokenizer_ptr);
                current = get_current_char(tokenizer_ptr);
            }

            advance_filter_tokenizer(tokenizer_ptr);  // Saltiamo la virgoletta di chiusura '"'

            buf[len] = '\0';  // Terminatore di stringa
            token.type = FILTER_TOKEN_STRING;
            token.value =
                buf;  // Passiamo direttamente il buffer allocato (senza bisogno di strdup!)
            return token;
        }

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
            return read_index(tokenizer_ptr);

        case '\0':
            token.type = FILTER_TOKEN_EOF;
            return token;


        default:
            fprintf(stderr, "Carattere %c non riconosciuto!\n", current);
            token.type = FILTER_TOKEN_ERROR;
            advance_filter_tokenizer(tokenizer_ptr);
            return token;
    }
}

char filter_tokenizer_peek(const FilterTokenizer* tokenizer) {
    if (tokenizer->filter_str == NULL) {
        return '\0';
    }
    // Legge il carattere corrente senza toccare il cursore
    return tokenizer->filter_str[tokenizer->cursor];
}
