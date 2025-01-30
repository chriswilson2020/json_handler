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

/* Set error information and update global error state */
static void set_parser_error(ParserState* state, JsonErrorCode code, const char* message) {
    if (state->error.code != JSON_ERROR_NONE) {
        return; /* Only record the first error */
    }

    state->error.code = code;
    state->error.line = state->line;
    state->error.column = state->column;
    strncpy(state->error.message, message, sizeof(state->error.message) -1);

    /* Capture context around the error location */
    const char* context_start = state->input - 20;
    if (context_start < state->input_start) {
        context_start = state->input_start;
    }

    size_t context_length = 40;
    if (context_start + context_length > state->input_start + state->input_length) {
        context_length = (state->input_start + state->input_length) - context_start;
    }

    /* copy context with ellipsis */
    size_t prefix_len = 0;
    if (context_start > state->input_start) {
        strcpy(state->error.context, "...");
        prefix_len = 3;
    }

    strncpy(state->error.context + prefix_len, context_start, sizeof(state->error.context) - prefix_len - 4);

    if (context_start + context_length < state->input_start + state->input_length) {
        strcat(state->error.context, "...");
    }

    /* Update global error state */
    memcpy(&last_error, &state->error, sizeof(JsonError));

}

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
    if (*state->input != '"') {
        set_parser_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Exoected '\"' at start of string");
        return NULL;
    }
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
            if (!*state->input) {
                set_parser_error(state, JSON_ERROR_UNTERMINATED_STRING, "Unexpected end of input after escape character");
                return NULL; // unexpected end
            }
            /* For now, just skip the escaped character */
            /* TODO: Validate escape sequences*/
        } else if ((unsigned char)*state->input < 0x20) {
            set_parser_error(state, JSON_ERROR_INVALID_STRING_CHAR, "Invalid control character in string");
            return NULL;
        }

        state->input++;
        state->column++;
        length++;
    }

    if (*state->input != '"') {
        set_parser_error(state, JSON_ERROR_UNTERMINATED_STRING, "Unterminated string");
        return NULL;
    } // Unterminate string

    // Allocate and copy string
    char* str = (char*)malloc(length + 1);
    if (!str) {
        set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to allocate memory string");
        return NULL;
    }


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
    if (!value) {
        set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to create JSON string value");
        free(str);
        return NULL;
    }

    free(str); // json_create_string makes its own copy
    return value;

}

/* Parse a number value */
static JsonValue* parse_number(ParserState* state) {
   const char* start = state->input;

   // Handle negative numbers 
   if (*state->input == '-') {
    state->input++;
    state->column++;
    if (!isdigit(*state->input)) {
        set_parser_error(state, JSON_ERROR_INVALID_NUMBER, "Expected a digit after minus sign");
        return NULL;
    }
   }

   // Parst integer part
   if (*state->input == '0') {
    state->input++;
    state->column++;
    if (isdigit(*state->input)) {
        set_parser_error(state, JSON_ERROR_INVALID_NUMBER, "Leading zeros are not allowed");
        return NULL;
    }
   } else if (isdigit(*state->input)) {
        while(isdigit(*state->input)) {
            state->input++;
            state->column++;
        }
   } else {
        set_parser_error(state, JSON_ERROR_INVALID_NUMBER, "Expected a digit");
        return NULL;
   }

   // Parse fractional input
   if (*state->input == '.') {
    state->input++;
    state->column++;
    if (!isdigit(*state->input)) {
        set_parser_error(state, JSON_ERROR_INVALID_NUMBER, "Expected digit after decimal point");
        return NULL;
    }
    while (isdigit(*state->input)) {
        state->input++;
        state->column++;
    }
   }

   // Handle scientific notation
   if (*state->input == 'e' || *state->input == 'E') {
    state->input++;
    state->column++;

    // Handle optionsl plus or minius sign in exponent
    if (*state->input == '+' || *state->input == '-') {
        state->input++;
        state->column++;
    }

    // Must have at least one digit in exponent
    if (!isdigit(*state->input)) {
        set_parser_error(state, JSON_ERROR_INVALID_NUMBER, "Expected digit in exponent");
        return NULL;
    }
    while (isdigit(*state->input)) {
        state->input++;
        state->column++;
    }

   }

   // Convert the accumulated string to a number
   char* number_str = (char*)malloc(state->input - start + 1);
   if (!number_str) {
    set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to allocate memory for number conversion");
    return NULL;
   }

   strncpy(number_str, start, state->input - start);
   number_str[state->input - start] = '\0';
  
    double number = atof(number_str);
    free(number_str);

   JsonValue* value = json_create_number(number);
   if (!value) {
    set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to create JSON number value");
    return NULL;
   }

   return value;
}

/* Parse an array */
static JsonValue* parse_array(ParserState* state) {
    if (*state->input != '[') {
        set_parser_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Expected '[' at start of array");
        return NULL;
    }

    /* Check nesting level before proceeding */
    if (state->nesting_level >= JSON_MAX_NESTING_DEPTH) {
        set_parser_error(state, JSON_ERROR_MAXIMUM_NESTING_REACHED, "Maximum nesting depth exceeded");
        return NULL;
    }

    state->nesting_level++;
    state->input++; // Skip opening bracket
    state->column++;

    JsonValue* array = json_create_array();
    if (!array) {
        set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to create array");
        return NULL;
    }

    skip_whitespace(state);

    // Handle empty array 
    if (*state->input == ']') {
        state->input++;
        state->column++;
        state->nesting_level--; // Decrement nesting level
        return array;
    }

    // Parse array elements
    while (1)
    {
        JsonValue* element = parse_value(state);
        if (!element) {
            state->nesting_level--; // Decrement on error
            json_free(array);
            return NULL;
        }

         // The problem is likely here - we might be keeping a reference 
        // to the element after appending it
        if (!json_array_append(array, element)) {
            set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION,
                           "Failed to append element to array");
            json_free(element);  // We might be double-freeing here
            json_free(array);
            return NULL;
        }

        skip_whitespace(state);

        // Check for end of array

        if (*state->input == ']') {
            state->input++; // move past closing bracket
            state->column++;
            state->nesting_level--;
            return array; //  Successfully parsed array
        }

        // If not the end we must see a comma
        if (*state->input != ',') {
            state->nesting_level--;
            set_parser_error(state, JSON_ERROR_EXPECTED_COMMA_OR_BRACKET, "Expected ',' or ']' after array element");
            json_free(array);
            return NULL;
        }

        state->input++; // Skip comma
        state->column++;

        skip_whitespace(state);

        // Check for trailing comma (not allowed in JSON)
        if (*state->input == ']') {
            state->nesting_level--;
            set_parser_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Trailing comma not allowed in array");
            json_free(array);
            return NULL;
        }
    }
    
}

/* Parse an object */
static JsonValue* parse_object(ParserState* state) {
    if (*state->input != '{') {
        set_parser_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Expected '{' at start of object");
        return NULL;
    }

    /* Check nesting level befor proceeding */
    if (state->nesting_level >= JSON_MAX_NESTING_DEPTH) {
        set_parser_error(state, JSON_ERROR_MAXIMUM_NESTING_REACHED, "Maximum nesting depth exceeded");
        return NULL;
    }

    state->nesting_level++;
    state->input++;
    state->column++;

    JsonValue* object = json_create_object();
    if (!object) {
        state->nesting_level--;
        set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failes to create object");
        return NULL;
    }

    skip_whitespace(state);

    // Handle empty object
    if (*state->input == '}') {
        state->input++;
        state->column++;
        return object;
    }

    // Parse object members
    while (1) {
        // Each key must be a string
        skip_whitespace(state); // I think this is redundant TODO:
        
        // Parse key (must be a string)
        JsonValue* key_value = parse_string(state);
        if (!key_value) {
            json_free(object);
            return NULL;
        }

        // Make a copy of the key string
        char* key = strdup(key_value->value.string);
        json_free(key_value); // We don't need the JsonValue wrapper anymore

        if (!key) {
            set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to allocate memory for object key");
            json_free(object);
            return NULL;
        }

        skip_whitespace(state);

        // Expect colon
        if (*state->input != ':') {
            set_parser_error(state, JSON_ERROR_EXPECTED_COLON, "Expected ':' after object key");
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
            set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to add key-value pair to object");
            free(key);
            json_free(value);
            json_free(object);
            return NULL;
        }

        free(key);
        skip_whitespace(state);

        // Check for the end of object
        if (*state->input == '}') {
            state->input++;
            state->column++;
            return object;
        }

        if (*state->input != ',') {
            set_parser_error(state, JSON_ERROR_EXPECTED_COMMA_OR_BRACE, "Expected ',' or '}' after object value");
            json_free(object);
            return NULL;
        }

        state->input++;
        state->column++;

        // skip whitespace after comma
        skip_whitespace(state);

        // Check for trailing comma (not allowed in JSON)
        if (*state->input == '}') {
            set_parser_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Expected string after comma, got '}'");
            json_free(object);
            return NULL;
        }
    }
}

/* Parse any JSON value */
static JsonValue* parse_value(ParserState* state) {
    skip_whitespace(state);

    // Store the startin position for error context
    // const char* start_pos = state->input;

    switch (*state->input)
    {
    case 'n':   // null
        if (strncmp(state->input, "null", 4) == 0) {
            state->input += 4;
            state->column += 4;
            JsonValue* value = json_create_null();
            if (!value) {
                set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to create null value");
                return NULL;
            }
            return value;
        }

        // If we see 'n' but not "null", its an invalid token
        set_parser_error(state, JSON_ERROR_INVALID_VALUE, "Invalid token: expected 'null'");
        return NULL;

    case 't': // true
        if (strncmp(state->input, "true", 4) == 0) {
            state->input += 4;
            state->column += 4;
            JsonValue* value = json_create_boolean(1);
            if (!value) {
                set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to create boolean value");
                return NULL;
            }
            return value;
        }
        set_parser_error(state, JSON_ERROR_INVALID_VALUE, "Invalid token: expected 'true'");
        return NULL;
    
    case 'f': // false
        if (strncmp(state->input, "false", 5) == 0) {
            state->input += 5;
            state->column += 5;
            JsonValue* value = json_create_boolean(0);
            if (!value) {
                set_parser_error(state, JSON_ERROR_MEMORY_ALLOCATION, "Failed to create boolean value");
                return NULL;
            }
            return value;
        }
        set_parser_error(state, JSON_ERROR_INVALID_VALUE, "Invalid token: expected 'false'");
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
    case '\0': // End of input
        set_parser_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Unexpected end of input");
        return NULL;

    default:
        {
            char message[100];
            if (isprint(*state->input)) {
                snprintf(message, sizeof(message), "Unexpected character '%c' at start of value", *state->input);
            } else {
                snprintf(message, sizeof(message), "Unexpected character (code: %d) at start of value", (unsigned char)*state->input);
            }
            set_parser_error(state, JSON_ERROR_INVALID_VALUE, message);
            return NULL;
        }
    }
}

/* Public parsing functions */
JsonValue* json_parse_string(const char* json_string) {
    // First, validate our input
    if (!json_string) {
        // For public functions, we update the global error state directly
        last_error.code = JSON_ERROR_INVALID_VALUE;
        strcpy(last_error.message, "In put string is NULL");
        last_error.line = 0;
        last_error.column = 0;
        return NULL;
    }

    // Initialise the parser state
    ParserState state = parser_state_create(json_string);

    // Parst the root value
    JsonValue* value = parse_value(&state);
    if (!value) {
        // parse_value will have already set the error state
        return NULL;
    }

    // If we parse a valid value, we should ony see whitespace
    skip_whitespace(&state);


    if (*state.input != '\0') {
        set_parser_error(&state, JSON_ERROR_UNEXPECTED_CHAR, "Unexpected content after JSON value");
        // Extra characters after valid JSON
        json_free(value);
        return NULL;
    }

    return value;
}

JsonValue* json_parse_file(const char* filename) {
    // Validate input filename
    if (!filename) {
        last_error.code = JSON_ERROR_INVALID_VALUE;
        strcpy(last_error.message, "Filename is NULL");
        last_error.line = 0;
        last_error.column = 0;
        return NULL;
    }

    // Open the file
    FILE* file = fopen(filename, "r");
    if (!file) {
        last_error.code = JSON_ERROR_INVALID_VALUE;
        snprintf(last_error.message, sizeof(last_error.message), "Could not open file: %s", filename);
        last_error.line = 0;
        last_error.column = 0;
        return NULL;
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        last_error.code = JSON_ERROR_INVALID_VALUE;
        strcpy(last_error.message, "Could not determine file size");
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        last_error.code = JSON_ERROR_INVALID_VALUE;
        strcpy(last_error.message, "Could not determine file size");
        return NULL;
    }

    // Return to start of file
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        last_error.code = JSON_ERROR_INVALID_VALUE;
        strcpy(last_error.message, "Could not read file");
        return NULL;
    }

    // Allocate buffer
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        last_error.code = JSON_ERROR_MEMORY_ALLOCATION;
        strcpy(last_error.message, "Could not allocate memore for file contents");
        return NULL;
    }

    // Read file content
    size_t read_size = fread(buffer, 1, size, file);
    fclose(file);

    if (read_size != (size_t)size) {
        free(buffer);
        last_error.code = JSON_ERROR_INVALID_VALUE;
        strcpy(last_error.message, "Could not read entire file");
        return NULL;
    }

    buffer[size] = '\0';

    // Parse the string
    JsonValue* value = json_parse_string(buffer);
    free(buffer);

    return value;
}