/* json.h */
#ifndef JSON_H
#define JSON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JSON_MAX_NESTING_DEPTH 32

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
    JSON_ERROR_UNEXPECTED_CHAR,
    JSON_ERROR_INVALID_NUMBER,
    JSON_ERROR_UNTERMINATED_STRING,
    JSON_ERROR_INVALID_STRING_CHAR,
    JSON_ERROR_INVALID_ESCAPE_SEQUENCE,
    JSON_ERROR_INVALID_UNICODE,
    JSON_ERROR_EXPECTED_KEY, 
    JSON_ERROR_EXPECTED_COLON,
    JSON_ERROR_EXPECTED_COMMA_OR_BRACKET,
    JSON_ERROR_EXPECTED_COMMA_OR_BRACE,
    JSON_ERROR_INVALID_VALUE,
    JSON_ERROR_MEMORY_ALLOCATION,
    JSON_ERROR_MAXIMUM_NESTING_REACHED,
    JSON_ERROR_FORMAT_ERROR,
    JSON_ERROR_FORMAT_BUFFER_OVERFLOW,
    JSON_ERROR_FORMAT_MEMORY_ALLOCATION,
    JSON_ERROR_FORMAT_INVALID_CONFIG,
    JSON_ERROR_FORMAT_FILE_WRITE,
    JSON_ERROR_FORMAT_NULL_INPUT,
} JsonErrorCode;

typedef enum {
    JSON_NUMBER_FORMAT_DECIMAL,     /* Regular decimal format (e.g., 123.456) */
    JSON_NUMBER_FORMAT_SCIENTIFIC,  /* Scientific notation (e.g., 1.23e+2) */
    JSON_NUMBER_FORMAT_AUTO        /* Automatically choose based on magnitude */
} JsonNumberFormat;

typedef struct JsonFormatConfig {
    const char* indent_string;      /* String to use for each indent level (e.g., " " or "\t")*/
    const char* line_end;           /* String to use for line endings (e.g., "\n") */
    int spaces_after_colon;         /* Number of spaces after colons in objecta */
    int spaces_after_comma;         /* NUmber of spaces after commas */
    int max_inline_length;          /* Maximum length for inline arrays/objects before breaking */
    JsonNumberFormat number_format; /* How to format numbers */
    int precision;                  /* Number of decimal places for flaoting point */
    int inline_simple_arrays;       /* Whether to keep simple arrays on one line */
    int sort_object_keys;           /* Whether to sort object keys alphabetically */
} JsonFormatConfig;

/* Default format configuration */
extern const JsonFormatConfig JSON_FORMAT_DEFAULT;
/* Compact format (minimal whitespace) */
extern const JsonFormatConfig JSON_FORMAT_COMPACT;
/* Pretty format (readable indentation) */
extern const JsonFormatConfig JSON_FORMAT_PRETTY;

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

const JsonError* json_get_last_error(void);      /* For parser errors */
const JsonError* json_get_validation_error(void); /* For validation errors */

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

/* Pretty Print functions */
char* json_format_string(const JsonValue* value, const JsonFormatConfig* config);
int json_format_file(const JsonValue* value, const char* filename, const JsonFormatConfig* config);

/* Writing functions */
int json_write_file(const JsonValue* value, const char* filename);
char* json_write_string(const JsonValue* value);

/* Validation function */
int json_validate_string(const char* json_string);
int json_validate_file(const char* filename);

/* Error handling */
const JsonError* json_get_last_error(void);

/* Cleanup function */
void json_free(JsonValue* value);

#endif /* JSON_H */