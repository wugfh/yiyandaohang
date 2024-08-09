#include "util.h"

void log_nmea_buf(nmea_t* nmea){
    printf("\n\r");
    printf("*************************************\n\r");
    printf("%s\n\r", nmea->debug_msg.buf);
    printf("*************************************\n\r");
    printf("\n\r");
}

int remove_html_tag(char* dest, char* src){
    if(src == NULL || dest == NULL){
        return -1;
    }
    int i = 0, j = 0;
    while(src[i] != '\0'){
        if(src[i] == '<' ){
            while(src[i] != '>') ++i;
            ++i;
        }
        dest[j++] = src[i++];
    }
    dest[j++] = '\0';
    return j;
}


int utf8_to_ucs2(unsigned char *utf8, unsigned char *ucs2) {
    unsigned int ch;
    int utflen = 0;
    if ((utf8[0] & 0xE0) == 0xC0) { // 2 bytes UTF-8 sequence
        ch = (utf8[0] & 0x1F) << 6;
        ch |= utf8[1] & 0x3F;
        ucs2[1] = ch & 0xFF;
        ucs2[0] = (ch >> 8) & 0xFF;
        utflen = 2;
    } else if ((utf8[0] & 0xF0) == 0xE0) { // 3 bytes UTF-8 sequence
        ch = (utf8[0] & 0x0F) << 12;
        ch |= (utf8[1] & 0x3F) << 6;
        ch |= utf8[2] & 0x3F;
        ucs2[1] = ch & 0xFF;
        ucs2[0] = (ch >> 8) & 0xFF;
        utflen = 3;
    } else { // ASCII character or invalid UTF-8 sequence
        ucs2[1] = utf8[0];
        ucs2[0] = '\0'; // Only the lower byte is significant for ASCII chars in UCS-2LE
        utflen = 1;
    }
    return utflen;
}

int str_utf8_2_ucs2(char* dest, char* src){
    if(src == NULL || dest == NULL){
        return 0;
    }
    int i = 0, j = 0;
    while(src[i] != '\0'){
        int utflen = utf8_to_ucs2((uint8_t*)&src[i], (uint8_t*)&dest[j]);
        j += 2;
        i += utflen;
    }
    dest[j++] = '\0';
    return j;
}

