#include <gnss.h>
#include <stdlib.h>
#include <string.h>
#include <port.h>
#include <cmsis_os.h>
#include <init.h>


#if GROUND_STATION == 0

float _strtof_powersOf10_[] = {
					10.0f,
					100.0f,
					1.0e4f,
					1.0e8f,
					1.0e16f,
					1.0e32f
};

float read_float(char *string,char **endPtr ) {
	int sign, expSign = 0;
	float fraction, dblExp;
	float *d;
	char *p;
	int c;
	int exp = 0;
	int fracExp = 0;
	int mantSize;
	int decPt;
	char *pExp;
	p = string;
	if (*p == '-') {
		sign = 1;
		p += 1;
	} else {
		if (*p == '+') {
			p += 1;
		}
		sign = 0;
	}
	decPt = -1;
	for (mantSize = 0; ; mantSize += 1)
	{
		c = *p;
		if (!(c >= '0' && c <= '9')) {
			if ((c != '.') || (decPt >= 0)) {
				break;
			}
			decPt = mantSize;
		}
		p += 1;
	}
	pExp  = p;
	p -= mantSize;
	if (decPt < 0) {
		decPt = mantSize;
	} else {
		mantSize -= 1;
	}
	if (mantSize > 18) {
		fracExp = decPt - 18;
		mantSize = 18;
	} else {
		fracExp = decPt - mantSize;
	}
	if (mantSize == 0) {
		fraction = 0.0;
		p = string;
		goto done;
	} else {
		int frac1, frac2;
		frac1 = 0;
		for ( ; mantSize > 9; mantSize -= 1)
		{
			c = *p;
			p += 1;
			if (c == '.') {
				c = *p;
				p += 1;
			}
			frac1 = 10*frac1 + (c - '0');
		}
		frac2 = 0;
		for (; mantSize > 0; mantSize -= 1)
		{
			c = *p;
			p += 1;
			if (c == '.') {
				c = *p;
				p += 1;
			}
			frac2 = 10*frac2 + (c - '0');
		}
		fraction = (1.0e9f * frac1) + frac2;
	}
	p = pExp;
	if ((*p == 'E') || (*p == 'e')) {
		p += 1;
		if (*p == '-') {
			expSign = 1;
			p += 1;
		} else {
			if (*p == '+') {
				p += 1;
			}
			expSign = 0;
		}
		while (*p >= '0' && *p <= '9') {
			exp = exp * 10 + (*p - '0');
			p += 1;
		}
	}
	if (expSign) {
		exp = fracExp - exp;
	} else {
		exp = fracExp + exp;
	}

	if (exp < 0) {
		expSign = 1;
		exp = -exp;
	} else {
		expSign = 0;
	}
	const int maxExponent = 38;

	if (exp > maxExponent) {
		exp = maxExponent;
	}
	dblExp = 1.0f;
	for (d = _strtof_powersOf10_; exp != 0; exp >>= 1, d += 1) {
		if (exp & 01) {
			dblExp *= *d;
		}
	}
	if (expSign) {
		fraction /= dblExp;
	} else {
		fraction *= dblExp;
	}
done:
	if (endPtr != NULL) {
		*endPtr = (char *) p;
	}
	if (sign) {
		return -fraction;
	}
	return fraction;
}


float read_lat(char * string){
	float min = read_float(string+2, NULL);
	string[2] = '\0';
	float deg = read_float(string, NULL);

	return deg + min/60.0f;
}

float read_lon(char * string){
	float min = read_float(string+3, NULL);
	string[3] = '\0';
	float deg = read_float(string, NULL);

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
            decoder->data.altitude = read_float((char *) decoder->accumulator, NULL);
            break;

        case GNSS_GGA_ALT_UNIT:
            if (decoder->accumulator[0] == 'F') {
                decoder->data.altitude = decoder->data.altitude * GNSS_FEET_CONVERSION;
            }
            decoder->done = 1;
            break;

        case GNSS_GGA_TIME:
            decoder->data.time = read_float((char *) decoder->accumulator, NULL);
            break;

        case GNSS_GGA_HDOP:
            decoder->data.hdop = read_float((char *) decoder->accumulator, NULL);
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
               decoder->data.speed = read_float((char *) decoder->accumulator, NULL)*0.514;
               break;

        case GNSS_RMC_EW:
            if (decoder->accumulator[0] == 'W') {
                decoder->data.longitude = decoder->data.longitude * (-1);
            }
            decoder->done = 1;
            break;

        case GNSS_RMC_TIME:
            decoder->data.time = read_float((char *) decoder->accumulator, NULL);
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


static gnss_data_t gnss_data = {0};



gnss_data_t gnss_get_data(void) {
	return gnss_data;
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
					taskENTER_CRITICAL();
					if(gnss_decoder.data.altitude != 0) {
						gnss_data.altitude = gnss_decoder.data.altitude;
						gnss_data.hdop = gnss_decoder.data.hdop;
					}
					gnss_data.longitude = gnss_decoder.data.longitude;
					gnss_data.latitude = gnss_decoder.data.latitude;

					gnss_data.speed = gnss_decoder.data.speed;
					//here save the data to the radioPacket or something
					//careful, save data with atomic operation.
					taskEXIT_CRITICAL();
				}
			}
		}
	}
}

#endif
