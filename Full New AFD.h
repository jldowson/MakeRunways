///////////////////////////////////////////////////////////
// 56 bytes initial header
///////////////////////////////////////////////////////////
typedef struct _tag_facdata_hdr
{
	WORD wStamp;			//  0 0x0201
	WORD nUnk1;				//  2 unknown 0x1992 (maybe FS2004 version number)
	WORD size;				//  4 0x0038
	WORD wResvd1;			//  6 reserved (0x0000)
	BYTE szCrc[6];			//  8 48-bits CRC or what ???
	BYTE szUnk1[2];			// 14 always 0xC301
	int nMagic;				// 16 magic number ? - 13451555
	DWORD nObjects;			// 20 number of object groups within file
	int nLatMax;			// 24 latitude max ???
	int nLatMin;			// 28 latitude min ???
	int nLonMax;			// 32 longitude max ???
	int nLonMin;			// 36 longitude min ???
	int nUnk2;				// 40 unknown integer 2
	int nUnk3;				// 44 unknown integer 3
	BYTE lpResvd[8];		// 48 reserved, always 0x0 ?
} NBGLHDR;

typedef struct _tag_objs
{	DWORD unk;
	int chunkctr;
	DWORD chunkoff;
	DWORD chunksize;
} NOBJ;

// Objects IDs
#define OBJTYPE_AIRPORT_MSFS 0x0056
#define OBJTYPE_AIRPORT		0x0003
#define OBJTYPE_RUNWAY		0x0004
#define OBJTYPE_VOR			0x0013
#define OBJTYPE_DME			0x0016
#define OBJTYPE_NDB			0x0017
#define OBJTYPE_MKB			0x0018
#define OBJTYPE_BOUNDARY	0x0020
#define OBJTYPE_WPT			0x0022
#define OBJTYPE_GEOPOL		0x0023
#define OBJTYPE_SCNOBJ		0x0025
#define OBJTYPE_REGIONINF	0x0027
#define OBJTYPE_MDLDATA		0x002B
#define OBJTYPE_EXCREC		0x002E

/*
 * at offset 56 dec we find the object groups table
 * array of 20 bytes size structures
 * number of groups is defined by nObjects field
 * of BGL header structure
 */
typedef struct tag_obj_table_group
{
	DWORD nObjType;			//  0 object type, see constansts above
	DWORD nUnk1;			//  4 unknown integer 1
	DWORD nGroupsCount;		//  8 number of object groups
	DWORD nGroupOffset;		// 12 absolute pointer to groups info table
	DWORD nTableSize;		// 16 size  of info table array pointed by nGroupOffset
} NSECTS;



#define MAXSOUTH 536870912
#define MAXEAST 805306368

#define NDB_MAXSOUTH MAXSOUTH
#define NDB_MAXEAST MAXEAST

#define VOR_MAXSOUTH NDB_MAXSOUTH
#define VOR_MAXEAST NDB_MAXEAST

// the following structure appears as a header before each navaids
// description
typedef struct tag_chr_hdr_t
{
	WORD wId;				// always 0x0019
	int nStrLen;			// length of name string
} name_hdr_t;


// NDB info
#define NDB_COMPASS_POINT	0x0000
#define NDB_MH				0x0001
#define NDB_H				0x0002
#define NDB_HH				0x0003

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

// VOR info
#define VOR_TERMINAL		0x01
#define VOR_LOW				0x02
#define VOR_HIGH			0x03
#define VOR_ILS				0x04
#define VOR_VOT				0x05

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

// marker info
#define MKB_INNER			0x00
#define MKB_MIDDLE			0x01
#define MKB_OUTER			0x02
#define MKB_BACKCOURSE		0x03

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

// waypoint info
#define WPT_NAMED			0x01
#define WPT_UNNAMED			0x02
#define WPT_VOR				0x03
#define WPT_NDB				0x04
#define WPT_OFFROUTE		0x05
#define WPT_IAF				0x06
#define WPT_FAF				0x07

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

// routes are nested immediately after waypoints
#define RTE_VICTOR			0x01
#define RTE_JET				0x02
#define RTE_BOTH			0x03

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

// boundary info
#define BOUNDARY_NONE			0x00
#define BOUNDARY_CENTER			0x01
#define BOUNDARY_CLASS_A		0x02
#define BOUNDARY_CLASS_B		0x03
#define BOUNDARY_CLASS_C		0x04
#define BOUNDARY_CLASS_D		0x05
#define BOUNDARY_CLASS_E		0x06
#define BOUNDARY_CLASS_F		0x07
#define BOUNDARY_CLASS_G		0x08
#define BOUNDARY_TOWER			0x09
#define BOUNDARY_CLEARANCE		0x0A
#define BOUNDARY_GROUND			0x0B
#define BOUNDARY_DEPARTURE		0x0C
#define BOUNDARY_APPROACH		0x0D
#define BOUNDARY_MOA			0x0E
#define BOUNDARY_RESTRICTED		0x0F
#define BOUNDARY_PROHIBITED		0x10
#define BOUNDARY_WARNING		0x11
#define BOUNDARY_ALERT			0x12
#define BOUNDARY_DANGER			0x13
#define BOUNDARY_NATIONAL_PARK	0x14
#define BOUNDARY_MODEC			0x15
#define BOUNDARY_RADAR			0x16
#define BOUNDARY_TRAINING		0x17

#define BOUNDARY_ALT_UNKNOWN	0x01
#define BOUNDARY_ALT_MSL		0x01
#define BOUNDARY_ALT_AGL		0x02
#define BOUNDARY_ALT_UNLIMITED	0x03


// boundary (0x20)
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

// origin (0x21)
typedef struct tag_origin_t
{
	WORD wType;				// 0000 - navaid type (0x21)
	DWORD nLen;				// 0002 - length of record in bytes
	WORD wCount;			// 0006 - count of origin entries
	WORD wUnk1;				// 0008 - unknown word 1;
} origin_t;
// after this, follows an array of (lat/lon/terminator) whose size is
// defined by the wCount field. This differs from other entries
// for lat comes first, lon comes second. terminator is a 16 bits integer

#pragma pack(push, 1)


// color structure
typedef struct tag_rgba_t
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
} rgba_t;

// scenery complexity
#define COMPLEXITY_VERY_SPARSE			0x00000000
#define COMPLEXITY_SPARSE				0x00000001
#define COMPLEXITY_NORMAL				0x00000002
#define COMPLEXITY_DENSE				0x00000003
#define COMPLEXITY_VERY_DENSE			0x00000004
#define COMPLEXITY_EXTREMELY_DENSE		0x00000005


// scenery object IDs
#define SCNOBJ_BUILDING					0x0001
#define SCNOBJ_LIBRARY_OBJ				0x0002
#define SCNOBJ_WINDSOCK					0x0003
#define SCNOBJ_EFFECT					0x0004
#define SCNOBJ_TRIGGER					0x0007


// object header, common to all objects
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

// library object structure - 20 bytes
typedef struct tag_libobj_t
{
	DWORD lpGuid[4];		// object 128-bit GUID
	float fScale;			// object scale
} libobj_t;


// windsock structure - 18 bytes
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


#define TRIGGER_REFUEL_REPAIR				0x0000
#define TRIGGER_WEATHER						0x0001

#define TWEATHER_RIDGE_LIFT					0x0001
#define TWEATHER_NONDIRECTIONAL_TURBULENCE	0x0002
#define TWEATHER_DIRECTIONAL_TURBULENCE		0x0003
#define TWEATHER_THERMAL					0x0004

typedef struct tag_trigger_weather_t
{
	WORD wId;				// trigger ID (0x0001)
	float fHeight;			// trigger height
	WORD wWeatherType;		// weather type, THERMAL, etc.
	float fDataHdg;			// weather data heading
	float fDataScalar;		// weather data scalar
	int nVertexCount;		// number of vertex
} trigger_weather_t;

// after the above, an array of X,Z points in single point format
// follows - size of array is stored in the nVertexCount field
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



// building structures
#define BLDG_FLAT_ROOF		0x0004
#define BLDG_RIDGE_ROOF		0x0006
#define BLDG_PEAKED_ROOF	0x0007
#define BLDG_SLANT_ROOF		0x0008
#define BLDG_PYRAMIDAL		0x0009
#define BLDG_MULTISIDED		0x000A

// the building header
typedef struct tag_building_hdr_t
{
	float fScale;			// scale of building
	WORD wUnk1;				// unknown WORD - offset to end of chunk ?
	WORD wChunkLen;			// length of building chunk
	WORD wId;				// building ID
} building_hdr_t;

// the rectangular building with flat roof
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

// the rectangular building with peaked roof
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


// the rectangular building with ridge roof
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

// the rectangular building with slant roof
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

// the pyramidal building
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

// the multisided building
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


#pragma pack(pop)

// exclusion rectangle structure
#define BIT_EXCLUDE_ALL			0x08
#define BIT_EXCLUDE_BEACON		0x10
#define BIT_EXCLUDE_EFFECT		0x20
#define BIT_EXCLUDE_BUILDING	0x40
#define BIT_EXCLUDE_LIBOBJ		0x80

#define BIT_EXCLUDE_TAXIWAY		0x01
#define BIT_EXCLUDE_TRIGGER		0x02
#define BIT_EXCLUDE_WINDSOCK	0x04

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


// airport structure
typedef struct tag_airport_t
{
	WORD wId;				//  0 record ID (0x3)
	DWORD nLen;				//  2 length of record (32 bits)
	BYTE lpUnk[6];			//  6 filled with 0 ???
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
} NAPT;
// string-z airport name follows

// airport sub-entries
#define APTENTRY_COMM		0x0012
#define APTENTRY_RUNWAY		0x0004
#define APTENTRY_HELIPAD	0x0026
#define	APTENTRY_DELAPT		0x0033

#define SURFACE_CEMENT				0x0000
#define SURFACE_CONCRETE			0x0000
#define SURFACE_GRASS				0x0001
#define SURFACE_WATER				0x0002
#define SURFACE_ASPHALT				0x0004
#define SURFACE_CLAY				0x0007
#define SURFACE_SNOW				0x0008
#define SURFACE_ICE					0x0009
#define SURFACE_DIRT				0x000C
#define SURFACE_CORAL				0x000D
#define SURFACE_GRAVEL				0x000E
#define SURFACE_OIL_TREATED			0x000F
#define SURFACE_STEEL_MATS			0x0010
#define SURFACE_BITUMINOUS			0x0011
#define SURFACE_BRICK				0x0012
#define SURFACE_MACADAM				0x0013
#define SURFACE_PLANKS				0x0014
#define SURFACE_SAND				0x0015
#define SURFACE_SHALE				0x0016
#define SURFACE_TARMAC				0x0017
#define SURFACE_UNKNOWN				0x00FE

#define DESIGNATOR_NONE				0x00
#define DESIGNATOR_LEFT				0x01
#define DESIGNATOR_RIGHT			0x02
#define DESIGNATOR_CENTER			0x03
#define DESIGNATOR_WATER			0x04


#define PATTERN_NO_PRIM_TAKEOFF		0x01
#define PATTERN_NO_PRIM_LANDING		0x02
#define PATTERN_PRIMARY_RIGHT		0x04
#define PATTERN_NO_SEC_TAKEOFF		0x08
#define PATTERN_NO_SEC_LANDING		0x10
#define PATTERN_SECONDARY_RIGHT		0x20

#define MARKING_EDGE				0x0001
#define MARKING_THRESHOLD			0x0002
#define MARKING_FIXED				0x0004
#define MARKING_TOUCHDOWN			0x0008
#define MARKING_DASHES				0x0010
#define MARKING_IDENT				0x0020
#define MARKING_PRECISION			0x0040
#define MARKING_EDGE_PAVEMENT		0x0080
#define MARKING_SINGLE_END			0x0100
#define MARKING_PRIMARY_CLOSED		0x0200
#define MARKING_SECONDARY_CLOSED	0x0400
#define MARKING_PRIMARY_STOL		0x0800
#define MARKING_SECONDARY_STOL		0x1000

#define LIGHTS_EDGE_LOW				0x01
#define LIGHTS_EDGE_MEDIUM			0x02
#define LIGHTS_EDGE_HIGH			0x03
#define LIGHTS_CENTER_LOW			0x04
#define LIGHTS_CENTER_MEDIUM		0x08
#define LIGHTS_CENTER_HIGH			0x0C
#define LIGHTS_CENTERED				0x10

// runway structure
// NOTE: BGLCOMP fails if Markings are included
// when Lights, Vasi or other stuff are included before!!
// in poor words: put Markings as the very first entry under
// runways
typedef struct tag_runway_t
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
	WORD wId;				// record ID
							// 0x5 = primary end threshold
							// 0x6 = secondary end threshold
							// 0x7 = primary end blastpad
							// 0x8 = secondary end blastpad
							// 0x9 = primary end overrun
							// 0xA = secondary end overrun
	DWORD nLen;				// length of record
	WORD wSurface;			// threshold surface
	float fLength;			// threshold length
	float fWidth;			// threshold width
} offset_threshold_t, blastpad_t, overrun_t;

#define ILS_PRIMARY_END		0x01	// disregard this
#define ILS_BACKCOURSE		0x04
#define ILS_DME				0x10
#define ILS_GS				0x0C

// ILS entry:
// composed by (in order):
// ils_t
// ilsgs_t (optional)
// ilsdme_t (optional)
// and closing ILS name, with initial name_hdr_t structure

// ils structure
typedef struct tag_ils_t
{
	WORD wId;				//  0 record ID (0x13 - same as VOR !!!)
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
	int nUnk1;				// 40 unknown integer 1
	WORD wUnk1;				// 44 unknown word 1
	BYTE bEnd;				// 46 runway end stored in the high part and:
							//    0x0 is primary, 0x1 is secondary
	BYTE bUnk1;				// 47 unknown byte 1
	float fHeading;			// 48 heading approach
	float fWidth;			// 52 ILS width ???
} NILS;

// ils GS structure
typedef struct ilsgs_t
{
	WORD wId;				// record ID (0x15)
	DWORD nLen;				// length of record in bytes
	WORD wFiller;			// filler
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
	int nAlt;				// altitude, meters * 1000
	float fRange;			// range in meters
	float fPitch;			// GS pitch
} ilsgs_t;

// ils dme structure
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

// airport COM structure
typedef struct tag_aptcomm_t
{
	WORD wId;				// record ID (0x12)
	DWORD nLen;				// length of record in bytes
	BYTE bCommType;			// comm type, see above
	BYTE bUnknown;
	int nFreq;				// frequency * 1000000
} aptcomm_t;				// comm name string-z follows


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

// delete airport flags
// NOTE: BGLCOMP fails when deleteAllApronlights is
// included !!!
#define BIT_DELETE_ALL_APPROACHES		0x01
#define BIT_DELETE_ALL_APRONS			0x04
#define BIT_DELETE_ALL_FREQUENCIES		0x08
#define BIT_DELETE_ALL_HELIPADS			0x10
#define BIT_DELETE_ALL_RUNWAYS			0x20
#define BIT_DELETE_ALL_STARTS			0x40
#define BIT_DELETE_ALL_TAXIWAYS			0x80

// delete airport structure
// this entry is nested inside an airport and is not a
// subentry like runways, helipad, etc. its position is
// (when available) just before airport name header
// note: other delete structures (e.g. deleterunway etc)
// are not included in compilation when their bit is set
// e.g. you have a deleterunway and delete_all_runways
// bit is set, then delete runway is not compiled by
// bglcomp
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

// delete runway structure
typedef struct tag_delete_runway_t
{
	BYTE bSurface;			// runway surface
	BYTE bStartNumber;		// runway start number
	BYTE bEndNumber;		// runway end number
	BYTE bDesignator;		// designator (high part stores start, low part end)
} delete_runway_t;


#define RWYSTART_RUNWAY		0x01
#define RWYSTART_WATER		0x02
#define RWYSTART_HELIPAD	0x03

// delete start structure
typedef struct tag_delete_start_t
{
	BYTE bNumber;			// number
	BYTE bDesignator;		// runway start designator (left, right, etc.)
	BYTE bType;				// type of runway (runway, helipad, water)
	BYTE bPad;				// pad (0x0)
} delete_start_t;

// delete frequency structure
typedef struct tag_delete_freq_t
{
	int nPacked;			// packed frequency number and comm type
} delete_freq_t;


// approach lights flags
// NOTE: bglcomp fails when system is SSALSF or SSALSR !!!
#define AP_SYSTEM_NONE		0x00
#define AP_SYSTEM_ODALS		0x01
#define AP_SYSTEM_MALSF		0x02
#define AP_SYSTEM_MALSR		0x03
#define AP_SYSTEM_SSALF		0x04
#define AP_SYSTEM_SSALR		0x05
#define AP_SYSTEM_ALSF1		0x06
#define AP_SYSTEM_ALSF2		0x07
#define AP_SYSTEM_RAIL		0x08
#define AP_SYSTEM_CALVERT	0x09
#define AP_SYSTEM_CALVERT2	0x0A
#define AP_SYSTEM_SALS		0x0C
#define	AP_ENDLIGHTS		0x20
#define	AP_REIL				0x40
#define	AP_TOUCHDOWN		0x80

// approach lights structure
typedef struct tag_approachlights_t
{
	WORD wId;				// record ID (0x26)
	DWORD nLen;				// record length
	BYTE bFlags;			// approach flags, see above
	BYTE bStrobes;			// number of sequenced strobes
} approachlights_t;

#define VASI_VASI21			0x0001
#define VASI_VASI31			0x0002
#define VASI_VASI22			0x0003
#define VASI_VASI32			0x0004
#define VASI_VASI23			0x0005
#define VASI_VASI33			0x0006
#define VASI_PAPI2			0x0007
#define VASI_PAPI4			0x0008
#define VASI_TRICOLOR		0x0009
#define VASI_PVASI			0x000A
#define VASI_TVASI			0x000B
#define VASI_BALL			0x000C
#define VASI_APAP			0x000D
#define VASI_PANELS			0x000D

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

typedef	union
{
	BYTE bEnd;				// runway end stored in the high part and:
							// 0x0 is primary, 0x1 is secondary
	BYTE bNumber;			// runway number, 0-36
} un1;

typedef	union
{
	BYTE bType;				// runway type stored in the high part and:
							// 0x1 is runway, 0x2 is water 0x3 is helipad

	BYTE bFlags;			// runway type stored in the high part and:
							// 0x1 is runway, 0x2 is water 0x3 is helipad
							// low part stores the designator:
							// 0x0: none 0x1: left: 0x2: right 0x3: center
							// 0x4: water
} un2;

// runway start structure
typedef struct tag_rwystart_t
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


// NOTE: BGLCOMP fails if name type EPARKING
// (in bglcomp doc) coz is not valid,
// use E_PARKING instead
//
// taxiway parking constants
#define PARKING_NAME_NONE				0x00
#define PARKING_NAME_PARKING			0x01
#define PARKING_NAME_N_PARKING			0x02
#define PARKING_NAME_NE_PARKING			0x03
#define PARKING_NAME_E_PARKING			0x04
#define PARKING_NAME_SE_PARKING			0x05
#define PARKING_NAME_S_PARKING			0x06
#define PARKING_NAME_SW_PARKING			0x07
#define PARKING_NAME_W_PARKING			0x07
#define PARKING_NAME_NW_PARKING			0x09
#define PARKING_NAME_GATE				0x0A
#define PARKING_NAME_DOCK				0x0B
#define PARKING_NAME_GATE_A				0x0C
#define PARKING_NAME_GATE_B				0x0D
#define PARKING_NAME_GATE_C				0x0E
#define PARKING_NAME_GATE_D				0x0F
#define PARKING_NAME_GATE_E				0x10
#define PARKING_NAME_GATE_F				0x11
#define PARKING_NAME_GATE_G				0x12
#define PARKING_NAME_GATE_H				0x13
#define PARKING_NAME_GATE_I				0x13
#define PARKING_NAME_GATE_J				0x15
#define PARKING_NAME_GATE_K				0x16
#define PARKING_NAME_GATE_L				0x17
#define PARKING_NAME_GATE_M				0x18
#define PARKING_NAME_GATE_N				0x19
#define PARKING_NAME_GATE_O				0x1A
#define PARKING_NAME_GATE_P				0x1B
#define PARKING_NAME_GATE_Q				0x1C
#define PARKING_NAME_GATE_R				0x1D
#define PARKING_NAME_GATE_S				0x1E
#define PARKING_NAME_GATE_T				0x1F
#define PARKING_NAME_GATE_U				0x20
#define PARKING_NAME_GATE_V				0x21
#define PARKING_NAME_GATE_W				0x22
#define PARKING_NAME_GATE_X				0x23
#define PARKING_NAME_GATE_Y				0x24
#define PARKING_NAME_GATE_Z				0x25

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
typedef struct tag_taxiway_parking_header_t
{
	WORD wId;				// record ID (0x1B)
	DWORD nLen;				// record length with subchunks
	WORD wCount;			// number of parking structures following
} taxiway_parking_header_t;


typedef struct tag_taxiway_parking_t
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
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
} taxiway_parking_t;


// region structure
typedef struct tag_region_t
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
} region_t;

// icao id entry (20 bytes)
typedef struct tag_icaoid_t
{
	BYTE bRegionIndex;		// 00 - subscript index into regions array, stored in the low part
	BYTE bCountryIndex;		// 01 - subscript index into countries array, stored in the low part
	BYTE bStateIndex;		// 02 - subscript index into states array, stored in the high part
	BYTE bUnk2;				// 03 - unknown byte 2
	WORD wCitiesIndex;		// 04 - subscript index into city names array
	WORD wAirportIndex;		// 06 - subscript index into airport names array
	DWORD nId;				// 08 - packed ICAO ID
	BYTE lpUnk[8];			// 12 - unknown array
} icaoid_t;


// lat/lon structure
typedef struct tag_latlon_t
{
	int nLon;				// longitude in packed format
	int nLat;				// latitude in packed format
} latlon_t;


// apron entry
// composed by:
// - an initial apron_t structure (header)
//
// - a subsequent array of latlon_t whose size is
//   defined by the wVertexCount field (that means
//   an apron may contain up to 65536 vertex
// - a final closing element which is a 24 bit integer
//   which value is 0x000000
//
// after the above chunks an huge chunk whose id is 0x0030
// follows (one for each apron !!!), sometimes containing
// a massive list of lat/long pairs (???)
// the offset at 0x7 within this huge chunk stores the
// apron details_draw and surface_draw flags
#define APRON_SURFACE_DRAW	0x01
#define APRON_DETAILS_DRAW	0x02


typedef struct tag_apron_t
{
	WORD wId;				// 0000 - record ID (0x37)
	DWORD nLen;				// 0002 - record length
	BYTE bSurface;			// 0006 - apron surface (same as runways)
	WORD wVertexCount;		// 0007 - count of vertex in this apron
} apron_t;

// NOTE: BGLCOMP fails when apron vertex includes biasX
// and biasZ
// Solution: use lat/lon, instead


// approach flags
#define APPROACH_GPS_OVL					0x08

#define APPROACH_TYPE_GPS					0x01
#define APPROACH_TYPE_VOR					0x02
#define APPROACH_TYPE_NDB					0x03
#define APPROACH_TYPE_ILS					0x04
#define APPROACH_TYPE_LOCALIZER				0x05
#define APPROACH_TYPE_SDF					0x06
#define APPROACH_TYPE_LDA					0x07
#define APPROACH_TYPE_VORDME				0x08
#define APPROACH_TYPE_NDBDME				0x09
#define APPROACH_TYPE_RNAV					0x0A
#define APPROACH_TYPE_LOCALIZER_BACKCOURSE	0x0B

// approach entry (0x24)
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

// approach legs (0x2D)
// missed approach legs (0x2E)
// transition legs (0x2F)
typedef struct tag_approach_legs_t
{
	WORD wId;				// 0000 - record ID (0x2F 0x2D or 0x2E)
	DWORD nLen;				// 0002 - record length
	WORD wLegsCount;		// 0006 - count of legs in
} approach_legs_t;

// transition constants
#define TRANSITION_TYPE_FULL	0x01
#define TRANSITION_TYPE_DME		0x02

// transition (0x2C) - 20 bytes or 36 bytes
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

// leg constants
#define	FIX_TYPE_VOR					0x02
#define	FIX_TYPE_NDB					0x03
#define	FIX_TYPE_TERMINAL_NDB			0x04
#define	FIX_TYPE_WAYPOINT				0x05
#define	FIX_TYPE_TERMINAL_WAYPOINT		0x06
#define	FIX_TYPE_LOCALIZER				0x08
#define	FIX_TYPE_RUNWAY					0x09

#define LEG_TYPE_AF						0x01
#define LEG_TYPE_CA						0x02
#define LEG_TYPE_CD						0x03
#define LEG_TYPE_CF						0x04
#define LEG_TYPE_CI						0x05
#define LEG_TYPE_CR						0x06
#define LEG_TYPE_DF						0x07
#define LEG_TYPE_FA						0x08
#define LEG_TYPE_FC						0x09
#define LEG_TYPE_FD						0x0A
#define LEG_TYPE_FM						0x0B
#define LEG_TYPE_HA						0x0C
#define LEG_TYPE_HF						0x0D
#define LEG_TYPE_HM						0x0E
#define LEG_TYPE_IF						0x0F
#define LEG_TYPE_PI						0x10
#define LEG_TYPE_RF						0x11
#define LEG_TYPE_TF						0x12
#define LEG_TYPE_VA						0x13
#define LEG_TYPE_VD						0x14
#define LEG_TYPE_VI						0x15
#define LEG_TYPE_VM						0x16
#define LEG_TYPE_VR						0x17

#define LEG_TRUE_COURSE_VALID			0x01
#define LEG_TIME_VALID					0x02
#define LEG_FLY_OVER					0x04

#define LEG_TURN_DIRECTION_L			0x01
#define LEG_TURN_DIRECTION_R			0x02
#define LEG_TURN_DIRECTION_E			0x03

#define LEG_ALT_DESCRIPTOR_A			0x01
#define LEG_ALT_DESCRIPTOR_PLUS			0x02
#define LEG_ALT_DESCRIPTOR_MINUS		0x03
#define LEG_ALT_DESCRIPTOR_B			0x04
#define LEG_ALT_DESCRIPTOR_C			0x02
#define LEG_ALT_DESCRIPTOR_G			0x01
#define LEG_ALT_DESCRIPTOR_H			0x02
#define LEG_ALT_DESCRIPTOR_J			0x02
#define LEG_ALT_DESCRIPTOR_V			0x02


// leg entry (44 bytes)
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

// taxiway point entry structure
#define TWP_NORMAL			0x00000001
#define TWP_HOLD_SHORT		0x00000002

typedef struct tag_twp_entry_t
{
	BYTE bType;				// 0000 - type, normal or hold_short
	BYTE bOrientation;		// 0001 - 0: forward, 1: reverse
	WORD wUnk1;				// 0002 - unknown word 1
	int nLon;				// 0004 - longitude in packed format
	int nLat;				// 0008 - latitude in packed format
} twp_entry_t;

// taxiway point structure (0x1A)
//
// composed by:
// - an 8 byte initial twp_t header
// - an attached array of twp_entry_t whose size is
//   delimited by the wSize field of the twp_t header
typedef struct tag_twp_t
{
	WORD wId;				// 0000 - record ID (0x1A)
	DWORD nLen;				// 0002 - record length
	WORD wSize;				// 0006 - size of point array
} twp_t;

// taxiname point structure (0x1D)
//
// composed by:
// - an 8 byte initial taxiname_t header
// - an attached array of 8 chars names (not null terminated)
//   delimited by the wSize field of the twp_t header
typedef struct tag_taxiname_t
{
	WORD wId;				// 0000 - record ID (0x1D)
	DWORD nLen;				// 0002 - record length
	WORD wNamesCount;		// 0006 - count of taxi names here under
} taxiname_t;

// taxiway path flags
#define TPATH_TYPE_TAXIWAY				0x01
#define TPATH_TYPE_TAXI					TPATH_TYPE_TAXIWAY
#define TPATH_TYPE_RUNWAY				0x02
#define TPATH_TYPE_PARKING				0x03
#define TPATH_TYPE_PATH					0x04
#define TPATH_TYPE_CLOSED				0x05

// NOTE: TPATH_DRAW_SURFACE and TPATH_DRAW_DETAIL
// bits are stripped when taxi type is TPATH_TYPE_PATH

#define TPATH_DRAW_SURFACE				0x20
#define TPATH_DRAW_DETAIL				0x40

#define TPATH_CENTERLINE				0x01
#define TPATH_CENTERLINE_LIGHTED		0x02
#define TPATH_LEFTEDGE_SOLID			0x04
#define TPATH_LEFTEDGE_DASHED			0x08
#define TPATH_LEFTEDGE_LIGHTED			0x10
#define TPATH_LEFTEDGE_SOLID_DASHED		0x0C
#define TPATH_RIGHTEDGE_SOLID			0x20
#define TPATH_RIGHTEDGE_DASHED			0x40
#define TPATH_RIGHTEDGE_SOLID_DASHED	0x60
#define TPATH_RIGHTEDGE_LIGHTED			0x80


// taxiway path structure (0x1C)
//
// composed by:
// - an initial 8 bytes header
// - an attached array of taxiway_path_t structures
//   whose size is defined by header's wCount field
typedef struct tag_taxipath_hdr_t
{
	WORD wId;				// 0000 - record ID (0x1C)
	DWORD nLen;				// 0002 - record length
	WORD wCount;			// 0006 - count of paths
} taxipath_hdr_t;

typedef struct tag_taxiway_path_t
{
	WORD wStart;			// 0000 - start index
	WORD wEnd;				// 0002 - end index
							//        high part of the high byte also
							//        stores runway designator
	BYTE bDrawFlags;		// 0004 - combination of draw flags and path types,
							//        see constant above
	BYTE bNumber;			// 0005 - runway number
	BYTE bCenterFlags;		// 0006 - centerline flags, see constants above
	BYTE bSurface;			// 0007 - runway surface
	float fWidth;			// 0008 - width in meters
	int nWeigthLimit;		// 0012 - weigth limit
	int nUnk;				// 0016 - unknown integer - terminator
} taxiway_path_t;



// apron edge lights entry (0x31)
// composed by:
// - an initial apron_edgelights_t structure (header)
//
// - a subsequent array of latlon_t whose size is
//   defined by the wVertexCount field (that means
//   an apronedgelights entry may contain up to
//   65536 vertex
typedef struct tag_apron_edgelights_t
{
	WORD wId;				// 0000 - record ID (0x31)
	DWORD nLen;				// 0002 - record length
	WORD wUnk1;				// 0006 - unknown word 1
	WORD wVertexCount;		// 0008 - number of edge light vertex
	BYTE lpUnk1[14];		// 0010 - unknown 14 bytes chunk ?
} apron_edgelights_t;


// Geopol info
#define GEOPOL_COASTLINE	0x80
#define GEOPOL_BOUNDARY		0x40

// geopol structure (0x23)
//
// composed by:
// - an initial 24 bytes long header (geopol_t)
// - an array on lon/lat whose size is defined by the
//   header's field at offset 7
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

// center frequency structure (0x12 found in bvcf.bgl)
// this is nested inside a boundary header

typedef struct tag_center_freq_t
{
	WORD wId;				// 0000 - record ID (0x12)
	DWORD nLen;				// 0002 - record length 
	WORD wUnk1;				// 0006 - unknown word, always 0xA ?
	int nFreq;				// 0008 - frequency, Mhz * 1000000
} center_freq_t;
// after the above an array of char which contains a descriptive
// name for center follows - length of bytes to transfer is obtained
// by: center_freq_t.nLen - sizeof(center_freq_t)


