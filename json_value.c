#include "json_value.h"

#include <stdlib.h>

#include "json_object.h"
#include "json_value.h"

void json_value_free(JsonValue* val) {
    if (!val) return;

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

JsonArray init_array(int capacity) {
    JsonArray arr;
    arr.capacity = capacity;
    arr.count = 0;
    return arr;
}
