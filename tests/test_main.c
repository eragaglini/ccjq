#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filter_engine.h"
#include "filter_parser.h"
#include "json_object.h"
#include "parser.h"

// Macro helper per le asserzioni
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

// Helper: effettua il parsing di un JSON in memoria
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
    ASSERT(strcmp(val->value.string, "of values") == 0, "Key 'types' content is not correct!");

    json_value_free(root);
    printf("test_simple_json: PASSED\n");
}

void test_filter_identity() {
    const char* text = "{ \"name\": \"Mario\", \"age\": 30 }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    Filter* filter = filter_compile(".");
    ASSERT(filter != NULL, "Identity filter is NULL!");
    JsonValue* result = filter_run(root, filter);
    ASSERT(result == root, "Identity filter '.' should return the root JsonValue itself");

    filter_free(filter);
    json_value_free(root);
    printf("test_filter_identity: PASSED\n");
}

void test_filter_simple_key() {
    const char* text = "{ \"name\": \"Luigi\", \"coins\": 100 }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // Test estrazione stringa con .name
    Filter* name_filter = filter_compile(".name");
    ASSERT(name_filter != NULL, "Filter '.name' compilation failed");
    JsonValue* name_res = filter_run(root, name_filter);
    ASSERT(name_res != NULL && name_res->type == JSON_STRING, "Value of '.name' must be a string");
    ASSERT(strcmp(name_res->value.string, "Luigi") == 0, "Value of '.name' is wrong");
    filter_free(name_filter);

    // Test estrazione numero con ."coins"
    Filter* coins_filter = filter_compile(".\"coins\"");
    ASSERT(coins_filter != NULL, "Filter '.\"coins\"' compilation failed");
    JsonValue* coins_res = filter_run(root, coins_filter);
    ASSERT(coins_res != NULL && coins_res->type == JSON_NUMBER, "Value of '.\"coins\"' must be a number");
    ASSERT_EQ((long long)coins_res->value.number, 100, "Value of '.\"coins\"' is wrong");
    filter_free(coins_filter);

    // Test chiave tra quadre .["name"]
    Filter* bracket_key = filter_compile(".[\"name\"]");
    ASSERT(bracket_key != NULL, "Filter '.[\"name\"]' compilation failed");
    JsonValue* bracket_res = filter_run(root, bracket_key);
    ASSERT(bracket_res != NULL && bracket_res->type == JSON_STRING, "Value of '.[\"name\"]' must be string");
    ASSERT(strcmp(bracket_res->value.string, "Luigi") == 0, "Value of '.[\"name\"]' is wrong");
    filter_free(bracket_key);

    json_value_free(root);
    printf("test_filter_simple_key: PASSED\n");
}

void test_filter_root_array_index() {
    const char* text = "[\"Apple\", \"Banana\", \"Cherry\"]";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // 1. Primo elemento: .[0]
    Filter* f0 = filter_compile(".[0]");
    ASSERT(f0 != NULL, "Filter '.[0]' compilation failed");
    JsonValue* res0 = filter_run(root, f0);
    ASSERT(res0 != NULL && res0->type == JSON_STRING, ".[0] should be a string");
    ASSERT(strcmp(res0->value.string, "Apple") == 0, ".[0] value is wrong");
    filter_free(f0);

    // 2. Terzo elemento: .[2]
    Filter* f2 = filter_compile(".[2]");
    ASSERT(f2 != NULL, "Filter '.[2]' compilation failed");
    JsonValue* res2 = filter_run(root, f2);
    ASSERT(res2 != NULL && res2->type == JSON_STRING, ".[2] should be a string");
    ASSERT(strcmp(res2->value.string, "Cherry") == 0, ".[2] value is wrong");
    filter_free(f2);

    // 3. Indice fuori dai limiti: .[10] -> NULL
    Filter* f_out = filter_compile(".[10]");
    ASSERT(f_out != NULL, "Filter '.[10]' compilation failed");
    JsonValue* res_out = filter_run(root, f_out);
    ASSERT(res_out == NULL, ".[10] out of bounds should return NULL");
    filter_free(f_out);

    json_value_free(root);
    printf("test_filter_root_array_index: PASSED\n");
}

void test_filter_missing_key() {
    const char* text = "{ \"name\": \"Bowser\" }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    Filter* missing_key_filter = filter_compile(".missing_key");
    ASSERT(missing_key_filter != NULL, "Filter compilation failed");
    JsonValue* result = filter_run(root, missing_key_filter);
    ASSERT(result->type == JSON_NULL, "Filter on missing key should return NULL");
    filter_free(missing_key_filter);

    json_value_free(root);
    printf("test_filter_missing_key: PASSED\n");
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

    // 1. Chiavi concatenate (.user.name)
    Filter* nested_key = filter_compile(".user.name");
    ASSERT(nested_key != NULL, "Nested key filter failed to compile");
    JsonValue* res1 = filter_run(root, nested_key);
    ASSERT(res1 != NULL && res1->type == JSON_STRING, "Failed to retrieve .user.name");
    ASSERT(strcmp(res1->value.string, "Yoshi") == 0, "Value of .user.name is wrong");
    filter_free(nested_key);

    // 2. Campo + Indice array (.user.friends[1])
    Filter* nested_array = filter_compile(".user.friends[1]");
    ASSERT(nested_array != NULL, "Nested array filter failed to compile");
    JsonValue* res2 = filter_run(root, nested_array);
    ASSERT(res2 != NULL && res2->type == JSON_STRING, "Failed to retrieve .user.friends[1]");
    ASSERT(strcmp(res2->value.string, "Luigi") == 0, "Value of .user.friends[1] is wrong");
    filter_free(nested_array);

    // 3. Concatenazione con Pipe (.user | .name)
    Filter* pipe_filter = filter_compile(".user | .name");
    ASSERT(pipe_filter != NULL, "Pipe filter failed to compile");
    JsonValue* res3 = filter_run(root, pipe_filter);
    ASSERT(res3 != NULL && res3->type == JSON_STRING, "Failed to evaluate .user | .name");
    ASSERT(strcmp(res3->value.string, "Yoshi") == 0, "Value of .user | .name is wrong");
    filter_free(pipe_filter);

    json_value_free(root);
    printf("test_filter_nested_and_chained: PASSED\n");
}

void test_filter_complex_array_objects() {
    const char* text =
        "[\n"
        "  { \"id\": 10, \"tags\": [\"admin\", \"staff\"] },\n"
        "  { \"id\": 20, \"tags\": [\"guest\"] }\n"
        "]";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // Test: .[0].tags[1] -> "staff"
    Filter* f1 = filter_compile(".[0].tags[1]");
    ASSERT(f1 != NULL, "Filter '.[0].tags[1]' failed to compile");
    JsonValue* r1 = filter_run(root, f1);
    ASSERT(r1 != NULL && r1->type == JSON_STRING, "Expected string for .[0].tags[1]");
    ASSERT(strcmp(r1->value.string, "staff") == 0, "Expected 'staff'");
    filter_free(f1);

    // Test: .[1].id -> 20
    Filter* f2 = filter_compile(".[1].id");
    ASSERT(f2 != NULL, "Filter '.[1].id' failed to compile");
    JsonValue* r2 = filter_run(root, f2);
    ASSERT(r2 != NULL && r2->type == JSON_NUMBER, "Expected number for .[1].id");
    ASSERT_EQ((long long)r2->value.number, 20, "Expected id 20");
    filter_free(f2);

    json_value_free(root);
    printf("test_filter_complex_array_objects: PASSED\n");
}

void test_filter_invalid_syntax() {
    const char* text = "{ \"name\": \"Peach\" }";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    Filter* empty_filter = filter_compile("");
    ASSERT(empty_filter == NULL, "Empty filter should fail");

    Filter* no_dot_filter = filter_compile("name");
    ASSERT(no_dot_filter == NULL, "Filter without leading dot should fail");

    Filter* malformed_filter = filter_compile("..");
    ASSERT(malformed_filter == NULL, "Malformed filter '..' should fail");

    json_value_free(root);
    printf("test_filter_invalid_syntax: PASSED\n");
}

void test_filter_optional_operator() {
    // 1. Test su tipo scalare (numero) con campo opzionale (.name?)
    const char* number_json = "123";
    JsonValue* root_num = parse_from_string(number_json);
    ASSERT(root_num != NULL, "Root should not be NULL");

    Filter* opt_field_filter = filter_compile(".name?");
    ASSERT(opt_field_filter != NULL, "Filter '.name?' failed to compile");
    JsonValue* res1 = filter_run(root_num, opt_field_filter);
    ASSERT(res1 == NULL, "Accessing field on number with '?' should return NULL without error");
    filter_free(opt_field_filter);
    json_value_free(root_num);

    // 2. Test su oggetto con indice di array opzionale (.[0]?)
    const char* obj_json = "{ \"user\": \"Mario\" }";
    JsonValue* root_obj = parse_from_string(obj_json);
    ASSERT(root_obj != NULL, "Root should not be NULL");

    Filter* opt_index_filter = filter_compile(".[0]?");
    ASSERT(opt_index_filter != NULL, "Filter '.[0]?' failed to compile");
    JsonValue* res2 = filter_run(root_obj, opt_index_filter);
    ASSERT(res2 == NULL, "Accessing index on object with '?' should return NULL without error");
    filter_free(opt_index_filter);
    json_value_free(root_obj);

    printf("test_filter_optional_operator: PASSED\n");
}

void test_filter_pipe_operator_edge_cases(void) {
    const char* text =
        "{\n"
        "  \"data\": {\n"
        "    \"items\": [{\"id\": 42, \"title\": \"Clean Code\"}]\n"
        "  }\n"
        "}";
    JsonValue* root = parse_from_string(text);
    ASSERT(root != NULL, "Root should not be NULL");

    // 1. Pipe multipli concatenati: .data | .items | .[0] | .title
    Filter* f_multi = filter_compile(".data | .items | .[0] | .title");
    ASSERT(f_multi != NULL, "Multi-pipe filter failed to compile");
    JsonValue* res_multi = filter_run(root, f_multi);
    ASSERT(res_multi != NULL && res_multi->type == JSON_STRING, "Multi-pipe result should be string");
    ASSERT(strcmp(res_multi->value.string, "Clean Code") == 0, "Multi-pipe extracted wrong value");
    filter_free(f_multi);

    // 2. Pipe con spaziature variabili
    Filter* f_spaces = filter_compile(".data    |    .items[0]   |   .id");
    ASSERT(f_spaces != NULL, "Spaced pipe filter failed to compile");
    JsonValue* res_spaces = filter_run(root, f_spaces);
    ASSERT(res_spaces != NULL && res_spaces->type == JSON_NUMBER, "Result should be number");
    ASSERT_EQ((long long)res_spaces->value.number, 42, "Value of id is wrong");
    filter_free(f_spaces);

    // 3. Sintassi non valida: Pipe sospeso a fine filtro
    Filter* f_trailing = filter_compile(".data |");
    ASSERT(f_trailing == NULL, "Trailing pipe should fail compilation");

    // 4. Sintassi non valida: Doppio pipe
    Filter* f_double = filter_compile(".data || .items");
    ASSERT(f_double == NULL, "Double pipe '||' should fail compilation");

    json_value_free(root);
    printf("test_filter_pipe_operator_edge_cases: PASSED\n");
}

int main() {
    printf("==========================================\n");
    printf("           RUNNING CCJQ UNIT TESTS        \n");
    printf("==========================================\n");

    test_simple_json();
    test_filter_identity();
    test_filter_simple_key();
    test_filter_root_array_index();
    test_filter_missing_key();
    test_filter_nested_and_chained();
    test_filter_complex_array_objects();
    test_filter_invalid_syntax();
    test_filter_optional_operator();
    test_filter_pipe_operator_edge_cases();

    printf("==========================================\n");
    printf("All unit tests passed successfully! 🎉\n");
    printf("==========================================\n");
    return 0;
}
