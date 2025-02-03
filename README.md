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

## Compatibility

### **Microcontroller Compatibility Table for JSON Library**
Here’s a **detailed compatibility table** for various microcontrollers, analyzing **file I/O, memory, floating-point, and required modifications**.

| **Microcontroller** | **CPU** | **RAM** | **File I/O (`fopen`, `fread`)** | **Dynamic Memory (`malloc`)** | **Floating-Point (`math.h`)** | **Runs JSON Library As-Is?** | **Required Modifications** |
|--------------------|---------|---------|---------------------------------|------------------------------|-----------------------------|----------------------------|------------------------------|
| **ESP32 (Xtensa)** | Dual-Core 240MHz | 520KB | ✅ SPIFFS, FatFS | ✅ Yes | ✅ Hardware FPU | ✅ Yes | None |
| **ESP32-S3** | Dual-Core 240MHz | 512KB + PSRAM | ✅ SPIFFS, FatFS | ✅ Yes | ✅ Hardware FPU | ✅ Yes | None |
| **ESP32-C3** | Single-Core 160MHz (RISC-V) | 400KB | ✅ SPIFFS, FatFS | ✅ Yes | ⚠️ Soft-FPU (slower math) | ⚠️ Yes, but slower | Optimize floating-point, use static buffers |
| **Raspberry Pi Pico (RP2040)** | Dual-Core 133MHz (Cortex-M0+) | 264KB | ❌ No native FS (use LittleFS) | ✅ Yes | ⚠️ Soft-FPU (slow math) | ⚠️ Needs filesystem | Implement FatFS or LittleFS |
| **STM32F103 ("Blue Pill")** | Cortex-M3 @ 72MHz | 20KB | ❌ No native FS | ✅ Yes | ⚠️ No FPU (slow math) | ⚠️ Needs FatFS | Implement FatFS, avoid float math |
| **STM32F4 (e.g. STM32F411RE)** | Cortex-M4 @ 100MHz | 128KB+ | ✅ FatFS | ✅ Yes | ✅ Hardware FPU | ✅ Yes | None |
| **STM32H7** | Cortex-M7 @ 480MHz | 1MB+ | ✅ FatFS | ✅ Yes | ✅ Hardware FPU | ✅ Yes | None |
| **BBC micro:bit v1 (nRF51822)** | Cortex-M0 @ 16MHz | 16KB | ❌ No native FS | ⚠️ Limited | ❌ No FPU | ❌ No | Needs filesystem, floating-point removal |
| **BBC micro:bit v2 (nRF52833)** | Cortex-M4 @ 64MHz | 128KB | ❌ No native FS | ✅ Yes | ✅ Hardware FPU | ⚠️ No FS support | Implement virtual FS (LittleFS) |
| **Teensy 4.1 (IMXRT1062)** | Cortex-M7 @ 600MHz | 1MB | ✅ SD, QSPI Flash | ✅ Yes | ✅ Hardware FPU | ✅ Yes | None |
| **Arduino Uno (ATmega328P)** | AVR 8-bit @ 16MHz | 2KB | ❌ No FS | ❌ No | ❌ No | ❌ No | No heap, no floating-point, no FS |
| **Arduino Mega (ATmega2560)** | AVR 8-bit @ 16MHz | 8KB | ❌ No FS | ❌ No | ❌ No | ❌ No | No heap, no floating-point, no FS |
| **Z80 (CP/M, embedded)** | Zilog Z80 @ 4-10MHz | 64KB | ❌ No FS | ❌ No | ❌ No | ❌ No | Needs full rewrite, lacks heap & float math |
| **MSP430** | 16-bit @ 16MHz | 512B-64KB | ❌ No FS | ❌ No | ❌ No | ❌ No | Not feasible |

### **Summary**
- ✅ **Runs without modification**: **ESP32, STM32F4, STM32H7, Teensy 4.1**.
- ⚠️ **Needs minor modifications**: **ESP32-C3, Raspberry Pi Pico, STM32F103, micro:bit v2** (filesystem, floating-point optimizations).
- ❌ **Not feasible**: **Z80, Arduino Uno/Mega, MSP430, micro:bit v1** (no heap, no filesystem, no floating-point).

---

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

