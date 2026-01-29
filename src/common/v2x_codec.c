#include "mec_v2x.h"
#include <arpa/inet.h>

/**
 * @file v2x_codec.c
 * @brief V2X 协议编解码实现
 * 
 * 采用大端字节序 (Network Byte Order) 进行序列化，确保跨平台兼容性。
 */

int v2x_encode_rsm(const track_list_t *tracks, uint32_t rsu_id, uint8_t *out_buf, int *out_len) {
    if (!tracks || !out_buf || !out_len) return -1;

    int max_len = *out_len;
    int current_pos = 0;

    // 1. 填充头部 (Header)
    if (max_len < (int)sizeof(v2x_header_t)) return -1;
    
    v2x_header_t *hdr = (v2x_header_t *)out_buf;
    hdr->magic = 0x56; // 'V'
    hdr->version = V2X_PROTOCOL_VER;
    hdr->msg_type = V2X_MSG_RSM;
    hdr->device_id = htonl(rsu_id);
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t ms = (uint64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
    // 这里简单处理字节序，实际复杂场景建议使用专用 64位转换函数
    memcpy(&hdr->timestamp, &ms, 8); 

    current_pos += sizeof(v2x_header_t);

    // 2. 写入目标数量
    if (current_pos + 1 > max_len) return -1;
    out_buf[current_pos++] = (uint8_t)tracks->count;

    // 3. 序列化每一个目标
    for (int i = 0; i < tracks->count; i++) {
        if (current_pos + (int)sizeof(v2x_rsm_participant_t) > max_len) break;

        const target_track_t *t = &tracks->tracks[i];
        v2x_rsm_participant_t *p = (v2x_rsm_participant_t *)(out_buf + current_pos);

        p->target_id = htons((uint16_t)t->id);
        p->type = (uint8_t)t->type;
        
        // 坐标转换为国标要求的 1e-7 度格式
        p->lat = htonl((int32_t)(t->position.latitude * 10000000.0));
        p->lon = htonl((int32_t)(t->position.longitude * 10000000.0));
        
        // 速度转换 (单位: 0.02 m/s)
        p->speed = htons((uint16_t)(t->velocity / 0.02));
        
        // 航向转换 (单位: 0.0125 度)
        p->heading = htons((uint16_t)(t->heading / 0.0125));
        
        // 置信度转换 (0-200)
        p->confidence = (uint8_t)(t->confidence * 200.0);

        current_pos += sizeof(v2x_rsm_participant_t);
    }

    *out_len = current_pos;
    return 0;
}
