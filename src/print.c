#include "print.h"

#include <stdio.h>

#include "json_object.h"
#include "json_value.h"

// Helper privato per stampare N spazi di indentazione (2 spazi per livello)
static void print_indent(FILE* stream, int level) {
    for (int i = 0; i < level; i++) {
        fprintf(stream, "  ");
    }
}

void json_print_string(const char* str) {
    putchar('"');
    for (size_t i = 0; str[i] != '\0'; i++) {
        unsigned char c = (unsigned char)str[i];
        switch (c) {
            case '"':  fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\b': fputs("\\b", stdout);  break;
            case '\f': fputs("\\f", stdout);  break;
            case '\n': fputs("\\n", stdout);  break;
            case '\r': fputs("\\r", stdout);  break;
            case '\t': fputs("\\t", stdout);  break;
            default:
                if (c < 0x20) {
                    // Caratteri di controllo generici non stampabili in formato unicode escape
                    printf("\\u%04x", c);
                } else {
                    putchar(c);
                }
                break;
        }
    }
    putchar('"');
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
            // %g rimuove zeri superflui (es. 10 al posto di 10.000000)
            fprintf(stream, "%g", val->value.number);
            break;

        case JSON_STRING:
            json_print_string(val->value.string);
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
            const JsonObject* obj = val->value.object;
            if (!obj || obj->count == 0 || obj->head_order == NULL) {
                fprintf(stream, "{}");
                break;
            }

            fprintf(stream, "{\n");
            const JsonPair* curr = obj->head_order; // Inizio della sequenza cronologica
            while (curr != NULL) {
                print_indent(stream, indent_level + 1);
                fprintf(stream, "\"%s\": ", curr->key);
                json_value_print(stream, curr->val, indent_level + 1);

                // Mette la virgola solo se c'è un elemento successivo
                if (curr->next_order != NULL) {
                    fprintf(stream, ",");
                }
                fprintf(stream, "\n");

                curr = curr->next_order; // Avanza al prossimo nodo inserito
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
