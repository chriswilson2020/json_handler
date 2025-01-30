#include <stdio.h>
#include "json.h"

/* test_json_parser.c */
#include "json.h"
#include <stdio.h>

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

    /* Test 1: Create and manipulate a JSON object */
    printf("Test 1: Creating a JSON object with various types\n");
    JsonValue* person = json_create_object();

    /* Add different types of values to the object */
    json_object_set(person, "name", json_create_string("John Doe"));
    json_object_set(person, "age", json_create_number(30));
    json_object_set(person, "is_student", json_create_boolean(1));
    json_object_set(person, "null_field", json_create_null());

    /* Create and populate an array of hobbies */
    JsonValue* hobbies = json_create_array();
    json_array_append(hobbies, json_create_string("reading"));
    json_array_append(hobbies, json_create_string("hiking"));
    json_array_append(hobbies, json_create_string("photography"));

    /* Add the array to our person object */
    json_object_set(person, "hobbies", hobbies);

    /* Create an address object */
    JsonValue* address = json_create_object();
    json_object_set(address, "street", json_create_string("123 Main st"));
    json_object_set(address, "city", json_create_string("Springfield"));
    json_object_set(address, "zip", json_create_string("12345"));

    /* Add the address object to our person object */
    json_object_set(person, "address", address);

    /* Print the entire object */
    printf("\n Complete JSON structure:\n");
    json_print_value(person, 0);
    printf("\n\n");

    /* Test 2: Accessing values */
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

    /* Test 3: Updating values */
    printf("Test 3 Updating values\n");
    json_object_set(person, "age", json_create_number(31)); // Happy Birthday!
    printf("Updated age: ");
    json_print_value(json_object_get(person, "age"), 0);
    printf("\n\n");

    /* Test 4: Test Nesting Depth */
    printf("\nTesting maximum nesting depth:\n");
    char* deep_json = generate_nested_json(JSON_MAX_NESTING_DEPTH + 1);
    if (deep_json) {
        run_test("Error - Excessive Nesting", deep_json);
        free(deep_json);
    }

    /* Clean up */
    json_free(person); // This will recursively free all nested objects and arrays

    printf("All JSON Library implementations tests completed successfully!\n");

    printf("\nJSON Parser Test Suite\n");
    printf("======================\n\n");

    /* Basic Value Tests */
    run_test("Null", "null");
    run_test("True", "true");
    run_test("False", "false");
    run_test("Number - Integer", "42");
    run_test("Number - Negative", "-42");
    run_test("Number - Float", "3.14159");
    run_test("Number - Scientific", "1.23e-4");
    run_test("String - Simple", "\"Hello, World!\"");
    run_test("String - Escaped", "\"Hello\\nWorld!\"");

    /* Number Tests - Scientific Notation */
    run_test("Number - Scientific Lowercase", "1.23e4");
    run_test("Number - Scientific Uppercase", "1.23E4");
    run_test("Number - Scientific Negative Exp", "1.23e-4");
    run_test("Number - Scientific Positive Exp", "1.23e+4");
    run_test("Number - Scientific No Decimal", "12e3");
    run_test("Number - Scientific Zero", "0.0e0");
    run_test("Number - Scientific Large", "1.23e15");
    run_test("Number - Scientific Small", "1.23e-15");

    /* Array Tests */
    run_test("Array - Empty", "[]");
    run_test("Array - Numbers", "[1, 2, 3, 4, 5]");
    run_test("Array - Mixed", "[null, true, 42, \"hello\"]");
    run_test("Array - Nested", "[[1,2],[3,4],[5,6]]");

    /* Object Tests */
    run_test("Object - Empty", "{}");
    run_test("Object - Simple", "{\"name\":\"John\",\"age\":30}");
    run_test("Object - Nested", "{\"user\":{\"name\":\"John\",\"age\":30}}");
    run_test("Object - Complex", 
             "{\"name\":\"John\",\"age\":30,\"isStudent\":false,"
             "\"grades\":[85,92,78],\"address\":{\"street\":\"123 Main St\","
             "\"city\":\"Springfield\"}}");

    /* String escape sequence tests */
    run_test("String - Basic Escapes", 
         "\"\\\"Hello\\nWorld\\\"\"");  // "Hello\nWorld"

    run_test("String - All Escapes",
         "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");

    run_test("String - Basic Unicode",
         "\"Hello \\u0057orld\"");  // "Hello World"

    run_test("String - Unicode Surrogate Pair",
         "\"\\uD83D\\uDE00\"");  // 😀 (smiling face emoji)
    
    /* Error Cases */
    run_test("Error - Invalid Number", "01234");
    run_test("Error - Unterminated String", "\"Hello");
    run_test("Error - Missing Comma", "[1 2 3]");
    run_test("Error - Trailing Comma", "[1,2,3,]");
    run_test("Error - Invalid Token", "undefined");
    run_test("Error - Unexpected End", "{\"name\":");
    run_test("Error - Invalid Unicode", "\"\\u123g\"");
    run_test("Error - Control Character", "\"\x01\"");


    // Number format errors
    run_test("Error - Incomplete Decimal", "42.");          // Missing digits after decimal point
    run_test("Error - Multiple Decimals", "42.12.34");     // Multiple decimal points
    run_test("Error - Just Decimal", ".");                  // Decimal with no digits

    // Scientific notation errors
    run_test("Error - Incomplete Exponent", "1.23e");      // 'e' with no following digits
    run_test("Error - Bare E", "e10");                     // Missing mantissa
    run_test("Error - Invalid Exponent Sign", "1.23e++4"); // Double plus in exponent
    run_test("Error - Multiple E", "1.23e2e3");           // Multiple exponents
    run_test("Error - Decimal In Exponent", "1.23e4.5");  // Decimal point in exponent


    /* Whitespace Handling */
    run_test("Whitespace - Mixed", " { \"key\" : [ 1 , 2 , 3 ] } ");
    run_test("Whitespace - Newlines", "{\n\"key\"\n:\n[\n1\n,\n2\n]\n}");

    run_test("Error - Invalid Escape",
         "\"\\x\"");  // \x is not a valid escape

    run_test("Error - Incomplete Unicode",
         "\"\\u12\"");  // Need 4 hex digits

    run_test("Error - Invalid Unicode",
         "\"\\uzzzz\"");  // Invalid hex digits

    run_test("Error - Incomplete Surrogate Pair",
         "\"\\uD83D\"");  // High surrogate without low surrogate

    run_test("Error - Invalid Surrogate Pair",
         "\"\\uD83D\\u0057\"");  // High surrogate followed by non-surrogate

    run_test("Error - Lone Low Surrogate",
         "\"\\uDE00\"");  // Low surrogate without high surrogate

    /* File Parsing Test */
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

    printf("\nAll tests completed!\n");
    return 0;

}

