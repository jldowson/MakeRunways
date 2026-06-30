#ifndef ITC_FULLAFD_H
#define ITC_FULLAFD_H

#include <windows.h>

/*
    itc_FullAFD.h

    Consolidated AFD/BGL reference header for MakeRunways.
    Base: NewAFD.h.
    Integrations: non-duplicate reference constants and structures from Full New AFD.h.

    Notes:
    - This file is a reference/archive header.
    - It should not replace NewAFD.h in the active parser unless explicitly tested.
    - Semantically duplicate legacy aliases were intentionally not copied.
*/

extern char *pPathName;
extern char *pSceneryName;
extern char *pMaterials;

#pragma pack(push, 1)

/*==========================================================================
    Section 1 - Consolidated MakeRunways base from NewAFD.h
  ==========================================================================*/

// Thanks are due to Alessandro G. Antonini, author of BGLXML, for much of this data


typedef struct _NSECTS
{
	DWORD nObjType;			//  0 layer/object type
	DWORD nUnk1;			//  4 v9 modeFlags; bit 0x00010000 = QMID64
	DWORD nGroupsCount;		//  8 subsection count
	DWORD nGroupOffset;		// 12 absolute offset of subsection index
	DWORD nTableSize;		// 16 subsection-index size in bytes
} NSECTS;

#define NSECTS_PER_FILE 32

typedef struct _NNAME
{
	WORD nObjType;			//  0  0x0019 
	DWORD nLen;				//  2
	char chName[1];			//  6
} NNAME;

typedef struct _NBGLHDR
{
	WORD wStamp;			//  0 0x0201
	WORD nUnk1;				//  2 unknown 0x1992 (maybe FS2004 version number)
	WORD size;				//  4 0x0038
	WORD wResvd1;			//  6 reserved (0x0000)
	BYTE szCrc[6];			//  8 48-bits CRC or what ???
	BYTE szUnk1[2];			// 14 always 0xC301 .............C6 01 in FSX (D6 01 in MSFS)
	__int32 nMagic;				// 16 magic number ? - 13451555 ... 134551555
	DWORD nObjects;			// 20 number of object groups within file
	__int32 nLatMax;			// 24 latitude max ???
	__int32 nLatMin;			// 28 latitude min ???
	__int32 nLonMax;			// 32 longitude max ???
	__int32 nLonMin;			// 36 longitude min ???
	__int32 nUnk2;				// 40 unknown integer 2
	__int32 nUnk3;				// 44 unknown integer 3
	BYTE lpResvd[8];		// 48 reserved
	NSECTS sects[NSECTS_PER_FILE];		// Allow fr max number of sects likely ???
} NBGLHDR;

typedef struct _NOBJ
{	DWORD unk;			// legacy unknown; v9 packed QMID32
	__int32 chunkctr;		// legacy chunk count; v9 itemCount
	DWORD chunkoff;		// absolute payload offset
	DWORD chunksize;	// payload size in bytes
} NOBJ;

/*=========================================================================
    MSFS2024 BGL v9 container (confirmed with BglExplorer v0.9)

    This is the outer BGL container, not an airport record. Offsets in layer
    descriptors and QMID subsection entries are absolute file offsets.
  =========================================================================*/

#define BGL_V9_MAGIC                       0x19920201UL
#define BGL_V9_HEADER_SIZE                 0x00000038UL
#define BGL_V9_LAYER_DESCRIPTOR_SIZE       0x00000014UL
#define BGL_V9_QMID32_ENTRY_SIZE           0x00000010UL
#define BGL_V9_QMID64_ENTRY_SIZE           0x00000014UL
#define BGL_V9_QMID64_FLAG                 0x00010000UL

/* Known v9 layer IDs recovered from BglExplorer's dispatch table. */
#define BGL_LAYER_AIRPORT                  3UL
#define BGL_LAYER_VOR                      19UL
#define BGL_LAYER_NDB                      23UL
#define BGL_LAYER_MARKER                   24UL
#define BGL_LAYER_BOUNDARY                 32UL
#define BGL_LAYER_WAYPOINT                 34UL
#define BGL_LAYER_GEOPOL                   35UL
#define BGL_LAYER_3D_SCENERY               37UL
#define BGL_LAYER_AIRPORT_NAME_OLD         39UL
#define BGL_LAYER_VOR_INDEX_OLD            40UL
#define BGL_LAYER_NDB_INDEX_OLD            41UL
#define BGL_LAYER_WAYPOINT_INDEX_OLD       42UL
#define BGL_LAYER_GUID_SCENERY_MODELS      43UL
#define BGL_LAYER_AIRPORT_SUMMARY          44UL
#define BGL_LAYER_EXCLUSION_RECTANGLES     46UL
#define BGL_LAYER_TIME_ZONES               47UL
#define BGL_LAYER_MODEL_BOX                48UL
#define BGL_LAYER_LANDMARK_LOCATION        49UL
#define BGL_LAYER_VOR_INDEX                50UL
#define BGL_LAYER_NDB_INDEX                51UL
#define BGL_LAYER_WAYPOINT_INDEX           52UL
#define BGL_LAYER_AIRPORT_NAME             53UL
#define BGL_LAYER_TERRAIN_VECTOR_DB        101UL

#pragma pack(push, 1)

typedef struct _BGLV9_FILE_HEADER
{
    DWORD magic;                // +0x00 0x19920201
    DWORD headerSize;           // +0x04 0x38
    FILETIME fileTimeUtc;       // +0x08 passed to FileTimeToSystemTime
    DWORD unknown10;            // +0x10, 0x08051803 in apx57210.bgl
    DWORD layerCount;           // +0x14
    DWORD parentQmid[8];        // +0x18, zero-terminated list
} BGLV9_FILE_HEADER;             // 0x38 bytes

typedef struct _BGLV9_LAYER_DESCRIPTOR
{
    DWORD layerType;            // +0x00 BGL_LAYER_*
    DWORD modeFlags;            // +0x04; bit 0x00010000 selects QMID64 entries
    DWORD subsectionCount;      // +0x08
    DWORD indexOffset;          // +0x0C absolute file offset
    DWORD indexSize;            // +0x10 bytes occupied by subsection index
} BGLV9_LAYER_DESCRIPTOR;        // 0x14 bytes

typedef struct _BGLV9_QMID32_ENTRY
{
    DWORD packedQmid;           // +0x00
    DWORD itemCount;            // +0x04 top-level records in payload
    DWORD payloadOffset;        // +0x08 absolute file offset
    DWORD payloadSize;          // +0x0C payload bytes
} BGLV9_QMID32_ENTRY;            // 0x10 bytes

typedef struct _BGLV9_QMID64_ENTRY
{
    ULONGLONG packedQmid;       // +0x00
    DWORD itemCount;            // +0x08 top-level records in payload
    DWORD payloadOffset;        // +0x0C absolute file offset
    DWORD payloadSize;          // +0x10 payload bytes
} BGLV9_QMID64_ENTRY;            // 0x14 bytes

#pragma pack(pop)


// Objects IDs
#define OBJTYPE_AIRPORT_MSFS 0x0056
#define OBJTYPE_AIRPORT_MSFS2024  0x0113
#define OBJTYPE_AIRPORT_MSFS2024_LEN 0x005C
#define OBJTYPE_AIRPORT		0x0003
#define OBJTYPE_RUNWAY		0x0004
#define OBJTYPE_START		0x0011
#define OBJTYPE_APCOMM		0x0012
#define OBJTYPE_VOR			0x0013
#define OBJTYPE_ILSADD		0x0014
#define OBJTYPE_GS			0x0015
#define OBJTYPE_DME			0x0016
#define OBJTYPE_NDB			0x0017
#define OBJTYPE_MKB			0x0018
#define OBJTYPE_NAME		0x0019
#define OBJTYPE_TAXIPOINT	0x001A
#define OBJTYPE_NEWTAXIPOINT	0x00AC
#define OBJTYPE_TAXIPARK	0x001B
#define OBJTYPE_TAXIPATH	0x001C
#define OBJTYPE_TAXINAME	0x001D
#define OBJTYPE_BOUNDARY	0x0020
#define OBJTYPE_WPT			0x0022
#define OBJTYPE_GEOPOL		0x0023
#define OBJTYPE_APPROACH	0x0024
#define OBJTYPE_SCNOBJ		0x0025
#define OBJTYPE_HELIPAD		0x0026
#define OBJTYPE_REGIONINF	0x0027
#define OBJTYPE_MDLDATA		0x002B
#define OBJTYPE_EXCREC		0x002E
#define OBJTYPE_APRONEDGELT	0x0031
#define OBJTYPE_DELETEAP	0x0033
#define OBJTYPE_APRON		0x0037
#define OBJTYPE_JETWAY		0x003A
#define OBJTYPE_MSFSJETWAY	0x00DE
#define	OBJTYPE_NEWAIRPORT	0x003C
#define	OBJTYPE_NEWNEWAIRPORT	0x00AB
#define	OBJTYPE_NEWTAXIPARK	0x003D
#define	OBJTYPE_NEWNEWTAXIPARK	0x00AD
#define	OBJTYPE_MSFSTAXIPARK	0x00E7
#define	OBJTYPE_NEWRUNWAY	0x003E
#define OBJTYPE_NEWTAXIPATH	0x0040
#define OBJTYPE_NEWNEWTAXIPATH	0x00AE
#define OBJTYPE_MSFSTAXIPATH	0x00D4
#define	OBJTYPE_MSFSRUNWAY	0x00CE
#define OBJTYPE_MSFSRUNWAY_LEN 0x0060
#define MAXSOUTH 536870912
#define MAXEAST 805306368

// airport structure
typedef struct _NAPT
{
	WORD wId;				//  0 record ID (0x3)
	DWORD nLen;				//  2 length of record (32 bits)
	BYTE nRunways;			//	6 No. of runways
	BYTE nComs;				//	7 No. of COMs
	BYTE nStarts;			//	8 No. of starts
	BYTE nApproaches;		//	9 No. of approaches
	BYTE nAprons;			// 10 No. of aprons + 128 for "delete airport"
	BYTE nHelipads;			// 11 No. of helipads
	__int32 nLon;				// 12 longitude, packed format
	__int32 nLat;				// 16 latitude, packed format
	__int32 nAlt;				// 20 altitude * 1000
	__int32 nTowerLon;			// 24 tower longitude, courtesy of Marco Sinchetto
	__int32 nTowerLat;			// 28 tower latitude
	__int32 nTowerAlt;			// 32 tower altitude meters * 1000
	float fMagVar;			// 36 magnetic deviation
	DWORD nId;				// 40 packed ICAO ID (0 = 65)
	__int32 nUnk4;				// 44 unknown integer 4
	DWORD nServices;		// 48 services, if any, otherwise 0x0
	DWORD dwUnknown;		// 52 FSX addition?
} NAPT;

#define PATTERN_NO_PRIM_TAKEOFF		0x01
#define PATTERN_NO_PRIM_LANDING		0x02
#define PATTERN_PRIMARY_RIGHT		0x04
#define PATTERN_NO_SEC_TAKEOFF		0x08
#define PATTERN_NO_SEC_LANDING		0x10
#define PATTERN_SECONDARY_RIGHT		0x20

// runway structure
typedef struct NRWY
{
	WORD wId;				//  0 record ID (0x4)
	DWORD nLen;				//  2 length of record
	BYTE wSurface;			//  6 runway surface
	BYTE bSpare;
	BYTE bStartNumber;		//  8 runway start number, 0-36
	BYTE bStartDesignator;	//  9 runway start designator
	BYTE bEndNumber;		// 10 runway end number, 0-36
	BYTE bEndDesignator;	// 11 runway end designator
	DWORD nPrimaryIlsId;	// 12 primary ILS id in packed format
	DWORD nSecondaryIlsId;	// 16 secondary ILS id in packed format
	__int32 nLon;				// 20 longitude in packed format
	__int32 nLat;				// 24 latitude in packed format
	__int32 nAlt;				// 28 altitude, meters * 1000
	float fLength;			// 32 length in meters
	float fWidth;			// 36 width in meters
	float fHeading;			// 40 approach heading
	float fPatternAlt;		// 44 pattern altitude in meters
	WORD wMarkings;			// 48 marking flags, see above
	BYTE bLights;			// 50 lights flags, see above
	BYTE bPatternFlags;		// 51 pattern flags, see above
} NRWY;

typedef struct MSFSRUNWAY
{
	NRWY rx;
	BYTE unknown[24]; // 52
	BYTE guidSurface[16];
} MSFSRUNWAY;

typedef struct tag_offset_threshold_t
{
	WORD wId;				// 0 record ID
							//			0x5 = primary end threshold
							//			0x6 = secondary end threshold
	DWORD nLen;				// 2 length of record
	WORD wSurface;			// 6 threshold surface
	float fLength;			// 8 threshold length
	float fWidth;			// 12 threshold width
} NOFFTHR;

// runway start structure
typedef struct _NSTART
{
	WORD wId;				//  0 record ID (0x11)
	DWORD nLen;				//  2 record length
	BYTE num;				//  6 was un1 lpUn1;				// see above
	BYTE des;				//  7 un2 lpUn2;				// see above ??????? flags
	__int32 nLon;				//  8 longitude in packed format
	__int32 nLat;				// 12 latitude in packed format
	__int32 nAlt;				// 16 altitude, meters * 1000
	float fHeading;			// 20 heading in degrees
} NSTART;

// airport COM structure
typedef struct _NCOMM
{
	WORD wId;				// 0 record ID (0x12)
	DWORD nLen;				// 2 length of record in bytes
	BYTE bCommType;			// 6 comm type, see above
	BYTE nUnknown;
	__int32 nFreq;				// 8 frequency * 1000000
	// variable length name follows
} NCOMM;

#define COMM_ATIS			0x0001
#define COMM_MULTICOM		0x0002
#define COMM_UNICOM			0x0003
#define COMM_CTAF			0x0004
#define COMM_GROUND			0x0005
#define COMM_TOWER			0x0006
#define COMM_CLEARANCE		0x0007
#define COMM_APPROACH		0x0008
#define COMM_DEPARTURE		0x0009
#define COMM_CENTER			0x000A
#define COMM_FSS			0x000B
#define COMM_AWOS			0x000C
#define COMM_ASOS			0x000D

// ils loc structure
typedef struct _NILSLOC
{	WORD nRec0014;			// 40 ILS LOC rec 0014 extension == 0x0014
	DWORD dwRec0014len;		// 42 length = 16
	BYTE bEnd;				// 46 runway end stored in the high part and:
	//    0x0 is primary, 0x1 is secondary
	BYTE bUnk1;				// 47 unknown byte 1
	float fHeading;			// 48 heading approach
	float fWidth;			// 52 ILS width ???
} NILSLOC;

// ils gs structure
typedef struct _NILSGS
{	WORD nRec0015;			// 56 ILS GS rec 0015 extension == 0x0015
	DWORD dwRec0015len;		// 58 length = 28
	WORD wUnk1;				// 62 unknown word 1
	DWORD dwGSLon;			// 64 GS longitude
	DWORD dwGSLat;			// 68 GS latitude
	DWORD dwGSElev;			// 72 GS elevation
	float fGSrange;			// 76 GS range
	float fGSpitch;			// 80 GS pitch
} NILSGS;

// ils structure
typedef struct _NILS
{	WORD wId;				//  0 record ID (0x13 - same as VOR !!!)
	DWORD nLen;				//  2 length of record in bytes
	BYTE bType;				//  6 type - ILS is 0x4
	BYTE bFlags;			//  7 ils flags
	__int32 nLon;				//  8 longitude in packed format
	__int32 nLat;				// 12 latitude in packed format
	__int32 nAlt;				// 16 altitude, meters * 1000
	__int32 nFreq;				// 20 frequency * 1000000
	float fRange;			// 24 range in meters
	float fMagVar;			// 28 magnetic deviation
	__int32 nId;				// 32 packed ICAO ID - base 0x42
	__int32 nRegion;			// 36 packed region ???
	NILSLOC loc;
} NILS;

typedef struct _NNAM
{	WORD wId;				//  0 record ID (0x19)
	DWORD nLen;				//  2 length of record in bytes
	char chName[1];
} NNAM;

// taxiway parking constants
#define PARKING_NAME_NONE				0x00
#define PARKING_NAME_PARKING			0x01

#define PARKING_NAME_N_PARKING			0x02 // Clockwise to NW = 0x09

#define PARKING_NAME_GATE				0x0A
#define PARKING_NAME_DOCK				0x0B

#define PARKING_NAME_GATE_A				0x0C // to Z = 0x25 (add x35 for character)

#define PARKING_PUSHB_NONE				0x00
#define PARKING_PUSHB_LEFT				0x40
#define PARKING_PUSHB_RIGHT				0x80
#define PARKING_PUSHB_BOTH				0xC0

#define PARKING_TYPE_RAMP_GA			0x01
#define PARKING_TYPE_RAMP_GA_SMALL		0x02
#define PARKING_TYPE_RAMP_GA_MEDIUM		0x03
#define PARKING_TYPE_RAMP_GA_LARGE		0x04
#define PARKING_TYPE_RAMP_CARGO			0x05
#define PARKING_TYPE_RAMP_MIL_CARGO		0x06
#define PARKING_TYPE_RAMP_MIL_COMBAT	0x07
#define PARKING_TYPE_GATE_SMALL			0x08
#define PARKING_TYPE_GATE_MEDIUM		0x09
#define PARKING_TYPE_GATE_HEAVY			0x0A
#define PARKING_TYPE_DOCK_GA			0x0B

// taxiway parking structures
// they are composed by:
//
// - an initial 8 bytes long header
// - an array of parking structures which size is
//   delimited in the header (wCount field)
//   each entry optionally contains airline codes
//   which is an array of strings (4 bytes max. per each string)
//   delimited by one or more null terminator (chr 0x0)
typedef struct _NGATEHDR
{	WORD wId;				// record ID (0x1B)
	DWORD nLen;				// record length with subchunks
	WORD wCount;			// number of parking structures following
} NGATEHDR;

typedef struct _NGATE
{	BYTE bPushBackName;		// pushback and name fields: it is a
							// combination of the above PARKING_NAME_XXX
							// and PARKING_PUSHB_XXX constants
	WORD wNumberType;		// combination of number and type
							// low part of the low byte part stores
							// PARK_TYPE_XXX defined above
							// subtract the constant to the word and
							// obtain the number multiplied by 16
	BYTE bCodeCount;		// number of airline codes stored in this entry
	// ######################  NOTE that I use top bit as "Jetway" flag!
	float fRadius;			// radius of parking (in meters)
	float fHeading;			// heading in degrees
	__int32 nLon;				// longitude in packed format
	__int32 nLat;				// latitude in packed format
} NGATE;

typedef struct _NJETWAY
{	WORD wId;				// record ID (0x3A)
	DWORD nLen;				// record length
	WORD wParkingNumber;	// ref to existing gate (top 12 bits of wNumberType
	WORD wGateName;
	DWORD nSize; // Size of library object record following
} NJETWAY;

typedef struct _NGATE2
{	BYTE bPushBackName;		// pushback and name fields: it is a
							// combination of the above PARKING_NAME_XXX
							// and PARKING_PUSHB_XXX constants
	WORD wNumberType;		// combination of number and type
							// low part of the low byte part stores
							// PARK_TYPE_XXX defined above
							// subtract the constant to the word and
							// obtain the number multiplied by 16
	BYTE bCodeCount;		// number of airline codes stored in this entry
	float fRadius;			// radius of parking (in meters)
	float fHeading;			// heading in degrees
	BYTE bUnknown[16];		// Added in FSX
	__int32 nLon;				// longitude in packed format
	__int32 nLat;				// latitude in packed format
} NGATE2;

typedef struct _NGATE3
{	BYTE bPushBackName;		// pushback and name fields: it is a
							// combination of the above PARKING_NAME_XXX
							// and PARKING_PUSHB_XXX constants
	WORD wNumberType;		// combination of number and type
							// low part of the low byte part stores
							// PARK_TYPE_XXX defined above
							// subtract the constant to the word and
							// obtain the number multiplied by 16
	BYTE bCodeCount;		// number of airline codes stored in this entry
	float fRadius;			// radius of parking (in meters)
	float fHeading;			// heading in degrees
	BYTE bUnknown[16];		// Added in FSX
	__int32 nLon;				// longitude in packed format
	__int32 nLat;				// latitude in packed format
	__int32 nAlt;
} NGATE3;

typedef struct _NGATE4
{
	BYTE bPushBackName;		// pushback and name fields: it is a
							// combination of the above PARKING_NAME_XXX
							// and PARKING_PUSHB_XXX constants
	WORD wNumberType;		// combination of number and type
							// low part of the low byte part stores
							// PARK_TYPE_XXX defined above
							// subtract the constant to the word and
							// obtain the number multiplied by 16
	BYTE bCodeCount;		// number of airline codes stored in this entry
	float fRadius;			// radius of parking (in meters)
	float fHeading;			// heading in degrees
	BYTE bUnknown[16];		// Added in FSX
	__int32 nLon;			// longitude in packed format
	__int32 nLat;			// latitude in packed format
	BYTE bUnknown2;			// ??
	BYTE bSuffix;			// Gate suffix letter - 0x00 = none, 0x0C = A, 0x0D = B, etc.
	BYTE bUnknown3[18];		// ?? 
} NGATE4;

#define BIT_DELETE_ALL_APPROACHES		0x01
#define BIT_DELETE_ALL_APRONS			0x04
#define BIT_DELETE_ALL_FREQUENCIES		0x08
#define BIT_DELETE_ALL_HELIPADS			0x10
#define BIT_DELETE_ALL_RUNWAYS			0x20
#define BIT_DELETE_ALL_STARTS			0x40
#define BIT_DELETE_ALL_TAXIWAYS			0x80

// taxiway path structure (0x1C)
//
// composed by:
// - an initial 8 bytes header
// - an attached array of taxiway_path_t structures
//   whose size is defined by header's wCount field
typedef struct _NTAXIHDR
{	WORD wId;				// 0000 - record ID (0x1C)
	DWORD nLen;				// 0002 - record length
	WORD wCount;			// 0006 - count of paths
} NTAXIHDR;

typedef struct _NTAXI
{	WORD wStart;			// 0000 - start index
	WORD wEnd;				// 0002 - end index
							//        high part of the high byte also
							//        stores runway designator
	BYTE bDrawFlags;		// 0004 - combination of draw flags and path types,
							//        see constant above
	BYTE bNumber;			// 0005 - runway number
	BYTE bCenterFlags;		// 0006 - centerline flags, see constants above
	BYTE bSurface;			// 0007 - runway surface
	float fWidth;			// 0008 - width in meters
	__int32 nWeightLimit;		// 0012 - weigth limit
	__int32 nUnk;				// 0016 - unknown integer - terminator
} NTAXI;

typedef struct _NEWNTAXI
{	WORD wStart;			// 0000 - start index
	WORD wEnd;				// 0002 - end index
							//        high part of the high byte also
							//        stores runway designator
	BYTE bDrawFlags;		// 0004 - combination of draw flags and path types,
							//        see constant above
	BYTE bNumber;			// 0005 - runway number
	BYTE bCenterFlags;		// 0006 - centerline flags, see constants above
	BYTE bSurface;			// 0007 - runway surface
	float fWidth;			// 0008 - width in meters
	__int32 nWeightLimit;		// 0012 - weigth limit
	__int32 nUnk;				// 0016 - unknown integer - terminator
	BYTE bUnk[16];			// 0020 - 16 extra bytes unknown use
} NEWNTAXI;

typedef struct _NEWNTAXI2
{	WORD wStart;			// 0000 - start index
	WORD wEnd;				// 0002 - end index
							//        high part of the high byte also
							//        stores runway designator
	BYTE bDrawFlags;		// 0004 - combination of draw flags and path types,
							//        see constant above
	BYTE bNumber;			// 0005 - runway number
	BYTE bCenterFlags;		// 0006 - centerline flags, see constants above
	BYTE bSurface;			// 0007 - runway surface
	float fWidth;			// 0008 - width in meters
	__int32 nWeightLimit;		// 0012 - weigth limit
	__int32 nUnk;				// 0016 - unknown integer - terminator
	BYTE bUnk[16];			// 0020 - 16 extra bytes unknown use
	__int32 nflatten;
	// ACCORDING TO Jon: __int32 nsurfaceQuery;
} NEWNTAXI2;

typedef struct _MSFSNTAXI
{	WORD wStart;			// 0000 - start index
	WORD wOldEnd;			// 0002 - not now end index
							//        high part of the high byte also
							//        stores runway designator in bits12-15
	BYTE bDrawFlags;		// 0004 - combination of draw flags and path types,
							//        see constant above
	BYTE bNumber;			// 0005 - runway number
	BYTE bCenterFlags;		// 0006 - centerline flags, see constants above
	BYTE bOldSurface;		// 0007 - was runway surface, now GUID below
	float fWidth;			// 0008 - width in meters
	__int32 nWeightLimit;		// 0012 - weigth limit
	BYTE bUnk1[8];			// 0016 - extra bytes unknown use
	BYTE guidSurface[16];	// 0024 - GUID for surface material
	BYTE bUnk2[6];			// 0040 - unknown
	WORD wEnd;				// 0046 - end index
} MSFSNTAXI;

// taxiway point entry structure
#define TWP_NORMAL			0x00000001
#define TWP_HOLD_SHORT		0x00000002

typedef struct _NTAXIPT
{	BYTE bType;				// 0000 - type, normal or hold_short
	BYTE bOrientation;		// 0001 - 0: forward, 1: reverse
	WORD wUnk1;				// 0002 - unknown word 1
	union
	{	__int32 nLon;			// 0004 - longitude in packed format
		float fLon;
	};
	union
	{	__int32 nLat;				// 0008 - latitude in packed format
		float fLat;
	};
} NTAXIPT;

typedef struct _NEWTAXIPT
{	BYTE bType;				// 0000 - type, normal or hold_short
	BYTE bOrientation;		// 0001 - 0: forward, 1: reverse
	WORD wUnk1;				// 0002 - unknown word 1
	union
	{	__int32 nLon;			// 0004 - longitude in packed format
		float fLon;
	};
	union
	{	__int32 nLat;				// 0008 - latitude in packed format
		float fLat;
	};
	__int32 nAltitude;
} NEWTAXIPT;

typedef struct _NTAXINM
{	char szName[8];
} NTAXINM;

// region structure
typedef struct _NREGION
{
	WORD wId;				// 0000 - record ID (0x27)
	WORD wUnk1;				// 0002 - unknown word 1
	WORD wUnk2;				// 0004 - unknown word 1
	WORD wRegionCount;		// 0006 - count of regions
	WORD wCountryCount;		// 0008 - count of countries
	WORD wStateCount;		// 0010 - count of states
	WORD wCityCount;		// 0012 - count of cities
	WORD wAirportCount;		// 0014 - count of airports
	WORD wIcaoCount;		// 0016 - count of ICAOs
	__int32 nRegionPtr;			// 0018 - relative offset to region name
	__int32 nCountryPtr;		// 0022 - relative offset to country name
	__int32 nStatePtr;			// 0026 - relative offset to state name
	__int32 nCityPtr;			// 0030 - relative offset to city name
	__int32 nAirportPtr;		// 0034 - relative offset to airport names
	__int32 nIcaoPtr;			// 0038 - relative offset to ICAO IDs
} NREGION;

// icao id entry (20 bytes)
typedef struct _NICAO
{
	BYTE bRegionIndex;		// 00 - subscript index into regions array, stored in the low part
	BYTE bCountryIndex;		// 01 - subscript index into countries array, stored in the low part
	WORD wStateIndex;		// 02 - subscript index into states array, stored in the high part
	WORD wCitiesIndex;		// 04 - subscript index into city names array
	WORD wAirportIndex;		// 06 - subscript index into airport names array
	DWORD nId;				// 08 - packed ICAO ID
	BYTE lpUnk[8];			// 12 - unknown array
} NICAO;

#define HELITYPE_NONE		0x00
#define HELITYPE_H			0x01
#define HELITYPE_SQUARE		0x02
#define HELITYPE_CIRCLE		0x03
#define HELITYPE_MEDICAL	0x04

#define HELI_TRANSPARENT	0x10
#define HELI_CLOSED			0x20

// helipad structure
// NOTE: BGLCOMP fails when red, green, blue tokens are
// included !!!
typedef struct tag_helipad_t
{
	WORD wId;				// record ID (0x26)
	DWORD nLen;				// record length
	BYTE bSurface;			// helipad surface (same as runway)
	BYTE bFlags;			// helipad flags, see constants above
	BYTE r;					// rgb
	BYTE g;					// rgb
	BYTE b;					// rgb
	BYTE a;					// alpha
	__int32 nLon;				// longitude in packed format
	__int32 nLat;				// latitude in packed format
	__int32 nAlt;				// altitude, meters * 1000
	float fLength;			// helipad length in meters
	float fWidth;			// helipad width in meters
	float fHeading;			// helipad heading
} helipad_t;

// vasi structure
typedef struct tag_vasi_t
{
	WORD wId;				// record ID, where:
	// (0xB = primary VASI on left side)
	// (0xC = primary VASI on right side)
	// (0xD = secondary VASI on left side)
	// (0xE = secondary VASI on right side)
	DWORD nLen;				// record length
	WORD wType;				// vasi type, see constants above
	float fBiasX;			// vasi bias along x side, in meters
	float fBiasZ;			// vasi bias along z side, in meters
	float fSpacing;			// vasi spacing, in meters
	float fPitch;			// vasi pitch in degrees
} vasi_t;

typedef struct tag_approachlights_t
{
	WORD wId;				// record ID (0x26)
	DWORD nLen;				// record length
	BYTE bFlags;			// approach flags, see above
	BYTE bStrobes;			// number of sequenced strobes
} approachlights_t;

typedef struct _NAPRON1HDR // simple apron type
{	WORD wId;				// 0000 - record ID (0x37)
	DWORD nLen;				// 0002 - record length
	BYTE bSurf;				// 0006 - surface type
	WORD wCount;			// 0007 - count of vertices
	DWORD dwLatLons[0];		// Pairs of Lat Lons for vetices
} NAPRON1HDR;

typedef struct _NAPRON2HDR // triangles apron type
{	WORD wId;				// 0000 - record ID (0x30)
	DWORD nLen;				// 0002 - record length
	BYTE bSurf;				// 0006 - surface type
	BYTE bFlags;			// 0007
	WORD wVCount;			// 0008 - count of vertices
	WORD wTCount;			// 000A - count of triangles
	DWORD dwLatLons[0];		// Pairs of Lat Lons for vertices
} NAPRON2HDR;

extern DWORD ulTotalBGLs, ulTotalBytes;


/*==========================================================================
    Section 2 - Non-duplicate constants imported from Full New AFD.h

    Existing macro names from NewAFD.h are omitted. Each imported macro is
    protected with #ifndef so this file can remain safe as a reference header.
  ==========================================================================*/

#ifndef NDB_MAXSOUTH
#define NDB_MAXSOUTH MAXSOUTH
#endif
#ifndef NDB_MAXEAST
#define NDB_MAXEAST MAXEAST
#endif
#ifndef VOR_MAXSOUTH
#define VOR_MAXSOUTH NDB_MAXSOUTH
#endif
#ifndef VOR_MAXEAST
#define VOR_MAXEAST NDB_MAXEAST
#endif
#ifndef NDB_COMPASS_POINT
#define NDB_COMPASS_POINT	0x0000
#endif
#ifndef NDB_MH
#define NDB_MH				0x0001
#endif
#ifndef NDB_H
#define NDB_H				0x0002
#endif
#ifndef NDB_HH
#define NDB_HH				0x0003
#endif
#ifndef VOR_TERMINAL
#define VOR_TERMINAL		0x01
#endif
#ifndef VOR_LOW
#define VOR_LOW				0x02
#endif
#ifndef VOR_HIGH
#define VOR_HIGH			0x03
#endif
#ifndef VOR_ILS
#define VOR_ILS				0x04
#endif
#ifndef VOR_VOT
#define VOR_VOT				0x05
#endif
#ifndef MKB_INNER
#define MKB_INNER			0x00
#endif
#ifndef MKB_MIDDLE
#define MKB_MIDDLE			0x01
#endif
#ifndef MKB_OUTER
#define MKB_OUTER			0x02
#endif
#ifndef MKB_BACKCOURSE
#define MKB_BACKCOURSE		0x03
#endif
#ifndef WPT_NAMED
#define WPT_NAMED			0x01
#endif
#ifndef WPT_UNNAMED
#define WPT_UNNAMED			0x02
#endif
#ifndef WPT_VOR
#define WPT_VOR				0x03
#endif
#ifndef WPT_NDB
#define WPT_NDB				0x04
#endif
#ifndef WPT_OFFROUTE
#define WPT_OFFROUTE		0x05
#endif
#ifndef WPT_IAF
#define WPT_IAF				0x06
#endif
#ifndef WPT_FAF
#define WPT_FAF				0x07
#endif
#ifndef RTE_VICTOR
#define RTE_VICTOR			0x01
#endif
#ifndef RTE_JET
#define RTE_JET				0x02
#endif
#ifndef RTE_BOTH
#define RTE_BOTH			0x03
#endif
#ifndef BOUNDARY_NONE
#define BOUNDARY_NONE			0x00
#endif
#ifndef BOUNDARY_CENTER
#define BOUNDARY_CENTER			0x01
#endif
#ifndef BOUNDARY_CLASS_A
#define BOUNDARY_CLASS_A		0x02
#endif
#ifndef BOUNDARY_CLASS_B
#define BOUNDARY_CLASS_B		0x03
#endif
#ifndef BOUNDARY_CLASS_C
#define BOUNDARY_CLASS_C		0x04
#endif
#ifndef BOUNDARY_CLASS_D
#define BOUNDARY_CLASS_D		0x05
#endif
#ifndef BOUNDARY_CLASS_E
#define BOUNDARY_CLASS_E		0x06
#endif
#ifndef BOUNDARY_CLASS_F
#define BOUNDARY_CLASS_F		0x07
#endif
#ifndef BOUNDARY_CLASS_G
#define BOUNDARY_CLASS_G		0x08
#endif
#ifndef BOUNDARY_TOWER
#define BOUNDARY_TOWER			0x09
#endif
#ifndef BOUNDARY_CLEARANCE
#define BOUNDARY_CLEARANCE		0x0A
#endif
#ifndef BOUNDARY_GROUND
#define BOUNDARY_GROUND			0x0B
#endif
#ifndef BOUNDARY_DEPARTURE
#define BOUNDARY_DEPARTURE		0x0C
#endif
#ifndef BOUNDARY_APPROACH
#define BOUNDARY_APPROACH		0x0D
#endif
#ifndef BOUNDARY_MOA
#define BOUNDARY_MOA			0x0E
#endif
#ifndef BOUNDARY_RESTRICTED
#define BOUNDARY_RESTRICTED		0x0F
#endif
#ifndef BOUNDARY_PROHIBITED
#define BOUNDARY_PROHIBITED		0x10
#endif
#ifndef BOUNDARY_WARNING
#define BOUNDARY_WARNING		0x11
#endif
#ifndef BOUNDARY_ALERT
#define BOUNDARY_ALERT			0x12
#endif
#ifndef BOUNDARY_DANGER
#define BOUNDARY_DANGER			0x13
#endif
#ifndef BOUNDARY_NATIONAL_PARK
#define BOUNDARY_NATIONAL_PARK	0x14
#endif
#ifndef BOUNDARY_MODEC
#define BOUNDARY_MODEC			0x15
#endif
#ifndef BOUNDARY_RADAR
#define BOUNDARY_RADAR			0x16
#endif
#ifndef BOUNDARY_TRAINING
#define BOUNDARY_TRAINING		0x17
#endif
#ifndef BOUNDARY_ALT_UNKNOWN
#define BOUNDARY_ALT_UNKNOWN	0x01
#endif
#ifndef BOUNDARY_ALT_MSL
#define BOUNDARY_ALT_MSL		0x01
#endif
#ifndef BOUNDARY_ALT_AGL
#define BOUNDARY_ALT_AGL		0x02
#endif
#ifndef BOUNDARY_ALT_UNLIMITED
#define BOUNDARY_ALT_UNLIMITED	0x03
#endif
#ifndef COMPLEXITY_VERY_SPARSE
#define COMPLEXITY_VERY_SPARSE			0x00000000
#endif
#ifndef COMPLEXITY_SPARSE
#define COMPLEXITY_SPARSE				0x00000001
#endif
#ifndef COMPLEXITY_NORMAL
#define COMPLEXITY_NORMAL				0x00000002
#endif
#ifndef COMPLEXITY_DENSE
#define COMPLEXITY_DENSE				0x00000003
#endif
#ifndef COMPLEXITY_VERY_DENSE
#define COMPLEXITY_VERY_DENSE			0x00000004
#endif
#ifndef COMPLEXITY_EXTREMELY_DENSE
#define COMPLEXITY_EXTREMELY_DENSE		0x00000005
#endif
#ifndef SCNOBJ_BUILDING
#define SCNOBJ_BUILDING					0x0001
#endif
#ifndef SCNOBJ_LIBRARY_OBJ
#define SCNOBJ_LIBRARY_OBJ				0x0002
#endif
#ifndef SCNOBJ_WINDSOCK
#define SCNOBJ_WINDSOCK					0x0003
#endif
#ifndef SCNOBJ_EFFECT
#define SCNOBJ_EFFECT					0x0004
#endif
#ifndef SCNOBJ_TRIGGER
#define SCNOBJ_TRIGGER					0x0007
#endif
#ifndef TRIGGER_REFUEL_REPAIR
#define TRIGGER_REFUEL_REPAIR				0x0000
#endif
#ifndef TRIGGER_WEATHER
#define TRIGGER_WEATHER						0x0001
#endif
#ifndef TWEATHER_RIDGE_LIFT
#define TWEATHER_RIDGE_LIFT					0x0001
#endif
#ifndef TWEATHER_NONDIRECTIONAL_TURBULENCE
#define TWEATHER_NONDIRECTIONAL_TURBULENCE	0x0002
#endif
#ifndef TWEATHER_DIRECTIONAL_TURBULENCE
#define TWEATHER_DIRECTIONAL_TURBULENCE		0x0003
#endif
#ifndef TWEATHER_THERMAL
#define TWEATHER_THERMAL					0x0004
#endif
#ifndef BLDG_FLAT_ROOF
#define BLDG_FLAT_ROOF		0x0004
#endif
#ifndef BLDG_RIDGE_ROOF
#define BLDG_RIDGE_ROOF		0x0006
#endif
#ifndef BLDG_PEAKED_ROOF
#define BLDG_PEAKED_ROOF	0x0007
#endif
#ifndef BLDG_SLANT_ROOF
#define BLDG_SLANT_ROOF		0x0008
#endif
#ifndef BLDG_PYRAMIDAL
#define BLDG_PYRAMIDAL		0x0009
#endif
#ifndef BLDG_MULTISIDED
#define BLDG_MULTISIDED		0x000A
#endif
#ifndef BIT_EXCLUDE_ALL
#define BIT_EXCLUDE_ALL			0x08
#endif
#ifndef BIT_EXCLUDE_BEACON
#define BIT_EXCLUDE_BEACON		0x10
#endif
#ifndef BIT_EXCLUDE_EFFECT
#define BIT_EXCLUDE_EFFECT		0x20
#endif
#ifndef BIT_EXCLUDE_BUILDING
#define BIT_EXCLUDE_BUILDING	0x40
#endif
#ifndef BIT_EXCLUDE_LIBOBJ
#define BIT_EXCLUDE_LIBOBJ		0x80
#endif
#ifndef BIT_EXCLUDE_TAXIWAY
#define BIT_EXCLUDE_TAXIWAY		0x01
#endif
#ifndef BIT_EXCLUDE_TRIGGER
#define BIT_EXCLUDE_TRIGGER		0x02
#endif
#ifndef BIT_EXCLUDE_WINDSOCK
#define BIT_EXCLUDE_WINDSOCK	0x04
#endif
#ifndef APTENTRY_COMM
#define APTENTRY_COMM		0x0012
#endif
#ifndef APTENTRY_RUNWAY
#define APTENTRY_RUNWAY		0x0004
#endif
#ifndef APTENTRY_HELIPAD
#define APTENTRY_HELIPAD	0x0026
#endif
#ifndef APTENTRY_DELAPT
#define	APTENTRY_DELAPT		0x0033
#endif
#ifndef SURFACE_CEMENT
#define SURFACE_CEMENT				0x0000
#endif
#ifndef SURFACE_CONCRETE
#define SURFACE_CONCRETE			0x0000
#endif
#ifndef SURFACE_GRASS
#define SURFACE_GRASS				0x0001
#endif
#ifndef SURFACE_WATER
#define SURFACE_WATER				0x0002
#endif
#ifndef SURFACE_ASPHALT
#define SURFACE_ASPHALT				0x0004
#endif
#ifndef SURFACE_CLAY
#define SURFACE_CLAY				0x0007
#endif
#ifndef SURFACE_SNOW
#define SURFACE_SNOW				0x0008
#endif
#ifndef SURFACE_ICE
#define SURFACE_ICE					0x0009
#endif
#ifndef SURFACE_DIRT
#define SURFACE_DIRT				0x000C
#endif
#ifndef SURFACE_CORAL
#define SURFACE_CORAL				0x000D
#endif
#ifndef SURFACE_GRAVEL
#define SURFACE_GRAVEL				0x000E
#endif
#ifndef SURFACE_OIL_TREATED
#define SURFACE_OIL_TREATED			0x000F
#endif
#ifndef SURFACE_STEEL_MATS
#define SURFACE_STEEL_MATS			0x0010
#endif
#ifndef SURFACE_BITUMINOUS
#define SURFACE_BITUMINOUS			0x0011
#endif
#ifndef SURFACE_BRICK
#define SURFACE_BRICK				0x0012
#endif
#ifndef SURFACE_MACADAM
#define SURFACE_MACADAM				0x0013
#endif
#ifndef SURFACE_PLANKS
#define SURFACE_PLANKS				0x0014
#endif
#ifndef SURFACE_SAND
#define SURFACE_SAND				0x0015
#endif
#ifndef SURFACE_SHALE
#define SURFACE_SHALE				0x0016
#endif
#ifndef SURFACE_TARMAC
#define SURFACE_TARMAC				0x0017
#endif
#ifndef SURFACE_UNKNOWN
#define SURFACE_UNKNOWN				0x00FE
#endif
#ifndef DESIGNATOR_NONE
#define DESIGNATOR_NONE				0x00
#endif
#ifndef DESIGNATOR_LEFT
#define DESIGNATOR_LEFT				0x01
#endif
#ifndef DESIGNATOR_RIGHT
#define DESIGNATOR_RIGHT			0x02
#endif
#ifndef DESIGNATOR_CENTER
#define DESIGNATOR_CENTER			0x03
#endif
#ifndef DESIGNATOR_WATER
#define DESIGNATOR_WATER			0x04
#endif
#ifndef MARKING_EDGE
#define MARKING_EDGE				0x0001
#endif
#ifndef MARKING_THRESHOLD
#define MARKING_THRESHOLD			0x0002
#endif
#ifndef MARKING_FIXED
#define MARKING_FIXED				0x0004
#endif
#ifndef MARKING_TOUCHDOWN
#define MARKING_TOUCHDOWN			0x0008
#endif
#ifndef MARKING_DASHES
#define MARKING_DASHES				0x0010
#endif
#ifndef MARKING_IDENT
#define MARKING_IDENT				0x0020
#endif
#ifndef MARKING_PRECISION
#define MARKING_PRECISION			0x0040
#endif
#ifndef MARKING_EDGE_PAVEMENT
#define MARKING_EDGE_PAVEMENT		0x0080
#endif
#ifndef MARKING_SINGLE_END
#define MARKING_SINGLE_END			0x0100
#endif
#ifndef MARKING_PRIMARY_CLOSED
#define MARKING_PRIMARY_CLOSED		0x0200
#endif
#ifndef MARKING_SECONDARY_CLOSED
#define MARKING_SECONDARY_CLOSED	0x0400
#endif
#ifndef MARKING_PRIMARY_STOL
#define MARKING_PRIMARY_STOL		0x0800
#endif
#ifndef MARKING_SECONDARY_STOL
#define MARKING_SECONDARY_STOL		0x1000
#endif
#ifndef LIGHTS_EDGE_LOW
#define LIGHTS_EDGE_LOW				0x01
#endif
#ifndef LIGHTS_EDGE_MEDIUM
#define LIGHTS_EDGE_MEDIUM			0x02
#endif
#ifndef LIGHTS_EDGE_HIGH
#define LIGHTS_EDGE_HIGH			0x03
#endif
#ifndef LIGHTS_CENTER_LOW
#define LIGHTS_CENTER_LOW			0x04
#endif
#ifndef LIGHTS_CENTER_MEDIUM
#define LIGHTS_CENTER_MEDIUM		0x08
#endif
#ifndef LIGHTS_CENTER_HIGH
#define LIGHTS_CENTER_HIGH			0x0C
#endif
#ifndef LIGHTS_CENTERED
#define LIGHTS_CENTERED				0x10
#endif
#ifndef ILS_PRIMARY_END
#define ILS_PRIMARY_END		0x01	// disregard this
#endif
#ifndef ILS_BACKCOURSE
#define ILS_BACKCOURSE		0x04
#endif
#ifndef ILS_DME
#define ILS_DME				0x10
#endif
#ifndef ILS_GS
#define ILS_GS				0x0C
#endif
#ifndef RWYSTART_RUNWAY
#define RWYSTART_RUNWAY		0x01
#endif
#ifndef RWYSTART_WATER
#define RWYSTART_WATER		0x02
#endif
#ifndef RWYSTART_HELIPAD
#define RWYSTART_HELIPAD	0x03
#endif
#ifndef AP_SYSTEM_NONE
#define AP_SYSTEM_NONE		0x00
#endif
#ifndef AP_SYSTEM_ODALS
#define AP_SYSTEM_ODALS		0x01
#endif
#ifndef AP_SYSTEM_MALSF
#define AP_SYSTEM_MALSF		0x02
#endif
#ifndef AP_SYSTEM_MALSR
#define AP_SYSTEM_MALSR		0x03
#endif
#ifndef AP_SYSTEM_SSALF
#define AP_SYSTEM_SSALF		0x04
#endif
#ifndef AP_SYSTEM_SSALR
#define AP_SYSTEM_SSALR		0x05
#endif
#ifndef AP_SYSTEM_ALSF1
#define AP_SYSTEM_ALSF1		0x06
#endif
#ifndef AP_SYSTEM_ALSF2
#define AP_SYSTEM_ALSF2		0x07
#endif
#ifndef AP_SYSTEM_RAIL
#define AP_SYSTEM_RAIL		0x08
#endif
#ifndef AP_SYSTEM_CALVERT
#define AP_SYSTEM_CALVERT	0x09
#endif
#ifndef AP_SYSTEM_CALVERT2
#define AP_SYSTEM_CALVERT2	0x0A
#endif
#ifndef AP_SYSTEM_SALS
#define AP_SYSTEM_SALS		0x0C
#endif
#ifndef AP_ENDLIGHTS
#define	AP_ENDLIGHTS		0x20
#endif
#ifndef AP_REIL
#define	AP_REIL				0x40
#endif
#ifndef AP_TOUCHDOWN
#define	AP_TOUCHDOWN		0x80
#endif
#ifndef VASI_VASI21
#define VASI_VASI21			0x0001
#endif
#ifndef VASI_VASI31
#define VASI_VASI31			0x0002
#endif
#ifndef VASI_VASI22
#define VASI_VASI22			0x0003
#endif
#ifndef VASI_VASI32
#define VASI_VASI32			0x0004
#endif
#ifndef VASI_VASI23
#define VASI_VASI23			0x0005
#endif
#ifndef VASI_VASI33
#define VASI_VASI33			0x0006
#endif
#ifndef VASI_PAPI2
#define VASI_PAPI2			0x0007
#endif
#ifndef VASI_PAPI4
#define VASI_PAPI4			0x0008
#endif
#ifndef VASI_TRICOLOR
#define VASI_TRICOLOR		0x0009
#endif
#ifndef VASI_PVASI
#define VASI_PVASI			0x000A
#endif
#ifndef VASI_TVASI
#define VASI_TVASI			0x000B
#endif
#ifndef VASI_BALL
#define VASI_BALL			0x000C
#endif
#ifndef VASI_APAP
#define VASI_APAP			0x000D
#endif
#ifndef VASI_PANELS
#define VASI_PANELS			0x000D
#endif
#ifndef PARKING_NAME_NE_PARKING
#define PARKING_NAME_NE_PARKING			0x03
#endif
#ifndef PARKING_NAME_E_PARKING
#define PARKING_NAME_E_PARKING			0x04
#endif
#ifndef PARKING_NAME_SE_PARKING
#define PARKING_NAME_SE_PARKING			0x05
#endif
#ifndef PARKING_NAME_S_PARKING
#define PARKING_NAME_S_PARKING			0x06
#endif
#ifndef PARKING_NAME_SW_PARKING
#define PARKING_NAME_SW_PARKING			0x07
#endif
#ifndef PARKING_NAME_W_PARKING
#define PARKING_NAME_W_PARKING			0x07
#endif
#ifndef PARKING_NAME_NW_PARKING
#define PARKING_NAME_NW_PARKING			0x09
#endif
#ifndef PARKING_NAME_GATE_B
#define PARKING_NAME_GATE_B				0x0D
#endif
#ifndef PARKING_NAME_GATE_C
#define PARKING_NAME_GATE_C				0x0E
#endif
#ifndef PARKING_NAME_GATE_D
#define PARKING_NAME_GATE_D				0x0F
#endif
#ifndef PARKING_NAME_GATE_E
#define PARKING_NAME_GATE_E				0x10
#endif
#ifndef PARKING_NAME_GATE_F
#define PARKING_NAME_GATE_F				0x11
#endif
#ifndef PARKING_NAME_GATE_G
#define PARKING_NAME_GATE_G				0x12
#endif
#ifndef PARKING_NAME_GATE_H
#define PARKING_NAME_GATE_H				0x13
#endif
#ifndef PARKING_NAME_GATE_I
#define PARKING_NAME_GATE_I				0x13
#endif
#ifndef PARKING_NAME_GATE_J
#define PARKING_NAME_GATE_J				0x15
#endif
#ifndef PARKING_NAME_GATE_K
#define PARKING_NAME_GATE_K				0x16
#endif
#ifndef PARKING_NAME_GATE_L
#define PARKING_NAME_GATE_L				0x17
#endif
#ifndef PARKING_NAME_GATE_M
#define PARKING_NAME_GATE_M				0x18
#endif
#ifndef PARKING_NAME_GATE_N
#define PARKING_NAME_GATE_N				0x19
#endif
#ifndef PARKING_NAME_GATE_O
#define PARKING_NAME_GATE_O				0x1A
#endif
#ifndef PARKING_NAME_GATE_P
#define PARKING_NAME_GATE_P				0x1B
#endif
#ifndef PARKING_NAME_GATE_Q
#define PARKING_NAME_GATE_Q				0x1C
#endif
#ifndef PARKING_NAME_GATE_R
#define PARKING_NAME_GATE_R				0x1D
#endif
#ifndef PARKING_NAME_GATE_S
#define PARKING_NAME_GATE_S				0x1E
#endif
#ifndef PARKING_NAME_GATE_T
#define PARKING_NAME_GATE_T				0x1F
#endif
#ifndef PARKING_NAME_GATE_U
#define PARKING_NAME_GATE_U				0x20
#endif
#ifndef PARKING_NAME_GATE_V
#define PARKING_NAME_GATE_V				0x21
#endif
#ifndef PARKING_NAME_GATE_W
#define PARKING_NAME_GATE_W				0x22
#endif
#ifndef PARKING_NAME_GATE_X
#define PARKING_NAME_GATE_X				0x23
#endif
#ifndef PARKING_NAME_GATE_Y
#define PARKING_NAME_GATE_Y				0x24
#endif
#ifndef PARKING_NAME_GATE_Z
#define PARKING_NAME_GATE_Z				0x25
#endif
#ifndef APRON_SURFACE_DRAW
#define APRON_SURFACE_DRAW	0x01
#endif
#ifndef APRON_DETAILS_DRAW
#define APRON_DETAILS_DRAW	0x02
#endif
#ifndef APPROACH_GPS_OVL
#define APPROACH_GPS_OVL					0x08
#endif
#ifndef APPROACH_TYPE_GPS
#define APPROACH_TYPE_GPS					0x01
#endif
#ifndef APPROACH_TYPE_VOR
#define APPROACH_TYPE_VOR					0x02
#endif
#ifndef APPROACH_TYPE_NDB
#define APPROACH_TYPE_NDB					0x03
#endif
#ifndef APPROACH_TYPE_ILS
#define APPROACH_TYPE_ILS					0x04
#endif
#ifndef APPROACH_TYPE_LOCALIZER
#define APPROACH_TYPE_LOCALIZER				0x05
#endif
#ifndef APPROACH_TYPE_SDF
#define APPROACH_TYPE_SDF					0x06
#endif
#ifndef APPROACH_TYPE_LDA
#define APPROACH_TYPE_LDA					0x07
#endif
#ifndef APPROACH_TYPE_VORDME
#define APPROACH_TYPE_VORDME				0x08
#endif
#ifndef APPROACH_TYPE_NDBDME
#define APPROACH_TYPE_NDBDME				0x09
#endif
#ifndef APPROACH_TYPE_RNAV
#define APPROACH_TYPE_RNAV					0x0A
#endif
#ifndef APPROACH_TYPE_LOCALIZER_BACKCOURSE
#define APPROACH_TYPE_LOCALIZER_BACKCOURSE	0x0B
#endif
#ifndef TRANSITION_TYPE_FULL
#define TRANSITION_TYPE_FULL	0x01
#endif
#ifndef TRANSITION_TYPE_DME
#define TRANSITION_TYPE_DME		0x02
#endif
#ifndef FIX_TYPE_VOR
#define	FIX_TYPE_VOR					0x02
#endif
#ifndef FIX_TYPE_NDB
#define	FIX_TYPE_NDB					0x03
#endif
#ifndef FIX_TYPE_TERMINAL_NDB
#define	FIX_TYPE_TERMINAL_NDB			0x04
#endif
#ifndef FIX_TYPE_WAYPOINT
#define	FIX_TYPE_WAYPOINT				0x05
#endif
#ifndef FIX_TYPE_TERMINAL_WAYPOINT
#define	FIX_TYPE_TERMINAL_WAYPOINT		0x06
#endif
#ifndef FIX_TYPE_LOCALIZER
#define	FIX_TYPE_LOCALIZER				0x08
#endif
#ifndef FIX_TYPE_RUNWAY
#define	FIX_TYPE_RUNWAY					0x09
#endif
#ifndef LEG_TYPE_AF
#define LEG_TYPE_AF						0x01
#endif
#ifndef LEG_TYPE_CA
#define LEG_TYPE_CA						0x02
#endif
#ifndef LEG_TYPE_CD
#define LEG_TYPE_CD						0x03
#endif
#ifndef LEG_TYPE_CF
#define LEG_TYPE_CF						0x04
#endif
#ifndef LEG_TYPE_CI
#define LEG_TYPE_CI						0x05
#endif
#ifndef LEG_TYPE_CR
#define LEG_TYPE_CR						0x06
#endif
#ifndef LEG_TYPE_DF
#define LEG_TYPE_DF						0x07
#endif
#ifndef LEG_TYPE_FA
#define LEG_TYPE_FA						0x08
#endif
#ifndef LEG_TYPE_FC
#define LEG_TYPE_FC						0x09
#endif
#ifndef LEG_TYPE_FD
#define LEG_TYPE_FD						0x0A
#endif
#ifndef LEG_TYPE_FM
#define LEG_TYPE_FM						0x0B
#endif
#ifndef LEG_TYPE_HA
#define LEG_TYPE_HA						0x0C
#endif
#ifndef LEG_TYPE_HF
#define LEG_TYPE_HF						0x0D
#endif
#ifndef LEG_TYPE_HM
#define LEG_TYPE_HM						0x0E
#endif
#ifndef LEG_TYPE_IF
#define LEG_TYPE_IF						0x0F
#endif
#ifndef LEG_TYPE_PI
#define LEG_TYPE_PI						0x10
#endif
#ifndef LEG_TYPE_RF
#define LEG_TYPE_RF						0x11
#endif
#ifndef LEG_TYPE_TF
#define LEG_TYPE_TF						0x12
#endif
#ifndef LEG_TYPE_VA
#define LEG_TYPE_VA						0x13
#endif
#ifndef LEG_TYPE_VD
#define LEG_TYPE_VD						0x14
#endif
#ifndef LEG_TYPE_VI
#define LEG_TYPE_VI						0x15
#endif
#ifndef LEG_TYPE_VM
#define LEG_TYPE_VM						0x16
#endif
#ifndef LEG_TYPE_VR
#define LEG_TYPE_VR						0x17
#endif
#ifndef LEG_TRUE_COURSE_VALID
#define LEG_TRUE_COURSE_VALID			0x01
#endif
#ifndef LEG_TIME_VALID
#define LEG_TIME_VALID					0x02
#endif
#ifndef LEG_FLY_OVER
#define LEG_FLY_OVER					0x04
#endif
#ifndef LEG_TURN_DIRECTION_L
#define LEG_TURN_DIRECTION_L			0x01
#endif
#ifndef LEG_TURN_DIRECTION_R
#define LEG_TURN_DIRECTION_R			0x02
#endif
#ifndef LEG_TURN_DIRECTION_E
#define LEG_TURN_DIRECTION_E			0x03
#endif
#ifndef LEG_ALT_DESCRIPTOR_A
#define LEG_ALT_DESCRIPTOR_A			0x01
#endif
#ifndef LEG_ALT_DESCRIPTOR_PLUS
#define LEG_ALT_DESCRIPTOR_PLUS			0x02
#endif
#ifndef LEG_ALT_DESCRIPTOR_MINUS
#define LEG_ALT_DESCRIPTOR_MINUS		0x03
#endif
#ifndef LEG_ALT_DESCRIPTOR_B
#define LEG_ALT_DESCRIPTOR_B			0x04
#endif
#ifndef LEG_ALT_DESCRIPTOR_C
#define LEG_ALT_DESCRIPTOR_C			0x02
#endif
#ifndef LEG_ALT_DESCRIPTOR_G
#define LEG_ALT_DESCRIPTOR_G			0x01
#endif
#ifndef LEG_ALT_DESCRIPTOR_H
#define LEG_ALT_DESCRIPTOR_H			0x02
#endif
#ifndef LEG_ALT_DESCRIPTOR_J
#define LEG_ALT_DESCRIPTOR_J			0x02
#endif
#ifndef LEG_ALT_DESCRIPTOR_V
#define LEG_ALT_DESCRIPTOR_V			0x02
#endif
#ifndef TPATH_TYPE_TAXIWAY
#define TPATH_TYPE_TAXIWAY				0x01
#endif
#ifndef TPATH_TYPE_TAXI
#define TPATH_TYPE_TAXI					TPATH_TYPE_TAXIWAY
#endif
#ifndef TPATH_TYPE_RUNWAY
#define TPATH_TYPE_RUNWAY				0x02
#endif
#ifndef TPATH_TYPE_PARKING
#define TPATH_TYPE_PARKING				0x03
#endif
#ifndef TPATH_TYPE_PATH
#define TPATH_TYPE_PATH					0x04
#endif
#ifndef TPATH_TYPE_CLOSED
#define TPATH_TYPE_CLOSED				0x05
#endif
#ifndef TPATH_DRAW_SURFACE
#define TPATH_DRAW_SURFACE				0x20
#endif
#ifndef TPATH_DRAW_DETAIL
#define TPATH_DRAW_DETAIL				0x40
#endif
#ifndef TPATH_CENTERLINE
#define TPATH_CENTERLINE				0x01
#endif
#ifndef TPATH_CENTERLINE_LIGHTED
#define TPATH_CENTERLINE_LIGHTED		0x02
#endif
#ifndef TPATH_LEFTEDGE_SOLID
#define TPATH_LEFTEDGE_SOLID			0x04
#endif
#ifndef TPATH_LEFTEDGE_DASHED
#define TPATH_LEFTEDGE_DASHED			0x08
#endif
#ifndef TPATH_LEFTEDGE_LIGHTED
#define TPATH_LEFTEDGE_LIGHTED			0x10
#endif
#ifndef TPATH_LEFTEDGE_SOLID_DASHED
#define TPATH_LEFTEDGE_SOLID_DASHED		0x0C
#endif
#ifndef TPATH_RIGHTEDGE_SOLID
#define TPATH_RIGHTEDGE_SOLID			0x20
#endif
#ifndef TPATH_RIGHTEDGE_DASHED
#define TPATH_RIGHTEDGE_DASHED			0x40
#endif
#ifndef TPATH_RIGHTEDGE_SOLID_DASHED
#define TPATH_RIGHTEDGE_SOLID_DASHED	0x60
#endif
#ifndef TPATH_RIGHTEDGE_LIGHTED
#define TPATH_RIGHTEDGE_LIGHTED			0x80
#endif
#ifndef GEOPOL_COASTLINE
#define GEOPOL_COASTLINE	0x80
#endif
#ifndef GEOPOL_BOUNDARY
#define GEOPOL_BOUNDARY		0x40
#endif

/*==========================================================================
    Section 3 - Non-duplicate structures imported from Full New AFD.h

    Intentionally not duplicated because NewAFD.h already has active versions:
    NBGLHDR, NOBJ, NSECTS, NNAME/NNAM, NAPT, NRWY, NILS/NILSGS, NSTART,
    NCOMM, NGATEHDR/NGATE/NGATE2/NGATE3/NGATE4, NTAXIHDR/NTAXI/NEWNTAXI/
    NEWNTAXI2/MSFSNTAXI, NTAXINM, NREGION/NICAO, helipad_t, vasi_t,
    approachlights_t, NAPRON1HDR/NAPRON2HDR.
  ==========================================================================*/

// Name info header used by legacy navaid records.
// Appears before the variable-length name string.
typedef struct tag_chr_hdr_t
{
	WORD wId;			// always 0x0019
	__int32 nStrLen;	// length of name string
} name_hdr_t;

typedef struct tag_ndb_t
{
	WORD wType;				// navaid type - 0x17 for ndb
	DWORD nLen;				// length of record in bytes
	WORD wNdbType;			// ndb type, compass_point, h, etc.
	int nFreq;				// navaid frequency * 1000
	int nLon;				// packed longitude
	int nLat;				// packed latitude
	int nAlt;				// navaid altitude, meters * 1000
	float fRange;			// navaid range
	float fMagVar;			// magnetic deviation
	char szId[4];			// encrypted navaid ID
	char szRegion[4];		// encrypted navaid Region
	name_hdr_t lpNameInfo;	// name info header
} ndb_t;

typedef struct tag_vor_t
{
	WORD wType;				//  0 navaid type - 0x13 for vor
	DWORD nLen;				//  2 length of record in bytes
	BYTE bType;				//  6 type
	BYTE bFlags;			//  7 various flags - default 0x10, BIT_DME_ONLY or'ed
	int nLon;				//  8 packed longitude
	int nLat;				// 12 packed latitude
	int nAlt;				// 16 altitude, meters * 1000
	int nFreq;				// 20 navaid frequency * 1000
	float fRange;			// 24 vor range
	float fMagVar;			// 28 magnetic deviation
	char szId[4];			// 32 encrypted navaid ID
	char szRegion[4];		// 36 encrypted navaid Region
	WORD wDmeType;			// 40 navaid type - 0x1600 for DME
	int nDmeLen;			// 42 length of record in bytes
	WORD wUnk1;				// 46 unknown word 1 - maybe used for 32 bits padding
	int nDmeLon;			// 48 packed DME longitude
	int nDmeLat;			// 52 packed DME latitude
	int nDmeAlt;			// 56 DME altitude * 1000
	float fDmeRange;		// 60 DME Range
	name_hdr_t lpNameInfo;	// 64 name info header
} NVOR;

typedef struct tag_mkb_t
{
	WORD wType;				// navaid type - 0x18 for marker
	WORD wLen;				// length of record in bytes
	BYTE bReserved;			// reserved - always 0x0
	WORD wHeading;			// heading in pseudo degrees
	BYTE bMkbType;			// marker type, inner, outer, etc.
	int nLon;				// packed longitude
	int nLat;				// packed latitude
	int nAlt;				// altitude, meters * 1000
	int nId;				// encrypted navaid ID
	int nRegion;			// encrypted navaid Region
} mkb_t;

typedef struct tag_wpt_t
{
	WORD wType;				// navaid type - 0x22 for waypoint
	DWORD nLen;				// length of record in bytes
	BYTE bWptKind;			// waypoint type, NAMED, UNNAMED, etc.
	BYTE bHasRoute;			// waypoint has nested route (0x0 or 0x1)
	int nLon;				// packed longitude
	int nLat;				// packed latitude
	float fMagVar;			// magvar (courtesy of Marco Sinchetto)
	int nId;				// encrypted navaid ID
	int nRegion;			// encrypted navaid Region
} wpt_t;

typedef struct tag_route_t
{
	BYTE bType;				// route type
	char szName[8];			// route name
	int nPrevId;			// next wayp. encrypted ID
	int nPrevRegion;		// next wayp. encrypted Region
	float fPrevAltitude;	// next wayp. altitude in meters
	int nNextId;			// next wayp. encrypted ID
	int nNextRegion;		// next wayp. encrypted Region
	float fNextAltitude;	// next wayp. altitude in meters
	BYTE bReserved[3];		// padded with 0x0
} route_t;

typedef struct tag_boundary_t
{
	WORD wType;				// navaid type - 0x20 for boundary
	DWORD nLen;				// length of record in bytes
	BYTE bBoundaryKind;		// boundary type, ALERT, APPROACH, etc.
	BYTE bAltKind;			// low part stores min_alt kind, high part stores max
	int nMinLon;			// packed longitude
	int nMinLat;			// packed latitude
	int nMinAlt;			// altitude * 1000
	int nMaxLon;			// packed longitude
	int nMaxLat;			// packed latitude
	int nMaxAlt;			// altitude * 1000
	WORD wNameId;			// name id, 0x19
	DWORD nNameLen;			// name length + 2 ?
} boundary_t;

typedef struct tag_origin_t
{
	WORD wType;				// 0000 - navaid type (0x21)
	DWORD nLen;				// 0002 - length of record in bytes
	WORD wCount;			// 0006 - count of origin entries
	WORD wUnk1;				// 0008 - unknown word 1;
} origin_t;

typedef struct tag_rgba_t
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
} rgba_t;

typedef struct tag_objheader_t
{
	WORD wId;				// object ID
	WORD wRecLen;			// record length in bytes
	int nLon;				// packed longitude
	int nLat;				// packed latitude
	int nAlt;				// altitude * 1000
	WORD bAltIsAgl;			// altitude is AGL
	WORD wPitch;			// pitch in pseudo-degrees
	WORD wBank;				// bank in pseudo-degrees
	WORD wHeading;			// heading in pseudo-degrees
	int nComplexity;		// image complexity
} objheader_t;

typedef struct tag_libobj_t
{
	DWORD lpGuid[4];		// object 128-bit GUID
	float fScale;			// object scale
} libobj_t;

typedef struct tag_windsock_t
{
	float fPoleHeight;		// pole height
	float fSockLength;		// sock length
	rgba_t lpPoleColor;		// pole color RGB
	rgba_t lpSockColor;		// sock color RGB
	WORD wLighted;			// windsock lighted, true or false
} windsock_t;

typedef struct tag_effect_t
{
	char szName[80];		// effect name
	char *pszParam;			// effect parameters (dynamically alloc'ed)
} effect_t;

typedef struct tag_trigger_weather_t
{
	WORD wId;				// trigger ID (0x0001)
	float fHeight;			// trigger height
	WORD wWeatherType;		// weather type, THERMAL, etc.
	float fDataHdg;			// weather data heading
	float fDataScalar;		// weather data scalar
	int nVertexCount;		// number of vertex
} trigger_weather_t;

typedef struct tag_trigger_vertex_t
{
	float fBiasX;
	float fBiasZ;
} trigger_vertex_t;

typedef struct tag_trigger_refuel_t
{
	WORD wId;				// trigger ID (0x0000)
	float fHeight;			// trigger height
	DWORD nAvailability;		// fuel kind and availability
	BYTE bFiller[10];		// 10 bytes data filler
} trigger_refuel_t;

typedef struct tag_building_hdr_t
{
	float fScale;			// scale of building
	WORD wUnk1;				// unknown WORD - offset to end of chunk ?
	WORD wChunkLen;			// length of building chunk
	WORD wId;				// building ID
} building_hdr_t;

typedef struct tag_rect_bldgfr_t
{
	WORD wSizeX;			// size X
	WORD wSizeZ;			// size Z

	WORD wBottomTexture;	// bottom texture
	WORD wSizeBottomY;		// Size Bottom Y
	WORD wTextIdxBottomX;	// texture index bottom X
	WORD wTextIdxBottomZ;	// texture index bottom Z

	WORD wWindowTexture;	// window texture
	WORD wSizeWindowY;		// size window Y
	WORD wTextIdxWindowX;	// texture index Window X
	WORD wTextIdxWindowY;	// texture index Window Y
	WORD wTextIdxWindowZ;	// texture index Window Z

	WORD wTopTexture;		// top texture
	WORD wSizeTopY;			// size top Y
	WORD wTextIdxTopX;		// texture index top X
	WORD wTextIdxTopZ;		// texture index top Z

	WORD wRoofTexture;		// roof texture
	WORD wTextIdxRoofX;		// texture index top X
	WORD wTextIdxRoofZ;		// texture index top Z
	WORD wUnk5;				// unknown word 4 - 0x22
} rect_bldgfr_t;

typedef struct tag_rect_bldgpr_t
{
	WORD wSizeX;			// size X
	WORD wSizeZ;			// size Z
	WORD wBottomTexture;	// bottom texture
	WORD wSizeBottomY;		// size bottom Y
	WORD wTextIdxBottomX;	// texture index bottom X
	WORD wTextIdxBottomZ;	// texture index bottom Z

	WORD wWindowTexture;	// window texture
	WORD wSizeWindowY;		// size window Y
	WORD wTextIdxWindowX;	// texture index Window X
	WORD wTextIdxWindowY;	// texture index Window Y
	WORD wTextIdxWindowZ;	// texture index Window Z

	WORD wTopTexture;		// unknown word 3 - 0x44
	WORD wSizeTopY;			// size top Y
	WORD wTextIdxTopX;		// texture index top X
	WORD wTextIdxTopZ;		// texture index top Z

	WORD wRoofTexture;		// unknown word 4 - 0x19
	WORD wTextIdxRoofX;		// texture index roof X
	WORD wTextIdxRoofZ;		// texture index roof Z
	WORD wSizeRoofY;		// size roof Y
	WORD wTextIdxRoofY;		// texture index roof Y
	WORD wUnk5;				// unknown word 5 - 0x22
} rect_bldgpr_t;

typedef struct tag_rect_bldgrr_t
{
	WORD wSizeX;			// size X
	WORD wSizeZ;			// size Z

	WORD wBottomTexture;	// bottom texture
	WORD wSizeBottomY;		// Size Bottom Y
	WORD wTextIdxBottomX;	// texture index bottom X
	WORD wTextIdxBottomZ;	// texture index bottom Z

	WORD wWindowTexture;	// window texture
	WORD wSizeWindowY;		// Size Window Y
	WORD wTextIdxWindowX;	// texture index Window X
	WORD wTextIdxWindowY;	// texture index Window Y
	WORD wTextIdxWindowZ;	// texture index Window Z

	WORD wTopTexture;		// top texture
	WORD wSizeTopY;			// size top Y
	WORD wTextIdxTopX;		// texture index top X
	WORD wTextIdxTopZ;		// texture index top Z

	WORD wRoofTexture;		// roof texture
	WORD wTextIdxRoofX;		// texture index roof X
	WORD wTextIdxRoofZ;		// texture index roof Z
	WORD wSizeRoofY;		// size roof Y
	WORD wTextIdxGableY;	// gable texture index Y

	WORD wGableTexture;		// gable texture
	WORD wTextIdxGableZ;	// gable texture index Z
	WORD wUnk5;				// unknown word 5 - 0x22
} rect_bldgrr_t;

typedef struct tag_rect_bldgsr_t
{
	WORD wSizeX;			// size X
	WORD wSizeZ;			// size Z

	WORD wBottomTexture;	// bottom texture
	WORD wSizeBottomY;		// size bottom Y
	WORD wTextIdxBottomX;	// texture index bottom X
	WORD wTextIdxBottomZ;	// texture index bottom Z

	WORD wWindowTexture;	// window texture
	WORD wSizeWindowY;		// size window Y
	WORD wTextIdxWindowX;	// texture index Window X
	WORD wTextIdxWindowY;	// texture index Window Y
	WORD wTextIdxWindowZ;	// texture index Window Z

	WORD wTopTexture;		// unknown word 3 - 0x44
	WORD wSizeTopY;			// size top Y
	WORD wTextIdxTopX;		// texture index top X
	WORD wTextIdxTopZ;		// texture index top Z

	WORD wRoofTexture;		// unknown word 4 - 0x19
	WORD wTextIdxRoofX;		// texture index roof X
	WORD wTextIdxRoofZ;		// texture index roof Z

	WORD wSizeRoofY;		// size roof Y
	WORD wTextIdxGableY;	// gable texture index Y
	WORD wGableTexture;		// gable texture
	WORD wTextIdxGableZ;	// gable texture index Z
	WORD wFaceTexture;		// face texture
	WORD wTextIdxFaceX;		// texture index face X
	WORD wTextIdxFaceY;		// texture index face Y
	WORD wUnk5;				// unknown word 5 - 0x22
} rect_bldgsr_t;

typedef struct tag_pyram_bldg_t
{
	WORD wSizeX;			// size X
	WORD wSizeZ;			// size Z
	WORD wSizeTopX;			// size top X
	WORD wSizeTopZ;			// size top Z

	WORD wBottomTexture;	// bottom texture
	WORD wSizeBottomY;		// size bottom Y
	WORD wTextIdxBottomX;	// texture index bottom X
	WORD wTextIdxBottomZ;	// texture index bottom Z

	WORD wWindowTexture;	// unknown word 2 - 0x29
	WORD wSizeWindowY;		// Size Window Y
	WORD wTextIdxWindowX;	// texture index Window X
	WORD wTextIdxWindowY;	// texture index Window Y
	WORD wTextIdxWindowZ;	// texture index Window Z

	WORD wTopTexture;		// unknown word 3 - 0x44
	WORD wSizeTopY;			// size top Y
	WORD wTextIdxTopX;		// texture index top X
	WORD wTextIdxTopZ;		// texture index top Z

	WORD wRoofTexture;		// unknown word 4 - 0x19
	WORD wTextIdxRoofX;		// texture index roof X
	WORD wTextIdxRoofZ;		// texture index roof Z
	WORD wUnk5;				// unknown word 5 - 0x22
} pyram_bldg_t;

typedef struct tag_ms_bldg_t
{
	WORD wSidesCount;		// # of sides

	WORD wSizeX;			// size X
	WORD wSizeZ;			// size Z
	WORD wBottomTexture;	// bottom texture ?

	WORD wSizeBottomY;		// Size Bottom Y
	WORD wTextIdxBottomX;	// texture index bottom X
	WORD wWindowTexture;	// window texture

	WORD wSizeWindowY;		// size window Y
	WORD wTextIdxWindowX;	// texture index Window X
	WORD wTextIdxWindowY;	// texture index Window Y

	WORD wTopTexture;		// top texture
	WORD wSizeTopY;			// size top Y

	WORD wTextIdxTopX;		// texture index top X
	WORD wRoofTexture;		// roof texture

	WORD wSizeRoofY;		// size roof Y
	WORD wTextIdxRoofX;		// texture index roof X
	WORD wTextIdxRoofZ;		// texture index roof Z
	WORD wUnk5;				// unknown word 5 - 0x22
} ms_bldg_t;

typedef struct tag_excrec_t
{
	WORD wFlags;			// exclusion flags, see above
							// low byte stores beacon to libobj
							// high byte stores taxiway to windsock
	WORD wFiller;			// filler
	int nLonMin;			// minimum longitude, packed format
	int nLatMax;			// maximum latitude, packed format
	int nLonMax;			// maximum longitude, packed format
	int nLatMin;			// minimum latitude, packed format
} excrec_t;

typedef struct ilsdme_t
{
	WORD wId;				// record ID (0x16)
	DWORD nLen;				// length of record in bytes
	WORD wFiller;			// filler
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
	int nAlt;				// altitude, meters * 1000
	float fRange;			// range in meters
} ilsdme_t;

typedef struct tag_delete_airport_t
{
	WORD wId;				// record ID (0x26)
	DWORD nLen;				// record length
	WORD wFlags;			// delete flags, see above
	BYTE bDelRwyCount;		// number of deleterunway records
	BYTE bDelStartCount;	// number of deletestart records
	BYTE bDelFreqCount;		// number of deletefrequency records
	BYTE bPad;				// padder
} delete_airport_t;

typedef struct tag_delete_runway_t
{
	BYTE bSurface;			// runway surface
	BYTE bStartNumber;		// runway start number
	BYTE bEndNumber;		// runway end number
	BYTE bDesignator;		// designator (high part stores start, low part end)
} delete_runway_t;

typedef struct tag_delete_start_t
{
	BYTE bNumber;			// number
	BYTE bDesignator;		// runway start designator (left, right, etc.)
	BYTE bType;				// type of runway (runway, helipad, water)
	BYTE bPad;				// pad (0x0)
} delete_start_t;

typedef struct tag_delete_freq_t
{
	int nPacked;			// packed frequency number and comm type
} delete_freq_t;

typedef struct tag_latlon_t
{
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
} latlon_t;

typedef struct tag_approach_t
{
	WORD wId;				// 0000 - record ID (0x24)
	DWORD nLen;				// 0002 - record length
	BYTE bSuffix;			// 0006 - suffix [0-9,A-Z,a-z]
	BYTE bNumber;			// 0007 - runway number (0-36)
	BYTE bApprDesignator;	// 0008 - a combination of designator (high part)
							// and approach_type flags (low part)
	BYTE lpPad[3];			// 0009 - padder
	int nId;				// 0012	- fixIdent in packed format (seed 0x42)
	int nRegion;			// 0016 - region in packed format
	float fAltitude;		// 0020 - altitude
	float fHeading;			// 0024 - heading
	float fMissedAltitude;	// 0028 - missed altitude
} approach_t;

typedef struct tag_approach_legs_t
{
	WORD wId;				// 0000 - record ID (0x2F 0x2D or 0x2E)
	DWORD nLen;				// 0002 - record length
	WORD wLegsCount;		// 0006 - count of legs in
} approach_legs_t;

typedef struct tag_transition_t
{
	WORD wId;				// 0000 - record ID (0x2C)
	DWORD nLen;				// 0002 - record length
	BYTE bType;				// 0006 - transition types
	BYTE bLegCount;			// 0007 - number of legs
	DWORD nId;				// 0008 - fix id
	DWORD nRegion;			// 0012 - fix region
	float fAltitude;		// 0016 - altitude in meters
	// the following is included only if transition_type = DME
	DWORD nDmeId;			// 0020 - DME id
	DWORD nDmeRegion;		// 0024 - DME region
	DWORD nRadial;			// 0028 - DME radial
	float fDmeDistance;		// 0032 - DME distance in meters
} transition_t;

typedef struct tag_leg_t
{
	BYTE bLegType;			// 0000 - leg type
	BYTE bAltitudeDtor;		// 0001 - altitude descriptor
	BYTE bTurnDirection;	// 0002 - turn direction
	BYTE bFlags;			// 0003 - stores: FLY OVER, TIME, TRUE COURSE bits
	DWORD nFixId;			// 0004 - fix id in packed format
							//        the lowest byte also stores
							//        the FIX_TYPE_xxx constants
							//        into its low part
	DWORD nFixRegion;		// 0008 - fix region in packed format
	DWORD nRecomId;			// 0012 - recommended id in packed format
							//        the lowest byte also stores
							//        the FIX_TYPE_xxx constants
							//        into its low part
	DWORD nRecomRegion;		// 0016 - recommended region in packed format
	float fTheta; 			// 0020 - theta
	float fRho; 			// 0024 - theta
	float fTrueCorse;		// 0028 - true corse or magnetic course, pseudo deg
	float fDistance;		// 0032 - distance
	float fAltitude1;		// 0036 - altitude 1
	float fAltitude2;		// 0040 - altitude 2
} leg_t;

typedef struct tag_twp_entry_t
{
	BYTE bType;				// 0000 - type, normal or hold_short
	BYTE bOrientation;		// 0001 - 0: forward, 1: reverse
	WORD wUnk1;				// 0002 - unknown word 1
	int nLon;				// 0004 - longitude in packed format
	int nLat;				// 0008 - latitude in packed format
} twp_entry_t;

typedef struct tag_twp_t
{
	WORD wId;				// 0000 - record ID (0x1A)
	DWORD nLen;				// 0002 - record length
	WORD wSize;				// 0006 - size of point array
} twp_t;

typedef struct tag_apron_edgelights_t
{
	WORD wId;				// 0000 - record ID (0x31)
	DWORD nLen;				// 0002 - record length
	WORD wUnk1;				// 0006 - unknown word 1
	WORD wVertexCount;		// 0008 - number of edge light vertex
	BYTE lpUnk1[14];		// 0010 - unknown 14 bytes chunk ?
} apron_edgelights_t;

typedef struct tag_geopol_t
{
	WORD wId;				// 0000 - record ID (0x23)
	DWORD nLen;				// 0002 - record length
	BYTE bVertexCount;		// 0006 - number of vertex (255 max?)
	BYTE bFlags;			// 0007 - flags, see constants above
	int nMinEast;			// 0008 - minimum east of geopol rectangle
	int nMinNorth;			// 0012 - minimum north of geopol rectangle
	int nMaxEast;			// 0016 - maximum east of geopol rectangle
	int nMaxNorth;			// 0020 - maximum north of geopol rectangle
} geopol_t;

typedef struct tag_center_freq_t
{
	WORD wId;				// 0000 - record ID (0x12)
	DWORD nLen;				// 0002 - record length 
	WORD wUnk1;				// 0006 - unknown word, always 0xA ?
	int nFreq;				// 0008 - frequency, Mhz * 1000000
} center_freq_t;


#pragma pack(pop)

#endif /* ITC_FULLAFD_H */
