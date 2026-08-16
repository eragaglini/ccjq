#include "filter_engine.h"

#include <stdio.h>

#include "filter.h"
#include "json_object.h"

static JsonValue* run_filter_field(JsonValue* jv, const Filter* filter) {
    if (jv->type != JSON_OBJECT) {
        fprintf(stderr, "Errore: atteso oggetto JSON per il campo '%s'\n", filter->key);
        return NULL;
    }
    return json_object_get(jv->value.object, filter->key);
}

static JsonValue* run_filter_array_index(JsonValue* jv, const Filter* filter) {
    if (jv->type != JSON_ARRAY) {
        fprintf(stderr, "Errore: atteso array JSON per l'indice %zu\n", filter->index);
        return NULL;
    }

    // Protezione contro accessi fuori dai limiti dell'array
    if (filter->index >= jv->value.array.count) {
        return NULL;
    }

    return jv->value.array.items[filter->index];
}

JsonValue* run_filter(JsonValue* jv, Filter* filter) {
    // 1. Condizioni di terminazione/sicurezza
    if (filter == NULL) {
        return jv;
    }
    if (jv == NULL) {
        return NULL;
    }

    // 2. Valutazione del singolo step corrente
    JsonValue* step_result = NULL;
    switch (filter->type) {
        case FILTER_IDENTITY:
            step_result = jv;
            break;

        case FILTER_FIELD:
            step_result = run_filter_field(jv, filter);
            break;

        case FILTER_ARRAY_INDEX:
            step_result = run_filter_array_index(jv, filter);
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
    return run_filter(step_result, filter->next);
}
