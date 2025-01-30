/* json.h */
#ifndef JSON_H
#define JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* JSON value types */
typedef enum {
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
} JsonType;

/* Error codes for JSON parsing */
typedef enum{
    JSON_ERROR_NONE = 0,
    JSON_UNEXPECTED_CHAT,
    JSON_ERROR_INVALID_NUMBER,
    JSON_ERROR_UNTERMINATED_STRING,
    JSON_ERROR_INVALID_STRING_CHAR,
    JSON_ERROR_INVALID_ESCAPE_SEQUENCE,
    JSON_ERROR_INVALID_UNICODE,
    JSON_ERROR_EXPECTED_KEY, 
    JSON_ERROR_EXPECTED_COLON,
    JSON_ERROR_EXPECTED_COMMA_OR_BRACKED,
    JSON_ERROR_EXPECTED_COMMA_OR_BRACE,
    JSON_ERROR_INVALID_VALUE,
    JSON_ERROR_MEMORY_ALLOCATION,
    JSON_ERROR_MAXIMUM_NESTING_REACHED
} JsonErrorCode;

typedef struct {
    JsonErrorCode code;
    size_t line;
    size_t column;
    char message[256];
    char context[64];
} JsonError;

/* Forward declaration for object and array structures */
struct JsonObject;
struct JsonArray;

/* Main JSON value structure */
typedef struct JsonValue {
    JsonType type;
    union {
        int boolean;
        double number;
        char* string;
        struct JsonArray* array;
        struct JsonObject* object;
    } value;
} JsonValue;

/* Key-calye pair for objects */
typedef struct JsonKeyValue {
    char* key;
    JsonValue* value;
    struct JsonKeyValue* next; /* For linked list implementation */
} JsonKeyValue;

/* Object structure */
typedef struct JsonObject {
    JsonKeyValue* pairs;
    size_t size;
} JsonObject;

/* Array structure */
typedef struct JsonArray {
    JsonValue** items;
    size_t size;
    size_t capacity;
} JsonArray;

/* Function Declarations */

/* Creation Functions */
JsonValue* json_create_null(void);
JsonValue* json_create_boolean(int value);
JsonValue* json_create_number(double value);
JsonValue* json_create_string(const char* value);
JsonValue* json_create_array(void);
JsonValue* json_create_object(void);

/* Print JSON Values */
void json_print_value(const JsonValue* value, int indent_level);

/* Array operations */
int json_array_append(JsonValue* array, JsonValue* value);
JsonValue* json_array_get(const JsonValue* array, size_t index);

/* Object operations */
int json_object_set(JsonValue* object, const char* key, JsonValue* value);
JsonValue* json_object_get(const JsonValue* object, const char* key);

/* Parsing functions */
JsonValue* json_parse_file(const char* filename);
JsonValue* json_parse_string(const char* json_string);

/* Writing functions */
int json_write_file(const JsonValue* value, const char* filename);
char* json_write_string(const JsonValue* value);

/* Validation function */
int json_validate_strgin(const char* json_string);

/* Error handling */
const JsonError* json_get_last_error(void);

/* Cleanup function */
void json_free(JsonValue* value);

#endif /* JSON_H */