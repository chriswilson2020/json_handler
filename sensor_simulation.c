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

int main() {
    srand(time(NULL));
    
    /* Simulate 20 readings, one every "second" */
    const size_t num_readings = 20;
    SensorReading readings[num_readings];
    
    /* Base timestamp */
    time_t base_time = time(NULL);
    
    /* Generate readings */
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
    
    /* Test different formatting options */
    printf("Default formatting:\n");
    char* default_output = json_format_string(data, &JSON_FORMAT_DEFAULT);
    printf("%s\n", default_output);
    free(default_output);
    
    printf("\nCompact formatting:\n");
    char* compact_output = json_format_string(data, &JSON_FORMAT_COMPACT);
    printf("%s\n", compact_output);
    free(compact_output);
    
    printf("\nPretty formatting:\n");
    char* pretty_output = json_format_string(data, &JSON_FORMAT_PRETTY);
    printf("%s\n", pretty_output);
    free(pretty_output);

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

    printf("\nCustom sensor formatting:\n");
    char* sensor_output = json_format_string(data, &sensor_config);
    printf("%s\n", sensor_output);
    free(sensor_output);
    
    /* Write to file */
    printf("\nWriting to sensor_data.json...\n");
    if (json_format_file(data, "sensor_data.json", &sensor_config)) {
        printf("Successfully wrote sensor data to file.\n");
    } else {
        printf("Failed to write sensor data to file.\n");
    }
    
    /* Cleanup */
    json_free(data);
    
    return 0;
}