#include "mec_common.h"

#define MAX_LINE_LENGTH 1024
#define MAX_CONFIGS 100

typedef struct config_entry {
    char key[128];
    char value[256];
} config_entry_t;

struct config_t {
    config_entry_t entries[MAX_CONFIGS];
    int count;
};

static void trim_whitespace(char *str) {
    char *end;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';
}

config_t* config_load(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        LOG_ERROR("Failed to open config file: %s", filename);
        return NULL;
    }
    
    config_t *config = mec_malloc(sizeof(config_t));
    if (!config) {
        fclose(file);
        return NULL;
    }
    
    config->count = 0;
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file) && config->count < MAX_CONFIGS) {
        trim_whitespace(line);
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') continue;
        
        char *delimiter = strchr(line, '=');
        if (!delimiter) continue;
        
        *delimiter = '\0';
        char *key = line;
        char *value = delimiter + 1;
        
        trim_whitespace(key);
        trim_whitespace(value);
        
        strncpy(config->entries[config->count].key, key, sizeof(config->entries[config->count].key) - 1);
        strncpy(config->entries[config->count].value, value, sizeof(config->entries[config->count].value) - 1);
        config->count++;
    }
    
    fclose(file);
    LOG_INFO("Loaded %d configuration entries from %s", config->count, filename);
    return config;
}

void config_free(config_t *config) {
    mec_free(config);
}

const char* config_get_string(config_t *config, const char *key, const char *default_value) {
    if (!config || !key) return default_value;
    
    for (int i = 0; i < config->count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            return config->entries[i].value;
        }
    }
    
    return default_value;
}

int config_get_int(config_t *config, const char *key, int default_value) {
    const char *str_value = config_get_string(config, key, NULL);
    if (!str_value) return default_value;
    
    char *endptr;
    int value = strtol(str_value, &endptr, 10);
    if (*endptr != '\0') {
        LOG_WARN("Invalid integer value for key %s: %s", key, str_value);
        return default_value;
    }
    
    return value;
}

double config_get_double(config_t *config, const char *key, double default_value) {
    const char *str_value = config_get_string(config, key, NULL);
    if (!str_value) return default_value;
    
    char *endptr;
    double value = strtod(str_value, &endptr);
    if (*endptr != '\0') {
        LOG_WARN("Invalid double value for key %s: %s", key, str_value);
        return default_value;
    }
    
    return value;
}