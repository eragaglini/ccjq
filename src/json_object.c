#include "json_object.h"
#include "json_value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define LOAD_FACTOR_THRESHOLD_NUM 3
#define LOAD_FACTOR_THRESHOLD_DEN 4 // 3/4 = 0.75 (75% di riempimento massimo)

/*
 * Algoritmo di Hashing djb2 di Dan Bernstein:
 * Estremamente veloce ed efficace per stringhe di testo brevi come le chiavi JSON.
 */
static size_t hash_string(const char* str) {
    size_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

/*
 * Calcola l'indice del bucket all'interno dell'array pairs
 */
static size_t get_bucket_index(const JsonObject* obj, const char* key) {
    return hash_string(key) % obj->capacity;
}

/*
 * Inizializza un nuovo JsonObject allocando i bucket a NULL con calloc.
 */
JsonObject* json_object_create(void) {
    JsonObject* obj = (JsonObject*)malloc(sizeof(JsonObject));
    if (!obj) {
        fprintf(stderr, "Errore: memoria insufficiente per JsonObject\n");
        return NULL;
    }

    obj->count = 0;
    obj->capacity = INITIAL_CAPACITY;
    obj->head_order = NULL;
    obj->tail_order = NULL;

    // calloc inizializza automaticamente tutti i puntatori dell'array a NULL
    obj->pairs = (JsonPair**)calloc(obj->capacity, sizeof(JsonPair*));
    if (!obj->pairs) {
        fprintf(stderr, "Errore: memoria insufficiente per i bucket della tabella\n");
        free(obj);
        return NULL;
    }

    return obj;
}

/*
 * Cerca un valore dato il nome della chiave in tempo O(1) medio.
 */
JsonValue* json_object_get(const JsonObject* obj, const char* key) {
    if (!obj || !key || obj->capacity == 0) {
        return NULL;
    }

    size_t index = get_bucket_index(obj, key);
    JsonPair* curr = obj->pairs[index];

    // Scorre la lista delle collisioni solo per quel determinato bucket
    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            return curr->val;
        }
        curr = curr->next_hash;
    }

    return NULL; // Chiave non presente
}

/*
 * Ridimensiona la tabella hash raddoppiando i bucket e ricalcolando gli indici.
 * Nota: NON dobbiamo ricreare o riordinare i nodi: basta ricollegare i puntatori next_hash!
 */
static bool json_object_resize(JsonObject* obj) {
    size_t old_capacity = obj->capacity;
    size_t new_capacity = old_capacity * 2;

    JsonPair** new_pairs = (JsonPair**)calloc(new_capacity, sizeof(JsonPair*));
    if (!new_pairs) {
        return false;
    }

    JsonPair** old_pairs = obj->pairs;
    obj->pairs = new_pairs;
    obj->capacity = new_capacity;

    // Rimappa ogni nodo esistente nel suo nuovo bucket
    for (size_t i = 0; i < old_capacity; i++) {
        JsonPair* curr = old_pairs[i];
        while (curr != NULL) {
            JsonPair* next_in_old_bucket = curr->next_hash;

            // Ricalcola il bucket con la nuova capacità
            size_t new_index = get_bucket_index(obj, curr->key);

            // Inserimento in testa nel nuovo bucket
            curr->next_hash = obj->pairs[new_index];
            obj->pairs[new_index] = curr;

            curr = next_in_old_bucket;
        }
    }

    // Libera il vecchio array di puntatori (i nodi JsonPair rimangono intatti in memoria)
    free(old_pairs);
    return true;
}

/*
 * Inserisce una nuova coppia chiave-valore o aggiorna il valore se già esistente.
 */
json_object_error_t json_object_set(JsonObject* obj, const char* key, JsonValue* value) {
    if (!obj || !key || !value) {
        return JSONOBJECT_ERROR_DATA;
    }

    // 1. Controllo di esistenza (Aggiornamento O(1))
    size_t index = get_bucket_index(obj, key);
    JsonPair* curr = obj->pairs[index];

    while (curr != NULL) {
        if (strcmp(curr->key, key) == 0) {
            // Chiave già presente: sovrascriviamo il valore liberando il precedente
            json_value_free(curr->val);
            curr->val = value;
            return JSONOBJECT_ERROR_OK; // Il conteggio e l'ordine non cambiano
        }
        curr = curr->next_hash;
    }

    // 2. Controllo del Load Factor (count / capacity >= 0.75)
    if ((obj->count + 1) * LOAD_FACTOR_THRESHOLD_DEN > obj->capacity * LOAD_FACTOR_THRESHOLD_NUM) {
        if (!json_object_resize(obj)) {
            return JSONOBJECT_ERROR_NOMEM;
        }
        // Ricalcola l'indice perché la capacità è cambiata
        index = get_bucket_index(obj, key);
    }

    // 3. Creazione del nuovo nodo
    JsonPair* new_pair = (JsonPair*)malloc(sizeof(JsonPair));
    if (!new_pair) {
        return JSONOBJECT_ERROR_NOMEM;
    }

    new_pair->key = strdup(key);
    if (!new_pair->key) {
        free(new_pair);
        return JSONOBJECT_ERROR_NOMEM;
    }
    new_pair->val = value;

    // 4. Collegamento nella Hash Table (Separate Chaining: inserimento in testa)
    new_pair->next_hash = obj->pairs[index];
    obj->pairs[index] = new_pair;

    // 5. Collegamento nella Lista Cronologica (Append in coda)
    new_pair->next_order = NULL;
    new_pair->prev_order = obj->tail_order;

    if (obj->tail_order != NULL) {
        obj->tail_order->next_order = new_pair;
    } else {
        // È il primissimo elemento inserito nell'oggetto
        obj->head_order = new_pair;
    }
    obj->tail_order = new_pair;

    obj->count++;
    return JSONOBJECT_ERROR_OK;
}

/*
 * Dealloca completamente l'oggetto, le stringhe delle chiavi, i nodi e i valori.
 */
void json_object_free(JsonObject* obj) {
    if (!obj) return;

    // Per liberare la memoria possiamo scorrere semplicemente la lista cronologica
    JsonPair* curr = obj->head_order;
    while (curr != NULL) {
        JsonPair* next = curr->next_order;

        free(curr->key);
        json_value_free(curr->val);
        free(curr);

        curr = next;
    }

    // Libera l'array dei bucket e la struttura contenitore
    free(obj->pairs);
    free(obj);
}
