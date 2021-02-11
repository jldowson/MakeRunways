/* MakeRwys.c
*******************************************************************************/

#include "MakeRwys.h"
#include "shfolder.h"
#include <winver.h>

BOOL fMSFS = FALSE, fLocal = FALSE, fDecCoords = FALSE;
char *pLocPak = NULL, *pContent = NULL, *pMaterials = NULL;
__int32 nVersion = -1;// <0 FS9, 0 FSX, 1 FSX-SE, 2 Prepar3D, 3 Prepar3D v2, 4 Prepar3D v3, 5 Prepar3D v4, 6 Prepar3D v5, 7 MSFS

#ifdef _DEBUG
	BOOL fDoMSFS = TRUE;
#else
	BOOL fDoMSFS = TRUE;
#endif

__int32 errnum = 0;
__int32 nMatchMyAirline = 0;
char chMyAirLine[5];
char chMyGates[32];
BOOL fProcessTA = FALSE;
extern char *pszParkType[];
extern char *pszGateType[];
extern __int32 fDeletionsPass, nMinRunwayLen;
extern BOOL fIncludeWater, fMarkJetways;
void UpdateTransitionAlts(void);
char *pszSimName[9] = {
	"FS9", "FSX", "FSX-SE", "Prepar3D", "Prepar3D v2",
	"Prepar3D v3", "Prepar3D v4", "Prepar3D v5", "MSFS" };
char *pPathName = 0;
char *pSceneryName = 0;
HINSTANCE hInstance;

/******************************************************************************
         Data
******************************************************************************/

char szParkNames[12][5] =
{	"", "Park", "PkN", "PkNE", "PkE", "PkSE",
	"PkS", "PkSW", "PkW", "PkNW", "", "Dock"
};

char *szFacilities[8] =
{	"Region", "Country", "State",
	"City", "Airport",
	"Unknown(2)", "VOR/DME/TACAN",
	"NDB"
};

char *szComTypes[14] =
{	"Unknown(0)", "ATIS", "Multicom", "Unicom",
	"CTAF", "Ground", "Tower", "CDEL", "Approach",
	"Departure", "Centre", "FSS", "AWOS", "?(>12)",
};

char *szNavs[9] =
{	"Unknown(0)", "VOR", "DME", "NDB", "Localizer",
	"Glideslope", "SDF", "Marker", "TACAN"
};

char *szNavClass[11] =
{	"(T)", "(L)", "(H)", "",
	"(MH)", "(H)", "(HH)", "(OM)", "(MM)", "(IM)", "(BC)"
};

char *szRwySurf[13] =
{	"Unknown", "Dirt", "Concrete", "Asphalt",
	"Grass", "Gravel", "Oil-treated", "Mats",
	"Snow", "Coral", "Water", "Brick", "Planks"
};

char chRwyT[6] = " LRCW";

char *chOddRwys[16] =
{	"N-S", "E-W", "NW-SE", "SW-NE",
	"S-N", "W-E", "SE-NW", "NE-SW",
	"N", "W", "NW", "SW",
	"S", "E", "SE", "NE"
};

char chLine[] = "=============================================================================\n";
char szMyPath[MAX_PATH + 32];
char szCurrentFilePath[MAX_PATH];
BGLHDR bglhdr;
NBGLHDR nbglhdr;

FILE *fpAFDS = NULL;
RWYLIST *pR = NULL, *pRlast = NULL;
ANGLE lat, lon;

HWND hWnd;
BOOL fUserAbort = 0, fWritingFiles = FALSE, fNewAirport = FALSE, fQuiet = FALSE;
BOOL fNoLoadLorby = FALSE;
__int32 fFS9 = 0;
DWORD ulTotalAPs = 0, ulTotalRwys = 0, ulTotalRwys2 = 0;
DWORD ulTotalBGLs = 0, ulTotalBytes = 0;

BYTE chASCII[257] =
"................................ !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~."\
"................................ !cP#Y|p.Ca<~.R-o+23'mJ..1o>qht?AAAAAAACEEEEIIIIDNOOOOOxOUUUUYDBaaaaaaaceeeeiiiidnooooo/ouuuuydy";

#ifdef _DEBUG
	BOOL fDebug = TRUE;
#else
	BOOL fDebug = FALSE;
#endif

/******************************************************************************
        Assorted conversions
******************************************************************************/

char *str2ascii(char *psz)
{	char *psz2 = psz;
	while (*psz2)
	{	*psz2 = chASCII[*psz2];
		psz2++;
	}

	return psz;
}

void DecodeRwy(unsigned long n, unsigned long c, char *pch, __int32 offs, __int32 size)
{	__int32 i;
	if ((n > 36) && (n < 45))
		i = sprintf(pch, "%s", chOddRwys[n-37+offs]);
	else i = sprintf(pch, "%d", n);

	sprintf(&pch[i], "%c", c > 4 ? '?' : chRwyT[c]);
}

unsigned long lmultdiv(unsigned long m1, unsigned long m2, unsigned long d, __int32 fRound)
{	double m1d = m1;
	double m2d = m2;
	double dd = d;
	double dr = fRound ? dd/2 : 0;
	double res = ((m1d * m2d) + dr) / dd;
	double resi;

	modf(res, &resi);

	return (unsigned long) resi;
}

void ToAngle(ANGLE *p, long i, unsigned long f, short type)
{	static char orients[6] = "NSEW";
	BOOL fNeg = i < 0;
	double v = labs(i), vf = f;
	if (fNeg) vf = - vf;
	v += vf / (65536.0 * 65536.0);

	p->orient = orients[type + fNeg];

	// convert to seconds
	if (type == 0) v *= (90.0 * 60.0 * 60.0) / 10001750.0; // Lat
	else if (type == 2) v *= (360.0 * 60.0 * 60.0) / (65536.0 * 65536.0); // Lon
	i = (long) v;

	p->fangle = (float) (v / 3600.0);
	if (fNeg) p->fangle = -p->fangle;
	
	p->degrees = (short) (i/3600L);
	p->minutes = (short) ((i%3600L)/60);
	p->seconds = (short) ((i%3600L)%60);
	v -= i;
	p->tenthou = (short) (v * 10000.0);
	p->fracmins = (short) (((long) p->seconds * 10000L + p->tenthou) / 60);
}

float ToFeet(long alt)
{	float res = lmultdiv(labs(alt), 328084, 256000,1)/100.0f;
	return alt < 0 ? -res : res;
}

double Heading(unsigned long i, unsigned short f, __int32 fZeroOk)
{	double ri, r = (360.0 * i) / (65536.0 * 65536.0);
	if (f) r += (360.0 * f) / (65536.0 * 65536.0 * 65536.0);

	if (fZeroOk)
	{	if (r > 180.0) r -= 360.0;
	}

	else
	{	modf(r, &ri);
		if (ri < 1.0) r += 360.0;
	}

	return r;
}

void RemoveComma(char *psz)
{	while (1)
	{	psz = strchr(psz,',');
		if (!psz) break;
		*psz = '-';
	}
}

void WritePosition(LOCATION *ploc, BOOL fWithAlt)
{	char chAlt[64];
	if (fWithAlt) sprintf(chAlt, "  %.2fft", ToFeet(ploc->elev/256));
	else chAlt[0] = 0;
	ToAngle(&lat, ploc->lat.i, ploc->lat.f, 0);
	ToAngle(&lon, ploc->lon.i, ploc->lon.f, 2);					
	fprintf(fpAFDS,"%c%02hu:%02hu:%02hu.%04hu  %c%03hu:%02hu:%02hu.%04hu%s",
		lat.orient, lat.degrees, lat.minutes, lat.seconds, lat.tenthou,
		lon.orient, lon.degrees, lon.minutes, lon.seconds, lon.tenthou,
		chAlt);
}

/******************************************************************************
         FacilityDetails
******************************************************************************/

void FacilityDetails(__int32 type, unsigned long offs, FILE *fpIn)
{	OBJECT o;
	__int32 fRunwaySeen = 0;
	double fHeading, fMagvar;
	static __int32 numrwys = 0;
	static unsigned char icao[8];
	static RWY runways[64];
	static RWYLIST ap;
	static __int32 fdoingrwy = 0;
	unsigned __int32 ulSeekLocSet = 0;

	ap.fAirport = 0; // No airport yet

	if (fseek(fpIn, offs, SEEK_SET)) return;
	if (fread(&o, 1, sizeof(OBJECT), fpIn) != sizeof(OBJECT)) return;
	if ((o.type == 0) || (o.len == 0)) return;
	
	if (o.type == 1)
	{	while (1)
		{	// Read stuff in OBJECT_CONTAINERs
			unsigned long hdr[2];
			unsigned __int32 offsthen = ftell(fpIn);
				
			__int32 fOk = 0;
			
			#ifdef HEX
				unsigned char ch[16];
			#endif

			if (fUserAbort) return;
		
			if ((fread(hdr, 1, 8, fpIn) != 8) || (hdr[0] == 0) || (hdr[1] == 0))
			{	if (!ulSeekLocSet) break;
				fseek(fpIn, ulSeekLocSet, SEEK_SET);
				if (fread(hdr, 1, 8, fpIn) != 8) break;
				if ((hdr[0] == 0) || (hdr[1] == 0)) break;
				ulSeekLocSet = 0;
			}

			if (hdr[1] <= sizeof(LOCATION))
			{	LOCATION loc;
				char chWork[48], chWork2[48];
				__int32 i, k = 0;
				unsigned __int32 offsnow = ftell(fpIn);
				ANGLE Rlat, Rlong;
				
				if (fread(&loc, 1, hdr[1], fpIn) != hdr[1]) break;
				fOk = 1;

				#ifdef HEX
					fprintf(fpAFDS,"   :type %X, length %d\n",
							hdr[0], hdr[1]);

					while ((unsigned int) k < hdr[1])
					{	__int32 j;
					
						i = hdr[1] > 16 ? 16 : hdr[1];
						fprintf(fpAFDS, "               : ");
						for (j = 0; j < i; j++)	fprintf(fpAFDS, "%02X ", ((unsigned char *) &loc)[j+k]);
						while (j < 17)
						{	fprintf(fpAFDS, "   ");
							j++;
						}
						for (j = 0; j < i; j++) fprintf(fpAFDS, "%c",
							isprint(((unsigned char *) &loc)[j+k]) ? ((unsigned char *) &loc)[j+k] : '.');
						fprintf(fpAFDS, "\n");
						k += 16;
					}

					fprintf(fpAFDS, "          ");
				#endif
	
				switch (hdr[0])
				{	case LOC_AP:
						ulSeekLocSet = 0;
						fprintf(fpAFDS, "          ");
						fRunwaySeen = fdoingrwy = 0;
						WritePosition(&loc, 1);
						ulTotalAPs++;
						fNewAirport = TRUE;
						if (fDebug)	fprintf(fpAFDS,"\n<<<<(a) A/pCtr+1 = %d >>>>\n", ulTotalAPs);
						fprintf(fpAFDS, "\n          in file: %s\n",szCurrentFilePath);
						break;

					case LOC_RWY:
						fprintf(fpAFDS, "          ");
						fRunwaySeen = 1;
						DecodeRwy(loc.rwyn, loc.rwyt, chWork2, 0, sizeof(chWork2));
						
						fprintf(fpAFDS, "Runway %s ", chWork2);
						WritePosition(&loc, 1);

						ToAngle(&Rlat, loc.lat.i, loc.lat.f, 0);
						ToAngle(&Rlong, loc.lon.i, loc.lon.f, 2);					
						runways[numrwys].fLat = runways[numrwys+1].fLat = Rlat.fangle;
						runways[numrwys].fLong = runways[numrwys+1].fLong = Rlong.fangle;

						loc.rwysurf &= 0xff; // Some sceneries have rogue values like 128!? ### 230719
						sprintf(chWork, "Unknown(%d)", loc.rwysurf);
						fHeading = Heading(loc.hdg.i, loc.hdg.f, 0);
						fMagvar = Heading(loc.magvar, 0, 1);
						fprintf(fpAFDS, "\n              Hdg: %.3f true (MagVar %.1f), %s, %d x %d ft",
							fHeading, fMagvar,
							loc.rwysurf > 10 ? chWork : szRwySurf[loc.rwysurf],
							loc.rwylen, loc.rwywid);					
						
						if (fdoingrwy && (numrwys < 32) && (loc.rwyt < 4) && loc.rwyn && (loc.rwyn <= 36))
						{	runways[numrwys].hdg = (unsigned long) (fHeading - fMagvar + 0.5);
							if (runways[numrwys].hdg > 360) runways[numrwys].hdg -= 360;
							runways[numrwys].fHdg = (float) (fHeading - fMagvar);
							if (runways[numrwys].fHdg > 360.0F) runways[numrwys].fHdg -= 360.0F;
							runways[numrwys].rwylen = (unsigned short) loc.rwylen;
							runways[numrwys].rwywid = (unsigned short) loc.rwywid;
							runways[numrwys].rwysurf = (char) loc.rwysurf;
							runways[numrwys].rwysurfNew = 24;
							runways[numrwys].rwyn = loc.rwyn;
							runways[numrwys].rwyt = loc.rwyt;
							runways[numrwys].fMagvar = (float) fMagvar;
							runways[numrwys].nOffThresh = 0;
							numrwys++;

							runways[numrwys].hdg = runways[numrwys-1].hdg + 180;
							if (runways[numrwys].hdg > 360) runways[numrwys].hdg -= 360;
							runways[numrwys].fHdg = runways[numrwys-1].fHdg + 180.0F;
							if (runways[numrwys].fHdg > 360.0F) runways[numrwys].fHdg -= 360.0F;
							runways[numrwys].rwylen = (unsigned short) loc.rwylen;
							runways[numrwys].rwywid = (unsigned short) loc.rwywid;
							runways[numrwys].rwysurf = (char) loc.rwysurf;
							runways[numrwys].rwysurfNew = 24;
							runways[numrwys].rwyn = 18 + loc.rwyn;
							if (runways[numrwys].rwyn > 36) runways[numrwys].rwyn -= 36;
							runways[numrwys].rwyt = loc.rwyt;
							if (runways[numrwys].rwyt && (runways[numrwys].rwyt < 3))
								runways[numrwys].rwyt ^= 3;
							runways[numrwys].fMagvar = (float) fMagvar;
							runways[numrwys].nOffThresh;
							numrwys++;
						}

						break;
	
					case LOC_SET:
						if (!fRunwaySeen)
						{	if (!ulSeekLocSet)
								ulSeekLocSet = offsthen;
							fOk = -1;
							break;
						}

						fprintf(fpAFDS, "          ");
						DecodeRwy(loc.rwysn, loc.setst, chWork2, 8, sizeof(chWork2));
						if (loc.sett != 1) sprintf(chWork, "(Unknown type=%d)", loc.sett);
						fprintf(fpAFDS, "Setdown %s %s ",
							loc.sett == 1 ? "Rwy" : chWork, chWork2);

						WritePosition(&loc, 1);
						
						if ((loc.sett == 1) && fdoingrwy && numrwys)
						{	// Add new chain entry if this is a runway pos
							for (i = 0; i < numrwys; i++)
							{	if ((runways[i].rwyn == loc.rwysn) && (runways[i].rwyt == loc.setst))
								{	RWYLIST *pL = (RWYLIST *) malloc(sizeof(RWYLIST));
									if (!pL) break;
									memset(pL, 0, sizeof(RWYLIST));

									memcpy(pL->r.chICAO, icao, 4);
									if (icao[3] == 0) pL->r.chICAO[3] = ' ';
									pL->r.chRwy[0] = '0' + (char) (runways[i].rwyn / 100);
									pL->r.chRwy[1] = '0' + (char) ((runways[i].rwyn % 100) / 10);
									pL->r.chRwy[2] = '0' + (char) (runways[i].rwyn % 10);
									pL->r.chRwy[3] = '0' + (char) runways[i].rwyt;

									pL->r.fAlt = ToFeet(loc.elev/256);
									pL->r.fLat = lat.fangle;						
									pL->r.fLong = lon.fangle;
									pL->r.uLen = runways[i].rwylen;
									pL->r.uWid = (WORD) runways[i].rwywid;
									pL->r.chSurf = runways[i].rwysurf & 0xff;
									pL->r.chSurfNew = runways[i].rwysurfNew & 0xff;
									pL->r.uHdg = (unsigned short) runways[i].hdg;
									pL->fHdg = runways[i].fHdg;
									pL->fMagvar = runways[i].fMagvar;
									pL->nOffThresh = runways[i].nOffThresh;
									pL->fLat = runways[i].fLat;
									pL->fLong = runways[i].fLong;
									memcpy(pL->r.chILS, runways[i].chfreq, 6);
									memcpy(pL->r.chILSHdg, runways[i].chILSHdg, 6);
									
									ProcessRunwayList(pL, TRUE, 0);
									fFS9 = -1; // Stay in <FS9 mode
									break;
								}
							}
						}

						break;

					case LOC_ICAO:
						fprintf(fpAFDS, "          ");
						fRunwaySeen = numrwys = 0;
						memset(runways, 0, sizeof(runways));
						memcpy(icao, loc.icao, sizeof(icao));
						fdoingrwy = 1;
						fprintf(fpAFDS, "ICAO code: %s", loc.icao);

						memset(&ap, 0, sizeof(RWYLIST));
						memcpy(ap.r.chICAO, loc.icao, 4);
						ap.r.fAlt = ToFeet(loc.elev/256);
						ap.r.fLat = lat.fangle;						
						ap.r.fLong = lon.fangle;
						ap.fAirport = 1;
						break;

					case LOC_NAV:
						fprintf(fpAFDS, "          ");
						if (loc.ntype > 12) fOk = 0;
						else
						{	if (loc.nclass > 10)
								sprintf(chWork2, "(class=%d)", loc.nclass);

							if (loc.ntype == 8)
							{	// TACANs have Channel numbers
								sprintf(chWork, "Ch.%X", loc.nfreq);
							}

							else if ((loc.ntype == 3) || (loc.ntype == 7))
							{	// NDB freq format
								if (loc.nfreq)
									sprintf(chWork, "%X.%01X", loc.nfreq >> 4, loc.nfreq & 0x0f);
								else chWork[0] = 0;
							}

							else // Rest are xxx.xx with lowest byte unused
							{	i = sprintf(chWork, "%03X.%02X",loc.nfreq >> 16, (loc.nfreq >> 8) & 0xff);

								if ((loc.ntype == 4) || (loc.ntype == 5))
								{	// Localizer or Glideslope
									sprintf(&chWork[i], " %s=%.1f%s",
										loc.ntype == 4 ? "Hdg" : "Slope",
										Heading(loc.hdg.i, loc.hdg.f, 0),
										loc.ntype == 4 ? " true" : "");
								}
							}

							if (fdoingrwy && numrwys && (loc.ntype == 4))
							{	__int32 hdg = (int) ((Heading(loc.hdg.i, loc.hdg.f, 0) + 5.0) / 10.0);
								__int32 diff1 = labs(runways[numrwys-1].rwyn - hdg);
								__int32 diff2 = labs(runways[numrwys-2].rwyn - hdg);
								if (diff1 > 18) diff1 = 36-18;
								if (diff2 > 18) diff2 = 36-18;
								memcpy(runways[numrwys-1-(diff1>diff2)].chfreq, chWork, 6);
							}
						
							fprintf(fpAFDS, "%s%s %s%s %s ",
									fRunwaySeen ? "  " : "",
									loc.id, szNavs[loc.ntype],
									loc.nclass > 10 ? chWork2 : szNavClass[loc.nclass], chWork);

							if (loc.nflags)
							{	if (loc.nflags & 1L)
									fprintf(fpAFDS, " voice announce ");
								if (loc.nflags & ~1L)
									fprintf(fpAFDS, "(Flags=%X) ", loc.nflags);
							}

							WritePosition(&loc, 1);
						}
						break;

					case LOC_FREQ:
						fprintf(fpAFDS, "          ");
						fprintf(fpAFDS, "%s=%X.%02X",
							szComTypes[loc.comt > 12 ? 13 : loc.comt],
								loc.freq >> 16, (loc.freq >> 8) & 0xff);

						if (ap.fAirport && ((loc.comt == 1) || ((loc.comt == 12) && (ap.Atis < 0x1800))))
						{	// Add ATIS/AWOS for FSMeteo
							ap.Atis = (unsigned short) (loc.freq >> 8);
								
							fprintf(fpAFDS, "\n          FSM ATIS/AWOS for %4s=%04X",
								ap.r.chICAO, ap.Atis);
						}
						
						break;

					default:
						fseek(fpIn, offsnow, SEEK_SET);
						fOk = 0;
						break;		
				}
			}

			if (fOk > 0) fprintf(fpAFDS,"\n");

			else if (fOk == 0)
			{	unsigned __int32 uL = hdr[1];

				#ifdef HEX
					fprintf(fpAFDS,"-- :type %X, length %d\n",
						hdr[0], uL);
					if (uL > 128)
					{	fprintf(fpAFDS,"    Long entry: only first 128 bytes shown!\n");
						hdr[1] = 128;
					}
					
					while (hdr[1])
					{	__int32 j, i = hdr[1] > 16 ? 16 : hdr[1];
						hdr[1] -= i;
						uL -= i;
						if (fread(ch, 1, i, fpIn) != (unsigned int) i) break;
						fprintf(fpAFDS, "            -- : ");
						for (j = 0; j < i; j++)	fprintf(fpAFDS, "%02X ", ch[j]);
						while (j < 17)
						{	fprintf(fpAFDS, "   ");
							j++;
						}
						for (j = 0; j < i; j++) fprintf(fpAFDS, "%c",
							isprint(ch[j]) ? ch[j] : '.');
						fprintf(fpAFDS, "\n");
					}
				#endif

				if (uL) fseek(fpIn, uL, SEEK_CUR);
			}
		}
	}

	else
	{	fprintf(fpAFDS,"            Object type %X, length %d, t=%X\n",
		o.type, o.len, o.t);
	}

	if (ap.fAirport)
	{	RWYLIST *pL = (RWYLIST *) malloc(sizeof(RWYLIST));
		if (pL)
		{	memcpy(pL, &ap, sizeof(RWYLIST));

			fprintf(fpAFDS, "          FSM A/P %4s, lat=%.6f, long=%.6f, alt=%.2f\n",
				ap.r.chICAO, (double) ap.r.fLat, (double) ap.r.fLong, ap.r.fAlt);

			ProcessRunwayList(pL, TRUE, 0);
		}		
	}
}

/******************************************************************************
         CheckBGL
******************************************************************************/

void CheckBGL(FILE *fpIn)
{	// Analyse NAME_LIST
	NAMELIST nl;
	__int32 ftm = 1, i, j = 0;
	unsigned long offs = ftell(fpIn);
	char chwork[64];
		
	fprintf(fpAFDS, chLine);
	chwork[0] = 0;

	while (1)
	{	if (fUserAbort) return;
		
		if (fread(&nl, 1, sizeof(NAMELIST), fpIn) < 8) break;
		if (!nl.type | !nl.len) break;
		if (nl.type != NAME_LIST)
		{	offs += nl.len + 8;
		}
		else
		{	// Read whole name list and process in memory:
			__int32 len = nl.len - sizeof(NAMELIST) + 8;
			NAMEENTRY *pnlist = (NAMEENTRY *) malloc(len);
			NAMEENTRY *pn = pnlist;
			if (!pnlist) break;
			if (fread(pnlist, 1, len, fpIn) != (unsigned int) len)
			{	free(pnlist);
				break;
			}
			
			offs = ftell(fpIn);
		
			while ((len >= 8) && pn->type && pn->len)
			{	if (fUserAbort) return;
		
				if (pn->type == NAME_ENTRY)
				{	char *pFac = NULL;
					__int32 fOk = 1;
					unsigned __int32 type = pn->t;

					if ((type >= FACILITY_REGION) && (type < FACILITY_AIRPORT))
						pFac = szFacilities[type - FACILITY_REGION];
					else if ((type == FACILITY_AIRPORT) &&
							(pn->st >= ST_AIRPORT) && (pn->st <= ST_VORNDB))
					{	type += pn->st - 1;
						pFac = szFacilities[type - FACILITY_REGION];
					}

					if (!pFac)
					{	pFac = chwork;
						fOk = 0;
					}

					for (i = 0; (i < (int)(type - FACILITY_REGION)) && (i < 4); i++)
							fprintf(fpAFDS,"  ");			
					fprintf(fpAFDS, "%s (%s)\n", pn->descr, pFac);

					if (fOk && pn->offs)
						FacilityDetails(type, pn->offs, fpIn);
				}

				len -= pn->len + 8;
				pn = (NAMEENTRY *) ((char *) pn + pn->len + 8);
			}

			free(pnlist);
		}

		fseek(fpIn, offs, SEEK_SET);
	}
}

/******************************************************************************
         SetSceneryCfgPath
******************************************************************************/

// nVers = 0 FSX, 1 ESP, 2 Prepar3D, 3 Prepar3D v2, 4 Prepar3D v3, 5 Prepar3D v4, 6 Prepar3D v5
__int32 SetSceneryCfgPath(char *psz, __int32 nVers)
{	static char *pszCfgPaths[7] = {
			"\\Microsoft\\FSX\\SCENERY.CFG",
			"\\Microsoft\\FSX-SE\\SCENERY.CFG",
			"\\Lockheed Martin\\Prepar3D\\SCENERY.CFG",
			"\\Lockheed Martin\\Prepar3D v2\\SCENERY.CFG",
			"\\Lockheed Martin\\Prepar3D v3\\SCENERY.CFG",
			"\\Lockheed Martin\\Prepar3D v4\\SCENERY.CFG",
			"\\Lockheed Martin\\Prepar3D v5\\SCENERY.CFG",
	};

	char szPathName[MAX_PATH], szPathAppend[MAX_PATH];
	char *pchUser, *pchUser2;
	HANDLE hDir;
	HANDLE hFile;

	WIN32_FIND_DATA fd;
	BOOL fSearching = FALSE, ftm = TRUE, fOkay = FALSE;
	__int32 len2;

	nVersion = nVers;
	
	if (nVers >= 4)
	{	// For Prepar3D v3 - v5, see if AddonOrganizer is available
		char szAddonsPath[(MAX_PATH * 2) + 16], szMyPath2[MAX_PATH], *pch;
		__int32 len, len2 = MAX_PATH + 64;

		szAddonsPath[0] = 0;
		GetModuleFileName(hInstance, szMyPath2, MAX_PATH);
		pch = strrchr(szMyPath2, '\\');
		if (pch) *pch = 0;
		else szMyPath2[0] = 0;
		len = strlen(szMyPath2);
		szMyPath2[len] = 0;

		if (GetFileAttributes("MakeRwys_Scenery.cfg") != INVALID_FILE_ATTRIBUTES)
		{	if (fNoLoadLorby)
			{	pch = strrchr(psz,'\\');
				if (pch) *pch = 0;
				strcat(psz, "\\MakeRwys_Scenery.cfg");
		
				return nVers;
			}

			else DeleteFile("MakeRwys_Scenery.cfg"); // Ensure don't use old one
		}

		if (GetFileAttributes("LorbySceneryExport.exe") != INVALID_FILE_ATTRIBUTES)
		{	strcpy(szAddonsPath, "LorbySceneryExport.exe");
			strcat(szAddonsPath, " MakeRwys_Scenery.cfg");
			fprintf(fpAFDS, "Using \x22%s\x22\n", szAddonsPath);
		}
		
		if (szAddonsPath[0])
		{	static STARTUPINFO sui = {
			sizeof(STARTUPINFO),
				NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0,
				0, SW_HIDE, 0, NULL, 0, 0, 0 };
			PROCESS_INFORMATION pi;
			
			if (CreateProcess(0,	// pointer to name of executable module 
						szAddonsPath, // pointer to command line string
						NULL,	// pointer to process security attributes 
						NULL,	// pointer to thread security attributes 
						FALSE,	// handle inheritance flag 
						CREATE_DEFAULT_ERROR_MODE, // creation flags 
						NULL,	// pointer to new environment block 
						szMyPath2,	// pointer to current directory name 
						&sui,	// pointer to STARTUPINFO 
						&pi)) 	// pointer to PROCESS_INFORMATION  
				
			{	fprintf(fpAFDS, "LorbySceneryExport executed: ");
				if (WaitForSingleObject(pi.hProcess, 30000) != WAIT_TIMEOUT)
				{	fprintf(fpAFDS, "Checking for \x22MakeRwys_Scenery.cfg\x22\n");
					if (GetFileAttributes("MakeRwys_Scenery.cfg") != INVALID_FILE_ATTRIBUTES)
					{	pch = strrchr(psz,'\\');
						if (pch) *pch = 0;
						strcat(psz, "\\MakeRwys_Scenery.cfg");
						return nVers;
					}
					fprintf(fpAFDS, "... ERROR: file not created!\n");					
				}
			}

			else
				fprintf(fpAFDS, "... ERROR: failed to run Lorby program, error=%d!\n",
					GetLastError());					
		}
		else fprintf(fpAFDS, "Will be using normal scenery.cfg file ...\n");					
	}

	// Need user path
	if (SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPathName) != S_OK)
		return -1;

	len2 = strlen(szPathName);
	if (len2 && (szPathName[len2-1] == '\\'))
		len2--;

	strcpy(&szPathName[len2], pszCfgPaths[nVers]);
	
	hFile = CreateFile(szPathName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);		
	if (hFile != INVALID_HANDLE_VALUE)
	{	CloseHandle(hFile);
		strcpy(psz, szPathName);
		return nVers;
	}
	
	szPathName[len2] = 0;
    
	// Try to find \\Microsoft\\FSX\\SCENERY.CFG for other users
	pchUser = strrchr(szPathName, '\\');
	if (pchUser)
	{	strcpy(szPathAppend, pchUser);
		*pchUser = 0;
		pchUser2 = strrchr(szPathName, '\\');
		if (!pchUser2) *pchUser = '\\';
		else
		{	len2 = (int) (pchUser2 - &szPathName[0]) + 1;
			strcpy(&szPathName[len2], "*");
			hDir = FindFirstFile(szPathName, &fd);
			fSearching = hDir != INVALID_HANDLE_VALUE;
		}
	}

	while (ftm || fSearching)
	{	BOOL fSkip = FALSE;
		ftm = FALSE;

		if (fSearching)
		{	if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				fSkip = TRUE;
			
			else
				strcpy(&szPathName[len2], fd.cFileName);
		}

		if (!fSkip)
		{	__int32 len = strlen(szPathName);
			DWORD lenh = 0;
		
			if (fSearching)
			{	strcat(szPathName, szPathAppend);
				len = strlen(szPathName);
			}
			
			strcpy(&szPathName[len], pszCfgPaths[nVers]);

			hFile = CreateFile(szPathName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			
			if (hFile != INVALID_HANDLE_VALUE)
			{	CloseHandle(hFile);
				strcpy(psz, szPathName);
				return nVers;
			}
		}

		if (fSearching)
			fSearching = FindNextFile(hDir, &fd);
	}

	return -1;
}

/******************************************************************************
         StartXML
******************************************************************************/

void StartXML(FILE *pfi)
{	fprintf(pfi,"<?xml version=\"1.0\"?>\x0d\x0a");
	fprintf(pfi,"<data>\x0d\x0a");	
	fprintf(pfi, "<SimVersion>%s</SimVersion>\x0d\x0a", pszSimName[nVersion + 1]);
}

/******************************************************************************
         StringXML
******************************************************************************/

char *StringXML(char *pszTo, char *pszFrom)
{	char *pszNow = pszTo;
	while (*pszFrom)
	{	if (*pszFrom == '&')
		{	strcpy(pszNow, "&amp;");
			pszNow += 5;
		}
	
		else if (*pszFrom == 0x22)
		{	strcpy(pszNow, "&quot;");
			pszNow += 6;
		}

		else if (*pszFrom == 0x27)
		{	strcpy(pszNow, "&apos;");
			pszNow += 6;
		}
	
		else if (*pszFrom == '<')
		{	strcpy(pszNow, "&lt;");
			pszNow += 4;
		}
	
		else if (*pszFrom == '>')
		{	strcpy(pszNow, "&gt;");
			pszNow += 4;
		}

		else 
		{	*pszNow = *pszFrom;
			pszNow++;
		}

		pszFrom++;
	}

	*pszNow = 0;
	
	return pszTo;
}

/******************************************************************************
			CheckTables
******************************************************************************/

void CheckTables(char *pfname)
{	RWYLIST *p = pR;
	static DWORD ulPrevTotalRwys = 0, ulPrevTotalEntries = 0;
	DWORD ulCheckTotalRwys = 0, ulCheckDeletes = 0, ulCheckAirports = 0,
		ulCheckGates = 0, ulCheckTaxis = 0, ulCheckOthers = 0, ulTotalEntries = 0;
	
	while (p)
	{	RWYLIST *pLast = p;
						
		if (p->fDelete && !p->fAirport) 
			ulCheckDeletes++;

		else if (p->fAirport)
			ulCheckAirports++;

		else if (p->pGateList)
			ulCheckGates++;

		else if (p->pTaxiwayList)
			ulCheckTaxis++;

		else if (p->r.chRwy[3] && (p->r.chRwy[3] < '4'))
			ulCheckTotalRwys++;

		else ulCheckOthers++;

		ulTotalEntries++;
		p = p->pTo;
	}

	fprintf(fpAFDS,"\n**** File=\x22%s\x22\n**** %s check: Total=%d (Rwys=%d, Airports=%d, Deletes=%d, GateLists=%d, TaxiLists=%d, Others=%d)",
		pfname, fDeletionsPass ? "Deletions pass" : "Adding pass", 
		ulTotalEntries, ulCheckTotalRwys, ulCheckAirports, ulCheckDeletes, ulCheckGates, ulCheckTaxis, ulCheckOthers);
	if (ulTotalEntries < ulPrevTotalEntries)
		fprintf(fpAFDS,"\n**** WARNING: Total is less than it was on last check!");
	if (ulCheckTotalRwys < ulPrevTotalRwys)
		fprintf(fpAFDS,"\n**** %s: Total runways is reduced%s",
			(ulCheckDeletes && fDeletionsPass) ? "NOTE" : "TABLE ERROR",
			(ulCheckDeletes && fDeletionsPass) ? " probably because of deletions." : "!");
	fprintf(fpAFDS,"\n\n");

	ulPrevTotalEntries = ulTotalEntries;
	ulPrevTotalRwys = ulCheckTotalRwys;
}

/******************************************************************************
         ScanSceneryArea
******************************************************************************/

void ScanSceneryArea(char *pszPath)
{	char szParam[512];
	FILE *fpIn;
	MY_WIN32_FIND_DATA find;
	HANDLE hFind;
	__int32 fpos;
			
	strncpy(szParam, pszPath, MAX_PATH);		
	fpos = strlen(szParam);
	if (fDeletionsPass > 0) fprintf(fpAFDS, "Path(Local/Remote)=%s\n", szParam);

	if (!fMSFS)
	{	strcat(szParam, "\\scenery");
		if ((GetFileAttributes(szParam) != FILE_ATTRIBUTE_DIRECTORY) && (fpos > 8) &&
			(_strnicmp(&szParam[fpos - 8], "\\scenery", 8) == 0))
			// Scenery path is complete already!
			szParam[fpos] = 0;
		else
			fpos += 8;
	}

	strcat(szParam, "\\*.bgl");
	fpos++;
	hFind = FindFirstFile(szParam, (WIN32_FIND_DATA *) &find);
	while (hFind != INVALID_HANDLE_VALUE)
	{	char *pch = strrchr(find.cFileName, '.');
		char* psz = 0;
				
		if (fUserAbort) return;
				
		if (pch && (_strnicmp(pch, ".BGL", 5) == 0)) // Eliminate files like "bgl_passive"
		{	char szFile1[MAX_PATH], szFile2[MAX_PATH], szFile3[MAX_PATH];
			szFile2[0] = szFile3[0] = 0;
			strcpy(&szParam[fpos], find.cFileName);
			if (!fDeletionsPass)
				++ulTotalBGLs;

			strcpy(szFile1, szParam);
			if (fMSFS)
			{	psz = strstr(szFile1, "Community");
				if (!psz) psz = strstr(szFile1, "Official");
				if (psz)
				{	strcpy(szFile2, psz);
					*psz = 0;
					psz = strrchr(szFile2, '\\');
				}
			}
			else
				psz = strrchr(szFile1, '\\');
			if (psz)
			{	psz++;
				strcpy(szFile3, psz);
				*psz = 0;
			}

			SetWindowText(GetDlgItem(hWnd, IDC_FILE1), szFile1);
			SetWindowText(GetDlgItem(hWnd, IDC_FILE2), szFile2);
			SetWindowText(GetDlgItem(hWnd, IDC_FILE3), szFile3);
			PostMessage(hWnd, WM_USER, 1, 0);
								
			fpIn = fopen(szParam, "rb");

			strcpy(pNextPathName, szParam);
			pPathName = pNextPathName;
			pNextPathName += strlen(pNextPathName) + 1;

			if (fpIn)
			{	BOOL fDone = FALSE;

				// See if file contains AFDs:
				if ((fFS9 >= 0) && (fread(&nbglhdr, 1, sizeof(NBGLHDR), fpIn) >= 
							(sizeof(NBGLHDR) - ((NSECTS_PER_FILE - 1) * sizeof(NSECTS)))))
				{	if (!fDeletionsPass)
						ulTotalBytes += sizeof(NBGLHDR);
					if (nbglhdr.wStamp == 0x0201)
					{	// New BGL format for FS2004?
						strcpy(szCurrentFilePath, szParam);
						CheckNewBGL(fpIn, &nbglhdr, find.nFileSizeLow);
						fDone = TRUE;
						if (fDebug) CheckTables(szParam);
					}
				}
						
				fseek(fpIn, 0, SEEK_SET);
				if (!fDone && (fFS9 <= 0) && (fread(&bglhdr, 1, sizeof(BGLHDR), fpIn) == sizeof(BGLHDR)))
				{	if (fDeletionsPass && (bglhdr.spare1 == 0x87654321) &&
							(bglhdr.dataptrsctr >= 1) &&
							bglhdr.namoffset &&
							!fseek(fpIn,bglhdr.namoffset, SEEK_SET))
					{	fprintf(fpAFDS, chLine);
						fprintf(fpAFDS, szParam);
						fprintf(fpAFDS, "\n");
						strcpy(szCurrentFilePath, szParam);
						CheckBGL(fpIn);
						if (fDebug) CheckTables(szParam);
					}
				}
				
				fclose(fpIn);
			}
		}
	
		if (!FindNextFile(hFind, (WIN32_FIND_DATA *) &find))
		{	FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}
}

/******************************************************************************
		 Data for scenery layer list
******************************************************************************/

__int32 nAreas[10000]; // Priorities 1-9999
char szTitles[10000][64];
char szPaths[10000][MAX_PATH];
BYTE bActive[10000];
__int32 nArea = 0;

char szAsoboPaths[1000][MAX_PATH];
__int32 nAsobo = 0;

/******************************************************************************
		 ProcessMSFSCommunity
******************************************************************************/

ProcessMSFSCommunity(char* pPath)
{	char szPath[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	strcpy(szPath, pPath);
	strcat(szPath, "*.BGL");

	hFind = FindFirstFile(szPath, (WIN32_FIND_DATA*)&fd);
	if ((hFind != INVALID_HANDLE_VALUE) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{	// found a scenery layer - add it to the list ...
		strcpy(szPaths[nArea++], pPath);
	}

	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;

	strcpy(szPath, pPath);
	strcat(szPath, "*.*");

	hFind = FindFirstFile(szPath, (WIN32_FIND_DATA*)&fd);
	while (hFind != INVALID_HANDLE_VALUE)
	{	// Check for directory
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
		{	strcpy(szPath, pPath);
			strcat(szPath, fd.cFileName);
			strcat(szPath, "\\");
			ProcessMSFSCommunity(szPath);
		}

		if (!FindNextFile(hFind, (WIN32_FIND_DATA*)&fd))
		{	FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}
}

/******************************************************************************
		 ProcessMSFSOfficial
******************************************************************************/

ProcessMSFSOfficial(char* pPath, BOOL fAsobo)
{	char szPath[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	
	strcpy(szPath, pPath);
	strcat(szPath, "*.BGL");

	hFind = FindFirstFile(szPath, (WIN32_FIND_DATA*) &fd);
	if ((hFind != INVALID_HANDLE_VALUE) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{	// found a scenery layer - add it to the list ...
		if (fAsobo)
			strcpy(szAsoboPaths[nAsobo++], pPath);
		else 
			strcpy(szPaths[nArea++], pPath);
	}

	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
	
	strcpy(szPath, pPath);
	strcat(szPath, "*.*");

	hFind = FindFirstFile(szPath, (WIN32_FIND_DATA*) &fd);
	while (hFind != INVALID_HANDLE_VALUE)
	{	// Check for directory
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") &&
				(_strnicmp(fd.cFileName, "asobo", 5) || (_strnicmp(fd.cFileName, "asobo-airport", 13) == 0)))
		{	strcpy(szPath, pPath);
			strcat(szPath, fd.cFileName);
			strcat(szPath, "\\");
			ProcessMSFSOfficial(szPath, strstr(szPath, "asobo-airport") != 0);
		}
	
		if (!FindNextFile(hFind, (WIN32_FIND_DATA*) &fd))
		{	FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
		}
	}
}

/******************************************************************************
		 CompleteTables (MSFS)
******************************************************************************/

void CompleteTables(void)
{
	char* psz;
	__int32 i = 0;
	while (i < nArea)
	{
		nAreas[i] = i + 1;
		bActive[i] = 0xff;
		__int32 j = strlen(szPaths[i]);
		BOOL fCommunity = FALSE;
		szPaths[i][--j] = 0; // Dispense with last backslash
		psz = strstr(szPaths[i], "OneStore");
		if (psz) psz += 9;
		else
		{
			psz = strstr(szPaths[i], "Steam");
			if (psz) psz += 6;
			else
			{
				psz = strstr(szPaths[i], "Community");
				if (psz)
				{
					psz += 10;
					fCommunity = TRUE;
				}
			}
		}

		if (psz)
		{
			strcpy(szTitles[i], psz);
			psz = (strchr(szTitles[i], '\\'));
			if ((_strnicmp(szTitles[i], "Asobo", 5) == 0) || fCommunity)
				*psz = 0;
			else
			{
				if (psz && (_strnicmp(++psz, "Scenery", 7) == 0))
					psz += 7;
				if (psz && !isdigit(psz[1]) && _strnicmp(&psz[1], "Base", 4) &&
					_strnicmp(&psz[1], "World", 5))
					*psz = 0;
			}
			while (psz = strchr(szTitles[i], '\\'))
				*psz = ' ';
		}

		i++;
	}

	// Now loop through all entries in pContent, looking for 'active="false"'.
	psz = pContent;
	while (psz)
	{	char* psz3, *psz2 = strstr(psz, "<Package");
		BOOL fDisabled = FALSE;
		
		if (!psz2)
			break;
		
		psz3 = strstr(psz2, "/>");
		if (psz3)
		{	*psz3 = 0;
			fDisabled = (strstr(psz2, "\"false\"") != NULL);
			*psz3 = '/';
		}
		
		if (fDisabled)
		{	// For each found search out table for match, and set active false.
			psz2 = strchr(psz2, '\x22');
			if (psz2)
			{	psz3 = strchr(++psz2, '\x22');
				if (psz3)
				{	i = 0;
					*psz3 = 0;
					
					while (i < nArea)
					{	if (strstr(szTitles[i], psz2))
						{	bActive[i] = 0;
							if (strcmp(psz2, "fs-base"))
								break;
						}
					
						i++;
					}
				}
			}
		}

		psz += 6;
	}

	if (pContent)
	{
		free(pContent);
		pContent = 0;
	}

	FILE *fpList = fopen("SceneryList.txt", "w");
	if (fpList)
	{	int i = 0;
		while (i < nArea)
		{	fprintf(fpList, "%03d\t%s\t%s\n\t%s\n",
				i + 1, bActive[i] ? "Active" : "Disabled",
				szTitles[i], szPaths[i]);
			i++;
		}
		fclose(fpList);
	}
}

/******************************************************************************
         MainRoutine
******************************************************************************/

char chAirportNames[60000000];
char chCityNames[60000000];
char chStateNames[60000000];
char chCountryNames[60000000];

char *pNextAirportName = chAirportNames;
char *pNextCityName = chCityNames;
char *pNextStateName = chStateNames;
char *pNextCountryName = chCountryNames;

char chPathNames[60000000], chWk[1024];
char *pNextPathName = chPathNames;

DWORD WINAPI MainRoutine (PVOID pvoid)
{	char szArea[512], szParam[64];
	char szCfgPath[MAX_PATH + 128];
	BOOL fOk = 0, fFSX = -1;
	
	memset(&nAreas[0], 0xff, sizeof(nAreas));
	memset(&bActive[0], 0, sizeof(bActive));

	ulTotalAPs = ulTotalRwys = 0;
	fpAFDS = fopen("Runways.txt","w");
	if (!fpAFDS)
	{	if (!fQuiet) MessageBox(NULL, "Error: cannot write Runways.txt file!", "MakeRunways Error", MB_ICONSTOP);
		return 0;
	}

	fprintf(fpAFDS, "Make Runways File: Version 5.11 by Pete Dowson\n");	
	
	// Need to locate current SCENERY.CFG elsewhere if this is FSX ...
	strcpy(szCfgPath, szMyPath);

	if (GetFileAttributes("Prepar3D.EXE") != INVALID_FILE_ATTRIBUTES)
	{	__int32 nVersIndex = 2;
		VS_FIXEDFILEINFO *pvsf = 0;
		char chBlock[16384];
		UINT cb;

		if (GetFileVersionInfo("Prepar3D.EXE", 0, 2048, chBlock) &&
				VerQueryValue(chBlock, TEXT("\\"), (LPVOID*)&pvsf, &cb) &&
				((pvsf->dwProductVersionMS >> 16) >= 2))
		{	nVersIndex = 3;
			if ((pvsf->dwProductVersionMS >> 16) >= 5)
				nVersIndex = 6;
			else if ((pvsf->dwProductVersionMS >> 16) >= 4)
				nVersIndex = 5;
			else if ((pvsf->dwProductVersionMS >> 16) >= 3)
				nVersIndex = 4;
		}

		fFSX = SetSceneryCfgPath(&szCfgPath[0], nVersIndex);
	}
	
	else if (GetFileAttributes("FSX.EXE") != INVALID_FILE_ATTRIBUTES)
	{	// Need to see if it's Steam Edition!
		char chBlock[2048];
		VS_FIXEDFILEINFO *pvsf = 0;
		UINT cb;

		if (GetFileVersionInfo("FSX.EXE", 0, 2048, chBlock) &&
			VerQueryValue(chBlock, TEXT("\\"), (LPVOID*)&pvsf, &cb))
		{	WORD wvers = (WORD) (pvsf->dwProductVersionLS >> 16);
			if (wvers >= 62607)
			{	fFSX = SetSceneryCfgPath(&szCfgPath[0], 1);
				if (GetFileAttributes(szCfgPath) == INVALID_FILE_ATTRIBUTES)
					fFSX = -1;
			}
		}
		
		if (fFSX < 0)
			fFSX = SetSceneryCfgPath(&szCfgPath[0], 0);
	}

	// fprintf(fpAFDS, "\nfFSX = %d, fDoMSFS = %d\n", fFSX, fDoMSFS);
	
	if ((fFSX < 0) && fLocal)
	{	fMSFS = TRUE;
		nVersion = 7;
		GetModuleFileName(NULL, szCfgPath, MAX_PATH);
		char* psz = strrchr(szCfgPath, '\\');
		if (psz) psz[1] = 0;
		ProcessMSFSCommunity(szCfgPath);
		CompleteTables();
		goto MAINLOOPS;
	}

	else if ((fFSX < 0) && fDoMSFS)
	{	// See if it is MSFS, in MS Store location
		char szCopyPath[MAX_PATH];

		strcpy(szCfgPath, getenv("LOCALAPPDATA"));
		strcat(szCfgPath, "\\Packages\\Microsoft.FlightSimulator_8wekyb3d8bbwe\\LocalCache");
		__int32 n = strlen(szCfgPath);

		strcpy(&szCfgPath[n], "\\UserCfg.opt");
		
		if (GetFileAttributes(szCfgPath) == INVALID_FILE_ATTRIBUTES)
		{	// try Steam
			strcpy(szCfgPath, getenv("APPDATA"));
			strcat(szCfgPath, "\\Microsoft Flight Simulator");
			n = strlen(szCfgPath);
			strcpy(&szCfgPath[n], "\\UserCfg.opt");
		}
		
		// Get content list (for disable options)
		strcpy(szCopyPath, szCfgPath);
		strcpy(&szCopyPath[n], "\\content.xml");
		
		HANDLE h = CreateFile(szCopyPath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (h)
		{	DWORD lenHi, len = GetFileSize((HANDLE) h, &lenHi);
			__int32 l;
			pContent = malloc((int) len + 1);
			ReadFile((HANDLE) h, pContent, len, &l, NULL);
			pContent[len] = 0;
			CloseHandle((HANDLE) h);
		}
		
		h = CreateFile(szCfgPath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (h)
		{	fOk = FALSE;
			__int32 lenHi, len = GetFileSize(h, &lenHi);
			char* pCfg = malloc((int) len + 1);
			
			if (pCfg)
			{
				__int32 l;
				ReadFile(h, pCfg, len, &l, NULL);
				pCfg[len] = 0;

				char* psz = strstr(pCfg, "InstalledPackagesPath");
				if (psz)
				{
					psz = strchr(psz, '\x22');
					if (psz)
					{
						strcpy(szCfgPath, &psz[1]);
						psz = strchr(szCfgPath, '\x22');
						if (psz) *psz = 0;
						fOk = TRUE;
					}
				}
				
				free(pCfg);
			}

			CloseHandle(h);
		}
		
		if (!fOk)
		{	//Make assumption if "proper" method fails:
			strcpy(szCfgPath, getenv("LOCALAPPDATA"));
			strcat(szCfgPath, "\\Packages\\Microsoft.FlightSimulator_8wekyb3d8bbwe\\LocalCache\\Packages\\");
		}

		__int32 fsPathLen = strlen(szCfgPath);
		strcpy(&szCfgPath[fsPathLen], "\\Official\\OneStore\\");
		
		if (GetFileAttributes(szCfgPath) == INVALID_FILE_ATTRIBUTES)
		{	// Not MS Store, try Steam version:
			strcpy(&szCfgPath[fsPathLen], "\\Official\\Steam\\");
		}
		
		if (GetFileAttributes(szCfgPath) != INVALID_FILE_ATTRIBUTES)
		{	fprintf(fpAFDS, "Found MSFS official scenery in: \n  \x22%s\x22\n", szCfgPath);

			// Load the language file for airport name look-up
			char szLangPath[MAX_PATH];
			strcpy(szLangPath, szCfgPath);
			strcat(szLangPath, "fs-base\\en-US.locPak");
			if (GetFileAttributes(szLangPath) != INVALID_FILE_ATTRIBUTES)
			{	HANDLE h = CreateFile(szLangPath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				if (h)
				{	DWORD lenHi, len = GetFileSize(h, &lenHi);
					pLocPak = malloc((int) len + 1);
					if (pLocPak)
					{	__int32 l;
						ReadFile(h, pLocPak, len, &l, NULL);
						pLocPak[len] = 0;
					}
					else
					{	free(pLocPak);
						pLocPak = 0;
					}
					CloseHandle(h);
				}
			}

			// Load the materials file for runway surface look-up
			char szMatsPath[MAX_PATH];
			strcpy(szMatsPath, szCfgPath);
			strcat(szMatsPath, "fs-base-material-lib\\MaterialLibs\\Base_MaterialLib\\Library.xml");
			if (GetFileAttributes(szMatsPath) != INVALID_FILE_ATTRIBUTES)
			{
				HANDLE h = CreateFile(szMatsPath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				if (h)
				{
					DWORD lenHi, len = GetFileSize((HANDLE)h, &lenHi);
					__int32 l;
					pMaterials = malloc((int) len + 1);
					if (pMaterials)
					{
						ReadFile((HANDLE)h, pMaterials, len, &l, NULL);
						pMaterials[len] = 0;
					}
					else
					{
						free(pMaterials);
						pMaterials = 0;
					}
					
					CloseHandle((HANDLE)h);
				}
			}

			ProcessMSFSOfficial(szCfgPath, FALSE);
			// Add the asobo-airport entries to the end of the main list
			__int32 i = 0;
			while (i < nAsobo)
				strcpy(szPaths[nArea++], szAsoboPaths[i++]);
		}
		
		strcpy(&szCfgPath[fsPathLen], "\\Community\\");
		if (GetFileAttributes(szCfgPath) != INVALID_FILE_ATTRIBUTES)
		{	fprintf(fpAFDS, "Found MSFS community scenery in: \n  \x22%s\x22\n", szCfgPath);
			ProcessMSFSCommunity(szCfgPath);
		}

		// complete the tables (with fiction at present)
		CompleteTables();

		fMSFS = TRUE;
		nVersion = 7;
		fprintf(fpAFDS, "Reading MSFS scenery:\n");
		goto MAINLOOPS;
	}

	fprintf(fpAFDS, "Reading %s scenery:\n", pszSimName[fFSX+1]);	
	fprintf(fpAFDS, "The CFG file being used is: \x22%s\x22\n", szCfgPath);

	while (nArea < 10000)
	{	if (fUserAbort) return 0;
		sprintf(szArea,"Area.%03d", nArea);	
		if (GetPrivateProfileString(szArea,"Layer","", szParam, 64, szCfgPath))
		{	__int32 n = atoi(szParam);
			if ((n >= 0) && (n <= 9999) && (nAreas[n] < 0) &&
					(GetPrivateProfileString(szArea,"Local","", szPaths[n], MAX_PATH, szCfgPath) ||
						GetPrivateProfileString(szArea,"Remote","", szPaths[n], MAX_PATH, szCfgPath)))
			{	nAreas[n] = nArea;
				GetPrivateProfileString(szArea,"Title","", szTitles[n], 64, szCfgPath);
				if (GetPrivateProfileString(szArea,"Active","", szParam, 64, szCfgPath) &&
						(_strnicmp(szParam, "TRUE", 4) == 0))
					bActive[n] = 0xff;
			}
		}
		else if (nArea > 999) break;

		nArea++;
	}
	
MAINLOOPS:
	nArea = 0;
	while (nArea < 10000)
	{	if (fUserAbort) return 0;
		if (nAreas[nArea] >= 0)
		{	char szAreaWk[512];
			__int32 len;
			
			sprintf(szArea,"Area.%03d", nAreas[nArea]);
			sprintf(szAreaWk, "%s \x22%s\x22 (Layer=%d)", szArea, szTitles[nArea], nArea);
			fprintf(fpAFDS, "\n%s%s\n", chLine, szAreaWk);
			
			SetWindowText(GetDlgItem(hWnd, IDC_AREA), szAreaWk);
			SetWindowText(GetDlgItem(hWnd, IDC_FILE1), "");
			SetWindowText(GetDlgItem(hWnd, IDC_FILE2), "");
			SetWindowText(GetDlgItem(hWnd, IDC_FILE3), "");

			if (!bActive[nArea])
			{	fprintf(fpAFDS, "... Not Active\n");
				nArea++;
				continue;
			}

			fDeletionsPass = 1;

			// Repeat next part if finds any Airports
			// First pass = deletions, second = inclusions

			if ((len = strlen(szPaths[nArea])) && (szPaths[nArea][len - 1] == '\\'))
				szPaths[nArea][len - 1] = 0;

			ScanSceneryArea(szPaths[nArea]);
			if (fUserAbort) return 0;

			if (fDeletionsPass < 0)
			{	fDeletionsPass = 0;
				pSceneryName = szTitles[nArea];
				ScanSceneryArea(szPaths[nArea]);
				if (fUserAbort) return 0;
			}
		}
	
		nArea++;
	}

	fprintf(fpAFDS, "\n");
	fprintf(fpAFDS, chLine);

	if (fUserAbort) return 0;
	fWritingFiles = TRUE;
							
	if (pR)
	{	RWYLIST *p;
		FILE *pf, *pf2, *pfg = 0, *pfmyg = 0, *pft = 0, *pfi = 0, *pfbin = 0, *pftbin = 0;
		BOOL ftmi = TRUE;
		
		// Create binary file with runway data
		p = pR;
		pf = fopen("FstarRC.rws","wb");
		if (pf)
		{	while (p)
			{	RWYLIST *pLast = p;
				if (!p->fDelete && !p->fAirport &&
						(*((DWORD *) &p->r.chRwy[0]) != 0x00383939) &&
						(*((DWORD *) &p->r.chRwy[0]) != 0x00393939))
					fwrite(&p->r, 40, 1, pf);
				p = p->pTo;
			}

			fclose(pf);
			fOk = 1;
		}

		// Create binary fsm and bin files with runway data
		p = pR;
		pf = fopen("airports.fsm","wb");
		if (pf)
		{	while (p)
			{	RWYLIST *pLast = p;
				if (!p->fDelete && p->fAirport)
				{	APDATA ap;
						
					*((DWORD *) &ap.chICAO[0]) = *((DWORD *) &p->r.chICAO[0]);
					if (ap.chICAO[3] == ' ') ap.chICAO[3] = 0;
					ap.fLat = p->r.fLat;
					ap.fLong = p->r.fLong;
					ap.nAlt = (int) (p->r.fAlt + 0.5f);
					ap.Atis = p->Atis;

					fwrite(&ap, sizeof(APDATA), 1, pf);
				}
					
				p = p->pTo;
			}

			if (pf) fclose(pf);
			pf = 0;
		}
		
		// Create csv files with runway data
		p = pR;
		pf = fopen("runways.csv","wb");
		if (pf)
		{	while (p)
			{	if (!p->fDelete && !p->fAirport && 
							((p->r.chRwy[3] < '4') || fIncludeWater) &&
							((p->r.chRwy[3] >= '4') || (fIncludeWater >= 0)) &&
							!p->pGateList && !p->pTaxiwayList &&
						p->r.chRwy[3]) // This last eliminates 998 and 999 maker entries
				{	if (p->r.chICAO[3] == ' ') p->r.chICAO[3] = 0;
					fprintf(pf,"%.4s,%.4s,%.6f,%.6f,%d,%d,%d,%.6s\x0d\x0a",
								p->r.chICAO, p->r.chRwy,
								(double) p->r.fLat, (double) p->r.fLong,
								(int) (p->r.fAlt + 0.5), p->r.uHdg,
								p->r.uLen, p->r.chILS[0] ? p->r.chILS : "0");
				}
				p = p->pTo;
			}

			fclose(pf);
			fOk |= 2;
		}

		p = pR;
		ulTotalRwys2 = 0;
		pf = fopen("r4.csv", "wb");
		pfbin = fopen("r5.bin","wb");
		pf2 = fopen("r5.csv", "wb");
		pfg = fopen("g5.csv", "wb");
		if (nMatchMyAirline) pfmyg = fopen(chMyGates, "wb");
		pft = fopen("t5.csv", "wb");
		pftbin = fopen("t5.bin", "wb");
		pfi = fopen("runways.xml", "wb");
		if (!pfi)
		{	DWORD err = GetLastError();
			DebugBreak();
		}

		if (pf || pf2 || pfi)
			{	static char *pszILSflags[] = {	"", "B", "G", "BG", "D", "BD", "DG", "BDG" };
				DWORD dwCurrentICAO = 0;
					
				while (p)
				{	RWYLIST *pLast = p;
					if (p->r.chICAO[3] == ' ') p->r.chICAO[3] = 0;
						
					if (pfi && !p->fDelete && p->fAirport) 
					{	// IYP's XML data file ...
						if (ftmi)
						{	StartXML(pfi);
							ftmi = FALSE;
						}

						if (*((DWORD *) p->r.chICAO) != dwCurrentICAO)
						{	if (dwCurrentICAO)
								fprintf(pfi, "</ICAO>\x0d\x0a");
								
							dwCurrentICAO = *((DWORD *) p->r.chICAO);
							
							fprintf(pfi, "<ICAO id=\"%.4s\">\x0d\x0a<ICAOName>%s</ICAOName>\x0d\x0a",
								p->r.chICAO,
								StringXML(chWk, p->pAirportName ? p->pAirportName : ""));
							fprintf(pfi, "<Country>%s</Country>\x0d\x0a",
								StringXML(chWk, p->pCountryName ? p->pCountryName : ""));
							if (p->pStateName && p->pStateName[0])
								fprintf(pfi, "<State>%s</State>\x0d\x0a",
									StringXML(chWk, p->pStateName));
							fprintf(pfi, "<City>%s</City>\x0d\x0a",
								StringXML(chWk, p->pCityName ? p->pCityName : ""));
							fprintf(pfi, "<File>%s</File>\x0d\x0a",
								StringXML(chWk, p->pPathName ? p->pPathName : ""));
							fprintf(pfi, "<SceneryName>%s</SceneryName>\x0d\x0a",
								StringXML(chWk, p->pSceneryName ? p->pSceneryName : "" ));
							fprintf(pfi, "<Longitude>%.6f</Longitude>\x0d\x0a<Latitude>%.6f</Latitude>\x0d\x0a",
									(double) p->r.fLong, (double) p->r.fLat);
							fprintf(pfi, "<Altitude>%.2f</Altitude>\x0d\x0a<MagVar>%.3f</MagVar>\x0d\x0a",
									(p->r.fAlt +0.005), (double) p->fMagvar);
						}
					}

					if (!p->fDelete && !p->fAirport) 
					{	if (!p->pGateList && !p->pTaxiwayList && p->r.chRwy[3] &&
								((p->r.chRwy[3] < '4') || fIncludeWater) &&
								((p->r.chRwy[3] >= '4') || (fIncludeWater >= 0)))
						{	if (pf) fprintf(pf,"%.4s,%.4s,%.6f,%.6f,%d,%.3f,%d,%.6s,%d,%.3f,%.6f,%.6f,%d\x0d\x0a",
									p->r.chICAO, p->r.chRwy,
									(double) p->r.fLat, (double) p->r.fLong,
									(int) (p->r.fAlt + 0.5), (double) p->fHdg,
									p->r.uLen, p->r.chILS[0] ? p->r.chILS : "0",
									p->r.uWid, (double) p->fMagvar,
									(double) p->fLat, (double) p->fLong,
									p->nOffThresh);

							if (pf2) fprintf(pf2,"%.4s,%.4s,%.6f,%.6f,%d,%.3f,%d,%.6s%s,%d,%.3f,%.6f,%.6f,%d%s%s\x0d\x0a",
									p->r.chICAO, p->r.chRwy,
									(double) p->r.fLat, (double) p->r.fLong,
									(int) (p->r.fAlt + 0.5), (double) p->fHdg,
									p->r.uLen, p->r.chILS[0] ? p->r.chILS : "0",
									pszILSflags[(p->fILSflags >> 2) & 7], // ILS FLAGS (put "" here else)
									p->r.uWid, (double) p->fMagvar,
									(double) p->fLat, (double) p->fLong,
									p->nOffThresh,
									p->fCTO ? ",CT" : ",", p->fCL ? ",CL" : ",");

							if (pfbin)
							{	struct {
									char chICAO[4];
									unsigned short wRwyNum;
									char chDesig; // L, C, R or space
									char chStatus[3]; // CT, CL, CTL or all zero
									unsigned char wSurface; // 0-24 as in New BGL format
									unsigned char bSpare;
									float fLatThresh;
									float fLongThresh;
									float fLatCentre;
									float fLongCentre;
									float fAltitude;  // feet
									float fThrOffset;  // feet
									float fHdgMag;
									float fMagVar;
									float fLength; // feet
									float fWidth; // feet
									float fILSfreq; // zero if none
									char chILSflags[4];  
									float fILShdg;
									char chILSid[8];
									float fILSslope;
								} rbin;

								memset(&rbin, sizeof(rbin), 0);
								*((DWORD *) &rbin.chICAO[0]) =
									*((DWORD *) &p->r.chICAO[0]);
								rbin.wRwyNum = atoi(p->r.chRwy);
								rbin.chDesig = chRwyT[rbin.wRwyNum % 10];
								rbin.wRwyNum /= 10;
								strcpy(&rbin.chStatus[0], p->fCTO && p->fCL ? "CTL" :
												p->fCTO ? "CT" :
												p->fCL ? ",CL" : "");
								rbin.wSurface = (p->r.chSurfNew > 24 ? p->r.chSurf > 12 ? 0 : p->r.chSurf :
															p->r.chSurfNew) & 0x7f;
								rbin.fLatThresh = p->r.fLat;
								rbin.fLongThresh = p->r.fLong;
								rbin.fLatCentre = p->fLat;
								rbin.fLongCentre = p->fLong;
								rbin.fAltitude = p->r.fAlt;
								rbin.fThrOffset = (float) p->nOffThresh;
								rbin.fHdgMag = p->fHdg;
								rbin.fMagVar = p->fMagvar;
								rbin.fLength = p->r.uLen;
								rbin.fWidth = p->r.uWid;
								rbin.fILSfreq = (float) (p->r.chILS[0] ? atof(p->r.chILS) : 0.0);
								strcpy(&rbin.chILSflags[0], pszILSflags[(p->fILSflags >> 2) & 7]);
								rbin.fILShdg = (float) atof(p->r.chILSHdg);
								memcpy(&rbin.chILSid[0], &p->r.chILSid[0], 8);
								rbin.fILSslope = p->r.fILSslope;
								
								fwrite(&rbin, sizeof(rbin), 1, pfbin);
							}

							if (pfi && !ftmi)
							{	// IYP's XML data file ...
								static char *pszDesig[5] = { "", "L", "R", "C", "W" };
								static char *pszLights[4] = { "NONE", "LOW", "MEDIUM", "HIGH" };
								static char *pszVASI[16] = { "NONE", "VASI21", "VASI31", "VASI22", "VASI32", "VASI23", "VASI33",
									"PAPI2", "PAPI4", "TRICOLOR", "PVASI", "TVASI", "BALL", "APAP/PANELS", "UNKNOWN14", "UNKNOWN15" };
								static char *pszAppLights[16] = { "NONE", "ODALS", "MALSF", "MALSR", "SSALF", "SSALR", "ALSF1",
									"ALSF2", "RAIL", "CALVERT", "CALVERT2", "MALS", "SALS", "UNKNOWN13", "SALS2", "UNKNOWN15" };

								fprintf(pfi, "<Runway id=\"%.2s%s\">\x0d\x0a<Len>%d</Len>\x0d\x0a<Hdg>%.3f</Hdg>\x0d\x0a",
										&p->r.chRwy[1], pszDesig[p->r.chRwy[3] & 7], p->r.uLen, (double) p->fHdg);
								ulTotalRwys2++;
								fprintf(pfi, "<Def>%s</Def>\x0d\x0a<ILSFreq>%s</ILSFreq>\x0d\x0a<ILSHdg>%s</ILSHdg>\x0d\x0a<ILSid>%s</ILSid>\x0d\x0a<ILSslope>%.2f</ILSslope>\x0d\x0a",
										(p->r.chSurfNew > 24) ? szRwySurf[(p->r.chSurf > 12) ? 0 : p->r.chSurf] :
                                        szNRwySurf[p->r.chSurfNew], p->r.chILS,	p->r.chILSHdg, p->r.chILSid, p->r.fILSslope);
								if (p->r.chNameILS[0])
									fprintf(pfi, "<ILSname>%s</ILSname>\x0d\x0a", 
										StringXML(chWk, p->r.chNameILS));
								
								fprintf(pfi, "<Lat>%.6f</Lat>\x0d\x0a<Lon>%.6f</Lon>\x0d\x0a",
									(double) p->r.fLat, (double) p->r.fLong);
								fprintf(pfi, "<FSStartLat>%.6f</FSStartLat>\x0d\x0a<FSStartLon>%.6f</FSStartLon>\x0d\x0a",
									(double) p->r.fFSLat, (double) p->r.fFSLong);
								fprintf(pfi, "<ClosedLanding>%s</ClosedLanding>\x0d\x0a",
										p->fCL ? "TRUE" : "FALSE");
								fprintf(pfi, "<ClosedTakeoff>%s</ClosedTakeoff>\x0d\x0a",
										p->fCTO ? "TRUE" : "FALSE");
								fprintf(pfi, "<ApproachLights>%s</ApproachLights>\x0d\x0a", pszAppLights[p->r.bAppLights & 0x0f]);
								fprintf(pfi, "<EdgeLights>%s</EdgeLights>\x0d\x0a<CenterLights>%s</CenterLights>\x0d\x0a<CenterRed>%s</CenterRed>\x0d\x0a",
									pszLights[p->r.bLights & 3], pszLights[(p->r.bLights >> 2) & 3], (p->r.bLights & 0x20) ? "TRUE" : "FALSE");
								fprintf(pfi, "<EndLights>%s</EndLights>\x0d\x0a<REIL>%s</REIL>\x0d\x0a<Touchdown>%s</Touchdown>\x0d\x0a",
									(p->r.bAppLights & 0x20) ? "TRUE" : "FALSE",
									(p->r.bAppLights & 0x40) ? "TRUE" : "FALSE",
									(p->r.bAppLights & 0x20) ? "TRUE" : "FALSE");
								fprintf(pfi, "<Strobes>%d</Strobes>\x0d\x0a", p->r.nStrobes);
								fprintf(pfi, "<LeftVASI>%s</LeftVASI>\x0d\x0a", pszVASI[p->r.bVASIleft & 0x0f]);
								if (p->r.bVASIleft & 0x0f)
								{	fprintf(pfi, "<LeftVASIbiasX>%.2f</LeftVASIbiasX>\x0d\x0a",
										p->r.fLeftBiasX + 0.005);
									fprintf(pfi, "<LeftVASIbiasZ>%.2f</LeftVASIbiasZ>\x0d\x0a",
										p->r.fLeftBiasZ + 0.005);
									fprintf(pfi, "<LeftVASIspacing>%.2f</LeftVASIspacing>\x0d\x0a",
										p->r.fLeftSpacing + 0.005);
									fprintf(pfi, "<LeftVASIpitch>%.2f</LeftVASIpitch>\x0d\x0a",
										p->r.fLeftPitch + 0.005);
								}

								fprintf(pfi, "<RightVASI>%s</RightVASI>\x0d\x0a", pszVASI[p->r.bVASIright & 0x0f]);

								if (p->r.bVASIright & 0x0f)
								{	fprintf(pfi, "<RightVASIbiasX>%.2f</RightVASIbiasX>\x0d\x0a",
										p->r.fRightBiasX + 0.005);
									fprintf(pfi, "<RightVASIbiasZ>%.2f</RightVASIbiasZ>\x0d\x0a",
										p->r.fRightBiasZ + 0.005);
									fprintf(pfi, "<RightVASIspacing>%.2f</RightVASIspacing>\x0d\x0a",
										p->r.fRightSpacing + 0.005);
									fprintf(pfi, "<RightVASIpitch>%.2f</RightVASIpitch>\x0d\x0a",
										p->r.fRightPitch + 0.005);
								}

								fprintf(pfi, "<ThresholdOffset>%d</ThresholdOffset>\x0d\x0a",
									p->nOffThresh);
								fprintf(pfi, "<PatternTakeOff>%s</PatternTakeOff>\x0d\x0a",
									(p->r.fPrimary) ?
										((p->r.bPatternFlags & 1) ? "None" :
										(p->r.bPatternFlags & 4) ? "Right" : "Left") :
										((p->r.bPatternFlags & 8) ? "None" :
										(p->r.bPatternFlags & 32) ? "Right" : "Left"));
								fprintf(pfi, "<PatternLanding>%s</PatternLanding>\x0d\x0a",
											(p->r.fPrimary) ?
											((p->r.bPatternFlags & 2) ? "None" :
										(p->r.bPatternFlags & 4) ? "Right" : "Left") :
										((p->r.bPatternFlags & 16) ? "None" :
										(p->r.bPatternFlags & 32) ? "Right" : "Left"));
								if (p->r.fPatternAlt)
									fprintf(pfi, "<PatternAltitude>%.0f</PatternAltitude>\x0d\x0a",
										(double) (p->r.fPatternAlt + 0.5));

								fprintf(pfi, "</Runway>\x0d\x0a");
							}
						}

						if (p->pGateList && pfg)
						{	// Gate list
							WORD w = 0, wCtr = p->pGateList->wCount;
							NGATEHDR *pgh = (NGATEHDR *) p->pGateList;
							NGATE *pg = (NGATE *) ((BYTE *) p->pGateList + sizeof(NGATEHDR));
							NGATE2 *pg2 = (NGATE2 *) ((pgh->wId == OBJTYPE_NEWTAXIPARK) ? pg : 0);
							NGATE3 *pg3 = (NGATE3 *) ((pgh->wId == OBJTYPE_NEWNEWTAXIPARK) ? pg : 0);
							if (pgh->wId == OBJTYPE_MSFSTAXIPARK) pg2 = (NGATE2 *) pg;

							while (w < wCtr)
							{	LOCATION locg;
								double dLat, dLon;
								char chLetter[4], *pszLetter;
								__int32 nPark = pg->bPushBackName & 0x3f;
								BOOL fMyAirlineGate = FALSE;
			
								if (nPark <= 11)
									pszLetter = szParkNames[nPark];
								
								else
								{	chLetter[0] = nPark + 0x35;
									chLetter[1] = 0;
									pszLetter = &chLetter[0];
								}

								SetLocPos(&locg, 0,
									pg3 ? pg3->nLat : pg2 ? pg2->nLat : pg->nLat,
									pg3 ? pg3->nLon : pg2 ? pg2->nLon : pg->nLon, 0, 0, &dLat, &dLon);								
								fprintf(pfg, "%.4s,%s,%d,%.6f,%.6f,%.1f,%.1f,%d%s", 
									p->r.chICAO, pszLetter,
									pg->wNumberType >> 4, dLat, dLon, 
									(double) pg->fRadius, (double) pg->fHeading, pg->wNumberType & 15,
									(fMarkJetways && (pg->bCodeCount & 0x80)) ? ",Jetway" : ",");

								// ######## List of airline codes ...
								if (pg->bCodeCount & 0x7f)
								{	BYTE b = pg->bCodeCount & 0x7f;
									char *pA = (char *) pg + (pg3 ? sizeof(NGATE3) : pg2 ? sizeof(NGATE2) : sizeof(NGATE));
									while (b-- && isprint(*pA))
									{	fprintf(pfg, ",%.4s", pA);

										if (nMatchMyAirline && (_strnicmp(pA, chMyAirLine, nMatchMyAirline) == 0))
											fMyAirlineGate = TRUE;

										pA += 4;
									}
								}
							
								fprintf(pfg,"\x0d\x0a");

								if (fMyAirlineGate)
								{	char *pszGateName, chGateName[5];
	
									pg->bPushBackName &= 0x3f;
									if (pg->bPushBackName > 11)
									{	chGateName[0] = pg->bPushBackName + 0x35;
										chGateName[1] = 0;
										pszGateName = &chGateName[0];
									}
	
									else pszGateName = szParkNames[pg->bPushBackName];
								
									fprintf(pfmyg, "%.4s, %s %s%d (%s)\x0d\x0a", 
											p->r.chICAO,
											pszParkType[pg->bPushBackName >= 11 ? 10 : pg->bPushBackName],
											pszGateName, pg->wNumberType >> 4, pszGateType[pg->wNumberType & 15]);
								}

								w++;
								pg = (NGATE *) ((BYTE *) pg + (4 * (int) (pg->bCodeCount & 0x7f)) +
										(pg3 ? sizeof(NGATE3) : pg2 ? sizeof(NGATE2) : sizeof(NGATE)));
								if (pgh->wId == OBJTYPE_MSFSTAXIPARK)
									pg = (NGATE*)((BYTE*)pg + 20); // 20 additional bytes
								if (pg2) pg2 = (NGATE2 *) pg;
								if (pg3) pg3 = (NGATE3 *) pg;
							}
						}

						if (p->pTaxiwayList && (pft || pftbin))
						{	// Taxiway list
							TWHDR *twh = p->pTaxiwayList;
							while (twh->wPoints)
							{	TW *tw = (TW *) &twh[1];
								if (pft) fprintf(pft, "%.4s,%.8s,%.2f", 
									p->r.chICAO, twh->chName, (double) twh->fMinWid);
								if (pftbin)
								{	struct {
										DWORD dwNumPts;
										char chICAO[4];
										char chName[8];
										float fMinWidth;
									} tbin;
									tbin.dwNumPts = twh->wPoints;
									*((DWORD *) &tbin.chICAO[0]) =
										*((DWORD *) &p->r.chICAO[0]);
									memcpy(&tbin.chName[0], &twh->chName[0], 8);
									tbin.fMinWidth = tw->fWid;

									fwrite(&tbin, sizeof(tbin), 1, pftbin);
								}

								while (twh->wPoints--)
								{	if (pft) fprintf(pft, ",%.6f,%.6f,%d,%.2f",
										(double) tw->fLat, (double) tw->fLon, tw->bType, (double) tw->fWid);
									if (pftbin)
									{	struct {
											float fLat;
											float fLon;
											float fType;
											float fWid;
										} tpt;
					
										tpt.fLat = tw->fLat;
										tpt.fLon = tw->fLon;
										tpt.fType = tw->bType;
										tpt.fWid = tw->fWid;
										fwrite(&tpt, sizeof(tpt), 1, pftbin);
									}
									
									tw++;
								}

								if (pft) fprintf(pft,"\x0d\x0a");
								twh = (TWHDR *) tw;
							}
						}
					}
					p = p->pTo;
				}

				if (pf) fclose(pf);
				if (pfbin) fclose(pfbin);
				if (pf2) fclose(pf2);
				if (pfg) fclose(pfg);
				if (pfmyg) fclose(pfmyg);
				if (pft) fclose(pft);
				if (pftbin) fclose(pftbin);
				if (pfi)
				{	if (ftmi) StartXML(pfi);
					if (dwCurrentICAO) 	fprintf(pfi, "</ICAO>\x0d\x0a");
					fprintf(pfi,"</data>\x0d\x0a");
					fclose(pfi);
				}
				fOk |= 2;
			}

		while (pR)
		{	RWYLIST *pLast = pR;
			if (pR->pGateList) free(pR->pGateList);
			if (pR->pTaxiwayList) free(pR->pTaxiwayList);
			pR = pR->pTo;
			free(pLast);
		}
	}

	MakeHelipadsFile();
	MakeCommsFile();
	if (fProcessTA)	UpdateTransitionAlts();
	fclose(fpAFDS);

	if (pLocPak) free(pLocPak);
	if (pMaterials) free(pMaterials);

	ulTotalRwys = ulTotalRwys2;
	PostMessage(hWnd, WM_USER, 1, fOk);
	
	fWritingFiles = FALSE;
	PostMessage(hWnd, WM_USER, 0, fOk);
	if (fQuiet) SendMessage(hWnd, WM_CLOSE, 0, 0);
	return 0;
}


/******************************************************************************
         DlgProc
******************************************************************************/

BOOL CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{	static HBRUSH hbrMain = NULL;
	PAINTSTRUCT ps;
	char wk[256];

	switch (msg)
	{	case WM_INITDIALOG:
			hbrMain = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
			SetWindowText(hDlg, "Make Runways: Version 5.11");
			if (fQuiet) ShowWindow(hDlg, SW_HIDE);
			return TRUE;

		case WM_COMMAND:
			fUserAbort = TRUE;
			if (!fWritingFiles)
				PostQuitMessage(0);
			return TRUE ;

		case WM_USER:
			switch (wParam)
			{	case 0:
					SetWindowText(GetDlgItem(hWnd, IDC_AREA),
						(lParam & 2) ? "All main data files produced okay" : "Failed to make all of the data files!");
					SetWindowText(GetDlgItem(hWnd, IDC_FILE1),
						(lParam & 1) ? " " : "Failed to make FStarRC RWS file!");
					SetWindowText(GetDlgItem(hWnd, IDC_FILE2), "");
					SetWindowText(GetDlgItem(hWnd, IDC_FILE3), "");
					SetWindowText(GetDlgItem(hWnd, IDC_PRESS),"OK");
					if (!fQuiet) MessageBeep(MB_ICONEXCLAMATION);
					// Drop through ...
	
				case 1: //Airports or runways
				case 2:
					sprintf(wk, "Total airports = %d, runways = %d", ulTotalAPs, ulTotalRwys);
					SetWindowText(GetDlgItem(hWnd, IDC_TOTALS1), wk);
					__int32 x = sprintf(wk, "Number of BGLs = %d", ulTotalBGLs);
					if (ulTotalBytes)
						sprintf(&wk[x], ", Bytes scanned = %d", ulTotalBytes);
					SetWindowText(GetDlgItem(hWnd, IDC_TOTALS2), wk);
					break;
					
				default:
					break;
			}

			break;
	
		case WM_PAINT:
			BeginPaint(hDlg, (LPPAINTSTRUCT) &ps);
			EndPaint(hDlg, (LPPAINTSTRUCT) &ps);
			return 0;

		case WM_SYSCOLORCHANGE:
			DeleteObject(hbrMain);
			hbrMain = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
			break;

		case WM_DESTROY:
			if (hbrMain) DeleteObject(hbrMain);
			break;

		// Painting text control backgrounds
		case WM_CTLCOLORSTATIC:
		{	POINT point;
			SetBkColor((HDC) wParam, GetSysColor(COLOR_BTNFACE));
			SetTextColor((HDC) wParam,	GetSysColor(COLOR_BTNTEXT));
			UnrealizeObject(hbrMain);

			point.x = point.y = 0;
			ClientToScreen(hDlg, &point);
			SetBrushOrgEx((HDC) wParam, point.x, point.y, NULL);
			return ((DWORD) hbrMain);
		}
	}

	return FALSE ;
}

/******************************************************************************
         WinMain
******************************************************************************/

LRESULT CALLBACK NullWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{	switch (message)	
	{	case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{
	char* pch = &lpszCmdLine[0];
	MSG msg;
	DWORD dwId;

	hInstance = hInst;

#ifdef _DEBUG
	MessageBox(NULL, "Press OK when ready ...", "MakeRunways Debugging", MB_OK);
#endif

	pch = strchr(pch, '/');

	while (pch && *pch)
	{	if (_strnicmp(&pch[1], "MSFS", 4) == 0)
		{	fDoMSFS = TRUE;
			pch += 5;
		}

		if (_strnicmp(&pch[1], "WATER", 5) == 0)
		{	fIncludeWater = TRUE;
			pch += 6;
			if (_strnicmp(&pch[0], "ONLY", 4) == 0)
			{
				fIncludeWater = -1;
				pch += 4;
			}
		}

		else if (_strnicmp(&pch[1], "JET", 3) == 0)
		{	fMarkJetways = TRUE;
			pch += 4;
		}

		else if (_strnicmp(&pch[1], "SSNG", 4) == 0)
		{	fNoLoadLorby = TRUE;
			pch += 5;
		}

		else if (_strnicmp(&pch[1], "OLDNODRAW", 9) == 0)

		{
			fNoDrawHoldConvert = FALSE;
			pch += 10;
		}

		else if (_strnicmp(&pch[1], "LOCAL", 5) == 0)

		{
			fLocal = TRUE;
			pch += 6;
		}

		else if (_strnicmp(&pch[1], "DECC", 4) == 0)

		{
			fDecCoords = TRUE;
			pch += 5;
		}

		else if (pch[1] == '>')

		{
			pch += 2;
			nMinRunwayLen = atoi(pch);
		}

		else if ((pch[1] == '+') && (toupper(pch[2]) == 'D'))

		{
			fDebug = TRUE;
			pch += 3;
		}

		else if ((pch[1] == '+') && (toupper(pch[2]) == 'T'))

		{
			fProcessTA = TRUE;
			pch += 3;
		}

		else if ((pch[1] == '+') && (toupper(pch[2]) == 'Q'))

		{
			fQuiet = TRUE;
			pch += 3;
		}

		else

		{
			pch++;
			nMatchMyAirline = strlen(pch);
			if (nMatchMyAirline > 4)

			{
				nMatchMyAirline = 4;
				pch[4] = 0;
			}

			strcpy(chMyAirLine, pch);
			sprintf(chMyGates, "%s Gates.txt", chMyAirLine);
			break;
		}

		pch = strchr(pch, '/');
	}

	GetModuleFileName(hInstance, szMyPath, MAX_PATH);

	pch = strrchr(szMyPath, '\\');
	if (pch) *(pch + 1) = 0;
	else szMyPath[0] = 0;
	strcat(szMyPath, "scenery.cfg");

	if (fQuiet)
	{
		WNDCLASS Class;
		char szAppName[] = "MakeRwys";
		// Register the class for our Window
		memset(&Class, 0, sizeof(WNDCLASS));

		Class.hCursor = LoadCursor(NULL, IDC_ARROW);
		Class.hIcon = NULL;
		Class.lpszMenuName = (LPSTR)NULL;
		Class.lpszClassName = szAppName;
		Class.hbrBackground = NULL; //(HBRUSH) (COLOR_BTNFACE + 1);
		Class.hInstance = hInst;
		Class.style = CS_HREDRAW | CS_VREDRAW;
		Class.lpfnWndProc = NullWndProc;
		if (!RegisterClass(&Class))
		{
			DWORD dwErr = GetLastError();
			DebugBreak();
		}

		hWnd = CreateWindow(szAppName, szAppName, 0, CW_USEDEFAULT, CW_USEDEFAULT,
			0, 0, NULL, NULL, hInstance, NULL);
		if (!hWnd)
		{
			DWORD dwErr = GetLastError();
			DebugBreak();
		}
		else ShowWindow(hWnd, SW_HIDE);
	}
	else
		hWnd = CreateDialog(hInstance, "MyDlg", 0, DlgProc);

	CreateThread(NULL, 0, MainRoutine, NULL, 0, &dwId);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(hWnd, (LPMSG)&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

/******************************************************************************
         UpdateTransitionAlts
******************************************************************************/

void UpdateTransitionAlts(void)
{	char *chM4 = 0, *chDAT = 0, *pchD, *pch;
	int nLenM4, nLenDAT;
	BOOL fRewrite = FALSE;
	
	FILE *fpM4 = fopen("M4.csv", "rb"), *fpDAT;
	if (!fpM4)
	{	fprintf(fpAFDS, "**** The /+T option is ignored because of no M4.CSV file supplied! ****\x0d\x0a");
		return;
	}

	fpDAT = fopen("Aivlasoft\\NavData\\Airports.DAT", "rb");
	if (!fpDAT)
	{	fprintf(fpAFDS, "**** The /+T option is ignored because of no ""Aivlasoft\\NavData\\Airports.DAT""! ****\x0d\x0a");
		fclose(fpM4);
		return;
	}

	fseek(fpM4, 0, SEEK_END);
	nLenM4 = ftell(fpM4);
	chM4 = malloc(2*(int) nLenM4); // Room for expansion
	fseek(fpM4, 0, SEEK_SET);

	fseek(fpDAT, 0, SEEK_END);
	nLenDAT = ftell(fpDAT);
	chDAT = malloc((int) nLenDAT+1);
	fseek(fpDAT, 0, SEEK_SET);

	nLenM4 = fread(chM4, 1, nLenM4, fpM4);
	nLenDAT = fread(chDAT, 1, nLenDAT, fpDAT);
	chM4[nLenM4] = chDAT[nLenDAT] = 0;
	fclose(fpM4);
	fclose(fpDAT);

	// For each TA in chDAT compare with the one inchM4 and log/change it if different
	pchD = &chDAT[0];
	while (pchD && *pchD)
	{	while (*pchD && (*pchD < 33))
			pchD++;
		if (*pchD)
		{	if (*pchD != ';')
			{	// Next 4 chars = ICAO:
				char szICAO[5];
				strncpy(szICAO, pchD, 4);
				szICAO[4] = 0;
				
				pch = strstr(chM4, szICAO);

				if (pch)
				{	char *pchEnd = strchr(pch, 13);
					if (!pchEnd) pchEnd = &chM4[nLenM4];
					pch = strchr(pch, ',');
					if (pch)
					{	pch = strchr(pch+1, ',');
						if (pch && (pch < pchEnd))
						{	int nOldTA = atoi(++pch), nNewTA;
							char ch = pchD[9];
							pchD[9] = 0;
							nNewTA = atoi(&pchD[4]);
							pchD[9] = ch;
							
							if ((nNewTA > 999) && (nOldTA != nNewTA))
							{	char szNewTA[16];
								int n = sprintf(szNewTA, "%d", nNewTA);
					
								if (n > (pchEnd - pch))
								{	// Need more space!
									memmove(&pch[n - (pchEnd - pch)], pch, &chM4[nLenM4] - pch + 1);
									nLenM4 += n - (pchEnd - pch);
								}
								memcpy(&pch[0], szNewTA, n);
								while (pch[n] > 31) pch[n++] = ' ';

								fprintf(fpAFDS, "Updated TA for %.4s (old %d, new %d)\x0d\x0a",
									szICAO, nOldTA, nNewTA);
								fRewrite = TRUE;
							}
						}								
					}
				}
			}

			pchD = strchr(pchD, 10);
		}
	}

	if (fRewrite)
	{	fpM4 = fopen("M4.csv", "wb");
		fwrite(chM4, 1, nLenM4, fpM4);
		fclose(fpM4);
	}

	if (chM4) free(chM4);
	if (chDAT) free(chDAT);
}

/******************************************************************************
         End of MakeRwys
******************************************************************************/
