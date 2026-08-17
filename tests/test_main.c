#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filter_engine.h"
#include "filter_parser.h"
#include "json_object.h"
#include "parser.h"

// Helper macro for assertions
#define ASSERT(condition, message)                                                          \
    do {                                                                                    \
        if (!(condition)) {                                                                 \
            fprintf(stderr, "Assertion failed in %s, function %s, line %d: %s\n", __FILE__, \
                    __func__, __LINE__, message);                                           \
            exit(1);                                                                        \
        }                                                                                   \
    } while (0)

#define ASSERT_EQ(a, b, message)                                                            \
    do {                                                                                    \
        if ((a) != (b)) {                                                                   \
            fprintf(stderr, "Assertion failed in %s, function %s, line %d: %s\n", __FILE__, \
                    __func__, __LINE__, message);                                           \
            fprintf(stderr, "  Expected: %lld\n", (long long)(b));                          \
            fprintf(stderr, "  Actual:   %lld\n", (long long)(a));                          \
            exit(1);                                                                        \
        }                                                                                   \
    } while (0)

// Helper: parsa una stringa JSON direttamente dalla memoria
static JsonValue* parse_from_string(const char* json_text) {
    FILE* fp = fmemopen((void*)json_text, strlen(json_text), "r");
    ASSERT(fp != NULL, "Failed to open memory stream");
    JsonValue* root = parse_json(fp);
    fclose(fp);
    return root;
}

void test_simple_json() {
    const char* text = "{ \"types\": \"of values\" }";
    JsonValue* root = parse_from_string(text);

    ASSERT(root != NULL, "Root should not be NULL");
    ASSERT(root->type == JSON_OBJECT, "Root must be an object");

    JsonValue* val = json_object_get(root->value.object, "types");
    ASSERT(val != NULL, "Key 'types' not found in object");
    ASSERT(val->type == JSON_STRING, "Value must be a string");
    ASSERT(strcmp(val->value.string, "of values") == 0, "Key \"types\" content is not correct!");

    json_value_free(root);
    printf("test_simple_json: PASSED\n");
}

void test_filter_identity() {
    const char* text = "{ \"name\": \"Mario\", \"age\": 30 }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // Il filtro "." deve restituire esattamente il nodo root
    Filter* filter = filter_compile(".");
    ASSERT(filter != NULL, "Identity filter is NULL!");
    JsonValue* result = filter_run(root, filter);
    ASSERT(result == root, "Identity filter '.' should return the root JsonValue itself");
    json_value_free(root);
    printf("test_filter_identity: PASSED\n");
}

void test_filter_simple_key() {
    const char* text = "{ \"name\": \"Luigi\", \"coins\": 100 }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // Test estrazione stringa con .name
    Filter* name_filter = filter_compile(".name");
    // usamo ->next per il check perché il primo filtro è sempre il filtro identità , che in fase
    // di esecuzione sarà eventualmente saltato
    ASSERT(name_filter->next->type == FILTER_FIELD, "Name filter should be field!!\n");
    JsonValue* name_res = filter_run(root, name_filter);
    ASSERT(name_res != NULL, "Filter '.name' returned NULL");
    ASSERT(name_res->type == JSON_STRING, "Value of '.name' must be a string");
    ASSERT(strcmp(name_res->value.string, "Luigi") == 0, "Value of '.name' is wrong");
    filter_free(name_filter);

    // Test estrazione numero con .coins
    Filter* coins_filter = filter_compile(".\"coins\"");
    JsonValue* coins_res = filter_run(root, coins_filter);

    ASSERT(coins_res != NULL, "Filter '.coins' returned NULL");
    ASSERT(coins_res->type == JSON_NUMBER, "Value of '.coins' must be a number");
    ASSERT_EQ((long long)coins_res->value.number, 100, "Value of '.coins' is wrong");
    filter_free(coins_filter);

    json_value_free(root);
    printf("test_filter_simple_key: PASSED\n");
}

void test_filter_missing_key() {
    const char* text = "{ \"name\": \"Bowser\" }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // Se la chiave non esiste, il lookup restituisce NULL
    Filter* missing_key_filter = filter_compile(".missing_key");
    JsonValue* result = filter_run(root, missing_key_filter);
    ASSERT(result == NULL, "Filter on missing key should return NULL");

    json_value_free(root);
    printf("test_filter_missing_key: PASSED\n");
}

void test_filter_invalid_syntax() {
    const char* text = "{ \"name\": \"Peach\" }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // Filtri che non iniziano con '.' o sintassi malformate

    Filter* empty_filter = filter_compile("");
    ASSERT(empty_filter == NULL, "Empty filter should fail");
    filter_free(empty_filter);
    Filter* no_dot_filter = filter_compile("name");
    ASSERT(no_dot_filter == NULL, "Filter without leading dot should fail");
    filter_free(no_dot_filter);

    Filter* malformed_filter = filter_compile("..");
    ASSERT(malformed_filter == NULL, "Malformed filter '..' should fail");
    filter_free(malformed_filter);

    json_value_free(root);
    printf("test_filter_invalid_syntax: PASSED\n");
}

int main() {
    printf("==========================================\n");
    printf("           RUNNING CCJQ UNIT TESTS        \n");
    printf("==========================================\n");

    test_simple_json();
    test_filter_identity();
    test_filter_simple_key();
    test_filter_missing_key();
    test_filter_invalid_syntax();

    printf("==========================================\n");
    printf("All unit tests passed successfully! 🎉\n");
    printf("==========================================\n");
    return 0;
}
