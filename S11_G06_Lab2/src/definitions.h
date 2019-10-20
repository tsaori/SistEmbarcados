#define MAXCOUNTER 0xFFFFFF

//MAX SYSTICK PERIOD 16777216
//To systick works correctly, the systemclock must fast then 8MHz
//datasheet p. 152
//#define MICROFREQUENCY 120000000
//
//#if MICROFREQUENCY>67108684
//    #define FrequencyDivider 8
//#elif MICROFREQUENCY>33554432
//    #define FrequencyDivider 4
//#elif MICROFREQUENCY>16777216
//    #define FrequencyDivider 2
//#else
//    #define FrequencyDivider 1
//#endif

#define FREQMIN 10000000
#define FREQMAX 120000000