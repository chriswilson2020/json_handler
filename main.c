#include <stdio.h>
#include "json.h"


int main()
{
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
    print_json_value(person, 0);
    printf("\n\n");

    /* Test 2: Accessing values */
    printf("Test 2: Accessing specific values\n");
    JsonValue* name = json_object_get(person, "name");
    JsonValue* age = json_object_get(person, "age");
    JsonValue* hobbies_array = json_object_get(person, "hobbies");

    printf("Name: ");
    print_json_value(name, 0);
    printf("\nAge: ");
    print_json_value(age, 0);
    printf("\nFirst hobby: ");
    print_json_value(json_array_get(hobbies_array, 0), 0);
    printf("\n\n");

    /* Test 3: Updating values */
    printf("Test 3 Updating values\n");
    json_object_set(person, "age", json_create_number(31)); // Happy Birthday!
    printf("Updated age: ");
    print_json_value(json_object_get(person, "age"), 0);
    printf("\n\n");

    /* Clean up */
    json_free(person); // This will recursively free all nested objects and arrays

    printf("All test completed successfully!\n");

    return EXIT_SUCCESS;
}
