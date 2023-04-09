#ifndef GNSS_H
#define GNSS_H

#include <stdint.h>

#define ACCU_SIZE 128
#define GNSS_GGA_TIME 1
#define GNSS_GGA_LATITUDE 2
#define GNSS_GGA_NS 3
#define GNSS_GGA_LONGITUDE 4
#define GNSS_GGA_EW 5
#define GNSS_GGA_HDOP 8
#define GNSS_GGA_ALTITUDE 9
#define GNSS_GGA_ALT_UNIT 10
#define GNSS_FEET_CONVERSION 0.3048f


#define GNSS_RMC_TIME 1
#define GNSS_RMC_LATITUDE 3
#define GNSS_RMC_NS 4
#define GNSS_RMC_LONGITUDE 5
#define GNSS_RMC_EW 6
#define GNSS_RMC_SPEED 7


typedef struct gnss_data {
    float longitude;
    float latitude;
    float altitude;
    float speed;
    float time;
    float hdop;
}gnss_data_t;

typedef enum gnss_trame_type {
    GGA,
    RMC,
    OTHER
}gnss_trame_type_t;

typedef struct gnss_context {
    gnss_trame_type_t type;
    uint8_t accumulator[ACCU_SIZE];
    uint16_t accu_count;
    uint16_t word_count;
    gnss_data_t data;
    uint8_t done;
} gnss_context_t;


void gnss_thread(void * arg);


#endif /* GNSS_H */
