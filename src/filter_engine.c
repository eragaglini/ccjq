#include "filter_engine.h"

#include <stdio.h>

#include "filter.h"
#include "json_object.h"

static JsonValue* filter_run_field(JsonValue* val, const Filter* filter) {
    if (val->type != JSON_OBJECT) {
        if (!filter->suppress_error) {
            fprintf(stderr, "Errore: atteso oggetto JSON per il campo '%s'\n", filter->key);
        }
        return NULL;
    }
    JsonValue* res = json_object_get(val->value.object, filter->key);
    if (!res)
    {
        return json_null_value();
    }
    return res;
}

static JsonValue* filter_run_array_index(JsonValue* val, const Filter* filter) {
    if (val->type != JSON_ARRAY) {
        if (!filter->suppress_error) {
            fprintf(stderr, "Errore: atteso array JSON per l'indice %zu\n", filter->index);
        }
        return NULL;
    }

    // Protezione contro accessi fuori dai limiti dell'array
    if (filter->index >= val->value.array.count) {
        fprintf(stderr, "Errore: indice %d fuori dai limiti dell'array!\n", (int)filter->index);
        return NULL;
    }

    JsonValue* res = val->value.array.items[filter->index];
    if (!res)
    {
        return json_null_value();
    }
    return res;
}

JsonValue* filter_run(JsonValue* val, Filter* filter) {
    // 1. Condizioni di terminazione/sicurezza
    if (filter == NULL) {
        return val;
    }
    if (val == NULL) {
        return NULL;
    }

    // 2. Valutazione del singolo step corrente
    JsonValue* step_result = NULL;
    switch (filter->type) {
        case FILTER_IDENTITY:
            step_result = val;
            break;

        case FILTER_FIELD:
            step_result = filter_run_field(val, filter);
            break;

        case FILTER_ARRAY_INDEX:
            step_result = filter_run_array_index(val, filter);
            break;

        default:
            fprintf(stderr, "Errore: tipo di filtro sconosciuto (%d)\n", filter->type);
            return NULL;
    }

    // 3. Se il risultato dello step è NULL o la catena è finita, ci fermiamo
    if (step_result == NULL || filter->next == NULL) {
        return step_result;
    }

    // 4. Altrimenti procediamo ricorsivamente con il nodo successivo
    return filter_run(step_result, filter->next);
}
