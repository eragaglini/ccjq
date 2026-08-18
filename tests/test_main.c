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

void test_parse_nested_json() {
    const char* text =
        "{\n"
        "  \"user\": {\n"
        "    \"name\": \"Yoshi\",\n"
        "    \"friends\": [\"Mario\", \"Luigi\"]\n"
        "  }\n"
        "}";

    // 1. Parsing della radice
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root non deve essere NULL");
    ASSERT(root->type == JSON_OBJECT, "Root deve essere un JSON_OBJECT");

    // 2. Verifica oggetto "user"
    JsonValue* user = json_object_get(root->value.object, "user");
    ASSERT(user != NULL, "Campo 'user' non trovato");
    ASSERT(user->type == JSON_OBJECT, "'user' deve essere un JSON_OBJECT");

    // 3. Verifica stringa "name" -> "Yoshi"
    JsonValue* name = json_object_get(user->value.object, "name");
    ASSERT(name != NULL, "Campo 'name' non trovato dentro 'user'");
    ASSERT(name->type == JSON_STRING, "'name' deve essere un JSON_STRING");
    ASSERT(strcmp(name->value.string, "Yoshi") == 0, "Il valore di 'name' deve essere 'Yoshi'");

    // 4. Verifica array "friends"
    JsonValue* friends = json_object_get(user->value.object, "friends");
    ASSERT(friends != NULL, "Campo 'friends' non trovato dentro 'user'");
    ASSERT(friends->type == JSON_ARRAY, "'friends' deve essere un JSON_ARRAY");
    ASSERT_EQ(friends->value.array.count, 2, "L'array 'friends' deve contenere 2 elementi");

    // 5. Verifica elementi dell'array
    JsonValue* item0 = friends->value.array.items[0];
    ASSERT(item0 != NULL, "Elemento friends[0] non deve essere NULL");
    ASSERT(item0->type == JSON_STRING, "friends[0] deve essere JSON_STRING");
    ASSERT(strcmp(item0->value.string, "Mario") == 0, "friends[0] deve essere 'Mario'");

    JsonValue* item1 = friends->value.array.items[1];
    ASSERT(item1 != NULL, "Elemento friends[1] non deve essere NULL");
    ASSERT(item1->type == JSON_STRING, "friends[1] deve essere JSON_STRING");
    ASSERT(strcmp(item1->value.string, "Luigi") == 0, "friends[1] deve essere 'Luigi'");

    // 6. Pulizia della memoria
    json_value_free(root);
    printf("test_parse_nested_json: PASSED\n");
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

void test_filter_nested_and_chained() {
    const char* text =
        "{\n"
        "  \"user\": {\n"
        "    \"name\": \"Yoshi\",\n"
        "    \"friends\": [\"Mario\", \"Luigi\"]\n"
        "  }\n"
        "}";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // 1. Test chiavi concatenate (.user.name)
    Filter* nested_key = filter_compile(".user.name");
    ASSERT(nested_key != NULL, "Nested key filter failed to compile");
    JsonValue* res1 = filter_run(root, nested_key);
    ASSERT(res1 != NULL && res1->type == JSON_STRING, "Failed to retrieve .user.name");
    ASSERT(strcmp(res1->value.string, "Yoshi") == 0, "Value of .user.name is wrong");
    filter_free(nested_key);

    // 2. Test catena con array (.user.friends.[1] oppure .user.friends[1])
    Filter* nested_array = filter_compile(".user.friends[1]");
    ASSERT(nested_array != NULL, "Nested array filter failed to compile");
    JsonValue* res2 = filter_run(root, nested_array);
    ASSERT(res2 != NULL && res2->type == JSON_STRING, "Failed to retrieve .user.friends[1]");
    ASSERT(strcmp(res2->value.string, "Luigi") == 0, "Value of .user.friends[1] is wrong");
    filter_free(nested_array);

    json_value_free(root);
    printf("test_filter_nested_and_chained: PASSED\n");
}

int main() {
    printf("==========================================\n");
    printf("           RUNNING CCJQ UNIT TESTS        \n");
    printf("==========================================\n");

    // test_simple_json();
    // test_filter_identity();
    // test_filter_simple_key();
    // test_filter_missing_key();
    // test_filter_invalid_syntax();
    test_filter_nested_and_chained();

    printf("==========================================\n");
    printf("All unit tests passed successfully! 🎉\n");
    printf("==========================================\n");
    return 0;
}
