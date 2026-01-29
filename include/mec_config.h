#ifndef MEC_CONFIG_H
#define MEC_CONFIG_H

typedef struct {
    char *key;
    char *value;
} config_entry_t;

typedef struct {
    config_entry_t *entries;
    int count;
    int capacity;
} config_t;

config_t* config_load(const char *filename);
void config_free(config_t *config);
const char* config_get_string(config_t *config, const char *key, const char *default_value);
int config_get_int(config_t *config, const char *key, int default_value);
double config_get_double(config_t *config, const char *key, double default_value);

#endif // MEC_CONFIG_H
