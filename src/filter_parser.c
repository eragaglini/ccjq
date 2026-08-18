#include "filter_parser.h"

#include <stdio.h>
#include <stdlib.h>

#include "filter.h"
#include "filter_tokenizer.h"

// Forward declarations delle funzioni statiche interne
static filter_parser_error_t filter_compile_next(Filter* filter, FilterTokenizer* tokenizer,
                                                FilterToken* token);
static filter_parser_error_t filter_dot_compile(Filter* filter, FilterTokenizer* tokenizer);
static filter_parser_error_t filter_array_index_compile(Filter* filter, FilterTokenizer* tokenizer);

/**
 * Compila il passo/transizione successivo della pipeline in base al token ricevuto.
 */
static filter_parser_error_t filter_compile_next(Filter* filter, FilterTokenizer* tokenizer,
                                                 FilterToken* token) {
    switch (token->type) {
        case FILTER_TOKEN_DOT:
            return filter_dot_compile(filter, tokenizer);

        case FILTER_TOKEN_LEFT_BRACKET:
            return filter_array_index_compile(filter, tokenizer);

        case FILTER_TOKEN_PIPE: {
            FilterToken next_tok = next_filter_token(tokenizer);
            if (next_tok.type == FILTER_TOKEN_DOT) {
                return filter_dot_compile(filter, tokenizer);
            }
            if (next_tok.value != NULL) {
                free(next_tok.value);
            }
            fprintf(stderr, "Errore di sintassi: atteso '.' dopo il pipe '|'\n");
            return FILTER_PARSER_ERROR_SYNTAX;
        }

        case FILTER_TOKEN_EOF:
            return FILTER_PARSER_ERROR_OK;

        default:
            fprintf(stderr, "Errore di sintassi: token %d non valido prima di EOF\n", token->type);
            return FILTER_PARSER_ERROR_SYNTAX;
    }
}

/**
 * Gestisce l'accesso a un indice di array o a una chiave delimitata da quadre: [0], ["chiave"]
 */
static filter_parser_error_t filter_array_index_compile(Filter* filter,
                                                        FilterTokenizer* tokenizer) {
    FilterToken token = next_filter_token(tokenizer);
    Filter* next_step = (Filter*)calloc(1, sizeof(Filter));
    if (!next_step) {
        if (token.value != NULL) {
            free(token.value);
        }
        fprintf(stderr, "Attenzione! Memoria non sufficiente!\n");
        return FILTER_PARSER_ERROR_NOMEM;
    }

    if (token.type == FILTER_TOKEN_STRING) {
        next_step->type = FILTER_FIELD;
        next_step->key = token.value;  // Passaggio di ownership della stringa al nodo
        filter->next = next_step;
    } else if (token.type == FILTER_TOKEN_NUMBER) {
        char* endptr;
        next_step->type = FILTER_ARRAY_INDEX;
        next_step->index = strtoul(token.value, &endptr, 10);
        free(token.value);  // Liberiamo il buffer temporaneo allocato da strdup
        filter->next = next_step;
    } else {
        if (token.value != NULL) {
            free(token.value);
        }
        free(next_step);
        fprintf(stderr, "Errore di sintassi: attesa stringa o indice numerico dentro '[]'\n");
        return FILTER_PARSER_ERROR_SYNTAX;
    }

    // Consumiamo la parentesi chiusa ']'
    token = next_filter_token(tokenizer);
    if (token.type != FILTER_TOKEN_RIGHT_BRACKET) {
        if (token.value != NULL) {
            free(token.value);
        }
        fprintf(stderr, "Errore di sintassi: atteso ']' di chiusura\n");
        return FILTER_PARSER_ERROR_SYNTAX;
    }

    token = next_filter_token(tokenizer);
    filter_parser_error_t err = filter_compile_next(next_step, tokenizer, &token);
    if (token.value != NULL) {
        free(token.value);
    }
    return err;
}

/**
 * Gestisce l'accesso a un campo preceduto da punto: .chiave, ."chiave", .[0]
 */
static filter_parser_error_t filter_dot_compile(Filter* filter, FilterTokenizer* tokenizer) {
    FilterToken token = next_filter_token(tokenizer);
    Filter* next_step = NULL;

    switch (token.type) {
        case FILTER_TOKEN_STRING:
            next_step = (Filter*)calloc(1, sizeof(Filter));
            if (!next_step) {
                if (token.value != NULL) {
                    free(token.value);
                }
                return FILTER_PARSER_ERROR_NOMEM;
            }
            next_step->type = FILTER_FIELD;
            next_step->key = token.value;
            filter->next = next_step;
            break;

        case FILTER_TOKEN_LEFT_BRACKET:
            // Passiamo 'filter' genitore affinché la parentesi si agganci correttamente
            return filter_array_index_compile(filter, tokenizer);

        case FILTER_TOKEN_EOF:
            fprintf(stderr, "Errore di sintassi: necessario specificare chiave o indice dopo il punto\n");
            return FILTER_PARSER_ERROR_SYNTAX;

        default:
            if (token.value != NULL) {
                free(token.value);
            }
            fprintf(stderr, "Errore di sintassi: token inatteso dopo il punto\n");
            return FILTER_PARSER_ERROR_SYNTAX;
    }

    token = next_filter_token(tokenizer);
    filter_parser_error_t err = filter_compile_next(next_step, tokenizer, &token);
    if (token.value != NULL) {
        free(token.value);
    }
    return err;
}

Filter* filter_compile(const char* filter_str) {
    if (filter_str == NULL || *filter_str == '\0') {
        return NULL;
    }

    FilterTokenizer tokenizer;
    init_filter_tokenizer(&tokenizer, filter_str);
    FilterToken token = next_filter_token(&tokenizer);

    if (token.type != FILTER_TOKEN_DOT) {
        if (token.value != NULL) {
            free(token.value);
        }
        fprintf(stderr, "Errore: l'espressione deve iniziare con '.'\n");
        return NULL;
    }

    Filter* root_filter = (Filter*)calloc(1, sizeof(Filter));
    if (!root_filter) {
        fprintf(stderr, "Attenzione! Memoria non sufficiente!\n");
        return NULL;
    }
    root_filter->type = FILTER_IDENTITY;

    // Se l'espressione è unicamente ".", restituiamo subito l'identità
    if (filter_tokenizer_peek(&tokenizer) == '\0') {
        return root_filter;
    }

    if (filter_compile_next(root_filter, &tokenizer, &token) != FILTER_PARSER_ERROR_OK) {
        filter_free(root_filter);
        return NULL;
    }

    return root_filter;
}
