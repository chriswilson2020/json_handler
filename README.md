# JSON C Library

## Overview
This is a lightweight and portable JSON parsing, validation, formatting, and serialization library written in C. It provides an efficient way to parse JSON from strings or files, validate JSON structures, format JSON output, create and manipulate JSON objects, and serialize them back to strings or files.

## Features
- JSON parsing from strings and files
- JSON validation for structure correctness
- JSON formatting with multiple styles (compact, pretty, default)
- JSON serialization to strings and files
- JSON file streaming for efficient processing
- JSON deep copy functionality
- JSON cleaning by removing invalid (NaN) entries
- Supports null, boolean, number, string, array, and object types
- Error handling with detailed messages
- Memory management functions for safe usage

## Installation
To use this library in your project, include the `json.h`, `json.c`, `json_parser.c`, `json_validate.c`, `json_format.c`, and `json_file.c` files in your source code and compile them together.

```sh
# Example compilation
gcc -o json_example example.c json.c json_parser.c json_validate.c json_format.c json_file.c -Wall -Wextra
```

## Usage

### Parsing JSON from a string
```c
#include "json.h"
#include <stdio.h>

int main() {
    const char *json_string = "{ \"name\": \"John\", \"age\": 30, \"city\": \"New York\" }";
    JsonValue *json = json_parse_string(json_string);
    
    if (json) {
        json_print_value(json, 0);
        json_free(json);
    } else {
        printf("Error parsing JSON: %s\n", json_get_last_error()->message);
    }
    return 0;
}
```

### Validating JSON
```c
#include "json.h"
#include <stdio.h>

int main() {
    const char *json_string = "{ \"name\": \"John\", \"age\": 30 }";
    if (json_validate_string(json_string)) {
        printf("Valid JSON!\n");
    } else {
        printf("Invalid JSON: %s\n", json_get_validation_error()->message);
    }
    return 0;
}
```

### Formatting JSON
```c
#include "json.h"
#include <stdio.h>

int main() {
    JsonValue *json_obj = json_create_object();
    json_object_set(json_obj, "message", json_create_string("Hello, World!"));

    char *formatted_json = json_format_string(json_obj, &JSON_FORMAT_PRETTY);
    if (formatted_json) {
        printf("%s\n", formatted_json);
        free(formatted_json);
    }
    json_free(json_obj);
    return 0;
}
```

### Writing JSON to a file
```c
#include "json.h"
#include <stdio.h>

int main() {
    JsonValue *json_obj = json_create_object();
    json_object_set(json_obj, "message", json_create_string("Hello, World!"));

    json_write_file(json_obj, "output.json");
    json_free(json_obj);
    return 0;
}
```

### Cleaning JSON Data (Removing NaN Values)
```c
#include "json.h"
#include <stdio.h>

int main() {
    JsonValue *array = json_create_array();
    json_array_append(array, json_create_number(NAN));
    json_array_append(array, json_create_number(42));

    JsonCleanStats stats;
    JsonValue *cleaned = json_clean_data(array, NULL, &stats);
    
    printf("Original count: %zu, Cleaned count: %zu, Removed count: %zu\n", 
           stats.original_count, stats.cleaned_count, stats.removed_count);
    json_free(cleaned);
    json_free(array);
    return 0;
}
```

## API Reference

### JSON Value Creation
- `JsonValue* json_create_null(void);`
- `JsonValue* json_create_boolean(int value);`
- `JsonValue* json_create_number(double value);`
- `JsonValue* json_create_string(const char* value);`
- `JsonValue* json_create_array(void);`
- `JsonValue* json_create_object(void);`

### JSON Parsing
- `JsonValue* json_parse_string(const char* json_string);`
- `JsonValue* json_parse_file(const char* filename);`
- `JsonValue* json_parse_stream(FILE* stream);`

### JSON Validation
- `int json_validate_string(const char* json_string);`
- `int json_validate_file(const char* filename);`
- `const JsonError* json_get_validation_error(void);`

### JSON Formatting
- `char* json_format_string(const JsonValue* value, const JsonFormatConfig* config);`
- `int json_format_file(const JsonValue* value, const char* filename, const JsonFormatConfig* config);`

### JSON Writing
- `int json_write_file(const JsonValue* value, const char* filename);`
- `int json_write_stream(const JsonValue* value, FILE* stream);`
- `char* json_write_string(const JsonValue* value);`

### JSON Cleaning
- `JsonValue* json_clean_data(const JsonValue* array, const char* field_name, JsonCleanStats* stats);`

### Memory Management
- `void json_free(JsonValue* value);`

### Error Handling
- `const JsonError* json_get_last_error(void);`
- `const JsonError* json_get_validation_error(void);`
- `const JsonError* json_get_file_error(void);`

## Contributing
If you find a bug or have suggestions for improvements, feel free to open an issue or submit a pull request.

## License
This project is licensed under the GPL-3.0 License.

