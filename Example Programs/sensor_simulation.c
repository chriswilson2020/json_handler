#include "json.h"
#include <time.h>
#include <math.h>
#include <unistd.h>

/* Simulated sensor reading with occasional data loss */
typedef struct {
    time_t timestamp;
    double temperature;
    int valid;
} SensorReading;

/* Generate simulated temperature between 20-25°C with some noise */
static double generate_temperature() {
    return 22.5 + ((double)rand() / RAND_MAX - 0.5) * 5.0;
}

/* Create a JSON array of sensor readings */
static JsonValue* create_sensor_data(SensorReading* readings, size_t count) {
    JsonValue* array = json_create_array();
    
    for (size_t i = 0; i < count; i++) {
        JsonValue* reading = json_create_object();
        
        /* Convert timestamp to string */
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", 
                localtime(&readings[i].timestamp));
        
        /* Add timestamp */
        json_object_set(reading, "timestamp", json_create_string(timestamp));
        
        /* Add temperature - NaN for invalid readings */
        json_object_set(reading, "temperature", 
                       json_create_number(readings[i].valid ? readings[i].temperature : NAN));
        
        json_array_append(array, reading);
    }
    
    return array;
}

void output_data(const char* title, JsonValue* data, const JsonFormatConfig* config) {
    printf("\n%s:\n", title);
    char* formatted = json_format_string(data, config);
    if (formatted) {
        printf("%s\n", formatted);
        free(formatted);
    }
}

int main() {
    srand(time(NULL));
    
    /* Simulate 20 readings, one every "second" */
    const size_t num_readings = 20;
    SensorReading readings[num_readings];
    
    /* Base timestamp */
    time_t base_time = time(NULL);
    
    /* Generate readings */
    printf("Generating sensor data with simulated data loss...\n");
    for (size_t i = 0; i < num_readings; i++) {
        readings[i].timestamp = base_time + i;
        readings[i].valid = 1;  /* Assume valid by default */
        readings[i].temperature = generate_temperature();
        
        /* Simulate data loss between readings 8-12 */
        if (i >= 8 && i <= 12) {
            readings[i].valid = 0;
        }
    }
    
    /* Create JSON array of readings */
    JsonValue* data = create_sensor_data(readings, num_readings);
    
    /* Custom formatting for sensor data */
    JsonFormatConfig sensor_config = {
        .indent_string = "  ",
        .line_end = "\n",
        .spaces_after_colon = 1,
        .spaces_after_comma = 0,
        .max_inline_length = 60,
        .number_format = JSON_NUMBER_FORMAT_DECIMAL,
        .precision = 2,  /* 2 decimal places for temperature */
        .inline_simple_arrays = 0,
        .sort_object_keys = 1
    };

    /* Output original data */
    output_data("Original sensor data (with missing values)", data, &sensor_config);

    /* Clean data and output statistics */
    printf("\nCleaning sensor data...\n");
    JsonCleanStats stats;
    JsonValue* cleaned = json_clean_data(data, "temperature", &stats);
    
    if (cleaned) {
        printf("\nData Cleaning Statistics:\n");
        printf("------------------------\n");
        printf("Original readings: %zu\n", stats.original_count);
        printf("Valid readings: %zu\n", stats.cleaned_count);
        printf("Removed readings: %zu\n", stats.removed_count);
        printf("Data loss percentage: %.1f%%\n", 
               (float)stats.removed_count / stats.original_count * 100);
        
        /* Output cleaned data */
        output_data("Cleaned sensor data", cleaned, &sensor_config);
        
        /* Write both original and cleaned data to files */
        printf("\nWriting data to files...\n");
        if (json_format_file(data, "sensor_data_original.json", &sensor_config)) {
            printf("Successfully wrote original data to sensor_data_original.json\n");
        } else {
            printf("Failed to write original data to file\n");
        }
        
        if (json_format_file(cleaned, "sensor_data_cleaned.json", &sensor_config)) {
            printf("Successfully wrote cleaned data to sensor_data_cleaned.json\n");
        } else {
            printf("Failed to write cleaned data to file\n");
        }
        
        json_free(cleaned);
    } else {
        printf("Failed to clean data\n");
    }
    
    /* Cleanup */
    json_free(data);
    
    return 0;
}