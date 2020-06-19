/* NewBGLs.h
*******************************************************************************/

/******************************************************************************
         Structs
******************************************************************************/

typedef struct _nbglhdr
{	WORD id; // == 0x0201
	WORD vers; // == 0x1992
	DWORD size; // == 0x0034 (52)
	DWORD unk[3];
	DWORD nsects; // Number of sect ptrs in hdr
	DWORD unk2[(52-24)/4];
} NBGLHDR;

typedef struct _nsects
{	DWORD type; // see below
	DWORD unk1;
	DWORD subsects; // number of subsect ptrs
	DWORD offset; // file offset to section hdr
	DWORD size; // size of section hdr
} NSECTS;

// Section types
#define SECT_APTS	0x03 //airports ****
#define SECT_VORI	0x13 //VOR and ILS
#define SECT_NDBS	0x17
#define SECT_MKRS	0x18
#define SECT_BNDY	0x20 //boundary
#define SECT_WYPT	0x22
#define SECT_GEOP	0x23 //geopol
#define SECT_OBJS	0x25
#define SECT_NAME	0x27 //namelist ****
#define SECT_MDLD	0x2D
#define SECT_APT2	0x2C //more airport data ****
#define SECT_EXCL	0x2E

// Section header -- "subsects" above give number of copies of this:
// NOT APPLIC TO BNDY and GEOP types
typedef struct _nssects
{	DWORD id; // ???
	DWORD nrecs; // No. of records in subsection
	DWORD offset; // file offset to subsection records
	DWORD size; // size of subsection
} NSSECTS;

// airport record
// One of these followed by the subrecords as numbered here (<<)
typedef struct _napt
{	WORD id; // = 0x03
	DWORD size;
	
	BYTE nrwys; // number of runways subrecords **** <<
	BYTE ncoms; // number of comms subrecords <<
	BYTE nstts; // number of starts subrecords <<
	BYTE napps; // number of approaches subrecords <<
	BYTE naprs; // 0-6 no. of aprons subrecords. 2^7 = deleteAirport flag **** <<
	BYTE nhels; // number of helipads subrecords <<

	DWORD lon; // double dLon = (lon * (360.0 / 0x30000000)) - 180.0;
	DWORD lat; // double lat = 90.0 - (lat * (180.0 / 0x20000000));
	DWORD alt; // in millimetres
	
	DWORD tlon; // tower lon
	DWORD tlat; // tower lat
	DWORD talt; // tower alt
	
	float magvar;
	
	DWORD ICAO; // chars reduced to 0-37 then converted base 38 (!)
	
	DWORD unk[2];
} NAPT;

// Above normally followed by any one/number of records.
// All START with WORD id and DWORD size.

// We're interested only in:

// Airport name
typedef struct _nname
{	WORD id; // = 0x19
	DWORD size; // record size
	char name[1]; // Name starts here ...
} NNAME;

// Runway
typedef struct _nrwy
{	WORD id; // = 0x04
	DWORD size; // of record
	WORD surf;
	BYTE num1; // main runway number (1-36, 37=N, etc)
	BYTE des1; // desig (0, 1 L, 2 R, 3 C, 4 Water? Helipad I reckon)
	BYTE num2; // Secondary rwy number
	BYTE des2; // and desig
	DWORD ils1id; // ICAO format Ident or 0
	DWORD ils2id;
	DWORD lon;
	DWORD lat;
	DWORD alt;
	float len; // metres
	float wid; // metres
	float hdg;
	float patalt; // Pattern altitude
	WORD markgs;
	BYTE lights;
	BYTE pattns;
} NRWY;

// Start
typedef struct _nstart
{	WORD id; // = 0x11
	DWORD size; // 24
	BYTE num; // Runway number, as above
	BYTE des; // bits 0-3 desig as above, bits 4-7: 1=Runway, 2=Water, 3=Helipad
	DWORD lon;
	DWORD lat;
	DWORD alt;
	float hdg;
} NSTART;

// ILS/VOR records:
typedef struct _nilsvor
{	WORD id; // = 0x13
	DWORD size; 
	BYTE type; // ILS = 4
	BYTE flags;
	DWORD lon;
	DWORD lat;
	DWORD alt;
	DWORD freq;
	float range; // metres
	float magvar;
	DWORD ident;	// in special format -- tie this to runway with 
					// ils1id or ils2id in runway record
	DWORD regapt; // bits 0-10 region id, 11-31 airport id (ILS only)
} NILSVOR;

/******************************************************************************
         End of NewBGLs.h
******************************************************************************/
