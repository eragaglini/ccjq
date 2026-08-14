#pragma once

#include <stddef.h> // size_t

// Forward declarations
typedef struct JsonValue JsonValue;
typedef struct JsonObject JsonObject;

/// @brief Error codes for library "json_object"
typedef enum json_object_error {
    JSONOBJECT_ERROR_OK = 0,
    JSONOBJECT_ERROR_NOMEM,
    JSONOBJECT_ERROR_DATA,
    JSONOBJECT_ERROR_COUNT,
} json_object_error_t;

JsonObject*         json_object_create(void);
json_object_error_t json_object_put(JsonObject* obj, const char* key, JsonValue* value);
JsonValue*          json_object_get(JsonObject* obj, const char* key);
void                json_object_free(JsonObject* obj);

// --- Funzioni per l'iterazione su JsonObject ---

/// Restituisce il numero di coppie chiave-valore presenti nell'oggetto
size_t              json_object_get_count(const JsonObject* obj);

/// Restituisce la chiave all'indice specificato (oppure NULL se fuori dai limiti)
const char*         json_object_get_key_at(const JsonObject* obj, size_t index);

/// Restituisce il valore all'indice specificato (oppure NULL se fuori dai limiti)
JsonValue*          json_object_get_value_at(const JsonObject* obj, size_t index);
