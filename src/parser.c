#include "parser.h"

#include <stdbool.h>
#include <stdio.h>  // sscanf
#include <stdlib.h>

#include "json_object.h"
#include "json_value.h"

static JsonValue* parse_value(Tokenizer* tknzptr, Token t) {
    // Token t = next_token(tknzptr);
    if (t.type == TOKEN_ERROR) return NULL;

    JsonValue* res = (JsonValue*)malloc(sizeof(JsonValue));
    if (!res) {
        fprintf(stderr, "Impossibile allocare memoria sufficiente!\n");
        return NULL;
    };
    // default null, se non viene parsato in nessun possibile json value
    // lo ritorniamo così e darà errore
    switch (t.type) {
        case TOKEN_LEFT_BRACE:
            res->type = JSON_OBJECT;
            res->value.object = parse_object(tknzptr);
            // se non è stato correttamente instanziato l'oggetto
            // ritorniamo subito NULL
            if (!res->value.object) {
                json_value_free(res);
                return NULL;
            }
            break;

        case TOKEN_LEFT_BRACKET:
            free(res);
            res = parse_array(tknzptr);
            break;

        case TOKEN_STRING:
            res->type = JSON_STRING;
            res->value.string = t.value;
            break;

        case TOKEN_NULL:
            res->type = JSON_NULL;
            break;

        case TOKEN_NUMBER:
            res->type = JSON_NUMBER;
            sscanf(t.value, "%lf", &res->value.number);
            break;

        case TOKEN_TRUE:
        case TOKEN_FALSE:
            res->type = JSON_BOOL;
            res->value.boolean = t.type == TOKEN_TRUE ? true : false;
            break;

        // Expecting 'STRING', 'NUMBER', 'NULL', 'TRUE', 'FALSE', '{', '[', got 'undefined'
        default:
            fprintf(stderr, "Token inaspettato!\n");
            break;
    }
    return res;
}

// La funzione di parsing prende semplicemente un FILE* generico
JsonValue* parse_json(FILE* stream) {
    Tokenizer tokenizer;
    init_tokenizer(&tokenizer, stream);
    Token t = next_token(&tokenizer);
    JsonValue* res = parse_value(&tokenizer, t);

    // dopo il parsing il file deve essere finito, altrimenti diamo un errore
    if (next_token(&tokenizer).type != TOKEN_EOF) {
        json_value_free(res);
        return NULL;
    }
    return res;
}

json_parser_error_t parse_and_add_key(Tokenizer* tokenizer, JsonObject* obj, const char* key) {
    // 1. Dobbiamo prima verificare e consumare i due punti ':'
    Token t = next_token(tokenizer);
    if (t.type != TOKEN_COLON) {
        json_object_free(obj);
        return JSONPARSER_ERROR_SYNTAX;  // o altro codice errore
    }

    // 2. Ora possiamo parsare il valore
    t = next_token(tokenizer);
    JsonValue* value = parse_value(tokenizer, t);
    if (!value) {
        json_object_free(obj);
        return JSONPARSER_ERROR_NOMEM;
    }

    if (json_object_set(obj, key, value) == JSONOBJECT_ERROR_NOMEM) {
        json_value_free(value);  // Ricordati di liberare value se la put fallisce!
        json_object_free(obj);
        return JSONPARSER_ERROR_NOMEM;
    }
    return JSONPARSER_ERROR_OK;
}
JsonObject* parse_object(Tokenizer* tokenizer) {
    JsonObject* res = json_object_create();
    // json_object_create() ha fallito, propaghiamo l'errore
    if (!res) return NULL;
    // accediamo
    Token t = next_token(tokenizer);
    if (t.type == TOKEN_RIGHT_BRACE) {
        return res;
    }
    // in un oggetto, la chiave deve essere sempre una stringa
    else if (t.type == TOKEN_STRING) {
        if (parse_and_add_key(tokenizer, res, t.value) != JSONPARSER_ERROR_OK) {
            // l'oggetto è già stato deallocato in parse_and_add_key()
            // posso ritornare null
            return NULL;
        }
        t = next_token(tokenizer);
        while (t.type == TOKEN_COMMA) {
            t = next_token(tokenizer);
            if (t.type != TOKEN_STRING) {
                fprintf(stderr, "Le chiavi di un JSON possono essere solo stringhe!\n");
                json_object_free(res);
                return NULL;
            }
            if (parse_and_add_key(tokenizer, res, t.value) != JSONPARSER_ERROR_OK) {
                // l'oggetto è già stato deallocato in parse_and_add_key()
                // posso ritornare null
                return NULL;
            }
            t = next_token(tokenizer);
        }
        if (t.type != TOKEN_RIGHT_BRACE) {
            fprintf(stderr, "Atteso: ':' o '}'\n");
            json_object_free(res);
            return NULL;
        }
    } else {
        fprintf(stderr, "Le chiavi di un JSON possono essere solo stringhe!\n");
        json_object_free(res);
        return NULL;
    }
    return res;
}

JsonValue* parse_array(Tokenizer* tokenizer) {
    // parse_array viene chiamata quando il token è '[',
    // lo mangiamo per andare avanti

    // 1. Allochiamo la struct JsonValue principale sullo Heap
    JsonValue* arr_value = (JsonValue*)malloc(sizeof(JsonValue));
    if (!arr_value) {
        fprintf(stderr, "Attenzione! Memoria insufficiente!\n");
        return NULL;
    };  // OOM (Out Of Memory)

    arr_value->type = JSON_ARRAY;
    arr_value->value.array.count = 0;
    arr_value->value.array.capacity = 4;  // Capacità iniziale
    arr_value->value.array.items =
        (JsonValue**)malloc(arr_value->value.array.capacity * sizeof(JsonValue*));

    if (!arr_value->value.array.items) {
        fprintf(stderr, "Attenzione! Memoria insufficiente per instanziare gli oggetti!\n");
        free(arr_value);
        return NULL;  // Fallimento allocazione buffer
    }

    // 2. Leggiamo il primo token dopo '['
    Token t = next_token(tokenizer);

    // Caso speciale: Array vuoto "[]"
    if (t.type == TOKEN_RIGHT_BRACKET) {
        return arr_value;  // Ritorna l'array vuoto valido!
    }

    // 3. Ciclo per leggere gli elementi
    while (true) {
        // Parsiamo il valore dell'elemento attuale
        JsonValue* element = parse_value(tokenizer, t);

        if (!element) {
            // ERRORE DI PARSING! Pulisci la memoria accumulata finora e ritorna NULL
            json_value_free(arr_value);
            fprintf(stderr, "Attenzione! Memoria insufficiente per instanziare gli oggetti!\n");
            return NULL;
        }

        // Ridimensiona se l'array è pieno
        if (arr_value->value.array.count >= arr_value->value.array.capacity) {
            size_t new_cap = arr_value->value.array.capacity * 2;
            JsonValue** new_items =
                (JsonValue**)realloc(arr_value->value.array.items, new_cap * sizeof(JsonValue*));

            if (!new_items) {
                json_value_free(element);
                json_value_free(arr_value);
                fprintf(stderr, "Attenzione! Memoria insufficiente per instanziare gli oggetti!\n");
                return NULL;  // OOM durante il realloc
            }
            arr_value->value.array.items = new_items;
            arr_value->value.array.capacity = new_cap;
        }

        // Aggiungiamo l'elemento
        arr_value->value.array.items[arr_value->value.array.count++] = element;

        // Verifichiamo la virgola ',' o la chiusura ']'
        t = next_token(tokenizer);
        if (t.type == TOKEN_RIGHT_BRACKET) {
            return arr_value;  // Successo!
        } else if (t.type != TOKEN_COMMA) {
            fprintf(stderr,
                    "Attenzione! Errore di sintassi! Attesa ',', Tipo di token ricevuto: %d!\n",
                    t.type);
            // Manca la virgola -> Errore di sintassi
            json_value_free(arr_value);
            return NULL;
        }

        t = next_token(tokenizer);
    }
}
