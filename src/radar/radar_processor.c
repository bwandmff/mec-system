#include "mec_radar.h"
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>

radar_processor_t* radar_processor_create(const radar_config_t *config) {
    if (!config) return NULL;
    
    radar_processor_t *processor = mec_malloc(sizeof(radar_processor_t));
    if (!processor) return NULL;
    
    processor->config = *config;
    processor->output_tracks = track_list_create(50);
    processor->fd = -1;
    
    if (!processor->output_tracks) {
        mec_free(processor);
        return NULL;
    }
    
    LOG_INFO("Created radar processor for radar %d", config->radar_id);
    return processor;
}

void radar_processor_destroy(radar_processor_t *processor) {
    if (!processor) return;
    
    radar_processor_stop(processor);
    if (processor->fd >= 0) {
        close(processor->fd);
    }
    track_list_free(processor->output_tracks);
    mec_free(processor);
}

static int setup_serial_port(const char *device_path, int baud_rate) {
    int fd = open(device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        LOG_ERROR("Failed to open radar device: %s", device_path);
        return -1;
    }
    
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        LOG_ERROR("Failed to get terminal attributes");
        close(fd);
        return -1;
    }
    
    // Configure serial port
    tty.c_cflag &= ~PARENB;   // No parity
    tty.c_cflag &= ~CSTOPB;   // One stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;       // 8 data bits
    tty.c_cflag &= ~CRTSCTS;  // No flow control
    tty.c_cflag |= CREAD | CLOCAL;
    
    tty.c_lflag &= ~ICANON;   // Non-canonical mode
    tty.c_lflag &= ~ECHO;     // No echo
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    
    tty.c_cc[VTIME] = 1;
    tty.c_cc[VMIN] = 0;
    
    // Set baud rate
    speed_t speed;
    switch (baud_rate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default:
            LOG_ERROR("Unsupported baud rate: %d", baud_rate);
            close(fd);
            return -1;
    }
    
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        LOG_ERROR("Failed to set terminal attributes");
        close(fd);
        return -1;
    }
    
    return fd;
}

int radar_processor_start(radar_processor_t *processor) {
    if (!processor) return -1;
    
    processor->fd = setup_serial_port(processor->config.device_path, processor->config.baud_rate);
    if (processor->fd < 0) {
        return -1;
    }
    
    if (thread_create(&processor->thread_ctx, radar_processing_thread, processor) != 0) {
        LOG_ERROR("Failed to start radar processing thread");
        close(processor->fd);
        processor->fd = -1;
        return -1;
    }
    
    LOG_INFO("Started radar processor for radar %d", processor->config.radar_id);
    return 0;
}

void radar_processor_stop(radar_processor_t *processor) {
    if (!processor) return;
    
    thread_destroy(&processor->thread_ctx);
    if (processor->fd >= 0) {
        close(processor->fd);
        processor->fd = -1;
    }
    LOG_INFO("Stopped radar processor for radar %d", processor->config.radar_id);
}

track_list_t* radar_processor_get_tracks(radar_processor_t *processor) {
    if (!processor) return NULL;
    return processor->output_tracks;
}

void* radar_processing_thread(void *arg) {
    radar_processor_t *processor = (radar_processor_t*)arg;
    if (!processor) return NULL;
    
    radar_detection_t detection;
    target_track_t track;
    
    while (processor->thread_ctx.running) {
        if (radar_read_data(processor, &detection) == 0) {
            if (radar_convert_to_track(&detection, &processor->config, &track) == 0) {
                thread_lock(&processor->thread_ctx);
                track_list_add(processor->output_tracks, &track);
                thread_unlock(&processor->thread_ctx);
            }
        }
        
        usleep(10000); // 10ms
    }
    
    return NULL;
}

int radar_read_data(radar_processor_t *processor, radar_detection_t *detection) {
    if (!processor || !detection || processor->fd < 0) return -1;
    
    // Simplified radar data reading - in practice would parse specific protocol
    static unsigned char buffer[1024];
    static int buffer_pos = 0;
    
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(processor->fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10ms timeout
    
    if (select(processor->fd + 1, &read_fds, NULL, NULL, &timeout) <= 0) {
        return -1;
    }
    
    int bytes_read = read(processor->fd, buffer + buffer_pos, sizeof(buffer) - buffer_pos);
    if (bytes_read <= 0) return -1;
    
    buffer_pos += bytes_read;
    
    // Look for complete packet (simplified - assume fixed size)
    if (buffer_pos >= 16) {
        // Parse radar data (simplified format)
        static int target_id = 2000;
        detection->target_id = target_id++;
        detection->range = ((buffer[0] << 8) | buffer[1]) * 0.1; // Convert to meters
        detection->angle = ((buffer[2] << 8) | buffer[3]) * 0.1 - 180.0; // Convert to degrees
        detection->velocity = ((buffer[4] << 8) | buffer[5]) * 0.1; // Convert to m/s
        detection->rcs = ((buffer[6] << 8) | buffer[7]) * 0.1; // Convert to dBm²
        gettimeofday(&detection->timestamp, NULL);
        
        // Move remaining data to beginning of buffer
        buffer_pos -= 16;
        memmove(buffer, buffer + 16, buffer_pos);
        
        return 0;
    }
    
    return -1;
}

int radar_convert_to_track(const radar_detection_t *detection, 
                          const radar_config_t *config, 
                          target_track_t *track) {
    if (!detection || !config || !track) return -1;
    
    // Convert polar to cartesian coordinates
    double x, y;
    if (radar_polar_to_cartesian(detection->range, detection->angle, &x, &y) != 0) {
        return -1;
    }
    
    track->id = detection->target_id;
    track->type = TARGET_VEHICLE; // Default, could be refined based on RCS
    track->position.latitude = y;  // Simplified - in practice would convert to WGS84
    track->position.longitude = x;
    track->position.altitude = 0.0;
    track->velocity = detection->velocity;
    track->heading = atan2(y, x) * 180.0 / M_PI;
    track->confidence = (detection->rcs > -10.0) ? 0.8 : 0.5; // Based on RCS
    track->sensor_id = config->radar_id;
    track->timestamp = detection->timestamp;
    
    return 0;
}

int radar_polar_to_cartesian(double range, double angle, double *x, double *y) {
    if (!x || !y) return -1;
    
    double angle_rad = angle * M_PI / 180.0;
    *x = range * cos(angle_rad);
    *y = range * sin(angle_rad);
    
    return 0;
}