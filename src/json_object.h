#ifndef JSON_OBJECT_H
#define JSON_OBJECT_H

#include <stdbool.h>
#include <stddef.h>

// Forward declaration per evitare dipendenze circolari
typedef struct JsonValue JsonValue;
typedef struct JsonPair JsonPair;
typedef struct JsonObject JsonObject;

// Codici di errore per le operazioni sull'oggetto
typedef enum {
    JSONOBJECT_ERROR_OK = 0,
    JSONOBJECT_ERROR_NOMEM,
    JSONOBJECT_ERROR_DATA
} json_object_error_t;

/*
 * ==============================================================================
 * Struttura del singolo elemento (Coppia Chiave-Valore)
 * ==============================================================================
 *
 *              +-------------------------+
 *              |        JsonPair         |
 *              +-------------------------+
 *              | key: "nome"             |
 *              | val: (JsonValue*)       |
 *              +-------------------------+
 *              | next_hash  -----------> | (Punta al prossimo nodo con lo stesso hash)
 *              | next_order -----------> | (Punta alla chiave inserita cronologicamente DOPO)
 *              | prev_order -----------> | (Punta alla chiave inserita cronologicamente PRIMA)
 *              +-------------------------+
 */
struct JsonPair {
    char* key;
    JsonValue* val;
    JsonPair* next_hash;   // Catena per le collisioni nel bucket dell'Hash Table
    JsonPair* next_order;  // Ordine cronologico: elemento successivo
    JsonPair* prev_order;  // Ordine cronologico: elemento precedente
};

/*
 * ==============================================================================
 * Struttura principale della Hash Table Ordinata
 * ==============================================================================
 *
 *  JsonObject
 *  +-----------------------+
 *  | count: 2              |
 *  | capacity: 4           |
 *  | head_order --------+  |
 *  | tail_order -----+  |  |
 *  | pairs           |  |  |
 *  +---|-------------|--|--+
 *      |             |  |
 *      v             |  |
 *    [0] -> NULL     |  |
 *    [1] -> [Pair A]-+  |   (Pair A inserito per primo)
 *            |          |
 *            | (next_order)
 *            v          |
 *    [2] -> [Pair B]<---+   (Pair B inserito per secondo)
 *    [3] -> NULL
 */
struct JsonObject {
    JsonPair** pairs;       // Array dinamico di puntatori ai bucket (JsonPair*)
    size_t count;           // Numero di coppie chiave-valore presenti
    size_t capacity;        // Numero totale di bucket allocati

    JsonPair* head_order;   // Primo elemento inserito (testa per la stampa ordinata)
    JsonPair* tail_order;   // Ultimo elemento inserito (coda per append rapido)
};

// Funzioni pubbliche
JsonObject* json_object_create(void);
void json_object_free(JsonObject* obj);
JsonValue* json_object_get(const JsonObject* obj, const char* key);
json_object_error_t json_object_set(JsonObject* obj, const char* key, JsonValue* value);

#endif // JSON_OBJECT_H
