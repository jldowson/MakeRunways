/******************************************************************************* 
NewBGLs.c
*******************************************************************************/

#include "MakeRwys.h"
#include <ctype.h>

extern char chRwyT[6];
extern char szParkNames[12][5];
extern char *pszParkType[];
extern char szCurrentFilePath[MAX_PATH];
__int64 *pLastSetGateList = 0;
__int32 fDeletionsPass = 0, nMinRunwayLen = 0; //1500
BYTE *pOffsetBase = NULL;

static WORD ReadBGLWordLE(const BYTE *p)
{
    WORD value;

    memcpy(&value, p, sizeof(value));
    return value;
}

static DWORD ReadBGLDwordLE(const BYTE *p)
{
    DWORD value;

    memcpy(&value, p, sizeof(value));
    return value;
}

static BOOL IsValidBGLFileRange(DWORD offset, DWORD length, DWORD fileSize)
{
    return (offset <= fileSize) && (length <= (fileSize - offset));
}

static BOOL IsMSFS2024BGLV9(const NBGLHDR *ph, DWORD fileSize)
{
    const BGLV9_FILE_HEADER *header =
        (const BGLV9_FILE_HEADER *)ph;

    if (!ph || fileSize < sizeof(BGLV9_FILE_HEADER))
        return FALSE;

    return
        header->magic == BGLV9_MAGIC &&
        header->headerSize == sizeof(BGLV9_FILE_HEADER);
}

static __int32 FileOffset32(const void *p)
{
	if (!pOffsetBase || !p)
		return 0;

	return (__int32)((const BYTE *)p - pOffsetBase);
}


BOOL fIncludeWater = FALSE, fMarkJetways = FALSE, fDebugThisEntry = FALSE;;
BOOL fNoDrawHoldConvert = TRUE; // ### 4900

RWYLIST *prwyPrevious = 0;

/******************************************************************************
         Data
******************************************************************************/

char *pszParkType[] =
				{	"", "Parking", "N Park", "NE Park", "E Park", "SE Park",
					"S Park", "SW Park", "W Park", "NW Park", "Gate", "Dock" };
char *pszGateType[] =
				{	"?", "GA Ramp", "GA Ramp Small", "GA Ramp Medium", "GA Ramp Large",
					"Cargo Ramp", "Mil Cargo Ramp", "Mil Combat Ramp",
					"Small Gate", "Medium Gate", "Heavy Gate", "GA Dock",
					"Fuel", "Vehicles", "GA Ramp Extra", "Gate Extra" };
				

// runway surfaces
const char *szNRwySurf[] =
{
	"Concrete", // 0
	"Grass",	// 1
	"Water",	// 2
	"Unknown3", // 3
	"Asphalt",  // 4
	"Unknown5", // 5
	"Unknown6", // 6
	"Clay",		// 7
	"Snow",		// 8
	"Ice",		// 9
	"Unknown10",// 10
	"Unknown11",// 11
	"Dirt",		// 12
	"Coral",	// 13
	"Gravel",	// 14
	"Oil-treated",// 15
	"Mats",     // 16
	"Bituminous",// 17
	"Brick",	// 18
	"Macadam",	// 19
	"Planks",	// 20
	"Sand",		// 21
	"Shale",	// 22
	"Tarmac",	// 23
	"Unknown24", // 24 fallback bucket for unmapped/unknown surface
};

char *pszComms[] = {
					"", "ATIS", "MULTICOM", "UNICOM", "CTAF", "GROUND", "TOWER",
						"CLEARANCE", "APPROACH", "DEPARTURE", "CENTRE", "FSS", "AWOS",
						"ASOS", "CLR PRETAXI", "REM CLR DELIV", "???" };

// Conversion FS2004 to FS2002 surface codes
char chOldSurf[25] = {
	2, 4, 10, 4, 3, 4, 4, 1, 8, 8, 2, 2,
	1, 9, 5, 6, 7, 3, 11, 3, 12, 5, 5, 3, 0 }; // 11, 12 as 0 old, but okay for XML

/******************************************************************************
        MSFS2024 runway surface mapping
******************************************************************************/

static WORD GetMSFS2024RunwaySurfaceLegacyIndex(const char *pszSurfaceName)
{
	__int32 x;

	if (!pszSurfaceName || !pszSurfaceName[0])
		return 24;

	// First keep compatibility with existing legacy MakeRunways names.
	for (x = 0; x < 25; x++)
	{
		if (_stricmp(szNRwySurf[x], pszSurfaceName) == 0)
			return (WORD)x;
	}

	/*
		MSFS2024 SDK surface aliases.

		Important note:
		The MSFS2024 Material Inspector shows BITUMINOUS and material name
		Runway_Bituminous. Some SDK/docs/community references show BITUMINUS.
		We accept BITUMINUS as compatibility alias, but use Bituminous /
		BITUMINOUS as the canonical readable form.
	*/

	if (_stricmp(pszSurfaceName, "UNKNOWN") == 0) return 24;

	if (_stricmp(pszSurfaceName, "CONCRETE") == 0) return 0;
	if (_stricmp(pszSurfaceName, "CEMENT") == 0) return 0;

	if (_stricmp(pszSurfaceName, "GRASS") == 0) return 1;
	if (_stricmp(pszSurfaceName, "GRASS_BUMPY") == 0) return 1;
	if (_stricmp(pszSurfaceName, "SHORT_GRASS") == 0) return 1;
	if (_stricmp(pszSurfaceName, "LONG_GRASS") == 0) return 1;
	if (_stricmp(pszSurfaceName, "HARD_TURF") == 0) return 1;
	if (_stricmp(pszSurfaceName, "TURF") == 0) return 1;

	if (_stricmp(pszSurfaceName, "WATER") == 0) return 2;
	if (_stricmp(pszSurfaceName, "WATER_FSX") == 0) return 2;
	if (_stricmp(pszSurfaceName, "OCEAN") == 0) return 2;
	if (_stricmp(pszSurfaceName, "POND") == 0) return 2;
	if (_stricmp(pszSurfaceName, "LAKE") == 0) return 2;
	if (_stricmp(pszSurfaceName, "RIVER") == 0) return 2;
	if (_stricmp(pszSurfaceName, "WASTE_WATER") == 0) return 2;

	if (_stricmp(pszSurfaceName, "ASPHALT") == 0) return 4;
	if (_stricmp(pszSurfaceName, "CLAY") == 0) return 7;
	if (_stricmp(pszSurfaceName, "SNOW") == 0) return 8;
	if (_stricmp(pszSurfaceName, "ICE") == 0) return 9;
	if (_stricmp(pszSurfaceName, "DIRT") == 0) return 12;
	if (_stricmp(pszSurfaceName, "CORAL") == 0) return 13;
	if (_stricmp(pszSurfaceName, "GRAVEL") == 0) return 14;
	if (_stricmp(pszSurfaceName, "OIL_TREATED") == 0) return 15;
	if (_stricmp(pszSurfaceName, "STEEL_MATS") == 0) return 16;
	if (_stricmp(pszSurfaceName, "STEEL") == 0) return 16;

	if (_stricmp(pszSurfaceName, "BITUMINOUS") == 0) return 17;
	if (_stricmp(pszSurfaceName, "BITUMINUS") == 0) return 17;

	if (_stricmp(pszSurfaceName, "BRICK") == 0) return 18;
	if (_stricmp(pszSurfaceName, "MACADAM") == 0) return 19;
	if (_stricmp(pszSurfaceName, "PLANKS") == 0) return 20;
	if (_stricmp(pszSurfaceName, "SAND") == 0) return 21;
	if (_stricmp(pszSurfaceName, "SHALE") == 0) return 22;
	if (_stricmp(pszSurfaceName, "TARMAC") == 0) return 23;

	return 24;
}


/******************************************************************************
         Assorted conversions
******************************************************************************/

static __int64 fslat2lat(__int32 ndblat, float *pf, double *pd)
{	double f = (((double)ndblat)*180)/MAXSOUTH;
	if (f>90.0)	f = -((f-180)+90);
	else f = 90-f;
	if (pd) *pd = f;
	if (pf) *pf = (float) f;
	return (__int64) ((f * 10001750.0) / 90.0);
}

static __int64 fslon2lon(__int32 ndblon, float *pf, double *pd)
{	double f = (((double)ndblon)*360)/MAXEAST;
	f -= 180;
	if (pd) *pd = f;
	if (pf) *pf = (float) f;
	return (__int64) ((f * 65536.0 * 65536.0) / 360.0);
}

static void str_rev(char *string)
{	DWORD i, j;
	DWORD length = strlen(string);	/* Number of characters in string	*/
	char temp;
	
	for (i = 0, j = length-1;		/* Counting from front and rear		*/
		i < length / 2; i++, j--) 
	{								/* until we reach the middle		*/
		temp = string[i];			/* Save front character				*/
		string[i] = string[j];		/* Switch with rear character		*/
		string[j] = temp;			/* Copy new rear character			*/
	}
}

static void DecodeID(DWORD number, char *p, BOOL fShift)
{	char r, c;
	char *p1 = p;
	DWORD q = fShift ? (number >> 5) : number;
	
	for (; q>0; q/=38)
	{	r = (char)(q % 38);
		c = (r<12) ? r+46 : r+53;
		*p++ = c;
	}
	*p = 0;
	

	if (!*p1)
	{  	*p1='0';
		*++p1= 0;
	}
	
	else
	    str_rev(p1);
}

/******************************************************************************
         MakeHelipadsFile
******************************************************************************/

HELI helipads[10000];
__int32 nHelipadCtr = 0;

__int32 hcompare(const void *arg1, const void *arg2)
{	HELI *h1 = (HELI *) arg1;
	HELI *h2 = (HELI *) arg2;

	return _stricmp(h1->chICAO, h2->chICAO);
}

void MakeHelipadsFile(void)
{	FILE *phf = fopen("helipads.csv", "wb");
	__int32 i = 0;

	if (phf)
	{	if (nHelipadCtr > 1)
			// First sort array into ICAO order
			qsort((void *) &helipads[0], (size_t) nHelipadCtr,
				sizeof(HELI), hcompare);

		while (i < nHelipadCtr)
		{	if (helipads[i].fDelete == 0)
				fprintf(phf, "%s,%.6f,%.6f,%.0f,%.1f,%d,%d,%d,%d\n",
					helipads[i].chICAO, helipads[i].fLat, helipads[i].fLong,
					helipads[i].fAlt, helipads[i].fHeading,
					helipads[i].sLen, helipads[i].sWidth,
					helipads[i].bSurface, helipads[i].bFlags);
			i++;
		}

		fclose(phf);
	}
}

void DeleteHelipads(char *pchICAO)
{	__int32 i = 0;
	while (i < nHelipadCtr)
	{	if (_stricmp(helipads[i].chICAO, pchICAO) == 0)
			helipads[i].fDelete = 255;
		i++;
	}
}

/******************************************************************************
        DoHelipadOnly
******************************************************************************/

void DoHelipadOnly(helipad_t* ph, char *pszICAO)
{	static char *pszFlags[5] = { "NONE", "H", "SQUARE", "CIRCLE", "MEDICAL" };
	static char szPrevICAO[9] = "";
	char szICAOText[9];
	char chWork[64];
	double dLat, dLon;
	HELI *phs = &helipads[nHelipadCtr++];

	memset(szICAOText, 0, sizeof(szICAOText));
	strncpy_s(szICAOText, sizeof(szICAOText), pszICAO ? pszICAO : "", _TRUNCATE);
		
	memset(phs->chICAO, 0, sizeof(phs->chICAO));
	strncpy_s(phs->chICAO, sizeof(phs->chICAO), szICAOText, _TRUNCATE);
	phs->bFlags = ph->bFlags;
	phs->bSurface = ph->bSurface;
	phs->fAlt = ((float) ph->nAlt)*3.28084F/1000;
	phs->fHeading = ph->fHeading;
	phs->sLen = (unsigned short) ((ph->fLength * 3.28084) + 0.5);
	phs->sWidth = (unsigned short) ((ph->fWidth * 3.28084) + 0.5);
	fslat2lat(ph->nLat, &phs->fLat, &dLat);
	fslon2lon(ph->nLon, &phs->fLong, &dLon);
	phs->fDelete = 0;
	sprintf(chWork, "UNKNOWN %d", ph->bSurface);			
	
	if (_stricmp(szICAOText, szPrevICAO) != 0)
		fprintf(fpAFDS, "\n");
	strcpy_s(szPrevICAO, sizeof(szPrevICAO), szICAOText);
	{
		BYTE bHelipadType = (BYTE)(ph->bFlags & 0x0f);
		const char *pszHelipadType = (bHelipadType < 5) ? pszFlags[bHelipadType] : "UNKNOWN";

		fprintf(fpAFDS, "          HELIPAD at %s: %s %s %s",
			szICAOText, pszHelipadType,
			ph->bFlags & 0x10 ? "transparent" : "",
			ph->bFlags & 0x20 ? "closed" : "");
	}

	fprintf(fpAFDS, "\n              Surface=%s, Lat=%.6f, Lon=%.6f, Alt=%.0fft",
		ph->bSurface > 24 ? chWork : szNRwySurf[ph->bSurface],
		dLat, dLon, (double) phs->fAlt);
	fprintf(fpAFDS, "\n              Length=%dft, Width=%dft, Heading=%.1fT\n\n",
		phs->sLen, phs->sWidth, (double) ph->fHeading);
}

/******************************************************************************
        DoHelipadFromMSFSTaxiPark
******************************************************************************/

void DoHelipadFromMSFSTaxiPark(NGATE4 *pg4, char *pszICAO, float fAirportAltFt)
{
	static char szPrevICAO[9] = "";
	char szICAOText[9];
	double dLat, dLon;
	HELI *phs;

	if (!pg4 || !pszICAO || !pszICAO[0])
		return;

	memset(szICAOText, 0, sizeof(szICAOText));
	strncpy_s(szICAOText, sizeof(szICAOText), pszICAO, _TRUNCATE);

	if (nHelipadCtr >= 10000)
	{
		fprintf(fpAFDS, "          WARNING: helipads.csv limit reached, MSFS2024 helicopter stand ignored for %s\n", pszICAO);
		return;
	}

	phs = &helipads[nHelipadCtr++];
	memset(phs, 0, sizeof(HELI));

	/*
		HELI is still a legacy internal structure with 4-char ICAO.
		For this first patch we keep the legacy output format unchanged.
	*/
	memset(phs->chICAO, 0, sizeof(phs->chICAO));
	strncpy_s(phs->chICAO, sizeof(phs->chICAO), szICAOText, _TRUNCATE);

	phs->bFlags = HELITYPE_H;
	phs->bSurface = 0;	// Concrete fallback
	phs->fAlt = fAirportAltFt;
	phs->fHeading = pg4->fHeading;

	/*
		TaxiwayParking stores radius in meters.
		helipads.csv expects length/width in feet.
		Use diameter = radius * 2.
	*/
	phs->sLen = (unsigned short)(((pg4->fRadius * 2.0F) * 3.28084F) + 0.5F);
	phs->sWidth = phs->sLen;

	fslat2lat(pg4->nLat, &phs->fLat, &dLat);
	fslon2lon(pg4->nLon, &phs->fLong, &dLon);

	phs->fDelete = 0;

	if (_stricmp(szICAOText, szPrevICAO) != 0)
		fprintf(fpAFDS, "\n");

	strcpy_s(szPrevICAO, sizeof(szPrevICAO), szICAOText);

	fprintf(fpAFDS,
		"          MSFS2024 HELISTAND from TaxiwayParking at %s: radius=%.1fm, heading=%.1fT\n",
		szICAOText, (double)pg4->fRadius, (double)pg4->fHeading);

	fprintf(fpAFDS,
		"              Lat=%.6f, Lon=%.6f, Alt=%.0fft, Length=%dft, Width=%dft\n\n",
		dLat, dLon, (double)phs->fAlt, phs->sLen, phs->sWidth);
}

/******************************************************************************
         PrintRWSLIST
******************************************************************************/

void PrintRWSLIST(RWYLIST *pL)
{	static char *pszDels[] = { "Runway", "DelRwy", "DelSta", "DelBth" };
	if (memcmp(&pL->r.chICAO[4], "99", 2))
	{	fprintf(fpAFDS, "\n              *** %s *** %.8s Lat %.6f Long %.6f Alt %.2f Hdg %d Len %d Wid %d",
			pszDels[-pL->fDelete],
			pL->r.chICAO,
			(double) pL->r.fLat,
			(double) pL->r.fLong,
			pL->r.fAlt,
			pL->r.uHdg,
			pL->r.uLen,
			pL->r.uWid);
		if (pL->r.chILS[1])
			fprintf(fpAFDS, " ILS %.6s%s%s%s%s", pL->r.chILS,
				(pL->fILSflags & 0x1C) ? ", Flags:" : "",
				(pL->fILSflags & 0x08) ? " GS" : "",
				(pL->fILSflags & 0x10) ? " DME" : "",
				(pL->fILSflags & 0x04) ? " BC" : "");
	}
}

/******************************************************************************
         IsBGLSourcePrefix
******************************************************************************/

static BOOL IsBGLSourcePrefix(const char *path, const char *prefix)
{
	const char *name;
	const char *backslash;
	const char *slash;

	if (!path || !prefix)
		return FALSE;

	name = path;
	backslash = strrchr(path, '\\');
	slash = strrchr(path, '/');

	if (backslash && (!slash || (backslash > slash)))
		name = backslash + 1;
	else if (slash)
		name = slash + 1;

	return (_strnicmp(name, prefix, 3) == 0);
}

/******************************************************************************
         MergeRunwayNavigation
******************************************************************************/

static void MergeRunwayNavigation(RWYLIST *destination, const RWYLIST *source)
{
	if (!destination || !source)
		return;

	/*
	    Preserve the ILS identifier even when the source record has not
	    resolved the corresponding frequency yet.
	*/
	if (source->r.chILSid[0])
	{
		memcpy(
			destination->r.chILSid,
			source->r.chILSid,
			sizeof(destination->r.chILSid));
	}

	/*
	    A non-empty frequency identifies a fully resolved ILS record.
	    Copy all associated fields as one coherent group.
	*/
	if (source->r.chILS[0])
	{
		memcpy(
			destination->r.chILS,
			source->r.chILS,
			sizeof(destination->r.chILS));

		memcpy(
			destination->r.chILSHdg,
			source->r.chILSHdg,
			sizeof(destination->r.chILSHdg));

		destination->r.fILSslope = source->r.fILSslope;

		memcpy(
			destination->r.chNameILS,
			source->r.chNameILS,
			sizeof(destination->r.chNameILS));

		destination->fILSflags = source->fILSflags;
	}
}

/******************************************************************************
         GetRunwayICAOKey
******************************************************************************/

static void GetRunwayICAOKey(const RWYLIST *entry, char key[9])
{
	int i;

	memset(key, 0, 9);

	if (!entry)
		return;

	if (entry->chICAOFull[0])
	{
		strncpy_s(key, 9, entry->chICAOFull, _TRUNCATE);
		return;
	}

	memcpy(key, entry->r.chICAO, 4);
	key[4] = 0;

	for (i = 3; i >= 0; i--)
	{
		if (key[i] != ' ')
			break;

		key[i] = 0;
	}
}

/******************************************************************************
         CompareRunwayListKey
******************************************************************************/

static __int32 CompareRunwayListKey(
	const RWYLIST *left,
	const RWYLIST *right,
	__int32 legacyLength)
{
	char leftICAO[9];
	char rightICAO[9];
	__int32 result;

	GetRunwayICAOKey(left, leftICAO);
	GetRunwayICAOKey(right, rightICAO);

	result = _stricmp(leftICAO, rightICAO);

	if (result != 0)
		return result;

	/*
	    A four-byte comparison is used when deleting or locating all
	    entries belonging to an airport.
	*/
	if (legacyLength <= 4)
		return 0;

	/*
	    The remaining four bytes of the legacy eight-byte key contain
	    the runway, gate or taxiway identifier.
	*/
	return memcmp(
		left->r.chRwy,
		right->r.chRwy,
		legacyLength - 4);
}

/******************************************************************************
         ProcessRunwayList
******************************************************************************/

void ProcessRunwayList(RWYLIST *pL, BOOL fAdd, BOOL fNoCtr)
{	BOOL fDelAll = (fAdd <= 0) && !pL->r.chRwy[0] && !pL->fAirport;
	__int32 nCompLen = fDelAll ? 4 : 8;

	// First, standardise chICAO 4 chars: ###4692
	if (pL->r.chICAO[3] == ' ') pL->r.chICAO[3] = 0;

	if ((fAdd > 0) && !pL->fAirport && !pL->pGateList && !pL->pTaxiwayList) PrintRWSLIST(pL);

	if (fDebugThisEntry)
	{	if (strncmp(pL->r.chRwy, "999", 3) == 0)
			fprintf(fpAFDS,"\nAdded %.4s entry for Gates\n", pL->r.chICAO);
		else if (strncmp(pL->r.chRwy, "998", 3) == 0)
			fprintf(fpAFDS,"\nAdded %.4s entry for Taxiways\n", pL->r.chICAO);
	}
								
	// Now link it in at correct place: (or find it if deleting)
	if (pRlast == NULL)
		pRlast = pR;
						
	if (pR == NULL)
	{	if (fAdd <= 0) return; // deleting when no runways yet!
		pR = pRlast = pL;
		ulTotalRwys++;
	}
										
	else
	{	__int32 fDir = 0;

		if (fAdd <= 0) pRlast = pR; // deletes need search from start
										
		while (pRlast) 
		{	__int32 comp = CompareRunwayListKey(pRlast, pL, nCompLen);
			if (fUserAbort) return;

			if (fNewAirport && (CompareRunwayListKey(pRlast, pL, 4) == 0))
			{	ulTotalAPs--;
				fNewAirport = FALSE;	
			}

			if (comp == 0) // Duplicate (or found one to delete)
			{
				if (fAdd > 0)
				{
					BOOL keepExistingAPX =
						fMSFS &&
						IsBGLSourcePrefix(pRlast->pPathName, "apx") &&
						(IsBGLSourcePrefix(pL->pPathName, "nax") ||
						IsBGLSourcePrefix(pL->pPathName, "nvx"));

					/*
						APX is the canonical airport/runway source.

						A later NAX/NVX entry may enrich the APX entry with
						navigation data, but must not replace its geometry,
						altitude, source pathname or scenery title.
					*/
					if (keepExistingAPX)
					{
						MergeRunwayNavigation(pRlast, pL);

						if ((pRlast->Atis < 0x1800) &&
							(pL->Atis >= 0x1800))
						{
							pRlast->Atis = pL->Atis;
						}

						if (!pRlast->pCountryName)
							pRlast->pCountryName = pL->pCountryName;

						if (!pRlast->pStateName)
							pRlast->pStateName = pL->pStateName;

						if (!pRlast->pCityName)
							pRlast->pCityName = pL->pCityName;

						if (!pRlast->pAirportName)
							pRlast->pAirportName = pL->pAirportName;

						if (!pRlast->pGateList)
						{
							pRlast->pGateList = pL->pGateList;
							pL->pGateList = NULL;
						}
						else if (pL->pGateList)
						{
							free(pL->pGateList);
							pL->pGateList = NULL;
						}

						if (!pRlast->pTaxiwayList)
						{
							pRlast->pTaxiwayList = pL->pTaxiwayList;
							pL->pTaxiwayList = NULL;
						}
						else if (pL->pTaxiwayList)
						{
							free(pL->pTaxiwayList);
							pL->pTaxiwayList = NULL;
						}

						if (pRlast->fDelete && !fNoCtr)
							ulTotalRwys++;

						pRlast->fDelete = 0;

						if (strncmp(pL->r.chRwy, "999", 3) == 0)
							prwyPrevious = pRlast;

						free(pL);
						break;
					}

					/*
						Normal precedence: the new entry replaces the previous
						one, but must inherit complete ILS information when the
						new source has not resolved it.
					*/
					if (!pL->r.chILS[0])
						MergeRunwayNavigation(pL, pRlast);
					
					if (pL->Atis < 0x1800)
						pL->Atis = pRlast->Atis;
					
					memcpy(&pRlast->r, &pL->r, sizeof(RWYDATA));
					memcpy(pRlast->chICAOFull, pL->chICAOFull, sizeof(pRlast->chICAOFull));
					
					pRlast->fHdg = pL->fHdg;
					pRlast->nOffThresh = pL->nOffThresh;
					pRlast->fMagvar = pL->fMagvar;
					pRlast->fLat = pL->fLat;
					pRlast->fLong = pL->fLong;
					
					if (pRlast->fDelete && !fNoCtr)
					{	ulTotalRwys++;
					}
					pRlast->fDelete = 0;
					pRlast->fCL = pL->fCL;
					pRlast->fCTO = pL->fCTO;
					if (!pRlast->pGateList)
						pRlast->pGateList = pL->pGateList;
					if (!pRlast->pTaxiwayList)
						pRlast->pTaxiwayList = pL->pTaxiwayList;
					pRlast->fILSflags = pL->fILSflags;

					if (!fMSFS || !(pRlast->pPathName && (strstr(pL->pPathName, "fs-base-nav"))))
						// Retain previous pathname if new one only MSFS NAV additions
						pRlast->pPathName = pL->pPathName;
					if (!fMSFS || !(pRlast->pSceneryName && (strstr(pL->pSceneryName, "fs-base-nav"))))
						// Retain previous title if new one only MSFS NAV additions
						pRlast->pSceneryName = pL->pSceneryName;
					if (strncmp(pL->r.chRwy, "999", 3) == 0)
						prwyPrevious = pRlast;
					free(pL);
					break;
				}

				// Deletion:
				// Just mark it for omission
				if (!fDelAll || ((pRlast->pGateList == 0) && (pRlast->pTaxiwayList == 0)))
				{	if (!pRlast->fDelete)
					{	if (!pRlast->fAirport && (pRlast->pGateList == 0) && (pRlast->pTaxiwayList == 0))
						{	ulTotalRwys--;
						}
						pRlast->fDelete = fAdd-1;
					}

					else if (pRlast->fDelete != fAdd-1) 
						pRlast->fDelete = -3;
				}
			
				if (pRlast->pGateList) free(pRlast->pGateList);
				if (pRlast->pTaxiwayList) free(pRlast->pTaxiwayList);
				pRlast->pGateList = 0;
				pRlast->pTaxiwayList = 0;

				if (!pRlast->fAirport)
				{	PrintRWSLIST(pRlast);
					fprintf(fpAFDS, "\n");
				}
				
				fDir = 1;

				pRlast = pRlast->pTo;
			}

			if (comp < 0)
			{	//  Goes later
				if ((fDir < 0) || !pRlast->pTo)
				{	// Insert after last
					if (fAdd <= 0) break;

					pL->pFrom = pRlast;
					pL->pTo = pRlast->pTo;
					if (pL->pTo) pL->pTo->pFrom = pL;
					pRlast->pTo = pL;
					pRlast = pL;
					if (!pL->fAirport && (pL->pGateList == 0) && (pL->pTaxiwayList == 0))
					{	ulTotalRwys++;
					}
					
					if (strncmp(pL->r.chRwy, "999", 3) == 0)
						prwyPrevious = pRlast;
					break;
				}

				fDir = 1;
				pRlast = pRlast->pTo;
			}

			else if (comp > 0)
			{	//  Goes earlier
				if ((fDir > 0) || !pRlast->pFrom)
				{	// Insert before last
					if (fAdd <= 0) break;

					pL->pFrom = pRlast->pFrom;
					pL->pTo = pRlast;
					pRlast->pFrom = pL;
					if (pL->pFrom) pL->pFrom->pTo = pL;
					else pR = pL;
					pRlast = pL;
					if (!pL->fAirport && (pL->pGateList == 0) && (pL->pTaxiwayList == 0))
					{	ulTotalRwys++;
					}
					
					if (strncmp(pL->r.chRwy, "999", 3) == 0)
						prwyPrevious = pRlast;
					break;
				}

				fDir = -1;
				pRlast = pRlast->pFrom;
			}
		}
	}			

	fNewAirport = FALSE;
}

/******************************************************************************
         AddRunway
******************************************************************************/

void AddRunway(RWYLIST *prwy)
{	RWYLIST *pL = (RWYLIST *) malloc(sizeof(RWYLIST)), *pRes = 0;
	if (pL)
	{	memcpy(pL, prwy, sizeof(RWYLIST));

		/*
		    Runways, gates and taxiway entries must retain their BGL
		    provenance so duplicate APX/NAX records can be merged
		    according to their source.
		*/
		if (!pL->pPathName)
			pL->pPathName = pPathName;

		if (!pL->pSceneryName)
			pL->pSceneryName = pSceneryName;

		ProcessRunwayList(pL, TRUE, 0);
		if (!pL->fAirport) fFS9 = 1; // Stay in >=FS9 mode		
	}
}

/******************************************************************************
         SetLocPos
******************************************************************************/

void SetLocPos(LOCATION *pLoc, __int32 alt, __int32 lat, __int32 lon, float *pflat, float *pflon, double *pdLat, double *pdLon)
{	DWORD wk;
	pLoc->elev = (__int32) ((alt * 65536.0) / 1000.0);
	*((__int64 *) &pLoc->lat.f) = fslat2lat(lat, pflat, pdLat);
	wk = (DWORD) pLoc->lat.f;
	pLoc->lat.i = (__int32) pLoc->lat.f;
	pLoc->lat.f = wk;
	*((__int64 *) &pLoc->lon.f) = fslon2lon(lon, pflon, pdLon);
	wk = (DWORD) pLoc->lon.f;
	pLoc->lon.i = (__int32) pLoc->lon.f;
	pLoc->lon.f = wk;
}

/******************************************************************************
         StoreName
******************************************************************************/

void StoreName(char *psz, NNAM *pName)
{	psz[0] = 0;
	if (pName->nLen > 6)
	{	__int32 nlen = min(pName->nLen-6, 255);
		memcpy(psz, pName->chName, nlen);
		while (nlen && !isalnum((unsigned char) psz[nlen - 1])) nlen--;
		psz[nlen] = 0;
	}
}

/******************************************************************************
         HdgDiff
******************************************************************************/

float HdgDiff(float f1, float f2)
{	float fres = (float) fabs(f1 - f2);
	if (fres > 180.0F) fres = 360.0F - fres;
	return fres;
}

/******************************************************************************
         NewILSs
******************************************************************************/

BOOL fMatchedILS = FALSE, fFoundSome = FALSE;
__int32 nOffsetILS, nSizeILS;
char chNameILS[256];
float fILSheading, fILSslope, fRange2;

/*
    Active MSFS 2024 BGL v9 context.

    The context is valid only while CheckMSFS2024BGLV9() is processing one
    file. It allows FindILSdetails() to search layer 19 without passing the
    v9 container layout through the legacy NSECTS/NOBJ interface.
*/
static const BYTE *pMSFS2024BGLV9Data = NULL;
static DWORD nMSFS2024BGLV9FileSize = 0;
static DWORD nMSFS2024BGLV9HeaderSize = 0;
static DWORD nMSFS2024BGLV9LayerCount = 0;

/*
    Global MSFS 2024 NVX/NAX ILS index.

    The v9 airport/APX record may reference an ILS ident whose full VOR/ILS
    record is stored in another NAX/NVX BGL.  This index is intentionally keyed
    first by localizer ident only; runway/ICAO association is left to later
    patches.  To keep the first fallback safe, candidates are still filtered by
    runway heading and, when runway coordinates are available, by proximity.
*/
typedef struct _GLOBAL_NVX_ILS_V9
{
    char chIdent[16];
    char chName[256];
    char chSourceFile[MAX_PATH];
    __int32 nFreq;          /* MakeRunways convention: 10 kHz units */
    __int32 nOffset;
    __int32 nSize;
    float fHeading;
    float fSlope;
    float fLat;
    float fLon;
    BYTE fFlags;
} GLOBAL_NVX_ILS_V9;

static GLOBAL_NVX_ILS_V9 *pGlobalNvxIlsV9 = NULL;
static DWORD nGlobalNvxIlsV9Count = 0;
static DWORD nGlobalNvxIlsV9Alloc = 0;
static DWORD nGlobalNvxIlsV9Duplicates = 0;
static DWORD nGlobalNvxIlsV9FilesWithIls = 0;
static DWORD nGlobalNvxIlsV9Lookups = 0;
static DWORD nGlobalNvxIlsV9Used = 0;
static DWORD nGlobalNvxIlsV9Misses = 0;
static DWORD nGlobalNvxIlsV9HeadingRejects = 0;
static DWORD nGlobalNvxIlsV9RangeRejects = 0;
static BOOL fMatchedGlobalNvxIlsV9 = FALSE;
static char chMatchedGlobalNvxIlsV9Source[MAX_PATH];
 
 /******************************************************************************
         DecodeIDV9
******************************************************************************/

static void DecodeIDV9(DWORD number, char *p)
{
	char r, c;
	char *p1 = p;
	DWORD q = number >> BGLV9_VOR_IDENT_SHIFT;

	for (; q > 0; q /= 38)
	{
		r = (char)(q % 38);
		c = (r < 12) ? r + 46 : r + 53;
		*p++ = c;
	}

	*p = 0;

	if (!*p1)
	{
		*p1 = '0';
		*++p1 = 0;
	}
	else
	{
		str_rev(p1);
	}
}

static void CopyIdentUpper(char *pszDst, size_t cbDst, const char *pszSrc)
{
	size_t i, j;
	if (!pszDst || !cbDst)
		return;

	pszDst[0] = 0;

	if (!pszSrc)
		return;

	while (*pszSrc && isspace((unsigned char)*pszSrc))
		pszSrc++;

	for (i = 0, j = 0; pszSrc[i] && (j + 1) < cbDst; i++)
	{
		if (isspace((unsigned char)pszSrc[i]))
			break;

		pszDst[j++] = (char)toupper((unsigned char)pszSrc[i]);
	}

	pszDst[j] = 0;
}

static BOOL ExtractIlsV9Record(
	const BYTE *record,
	DWORD recordSize,
	GLOBAL_NVX_ILS_V9 *pOut)
{
	const BGLV9_VOR_ILS_RECORD *pIls;
	DWORD childOffset;
	BOOL fHaveLocalizer = FALSE;
	float fHeading = 0.0F;
	float fSlope = 0.0F;

	if (!record || !pOut || (recordSize < sizeof(BGLV9_VOR_ILS_RECORD)))
		return FALSE;

	if (ReadBGLWordLE(record) != BGLV9_RECORD_VOR_ILS)
		return FALSE;

	pIls = (const BGLV9_VOR_ILS_RECORD *)record;

	if (pIls->facilityType != BGLV9_VOR_FACILITY_ILS)
		return FALSE;

	memset(pOut, 0, sizeof(*pOut));
	DecodeIDV9(pIls->packedIdent, pOut->chIdent);

	childOffset = BGLV9_VOR_ILS_FIXED_SIZE;

	while ((childOffset + sizeof(BGLV9_RECORD_HEADER)) <= recordSize)
	{
		const BYTE *child = record + childOffset;
		WORD childType = ReadBGLWordLE(child);
		DWORD childSize = ReadBGLDwordLE(child + 2);

		if ((childSize < sizeof(BGLV9_RECORD_HEADER)) ||
			(childSize > (recordSize - childOffset)))
		{
			break;
		}

		if ((childType == BGLV9_RECORD_ILS_LOCALIZER) &&
			(childSize >= sizeof(BGLV9_ILS_LOCALIZER_RECORD)))
		{
			const BGLV9_ILS_LOCALIZER_RECORD *pLocalizer =
				(const BGLV9_ILS_LOCALIZER_RECORD *)child;

			fHeading = pLocalizer->heading;
			fHaveLocalizer = TRUE;
		}
		else if ((childType == BGLV9_RECORD_ILS_GLIDESLOPE) &&
			(childSize >= sizeof(BGLV9_ILS_GLIDESLOPE_RECORD)))
		{
			const BGLV9_ILS_GLIDESLOPE_RECORD *pGlideslope =
				(const BGLV9_ILS_GLIDESLOPE_RECORD *)child;

			fSlope = pGlideslope->pitchDegrees;
		}
		else if (childType == BGLV9_RECORD_NAME)
		{
			StoreName(pOut->chName, (NNAM *)child);
		}

		childOffset += childSize;
	}

	if (!fHaveLocalizer)
		return FALSE;

	pOut->nFreq = (__int32)((pIls->frequencyHz + 5000) / 10000);
	pOut->nOffset = FileOffset32(record);
	pOut->nSize = (__int32)recordSize;
	pOut->fHeading = fHeading;
	pOut->fSlope = fSlope;
	pOut->fFlags = pIls->flags;
	fslat2lat(pIls->latitude, &pOut->fLat, 0);
	fslon2lon(pIls->longitude, &pOut->fLon, 0);

	if (szCurrentFilePath[0])
		strncpy_s(
			pOut->chSourceFile,
			sizeof(pOut->chSourceFile),
			szCurrentFilePath,
			_TRUNCATE);

	return pOut->chIdent[0] && pOut->nFreq;
}

static BOOL IsDuplicateGlobalNvxIlsV9(const GLOBAL_NVX_ILS_V9 *pIls)
{
	DWORD i;

	if (!pIls)
		return TRUE;

	for (i = 0; i < nGlobalNvxIlsV9Count; i++)
	{
		GLOBAL_NVX_ILS_V9 *p = &pGlobalNvxIlsV9[i];

		if ((p->nOffset == pIls->nOffset) &&
			(!_stricmp(p->chSourceFile, pIls->chSourceFile)))
		{
			return TRUE;
		}

		if (!_stricmp(p->chIdent, pIls->chIdent) &&
			(p->nFreq == pIls->nFreq) &&
			(HdgDiff(p->fHeading, pIls->fHeading) < 0.01F) &&
			(fabs((double)(p->fLat - pIls->fLat)) < 0.000001) &&
			(fabs((double)(p->fLon - pIls->fLon)) < 0.000001))
		{
			return TRUE;
		}
	}

	return FALSE;
}

static BOOL AddGlobalNvxIlsV9(const GLOBAL_NVX_ILS_V9 *pIls)
{
	GLOBAL_NVX_ILS_V9 *pNew;
	DWORD nNewAlloc;

	if (!pIls || !pIls->chIdent[0] || !pIls->nFreq)
		return FALSE;

	if (IsDuplicateGlobalNvxIlsV9(pIls))
	{
		nGlobalNvxIlsV9Duplicates++;
		return FALSE;
	}

	if (nGlobalNvxIlsV9Count >= nGlobalNvxIlsV9Alloc)
	{
		nNewAlloc = nGlobalNvxIlsV9Alloc + 256;
		pNew = (GLOBAL_NVX_ILS_V9 *)realloc(
			pGlobalNvxIlsV9,
			nNewAlloc * sizeof(GLOBAL_NVX_ILS_V9));

		if (!pNew)
			return FALSE;

		pGlobalNvxIlsV9 = pNew;
		nGlobalNvxIlsV9Alloc = nNewAlloc;
	}

	pGlobalNvxIlsV9[nGlobalNvxIlsV9Count++] = *pIls;
	return TRUE;
}

static void IndexGlobalNvxIlsV9Payload(
	const BYTE *payload,
	DWORD payloadSize,
		DWORD expectedRecords)
{
	DWORD recordIndex;
	DWORD cursor = 0;

	for (recordIndex = 0; recordIndex < expectedRecords; recordIndex++)
	{
		const BYTE *record;
		DWORD recordSize;
		WORD recordType;

		if ((payloadSize - cursor) < sizeof(BGLV9_RECORD_HEADER))
			return;

		record = payload + cursor;
		recordType = ReadBGLWordLE(record);
		recordSize = ReadBGLDwordLE(record + 2);

		if ((recordSize < sizeof(BGLV9_RECORD_HEADER)) ||
			(recordSize > (payloadSize - cursor)))
		{
			return;
		}

		if ((recordType == BGLV9_RECORD_VOR_ILS) &&
			(recordSize >= sizeof(BGLV9_VOR_ILS_RECORD)))
		{
			GLOBAL_NVX_ILS_V9 ils;

			if (ExtractIlsV9Record(record, recordSize, &ils))
				AddGlobalNvxIlsV9(&ils);
		}

		cursor += recordSize;
	}
}

static void BuildGlobalNvxIlsV9Index(void)
{
	DWORD i;
	DWORD nCountBefore;

	if (!pMSFS2024BGLV9Data ||
		!nMSFS2024BGLV9FileSize ||
		!nMSFS2024BGLV9LayerCount)
	{
		return;
	}

	nCountBefore = nGlobalNvxIlsV9Count;

	for (i = 0; i < nMSFS2024BGLV9LayerCount; i++)
	{
		const BYTE *layer =
			pMSFS2024BGLV9Data +
			nMSFS2024BGLV9HeaderSize +
			(i * (DWORD)sizeof(BGLV9_LAYER_DESCRIPTOR));
		DWORD layerType = ReadBGLDwordLE(layer);
		DWORD modeFlags = ReadBGLDwordLE(layer + 4);
		DWORD subsectionCount = ReadBGLDwordLE(layer + 8);
		DWORD indexOffset = ReadBGLDwordLE(layer + 12);
		DWORD indexSize = ReadBGLDwordLE(layer + 16);
		DWORD indexStride =
			(modeFlags & BGLV9_QMID64_FLAG)
				? (DWORD)sizeof(BGLV9_QMID64_ENTRY)
				: (DWORD)sizeof(BGLV9_QMID32_ENTRY);
		DWORD requiredIndexSize;
		DWORD j;

		if (layerType != BGLV9_LAYER_VOR)
			continue;

		if (subsectionCount > ((DWORD)-1 / indexStride))
			continue;

		requiredIndexSize = subsectionCount * indexStride;

		if ((indexSize < requiredIndexSize) ||
			!IsValidBGLFileRange(
				indexOffset,
				indexSize,
				nMSFS2024BGLV9FileSize))
		{
			continue;
		}

		for (j = 0; j < subsectionCount; j++)
		{
			const BYTE *entry =
				pMSFS2024BGLV9Data +
				indexOffset +
				(j * indexStride);
			DWORD itemCount;
			DWORD payloadOffset;
			DWORD payloadSize;

			if (modeFlags & BGLV9_QMID64_FLAG)
 			{		
				itemCount = ReadBGLDwordLE(entry + 8);
				payloadOffset = ReadBGLDwordLE(entry + 12);
				payloadSize = ReadBGLDwordLE(entry + 16);
			}
			else
			{
				itemCount = ReadBGLDwordLE(entry + 4);
				payloadOffset = ReadBGLDwordLE(entry + 8);
				payloadSize = ReadBGLDwordLE(entry + 12);
 			}

			if (!itemCount ||
				!IsValidBGLFileRange(
					payloadOffset,
					payloadSize,
					nMSFS2024BGLV9FileSize))
 			{
 				continue;
 			}			


			IndexGlobalNvxIlsV9Payload(
				pMSFS2024BGLV9Data + payloadOffset,
				payloadSize,
				itemCount);
		}
	}

	if (nGlobalNvxIlsV9Count > nCountBefore)
		nGlobalNvxIlsV9FilesWithIls++;
}



static float GlobalNvxIlsV9Range2(const GLOBAL_NVX_ILS_V9 *pIls, const RWYLIST *prwy)
{
	float fLatDiff;
	float fLonDiff;

	if (!pIls || !prwy)
		return 999.0F;
	fLatDiff = pIls->fLat - prwy->fLat;
	fLonDiff = (float)((pIls->fLon - prwy->fLong) *
		cos((pIls->fLat + prwy->fLat) * PI / 360.0));

	return (fLatDiff * fLatDiff) + (fLonDiff * fLonDiff);
}

static __int32 MatchGlobalNvxIlsV9(char *psz, RWYLIST *prwy)
{
	char chIdent[16];
	const GLOBAL_NVX_ILS_V9 *pBest = NULL;
	float fBestScore = 999999.0F;
	DWORD i;
	BOOL fHaveRunwayPosition;

	fMatchedGlobalNvxIlsV9 = FALSE;
	chMatchedGlobalNvxIlsV9Source[0] = 0;
	CopyIdentUpper(chIdent, sizeof(chIdent), psz);

	if (!chIdent[0])
		return 0;

	nGlobalNvxIlsV9Lookups++;

	if (!nGlobalNvxIlsV9Count)
	{
		nGlobalNvxIlsV9Misses++;
 		return 0;
	}

	fHaveRunwayPosition =
		prwy &&
		((fabs((double)prwy->fLat) > 0.000001) ||
		 (fabs((double)prwy->fLong) > 0.000001));

	for (i = 0; i < nGlobalNvxIlsV9Count; i++)
	{
		const GLOBAL_NVX_ILS_V9 *pIls = &pGlobalNvxIlsV9[i];
		float fHeadingDiff;
		float fRange2 = 0.0F;
		float fScore;

		if (_stricmp(pIls->chIdent, chIdent) != 0)
			continue;

		fHeadingDiff = prwy ? HdgDiff(pIls->fHeading, prwy->fHdg) : 0.0F;
		if (prwy && (fHeadingDiff >= 40.0F))
		{
			nGlobalNvxIlsV9HeadingRejects++;
			continue;
		}

		if (fHaveRunwayPosition)
		{
			fRange2 = GlobalNvxIlsV9Range2(pIls, prwy);

			/* Same 3 NM safety radius used by the legacy range check. */
			if (fRange2 >= 0.0025F)
			{
				nGlobalNvxIlsV9RangeRejects++;
				continue;
			}
		}

		fScore = (fRange2 * 100000.0F) + fHeadingDiff;

		if (!pBest || (fScore < fBestScore))
		{
			pBest = pIls;
			fBestScore = fScore;
		}
	}

	if (!pBest)
	{
		nGlobalNvxIlsV9Misses++;
		return 0;
	}

	fILSheading = pBest->fHeading;
	fILSslope = pBest->fSlope;
	fMatchedILS = TRUE;
	fFoundSome = TRUE;
	nOffsetILS = pBest->nOffset;
	nSizeILS = pBest->nSize;
	chNameILS[0] = 0;
	strncpy_s(chNameILS, sizeof(chNameILS), pBest->chName, _TRUNCATE);

	if (prwy)
		prwy->fILSflags = pBest->fFlags;

	fMatchedGlobalNvxIlsV9 = TRUE;
	nGlobalNvxIlsV9Used++;
	strncpy_s(
		chMatchedGlobalNvxIlsV9Source,
		sizeof(chMatchedGlobalNvxIlsV9Source),
		pBest->chSourceFile,
		_TRUNCATE);

	return pBest->nFreq;
}

/******************************************************************************
         ReportGlobalNvxIlsV9Index
******************************************************************************/

void ReportGlobalNvxIlsV9Index(void)
{
	if (!fpAFDS)
		return;

	if (!nGlobalNvxIlsV9Count &&
		!nGlobalNvxIlsV9Duplicates &&
		!nGlobalNvxIlsV9Lookups)
	{
		return;
	}

	fprintf(fpAFDS,
		"\nMSFS2024 global NVX/NAX ILS index:\n"
		"  files with ILS records      = %lu\n"
		"  records indexed             = %lu\n"
		"  duplicate records skipped   = %lu\n"
		"  fallback lookups            = %lu\n"
		"  fallback matches used       = %lu\n"
		"  fallback misses             = %lu\n"
		"  rejected by heading safety  = %lu\n"
		"  rejected by range safety    = %lu\n",
		(unsigned long)nGlobalNvxIlsV9FilesWithIls,
		(unsigned long)nGlobalNvxIlsV9Count,
		(unsigned long)nGlobalNvxIlsV9Duplicates,
		(unsigned long)nGlobalNvxIlsV9Lookups,
		(unsigned long)nGlobalNvxIlsV9Used,
		(unsigned long)nGlobalNvxIlsV9Misses,
		(unsigned long)nGlobalNvxIlsV9HeadingRejects,
		(unsigned long)nGlobalNvxIlsV9RangeRejects);
}

void FreeGlobalNvxIlsV9Index(void)
{
	if (pGlobalNvxIlsV9)
	{
		free(pGlobalNvxIlsV9);
		pGlobalNvxIlsV9 = NULL;
	}

	nGlobalNvxIlsV9Count = 0;
	nGlobalNvxIlsV9Alloc = 0;
}

/******************************************************************************
         NewILSsV9
******************************************************************************/

static __int32 NewILSsV9(
	const BYTE *payload,
	DWORD payloadSize,
	DWORD expectedRecords,
	char *psz,
	RWYLIST *prwy)
{
	DWORD recordIndex;
	DWORD cursor = 0;
	char chSearchIdent[16];

	CopyIdentUpper(chSearchIdent, sizeof(chSearchIdent), psz);

	for (recordIndex = 0; recordIndex < expectedRecords; recordIndex++)
	{
		const BYTE *record;
		DWORD recordSize;
		WORD recordType;

		if ((payloadSize - cursor) < sizeof(BGLV9_RECORD_HEADER))
			return 0;

		record = payload + cursor;
		recordType = ReadBGLWordLE(record);
		recordSize = ReadBGLDwordLE(record + 2);

		if ((recordSize < sizeof(BGLV9_RECORD_HEADER)) ||
			(recordSize > (payloadSize - cursor)))
		{
			return 0;
		}

		if ((recordType == BGLV9_RECORD_VOR_ILS) &&
			(recordSize >= sizeof(BGLV9_VOR_ILS_RECORD)))
		{
			GLOBAL_NVX_ILS_V9 ils;

			if (!ExtractIlsV9Record(record, recordSize, &ils))
			{
				cursor += recordSize;
				continue;
			}

			if (_stricmp(ils.chIdent, chSearchIdent) != 0)
			{
				cursor += recordSize;
				continue;
			}

			if (prwy && (HdgDiff(ils.fHeading, prwy->fHdg) >= 40.0F))
			{
				cursor += recordSize;
				continue;
			}

			fILSheading = ils.fHeading;
			fILSslope = ils.fSlope;
			fMatchedILS = TRUE;
			fFoundSome = TRUE;
			nOffsetILS = ils.nOffset;
			nSizeILS = ils.nSize;
			chNameILS[0] = 0;
			strncpy_s(chNameILS, sizeof(chNameILS), ils.chName, _TRUNCATE);

			if (prwy)
			{
				prwy->fILSflags = ils.fFlags;
			}

			return ils.nFreq;
		}

		cursor += recordSize;
	}

	return 0;
}

/******************************************************************************
         MatchILSV9
******************************************************************************/

static __int32 MatchILSV9(char *psz, RWYLIST *prwy)
{
	DWORD i;

	fMatchedILS = FALSE;
	chNameILS[0] = 0;

	if (!pMSFS2024BGLV9Data ||
		!nMSFS2024BGLV9FileSize ||
		!nMSFS2024BGLV9LayerCount ||
		!psz ||
		!psz[0])
	{
		return 0;
	}

	for (i = 0; i < nMSFS2024BGLV9LayerCount; i++)
	{
		const BYTE *layer =
			pMSFS2024BGLV9Data +
			nMSFS2024BGLV9HeaderSize +
			(i * (DWORD)sizeof(BGLV9_LAYER_DESCRIPTOR));
		DWORD layerType = ReadBGLDwordLE(layer);
		DWORD modeFlags = ReadBGLDwordLE(layer + 4);
		DWORD subsectionCount = ReadBGLDwordLE(layer + 8);
		DWORD indexOffset = ReadBGLDwordLE(layer + 12);
		DWORD indexSize = ReadBGLDwordLE(layer + 16);
		DWORD indexStride =
			(modeFlags & BGLV9_QMID64_FLAG)
				? (DWORD)sizeof(BGLV9_QMID64_ENTRY)
				: (DWORD)sizeof(BGLV9_QMID32_ENTRY);
		DWORD requiredIndexSize;
		DWORD j;

		if (fUserAbort)
			return 0;

		if (layerType != BGLV9_LAYER_VOR)
			continue;

		if (subsectionCount > ((DWORD)-1 / indexStride))
			continue;

		requiredIndexSize = subsectionCount * indexStride;

		if ((indexSize < requiredIndexSize) ||
			!IsValidBGLFileRange(
				indexOffset,
				indexSize,
				nMSFS2024BGLV9FileSize))
		{
			continue;
		}

		for (j = 0; j < subsectionCount; j++)
		{
			const BYTE *entry =
				pMSFS2024BGLV9Data +
				indexOffset +
				(j * indexStride);
			DWORD itemCount;
			DWORD payloadOffset;
			DWORD payloadSize;
			__int32 nFreq;

			if (fUserAbort)
				return 0;

			if (modeFlags & BGLV9_QMID64_FLAG)
			{
				itemCount = ReadBGLDwordLE(entry + 8);
				payloadOffset = ReadBGLDwordLE(entry + 12);
				payloadSize = ReadBGLDwordLE(entry + 16);
			}
			else
			{
				itemCount = ReadBGLDwordLE(entry + 4);
				payloadOffset = ReadBGLDwordLE(entry + 8);
				payloadSize = ReadBGLDwordLE(entry + 12);
			}

			if (!itemCount ||
				!IsValidBGLFileRange(
					payloadOffset,
					payloadSize,
					nMSFS2024BGLV9FileSize))
			{
				continue;
			}

			nFreq = NewILSsV9(
				pMSFS2024BGLV9Data + payloadOffset,
				payloadSize,
				itemCount,
				psz,
				prwy);

			if (nFreq)
				return nFreq;
		}
	}

	if (prwy)
		prwy->fILSflags = 0;

	return 0;
}

/******************************************************************************
        NewILSs
 ******************************************************************************/

// Mode 0 = Match ID & Dir
//      1 = Match Name & Dir & Range
//      -1 = List all by Range

__int32 NewILSs(NILS *pi, DWORD size, char *psz, RWYLIST *prwy, __int32 nMode)
{	char chId[16], chRwy[6], *pch;
	NILS *pi2;
	DWORD size2;
	BOOL fInRange = FALSE;
	
	fMatchedILS = FALSE;
	chNameILS[0] = 0;

	while ((size > 6) && (size >= pi->nLen))
	{	__int32 nThisLen = sizeof(NILS);

		if (fUserAbort) return 0;
		
		if ((pi->wId == OBJTYPE_VOR) && (pi->bType == 4))
		{	// ILS record found
			BOOL fMatchID;
			nThisLen = pi->nLen;
			fILSheading = pi->loc.fHeading;
			fILSslope = (pi->loc.nRec0014 == 0x0015) ? ((NILSGS *) &pi->loc.nRec0014)->fGSpitch :
						*((WORD *) ((BYTE *) &pi->loc.fWidth + 4)) == 0x0015 ?
							((NILSGS *) ((BYTE *) &pi->loc.fWidth + 4))->fGSpitch : 0.00F;
			nOffsetILS = FileOffset32(&pi->wId);
			nSizeILS = pi->nLen;
			
			DecodeID(pi->nId, chId, 1);
			fMatchID = _stricmp(chId, psz) == 0;

			if (prwy && fMatchID && (HdgDiff(pi->loc.fHeading, prwy->fHdg) < 40.0))
			{	prwy->fILSflags = pi->bFlags;
				fMatchedILS = TRUE;
				fInRange = TRUE;
			}

			if (nMode && prwy)
			{	// Check range
				LOCATION loc;
				float fLat, fLon, fWk1, fWk2;
				SetLocPos(&loc, pi->nAlt, pi->nLat, pi->nLon, &fLat, &fLon, 0 ,0);

				fWk1 = fLat - prwy->fLat;
				fWk2 = (float) ((fLon - prwy->fLong) * cos((fLat + prwy->fLat)* PI / 360.0));
				
				fRange2 = (fWk1*fWk1) + (fWk2*fWk2);

				fInRange = fRange2 < 0.0025F; // 3.0nm = 1/20th degree = ^2 
			}

			fFoundSome |= fInRange;

			if (fInRange && (nMode < 2))
			{	// look for the name
				size2 = size - sizeof(NILS);
				pi2 = (NILS *) ((BYTE *) pi + sizeof(NILS));
			
				// Set up proper runway name for comparison
				pch = &prwy->r.chRwy[0];
				if (*pch == '0') pch++;
				chRwy[0] = pch[0];
				chRwy[1] = pch[1];
				chRwy[2] = chRwyT[pch[2] & 3];
				chNameILS[0] = 0;
				
				while ((size2 > 6) && (size2 >= pi2->nLen))
				{	if (pi2->wId == OBJTYPE_NAME)
					{	StoreName(chNameILS, (NNAM *) pi2);
						if ((nMode == 0) && fMatchedILS)
						{	prwy->r.fILSslope = fILSslope;
							return pi->nFreq;
						}
						
						if (nMode == 1)
						{	//pch = strrchr(chNameILS, ' ');
							//if ((pch && strncmp(&pch[1], chRwy, strlen(pch+1)) == 0)
							//		&& (HdgDiff(pi->loc.fHeading, prwy->fHdg) < 80.0))

							if (HdgDiff(pi->loc.fHeading, prwy->fHdg) < 80.0)
							{	prwy->fILSflags = pi->bFlags;
								strcpy(psz, chId);
								fMatchedILS = fMatchID;
								return pi->nFreq;
							}
						}
	
						break;
					}
		
					if ((pi2->wId != OBJTYPE_ILSADD) &&
							(pi2->wId != OBJTYPE_GS) &&
							(pi2->wId != OBJTYPE_DME))
						break;
								
					size2 -= pi2->nLen;
					pi2 = (NILS *) ((BYTE *) pi2 + pi2->nLen);
				}
			}

			/*if (fInRange) // && (nMode < 0))
			{	fprintf(fpAFDS, "\n              ");
				fprintf(fpAFDS, "ILS ID=%s: Freq %.2f Heading %.1f (%s), Range %.2f, Slope %.2f \x22%s\x22",
					chId, (double) (pi->nFreq / 1000000.0),
					(double) pi->loc.fHeading,
					(pi->loc.bEnd & 16) ? "Secondary" : "Primary",
					60.0 * sqrt((double) fRange2),
					(double) fILSslope,
					chNameILS);
			}*/
		}

		else
			nThisLen = pi->nLen;
		
		size -= nThisLen;
		pi = (NILS *) ((BYTE *) pi + nThisLen);
	}

	if (prwy && (nMode >= 0)) prwy->fILSflags = 0;
	return 0;
}

/******************************************************************************
         MatchILS
******************************************************************************/

__int32 MatchILS(DWORD nObjs, NSECTS *ps, BYTE *p, char *psz, RWYLIST *prwy, __int32 nMode)
{	// Look for VORs (ILSs)
	DWORD i, j;

	for (i = 0; i < nObjs; i++)
	{	DWORD offs = ps[i].nGroupOffset;

		if (fUserAbort) return 0;
		
		for (j = 0; j < ps[i].nGroupsCount; j++)
		{	if (ps[i].nObjType == OBJTYPE_VOR)
			{	NOBJ *po = (NOBJ *) &p[offs];

				__int32 nFreq = NewILSs((NILS *) &p[po->chunkoff], po->chunksize, psz, prwy, nMode);
				if (nFreq)
					return (nFreq + 5000) / 10000;
			}

			offs += sizeof(NOBJ);
		}
	}

	return 0;
}

/******************************************************************************
         FindStart
******************************************************************************/

BOOL FindStart(RWYLIST *prwy, NAPT *pa, DWORD size, char *psz)
{	LOCATION loc;
	char chWk[16];
	RWYLIST *pL;

	while ((size > 6) && (size >= pa->nLen))
	{	__int32 nThisLen = pa->nLen;

		if (fUserAbort) return 0;
		
		if (pa->wId == OBJTYPE_START)
		{	// Start record found
			NSTART *ps = (NSTART *) pa;

			nThisLen = ps->nLen;

			if ((ps->des >> 4) == 1)
			{	// Runway start
				DecodeRwy(ps->num, ps->des & 15, chWk, 0, sizeof(chWk));
				if (strcmp(chWk, psz) == 0)
				{	fprintf(fpAFDS, "              Start %s: ", chWk); 
					SetLocPos(&loc, ps->nAlt, ps->nLat, ps->nLon, &prwy->r.fLat, &prwy->r.fLong, 0, 0);
					WritePosition(&loc, 1);
					fprintf(fpAFDS, " Hdg: %.1fT, Length %dft \n", (double) ps->fHeading, prwy->r.uLen);
	
					return TRUE;
				}
			}
		}

		size -= nThisLen;
		pa = (NAPT *) ((BYTE *) pa + nThisLen);
	}

	// See if there's an undeleted start from a previous scenery file
	pL = pR;
	while (pL) 
	{	if (fUserAbort) return FALSE;

		if ((memcmp(&prwy->r, &pL->r, 8) == 0) && (pL->fDelete == -1))
		{	// Found one without start deleted!
			fprintf(fpAFDS, "              Start %s re-used from previous deleted runway\n", psz); 

			prwy->r.fLat = pL->r.fLat;
			prwy->r.fLong = pL->r.fLong;
			prwy->nOffThresh = pL->nOffThresh;
			return TRUE;
		}
		
		pL = pL->pTo;
	}

	return FALSE;
}

/******************************************************************************
		DebugRwyAdditions
******************************************************************************/

// Type = 5 for primary, 6 for secondary
void DebugRwyAdditions(NAPT * pa, DWORD size)
{	while ((size > 6) && (size >= pa->nLen))
	{	__int32 nThisLen = pa->nLen, i = 0, x, j;
		char wk[256];
		BYTE* pb = (BYTE *) pa;

		if (fUserAbort) return;
		
		fprintf(fpAFDS, "### AFTER RWY: at OFFSET %08X ID=%04X LEN=%d\n",
			FileOffset32(&pa->wId), pa->wId, nThisLen);
		j = nThisLen;
		while (j--)
		{	if ((i & 15) == 0)
				x = sprintf(wk, "   %04X:", i);
			x += sprintf(&wk[x], " %02X", *pb++);
			if (((++i & 15) == 0) || (j == 0))
			{	fprintf(fpAFDS, "%s\n", wk);
				x = 0;
			}
		}

		if (nThisLen == 0) break; // Safety precaution (why needed?)
		size -= nThisLen;
		pa = (NAPT*)((BYTE*)pa + nThisLen);
	}
}

/******************************************************************************
         FindOffThresh
******************************************************************************/

// Type = 5 for primary, 6 for secondary: negative for MSFS (my conventio)
BOOL FindOffThresh(RWYLIST* prwy, NAPT* pa, DWORD size, short int nType)
{
	while ((size > 6) && (size >= pa->nLen))
	{	__int32 nThisLen = pa->nLen;

		if (fUserAbort) return 0;

		prwy->nOffThresh = 0;

		if ((pa->wId == nType) || (pa->wId == -nType))
		{	// Threshold record found
			NOFFTHR *ps = (NOFFTHR *) ((BYTE *) pa + ((nType < 0) ? 16 : 0));

			nThisLen = pa->nLen;
			prwy->nOffThresh = (unsigned __int32) ((ps->fLength * 3.28084F) + .5F);

			fprintf(fpAFDS, "              Offset Threshold %s: %d feet\n",
				(abs(nType) == 5) ? "primary" : "secondary", prwy->nOffThresh); 
			return TRUE;
		}

		if (nThisLen == 0) break; // Safety precaution (why needed?)
		size -= nThisLen;
		pa = (NAPT *) ((BYTE *) pa + nThisLen);
	}

	return FALSE;
}

/******************************************************************************
         FindVASI
******************************************************************************/

// Type = 11 for primary left, 12 for primary right, 13 sec left, 14 sec right
BOOL FindVASI(RWYLIST *prwy, NAPT *pa, DWORD size, WORD nType)
{	while (pa->nLen && (size > 6) && (size >= pa->nLen))
	{	__int32 nThisLen = pa->nLen;

		if (fUserAbort) return 0;

		// ########## ??? prwy->nOffThresh = 0;
		
		if (pa->wId == nType)
		{	// Start record found
			vasi_t *ps = (vasi_t *) pa;

			nThisLen = ps->nLen;
			if (nType & 1)
			{	prwy->r.bVASIleft = (BYTE) ps->wType;
				prwy->r.fLeftBiasX = ps->fBiasX;
				prwy->r.fLeftBiasZ = ps->fBiasZ;
				prwy->r.fLeftSpacing = ps->fSpacing;
				prwy->r.fLeftPitch = ps->fPitch;
			}
			else
			{	prwy->r.bVASIright = (BYTE) ps->wType;
				prwy->r.fRightBiasX = ps->fBiasX;
				prwy->r.fRightBiasZ = ps->fBiasZ;
				prwy->r.fRightSpacing = ps->fSpacing;
				prwy->r.fRightPitch = ps->fPitch;
			}
			return TRUE;
		}

		size -= nThisLen;
		pa = (NAPT *) ((BYTE *) pa + nThisLen);
	}

	return FALSE;
}

/******************************************************************************
         FindAppLights
******************************************************************************/

// Type = 15 primary, 16 secondary, or 0xDF, 0xE0 for MSFS
BOOL FindAppLights(RWYLIST *prwy, NAPT *pa, DWORD size, WORD nType)
{	while ((size > 6) && (size >= pa->nLen))
	{	__int32 nThisLen = pa->nLen;

		if (fUserAbort) return 0;

		// ########## ??? prwy->nOffThresh = 0;
		
		if (pa->wId == nType)
		{	// Applights record found
			approachlights_t *ps = (approachlights_t *) pa;

			nThisLen = ps->nLen;
			prwy->r.bAppLights = ps->bFlags;
			prwy->r.nStrobes = ps->bStrobes;
			return TRUE;
		}

		size -= nThisLen;
		pa = (NAPT *) ((BYTE *) pa + nThisLen);
	}

	return FALSE;
}
/******************************************************************************
         WriteFSM
******************************************************************************/

void WriteFSM(RWYLIST *pAp)
{	RWYLIST *pL = (RWYLIST *) malloc(sizeof(RWYLIST));
	if (pL)
	{	memcpy(pL, pAp, sizeof(RWYLIST));
		pAp->fAirport = 0;
		pAp->Atis = 0;

		fprintf(fpAFDS, "          FSM A/P %4s, lat=%.6f, long=%.6f, alt=%.2f\n",
			pAp->r.chICAO, (double) pAp->r.fLat, (double) pAp->r.fLong, pAp->r.fAlt);

		ProcessRunwayList(pL, TRUE, TRUE);
	}		
}

/******************************************************************************
         MakeTaxiwayList
******************************************************************************/

TWHDR *MakeTaxiwayList(NTAXIPT *pT, NTAXINM *pN, NTAXI *pP, WORD wT, WORD wN, WORD wP)
{	WORD wo = wN, w2;
	__int32 wp1, wp2;
	__int32 nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg = NULL;
	NGATE2 **ppg2 = NULL;
	LOCATION locg;

	if (pLastSetGateList)
	{	wGateCtr = (WORD) (*pLastSetGateList & 0xffff);
		ppg = (NGATE **) &pLastSetGateList[1];
		ppg2 = (NGATE2 **) (
			(((*pLastSetGateList >> 16) == OBJTYPE_NEWTAXIPARK) ||
				((*pLastSetGateList >> 16) == OBJTYPE_MSFSTAXIPARK)) ? ppg : 0); // ############### MSFSTAXIPARK ?? #########
	}

	if (!twh)
	{	fprintf(fpAFDS, "  ##### Error making Taxiway list: Need %d bytes memory block, not available!\n        (%d Points, %d Paths, %d Names)\n",
			nAllocSize, wT, wP, wN);
		return 0;
	}
	
	while (wo)
	{	WORD wMin = 0, wF = 0xffff;
		WORD w = 0, wPts = 0;
		BOOL fDone = FALSE;
		TW *tw = (TW *) &twh[1];
	
		while (w < wN)
		{	// Find next lowest taxiway name
			if (pN[w].szName[0] != 0xFF)
			{	if (w && (strncmp(pN[wMin].szName, pN[w].szName, 8) >= 0))
					wMin = w;
				fDone = TRUE;
			}

			w++;
		}

		if (!fDone) break; // All done now

		memcpy(twh->chName, pN[wMin].szName, 8);
		twh->fMaxWid = 0.0F;
		twh->fMinWid = 3000000.0F;
		twh->wPoints = wPts;

		// Search for 1st point in chained path fragment
		for (w = 0; w < wP; w++)
		{	if ((pP[w].bNumber == wMin) && 
					(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
						(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

						((pP[w].bDrawFlags == 3) &&
						(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
					))
			{	BOOL f1seen = FALSE;
				BOOL f2seen = FALSE;

				wp2 = (pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd;
				wp1 = pP[w].wStart;
				
				// See if this has a lone end
				for (w2 = 0; w2 < wP; w2++)
				{	if ((pP[w2].bNumber == wMin) && (w2 != w) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						))
					{	if ((pP[w2].wStart == wp1) || (pP[w2].wEnd == wp1))
							f1seen = TRUE;
						if ((pP[w2].wStart == wp2) || (pP[w2].wEnd == wp2))
							f2seen = TRUE;

						if (f1seen && f2seen)
							break;
					}
				}

				wF = w;
				
				if (!f1seen || !f2seen)
				{	if (f1seen)
					{	__int32 wrk = wp1;
						wp1 = wp2;
						wp2 = wrk;
					}
					
					break;
				}
			}
		}

		if (wF != 0xffff)
		{	// Start found, maybe (else circular or multiple)
			WORD wStart = wp1;
			char szTaxiway[9];
			szTaxiway[8] = 0;

			w = wF;

			if (wp1 >= 0)
			{	tw[wPts].bOrientation = pT[wp1].bOrientation;
				tw[wPts].bType = pT[wp1].bType;
				if (fNoDrawHoldConvert && (pT[wp1].bType == 5))
					// Convert "no draw" holds to normal types ### 4880
					tw[wPts].bType = 7;
				tw[wPts].fLat = pT[wp1].fLat;
				tw[wPts].fLon = pT[wp1].fLon;
			}

			else
			{	__int32 wp1x = (-wp1) - 1;
				if (ppg2 && ppg2[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg2[wp1x])).nLat,
						(*(ppg2[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else if (ppg && ppg[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg[wp1x])).nLat,
						(*(ppg[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else
				{
					tw[wPts].fLat = 0.0F;
					tw[wPts].fLon = 0.0F;
				}

				tw[wPts].bOrientation = 0;
				tw[wPts].bType = 5;
			}

			tw[wPts].fWid = pP[wF].fWidth;
			tw[wPts].bPtype = pP[w].bDrawFlags; // Path type
			twh->fMaxWid = twh->fMinWid = pP[w].fWidth;
			// if (pP[w].bDrawFlags != 1) twh->chName[0] = 0;// No names for non-taxi. #T#
			wPts++;
			strncpy(szTaxiway, twh->chName, 8);

			if (wp1 >= 0)
				fprintf(fpAFDS, "        TaxiWay %s: %d", szTaxiway, wp1);
			else
				fprintf(fpAFDS, "        TaxiWay %s: G%d", szTaxiway, (-wp1)-1);

			// Set next point and find further reference
			while (1)
			{	if (wp2 >= 0)
				{	tw[wPts].bOrientation = pT[wp2].bOrientation;
					tw[wPts].bType = pT[wp2].bType;
					if (fNoDrawHoldConvert && (pT[wp2].bType == 5))
						// Convert "no draw" holds to normal types ### 4880
						tw[wPts].bType = 7;
					tw[wPts].fLat = pT[wp2].fLat;
					tw[wPts].fLon = pT[wp2].fLon;
				}

				else
				{	__int32 wp2x = (-wp2)-1;
					if (ppg2 && ppg2[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg2[wp2x])).nLat,
							(*(ppg2[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else if (ppg && ppg[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg[wp2x])).nLat,
							(*(ppg[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else
					{
						tw[wPts].fLat = 0.0F;
						tw[wPts].fLon = 0.0F;
					}

					tw[wPts].bOrientation = 0;
					tw[wPts].bType = 5;
				}

				tw[wPts].bPtype = 0; // Path type
				tw[wPts].fWid = 0; // Terminate, for now
				wPts++;

				pP[w].bDrawFlags = 0; // Don't use again

				if (wp2 >= 0)
					fprintf(fpAFDS, "-%d", wp2);
				else
					fprintf(fpAFDS, "-G%d", (-wp2)-1);

				if (wp2 == wStart)
				{	fprintf(fpAFDS, " [looped]");
					break; // Looped path
				}

				// Find next reference, left or right ...
				for (w = 0; w < wP; w++)
				{	if ((pP[w].bNumber == wMin) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						) &&
						
							(	(pP[w].wStart == wp2) || 
								((pP[w].bDrawFlags != 3) && (pP[w].wEnd == wp2))
							))
						break;
				}

				if (w >= wP)
					break;

				tw[wPts-1].bPtype = pP[w].bDrawFlags; // Path type
				tw[wPts-1].fWid = pP[w].fWidth;
				if (twh->fMaxWid < pP[w].fWidth)
					twh->fMaxWid = pP[w].fWidth;
				if (twh->fMinWid > pP[w].fWidth)
					twh->fMinWid = pP[w].fWidth;
			
				wp2 = (pP[w].wStart == wp2) ?
							((pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd) :
							pP[w].wStart;
			}

			fprintf(fpAFDS, "\n");
		}

		else
		{	// Done with this path name now.
			pN[wMin].szName[0] = 0xFF;
			wo--;
		}

		twh->wPoints = wPts;
		if (wPts) twh = (TWHDR *) &tw[wPts];
	}

	twh->wPoints = 0; // terminator

{
	__int32 nTaxiwayDataSize = (__int32)((BYTE *) &twh[1] - (BYTE *) twh0);

	if ((nTaxiwayDataSize > 0) && (nAllocSize > nTaxiwayDataSize))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + nTaxiwayDataSize);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, nTaxiwayDataSize);
			free(twh0);
			twh0 = twh1;
		}
	}
}
	return twh0;
}

/******************************************************************************
         MakeTaxiwayList2 for sloped taxiways
******************************************************************************/

TWHDR *MakeTaxiwayList2(NEWTAXIPT *pT, NTAXINM *pN, NTAXI *pP, WORD wT, WORD wN, WORD wP)
{	WORD wo = wN, w2;
	__int32 wp1, wp2;
	__int32 nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg = NULL;
	NGATE3 **ppg3 = NULL;
	LOCATION locg;

	if (pLastSetGateList)
	{	wGateCtr = (WORD) (*pLastSetGateList & 0xffff);
		ppg = (NGATE **) &pLastSetGateList[1];
		ppg3 = (NGATE3 **) (((*pLastSetGateList >> 16) == OBJTYPE_NEWNEWTAXIPARK) ? ppg : 0);
	}

	if (!twh)
	{	fprintf(fpAFDS, "  ##### Error making Taxiway list: Need %d bytes memory block, not available!\n        (%d Points, %d Paths, %d Names)\n",
			nAllocSize, wT, wP, wN);
		return 0;
	}
	
	while (wo)
	{	WORD wMin = 0, wF = 0xffff;
		WORD w = 0, wPts = 0;
		BOOL fDone = FALSE;
		TW *tw = (TW *) &twh[1];
	
		while (w < wN)
		{	// Find next lowest taxiway name
			if (pN[w].szName[0] != 0xFF)
			{	if (w && (strncmp(pN[wMin].szName, pN[w].szName, 8) >= 0))
					wMin = w;
				fDone = TRUE;
			}

			w++;
		}

		if (!fDone) break; // All done now

		memcpy(twh->chName, pN[wMin].szName, 8);
		twh->fMaxWid = 0.0F;
		twh->fMinWid = 3000000.0F;
		twh->wPoints = wPts;

		// Search for 1st point in chained path fragment
		for (w = 0; w < wP; w++)
		{	if ((pP[w].bNumber == wMin) && 
					(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
						(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

						((pP[w].bDrawFlags == 3) &&
						(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
					))
			{	BOOL f1seen = FALSE;
				BOOL f2seen = FALSE;

				wp2 = (pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd;
				wp1 = pP[w].wStart;
				
				// See if this has a lone end
				for (w2 = 0; w2 < wP; w2++)
				{	if ((pP[w2].bNumber == wMin) && (w2 != w) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						))
					{	if ((pP[w2].wStart == wp1) || (pP[w2].wEnd == wp1))
							f1seen = TRUE;
						if ((pP[w2].wStart == wp2) || (pP[w2].wEnd == wp2))
							f2seen = TRUE;

						if (f1seen && f2seen)
							break;
					}
				}

				wF = w;
				
				if (!f1seen || !f2seen)
				{	if (f1seen)
					{	__int32 wrk = wp1;
						wp1 = wp2;
						wp2 = wrk;
					}
					
					break;
				}
			}
		}

		if (wF != 0xffff)
		{	// Start found, maybe (else circular or multiple)
			WORD wStart = wp1;
			char szTaxiway[9];
			szTaxiway[8] = 0;

			w = wF;

			if (wp1 >= 0)
			{	tw[wPts].bOrientation = pT[wp1].bOrientation;
				tw[wPts].bType = pT[wp1].bType;
				if (fNoDrawHoldConvert && (pT[wp1].bType == 5))
					// Convert "no draw" holds to normal types ### 4880
					tw[wPts].bType = 7;
				tw[wPts].fLat = pT[wp1].fLat;
				tw[wPts].fLon = pT[wp1].fLon;
			}

			else
			{	__int32 wp1x = (-wp1) - 1;

				if (ppg3 && ppg3[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg3[wp1x])).nLat,
						(*(ppg3[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else if (ppg && ppg[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg[wp1x])).nLat,
						(*(ppg[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else
				{
					tw[wPts].fLat = 0.0F;
					tw[wPts].fLon = 0.0F;
				}

				tw[wPts].bOrientation = 0;
				tw[wPts].bType = 5;
			}

			tw[wPts].fWid = pP[wF].fWidth;
			tw[wPts].bPtype = pP[w].bDrawFlags; // Path type
			twh->fMaxWid = twh->fMinWid = pP[w].fWidth;
			// if (pP[w].bDrawFlags != 1) twh->chName[0] = 0;// No names for non-taxi. #T#
			wPts++;
			strncpy(szTaxiway, twh->chName, 8);

			if (wp1 >= 0)
				fprintf(fpAFDS, "        TaxiWay %s: %d", szTaxiway, wp1);
			else
				fprintf(fpAFDS, "        TaxiWay %s: G%d", szTaxiway, (-wp1)-1);

			// Set next point and find further reference
			while (1)
			{	if (wp2 >= 0)
				{	tw[wPts].bOrientation = pT[wp2].bOrientation;
					tw[wPts].bType = pT[wp2].bType;
					if (fNoDrawHoldConvert && (pT[wp2].bType == 5))
						// Convert "no draw" holds to normal types ### 4880
						tw[wPts].bType = 7;
					tw[wPts].fLat = pT[wp2].fLat;
					tw[wPts].fLon = pT[wp2].fLon;
				}

				else
				{	__int32 wp2x = (-wp2) - 1;

					if (ppg3 && ppg3[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg3[wp2x])).nLat,
							(*(ppg3[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else if (ppg && ppg[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg[wp2x])).nLat,
							(*(ppg[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else
					{
						tw[wPts].fLat = 0.0F;
						tw[wPts].fLon = 0.0F;
					}

					tw[wPts].bOrientation = 0;
					tw[wPts].bType = 5;
				}

				tw[wPts].bPtype = 0; // Path type
				tw[wPts].fWid = 0; // Terminate, for now
				wPts++;

				pP[w].bDrawFlags = 0; // Don't use again

				if (wp2 >= 0)
					fprintf(fpAFDS, "-%d", wp2);
				else
					fprintf(fpAFDS, "-G%d", (-wp2)-1);

				if (wp2 == wStart)
				{	fprintf(fpAFDS, " [looped]");
					break; // Looped path
				}

				// Find next reference, left or right ...
				for (w = 0; w < wP; w++)
				{	if ((pP[w].bNumber == wMin) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						) &&
						
							(	(pP[w].wStart == wp2) || 
								((pP[w].bDrawFlags != 3) && (pP[w].wEnd == wp2))
							))
						break;
				}

				if (w >= wP)
					break;

				tw[wPts-1].bPtype = pP[w].bDrawFlags; // Path type
				tw[wPts-1].fWid = pP[w].fWidth;
				if (twh->fMaxWid < pP[w].fWidth)
					twh->fMaxWid = pP[w].fWidth;
				if (twh->fMinWid > pP[w].fWidth)
					twh->fMinWid = pP[w].fWidth;
			
				wp2 = (pP[w].wStart == wp2) ?
							((pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd) :
							pP[w].wStart;
			}

			fprintf(fpAFDS, "\n");
		}

		else
		{	// Done with this path name now.
			pN[wMin].szName[0] = 0xFF;
			wo--;
		}

		twh->wPoints = wPts;
		if (wPts) twh = (TWHDR *) &tw[wPts];
	}

	twh->wPoints = 0; // terminator

{
	__int32 nTaxiwayDataSize = (__int32)((BYTE *) &twh[1] - (BYTE *) twh0);

	if ((nTaxiwayDataSize > 0) && (nAllocSize > nTaxiwayDataSize))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + nTaxiwayDataSize);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, nTaxiwayDataSize);
			free(twh0);
			twh0 = twh1;
		}
	}
}
	return twh0;
}

/******************************************************************************
         NewMakeTaxiwayList
******************************************************************************/

TWHDR *NewMakeTaxiwayList(NTAXIPT *pT, NTAXINM *pN, NEWNTAXI *pP, WORD wT, WORD wN, WORD wP)
{	WORD wo = wN, w2;
	__int32 wp1, wp2;
	__int32 nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg = NULL;
	NGATE2 **ppg2 = NULL;
	LOCATION locg;

	if (pLastSetGateList)
	{	wGateCtr = (WORD) (*pLastSetGateList & 0xffff);
		ppg = (NGATE **) &pLastSetGateList[1];
		ppg2 = (NGATE2 **) (((*pLastSetGateList >> 16) == OBJTYPE_NEWTAXIPARK) ? ppg : 0);
		// ############### MSFSTAXIPARK ?? #########
	}

	if (!twh)
	{	fprintf(fpAFDS, "  ##### Error making Taxiway list: Need %d bytes memory block, not available!\n        (%d Points, %d Paths, %d Names)\n",
			nAllocSize, wT, wP, wN);
		return 0;
	}
	
	while (wo)
	{	WORD wMin = 0, wF = 0xffff;
		WORD w = 0, wPts = 0;
		BOOL fDone = FALSE;
		TW *tw = (TW *) &twh[1];
	
		while (w < wN)
		{	// Find next lowest taxiway name
			if (pN[w].szName[0] != 0xFF)
			{	if (w && (strncmp(pN[wMin].szName, pN[w].szName, 8) >= 0))
					wMin = w;
				fDone = TRUE;
			}

			w++;
		}

		if (!fDone) break; // All done now

		memcpy(twh->chName, pN[wMin].szName, 8);
		twh->fMaxWid = 0.0F;
		twh->fMinWid = 3000000.0F;
		twh->wPoints = wPts;

		// Search for 1st point in chained path fragment
		for (w = 0; w < wP; w++)
		{	if ((pP[w].bNumber == wMin) && 
					(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
						(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

						((pP[w].bDrawFlags == 3) &&
						(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
					))
			{	BOOL f1seen = FALSE;
				BOOL f2seen = FALSE;

				wp2 = (pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd;
				wp1 = pP[w].wStart;
				
				// See if this has a lone end
				for (w2 = 0; w2 < wP; w2++)
				{	if ((pP[w2].bNumber == wMin) && (w2 != w) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						))
					{	if ((pP[w2].wStart == wp1) || (pP[w2].wEnd == wp1))
							f1seen = TRUE;
						if ((pP[w2].wStart == wp2) || (pP[w2].wEnd == wp2))
							f2seen = TRUE;

						if (f1seen && f2seen)
							break;
					}
				}

				wF = w;
				
				if (!f1seen || !f2seen)
				{	if (f1seen)
					{	__int32 wrk = wp1;
						wp1 = wp2;
						wp2 = wrk;
					}
					
					break;
				}
			}
		}

		if (wF != 0xffff)
		{	// Start found, maybe (else circular or multiple)
			WORD wStart = wp1;
			char szTaxiway[9];
			szTaxiway[8] = 0;

			w = wF;

			if (wp1 >= 0)
			{	tw[wPts].bOrientation = pT[wp1].bOrientation;
				tw[wPts].bType = pT[wp1].bType;
				if (fNoDrawHoldConvert && (pT[wp1].bType == 5))
					// Convert "no draw" holds to normal types ### 4880
					tw[wPts].bType = 7;
				tw[wPts].fLat = pT[wp1].fLat;
				tw[wPts].fLon = pT[wp1].fLon;
			}

			else
			{	__int32 wp1x = (-wp1) - 1;

				if (ppg2 && ppg2[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg2[wp1x])).nLat,
						(*(ppg2[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else if (ppg && ppg[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg[wp1x])).nLat,
						(*(ppg[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else
				{
					tw[wPts].fLat = 0.0F;
					tw[wPts].fLon = 0.0F;
				}

				tw[wPts].bOrientation = 0;
				tw[wPts].bType = 5;
			}

			tw[wPts].fWid = pP[wF].fWidth;
			tw[wPts].bPtype = pP[w].bDrawFlags; // Path type
			twh->fMaxWid = twh->fMinWid = pP[w].fWidth;
			// if (pP[w].bDrawFlags != 1) twh->chName[0] = 0;// No names for non-taxi. #T#
			wPts++;
			strncpy(szTaxiway, twh->chName, 8);

			if (wp1 >= 0)
				fprintf(fpAFDS, "        TaxiWay %s: %d", szTaxiway, wp1);
			else
				fprintf(fpAFDS, "        TaxiWay %s: G%d", szTaxiway, (-wp1)-1);

			// Set next point and find further reference
			while (1)
			{	if (wp2 >= 0)
				{	tw[wPts].bOrientation = pT[wp2].bOrientation;
					tw[wPts].bType = pT[wp2].bType;
					if (fNoDrawHoldConvert && (pT[wp2].bType == 5))
						// Convert "no draw" holds to normal types ### 4880
						tw[wPts].bType = 7;
					tw[wPts].fLat = pT[wp2].fLat;
					tw[wPts].fLon = pT[wp2].fLon;
				}

				else
				{	__int32 wp2x = (-wp2) - 1;

					if (ppg2 && ppg2[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg2[wp2x])).nLat,
							(*(ppg2[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else if (ppg && ppg[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg[wp2x])).nLat,
							(*(ppg[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else
					{
						tw[wPts].fLat = 0.0F;
						tw[wPts].fLon = 0.0F;
					}

					tw[wPts].bOrientation = 0;
					tw[wPts].bType = 5;
				}

				tw[wPts].bPtype = 0; // Path type
				tw[wPts].fWid = 0; // Terminate, for now
				wPts++;

				pP[w].bDrawFlags = 0; // Don't use again

				if (wp2 >= 0)
					fprintf(fpAFDS, "-%d", wp2);
				else
					fprintf(fpAFDS, "-G%d", (-wp2)-1);

				if (wp2 == wStart)
				{	fprintf(fpAFDS, " [looped]");
					break; // Looped path
				}

				// Find next reference, left or right ...
				for (w = 0; w < wP; w++)
				{	if ((pP[w].bNumber == wMin) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						) &&
						
							(	(pP[w].wStart == wp2) || 
								((pP[w].bDrawFlags != 3) && (pP[w].wEnd == wp2))
							))
						break;
				}

				if (w >= wP)
					break;

				tw[wPts-1].bPtype = pP[w].bDrawFlags; // Path type
				tw[wPts-1].fWid = pP[w].fWidth;
				if (twh->fMaxWid < pP[w].fWidth)
					twh->fMaxWid = pP[w].fWidth;
				if (twh->fMinWid > pP[w].fWidth)
					twh->fMinWid = pP[w].fWidth;
			
				wp2 = (pP[w].wStart == wp2) ?
							((pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd) :
							pP[w].wStart;
			}

			fprintf(fpAFDS, "\n");
		}

		else
		{	// Done with this path name now.
			pN[wMin].szName[0] = 0xFF;
			wo--;
		}

		twh->wPoints = wPts;
		if (wPts) twh = (TWHDR *) &tw[wPts];
	}

	twh->wPoints = 0; // terminator

{
	__int32 nTaxiwayDataSize = (__int32)((BYTE *) &twh[1] - (BYTE *) twh0);

	if ((nTaxiwayDataSize > 0) && (nAllocSize > nTaxiwayDataSize))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + nTaxiwayDataSize);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, nTaxiwayDataSize);
			free(twh0);
			twh0 = twh1;
		}
	}
}
	return twh0;
}

/******************************************************************************
         NewMakeTaxiwayList2 for sloped taxiways
******************************************************************************/

TWHDR *NewMakeTaxiwayList2(NEWTAXIPT *pT, NTAXINM *pN, NEWNTAXI2 *pP, WORD wT, WORD wN, WORD wP)
{	WORD wo = wN, w2;
	__int32 wp1, wp2;
	__int32 nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg = NULL;
	NGATE2 **ppg2 = NULL;
	NGATE3 **ppg3 = NULL;
	LOCATION locg;

	if (pLastSetGateList)
	{	wGateCtr = (WORD) (*pLastSetGateList & 0xffff);
		ppg = (NGATE **) &pLastSetGateList[1];
		ppg2 = (NGATE2 **) (((*pLastSetGateList >> 16) == OBJTYPE_NEWTAXIPARK) ? ppg : 0);
		ppg3 = (NGATE3 **) (((*pLastSetGateList >> 16) == OBJTYPE_NEWNEWTAXIPARK) ? ppg : 0);
		// ############### MSFSTAXIPARK ?? #########
	}

	if (!twh)
	{	fprintf(fpAFDS, "  ##### Error making Taxiway list: Need %d bytes memory block, not available!\n        (%d Points, %d Paths, %d Names)\n",
			nAllocSize, wT, wP, wN);
		return 0;
	}
	
	while (wo)
	{	WORD wMin = 0, wF = 0xffff;
		WORD w = 0, wPts = 0;
		BOOL fDone = FALSE;
		TW *tw = (TW *) &twh[1];
	
		while (w < wN)
		{	// Find next lowest taxiway name
			if (pN[w].szName[0] != 0xFF)
			{	if (w && (strncmp(pN[wMin].szName, pN[w].szName, 8) >= 0))
					wMin = w;
				fDone = TRUE;
			}

			w++;
		}

		if (!fDone) break; // All done now

		memcpy(twh->chName, pN[wMin].szName, 8);
		twh->fMaxWid = 0.0F;
		twh->fMinWid = 3000000.0F;
		twh->wPoints = wPts;

		// Search for 1st point in chained path fragment
		for (w = 0; w < wP; w++)
		{	if ((pP[w].bNumber == wMin) && 
					(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
						(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

						((pP[w].bDrawFlags == 3) &&
						(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
					))
			{	BOOL f1seen = FALSE;
				BOOL f2seen = FALSE;

				wp2 = (pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd;
				wp1 = pP[w].wStart;
				
				// See if this has a lone end
				for (w2 = 0; w2 < wP; w2++)
				{	if ((pP[w2].bNumber == wMin) && (w2 != w) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						))
					{	if ((pP[w2].wStart == wp1) || (pP[w2].wEnd == wp1))
							f1seen = TRUE;
						if ((pP[w2].wStart == wp2) || (pP[w2].wEnd == wp2))
							f2seen = TRUE;

						if (f1seen && f2seen)
							break;
					}
				}

				wF = w;
				
				if (!f1seen || !f2seen)
				{	if (f1seen)
					{	__int32 wrk = wp1;
						wp1 = wp2;
						wp2 = wrk;
					}
					
					break;
				}
			}
		}

		if (wF != 0xffff)
		{	// Start found, maybe (else circular or multiple)
			WORD wStart = wp1;
			char szTaxiway[9];
			szTaxiway[8] = 0;

			w = wF;

			if (wp1 >= 0)
			{	tw[wPts].bOrientation = pT[wp1].bOrientation;
				tw[wPts].bType = pT[wp1].bType;
				if (fNoDrawHoldConvert && (pT[wp1].bType == 5))
					// Convert "no draw" holds to normal types ### 4880
					tw[wPts].bType = 7;
				tw[wPts].fLat = pT[wp1].fLat;
				tw[wPts].fLon = pT[wp1].fLon;
			}

			else
			{	__int32 wp1x = (-wp1) - 1;

				if (ppg3 && ppg3[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg3[wp1x])).nLat,
						(*(ppg3[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else if (ppg2 && ppg2[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg2[wp1x])).nLat,
						(*(ppg2[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else if (ppg && ppg[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg[wp1x])).nLat,
						(*(ppg[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else
				{
					tw[wPts].fLat = 0.0F;
					tw[wPts].fLon = 0.0F;
				}

				tw[wPts].bOrientation = 0;
				tw[wPts].bType = 5;
			}

			tw[wPts].fWid = pP[wF].fWidth;
			tw[wPts].bPtype = pP[w].bDrawFlags; // Path type
			twh->fMaxWid = twh->fMinWid = pP[w].fWidth;
			// if (pP[w].bDrawFlags != 1) twh->chName[0] = 0;// No names for non-taxi. #T#
			wPts++;
			strncpy(szTaxiway, twh->chName, 8);

			if (wp1 >= 0)
				fprintf(fpAFDS, "        TaxiWay %s: %d", szTaxiway, wp1);
			else
				fprintf(fpAFDS, "        TaxiWay %s: G%d", szTaxiway, (-wp1)-1);

			// Set next point and find further reference
			while (1)
			{	if (wp2 >= 0)
				{	tw[wPts].bOrientation = pT[wp2].bOrientation;
					tw[wPts].bType = pT[wp2].bType;
					if (fNoDrawHoldConvert && (pT[wp2].bType == 5))
						// Convert "no draw" holds to normal types ### 4880
						tw[wPts].bType = 7;
					tw[wPts].fLat = pT[wp2].fLat;
					tw[wPts].fLon = pT[wp2].fLon;
				}

				else
				{	__int32 wp2x = (-wp2) - 1;

					if (ppg3 && ppg3[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg3[wp2x])).nLat,
							(*(ppg3[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else if (ppg2 && ppg2[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg2[wp2x])).nLat,
							(*(ppg2[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else if (ppg && ppg[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg[wp2x])).nLat,
							(*(ppg[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}
					else
					{
						tw[wPts].fLat = 0.0F;
						tw[wPts].fLon = 0.0F;
					}

					tw[wPts].bOrientation = 0;
					tw[wPts].bType = 5;
				}

				tw[wPts].bPtype = 0; // Path type
				tw[wPts].fWid = 0; // Terminate, for now
				wPts++;

				pP[w].bDrawFlags = 0; // Don't use again

				if (wp2 >= 0)
					fprintf(fpAFDS, "-%d", wp2);
				else
					fprintf(fpAFDS, "-G%d", (-wp2)-1);

				if (wp2 == wStart)
				{	fprintf(fpAFDS, " [looped]");
					break; // Looped path
				}

				// Find next reference, left or right ...
				for (w = 0; w < wP; w++)
				{	if ((pP[w].bNumber == wMin) &&
						(	(((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
							(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
						) &&
						
							(	(pP[w].wStart == wp2) || 
								((pP[w].bDrawFlags != 3) && (pP[w].wEnd == wp2))
							))
						break;
				}

				if (w >= wP)
					break;

				tw[wPts-1].bPtype = pP[w].bDrawFlags; // Path type
				tw[wPts-1].fWid = pP[w].fWidth;
				if (twh->fMaxWid < pP[w].fWidth)
					twh->fMaxWid = pP[w].fWidth;
				if (twh->fMinWid > pP[w].fWidth)
					twh->fMinWid = pP[w].fWidth;
			
				wp2 = (pP[w].wStart == wp2) ?
							((pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd) :
							pP[w].wStart;
			}

			fprintf(fpAFDS, "\n");
		}

		else
		{	// Done with this path name now.
			pN[wMin].szName[0] = 0xFF;
			wo--;
		}

		twh->wPoints = wPts;
		if (wPts) twh = (TWHDR *) &tw[wPts];
	}

	twh->wPoints = 0; // terminator

{
	__int32 nTaxiwayDataSize = (__int32)((BYTE *) &twh[1] - (BYTE *) twh0);

	if ((nTaxiwayDataSize > 0) && (nAllocSize > nTaxiwayDataSize))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + nTaxiwayDataSize);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, nTaxiwayDataSize);
			free(twh0);
			twh0 = twh1;
		}
	}
}
	return twh0;
}

/******************************************************************************
		 NewMakeTaxiwayList3 for MSFS
******************************************************************************/

TWHDR* NewMakeTaxiwayList3(NTAXIPT* pT, NTAXINM* pN, MSFSNTAXI* pP, WORD wT, WORD wN, WORD wP)
{
	WORD wo = wN, w2;
	__int32 wp1, wp2;
	__int32 nAllocSize = (40 * wP) + (32 * wT * wP);
	TWHDR* twh = (TWHDR*)malloc(nAllocSize);
	TWHDR* twh0 = twh;

	WORD wGateCtr = 0;
	NGATE2** ppg4 = NULL;
	LOCATION locg;

	if (pLastSetGateList)
	{	wGateCtr = (WORD)(*pLastSetGateList & 0xffff);
		ppg4 = (NGATE2**)&pLastSetGateList[1];	
	}

	if (!twh)
	{
		fprintf(fpAFDS, "  ##### Error making Taxiway list: Need %d bytes memory block, not available!\n        (%d Points, %d Paths, %d Names)\n",
			nAllocSize, wT, wP, wN);
		return 0;
	}

	while (wo)
	{
		WORD wMin = 0, wF = 0xffff;
		WORD w = 0, wPts = 0;
		BOOL fDone = FALSE;
		TW* tw = (TW*)&twh[1];

		while (w < wN)
		{	// Find next lowest taxiway name
			if (pN[w].szName[0] != 0xFF)
			{
				if (w && (strncmp(pN[wMin].szName, pN[w].szName, 8) >= 0))
					wMin = w;
				fDone = TRUE;
			}

			w++;
		}

		if (!fDone) break; // All done now

		memcpy(twh->chName, pN[wMin].szName, 8);
		twh->fMaxWid = 0.0F;
		twh->fMinWid = 3000000.0F;
		twh->wPoints = wPts;

		// Search for 1st point in chained path fragment
		for (w = 0; w < wP; w++)
		{
			if ((pP[w].bNumber == wMin) &&
				((((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
					(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

					((pP[w].bDrawFlags == 3) &&
						(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
					))
			{
				BOOL f1seen = FALSE;
				BOOL f2seen = FALSE;

				wp2 = (pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd;
				wp1 = pP[w].wStart;

				// See if this has a lone end
				for (w2 = 0; w2 < wP; w2++)
				{
					if ((pP[w2].bNumber == wMin) && (w2 != w) &&
						((((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
								(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
							))
					{
						if ((pP[w2].wStart == wp1) || (pP[w2].wEnd == wp1))
							f1seen = TRUE;
						if ((pP[w2].wStart == wp2) || (pP[w2].wEnd == wp2))
							f2seen = TRUE;

						if (f1seen && f2seen)
							break;
					}
				}

				wF = w;

				if (!f1seen || !f2seen)
				{
					if (f1seen)
					{
						__int32 wrk = wp1;
						wp1 = wp2;
						wp2 = wrk;
					}

					break;
				}
			}
		}

		if (wF != 0xffff)
		{	// Start found, maybe (else circular or multiple)
			WORD wStart = wp1;
			char szTaxiway[9];
			szTaxiway[8] = 0;

			w = wF;

			if (wp1 >= 0)
			{
				tw[wPts].bOrientation = pT[wp1].bOrientation;
				tw[wPts].bType = pT[wp1].bType;
				if (fNoDrawHoldConvert && (pT[wp1].bType == 5))
					// Convert "no draw" holds to normal types ### 4880
					tw[wPts].bType = 7;
				tw[wPts].fLat = pT[wp1].fLat;
				tw[wPts].fLon = pT[wp1].fLon;
			}

			else
			{
				__int32 wp1x = (-wp1) - 1;

				if (ppg4 && ppg4[wp1x])
				{
					SetLocPos(&locg, 0,
						(*(ppg4[wp1x])).nLat, (*(ppg4[wp1x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
				}
				else
				{
					tw[wPts].fLat = 0.0F;
					tw[wPts].fLon = 0.0F;
				}

				tw[wPts].bOrientation = 0;
				tw[wPts].bType = 5;
			}

			tw[wPts].fWid = pP[wF].fWidth;
			tw[wPts].bPtype = pP[w].bDrawFlags; // Path type
			twh->fMaxWid = twh->fMinWid = pP[w].fWidth;
			// if (pP[w].bDrawFlags != 1) twh->chName[0] = 0;// No names for non-taxi. #T#
			wPts++;
			strncpy(szTaxiway, twh->chName, 8);

			if (wp1 >= 0)
				fprintf(fpAFDS, "        TaxiWay %s: %d", szTaxiway, wp1);
			else
				fprintf(fpAFDS, "        TaxiWay %s: G%d", szTaxiway, (-wp1) - 1);

			// Set next point and find further reference
			while (1)
			{
				if (wp2 >= 0)
				{
					tw[wPts].bOrientation = pT[wp2].bOrientation;
					tw[wPts].bType = pT[wp2].bType;
					if (fNoDrawHoldConvert && (pT[wp2].bType == 5))
						// Convert "no draw" holds to normal types ### 4880
						tw[wPts].bType = 7;
					tw[wPts].fLat = pT[wp2].fLat;
					tw[wPts].fLon = pT[wp2].fLon;
				}

				else
				{
					__int32 wp2x = (-wp2) - 1;
					if (ppg4 && ppg4[wp2x])
					{
						SetLocPos(&locg, 0,
							(*(ppg4[wp2x])).nLat, (*(ppg4[wp2x])).nLon,
							&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
					}

					//else fprintf(fpAFDS, "### ERROR in TaxiWay: (wp2x) %d\n", wp2x);

					tw[wPts].bOrientation = 0;
					tw[wPts].bType = 5;
				}

				tw[wPts].bPtype = 0; // Path type
				tw[wPts].fWid = 0; // Terminate, for now
				wPts++;

				pP[w].bDrawFlags = 0; // Don't use again

				if (wp2 >= 0)
					fprintf(fpAFDS, "-%d", wp2);
				else
					fprintf(fpAFDS, "-G%d", (-wp2) - 1);

				if (wp2 == wStart)
				{
					fprintf(fpAFDS, " [looped]");
					break; // Looped path
				}

				// Find next reference, left or right ...
				for (w = 0; w < wP; w++)
				{
					if ((pP[w].bNumber == wMin) &&
						((((pP[w].bDrawFlags == 1) || (pP[w].bDrawFlags == 4)) &&					// #T# TAXIWAY TYPE #T#
							(pP[w].wEnd < wT) && (pP[w].wStart < wT)) ||

							((pP[w].bDrawFlags == 3) &&
								(pP[w].wEnd < wGateCtr) && (pP[w].wStart < wT))
							) &&

						((pP[w].wStart == wp2) ||
							((pP[w].bDrawFlags != 3) && (pP[w].wEnd == wp2))
							))
						break;
				}

				if (w >= wP)
					break;

				tw[wPts - 1].bPtype = pP[w].bDrawFlags; // Path type
				tw[wPts - 1].fWid = pP[w].fWidth;
				if (twh->fMaxWid < pP[w].fWidth)
					twh->fMaxWid = pP[w].fWidth;
				if (twh->fMinWid > pP[w].fWidth)
					twh->fMinWid = pP[w].fWidth;

				wp2 = (pP[w].wStart == wp2) ?
					((pP[w].bDrawFlags == 3) ? -(pP[w].wEnd + 1) : pP[w].wEnd) :
					pP[w].wStart;
			}

			fprintf(fpAFDS, "\n");
		}

		else
		{	// Done with this path name now.
			pN[wMin].szName[0] = 0xFF;
			wo--;
		}

		twh->wPoints = wPts;
		if (wPts) twh = (TWHDR*)&tw[wPts];
	}

	twh->wPoints = 0; // terminator

{
	__int32 nTaxiwayDataSize = (__int32)((BYTE *) &twh[1] - (BYTE *) twh0);

	if ((nTaxiwayDataSize > 0) && (nAllocSize > nTaxiwayDataSize))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + nTaxiwayDataSize);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, nTaxiwayDataSize);
			free(twh0);
			twh0 = twh1;
		}
	}
}
	return twh0;
}

/******************************************************************************
         copyxmlstring
******************************************************************************/

void copyxmlstring(char *pTo, char *pFrom)
{	char ch, *pToOrig = pTo;
	while (ch = *pFrom)
	{	if (ch == '&')
		{	*pTo++ = 'a';
			*pTo++ = 'n';
			*pTo++ = 'd';
		}
		else if (ch == '<')
			*pTo++ = '(';
		else if (ch == '>')
			*pTo++ = ')';
		else if (ch == '/')
			*pTo++ = '-';
		else if ((ch != '\'') && (ch != '\"'))
			*pTo++ = ch;
		pFrom++;
	}

	*pTo = 0;
	str2ascii(pToOrig);
}

/******************************************************************************
         CorrectRunwayMagvar
******************************************************************************/

void CorrectRunwayMagvar(char *pchICAO, float fNewMagvar)
{	if (pR)
	{	RWYLIST *p = pR;
		while (p)
		{	if (!p->fDelete && p->fAirport && (_strnicmp(pchICAO, p->r.chICAO, 4) == 0)) // WAS !p->fAirport
			{	float fILShdg = (float) atof(p->r.chILSHdg);
				char chILS[16];
				p->fHdg += p->fMagvar - fNewMagvar;
				if (p->fHdg < 0) p->fHdg += 360.0f;
				fILShdg += p->fMagvar - fNewMagvar;
				if (fILShdg < 0) fILShdg += 360.0f;
				sprintf(chILS, "%.4f", fILShdg);
				strncpy(p->r.chILSHdg, chILS, 5);
				p->r.chILSHdg[5] = 0;
				p->fMagvar = fNewMagvar;
			}
			p = p->pTo;
		}
	}
}

/******************************************************************************
		 FindILSdetails
******************************************************************************/

 void FindILSdetails(DWORD nObjs, NSECTS* ps, BYTE* p, char* psz, RWYLIST* prwy, __int32 nMode)
 {
 	float fILSHdgMag;
	__int32 nFreq;

	fMatchedGlobalNvxIlsV9 = FALSE;
	chMatchedGlobalNvxIlsV9Source[0] = 0;

	if (pMSFS2024BGLV9Data && (nMode == 0))
		nFreq = MatchILSV9(psz, prwy);
	else
		nFreq = MatchILS(nObjs, ps, p, psz, prwy, nMode);

	if (!nFreq && (nMode == 0))
		nFreq = MatchGlobalNvxIlsV9(psz, prwy);
 
	if (!nFreq)
		return;

	if (!fMatchedILS)
		return;

	sprintf(prwy->r.chILS, "%.2f", nFreq / 100.0);
	fILSHdgMag = fILSheading - prwy->fMagvar;
	if (fILSHdgMag <= 0.0F) fILSHdgMag += 360.0F;
	else if (fILSHdgMag > 360.0F) fILSHdgMag -= 360.0F;

	char wk[8] = { 0 };
	__int32 l = sprintf_s(wk, sizeof(wk), "%.3f", (double)fILSHdgMag);

	if (l < 0)
		wk[0] = 0;
	else if (l > 5)
		wk[5] = 0;
	else if ((l > 0) && (wk[l - 1] == '0'))
		wk[l - 1] = 0;

	memset(prwy->r.chILSHdg, 0, sizeof(prwy->r.chILSHdg));
	memcpy(prwy->r.chILSHdg, wk, sizeof(prwy->r.chILSHdg));

	prwy->r.fILSslope = fILSslope;

	fprintf(fpAFDS, "\n              ");
	fprintf(fpAFDS, "ILS: %s  %s Hdg: %.1f %s%s%s%s  Slope: %.2f, \x22%s\x22",
		psz, prwy->r.chILS, (double)fILSheading,
		(prwy->fILSflags & 0x1C) ? ", Flags:" : "",
		(prwy->fILSflags & 0x08) ? " GS" : "",
		(prwy->fILSflags & 0x10) ? " DME" : "",
		(prwy->fILSflags & 0x04) ? " BC" : "",
		(double) prwy->r.fILSslope, chNameILS);

	if (fMatchedGlobalNvxIlsV9)
	{
		fprintf(fpAFDS, " [global NVX/NAX: %s]",
			chMatchedGlobalNvxIlsV9Source[0]
				? chMatchedGlobalNvxIlsV9Source
				: "unknown source");
	}
		
	strncpy(prwy->r.chNameILS, chNameILS, 31);
	prwy->r.chNameILS[31] = 0;
}

/******************************************************************************
		 GetNamestring
******************************************************************************/

void GetNameString(char* p)
{	char* psz;
	if ((*p == 0) || _strnicmp(p, "TT:AIRPORT", 10))
		return;
	psz = strstr(pLocPak, &p[3]);
	if (psz)
	{	char* psz2;
		psz = strstr(psz, ": \x22");
		if (psz)
		{	psz += 3;
			psz2 = strchr(psz, '\x22');
			strncpy(p, psz, (__int32)(psz2 - psz));
			p[psz2 - psz] = 0;

			// Remove all \ characters:
			psz = p;
			while (psz = strchr(psz, '\\'))
				memmove(psz, &psz[1], strlen(psz));
			if (p[strlen(p) - 1] == '/')
				p[strlen(p) - 1] = 0;
		}
	}
	else p[0] = 0;
}

/******************************************************************************
		SetICAOFull
 *****************************************************************************/

 static void SetICAOFull(RWYLIST *pL, const char *pszICAO)
 {
	memset(pL->chICAOFull, 0, sizeof(pL->chICAOFull));

	if (pszICAO && pszICAO[0])
	{
		strncpy(pL->chICAOFull, pszICAO, sizeof(pL->chICAOFull) - 1);
		pL->chICAOFull[sizeof(pL->chICAOFull) - 1] = 0;
	}

 }

/******************************************************************************
        WriteMSFS2024HelistandCandidate
******************************************************************************/

void WriteMSFS2024HelistandCandidate(NGATE4 *pg4, char *pszICAO, float fAirportAltFt)
{
	static BOOL fHeaderDone = FALSE;
	FILE *phf;
	char szHelistandsPath[MAX_PATH];
	char *pszSlash;
	double dLat = 0.0;
	double dLon = 0.0;
	float fLat = 0.0F;
	float fLon = 0.0F;
	WORD wRawNumberType;
	BYTE bType4;
	BYTE bType5;
	WORD wNumber4;
	WORD wNumber5;
	unsigned short sDiameterFt;

	if (!pg4 || !pszICAO || !pszICAO[0])
		return;

	wRawNumberType = pg4->wNumberType;
	bType4 = (BYTE)(wRawNumberType & 15);
	bType5 = (BYTE)(wRawNumberType & 31);
	wNumber4 = (WORD)(wRawNumberType >> 4);
	wNumber5 = (WORD)(wRawNumberType >> 5);

	sDiameterFt = (unsigned short)(((pg4->fRadius * 2.0F) * 3.28084F) + 0.5F);

	fslat2lat(pg4->nLat, &fLat, &dLat);
	fslon2lon(pg4->nLon, &fLon, &dLon);

	szHelistandsPath[0] = 0;
	GetModuleFileName(NULL, szHelistandsPath, MAX_PATH);

	pszSlash = strrchr(szHelistandsPath, '\\');
	if (pszSlash)
		*(pszSlash +1) = 0;
	else
		szHelistandsPath[0] = 0;
	
	strcat_s(szHelistandsPath, MAX_PATH, "helistands_msfs2024.csv");

	phf = fopen(szHelistandsPath, fHeaderDone ? "ab" : "wb");

	if (!phf)
	{
		fprintf(fpAFDS,
		"		WARNING: unable to create %s for %s\n", szHelistandsPath, pszICAO);
			return;
	}

	if (!fHeaderDone)
	{
		fprintf(phf,
			"ICAO,Latitude,Longitude,AltitudeFt,Heading,RadiusM,DiameterFt,"
			"RawNumberType,Type4,Type5,Number4,Number5,Name,Suffix,Source\r\n");

		fHeaderDone = TRUE;
	}

	fprintf(phf,
		"%s,%.6f,%.6f,%.0f,%.1f,%.1f,%u,0x%04X,%u,%u,%u,%u,%u,%u,MSFS2024_TaxiwayParking\r\n",
		pszICAO,
		fLat,
		fLon,
		fAirportAltFt,
		(double)pg4->fHeading,
		(double)pg4->fRadius,
		(unsigned int)sDiameterFt,
		(unsigned int)wRawNumberType,
		(unsigned int)bType4,
		(unsigned int)bType5,
		(unsigned int)wNumber4,
		(unsigned int)wNumber5,
		(unsigned int)(pg4->bPushBackName & 0x3f),
		(unsigned int)pg4->bSuffix);

	fclose(phf);

	fprintf(fpAFDS,
		"          MSFS2024 HELISTAND candidate from TaxiwayParking at %s: radius=%.1fm, heading=%.1fT\n",
		pszICAO,
		(double)pg4->fRadius,
		(double)pg4->fHeading);
}

/******************************************************************************
         NewApts
******************************************************************************/

void NewApts(NAPT *pa, DWORD size, DWORD nObjs, NSECTS *ps, BYTE *p, NREGION *pRegion)
{	DWORD id = 0;
	char chICAO[9], chILSidP[8], chILSidS[8];
	char chWork[48], chWork2[16];
	LOCATION loc;
	float fMagvar, fHeading, fapLat, fapLon, fAirportAltFt = 0.0F;
	RWYLIST rwy1, rwy2, ap;
	WORD wTpath = 0, wTpnt = 0, wTname = 0;
	NTAXI *pTpath = 0;
	NEWNTAXI *pNTpath = 0;
	NEWNTAXI2 *pNTpath2 = 0;
	MSFSNTAXI* pNTpath3 = 0;
	NTAXIPT *pTpnt = 0;
	NEWTAXIPT *pNTpnt = 0;
	NTAXINM *pTname = 0;
	__int32 nCommStart = 0, nCommEnd = 0;
	__int32 nCommDelStart = 0, nCommDelEnd = 0;
	char *pApName = 0;
	char *pCitName = 0, *pStaName = 0, *pCtyName = 0;
	BOOL fDelTitleDone = FALSE;
	BOOL fNewTaxiPath = FALSE;
	
	if (pLastSetGateList)
	{	free(pLastSetGateList);
		pLastSetGateList = 0;
	}

	ap.fAirport = 0;
	chICAO[0] = 0;
			
	while ((size > 6) && (size >= pa->nLen))
	{	__int32 nThisLen = pa->nLen;
		if (fUserAbort) return;

		if ((pa->wId == OBJTYPE_AIRPORT) || (pa->wId == OBJTYPE_NEWAIRPORT) ||
				(pa->wId == OBJTYPE_NEWNEWAIRPORT) || (pa->wId == OBJTYPE_AIRPORT_MSFS) ||
				(pa->wId == OBJTYPE_AIRPORT_MSFS2024))
		{	// Airport record found
			if (!fDeletionsPass && ap.fAirport)
			{	if (nCommStart || nCommDelStart)
					AddComms(chICAO, nCommStart, nCommEnd, nCommDelStart, nCommDelEnd, pApName);
				WriteFSM(&ap);
				nCommStart = nCommEnd = nCommDelStart = nCommDelEnd = 0;
				pApName = pCitName = pStaName = pCtyName = 0;
				pLastSetGateList = 0;
			}
			
			id = pa->nId;
			fDebugThisEntry = FALSE;

			// decode ICAO
			DecodeID(id, chICAO, 1);
			nThisLen = sizeof(NAPT);
			if (pa->wId == OBJTYPE_AIRPORT) nThisLen -= 4;
			else if (pa->wId == OBJTYPE_NEWNEWAIRPORT) nThisLen += 4;
			else if (pa->wId == OBJTYPE_AIRPORT_MSFS) nThisLen += 12;
			else if (pa->wId == OBJTYPE_AIRPORT_MSFS2024) nThisLen = OBJTYPE_AIRPORT_MSFS2024_LEN;

			if (pa->wId == OBJTYPE_AIRPORT_MSFS2024)
			{
				BYTE *pNext = (BYTE *)pa + nThisLen;
				WORD wNextId = *((WORD *)pNext);
				DWORD dwNextLen = *((DWORD *)(pNext + 2));

				if ((wNextId == OBJTYPE_NAME) && (dwNextLen > 6) && (dwNextLen < pa->nLen))
				{
					NNAM *pn = (NNAM *)pNext;
					/*
						Expected pattern example:
						TT:AIRPORTLR.LLRM.name
						TT:AIRPORTLR.LL60.name
					*/
					{
						char *p1 = strchr(pn->chName, '.');
						char *p2 = p1 ? strchr(p1 + 1, '.') : NULL;

						if (p1 && p2)
						{
							int n = (int)(p2 - p1 - 1);

							    /*
									MSFS2024 airport ident:
									SDK allows 3 to 8 characters.
									chICAO is 9 bytes: 8 chars + terminating zero.
								*/

							if ((n >= 3) && (n <= 8) && (n < (int)sizeof(chICAO)))
							{
								memset(chICAO, 0, sizeof(chICAO));
								memcpy(chICAO, p1 + 1, n);
								chICAO[n] = 0;
							}
							else
							{
								fprintf(fpAFDS, "          *** WARNING: MSFS2024 airport ICAO not recovered; invalid NAME token length=%d\n", n);
							}							
						}
					}
				}

				/*
					In MSFS2024 airport record 0x0113, pa->nId may be zero.
					Keep id non-zero so following runway records are not skipped.
				*/
				if (id == 0)
					id = 1;
			}

			if (!fDeletionsPass)
			{	
				if (fDebug)	
				{__int32 nThisOffset = FileOffset32(&pa->wId);
				fprintf(fpAFDS, "OFFSET %08X-%08X:  ", nThisOffset, nThisOffset + nThisLen);
				}
				
				fprintf(fpAFDS, "\nAirport %s :", chICAO);
				
				SetLocPos(&loc, pa->nAlt, pa->nLat, pa->nLon, &fapLat, &fapLon, 0, 0);
				fAirportAltFt = ((float)pa->nAlt) * 3.28084F / 1000.0F;
				fMagvar = 360.0F - pa->fMagVar;
				if (fMagvar > 180.0F) fMagvar -= 360.0F;

				WritePosition(&loc, 1);

				ulTotalAPs++;
				fNewAirport = TRUE;

				// Now go through all existing runway records for this airport and "correct" the Magvar ...
				CorrectRunwayMagvar(&chICAO[0], fMagvar);

				// Also check for addition of ILS details to runways
				if (pR)
				{	RWYLIST* pRL = pR;
					while (pRL)
					{	if (!pRL->fDelete && (_strnicmp(chICAO, pRL->r.chICAO, 4) == 0))
						{	if (pRL->r.chILSid[0])
							{	FindILSdetails(nObjs, ps, p, pRL->r.chILSid, pRL, 0);
							}
						}
						pRL = pRL->pTo;
					}
				}
			
				// Find City and Airport Names
				if (pRegion)
				{	__int32 nICAOs = pRegion->wIcaoCount;
					NICAO *pICAOs = (NICAO *) ((BYTE *) pRegion + pRegion->nIcaoPtr);

					while (nICAOs--)
					{	if (pICAOs->nId == id)
						{	// Found this ICAO id
							DWORD *ppCountries = (DWORD *) ((BYTE *) pRegion + pRegion->nCountryPtr);
							__int32 nCountries = pRegion->wCountryCount, nCountryNum = pICAOs->bCountryIndex;
							DWORD *ppStates = (DWORD *) ((BYTE *) pRegion + pRegion->nStatePtr);
							__int32 nStates = pRegion->wStateCount, nStateNum = pICAOs->wStateIndex / 16;
							DWORD *ppCities = (DWORD *) ((BYTE *) pRegion + pRegion->nCityPtr);
							__int32 nCities = pRegion->wCityCount, nCityNum = pICAOs->wCitiesIndex;
							DWORD *ppAirports = (DWORD *) ((BYTE *) pRegion + pRegion->nAirportPtr);
							__int32 nAirports = pRegion->wAirportCount, nAirportNum = pICAOs->wAirportIndex;

							if (nCountries > nCountryNum)
							{	copyxmlstring(pNextCountryName, (char *) ppCountries + ppCountries[nCountryNum] + (nCountries * 4));
								if (pLocPak) GetNameString(pNextCountryName);
								fprintf(fpAFDS, "\n          Country Name=\x22%s\x22", pNextCountryName);
								pCtyName = pNextCountryName;
								pNextCountryName += strlen(pNextCountryName) + 1;
							}

							if (nStates > nStateNum)
							{	copyxmlstring(pNextStateName, (char *) ppStates + ppStates[nStateNum] + (nStates * 4));
								if (pLocPak) GetNameString(pNextStateName);
								fprintf(fpAFDS, "\n          State Name=\x22%s\x22", pNextStateName);
								pStaName = pNextStateName;
								pNextStateName += strlen(pNextStateName) + 1;
							}

							if (nCities > nCityNum)
							{	copyxmlstring(pNextCityName, (char *) ppCities + ppCities[nCityNum] + (nCities * 4));
								if (pLocPak) GetNameString(pNextCityName);
								fprintf(fpAFDS, "\n          City Name=\x22%s\x22", pNextCityName);
								pCitName = pNextCityName;
								pNextCityName += strlen(pNextCityName) + 1;
							}

							if (nAirports > nAirportNum)
							{	copyxmlstring(pNextAirportName, (char *) ppAirports + ppAirports[nAirportNum] + (nAirports * 4));
								if (pLocPak) GetNameString(pNextAirportName);
								fprintf(fpAFDS, "\n          Airport Name=\x22%s\x22\n", pNextAirportName);
								pApName = pNextAirportName;
								pNextAirportName += strlen(pNextAirportName) + 1;
							}
						
							break;
						}

						pICAOs++;
					}
				}

				wTpath = wTpnt = wTname = 0;
				pTpath = 0;
				pNTpath = 0;
				pTpnt = 0;
				pNTpnt = 0;
				pTname = 0;
	
				fprintf(fpAFDS, "\n          in file: %s\n\n",szCurrentFilePath);

				memset(&ap, 0, sizeof(RWYLIST));
				memcpy(ap.r.chICAO, chICAO, 4);
				SetICAOFull(&ap, chICAO);
				ap.r.fAlt = ToFeet(loc.elev/256);
				ap.r.fLat = fapLat;						
				ap.r.fLong = fapLon;
				ap.fMagvar = fMagvar;
				ap.fAirport = 1;
				ap.pCityName = pCitName;
				ap.pStateName = pStaName;
				ap.pCountryName = pCtyName;
				ap.pAirportName = pApName;
				ap.pPathName = pPathName;
				ap.pSceneryName = pSceneryName;
			}
		}

		else if (fDeletionsPass && id && (pa->wId == OBJTYPE_DELETEAP))
		{	// Delete Airport record found!
			BYTE *pd = (BYTE *) pa;

			if (!fDelTitleDone)
			{	fprintf(fpAFDS, "%s%s\n%s",	chLine, szCurrentFilePath, chLine);
				fDelTitleDone = TRUE;
				fprintf(fpAFDS, "Deletions check for Airport %s:\n", chICAO);
			}

			memset(&rwy1, 0, sizeof(RWYLIST));
			memcpy(rwy1.r.chICAO, chICAO, 4);
			SetICAOFull(&rwy1, chICAO);
			if (chICAO[3] == 0) rwy1.r.chICAO[3] = ' ';

			if (pd[6] & BIT_DELETE_ALL_TAXIWAYS)
			{	// delete the gates & taxiways?
				fprintf(fpAFDS, "          Delete all taxiways!\n");
				strcpy(rwy1.r.chRwy, "999");
				ProcessRunwayList(&rwy1, 0, 0);
				strcpy(rwy1.r.chRwy, "998");
				ProcessRunwayList(&rwy1, 0, 0);
			}
			
			if (pd[6] & (BIT_DELETE_ALL_RUNWAYS | BIT_DELETE_ALL_STARTS))
			{	// delete all runways!
				__int32 fDelMode =
					((pd[6] & (BIT_DELETE_ALL_RUNWAYS | BIT_DELETE_ALL_STARTS)) == (BIT_DELETE_ALL_RUNWAYS | BIT_DELETE_ALL_STARTS)) ? 3 :
					(pd[6] & BIT_DELETE_ALL_RUNWAYS) ? 1 : 2;
				rwy1.r.chRwy[0] = 0;
				fprintf(fpAFDS, "          Delete all %s!\n",
					(fDelMode == 3) ? "runways and starts" :
					(fDelMode == 1) ? "runways" : "starts");
				ProcessRunwayList(&rwy1, -(fDelMode - 1), 0);
			}

			if (pd[6] & BIT_DELETE_ALL_HELIPADS)
			{	// delete the helipads?
				fprintf(fpAFDS, "          Delete all helipads!\n");
				DeleteHelipads(chICAO);
			}

			else // Need to check details
			{	__int32 i, j = 13;
				
				// pd[8]= Number of runways to delete
				for (i = 0; i < pd[8]; i++)
				{	DecodeRwy(pd[j], pd[j+2] & 15, chWork2, 0, sizeof(chWork2));
					rwy1.r.chRwy[0] = '0' + (pd[j] / 100);
					rwy1.r.chRwy[1] = '0' + ((pd[j] % 100) / 10);
					rwy1.r.chRwy[2] = '0' + (char) (pd[j] % 10);
					rwy1.r.chRwy[3] = '0' + (char) (pd[j+2] & 15);
					fprintf(fpAFDS, "          Delete runway %s\n", chWork2);
					ProcessRunwayList(&rwy1, 0, 0);

					DecodeRwy(pd[j+1], pd[j+2] >> 4, chWork2, 0, sizeof(chWork2));
					rwy1.r.chRwy[0] = '0' + (pd[j+1] / 100);
					rwy1.r.chRwy[1] = '0' + ((pd[j+1] % 100) / 10);
					rwy1.r.chRwy[2] = '0' + (char) (pd[j+1] % 10);
					rwy1.r.chRwy[3] = '0' + (char) (pd[j+2] >> 4);
					fprintf(fpAFDS, "          Delete runway %s\n", chWork2);
					ProcessRunwayList(&rwy1, 0, 0);

					j += 4;
				}

				// pd[9]= Number of starts to delete
				j = 12 + (pd[8] * 4);
				for (i = 0; i < pd[9]; i++) if (pd[j+2] == 1)
				{	DecodeRwy(pd[j], pd[j+1] & 15, chWork2, 0, sizeof(chWork2));
					rwy1.r.chRwy[0] = '0' + (pd[j] / 100);
					rwy1.r.chRwy[1] = '0' + ((pd[j] % 100) / 10);
					rwy1.r.chRwy[2] = '0' + (char) (pd[j] % 10);
					rwy1.r.chRwy[3] = '0' + (char) (pd[j+2] & 15);
					fprintf(fpAFDS, "          Delete start %s\n", chWork2);
					ProcessRunwayList(&rwy1, -1, 0);

					j += 4;
				}
			}

			if (pd[6] & BIT_DELETE_ALL_TAXIWAYS)
			{	// delete the gates & taxiways?
				fprintf(fpAFDS, "          Delete all taxiways!\n");
				strcpy(rwy1.r.chRwy, "999");
				ProcessRunwayList(&rwy1, 0, 0);
				strcpy(rwy1.r.chRwy, "998");
				ProcessRunwayList(&rwy1, 0, 0);
			}
			
			// Frequency handling
			if (pd[6] & BIT_DELETE_ALL_FREQUENCIES)
			{	// delete all frequencies!
				fprintf(fpAFDS, "          COM: Delete all frequencies!\n");
				nCommDelStart = nCommDelEnd = nCommStart = nCommEnd = 0;
				DeleteComms(chICAO);
			}

			else if (pd[10]) // Need to check details
			{	// pd[10]= Number of starts to delete
				__int32 i, j = 12 + (pd[8] * 4) + (pd[9] * 4);
				if (!nCommDelStart) nCommDelStart = ftell(fpAFDS);
				for (i = 0; i < pd[10]; i++) if (pd[j+3] & 0xf0)
				{	__int32 type = (pd[j+3] >> 4) & 0xff;
					fprintf(fpAFDS, "          COM: Delete, Type=%d (%s), Freq=%.2f\n",
						type, pszComms[(type > 15) ? 16 : type],
						(double) ((*((__int32 *) &pd[j]) & 0x0fffffff) / 10000) / 100.0);
					// Specific frequency deletion to follow ###################################
					j += 4;
				}
				nCommDelEnd = ftell(fpAFDS);
			}
		} 

		else if (!fDeletionsPass && id &&
				((pa->wId == OBJTYPE_RUNWAY) || (pa->wId == OBJTYPE_NEWRUNWAY)
					|| (pa->wId == OBJTYPE_MSFSRUNWAY)))
		{	// Runway record found
			NRWY *pr = (NRWY *) pa;
			__int32 nFreq = 0, fOk = 0, fList = 0;
			ANGLE Rlat, Rlong;
			WORD wSurf = 24;

			nThisLen = pr->nLen;

			memset(&rwy1, 0, sizeof(RWYLIST));
			memcpy(rwy1.r.chICAO, chICAO, 4);
			SetICAOFull(&rwy1, chICAO);
			if (chICAO[3] == 0) rwy1.r.chICAO[3] = ' ';
			rwy1.fMagvar = fMagvar;
			memcpy(&rwy2, &rwy1, sizeof(RWYLIST));
			chILSidP[0] = chILSidS[0] = 0;
				
			fprintf(fpAFDS, "          ");
			DecodeRwy(pr->bStartNumber, pr->bStartDesignator, chWork2, 0, sizeof(chWork2));
			rwy1.r.chRwy[0] = '0' + (char) (pr->bStartNumber / 100);
			rwy1.r.chRwy[1] = '0' + (char) ((pr->bStartNumber % 100) / 10);
			rwy1.r.chRwy[2] = '0' + (char) (pr->bStartNumber % 10);
			rwy1.r.chRwy[3] = '0' + (char) pr->bStartDesignator;
			rwy1.fCTO = pr->bPatternFlags & PATTERN_NO_PRIM_TAKEOFF;
			rwy1.fCL = pr->bPatternFlags & PATTERN_NO_PRIM_LANDING;
			rwy1.r.fPrimary = 1;
				
			DecodeRwy(pr->bEndNumber, pr->bEndDesignator, chWork, 0, sizeof(chWork));
			rwy2.r.chRwy[0] = '0' + (char) (pr->bEndNumber / 100);
			rwy2.r.chRwy[1] = '0' + (char) ((pr->bEndNumber % 100) / 10);
			rwy2.r.chRwy[2] = '0' + (char) (pr->bEndNumber % 10);
			rwy2.r.chRwy[3] = '0' + (char) pr->bEndDesignator;
			rwy2.fCTO = pr->bPatternFlags & PATTERN_NO_SEC_TAKEOFF;
			rwy2.fCL = pr->bPatternFlags & PATTERN_NO_SEC_LANDING;
			rwy2.r.fPrimary = 0;

			fprintf(fpAFDS, "Runway %s/%s centre: ", chWork2, chWork);
			SetLocPos(&loc, pr->nAlt, pr->nLat, pr->nLon, 0, 0, 0, 0);
			WritePosition(&loc, 1);
			fprintf(fpAFDS, "\n");
			
			if (rwy1.fCTO || rwy1.fCL)
				fprintf(fpAFDS, "              Runway %s closed for %s\n", chWork2,
				(rwy1.fCL && rwy1.fCTO) ? "landing and take-off" :
				(rwy1.fCL) ? "landing" : "take-off");

			if (rwy2.fCTO || rwy2.fCL)
				fprintf(fpAFDS, "              Runway %s closed for %s\n", chWork,
				(rwy2.fCL && rwy2.fCTO) ? "landing and take-off" :
				(rwy2.fCL) ? "landing" : "take-off");

			ToAngle(&Rlat, loc.lat.i, loc.lat.f, 0);
			ToAngle(&Rlong, loc.lon.i, loc.lon.f, 2);					
			rwy1.fLat = rwy2.fLat = Rlat.fangle;
			rwy1.fLong = rwy2.fLong = Rlong.fangle;

			if ((pa->wId == OBJTYPE_MSFSRUNWAY) && pMaterials)
			{	// Form character equivalent of surface GUID
				char chGUID[64];
				MSFSRUNWAY* pr2 = (MSFSRUNWAY*) pr;
				sprintf(chGUID, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
					*((DWORD *)&pr2->guidSurface[0]),
					*((WORD *)&pr2->guidSurface[4]),
					*((WORD *)&pr2->guidSurface[6]),
					pr2->guidSurface[8], pr2->guidSurface[9],
					pr2->guidSurface[10], pr2->guidSurface[11], pr2->guidSurface[12],
					pr2->guidSurface[13], pr2->guidSurface[14], pr2->guidSurface[15]);
				
				// find GUID is Materials file:
				char* psz = strstr(pMaterials, chGUID);
				if (psz)
				{
					char *psz2 = strchr(psz, '>'); // End of entry
					psz = strstr(psz, "SurfaceType=");
					if (psz && psz2 && (psz < psz2))
					{
						char szName[32];
						strncpy_s(szName, sizeof(szName), &psz[13], _TRUNCATE);
						psz = strchr(szName, '\x22');
						if (psz)
						{
							*psz = 0;
							wSurf = GetMSFS2024RunwaySurfaceLegacyIndex(szName);
						}
					}
				}
			}
			else wSurf = pr->wSurface & 0x7f; // remove transparency flag ### 240719
			rwy1.r.chSurfNew = rwy2.r.chSurfNew = wSurf > 23 ? 24 : wSurf;
			rwy1.r.chSurf = rwy2.r.chSurf = chOldSurf[rwy1.r.chSurfNew];

			rwy1.r.fAlt = rwy2.r.fAlt = ToFeet(loc.elev/256);
			rwy1.r.uLen = rwy2.r.uLen = (WORD) ((pr->fLength * 3.28084) + 0.5);
			rwy1.r.uWid = rwy2.r.uWid = (WORD) ((pr->fWidth * 3.28084) + 0.5);
			rwy1.r.bLights = rwy2.r.bLights = pr->bLights;
			rwy1.r.bPatternFlags = rwy2.r.bPatternFlags = pr->bPatternFlags;
			rwy1.r.fPatternAlt = rwy2.r.fPatternAlt = pr->fPatternAlt;
			fHeading = pr->fHeading;
			
			if (((fIncludeWater < 0) && (pr->bStartDesignator == 4)) ||
					((fIncludeWater >= 0) && (pr->bStartDesignator < (4 + fIncludeWater))))
			{	if (FindStart(&rwy1, pa, size, chWork2) && (rwy1.r.uLen > nMinRunwayLen)) fOk |= 1;
				else if ((rwy1.r.uLen > nMinRunwayLen) && (!rwy1.fCL || !rwy1.fCTO))
					fOk |= 1;

				// Save copies of original start location for special XML entries
				rwy1.r.fFSLat = rwy1.r.fLat;
				rwy1.r.fFSLong = rwy1.r.fLong;

				if (fOk & 1)
				{	// Compute better thresholds and get other stuff
					rwy1.r.fLat = (float) ((double) rwy1.fLat - (((double) rwy1.r.uLen * cos((double) fHeading * PI / 180.0)) / 729132.0));
					rwy1.r.fLong =  (float) ((double) rwy1.fLong - (((double) rwy1.r.uLen * sin((double) fHeading * PI / 180.0)) /
										(729132.0 * cos((double) rwy1.fLat * PI / 180.0))));
					fprintf(fpAFDS, "              Computed start %s: Lat %.6f Long %.6f\n",
								chWork2, (double) rwy1.r.fLat, (double) rwy1.r.fLong); 

					if (fDebug)
					{	fprintf(fpAFDS, "### %sRunway1 struct (len=%04X, size=%04zu):\n",
							(pa->wId == OBJTYPE_NEWRUNWAY) ? "New" :
							(pa->wId == OBJTYPE_MSFSRUNWAY) ? "MSFS" : "", pa->nLen,
							(pa->wId == OBJTYPE_MSFSRUNWAY) ? OBJTYPE_MSFSRUNWAY_LEN : sizeof(NRWY));
					}
					
					if (pa->wId == OBJTYPE_RUNWAY)
					{	// if (fDebug) DebugRwyAdditions((NAPT*)((BYTE*)pa + sizeof(NRWY)), nThisLen - sizeof(NRWY));
						FindOffThresh(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 5);
						FindVASI(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 11);
						FindVASI(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 12);
						FindAppLights(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 15);
					}

					else if (pa->wId == OBJTYPE_NEWRUNWAY)
					{	// if (fDebug) DebugRwyAdditions((NAPT*)((BYTE*)pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16);
						FindOffThresh(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 5);
						FindVASI(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 11);
						FindVASI(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 12);
						FindAppLights(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 15);
					}
	
					else if (pa->wId == OBJTYPE_MSFSRUNWAY)
					{	// if (fDebug) DebugRwyAdditions((NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN);
						FindOffThresh(&rwy1, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, -5);
						FindVASI(&rwy1, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, 11);
						FindVASI(&rwy1, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, 12);
						FindAppLights(&rwy1, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, 0xdf);
					}
	
					else
					{	//******************** THIS METHOD DOESN'T WORK ON OLDER BGLs. WHY? **************
						// if (fDebug) DebugRwyAdditions((NAPT*)((BYTE*)pa + pa->nLen), nThisLen - pa->nLen);
						FindOffThresh(&rwy1, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 5);
						FindVASI(&rwy1, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 11);
						FindVASI(&rwy1, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 12);
						FindAppLights(&rwy1, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 15);
					}
				}							
			}

			if (((fIncludeWater < 0) && (pr->bEndDesignator == 4)) ||
					((fIncludeWater >= 0) && (pr->bEndDesignator < (4 + fIncludeWater))))
			{	if (FindStart(&rwy2, pa, size, chWork) && (rwy2.r.uLen > nMinRunwayLen)) fOk |= 2;
				else if ((rwy2.r.uLen > nMinRunwayLen) && (!rwy2.fCL || !rwy2.fCTO)) //#####################################xxx
					fOk |= 2;
				
				// Save copies of original start location for special XML entries
				rwy2.r.fFSLat = rwy2.r.fLat;
				rwy2.r.fFSLong = rwy2.r.fLong;

				if (fOk & 2)
				{	// Compute better thresholds
					rwy2.r.fLat = (float) ((double) rwy2.fLat - (((double) rwy2.r.uLen * cos(((double) fHeading + 180.0) * PI / 180.0)) / 729132.0));
					rwy2.r.fLong =  (float) ((double) rwy2.fLong - (((double) rwy2.r.uLen * sin(((double) fHeading + 180.0) * PI / 180.0)) /
										(729132.0 * cos((double) rwy2.fLat * PI / 180.0))));
					fprintf(fpAFDS, "              Computed start %s: Lat %.6f Long %.6f\n",
								chWork, (double) rwy2.r.fLat, (double) rwy2.r.fLong); 

					if (fDebug)
						fprintf(fpAFDS,"### %sRunway2 struct (len=%04X, size=%04zu):\n",
							(pa->wId == OBJTYPE_NEWRUNWAY) ? "New" : "", pa->nLen, sizeof(NRWY));

					if (pa->wId == OBJTYPE_RUNWAY)
					{	FindOffThresh(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 6);
						FindVASI(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 13);
						FindVASI(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 14);
						FindAppLights(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 16);
					}

					else if (pa->wId == OBJTYPE_NEWRUNWAY)
					{	FindOffThresh(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 6);
						FindVASI(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 11);
						FindVASI(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 12);
						FindAppLights(&rwy2, (NAPT *) ((BYTE *) pa + sizeof(NRWY) + 16), nThisLen - sizeof(NRWY) - 16, 15);
					}

					else if (pa->wId == OBJTYPE_MSFSRUNWAY)
					{	FindOffThresh(&rwy2, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, -6);
						FindVASI(&rwy2, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, 11);
						FindVASI(&rwy2, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, 12);
						FindAppLights(&rwy2, (NAPT*)((BYTE*)pa + OBJTYPE_MSFSRUNWAY_LEN), nThisLen - OBJTYPE_MSFSRUNWAY_LEN, 0xe0);
					}

					else
					{	//******************** THIS METHOD DOESN'T WORK ON OLDER BGLs. WHY? **************
						FindOffThresh(&rwy2, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 6);
						FindVASI(&rwy2, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 13);
						FindVASI(&rwy2, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 14);
						FindAppLights(&rwy2, (NAPT *) ((BYTE *) pa + pa->nLen), nThisLen - pa->nLen, 16);
						// NB last three lines were &rwy1. Why?
					}
				}
			}
			
			rwy1.fHdg = fHeading - fMagvar;
			if (rwy1.fHdg > 360.0F) rwy1.fHdg -= 360.0F;
			if (rwy1.fHdg < 0.0F) rwy1.fHdg += 360.0F;
			rwy2.fHdg = rwy1.fHdg + 180.0F;
			if (rwy2.fHdg > 360.0F) rwy2.fHdg -= 360.0F;
			if (rwy2.fHdg < 0.0F) rwy2.fHdg += 360.0F;
			
			rwy1.r.uHdg = (unsigned short)  (rwy1.fHdg + 0.5);
			rwy2.r.uHdg = (unsigned short)  (rwy2.fHdg + 0.5);
			
			sprintf(chWork, "UNKNOWN %d", wSurf);			
			fprintf(fpAFDS, "              Hdg: %.3f true (MagVar %.3f), %s%s, %d x %d ft",
				(double) fHeading, (double) fMagvar,
				wSurf > 23 ? chWork : szNRwySurf[wSurf],
				(pr->wSurface & 128) ? " (Transparent)" : "",
				(__int32) ((pr->fLength * 3.28084) + 0.5),
				(__int32) ((pr->fWidth * 3.28084) + 0.5));					

			fFoundSome = FALSE; // ILS search flag

			if (pr->nPrimaryIlsId)
			{	DecodeID(pr->nPrimaryIlsId, chWork, 0);
				fprintf(fpAFDS, "\n              Primary ILS ID = %s", chWork);
				strcpy(chILSidP, chWork);
				memcpy(rwy1.r.chILSid, chILSidP, 6);
				FindILSdetails(nObjs, ps, p, chILSidP, &rwy1, 0);
			}
			
			else
			{	chWork[0] = 0;
				nFreq = 0;
			}
			
			if (pr->nSecondaryIlsId)
			{	DecodeID(pr->nSecondaryIlsId, chWork, 0);
				fprintf(fpAFDS, "\n              Secondary ILS ID = %s", chWork);
				strcpy(chILSidS, chWork);
				memcpy(rwy2.r.chILSid, chILSidS, 6);
				FindILSdetails(nObjs, ps, p, chILSidS, &rwy2, 0);
			}
			
			else
			{	chWork[0] = 0;
				nFreq = 0;
			}
						
			if (fOk & 1)AddRunway(&rwy1);
			if (fOk & 2)AddRunway(&rwy2);
			prwyPrevious = 0;
			fprintf(fpAFDS, "\n");			
		}

		
		else if (id && (pa->wId == OBJTYPE_NAME))
		{	// Airport name found - Find City Name first
			if (pRegion)
			{	__int32 nICAOs = pRegion->wIcaoCount;
				NICAO *pICAOs = (NICAO *) ((BYTE *) pRegion + pRegion->nIcaoPtr);

				while (nICAOs--)
				{	if (pICAOs->nId == id)
					{	// Found this ICAO id
						DWORD *ppCities = (DWORD *) ((BYTE *) pRegion + pRegion->nCityPtr);
						__int32 nCities = pRegion->wCityCount, nCityNum = pICAOs->wCitiesIndex;

						if (nCities > nCityNum)
						{	strcpy(pNextCityName, (char *) ppCities + ppCities[nCityNum] + (nCities * 4));
                        	fprintf(fpAFDS, "          City Name=\x22%s\x22\n", pNextCityName);
							pCitName = pNextCityName;
							pNextCityName += strlen(pNextCityName) + 1;
						}
						
						break;
					}

					pICAOs++;
				}
			}

			StoreName(pNextAirportName, (NNAM *)pa);

			if (!fDeletionsPass)
				fprintf(fpAFDS,
					"          Airport Name=\"%s\"\n",
					pNextAirportName);

			pApName = pNextAirportName;
			pNextAirportName += strlen(pNextAirportName) + 1;
		}


		else if (!fDeletionsPass && id && (pa->wId == OBJTYPE_APCOMM))
		{	// Airport comms record found
			NCOMM *pc = (NCOMM *) pa;
			char chName[256];
			chName[0] = 0;

			if (pc->nLen > sizeof(NCOMM))
			{	__int32 nlen = min(pc->nLen - sizeof(NCOMM), 255);
				memcpy(chName, (char *) pc + sizeof(NCOMM), nlen);
				while (nlen && !isalnum((unsigned char) chName[nlen - 1])) nlen--;
				chName[nlen] = 0; // Get rid of bad terminations
			}

			if (!nCommStart) nCommStart = ftell(fpAFDS);

			fprintf(fpAFDS, "          COM: Type=%d (%s), Freq=%.3f, Name=\x22%s\x22\n",
					pc->bCommType, pszComms[(pc->bCommType > 15) ? 16 : pc->bCommType],
					(double)((pc->nFreq + 500) / 1000) / 1000.0, chName); // Allows for 8.33 spacing
					//(double) ((pc->nFreq + 1000)/ 10000)/100.0,	chName);

			nCommEnd = ftell(fpAFDS);

			if (ap.fAirport && ((pc->bCommType == 1) || (((pc->bCommType == 12) || (pc->bCommType == 13)) && (ap.Atis < 0x1800))))
			{	// ATIS / AWOS / ASOS
				__int32 nAtis =((pc->nFreq + 5000) / 10000) % 10000;

				// convert to BCD
				ap.Atis = (nAtis/1000) << 12;
				nAtis %= 1000;
				ap.Atis |= (nAtis/100) << 8;
				nAtis %= 100;
				ap.Atis |= (nAtis/10) << 4;
				nAtis %= 10;
				if (nAtis == 8) nAtis = 7;
				if (nAtis == 3) nAtis = 2;
				ap.Atis |= nAtis;

				//fprintf(fpAFDS, "          FSM ATIS/AWOS/ASOS for %4s=%04X\n",
				//		ap.r.chICAO, ap.Atis);	
			}
		}

		else if (!fDeletionsPass && id &&
			((pa->wId == OBJTYPE_JETWAY) || (pa->wId == OBJTYPE_MSFSJETWAY)) &&
				fMarkJetways &&	pLastSetGateList && prwyPrevious)
		{	// Jetway record found
			NJETWAY *pjw = (NJETWAY *) pa;
			__int32 nGateNum = pjw->wParkingNumber;
			BOOL fJetwayOk = FALSE;

			NGATEHDR *pgh= prwyPrevious->pGateList;
            WORD w = 0, wCtr = pgh->wCount;
			NGATE *pg = (NGATE *) ((BYTE *) pgh + sizeof(NGATEHDR));
			NGATE2 *pg2 = (NGATE2 *) ((pgh->wId == OBJTYPE_NEWTAXIPARK) ? pg : 0);
			NGATE3 *pg3 = (NGATE3 *) ((pgh->wId == OBJTYPE_NEWNEWTAXIPARK) ? pg : 0);
			NGATE4 *pg4 = (NGATE4 *) ((pgh->wId == OBJTYPE_MSFSTAXIPARK) ? pg : 0);
			__int32 nPark = pjw->wGateName & 0x3f;
			char chLetter[2];
			chLetter[0] = nPark + 0x35;
			chLetter[1] = 0;

			
			// ############### MSFSTAXIPARK ?? #########

			#ifdef _DEBUG
				fDebugThisEntry = TRUE;
			#endif	

				fprintf(fpAFDS, "          %s %s%d has Jetway\n",
					pszParkType[pjw->wGateName >= 11 ? 10 : pjw->wGateName],
					(nPark <= 11) ? szParkNames[nPark] : chLetter, nGateNum);


			if (fDebugThisEntry)
				fprintf(fpAFDS,"\nSearching %d entries for GateName %d, Num %d\n   ", wCtr, pjw->wGateName, nGateNum);
	
			while (w < wCtr)
			{	if (fDebugThisEntry)
					fprintf(fpAFDS,"%d: Number=%d, Name=%d:%s\n   ", w, (pg->wNumberType >> 4), pg->bPushBackName &0x3F, pszParkType[(pg->bPushBackName & 0x3f) >= 11 ? 10 : (pg->bPushBackName & 0x0f)]);

		 		if (((pg->wNumberType >> 4) == nGateNum) && ((pg->bPushBackName & 0x3F) == pjw->wGateName))
            	{	pg->bCodeCount |= 0x80; // Flag Jetway using top bit of code count
					fJetwayOk = TRUE;
					break;
				}

				w++;
				pg = (NGATE *) ((BYTE *) pg + (4 * (pg->bCodeCount & 0x7f)) + (pg4 ? sizeof(NGATE4) : pg3 ? sizeof(NGATE3) : pg2 ? sizeof(NGATE2) : sizeof(NGATE)));
				if (pg2) pg2 = (NGATE2 *) pg;
				if (pg3) pg3 = (NGATE3 *) pg;
				if (pg4) pg4 = (NGATE4 *) pg;
			}

			if (fDebugThisEntry)
				fprintf(fpAFDS, "\n... Gate %s\n", fJetwayOk ? "flagged" : "not found!");
		}
		
		else if (!fDeletionsPass && id && ((pa->wId == OBJTYPE_TAXIPARK) || (pa->wId == OBJTYPE_NEWTAXIPARK)
				|| (pa->wId == OBJTYPE_NEWNEWTAXIPARK)
				|| (pa->wId == OBJTYPE_MSFSTAXIPARK))) // ############### MSFSTAXIPARK ?? #########
		{	// Gate record found
			NGATEHDR *pgh = (NGATEHDR *) pa;
			WORD w = 0, wCtr = pgh->wCount;
			NGATE *pg = (NGATE *) ((BYTE *) pa + sizeof(NGATEHDR));
			NGATE2 *pg2 = (NGATE2 *) ((pa->wId == OBJTYPE_NEWTAXIPARK) ? pg : 0);
			NGATE3 *pg3 = (NGATE3 *) ((pa->wId == OBJTYPE_NEWNEWTAXIPARK) ? pg : 0);
			NGATE4 *pg4 = (NGATE4 *) ((pa->wId == OBJTYPE_MSFSTAXIPARK) ? pg : 0);

			nThisLen = pgh->nLen;

			// Will add to data for g5.csv file
			memset(&rwy1, 0, sizeof(RWYLIST));
			memcpy(rwy1.r.chICAO, chICAO, 4);
			SetICAOFull(&rwy1, chICAO);
			rwy1.pGateList = (NGATEHDR *) malloc(nThisLen);
			
			pLastSetGateList = (long long *) malloc(sizeof(__int64) * (wCtr+1));
			if (pLastSetGateList)
				pLastSetGateList[0] = (__int32) wCtr | (pa->wId << 16);
			
			strcpy(rwy1.r.chRwy, "999");
			memcpy(rwy1.pGateList, pa, nThisLen);
			AddRunway(&rwy1);
			
			while (w < wCtr)
			{	// For now just dump to txt file:
				LOCATION locg;
				float fLat, fLon;
				char *pszGateName, chGateName[5];
				char* pszGateSuffix, chGateSuffix[2];

				pszGateSuffix = "";

				pg->bPushBackName &= 0x3f;
				if (pg->bPushBackName > 11)
				{	chGateName[0] = pg->bPushBackName + 0x35;
					chGateName[1] = 0;
					pszGateName = &chGateName[0];
					if (pg4 && pg4->bSuffix > 0)
					{
						chGateSuffix[0] = pg4->bSuffix + 0x35;
						chGateSuffix[1] = 0;
						pszGateSuffix = &chGateSuffix[0];
					}
				}

				else
				{
					pszGateName = szParkNames[pg->bPushBackName];
					if (pg4 && pg4->bSuffix > 0)
					{
						chGateSuffix[0] = pg4->bSuffix + 0x35;
						chGateSuffix[1] = 0;
						pszGateSuffix = &chGateSuffix[0];
					}
				}
				fprintf(fpAFDS, "          %s %s%d%s [#G%d]:  ", 
					pszParkType[pg->bPushBackName >= 11 ? 10 : pg->bPushBackName],
					pszGateName, pg->wNumberType >> 4, pszGateSuffix, w);
				SetLocPos(&locg, 0, 
					pg4 ? pg4->nLat : pg3 ? pg3->nLat : pg2 ? pg2->nLat : pg->nLat,
					pg4 ? pg4->nLon : pg3 ? pg3->nLon : pg2 ? pg2->nLon : pg->nLon,
					&fLat, &fLon, 0, 0);
				WritePosition(&locg, 0);
				fprintf(fpAFDS, "\n");
				
				fprintf(fpAFDS, "              Type %d (%s), Size %.1fm, Hdg %.1fT\n",
					pg->wNumberType & 15, pszGateType[pg->wNumberType & 15],
					(double) pg->fRadius, (double) pg->fHeading);

				if (pa->wId == OBJTYPE_MSFSTAXIPARK)
				{
					NGATE4 *pg4 = (NGATE4 *)pg;

					if ((pg4->wNumberType & 31) == 16)
						WriteMSFS2024HelistandCandidate(pg4, chICAO, fAirportAltFt);
				}

				if (pg->bCodeCount & 0x7f)
				{	BYTE b = pg->bCodeCount & 0x7f;
					char *pA = (char *) pg +
						(pg4 ? sizeof(NGATE4) :
						pg3 ? sizeof(NGATE3) :
						pg2 ? sizeof(NGATE2) :
						sizeof(NGATE));
					fprintf(fpAFDS, "              Airlines:");

					while (b-- && isprint(*pA))
					{	fprintf(fpAFDS, " %.4s", pA);
						pA += 4;
					}
					fprintf(fpAFDS, "\n");
				}
				
				w++;
				pLastSetGateList[w] = (__int64) pg; // Index for use in Taxipath decode
				pg = (NGATE *) ((BYTE *) pg + (4 * (pg->bCodeCount & 0x7f)) + 
					(pg4 ? sizeof(NGATE4) : pg3 ? sizeof(NGATE3) : pg2 ? sizeof(NGATE2) : sizeof(NGATE)));
				if (pg2) pg2 = (NGATE2 *) pg;
				if (pg3) pg3 = (NGATE3 *) pg;
				if (pg4) pg4 = (NGATE4 *) pg;
			}
		}
		
		else if (!fDeletionsPass && id && ((pa->wId == OBJTYPE_TAXIPATH) ||
				(pa->wId == OBJTYPE_NEWTAXIPATH) || (pa->wId == OBJTYPE_NEWNEWTAXIPATH)
					|| (pa->wId == OBJTYPE_MSFSTAXIPATH)))
		{	// Taxi path record found
			NTAXIHDR *pth = (NTAXIHDR *) pa;
			WORD w = 0;

			if (pa->wId == OBJTYPE_TAXIPATH)
			{	fNewTaxiPath = FALSE;
				pTpath = (NTAXI *) ((BYTE *) pa + sizeof(NTAXIHDR));

				wTpath = pth->wCount;
				
				nThisLen = pth->nLen;
			
				while (w < wTpath)
				{	// For now just dump to txt file:
					static char *pszPathTypes[] = { "?", "Taxi","Runway","Parking","Path","Closed" };

					pTpath[w].wEnd &= 0x0fff;
					pTpath[w].wStart &= 0x0fff;
					pTpath[w].bDrawFlags &= 0x0f;

					fprintf(fpAFDS, "          Taxipath (%s%d):  Type %d (%s), Start#=%d, End#=%s%d, Wid=%.2fm\n", 
						(pTpath[w].bDrawFlags == 2) ? "Runway " : "Name #",
						pTpath[w].bNumber,
						pTpath[w].bDrawFlags,
						pTpath[w].bDrawFlags < 6 ? pszPathTypes[pTpath[w].bDrawFlags] : "?",
						pTpath[w].wStart, (pTpath[w].bDrawFlags == 3) ? "G" : "",
						pTpath[w].wEnd, (double) pTpath[w].fWidth);
					w++;
				}
			}

			else if (pa->wId == OBJTYPE_NEWTAXIPATH)
			{	pNTpath = (NEWNTAXI *) ((BYTE *) pa + sizeof(NTAXIHDR));
				fNewTaxiPath = TRUE;
				wTpath = pth->wCount;		
				nThisLen = pth->nLen;
			
				while (w < wTpath)
				{	// For now just dump to txt file:
					static char *pszPathTypes[] = { "?", "Taxi","Runway","Parking","Path","Closed" };

					// Removed these for New BGL format
					//pNTpath[w].wEnd &= 0x0fff;
					//pNTpath[w].wStart &= 0x0fff;
					pNTpath[w].bDrawFlags &= 0x0f;

					fprintf(fpAFDS, "          Taxipath (%s%d):  Type %d (%s), Start#=%d, End#=%s%d, Wid=%.2fm\n", 
						(pNTpath[w].bDrawFlags == 2) ? "Runway " : "Name #",
						pNTpath[w].bNumber,
						pNTpath[w].bDrawFlags,
						pNTpath[w].bDrawFlags < 6 ? pszPathTypes[pNTpath[w].bDrawFlags] : "?",
						pNTpath[w].wStart, (pNTpath[w].bDrawFlags == 3) ? "G" : "",
						pNTpath[w].wEnd, (double) pNTpath[w].fWidth);
					w++;
				}
			}

			else if (pa->wId == OBJTYPE_MSFSTAXIPATH)
			{	pNTpath3 = (MSFSNTAXI*)((BYTE*)pa + sizeof(NTAXIHDR));
				fNewTaxiPath = -1;
				wTpath = pth->wCount;
				nThisLen = pth->nLen;

				while (w < wTpath)
				{	// For now just dump to txt file:
					static char* pszPathTypes[] = { "?", "Taxi","Runway","Parking","Path","Closed" };

					// Removed these for MSFS
					//pNTpath3[w].wEnd &= 0x0fff;
					//pNTpath3[w].wStart &= 0x0fff;
					pNTpath3[w].bDrawFlags &= 0x0f;

					fprintf(fpAFDS, "          Taxipath (%s%d):  Type %d (%s), Start#=%d, End#=%s%d, Wid=%.2fm\n",
						(pNTpath3[w].bDrawFlags == 2) ? "Runway " : "Name #",
						pNTpath3[w].bNumber,
						pNTpath3[w].bDrawFlags,
						pNTpath3[w].bDrawFlags < 6 ? pszPathTypes[pNTpath3[w].bDrawFlags] : "?",
						pNTpath3[w].wStart, (pNTpath3[w].bDrawFlags == 3) ? "G" : "",
						pNTpath3[w].wEnd, (double)pNTpath3[w].fWidth);
					w++;
				}
			}

			else
			{	pNTpath2 = (NEWNTAXI2 *) ((BYTE *) pa + sizeof(NTAXIHDR));
				fNewTaxiPath = TRUE;
				wTpath = pth->wCount;		
				nThisLen = pth->nLen;
			
				while (w < wTpath)
				{	// For now just dump to txt file:
					static char *pszPathTypes[] = { "?", "Taxi","Runway","Parking","Path","Closed" };

					// Why are these here?
					//pNTpath2[w].wEnd &= 0x0fff;
					//pNTpath2[w].wStart &= 0x0fff;
					pNTpath2[w].bDrawFlags &= 0x0f;

					fprintf(fpAFDS, "          Taxipath (%s%d):  Type %d (%s), Start#=%d, End#=%s%d, Wid=%.2fm\n", 
						(pNTpath2[w].bDrawFlags == 2) ? "Runway " : "Name #",
						pNTpath2[w].bNumber,
						pNTpath2[w].bDrawFlags,
						pNTpath2[w].bDrawFlags < 6 ? pszPathTypes[pNTpath2[w].bDrawFlags] : "?",
						pNTpath2[w].wStart, (pNTpath2[w].bDrawFlags == 3) ? "G" : "",
						pNTpath2[w].wEnd, (double) pNTpath2[w].fWidth);
					w++;
				}
			}
		}

		else if (!fDeletionsPass && id && (pa->wId == OBJTYPE_TAXINAME))
		{	// Taxi name record found
			NTAXIHDR *pth = (NTAXIHDR *) pa;
			WORD w = 0;
			pTname = (NTAXINM *) ((BYTE *) pa + sizeof(NTAXIHDR));
			wTname = pth->wCount;
			nThisLen = pth->nLen;
			
			while (w < wTname)
			{	// For now just dump to txt file:
				fprintf(fpAFDS, "          Taxiname:  #%d = %.8s\n", w, pTname[w].szName);
				w++;
			}
		}

		else if (!fDeletionsPass && id && (pa->wId == OBJTYPE_TAXIPOINT))
		{	// Taxi path record found
			NTAXIHDR *pth = (NTAXIHDR *) pa;
			WORD w = 0;
			pTpnt = (NTAXIPT *) ((char *) pa + sizeof(NTAXIHDR));
			wTpnt = pth->wCount;

			nThisLen = pth->nLen;
			
			while (w < wTpnt)
			{	// For now just dump to txt file:
				LOCATION loct;
				float fLat, fLon;
				static char *pszTaxiPtTypes[] = { "?", "normal", "Hold Short", "?", "ILS Hold Short",
					"Gate/park", "ILS Hold Short No Draw", "Hold Short No Draw"};

				// Convert "no draw" holds to normal types ### 4880
				BYTE bType =
					(fNoDrawHoldConvert && (pTpnt[w].bType == 5)) ? 7 :
					pTpnt[w].bType;

				fprintf(fpAFDS, "          Taxipoint #%d, type %d (%s):  ",
					w, bType, bType < 8 ? pszTaxiPtTypes[bType] : "?");
				SetLocPos(&loct, 0, pTpnt[w].nLat, pTpnt[w].nLon, &fLat, &fLon, 0, 0);
				if (fDecCoords)
					fprintf(fpAFDS, "%.9f  %.9f", fLat, fLon);
				else
					WritePosition(&loct, 0);
				fprintf(fpAFDS, "  -- %s\n", pTpnt[w].bOrientation == 1 ? "Reverse" : "Forward");
				pTpnt[w].fLat = fLat; // For later ...
				pTpnt[w].fLon = fLon;
				w++;
			}
		}

		else if (!fDeletionsPass && id && (pa->wId == OBJTYPE_NEWTAXIPOINT))
		{	// New Taxi path record found
			NTAXIHDR *pth = (NTAXIHDR *) pa;
			WORD w = 0;
			pNTpnt = (NEWTAXIPT *) ((char *) pa + sizeof(NTAXIHDR));
			wTpnt = pth->wCount;

			nThisLen = pth->nLen;

			while (w < wTpnt)
			{	// For now just dump to txt file:
				LOCATION loct;
				float fLat, fLon;
				static char *pszTaxiPtTypes[] = { "?", "normal", "Hold Short", "?", "ILS Hold Short",
					"Gate/park", "ILS Hold Short No Draw", "Hold Short No Draw"};

				// Convert "no draw" holds to normal types ### 4880
				BYTE bType =
					(fNoDrawHoldConvert && (pNTpnt[w].bType == 5)) ? 7 :
				pNTpnt[w].bType;

				fprintf(fpAFDS, "          Taxipoint #%d, type %d (%s):  ",
					w, bType, bType < 8 ? pszTaxiPtTypes[bType] : "?");
				SetLocPos(&loct, 0, pNTpnt[w].nLat, pNTpnt[w].nLon, &fLat, &fLon, 0, 0);
				if (fDecCoords)
					fprintf(fpAFDS, "%.9f  %.9f", fLat, fLon);
				else
					WritePosition(&loct, 0);
				fprintf(fpAFDS, "  -- %s\n", pNTpnt[w].bOrientation == 1 ? "Reverse" : "Forward");
				pNTpnt[w].fLat = fLat; // For later ...
				pNTpnt[w].fLon = fLon;
				w++;
			}
		}

		else if (!fDeletionsPass && id && (pa->wId == OBJTYPE_HELIPAD))
			DoHelipadOnly((helipad_t*) pa, &chICAO[0]);
	
		if (wTpnt && wTname && wTpath)
		{	// Generate taxiway table for this airport's t5.csv file.
			memset(&rwy1, 0, sizeof(RWYLIST));
			memcpy(rwy1.r.chICAO, chICAO, 4);
			SetICAOFull(&rwy1, chICAO);
			if (pNTpnt)
			{	rwy1.pTaxiwayList = 
					fNewTaxiPath ?
					(fNewTaxiPath == -1) ?
						NewMakeTaxiwayList3((NTAXIPT *) pNTpnt, pTname, pNTpath3, wTpnt, wTname, wTpath) :
						NewMakeTaxiwayList2(pNTpnt, pTname, pNTpath2, wTpnt, wTname, wTpath) :
						MakeTaxiwayList2(pNTpnt, pTname, pTpath, wTpnt, wTname, wTpath);
			}
			else if (pTpnt)
			{
				rwy1.pTaxiwayList =
					fNewTaxiPath ?
					(fNewTaxiPath == -1) ?
					NewMakeTaxiwayList3(pTpnt, pTname, pNTpath3, wTpnt, wTname, wTpath) :
					NewMakeTaxiwayList(pTpnt, pTname, pNTpath, wTpnt, wTname, wTpath) :
					MakeTaxiwayList(pTpnt, pTname, pTpath, wTpnt, wTname, wTpath);

				/*TEMP ###########################################
				if ((strncmp("KATL", chICAO, 4) == 0) && rwy1.pTaxiwayList)
				{
					TWHDR *px = rwy1.pTaxiwayList;
					int ix = 0;
					while (px)
					{
						fprintf(fpAFDS, "TaxiName %d = %s\n", ix++, px->chName);
						px = (int)px + (px->wPoints * sizeof(TW)) + sizeof(TWHDR);
						if (px->wPoints == 0) break;
					}
				}
				// ################################################*/
			}

			if (rwy1.pTaxiwayList)
			{	strcpy(rwy1.r.chRwy, "998");
				AddRunway(&rwy1);			
			}

			wTpath = wTpnt = wTname = 0;
		}

		size -= nThisLen;
		pa =  (NAPT *) ((BYTE *) pa + nThisLen);
		if (nThisLen == 0)
		{	fprintf(fpAFDS, "#### Error: record length == 0\n");
			break;
		}
	}

	if (ap.fAirport)
	{	if (nCommStart || nCommDelStart)
			AddComms(chICAO, nCommStart, nCommEnd, nCommDelStart, nCommDelEnd, pApName);
		ap.pAirportName = pApName;
		ap.pCityName = pCitName;
		WriteFSM(&ap);
	}
}

/******************************************************************************
         ValidateMSFS2024AirportSubsection

         The QMID index already supplies the number of top-level airport
         records. Validate those records before handing the payload to
         NewApts(), and return the exact number of bytes occupied by them.
******************************************************************************/

static BOOL ValidateMSFS2024AirportSubsection(
    const BYTE *fileData,
    DWORD fileSize,
    DWORD payloadOffset,
    DWORD payloadSize,
    DWORD expectedRecords,
    DWORD *validatedSize)
{
    DWORD cursor = payloadOffset;
    DWORD endOffset;
    DWORD recordIndex;

    if (validatedSize)
        *validatedSize = 0;

    if (!fileData || !validatedSize || !expectedRecords)
        return FALSE;

    if (!IsValidBGLFileRange(payloadOffset, payloadSize, fileSize))
        return FALSE;

    endOffset = payloadOffset + payloadSize;

    for (recordIndex = 0; recordIndex < expectedRecords; recordIndex++)
    {
        WORD recordType;
        DWORD recordSize;

        if ((endOffset - cursor) < 6)
            return FALSE;

        recordType = ReadBGLWordLE(fileData + cursor);
        recordSize = ReadBGLDwordLE(fileData + cursor + 2);

        if (recordType != OBJTYPE_AIRPORT_MSFS2024)
            return FALSE;

        if ((recordSize < 6) || (recordSize > (endOffset - cursor)))
            return FALSE;

        cursor += recordSize;
    }

    *validatedSize = cursor - payloadOffset;
    return TRUE;
}

/******************************************************************************
         CheckMSFS2024BGLV9

         Dedicated MSFS2024 v9 container walker.

         Layer type 3 contains airport-related data, but may also contain
         other record families, such as the 0x00E8 records found in OBX
         files. Only subsections beginning with an Airport record 0x0113
         are passed to NewApts(). It prevents unrelated Terrain
         Vector DB, 3D Scenery and Airport Name payloads from being treated as
         linear airport records.
******************************************************************************/

static BOOL CheckMSFS2024BGLV9(FILE *fpIn, NBGLHDR *ph, DWORD fsize)
{
    const BGLV9_FILE_HEADER *header =
        (const BGLV9_FILE_HEADER *)ph;
    BYTE *p = NULL;
    DWORD headerSize;
    DWORD layerCount;
    DWORD layerTableSize;
    DWORD i;

    if (!IsMSFS2024BGLV9(ph, fsize))
        return FALSE;

    headerSize = header->headerSize;
    layerCount = header->layerCount;

   /*
    A v9 container may legitimately contain only the 0x38-byte
    header and no layer descriptors.
	*/
	if (!layerCount)
		return TRUE;

    if (layerCount > ((DWORD)-1 / (DWORD)sizeof(BGLV9_LAYER_DESCRIPTOR)))
    {
        fprintf(fpAFDS,
            "%s%s\n!!!! FAILED: MSFS2024 BGL v9 layer table overflow\n%s",
            chLine, szCurrentFilePath, chLine);
        return TRUE;
    }

    layerTableSize = layerCount * (DWORD)sizeof(BGLV9_LAYER_DESCRIPTOR);

    if (!IsValidBGLFileRange(headerSize, layerTableSize, fsize))
    {
        fprintf(fpAFDS,
            "%s%s\n!!!! FAILED: MSFS2024 BGL v9 layer table outside file\n%s",
            chLine, szCurrentFilePath, chLine);
        return TRUE;
    }

    p = (BYTE *)malloc(fsize);
    if (!p)
    {
        fprintf(fpAFDS,
            "%s%s\n!!!! FAILED: unable to allocate MSFS2024 BGL v9 buffer\n%s",
            chLine, szCurrentFilePath, chLine);
        return TRUE;
    }

    fseek(fpIn, 0, SEEK_SET);
    if (fread(p, 1, fsize, fpIn) != fsize)
    {
        free(p);
        fprintf(fpAFDS,
            "%s%s\n!!!! FAILED: error reading MSFS2024 BGL v9 file\n%s",
            chLine, szCurrentFilePath, chLine);
        return TRUE;
    }

    pOffsetBase = p;
	pMSFS2024BGLV9Data = p;
	nMSFS2024BGLV9FileSize = fsize;
	nMSFS2024BGLV9HeaderSize = headerSize;
	nMSFS2024BGLV9LayerCount = layerCount;
	BuildGlobalNvxIlsV9Index();
    ulTotalBytes += fsize - headerSize;

    if (!fDeletionsPass)
    {
        fprintf(fpAFDS, "%s%s\n%s", chLine, szCurrentFilePath, chLine);
        fprintf(fpAFDS,
            "MSFS2024 BGL v9: headerSize=0x%lX layers=%lu\n",
            (unsigned long)headerSize,
            (unsigned long)layerCount);
    }

    for (i = 0; i < layerCount; i++)
    {
        const BYTE *layer = p + headerSize + (i * (DWORD)sizeof(BGLV9_LAYER_DESCRIPTOR));
        DWORD layerType = ReadBGLDwordLE(layer);
        DWORD modeFlags = ReadBGLDwordLE(layer + 4);
        DWORD subsectionCount = ReadBGLDwordLE(layer + 8);
        DWORD indexOffset = ReadBGLDwordLE(layer + 12);
        DWORD indexSize = ReadBGLDwordLE(layer + 16);
        DWORD indexStride =
    		(modeFlags & BGLV9_QMID64_FLAG)
        		? (DWORD)sizeof(BGLV9_QMID64_ENTRY)
        		: (DWORD)sizeof(BGLV9_QMID32_ENTRY);
        DWORD requiredIndexSize;
        DWORD j;

        if (layerType != BGLV9_LAYER_AIRPORT)
            continue;

        if (subsectionCount > ((DWORD)-1 / indexStride))
        {
            fprintf(fpAFDS,
                "    *** INVALID MSFS2024 Airport subsection count: %lu\n",
				(unsigned long)subsectionCount);
            continue;
        }

        requiredIndexSize = subsectionCount * indexStride;

        if ((indexSize < requiredIndexSize) ||
            !IsValidBGLFileRange(indexOffset, indexSize, fsize))
        {
            fprintf(fpAFDS,
                "    *** INVALID MSFS2024 Airport subsection table\n");
            continue;
        }

        for (j = 0; j < subsectionCount; j++)
        {
            const BYTE *entry = p + indexOffset + (j * indexStride);
            DWORD itemCount;
            DWORD payloadOffset;
            DWORD payloadSize;
            DWORD validatedSize = 0;

            if (modeFlags & BGLV9_QMID64_FLAG)
            {
                itemCount = ReadBGLDwordLE(entry + 8);
                payloadOffset = ReadBGLDwordLE(entry + 12);
                payloadSize = ReadBGLDwordLE(entry + 16);
            }
            else
            {
                itemCount = ReadBGLDwordLE(entry + 4);
                payloadOffset = ReadBGLDwordLE(entry + 8);
                payloadSize = ReadBGLDwordLE(entry + 12);
            }

            if (fUserAbort)
            {
				pMSFS2024BGLV9Data = NULL;
                nMSFS2024BGLV9FileSize = 0;
                nMSFS2024BGLV9HeaderSize = 0;
                nMSFS2024BGLV9LayerCount = 0;
                pOffsetBase = NULL;
                free(p);
                return TRUE;
            }

           /*
				An empty QMID subsection requires no processing.
			*/
			if (!itemCount)
				continue;

			/*
				Validate the payload range before reading its first record type.
				This remains a structural error regardless of the record family.
			*/
			if (!IsValidBGLFileRange(
					payloadOffset,
					payloadSize,
					fsize) ||
				(payloadSize < sizeof(WORD)))
			{
				fprintf(fpAFDS,
					"    *** INVALID MSFS2024 layer 3 subsection %lu: "
					"records=%lu payload=0x%08lX size=0x%08lX\n",
					(unsigned long)j,
					(unsigned long)itemCount,
					(unsigned long)payloadOffset,
					(unsigned long)payloadSize);

				continue;
			}

			/*
				Layer type 3 contains airport-related data, but it is not
				guaranteed to contain Airport records.

				Airport records use top-level type 0x0113.
				OBX object records such as 0x00E8 are valid but are not handled
				by NewApts(), so their entire subsection is skipped silently.
			*/
			if (ReadBGLWordLE(p + payloadOffset) !=
				OBJTYPE_AIRPORT_MSFS2024)
			{
				continue;
			}

			/*
				From this point the subsection is expected to contain a sequence
				of MSFS2024 Airport records.
			*/
			if (!ValidateMSFS2024AirportSubsection(
					p,
					fsize,
					payloadOffset,
					payloadSize,
					itemCount,
					&validatedSize))
			{
				fprintf(fpAFDS,
					"    *** INVALID MSFS2024 Airport subsection %lu: "
					"records=%lu payload=0x%08lX size=0x%08lX\n",
					(unsigned long)j,
					(unsigned long)itemCount,
					(unsigned long)payloadOffset,
					(unsigned long)payloadSize);

				continue;
			}

            if (validatedSize != payloadSize)
			{
				fprintf(fpAFDS,
					"    *** WARNING: MSFS2024 Airport subsection %lu has "
					"%lu trailing bytes; payload=0x%08lX size=0x%08lX "
					"validated=0x%08lX\n",
					(unsigned long)j,
					(unsigned long)(payloadSize - validatedSize),
					(unsigned long)payloadOffset,
					(unsigned long)payloadSize,
					(unsigned long)validatedSize);
			}

            if (fDeletionsPass)
                fDeletionsPass = -1;

            NewApts(
                (NAPT *)(p + payloadOffset),
                validatedSize,
				0,
				NULL,
                p,
                NULL);
        }
    }

	pMSFS2024BGLV9Data = NULL;
    nMSFS2024BGLV9FileSize = 0;
    nMSFS2024BGLV9HeaderSize = 0;
    nMSFS2024BGLV9LayerCount = 0;
    pOffsetBase = NULL;
    free(p);
    return TRUE;
}

/******************************************************************************
         CheckNewBGL
******************************************************************************/

void CheckNewBGL(FILE *fpIn, NBGLHDR *ph, DWORD fsize)
{	NSECTS *ps;
	NREGION *pRegion = 0;	
	BOOL fRegion = FALSE;
	DWORD i, j, k;
	BYTE *p = NULL;

	/* Dedicated parser for the MSFS2024 v9 container. */
	if (CheckMSFS2024BGLV9(fpIn, ph, fsize))
		return;

	if (ph->nObjects > NSECTS_PER_FILE)
	{	fprintf(fpAFDS, "%s%s\n!!!! FAILED: too many sections (%d)\n%s",
				chLine, szCurrentFilePath, ph->nObjects, chLine);
		return;
	}
	
	// Section table
	ps = (NSECTS *) ((BYTE *) ph + ph->size);
	
	// Look for airports and VORs (ILSs)
	for (i = 0; i < ph->nObjects; i++)
	{	if (ps[i].nObjType == OBJTYPE_AIRPORT)
		{	DWORD offs = ps[i].nGroupOffset;

			if (!p)
			{	// Read complete file
				p = (BYTE *) malloc(fsize);
				ulTotalBytes += fsize - sizeof(NBGLHDR);
				pOffsetBase = p;

				fseek(fpIn, 0, SEEK_SET);
				if (!p || (fread(p, 1, fsize, fpIn) != fsize))
				{	if (p) free(p);
					fprintf(fpAFDS, "%s%s\n!!!! FAILED: error reading the file!\n%s",
						chLine, szCurrentFilePath, chLine);
					return;
				}

				if (!fDeletionsPass)
					fprintf(fpAFDS, "%s%s\n%s",	chLine, szCurrentFilePath, chLine);
			}
			
			if (fUserAbort)
			{	if (p) free(p);
					return;
			}
	
			for (j = 0; j < ps[i].nGroupsCount; j++)
			{	NOBJ *po = (NOBJ *) &p[offs];
		
				if (fUserAbort)
				{	if (p) free(p);
						return;
				}	

				if (!fRegion)
				{	// Look for regional data (for city names)
					fRegion = TRUE;
					for (k = 0; k < ph->nObjects; k++)
					{	DWORD offs = ps[k].nGroupOffset;
						if (fUserAbort)
						{	if (p) free(p);
							return;
						}
		
						if (ps[k].nObjType == OBJTYPE_REGIONINF)
						{	// Need to store details so can find City name, for IYP file
							NOBJ *po2 = (NOBJ *) &p[offs];
							pRegion = (NREGION *) &p[po2->chunkoff];
							break;
						}			
					}
				}

				if (fDeletionsPass) fDeletionsPass = -1;
				//NewApts((NAPT *) &p[po->chunkoff], po->chunksize, ph->nObjects, ps, p, pRegion);
				fprintf(fpAFDS,
					"    AIRPORT CHUNK: chunkoff=0x%08lX chunksize=%lu fileSize=%lu",
					(unsigned long)po->chunkoff,
					(unsigned long)po->chunksize,
					(unsigned long)fsize);

				if (po->chunkoff >= fsize)
				{
					fprintf(fpAFDS, "  *** INVALID chunkoff >= fileSize\n");
				}
				else if ((po->chunkoff + po->chunksize) > fsize)
				{
					fprintf(fpAFDS, "  *** INVALID chunk outside file\n");
				}
				else if (po->chunksize < 8)
				{
					fprintf(fpAFDS, "  *** INVALID chunksize too small\n");
				}
				else
				{
					NAPT *dbgpa = (NAPT *)&p[po->chunkoff];

					fprintf(fpAFDS,
						"  firstRecord: wId=0x%04X nLen=%u\n",
						dbgpa->wId,
						dbgpa->nLen);

					NewApts((NAPT *) &p[po->chunkoff], po->chunksize, ph->nObjects, ps, p, pRegion);
				}				
				offs += sizeof(NOBJ);
			}
		}
	}

	if (p) free(p);
	pOffsetBase = NULL;
}

/******************************************************************************
         End of NewBGLs
******************************************************************************/
