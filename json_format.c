/* json_format.c */
#include "json.h"
#include <math.h>

/* Define default pretty print configurations */
const JsonFormatConfig JSON_FORMAT_DEFAULT = {
    .indent_string = "  ",          /* Two spaces */
    .line_end = "\n",
    .spaces_after_colon = 1,
    .spaces_after_comma = 1,
    .max_inline_length = 80,
    .number_format = JSON_NUMBER_FORMAT_AUTO,
    .precision = 6,
    .inline_simple_arrays = 1,
    .sort_object_keys = 0,
};

const JsonFormatConfig JSON_FORMAT_COMPACT = {
    .indent_string = "",
    .line_end = "",
    .spaces_after_colon = 0,
    .spaces_after_comma = 0,
    .max_inline_length = 0,     /* Always inline */
    .number_format = JSON_NUMBER_FORMAT_AUTO,
    .precision = 6,
    .inline_simple_arrays = 1,
    .sort_object_keys = 0,
};

const JsonFormatConfig JSON_FORMAT_PRETTY = {
    .indent_string = "    ",        /* Four spaces */
    .line_end = "\n",
    .spaces_after_colon = 1,
    .spaces_after_comma = 1,
    .max_inline_length = 60,
    .number_format = JSON_NUMBER_FORMAT_AUTO,
    .precision = 6,
    .inline_simple_arrays = 1,
    .sort_object_keys = 1,
};

/* Helper struct for string building */
typedef struct {
    char* buffer;
    size_t size;
    size_t capacity;
    const JsonFormatConfig* config;
    int indent_level;
} StringBuilder;


/* Pretty Print functions */
static StringBuilder* string_builder_create(const JsonFormatConfig* config) {
    StringBuilder* sb = (StringBuilder*)malloc(sizeof(StringBuilder));
    if (!sb) return NULL;

    sb->capacity = 1024; /* Initial capacity */
    sb->buffer = (char*)malloc(sb->capacity);

    if (!sb->buffer) {
        free(sb);
        return NULL;
    }

    sb->size = 0;
    sb->buffer[0] = '\0';
    sb->config = config;
    sb->indent_level = 0;
    return sb;
}

/* Ensure the string builder has enough capacity */
static int string_builder_ensure_capacity(StringBuilder* sb, size_t additional) {
    if (sb->size + additional >= sb->capacity) {
        size_t new_capacity = sb->capacity * 2;
        while (sb->size + additional >= new_capacity) {
            new_capacity *= 2;
        }

        char* new_buffer = (char*)realloc(sb->buffer, new_capacity);
        if (!new_buffer) return 0;

        sb->buffer = new_buffer;
        sb->capacity = new_capacity;
    }
    return 1;
}

/* Append a string to the builder */
static int string_builder_append(StringBuilder* sb, const char* str) {
    size_t len = strlen(str);
    if (!string_builder_ensure_capacity(sb, len + 1)) return 0;

    strcpy(sb->buffer + sb->size, str);
    sb->size += len;
    return 1;
}

/* Format and append a number */
static int string_builder_append_number(StringBuilder* sb, double num) {
    char buffer[32];
    int len;

    switch (sb->config->number_format) {
        case JSON_NUMBER_FORMAT_DECIMAL:
            len = snprintf(buffer, sizeof(buffer), "%.*f", sb->config->precision, num);
            break;

        case JSON_NUMBER_FORMAT_SCIENTIFIC:
            len = snprintf(buffer, sizeof(buffer), "%.*e", sb->config->precision, num);
            break;
        
        case JSON_NUMBER_FORMAT_AUTO:
        default:
            if (fabs(num) < 0.0001 || fabs(num) > 100000.0) {
                len = snprintf(buffer, sizeof(buffer), "%.*e", sb->config->precision, num);
            } else {
                len = snprintf(buffer, sizeof(buffer), "%.*f", sb->config->precision, num);
            }
            break;
        }

        if (len < 0 || len >= sizeof(buffer)) return 0;
        return string_builder_append(sb, buffer);
    }

/* Append current indentaion level */
static int string_builder_append_indent(StringBuilder* sb) {
    for (int i = 0; i < sb->indent_level; i++) {
        if (!string_builder_append(sb, sb->config->indent_string)) return 0;
    }
    return 1;
}

/* Helper function for string escapting */
static int string_builder_append_escaped_string(StringBuilder* sb, const char* str) {
    if (!string_builder_append(sb, "\"")) return 0;

    for (const char* p = str; *p; p++)
    {
        char escaped[8];
        switch (*p) {
            case '"':  strcpy(escaped, "\\\""); break;
            case '\\': strcpy(escaped, "\\\\"); break;
            case '\b': strcpy(escaped, "\\b");  break;
            case '\f': strcpy(escaped, "\\f");  break;
            case '\n': strcpy(escaped, "\\n");  break;
            case '\r': strcpy(escaped, "\\r");  break;
            case '\t': strcpy(escaped, "\\t");  break;
            default:
                if ((unsigned char)*p < 32) {
                    snprintf(escaped, sizeof(escaped), "\\u%04x", (unsigned char)*p);
                } else {
                    escaped[0] = *p;
                    escaped[1] = '\0';
                }
                break;
            }
            if (!string_builder_append(sb, escaped)) return 0;
        }
        
        return string_builder_append(sb, "\"");
}
    
/* Forward declaration for recursive formatting */
static int format_value(StringBuilder* sb, const JsonValue* value);

/* Format an array */
static int format_array(StringBuilder* sb, const JsonValue* value) {
    if (!string_builder_append(sb, "[")) return 0;
    
    JsonArray* array = value->value.array;
    int simple_array = sb->config->inline_simple_arrays;
    
    /* Check if array is "simple" (contains only primitive values) */
    if (simple_array) {
        for (size_t i = 0; i < array->size; i++) {
            JsonValue* item = array->items[i];
            if (item->type == JSON_ARRAY || item->type == JSON_OBJECT) {
                simple_array = 0;
                break;
            }
        }
    }
    
    if (!simple_array) {
        string_builder_append(sb, sb->config->line_end);
        sb->indent_level++;
    }
    
    for (size_t i = 0; i < array->size; i++) {
        if (!simple_array) {
            string_builder_append_indent(sb);
        }
        
        if (!format_value(sb, array->items[i])) return 0;
        
        if (i < array->size - 1) {
            string_builder_append(sb, ",");
            if (simple_array) {
                for (int j = 0; j < sb->config->spaces_after_comma; j++) {
                    string_builder_append(sb, " ");
                }
            } else {
                string_builder_append(sb, sb->config->line_end);
            }
        }
    }
    
    if (!simple_array) {
        sb->indent_level--;
        string_builder_append(sb, sb->config->line_end);
        string_builder_append_indent(sb);
    }
    
    return string_builder_append(sb, "]");
}

/* Helper structure for sorting object keys */
typedef struct {
    const char* key;
    JsonValue* value;
} KeyValuePair;

/* Comparision functiuon for sorting keys */
static int compare_keys(const void* a, const void* b) {
    return strcmp(((KeyValuePair*)a)->key, ((KeyValuePair*)b)->key);
}

/* Format an object */
static int format_object(StringBuilder* sb, const JsonValue* value) {
    if(!string_builder_append(sb, "{")) return 0;

    JsonObject* object = value->value.object;
    if (!object->pairs) {
        return string_builder_append(sb, "}");
    }

    string_builder_append(sb, sb->config->line_end);
    sb->indent_level++;

    /* Count pairs and optionally sort them */
    size_t pair_count = 0;
    KeyValuePair* pairs = NULL;

    if (sb->config->sort_object_keys) {
        /* Count pairs first */
        JsonKeyValue* current = object->pairs;
        while (current) {
            pair_count++;
            current = current->next;
        }

        /* Allocate array for sorting */
        pairs = (KeyValuePair*)malloc(pair_count * sizeof(KeyValuePair));
        if (!pairs) return 0;

        /* Fill array */
        current = object->pairs;
        for (size_t i = 0; i < pair_count; i++) {
            pairs[i].key = current->key;
            pairs[i].value = current->value;
            current = current->next;
        }

        /* Sort the array */
        qsort(pairs, pair_count, sizeof(KeyValuePair), compare_keys);

    }

    /* Iterator for either sorted or unsorted output */
    size_t i = 0;
    JsonKeyValue* current = object->pairs;

    while (pairs ? (i < pair_count) : (current != NULL)) {
        const char* key;
        JsonValue* value;

        if (pairs) {
            key = pairs[i].key;
            value = pairs[i].value;
        } else {
            key = current->key;
            value = current->value;
        }

        string_builder_append_indent(sb);
        string_builder_append_escaped_string(sb, key);
        string_builder_append(sb, ":");

        for (int j = 0; j < sb->config->spaces_after_colon; j++) {
            string_builder_append(sb, " ");
        }

        if (!format_value(sb, value)) {
            free(pairs);
            return 0;
        }

        if ((pairs && i < pair_count - 1) || (!pairs && current->next)) {
            string_builder_append(sb, ",");
            string_builder_append(sb, sb->config->line_end);
        }

        if (pairs) {
            i++;
        } else {
            current = current->next;
        }
    }

    free(pairs);

    sb->indent_level--;
    string_builder_append(sb, sb->config->line_end);
    string_builder_append_indent(sb);
    return string_builder_append(sb, "}");
}

/* Format any JSON value */
static int format_value(StringBuilder* sb, const JsonValue* value) {
    if (!value) return string_builder_append(sb, "null");

    switch (value->type) {
        case JSON_NULL:
            return string_builder_append(sb, "null");
        
        case JSON_BOOLEAN:
            return string_builder_append(sb, value->value.boolean ? "true" : "false");

        case JSON_NUMBER:
            return string_builder_append_number(sb, value->value.number);
        
        case JSON_STRING:
            return string_builder_append_escaped_string(sb, value->value.string);
        
        case JSON_ARRAY:
            return format_array(sb, value);
        
        case JSON_OBJECT:
            return format_object(sb, value);

        default:
            return 0;
    }
}


/* Public formatting function */
char* json_format_string(const JsonValue* value, const JsonFormatConfig* config) {
    if (!config) config = &JSON_FORMAT_DEFAULT;

    StringBuilder* sb = string_builder_create(config);
    if (!sb) return NULL;

    if (!format_value(sb, value)) {
        free(sb->buffer);
        free(sb);
        return NULL;
    }

    /* Add final newline if configured */
    if (config->line_end[0]) {
        string_builder_append(sb, config->line_end);
    }

    char* result = strdup(sb->buffer);
    free(sb->buffer);
    free(sb);
    return result;
}

/* File output function */
int json_format_file(const JsonValue* value, const char* filename, const JsonFormatConfig* config) {
    char* formatted = json_format_string(value, config);
    if (!formatted) return 0;

    FILE* file = fopen(filename, "w");
    if (!file) {
        free(formatted);
        return 0;
    }

    size_t len = strlen(formatted);
    size_t written = fwrite(formatted, 1, len, file);

    fclose(file);
    free(formatted);

    return written == len;

}