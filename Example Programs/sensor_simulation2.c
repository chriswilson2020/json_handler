/* sensor_simulation.c */
#include "json.h"
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    time_t timestamp;
    double temperature;
    int valid;
} SensorReading;

/* Generate temperature between 20-25°C with noise */
static double generate_temperature() {
    return 22.5 + ((double)rand() / RAND_MAX - 0.5) * 5.0;
}

/* Create a JSON array of sensor readings */
static JsonValue* create_sensor_data(SensorReading* readings, size_t count) {
    JsonValue* array = json_create_array();
    
    for (size_t i = 0; i < count; i++) {
        JsonValue* reading = json_create_object();
        
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", 
                localtime(&readings[i].timestamp));
        
        json_object_set(reading, "timestamp", json_create_string(timestamp));
        json_object_set(reading, "temperature", 
                       json_create_number(readings[i].valid ? readings[i].temperature : NAN));
        
        json_array_append(array, reading);
    }
    
    return array;
}

/* Simulate continuous data collection */
static void simulate_continuous_collection(const char* output_file, int duration_sec) {
    JsonFileWriteConfig write_config = {
        .buffer_size = 4096,
        .temp_suffix = ".tmp",
        .sync_on_close = 1
    };

    FILE* stream = fopen(output_file, "w");
    if (!stream) {
        printf("Failed to open output file\n");
        return;
    }

    /* Write array start */
    fprintf(stream, "[\n");

    time_t start_time = time(NULL);
    int reading_count = 0;

    printf("Starting continuous data collection for %d seconds...\n", duration_sec);
    
    while (time(NULL) - start_time < duration_sec) {
        SensorReading reading = {
            .timestamp = time(NULL),
            .temperature = generate_temperature(),
            .valid = (rand() % 10 > 1)  /* 80% chance of valid reading */
        };

        JsonValue* reading_json = create_sensor_data(&reading, 1);
        JsonValue* single_reading = json_array_get(reading_json, 0);

        /* Write comma for all but first reading */
        if (reading_count > 0) {
            fprintf(stream, ",\n");
        }

        /* Write the reading */
        char* formatted = json_format_string(single_reading, &JSON_FORMAT_COMPACT);
        if (formatted) {
            fprintf(stream, "  %s", formatted);
            free(formatted);
        }

        fflush(stream);  /* Ensure data is written */
        reading_count++;

        json_free(reading_json);
        usleep(500000);  /* Sleep for 500ms */
    }

    /* Write array end */
    fprintf(stream, "\n]\n");
    fclose(stream);

    printf("Collected %d readings\n", reading_count);
}

/* Process collected data */
static void process_data_file(const char* input_file) {
    printf("\nProcessing collected data from %s\n", input_file);

    JsonValue* data = json_parse_file(input_file);
    if (!data) {
        const JsonError* error = json_get_file_error();
        printf("Failed to read data file: %s\n", error->message);
        return;
    }

    /* Clean data */
    JsonCleanStats stats;
    JsonValue* cleaned = json_clean_data(data, "temperature", &stats);
    
    if (cleaned) {
        printf("\nData Processing Results:\n");
        printf("----------------------\n");
        printf("Total readings: %zu\n", stats.original_count);
        printf("Valid readings: %zu\n", stats.cleaned_count);
        printf("Invalid readings: %zu\n", stats.removed_count);
        printf("Data quality: %.1f%%\n", 
               (float)stats.cleaned_count / stats.original_count * 100);

        /* Write cleaned data */
        char cleaned_file[256];
        snprintf(cleaned_file, sizeof(cleaned_file), "%s.cleaned", input_file);
        
        if (json_write_file_ex(cleaned, cleaned_file, NULL)) {
            printf("\nCleaned data written to %s\n", cleaned_file);
        } else {
            const JsonError* error = json_get_file_error();
            printf("Failed to write cleaned data: %s\n", error->message);
        }

        json_free(cleaned);
    }

    json_free(data);
}

int main() {
    srand(time(NULL));
    
    const char* data_file = "sensor_stream.json";
    
    /* Test 1: Continuous data collection */
    printf("=== Test 1: Continuous Data Collection ===\n");
    simulate_continuous_collection(data_file, 10);  /* Collect for 10 seconds */

    /* Test 2: Process collected data */
    printf("\n=== Test 2: Data Processing ===\n");
    process_data_file(data_file);

    /* Test 3: Partial file reading demonstration */
    printf("\n=== Test 3: Partial File Reading ===\n");
    JsonFileReader* reader = json_file_reader_create(data_file, 256);
    if (reader) {
        printf("Reading data in chunks:\n");
        int chunk_count = 0;
        while (json_file_reader_next(reader) != NULL) {
            chunk_count++;
            printf("\rProcessed chunk %d", chunk_count);
            fflush(stdout);
        }
        printf("\nFinished reading %d chunks\n", chunk_count);
        json_file_reader_free(reader);
    } else {
        const JsonError* error = json_get_file_error();
        printf("Failed to create file reader: %s\n", error->message);
    }

    return 0;
}