#pragma once




////////////////////////////////////////////////////////////////////////////////
//constants

const double pi = 3.14159265358979323846;
const double pi2 = 2. * pi;
const double pi_half = 1.57079632679489661923;
const double deg2rad = pi / 180.;
const double rad2deg = 180. / pi;
const double mas_to_rad = 1. / 1000. / 3600. * deg2rad;
const double arcsec2rad = pi / (180. * 3600.);
const double rad2arcsec = (3600. * 180.) / pi;
const double AU_to_m = 149597870700.;//1 AU [m]
const double ms2m = 1. / 149896.2293;//two-way milliseconds to 1-way meter; Light travels 300,000 meters in 1 millisecond.
const double C_m_per_s = 299792458.6;//speed of light m/s
const double C_m_per_ms = 299792.4586;
const double C_m_per_us = 299.7924586;
const double C_m_per_ns = 0.2997924586;
const double C_m_per_ps = 0.0002997924586;
const double standard_epoch_GPS_JD = 2444244.5;//GPS standard epoch, JD = 2,444,244.5 (January 6th, 1980, 00:00 UTC)
const double standard_epoch_J2000 = 2451545.0;//Standard epoch J2000.0, JD = 2,451,545.0 (January 1st, 2000, 12:00 UTC)
const double JDmMJD_offset = 2400000.5;
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//types

typedef struct { double v[3]; } d3;
typedef struct { double v[3][3]; } d33;//v[row][column], v[i=row][j=col]
typedef struct { double az, el; } dae;
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//operators
d3 operator * (const d3& V, const double& C)
{
	d3 out;
	out.v[0] = V.v[0] * C;
	out.v[1] = V.v[1] * C;
	out.v[2] = V.v[2] * C;
	return out;
};

d3 operator * (const d33& M, const d3& V)//matrix vector product
{
	d3 out;
	out.v[0] = M.v[0][0] * V.v[0] + M.v[0][1] * V.v[1] + M.v[0][2] * V.v[2];
	out.v[1] = M.v[1][0] * V.v[0] + M.v[1][1] * V.v[1] + M.v[1][2] * V.v[2];
	out.v[2] = M.v[2][0] * V.v[0] + M.v[2][1] * V.v[1] + M.v[2][2] * V.v[2];
	return out;
};

d33 operator * (const d33& L, const d33& R)//left matrix, right matrix product
{
	d33 M;//out matrix

	M.v[0][0] = L.v[0][0] * R.v[0][0] + L.v[0][1] * R.v[1][0] + L.v[0][2] * R.v[2][0];
	M.v[0][1] = L.v[0][0] * R.v[0][1] + L.v[0][1] * R.v[1][1] + L.v[0][2] * R.v[2][1];
	M.v[0][2] = L.v[0][0] * R.v[0][2] + L.v[0][1] * R.v[1][2] + L.v[0][2] * R.v[2][2];

	M.v[1][0] = L.v[1][0] * R.v[0][0] + L.v[1][1] * R.v[1][0] + L.v[1][2] * R.v[2][0];
	M.v[1][1] = L.v[1][0] * R.v[0][1] + L.v[1][1] * R.v[1][1] + L.v[1][2] * R.v[2][1];
	M.v[1][2] = L.v[1][0] * R.v[0][2] + L.v[1][1] * R.v[1][2] + L.v[1][2] * R.v[2][2];

	M.v[2][0] = L.v[2][0] * R.v[0][0] + L.v[2][1] * R.v[1][0] + L.v[2][2] * R.v[2][0];
	M.v[2][1] = L.v[2][0] * R.v[0][1] + L.v[2][1] * R.v[1][1] + L.v[2][2] * R.v[2][1];
	M.v[2][2] = L.v[2][0] * R.v[0][2] + L.v[2][1] * R.v[1][2] + L.v[2][2] * R.v[2][2];

	return M;
};
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//structures

typedef struct
{
	//https://www.celestrak.com/NORAD/documentation/tle-fmt.php
	int NORAD;
	double MJD;
	int Launch_YYYY;//10-11 International Designator(Last two digits of launch year)
	int Launch_No;//12-14 International Designator(Launch number of the year)
	std::string International_Designator;//COSPAR
	char Name[100];
	char Line0[100];
	char Line1[100];
	char Line2[100];
} str_TLE_record;

typedef struct
{
	int ID;//some station ID, Code...
	std::string Name;//Site name
	d3 ITRFm;
	double Em;//E=Elevation
	dae ENrad;//lon lat rad
} XYZELoLa;

typedef struct iau80data
{
	int    iar80[107][6];
	double rar80[107][5];
} iau80data;
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//functions

void TLE_import_https(
	char* https_path,// "https://celestrak.com/NORAD/elements/supplemental/starlink.txt"
	std::vector <str_TLE_record>* TLE
);

void format_TLE_line(char line[]);
void TLE_get_parameters(str_TLE_record* out, char* L0, char* L1, char* L2);
void clear(str_TLE_record* s);

double MJD_DoY1_d(
	int year,//2006
	double doy1//day of year starting from 1, Jan 1=1doy
);

void date_from_year_and_doy1(//doy1=day of year starting from 1, Jan 1=1doy
	int year,
	int doy1,
	int* month,
	int* day
);

double MJD(int year, int month, int day, int hour, int minute, double second);//year:2006
int MJD(int year, int month, int day);//year:2006

std::string to_lower(std::string str);
void download_starlink_TLE(std::vector <str_TLE_record>* TLE);
long double present_MJD_UTC();

void give_present_time_UTC(
	unsigned short* YearNow,
	unsigned short* MonthNow,
	unsigned short* DayNow,
	unsigned short* HourNow,
	unsigned short* MinNow,
	unsigned short* SecNow,
	unsigned short* mSecNow
);

void iau80in(iau80data& iau80rec);
std::string timestamp_UTC(double MJD);
std::string timestamp_UTC_1s(double MJD);
std::string timestamp_UTC_1ms(double MJD);

void date_and_time_from_MJD(
	double MJD,
	int* year,
	int* month,
	int* day,
	int* hour,
	int* minute,
	int* isec,
	double* sec_fraction
);

////////////////////////////////////////////////////////////////////////////////
//classes

class eph_tle
{
	//#define SGP4Version  "SGP4 Version 2016-03-09"

	/*     ----------------------------------------------------------------
*
*                                 SGP4.h
*
*    this file contains the sgp4 procedures for analytical propagation
*    of a satellite. the code was originally released in the 1980 and 1986
*    spacetrack papers. a detailed discussion of the theory and history
*    may be found in the 2006 aiaa paper by vallado, crawford, hujsak,
*    and kelso.
*
*    current :
*               7 dec 15  david vallado
*                           fix jd, jdfrac
*    changes :
*               3 nov 14  david vallado
*                           update to msvs2013 c++
*              30 Dec 11  david vallado
*                           consolidate updated code version
*              30 Aug 10  david vallado
*                           delete unused variables in initl
*                           replace pow inetger 2, 3 with multiplies for speed
*               3 Nov 08  david vallado
*                           put returns in for error codes
*              29 sep 08  david vallado
*                           fix atime for faster operation in dspace
*                           add operationmode for afspc (a) or improved (i)
*                           performance mode
*              20 apr 07  david vallado
*                           misc fixes for constants
*              11 aug 06  david vallado
*                           chg lyddane choice back to strn3, constants, misc doc
*              15 dec 05  david vallado
*                           misc fixes
*              26 jul 05  david vallado
*                           fixes for paper
*                           note that each fix is preceded by a
*                           comment with "sgp4fix" and an explanation of
*                           what was changed
*              10 aug 04  david vallado
*                           2nd printing baseline working
*              14 may 01  david vallado
*                           2nd edition baseline
*                     80  norad
*                           original baseline
*       ----------------------------------------------------------------      */

/*     ----------------------------------------------------------------
*
*                               sgp4unit.cpp
*
*    this file contains the sgp4 procedures for analytical propagation
*    of a satellite. the code was originally released in the 1980 and 1986
*    spacetrack papers. a detailed discussion of the theory and history
*    may be found in the 2006 aiaa paper by vallado, crawford, hujsak,
*    and kelso.
*
*                            companion code for
*               fundamentals of astrodynamics and applications
*                                    2013
*                              by david vallado
*
*     (w) 719-573-2600, email dvallado@agi.com, davallado@gmail.com
*
*    current :
*               7 dec 15  david vallado
*                           fix jd, jdfrac
*    changes :
*               3 nov 14  david vallado
*                           update to msvs2013 c++
*              30 aug 10  david vallado
*                           delete unused variables in initl
*                           replace pow integer 2, 3 with multiplies for speed
*               3 nov 08  david vallado
*                           put returns in for error codes
*              29 sep 08  david vallado
*                           fix atime for faster operation in dspace
*                           add operationmode for afspc (a) or improved (i)
*                           performance mode
*              16 jun 08  david vallado
*                           update small eccentricity check
*              16 nov 07  david vallado
*                           misc fixes for better compliance
*              20 apr 07  david vallado
*                           misc fixes for constants
*              11 aug 06  david vallado
*                           chg lyddane choice back to strn3, constants, misc doc
*              15 dec 05  david vallado
*                           misc fixes
*              26 jul 05  david vallado
*                           fixes for paper
*                           note that each fix is preceded by a
*                           comment with "sgp4fix" and an explanation of
*                           what was changed
*              10 aug 04  david vallado
*                           2nd printing baseline working
*              14 may 01  david vallado
*                           2nd edition baseline
*                     80  norad
*                           original baseline
*       ----------------------------------------------------------------      */

	typedef enum
	{
		wgs72old,
		wgs72,
		wgs84
	} gravconsttype;

	typedef enum eOpt
	{
		e80,
		e96,
		e00a,
		e00b
	} eOpt;

	typedef struct elsetrec
	{
		long int satnum;
		int  epochyr, epochtynumrev;
		int error;
		char operationmode;
		bool deep_space;
		bool initialize;

		//Near Earth
		int isimp;
		double aycof, con41, cc1, cc4, cc5, d2, d3, d4, delmo, eta, argpdot, omgcof, sinmao, t, t2cof, t3cof,
			t4cof, t5cof, x1mth2, x7thm1, mdot, nodedot, xlcof, xmcof, nodecf;

		//Deep Space
		int irez;
		double d2201, d2211, d3210, d3222, d4410, d4422, d5220, d5232,
			d5421, d5433, dedt, del1, del2, del3, didt, dmdt,
			dnodt, domdt, e3, ee2, peo, pgho, pho, pinco,
			plo, se2, se3, sgh2, sgh3, sgh4, sh2, sh3,
			si2, si3, sl2, sl3, sl4, gsto, xfact, xgh2,
			xgh3, xgh4, xh2, xh3, xi2, xi3, xl2, xl3,
			xl4, xlamo, zmol, zmos, atime, xli, xni;

		double a, altp, alta, epochdays, jdsatepoch, jdsatepochF, nddot, ndot, bstar, rcse, inclo, nodeo, ecco, argpo, mo, no_kozai;
		//sgp4fix add new variables from tle
		char classification, intldesg[11];
		int ephtype;
		long elnum, revnum;
		//sgp4fix add unkozai'd variable
		double no_unkozai;
		//sgp4fix add singly averaged variables
		double am, em, im, Om, om, mm, nm;
		//sgp4fix add constant parameters to eliminate mutliple calls during execution
		double tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2;

		//Additional elements to capture relevant TLE and object information:       
		long dia_mm; // RSO dia in mm
		double period_sec; // Period in seconds
		unsigned char active; // "Active S/C" flag (0=n, 1=y) 
		unsigned char not_orbital; // "Orbiting S/C" flag (0=n, 1=y)  
		double rcs_m2; // "RCS (m^2)" storage  

	} elsetrec;

public:
	//The time is based on UTC and the coordinate system should be considered a true-equator, mean equinox system.
	eph_tle();
	~eph_tle();

	void initialize(std::string L0, std::string L1, std::string L2);
	void state_ECI_J2000(double MJD_UTC, d3* r_ECI_J2000_m, d3* v_ECI_J2000_mps);
	int get_NORAD();


	//endd public

private:

	elsetrec satrec;
	iau80data iau80rec;
	d33 SGP4_TEME_to_J2000;

	bool sgp4init
	(
		gravconsttype whichconst, char opsmode, const int satn, const double epoch,
		const double xbstar, const double xndot, const double xnddot, const double xecco, const double xargpo,
		const double xinclo, const double xmo, const double xno,
		const double xnodeo, elsetrec& satrec
	);

	bool sgp4
	(
		// no longer need gravconsttype whichconst, all data contained in satrec
		elsetrec& satrec, double tsince,
		double r[3], double v[3]
	);

	void getgravconst
	(
		gravconsttype whichconst,
		double& tumin,
		double& mu,
		double& radiusearthkm,
		double& xke,
		double& j2,
		double& j3,
		double& j4,
		double& j3oj2
	);

	// older sgp4io methods
	void twoline2rv
	(
		char      longstr1[130], char longstr2[130],
		char opsmode,
		gravconsttype       whichconst,
		elsetrec& satrec
	);

	// older sgp4ext methods
	double gstime(double jdut1);
	double sgn(double x);
	double mag(double x[3]);
	void cross(double vec1[3], double vec2[3], double outvec[3]);
	double dot(double x[3], double y[3]);
	double angle(double vec1[3], double vec2[3]);
	void newtonnu(double ecc, double nu, double& e0, double& m);
	double asinh(double xval);
	void rv2coe(double r[3], double v[3], const double mu, double& p, double& a, double& ecc, double& incl, double& omega, double& argp, double& nu, double& m, double& arglat, double& truelon, double& lonper);
	void jday(int year, int mon, int day, int hr, int minute, double sec, double& jd, double& jdFrac);
	void days2mdhms(int year, double days, int& mon, int& day, int& hr, int& minute, double& sec);
	void invjday(double jd, double jdFrac, int& year, int& mon, int& day, int& hr, int& minute, double& sec);

	/* ----------- local functions - only ever used internally by sgp4 ---------- */
	void dpper
	(
		double e3, double ee2, double peo, double pgho, double pho,
		double pinco, double plo, double se2, double se3, double sgh2,
		double sgh3, double sgh4, double sh2, double sh3, double si2,
		double si3, double sl2, double sl3, double sl4, double t,
		double xgh2, double xgh3, double xgh4, double xh2, double xh3,
		double xi2, double xi3, double xl2, double xl3, double xl4,
		double zmol, double zmos, double inclo,
		bool initialize,
		double& ep, double& inclp, double& nodep, double& argpp, double& mp,
		char opsmode
	);

	void dscom
	(
		double epoch, double ep, double argpp, double tc, double inclp,
		double nodep, double np,
		double& snodm, double& cnodm, double& sinim, double& cosim, double& sinomm,
		double& cosomm, double& day, double& e3, double& ee2, double& em,
		double& emsq, double& gam, double& peo, double& pgho, double& pho,
		double& pinco, double& plo, double& rtemsq, double& se2, double& se3,
		double& sgh2, double& sgh3, double& sgh4, double& sh2, double& sh3,
		double& si2, double& si3, double& sl2, double& sl3, double& sl4,
		double& s1, double& s2, double& s3, double& s4, double& s5,
		double& s6, double& s7, double& ss1, double& ss2, double& ss3,
		double& ss4, double& ss5, double& ss6, double& ss7, double& sz1,
		double& sz2, double& sz3, double& sz11, double& sz12, double& sz13,
		double& sz21, double& sz22, double& sz23, double& sz31, double& sz32,
		double& sz33, double& xgh2, double& xgh3, double& xgh4, double& xh2,
		double& xh3, double& xi2, double& xi3, double& xl2, double& xl3,
		double& xl4, double& nm, double& z1, double& z2, double& z3,
		double& z11, double& z12, double& z13, double& z21, double& z22,
		double& z23, double& z31, double& z32, double& z33, double& zmol,
		double& zmos
	);

	void dsinit
	(
		//sgp4fix no longer needed pass in xke
		//gravconsttype whichconst,
		double xke,
		double cosim, double emsq, double argpo, double s1, double s2,
		double s3, double s4, double s5, double sinim, double ss1,
		double ss2, double ss3, double ss4, double ss5, double sz1,
		double sz3, double sz11, double sz13, double sz21, double sz23,
		double sz31, double sz33, double t, double tc, double gsto,
		double mo, double mdot, double no, double nodeo, double nodedot,
		double xpidot, double z1, double z3, double z11, double z13,
		double z21, double z23, double z31, double z33, double ecco,
		double eccsq, double& em, double& argpm, double& inclm, double& mm,
		double& nm, double& nodem,
		int& irez,
		double& atime, double& d2201, double& d2211, double& d3210, double& d3222,
		double& d4410, double& d4422, double& d5220, double& d5232, double& d5421,
		double& d5433, double& dedt, double& didt, double& dmdt, double& dndt,
		double& dnodt, double& domdt, double& del1, double& del2, double& del3,
		double& xfact, double& xlamo, double& xli, double& xni
	);

	void dspace
	(
		int irez,
		double d2201, double d2211, double d3210, double d3222, double d4410,
		double d4422, double d5220, double d5232, double d5421, double d5433,
		double dedt, double del1, double del2, double del3, double didt,
		double dmdt, double dnodt, double domdt, double argpo, double argpdot,
		double t, double tc, double gsto, double xfact, double xlamo,
		double no,
		double& atime, double& em, double& argpm, double& inclm, double& xli,
		double& mm, double& xni, double& nodem, double& dndt, double& nm
	);

	void initl
	(
		// sgp4fix satn not needed. include in satrec in case needed later  
		// int satn,      
		// sgp4fix just pass in xke and j2
		// gravconsttype whichconst, 
		double xke, double j2,
		double ecco, double epoch, double inclo, double no_kozai, char opsmode,
		bool& deep_space, double& ainv, double& ao, double& con41, double& con42, double& cosio,
		double& cosio2, double& eccsq, double& omeosq, double& posq,
		double& rp, double& rteosq, double& sinio, double& gsto, double& no_unkozai
	);

	d33 TEME_to_J2000(iau80data& iau80rec, double MJD_UTC, int order, int eqeterms, char optteme);
	d33 precess(double ttt, eOpt opt, double& psia, double& wa, double& epsa, double& chia);
	d33 nutation(double ttt, double ddpsi, double ddeps, iau80data& iau80rec, char nutopt, double& deltapsi, double& deltaeps, double& trueeps, double& meaneps, double& omega);
	d33 truemean(double ttt, int order, int eqeterms, char opt, iau80data& iau80rec);
	void fundarg(
		double ttt, eOpt opt,
		double& l, double& l1, double& f, double& d, double& omega,
		double& lonmer, double& lonven, double& lonear, double& lonmar,
		double& lonjup, double& lonsat, double& lonurn, double& lonnep,
		double& precrate
	);






	//endd private
};


