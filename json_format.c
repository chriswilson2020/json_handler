/* json_format.c */
#include "json.h"
#include <math.h>

static JsonError current_error;

static void set_format_error(JsonErrorCode code, const char *message)
{
    current_error.code = code;
    strncpy(current_error.message, message, sizeof(current_error.message) - 1);
    current_error.message[sizeof(current_error.message) - 1] = '\0';
    current_error.line = 0; /* Line/column not applicable for formatting */
    current_error.column = 0;
    current_error.context[0] = '\0';
}

/* Define default pretty print configurations */
const JsonFormatConfig JSON_FORMAT_DEFAULT = {
    .indent_string = "  ", /* Two spaces */
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
    .max_inline_length = 0, /* Always inline */
    .number_format = JSON_NUMBER_FORMAT_AUTO,
    .precision = 6,
    .inline_simple_arrays = 1,
    .sort_object_keys = 0,
};

const JsonFormatConfig JSON_FORMAT_PRETTY = {
    .indent_string = "    ", /* Four spaces */
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
typedef struct
{
    char *buffer;
    size_t size;
    size_t capacity;
    const JsonFormatConfig *config;
    int indent_level;
} StringBuilder;

/* Helper to skip values around NaN */
static int should_skip_value(const JsonValue *value)
{
    return value && value->type == JSON_NUMBER && isnan(value->value.number);
}

/* Helper function to check for more valid pairs */
static int has_more_valid_pairs(JsonKeyValue* start) {
    JsonKeyValue* current = start;
    while (current) {
        if (!should_skip_value(current->value)){
            return 1;
        }
        current = current->next;
    }
    return 0;
}

/* Pretty Print functions */
static StringBuilder *string_builder_create(const JsonFormatConfig *config)
{
    StringBuilder *sb = (StringBuilder *)malloc(sizeof(StringBuilder));
    if (!sb)
    {
        set_format_error(JSON_ERROR_FORMAT_MEMORY_ALLOCATION, "Failed to allocate StringBuilder");
        return NULL;
    }

    sb->capacity = 1024; /* Initial capacity */
    sb->buffer = (char *)malloc(sb->capacity);

    if (!sb->buffer)
    {
        free(sb);
        set_format_error(JSON_ERROR_FORMAT_MEMORY_ALLOCATION, "Failed to allocate StringBuilder buffer");
        return NULL;
    }

    sb->size = 0;
    sb->buffer[0] = '\0';
    sb->config = config;
    sb->indent_level = 0;
    return sb;
}

/* Ensure the string builder has enough capacity */
static int string_builder_ensure_capacity(StringBuilder *sb, size_t additional)
{
    if (sb->size + additional >= sb->capacity)
    {
        size_t new_capacity = sb->capacity * 2;
        while (sb->size + additional >= new_capacity)
        {
            new_capacity *= 2;
        }

        char *new_buffer = (char *)realloc(sb->buffer, new_capacity);
        if (!new_buffer)
        {
            set_format_error(JSON_ERROR_FORMAT_MEMORY_ALLOCATION, "Failed to relocate StringBuilder buffer");
            return 0;
        }

        sb->buffer = new_buffer;
        sb->capacity = new_capacity;
    }
    return 1;
}

/* Append a string to the builder */
static int string_builder_append(StringBuilder *sb, const char *str)
{
    size_t len = strlen(str);
    if (!string_builder_ensure_capacity(sb, len + 1))
        return 0;

    strcpy(sb->buffer + sb->size, str);
    sb->size += len;
    return 1;
}

/* Format and append a number */
static int string_builder_append_number(StringBuilder *sb, double num)
{
    // Check for invalid numbers
    if (isnan(num))
    {
        set_format_error(JSON_ERROR_INVALID_NUMBER_NAN,
                         "Cannot format NaN value");
        return 0;
    }

    if (isinf(num))
    {
        set_format_error(JSON_ERROR_INVALID_NUMBER_INFINITY,
                         "Cannot format Infinity value");
        return 0;
    }

    char buffer[32];
    int len;

    switch (sb->config->number_format)
    {
    case JSON_NUMBER_FORMAT_DECIMAL:
        len = snprintf(buffer, sizeof(buffer), "%.*f", sb->config->precision, num);
        break;

    case JSON_NUMBER_FORMAT_SCIENTIFIC:
        len = snprintf(buffer, sizeof(buffer), "%.*e", sb->config->precision, num);
        break;

    case JSON_NUMBER_FORMAT_AUTO:
    default:
        if (fabs(num) < 0.0001 || fabs(num) > 100000.0)
        {
            len = snprintf(buffer, sizeof(buffer), "%.*e", sb->config->precision, num);
        }
        else
        {
            len = snprintf(buffer, sizeof(buffer), "%.*f", sb->config->precision, num);
        }
        break;
    }

    if (len < 0 || len >= sizeof(buffer))
        return 0;
    return string_builder_append(sb, buffer);
}

/* Append current indentaion level */
static int string_builder_append_indent(StringBuilder *sb)
{
    for (int i = 0; i < sb->indent_level; i++)
    {
        if (!string_builder_append(sb, sb->config->indent_string))
            return 0;
    }
    return 1;
}

/* Helper function for string escapting */
static int string_builder_append_escaped_string(StringBuilder *sb, const char *str)
{
    if (!string_builder_append(sb, "\""))
        return 0;

    for (const char *p = str; *p; p++)
    {
        char escaped[8];
        switch (*p)
        {
        case '"':
            strcpy(escaped, "\\\"");
            break;
        case '\\':
            strcpy(escaped, "\\\\");
            break;
        case '\b':
            strcpy(escaped, "\\b");
            break;
        case '\f':
            strcpy(escaped, "\\f");
            break;
        case '\n':
            strcpy(escaped, "\\n");
            break;
        case '\r':
            strcpy(escaped, "\\r");
            break;
        case '\t':
            strcpy(escaped, "\\t");
            break;
        default:
            if ((unsigned char)*p < 32)
            {
                snprintf(escaped, sizeof(escaped), "\\u%04x", (unsigned char)*p);
            }
            else
            {
                escaped[0] = *p;
                escaped[1] = '\0';
            }
            break;
        }
        if (!string_builder_append(sb, escaped))
            return 0;
    }

    return string_builder_append(sb, "\"");
}

/* Forward declaration for recursive formatting */
static int format_value(StringBuilder *sb, const JsonValue *value);

/* Format an array */
static int format_array(StringBuilder *sb, const JsonValue *value)
{
    if (!string_builder_append(sb, "["))
        return 0;

    JsonArray *array = value->value.array;
    int simple_array = sb->config->inline_simple_arrays;
    size_t valid_count = 0; // Track number of non-NaN values

    /* Check if array is "simple" (contains only primitive values) */
    if (simple_array)
    {
        for (size_t i = 0; i < array->size; i++)
        {
            JsonValue *item = array->items[i];
            if (!should_skip_value(item))
            {
                valid_count++;
                if (item->type == JSON_ARRAY || item->type == JSON_OBJECT)
                {
                    simple_array = 0;
                }
            }
        }
    }

    if (!simple_array)
    {
        string_builder_append(sb, sb->config->line_end);
        sb->indent_level++;
    }

    size_t formatted_count = 0; // Track how many values we've formatted

    for (size_t i = 0; i < array->size; i++)
    {
        JsonValue *item = array->items[i];

        /* Skip NaN values */
        if (should_skip_value(item))
        {
            continue;
        }

        if (!simple_array)
        {
            string_builder_append_indent(sb);
        }

        if (!format_value(sb, item))
            return 0;

        formatted_count++;
        if (formatted_count < valid_count)
        { // Only add comma if not last valid item
            string_builder_append(sb, ",");
            if (simple_array)
            {
                for (int j = 0; j < sb->config->spaces_after_comma; j++)
                {
                    string_builder_append(sb, " ");
                }
            }
            else
            {
                string_builder_append(sb, sb->config->line_end);
            }
        }
    }

    if (!simple_array)
    {
        sb->indent_level--;
        string_builder_append(sb, sb->config->line_end);
        string_builder_append_indent(sb);
    }

    return string_builder_append(sb, "]");
}

/* Helper structure for sorting object keys */
typedef struct
{
    const char *key;
    JsonValue *value;
} KeyValuePair;

/* Comparision functiuon for sorting keys */
static int compare_keys(const void *a, const void *b)
{
    return strcmp(((KeyValuePair *)a)->key, ((KeyValuePair *)b)->key);
}

/* Format an object Modified to properly handle keys and NaN values */
static int format_object(StringBuilder* sb, const JsonValue* value) {
    if (!string_builder_append(sb, "{")) return 0;

    JsonObject* object = value->value.object;
    if (!object->pairs) {
        return string_builder_append(sb, "}");
    }

    string_builder_append(sb, sb->config->line_end);
    sb->indent_level++;

    /* Count valid pairs and collect them */
    size_t valid_pair_count = 0;
    KeyValuePair* pairs = NULL;
    JsonKeyValue* current = object->pairs;

    /* First pass: count valid pairs */
    while (current) {
        if (!should_skip_value(current->value)) {
            valid_pair_count++;
        }
        current = current->next;
    }

    if (valid_pair_count > 0) {
        /* Allocate array for sorting/processing */
        pairs = (KeyValuePair*)malloc(valid_pair_count * sizeof(KeyValuePair));
        if (!pairs) return 0;

        /* Second pass: fill array with non-NaN values */
        current = object->pairs;
        size_t idx = 0;
        while (current && idx < valid_pair_count) {
            if (!should_skip_value(current->value)) {
                pairs[idx].key = current->key;
                pairs[idx].value = current->value;
                idx++;
            }
            current = current->next;
        }

        /* Sort if configured to do so */
        if (sb->config->sort_object_keys) {
            qsort(pairs, valid_pair_count, sizeof(KeyValuePair), compare_keys);
        }

        /* Format all valid pairs */
        for (size_t i = 0; i < valid_pair_count; i++) {
            string_builder_append_indent(sb);
            string_builder_append_escaped_string(sb, pairs[i].key);
            string_builder_append(sb, ":");

            for (int j = 0; j < sb->config->spaces_after_colon; j++) {
                string_builder_append(sb, " ");
            }

            if (!format_value(sb, pairs[i].value)) {
                free(pairs);
                return 0;
            }

            if (i < valid_pair_count - 1) {
                string_builder_append(sb, ",");
                string_builder_append(sb, sb->config->line_end);
            }
        }

        free(pairs);
    }

    sb->indent_level--;
    string_builder_append(sb, sb->config->line_end);
    string_builder_append_indent(sb);
    return string_builder_append(sb, "}");
}

/* Format any JSON value */
static int format_value(StringBuilder *sb, const JsonValue *value)
{
    if (!value)
        return string_builder_append(sb, "null");

    switch (value->type)
    {
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
char *json_format_string(const JsonValue *value, const JsonFormatConfig *config)
{
    /* Reset error state */
    current_error.code = JSON_ERROR_NONE;

    if (!value)
    {
        set_format_error(JSON_ERROR_FORMAT_NULL_INPUT, "NULL value passed to json_format_string");
        return NULL;
    }

    if (!config)
        config = &JSON_FORMAT_DEFAULT;

    /* Validate config */
    if (!config->indent_string || !config->line_end ||
        config->spaces_after_colon < 0 || config->spaces_after_comma < 0 ||
        config->max_inline_length < 0 || config->precision < 0)
    {
        set_format_error(JSON_ERROR_FORMAT_INVALID_CONFIG,
                         "Invalid format configuration");
        return NULL;
    }

    StringBuilder *sb = string_builder_create(config);
    if (!sb)
        return NULL; /* Error already set by string builder create */

    if (!format_value(sb, value))
    {
        if (current_error.code == JSON_ERROR_NONE)
        {
            set_format_error(JSON_ERROR_FORMAT_ERROR, "Failed to format JSON value");
        }
        free(sb->buffer);
        free(sb);
        return NULL;
    }

    /* Add final newline if configured */
    if (config->line_end[0])
    {
        if (!string_builder_append(sb, config->line_end))
        {
            set_format_error(JSON_ERROR_FORMAT_BUFFER_OVERFLOW, "Failed to append final newline");
            free(sb->buffer);
            free(sb);
            return NULL;
        }
        string_builder_append(sb, config->line_end);
    }

    char *result = strdup(sb->buffer);
    if (!result)
    {
        set_format_error(JSON_ERROR_FORMAT_MEMORY_ALLOCATION, "Failed to duplicate final string");
        return NULL;
    }
    free(sb->buffer);
    free(sb);
    return result;
}

/* File output function */
int json_format_file(const JsonValue *value, const char *filename, const JsonFormatConfig *config)
{
    if (!filename)
    {
        set_format_error(JSON_ERROR_FORMAT_NULL_INPUT, "NULL filename passed to json_format_file");
        return 0;
    }
    char *formatted = json_format_string(value, config);
    if (!formatted)
    {
        /* Error alredy set by json_format_string */
        return 0;
    }

    FILE *file = fopen(filename, "w");
    if (!file)
    {
        set_format_error(JSON_ERROR_FORMAT_FILE_WRITE, "Failed to open output file for writing");
        free(formatted);
        return 0;
    }

    size_t len = strlen(formatted);
    size_t written = fwrite(formatted, 1, len, file);

    if (written != len)
    {
        set_format_error(JSON_ERROR_FORMAT_FILE_WRITE, "Failed to write complete formatted JSON to file");
        fclose(file);
        free(formatted);
        return 0;
    }

    fclose(file);
    free(formatted);

    return written == len;
}