
////////////////////////////////////////////////////////////////////////////////////////////////
// SatCon Prediction Model
// 
// This code is for educational purposes only. 
// This software project is registered at the University of Texas at Austin 
// and is released under permissive MIT license.
// 
// Use or distribution of the dependencies of this project may be limited for non-educational purposes. 
//
// Project developers and contributors:
// Daniel Kucharski, since July 1, 2021, allsky@utexas.edu
//
// 
////////////////////////////////////////////////////////////////////////////////////////////////


//Ports
#include <afxwin.h>//MFC core and standard components
#include <afxext.h>//MFC extensions
#include <afxinet.h>
#define DL_BUFFER_SIZE 4096

#include <iostream>
#include <limits>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <limits.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <sys/types.h>
#include <io.h>
#include <istream>
#include <ostream>
#include <vector>

#define MAXBUF 200
#define MAX_PATH 260

#include "satcon.h"



int main(int argc, const char* argv[])
{
	printf("\nSatCon Prediction Model v0.01\n");

	std::vector <str_TLE_record> TLE;
	download_starlink_TLE(&TLE);//download Starlink prediction from Calestrak.com

	//satellite state vector prediction
	double MJD_UTC_start = present_MJD_UTC();//prediction epoch start
	double MJD_UTC_stop = MJD_UTC_start + 1.;//prediction epoch stop
	int step_s = 60;//prediction epoch step
	int step_hmn = int((MJD_UTC_stop - MJD_UTC_start) * 86400.) / step_s + 1;


	printf("\nPredicting state vectors\n");

	//predict satellite state vector 
	//and save results to output file
	std::string out_path = "prediction.csv";
	FILE* FFout = nullptr;
	errno_t err = fopen_s(&FFout, out_path.c_str(), "w");

	//add header
	char line_txt[500];
	sprintf_s(line_txt, "%s", "");
	sprintf_s(line_txt, "%s", "Timestamp_UTC,MJD,SOD,NORAD,Sat_pos_ECI_J2000_X_m,Sat_pos_ECI_J2000_Y_m,Sat_pos_ECI_J2000_Z_m,Sat_vel_ECI_J2000_X_mps,Sat_vel_ECI_J2000_Y_mps,Sat_vel_ECI_J2000_Z_mps\n");

	if (!err && FFout != NULL)
		err = fprintf_s(FFout, line_txt);

	fclose(FFout);

	for (int iSat = 0; iSat < TLE.size(); iSat++)
	//for (int iSat = 0; iSat < 1; iSat++)
	{		
		err = fopen_s(&FFout, out_path.c_str(), "a");
		printf("predicting satellite %d/%d, %d %s \n", iSat+1, TLE.size(), TLE[iSat].NORAD, TLE[iSat].Name);

		eph_tle sat_ephemeris;
		sat_ephemeris.initialize(TLE[iSat].Line0, TLE[iSat].Line1, TLE[iSat].Line2);
		int NORAD = sat_ephemeris.get_NORAD();

		for (int iStep = 0; iStep < step_hmn; iStep++)
		{
			double MJD_UTC = MJD_UTC_start + double(iStep * step_s) / 86400.;

			//get sat inertial state vector
			d3 sat_ECI_J2000_m, sat_ECI_J2000_mps;
			sat_ephemeris.state_ECI_J2000(MJD_UTC, &sat_ECI_J2000_m, &sat_ECI_J2000_mps);

			//format and store record
			int iMJD = int(MJD_UTC);
			double SOD = (MJD_UTC - double(iMJD)) * 86400.;
			
			//std::string timestamp = timestamp_UTC(MJD_UTC);
			std::string timestamp = timestamp_UTC_1s(MJD_UTC);
			//std::string timestamp = timestamp_UTC_1ms(MJD_UTC);
			
			sprintf_s(line_txt, "%s", "");
			//sprintf_s(line_txt, "%s,%d,%.3lf,%d,%20.3lf,%20.3lf,%20.3lf,%20.3lf,%20.3lf,%20.3lf\n",
			sprintf_s(line_txt, "%s,%d,%.3lf,%d,%20.0lf,%20.0lf,%20.0lf,%20.3lf,%20.3lf,%20.3lf\n",
				timestamp.c_str(),
				iMJD,
				SOD,
				NORAD,
				sat_ECI_J2000_m.v[0],
				sat_ECI_J2000_m.v[1],
				sat_ECI_J2000_m.v[2],
				sat_ECI_J2000_mps.v[0],
				sat_ECI_J2000_mps.v[1],
				sat_ECI_J2000_mps.v[2]
			);

			err = fprintf_s(FFout, line_txt);
		}
		fclose(FFout);
	}	
	
	printf("\ndone!\n\n");

	TLE.resize(0);

	system("pause");//press any key to exit type of stuff
	return 0;
}

//------------------------------------------------------------------------------
void TLE_import_https(
	char* https_path,// "https://celestrak.com/NORAD/elements/supplemental/starlink.txt"
	std::vector <str_TLE_record>* TLE
)
{
	(*TLE).resize(0);

	CString url = https_path;
	CString server, object;
	INTERNET_PORT port;
	DWORD serviceType;
	::AfxParseURL(url, serviceType, server, object, port);
	CInternetSession sess(server, 1, INTERNET_OPEN_TYPE_PRECONFIG);
	CHttpConnection* http = (CHttpConnection*)NULL;
	http = sess.GetHttpConnection(server, port, _T(""), _T(""));
	DWORD m_dwFlags = INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

	//get data
	UINT  cbData2;
	CString  szResult[2];
	CHttpFile* pHttpFile2;

	BYTE  nData2[65536];
	LPCTSTR pszQuery = object;

	for (pHttpFile2 = (CHttpFile*)NULL; http != (CHttpConnection*)NULL; )
	{
		try
		{
			if ((pHttpFile2 = http->OpenRequest(CHttpConnection::HTTP_VERB_GET, pszQuery, (LPCTSTR)NULL, 1, (LPCTSTR*)NULL, (LPCTSTR)NULL, m_dwFlags)))
			{
				for (pHttpFile2->SendRequest(), ZeroMemory(nData2, sizeof(nData2) / sizeof(BYTE)); (cbData2 = pHttpFile2->Read(nData2, sizeof(nData2) / sizeof(BYTE) - 1)) > 0; ZeroMemory(nData2, sizeof(nData2) / sizeof(BYTE)))
				{
					MultiByteToWideChar(CP_ACP, 0, (LPCSTR)nData2, cbData2, szResult[1].GetBufferSetLength(cbData2), cbData2 + 1);
					szResult[1].ReleaseBuffer();
					szResult[0] += szResult[1];
				}
				pHttpFile2->Close();
				delete pHttpFile2;
			}
			break;
		}
		catch (...)
		{
			if (pHttpFile2 != (CHttpFile*)NULL)
			{
				pHttpFile2->Close();
				delete pHttpFile2;
			}
			szResult[0].Empty();
			break;
		}
	}

	http->Close();
	http = (CHttpConnection*)NULL;
	delete http;
	sess.Close();

	if (szResult[0].GetLength() > 100)
	{
		char buf[MAXBUF];
		char Line[MAX_PATH], L0[MAX_PATH], L1[MAX_PATH], L2[MAX_PATH];//3LE

		sprintf_s(buf, "%s", "");
		sprintf_s(Line, "%s", "");
		sprintf_s(L0, "%s", "");
		sprintf_s(L1, "%s", "");
		sprintf_s(L2, "%s", "");

		CT2CA pszConvertedAnsiString(szResult[0]);
		std::string data_str(pszConvertedAnsiString);
		std::istringstream data_stream(data_str);
		std::string line;

		while (std::getline(data_stream, line))
		{
			sprintf_s(Line, "%s", "");
			sprintf_s(Line, "%s", line.c_str());

			// strip trailing '\n' if it exists
			int length = strlen(Line) - 1;
			if (Line[length] == '\n')
				Line[length] = 0;

			if (length > 1)
			{
				if (!memcmp(Line, "0 ", 2))
				{
					sprintf_s(L0, "%s", Line);
				}
				else
					if (!memcmp(Line, "1 ", 2))
					{
						sprintf_s(L1, "%s", Line);
					}
					else if (!memcmp(Line, "2 ", 2))
					{
						sprintf_s(L2, "%s", Line);

						int L1_length = strlen(L1);
						int L2_length = strlen(L2);

						if ((L1_length >= 68) && (L2_length >= 68))
						{
							format_TLE_line(L0);
							format_TLE_line(L1);
							format_TLE_line(L2);

							str_TLE_record set;
							TLE_get_parameters(&set, L0, L1, L2);

							if (set.MJD > 30000.)
								(*TLE).push_back(set);
						}

						sprintf_s(buf, "%s", "");
						sprintf_s(Line, "%s", "");
						sprintf_s(L0, "%s", "");
						sprintf_s(L1, "%s", "");
						sprintf_s(L2, "%s", "");
					}
					else
					{
						sprintf_s(L0, "0 %s", Line);
					}
			}

			sprintf_s(Line, "%s", "");
		}
	}

	szResult[0].Empty();
}
//------------------------------------------------------------------------------
void format_TLE_line(char line[])
{
	char c;
	unsigned int i = 0;

	if (line[0] == char('0'))
	{

		while (line[i])
		{
			c = line[i];
			line[i] = tolower(c);

			//replace bad characters
			if ((line[i] == char('/')) ||
				(line[i] == char(' ')) ||
				(line[i] == char('(')) ||
				(line[i] == char(')')) ||
				(line[i] == char('\r')))
				line[i] = char('_');

			i++;
		}
	}
	else if ((line[0] == char('1')) || (line[0] == char('2')))
	{
		while (line[i])
		{
			c = line[i];
			line[i] = tolower(c);

			//replace bad characters
			if (line[i] == char('\r'))
				line[i] = char('_');

			i++;
		}
	}

	//'_' = dec 95, oct 137, hex 5f, https://en.cppreference.com/w/cpp/language/ascii					
	if (i > 1)
	{
		int k = 1;
		for (int j = 1; j < i; j++)
		{
			if (((line[k - 1] == 95) && (line[j] == 95)) ||//two consecutive characters are '_'
				((j == (i - 1)) && (line[j] == 95)))//last character is '_'
			{
				///double dd = 5.;
			}
			else
			{
				line[k] = line[j];
				k++;
			}
		}

		if (line[k - 1] == 95)//last character is '_'
			k--;

		for (int j = k; k < i; k++)
			line[j] = '\0';
	}
}
//------------------------------------------------------------------------------
void TLE_get_parameters(str_TLE_record* out, char* L0, char* L1, char* L2)
{
	clear(out);
	sprintf_s((*out).Line0, "%s", L0);
	sprintf_s((*out).Line1, "%s", L1);
	sprintf_s((*out).Line2, "%s", L2);

	int L0_length = strlen(L0);
	char buf[MAXBUF];
	sprintf_s(buf, "%s", "");

	//get COSPAR
	//000000000011111111112222222222333333333344444444445555555555666666666
	//012345678901234567890123456789012345678901234567890123456789012345678
	//1 16908U 86061A   19305.80007987 -.00000099 +00000-0 -86868-5 0  9993
	sscanf_s(L1, "%*9c%8c", buf);
	buf[8] = '\0';
	(*out).International_Designator = buf;
	(*out).International_Designator = to_lower((*out).International_Designator);
	//remove spaces
	int iLength = (*out).International_Designator.length();
	if (iLength > 0)
	{
		sprintf_s(buf, "%s", "");
		sprintf_s(buf, "%s", (*out).International_Designator.c_str());

		int newLength = 0;
		for (int i = 0; i < iLength; i++)
		{
			if (buf[i] == char(' '))
			{
				//skip
			}
			else
			{
				buf[newLength] = buf[i];
				newLength++;
			}
		}
		buf[newLength] = '\0';
		(*out).International_Designator = buf;
	}

	//get MJD
	//000000000011111111112222222222333333333344444444445555555555666666666
	//012345678901234567890123456789012345678901234567890123456789012345678
	//1 16908U 86061A   19305.80007987 -.00000099 +00000-0 -86868-5 0  9993
	int year = 0;//19-20 Epoch Year (Last two digits of year)
	double doy1 = 0.;//21-32 Epoch (Day of the year and fractional portion of the day)
	sscanf_s(L1, "%*2c%5d%*2c%2d%3d%*4c%2d%12lf", &(*out).NORAD, &(*out).Launch_YYYY, &(*out).Launch_No, &year, &doy1);

	if ((*out).Launch_YYYY < 50)
		(*out).Launch_YYYY += 2000;
	else
		(*out).Launch_YYYY += 1900;

	if (year < 50.)
		year += 2000.;
	else
		year += 1900.;

	(*out).MJD = MJD_DoY1_d(year, doy1);

	if (L0_length > 2)
	{
		int name_length = L0_length - 2;
		memcpy(buf, L0 + 2, name_length);
		buf[name_length] = 0;
		sprintf_s((*out).Name, "%s", buf);
	}
	else
	{
		sprintf_s((*out).Name, "%d", (*out).NORAD);
	}
}
//------------------------------------------------------------------------------
void clear(str_TLE_record* s)
{
	(*s).NORAD = 0;
	(*s).MJD = 0.;
	(*s).Launch_YYYY = 0;
	(*s).Launch_No = 0;
	(*s).International_Designator = "";
	sprintf_s((*s).Name, "%s", "");
	sprintf_s((*s).Line0, "%s", "");
	sprintf_s((*s).Line1, "%s", "");
	sprintf_s((*s).Line2, "%s", "");
}
//------------------------------------------------------------------------------
double MJD_DoY1_d(
	int year,//2006
	double doy1//day of year starting from 1, Jan 1=1doy
)
{
	int month, day, Hour, Minute;
	date_from_year_and_doy1(year, int(doy1), &month, &day);
	double SOD = (doy1 - double(int(doy1))) * 86400.;
	Hour = int(SOD) / 3600;
	Minute = int(SOD) - Hour * 3600;
	double Second = SOD - double(Hour * 3600) - double(Minute * 60);
	double Full_MJD = MJD(year, month, day, Hour, Minute, Second);
	return Full_MJD;
}
//------------------------------------------------------------------------------
void date_from_year_and_doy1(//doy1=day of year starting from 1, Jan 1=1doy
	int year,
	int doy1,
	int* month,
	int* day
)
{
	int d[12] = { 31,59,90,120,151,181,212,243,273,304,334,365 };
	//is leap year? XXI century
	if (year % 4 == 0)
	{
		for (unsigned int i = 1; i < 12; i++)
			d[i] = d[i] + 1;
	}

	*month = 1;
	for (unsigned int i = 0; i < 12; i++)
	{
		if (doy1 > d[i])
			*month = i + 2;
	}
	if (*month > 1)
		*day = doy1 - d[*month - 2];
	else
		*day = doy1;
}
//------------------------------------------------------------------------------
double MJD(int year, int month, int day, int hour, int minute, double second)//year:2006
{
	int iMJD = MJD(year, month, day);//year:2006
	double result = double(iMJD) + (double(hour * 3600 + minute * 60) + second) / 86400.;
	return result;
}
//------------------------------------------------------------------------------
int MJD(int year, int month, int day)//year:2006
{
	static int k[] = { 0,31,59,90,120,151,181,212,243,273,304,334 };
	if (year % 4 == 0 && month > 2) ++day;
	int iMJD = (k[--month] + day + (year - 1972) * 365 + (year - 1969) / 4) + 41316;
	return iMJD;
}
//------------------------------------------------------------------------------
std::string to_lower(std::string str)
{
	std::string out = "";
	int length = str.length();
	if (length > 0)
	{
		char c;
		char* txt = new char[length + 1];
		sprintf(txt, "%s", "");
		sprintf(txt, "%s", str.c_str());

		for (int i = 0; i < length; i++)
		{
			c = txt[i];
			txt[i] = tolower(c);
		}
		out = std::string(txt);
		delete[]txt;
	}
	return out;
}
//------------------------------------------------------------------------------
void download_starlink_TLE(std::vector <str_TLE_record>* TLE)
{
	char Celestrak_Starlink[MAX_PATH];
	sprintf_s(Celestrak_Starlink, "%s", "");
	sprintf_s(Celestrak_Starlink, "%s", "https://celestrak.com/NORAD/elements/supplemental/starlink.txt");

	printf("\nDownloading TLE from: %s\n", Celestrak_Starlink);
	TLE_import_https(Celestrak_Starlink, TLE);

	char OutFilename[MAX_PATH];
	sprintf_s(OutFilename, "%s", "");
	sprintf_s(OutFilename, "%s", "starlink.txt");

	printf("\nSatellites found: %d\n", (*TLE).size());
	printf("\nSaving TLE to: %s\n", OutFilename);
	FILE* FFout;
	FFout = fopen(OutFilename, "w");

	if ((*TLE).size() > 0)
		for (int i = 0; i < (*TLE).size(); i++)
		{
			fprintf(FFout, "%s\n", (*TLE)[i].Name);
			fprintf(FFout, "%s\n", (*TLE)[i].Line1);
			fprintf(FFout, "%s\n", (*TLE)[i].Line2);
		}
	fclose(FFout);
}
//------------------------------------------------------------------------------
long double present_MJD_UTC()
{
	unsigned short YearNow, MonthNow, DayNow;
	unsigned short HourNow, MinNow, SecNow, mSecNow;
	give_present_time_UTC(&YearNow, &MonthNow, &DayNow, &HourNow, &MinNow, &SecNow, &mSecNow);
	return MJD(YearNow, MonthNow, DayNow, HourNow, MinNow, double(SecNow));
}
//------------------------------------------------------------------------------
void give_present_time_UTC(
	unsigned short* YearNow,
	unsigned short* MonthNow,
	unsigned short* DayNow,
	unsigned short* HourNow,
	unsigned short* MinNow,
	unsigned short* SecNow,
	unsigned short* mSecNow
)
{
	SYSTEMTIME st;
	GetSystemTime(&st);//Retrieves the current system date and time.The system time is expressed in Coordinated Universal Time(UTC).
	*YearNow = st.wYear;
	*MonthNow = st.wMonth;
	*DayNow = st.wDay;
	*HourNow = st.wHour;
	*MinNow = st.wMinute;
	*SecNow = st.wSecond;
	*mSecNow = st.wMilliseconds;
}
//------------------------------------------------------------------------------
void iau80in(iau80data& iau80rec)
{
	/* -----------------------------------------------------------------------------
*
*                           function iau80in
*
*  this function initializes the nutation matricies needed for reduction
*    calculations. the routine needs the filename of the files as input.
*
*  author        : david vallado                  719-573-2600   27 may 2002
*
*  revisions
*    vallado     - conversion to c++                             21 feb 2005
*
*  inputs          description                    range / units
*
*  outputs       :
*    iau80rec    - record containing the iau80 constants rad
*
*  locals        :
*    convrt      - conversion factor to degrees
*    i,j         - index
*
*  coupling      :
*    none        -
*
*  references    :
* --------------------------------------------------------------------------- */

	double convrt = 0.0001 / 3600.0; // 0.0001" to deg

	for (int i = 0; i < 107; i++)
	{
		for (int j = 0; j < 6; j++)
			iau80rec.iar80[i][j] = 0.;

		for (int j = 0; j < 5; j++)
			iau80rec.rar80[i][j] = 0.;
	}

	int i = 0;//fortran style, will be counting from 1																			
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -171996.00; iau80rec.rar80[i][2] = -174.20; iau80rec.rar80[i][3] = 92025.00; iau80rec.rar80[i][4] = 8.90;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -13187.00; iau80rec.rar80[i][2] = -1.60; iau80rec.rar80[i][3] = 5736.00; iau80rec.rar80[i][4] = -3.10;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -2274.00; iau80rec.rar80[i][2] = -0.20; iau80rec.rar80[i][3] = 977.00; iau80rec.rar80[i][4] = -0.50;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 2062.00; iau80rec.rar80[i][2] = 0.20; iau80rec.rar80[i][3] = -895.00; iau80rec.rar80[i][4] = 0.50;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 1426.00; iau80rec.rar80[i][2] = -3.40; iau80rec.rar80[i][3] = 54.00; iau80rec.rar80[i][4] = -0.10;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 712.00; iau80rec.rar80[i][2] = 0.10; iau80rec.rar80[i][3] = -7.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -517.00; iau80rec.rar80[i][2] = 1.20; iau80rec.rar80[i][3] = 224.00; iau80rec.rar80[i][4] = -0.60;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -386.00; iau80rec.rar80[i][2] = -0.40; iau80rec.rar80[i][3] = 200.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -301.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 129.00; iau80rec.rar80[i][4] = -0.10;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 217.00; iau80rec.rar80[i][2] = -0.50; iau80rec.rar80[i][3] = -95.00; iau80rec.rar80[i][4] = 0.30;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -158.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 129.00; iau80rec.rar80[i][2] = 0.10; iau80rec.rar80[i][3] = -70.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 123.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -53.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 63.00; iau80rec.rar80[i][2] = 0.10; iau80rec.rar80[i][3] = -33.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 63.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -2.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -59.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 26.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -58.00; iau80rec.rar80[i][2] = -0.10; iau80rec.rar80[i][3] = 32.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -51.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 27.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 48.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 46.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -24.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -38.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 16.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -31.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 13.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 29.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 29.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -12.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 26.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -22.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 21.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -10.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 2; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 17.00; iau80rec.rar80[i][2] = -0.10; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 2; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -16.00; iau80rec.rar80[i][2] = 0.10; iau80rec.rar80[i][3] = 7.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 16.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -8.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -15.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 9.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -13.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 7.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -12.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 6.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 11.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -10.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 5.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -8.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -7.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -7.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -7.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 7.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -6.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -6.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 6.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 6.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 6.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -5.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -5.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -5.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 3.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 5.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -1; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -4.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 1; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -4.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -4.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 4.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 4.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -2.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 4.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -2.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -1; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 3; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 3.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 4; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = -2; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 3; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 1; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 2.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -4; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -4; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 4; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -1; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 4; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = -1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 4; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 3; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 4; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 2; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 2; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = -1.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 2; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = -2; iau80rec.iar80[i][4] = 0; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 1; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = -2; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = 0; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 1; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = -1; iau80rec.iar80[i][2] = -1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 2; iau80rec.iar80[i][5] = 1; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;
	i++;	iau80rec.iar80[i][1] = 0; iau80rec.iar80[i][2] = 1; iau80rec.iar80[i][3] = 0; iau80rec.iar80[i][4] = 1; iau80rec.iar80[i][5] = 0; iau80rec.rar80[i][1] = 1.00; iau80rec.rar80[i][2] = 0.00; iau80rec.rar80[i][3] = 0.00; iau80rec.rar80[i][4] = 0.00;

	for (int i = 0; i < 107; i++)
	{
		for (int j = 1; j <= 4; j++)
			iau80rec.rar80[i][j] = iau80rec.rar80[i][j] * convrt;
	}
}
//------------------------------------------------------------------------------
eph_tle::eph_tle()
{
	//The time is based on UTC and the coordinate system should be considered a true-equator, mean equinox system.
	iau80in(iau80rec);
}
//------------------------------------------------------------------------------
eph_tle::~eph_tle()
{
	//Destructors do not accept arguments

}
//------------------------------------------------------------------------------

/*-----------------------------------------------------------------------------
	*
	*                             procedure sgp4init
	*
	//The time is based on UTC and the coordinate system should be considered a true-equator, mean equinox system.
	*  this procedure initializes variables for sgp4.
	*
	*  author        : david vallado                  719-573-2600   28 jun 2005
	*
	*  inputs        :
	*    opsmode     - mode of operation afspc or improved 'a', 'i'
	*    whichconst  - which set of constants to use  72, 84
	*    satn        - satellite number
	*    bstar       - sgp4 type drag coefficient              kg/m2er
	*    ecco        - eccentricity
	*    epoch       - epoch time in days from jan 0, 1950. 0 hr
	*    argpo       - argument of perigee (output if ds)
	*    inclo       - inclination
	*    mo          - mean anomaly (output if ds)
	*    no          - mean motion
	*    nodeo       - right ascension of ascending node
	*
	*  outputs       :
	*    satrec      - common values for subsequent calls
	*    return code - non-zero on error.
	*                   1 - mean elements, ecc >= 1.0 or ecc < -0.001 or a < 0.95 er
	*                   2 - mean motion less than 0.0
	*                   3 - pert elements, ecc < 0.0  or  ecc > 1.0
	*                   4 - semi-latus rectum < 0.0
	*                   5 - epoch elements are sub-orbital
	*                   6 - satellite has decayed
	*
	*  locals        :
	*    cnodm  , snodm  , cosim  , sinim  , cosomm , sinomm
	*    cc1sq  , cc2    , cc3
	*    coef   , coef1
	*    cosio4      -
	*    day         -
	*    dndt        -
	*    em          - eccentricity
	*    emsq        - eccentricity squared
	*    eeta        -
	*    etasq       -
	*    gam         -
	*    argpm       - argument of perigee
	*    nodem       -
	*    inclm       - inclination
	*    mm          - mean anomaly
	*    nm          - mean motion
	*    perige      - perigee
	*    pinvsq      -
	*    psisq       -
	*    qzms24      -
	*    rtemsq      -
	*    s1, s2, s3, s4, s5, s6, s7          -
	*    sfour       -
	*    ss1, ss2, ss3, ss4, ss5, ss6, ss7         -
	*    sz1, sz2, sz3
	*    sz11, sz12, sz13, sz21, sz22, sz23, sz31, sz32, sz33        -
	*    tc          -
	*    temp        -
	*    temp1, temp2, temp3       -
	*    tsi         -
	*    xpidot      -
	*    xhdot1      -
	*    z1, z2, z3          -
	*    z11, z12, z13, z21, z22, z23, z31, z32, z33         -
	*
	*  coupling      :
	*    getgravconst-
	*    initl       -
	*    dscom       -
	*    dpper       -
	*    dsinit      -
	*    sgp4        -
	*
	*  references    :
	*    hoots, roehrich, norad spacetrack report #3 1980
	*    hoots, norad spacetrack report #6 1986
	*    hoots, schumacher and glover 2004
	*    vallado, crawford, hujsak, kelso  2006
	----------------------------------------------------------------------------*/

bool eph_tle::sgp4init
(
	gravconsttype whichconst, char opsmode, const int satn, const double epoch,
	const double xbstar, const double xndot, const double xnddot, const double xecco, const double xargpo,
	const double xinclo, const double xmo, const double xno_kozai,
	const double xnodeo, elsetrec& satrec
)
{
	/* --------------------- local variables ------------------------ */
	double ao, ainv, con42, cosio, sinio, cosio2, eccsq,
		omeosq, posq, rp, rteosq,
		cnodm, snodm, cosim, sinim, cosomm, sinomm, cc1sq,
		cc2, cc3, coef, coef1, cosio4, day, dndt,
		em, emsq, eeta, etasq, gam, argpm, nodem,
		inclm, mm, nm, perige, pinvsq, psisq, qzms24,
		rtemsq, s1, s2, s3, s4, s5, s6,
		s7, sfour, ss1, ss2, ss3, ss4, ss5,
		ss6, ss7, sz1, sz2, sz3, sz11, sz12,
		sz13, sz21, sz22, sz23, sz31, sz32, sz33,
		tc, temp, temp1, temp2, temp3, tsi, xpidot,
		xhdot1, z1, z2, z3, z11, z12, z13,
		z21, z22, z23, z31, z32, z33,
		qzms2t, ss, x2o3, r[3], v[3],
		delmotemp, qzms2ttemp, qzms24temp;

	/* ------------------------ initialization --------------------- */
	// sgp4fix divisor for divide by zero check on inclination
	// the old check used 1.0 + cos(pi-1.0e-9), but then compared it to
	// 1.5 e-12, so the threshold was changed to 1.5e-12 for consistency
	const double temp4 = 1.5e-12;

	/* ----------- set all near earth variables to zero ------------ */
	satrec.isimp = 0;   satrec.deep_space = false; satrec.aycof = 0.0;
	satrec.con41 = 0.0; satrec.cc1 = 0.0; satrec.cc4 = 0.0;
	satrec.cc5 = 0.0; satrec.d2 = 0.0; satrec.d3 = 0.0;
	satrec.d4 = 0.0; satrec.delmo = 0.0; satrec.eta = 0.0;
	satrec.argpdot = 0.0; satrec.omgcof = 0.0; satrec.sinmao = 0.0;
	satrec.t = 0.0; satrec.t2cof = 0.0; satrec.t3cof = 0.0;
	satrec.t4cof = 0.0; satrec.t5cof = 0.0; satrec.x1mth2 = 0.0;
	satrec.x7thm1 = 0.0; satrec.mdot = 0.0; satrec.nodedot = 0.0;
	satrec.xlcof = 0.0; satrec.xmcof = 0.0; satrec.nodecf = 0.0;

	/* ----------- set all deep space variables to zero ------------ */
	satrec.irez = 0;   satrec.d2201 = 0.0; satrec.d2211 = 0.0;
	satrec.d3210 = 0.0; satrec.d3222 = 0.0; satrec.d4410 = 0.0;
	satrec.d4422 = 0.0; satrec.d5220 = 0.0; satrec.d5232 = 0.0;
	satrec.d5421 = 0.0; satrec.d5433 = 0.0; satrec.dedt = 0.0;
	satrec.del1 = 0.0; satrec.del2 = 0.0; satrec.del3 = 0.0;
	satrec.didt = 0.0; satrec.dmdt = 0.0; satrec.dnodt = 0.0;
	satrec.domdt = 0.0; satrec.e3 = 0.0; satrec.ee2 = 0.0;
	satrec.peo = 0.0; satrec.pgho = 0.0; satrec.pho = 0.0;
	satrec.pinco = 0.0; satrec.plo = 0.0; satrec.se2 = 0.0;
	satrec.se3 = 0.0; satrec.sgh2 = 0.0; satrec.sgh3 = 0.0;
	satrec.sgh4 = 0.0; satrec.sh2 = 0.0; satrec.sh3 = 0.0;
	satrec.si2 = 0.0; satrec.si3 = 0.0; satrec.sl2 = 0.0;
	satrec.sl3 = 0.0; satrec.sl4 = 0.0; satrec.gsto = 0.0;
	satrec.xfact = 0.0; satrec.xgh2 = 0.0; satrec.xgh3 = 0.0;
	satrec.xgh4 = 0.0; satrec.xh2 = 0.0; satrec.xh3 = 0.0;
	satrec.xi2 = 0.0; satrec.xi3 = 0.0; satrec.xl2 = 0.0;
	satrec.xl3 = 0.0; satrec.xl4 = 0.0; satrec.xlamo = 0.0;
	satrec.zmol = 0.0; satrec.zmos = 0.0; satrec.atime = 0.0;
	satrec.xli = 0.0; satrec.xni = 0.0;

	/* ------------------------ earth constants ----------------------- */
	// sgp4fix identify constants and allow alternate values
	// this is now the only call for the constants
	getgravconst(whichconst, satrec.tumin, satrec.mu, satrec.radiusearthkm, satrec.xke, satrec.j2, satrec.j3, satrec.j4, satrec.j3oj2);

	//-------------------------------------------------------------------------
	satrec.error = 0;
	satrec.operationmode = opsmode;
	satrec.satnum = satn;

	// sgp4fix - note the following variables are also passed directly via satrec.
	// it is possible to streamline the sgp4init call by deleting the "x"
	// variables, but the user would need to set the satrec.* values first. we
	// include the additional assignments in case twoline2rv is not used.
	satrec.bstar = xbstar;
	// sgp4fix allow additional parameters in the struct
	satrec.ndot = xndot;
	satrec.nddot = xnddot;
	satrec.ecco = xecco;
	satrec.argpo = xargpo;
	satrec.inclo = xinclo;
	satrec.mo = xmo;
	// sgp4fix rename variables to clarify which mean motion is intended
	satrec.no_kozai = xno_kozai;
	satrec.nodeo = xnodeo;

	// single averaged mean elements
	satrec.am = 0.;
	satrec.em = 0.;
	satrec.im = 0.;
	satrec.Om = 0.;
	satrec.mm = 0.;
	satrec.nm = 0.;

	/* ------------------------ earth constants ----------------------- */
	// sgp4fix identify constants and allow alternate values no longer needed
	// getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );
	ss = 78.0 / satrec.radiusearthkm + 1.0;
	// sgp4fix use multiply for speed instead of pow
	qzms2ttemp = (120.0 - 78.0) / satrec.radiusearthkm;
	qzms2t = qzms2ttemp * qzms2ttemp * qzms2ttemp * qzms2ttemp;
	x2o3 = 2.0 / 3.0;

	satrec.initialize = true;
	satrec.t = 0.0;

	// sgp4fix remove satn as it is not needed in initl
	initl
	(satrec.xke, satrec.j2, satrec.ecco, epoch, satrec.inclo, satrec.no_kozai, satrec.operationmode,
		satrec.deep_space, ainv, ao, satrec.con41, con42, cosio,
		cosio2, eccsq, omeosq,
		posq, rp, rteosq, sinio, satrec.gsto, satrec.no_unkozai);

	satrec.a = pow(satrec.no_unkozai * satrec.tumin, (-2.0 / 3.0));
	satrec.alta = satrec.a * (1.0 + satrec.ecco) - 1.0;
	satrec.altp = satrec.a * (1.0 - satrec.ecco) - 1.0;
	satrec.error = 0;

	// sgp4fix remove this check as it is unnecessary
	// the mrt check in sgp4 handles decaying satellite cases even if the starting
	// condition is below the surface of te earth
	//     if (rp < 1.0)
	//       {
	//         printf("# *** satn%d epoch elts sub-orbital ***\n", satn);
	//         satrec.error = 5;
	//       }

	if ((omeosq >= 0.0) || (satrec.no_unkozai >= 0.0))
	{
		satrec.isimp = 0;
		if (rp < (220.0 / satrec.radiusearthkm + 1.0))
			satrec.isimp = 1;
		sfour = ss;
		qzms24 = qzms2t;
		perige = (rp - 1.0) * satrec.radiusearthkm;

		/* - for perigees below 156 km, s and qoms2t are altered - */
		if (perige < 156.0)
		{
			sfour = perige - 78.0;
			if (perige < 98.0)
				sfour = 20.0;
			// sgp4fix use multiply for speed instead of pow
			qzms24temp = (120.0 - sfour) / satrec.radiusearthkm;
			qzms24 = qzms24temp * qzms24temp * qzms24temp * qzms24temp;
			sfour = sfour / satrec.radiusearthkm + 1.0;
		}
		pinvsq = 1.0 / posq;

		tsi = 1.0 / (ao - sfour);
		satrec.eta = ao * satrec.ecco * tsi;
		etasq = satrec.eta * satrec.eta;
		eeta = satrec.ecco * satrec.eta;
		psisq = fabs(1.0 - etasq);
		coef = qzms24 * pow(tsi, 4.0);
		coef1 = coef / pow(psisq, 3.5);
		cc2 = coef1 * satrec.no_unkozai * (ao * (1.0 + 1.5 * etasq + eeta *
			(4.0 + etasq)) + 0.375 * satrec.j2 * tsi / psisq * satrec.con41 *
			(8.0 + 3.0 * etasq * (8.0 + etasq)));
		satrec.cc1 = satrec.bstar * cc2;
		cc3 = 0.0;
		if (satrec.ecco > 1.0e-4)
			cc3 = -2.0 * coef * tsi * satrec.j3oj2 * satrec.no_unkozai * sinio / satrec.ecco;
		satrec.x1mth2 = 1.0 - cosio2;
		satrec.cc4 = 2.0 * satrec.no_unkozai * coef1 * ao * omeosq *
			(satrec.eta * (2.0 + 0.5 * etasq) + satrec.ecco *
				(0.5 + 2.0 * etasq) - satrec.j2 * tsi / (ao * psisq) *
				(-3.0 * satrec.con41 * (1.0 - 2.0 * eeta + etasq *
					(1.5 - 0.5 * eeta)) + 0.75 * satrec.x1mth2 *
					(2.0 * etasq - eeta * (1.0 + etasq)) * cos(2.0 * satrec.argpo)));
		satrec.cc5 = 2.0 * coef1 * ao * omeosq * (1.0 + 2.75 *
			(etasq + eeta) + eeta * etasq);
		cosio4 = cosio2 * cosio2;
		temp1 = 1.5 * satrec.j2 * pinvsq * satrec.no_unkozai;
		temp2 = 0.5 * temp1 * satrec.j2 * pinvsq;
		temp3 = -0.46875 * satrec.j4 * pinvsq * pinvsq * satrec.no_unkozai;
		satrec.mdot = satrec.no_unkozai + 0.5 * temp1 * rteosq * satrec.con41 + 0.0625 *
			temp2 * rteosq * (13.0 - 78.0 * cosio2 + 137.0 * cosio4);
		satrec.argpdot = -0.5 * temp1 * con42 + 0.0625 * temp2 *
			(7.0 - 114.0 * cosio2 + 395.0 * cosio4) +
			temp3 * (3.0 - 36.0 * cosio2 + 49.0 * cosio4);
		xhdot1 = -temp1 * cosio;
		satrec.nodedot = xhdot1 + (0.5 * temp2 * (4.0 - 19.0 * cosio2) +
			2.0 * temp3 * (3.0 - 7.0 * cosio2)) * cosio;
		xpidot = satrec.argpdot + satrec.nodedot;
		satrec.omgcof = satrec.bstar * cc3 * cos(satrec.argpo);
		satrec.xmcof = 0.0;
		if (satrec.ecco > 1.0e-4)
			satrec.xmcof = -x2o3 * coef * satrec.bstar / eeta;
		satrec.nodecf = 3.5 * omeosq * xhdot1 * satrec.cc1;
		satrec.t2cof = 1.5 * satrec.cc1;
		// sgp4fix for divide by zero with xinco = 180 deg
		if (fabs(cosio + 1.0) > 1.5e-12)
			satrec.xlcof = -0.25 * satrec.j3oj2 * sinio * (3.0 + 5.0 * cosio) / (1.0 + cosio);
		else
			satrec.xlcof = -0.25 * satrec.j3oj2 * sinio * (3.0 + 5.0 * cosio) / temp4;
		satrec.aycof = -0.5 * satrec.j3oj2 * sinio;
		// sgp4fix use multiply for speed instead of pow
		delmotemp = 1.0 + satrec.eta * cos(satrec.mo);
		satrec.delmo = delmotemp * delmotemp * delmotemp;
		satrec.sinmao = sin(satrec.mo);
		satrec.x7thm1 = 7.0 * cosio2 - 1.0;

		/* --------------- deep space initialization ------------- */
		if ((2 * pi / satrec.no_unkozai) >= 225.0)
		{
			satrec.deep_space = true;
			satrec.isimp = 1;
			tc = 0.0;
			inclm = satrec.inclo;

			dscom
			(
				epoch, satrec.ecco, satrec.argpo, tc, satrec.inclo, satrec.nodeo,
				satrec.no_unkozai, snodm, cnodm, sinim, cosim, sinomm, cosomm,
				day, satrec.e3, satrec.ee2, em, emsq, gam,
				satrec.peo, satrec.pgho, satrec.pho, satrec.pinco,
				satrec.plo, rtemsq, satrec.se2, satrec.se3,
				satrec.sgh2, satrec.sgh3, satrec.sgh4,
				satrec.sh2, satrec.sh3, satrec.si2, satrec.si3,
				satrec.sl2, satrec.sl3, satrec.sl4, s1, s2, s3, s4, s5,
				s6, s7, ss1, ss2, ss3, ss4, ss5, ss6, ss7, sz1, sz2, sz3,
				sz11, sz12, sz13, sz21, sz22, sz23, sz31, sz32, sz33,
				satrec.xgh2, satrec.xgh3, satrec.xgh4, satrec.xh2,
				satrec.xh3, satrec.xi2, satrec.xi3, satrec.xl2,
				satrec.xl3, satrec.xl4, nm, z1, z2, z3, z11,
				z12, z13, z21, z22, z23, z31, z32, z33,
				satrec.zmol, satrec.zmos
			);
			dpper
			(
				satrec.e3, satrec.ee2, satrec.peo, satrec.pgho,
				satrec.pho, satrec.pinco, satrec.plo, satrec.se2,
				satrec.se3, satrec.sgh2, satrec.sgh3, satrec.sgh4,
				satrec.sh2, satrec.sh3, satrec.si2, satrec.si3,
				satrec.sl2, satrec.sl3, satrec.sl4, satrec.t,
				satrec.xgh2, satrec.xgh3, satrec.xgh4, satrec.xh2,
				satrec.xh3, satrec.xi2, satrec.xi3, satrec.xl2,
				satrec.xl3, satrec.xl4, satrec.zmol, satrec.zmos, inclm, satrec.initialize,
				satrec.ecco, satrec.inclo, satrec.nodeo, satrec.argpo, satrec.mo,
				satrec.operationmode
			);

			argpm = 0.0;
			nodem = 0.0;
			mm = 0.0;

			dsinit
			(
				satrec.xke,
				cosim, emsq, satrec.argpo, s1, s2, s3, s4, s5, sinim, ss1, ss2, ss3, ss4,
				ss5, sz1, sz3, sz11, sz13, sz21, sz23, sz31, sz33, satrec.t, tc,
				satrec.gsto, satrec.mo, satrec.mdot, satrec.no_unkozai, satrec.nodeo,
				satrec.nodedot, xpidot, z1, z3, z11, z13, z21, z23, z31, z33,
				satrec.ecco, eccsq, em, argpm, inclm, mm, nm, nodem,
				satrec.irez, satrec.atime,
				satrec.d2201, satrec.d2211, satrec.d3210, satrec.d3222,
				satrec.d4410, satrec.d4422, satrec.d5220, satrec.d5232,
				satrec.d5421, satrec.d5433, satrec.dedt, satrec.didt,
				satrec.dmdt, dndt, satrec.dnodt, satrec.domdt,
				satrec.del1, satrec.del2, satrec.del3, satrec.xfact,
				satrec.xlamo, satrec.xli, satrec.xni
			);
		}

		/* ----------- set variables if not deep space ----------- */
		if (satrec.isimp != 1)
		{
			cc1sq = satrec.cc1 * satrec.cc1;
			satrec.d2 = 4.0 * ao * tsi * cc1sq;
			temp = satrec.d2 * tsi * satrec.cc1 / 3.0;
			satrec.d3 = (17.0 * ao + sfour) * temp;
			satrec.d4 = 0.5 * temp * ao * tsi * (221.0 * ao + 31.0 * sfour) *
				satrec.cc1;
			satrec.t3cof = satrec.d2 + 2.0 * cc1sq;
			satrec.t4cof = 0.25 * (3.0 * satrec.d3 + satrec.cc1 *
				(12.0 * satrec.d2 + 10.0 * cc1sq));
			satrec.t5cof = 0.2 * (3.0 * satrec.d4 +
				12.0 * satrec.cc1 * satrec.d3 +
				6.0 * satrec.d2 * satrec.d2 +
				15.0 * cc1sq * (2.0 * satrec.d2 + cc1sq));
		}
	} // if omeosq = 0 ...

	/* finally propogate to zero epoch to initialize all others. */
	// sgp4fix take out check to let satellites process until they are actually below earth surface
	//       if(satrec.error == 0)
	sgp4(satrec, 0.0, r, v);

	satrec.initialize = false;

	//#include "debug6.cpp"
	//sgp4fix return boolean. satrec.error contains any error codes
	return true;
}  // sgp4init
//------------------------------------------------------------------------------

	/*-----------------------------------------------------------------------------
	*
	*                             procedure sgp4
	*
	*  this procedure is the sgp4 prediction model from space command. this is an
	*    updated and combined version of sgp4 and sdp4, which were originally
	*    published separately in spacetrack report #3. this version follows the
	*    methodology from the aiaa paper (2006) describing the history and
	*    development of the code.
	*
	*  author        : david vallado                  719-573-2600   28 jun 2005
	*
	*  inputs        :
	*    satrec	 - initialised structure from sgp4init() call.
	*    tsince	 - time since epoch (minutes)
	*
	*  outputs       :
	*    r           - position vector                     km
	*    v           - velocity                            km/sec
	*  return code - non-zero on error.
	*                   1 - mean elements, ecc >= 1.0 or ecc < -0.001 or a < 0.95 er
	*                   2 - mean motion less than 0.0
	*                   3 - pert elements, ecc < 0.0  or  ecc > 1.0
	*                   4 - semi-latus rectum < 0.0
	*                   5 - epoch elements are sub-orbital
	*                   6 - satellite has decayed
	*
	*  locals        :
	*    am          -
	*    axnl, aynl        -
	*    betal       -
	*    cosim   , sinim   , cosomm  , sinomm  , cnod    , snod    , cos2u   ,
	*    sin2u   , coseo1  , sineo1  , cosi    , sini    , cosip   , sinip   ,
	*    cosisq  , cossu   , sinsu   , cosu    , sinu
	*    delm        -
	*    delomg      -
	*    dndt        -
	*    eccm        -
	*    emsq        -
	*    ecose       -
	*    el2         -
	*    eo1         -
	*    eccp        -
	*    esine       -
	*    argpm       -
	*    argpp       -
	*    omgadf      -c
	*    pl          -
	*    r           -
	*    rtemsq      -
	*    rdotl       -
	*    rl          -
	*    rvdot       -
	*    rvdotl      -
	*    su          -
	*    t2  , t3   , t4    , tc
	*    tem5, temp , temp1 , temp2  , tempa  , tempe  , templ
	*    u   , ux   , uy    , uz     , vx     , vy     , vz
	*    inclm       - inclination
	*    mm          - mean anomaly
	*    nm          - mean motion
	*    nodem       - right asc of ascending node
	*    xinc        -
	*    xincp       -
	*    xl          -
	*    xlm         -
	*    mp          -
	*    xmdf        -
	*    xmx         -
	*    xmy         -
	*    nodedf      -
	*    xnode       -
	*    nodep       -
	*    np          -
	*
	*  coupling      :
	*    getgravconst- no longer used. Variables are conatined within satrec
	*    dpper
	*    dpspace
	*
	*  references    :
	*    hoots, roehrich, norad spacetrack report #3 1980
	*    hoots, norad spacetrack report #6 1986
	*    hoots, schumacher and glover 2004
	*    vallado, crawford, hujsak, kelso  2006
	----------------------------------------------------------------------------*/

bool eph_tle::sgp4(elsetrec& satrec, double tsince, double r[3], double v[3])
{
	double am, axnl, aynl, betal, cosim, cnod,
		cos2u, coseo1, cosi, cosip, cosisq, cossu, cosu,
		delm, delomg, em, emsq, ecose, el2, eo1,
		ep, esine, argpm, argpp, argpdf, pl, mrt = 0.0,
		mvt, rdotl, rl, rvdot, rvdotl, sinim,
		sin2u, sineo1, sini, sinip, sinsu, sinu,
		snod, su, t2, t3, t4, tem5, temp,
		temp1, temp2, tempa, tempe, templ, u, ux,
		uy, uz, vx, vy, vz, inclm, mm,
		nm, nodem, xinc, xincp, xl, xlm, mp,
		xmdf, xmx, xmy, nodedf, xnode, nodep, tc, dndt,
		x2o3, vkmpersec, delmtemp;
	int ktr;

	/* ------------------ set mathematical constants --------------- */
	// sgp4fix divisor for divide by zero check on inclination
	// the old check used 1.0 + cos(pi-1.0e-9), but then compared it to
	// 1.5 e-12, so the threshold was changed to 1.5e-12 for consistency
	const double temp4 = 1.5e-12;
	x2o3 = 2.0 / 3.0;
	// sgp4fix identify constants and allow alternate values
	// getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );
	vkmpersec = satrec.radiusearthkm * satrec.xke / 60.0;

	/* --------------------- clear sgp4 error flag ----------------- */
	satrec.t = tsince;
	satrec.error = 0;

	/* ------- update for secular gravity and atmospheric drag ----- */
	xmdf = satrec.mo + satrec.mdot * satrec.t;
	argpdf = satrec.argpo + satrec.argpdot * satrec.t;
	nodedf = satrec.nodeo + satrec.nodedot * satrec.t;
	argpm = argpdf;
	mm = xmdf;
	t2 = satrec.t * satrec.t;
	nodem = nodedf + satrec.nodecf * t2;
	tempa = 1.0 - satrec.cc1 * satrec.t;
	tempe = satrec.bstar * satrec.cc4 * satrec.t;
	templ = satrec.t2cof * t2;

	if (satrec.isimp != 1)
	{
		delomg = satrec.omgcof * satrec.t;
		// sgp4fix use mutliply for speed instead of pow
		delmtemp = 1.0 + satrec.eta * cos(xmdf);
		delm = satrec.xmcof *
			(delmtemp * delmtemp * delmtemp -
				satrec.delmo);
		temp = delomg + delm;
		mm = xmdf + temp;
		argpm = argpdf - temp;
		t3 = t2 * satrec.t;
		t4 = t3 * satrec.t;
		tempa = tempa - satrec.d2 * t2 - satrec.d3 * t3 -
			satrec.d4 * t4;
		tempe = tempe + satrec.bstar * satrec.cc5 * (sin(mm) -
			satrec.sinmao);
		templ = templ + satrec.t3cof * t3 + t4 * (satrec.t4cof +
			satrec.t * satrec.t5cof);
	}

	nm = satrec.no_unkozai;
	em = satrec.ecco;
	inclm = satrec.inclo;
	if (satrec.deep_space)
	{
		tc = satrec.t;
		dspace
		(
			satrec.irez,
			satrec.d2201, satrec.d2211, satrec.d3210,
			satrec.d3222, satrec.d4410, satrec.d4422,
			satrec.d5220, satrec.d5232, satrec.d5421,
			satrec.d5433, satrec.dedt, satrec.del1,
			satrec.del2, satrec.del3, satrec.didt,
			satrec.dmdt, satrec.dnodt, satrec.domdt,
			satrec.argpo, satrec.argpdot, satrec.t, tc,
			satrec.gsto, satrec.xfact, satrec.xlamo,
			satrec.no_unkozai, satrec.atime,
			em, argpm, inclm, satrec.xli, mm, satrec.xni,
			nodem, dndt, nm
		);
	} // if method = d

	if (nm <= 0.0)
	{
		//         printf("# error nm %f\n", nm);
		satrec.error = 2;
		// sgp4fix add return
		return false;
	}
	am = std::pow((satrec.xke / nm), x2o3) * tempa * tempa;
	nm = satrec.xke / std::pow(am, 1.5);
	em = em - tempe;

	// fix tolerance for error recognition
	// sgp4fix am is fixed from the previous nm check
	if ((em >= 1.0) || (em < -0.001)/* || (am < 0.95)*/)
	{
		//         printf("# error em %f\n", em);
		satrec.error = 1;
		// sgp4fix to return if there is an error in eccentricity
		return false;
	}
	// sgp4fix fix tolerance to avoid a divide by zero
	if (em < 1.0e-6)
		em = 1.0e-6;
	mm = mm + satrec.no_unkozai * templ;
	xlm = mm + argpm + nodem;
	emsq = em * em;
	temp = 1.0 - emsq;

	nodem = fmod(nodem, pi2);
	argpm = fmod(argpm, pi2);
	xlm = fmod(xlm, pi2);
	mm = fmod(xlm - argpm - nodem, pi2);

	// sgp4fix recover singly averaged mean elements
	satrec.am = am;
	satrec.em = em;
	satrec.im = inclm;
	satrec.Om = nodem;
	satrec.om = argpm;
	satrec.mm = mm;
	satrec.nm = nm;

	/* ----------------- compute extra mean quantities ------------- */
	sinim = sin(inclm);
	cosim = cos(inclm);

	/* -------------------- add lunar-solar periodics -------------- */
	ep = em;
	xincp = inclm;
	argpp = argpm;
	nodep = nodem;
	mp = mm;
	sinip = sinim;
	cosip = cosim;
	if (satrec.deep_space)
	{
		dpper
		(
			satrec.e3, satrec.ee2, satrec.peo,
			satrec.pgho, satrec.pho, satrec.pinco,
			satrec.plo, satrec.se2, satrec.se3,
			satrec.sgh2, satrec.sgh3, satrec.sgh4,
			satrec.sh2, satrec.sh3, satrec.si2,
			satrec.si3, satrec.sl2, satrec.sl3,
			satrec.sl4, satrec.t, satrec.xgh2,
			satrec.xgh3, satrec.xgh4, satrec.xh2,
			satrec.xh3, satrec.xi2, satrec.xi3,
			satrec.xl2, satrec.xl3, satrec.xl4,
			satrec.zmol, satrec.zmos, satrec.inclo,
			false, ep, xincp, nodep, argpp, mp, satrec.operationmode
		);
		if (xincp < 0.0)
		{
			xincp = -xincp;
			nodep = nodep + pi;
			argpp = argpp - pi;
		}
		if ((ep < 0.0) || (ep > 1.0))
		{
			//            printf("# error ep %f\n", ep);
			satrec.error = 3;
			// sgp4fix add return
			return false;
		}
	} // if method = d

	/* -------------------- long period periodics ------------------ */
	if (satrec.deep_space)
	{
		sinip = sin(xincp);
		cosip = cos(xincp);
		satrec.aycof = -0.5 * satrec.j3oj2 * sinip;
		// sgp4fix for divide by zero for xincp = 180 deg
		if (fabs(cosip + 1.0) > 1.5e-12)
			satrec.xlcof = -0.25 * satrec.j3oj2 * sinip * (3.0 + 5.0 * cosip) / (1.0 + cosip);
		else
			satrec.xlcof = -0.25 * satrec.j3oj2 * sinip * (3.0 + 5.0 * cosip) / temp4;
	}
	axnl = ep * cos(argpp);
	temp = 1.0 / (am * (1.0 - ep * ep));
	aynl = ep * sin(argpp) + temp * satrec.aycof;
	xl = mp + argpp + nodep + temp * satrec.xlcof * axnl;

	/* --------------------- solve kepler's equation --------------- */
	u = fmod(xl - nodep, pi2);
	eo1 = u;
	tem5 = 9999.9;
	ktr = 1;
	//   sgp4fix for kepler iteration
	//   the following iteration needs better limits on corrections
	while ((fabs(tem5) >= 1.0e-12) && (ktr <= 10))
	{
		sineo1 = sin(eo1);
		coseo1 = cos(eo1);
		tem5 = 1.0 - coseo1 * axnl - sineo1 * aynl;
		tem5 = (u - aynl * coseo1 + axnl * sineo1 - eo1) / tem5;
		if (fabs(tem5) >= 0.95)
			tem5 = tem5 > 0.0 ? 0.95 : -0.95;
		eo1 = eo1 + tem5;
		ktr = ktr + 1;
	}

	/* ------------- short period preliminary quantities ----------- */
	ecose = axnl * coseo1 + aynl * sineo1;
	esine = axnl * sineo1 - aynl * coseo1;
	el2 = axnl * axnl + aynl * aynl;
	pl = am * (1.0 - el2);
	if (pl < 0.0)
	{
		//         printf("# error pl %f\n", pl);
		satrec.error = 4;
		// sgp4fix add return
		return false;
	}
	else
	{
		rl = am * (1.0 - ecose);
		rdotl = sqrt(am) * esine / rl;
		rvdotl = sqrt(pl) / rl;
		betal = sqrt(1.0 - el2);
		temp = esine / (1.0 + betal);
		sinu = am / rl * (sineo1 - aynl - axnl * temp);
		cosu = am / rl * (coseo1 - axnl + aynl * temp);
		su = atan2(sinu, cosu);
		sin2u = (cosu + cosu) * sinu;
		cos2u = 1.0 - 2.0 * sinu * sinu;
		temp = 1.0 / pl;
		temp1 = 0.5 * satrec.j2 * temp;
		temp2 = temp1 * temp;

		/* -------------- update for short period periodics ------------ */
		if (satrec.deep_space)
		{
			cosisq = cosip * cosip;
			satrec.con41 = 3.0 * cosisq - 1.0;
			satrec.x1mth2 = 1.0 - cosisq;
			satrec.x7thm1 = 7.0 * cosisq - 1.0;
		}
		mrt = rl * (1.0 - 1.5 * temp2 * betal * satrec.con41) + 0.5 * temp1 * satrec.x1mth2 * cos2u;
		su = su - 0.25 * temp2 * satrec.x7thm1 * sin2u;
		xnode = nodep + 1.5 * temp2 * cosip * sin2u;
		xinc = xincp + 1.5 * temp2 * cosip * sinip * cos2u;
		mvt = rdotl - nm * temp1 * satrec.x1mth2 * sin2u / satrec.xke;
		rvdot = rvdotl + nm * temp1 * (satrec.x1mth2 * cos2u + 1.5 * satrec.con41) / satrec.xke;

		/* --------------------- orientation vectors ------------------- */
		sinsu = sin(su);
		cossu = cos(su);
		snod = sin(xnode);
		cnod = cos(xnode);
		sini = sin(xinc);
		cosi = cos(xinc);
		xmx = -snod * cosi;
		xmy = cnod * cosi;
		ux = xmx * sinsu + cnod * cossu;
		uy = xmy * sinsu + snod * cossu;
		uz = sini * sinsu;
		vx = xmx * cossu - cnod * sinsu;
		vy = xmy * cossu - snod * sinsu;
		vz = sini * cossu;

		/* --------- position and velocity (in km and km/sec) ---------- */
		r[0] = (mrt * ux) * satrec.radiusearthkm;
		r[1] = (mrt * uy) * satrec.radiusearthkm;
		r[2] = (mrt * uz) * satrec.radiusearthkm;
		v[0] = (mvt * ux + rvdot * vx) * vkmpersec;
		v[1] = (mvt * uy + rvdot * vy) * vkmpersec;
		v[2] = (mvt * uz + rvdot * vz) * vkmpersec;
	}  // if pl > 0

	// sgp4fix for decaying satellites
	if (mrt < 1.0)
	{
		//         printf("# decay condition %11.6f \n",mrt);
		satrec.error = 6;
		return false;
	}

	//#include "debug7.cpp"
	return true;
}  // sgp4
//------------------------------------------------------------------------------

	/* -----------------------------------------------------------------------------
	*
	*                           function getgravconst
	*
	*  this function gets constants for the propagator. note that mu is identified to
	*    facilitiate comparisons with newer models. the common useage is wgs72.
	*
	*  author        : david vallado                  719-573-2600   21 jul 2006
	*
	*  inputs        :
	*    whichconst  - which set of constants to use  wgs72old, wgs72, wgs84
	*
	*  outputs       :
	*    tumin       - minutes in one time unit
	*    mu          - earth gravitational parameter
	*    radiusearthkm - radius of the earth in km
	*    xke         - reciprocal of tumin
	*    j2, j3, j4  - un-normalized zonal harmonic values
	*    j3oj2       - j3 divided by j2
	*
	*  locals        :
	*
	*  coupling      :
	*    none
	*
	*  references    :
	*    norad spacetrack report #3
	*    vallado, crawford, hujsak, kelso  2006
	--------------------------------------------------------------------------- */

void eph_tle::getgravconst
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
)
{

	switch (whichconst)
	{
		// -- wgs-72 low precision str#3 constants --
	case wgs72old:
		mu = 398600.79964;        // in km3 / s2
		radiusearthkm = 6378.135;     // km
		xke = 0.0743669161;        // reciprocal of tumin
		tumin = 1.0 / xke;
		j2 = 0.001082616;
		j3 = -0.00000253881;
		j4 = -0.00000165597;
		j3oj2 = j3 / j2;
		break;
		// ------------ wgs-72 constants ------------
	case wgs72:
		mu = 398600.8;            // in km3 / s2
		radiusearthkm = 6378.135;     // km
		xke = 60.0 / sqrt(radiusearthkm * radiusearthkm * radiusearthkm / mu);
		tumin = 1.0 / xke;
		j2 = 0.001082616;
		j3 = -0.00000253881;
		j4 = -0.00000165597;
		j3oj2 = j3 / j2;
		break;
	case wgs84:
		// ------------ wgs-84 constants ------------
		mu = 398600.5;            // in km3 / s2
		radiusearthkm = 6378.137;     // km
		xke = 60.0 / sqrt(radiusearthkm * radiusearthkm * radiusearthkm / mu);
		tumin = 1.0 / xke;
		j2 = 0.00108262998905;
		j3 = -0.00000253215306;
		j4 = -0.00000161098761;
		j3oj2 = j3 / j2;
		break;
	default:
		//fprintf(stderr, "unknown gravity option (%d)\n", whichconst);
		break;
	}

}//getgravconst

//------------------------------------------------------------------------------
	/* -----------------------------------------------------------------------------
	*
	*                           function twoline2rv
	*
	*  this function converts the two line element set character string data to
	*    variables and initializes the sgp4 variables. several intermediate varaibles
	*    and quantities are determined. note that the result is a structure so multiple
	*    satellites can be processed simaltaneously without having to reinitialize. the
	*    verification mode is an important option that permits quick checks of any
	*    changes to the underlying technical theory. this option works using a
	*    modified tle file in which the start, stop, and delta time values are
	*    included at the end of the second line of data. this only works with the
	*    verification mode. the catalog mode simply propagates from -1440 to 1440 min
	*    from epoch and is useful when performing entire catalog runs.
	*
	*  author        : david vallado                  719-573-2600    1 mar 2001
	*
	*  inputs        :
	*    longstr1    - first line of the tle
	*    longstr2    - second line of the tle
	*    typerun     - type of run                    verification 'v', catalog 'c',
	*                                                 manual 'm'
	*    typeinput   - type of manual input           mfe 'm', epoch 'e', dayofyr 'd'
	*    opsmode     - mode of operation afspc or improved 'a', 'i'
	*    whichconst  - which set of constants to use  72, 84
	*
	*  outputs       :
	*    satrec      - structure containing all the sgp4 satellite information
	*
	*  coupling      :
	*    getgravconst-
	*    days2mdhms  - conversion of days to month, day, hour, minute, second
	*    jday        - convert day month year hour minute second into julian date
	*    sgp4init    - initialize the sgp4 variables
	*
	*  references    :
	*    norad spacetrack report #3
	*    vallado, crawford, hujsak, kelso  2006
	--------------------------------------------------------------------------- */

void eph_tle::twoline2rv
(
	char longstr1[130], char longstr2[130],
	char opsmode,
	gravconsttype whichconst,
	elsetrec& satrec
)
{
	const double xpdotp = 1440.0 / pi2;  // 229.1831180523293
	double sec;
	int cardnumb, j;
	// sgp4fix include in satrec
	// long revnum = 0, elnum = 0;
	// char classification, intldesg[11];
	int year = 0;
	int mon, day, hr, minute, nexp, ibexp;

	// sgp4fix no longer needed
	// getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );

	satrec.error = 0;

	// set the implied decimal points since doing a formated read
	// fixes for bad input data values (missing, ...)
	for (j = 10; j <= 15; j++)
		if (longstr1[j] == ' ')
			longstr1[j] = '_';

	if (longstr1[44] != ' ')
		longstr1[43] = longstr1[44];
	longstr1[44] = '.';
	if (longstr1[7] == ' ')
		longstr1[7] = 'U';
	if (longstr1[9] == ' ')
		longstr1[9] = '.';
	for (j = 45; j <= 49; j++)
		if (longstr1[j] == ' ')
			longstr1[j] = '0';
	if (longstr1[51] == ' ')
		longstr1[51] = '0';
	if (longstr1[53] != ' ')
		longstr1[52] = longstr1[53];
	longstr1[53] = '.';
	longstr2[25] = '.';
	for (j = 26; j <= 32; j++)
		if (longstr2[j] == ' ')
			longstr2[j] = '0';
	if (longstr1[62] == ' ')
		longstr1[62] = '0';
	if (longstr1[68] == ' ')
		longstr1[68] = '0';

	sscanf_s(longstr1, "%2d %5ld %1c %10s %2d %12lf %11lf %7lf %2d %7lf %2d %2d %6ld ",
		&cardnumb, &satrec.satnum, &satrec.classification, sizeof(char), &satrec.intldesg, 11 * sizeof(char), &satrec.epochyr,
		&satrec.epochdays, &satrec.ndot, &satrec.nddot, &nexp, &satrec.bstar, &ibexp, &satrec.ephtype, &satrec.elnum);

	if (longstr2[52] == ' ')
	{
		sscanf_s(longstr2, "%2d %5ld %9lf %9lf %8lf %9lf %9lf %10lf %6ld \n",
			&cardnumb, &satrec.satnum, &satrec.inclo,
			&satrec.nodeo, &satrec.ecco, &satrec.argpo, &satrec.mo, &satrec.no_kozai,
			&satrec.revnum);
	}
	else
	{
		sscanf_s(longstr2, "%2d %5ld %9lf %9lf %8lf %9lf %9lf %11lf %6ld \n",
			&cardnumb, &satrec.satnum, &satrec.inclo,
			&satrec.nodeo, &satrec.ecco, &satrec.argpo, &satrec.mo, &satrec.no_kozai,
			&satrec.revnum);
	}


	// ---- find no, ndot, nddot ----
	satrec.no_kozai = satrec.no_kozai / xpdotp; //* rad/min
	satrec.nddot = satrec.nddot * pow(10.0, nexp);
	satrec.bstar = satrec.bstar * pow(10.0, ibexp);

	// ---- convert to sgp4 units ----
	// satrec.a    = pow( satrec.no_kozai*tumin , (-2.0/3.0) );
	satrec.ndot = satrec.ndot / (xpdotp * 1440.0);  //* ? * minperday
	satrec.nddot = satrec.nddot / (xpdotp * 1440.0 * 1440);

	// ---- find standard orbital elements ----
	satrec.inclo = satrec.inclo * deg2rad;
	satrec.nodeo = satrec.nodeo * deg2rad;
	satrec.argpo = satrec.argpo * deg2rad;
	satrec.mo = satrec.mo * deg2rad;

	//sgp4fix not needed here
	//satrec.alta = satrec.a*(1.0 + satrec.ecco) - 1.0;
	//satrec.altp = satrec.a*(1.0 - satrec.ecco) - 1.0;

	//----------------------------------------------------------------
	// find sgp4epoch time of element set
	// remember that sgp4 uses units of days from 0 jan 1950 (sgp4epoch)
	// and minutes from the epoch (time)
	//----------------------------------------------------------------

	//---------------- temp fix for years from 1957-2056 -------------------
	//--------- correct fix will occur when year is 4-digit in tle ---------
	if (satrec.epochyr < 57)
		year = satrec.epochyr + 2000;
	else
		year = satrec.epochyr + 1900;

	days2mdhms(year, satrec.epochdays, mon, day, hr, minute, sec);
	jday(year, mon, day, hr, minute, sec, satrec.jdsatepoch, satrec.jdsatepochF);

	// ---------------- initialize the orbit at sgp4epoch -------------------
	sgp4init(whichconst, opsmode, satrec.satnum, (satrec.jdsatepoch + satrec.jdsatepochF) - 2433281.5, satrec.bstar,
		satrec.ndot, satrec.nddot, satrec.ecco, satrec.argpo, satrec.inclo, satrec.mo, satrec.no_kozai,
		satrec.nodeo, satrec);

} // twoline2rv

//------------------------------------------------------------------------------
double eph_tle::gstime(double jdut1)
{
	double tut1 = (jdut1 - 2451545.0) / 36525.0;
	double temp = -6.2e-6 * tut1 * tut1 * tut1 + 0.093104 * tut1 * tut1 +
		(876600.0 * 3600 + 8640184.812866) * tut1 + 67310.54841;  // sec
	temp = fmod(temp * deg2rad / 240.0, pi2); //360/86400 = 1/240, to deg, to rad

	// ------------------------ check quadrants ---------------------
	if (temp < 0.0)
		temp += pi2;

	return temp;
}
//------------------------------------------------------------------------------
double eph_tle::sgn(double x)
{
	if (x < 0.0)
	{
		return -1.0;
	}
	else
	{
		return 1.0;
	}

}
//------------------------------------------------------------------------------
double eph_tle::mag(double x[3])
{
	return sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
} 
//------------------------------------------------------------------------------
void eph_tle::cross(double vec1[3], double vec2[3], double outvec[3])
{
	outvec[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
	outvec[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
	outvec[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
}
//------------------------------------------------------------------------------
double  eph_tle::dot(double x[3], double y[3])
{
	return (x[0] * y[0] + x[1] * y[1] + x[2] * y[2]);
}
//------------------------------------------------------------------------------
double  eph_tle::angle(double vec1[3], double vec2[3])
{
	double _small = 0.00000001;
	double undefined = 999999.1;
	double magv1 = mag(vec1);
	double magv2 = mag(vec2);

	if (magv1 * magv2 > _small * _small)
	{
		double temp = dot(vec1, vec2) / (magv1 * magv2);
		if (fabs(temp) > 1.0)
			temp = sgn(temp) * 1.0;
		return acos(temp);
	}
	else
		return undefined;
}
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
	*
	*                           function newtonnu
	*
	*  this function solves keplers equation when the true anomaly is known.
	*    the mean and eccentric, parabolic, or hyperbolic anomaly is also found.
	*    the parabolic limit at 168ø is arbitrary. the hyperbolic anomaly is also
	*    limited. the hyperbolic sine is used because it's not double valued.
	*
	*  author        : david vallado                  719-573-2600   27 may 2002
	*
	*  revisions
	*    vallado     - fix small                                     24 sep 2002
	*
	*  inputs          description                    range / units
	*    ecc         - eccentricity                   0.0  to
	*    nu          - true anomaly                   -2pi to 2pi rad
	*
	*  outputs       :
	*    e0          - eccentric anomaly              0.0  to 2pi rad       153.02 ø
	*    m           - mean anomaly                   0.0  to 2pi rad       151.7425 ø
	*
	*  locals        :
	*    e1          - eccentric anomaly, next value  rad
	*    sine        - sine of e
	*    cose        - cosine of e
	*    ktr         - index
	*
	*  coupling      :
	*    asinh       - arc hyperbolic sine
	*
	*  references    :
	*    vallado       2013, 77, alg 5
	* --------------------------------------------------------------------------- */

void eph_tle::newtonnu(double ecc, double nu, double& e0, double& m)
{
	double sine, cose;

	// ---------------------  implementation   ---------------------
	e0 = 999999.9;
	m = 999999.9;
	double _small = 0.00000001;


	if (fabs(ecc) < _small)// --------------------------- circular ------------------------
	{
		m = nu;
		e0 = nu;
	}
	else if (ecc < 1.0 - _small)// ---------------------- elliptical -----------------------
	{
		sine = (sqrt(1.0 - ecc * ecc) * sin(nu)) / (1.0 + ecc * cos(nu));
		cose = (ecc + cos(nu)) / (1.0 + ecc * cos(nu));
		e0 = atan2(sine, cose);
		m = e0 - ecc * sin(e0);
	}
	else if (ecc > 1.0 + _small)// -------------------- hyperbolic  --------------------
	{
		if ((ecc > 1.0) && (fabs(nu) + 0.00001 < pi - acos(1.0 / ecc)))
		{
			sine = (sqrt(ecc * ecc - 1.0) * sin(nu)) / (1.0 + ecc * cos(nu));
			e0 = asinh(sine);
			m = ecc * sinh(e0) - e0;
		}
	}
	else if (fabs(nu) < 168.0 * pi / 180.0)// ----------------- parabolic ---------------------
	{
		e0 = tan(nu * 0.5);
		m = e0 + (e0 * e0 * e0) / 3.0;
	}

	if (ecc < 1.0)
	{
		m = fmod(m, pi2);
		if (m < 0.0)
			m = m + pi2;
		e0 = fmod(e0, pi2);
	}
}  // newtonnu
//------------------------------------------------------------------------------
double eph_tle::asinh(double xval)
{
	return log(xval + sqrt(xval * xval + 1.0));
}
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
	*
	*                           function rv2coe
	*
	*  this function finds the classical orbital elements given the geocentric
	*    equatorial position and velocity vectors.
	*
	*  author        : david vallado                  719-573-2600   21 jun 2002
	*
	*  revisions
	*    vallado     - fix special cases                              5 sep 2002
	*    vallado     - delete extra check in inclination code        16 oct 2002
	*    vallado     - add constant file use                         29 jun 2003
	*    vallado     - add mu                                         2 apr 2007
	*
	*  inputs          description                    range / units
	*    r           - ijk position vector            km
	*    v           - ijk velocity vector            km / s
	*    mu          - gravitational parameter        km3 / s2
	*
	*  outputs       :
	*    p           - semilatus rectum               km
	*    a           - semimajor axis                 km
	*    ecc         - eccentricity
	*    incl        - inclination                    0.0  to pi rad
	*    omega       - right ascension of ascending node    0.0  to 2pi rad
	*    argp        - argument of perigee            0.0  to 2pi rad
	*    nu          - true anomaly                   0.0  to 2pi rad
	*    m           - mean anomaly                   0.0  to 2pi rad
	*    arglat      - argument of latitude      (ci) 0.0  to 2pi rad
	*    truelon     - true longitude            (ce) 0.0  to 2pi rad
	*    lonper      - longitude of periapsis    (ee) 0.0  to 2pi rad
	*
	*  locals        :
	*    hbar        - angular momentum h vector      km2 / s
	*    ebar        - eccentricity     e vector
	*    nbar        - line of nodes    n vector
	*    c1          - v**2 - u/r
	*    rdotv       - r dot v
	*    hk          - hk unit vector
	*    sme         - specfic mechanical energy      km2 / s2
	*    i           - index
	*    e           - eccentric, parabolic,
	*                  hyperbolic anomaly             rad
	*    temp        - temporary variable
	*    typeorbit   - type of orbit                  ee, ei, ce, ci
	*
	*  coupling      :
	*    mag         - magnitude of a vector
	*    cross       - cross product of two vectors
	*    angle       - find the angle between two vectors
	*    newtonnu    - find the mean anomaly
	*
	*  references    :
	*    vallado       2013, 113, alg 9, ex 2-5
	* --------------------------------------------------------------------------- */

void eph_tle::rv2coe
(
	double r[3], double v[3], const double mu,
	double& p, double& a, double& ecc, double& incl, double& omega, double& argp,
	double& nu, double& m, double& arglat, double& truelon, double& lonper
)
{
	double hbar[3], nbar[3], magr, magv, magn, ebar[3], sme, rdotv, temp, c1, hk, magh, e;

	int i;
	// switch this to an integer msvs seems to have probelms with this and strncpy_s
	//char typeorbit[2];
	int typeorbit;
	// here 
	// typeorbit = 1 = 'ei'
	// typeorbit = 2 = 'ce'
	// typeorbit = 3 = 'ci'
	// typeorbit = 4 = 'ee'

	double _small = 0.00000001;
	double undefined = 999999.1;
	double infinite = 999999.9;

	// -------------------------  implementation   -----------------
	magr = mag(r);
	magv = mag(v);

	// ------------------  find h n and e vectors   ----------------
	cross(r, v, hbar);
	magh = mag(hbar);
	if (magh > _small)
	{
		nbar[0] = -hbar[1];
		nbar[1] = hbar[0];
		nbar[2] = 0.0;
		magn = mag(nbar);
		c1 = magv * magv - mu / magr;
		rdotv = dot(r, v);
		for (i = 0; i <= 2; i++)
			ebar[i] = (c1 * r[i] - rdotv * v[i]) / mu;

		ecc = mag(ebar);

		// ------------  find a e and semi-latus rectum   ----------
		sme = (magv * magv * 0.5) - (mu / magr);
		if (fabs(sme) > _small)
			a = -mu / (2.0 * sme);
		else
			a = infinite;

		p = magh * magh / mu;

		// -----------------  find inclination   -------------------
		hk = hbar[2] / magh;
		incl = acos(hk);

		// --------  determine type of orbit for later use  --------
		// ------ elliptical, parabolic, hyperbolic inclined -------
		//#ifdef _MSC_VER  // chk if compiling under MSVS
		//		   strcpy_s(typeorbit, 2 * sizeof(char), "ei");
		//#else
		//		   strcpy(typeorbit, "ei");
		//#endif
		typeorbit = 1;

		if (ecc < _small)
		{
			// ----------------  circular equatorial ---------------
			if ((incl < _small) | (fabs(incl - pi) < _small))
			{
				//#ifdef _MSC_VER
				//				   strcpy_s(typeorbit, sizeof(typeorbit), "ce");
				//#else
				//				   strcpy(typeorbit, "ce");
				//#endif
				typeorbit = 2;
			}
			else
			{
				// --------------  circular inclined ---------------
				//#ifdef _MSC_VER
				//				   strcpy_s(typeorbit, sizeof(typeorbit), "ci");
				//#else
				//				   strcpy(typeorbit, "ci");
				//#endif
				typeorbit = 3;
			}
		}
		else
		{
			// - elliptical, parabolic, hyperbolic equatorial --
			if ((incl < _small) | (fabs(incl - pi) < _small)) {
				//#ifdef _MSC_VER
				//				   strcpy_s(typeorbit, sizeof(typeorbit), "ee");
				//#else
				//				   strcpy(typeorbit, "ee");
				//#endif
				typeorbit = 4;
			}
		}

		// ----------  find right ascension of the ascending node ------------
		if (magn > _small)
		{
			temp = nbar[0] / magn;
			if (fabs(temp) > 1.0)
				temp = sgn(temp);
			omega = acos(temp);
			if (nbar[1] < 0.0)
				omega = pi2 - omega;
		}
		else
			omega = undefined;

		// ---------------- find argument of perigee ---------------
		//if (strcmp(typeorbit, "ei") == 0)
		if (typeorbit == 1)
		{
			argp = angle(nbar, ebar);
			if (ebar[2] < 0.0)
				argp = pi2 - argp;
		}
		else
			argp = undefined;

		// ------------  find true anomaly at epoch    -------------
		//if (typeorbit[0] == 'e')
		if ((typeorbit == 1) || (typeorbit == 4))
		{
			nu = angle(ebar, r);
			if (rdotv < 0.0)
				nu = pi2 - nu;
		}
		else
			nu = undefined;

		// ----  find argument of latitude - circular inclined -----
		//if (strcmp(typeorbit, "ci") == 0)
		if (typeorbit == 3)
		{
			arglat = angle(nbar, r);
			if (r[2] < 0.0)
				arglat = pi2 - arglat;
			m = arglat;
		}
		else
			arglat = undefined;

		// -- find longitude of perigee - elliptical equatorial ----
		//if ((ecc>_small) && (strcmp(typeorbit, "ee") == 0))
		if ((ecc > _small) && (typeorbit == 4))
		{
			temp = ebar[0] / ecc;
			if (fabs(temp) > 1.0)
				temp = sgn(temp);
			lonper = acos(temp);
			if (ebar[1] < 0.0)
				lonper = pi2 - lonper;
			if (incl > pi_half)
				lonper = pi2 - lonper;
		}
		else
			lonper = undefined;

		// -------- find true longitude - circular equatorial ------
		//if ((magr>_small) && (strcmp(typeorbit, "ce") == 0))
		if ((magr > _small) && (typeorbit == 2))
		{
			temp = r[0] / magr;
			if (fabs(temp) > 1.0)
				temp = sgn(temp);
			truelon = acos(temp);
			if (r[1] < 0.0)
				truelon = pi2 - truelon;
			if (incl > pi_half)
				truelon = pi2 - truelon;
			m = truelon;
		}
		else
			truelon = undefined;

		// ------------ find mean anomaly for all orbits -----------
		//if (typeorbit[0] == 'e')
		if ((typeorbit == 1) || (typeorbit == 4))
			newtonnu(ecc, nu, e, m);
	}
	else
	{
		p = undefined;
		a = undefined;
		ecc = undefined;
		incl = undefined;
		omega = undefined;
		argp = undefined;
		nu = undefined;
		m = undefined;
		arglat = undefined;
		truelon = undefined;
		lonper = undefined;
	}
}  // rv2coe
//------------------------------------------------------------------------------

	/* -----------------------------------------------------------------------------
	*
	*                           procedure jday
	*
	*  this procedure finds the julian date given the year, month, day, and time.
	*    the julian date is defined by each elapsed day since noon, jan 1, 4713 bc.
	*
	*  algorithm     : calculate the answer in one step for efficiency
	*
	*  author        : david vallado                  719-573-2600    1 mar 2001
	*
	*  inputs          description                    range / units
	*    year        - year                           1900 .. 2100
	*    mon         - month                          1 .. 12
	*    day         - day                            1 .. 28,29,30,31
	*    hr          - universal time hour            0 .. 23
	*    min         - universal time min             0 .. 59
	*    sec         - universal time sec             0.0 .. 59.999
	*
	*  outputs       :
	*    jd          - julian date                    days from 4713 bc
	*    jdfrac      - julian date fraction into day  days from 4713 bc
	*
	*  locals        :
	*    none.
	*
	*  coupling      :
	*    none.
	*
	*  references    :
	*    vallado       2013, 183, alg 14, ex 3-4
	* --------------------------------------------------------------------------- */

void eph_tle::jday(int year, int mon, int day, int hr, int minute, double sec, double& jd, double& jdFrac)
{
	jd = 367.0 * year -
		floor((7 * (year + floor((mon + 9) / 12.0))) * 0.25) +
		floor(275 * mon / 9.0) +
		day + 1721013.5;  // use - 678987.0 to go to mjd directly
	jdFrac = (sec + minute * 60.0 + hr * 3600.0) / 86400.0;

	// check that the day and fractional day are correct
	if (fabs(jdFrac) > 1.0)
	{
		double dtt = floor(jdFrac);
		jd = jd + dtt;
		jdFrac = jdFrac - dtt;
	}

	// - 0.5*sgn(100.0*year + mon - 190002.5) + 0.5;
}  // jday
//------------------------------------------------------------------------------

	/* -----------------------------------------------------------------------------
	*
	*                           procedure days2mdhms
	*
	*  this procedure converts the day of the year, days, to the equivalent month
	*    day, hour, minute and second.
	*
	*  algorithm     : set up array for the number of days per month
	*                  find leap year - use 1900 because 2000 is a leap year
	*                  loop through a temp value while the value is < the days
	*                  perform int conversions to the correct day and month
	*                  convert remainder into h m s using type conversions
	*
	*  author        : david vallado                  719-573-2600    1 mar 2001
	*
	*  inputs          description                    range / units
	*    year        - year                           1900 .. 2100
	*    days        - julian day of the year         1.0  .. 366.0
	*
	*  outputs       :
	*    mon         - month                          1 .. 12
	*    day         - day                            1 .. 28,29,30,31
	*    hr          - hour                           0 .. 23
	*    min         - minute                         0 .. 59
	*    sec         - second                         0.0 .. 59.999
	*
	*  locals        :
	*    dayofyr     - day of year
	*    temp        - temporary extended values
	*    inttemp     - temporary int value
	*    i           - index
	*    lmonth[13]  - int array containing the number of days per month
	*
	*  coupling      :
	*    none.
	* --------------------------------------------------------------------------- */

void  eph_tle::days2mdhms(int year, double days, int& mon, int& day, int& hr, int& minute, double& sec)
{
	int i, inttemp, dayofyr;
	double    temp;
	int lmonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	dayofyr = (int)floor(days);
	/* ----------------- find month and day of month ---------------- */
	if ((year % 4) == 0)
		lmonth[2] = 29;

	i = 1;
	inttemp = 0;
	while ((dayofyr > inttemp + lmonth[i]) && (i < 12))
	{
		inttemp = inttemp + lmonth[i];
		i++;
	}
	mon = i;
	day = dayofyr - inttemp;

	/* ----------------- find hours minutes and seconds ------------- */
	temp = (days - dayofyr) * 24.0;
	hr = (int)floor(temp);
	temp = (temp - hr) * 60.0;
	minute = (int)floor(temp);
	sec = (temp - minute) * 60.0;
}  // days2mdhms
//------------------------------------------------------------------------------

	/* -----------------------------------------------------------------------------
	*
	*                           procedure invjday
	*
	*  this procedure finds the year, month, day, hour, minute and second
	*  given the julian date. tu can be ut1, tdt, tdb, etc.
	*
	*  algorithm     : set up starting values
	*                  find leap year - use 1900 because 2000 is a leap year
	*                  find the elapsed days through the year in a loop
	*                  call routine to find each individual value
	*
	*  author        : david vallado                  719-573-2600    1 mar 2001
	*
	*  inputs          description                    range / units
	*    jd          - julian date                    days from 4713 bc
	*    jdfrac      - julian date fraction into day  days from 4713 bc
	*
	*  outputs       :
	*    year        - year                           1900 .. 2100
	*    mon         - month                          1 .. 12
	*    day         - day                            1 .. 28,29,30,31
	*    hr          - hour                           0 .. 23
	*    min         - minute                         0 .. 59
	*    sec         - second                         0.0 .. 59.999
	*
	*  locals        :
	*    days        - day of year plus fractional
	*                  portion of a day               days
	*    tu          - julian centuries from 0 h
	*                  jan 0, 1900
	*    temp        - temporary double values
	*    leapyrs     - number of leap years from 1900
	*
	*  coupling      :
	*    days2mdhms  - finds month, day, hour, minute and second given days and year
	*
	*  references    :
	*    vallado       2013, 203, alg 22, ex 3-13
	* --------------------------------------------------------------------------- */

void eph_tle::invjday(double jd, double jdfrac, int& year, int& mon, int& day, int& hr, int& minute, double& sec)
{
	int leapyrs;
	double dt, days, tu, temp;

	// check jdfrac for multiple days
	if (fabs(jdfrac) >= 1.0)
	{
		jd = jd + floor(jdfrac);
		jdfrac = jdfrac - floor(jdfrac);
	}

	// check for fraction of a day included in the jd
	dt = jd - floor(jd) - 0.5;
	if (fabs(dt) > 0.00000001)
	{
		jd = jd - dt;
		jdfrac = jdfrac + dt;
	}

	/* --------------- find year and days of the year --------------- */
	temp = jd - 2415019.5;
	tu = temp / 365.25;
	year = 1900 + (int)floor(tu);
	leapyrs = (int)floor((year - 1901) * 0.25);

	days = floor(temp - ((year - 1900) * 365.0 + leapyrs));

	/* ------------ check for case of beginning of a year ----------- */
	if (days + jdfrac < 1.0)
	{
		year = year - 1;
		leapyrs = (int)floor((year - 1901) * 0.25);
		days = floor(temp - ((year - 1900) * 365.0 + leapyrs));
	}

	/* ----------------- find remaining data  ------------------------- */
	days2mdhms(year, days + jdfrac, mon, day, hr, minute, sec);
}  // invjday
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
	*
	*                           procedure dpper
	*
	*  this procedure provides deep space long period periodic contributions
	*    to the mean elements.  by design, these periodics are zero at epoch.
	*    this used to be dscom which included initialization, but it's really a
	*    recurring function.
	*
	*  author        : david vallado                  719-573-2600   28 jun 2005
	*
	*  inputs        :
	*    e3          -
	*    ee2         -
	*    peo         -
	*    pgho        -
	*    pho         -
	*    pinco       -
	*    plo         -
	*    se2 , se3 , sgh2, sgh3, sgh4, sh2, sh3, si2, si3, sl2, sl3, sl4 -
	*    t           -
	*    xh2, xh3, xi2, xi3, xl2, xl3, xl4 -
	*    zmol        -
	*    zmos        -
	*    ep          - eccentricity                           0.0 - 1.0
	*    inclo       - inclination - needed for lyddane modification
	*    nodep       - right ascension of ascending node
	*    argpp       - argument of perigee
	*    mp          - mean anomaly
	*
	*  outputs       :
	*    ep          - eccentricity                           0.0 - 1.0
	*    inclp       - inclination
	*    nodep        - right ascension of ascending node
	*    argpp       - argument of perigee
	*    mp          - mean anomaly
	*
	*  locals        :
	*    alfdp       -
	*    betdp       -
	*    cosip  , sinip  , cosop  , sinop  ,
	*    dalf        -
	*    dbet        -
	*    dls         -
	*    f2, f3      -
	*    pe          -
	*    pgh         -
	*    ph          -
	*    pinc        -
	*    pl          -
	*    sel   , ses   , sghl  , sghs  , shl   , shs   , sil   , sinzf , sis   ,
	*    sll   , sls
	*    xls         -
	*    xnoh        -
	*    zf          -
	*    zm          -
	*
	*  coupling      :
	*    none.
	*
	*  references    :
	*    hoots, roehrich, norad spacetrack report #3 1980
	*    hoots, norad spacetrack report #6 1986
	*    hoots, schumacher and glover 2004
	*    vallado, crawford, hujsak, kelso  2006
	----------------------------------------------------------------------------*/

void eph_tle::dpper
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
)
{
	/* --------------------- local variables ------------------------ */
	double alfdp, betdp, cosip, cosop, dalf, dbet, dls,
		f2, f3, pe, pgh, ph, pinc, pl,
		sel, ses, sghl, sghs, shll, shs, sil,
		sinip, sinop, sinzf, sis, sll, sls, xls,
		xnoh, zf, zm, zel, zes, znl, zns;

	/* ---------------------- constants ----------------------------- */
	zns = 1.19459e-5;
	zes = 0.01675;
	znl = 1.5835218e-4;
	zel = 0.05490;

	/* --------------- calculate time varying periodics ----------- */
	zm = zmos + zns * t;
	// be sure that the initial call has time set to zero
	if (initialize)
		zm = zmos;
	zf = zm + 2.0 * zes * sin(zm);
	sinzf = sin(zf);
	f2 = 0.5 * sinzf * sinzf - 0.25;
	f3 = -0.5 * sinzf * cos(zf);
	ses = se2 * f2 + se3 * f3;
	sis = si2 * f2 + si3 * f3;
	sls = sl2 * f2 + sl3 * f3 + sl4 * sinzf;
	sghs = sgh2 * f2 + sgh3 * f3 + sgh4 * sinzf;
	shs = sh2 * f2 + sh3 * f3;
	zm = zmol + znl * t;

	if (initialize)
		zm = zmol;
	zf = zm + 2.0 * zel * sin(zm);
	sinzf = sin(zf);
	f2 = 0.5 * sinzf * sinzf - 0.25;
	f3 = -0.5 * sinzf * cos(zf);
	sel = ee2 * f2 + e3 * f3;
	sil = xi2 * f2 + xi3 * f3;
	sll = xl2 * f2 + xl3 * f3 + xl4 * sinzf;
	sghl = xgh2 * f2 + xgh3 * f3 + xgh4 * sinzf;
	shll = xh2 * f2 + xh3 * f3;
	pe = ses + sel;
	pinc = sis + sil;
	pl = sls + sll;
	pgh = sghs + sghl;
	ph = shs + shll;

	if (!initialize)
	{
		pe = pe - peo;
		pinc = pinc - pinco;
		pl = pl - plo;
		pgh = pgh - pgho;
		ph = ph - pho;
		inclp = inclp + pinc;
		ep = ep + pe;
		sinip = sin(inclp);
		cosip = cos(inclp);

		/* ----------------- apply periodics directly ------------ */
		//  sgp4fix for lyddane choice
		//  strn3 used original inclination - this is technically feasible
		//  gsfc used perturbed inclination - also technically feasible
		//  probably best to readjust the 0.2 limit value and limit discontinuity
		//  0.2 rad = 11.45916 deg
		//  use next line for original strn3 approach and original inclination
		//  if (inclo >= 0.2)
		//  use next line for gsfc version and perturbed inclination
		if (inclp >= 0.2)
		{
			ph = ph / sinip;
			pgh = pgh - cosip * ph;
			argpp = argpp + pgh;
			nodep = nodep + ph;
			mp = mp + pl;
		}
		else
		{
			/* ---- apply periodics with lyddane modification ---- */
			sinop = sin(nodep);
			cosop = cos(nodep);
			alfdp = sinip * sinop;
			betdp = sinip * cosop;
			dalf = ph * cosop + pinc * cosip * sinop;
			dbet = -ph * sinop + pinc * cosip * cosop;
			alfdp = alfdp + dalf;
			betdp = betdp + dbet;
			nodep = fmod(nodep, pi2);
			//  sgp4fix for afspc written intrinsic functions
			// nodep used without a trigonometric function ahead
			if ((nodep < 0.0) && (opsmode == 'a'))
				nodep = nodep + pi2;
			xls = mp + argpp + cosip * nodep;
			dls = pl + pgh - pinc * nodep * sinip;
			xls = xls + dls;
			xnoh = nodep;
			nodep = atan2(alfdp, betdp);
			//  sgp4fix for afspc written intrinsic functions
			// nodep used without a trigonometric function ahead
			if ((nodep < 0.0) && (opsmode == 'a'))
				nodep = nodep + pi2;

			if (fabs(xnoh - nodep) > pi)
			{
				if (nodep < xnoh)
					nodep = nodep + pi2;
				else
					nodep = nodep - pi2;
			}

			mp = mp + pl;
			argpp = xls - mp - cosip * nodep;
		}
	}

	//#include "debug1.cpp"
}  // dpper
//------------------------------------------------------------------------------

/*-----------------------------------------------------------------------------
	*
	*                           procedure dscom
	*
	*  this procedure provides deep space common items used by both the secular
	*    and periodics subroutines.  input is provided as shown. this routine
	*    used to be called dpper, but the functions inside weren't well organized.
	*
	*  author        : david vallado                  719-573-2600   28 jun 2005
	*
	*  inputs        :
	*    epoch       -
	*    ep          - eccentricity
	*    argpp       - argument of perigee
	*    tc          -
	*    inclp       - inclination
	*    nodep       - right ascension of ascending node
	*    np          - mean motion
	*
	*  outputs       :
	*    sinim  , cosim  , sinomm , cosomm , snodm  , cnodm
	*    day         -
	*    e3          -
	*    ee2         -
	*    em          - eccentricity
	*    emsq        - eccentricity squared
	*    gam         -
	*    peo         -
	*    pgho        -
	*    pho         -
	*    pinco       -
	*    plo         -
	*    rtemsq      -
	*    se2, se3         -
	*    sgh2, sgh3, sgh4        -
	*    sh2, sh3, si2, si3, sl2, sl3, sl4         -
	*    s1, s2, s3, s4, s5, s6, s7          -
	*    ss1, ss2, ss3, ss4, ss5, ss6, ss7, sz1, sz2, sz3         -
	*    sz11, sz12, sz13, sz21, sz22, sz23, sz31, sz32, sz33        -
	*    xgh2, xgh3, xgh4, xh2, xh3, xi2, xi3, xl2, xl3, xl4         -
	*    nm          - mean motion
	*    z1, z2, z3, z11, z12, z13, z21, z22, z23, z31, z32, z33         -
	*    zmol        -
	*    zmos        -
	*
	*  locals        :
	*    a1, a2, a3, a4, a5, a6, a7, a8, a9, a10         -
	*    betasq      -
	*    cc          -
	*    ctem, stem        -
	*    x1, x2, x3, x4, x5, x6, x7, x8          -
	*    xnodce      -
	*    xnoi        -
	*    zcosg  , zsing  , zcosgl , zsingl , zcosh  , zsinh  , zcoshl , zsinhl ,
	*    zcosi  , zsini  , zcosil , zsinil ,
	*    zx          -
	*    zy          -
	*
	*  coupling      :
	*    none.
	*
	*  references    :
	*    hoots, roehrich, norad spacetrack report #3 1980
	*    hoots, norad spacetrack report #6 1986
	*    hoots, schumacher and glover 2004
	*    vallado, crawford, hujsak, kelso  2006
	----------------------------------------------------------------------------*/

void eph_tle::dscom
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
)
{
	/* -------------------------- constants ------------------------- */
	const double zes = 0.01675;
	const double zel = 0.05490;
	const double c1ss = 2.9864797e-6;
	const double c1l = 4.7968065e-7;
	const double zsinis = 0.39785416;
	const double zcosis = 0.91744867;
	const double zcosgs = 0.1945905;
	const double zsings = -0.98088458;

	/* --------------------- local variables ------------------------ */
	int lsflg;
	double a1, a2, a3, a4, a5, a6, a7,
		a8, a9, a10, betasq, cc, ctem, stem,
		x1, x2, x3, x4, x5, x6, x7,
		x8, xnodce, xnoi, zcosg, zcosgl, zcosh, zcoshl,
		zcosi, zcosil, zsing, zsingl, zsinh, zsinhl, zsini,
		zsinil, zx, zy;

	nm = np;
	em = ep;
	snodm = sin(nodep);
	cnodm = cos(nodep);
	sinomm = sin(argpp);
	cosomm = cos(argpp);
	sinim = sin(inclp);
	cosim = cos(inclp);
	emsq = em * em;
	betasq = 1.0 - emsq;
	rtemsq = sqrt(betasq);

	/* ----------------- initialize lunar solar terms --------------- */
	peo = 0.0;
	pinco = 0.0;
	plo = 0.0;
	pgho = 0.0;
	pho = 0.0;
	day = epoch + 18261.5 + tc / 1440.0;
	xnodce = fmod(4.5236020 - 9.2422029e-4 * day, pi2);
	stem = sin(xnodce);
	ctem = cos(xnodce);
	zcosil = 0.91375164 - 0.03568096 * ctem;
	zsinil = sqrt(1.0 - zcosil * zcosil);
	zsinhl = 0.089683511 * stem / zsinil;
	zcoshl = sqrt(1.0 - zsinhl * zsinhl);
	gam = 5.8351514 + 0.0019443680 * day;
	zx = 0.39785416 * stem / zsinil;
	zy = zcoshl * ctem + 0.91744867 * zsinhl * stem;
	zx = atan2(zx, zy);
	zx = gam + zx - xnodce;
	zcosgl = cos(zx);
	zsingl = sin(zx);

	/* ------------------------- do solar terms --------------------- */
	zcosg = zcosgs;
	zsing = zsings;
	zcosi = zcosis;
	zsini = zsinis;
	zcosh = cnodm;
	zsinh = snodm;
	cc = c1ss;
	xnoi = 1.0 / nm;

	for (lsflg = 1; lsflg <= 2; lsflg++)
	{
		a1 = zcosg * zcosh + zsing * zcosi * zsinh;
		a3 = -zsing * zcosh + zcosg * zcosi * zsinh;
		a7 = -zcosg * zsinh + zsing * zcosi * zcosh;
		a8 = zsing * zsini;
		a9 = zsing * zsinh + zcosg * zcosi * zcosh;
		a10 = zcosg * zsini;
		a2 = cosim * a7 + sinim * a8;
		a4 = cosim * a9 + sinim * a10;
		a5 = -sinim * a7 + cosim * a8;
		a6 = -sinim * a9 + cosim * a10;

		x1 = a1 * cosomm + a2 * sinomm;
		x2 = a3 * cosomm + a4 * sinomm;
		x3 = -a1 * sinomm + a2 * cosomm;
		x4 = -a3 * sinomm + a4 * cosomm;
		x5 = a5 * sinomm;
		x6 = a6 * sinomm;
		x7 = a5 * cosomm;
		x8 = a6 * cosomm;

		z31 = 12.0 * x1 * x1 - 3.0 * x3 * x3;
		z32 = 24.0 * x1 * x2 - 6.0 * x3 * x4;
		z33 = 12.0 * x2 * x2 - 3.0 * x4 * x4;
		z1 = 3.0 * (a1 * a1 + a2 * a2) + z31 * emsq;
		z2 = 6.0 * (a1 * a3 + a2 * a4) + z32 * emsq;
		z3 = 3.0 * (a3 * a3 + a4 * a4) + z33 * emsq;
		z11 = -6.0 * a1 * a5 + emsq * (-24.0 * x1 * x7 - 6.0 * x3 * x5);
		z12 = -6.0 * (a1 * a6 + a3 * a5) + emsq *
			(-24.0 * (x2 * x7 + x1 * x8) - 6.0 * (x3 * x6 + x4 * x5));
		z13 = -6.0 * a3 * a6 + emsq * (-24.0 * x2 * x8 - 6.0 * x4 * x6);
		z21 = 6.0 * a2 * a5 + emsq * (24.0 * x1 * x5 - 6.0 * x3 * x7);
		z22 = 6.0 * (a4 * a5 + a2 * a6) + emsq *
			(24.0 * (x2 * x5 + x1 * x6) - 6.0 * (x4 * x7 + x3 * x8));
		z23 = 6.0 * a4 * a6 + emsq * (24.0 * x2 * x6 - 6.0 * x4 * x8);
		z1 = z1 + z1 + betasq * z31;
		z2 = z2 + z2 + betasq * z32;
		z3 = z3 + z3 + betasq * z33;
		s3 = cc * xnoi;
		s2 = -0.5 * s3 / rtemsq;
		s4 = s3 * rtemsq;
		s1 = -15.0 * em * s4;
		s5 = x1 * x3 + x2 * x4;
		s6 = x2 * x3 + x1 * x4;
		s7 = x2 * x4 - x1 * x3;

		/* ----------------------- do lunar terms ------------------- */
		if (lsflg == 1)
		{
			ss1 = s1;
			ss2 = s2;
			ss3 = s3;
			ss4 = s4;
			ss5 = s5;
			ss6 = s6;
			ss7 = s7;
			sz1 = z1;
			sz2 = z2;
			sz3 = z3;
			sz11 = z11;
			sz12 = z12;
			sz13 = z13;
			sz21 = z21;
			sz22 = z22;
			sz23 = z23;
			sz31 = z31;
			sz32 = z32;
			sz33 = z33;
			zcosg = zcosgl;
			zsing = zsingl;
			zcosi = zcosil;
			zsini = zsinil;
			zcosh = zcoshl * cnodm + zsinhl * snodm;
			zsinh = snodm * zcoshl - cnodm * zsinhl;
			cc = c1l;
		}
	}

	zmol = fmod(4.7199672 + 0.22997150 * day - gam, pi2);
	zmos = fmod(6.2565837 + 0.017201977 * day, pi2);

	/* ------------------------ do solar terms ---------------------- */
	se2 = 2.0 * ss1 * ss6;
	se3 = 2.0 * ss1 * ss7;
	si2 = 2.0 * ss2 * sz12;
	si3 = 2.0 * ss2 * (sz13 - sz11);
	sl2 = -2.0 * ss3 * sz2;
	sl3 = -2.0 * ss3 * (sz3 - sz1);
	sl4 = -2.0 * ss3 * (-21.0 - 9.0 * emsq) * zes;
	sgh2 = 2.0 * ss4 * sz32;
	sgh3 = 2.0 * ss4 * (sz33 - sz31);
	sgh4 = -18.0 * ss4 * zes;
	sh2 = -2.0 * ss2 * sz22;
	sh3 = -2.0 * ss2 * (sz23 - sz21);

	/* ------------------------ do lunar terms ---------------------- */
	ee2 = 2.0 * s1 * s6;
	e3 = 2.0 * s1 * s7;
	xi2 = 2.0 * s2 * z12;
	xi3 = 2.0 * s2 * (z13 - z11);
	xl2 = -2.0 * s3 * z2;
	xl3 = -2.0 * s3 * (z3 - z1);
	xl4 = -2.0 * s3 * (-21.0 - 9.0 * emsq) * zel;
	xgh2 = 2.0 * s4 * z32;
	xgh3 = 2.0 * s4 * (z33 - z31);
	xgh4 = -18.0 * s4 * zel;
	xh2 = -2.0 * s2 * z22;
	xh3 = -2.0 * s2 * (z23 - z21);

	//#include "debug2.cpp"
}  // dscom
//------------------------------------------------------------------------------

/*-----------------------------------------------------------------------------
	*
	*                           procedure dsinit
	*
	*  this procedure provides deep space contributions to mean motion dot due
	*    to geopotential resonance with half day and one day orbits.
	*
	*  author        : david vallado                  719-573-2600   28 jun 2005
	*
	*  inputs        :
	*    xke         - reciprocal of tumin
	*    cosim, sinim-
	*    emsq        - eccentricity squared
	*    argpo       - argument of perigee
	*    s1, s2, s3, s4, s5      -
	*    ss1, ss2, ss3, ss4, ss5 -
	*    sz1, sz3, sz11, sz13, sz21, sz23, sz31, sz33 -
	*    t           - time
	*    tc          -
	*    gsto        - greenwich sidereal time                   rad
	*    mo          - mean anomaly
	*    mdot        - mean anomaly dot (rate)
	*    no          - mean motion
	*    nodeo       - right ascension of ascending node
	*    nodedot     - right ascension of ascending node dot (rate)
	*    xpidot      -
	*    z1, z3, z11, z13, z21, z23, z31, z33 -
	*    eccm        - eccentricity
	*    argpm       - argument of perigee
	*    inclm       - inclination
	*    mm          - mean anomaly
	*    xn          - mean motion
	*    nodem       - right ascension of ascending node
	*
	*  outputs       :
	*    em          - eccentricity
	*    argpm       - argument of perigee
	*    inclm       - inclination
	*    mm          - mean anomaly
	*    nm          - mean motion
	*    nodem       - right ascension of ascending node
	*    irez        - flag for resonance           0-none, 1-one day, 2-half day
	*    atime       -
	*    d2201, d2211, d3210, d3222, d4410, d4422, d5220, d5232, d5421, d5433    -
	*    dedt        -
	*    didt        -
	*    dmdt        -
	*    dndt        -
	*    dnodt       -
	*    domdt       -
	*    del1, del2, del3        -
	*    ses  , sghl , sghs , sgs  , shl  , shs  , sis  , sls
	*    theta       -
	*    xfact       -
	*    xlamo       -
	*    xli         -
	*    xni
	*
	*  locals        :
	*    ainv2       -
	*    aonv        -
	*    cosisq      -
	*    eoc         -
	*    f220, f221, f311, f321, f322, f330, f441, f442, f522, f523, f542, f543  -
	*    g200, g201, g211, g300, g310, g322, g410, g422, g520, g521, g532, g533  -
	*    sini2       -
	*    temp        -
	*    temp1       -
	*    theta       -
	*    xno2        -
	*
	*  coupling      :
	*    getgravconst- no longer used
	*
	*  references    :
	*    hoots, roehrich, norad spacetrack report #3 1980
	*    hoots, norad spacetrack report #6 1986
	*    hoots, schumacher and glover 2004
	*    vallado, crawford, hujsak, kelso  2006
	----------------------------------------------------------------------------*/

void eph_tle::dsinit
(
	// sgp4fix just send in xke as a constant and eliminate getgravconst call
	// gravconsttype whichconst, 
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
)
{
	/* --------------------- local variables ------------------------ */
	double ainv2, aonv = 0.0, cosisq, eoc, f220, f221, f311,
		f321, f322, f330, f441, f442, f522, f523,
		f542, f543, g200, g201, g211, g300, g310,
		g322, g410, g422, g520, g521, g532, g533,
		ses, sgs, sghl, sghs, shs, shll, sis,
		sini2, sls, temp, temp1, theta, xno2, q22,
		q31, q33, root22, root44, root54, rptim, root32,
		root52, x2o3, znl, emo, zns, emsqo;

	q22 = 1.7891679e-6;
	q31 = 2.1460748e-6;
	q33 = 2.2123015e-7;
	root22 = 1.7891679e-6;
	root44 = 7.3636953e-9;
	root54 = 2.1765803e-9;
	rptim = 4.37526908801129966e-3; // this equates to 7.29211514668855e-5 rad/sec
	root32 = 3.7393792e-7;
	root52 = 1.1428639e-7;
	x2o3 = 2.0 / 3.0;
	znl = 1.5835218e-4;
	zns = 1.19459e-5;

	// sgp4fix identify constants and allow alternate values
	// just xke is used here so pass it in rather than have multiple calls
	// getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );

	/* -------------------- deep space initialization ------------ */
	irez = 0;
	if ((nm < 0.0052359877) && (nm > 0.0034906585))
		irez = 1;
	if ((nm >= 8.26e-3) && (nm <= 9.24e-3) && (em >= 0.5))
		irez = 2;

	/* ------------------------ do solar terms ------------------- */
	ses = ss1 * zns * ss5;
	sis = ss2 * zns * (sz11 + sz13);
	sls = -zns * ss3 * (sz1 + sz3 - 14.0 - 6.0 * emsq);
	sghs = ss4 * zns * (sz31 + sz33 - 6.0);
	shs = -zns * ss2 * (sz21 + sz23);
	// sgp4fix for 180 deg incl
	if ((inclm < 5.2359877e-2) || (inclm > pi - 5.2359877e-2))
		shs = 0.0;
	if (sinim != 0.0)
		shs = shs / sinim;
	sgs = sghs - cosim * shs;

	/* ------------------------- do lunar terms ------------------ */
	dedt = ses + s1 * znl * s5;
	didt = sis + s2 * znl * (z11 + z13);
	dmdt = sls - znl * s3 * (z1 + z3 - 14.0 - 6.0 * emsq);
	sghl = s4 * znl * (z31 + z33 - 6.0);
	shll = -znl * s2 * (z21 + z23);
	// sgp4fix for 180 deg incl
	if ((inclm < 5.2359877e-2) || (inclm > pi - 5.2359877e-2))
		shll = 0.0;
	domdt = sgs + sghl;
	dnodt = shs;
	if (sinim != 0.0)
	{
		domdt = domdt - cosim / sinim * shll;
		dnodt = dnodt + shll / sinim;
	}

	/* ----------- calculate deep space resonance effects -------- */
	dndt = 0.0;
	theta = fmod(gsto + tc * rptim, pi2);
	em = em + dedt * t;
	inclm = inclm + didt * t;
	argpm = argpm + domdt * t;
	nodem = nodem + dnodt * t;
	mm = mm + dmdt * t;
	//   sgp4fix for negative inclinations
	//   the following if statement should be commented out
	//if (inclm < 0.0)
	//  {
	//    inclm  = -inclm;
	//    argpm  = argpm - pi;
	//    nodem = nodem + pi;
	//  }

	/* -------------- initialize the resonance terms ------------- */
	if (irez != 0)
	{
		aonv = pow(nm / xke, x2o3);

		/* ---------- geopotential resonance for 12 hour orbits ------ */
		if (irez == 2)
		{
			cosisq = cosim * cosim;
			emo = em;
			em = ecco;
			emsqo = emsq;
			emsq = eccsq;
			eoc = em * emsq;
			g201 = -0.306 - (em - 0.64) * 0.440;

			if (em <= 0.65)
			{
				g211 = 3.616 - 13.2470 * em + 16.2900 * emsq;
				g310 = -19.302 + 117.3900 * em - 228.4190 * emsq + 156.5910 * eoc;
				g322 = -18.9068 + 109.7927 * em - 214.6334 * emsq + 146.5816 * eoc;
				g410 = -41.122 + 242.6940 * em - 471.0940 * emsq + 313.9530 * eoc;
				g422 = -146.407 + 841.8800 * em - 1629.014 * emsq + 1083.4350 * eoc;
				g520 = -532.114 + 3017.977 * em - 5740.032 * emsq + 3708.2760 * eoc;
			}
			else
			{
				g211 = -72.099 + 331.819 * em - 508.738 * emsq + 266.724 * eoc;
				g310 = -346.844 + 1582.851 * em - 2415.925 * emsq + 1246.113 * eoc;
				g322 = -342.585 + 1554.908 * em - 2366.899 * emsq + 1215.972 * eoc;
				g410 = -1052.797 + 4758.686 * em - 7193.992 * emsq + 3651.957 * eoc;
				g422 = -3581.690 + 16178.110 * em - 24462.770 * emsq + 12422.520 * eoc;
				if (em > 0.715)
					g520 = -5149.66 + 29936.92 * em - 54087.36 * emsq + 31324.56 * eoc;
				else
					g520 = 1464.74 - 4664.75 * em + 3763.64 * emsq;
			}
			if (em < 0.7)
			{
				g533 = -919.22770 + 4988.6100 * em - 9064.7700 * emsq + 5542.21 * eoc;
				g521 = -822.71072 + 4568.6173 * em - 8491.4146 * emsq + 5337.524 * eoc;
				g532 = -853.66600 + 4690.2500 * em - 8624.7700 * emsq + 5341.4 * eoc;
			}
			else
			{
				g533 = -37995.780 + 161616.52 * em - 229838.20 * emsq + 109377.94 * eoc;
				g521 = -51752.104 + 218913.95 * em - 309468.16 * emsq + 146349.42 * eoc;
				g532 = -40023.880 + 170470.89 * em - 242699.48 * emsq + 115605.82 * eoc;
			}

			sini2 = sinim * sinim;
			f220 = 0.75 * (1.0 + 2.0 * cosim + cosisq);
			f221 = 1.5 * sini2;
			f321 = 1.875 * sinim * (1.0 - 2.0 * cosim - 3.0 * cosisq);
			f322 = -1.875 * sinim * (1.0 + 2.0 * cosim - 3.0 * cosisq);
			f441 = 35.0 * sini2 * f220;
			f442 = 39.3750 * sini2 * sini2;
			f522 = 9.84375 * sinim * (sini2 * (1.0 - 2.0 * cosim - 5.0 * cosisq) +
				0.33333333 * (-2.0 + 4.0 * cosim + 6.0 * cosisq));
			f523 = sinim * (4.92187512 * sini2 * (-2.0 - 4.0 * cosim +
				10.0 * cosisq) + 6.56250012 * (1.0 + 2.0 * cosim - 3.0 * cosisq));
			f542 = 29.53125 * sinim * (2.0 - 8.0 * cosim + cosisq *
				(-12.0 + 8.0 * cosim + 10.0 * cosisq));
			f543 = 29.53125 * sinim * (-2.0 - 8.0 * cosim + cosisq *
				(12.0 + 8.0 * cosim - 10.0 * cosisq));
			xno2 = nm * nm;
			ainv2 = aonv * aonv;
			temp1 = 3.0 * xno2 * ainv2;
			temp = temp1 * root22;
			d2201 = temp * f220 * g201;
			d2211 = temp * f221 * g211;
			temp1 = temp1 * aonv;
			temp = temp1 * root32;
			d3210 = temp * f321 * g310;
			d3222 = temp * f322 * g322;
			temp1 = temp1 * aonv;
			temp = 2.0 * temp1 * root44;
			d4410 = temp * f441 * g410;
			d4422 = temp * f442 * g422;
			temp1 = temp1 * aonv;
			temp = temp1 * root52;
			d5220 = temp * f522 * g520;
			d5232 = temp * f523 * g532;
			temp = 2.0 * temp1 * root54;
			d5421 = temp * f542 * g521;
			d5433 = temp * f543 * g533;
			xlamo = fmod(mo + nodeo + nodeo - theta - theta, pi2);
			xfact = mdot + dmdt + 2.0 * (nodedot + dnodt - rptim) - no;
			em = emo;
			emsq = emsqo;
		}

		/* ---------------- synchronous resonance terms -------------- */
		if (irez == 1)
		{
			g200 = 1.0 + emsq * (-2.5 + 0.8125 * emsq);
			g310 = 1.0 + 2.0 * emsq;
			g300 = 1.0 + emsq * (-6.0 + 6.60937 * emsq);
			f220 = 0.75 * (1.0 + cosim) * (1.0 + cosim);
			f311 = 0.9375 * sinim * sinim * (1.0 + 3.0 * cosim) - 0.75 * (1.0 + cosim);
			f330 = 1.0 + cosim;
			f330 = 1.875 * f330 * f330 * f330;
			del1 = 3.0 * nm * nm * aonv * aonv;
			del2 = 2.0 * del1 * f220 * g200 * q22;
			del3 = 3.0 * del1 * f330 * g300 * q33 * aonv;
			del1 = del1 * f311 * g310 * q31 * aonv;
			xlamo = fmod(mo + nodeo + argpo - theta, pi2);
			xfact = mdot + xpidot - rptim + dmdt + domdt + dnodt - no;
		}

		/* ------------ for sgp4, initialize the integrator ---------- */
		xli = xlamo;
		xni = no;
		atime = 0.0;
		nm = no + dndt;
	}

	//#include "debug3.cpp"
}  // dsinit
//------------------------------------------------------------------------------

	/*-----------------------------------------------------------------------------
	*
	*                           procedure dspace
	*
	*  this procedure provides deep space contributions to mean elements for
	*    perturbing third body.  these effects have been averaged over one
	*    revolution of the sun and moon.  for earth resonance effects, the
	*    effects have been averaged over no revolutions of the satellite.
	*    (mean motion)
	*
	*  author        : david vallado                  719-573-2600   28 jun 2005
	*
	*  inputs        :
	*    d2201, d2211, d3210, d3222, d4410, d4422, d5220, d5232, d5421, d5433 -
	*    dedt        -
	*    del1, del2, del3  -
	*    didt        -
	*    dmdt        -
	*    dnodt       -
	*    domdt       -
	*    irez        - flag for resonance           0-none, 1-one day, 2-half day
	*    argpo       - argument of perigee
	*    argpdot     - argument of perigee dot (rate)
	*    t           - time
	*    tc          -
	*    gsto        - gst
	*    xfact       -
	*    xlamo       -
	*    no          - mean motion
	*    atime       -
	*    em          - eccentricity
	*    ft          -
	*    argpm       - argument of perigee
	*    inclm       - inclination
	*    xli         -
	*    mm          - mean anomaly
	*    xni         - mean motion
	*    nodem       - right ascension of ascending node
	*
	*  outputs       :
	*    atime       -
	*    em          - eccentricity
	*    argpm       - argument of perigee
	*    inclm       - inclination
	*    xli         -
	*    mm          - mean anomaly
	*    xni         -
	*    nodem       - right ascension of ascending node
	*    dndt        -
	*    nm          - mean motion
	*
	*  locals        :
	*    delt        -
	*    ft          -
	*    theta       -
	*    x2li        -
	*    x2omi       -
	*    xl          -
	*    xldot       -
	*    xnddt       -
	*    xndt        -
	*    xomi        -
	*
	*  coupling      :
	*    none        -
	*
	*  references    :
	*    hoots, roehrich, norad spacetrack report #3 1980
	*    hoots, norad spacetrack report #6 1986
	*    hoots, schumacher and glover 2004
	*    vallado, crawford, hujsak, kelso  2006
	----------------------------------------------------------------------------*/

void eph_tle::dspace
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
)
{
	int iretn, iret;
	double delt, ft, theta, x2li, x2omi, xl, xldot, xnddt, xndt, xomi, g22, g32,
		g44, g52, g54, fasx2, fasx4, fasx6, rptim, step2, stepn, stepp;

	fasx2 = 0.13130908;
	fasx4 = 2.8843198;
	fasx6 = 0.37448087;
	g22 = 5.7686396;
	g32 = 0.95240898;
	g44 = 1.8014998;
	g52 = 1.0508330;
	g54 = 4.4108898;
	rptim = 4.37526908801129966e-3; // this equates to 7.29211514668855e-5 rad/sec
	stepp = 720.0;
	stepn = -720.0;
	step2 = 259200.0;

	/* ----------- calculate deep space resonance effects ----------- */
	dndt = 0.0;
	theta = fmod(gsto + tc * rptim, pi2);
	em = em + dedt * t;

	inclm = inclm + didt * t;
	argpm = argpm + domdt * t;
	nodem = nodem + dnodt * t;
	mm = mm + dmdt * t;

	//   sgp4fix for negative inclinations
	//   the following if statement should be commented out
	//  if (inclm < 0.0)
	// {
	//    inclm = -inclm;
	//    argpm = argpm - pi;
	//    nodem = nodem + pi;
	//  }

	/* - update resonances : numerical (euler-maclaurin) integration - */
	/* ------------------------- epoch restart ----------------------  */
	//   sgp4fix for propagator problems
	//   the following integration works for negative time steps and periods
	//   the specific changes are unknown because the original code was so convoluted

	// sgp4fix take out atime = 0.0 and fix for faster operation
	ft = 0.0;
	if (irez != 0)
	{
		// sgp4fix streamline check
		if ((atime == 0.0) || (t * atime <= 0.0) || (fabs(t) < fabs(atime)))
		{
			atime = 0.0;
			xni = no;
			xli = xlamo;
		}
		// sgp4fix move check outside loop
		if (t > 0.0)
			delt = stepp;
		else
			delt = stepn;

		iretn = 381; // added for do loop
		iret = 0; // added for loop
		while (iretn == 381)
		{
			/* ------------------- dot terms calculated ------------- */
			/* ----------- near - synchronous resonance terms ------- */
			if (irez != 2)
			{
				xndt = del1 * sin(xli - fasx2) + del2 * sin(2.0 * (xli - fasx4)) + del3 * sin(3.0 * (xli - fasx6));
				xldot = xni + xfact;
				xnddt = del1 * cos(xli - fasx2) +
					2.0 * del2 * cos(2.0 * (xli - fasx4)) +
					3.0 * del3 * cos(3.0 * (xli - fasx6));
				xnddt = xnddt * xldot;
			}
			else
			{
				/* --------- near - half-day resonance terms -------- */
				xomi = argpo + argpdot * atime;
				x2omi = xomi + xomi;
				x2li = xli + xli;
				xndt = d2201 * sin(x2omi + xli - g22) + d2211 * sin(xli - g22) +
					d3210 * sin(xomi + xli - g32) + d3222 * sin(-xomi + xli - g32) +
					d4410 * sin(x2omi + x2li - g44) + d4422 * sin(x2li - g44) +
					d5220 * sin(xomi + xli - g52) + d5232 * sin(-xomi + xli - g52) +
					d5421 * sin(xomi + x2li - g54) + d5433 * sin(-xomi + x2li - g54);
				xldot = xni + xfact;
				xnddt = d2201 * cos(x2omi + xli - g22) + d2211 * cos(xli - g22) +
					d3210 * cos(xomi + xli - g32) + d3222 * cos(-xomi + xli - g32) +
					d5220 * cos(xomi + xli - g52) + d5232 * cos(-xomi + xli - g52) +
					2.0 * (d4410 * cos(x2omi + x2li - g44) +
						d4422 * cos(x2li - g44) + d5421 * cos(xomi + x2li - g54) +
						d5433 * cos(-xomi + x2li - g54));
				xnddt = xnddt * xldot;
			}

			/* ----------------------- integrator ------------------- */
			// sgp4fix move end checks to end of routine
			if (fabs(t - atime) >= stepp)
			{
				iret = 0;
				iretn = 381;
			}
			else // exit here
			{
				ft = t - atime;
				iretn = 0;
			}

			if (iretn == 381)
			{
				xli = xli + xldot * delt + xndt * step2;
				xni = xni + xndt * delt + xnddt * step2;
				atime = atime + delt;
			}
		}  // while iretn = 381

		nm = xni + xndt * ft + xnddt * ft * ft * 0.5;
		xl = xli + xldot * ft + xndt * ft * ft * 0.5;
		if (irez != 1)
		{
			mm = xl - 2.0 * nodem + 2.0 * theta;
			dndt = nm - no;
		}
		else
		{
			mm = xl - nodem - argpm + theta;
			dndt = nm - no;
		}
		nm = no + dndt;
	}

	//#include "debug4.cpp"
}  // dsspace
//------------------------------------------------------------------------------

	/*-----------------------------------------------------------------------------
	*
	*                           procedure initl
	*
	*  this procedure initializes the spg4 propagator. all the initialization is
	*    consolidated here instead of having multiple loops inside other routines.
	*
	*  author        : david vallado                  719-573-2600   28 jun 2005
	*
	*  inputs        :
	*    satn        - satellite number - not needed, placed in satrec
	*    xke         - reciprocal of tumin
	*    j2          - j2 zonal harmonic
	*    ecco        - eccentricity                           0.0 - 1.0
	*    epoch       - epoch time in days from jan 0, 1950. 0 hr
	*    inclo       - inclination of satellite
	*    no          - mean motion of satellite
	*
	*  outputs       :
	*    ainv        - 1.0 / a
	*    ao          - semi major axis
	*    con41       -
	*    con42       - 1.0 - 5.0 cos(i)
	*    cosio       - cosine of inclination
	*    cosio2      - cosio squared
	*    eccsq       - eccentricity squared
	*    method      - flag for deep space                    'd', 'n'
	*    omeosq      - 1.0 - ecco * ecco
	*    posq        - semi-parameter squared
	*    rp          - radius of perigee
	*    rteosq      - square root of (1.0 - ecco*ecco)
	*    sinio       - sine of inclination
	*    gsto        - gst at time of observation               rad
	*    no          - mean motion of satellite
	*
	*  locals        :
	*    ak          -
	*    d1          -
	*    del         -
	*    adel        -
	*    po          -
	*
	*  coupling      :
	*    getgravconst- no longer used
	*    gstime      - find greenwich sidereal time from the julian date
	*
	*  references    :
	*    hoots, roehrich, norad spacetrack report #3 1980
	*    hoots, norad spacetrack report #6 1986
	*    hoots, schumacher and glover 2004
	*    vallado, crawford, hujsak, kelso  2006
	----------------------------------------------------------------------------*/
void eph_tle::initl
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
)
{
	///////////////////////////////////
	//get TEME->J2000 matrix
	double JD_UTC = epoch + 2433281.5;
	double MJD_UTC = JD_UTC - JDmMJD_offset;
	int order = 106;// number of terms for nutation: 4, 50, 106, ...
	int eqeterms = 2;// number of terms for eqe: 0, 2
	char optteme = 'a';//option for processing: a - complete nutation, b - truncated nutation, c - truncated transf matrix
	SGP4_TEME_to_J2000 = TEME_to_J2000(iau80rec, MJD_UTC, order, eqeterms, optteme);
	///////////////////////////////////

	/* --------------------- local variables ------------------------ */
	double ak, d1, del, adel, po, x2o3;

	// sgp4fix use old way of finding gst
	double ds70;
	double ts70, tfrac, c1, thgr70, fk5r, c1p2p;

	/* ----------------------- earth constants ---------------------- */
	// sgp4fix identify constants and allow alternate values
	// only xke and j2 are used here so pass them in directly
	// getgravconst( whichconst, tumin, mu, radiusearthkm, xke, j2, j3, j4, j3oj2 );
	x2o3 = 2.0 / 3.0;

	/* ------------- calculate auxillary epoch quantities ---------- */
	eccsq = ecco * ecco;
	omeosq = 1.0 - eccsq;
	rteosq = sqrt(omeosq);
	cosio = cos(inclo);
	cosio2 = cosio * cosio;

	/* ------------------ un-kozai the mean motion ----------------- */
	ak = std::pow(xke / no_kozai, x2o3);
	d1 = 0.75 * j2 * (3.0 * cosio2 - 1.0) / (rteosq * omeosq);
	del = d1 / (ak * ak);
	adel = ak * (1.0 - del * del - del *
		(1.0 / 3.0 + 134.0 * del * del / 81.0));
	del = d1 / (adel * adel);
	no_unkozai = no_kozai / (1.0 + del);

	ao = std::pow(xke / (no_unkozai), x2o3);
	sinio = sin(inclo);
	po = ao * omeosq;
	con42 = 1.0 - 5.0 * cosio2;
	con41 = -con42 - cosio2 - cosio2;
	ainv = 1.0 / ao;
	posq = po * po;
	rp = ao * (1.0 - ecco);
	deep_space = false;

	// sgp4fix modern approach to finding sidereal time
	//   if (opsmode == 'a')
	//      {
	// sgp4fix use old way of finding gst
	// count integer number of days from 0 jan 1970
	ts70 = epoch - 7305.0;
	ds70 = floor(ts70 + 1.0e-8);
	tfrac = ts70 - ds70;
	// find greenwich location at epoch
	c1 = 1.72027916940703639e-2;
	thgr70 = 1.7321343856509374;
	fk5r = 5.07551419432269442e-15;
	c1p2p = c1 + pi2;
	double gsto1 = fmod(thgr70 + c1 * ds70 + c1p2p * tfrac + ts70 * ts70 * fk5r, pi2);
	if (gsto1 < 0.0)
		gsto1 = gsto1 + pi2;
	//    }
	//    else
	gsto = gstime(epoch + 2433281.5);

	//#include "debug5.cpp"
}  // initl
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
*
*                           function teme_j2k
*
*  this function transforms a vector between the true equator mean equinox system,
*    (teme) and the mean equator mean equinox (j2000) system.
*
*  author        : david vallado                  719-573-2600   25 jun 2002
*
*  revisions
*    vallado     - add order                                     29 sep 2002
*    vallado     - conversion to c++                             21 feb 2005
*
*  inputs          description                    range / units
*    rteme       - position vector of date
*                    true equator, mean equinox   km
*    vteme       - velocity vector of date
*                    true equator, mean equinox   km/s
*    ateme       - acceleration vector of date
*                    true equator, mean equinox   km/s2
*    direct      - direction of transfer          eFrom, 'TOO '
*    iau80rec    - record containing the iau80 constants rad
*    ttt         - julian centuries of tt         centuries
*    eqeterms    - number of terms for eqe        0, 2
*    opt1        - option for processing          a - complete nutation
*                                                 b - truncated nutation
*                                                 c - truncated transf matrix
*
*  outputs       :
*    rj2k        - position vector j2k            km
*    vj2k        - velocity vector j2k            km/s
*    aj2k        - acceleration vector j2k        km/s2
*
*  locals        :
*    prec        - matrix for mod - j2k
*    nutteme     - matrix for mod - teme - an approximation for nutation
*    tm          - combined matrix for teme
*
*  coupling      :
*   precess      - rotation for precession        j2k - mod
*   truemean     - rotation for truemean          j2k - teme
*
*  references    :
*    vallado       2007, 236
* --------------------------------------------------------------------------- */

d33 eph_tle::TEME_to_J2000(iau80data& iau80rec, double MJD_UTC, int order, int eqeterms, char optteme)
{
	//time conversion
	//MJD_UTC-> TT

	/*
	https://ascom-standards.org/Help/Developer61/html/N_ASCOM_Astrometry_NOVASCOM.htm
	TT Time Scale
	The Terrestrial Dynamical Time (TT) scale is used in solar system orbital calculations.
	It is based completely on planetary motions; you can think of the solar system as a giant TT clock.
	It differs from UT1 by an amount called "delta-t", which slowly increases with time, and is about 60 seconds right now (1998).
	You can convert Julian dates between UTC and TT using the methods Julian_TJD() and TJD_Julian().

	AstroUtils.JulianDateTT Method
	double JulianDateTT(double DeltaUT1)
	Current value for Delta-UT1, the difference between UTC and UT1; always in the range -0.9 to +0.9 seconds. Use 0.0 to calculate TT through TAI. Delta-UT1 varies irregularly throughout the year.
	When Delta-UT1 is provided, Terrestrial time is calculated as TT = UTC + DeltaUT1 + DeltaT.
	Otherwise, when Delta-UT1 is 0.0, TT is calculated as TT = UTC + ΔAT + 32.184s,
	where ΔAT is the current number of leap seconds applied to UTC (34 at April 2012, with the 35th being added at the end of June 2012). The resulting TT value is then converted to a Julian date and returned.
	*/

	double dut1_s = 0.;//seconds, neglect...; delta of ut1 - utc [s]
	double time_offset_s = 32.184;
	double delta_AT_s = 34.;
	double JD_TT = (MJD_UTC + delta_AT_s / 86400. + time_offset_s / 86400.) + 2400000.5;
	double ttt = (JD_TT - 2451545.0) / 36525.0;//julian centuries of TT

	double psia, wa, epsa, chia;
	d33 prec = precess(ttt, e80, psia, wa, epsa, chia);
	d33 nutteme = truemean(ttt, order, eqeterms, optteme, iau80rec);

	//TEME->J2000
	d33 out = prec * nutteme;
	return out;
}  // procedure teme_j2k
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
*
*                           function precess
*
*  this function calulates the transformation matrix that accounts for the effects
*    of precession. both the 1980 and 2000 theories are handled. note that the
*    required parameters differ a little.
*
*  author        : david vallado                  719-573-2600   25 jun 2002
*
*  revisions
*    vallado     - conversion to c++                             21 feb 2005
*    vallado     - misc updates, nomenclature, etc               23 nov 2005
*
*  inputs          description                    range / units
*    ttt         - julian centuries of tt
*    opt         - method option                  e00a, e00b, e96, e80
*
*  outputs       :
*    prec        - transformation matrix for mod - j2000 (80 only)
*    psia        - cannonical precession angle    rad    (00 only)
*    wa          - cannonical precession angle    rad    (00 only)
*    epsa        - cannonical precession angle    rad    (00 only)
*    chia        - cannonical precession angle    rad    (00 only)
*    prec        - matrix converting from "mod" to gcrf
*
*  locals        :
*    zeta        - precession angle               rad
*    z           - precession angle               rad
*    theta       - precession angle               rad
*    oblo        - obliquity value at j2000 epoch "//
*
*  coupling      :
*    none        -
*
*  references    :
*    vallado       2007, 217, 228
* --------------------------------------------------------------------------- */

d33 eph_tle::precess(double ttt, eOpt opt, double& psia, double& wa, double& epsa, double& chia)
{
	d33 prec;
	double zeta, theta, z, coszeta, sinzeta, costheta, sintheta, cosz, sinz, oblo;
	double convrt = pi / (180.0 * 3600.0);

	// ------------------- iau 77 precession angles --------------------
	if ((opt == e80) | (opt == e96))
	{
		oblo = 84381.448; // "
		psia = ((-0.001147 * ttt - 1.07259) * ttt + 5038.7784) * ttt; // "
		wa = ((-0.007726 * ttt + 0.05127) * ttt) + oblo;
		epsa = ((0.001813 * ttt - 0.00059) * ttt - 46.8150) * ttt + oblo;
		chia = ((-0.001125 * ttt - 2.38064) * ttt + 10.5526) * ttt;

		zeta = ((0.017998 * ttt + 0.30188) * ttt + 2306.2181) * ttt; // "
		theta = ((-0.041833 * ttt - 0.42665) * ttt + 2004.3109) * ttt;
		z = ((0.018203 * ttt + 1.09468) * ttt + 2306.2181) * ttt;
	}
	// ------------------ iau 00 precession angles -------------------
	else
	{
		oblo = 84381.406; // "
		psia = ((((-0.0000000951 * ttt + 0.000132851) * ttt - 0.00114045) * ttt - 1.0790069) * ttt + 5038.481507) * ttt; // "
		wa = ((((0.0000003337 * ttt - 0.000000467) * ttt - 0.00772503) * ttt + 0.0512623) * ttt - 0.025754) * ttt + oblo;
		epsa = ((((-0.0000000434 * ttt - 0.000000576) * ttt + 0.00200340) * ttt - 0.0001831) * ttt - 46.836769) * ttt + oblo;
		chia = ((((-0.0000000560 * ttt + 0.000170663) * ttt - 0.00121197) * ttt - 2.3814292) * ttt + 10.556403) * ttt;

		zeta = ((((-0.0000003173 * ttt - 0.000005971) * ttt + 0.01801828) * ttt + 0.2988499) * ttt + 2306.083227) * ttt + 2.650545; // "
		theta = ((((-0.0000001274 * ttt - 0.000007089) * ttt - 0.04182264) * ttt - 0.4294934) * ttt + 2004.191903) * ttt;
		z = ((((0.0000002904 * ttt - 0.000028596) * ttt + 0.01826837) * ttt + 1.0927348) * ttt + 2306.077181) * ttt - 2.650545;
	}

	// convert units to rad
	psia = psia * convrt;
	wa = wa * convrt;
	oblo = oblo * convrt;
	epsa = epsa * convrt;
	chia = chia * convrt;

	zeta = zeta * convrt;
	theta = theta * convrt;
	z = z * convrt;

	if ((opt == e80) | (opt == e96))
	{
		coszeta = cos(zeta);
		sinzeta = sin(zeta);
		costheta = cos(theta);
		sintheta = sin(theta);
		cosz = cos(z);
		sinz = sin(z);

		// ----------------- form matrix  mod to gcrf ------------------
		prec.v[0][0] = coszeta * costheta * cosz - sinzeta * sinz;
		prec.v[0][1] = coszeta * costheta * sinz + sinzeta * cosz;
		prec.v[0][2] = coszeta * sintheta;
		prec.v[1][0] = -sinzeta * costheta * cosz - coszeta * sinz;
		prec.v[1][1] = -sinzeta * costheta * sinz + coszeta * cosz;
		prec.v[1][2] = -sinzeta * sintheta;
		prec.v[2][0] = -sintheta * cosz;
		prec.v[2][1] = -sintheta * sinz;
		prec.v[2][2] = costheta;

		// ----------------- do rotations instead ----------------------
		// p1 = rot3mat( z );
		// p2 = rot2mat( -theta );
		// p3 = rot3mat( zeta );
		// prec = p3 * p2*p1;
	}
	else
	{
		/*
		double p1[3][3], p2[3][3], p3[3][3], p4[3][3], tr1[3][3], tr2[3][3];
		rot3mat(-chia, p1);
		rot1mat(wa, p2);
		rot3mat(psia, p3);
		rot1mat(-oblo, p4);
		matmult(p4, p3, tr1, 3, 3, 3);
		matmult(tr1, p2, tr2, 3, 3, 3);
		matmult(tr2, p1, prec, 3, 3, 3);
		*/
	}

	//printf("pr %11.7f  %11.7f  %11.7f %11.7fdeg \n", psia * 180 / pi, wa * 180 / pi, epsa * 180 / pi, chia * 180 / pi);
	//printf("pr %11.7f  %11.7f  %11.7fdeg \n", zeta * 180 / pi, theta * 180 / pi, z * 180 / pi);

	return prec;
}  // procedure precess
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
*
*                           function nutation
*
*  this function calulates the transformation matrix that accounts for the
*    effects of nutation.
*
*  author        : david vallado                  719-573-2600   27 jun 2002
*
*  revisions
*    vallado     - consolidate with iau 2000                     14 feb 2005
*    vallado     - conversion to c++                             21 feb 2005
*
*  inputs          description                    range / units
*    ttt         - julian centuries of tt
*    ddpsi       - delta psi correction to gcrf   rad
*    ddeps       - delta eps correction to gcrf   rad
*    nutopt      - nutation option                calc 'c', read 'r'
*    iau80rec    - record containing the iau80 constants rad
*
*  outputs       :
*    deltapsi    - nutation in longiotude angle   rad
*    trueeps     - true obliquity of the ecliptic rad
*    meaneps     - mean obliquity of the ecliptic rad
*    omega       -                                rad
*    nut         - transform matrix for tod -     mod
*
*  locals        :
*    iar80       - integers for fk5 1980
*    rar80       - reals for fk5 1980
*    l           -                                rad
*    ll          -                                rad
*    f           -                                rad
*    d           -                                rad
*    deltaeps    - change in obliquity            rad
*
*  coupling      :
*    fundarg     - find fundamental arguments
*    fmod      - modulus division
*
*  references    :
*    vallado       2007, 217, 228
* --------------------------------------------------------------------------- */

d33 eph_tle::nutation(double ttt, double ddpsi, double ddeps, iau80data& iau80rec, char nutopt, double& deltapsi, double& deltaeps, double& trueeps, double& meaneps, double& omega)
{
	d33 nut;
	double l, l1, f, d, lonmer, lonven, lonear, lonmar, lonjup, lonsat, lonurn, lonnep, precrate, cospsi, sinpsi, coseps, sineps, costrueeps, sintrueeps;
	int  i;
	double tempval;

	//determine coefficients for iau 1980 nutation theory
	meaneps = ((0.001813 * ttt - 0.00059) * ttt - 46.8150) * ttt + 84381.448;
	meaneps = fmod(meaneps / 3600.0, 360.0);
	meaneps = meaneps * deg2rad;

	if (nutopt == 'c')
	{
		fundarg(ttt, e80, l, l1, f, d, omega, lonmer, lonven, lonear, lonmar, lonjup, lonsat, lonurn, lonnep, precrate);

		deltapsi = 0.0;
		deltaeps = 0.0;
		for (i = 106; i >= 1; i--)
		{
			tempval = iau80rec.iar80[i][1] * l + iau80rec.iar80[i][2] * l1 + iau80rec.iar80[i][3] * f + iau80rec.iar80[i][4] * d + iau80rec.iar80[i][5] * omega;
			deltapsi = deltapsi + (iau80rec.rar80[i][1] + iau80rec.rar80[i][2] * ttt) * sin(tempval);
			deltaeps = deltaeps + (iau80rec.rar80[i][3] + iau80rec.rar80[i][4] * ttt) * cos(tempval);
		}

		// --------------- find nutation parameters --------------------
		deltapsi = fmod(deltapsi + ddpsi / deg2rad, 360.0) * deg2rad;
		deltaeps = fmod(deltaeps + ddeps / deg2rad, 360.0) * deg2rad;
	}

	trueeps = meaneps + deltaeps;

	cospsi = cos(deltapsi);
	sinpsi = sin(deltapsi);
	coseps = cos(meaneps);
	sineps = sin(meaneps);
	costrueeps = cos(trueeps);
	sintrueeps = sin(trueeps);

	nut.v[0][0] = cospsi;
	nut.v[0][1] = costrueeps * sinpsi;
	nut.v[0][2] = sintrueeps * sinpsi;
	nut.v[1][0] = -coseps * sinpsi;
	nut.v[1][1] = costrueeps * coseps * cospsi + sintrueeps * sineps;
	nut.v[1][2] = sintrueeps * coseps * cospsi - sineps * costrueeps;
	nut.v[2][0] = -sineps * sinpsi;
	nut.v[2][1] = costrueeps * sineps * cospsi - sintrueeps * coseps;
	nut.v[2][2] = sintrueeps * sineps * cospsi + costrueeps * coseps;

	// n1 = rot1mat( trueeps );
	// n2 = rot3mat( deltapsi );
	// n3 = rot1mat( -meaneps );
	// nut = n3 * n2 * n1;

	//printf("meaneps %11.7f dp  %11.7f de  %11.7f te  %11.7f  \n", meaneps * 180 / pi, deltapsi * 180 / pi, eltaeps * 180 / pi, trueeps * 180 / pi);

	return nut;
}  // procedure nutation
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
*
*                           function truemean
*
*  this function forms the transformation matrix to go between the
*    norad true equator mean equinox of date and the mean equator mean equinox
*    of date (gcrf).  the results approximate the effects of nutation and
*    precession.
*
*  author        : david vallado                  719-573-2600   25 jun 2002
*
*  revisions
*    vallado     - fixes to order                                29 sep 2002
*    vallado     - fixes to all options                           6 may 2003
*    vallado     - conversion to c++                             21 feb 2005
*
*  inputs          description                    range / units
*    ttt         - julian centuries of tt
*    order       - number of terms for nutation   4, 50, 106, ...
*    eqeterms    - number of terms for eqe        0, 2
*    opt1        - option for processing          a - complete nutation
*                                                 b - truncated nutation
*                                                 c - truncated transf matrix
*    iau80rec    - record containing the iau80 constants rad
*
*  outputs       :
*    nutteme     - matrix for mod - teme - an approximation for nutation
*
*  locals        :
*    prec        - matrix for mod - j2000
*    tm          - combined matrix for teme
*    l           -                                rad
*    ll          -                                rad
*    f           -                                rad
*    d           -                                rad
*    omega       -                                rad
*    deltapsi    - nutation angle                 rad
*    deltaeps    - change in obliquity            rad
*    eps         - mean obliquity of the ecliptic rad
*    trueeps     - true obliquity of the ecliptic rad
*    meaneps     - mean obliquity of the ecliptic rad
*
*  coupling      :
*
*
*  references    :
*    vallado       2007, 236
* --------------------------------------------------------------------------- */

d33 eph_tle::truemean(double ttt, int order, int eqeterms, char opt, iau80data& iau80rec)
{
	double l, l1, f, d, omega, cospsi, sinpsi, coseps, sineps, costrueeps, sintrueeps, meaneps, deltapsi, deltaeps, trueeps, tempval, jdttt, eqe;
	int i;
	d33 nut, st;

	//determine coefficients for iau 1980 nutation theory
	meaneps = ((0.001813 * ttt - 0.00059) * ttt - 46.8150) * ttt + 84381.448;
	meaneps = fmod(meaneps / 3600.0, 360.0);
	meaneps = meaneps * deg2rad;

	l = ((((0.064) * ttt + 31.310) * ttt + 1717915922.6330) * ttt) / 3600.0 + 134.96298139;
	l1 = ((((-0.012) * ttt - 0.577) * ttt + 129596581.2240) * ttt) / 3600.0 + 357.52772333;
	f = ((((0.011) * ttt - 13.257) * ttt + 1739527263.1370) * ttt) / 3600.0 + 93.27191028;
	d = ((((0.019) * ttt - 6.891) * ttt + 1602961601.3280) * ttt) / 3600.0 + 297.85036306;
	omega = ((((0.008) * ttt + 7.455) * ttt - 6962890.5390) * ttt) / 3600.0 + 125.04452222;

	l = fmod(l, 360.0) * deg2rad;
	l1 = fmod(l1, 360.0) * deg2rad;
	f = fmod(f, 360.0) * deg2rad;
	d = fmod(d, 360.0) * deg2rad;
	omega = fmod(omega, 360.0) * deg2rad;

	deltapsi = 0.0;
	deltaeps = 0.0;
	for (i = 1; i <= order; i++)   // the eqeterms in nut80.dat are already sorted
	{
		tempval = iau80rec.iar80[i][1] * l + iau80rec.iar80[i][2] * l1 + iau80rec.iar80[i][3] * f + iau80rec.iar80[i][4] * d + iau80rec.iar80[i][5] * omega;
		deltapsi = deltapsi + (iau80rec.rar80[i][1] + iau80rec.rar80[i][2] * ttt) * sin(tempval);
		deltaeps = deltaeps + (iau80rec.rar80[i][3] + iau80rec.rar80[i][4] * ttt) * cos(tempval);
	}

	// --------------- find nutation parameters --------------------
	deltapsi = fmod(deltapsi, 360.0) * deg2rad;
	deltaeps = fmod(deltaeps, 360.0) * deg2rad;
	trueeps = meaneps + deltaeps;

	cospsi = cos(deltapsi);
	sinpsi = sin(deltapsi);
	coseps = cos(meaneps);
	sineps = sin(meaneps);
	costrueeps = cos(trueeps);
	sintrueeps = sin(trueeps);

	jdttt = ttt * 36525.0 + 2451545.0;
	// small disconnect with ttt instead of ut1
	if ((jdttt > 2450449.5) && (eqeterms > 0))
		eqe = deltapsi * cos(meaneps) + 0.00264 * pi / (3600 * 180) * sin(omega) + 0.000063 * pi / (3600 * 180) * sin(2.0 * omega);
	else
		eqe = deltapsi * cos(meaneps);

	nut.v[0][0] = cospsi;
	nut.v[0][1] = costrueeps * sinpsi;
	if (opt == 'b')
		nut.v[0][1] = 0.0;
	nut.v[0][2] = sintrueeps * sinpsi;
	nut.v[1][0] = -coseps * sinpsi;
	if (opt == 'b')
		nut.v[1][0] = 0.0;
	nut.v[1][1] = costrueeps * coseps * cospsi + sintrueeps * sineps;
	nut.v[1][2] = sintrueeps * coseps * cospsi - sineps * costrueeps;
	nut.v[2][0] = -sineps * sinpsi;
	nut.v[2][1] = costrueeps * sineps * cospsi - sintrueeps * coseps;
	nut.v[2][2] = sintrueeps * sineps * cospsi + costrueeps * coseps;

	st.v[0][0] = cos(eqe);
	st.v[0][1] = -sin(eqe);
	st.v[0][2] = 0.0;
	st.v[1][0] = sin(eqe);
	st.v[1][1] = cos(eqe);
	st.v[1][2] = 0.0;
	st.v[2][0] = 0.0;
	st.v[2][1] = 0.0;
	st.v[2][2] = 1.0;

	d33 nutteme = st * nut;

	if (opt == 'c')
	{
		nutteme.v[0][0] = 1.0;
		nutteme.v[0][1] = 0.0;
		nutteme.v[0][2] = deltapsi * sineps;
		nutteme.v[1][0] = 0.0;
		nutteme.v[1][1] = 1.0;
		nutteme.v[1][2] = deltaeps;
		nutteme.v[2][0] = -deltapsi * sineps;
		nutteme.v[2][1] = -deltaeps;
		nutteme.v[2][2] = 1.0;
	}

	// tm = nutteme * prec;

	return nutteme;
}   // procedure truemean
//------------------------------------------------------------------------------

/* -----------------------------------------------------------------------------
*
*                           function fundarg
*
*  this function calulates the delauany variables and planetary values for
*  several theories.
*
*  author        : david vallado                  719-573-2600   16 jul 2004
*
*  revisions
*    vallado     - conversion to c++                             23 nov 2005
*
*  inputs          description                    range / units
*    ttt         - julian centuries of tt
*    opt         - method option                  e00a, e00b, e96, e80
*
*  outputs       :
*    l           - delaunay element               rad
*    ll          - delaunay element               rad
*    f           - delaunay element               rad
*    d           - delaunay element               rad
*    omega       - delaunay element               rad
*    planetary longitudes                         rad
*
*  locals        :
*
*
*  coupling      :
*    none        -
*
*  references    :
*    vallado       2007, 217
* --------------------------------------------------------------------------- */

void eph_tle::fundarg
(
	double ttt, eOpt opt,
	double& l, double& l1, double& f, double& d, double& omega,
	double& lonmer, double& lonven, double& lonear, double& lonmar,
	double& lonjup, double& lonsat, double& lonurn, double& lonnep,
	double& precrate
)
{
	// ---- determine coefficients for icrs nutation theories ----
	// ---- iau-2000a theory
	if (opt == e00a)
	{
		// ------ form the delaunay fundamental arguments in deg
		l = ((((-0.00024470 * ttt + 0.051635) * ttt + 31.8792) * ttt + 1717915923.2178) * ttt + 485868.249036) / 3600.0;
		l1 = ((((-0.00001149 * ttt + 0.000136) * ttt - 0.5532) * ttt + 129596581.0481) * ttt + 1287104.793048) / 3600.0;
		f = ((((+0.00000417 * ttt - 0.001037) * ttt - 12.7512) * ttt + 1739527262.8478) * ttt + 335779.526232) / 3600.0;
		d = ((((-0.00003169 * ttt + 0.006593) * ttt - 6.3706) * ttt + 1602961601.2090) * ttt + 1072260.703692) / 3600.0;
		omega = ((((-0.00005939 * ttt + 0.007702) * ttt + 7.4722) * ttt - 6962890.5431) * ttt + 450160.398036) / 3600.0;

		// ------ form the planetary arguments in deg
		lonmer = (908103.259872 + 538101628.688982 * ttt) / 3600.0;
		lonven = (655127.283060 + 210664136.433548 * ttt) / 3600.0;
		lonear = (361679.244588 + 129597742.283429 * ttt) / 3600.0;
		lonmar = (1279558.798488 + 68905077.493988 * ttt) / 3600.0;
		lonjup = (123665.467464 + 10925660.377991 * ttt) / 3600.0;
		lonsat = (180278.799480 + 4399609.855732 * ttt) / 3600.0;
		lonurn = (1130598.018396 + 1542481.193933 * ttt) / 3600.0;
		lonnep = (1095655.195728 + 786550.320744 * ttt) / 3600.0;
		precrate = ((1.112022 * ttt + 5028.8200) * ttt) / 3600.0;
	}

	// ---- iau-2000b theory
	if (opt == e00b)
	{
		// ------ form the delaunay fundamental arguments in deg
		l = (1717915923.2178 * ttt + 485868.249036) / 3600.0;
		l1 = (129596581.0481 * ttt + 1287104.79305) / 3600.0;
		f = (1739527262.8478 * ttt + 335779.526232) / 3600.0;
		d = (1602961601.2090 * ttt + 1072260.70369) / 3600.0;
		omega = (-6962890.5431 * ttt + 450160.398036) / 3600.0;

		// ------ form the planetary arguments in deg
		lonmer = 0.0;
		lonven = 0.0;
		lonear = 0.0;
		lonmar = 0.0;
		lonjup = 0.0;
		lonsat = 0.0;
		lonurn = 0.0;
		lonnep = 0.0;
		precrate = 0.0;
	}

	// ---- iau-1996 theory
	if (opt == e96)
	{
		// ------ form the delaunay fundamental arguments in deg
		l = ((((-0.00024470 * ttt + 0.051635) * ttt + 31.8792) * ttt + 1717915923.2178) * ttt) / 3600.0 + 134.96340251;
		l1 = ((((-0.00001149 * ttt - 0.000136) * ttt - 0.5532) * ttt + 129596581.0481) * ttt) / 3600.0 + 357.52910918;
		f = ((((+0.00000417 * ttt + 0.001037) * ttt - 12.7512) * ttt + 1739527262.8478) * ttt) / 3600.0 + 93.27209062;
		d = ((((-0.00003169 * ttt + 0.006593) * ttt - 6.3706) * ttt + 1602961601.2090) * ttt) / 3600.0 + 297.85019547;
		omega = ((((-0.00005939 * ttt + 0.007702) * ttt + 7.4722) * ttt - 6962890.2665) * ttt) / 3600.0 + 125.04455501;
		// ------ form the planetary arguments in deg
		lonmer = 0.0;
		lonven = 181.979800853 + 58517.8156748 * ttt;
		lonear = 100.466448494 + 35999.3728521 * ttt;
		lonmar = 355.433274605 + 19140.299314 * ttt;
		lonjup = 34.351483900 + 3034.90567464 * ttt;
		lonsat = 50.0774713998 + 1222.11379404 * ttt;
		lonurn = 0.0;
		lonnep = 0.0;
		precrate = (0.0003086 * ttt + 1.39697137214) * ttt;
	}

	// ---- iau-1980 theory
	if (opt == e80)
	{
		// ------ form the delaunay fundamental arguments in deg
		l = ((((0.064) * ttt + 31.310) * ttt + 1717915922.6330) * ttt) / 3600.0 + 134.96298139;
		l1 = ((((-0.012) * ttt - 0.577) * ttt + 129596581.2240) * ttt) / 3600.0 + 357.52772333;
		f = ((((0.011) * ttt - 13.257) * ttt + 1739527263.1370) * ttt) / 3600.0 + 93.27191028;
		d = ((((0.019) * ttt - 6.891) * ttt + 1602961601.3280) * ttt) / 3600.0 + 297.85036306;
		omega = ((((0.008) * ttt + 7.455) * ttt - 6962890.5390) * ttt) / 3600.0 + 125.04452222;
		// ------ form the planetary arguments in deg
// iers tn13 shows no planetary
// seidelmann shows these equations
// circ 163 shows no planetary
// ???????
		lonmer = 252.3 + 149472.0 * ttt;
		lonven = 179.9 + 58517.8 * ttt;
		lonear = 98.4 + 35999.4 * ttt;
		lonmar = 353.3 + 19140.3 * ttt;
		lonjup = 32.3 + 3034.9 * ttt;
		lonsat = 48.0 + 1222.1 * ttt;
		lonurn = 0.0;
		lonnep = 0.0;
		precrate = 0.0;
	}

	// ---- convert units from deg to rad
	l = fmod(l, 360.0) * deg2rad;
	l1 = fmod(l1, 360.0) * deg2rad;
	f = fmod(f, 360.0) * deg2rad;
	d = fmod(d, 360.0) * deg2rad;
	omega = fmod(omega, 360.0) * deg2rad;

	lonmer = fmod(lonmer, 360.0) * deg2rad;
	lonven = fmod(lonven, 360.0) * deg2rad;
	lonear = fmod(lonear, 360.0) * deg2rad;
	lonmar = fmod(lonmar, 360.0) * deg2rad;
	lonjup = fmod(lonjup, 360.0) * deg2rad;
	lonsat = fmod(lonsat, 360.0) * deg2rad;
	lonurn = fmod(lonurn, 360.0) * deg2rad;
	lonnep = fmod(lonnep, 360.0) * deg2rad;
	precrate = fmod(precrate, 360.0) * deg2rad;

	//printf("fa %11.7f  %11.7f  %11.7f  %11.7f  %11.7f deg \n", l * 180 / pi, l1 * 180 / pi, f * 180 / pi, d * 180 / pi, omega * 180 / pi);
	//printf("fa %11.7f  %11.7f  %11.7f  %11.7f deg \n", lonmer * 180 / pi, lonven * 180 / pi, lonear * 180 / pi, lonmar * 180 / pi);
	//printf("fa %11.7f  %11.7f  %11.7f  %11.7f deg \n", lonjup * 180 / pi, lonsat * 180 / pi, lonurn * 180 / pi, lonnep * 180 / pi);
	//printf("fa %11.7f  \n", precrate * 180 / pi);
}  // procedure fundarg
//------------------------------------------------------------------------------
void eph_tle::initialize(std::string L0, std::string L1, std::string L2)
{
	//char opsmode = 'i';//improved sgp4 resulting in smoother behavior
	char opsmode = 'a';//best understanding of how afspc code works

	//gravconsttype  whichconst = wgs72old;	
	gravconsttype  whichconst = wgs72;
	//gravconsttype  whichconst = wgs84;

	char longstr1[130], longstr2[130];
	sprintf_s(longstr1, "%s", "");
	sprintf_s(longstr2, "%s", "");

	sprintf_s(longstr1, "%s", L1.c_str());
	sprintf_s(longstr2, "%s", L2.c_str());

	twoline2rv(longstr1, longstr2, opsmode, whichconst, satrec);
}
//------------------------------------------------------------------------------
void eph_tle::state_ECI_J2000(double MJD_UTC, d3* r_ECI_J2000_m, d3* v_ECI_J2000_mps)
{
	if (satrec.error == 0)
	{
		double JD_UTC = MJD_UTC + JDmMJD_offset;
		double TLE_JD_UTC = satrec.jdsatepoch + satrec.jdsatepochF;//JD+JD_Fraction
		double tsince = (JD_UTC - TLE_JD_UTC) * 1440.0;//time since epoch [minutes]

		d3 r_TEME_km, v_TEME_kmps;
		sgp4(satrec, tsince, r_TEME_km.v, v_TEME_kmps.v);

		//position and velocity (in km and km/sec)
		if (r_ECI_J2000_m != nullptr)
			*r_ECI_J2000_m = SGP4_TEME_to_J2000 * r_TEME_km * 1000.;

		if (v_ECI_J2000_mps != nullptr)
			*v_ECI_J2000_mps = SGP4_TEME_to_J2000 * v_TEME_kmps * 1000.;
	}
}
//------------------------------------------------------------------------------
int eph_tle::get_NORAD()
{
	return satrec.satnum;
}
//------------------------------------------------------------------------------
std::string timestamp_UTC(double MJD)
{
	char txttime[MAXBUF];
	sprintf(txttime, "%s", "");

	int year0, month0, day0, hour0, minute0, isec;
	double sec_fraction;
	date_and_time_from_MJD(MJD, &year0, &month0, &day0, &hour0, &minute0, &isec, &sec_fraction);
	double sec = double(isec) + sec_fraction;

	sprintf(txttime, "%04d-%02d-%02dT%02d:%02d:%09.6lfZ", year0, month0, day0, hour0, minute0, sec);
	return std::string(txttime);
}
//------------------------------------------------------------------------------
std::string timestamp_UTC_1s(double MJD)
{
	char txttime[MAXBUF];
	sprintf(txttime, "%s", "");

	int year0, month0, day0, hour0, minute0, isec;
	double sec_fraction;
	date_and_time_from_MJD(MJD, &year0, &month0, &day0, &hour0, &minute0, &isec, &sec_fraction);
	isec = int(round(double(isec) + sec_fraction));


	sprintf(txttime, "%04d-%02d-%02dT%02d:%02d:%02dZ", year0, month0, day0, hour0, minute0, isec);
	return std::string(txttime);
}
//------------------------------------------------------------------------------
std::string timestamp_UTC_1ms(double MJD)
{
	char txttime[MAXBUF];
	sprintf(txttime, "%s", "");

	int year0, month0, day0, hour0, minute0, isec;
	double sec_fraction;
	date_and_time_from_MJD(MJD, &year0, &month0, &day0, &hour0, &minute0, &isec, &sec_fraction);
	double sec = double(isec) + sec_fraction;

	sprintf(txttime, "%04d-%02d-%02dT%02d:%02d:%06.3lfZ", year0, month0, day0, hour0, minute0, sec);
	return std::string(txttime);
}
//------------------------------------------------------------------------------
void date_and_time_from_MJD(
	double MJD,
	int* year,
	int* month,
	int* day,
	int* hour,
	int* minute,
	int* isec,
	double* sec_fraction
)
{
	long int st = 678881L;
	long int st1 = 146097L;
	long int a = 4L * (int(MJD) + st) + 3L;
	long int b = 0.;
	if (st1 != 0.)
		b = a / st1;
	else
		b = 0.;//error
	long int c = a - b * st1;
	long int f = 4 * (c / 4) + 3;
	c = f / 1461;
	f = (f - c * 1461 + 4) / 4;
	long int e = f * 5 - 3;
	*year = (int)(b * 100 + c);
	*day = (int)((e - (e / 153) * 153 + 5) / 5);
	*month = (int)(e / 153 + 3);
	if (*month > 12)
	{
		*month -= 12;
		++* year;
	}


	double time = (MJD - int(MJD)) * 86400.;
	*hour = int(time) / 3600;
	*minute = (int(time) - *hour * 3600) / 60;
	double dsec = time - double(*hour * 3600) - double(*minute * 60);
	*isec = int(dsec);
	*sec_fraction = dsec - double(*isec);
}
//------------------------------------------------------------------------------