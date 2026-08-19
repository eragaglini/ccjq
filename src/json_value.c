#include "json_value.h"

#include <stdlib.h>

#include "json_object.h"
#include "json_value.h"

// Istanza con visibilità limitata a questa Translation Unit (static)
static JsonValue JSON_NULL_INSTANCE = { .type = JSON_NULL };

JsonValue* json_null_value(void) {
    return &JSON_NULL_INSTANCE;
}

void json_value_free(JsonValue* val) {
    if (!val || (val == &JSON_NULL_INSTANCE)) return;

    switch (val->type) {
        case JSON_STRING:
            free(val->value.string);
            break;

        case JSON_OBJECT:
            json_object_free(val->value.object);
            break;

        case JSON_ARRAY:
            for (size_t i = 0; i < val->value.array.count; i++) {
                json_value_free(val->value.array.items[i]);
            }
            free(val->value.array.items);  // Libera il buffer dell'array
            break;

        case JSON_NUMBER:
        case JSON_BOOL:
        case JSON_NULL:
            // Non hanno allocazioni interne su Heap
            break;
    }

    free(val);  // Libera la struct JsonValue stessa
}
