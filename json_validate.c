/* json_validate.c */
#include "json.h"
#include <ctype.h>

/* Single validation error state */
static JsonError validation_error = {
    .code = JSON_ERROR_NONE,
    .line = 1,
    .column = 1,
    .message = "",
    .context = ""
};

const JsonError* json_get_validation_error(void) {
    return &validation_error;
}

/* Validator state structure */
typedef struct {
    const char* input;
    const char* input_start;
    size_t input_length;
    size_t line;
    size_t column;
    size_t nesting_level;
} ValidatorState;

/* Initialize validator state */
static ValidatorState create_validator_state(const char* input) {
    ValidatorState state = {
        .input = input,
        .input_start = input,
        .input_length = input ? strlen(input) : 0,
        .line = 1,
        .column = 1,
        .nesting_level = 0
    };

    /* Reset global error state */
    validation_error.code = JSON_ERROR_NONE;
    validation_error.line = 1;
    validation_error.column = 1;
    validation_error.message[0] = '\0';
    validation_error.context[0] = '\0';

    return state;
}

/* Set validation error */
static void set_validation_error(const ValidatorState* state, JsonErrorCode code, const char* message) {
    if (validation_error.code != JSON_ERROR_NONE) {
        return; /* Only record first error */
    }
    
    validation_error.code = code;
    validation_error.line = state->line;
    validation_error.column = state->column;
    strncpy(validation_error.message, message, sizeof(validation_error.message) - 1);
    validation_error.message[sizeof(validation_error.message) - 1] = '\0';
    
    if (!state->input) {
        validation_error.context[0] = '\0';
        return;
    }

    /* Capture error context */
    const char* context_start = state->input - 20;
    if (context_start < state->input_start) {
        context_start = state->input_start;
    }

    size_t context_length = 40;
    if (context_start + context_length > state->input_start + state->input_length) {
        context_length = (state->input_start + state->input_length) - context_start;
    }

    /* Copy context */
    if (context_start > state->input_start) {
        strcpy(validation_error.context, "...");
        strncpy(validation_error.context + 3, context_start,
                sizeof(validation_error.context) - 7); /* -7 for "..." + "..." + null */
        if (context_start + context_length < state->input_start + state->input_length) {
            strcat(validation_error.context, "...");
        }
    } else {
        strncpy(validation_error.context, context_start,
                sizeof(validation_error.context) - 4); /* -4 for "..." + null */
        if (context_start + context_length < state->input_start + state->input_length) {
            strcat(validation_error.context, "...");
        }
    }
}



/* Forward declarations */
static int validate_value(ValidatorState* state);
static int validate_string(ValidatorState* state);
static int validate_number(ValidatorState* state);
static int validate_array(ValidatorState* state);
static int validate_object(ValidatorState* state);

/* Skip whitespace characters */
static void skip_whitespace(ValidatorState* state) {
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

/* Validate a string */
static int validate_string(ValidatorState* state) {
    if (*state->input != '"') {
        set_validation_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Expected '\"' at start of string");
        return 0;
    }

    state->input++;
    state->column++;

    while (*state->input && *state->input != '"') {
        if ((unsigned char)*state->input < 0x20) {
            set_validation_error(state, JSON_ERROR_INVALID_STRING_CHAR, "Invalid control character in string");
            return 0;
        }

        if (*state->input == '\\') {
            state->input++;
            state->column++;

            switch (*state->input) {
                    case '"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    state->input++;
                    state->column++;
                    break;

                case 'u': {
                    state->input++;
                    state->column++;

                    /* Validate 4 hex digits */
                    for (int i = 0; i < 4; i++) {
                        if (!isxdigit(*state->input)) {
                            set_validation_error(state, JSON_ERROR_INVALID_UNICODE, "Invalid hex digit in unicode escape");
                            return 0;
                        }
                        state->input++;
                        state->column++;
                    }
                    break;
                }
                default:
                    set_validation_error(state, JSON_ERROR_INVALID_ESCAPE_SEQUENCE, "Invalid escape sequence");
                    return 0;
            }
        } else {
            state->input++;
            state->column++;
        }
    }

    if (*state->input != '"') {
        set_validation_error(state, JSON_ERROR_UNTERMINATED_STRING, "Unterminated string");
        return 0;
    }

    state->input++;
    state->column++;
    return 1;
}

/* Validate a number */
static int validate_number(ValidatorState* state) {
    /* Optional minus sign */
    if (*state->input == '-') {
        state->input++;
        state->column++;
    }

    /* Integer part */
    if (*state->input == '0') {
        state->input++;
        state->column++;
        if (isdigit(*state->input)) {
            set_validation_error(state, JSON_ERROR_INVALID_NUMBER, "Leading zeros not allowed");
            return 0;
        }
    } else if (isdigit(*state->input)) {
        while (isdigit(*state->input)) {
            state->input++;
            state->column++;
        }
    } else {
        set_validation_error(state, JSON_ERROR_INVALID_NUMBER, "Expected digit");
        return 0;
    }

    /* Fractional part */
    if(*state->input == '.') {
        state->input++;
        state->column++;
        if (!isdigit(*state->input)) {
            set_validation_error(state, JSON_ERROR_INVALID_NUMBER, "Exoected digit after decimal point");
            return 0;
        }
        while (isdigit(*state->input)) {
            state->input++;
            state->column++;
        }
    }

    /* Exponent */
    if (*state->input == 'e' || *state->input == 'E') {
        state->input++;
        state->column++;

        if (*state->input == '+' || *state->input == '-') {
            state->input++;
            state->column++;
        }

        if (!isdigit(*state->input)) {
            set_validation_error(state, JSON_ERROR_INVALID_NUMBER, "Expected digit in exponent");
            return 0;
        }

        while (isdigit(*state->input)) {
            state->input++;
            state->column++;
        }
    }
    return 1;

}

/* Validate an Array */
static int validate_array(ValidatorState* state) {
    if (*state->input != '[') {
        set_validation_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Expected '[' at start of array");
        return 0;
    }

    if (state->nesting_level >= JSON_MAX_NESTING_DEPTH) {
        set_validation_error(state, JSON_ERROR_MAXIMUM_NESTING_REACHED, "Maximum nesting depth exceeded");
        return 0;
    }

    state->nesting_level++;
    state->input++;
    state->column++;

    skip_whitespace(state);

    /* Empty array */
    if (*state->input == ']') {
        state->input++;
        state->column++;
        state->nesting_level--;
        return 1;
    }

    /* Validate array elements*/
    while (1) {
        if (!validate_value(state)) {
            return 0;
        }

        skip_whitespace(state);

        if(*state->input == ']') {
            state->input++;
            state->column++;
            state->nesting_level--;
            return 1;
        }

        if (*state->input != ',') {
            set_validation_error(state, JSON_ERROR_EXPECTED_COMMA_OR_BRACKET, "Expected ',' or ']' after array element");
            return 0;
        }

        state->input++;
        state->column++;
        skip_whitespace(state);

        /* Check trailing comma */
        if (*state->input == ']') {
            set_validation_error(state,JSON_ERROR_UNEXPECTED_CHAR, "Trailing comma not allowed in array");
            return 0;
        }
    }
}

/* Validate an object */
static int validate_object(ValidatorState* state) {
    if (*state->input != '{') {
        set_validation_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Expected '{' at start of obejct");
        return 0;
    }

    if (state->nesting_level >= JSON_MAX_NESTING_DEPTH) {
        set_validation_error(state, JSON_ERROR_MAXIMUM_NESTING_REACHED, "Maximum nesting depth exceeded");
        return 0;
    }

    state->nesting_level++;
    state->input++;
    state->column++;

    skip_whitespace(state);
    
    /* Empty object */
    if (*state->input == '}') {
        state->input++;
        state->column++;
        state->nesting_level--;
        return 1;
    }

    /* Validate object members */
    while (1) {
        /* Validate string key */
        if (!validate_string(state)) {
            return 0;
        }

        skip_whitespace(state);

        /* Expect colon */
        if (*state->input != ':') {
            set_validation_error(state, JSON_ERROR_EXPECTED_COLON, "Expected ':' after object key");
            return 0;
        }

        state->input++;
        state->column++;

        skip_whitespace(state);

        /* validate value*/
        if (!validate_value(state)) {
            return 0;
        }

        skip_whitespace(state);

        if (*state->input == '}') {
            state->input++;
            state->column++;
            state->nesting_level--;
            return 1;
        }

        if (*state->input != ',') {
            set_validation_error(state, JSON_ERROR_EXPECTED_COMMA_OR_BRACE, "Expected ',' or '}' after object value");
            return 0;
        }

        state->input++;
        state->column++;

        skip_whitespace(state);

        /* Check for trailing comma */
        if (*state->input == '}') {
            set_validation_error(state, JSON_ERROR_UNEXPECTED_CHAR, "Trailing comma not allowed in object");
            return 0;
        }
    }
}

/* Validate any JSON value */
static int validate_value(ValidatorState* state) {
    skip_whitespace(state);
    
    switch (*state->input) {
        case 'n':
            if (strncmp(state->input, "null", 4) == 0) {
                state->input += 4;
                state->column += 4;
                return 1;
            }
            set_validation_error(state, JSON_ERROR_INVALID_VALUE,
                               "Invalid token: expected 'null'");
            return 0;
            
        case 't':
            if (strncmp(state->input, "true", 4) == 0) {
                state->input += 4;
                state->column += 4;
                return 1;
            }
            set_validation_error(state, JSON_ERROR_INVALID_VALUE,
                               "Invalid token: expected 'true'");
            return 0;
            
        case 'f':
            if (strncmp(state->input, "false", 5) == 0) {
                state->input += 5;
                state->column += 5;
                return 1;
            }
            set_validation_error(state, JSON_ERROR_INVALID_VALUE,
                               "Invalid token: expected 'false'");
            return 0;
            
        case '"':
            return validate_string(state);
            
        case '[':
            return validate_array(state);
            
        case '{':
            return validate_object(state);
            
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
            return validate_number(state);
            
        case '\0':
            set_validation_error(state, JSON_ERROR_UNEXPECTED_CHAR,
                               "Unexpected end of input");
            return 0;
            
        default:
            {
                char message[100];
                if (isprint(*state->input)) {
                    snprintf(message, sizeof(message),
                            "Unexpected character '%c'", *state->input);
                } else {
                    snprintf(message, sizeof(message),
                            "Unexpected character (code: %d)", (unsigned char)*state->input);
                }
                set_validation_error(state, JSON_ERROR_INVALID_VALUE, message);
                return 0;
            }
    }
}

/* Public validation interface */
int json_validate_string(const char* json_string) {
    ValidatorState state = create_validator_state(json_string);
    
    if (!json_string) {
        set_validation_error(&state, JSON_ERROR_INVALID_VALUE, "Input string is NULL");
        return 0;
    }
    
    /* Validate root value */
    if (!validate_value(&state)) {
        return 0;  /* Error already set by validate_value */
    }
    
    /* Check for trailing content */
    skip_whitespace(&state);
    if (*state.input != '\0') {
        set_validation_error(&state, JSON_ERROR_UNEXPECTED_CHAR,
                           "Unexpected content after JSON value");
        return 0;
    }
    
    return 1;
}

/* File validation function */
int json_validate_file(const char* filename) {
    ValidatorState state = create_validator_state(NULL);
    
    if (!filename) {
        set_validation_error(&state, JSON_ERROR_INVALID_VALUE, "Filename is NULL");
        return 0;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        char message[256];
        snprintf(message, sizeof(message), "Could not open file: %s", filename);
        set_validation_error(&state, JSON_ERROR_INVALID_VALUE, message);
        return 0;
    }

    /* Get file size */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        set_validation_error(&state, JSON_ERROR_INVALID_VALUE, "Could not determine file size");
        return 0;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        set_validation_error(&state, JSON_ERROR_INVALID_VALUE, "Could not determine file size");
        return 0;
    }

    /* Return to start of file */
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        set_validation_error(&state, JSON_ERROR_INVALID_VALUE, "Could not read file");
        return 0;
    }

    /* Allocate buffer */
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        set_validation_error(&state, JSON_ERROR_MEMORY_ALLOCATION, 
                           "Could not allocate memory for file contents");
        return 0;
    }

    /* Read file content */
    size_t read_size = fread(buffer, 1, size, file);
    fclose(file);

    if (read_size != (size_t)size) {
        free(buffer);
        set_validation_error(&state, JSON_ERROR_INVALID_VALUE, "Could not read entire file");
        return 0;
    }

    buffer[size] = '\0';

    /* Validate the string */
    int result = json_validate_string(buffer);
    if (!result) {
        /* Get the validation error from string validation and preserve it */
        const JsonError* string_error = json_get_validation_error();
        memcpy(&validation_error, string_error, sizeof(JsonError));
    }
    free(buffer);
    return result;
}