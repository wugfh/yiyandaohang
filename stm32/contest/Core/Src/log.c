#include "log.h"

void log_nmea_buf(nmea_t* nmea){
    printf("\n\r");
    printf("*************************************\n\r");
    printf("%s\n\r", nmea->debug_buf);
    printf("*************************************\n\r");
    printf("\n\r");
}

