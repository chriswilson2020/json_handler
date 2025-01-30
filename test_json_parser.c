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

int main() {
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