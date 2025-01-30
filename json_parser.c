/* json_parser.c */
#include "json.h"
#include <ctype.h>

/* parser state structure */
typedef struct ParserState {
    const char* input; // Current position in input string
    const char* input_start;
    size_t input_length;
    size_t line;
    size_t column;
    size_t nesting_level;
    JsonError error;
} ParserState;


/* Global error state */
static JsonError last_error;

/* Get the last error that occured */
const JsonError* json_get_last_error(void) {
    return &last_error;
}
/* Initialise parse state */
static ParserState parser_state_create(const char* input) {
    ParserState state = {
        .input = input,
        .input_start = input,
        .input_length = strlen(input),
        .line = 1,
        .column = 1,
        .nesting_level = 0,
    };

    /* Initialise error structure */
    state.error.code = JSON_ERROR_NONE;
    state.error.line = 0;
    state.error.column = 0;
    state.error.message[0] = '\0';
    state.error.context[0] = '\0';

    /* Reset global error state */
    memcpy(&last_error, &state.error, sizeof(JsonError));

    return state;
}

/* Set error information*/

/* Helper function to skup whitespace */
static void skip_whitespace(ParserState* state) {
    while (isspace(*state->input)) {
        if (*state->input == '\n') {
            state->line++;
            state->column = 1;
        } else {
            state->column++;
        }
        state->input++;
    }
}

/* Forward declarations for recursive descent parser */
static JsonValue* parse_value(ParserState* state);
static JsonValue* parse_string(ParserState* state);
static JsonValue* parse_number(ParserState* state);
static JsonValue* parse_array(ParserState* state);
static JsonValue* parse_object(ParserState* state);

/* Parse a string value */
static JsonValue* parse_string(ParserState* state) {
    if (*state->input != '"') return NULL;
    state->input++; // Skip opening quote
    state->column++;

    //First pass: count length and validate
    const char* start = state->input;
    size_t length = 0;
    while (*state->input && *state->input != '"')
    {
        // Handle escape sequences
        if (*state->input == '\\') {
            state->input++;
            state->column++;
            if (!*state->input) return NULL; // unexpected end
            // TODO: Add proper escape sequence handling
        }
        state->input++;
        state->column++;
        length++;
    }

    if (*state->input != '"') return NULL; // Unterminate string

    // Allocate and copy string
    char* str = (char*)malloc(length + 1);
    if (!str) return NULL;


    // Second pass: copy characters
    const char* src = start;
    char* dst = str;
    while (src < state->input) {
        if (*src == '\\') {
            src++;
            // TODO handle escape sequences properly
            *dst++ = *src++;
        } else {
            *dst++ = *src++;
        }
    }
    
    *dst = '\0';

    state->input++; // Skip closing quote
    state->column++;

    JsonValue* value = json_create_string(str);
    free(str); // json_create_string makes its own copy
    return value;

}

/* Parse a number value */
static JsonValue* parse_number(ParserState* state) {
    char* endptr;
    double number = strtod(state->input, &endptr);

    if (endptr == state->input) return NULL;

    state->column += (endptr - state->input);
    state->input = endptr;

    return json_create_number(number);
}

/* Parse an array */
static JsonValue* parse_array(ParserState* state) {
    if (*state->input != '[') return NULL;
    state->input++; // Skip opening bracket
    state->column++;

    JsonValue* array = json_create_array();
    if (!array) return NULL;

    skip_whitespace(state);

    // Handle empty array 
    if (*state->input == ']') {
        state->input++;
        state->column++;
        return array;
    }

    // Parse array elements
    while (1)
    {
        JsonValue* element = parse_value(state);
        if (!element) {
            json_free(array);
            return NULL;
        }

        if (!json_array_append(array, element)) {
            json_free(element);
            json_free(array);
            return NULL;
        }

        skip_whitespace(state);

        if (*state->input == ']') {
            state->input++;
            state->column++;
            return array;
        }

        if (*state->input != ',') {
            json_free(array);
            return NULL;
        }

        state->input++; // Skip comma
        state->column++;
        skip_whitespace(state);
    }
    
}

/* Parse an object */
static JsonValue* parse_object(ParserState* state) {
    if (*state->input != '{') return NULL;
    state->input++;
    state->column++;

    JsonValue* object = json_create_object();
    if (!object) return NULL;

    skip_whitespace(state);

    // Handle empty object
    if (*state->input == '}') {
        state->input++;
        state->column++;
        return object;
    }

    while (1) {
        skip_whitespace(state);
        
        // Parse key (must be a string)
        JsonValue* key_value = parse_string(state);
        if (!key_value) {
            json_free(object);
            return NULL;
        }

        char* key = strdup(key_value->value.string);
        json_free(key_value);

        skip_whitespace(state);

        // Expect colon
        if (*state->input != ':') {
            free(key);
            json_free(object);
            return NULL;
        }
        state->input++; //skip colon
        state->column++;

        skip_whitespace(state);

        // Parse value
        JsonValue* value = parse_value(state);
        if (!value) {
            free(key);
            json_free(object);
            return NULL;
        }

        // Add key-value pair to object
        if (!json_object_set(object, key, value)) {
            free(key);
            json_free(value);
            json_free(object);
            return NULL;
        }

        free(key);
        skip_whitespace(state);

        if (*state->input == '}') {
            state->input++;
            state->column++;
            return object;
        }

        if (*state->input != ',') {
            json_free(object);
            return NULL;
        }

        state->input++;
        state->column++;
    }
}

/* Parse any JSON value */
static JsonValue* parse_value(ParserState* state) {
    skip_whitespace(state);

    switch (*state->input)
    {
    case 'n':   // null
        if (strncmp(state->input, "null", 4) == 0) {
            state->input += 4;
            state->column += 4;
            return json_create_null();
        }
        return NULL;

    case 't': // true
        if (strncmp(state->input, "true", 4) == 0) {
            state->input += 4;
            state->column += 4;
            return json_create_boolean(1);
        }
        return NULL;
    
    case 'f': // false
        if (strncmp(state->input, "false", 5) == 0) {
            state->input += 5;
            state->column += 5;
            return json_create_boolean(0);
        }
        return NULL;

    case '"':
        return parse_string(state);
    
    case '[':
        return parse_array(state);

    case '{':
        return parse_object(state);

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return parse_number(state);


    default:
        return NULL;
    }
}

/* Public parsing functions */
JsonValue* json_parse_string(const char* json_string) {
    if (!json_string) return NULL;

    ParserState state = parser_state_create(json_string);
    JsonValue* value = parse_value(&state);

    skip_whitespace(&state);
    if (*state.input != '\0') {
        // Extra characters after valid JSON
        json_free(value);
        return NULL;
    }

    return value;
}

JsonValue* json_parse_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    // Read file content
    size_t read_size = fread(buffer, 1, size, file);
    fclose(file);

    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';

    // Parse the string
    JsonValue* value = json_parse_string(buffer);
    free(buffer);

    return value;
}