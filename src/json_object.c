#include "json_object.h"

#include <stddef.h>  // ptrdiff_t
#include <stdio.h>   // fprintf, stderr
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // strcmp, strdup

#include "json_value.h"

typedef struct {
    char* key;
    JsonValue* val;
} JsonPair;

struct JsonObject {
    JsonPair* pairs;
    size_t count;
    size_t capacity;
};

// 1. Creazione dell'oggetto
JsonObject* json_object_create(void) {
    JsonObject* obj = (JsonObject*)malloc(sizeof(JsonObject));
    if (!obj) {
        fprintf(stderr, "Impossibile allocare memoria sufficiente per l'oggetto!\n");
        return NULL;
    }

    obj->count = 0;
    obj->capacity = 4;  // Capacità iniziale minima
    obj->pairs = (JsonPair*)malloc(obj->capacity * sizeof(JsonPair));
    if (!obj->pairs) {
        fprintf(stderr, "Memoria non sufficiente per istanziare %zu coppie chiave-valore!\n",
                obj->capacity);
        free(obj);
        return NULL;
    }
    return obj;
}

// Funzione helper privata (non visibile fuori da json_object.c)
static ptrdiff_t json_object_find_index(JsonObject* obj, const char* key) {
    if (!obj || !key) return -1;

    for (size_t i = 0; i < obj->count; i++) {
        if (strcmp(obj->pairs[i].key, key) == 0) {
            return (ptrdiff_t)i;
        }
    }
    return -1;
}

// 2. Ricerca O(N)
JsonValue* json_object_get(JsonObject* obj, const char* key) {
    ptrdiff_t index = json_object_find_index(obj, key);
    if (index >= 0) {
        return obj->pairs[index].val;
    }
    return NULL;
}

// 3. Inserimento con auto-resize e strdup()
json_object_error_t json_object_put(JsonObject* obj, const char* key, JsonValue* value) {
    if (!obj || !key) return JSONOBJECT_ERROR_DATA;

    // A. Se la chiave esiste già, aggiorniamo il valore
    ptrdiff_t index = json_object_find_index(obj, key);
    if (index >= 0) {
        json_value_free(obj->pairs[index].val);  // Libera il vecchio valore sovrascritto
        obj->pairs[index].val = value;
        return JSONOBJECT_ERROR_OK;
    }

    // B. Se l'array è pieno, raddoppiamo la capacità con realloc
    if (obj->count >= obj->capacity) {
        size_t new_capacity = obj->capacity * 2;
        JsonPair* new_pairs = (JsonPair*)realloc(obj->pairs, new_capacity * sizeof(JsonPair));
        if (!new_pairs) {
            fprintf(stderr, "Attenzione! La memoria non è sufficiente per riallocare le coppie!\n");
            return JSONOBJECT_ERROR_NOMEM;
        }

        obj->pairs = new_pairs;
        obj->capacity = new_capacity;
    }

    // C. Inseriamo la nuova coppia duplicando la chiave stringa
    obj->pairs[obj->count].key = strdup(key);  // Crea una copia indipendente
    if (!obj->pairs[obj->count].key) {
        return JSONOBJECT_ERROR_NOMEM;
    }
    obj->pairs[obj->count].val = value;
    obj->count++;
    return JSONOBJECT_ERROR_OK;
}

// 4. Deallocazione della memoria
void json_object_free(JsonObject* obj) {
    if (!obj) return;

    for (size_t i = 0; i < obj->count; i++) {
        free(obj->pairs[i].key);             // Libera la stringa della chiave
        json_value_free(obj->pairs[i].val);  // Libera ricorsivamente il valore JSON
    }
    free(obj->pairs);  // Libera l'array di coppie
    free(obj);         // Libera la struct container
}

// --- Funzioni per l'iterazione su JsonObject ---

size_t json_object_get_count(const JsonObject* obj) {
    if (!obj) return 0;
    return obj->count;
}

const char* json_object_get_key_at(const JsonObject* obj, size_t index) {
    if (!obj || index >= obj->count) {
        return NULL;
    }
    return obj->pairs[index].key;
}

JsonValue* json_object_get_value_at(const JsonObject* obj, size_t index) {
    if (!obj || index >= obj->count) {
        return NULL;
    }
    return obj->pairs[index].val;
}
