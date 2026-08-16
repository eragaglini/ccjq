#include "filter_parser.h"

#include <stdio.h>
#include <stdlib.h>

#include "filter.h"
#include "filter_tokenizer.h"

static filter_parser_error_t compile_dot_filter(Filter* filter, FilterTokenizer* tokenizer_ptr) {
    FilterToken token = next_filter_token(tokenizer_ptr);
    Filter* next_step = NULL;

    switch (token.type) {
        case FILTER_TOKEN_STRING:
            next_step = (Filter*)calloc(1, sizeof(Filter));
            next_step->type = FILTER_FIELD;
            next_step->key = token.value;
            filter->next = next_step;
            break;

        case FILTER_TOKEN_LEFT_BRACKET:
            token = next_filter_token(tokenizer_ptr);
            if (token.type == FILTER_TOKEN_STRING) {
                next_step = (Filter*)calloc(1, sizeof(Filter));
                next_step->type = FILTER_FIELD;
                next_step->key = token.value; // Corretto qui
                filter->next = next_step;
            } else if (token.type == FILTER_TOKEN_NUMBER) {
                next_step = (Filter*)calloc(1, sizeof(Filter));
                char* endptr;
                next_step->type = FILTER_ARRAY_INDEX;
                next_step->index = strtoul(token.value, &endptr, 10);
                filter->next = next_step;
            } else {
                fprintf(stderr, "Errore di sintassi: attesa stringa o indice numerico dentro '[]'\n");
                return FILTER_PARSER_ERROR_SYNTAX;
            }

            // Consumiamo la parentesi chiusa ']'
            token = next_filter_token(tokenizer_ptr);
            if (token.type != FILTER_TOKEN_RIGHT_BRACKET) {
                fprintf(stderr, "Errore di sintassi: atteso ']' di chiusura\n");
                return FILTER_PARSER_ERROR_SYNTAX;
            }
            break;

        case FILTER_TOKEN_EOF:
            fprintf(stderr, "Errore di sintassi: necessario specificare chiave o indice dopo il punto\n");
            return FILTER_PARSER_ERROR_SYNTAX;

        default:
            fprintf(stderr, "Errore di sintassi: token inatteso dopo il punto\n");
            return FILTER_PARSER_ERROR_SYNTAX;
    }

    // Processa gli step successivi (. o |)
    token = next_filter_token(tokenizer_ptr);
    if (token.type == FILTER_TOKEN_DOT) {
        return compile_dot_filter(next_step, tokenizer_ptr);
    } else if (token.type == FILTER_TOKEN_PIPE) {
        token = next_filter_token(tokenizer_ptr);
        if (token.type == FILTER_TOKEN_DOT) {
            return compile_dot_filter(next_step, tokenizer_ptr);
        }
        fprintf(stderr, "Errore di sintassi: atteso '.' dopo il pipe '|'\n");
        return FILTER_PARSER_ERROR_SYNTAX;
    } else if (token.type != FILTER_TOKEN_EOF) {
        fprintf(stderr, "Errore di sintassi: token %d non valido prima di EOF\n", token.type);
        return FILTER_PARSER_ERROR_SYNTAX;
    }

    return FILTER_PARSER_ERROR_OK;
}

Filter* compile_filter(const char* filter_str) {
    if (filter_str == NULL || *filter_str == '\0') {
        return NULL;
    }

    FilterTokenizer tokenizer;
    init_filter_tokenizer(&tokenizer, filter_str);
    FilterToken tkn = next_filter_token(&tokenizer);

    if (tkn.type != FILTER_TOKEN_DOT) {
        fprintf(stderr, "Errore: l'espressione deve iniziare con '.'\n");
        return NULL;
    }

    Filter* step = (Filter*)calloc(1, sizeof(Filter));
    step->type = FILTER_IDENTITY;

    // Se la stringa era solo ".", ritorniamo il nodo identità
    if (filter_tokenizer_peek(&tokenizer) == '\0') {
        return step;
    }

    if (compile_dot_filter(step, &tokenizer) != FILTER_PARSER_ERROR_OK) {
        filter_free(step);
        return NULL;
    }

    return step;
}
