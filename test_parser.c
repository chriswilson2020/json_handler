/* test_parser.c */
#include "json.h"


int main() {
    printf("JSON Parser Tests\n");
    printf("=================\n\n");

    // Test 1: Parse a simple string
    const char* test1 = "\"Hello, World!\"";
    printf("Test 1: Parsing string: %s\n", test1);
    JsonValue* value1 = json_parse_string(test1);
    if (value1 && value1->type == JSON_STRING) {
        printf("Success! Parsed string: %s\n", value1->value.string);
    } else {
        printf("Failed to parse string\n");
    }
    json_free(value1);

    // Test 2: Parse a number
    const char* test2 = "42.5";
    printf("\nTest 2: Parsing number: %s\n", test2);
    JsonValue* value2 = json_parse_string(test2);
    if (value2 && value2->type == JSON_NUMBER) {
        printf("Success! Parsed number: %.2f\n", value2->value.number);
    } else {
        printf("Failed to parse number\n");
    }
    json_free(value2);

    // Test 3: Parse and array
    const char* test3 = "[1, 2, 3, \"four\", true]";
    printf("\nTest 3: Parsing array: %s\n", test3);
    JsonValue* value3 = json_parse_string(test3);
    if (value3 && value3->type == JSON_ARRAY) {
        printf("Success! Parsed array with %zu elements\n", value3->value.array->size);
        json_print_value(value3, 0);
        printf("\n");
    } else {
        printf("Failed to parse array\n");
    }
    json_free(value3);

    // Test 4: Parse a complex object
    const char* test4 = "{"
        "\"name\": \"John Doe\","
        "\"age\": 20,"
        "\"is_student\": false,"
        "\"grades\": [85, 92, 78],"
        "\"address\": {"
            "\"street\": \"123 Main St\","
            "\"city\": \"Springfield\""
        "}"
    "}";
    printf("\nTest 4: Parsing complex object:\n%s\n", test4);
    JsonValue* value4 = json_parse_string(test4);
    if (value4 && value4->type == JSON_OBJECT) {
        printf("Success! Parsed object:\n");
        json_print_value(value4, 0);
        printf("\n");
    } else { 
        printf("Failed to parse object\n");
    }
    json_free(value4);

    printf("\nAll tests completed!\n");
    return 0;

}