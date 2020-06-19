/* Comms.c
*******************************************************************************/

#include "MakeRwys.h"

/******************************************************************************
         Data
******************************************************************************/

typedef struct 
{	char chICAO[4];
	int nStart;
	int nEnd;
	int nStartDel;
	int nEndDel;
	char *pApName;
} COMMSLIST;

typedef struct
{	int nType;
	char chFreq[6];
	char szName[256];
} COMMSENTRY;

COMMSLIST *pCommsList = 0;
int nComms = 0;

/******************************************************************************
         DeleteComms
******************************************************************************/

void DeleteComms(char *pchICAO)
{	int i = 0;
	if (!pCommsList) return;

	while (i < nComms)
	{	if (*((DWORD *) &pCommsList[i].chICAO[0]) == *((DWORD *) pchICAO))
		{	int n = nComms - i - 1;
			if (n) memmove(&pCommsList[i], &pCommsList[i+1], n * sizeof(COMMSLIST));
			nComms--;
			if (i) i--;
		}

		else if (strncmp(pCommsList[i].chICAO, pchICAO, 4) > 0)
			break;
	
		i++;
	}
}

/******************************************************************************
         AddComms
******************************************************************************/

void AddComms(char *pchICAO, int nCommStart, int nCommEnd, int nCommDelStart, int nCommDelEnd, char *pApName)
{	static BOOL ftm = TRUE;
	int i = 0, n;

	if (!pCommsList)
		pCommsList = malloc(1000000 * sizeof(COMMSLIST));
	
	if (!pCommsList)
	{	if (ftm) fprintf(fpAFDS, "*** ERROR: Could not obtain enough memory to build COMMS list! ***\n");
		ftm = FALSE;
		return;
	}

	while (i < nComms)
	{	// Insert in correct oder
		if (strncmp(pCommsList[i].chICAO, pchICAO, 4) > 0)
			break;
		i++;
	}
		
	// fprintf(fpAFDS, "COMMS: Added %.04s after %.04s\n", pchICAO, i ? pCommsList[i-1].chICAO : "");
	n = nComms - i;
	if (n) memmove(&pCommsList[i+1], &pCommsList[i], n * sizeof(COMMSLIST));
	nComms++;

	*((DWORD *) &pCommsList[i].chICAO[0]) = *((DWORD *) &pchICAO[0]);
	pCommsList[i].nEnd = nCommEnd;
	pCommsList[i].nEndDel = nCommDelEnd;
	pCommsList[i].nStart = nCommStart;
	pCommsList[i].nStartDel = nCommDelStart;
	pCommsList[i].pApName = pApName;
}

/******************************************************************************
         MakeCommsFile
******************************************************************************/

#define CLIST_MAX 512
void MakeCommsFile(void)
{	FILE *pf = fopen("f5.csv","wb");
	FILE *pf2 = fopen("f4.csv","wb");
	FILE *pt = fopen("runways.txt","rb");
	int i = 0, j = 0;
	COMMSENTRY c[CLIST_MAX];
	char *pch2 = 0, *pch = 0, *pchData = 0;
	char chPrevICAO[5], *pchICAO = 0;
	memset(&chPrevICAO[0], 0, 5);

	__try{

	if (nComms && pf && pf2 && pt)
	{	if (pchICAO) strncpy(chPrevICAO, pchICAO, 4);
		pchICAO = pCommsList[0].chICAO;
		
		fprintf(pf2, "'4004\x0d\x0a");

		errnum = 0;
	
		while (i <= nComms)
		{		if ((i >= nComms) || (*((DWORD *) &pCommsList[i].chICAO[0]) != *((DWORD *) &pchICAO[0])))
				{	// Output results for this ICAO
					int t, k = j-1;
					BOOL fAddName = FALSE;

					errnum = 1;
					while (k >= 0)
					{	if (c[k].nType)
						{	fAddName = TRUE;
							break;
						}
						k--;
					}
					
					errnum = 2;
					if (fAddName)
					{	char chFreqs2[14][6];
						// was static int nEquivs[16] = { -1, 0, 5, 4, 10, 2, 3, 8, 6, 7, -1, -1, -1, -1, 1, -1 };
						static int nEquivs[16] = { -1, 0, 5, 4, 10, 2, 3, 8, 6, 7, -1, 1, 1, 1, 1, -1 };
						memset(chFreqs2, 0, sizeof(chFreqs2));

						errnum = 3;

						k = j-1;
						while (k >= 0)
						{	if (c[k].nType == 0)
							{	char chName[61], *psz;
								fprintf(pf, "%.4s,0,0,\x22%s\x22\x0d\x0a", pchICAO,c[k].szName);
								strncpy(chName, c[k].szName, 60);
								chName[60] = 0;
								psz = chName;

								errnum = 4;

								do 
								{	psz = strchr(psz,',');
									if (psz) *psz = '-';
								} while (psz);
								fprintf(pf2, "%.4s,%s", pchICAO, chName);
								break;
							}
							k--;
							errnum = 5;
						}
					
						for (t = 1; t < 16; t++)
						{	k = 0;
							while (k < j)
							{	if (c[k].nType == t)
								{	fprintf(pf, "%.4s,%d,%.6s,\x22%s\x22\x0d\x0a", pchICAO,c[k].nType,c[k].chFreq,c[k].szName);
									errnum = 6;
									if (nEquivs[t] >= 0)
									{	if (chFreqs2[nEquivs[t]][0] == 0)
											memcpy(chFreqs2[nEquivs[t]], c[k].chFreq, 6);
										if (t == 8) // Use 2nd Dep for Arr if none ...
											memcpy(chFreqs2[11], c[k].chFreq, 6);
										else if (t == 9) // Use 2nd Arr for Dep if none ...
											memcpy(chFreqs2[12], c[k].chFreq, 6);
										else if (t == 6) // Use 2nd Twr for Gnd if none ...
											memcpy(chFreqs2[13], c[k].chFreq, 6);
										errnum = 7;
									}
								}
								k++;
							}
						}
						errnum = 8;

						// Fill in alternatives ...
						if (chFreqs2[1][0] == 0) memcpy(chFreqs2[1], chFreqs2[8], 6);
						// if (chFreqs2[3][0] == 0) memcpy(chFreqs2[3], chFreqs2[9], 6); // 9 unused
						if (chFreqs2[5][0] == 0) memcpy(chFreqs2[5], chFreqs2[10], 6);
						if (chFreqs2[7][0] == 0) memcpy(chFreqs2[7], chFreqs2[11], 6);
						if (chFreqs2[6][0] == 0) memcpy(chFreqs2[6], chFreqs2[12], 6);
						if (chFreqs2[2][0] == 0) memcpy(chFreqs2[2], chFreqs2[13], 6);

						errnum = 9;

						for (t = 0; t < 8; t++)
							fprintf(pf2, ",%.6s", chFreqs2[t][0] ? chFreqs2[t] : "0");
						fprintf(pf2, "\x0d\x0a");
						errnum = 10;
					}
	
					// Set up for next ICAO	
					pchICAO = pCommsList[i].chICAO;
					j = 0;
					errnum = 11;
				}

				errnum = 12;

				if (i < nComms)
				{	// See to individual deletions first ...
					if (j && pCommsList[i].nStartDel && pCommsList[i].nEndDel)
					{	int nLen = pCommsList[i].nEndDel - pCommsList[i].nStartDel;
						char *pchData = malloc(nLen+1);
						if (pchData)
						{	char *pch = pchData;
							COMMSENTRY cwk;

							errnum = 13;

							memset(pchData, 0, nLen+1);
							fseek(pt, pCommsList[i].nStartDel, SEEK_SET);
							fread(pchData, 1, nLen, pt);
							errnum = 14;

							while (1)
							{	int j2 = 0;

								pch = strstr(pch, "COM:");
								if (!pch) break;

								pch = strchr(pch, '=');
								if (!pch) break;
								pch++;
								cwk.nType = atoi(pch);

								errnum = 15;

								pch = strchr(pch, '=');
								if (!pch) break;
								pch++;
								memcpy(&cwk.chFreq[0], pch, 6);
								errnum = 16;

								while (j2 < j)
								{	if (memcmp(&cwk.nType, &c[j2].nType, 10) == 0)
										c[j2].nType = 0;
									j2++; // ############### was omitted
								}
								errnum = 17;
							}

							free(pchData);
							errnum = 18;
						}
					}
					errnum = 19;

					// Now for airport name
					if (pCommsList[i].pApName)
					{	memset(c[j].szName, 0, 256);
						strcpy(c[j].szName, pCommsList[i].pApName);
						str2ascii(c[j].szName);
						c[j].nType = 0;
						if (j < (CLIST_MAX-1))
							j++;
						errnum = 20;
					}
					errnum = 21;
					
					// Now add details
					if (pCommsList[i].nStart && pCommsList[i].nEnd)
					{	int nLen = pCommsList[i].nEnd - pCommsList[i].nStart;
						pchData = malloc(nLen+1);
						if (pchData)
						{	pch = pchData;

							memset(pchData, 0, nLen+1);
							fseek(pt, pCommsList[i].nStart, SEEK_SET);
							fread(pchData, 1, nLen, pt);
							pchData[nLen] = 0;
							errnum = 22;

							while (j < CLIST_MAX)
							{	pch = strstr(pch, "COM:");
								if (!pch) break;

								pch = strchr(pch, '=');
								if (!pch) break;
								pch++;
								c[j].nType = atoi(pch);

								errnum = 23;

								pch = strchr(pch, '=');
								if (!pch) break;
								pch++;
								memcpy(&c[j].chFreq[0], pch, 6);
								errnum = 24;

								// If already same type and freq, delete previous one
								if (j)
								{	int j2;
									errnum = 25;

									for (j2 = 0; j2 < j; j2++)
									{	if (memcmp(&c[j2].nType, &c[j].nType, 10) == 0)
											c[j2].nType = 0;
										errnum = 26;
									}
								}
								errnum = 27;

								pch = strchr(pch, '=');
								if (!pch) break;
								pch += 2;
								pch2 = strchr(pch, 0x22);
								errnum = j;

								if (!pch2) break;
								memset(c[j].szName, 0, 256);
								memcpy(c[j].szName, pch, min(pch2-pch, 255));
								errnum = 29;

								if (j < (CLIST_MAX-1))
									j++;
							}
							errnum = 30;

							if (pchData)
							{	free(pchData);
								pchData = 0;
							}
						}
					}
				}
				errnum = 31;

				i++;
		}
	}

	errnum = 32;

	if (pt) fclose(pt);
	if (pf2) fclose(pf2);
	if (pf) fclose(pf);

	// Exception Handler
	}
	__except(	EXCEPTION_EXECUTE_HANDLER)	
	{	fprintf(fpAFDS, "\n\n***ERROR %d: pch2=%08X, pch=%08X, j=%d, pchData=%08X\n",
			errnum, (DWORD) pch2, (DWORD) pch, j, (DWORD) pchData);
		fprintf(fpAFDS, "          prevICAO=%s, thisICAO=%.4s\n", chPrevICAO, pchICAO);
	}
}

/******************************************************************************
         End of COMMS
******************************************************************************/
