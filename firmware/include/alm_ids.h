#ifndef FIRMWARE_INCLUDE_ALM_IDS_H
#define FIRMWARE_INCLUDE_ALM_IDS_H

#include <stdint.h>

typedef enum {
    ALM_001 = 1,  ALM_002,  ALM_003,  ALM_004,  ALM_005,
    ALM_006,      ALM_007,  ALM_008,  ALM_009,  ALM_010,
    ALM_011,      ALM_012,  ALM_013,  ALM_014,  ALM_015,
    ALM_016,      ALM_017,  ALM_018,  ALM_019,  ALM_020,
    ALM_021,      ALM_022,  ALM_023,  ALM_024,  ALM_025,
    ALM_026,      ALM_027,  ALM_028,  ALM_029,  ALM_030,
    ALM_031,      ALM_032,  ALM_033,  ALM_034,  ALM_035,
    ALM_036,      ALM_037,  ALM_038,  ALM_039,  ALM_040,
    ALM_041,      ALM_042,  ALM_043,  ALM_044,  ALM_045,
    ALM_046,      ALM_047,  ALM_048,  ALM_049,  ALM_050,
    ALM_051,      ALM_052,  ALM_053,  ALM_054,  ALM_055,
    ALM_056,      ALM_057,  ALM_058,  ALM_059,  ALM_060,
    ALM_061,      ALM_062,  ALM_063,  ALM_064,  ALM_065
} alm_id_t;

#define ALM_ID_MIN   ((uint16_t)ALM_001)
#define ALM_ID_MAX   ((uint16_t)ALM_065)

#endif
