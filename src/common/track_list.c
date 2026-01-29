#include "mec_common.h"

track_list_t* track_list_create(int initial_capacity) {
    track_list_t *list = malloc(sizeof(track_list_t));
    if (!list) return NULL;
    
    // 从高性能内存池分配航迹缓冲区
    list->tracks = mec_malloc(initial_capacity * sizeof(target_track_t));
    if (!list->tracks) {
        mec_free(list);
        return NULL;
    }
    
    list->count = 0;
    list->capacity = initial_capacity;
    list->ref_count = 1; // 初始引用为 1
    pthread_mutex_init(&list->ref_lock, NULL);
    
    return list;
}

void track_list_retain(track_list_t *list) {
    if (!list) return;
    pthread_mutex_lock(&list->ref_lock);
    list->ref_count++;
    pthread_mutex_unlock(&list->ref_lock);
}

void track_list_release(track_list_t *list) {
    if (!list) return;
    
    int destroy = 0;
    pthread_mutex_lock(&list->ref_lock);
    list->ref_count--;
    if (list->ref_count <= 0) {
        destroy = 1;
    }
    pthread_mutex_unlock(&list->ref_lock);
    
    if (destroy) {
        pthread_mutex_destroy(&list->ref_lock);
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
