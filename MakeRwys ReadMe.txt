MakeRwys.EXE  Version 4.6991, by Pete Dowson
===========================================

FS2000, FS2002, FS2004
----------------------
Place the EXE into your main FS folder
where the SCENERY.CFG file is located.

FSX, FSX-SE, Prepar3D
---------------------
Place the EXE into your the relevant main FS folder. For FSX Steam Edition
this will be in the Steam folder, under "steamapps\common".

MakeRunways will find the correct Scenery.CFG file automatically based on
the EXE name it finds there and its version number.

THEN
----
Double click the MakeRwys.EXE file in Explorer to run it.

The files created are derived by checking all of the BGLs in all of the Active 
scenery layers with locally installed files, as defined in the SCENERY.CFG 
file.

The process is different with pre-FS2004 AFD files and the new FS2004 airport 
data files, though the usable databases produced are in the same format.

The files produced are:

Runways.txt
	This is a text file showing the data analysed by the program in 
	readable format. Use this to check on things when something odd seems 
	to happen. you can find conflicting or overriding sceneries and so on 
	this way. However, be warned. The file will typically grow to over 10 
	megabytes. You need to view it with a text editor capable of dealing 
	easily with large files.
	
	You can delete this file afterwards. It is only for your viewing, 
	nothing else.
	
Runways.xml
   An XML file providing a database of airports and their runways.
	
FStarRC.rws
	This is a binary runways database used by my FStarRC program.
	
Airports.fsm
	This is a binary airports database used by FS_Meteo, at present after
	re-naming to "Runways.rws".
	
Runways.csv
	This is a comma-separated text database of all the runways, sorted, 
	and it is in the particular format used by Radar Contact (all versions
	up until the change to the following:
	
R4.csv
	This is the same as the previous CSV file, but includes additional
	runway data added at the end of each line. This is used 
	in Radar Contact 4 (before RC4.3) for more precise ATC operations.
	
R5.csv
	This is the same as R4.csv but with runway closure indicators in an
	extra two fields - CL closed for landing and/or CT closed for
	take-off. This file is used by RCV4.3 and RC5.
	
R5.bin
   This is a binary version of the R5.csv file, with some extra fields.
   Details of the format are given below.
	
G5.csv
	This contains information about Gates, Ramps, Parking places, along
	with airline lists where specified. It is used by later versions of
	Radar contact.
	
T5.csv
	This contains details of all taxi paths.It is used by later versions of
	Radar contact.
	
T5.bin
   This is a binary version of the T5.csv file.
   Details of the format are given below.
	
F5.csv	
	This lists all COM frequencies and associated names, including the
	airport name, for Radar Contact 5 or later.
	
F4.csv	
	This lists all COM frequencies and associated names, including the
	airport name, for Radar Contact 4. If you want to use this with RC4
	you must copy it to the Radar Contact 'data' subfolder. Make a safe
	copy of your original F4.CSV file first (I usually just rename it
	to "f4orig.csv").
	
	Note that MakeRwys matches frequences as best it can, with the
	following extra "fiddles" to ensure a good range of frequencies:
	
	1. Clearance Delivery is preferably met by FS type 14 "CD", but else is
	   met by type 7 "CLEARANCE".
	   
	2. Multicom is met by FS type 2 "MULTICOM", but failing that by type 4
	   "CTAF".
	   
	3. Approach is from FS type 8 "APPROACH", but failing that from a 
	   second Type 9 "DEPARTURE" if there are more than one of the latter.
	
	4. Deparature is from FS type 9 "DEPARTURE", but failing that from a 
	   second Type 8 "ARRIVAL" if there are more than one of the latter.
	   
	Centre, FSS, AWOS and ASOS frequencies aren't used for this file.
	
Helipads.csv
  This is a list of all the Helipads found, sorted into ascending ICAO order.
  Details of the fields are given below.
	
		
=========================================================
IMPORTANT NOTE ABOUT RUNWAY INCLUSION

Runways are only included in any of the files (except for being logged
in the Runways.txt file) if they are equipped with defined threasholds,
or "start positions". Without those they may as well be omitted because
the files then won't serve their purpose of assisting programs locate
and land aircraft.

The whole data structure within MakeRunways, from which it prduces
these files, is predicated on this assumption.

=========================================================
MINIMUM RUNWAY LENGTH

By default MakeRunways imposes a minimum runway legth of 1500 feet,
otherwise runwaysc are omitted from the data files. This is to
eliminate so-called "ghost" runways being included -- very small
runways provided only to allow AI Traffic to be directed better for
landings and takeoffs.

If necessary you can override this value. Just use a command line
parameter in the form:

    />n
    
where n gives the nmuber of feet to be considered the miaximum for
exclusion. Take care not to make this too small for fear of
including those "ghosts", but if you really do want to see all,
you can set />0.

		
=========================================================
WATER RUNWAYS

The runway lists will normally not contain any water runways.
If you need these included just add this command line parameter:

			/Water
			
If you want files containing ONLY water runways use

			/WaterOnly

=========================================================
UPDATING RADAR CONTACT's TRANSITION ALTITUDES FILE

If you are a user of Aivlasoft's EFB program for FSX/P3D, then part of
the updated data you will have is a file with the correct Transition
Altitudes for most airports in the world. MakeRunways can now 
automatically use that file to update Radar Contact's list too.

In order to do this, you must copy RC's "M4.CSV" file into the root
FS folder, next to MakeRunways. Don't worry about EFB -- if that is 
correctly installed then MakeRunways will find it.

To tell MakeRunways to update M4.CSV add a command line parameter:

/+T

(T for Transition Altitude).

When MakeRunways has finished simply copy the M4.CSV file back into
RC's Data folder.

=========================================================
GATES FOR AIRLINE FILE

There is also an optional file, listing all the Gates for a specifically
selected airline. To obtain this for your preferred airline, run
MakeRwys.rexe with a command line parameter such as (e.g.)
	
	/BAW
	
for the airline code BAW (British Airways).

If you are also using the />n and/or /+T parameters, those must come first.
	
The gates are listed in sorted order of Airport, but not of Gate name or
number -- the Gates are in their scenery file order. The file produced will
be an ordinary text file named <airline code> Gates.txt. e.g.

  BAW Gates.txt
  
in the above case.

=================	
	
After running the program to completion, just copy the files to their 
respective program's folders.

Have fun!

=========================================================
VALUES PROVIDED IN EACH CSV FILE:
=========================================================

RUNWAYS.CSV (ORIGINAL RUNWAYS FILE -- for very old versions of RC):

   ICAO, Rwy*, Latitude*, Longitude*, Altitude*, HeadingMag, Length*, ILSfreq*

R4.CSV (RUNWAYS FILE for RCV4, until 4.3):

   ICAO, Rwy*, Latitude*, Longitude*, Altitude*, HeadingMag, Length*, ILSfreq*, Width*, MagVar, CentreLatitude, CentreLongitude, ThresholdOffset*

R5.CSV (RUNWAYS FILE for RCV5):

   ICAO, Rwy*, Latitude*, Longitude*, Altitude*, HeadingMag, Length*, ILSfreqFlags*, Width*, MagVar, CentreLatitude, CentreLongitude, ThresholdOffset*,Status*
   
R5.BIN see below

RUNWAYS.XML (Airports and runways for "It's Your Plane")
    
   For each Airport: ICAO id, ICAOName, City, Longitude, Latitude, Altitude (feet), MagVar
   For each Airport: ICAO id, ICAOName, City, Longitude, Latitude, Altitude (feet), MagVar
   And, within each Airport section, for each runway:
     Runway id, Len (feet), Hdg (Magnetic: add MagVar for True), Def (surface), ILSFreq (nnn.nn), ILSHdg (Mag),
     ILSid, ILSslope, Lat and Lon of threshold/start, Lat and Lon of FS's "Startt" point (which may be to the
     side of the runway), ClosedLanding (TRUE or FALSE) and ClosedTakeoff (TRUE or FALSE), EndLights (NONE, LOW, MEDIUM or HIGH),
     CenterLights (NONE, LOW, MEDIUM or HIGH), CenterRed (TRUE or FALSE),
 	Added 4.699:
		 Threshold offset, ILS name (first 31 chars only), VASI lights, VASI values (X/Z/Spacing/Pitch), Approach lights,
		 attern direction and altitude, source BGL filepath, Scenery layer title.
		 
G5.CSV (GATES for RCV5):

  ICAO, GateName*, GateNumber, Latitude, Longitude, Radius*, HeadingTrue, GateType*, AirlineCodeList ...
  
F5.CSV (COMMS FREQUENCIES for RCV5)

  ICAO, CommsType*, Frequency, "name of facility"
	  (with an entry for Airport Name with CommsType=0 and Frequency=0)

F4.CSV (COMMS FREQUENCIES for RCV4)

	ICAO, Airport Name, ATISfreq, CDfreq, GroundFreq, TowerFreq, UnicomFreq, MulticomFreq, ApproachFreq, DepartureFreq 

T5.CSV (TAXIWAYS for RCV5):

  ICAO,TaxiwayName,MinimumWidthMetres,PointList ...
  
T5.BIM see below

HELIPADS.CSV

  ICAO, Latitude, Longitude, Altitude(ft), HeadingTrue, Length(ft), Width(ft), SurfaceType*, Flags*
	
	Surface type is a number 0-23, see list below. 'Flags' is a numerical value made up of one of these "types":
	  NONE		0
    H			  1
    SQUARE	2
    CIRCLE	3
    MEDICAL	4
   plus optionally 16 for "Transparent" and/or 32 for "Closed". 

  SURFACE TYPES:
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

  
--------------------------------------------------------------------------------

where PointList is a sequence of: Latitude,Longitude,TaxiType*,WidthMetres

* Rwy is nnn for runway number, then 0, 1=L, 2=R, 3=C, 4=W (water)
  Note that runways designated N, NE, etc are denoted by runway
  numbers over 36, as follows:
	37 =  N-S
	38 =  E-W
	39 =  NW-SE
	40 =  SW-NE
	41 =  S-N
	42 =  W-E
	43 =  SE-NW
	44 =  NE-SW
	45 =  N
	46 =  W
	47 =  NW
	48 =  SW
	49 =  S
	50 =  E
	51 =  SE
	52 =  NE
* Latitude is of threshold or AFCAD's "runway start"
* Longitude is of threshold or AFCAD's "runway start"
* Altitude in feet
* Length in feet
* ILSFreq as nnnnn for nnn.nn, or just 0 when no ILS
* ILSFreqFlags is ILSFreq then optional B (backcourse), D (DME) G (Glideslope) equipped
* Width in feet
* ThresholdOffset in feet
* Status: ,CT for Closed for Takeoff and/or ,CL for Closed for Landing
* Radius in metres

* TaxiType is 
	0 = unknown
	1 = Normal
	2 = Hold short
	3 = unknown
	4 = ILS hold short
	5 = Gate/Park

* GateName is either omitted or one of
	Dock
	Park
	PkN
	PkNE
	PkE
	PkSE
	PkS
	PkSW
	PkW
	PkNW

* GateType is:
	0 = none
	1 = Ramp GA
	2 = Ramp GA Small
	3 = Ramp GA Medium
	4 = Ramp GA Large
	5 = Ramp Cargo
	6 = Ramp Military Cargo
	7 = Ramp Military Combat
	8 = Gate Small
	9 = Gate Medium
	10 = Gate Large
	11 = Dock GA

* CommsType is:
	0 = Special entry with airport name, zero frequency
	1 = ATIS
	2 = MULTICOM
	3 = UNICOM
	4 = CTAF
	5 = GROUND
	6 = TOWER
	7 = CLEARANCE
	8 = APPROACH
	9 = DEPARTURE
	10 = CENTRE
	11 = FSS
	12 = AWOS
	13 = ASOS
	14 = CLEARANCE PRE-TAXI
	15 = REMOTE CLEARANCE DELIVERY
	
--------------------------------------------------------------------------------

R5.BIN:  binary format Runways file
Record format:
 
struct {
    char chICAO[4];
    unsigned short wRwyNum;
    char chDesig; // L, C, R or space
    char chStatus[3]; // CT, CL, CTL or all zero
    unsigned short wSurface; // 0-23 as in New BGL format
    float fLatThresh;
    float fLongThresh;
    float fLatCentre;
    float fLongCentre;
    float fAltitude; // feet
    float fThrOffset; // feet
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
 
--------------------------------------------------------------------------------

T5.BIN:  binary format Taxiways file
Record format:
 
struct {
    DWORD dwNumPts;
    char chICAO[4];
    char chName[8]; // zero if not named
    float fMinWidth;
} tbin;
 
followed by dwNumPts x
 
struct
    float fLat;
    float fLon;
    float fType;
    float fWidth; // to next point, =0 for last point
} tpt;
 
=========================================================
RUNNING MAKERUNWAYS SILENTLY
=========================================================

If an application wishes MakeRunways to run without any
progress dialogue then it can start it with the
command line parameter

     /+Q
     
=========================================================
Pete Dowson, 23rd September 2016
=========================================================
