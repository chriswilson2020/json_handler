/* json_file.c */
#include "json.h"

#define DEFAULT_BUFFER_SIZE (8192)
#define DEFAULT_TEMP_SUFFIX ".tmp"

/* File operation error state */
static JsonError file_error = {
    .code = JSON_ERROR_NONE,
    .line = 0,
    .column = 0,
    .message = "",
    .context = ""
};

/* Get the last file error */
const JsonError* json_get_file_error(void) {
    return &file_error;
}

/* Set file error with context */
static void set_file_error(JsonErrorCode code, const char* message) {
    file_error.code = code;
    strncpy(file_error.message, message, sizeof(file_error.message) - 1);
    file_error.message[sizeof(file_error.message) - 1] = '\0';
    file_error.line = 0;
    file_error.column = 0;
    file_error.context[0] = '\0';
}

/* Stream operations */
int json_write_stream(const JsonValue* value, FILE* stream) {
    if (!value || !stream) {
        set_file_error(JSON_ERROR_INVALID_VALUE, "Invalid parameters for stream writing");
        return 0;
    }

    /* Format the value using compact formatting by default */
    char* json_str = json_format_string(value, &JSON_FORMAT_COMPACT);
    if (!json_str) {
        set_file_error(JSON_ERROR_MEMORY_ALLOCATION, "Failed to format JSON");
        return 0;
    }

    size_t len = strlen(json_str);
    size_t written = fwrite(json_str, 1, len, stream);
    free(json_str);

    if (written != len) {
        set_file_error(JSON_ERROR_FILE_WRITE, "Failed to write complete data to stream");
        return 0;
    }

    return 1;
}

JsonValue* json_parse_stream(FILE* stream) {
    if (!stream) {
        set_file_error(JSON_ERROR_INVALID_VALUE, "NULL stream pointer");
        return NULL;
    }

    long current_pos = ftell(stream);
    if (current_pos < 0) {
        set_file_error(JSON_ERROR_FILE_READ, "Failed to determine stream position");
        return NULL;
    }

    if (fseek(stream, 0, SEEK_END) != 0) {
        set_file_error(JSON_ERROR_FILE_READ, "Failed to seek stream");
        return NULL;
    }

    long size = ftell(stream);
    if (size < 0) {
        set_file_error(JSON_ERROR_FILE_READ, "Failed to determine stream size");
        return NULL;
    }

    if (fseek(stream, current_pos, SEEK_SET) != 0) {
        set_file_error(JSON_ERROR_FILE_READ, "Failed to restore stream position");
        return NULL;
    }

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        set_file_error(JSON_ERROR_MEMORY_ALLOCATION, "Failed to allocate buffer for file reading");
        return NULL;
    }

    size_t read_size = fread(buffer, 1, size, stream);
    if (read_size != (size_t)size) {
        set_file_error(JSON_ERROR_FILE_READ, "Failed to read complete stream");
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    JsonValue* result = json_parse_string(buffer);
    free(buffer);

    if (!result) {
        const JsonError* parse_error = json_get_last_error();
        set_file_error(parse_error->code, parse_error->message);
    }

    return result;
}

/* Basic file writing */
int json_write_file(const JsonValue* value, const char* filename) {
    if (!value || !filename) {
        set_file_error(JSON_ERROR_INVALID_VALUE, "Invalid parameters for file writing");
        return 0;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        set_file_error(JSON_ERROR_FILE_WRITE, "Failed to open file for writing");
        return 0;
    }

    int result = json_write_stream(value, file);
    if (!result) {
        fclose(file);
        return 0;
    }

    if (fflush(file) != 0) {
        set_file_error(JSON_ERROR_FILE_WRITE, "Failed to flush file buffer");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

/* Partial file reading */
JsonFileReader* json_file_reader_create(const char* filename, size_t buffer_size) {
    if (!filename) {
        set_file_error(JSON_ERROR_INVALID_VALUE, "NULL filename");
        return NULL;
    }

    JsonFileReader* reader = (JsonFileReader*)malloc(sizeof(JsonFileReader));
    if (!reader) {
        set_file_error(JSON_ERROR_MEMORY_ALLOCATION, "Failed to allocate file reader");
        return NULL;
    }

    reader->file = fopen(filename, "rb");
    if (!reader->file) {
        set_file_error(JSON_ERROR_FILE_READ, "Failed to open file for reading");
        free(reader);
        return NULL;
    }

    reader->buffer_size = buffer_size ? buffer_size : DEFAULT_BUFFER_SIZE;
    reader->buffer = (char*)malloc(reader->buffer_size);
    if (!reader->buffer) {
        set_file_error(JSON_ERROR_MEMORY_ALLOCATION, "Failed to allocate read buffer");
        fclose(reader->file);
        free(reader);
        return NULL;
    }

    reader->bytes_read = 0;
    return reader;
}

JsonValue* json_file_reader_next(JsonFileReader* reader) {
    if (!reader || !reader->file || !reader->buffer) {
        set_file_error(JSON_ERROR_INVALID_VALUE, "Invalid file reader");
        return NULL;
    }

    size_t bytes = fread(reader->buffer, 1, reader->buffer_size - 1, reader->file);
    if (bytes == 0) {
        if (feof(reader->file)) {
            return NULL; /* Normal EOF */
        }
        set_file_error(JSON_ERROR_FILE_READ, "Failed to read from file");
        return NULL;
    }

    reader->buffer[bytes] = '\0';
    reader->bytes_read += bytes;

    return json_parse_string(reader->buffer);
}

void json_file_reader_free(JsonFileReader* reader) {
    if (reader) {
        if (reader->file) {
            fclose(reader->file);
        }
        free(reader->buffer);
        free(reader);
    }
}

/* File writing with configuration */
int json_write_file_ex(const JsonValue* value, const char* filename,
                      const JsonFileWriteConfig* config) {
    if (!value || !filename) {
        set_file_error(JSON_ERROR_INVALID_VALUE, "Invalid parameters for file writing");
        return 0;
    }

    /* Use default config if none provided */
    JsonFileWriteConfig default_config = {
        .buffer_size = DEFAULT_BUFFER_SIZE,
        .temp_suffix = DEFAULT_TEMP_SUFFIX,
        .sync_on_close = 1
    };

    const JsonFileWriteConfig* cfg = config ? config : &default_config;

    /* Create temporary filename */
    size_t temp_name_len = strlen(filename) + strlen(cfg->temp_suffix) + 1;
    char* temp_filename = (char*)malloc(temp_name_len);
    if (!temp_filename) {
        set_file_error(JSON_ERROR_MEMORY_ALLOCATION, "Failed to allocate temporary filename");
        return 0;
    }

    snprintf(temp_filename, temp_name_len, "%s%s", filename, cfg->temp_suffix);

    /* Open temporary file */
    FILE* file = fopen(temp_filename, "wb");
    if (!file) {
        set_file_error(JSON_ERROR_FILE_WRITE, "Failed to create temporary file");
        free(temp_filename);
        return 0;
    }

    /* Set up buffering if requested */
    char* buffer = NULL;
    if (cfg->buffer_size > 0) {
        buffer = (char*)malloc(cfg->buffer_size);
        if (buffer) {
            setvbuf(file, buffer, _IOFBF, cfg->buffer_size);
        }
    }

    /* Write JSON to temporary file */
    int success = json_write_stream(value, file);

    /* Flush and close */
    if (success && cfg->sync_on_close) {
        if (fflush(file) != 0) {
            set_file_error(JSON_ERROR_FILE_WRITE, "Failed to flush file buffer");
            success = 0;
        }
    }

    fclose(file);
    free(buffer);

    if (success) {
        /* Rename temporary file to target filename */
        if (remove(filename) != 0) {
            FILE* test = fopen(filename, "r");
            if (test) {
                /* File exists but couldn't be removed */
                fclose(test);
                set_file_error(JSON_ERROR_FILE_WRITE, "Failed to remove existing file");
                success = 0;
            }
        }
        if (success && rename(temp_filename, filename) != 0) {
            set_file_error(JSON_ERROR_FILE_WRITE, "Failed to rename temporary file");
            success = 0;
        }
    }

    if (!success) {
        remove(temp_filename); /* Clean up temp file on failure */
    }

    free(temp_filename);
    return success;
}