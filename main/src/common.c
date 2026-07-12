

#include "common.h"

void print_hex(const uint8_t *data,
               uint8_t len)
{
    if(data == NULL || len == 0)
    {
        printf("<empty>\n");
        return;
    }

    for(int i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }

    printf("\n");
}