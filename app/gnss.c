#include <gnss.h>
#include <stdlib.h>
#include <string.h>
#include <port.h>




float read_lat(char * string){
	float min = strtof(string+2, NULL);
	string[2] = '\0';
	float deg = strtof(string, NULL);

	return deg + min/60.0f;
}

float read_lon(char * string){
	float min = strtof(string+3, NULL);
	string[3] = '\0';
	float deg = strtof(string, NULL);

	return deg + min/60.0f;
}

void gnss_decode_gga(gnss_context_t * decoder) {

    switch (decoder->word_count) {

        case GNSS_GGA_LATITUDE:
            decoder->data.latitude = read_lat((char *) decoder->accumulator);
            break;

        case GNSS_GGA_LONGITUDE:
            decoder->data.longitude = read_lon((char *) decoder->accumulator);
            break;

        case GNSS_GGA_NS:
            if (decoder->accumulator[0] == 'S') {
                decoder->data.latitude = decoder->data.latitude * (-1);
            }
            break;

        case GNSS_GGA_EW:
            if (decoder->accumulator[0] == 'W') {
                decoder->data.longitude = decoder->data.longitude * (-1);
            }
            break;

        case GNSS_GGA_ALTITUDE:
            decoder->data.altitude = strtof((char *) decoder->accumulator, NULL);
            break;

        case GNSS_GGA_ALT_UNIT:
            if (decoder->accumulator[0] == 'F') {
                decoder->data.altitude = decoder->data.altitude * GNSS_FEET_CONVERSION;
            }
            decoder->done = 1;
            break;

        case GNSS_GGA_TIME:
            decoder->data.time = strtof((char *) decoder->accumulator, NULL);
            break;

        case GNSS_GGA_HDOP:
            decoder->data.hdop = strtof((char *) decoder->accumulator, NULL);
            break;
    }
}

void gnss_decode_rmc(gnss_context_t * decoder) {

    switch (decoder->word_count) {

        case GNSS_RMC_LATITUDE:
        	//debug_log("GNSS: LAT\n");
            decoder->data.latitude = read_lat((char *) decoder->accumulator);
            break;

        case GNSS_RMC_LONGITUDE:
        	//debug_log("GNSS: LON\n");
            decoder->data.longitude = read_lon((char *) decoder->accumulator);
            break;

        case GNSS_RMC_NS:
            if (decoder->accumulator[0] == 'S') {
                decoder->data.latitude = decoder->data.latitude * (-1);
            }
            break;

        case GNSS_RMC_SPEED:
               decoder->data.speed = strtof((char *) decoder->accumulator, NULL)*0.514;
               break;

        case GNSS_RMC_EW:
            if (decoder->accumulator[0] == 'W') {
                decoder->data.longitude = decoder->data.longitude * (-1);
            }
            decoder->done = 1;
            break;

        case GNSS_RMC_TIME:
            decoder->data.time = strtof((char *) decoder->accumulator, NULL);
            decoder->data.altitude = 0;
            decoder->data.hdop = 0;
            break;
    }
}


void gnss_handle_fragment(gnss_context_t * decoder, volatile uint8_t c) {

	//debug_log("gnss handle frag: %c\n", c);

    switch (c) {

        case '$':
            decoder->accu_count = 0;
            decoder->word_count = 0;
            decoder->done = 0;
            break;

        case ',':
            decoder->accumulator[decoder->accu_count] = '\0';
            if(decoder->word_count == 0) {
            	//debug_log("gnss next: %s\n", decoder->accumulator);
                if (strcmp((char*)decoder->accumulator, "GNGGA") == 0) {
                	//debug_log("decoder: GGA\n");
                    decoder->type = GGA;
                }
                else if (strcmp((char*)decoder->accumulator, "GNRMC") == 0) {
                	//debug_log("decoder: RMC\n");
                    decoder->type = RMC;
                } else {
                    decoder->type = OTHER;
                }
            } else {
                switch (decoder->type) {
                    case GGA:
                        gnss_decode_gga(decoder);
                        break;
                    case RMC:
                        gnss_decode_rmc(decoder);
                        break;
                    default:
                        break;
                }
            }
            decoder->word_count += 1;
            decoder->accu_count = 0;
            break;

        default:
            decoder->accumulator[decoder->accu_count] = c;
            decoder->accu_count++;
            break;
    }
    //debug_log("ret from gnss: %d\n", decoder->stat);
}


static gnss_context_t gnss_decoder = {0};

void gnss_thread(void * arg) {




	for(;;) {
		//NMEA Parser

		if(uart_wait(0xffff) == e_success) {
			uint8_t * buffer;
			uint16_t len = uart_get_buffer(&buffer);
			for(uint16_t i = 0; i < len; i++) {
				gnss_handle_fragment(&gnss_decoder, buffer[i]);
				if(gnss_decoder.done) {
					//here save the data to the radioPacket or something
					//careful, save data with atomic operation.
				}
			}
		}
	}
}
