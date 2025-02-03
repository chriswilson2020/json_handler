/* json.c */
#include "json.h"
#include <math.h>

/* Helper function to create a new JsonValue */
static JsonValue *json_value_init(void)
{
    JsonValue *value = (JsonValue *)malloc(sizeof(JsonValue));
    if (value)
    {
        memset(value, 0, sizeof(JsonValue));
    }
    return value;
}

/* Helpler functionn to check if a value is valid for output */
int json_is_valid_for_output(const JsonValue *value)
{
    if (!value)
        return 0;
    if (value->type == JSON_NUMBER && isnan(value->value.number))
    {
        return 0;
    }
    return 1;
}

JsonValue *json_create_null(void)
{
    JsonValue *value = json_value_init();
    if (value)
    {
        value->type = JSON_NULL;
    }
    return value;
}

JsonValue *json_create_boolean(int boolean_value)
{
    JsonValue *value = json_value_init();
    if (value)
    {
        value->type = JSON_BOOLEAN;
        value->value.boolean = boolean_value;
    }
    return value;
}

JsonValue *json_create_number(double number_value)
{

    JsonValue *value = json_value_init();
    if (value)
    {
        value->type = JSON_NUMBER;
        value->value.number = number_value;
    }
    return value;
}

JsonValue *json_create_string(const char *string_value)
{
    JsonValue *value = json_value_init();
    if (value && string_value)
    {
        value->type = JSON_STRING;
        value->value.string = strdup(string_value);
        if (!value->value.string)
        {
            free(value);
            return NULL;
        }
    }
    return value;
}

/* Helper function to count non-NaN values in an array */
static size_t count_valid_array_items(const JsonArray *array)
{
    size_t count = 0;
    for (size_t i = 0; i < array->size; i++)
    {
        if (!array->items[i] ||
            (array->items[i]->type == JSON_NUMBER && isnan(array->items[i]->value.number)))
        {
            continue;
        }
        count++;
    }
    return count;
}

JsonValue *json_create_array(void)
{
    JsonValue *value = json_value_init();
    if (value)
    {
        value->type = JSON_ARRAY;
        value->value.array = (JsonArray *)malloc(sizeof(JsonArray));
        if (!value->value.array)
        {
            free(value);
            return NULL;
        }
        value->value.array->items = NULL;
        value->value.array->size = 0;
        value->value.array->capacity = 0;
    }
    return value;
}

JsonValue *json_create_object(void)
{
    JsonValue *value = json_value_init();
    if (value)
    {
        value->type = JSON_OBJECT;
        value->value.object = (JsonObject *)malloc(sizeof(JsonObject));
        if (!value->value.object)
        {
            free(value);
            return NULL;
        }
        value->value.object->pairs = NULL;
        value->value.object->size = 0;
    }
    return value;
}

/* Modified print function to handle NaN values */
void json_print_value(const JsonValue *value, int indent_level)
{
    if (!value)
    {
        printf("null");
        return;
    }

    /* Skip NaN values in arrays/objects */
    if (value->type == JSON_NUMBER && isnan(value->value.number))
    {
        return;
    }

    /* Print indentation */
    for (int i = 0; i < indent_level; i++)
    {
        printf("  ");
    }

    switch (value->type)
    {
        case JSON_NULL:
            printf("null");
            break;

        case JSON_BOOLEAN:
            printf("%s", value->value.boolean ? "true" : "false");
            break;

        case JSON_NUMBER:
            if (fabs(value->value.number) < 0.0001 || fabs(value->value.number) > 100000.0)
            {
                printf("%e", value->value.number);
            }
            else
            {
                printf("%.6f", value->value.number);
            }
            break;

        case JSON_STRING:
            printf("\"%s\"", value->value.string);
            break;

        case JSON_ARRAY:
            {
                printf("[\n");
                size_t valid_count = count_valid_array_items(value->value.array);
                size_t printed = 0;
                
                for (size_t i = 0; i < value->value.array->size; i++)
                {
                    JsonValue *item = value->value.array->items[i];
                    if (!json_is_valid_for_output(item))
                    {
                        continue;
                    }
                    
                    json_print_value(item, indent_level + 1);
                    printed++;
                    
                    if (printed < valid_count)
                    {
                        printf(",");
                    }
                    printf("\n");
                }
                
                for (int i = 0; i < indent_level; i++)
                {
                    printf("  ");
                }
                printf("]");
            }
            break;

        case JSON_OBJECT:
            {
                printf("{\n");
                JsonKeyValue *pair = value->value.object->pairs;
                int first = 1;
                
                while (pair)
                {
                    if (!json_is_valid_for_output(pair->value))
                    {
                        pair = pair->next;
                        continue;
                    }
                    
                    if (!first)
                    {
                        printf(",\n");
                    }
                    first = 0;
                    
                    for (int i = 0; i < indent_level + 1; i++)
                    {
                        printf("  ");
                    }
                    printf("\"%s\": ", pair->key);
                    json_print_value(pair->value, 0);
                    pair = pair->next;
                }
                printf("\n");
                
                for (int i = 0; i < indent_level; i++)
                {
                    printf("  ");
                }
                printf("}");
            }
            break;
    }
}

/* Memory cleanup function */
void json_free(JsonValue *value)
{
    if (!value)
        return;

    switch (value->type)
    {
    case JSON_STRING:
        free(value->value.string);
        break;
    case JSON_ARRAY:
        if (value->value.array)
        {
            for (size_t i = 0; i < value->value.array->size; i++)
            {
                json_free(value->value.array->items[i]);
            }
            free(value->value.array->items);
            free(value->value.array);
        }
        break;
    case JSON_OBJECT:
        if (value->value.object)
        {
            JsonKeyValue *current = value->value.object->pairs;
            while (current)
            {
                JsonKeyValue *next = current->next;
                free(current->key);
                json_free(current->value);
                free(current);
                current = next;
            }
            free(value->value.object);
        }
        break;
    default:
        break;
    }
    free(value);
}

/* Array manipulation functions */
int json_array_append(JsonValue *array_value, JsonValue *value)
{
    if (!array_value || array_value->type != JSON_ARRAY)
    {
        return 0; // Error: invalid parameters
    }

    // Allow NULL or NaN values to be stored
    JsonArray *array = array_value->value.array;

    /* Check if we need to resize the array */
    if (array->size >= array->capacity)
    {
        size_t new_capacity = array->capacity == 0 ? 8 : array->capacity * 2;
        JsonValue **new_items = (JsonValue **)realloc(array->items,
                                                      new_capacity * sizeof(JsonValue *));
        if (!new_items)
        {
            return 0; // Error: memory allocation failed
        }

        array->items = new_items;
        array->capacity = new_capacity;
    }

    /* Add the new value to the array */
    array->items[array->size++] = value;
    return 1; // Success
}

JsonValue *json_array_get(const JsonValue *array_value, size_t index)
{
    if (!array_value || array_value->type != JSON_ARRAY)
    {
        return NULL; // Error: invalid array
    }

    JsonArray *array = array_value->value.array;
    if (!array || !array->items || index >= array->size)
    {
        return NULL; // Error: invalid array structure or index out of bounds
    }

    return array->items[index];
}



/* Helper function to create a new key-value pair */
static JsonKeyValue *create_key_value_pair(const char *key, JsonValue *value)
{
    JsonKeyValue *pair = (JsonKeyValue *)malloc(sizeof(JsonKeyValue));
    if (!pair)
    {
        return NULL;
    }

    pair->key = strdup(key);
    if (!pair->key)
    {
        free(pair);
        return NULL;
    }

    pair->value = value;
    pair->next = NULL;
    return pair;
}

/* Modified Object set to handle NaN values */
int json_object_set(JsonValue *object_value, const char *key, JsonValue *value)
{
    if (!object_value || object_value->type != JSON_OBJECT || !key )
    {
        return 0; // Error: invalid parameters
    }

    // Allow NULL or NaN values to be stored
    JsonObject *object = object_value->value.object;

    /* First check if the key already exists */
    JsonKeyValue *current = object->pairs;
    while (current)
    {
        if (strcmp(current->key, key) == 0)
        {
            /* Key exists, update the value */
            json_free(current->value); // Free the old value
            current->value = value;
            return 1;
        }
        current = current->next;
    }

    /* Key doesn't exist, create a new pair */
    JsonKeyValue *new_pair = create_key_value_pair(key, value);
    if (!new_pair)
    {
        return 0; // Error: memory allocation failed
    }

    /* Add the new pair at the beginning of the list */
    new_pair->next = object->pairs;
    object->pairs = new_pair;
    object->size++;

    return 1;
}

JsonValue *json_object_get(const JsonValue *object_value, const char *key)
{
    if (!object_value || object_value->type != JSON_OBJECT || !key)
    {
        return NULL; // Error: invalid parameters
    }

    JsonObject *object = object_value->value.object;
    JsonKeyValue *current = object->pairs;

    /* Search for the key */
    while (current)
    {
        if (strcmp(current->key, key) == 0)
        {
            return current->value;
        }
        current = current->next;
    }
    return NULL; // Key not found
}
