#include "print.h"

#include <stdio.h>

#include "json_object.h"

// Helper privato per stampare N spazi di indentazione
static void print_indent(FILE* stream, int level) {
    for (int i = 0; i < level; i++) {
        fprintf(stream, "  ");  // 2 spazi per ogni livello
    }
}

void json_value_print(FILE* stream, const JsonValue* val, int indent_level) {
    if (!val) {
        fprintf(stream, "null");
        return;
    }

    switch (val->type) {
        case JSON_NULL:
            fprintf(stream, "null");
            break;

        case JSON_BOOL:
            fprintf(stream, val->value.boolean ? "true" : "false");
            break;

        case JSON_NUMBER:
            // %g rimuove gli zeri decimali superflui
            fprintf(stream, "%g", val->value.number);
            break;

        case JSON_STRING:
            fprintf(stream, "\"%s\"", val->value.string);
            break;

        case JSON_ARRAY: {
            size_t count = val->value.array.count;
            if (count == 0) {
                fprintf(stream, "[]");
                break;
            }

            fprintf(stream, "[\n");
            for (size_t i = 0; i < count; i++) {
                print_indent(stream, indent_level + 1);
                json_value_print(stream, val->value.array.items[i], indent_level + 1);

                if (i < count - 1) {
                    fprintf(stream, ",");
                }
                fprintf(stream, "\n");
            }
            print_indent(stream, indent_level);
            fprintf(stream, "]");
            break;
        }

        case JSON_OBJECT: {
            JsonObject* obj = val->value.object;
            size_t count = json_object_get_count(obj);  // Aggiungi getter per il conteggio chiavi

            if (count == 0) {
                fprintf(stream, "{}");
                break;
            }

            fprintf(stream, "{\n");
            for (size_t i = 0; i < count; i++) {
                const char* key = json_object_get_key_at(obj, i);  // Helper per iterare le chiavi
                JsonValue* child = json_object_get_value_at(obj, i);

                print_indent(stream, indent_level + 1);
                fprintf(stream, "\"%s\": ", key);
                json_value_print(stream, child, indent_level + 1);

                if (i < count - 1) {
                    fprintf(stream, ",");
                }
                fprintf(stream, "\n");
            }
            print_indent(stream, indent_level);
            fprintf(stream, "}");
            break;
        }
    }
}

void json_value_dump(const JsonValue* val) {
    json_value_print(stdout, val, 0);
    printf("\n");
}
