// Thanks are due to Alessandro G. Antonini, author of BGLXML, for much of this data
#define ADDMOREVASI 1
#define ADDFILEDETAILS 1

#ifdef ADDFILEDETAILS
	extern char *pPathName;
	extern char *pSceneryName;
#endif


typedef struct _NSECTS
{
	DWORD nObjType;			//  0 object type, see constants above
	DWORD nUnk1;			//  4 unknown integer 1
	DWORD nGroupsCount;		//  8 number of object groups
	DWORD nGroupOffset;		// 12 absolute pointer to groups info table
	DWORD nTableSize;		// 16 size  of info table array pointed by nGroupOffset
} NSECTS;

#define NSECTS_PER_FILE 32

typedef struct _NBGLHDR
{
	WORD wStamp;			//  0 0x0201
	WORD nUnk1;				//  2 unknown 0x1992 (maybe FS2004 version number)
	WORD size;				//  4 0x0038
	WORD wResvd1;			//  6 reserved (0x0000)
	BYTE szCrc[6];			//  8 48-bits CRC or what ???
	BYTE szUnk1[2];			// 14 always 0xC301 .............C6 01 in FSX
	int nMagic;				// 16 magic number ? - 13451555 ... 134551555
	DWORD nObjects;			// 20 number of object groups within file
	int nLatMax;			// 24 latitude max ???
	int nLatMin;			// 28 latitude min ???
	int nLonMax;			// 32 longitude max ???
	int nLonMin;			// 36 longitude min ???
	int nUnk2;				// 40 unknown integer 2
	int nUnk3;				// 44 unknown integer 3
	BYTE lpResvd[8];		// 48 reserved
	NSECTS sects[NSECTS_PER_FILE];		// Allow fr max number of sects likely ???
} NBGLHDR;

typedef struct _NOBJ
{	DWORD unk;
	int chunkctr;
	DWORD chunkoff;
	DWORD chunksize;
} NOBJ;

// Objects IDs
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
#define	OBJTYPE_NEWAIRPORT	0x003C
#define	OBJTYPE_NEWNEWAIRPORT	0x00AB
#define	OBJTYPE_NEWTAXIPARK	0x003D
#define	OBJTYPE_NEWNEWTAXIPARK	0x00AD
#define	OBJTYPE_NEWRUNWAY	0x003E
#define OBJTYPE_NEWTAXIPATH	0x0040
#define OBJTYPE_NEWNEWTAXIPATH	0x00AE
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
	int nLon;				// 12 longitude, packed format
	int nLat;				// 16 latitude, packed format
	int nAlt;				// 20 altitude * 1000
	int nTowerLon;			// 24 tower longitude, courtesy of Marco Sinchetto
	int nTowerLat;			// 28 tower latitude
	int nTowerAlt;			// 32 tower altitude meters * 1000
	float fMagVar;			// 36 magnetic deviation
	DWORD nId;				// 40 packed ICAO ID (0 = 65)
	int nUnk4;				// 44 unknown integer 4
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
	int nLon;				// 20 longitude in packed format
	int nLat;				// 24 latitude in packed format
	int nAlt;				// 28 altitude, meters * 1000
	float fLength;			// 32 length in meters
	float fWidth;			// 36 width in meters
	float fHeading;			// 40 approach heading
	float fPatternAlt;		// 44 pattern altitude in meters
	WORD wMarkings;			// 48 marking flags, see above
	BYTE bLights;			// 50 lights flags, see above
	BYTE bPatternFlags;		// 51 pattern flags, see above
} NRWY;

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
	int nLon;				//  8 longitude in packed format
	int nLat;				// 12 latitude in packed format
	int nAlt;				// 16 altitude, meters * 1000
	float fHeading;			// 20 heading in degrees
} NSTART;

// airport COM structure
typedef struct _NCOMM
{
	WORD wId;				// 0 record ID (0x12)
	DWORD nLen;				// 2 length of record in bytes
	BYTE bCommType;			// 6 comm type, see above
	BYTE nUnknown;
	int nFreq;				// 8 frequency * 1000000
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
	int nLon;				//  8 longitude in packed format
	int nLat;				// 12 latitude in packed format
	int nAlt;				// 16 altitude, meters * 1000
	int nFreq;				// 20 frequency * 1000000
	float fRange;			// 24 range in meters
	float fMagVar;			// 28 magnetic deviation
	int nId;				// 32 packed ICAO ID - base 0x42
	int nRegion;			// 36 packed region ???
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
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
} NGATE;

typedef struct _NJETWAY
{	WORD wId;				// record ID (0x3A)
	DWORD nLen;				// record length
	WORD wParkingNumber;	// ref to existing gate (top 12 bits of wNumberType
	DWORD dwUnkown;
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
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
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
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
	int nAlt;
} NGATE3;

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
	int nWeightLimit;		// 0012 - weigth limit
	int nUnk;				// 0016 - unknown integer - terminator
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
	int nWeightLimit;		// 0012 - weigth limit
	int nUnk;				// 0016 - unknown integer - terminator
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
	int nWeightLimit;		// 0012 - weigth limit
	int nUnk;				// 0016 - unknown integer - terminator
	BYTE bUnk[16];			// 0020 - 16 extra bytes unknown use
	int nflatten;
	// ACCORDING TO Jon: int nsurfaceQuery;
} NEWNTAXI2;

// taxiway point entry structure
#define TWP_NORMAL			0x00000001
#define TWP_HOLD_SHORT		0x00000002

typedef struct _NTAXIPT
{	BYTE bType;				// 0000 - type, normal or hold_short
	BYTE bOrientation;		// 0001 - 0: forward, 1: reverse
	WORD wUnk1;				// 0002 - unknown word 1
	union
	{	int nLon;			// 0004 - longitude in packed format
		float fLon;
	};
	union
	{	int nLat;				// 0008 - latitude in packed format
		float fLat;
	};
} NTAXIPT;

typedef struct _NEWTAXIPT
{	BYTE bType;				// 0000 - type, normal or hold_short
	BYTE bOrientation;		// 0001 - 0: forward, 1: reverse
	WORD wUnk1;				// 0002 - unknown word 1
	union
	{	int nLon;			// 0004 - longitude in packed format
		float fLon;
	};
	union
	{	int nLat;				// 0008 - latitude in packed format
		float fLat;
	};
	int nAltitude;
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
	int nRegionPtr;			// 0018 - relative offset to region name
	int nCountryPtr;		// 0022 - relative offset to country name
	int nStatePtr;			// 0026 - relative offset to state name
	int nCityPtr;			// 0030 - relative offset to city name
	int nAirportPtr;		// 0034 - relative offset to airport names
	int nIcaoPtr;			// 0038 - relative offset to ICAO IDs
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
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
	int nAlt;				// altitude, meters * 1000
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
