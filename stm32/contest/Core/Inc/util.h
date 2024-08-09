#pragma once

#include "nmea.h"

int utf8_to_ucs2(unsigned char *utf8, unsigned char *ucs2);
int str_utf8_2_ucs2(char* dest, char* src);
void log_nmea_buf(nmea_t* nmea);
int remove_html_tag(char* dest, char* src);

