#include "mec_common.h"

track_list_t* track_list_create(int initial_capacity) {
    track_list_t *list = mec_malloc(sizeof(track_list_t));
    if (!list) return NULL;
    
    list->tracks = mec_malloc(initial_capacity * sizeof(target_track_t));
    if (!list->tracks) {
        mec_free(list);
        return NULL;
    }
    
    list->count = 0;
    list->capacity = initial_capacity;
    return list;
}

void track_list_free(track_list_t *list) {
    if (list) {
        mec_free(list->tracks);
        mec_free(list);
    }
}

int track_list_add(track_list_t *list, const target_track_t *track) {
    if (!list || !track) return -1;
    
    if (list->count >= list->capacity) {
        int new_capacity = list->capacity * 2;
        target_track_t *new_tracks = mec_realloc(list->tracks, 
                                                new_capacity * sizeof(target_track_t));
        if (!new_tracks) return -1;
        
        list->tracks = new_tracks;
        list->capacity = new_capacity;
    }
    
    list->tracks[list->count] = *track;
    list->count++;
    return 0;
}

void track_list_clear(track_list_t *list) {
    if (list) {
        list->count = 0;
    }
}