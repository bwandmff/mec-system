#include "mec_common.h"
#include "mec_config.h"
#include "mec_logging.h"

#define MAX_LINE_LENGTH 1024
#define MAX_CONFIGS 100

// We need to define the struct config_t internals here as they are opaque in the header
// But wait, the header already typedef'd it. Let's adjust to match the implementation structure
// or adjust implementation to match header. The header used dynamic arrays (char*), implementation fixed size.
// Let's stick to the implementation's simplicity but make it compatible with the typedef in header.

// The header defines:
/*
typedef struct {
    char *key;
    char *value;
} config_entry_t;

typedef struct {
    config_entry_t *entries;
    int count;
    int capacity;
} config_t;
*/

// The current implementation uses fixed arrays. Let's rewrite to match the dynamic header definition.

static void trim_whitespace(char *str) {
    char *end;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == 0) return;
    
    // Shift content to beginning
    if (str > str) {
        char *src = str;
        char *dst = str; // this logic is wrong if we don't know original start.
        // Easier:
    }
    // Re-implement trim more safely for in-place
    // Actually, the original implementation was modifying the buffer passed in.
    
    // Let's use a simple approach: return pointer to first non-space, zero terminate end.
}
// Helper to trim in-place (assuming mutable buffer)
static char* trim(char *str) {
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
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
    
    config->capacity = 16;
    config->count = 0;
    config->entries = mec_malloc(sizeof(config_entry_t) * config->capacity);
    
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        char *trimmed = trim(line);
        
        // Skip empty lines and comments
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;
        
        char *delimiter = strchr(trimmed, '=');
        if (!delimiter) continue;
        
        *delimiter = '\0';
        char *key = trim(trimmed);
        char *value = trim(delimiter + 1);
        
        if (config->count >= config->capacity) {
            config->capacity *= 2;
            config->entries = mec_realloc(config->entries, sizeof(config_entry_t) * config->capacity);
        }
        
        config->entries[config->count].key = strdup(key);
        config->entries[config->count].value = strdup(value);
        config->count++;
    }
    
    fclose(file);
    LOG_INFO("Loaded %d configuration entries from %s", config->count, filename);
    return config;
}

void config_free(config_t *config) {
    if (!config) return;
    for (int i = 0; i < config->count; i++) {
        free(config->entries[i].key);
        free(config->entries[i].value);
    }
    mec_free(config->entries);
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
