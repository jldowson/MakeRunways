/* MakeRwys.h
*******************************************************************************/

//#define HEX 1
#define PI 3.14159265358979

#define _CRT_SECURE_NO_DEPRECATE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#include "resource.h"
#include "NewAFD.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine,
      int cmdShow);

/******************************************************************************
         Structs
******************************************************************************/

typedef struct _MY_WIN32_FIND_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR   cFileName[ MAX_PATH ];
    CHAR   cAlternateFileName[ 14 ];
	WORD unknown; // Added by PLD to avoid problems!
} MY_WIN32_FIND_DATA;

typedef struct _bglhdr
{	unsigned short id;
	unsigned long maxN;
	unsigned long minN;
	unsigned long maxE;
	unsigned long minE;
	unsigned long voroffset;
	unsigned short minfreq;
	unsigned short maxfreq;
	unsigned long assortedoffs[8];
	unsigned long procoffset;
	unsigned long sn10offset;
	unsigned long menuoffset;
	unsigned long sn12offset;
	unsigned long atisoffset;
	unsigned long ndboffset;
	unsigned long dynoffset;
	unsigned long libraryidmin1;
	unsigned long libraryidmin2;
	unsigned long libraryidmax1;
	unsigned long libraryidmax2;
	unsigned long miscoffset;
	unsigned long titloffset;
	unsigned long magvoffset;
	unsigned long excloffset;
	unsigned long spare1; // Magic # = 0x87654321
	unsigned long spare2; // = 0
	unsigned short spare3; // = 0
	unsigned char guid128[16]; // Unique GUID!!
	unsigned long version; // = 1
	unsigned long spare4; // = 0
	unsigned long dataptrsctr; // >= 3 or else (3 normally
	unsigned long namoffset; // offset of NAME_LIST
	unsigned long spare5; // = 0
	unsigned long objoffset; // offset of OBJECTS (but use ptrs in NAME_LIST)
} BGLHDR;

#define NAME_LIST (0x30001)
#define NAME_ENTRY (0x30002)

typedef struct
{	unsigned long type;		// 0x30001 = NAME_LIST, 0 = end, others?
	unsigned long len;		// length of remainder of NAME_LIST after this
	unsigned long language; // 0x409 = US English
	unsigned long charset;	// 0 = STRINGZ_ASCII
	unsigned long spare;		// 0
	unsigned long n;			// number of NAME_ENTRY's
} NAMELIST;

#define FACILITY_REGION 1
#define FACILITY_COUNTRY 2
#define FACILITY_STATE 3
#define FACILITY_CITY 4
#define FACILITY_AIRPORT 5
#define ST_AIRPORT 1
#define ST_UNKNOWN 2
#define ST_DMETACAN 3
#define ST_VORNDB 4

typedef struct
{	unsigned long type;		// 0x30002 = NAME_ENTRY, 0 = end, others?
	unsigned long len;		// length of remainder of NAME_ENTRY after this
	unsigned short st;			// subtype: see above	
	unsigned short t;			// FACILITY_ type: see above	
	unsigned long offs;		// OBJECT_CONTAINER file offset
	char descr[256];			// name, ASCIIZ, padded to mult of 4 bytes
} NAMEENTRY;

typedef struct
{	unsigned short f;
	long i;
} IF48;

typedef struct
{	unsigned long f;
	long i;
} IF64;

typedef struct
{	unsigned long type;	// OBJECT_CONTAINER == 1?
	unsigned long len;	// length of rest
	unsigned long t;
	unsigned long reserved;
} OBJECT;

#define LOC_AP		0x10001
#define LOC_RWY	0x10002
#define LOC_NAV	0x10004
#define LOC_SET 	0x10005
#define LOC_ICAO	0x10006
#define LOC_FREQ	0x20001

typedef union
{	struct	// LOC_FREQ
	{	unsigned long freq;
		unsigned long comt;
	};

	unsigned char icao[8]; // LOC_ICAO

	struct	// all other LOCs
	{	IF64 lat;
		IF64 lon;
		unsigned short sparey;
		int elev;
		IF48 hdg;

		union
		{	struct	// LOC_RWY
			{	unsigned long magvar;
				unsigned long rwylen;
				unsigned long rwywid;
				unsigned long rwyt;
				unsigned long rwyn;
				unsigned long rwysurf;
				unsigned long rwysurfNew;
				unsigned long spare5;
			};

			struct	// LOC_SET
			{	unsigned long sett; // = 1 for SET_TAKEOFF
				unsigned long setst; // as rwyt for SET_TAKEOFF
				unsigned long rwysn;
				unsigned long spare3;
			};

			struct	// LOC_NAV
			{	unsigned int nfreq;
				unsigned int ntype;
				unsigned int nclass;
				unsigned int nflags;
				unsigned int spare4;
				char id[8]; // Nav id
			};
		};
	};
} LOCATION;

typedef struct
{	float fangle;
	short degrees;
	short minutes;
	short fracmins;
	short seconds;
	short tenthou;
	char orient; // N E S W
} ANGLE;

typedef struct
{	char chICAO[4];			// 0
	char chRwy[4];			// 4
	float fLat;				// 8
	float fLong;			// 12
	float fAlt;				// 16
	unsigned short uLen;	// 20
	char chSurf;			// 22
	char chSurfNew;			// 23
	unsigned short uHdg;	// 24
	char chILS[8];			// 26
	char chILSHdg[6];		// 34
	WORD uWid;				// 40
	float fFSLat;			// 42
	float fFSLong;			// 46
	char chILSid[8];		// 50
	float fILSslope;		// 58
	BYTE bLights;			// 62
	BYTE bPatternFlags;		// 63							
	float fPatternAlt;		// 64
	BYTE fPrimary;			// 68
	BYTE bVASIleft;			// 69
	BYTE bVASIright;		// 70
	BYTE bAppLights;		// 71
	BYTE nStrobes;			// 72
	char chNameILS[32];		// 73
	BYTE bSpare[3];			// 105
	float fLeftBiasX;		// 108
	float fLeftBiasZ;		// 112
	float fLeftSpacing;		// 116
	float fLeftPitch;		// 120
	float fRightBiasX;		// 124
	float fRightBiasZ;		// 128
	float fRightSpacing;	// 132
	float fRightPitch;		// 136
} RWYDATA;					// 108

typedef struct
{	char chICAO[4];
	float fLat;
	float fLong;
	int nAlt;
	unsigned short Atis;
} APDATA;

typedef struct 
{	char chName[8];
	float fMinWid;
	float fMaxWid;
	WORD wPoints; // == 0 to terminate
} TWHDR;

typedef struct
{	float fLat;
	float fLon;
	float fWid;
	BYTE bType;
	BYTE bPtype;
	BYTE bOrientation;
	BYTE spare;
} TW;

typedef struct _rwylist
{	struct _rwylist *pTo;
	struct _rwylist *pFrom;
	RWYDATA r;
	signed char fDelete;
	BYTE fAirport;
	unsigned short Atis;
	float fMagvar;
	float fHdg;
	unsigned long nOffThresh;
	float fLat;
	float fLong;
	NGATEHDR *pGateList;
	BYTE fCTO;
	BYTE fCL;
	BYTE fILSflags;
	BYTE bSpare;
	TWHDR *pTaxiwayList;
	char *pCountryName;
	char *pStateName;
	char *pCityName;
	char *pAirportName;
	char *pPathName;
	char *pSceneryName;
} RWYLIST;

typedef struct
{	unsigned long hdg;
	unsigned short rwylen;
	char rwysurf;
	char rwysurfNew;
	unsigned long rwyt;
	unsigned long rwyn;
	char chfreq[8];
	char chILSHdg[8];
	unsigned long rwywid;
	float fMagvar;
	float fHdg;
	unsigned long nOffThresh;
	float fLat;
	float fLong;
} RWY;

typedef struct
{	char  chICAO[4];		// 0
	float fLat;				// 4
	float fLong;			// 8
	float fAlt;				// 12
	float fHeading;			// 16
	unsigned short sLen;	// 20
	unsigned short sWidth;	// 22
	BYTE  bSurface;			// 24
	BYTE  bFlags;			// 25
	BYTE  bSpare;			// 26
	BYTE  fDelete;			// 27
	// 28
} HELI;

extern BOOL fMSFS;
extern HELI helipads[10000];
extern int nHelipadCtr;
extern BOOL fDebug;
extern int errnum;
extern int nComms;

extern FILE *fpAFDS;
extern RWYLIST *pR, *pRlast;
extern char chLine[];
extern void DecodeRwy(unsigned long n, unsigned long c, char *pch, int offs, int size);
extern void CheckNewBGL(FILE *fpIn, NBGLHDR *ph, DWORD size);
extern void WritePosition(LOCATION *ploc, BOOL fWithAlt);
extern void ToAngle(ANGLE *p, long i, unsigned long f, short type);
extern void ProcessRunwayList(RWYLIST *pL, BOOL fAdd, BOOL fNoCtr);
extern void SetLocPos(LOCATION *pLoc, int alt, int lat, int lon, float *pflat, float *pflon, double *pdLat, double *pdLon);
extern float ToFeet(long alt);
extern char *str2ascii(char *psz);

extern HWND hWnd;
extern BOOL fUserAbort, fNewAirport;
extern BOOL fNoDrawHoldConvert;
extern int fFS9;					
extern DWORD ulTotalAPs, ulTotalRwys;
extern char *pNextAirportName, *pNextCityName, *pNextStateName, *pNextCountryName, *pNextPathName;

extern void DeleteComms(char *pchICAO);
extern void AddComms(char *pchICAO, int nCommstart, int nCommEnd, int nCommDelStart, int nCommDelEnd, char *pApName);
extern void MakeCommsFile(void);
extern void MakeHelipadsFile(void);

extern const char *szNRwySurf[];
extern char* pLocPak;

/******************************************************************************
         End of MakeRwys.h
******************************************************************************/
