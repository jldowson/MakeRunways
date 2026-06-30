#ifndef BGLV9_H
#define BGLV9_H

#include <windows.h>

/*
    MSFS 2024 BGL v9 outer container.

    Confirmed against BglExplorer v0.9 and apx57210.bgl:
    - 0x38-byte file header
    - 0x14-byte layer descriptors
    - 0x10-byte QMID32 subsection entries
    - 0x14-byte QMID64 subsection entries when modeFlags bit 0x00010000 is set
    - all offsets are absolute from the beginning of the BGL file
*/

#define BGLV9_MAGIC                       0x19920201UL
#define BGLV9_QMID64_FLAG                 0x00010000UL
#define BGLV9_PARENT_QMID_COUNT           8UL

/* Known layer IDs recovered from BglExplorer's dispatch table. */
#define BGLV9_LAYER_AIRPORT               3UL
#define BGLV9_LAYER_VOR                   19UL
#define BGLV9_LAYER_NDB                   23UL
#define BGLV9_LAYER_MARKER                24UL
#define BGLV9_LAYER_BOUNDARY              32UL
#define BGLV9_LAYER_WAYPOINT              34UL
#define BGLV9_LAYER_GEOPOL                35UL
#define BGLV9_LAYER_3D_SCENERY            37UL
#define BGLV9_LAYER_AIRPORT_NAME_OLD      39UL
#define BGLV9_LAYER_VOR_INDEX_OLD         40UL
#define BGLV9_LAYER_NDB_INDEX_OLD         41UL
#define BGLV9_LAYER_WAYPOINT_INDEX_OLD    42UL
#define BGLV9_LAYER_GUID_SCENERY_MODELS   43UL
#define BGLV9_LAYER_AIRPORT_SUMMARY       44UL
#define BGLV9_LAYER_EXCLUSION_RECTANGLES  46UL
#define BGLV9_LAYER_TIME_ZONES            47UL
#define BGLV9_LAYER_MODEL_BOX             48UL
#define BGLV9_LAYER_LANDMARK_LOCATION     49UL
#define BGLV9_LAYER_VOR_INDEX             50UL
#define BGLV9_LAYER_NDB_INDEX             51UL
#define BGLV9_LAYER_WAYPOINT_INDEX        52UL
#define BGLV9_LAYER_AIRPORT_NAME          53UL
#define BGLV9_LAYER_TERRAIN_VECTOR_DB     101UL
/*
    Confirmed MSFS 2024 v9 record IDs used inside layer 19 (VOR).
    Record 0x0105 contains VOR, ILS and related radio-navigation facilities. The facilityType field distinguishes the subtype.*/
#define BGLV9_RECORD_VOR_ILS               0x0105U
#define BGLV9_RECORD_ILS_LOCALIZER         0x0115U
#define BGLV9_RECORD_ILS_GLIDESLOPE        0x0015U
#define BGLV9_RECORD_ILS_DME               0x0114U
#define BGLV9_RECORD_NAME                  0x0019U

/* Confirmed facility subtype in a BGLV9_RECORD_VOR_ILS record. */
#define BGLV9_VOR_FACILITY_ILS             4U
/*
    The packed VOR/ILS identifier is base-38 encoded and shifted left by six bits in MSFS 2024 v9.*/
#define BGLV9_VOR_IDENT_SHIFT              6U

/* Size of the fixed part before the first child record. */
#define BGLV9_VOR_ILS_FIXED_SIZE           0x30UL


#pragma pack(push, 1)

typedef struct _BGLV9_FILE_HEADER
{
    DWORD magic;                              /* +0x00: BGLV9_MAGIC */
    DWORD headerSize;                         /* +0x04: 0x38 */
    FILETIME fileTimeUtc;                     /* +0x08 */
    DWORD unknown10;                          /* +0x10 */
    DWORD layerCount;                         /* +0x14 */
    DWORD parentQmid[BGLV9_PARENT_QMID_COUNT];/* +0x18 */
} BGLV9_FILE_HEADER;

typedef struct _BGLV9_LAYER_DESCRIPTOR
{
    DWORD layerType;                          /* +0x00: BGLV9_LAYER_* */
    DWORD modeFlags;                          /* +0x04 */
    DWORD subsectionCount;                    /* +0x08 */
    DWORD indexOffset;                        /* +0x0C: absolute file offset */
    DWORD indexSize;                          /* +0x10: subsection-index bytes */
} BGLV9_LAYER_DESCRIPTOR;

typedef struct _BGLV9_QMID32_ENTRY
{
    DWORD packedQmid;                         /* +0x00 */
    DWORD itemCount;                          /* +0x04 */
    DWORD payloadOffset;                      /* +0x08: absolute file offset */
    DWORD payloadSize;                        /* +0x0C */
} BGLV9_QMID32_ENTRY;

typedef struct _BGLV9_QMID64_ENTRY
{
    ULONGLONG packedQmid;                     /* +0x00 */
    DWORD itemCount;                          /* +0x08 */
    DWORD payloadOffset;                      /* +0x0C: absolute file offset */
    DWORD payloadSize;                        /* +0x10 */
} BGLV9_QMID64_ENTRY;

/*Common six-byte record header used by the v9 VOR/ILS records and their child records.*/

typedef struct _BGLV9_RECORD_HEADER
{
    WORD recordType;                          /* +0x00 */
    DWORD recordSize;                         /* +0x02 */
} BGLV9_RECORD_HEADER;

/*
    Fixed 0x30-byte portion of record 0x0105.

    Child records, such as localizer, glideslope, DME and name, begin immediately after this structure.
*/
typedef struct _BGLV9_VOR_ILS_RECORD
{
    WORD recordType;                          /* +0x00: 0x0105 */
    DWORD recordSize;                         /* +0x02: complete record size */
    BYTE facilityType;                        /* +0x06: 4 = ILS */
    BYTE flags;                               /* +0x07 */
    LONG longitude;                           /* +0x08: packed longitude */
    LONG latitude;                            /* +0x0C: packed latitude */
    LONG altitude;                            /* +0x10: metres * 1000 */
    DWORD frequencyHz;                        /* +0x14: frequency in Hz */
    float rangeMeters;                        /* +0x18 */
    float magneticVariation;                  /* +0x1C */
    DWORD packedIdent;                        /* +0x20: base-38 << 6 */
    DWORD packedRegion;                       /* +0x24 */
    DWORD unknown28;                          /* +0x28 */
    DWORD unknown2C;                          /* +0x2C */
} BGLV9_VOR_ILS_RECORD;

/*
    MSFS 2024 localizer child record 0x0115.

    This extends the legacy 0x0014 localizer layout by one DWORD.
*/
typedef struct _BGLV9_ILS_LOCALIZER_RECORD
{
    WORD recordType;                          /* +0x00: 0x0115 */
    DWORD recordSize;                         /* +0x02: normally 0x14 */
    WORD unknown06;                           /* +0x06 */
    float heading;                            /* +0x08 */
    float width;                              /* +0x0C */
    DWORD unknown10;                          /* +0x10 */
} BGLV9_ILS_LOCALIZER_RECORD;

/*
    Glideslope child record 0x0015.
    Its 0x1C-byte layout remains compatible with the legacy glideslope record.
*/

typedef struct _BGLV9_ILS_GLIDESLOPE_RECORD
{
    WORD recordType;                          /* +0x00: 0x0015 */
    DWORD recordSize;                         /* +0x02: normally 0x1C */
    WORD unknown06;                           /* +0x06 */
    LONG longitude;                           /* +0x08 */
    LONG latitude;                            /* +0x0C */
    LONG altitude;                            /* +0x10: metres * 1000 */
    float rangeMeters;                        /* +0x14 */
    float pitchDegrees;                       /* +0x18 */
} BGLV9_ILS_GLIDESLOPE_RECORD;


#pragma pack(pop)

/* Compile-time layout checks, valid in both C and C++. */
typedef char BGLV9_ASSERT_FILE_HEADER_SIZE[(sizeof(BGLV9_FILE_HEADER) == 0x38) ? 1 : -1];
typedef char BGLV9_ASSERT_LAYER_DESCRIPTOR_SIZE[(sizeof(BGLV9_LAYER_DESCRIPTOR) == 0x14) ? 1 : -1];
typedef char BGLV9_ASSERT_QMID32_ENTRY_SIZE[(sizeof(BGLV9_QMID32_ENTRY) == 0x10) ? 1 : -1];
typedef char BGLV9_ASSERT_QMID64_ENTRY_SIZE[(sizeof(BGLV9_QMID64_ENTRY) == 0x14) ? 1 : -1];
typedef char BGLV9_ASSERT_RECORD_HEADER_SIZE[(sizeof(BGLV9_RECORD_HEADER) == 0x06) ? 1 : -1];
typedef char BGLV9_ASSERT_VOR_ILS_RECORD_SIZE[(sizeof(BGLV9_VOR_ILS_RECORD) == 0x30) ? 1 : -1];
typedef char BGLV9_ASSERT_ILS_LOCALIZER_SIZE[(sizeof(BGLV9_ILS_LOCALIZER_RECORD) == 0x14) ? 1 : -1];
typedef char BGLV9_ASSERT_ILS_GLIDESLOPE_SIZE[(sizeof(BGLV9_ILS_GLIDESLOPE_RECORD) == 0x1C) ? 1 : -1];

#endif /* BGLV9_H */
