#pragma once
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct JsonValue JsonValue;
typedef struct JsonObject JsonObject;

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

/// @brief Error codes for library "json_value"
typedef enum json_value_error {
    /// No error
    JSONVALUE_ERROR_OK = 0,
    /// Out of memory (RAM)
    JSONVALUE_ERROR_NOMEM,
    // etc etc
    /// Total # of errors in this list (NOT AN ACTUAL ERROR CODE);
    /// NOTE: that for this to work, it assumes your first error code is value 0 and you let it
    /// naturally increment from there, as is done above, without explicitly altering any error
    /// values above
    JSONVALUE_ERROR_COUNT,
} json_value_error_t;

typedef struct {
    JsonValue **items;
    size_t count;
    size_t capacity;
} JsonArray;

struct JsonValue {
    JsonType type;
    union {
        bool boolean;
        double number;
        char *string;
        JsonArray array;
        JsonObject *object;
    } value;
};

void json_value_free(JsonValue *val);
JsonArray init_array(int capacity);
JsonArray init_array(int capacity);
json_value_error_t add_element(JsonValue* val, JsonArray* arr);
