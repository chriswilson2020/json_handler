#include <stdio.h>
#include "json.h"

void test_formatting_options(void) {
    printf("\nTesting JSON Formatting Options\n");
    printf("==============================\n\n");

    /* Create a complex test object */
    JsonValue* obj = json_create_object();
    json_object_set(obj, "string", json_create_string("Hello\nWorld"));
    json_object_set(obj, "number", json_create_number(123.456));
    
    JsonValue* array = json_create_array();
    json_array_append(array, json_create_number(1));
    json_array_append(array, json_create_number(2));
    json_array_append(array, json_create_number(3));
    json_object_set(obj, "array", array);
    
    JsonValue* nested = json_create_object();
    json_object_set(nested, "a", json_create_string("value"));
    json_object_set(nested, "b", json_create_boolean(1));
    json_object_set(obj, "nested", nested);

    /* Test default formatting */
    printf("Default formatting:\n");
    char* default_output = json_format_string(obj, &JSON_FORMAT_DEFAULT);
    printf("%s\n", default_output);
    free(default_output);

    /* Test compact formatting */
    printf("\nCompact formatting:\n");
    char* compact_output = json_format_string(obj, &JSON_FORMAT_COMPACT);
    printf("%s\n", compact_output);
    free(compact_output);

    /* Test pretty formatting */
    printf("\nPretty formatting:\n");
    char* pretty_output = json_format_string(obj, &JSON_FORMAT_PRETTY);
    printf("%s\n", pretty_output);
    free(pretty_output);

    /* Test custom formatting */
    JsonFormatConfig custom_config = {
        .indent_string = "\t",           /* Use tabs */
        .line_end = "\n",
        .spaces_after_colon = 2,         /* Extra spacing after colons */
        .spaces_after_comma = 2,         /* Extra spacing after commas */
        .max_inline_length = 40,         /* Short lines */
        .number_format = JSON_NUMBER_FORMAT_SCIENTIFIC,
        .precision = 3,                  /* 3 decimal places */
        .inline_simple_arrays = 0,       /* Always break arrays */
        .sort_object_keys = 1            /* Sort keys */
    };

    printf("\nCustom formatting:\n");
    char* custom_output = json_format_string(obj, &custom_config);
    printf("%s\n", custom_output);
    free(custom_output);

    /* Test file output */
    printf("\nTesting file output... ");
    if (json_format_file(obj, "test_output.json", &JSON_FORMAT_PRETTY)) {
        printf("Success!\n");
    } else {
        printf("Failed!\n");
    }

    /* Clean up */
    json_free(obj);
}

/* Helper function to test validation and print results */
static void test_validation(const char* test_name, const char* json_input) {
    printf("\nValidation Test: %s\n", test_name);
    printf("Input: %s\n", json_input);

    if (json_validate_string(json_input)) {
        printf("Validation Result: Valid JSON\n");
    } else {
        /* Add a separate function to get validation errors */
        const JsonError* error = json_get_validation_error();
        printf("Validation Result: Invalid JSON\n");
        printf("Error at line %zu, column %zu\n", error->line, error->column);
        printf("Error: %s\n", error->message);
        printf("Context: %s\n", error->context);
    }
}



void test_json_validation(void) {
    printf("\nJSON Validation Test Suite\n");
    printf("=========================\n");

    /* Test successful cases */
    test_validation("Simple Object", "{\"name\":\"John\",\"age\":30}");
    test_validation("Simple Array", "[1,2,3,4,5]");
    test_validation("Mixed Types", "[null,true,42,\"hello\"]");
    test_validation("Nested Structures", 
                   "{\"user\":{\"name\":\"John\",\"scores\":[85,92,78]}}");
    test_validation("Empty Object", "{}");
    test_validation("Empty Array", "[]");
    test_validation("All Primitive Types", 
                   "{\"null\":null,\"bool\":true,\"num\":42,\"str\":\"text\"}");
    test_validation("String Escapes", 
                   "\"\\\"Hello\\nWorld\\\"\"");
    test_validation("Unicode", 
                   "\"Hello \\u0057orld\"");
    test_validation("Scientific Notation",
                   "[1.23e-4, -1.23E+4, 0.0e0]");

    /* Test error cases */
    test_validation("Error - NULL Input", NULL);
    test_validation("Error - Empty String", "");
    test_validation("Error - Invalid Token", "undefined");
    test_validation("Error - Incomplete Object", "{\"name\":}");
    test_validation("Error - Missing Quotes", "{name:\"John\"}");
    test_validation("Error - Trailing Comma", "[1,2,3,]");
    test_validation("Error - Invalid Number", "01234");
    test_validation("Error - Invalid Unicode", "\"\\u123g\"");
    test_validation("Error - Control Character", "\"\x01\"");
    test_validation("Error - Unterminated String", "\"Hello");
    test_validation("Error - Missing Comma", "[1 2 3]");
    test_validation("Error - Extra Content", "{\"name\":\"John\"} extra");
    test_validation("Error - Invalid Escape", "\"\\x\"");
    test_validation("Error - Incomplete Unicode", "\"\\u12\"");

    /* Test file validation */
    printf("\nTesting file validation:\n");
    if (json_validate_file("test.json")) {
        printf("test.json is valid JSON\n");
    } else {
        const JsonError* error = json_get_last_error();
        printf("test.json validation failed: %s\n", error->message);
    }

    /* Test with non-existent file */
    printf("\nTesting non-existent file validation:\n");
    if (json_validate_file("nonexistent.json")) {
        printf("nonexistent.json is valid JSON (unexpected!)\n");
    } else {
        const JsonError* error = json_get_last_error();
        printf("nonexistent.json validation failed (expected): %s\n", error->message);
    }
}

/* Helper function to run a test and report results */
static void run_test(const char* test_name, const char* json_input) {
    printf("\nTest: %s\n", test_name);
    printf("Input: %s\n", json_input);

    JsonValue* value = json_parse_string(json_input);

    if (value) {
        printf("Success! Parsed value:\n");
        json_print_value(value, 0);
        printf("\n");
        json_free(value);
    } else {
        const JsonError* error = json_get_last_error();
        printf("Parse failed at line %zu, column %zu\n", error->line, error->column);
        printf("Error: %s\n", error->message);
        printf("Context: %s\n", error->context);
    }
}

/* Helper function to generate deeply nested JSON */
static char* generate_nested_json(int depth) {
    /* Calculate required buffer size: 
       2 chars per level for brackets [] plus 1 for null terminator */
    size_t size = (depth * 2) + 1;
    char* json = (char*)malloc(size);
    if (!json) return NULL;
    
    /* Fill with opening brackets */
    for (int i = 0; i < depth; i++) {
        json[i] = '[';
    }
    
    /* Fill with closing brackets */
    for (int i = 0; i < depth; i++) {
        json[depth + i] = ']';
    }
    
    json[size - 1] = '\0';
    return json;
}

int main() {
    printf("Testing JSON Library Implementation\n");
    printf("===================================\n\n");

    /* Test 1: Object Creation and Manipulation */
    printf("Test 1: Creating a JSON object with various types\n");
    JsonValue* person = json_create_object();
    json_object_set(person, "name", json_create_string("John Doe"));
    json_object_set(person, "age", json_create_number(30));
    json_object_set(person, "is_student", json_create_boolean(1));
    json_object_set(person, "null_field", json_create_null());

    JsonValue* hobbies = json_create_array();
    json_array_append(hobbies, json_create_string("reading"));
    json_array_append(hobbies, json_create_string("hiking"));
    json_array_append(hobbies, json_create_string("photography"));
    json_object_set(person, "hobbies", hobbies);

    JsonValue* address = json_create_object();
    json_object_set(address, "street", json_create_string("123 Main st"));
    json_object_set(address, "city", json_create_string("Springfield"));
    json_object_set(address, "zip", json_create_string("12345"));
    json_object_set(person, "address", address);

    printf("\nComplete JSON structure:\n");
    json_print_value(person, 0);
    printf("\n\n");

    /* Test 2: Value Access */
    printf("Test 2: Accessing specific values\n");
    JsonValue* name = json_object_get(person, "name");
    JsonValue* age = json_object_get(person, "age");
    JsonValue* hobbies_array = json_object_get(person, "hobbies");

    printf("Name: ");
    json_print_value(name, 0);
    printf("\nAge: ");
    json_print_value(age, 0);
    printf("\nFirst hobby: ");
    json_print_value(json_array_get(hobbies_array, 0), 0);
    printf("\n\n");

    /* Test 3: Value Updates */
    printf("Test 3: Updating values\n");
    json_object_set(person, "age", json_create_number(31));
    printf("Updated age: ");
    json_print_value(json_object_get(person, "age"), 0);
    printf("\n\n");

    /* Test 4: Nesting Depth */
    printf("Testing maximum nesting depth:\n");
    char* deep_json = generate_nested_json(JSON_MAX_NESTING_DEPTH + 1);
    if (deep_json) {
        run_test("Error - Excessive Nesting", deep_json);
        free(deep_json);
    }

    json_free(person);

    printf("Basic functionality tests completed successfully!\n");

    /* Parser Test Suite */
    printf("\nJSON Parser Test Suite\n");
    printf("======================\n\n");

    /* Basic Values */
    run_test("Null", "null");
    run_test("True", "true");
    run_test("False", "false");
    
    /* Numbers */
    run_test("Number - Integer", "42");
    run_test("Number - Negative", "-42");
    run_test("Number - Float", "3.14159");
    run_test("Number - Scientific", "1.23e-4");
    run_test("Number - Scientific Lowercase", "1.23e4");
    run_test("Number - Scientific Uppercase", "1.23E4");
    run_test("Number - Scientific Negative Exp", "1.23e-4");
    run_test("Number - Scientific Positive Exp", "1.23e+4");
    run_test("Number - Scientific No Decimal", "12e3");
    run_test("Number - Scientific Zero", "0.0e0");
    run_test("Number - Scientific Large", "1.23e15");
    run_test("Number - Scientific Small", "1.23e-15");

    /* Strings */
    run_test("String - Simple", "\"Hello, World!\"");
    run_test("String - Escaped", "\"Hello\\nWorld!\"");
    run_test("String - Basic Escapes", "\"\\\"Hello\\nWorld\\\"\"");
    run_test("String - All Escapes", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    run_test("String - Basic Unicode", "\"Hello \\u0057orld\"");
    run_test("String - Unicode Surrogate Pair", "\"\\uD83D\\uDE00\"");

    /* Arrays */
    run_test("Array - Empty", "[]");
    run_test("Array - Numbers", "[1, 2, 3, 4, 5]");
    run_test("Array - Mixed", "[null, true, 42, \"hello\"]");
    run_test("Array - Nested", "[[1,2],[3,4],[5,6]]");

    /* Objects */
    run_test("Object - Empty", "{}");
    run_test("Object - Simple", "{\"name\":\"John\",\"age\":30}");
    run_test("Object - Nested", "{\"user\":{\"name\":\"John\",\"age\":30}}");
    run_test("Object - Complex", 
             "{\"name\":\"John\",\"age\":30,\"isStudent\":false,"
             "\"grades\":[85,92,78],\"address\":{\"street\":\"123 Main St\","
             "\"city\":\"Springfield\"}}");

    /* Error Cases */
    run_test("Error - Invalid Number", "01234");
    run_test("Error - Unterminated String", "\"Hello");
    run_test("Error - Missing Comma", "[1 2 3]");
    run_test("Error - Trailing Comma", "[1,2,3,]");
    run_test("Error - Invalid Token", "undefined");
    run_test("Error - Unexpected End", "{\"name\":");
    run_test("Error - Invalid Unicode", "\"\\u123g\"");
    run_test("Error - Control Character", "\"\x01\"");
    run_test("Error - Incomplete Decimal", "42.");
    run_test("Error - Multiple Decimals", "42.12.34");
    run_test("Error - Just Decimal", ".");
    run_test("Error - Incomplete Exponent", "1.23e");
    run_test("Error - Bare E", "e10");
    run_test("Error - Invalid Exponent Sign", "1.23e++4");
    run_test("Error - Multiple E", "1.23e2e3");
    run_test("Error - Decimal In Exponent", "1.23e4.5");
    run_test("Error - Invalid Escape", "\"\\x\"");
    run_test("Error - Incomplete Unicode", "\"\\u12\"");
    run_test("Error - Invalid Unicode", "\"\\uzzzz\"");
    run_test("Error - Incomplete Surrogate Pair", "\"\\uD83D\"");
    run_test("Error - Invalid Surrogate Pair", "\"\\uD83D\\u0057\"");
    run_test("Error - Lone Low Surrogate", "\"\\uDE00\"");

    /* Whitespace Cases */
    run_test("Whitespace - Mixed", " { \"key\" : [ 1 , 2 , 3 ] } ");
    run_test("Whitespace - Newlines", "{\n\"key\"\n:\n[\n1\n,\n2\n]\n}");

    /* File Operations */
    printf("\nTesting file parsing:\n");
    JsonValue* file_value = json_parse_file("test.json");
    if (file_value) {
        printf("Successfully parsed test.json:\n");
        json_print_value(file_value, 0);
        printf("\n");
        json_free(file_value);
    } else {
        const JsonError* error = json_get_last_error();
        printf("Failed to parse test.json: %s\n", error->message);
    }

    /* Format Tests */
    printf("\n=== Pretty Print Formatting Tests ===\n");
    test_formatting_options();

    /* Validation Tests */
    printf("\n=== JSON Validation Tests ===\n");
    test_json_validation();

    printf("\nAll tests completed!\n");
    return 0;
}