/* NewBGLs.c
*******************************************************************************/

#include "MakeRwys.h"

extern char chRwyT[6];
extern char szParkNames[12][5];
extern char szCurrentFilePath[MAX_PATH];
int nOffsetBase = 0;
int *pLastSetGateList = 0;
int fDeletionsPass = 0, nMinRunwayLen = 1500;
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
					"?", "Vehicles", "?", "?" };
				

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
	"Concrete", // 24 ###added 230719 (fiddle?)
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
         Assorted conversions
******************************************************************************/

static __int64 fslat2lat(int ndblat, float *pf, double *pd)
{	double f = (((double)ndblat)*180)/MAXSOUTH;
	if (f>90.0)	f = -((f-180)+90);
	else f = 90-f;
	if (pd) *pd = f;
	if (pf) *pf = (float) f;
	return (__int64) ((f * 10001750.0) / 90.0);
}

static __int64 fslon2lon(int ndblon, float *pf, double *pd)
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
int nHelipadCtr = 0;

int hcompare(const void *arg1, const void *arg2)
{	HELI *h1 = (HELI *) arg1;
	HELI *h2 = (HELI *) arg2;

	return _strnicmp(h1->chICAO, h2->chICAO, 4);
}

void MakeHelipadsFile(void)
{	FILE *phf = fopen("helipads.csv", "wb");
	int i = 0;

	if (phf)
	{	if (nHelipadCtr > 1)
			// First sort array into ICAO order
			qsort((void *) &helipads[0], (size_t) nHelipadCtr,
				sizeof(HELI), hcompare);

		while (i < nHelipadCtr)
		{	if (helipads[i].fDelete == 0)
				fprintf(phf, "%.4s,%.6f,%.6f,%.0f,%.1f,%d,%d,%d,%d\n",
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
{	int i = 0;
	while (i < nHelipadCtr)
	{	if (strncmp(helipads[i].chICAO, pchICAO, 4) == 0)
			helipads[i].fDelete = 255;
		i++;
	}
}

/******************************************************************************
        DoHelipadOnly
******************************************************************************/

void DoHelipadOnly(helipad_t* ph, char *pszICAO)
{	static char *pszFlags[5] = { "", "H", "Square", "Circle", "Medical" };
	static char szPrevICAO[5];
	char chWork[64];
	double dLat, dLon;
	HELI *phs = &helipads[nHelipadCtr++];
		
	*((DWORD *) (&phs->chICAO[0])) = *((DWORD *) pszICAO);
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
	
	if (strncmp(pszICAO, szPrevICAO, 4) != 0)
		fprintf(fpAFDS, "\n");
	strncpy(szPrevICAO, pszICAO, 5);
	fprintf(fpAFDS, "          HELIPAD at %s: %s %s %s",
		pszICAO, pszFlags[ph->bFlags & 0x0f],
		ph->bFlags & 0x10 ? "transparent" : "",
		ph->bFlags & 0x20 ? "closed" : "");

	fprintf(fpAFDS, "\n              Surface=%s, Lat=%.6f, Lon=%.6f, Alt=%.0fft",
		ph->bSurface > 24 ? chWork : szNRwySurf[ph->bSurface],
		dLat, dLon, (double) phs->fAlt);
	fprintf(fpAFDS, "\n              Length=%dft, Width=%dft, Heading=%.1fT\n\n",
		phs->sLen, phs->sWidth, (double) ph->fHeading);
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
         ProcessRunwayList
******************************************************************************/

void ProcessRunwayList(RWYLIST *pL, BOOL fAdd, BOOL fNoCtr)
{	BOOL fDelAll = (fAdd <= 0) && !pL->r.chRwy[0] && !pL->fAirport;
	int nCompLen = fDelAll ? 4 : 8;

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
	{	int fDir = 0;

		if (fAdd <= 0) pRlast = pR; // deletes need search from start
										
		while (pRlast) 
		{	int comp = memcmp(&pRlast->r, &pL->r, nCompLen);
			if (fUserAbort) return;

			if (fNewAirport && (memcmp(&pRlast->r, &pL->r, 4) == 0))
			{	ulTotalAPs--;
				fNewAirport = FALSE;	
			}

			if (comp == 0) // Duplicate (or found one to delete)
			{	if (fAdd > 0)
				{	// Copy in new data and then discard alloc!
					// NB Except for gates and taxiways, when keep first ones if exist
					// But keep ILS/ATIS if the new one hasn't got it or it is < 118.00!
					if (pL->r.chILS[1] == 0)
					{	memcpy(pL->r.chILS, pRlast->r.chILS, sizeof(pL->r.chILS));
						memcpy(pL->r.chILSHdg, pRlast->r.chILSHdg, sizeof(pL->r.chILSHdg));
						pL->fILSflags = pRlast->fILSflags;
					}
					
					if (pL->Atis < 0x1800)
						pL->Atis = pRlast->Atis;
					
					memcpy(&pRlast->r, &pL->r, sizeof(RWYDATA));
					
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
		ProcessRunwayList(pL, TRUE, 0);
		if (!pL->fAirport) fFS9 = 1; // Stay in >=FS9 mode		
	}
}

/******************************************************************************
         SetLocPos
******************************************************************************/

void SetLocPos(LOCATION *pLoc, int alt, int lat, int lon, float *pflat, float *pflon, double *pdLat, double *pdLon)
{	DWORD wk;
	pLoc->elev = (int) ((alt * 65536.0) / 1000.0);
	*((__int64 *) &pLoc->lat.f) = fslat2lat(lat, pflat, pdLat);
	wk = (DWORD) pLoc->lat.f;
	pLoc->lat.i = (int) pLoc->lat.f;
	pLoc->lat.f = wk;
	*((__int64 *) &pLoc->lon.f) = fslon2lon(lon, pflon, pdLon);
	wk = (DWORD) pLoc->lon.f;
	pLoc->lon.i = (int) pLoc->lon.f;
	pLoc->lon.f = wk;
}

/******************************************************************************
         StoreName
******************************************************************************/

void StoreName(char *psz, NNAM *pName)
{	psz[0] = 0;
	if (pName->nLen > 6)
	{	int nlen = min(pName->nLen-6, 255);
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
int nOffsetILS, nSizeILS;
char chNameILS[256];
float fILSheading, fILSslope, fRange2;

// Mode 0 = Match ID & Dir
//      1 = Match Name & Dir & Range
//      -1 = List all by Range

int NewILSs(NILS *pi, DWORD size, char *psz, RWYLIST *prwy, int nMode)
{	char chId[16], chRwy[6], *pch;
	NILS *pi2;
	DWORD size2;
	BOOL fInRange = FALSE;
	
	fMatchedILS = FALSE;
	chNameILS[0] = 0;

	while ((size > 6) && (size >= pi->nLen))
	{	int nThisLen = sizeof(NILS);

		if (fUserAbort) return 0;
		
		if ((pi->wId == OBJTYPE_VOR) && (pi->bType == 4))
		{	// ILS record found
			BOOL fMatchID;
			nThisLen = pi->nLen;
			fILSheading = pi->loc.fHeading;
			fILSslope = (pi->loc.nRec0014 == 0x0015) ? ((NILSGS *) &pi->loc.nRec0014)->fGSpitch :
						*((WORD *) ((BYTE *) &pi->loc.fWidth + 4)) == 0x0015 ?
							((NILSGS *) ((BYTE *) &pi->loc.fWidth + 4))->fGSpitch : 0.00F;
			nOffsetILS = (int) &pi->wId - nOffsetBase;
			nSizeILS = pi->nLen;
			
			DecodeID(pi->nId, chId, 1);
			fMatchID = _stricmp(chId, psz) == 0;

			if (fMatchID && (HdgDiff(pi->loc.fHeading, prwy->fHdg) < 40.0))
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

	if (nMode >= 0) prwy->fILSflags = 0;
	return 0;
}

/******************************************************************************
         MatchILS
******************************************************************************/

int MatchILS(DWORD nObjs, NSECTS *ps, BYTE *p, char *psz, RWYLIST *prwy, int nMode)
{	// Look for VORs (ILSs)
	DWORD i, j;

	for (i = 0; i < nObjs; i++)
	{	DWORD offs = ps[i].nGroupOffset;

		if (fUserAbort) return 0;
		
		for (j = 0; j < ps[i].nGroupsCount; j++)
		{	if (ps[i].nObjType == OBJTYPE_VOR)
			{	NOBJ *po = (NOBJ *) &p[offs];

				int nFreq = NewILSs((NILS *) &p[po->chunkoff], po->chunksize, psz, prwy, nMode);
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
	{	int nThisLen = pa->nLen;

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
	{	int nThisLen = pa->nLen, i = 0, x, j;
		char wk[256];
		BYTE* pb = (BYTE *) pa;

		if (fUserAbort) return;
		
		fprintf(fpAFDS, "### AFTER RWY: at OFFSET %08X ID=%04X LEN=%d\n",
			(int) ((BYTE *) &pa->wId - nOffsetBase), pa->wId, nThisLen);
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
	{	int nThisLen = pa->nLen;

		if (fUserAbort) return 0;

		prwy->nOffThresh = 0;

		if ((pa->wId == nType) || (pa->wId == -nType))
		{	// Threshold record found
			NOFFTHR *ps = (NOFFTHR *) ((BYTE *) pa + ((nType < 0) ? 16 : 0));

			nThisLen = pa->nLen;
			prwy->nOffThresh = (unsigned int) ((ps->fLength * 3.28084F) + .5F);

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
	{	int nThisLen = pa->nLen;

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
	{	int nThisLen = pa->nLen;

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
	int wp1, wp2;
	int nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg;
	NGATE2 **ppg2;
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
					{	int wrk = wp1;
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
			{	int wp1x = (-wp1) - 1;
				SetLocPos(&locg, 0, 
					ppg2 ? (*(ppg2[wp1x])).nLat : (*(ppg[wp1x])).nLat,
					ppg2 ? (*(ppg2[wp1x])).nLon : (*(ppg[wp1x])).nLon,
					&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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
				{	int wp2x = (-wp2)-1;
					SetLocPos(&locg, 0,
						ppg2 ? (*(ppg2[wp2x])).nLat : (*(ppg[wp2x])).nLat,
						ppg2 ? (*(ppg2[wp2x])).nLon : (*(ppg[wp2x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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

	//fprintf(fpAFDS, "Taxiway Data Size = %d, Orig Allocation = %d\n",
	//	(int) &twh[1] - (int) twh0, nAllocSize);

	if (nAllocSize > ((int) &twh[1] - (int) twh0))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + (int) &twh[1] - (int) twh0);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, (int) &twh[1] - (int) twh0);
			free(twh0);
			twh0 = twh1;
		}
	}

//	if (pLastSetGateList)
//	{	free(pLastSetGateList);
//		pLastSetGateList = 0;
//	}

	return twh0;
}

/******************************************************************************
         MakeTaxiwayList2 for sloped taxiways
******************************************************************************/

TWHDR *MakeTaxiwayList2(NEWTAXIPT *pT, NTAXINM *pN, NTAXI *pP, WORD wT, WORD wN, WORD wP)
{	WORD wo = wN, w2;
	int wp1, wp2;
	int nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg;
	NGATE3 **ppg3;
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
					{	int wrk = wp1;
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
			{	int wp1x = (-wp1) - 1;
				SetLocPos(&locg, 0, 
					ppg3 ? (*(ppg3[wp1x])).nLat : (*(ppg[wp1x])).nLat,
					ppg3 ? (*(ppg3[wp1x])).nLon : (*(ppg[wp1x])).nLon,
					&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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
				{	int wp2x = (-wp2)-1;
					SetLocPos(&locg, 0,
						ppg3 ? (*(ppg3[wp2x])).nLat : (*(ppg[wp2x])).nLat,
						ppg3 ? (*(ppg3[wp2x])).nLon : (*(ppg[wp2x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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

	//fprintf(fpAFDS, "Taxiway Data Size = %d, Orig Allocation = %d\n",
	//	(int) &twh[1] - (int) twh0, nAllocSize);

	if (nAllocSize > ((int) &twh[1] - (int) twh0))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + (int) &twh[1] - (int) twh0);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, (int) &twh[1] - (int) twh0);
			free(twh0);
			twh0 = twh1;
		}
	}

//	if (pLastSetGateList)
//	{	free(pLastSetGateList);
//		pLastSetGateList = 0;
//	}

	return twh0;
}

/******************************************************************************
         NewMakeTaxiwayList
******************************************************************************/

TWHDR *NewMakeTaxiwayList(NTAXIPT *pT, NTAXINM *pN, NEWNTAXI *pP, WORD wT, WORD wN, WORD wP)
{	WORD wo = wN, w2;
	int wp1, wp2;
	int nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg;
	NGATE2 **ppg2;
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
					{	int wrk = wp1;
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
			{	int wp1x = (-wp1) - 1;
				SetLocPos(&locg, 0, 
					ppg2 ? (*(ppg2[wp1x])).nLat : (*(ppg[wp1x])).nLat,
					ppg2 ? (*(ppg2[wp1x])).nLon : (*(ppg[wp1x])).nLon,
					&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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
				{	int wp2x = (-wp2)-1;
					SetLocPos(&locg, 0,
						ppg2 ? (*(ppg2[wp2x])).nLat : (*(ppg[wp2x])).nLat,
						ppg2 ? (*(ppg2[wp2x])).nLon : (*(ppg[wp2x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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

	//fprintf(fpAFDS, "Taxiway Data Size = %d, Orig Allocation = %d\n",
	//	(int) &twh[1] - (int) twh0, nAllocSize);

	if (nAllocSize > ((int) &twh[1] - (int) twh0))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + (int) &twh[1] - (int) twh0);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, (int) &twh[1] - (int) twh0);
			free(twh0);
			twh0 = twh1;
		}
	}

//	if (pLastSetGateList)
//	{	free(pLastSetGateList);
//		pLastSetGateList = 0;
//	}

	return twh0;
}

/******************************************************************************
         NewMakeTaxiwayList2 for sloped taxiways
******************************************************************************/

TWHDR *NewMakeTaxiwayList2(NEWTAXIPT *pT, NTAXINM *pN, NEWNTAXI2 *pP, WORD wT, WORD wN, WORD wP)
{	WORD wo = wN, w2;
	int wp1, wp2;
	int nAllocSize = (40*wP) + (32*wT*wP);
	TWHDR *twh = (TWHDR *) malloc(nAllocSize);
	TWHDR *twh0 = twh;

	WORD wGateCtr = 0;
	NGATE **ppg;
	NGATE2 **ppg2;
	NGATE3 **ppg3;
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
					{	int wrk = wp1;
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
			{	int wp1x = (-wp1) - 1;
				SetLocPos(&locg, 0, 
					ppg3 ? (*(ppg3[wp1x])).nLat : ppg2 ? (*(ppg2[wp1x])).nLat : (*(ppg[wp1x])).nLat,
					ppg3 ? (*(ppg3[wp1x])).nLon : ppg2 ? (*(ppg2[wp1x])).nLon : (*(ppg[wp1x])).nLon,
					&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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
				{	int wp2x = (-wp2)-1;
					SetLocPos(&locg, 0,
						ppg3 ? (*(ppg3[wp2x])).nLat : ppg2 ? (*(ppg2[wp2x])).nLat : (*(ppg[wp2x])).nLat,
						ppg3 ? (*(ppg3[wp2x])).nLon : ppg2 ? (*(ppg2[wp2x])).nLon : (*(ppg[wp2x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);								
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

	//fprintf(fpAFDS, "Taxiway Data Size = %d, Orig Allocation = %d\n",
	//	(int) &twh[1] - (int) twh0, nAllocSize);

	if (nAllocSize > ((int) &twh[1] - (int) twh0))
	{	// Revise allocation to suit needs
		TWHDR *twh1 = malloc(32 + (int) &twh[1] - (int) twh0);
		if (twh1)
		{	memcpy((BYTE *) twh1, (BYTE *) twh0, (int) &twh[1] - (int) twh0);
			free(twh0);
			twh0 = twh1;
		}
	}

//	if (pLastSetGateList)
//	{	free(pLastSetGateList);
//		pLastSetGateList = 0;
//	}

	return twh0;
}

/******************************************************************************
		 NewMakeTaxiwayList3 for MSFS
******************************************************************************/

TWHDR* NewMakeTaxiwayList3(NTAXIPT* pT, NTAXINM* pN, MSFSNTAXI* pP, WORD wT, WORD wN, WORD wP)
{
	WORD wo = wN, w2;
	int wp1, wp2;
	int nAllocSize = (40 * wP) + (32 * wT * wP);
	TWHDR* twh = (TWHDR*)malloc(nAllocSize);
	TWHDR* twh0 = twh;

	WORD wGateCtr = 0;
	NGATE2** ppg4;
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
						int wrk = wp1;
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
				int wp1x = (-wp1) - 1;
				SetLocPos(&locg, 0,
					(*(ppg4[wp1x])).nLat, (*(ppg4[wp1x])).nLon,
					&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
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
					int wp2x = (-wp2) - 1;
					SetLocPos(&locg, 0,
						(*(ppg4[wp2x])).nLat, (*(ppg4[wp2x])).nLon,
						&tw[wPts].fLat, &tw[wPts].fLon, 0, 0);
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

	//fprintf(fpAFDS, "Taxiway Data Size = %d, Orig Allocation = %d\n",
	//	(int) &twh[1] - (int) twh0, nAllocSize);

	if (nAllocSize > ((int)&twh[1] - (int)twh0))
	{	// Revise allocation to suit needs
		TWHDR* twh1 = malloc(32 + (int)&twh[1] - (int)twh0);
		if (twh1)
		{
			memcpy((BYTE*)twh1, (BYTE*)twh0, (int)&twh[1] - (int)twh0);
			free(twh0);
			twh0 = twh1;
		}
	}

	//	if (pLastSetGateList)
	//	{	free(pLastSetGateList);
	//		pLastSetGateList = 0;
	//	}

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

void FindILSdetails(DWORD nObjs, NSECTS* ps, BYTE* p, char* psz, RWYLIST* prwy, int nMode)
{
	float fILSHdgMag;
	int nFreq = MatchILS(nObjs, ps, p, psz, prwy, nMode);

	if (!nFreq)
		return;

	if (!fMatchedILS)
		return;

	sprintf(prwy->r.chILS, "%.2f", nFreq / 100.0);
	fILSHdgMag = fILSheading - prwy->fMagvar;
	if (fILSHdgMag <= 0.0F) fILSHdgMag += 360.0F;
	else if (fILSHdgMag > 360.0F) fILSHdgMag -= 360.0F;

	char wk[8];
	int l = l = sprintf(wk, "%.3f", (double)fILSHdgMag);
	if (l > 5) wk[5] = 0;
	else if (wk[l - 1] == '0') wk[l - 1] = 0;
	memcpy(prwy->r.chILSHdg, wk, 6);

	prwy->r.fILSslope = fILSslope;

	fprintf(fpAFDS, "\n              ");
	fprintf(fpAFDS, "ILS: %s  %s Hdg: %.1f %s%s%s%s  Slope: %.2f, \x22%s\x22",
		psz, prwy->r.chILS, (double)fILSheading,
		(prwy->fILSflags & 0x1C) ? ", Flags:" : "",
		(prwy->fILSflags & 0x08) ? " GS" : "",
		(prwy->fILSflags & 0x10) ? " DME" : "",
		(prwy->fILSflags & 0x04) ? " BC" : "",
		(double) prwy->r.fILSslope, chNameILS);
		
	strncpy(prwy->r.chNameILS, chNameILS, 31);
	prwy->r.chNameILS[31] = 0;
}

/******************************************************************************
		 GetNamestring
******************************************************************************/

void GetNameString(char* p)
{	char* psz;
	if ((*p == 0) || strnicmp(p, "TT:AIRPORT", 10))
		return;
	psz = strstr(pLocPak, &p[3]);
	if (psz)
	{	char* psz2;
		psz = strstr(psz, ": \x22");
		if (psz)
		{	psz += 3;
			psz2 = strchr(psz, '\x22');
			strncpy(p, psz, (int)(psz2 - psz));
			p[psz2 - psz] = 0;
		}
	}
	else p[0] = 0;
}

/******************************************************************************
         NewApts
******************************************************************************/

void NewApts(NAPT *pa, DWORD size, DWORD nObjs, NSECTS *ps, BYTE *p, NREGION *pRegion)
{	DWORD id = 0;
	char chICAO[5], chILSidP[8], chILSidS[8];
	char chWork[48], chWork2[16];
	LOCATION loc;
	float fMagvar, fHeading, fapLat, fapLon;
	RWYLIST rwy1, rwy2, ap;
	WORD wTpath = 0, wTpnt = 0, wTname = 0;
	NTAXI *pTpath = 0;
	NEWNTAXI *pNTpath = 0;
	NEWNTAXI2 *pNTpath2 = 0;
	MSFSNTAXI* pNTpath3 = 0;
	NTAXIPT *pTpnt = 0;
	NEWTAXIPT *pNTpnt = 0;
	NTAXINM *pTname = 0;
	int nCommStart = 0, nCommEnd = 0;
	int nCommDelStart = 0, nCommDelEnd = 0;
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
	{	int nThisLen = pa->nLen;
		if (fUserAbort) return;

		if ((pa->wId == OBJTYPE_AIRPORT) || (pa->wId == OBJTYPE_NEWAIRPORT) ||
				(pa->wId == OBJTYPE_NEWNEWAIRPORT) || (pa->wId == OBJTYPE_AIRPORT_MSFS))
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

			if (!fDeletionsPass)
			{	if (fDebug)	fprintf(fpAFDS,"OFFSET %08X-%08X:  ", (int) &pa->wId - nOffsetBase, (int) &pa->wId - nOffsetBase + nThisLen);
				
				fprintf(fpAFDS, "\nAirport %s :", chICAO);
				
				SetLocPos(&loc, pa->nAlt, pa->nLat, pa->nLon, &fapLat, &fapLon, 0, 0);
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
				{	int nICAOs = pRegion->wIcaoCount;
					NICAO *pICAOs = (NICAO *) ((BYTE *) pRegion + pRegion->nIcaoPtr);

					while (nICAOs--)
					{	if (pICAOs->nId == id)
						{	// Found this ICAO id
							DWORD *ppCountries = (DWORD *) ((BYTE *) pRegion + pRegion->nCountryPtr);
							int nCountries = pRegion->wCountryCount, nCountryNum = pICAOs->bCountryIndex;
							DWORD *ppStates = (DWORD *) ((BYTE *) pRegion + pRegion->nStatePtr);
							int nStates = pRegion->wStateCount, nStateNum = pICAOs->wStateIndex / 16;
							DWORD *ppCities = (DWORD *) ((BYTE *) pRegion + pRegion->nCityPtr);
							int nCities = pRegion->wCityCount, nCityNum = pICAOs->wCitiesIndex;
							DWORD *ppAirports = (DWORD *) ((BYTE *) pRegion + pRegion->nAirportPtr);
							int nAirports = pRegion->wAirportCount, nAirportNum = pICAOs->wAirportIndex;

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
				int fDelMode =
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
			{	int i, j = 13;
				
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
				int i, j = 12 + (pd[8] * 4) + (pd[9] * 4);
				if (!nCommDelStart) nCommDelStart = ftell(fpAFDS);
				for (i = 0; i < pd[10]; i++) if (pd[j+3] & 0xf0)
				{	int type = (pd[j+3] >> 28) & 0xff;		//  ########################### ?????????? >>28 on a Byte?
					fprintf(fpAFDS, "          COM: Delete, Type=%d (%s), Freq=%.2f\n",
						type, pszComms[(type > 15) ? 16 : type],
						(double) ((*((int *) &pd[j]) & 0x0fffffff) / 10000) / 100.0);
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
			int nFreq = 0, fOk = 0, fList = 0;
			ANGLE Rlat, Rlong;
			WORD wSurf;

			nThisLen = pr->nLen;

			memset(&rwy1, 0, sizeof(RWYLIST));
			memcpy(rwy1.r.chICAO, chICAO, 4);
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

			wSurf = pr->wSurface & 0x7f; // remove transparency flag ### 240719
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
					{	fprintf(fpAFDS, "### %sRunway1 struct (len=%04X, size=%04x):\n",
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
						fprintf(fpAFDS,"### %sRunway2 struct (len=%04X, size=%04x):\n",
							(pa->wId == OBJTYPE_NEWRUNWAY) ? "New" : "", pa->nLen, sizeof(NRWY));

					if (pa->wId == OBJTYPE_RUNWAY)
					{	FindOffThresh(&rwy1, (NAPT *) ((BYTE *) pa + sizeof(NRWY)), nThisLen - sizeof(NRWY), 6);
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
				(int) ((pr->fLength * 3.28084) + 0.5),
				(int) ((pr->fWidth * 3.28084) + 0.5));					

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

		/**********************************************************************
		else if (id && (pa->wId == OBJTYPE_NAME))
		{	// Airport name found

			// Find City Name first
			if (pRegion)
			{	int nICAOs = pRegion->wIcaoCount;
				NICAO *pICAOs = (NICAO *) ((BYTE *) pRegion + pRegion->nIcaoPtr);

				while (nICAOs--)
				{	if (pICAOs->nId == id)
					{	// Found this ICAO id
						DWORD *ppCities = (DWORD *) ((BYTE *) pRegion + pRegion->nCityPtr);
						int nCities = pRegion->wCityCount, nCityNum = pICAOs->wCitiesIndex;

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

			StoreName(pNextAirportName, (NNAM *) pa);
			fprintf(fpAFDS, "          Airport Name=\x22%s\x22\n", pNextAirportName);
			pApName = pNextAirportName;
			pNextAirportName += strlen(pNextAirportName) + 1;
		}
		//****************************************************************************/

		else if (!fDeletionsPass && id && (pa->wId == OBJTYPE_APCOMM))
		{	// Airport comms record found
			NCOMM *pc = (NCOMM *) pa;
			char chName[256];
			chName[0] = 0;

			if (pc->nLen > sizeof(NCOMM))
			{	int nlen = min(pc->nLen - sizeof(NCOMM), 255);
				memcpy(chName, (char *) pc + sizeof(NCOMM), nlen);
				while (nlen && !isalnum((unsigned char) chName[nlen - 1])) nlen--;
				chName[nlen] = 0; // Get rid of bad terminations
			}

			if (!nCommStart) nCommStart = ftell(fpAFDS);

			fprintf(fpAFDS, "          COM: Type=%d (%s), Freq=%.2f, Name=\x22%s\x22\n",
					pc->bCommType, pszComms[(pc->bCommType > 15) ? 16 : pc->bCommType],
					(double) ((pc->nFreq + 1000)/ 10000)/100.0,	chName);

			nCommEnd = ftell(fpAFDS);

			if (ap.fAirport && ((pc->bCommType == 1) || (((pc->bCommType == 12) || (pc->bCommType == 13)) && (ap.Atis < 0x1800))))
			{	// ATIS / AWOS / ASOS
				int nAtis =((pc->nFreq + 5000) / 10000) % 10000;

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
			int nGateNum = pjw->wParkingNumber;
			BOOL fJetwayOk = FALSE;

			NGATEHDR *pgh= prwyPrevious->pGateList;
            WORD w = 0, wCtr = pgh->wCount;
			NGATE *pg = (NGATE *) ((BYTE *) pgh + sizeof(NGATEHDR));
			NGATE2 *pg2 = (NGATE2 *) ((pgh->wId == OBJTYPE_NEWTAXIPARK) ? pg : 0);
			NGATE3 *pg3 = (NGATE3 *) ((pgh->wId == OBJTYPE_NEWNEWTAXIPARK) ? pg : 0);
			// ############### MSFSTAXIPARK ?? #########

			fprintf(fpAFDS, "          Gate %d has Jetway\n", nGateNum);
			if (fDebugThisEntry)
				fprintf(fpAFDS,"\nSearching %d entries:\n   ", wCtr);
	
			while (w < wCtr)
			{	if (fDebugThisEntry)
					fprintf(fpAFDS,"%d,", (pg->wNumberType >> 4));

				if ((pg->wNumberType >> 4) == nGateNum)
            	{	pg->bCodeCount |= 0x80; // Flag Jetway using top bit of code count
					fJetwayOk = TRUE;
					break;
				}

				w++;
				pg = (NGATE *) ((BYTE *) pg + (4 * (pg->bCodeCount & 0x7f)) + (pg3 ? sizeof(NGATE3) : pg2 ? sizeof(NGATE2) : sizeof(NGATE)));
				if (pg2) pg2 = (NGATE2 *) pg;
				if (pg3) pg3 = (NGATE3 *) pg;
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
			if (pa->wId == OBJTYPE_MSFSTAXIPARK) pg2 = (NGATE2 *) pg;

			nThisLen = pgh->nLen;

			// Will add to data for g5.csv file
			memset(&rwy1, 0, sizeof(RWYLIST));
			memcpy(rwy1.r.chICAO, chICAO, 4);
			rwy1.pGateList = (NGATEHDR *) malloc(nThisLen);
			
			pLastSetGateList = (int *) malloc(4 * (wCtr+1));
			if (pLastSetGateList)
				pLastSetGateList[0] = (int) wCtr | (pa->wId << 16);
			
			strcpy(rwy1.r.chRwy, "999");
			memcpy(rwy1.pGateList, pa, nThisLen);
			AddRunway(&rwy1);
			
			while (w < wCtr)
			{	// For now just dump to txt file:
				LOCATION locg;
				float fLat, fLon;
				char *pszGateName, chGateName[5];

				pg->bPushBackName &= 0x3f;
				if (pg->bPushBackName > 11)
				{	chGateName[0] = pg->bPushBackName + 0x35;
					chGateName[1] = 0;
					pszGateName = &chGateName[0];
				}

				else pszGateName = szParkNames[pg->bPushBackName];
				
				fprintf(fpAFDS, "          %s %s%d [#G%d]:  ", 
					pszParkType[pg->bPushBackName >= 11 ? 10 : pg->bPushBackName],
					pszGateName, pg->wNumberType >> 4, w);
				SetLocPos(&locg, 0, 
					pg3 ? pg3->nLat : pg2 ? pg2->nLat : pg->nLat,
					pg3 ? pg3->nLon : pg2 ? pg2->nLon : pg->nLon,
					&fLat, &fLon, 0, 0);
				WritePosition(&locg, 0);
				fprintf(fpAFDS, "\n");
				
				fprintf(fpAFDS, "              Type %d (%s), Size %.1fm, Hdg %.1fT\n",
					pg->wNumberType & 15, pszGateType[pg->wNumberType & 15],
					(double) pg->fRadius, (double) pg->fHeading);	

				if (pg->bCodeCount & 0x7f)
				{	BYTE b = pg->bCodeCount & 0x7f;
					char *pA = (char *) pg +
						(pg3 ? sizeof(NGATE3) :
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
				pLastSetGateList[w] = (int) pg; // Index for use in Taxipath decode
				pg = (NGATE *) ((BYTE *) pg + (4 * (pg->bCodeCount & 0x7f)) + 
					(pg3 ? sizeof(NGATE3) : pg2 ? sizeof(NGATE2) : sizeof(NGATE)));
				if (pa->wId == OBJTYPE_MSFSTAXIPARK)
					pg = (NGATE*) ((BYTE *) pg + 20); // 20 additional bytes
				if (pg2) pg2 = (NGATE2 *) pg;
				if (pg3) pg3 = (NGATE3 *) pg;
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

					pNTpath[w].wEnd &= 0x0fff;
					pNTpath[w].wStart &= 0x0fff;
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

					pNTpath3[w].wEnd &= 0x0fff;
					pNTpath3[w].wStart &= 0x0fff;
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

					pNTpath2[w].wEnd &= 0x0fff;
					pNTpath2[w].wStart &= 0x0fff;
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
			if (pNTpnt)
			{	rwy1.pTaxiwayList = 
					fNewTaxiPath ?
					(fNewTaxiPath == -1) ?
						NewMakeTaxiwayList3(pNTpnt, pTname, pNTpath3, wTpnt, wTname, wTpath) :
						NewMakeTaxiwayList2(pNTpnt, pTname, pNTpath2, wTpnt, wTname, wTpath) :
						MakeTaxiwayList2(pNTpnt, pTname, pTpath, wTpnt, wTname, wTpath);
			}
			else if (pTpnt)
			{	rwy1.pTaxiwayList = 
					fNewTaxiPath ?
					(fNewTaxiPath == -1) ?
						NewMakeTaxiwayList3(pTpnt, pTname, pNTpath3, wTpnt, wTname, wTpath) :
						NewMakeTaxiwayList(pTpnt, pTname, pNTpath, wTpnt, wTname, wTpath) :
						MakeTaxiwayList(pTpnt, pTname, pTpath, wTpnt, wTname, wTpath);
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
         CheckNewBGL
******************************************************************************/

void CheckNewBGL(FILE *fpIn, NBGLHDR *ph, DWORD fsize)
{	NSECTS *ps;
	NREGION *pRegion = 0;	
	BOOL fRegion = FALSE;
	DWORD i, j, k;
	BYTE *p = NULL;

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
				nOffsetBase = (int) p;

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
				NewApts((NAPT *) &p[po->chunkoff], po->chunksize, ph->nObjects, ps, p, pRegion);
				offs += sizeof(NOBJ);
			}
		}
	}

	if (p) free(p);
}

/******************************************************************************
         End of NewBGLs
******************************************************************************/
