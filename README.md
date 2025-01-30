# JSON C Library

## Overview
This is a lightweight and portable JSON parsing and serialization library written in C. It provides an efficient way to parse JSON from strings or files, create and manipulate JSON objects, and serialize them back to strings or files.

## Features
- JSON parsing from strings and files
- JSON serialization to strings and files
- Supports null, boolean, number, string, array, and object types
- Error handling with detailed messages
- Memory management functions for safe usage

## Installation
To use this library in your project, simply include the `json.h` and `json.c` files in your source code and compile them together.

```sh
# Example compilation
gcc -o json_example example.c json.c json_parser.c -Wall -Wextra
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

### Creating a JSON object programmatically
```c
#include "json.h"
#include <stdio.h>

int main() {
    JsonValue *json_obj = json_create_object();
    json_object_set(json_obj, "name", json_create_string("Alice"));
    json_object_set(json_obj, "age", json_create_number(25));
    json_object_set(json_obj, "isStudent", json_create_boolean(0));

    json_print_value(json_obj, 0);
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

### JSON Writing
- `int json_write_file(const JsonValue* value, const char* filename);`
- `char* json_write_string(const JsonValue* value);`

### JSON Manipulation
- `int json_array_append(JsonValue* array, JsonValue* value);`
- `JsonValue* json_array_get(const JsonValue* array, size_t index);`
- `int json_object_set(JsonValue* object, const char* key, JsonValue* value);`
- `JsonValue* json_object_get(const JsonValue* object, const char* key);`

### Memory Management
- `void json_free(JsonValue* value);`

### Error Handling
- `const JsonError* json_get_last_error(void);`

## Contributing
If you find a bug or have suggestions for improvements, feel free to open an issue or submit a pull request.

## License
This project is licensed under the MIT License.

