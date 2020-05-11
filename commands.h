#include <time.h>

/**
* A commands definition file that is shared among mdp.c and simulator.c
*
* @author Vincent Nigro
* @version 0.0.1
*/

#define HALF_MINUTE 30
#define ONE_MINUTE 60
#define FRAME_SIZE 16
#define HEADER_WIDTH 4

const unsigned long         H1                  = 0x0ABCABCABCABCFFF; // 773682123238002687
const unsigned long         H2                  = 0x0CBACBACBACBAFFF; // 917269416852041727
const unsigned long         END                 = 0xFFFFFFFFFFFFFFFF; // -1
const unsigned long         KILL                = 0xA83732340C01F07C; // 12121212121212121212
const unsigned long         SOH                 = 0x7B5BAD595E238E38; // 8888888888888888888
const unsigned long         GOOD                = 0x56D2B19ED61DA482; // 6256258128126256258
const unsigned long         BAD                 = 0x70CCE976EA97BC7C; // 81281281124448128124
const unsigned long         ICING_ALARM         = 0x111636480DE784FF; // 1231231231231231231
const unsigned long         OVERHEAT_ALARM      = 0x3F5897499134DA54; // 4564564564564564564
const unsigned long         SENSOR_1_ALARM      = 0x146BB88485A0B17B; // 1471472582583693691
const unsigned long         SENSOR_2_ALARM      = 0x1116395028409722; // 1231234564567897890
const unsigned long         SENSOR_3_ALARM      = 0x1139C6736D1C49A7; // 1241241371371391399
const unsigned long         SENSOR_4_ALARM      = 0x0CEEE2C5E648074A; // 931931512512186186
const unsigned long         SENSOR_5_ALARM      = 0x78023A955400C1EA; // 8647538647538647530

/**
* Loops through the value passed to the function and converts to a
* an unsigned char *. Moves from left to right and assumes little
* endian binary format.
*
* @param size
* @param ptr
* @return void
*/
void printBits(size_t const size, void const * const ptr)
{
    unsigned char bit;
    unsigned char * b = (unsigned char *) ptr;

    for (int i = size - 1; i >= 0; i--)
    {
        for (int j = 7; j >= 0; j--)
        {
            // Shifts byte by J and & by 1
            bit = (b[i] >> j) & 1;
            printf("%u", bit);
        }
        printf(" ");
    }
    printf("\n");
    puts("");
}

/**
*
*
*/
void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds * (CLOCKS_PER_SEC / 1000);
    now = then = clock();

    while((now - then) < pause)
        now = clock();
}
