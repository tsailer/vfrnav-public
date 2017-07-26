//
// C++ Implementation: grib2dump
//
// Description: GRIB2 File Header Dump
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013, 2015
//
// Copyright: See COPYING file that comes with this distribution
//
//

// http://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc.shtml
// http://www.wmo.int/pages/prog/www/WMOCodes/Guides/GRIB/GRIB2_062006.pdf
// http://www.ftp.ncep.noaa.gov/data/nccf/com/gfs/prod/gfs.2013032306/gfs.t06z.pgrb2f03

#include "sysdeps.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <cmath>
#include <glibmm.h>
#include <giomm.h>
#include <glibmm/datetime.h>
#ifdef HAVE_OPENJPEG
#include <openjpeg.h>
#endif

#include "grib2.h"

class GRIB2File : public GRIB2 {
public:
	GRIB2File(void);


	int parse(const std::string& filename);

	int section0(Glib::RefPtr<Gio::FileInputStream> instream);
	int section1(const uint8_t *sec, unsigned int len);
	int section2(const uint8_t *sec, unsigned int len);
	int section3(const uint8_t *sec, unsigned int len);
	int section4(const uint8_t *sec, unsigned int len);
	int section5(const uint8_t *sec, unsigned int len);
	int section6(const uint8_t *sec, unsigned int len);
	int section7(Glib::RefPtr<Gio::FileInputStream> instream, unsigned int len);

protected:
	struct simple_id {
		uint8_t id;
		const char *str;
	};

	struct simple16_id {
		uint16_t id;
		const char *str;
	};

	static const struct simple_id sigreftime_id[];
	static const struct simple_id sourcegriddef_id[];
	static const struct simple16_id gridtempl_id[];
	static const struct simple_id earthshape_id[];
	static const struct simple16_id proddeftempl_id[];
	static const struct simple16_id datareprtempl_id[];
	static const struct simple_id bitmapind_id[];
	static const struct simple_id timerangeunit_id[];
	static const struct simple_id cplxpkgorigfieldtype_id[];
	static const struct simple_id cplxpkggrpsplit_id[];
	static const struct simple_id cplxpkgmissvalmgmt_id[];
	static const struct simple_id cplxpkgspatdifforder_id[];

	struct surface_id {
		uint8_t id;
		const char *str;
		const char *unit;
	};

	static const struct simple_id statprocess_id[];
	static const struct simple_id stattimeincr_id[];

	uint16_t m_centerid;
	uint16_t m_datarepresentation;
	uint8_t m_productdiscipline;
	float m_scaler;
	int16_t m_scalee;
	int16_t m_scaled;
};

const struct GRIB2File::simple_id GRIB2File::sigreftime_id[] = {
	{ 0, "Analysis" },
	{ 1, "Start of Forecast" },
	{ 2, "Verifying Time of Forecast" },
	{ 3, "Observation Time" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::sourcegriddef_id[] = {
	{ 0, "Specified" },
	{ 1, "Predetermined Grid Definition - Defined by Originating Center" },
	{ 255, "A grid definition does not apply to this product." },
	{ 0, 0 }
};

const struct GRIB2File::simple16_id GRIB2File::gridtempl_id[] = {
	{ 0, "Latitude/Longitude" },
	{ 1, "Rotated Latitude/Longitude" },
	{ 2, "Stretched Latitude/Longitude" },
	{ 3, "Rotated and Stretched Latitude/Longitude" },
	{ 10, "Mercator" },
	{ 20, "Polar Stereographic Projection" },
	{ 30, "Lambert Conformal" },
	{ 31, "Albers Equal Area" },
	{ 40, "Gaussian Latitude/Longitude" },
	{ 41, "Rotated Gaussian Latitude/Longitude" },
	{ 42, "Stretched Gaussian Latitude/Longitude" },
	{ 43, "Rotated and Stretched Gaussian Latitude/Longitude" },
	{ 44, "Latitude/Longitude With Data-Sampling From A Higher Resolution Latitude/Longitude Source-Grid" },
	{ 50, "Spherical Harmonic Coefficients" },
	{ 51, "Rotated Spherical Harmonic Coefficients" },
	{ 52, "Stretched Spherical Harmonic Coefficients" },
	{ 53, "Rotated and Stretched Spherical Harmonic Coefficients" },
	{ 90, "Space View Perspective or Orthographic" },
	{ 100, "Triangular Grid Based on an Icosahedron" },
	{ 101, "General Unstructured Grid" },
	{ 110, "Equatorial Azimuthal Equidistant Projection" },
	{ 120, "Azimuth-Range Projection" },
	{ 130, "Irregular Latitude/Longitude" },
	{ 204, "Curvilinear Orthogonal Grids" },
	{ 1000, "Cross Section Grid with Points Equally Spaced on the Horizontal" },
	{ 1100, "Hovmoller Diagram with Points Equally Spaced on the Horizontal" },
	{ 1200, "Time Section Grid" },
	{ 32768, "Rotated Latitude/Longitude (Arakawa Staggered E-Grid)" },
	{ 32769, "Rotated Latitude/Longitude (Arakawa Non-E Staggered Grid)" },
	{ 65535, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::earthshape_id[] = {
	{ 0, "Earth assumed spherical with radius = 6,367,470.0 m" },
	{ 1, "Earth assumed spherical with radius specified (in m) by data producer" },
	{ 2, "Earth assumed oblate spheriod with size as determined by IAU in 1965 (major axis = 6,378,160.0 m, minor axis = 6,356,775.0 m, f = 1/297.0)" },
	{ 3, "Earth assumed oblate spheriod with major and minor axes specified (in km) by data producer" },
	{ 4, "Earth assumed oblate spheriod as defined in IAG-GRS80 model (major axis = 6,378,137.0 m, minor axis = 6,356,752.314 m, f = 1/298.257222101)" },
	{ 5, "Earth assumed represented by WGS84 (as used by ICAO since 1998) (Uses IAG-GRS80 as a basis)" },
	{ 6, "Earth assumed spherical with radius = 6,371,229.0 m" },
	{ 7, "Earth assumed oblate spheroid with major and minor axes specified (in m) by data producer" },
	{ 8, "Earth model assumed spherical with radius 6,371,200 m, but the horizontal datum of the resulting Latitude/Longitude field is the WGS84 reference frame" },
	{ 9, "Earth represented by the OSGB 1936 Datum, using the Airy_1830 Spheroid, the Greenwich meridian as 0 Longitude, the Newlyn datum as mean sea level, 0 height." },
	{ 10, "Earth model assumed WGS84 with Corrected Geomagnetic Coordinates (Latitude and Longitude) defined by (Gustafsson et al., 1992)" },
	{ 11, "Sun model assumed spherical with radius=695,990.000 m (Allen, C.W., 1976 Astrophysical Quantities (3rd Ed.; London: Athlone)). Stonyhurst latitude and longitude system with origin at the intersection of the solar central meridian as seen from Earth and the solar equator. See Thompson, W, Coordinate systems for solar image data, A&A 449, 791-803 (2006)" },
	{ 12, "Sun model assumed spherical with radius=695,990.000 m (Allen, C.W., 1976 Astrophysical Quantities (3rd Ed.; London: Athlone)). Carrington latitude and longitude system that rotate with a side real period of 25.38 days. See Thompson, W, Coordinate systems for solar image data, A&A 449, 791-803 (2006)" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple16_id GRIB2File::proddeftempl_id[] = {
	{ 0, "Analysis or forecast at a horizontal level or in a horizontal layer at a point in time." },
	{ 1, "Individual ensemble forecast, control and perturbed, at a horizontal level or in a horizontal layer at a point in time." },
	{ 2, "Derived forecasts based on all ensemble members at a horizontal level or in a horizontal layer at a point in time." },
	{ 3, "Derived forecasts based on a cluster of ensemble members over a rectangular area at a horizontal level or in a horizontal layer at a point in time." },
	{ 4, "Derived forecasts based on a cluster of ensemble members over a circular area at a horizontal level or in a horizontal layer at a point in time." },
	{ 5, "Probability forecasts at a horizontal level or in a horizontal layer at a point in time." },
	{ 6, "Percentile forecasts at a horizontal level or in a horizontal layer at a point in time." },
	{ 7, "Analysis or forecast error at a horizontal level or in a horizontal layer at a point in time." },
	{ 8, "Average, accumulation, extreme values or other statistically processed values at a horizontal level or in a horizontal layer in a continuous or non-continuous time interval." },
	{ 9, "Probability forecasts at a horizontal level or in a horizontal layer in a continuous or non-continuous time interval." },
	{ 10, "Percentile forecasts at a horizontal level or in a horizontal layer in a continuous or non-continuous time interval." },
	{ 11, "Individual ensemble forecast, control and perturbed, at a horizontal level or in a horizontal layer, in a continuous or non-continuous time interval." },
	{ 12, "Derived forecasts based on all ensemble members at a horizontal level or in a horizontal layer, in a continuous or non-continuous time interval." },
	{ 13, "Derived forecasts based on a cluster of ensemble members over a rectangular area at a horizontal level or in a horizontal layer, in a continuous or non-continuous time interval." },
	{ 14, "Derived forecasts based on a cluster of ensemble members over a circular area at a horizontal level or in a horizontal layer, in a continuous or non-continuous time interval." },
	{ 15, "Average, accumulation, extreme values or other statistically-processed values over a spatial area at a horizontal level or in a horizontal layer at a point in time." },
	{ 20, "Radar product" },
	{ 30, "Satellite product (deprecated)" },
	{ 31, "Satellite product" },
	{ 32, "Analysis or forecast at a horizontal level or in a horizontal layer at a point in time for simulate" },
	{ 40, "Analysis or forecast at a horizontal level or in a horizontal layer at a point in time for atmospheric chemical constituents." },
	{ 41, "Individual ensemble forecast, control and perturbed, at a horizontal level or in a horizontal layer at a point in time for atmospheric chemical constituents." },
	{ 42, "Average, accumulation, and/or extreme values or other statistically processed values at a horizontal level or in a horizontal layer in a continuous or non-continuous time interval for atmospheric chemical constituents." },
	{ 43, "Individual ensemble forecast, control and perturbed, at a horizontal level or in a horizontal layer, in a continuous or non-continuous time interval for atmospheric chemical constituents." },
	{ 44, "Analysis or forecast at a horizontal level or in a horizontal layer at a point in time for aerosol." },
	{ 45, "Individual ensemble forecast, control and perturbed, at a horizontal level or in a horizontal layer, in a continuous or non-continuous time interval for aerosol." },
	{ 46, "Average, accumulation, and/or extreme values or other statistically processed values at a horizontal level or in a horizontal layer in a continuous or non-continuous time interval for aerosol." },
	{ 47, "Individual ensemble forecast, control and perturbed, at a horizontal level or in a horizontal layer, in a continuous or non-continuous time interval for aerosol." },
	{ 48, "Analysis or forecast at a horizontal level or in a horizontal layer at a point in time for aerosol." },
	{ 50 , "Analysis or forecast of a multi component parameter or matrix element at error at a point in time." },
	{ 51, "Categorical forecast at a horizontal level or in a horizontal layer at a point in time." },
	{ 52 , "Analysis or forecast of Wave Parameters at the Sea Surface at a point in time." },
	{ 91, "Categorical forecast at a horizontal level or in a horizontal layer in a continuous or non-continuous time interval." },
	{ 254, "CCITT IA5 character string" },
	{ 1000, "Cross-section of analysis and forecast at a point in time." },
	{ 1001, "Cross-section of averaged or otherwise statistically processed analysis or forecast over a range of time." },
	{ 1002, "Cross-section of analysis and forecast, averaged or otherwise statistically-processed over latitude or longitude." },
	{ 1100, "Hovmoller-type grid with no averaging or other statistical processing" },
	{ 1101, "Hovmoller-type grid with averaging or other statistical processing" },
	{ 65535, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple16_id GRIB2File::datareprtempl_id[] = {
	{ 0, "Grid Point Data - Simple Packing" },
	{ 1, "Matrix Value at Grid Point - Simple Packing" },
	{ 2, "Grid Point Data - Complex Packing" },
	{ 3, "Grid Point Data - Complex Packing and Spatial Differencing" },
	{ 4, "Grid Point Data - IEEE Floating Point Data" },
	{ 40, "Grid Point Data - JPEG2000 Compression" },
	{ 41 , "Grid Point Data - PNG Compression" },
	{ 42, "Grid Point and Spectral Data - CCSDS szip" },
	{ 50, "Spectral Data - Simple Packing" },
	{ 51, "Spectral Data - Complex Packing" },
	{ 61, "Grid Point Data - Simple Packing With Logarithm Pre-processing" },
	{ 200, "Run Length Packing With Level Values" },
	{ 65535, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::bitmapind_id[] = {
	{ 0, "A bit map applies to this product and is specified in this section." },
	{ 254, "A bit map previously defined in the same GRIB2 message applies to this product." },
	{ 255, "A bit map does not apply to this product." },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::timerangeunit_id[] = {
	{ 0, "Minute" },
	{ 1, "Hour" },
	{ 2, "Day" },
	{ 3, "Month" },
	{ 4, "Year" },
	{ 5, "Decade" },
	{ 6, "Normal (30 Years)" },
	{ 7, "Century" },
	{ 8, "Reserved" },
	{ 9, "Reserved" },
	{ 10, "3 Hours" },
	{ 11, "6 Hours" },
	{ 12, "12 Hours" },
	{ 13, "Second" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::statprocess_id[] = {
	{ 0, "Average" },
	{ 1, "Accumulation" },
	{ 2, "Maximum" },
	{ 3, "Minimum" },
	{ 4, "Difference (value at the end of the time range minus value at the beginning)" },
	{ 5, "Root Mean Square" },
	{ 6, "Standard Deviation" },
	{ 7, "Covariance (temporal variance)" },
	{ 8, "Difference ( value at the beginning of the time range minus value at the end)" },
	{ 9, "Ratio" },
	{ 10, "Standardized Anomaly" },
	{ 192, "Climatological Mean Value: multiple year averages of quantities which are themselves means over some period of time (P2) less than a year. The reference time (R) indicates the date and time of the start of a period of time, given by R to R + P2, over which a mean is formed; N indicates the number of such period-means that are averaged together to form the climatological value, assuming that the N period-mean fields are separated by one year. The reference time indicates the start of the N-year climatology. N is given in octets 22-23 of the PDS. If P1 = 0 then the data averaged in the basic interval P2 are assumed to be continuous, i.e., all available data are simply averaged together. If P1 = 1 (the units of time - octet 18, code table 4 - are not relevant here) then the data averaged together in the basic interval P2 are valid only at the time (hour, minute) given in the reference time, for all the days included in the P2 period. The units of P2 are given by the contents of octet 18 and Table 4." },
	{ 193, "Average of N forecasts (or initialized analyses); each product has forecast period of P1 (P1=0 for initialized analyses); products have reference times at intervals of P2, beginning at the given reference time." },
	{ 194, "Average of N uninitialized analyses, starting at reference time, at intervals of P2." },
	{ 195, "Average of forecast accumulations. P1 = start of accumulation period. P2 = end of accumulation period. Reference time is the start time of the first forecast, other forecasts at 24-hour intervals. Number in Ave = number of forecasts used." },
	{ 196, "Average of successive forecast accumulations. P1 = start of accumulation period. P2 = end of accumulation period. Reference time is the start time of the first forecast, other forecasts at (P2 - P1) intervals. Number in Ave = number of forecasts used" },
	{ 197, "Average of forecast averages. P1 = start of averaging period. P2 = end of averaging period. Reference time is the start time of the first forecast, other forecasts at 24-hour intervals. Number in Ave = number of forecast used" },
	{ 198, "Average of successive forecast averages. P1 = start of averaging period. P2 = end of averaging period. Reference time is the start time of the first forecast, other forecasts at (P2 - P1) intervals. Number in Ave = number of forecasts used" },
	{ 199, "Climatological Average of N analyses, each a year apart, starting from initial time R and for the period from R+P1 to R+P2." },
	{ 200, "Climatological Average of N forecasts, each a year apart, starting from initial time R and for the period from R+P1 to R+P2." },
	{ 201, "Climatological Root Mean Square difference between N forecasts and their verifying analyses, each a year apart, starting with initial time R and for the period from R+P1 to R+P2." },
	{ 202, "Climatological Standard Deviation of N forecasts from the mean of the same N forecasts, for forecasts one year apart. The first forecast starts wtih initial time R and is for the period from R+P1 to R+P2." },
	{ 203, "Climatological Standard Deviation of N analyses from the mean of the same N analyses, for analyses one year apart. The first analyses is valid for  period R+P1 to R+P2." },
	{ 204, "Average of forecast accumulations. P1 = start of accumulation period. P2 = end of accumulation period. Reference time is the start time of the first forecast, other forecasts at 6-hour intervals. Number in Ave = number of forecast used" },
	{ 205, "Average of forecast averages. P1 = start of averaging period. P2 = end of averaging period. Reference time is the start time of the first forecast, other forecasts at 6-hour intervals. Number in Ave = number of forecast used" },
	{ 206, "Average of forecast accumulations. P1 = start of accumulation period. P2 = end of accumulation period. Reference time is the start time of the first forecast, other forecasts at 12-hour intervals. Number in Ave = number of forecast used" },
	{ 207, "Average of forecast averages. P1 = start of averaging period. P2 = end of averaging period. Reference time is the start time of the first forecast, other forecasts at 12-hour intervals. Number in Ave = number of forecast used" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::stattimeincr_id[] = {
	{ 1, "Successive times processed have same forecast time, start time of forecast is incremented." },
	{ 2, "Successive times processed have same start time of forecast, forecast time is incremented." },
	{ 3, "Successive times processed have start time of forecast incremented and forecast time decremented so that valid time remains constant." },
	{ 4, "Successive times processed have start time of forecast decremented and forecast time incremented so that valid time remains constant." },
	{ 5, "Floating subinterval of time between forecast time and end of overall time interval.(see Note 1)" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::cplxpkgorigfieldtype_id[] = {
	{ 0, "Floating Point" },
	{ 1, "Integer" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::cplxpkggrpsplit_id[] = {
	{ 0, "Row by Row Splitting" },
	{ 1, "General Group Splitting" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::cplxpkgmissvalmgmt_id[] = {
	{ 0, "No explicit missing values included within the data values" },
	{ 1, "Primary missing values included within the data values" },
	{ 2, "Primary and secondary missing values included within the data values" },
	{ 255, "Missing" },
	{ 0, 0 }
};

const struct GRIB2File::simple_id GRIB2File::cplxpkgspatdifforder_id[] = {
	{ 0, "Reserved" },
	{ 1, "First-Order Spatial Differencing" },
	{ 2, "Second-Order Spatial Differencing" },
	{ 255, "Missing" },
	{ 0, 0 }
};

GRIB2File::GRIB2File(void)
	: m_centerid(0xffff), m_datarepresentation(0), m_productdiscipline(255), m_scaler(0), m_scalee(0), m_scaled(0)
{
}

int GRIB2File::parse(const std::string& filename)
{
	Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(filename));
	Glib::RefPtr<Gio::FileInputStream> instream(file->read());
	int r(section0(instream));
	if (r)
		return r;
	return 0;
}

int GRIB2File::section0(Glib::RefPtr<Gio::FileInputStream> instream)
{
	for (;;) {
		uint8_t sec0[16];
		gssize r(instream->read(sec0, sizeof(sec0)));
		if (!r)
			return 0;
		if (r < sizeof(sec0))
			return -1;
		if (memcmp(&sec0[0], &grib2_header[0], sizeof(grib2_header)))
			return -2;
		if (sec0[7] != 0x02)
			return -3;
		unsigned int len(sec0[15] | (((unsigned int)sec0[14]) << 8) |
				 (((unsigned int)sec0[13]) << 16) | (((unsigned int)sec0[12]) << 24));
		const ParamDiscipline *disc(find_discipline(sec0[6]));
		std::cerr << "Section 0: Indicator Section" << std::endl
			  << "Discipline " << (unsigned int)sec0[6] << " (" << disc->get_str("?") << ')' << std::endl
			  << "File length " << len << std::endl << std::endl;
		m_productdiscipline = sec0[6];
		for (;;) {
			uint8_t sec[5];
			{
				gssize r(instream->read(sec, 4));
				if (r < 4)
					break;
			}
			unsigned int len(sec[3] | (((unsigned int)sec[2]) << 8) |
					 (((unsigned int)sec[1]) << 16) | (((unsigned int)sec[0]) << 24));
			if (len == grib2_terminator)
				break;
			{
				gssize r(instream->read(sec + 4, sizeof(sec) - 4));
				if (r < sizeof(sec) - 4)
					break;
			}
			std::cerr << "Section Length " << len << std::endl
				  << "Section ID " << (unsigned int)sec[4] << std::endl
				  << "File Offset " << (instream->tell() - sizeof(sec)) << std::endl;
			unsigned int len1(std::max(len, (unsigned int)sizeof(sec)) - sizeof(sec));
			if (sec[4] == 7) {
				int r(section7(instream, len1));
				if (r < 0)
					return r;
				continue;
			}
			if (len1 > 65536) {
				instream->skip(len1);
				continue;
			}
			uint8_t buf[len1];
			if (len1 > 0) {
				gssize r(instream->read(buf, len1));
				if (r < len1)
					return -1;
			}
			switch (sec[4]) {
			case 1:
			{
				int r(section1(buf, len1));
				if (r < 0)
					return r;
				break;
			}

			case 2:
			{
				int r(section2(buf, len1));
				if (r < 0)
					return r;
				break;
			}

			case 3:
			{
				int r(section3(buf, len1));
				if (r < 0)
					return r;
				break;
			}

			case 4:
			{
				int r(section4(buf, len1));
				if (r < 0)
					return r;
				break;
			}

			case 5:
			{
				int r(section5(buf, len1));
				if (r < 0)
					return r;
				break;
			}

			case 6:
			{
				int r(section6(buf, len1));
				if (r < 0)
					return r;
				break;
			}

			default:
				break;
			}
		}
	}
	return 0;
}

int GRIB2File::section1(const uint8_t *sec, unsigned int len)
{
	if (len < 16)
		return -2;
	uint16_t centerid(sec[1] | (((unsigned int)sec[0]) << 8));
	m_centerid = centerid;
	uint16_t subcenterid(sec[3] | (((unsigned int)sec[2]) << 8));
	const struct simple_id *srtid(sigreftime_id);
	while (srtid->str && srtid->id != sec[6])
		++srtid;
	Glib::DateTime reftime(Glib::DateTime::create_utc((((unsigned int)sec[7]) << 8) | sec[8], sec[9], sec[10],
							  sec[11], sec[12], sec[13]));
	std::cerr << "Section 1: Identification Section" << std::endl
		  << "Center ID " << centerid << " (" << find_centerid_str(centerid, "?") << ')' << std::endl
		  << "Subcenter ID " << subcenterid << " (" << find_subcenterid_str(centerid, subcenterid, "?") << ')' << std::endl
		  << "GRIB master tables version " << (unsigned int)sec[4] << std::endl
		  << "GRIB local tables version " << (unsigned int)sec[5] << std::endl
		  << "Significance of reference time " << (unsigned int)sec[6]
		  << " (" << (srtid->str ? srtid->str : "?") << ')' << std::endl
		  << "Reference Time " << reftime.format("%F %H:%M:%S") << std::endl
		  << "Production Status " << (unsigned int)sec[14] << " (" << find_productionstatus_str(sec[14], "?") << ')' << std::endl
		  << "Type of processed data " << (unsigned int)sec[15] << " (" << find_datatype_str(sec[15], "?") << ')' << std::endl
		  << std::endl;
	return 0;
}

int GRIB2File::section2(const uint8_t *sec, unsigned int len)
{
	std::cerr << "Section 2: Local Section" << std::endl
		  << std::endl;
	return 0;
}

int GRIB2File::section3(const uint8_t *sec, unsigned int len)
{
	if (len < 10)
		return -2;
	unsigned int ndata(sec[4] | (((unsigned int)sec[3]) << 8) |
			   (((unsigned int)sec[2]) << 16) | (((unsigned int)sec[1]) << 24));
	const struct simple_id *gdid(sourcegriddef_id);
	while (gdid->str && gdid->id != sec[0])
		++gdid;
	uint16_t gridtempl(sec[8] | (((unsigned int)sec[7]) << 8));
	const struct simple16_id *gtid(gridtempl_id);
	while (gtid->str && gtid->id != gridtempl)
		++gtid;
	std::cerr << "Section 3: Grid Definition Section" << std::endl
		  << "Source of grid definition " << (unsigned int)sec[0] << " (" << (gdid->str ? gdid->str : "?") << ')' << std::endl
		  << "Number of data points " << ndata << std::endl
		  << "Number of octets for optional list of numbers defining number of points " << (unsigned int)sec[5] << std::endl
		  << "Interpolation of list of numbers defining number of points " << (unsigned int)sec[6] << std::endl
		  << "Grid Definition Template " << gridtempl << " (" << (gtid->str ? gtid->str : "?") << ')' << std::endl;
	switch (gridtempl) {
	case 0:
	{
		if (len < 67)
			return -2;
		const struct simple_id *esid(earthshape_id);
		while (esid->str && esid->id != sec[9])
			++esid;
		uint32_t radiusearth(sec[14] | (((uint32_t)sec[13]) << 8) |
				     (((uint32_t)sec[12]) << 16) | (((uint32_t)sec[11]) << 24));
		uint32_t majoraxis(sec[19] | (((uint32_t)sec[18]) << 8) |
				   (((uint32_t)sec[17]) << 16) | (((uint32_t)sec[16]) << 24));
		uint32_t minoraxis(sec[24] | (((uint32_t)sec[23]) << 8) |
				   (((uint32_t)sec[22]) << 16) | (((uint32_t)sec[21]) << 24));
		uint32_t Ni(sec[28] | (((uint32_t)sec[27]) << 8) |
			    (((uint32_t)sec[26]) << 16) | (((uint32_t)sec[25]) << 24));
		uint32_t Nj(sec[32] | (((uint32_t)sec[31]) << 8) |
			    (((uint32_t)sec[30]) << 16) | (((uint32_t)sec[29]) << 24));
		uint32_t basicangle(sec[36] | (((uint32_t)sec[35]) << 8) |
				    (((uint32_t)sec[34]) << 16) | (((uint32_t)sec[33]) << 24));
		uint32_t subdivbasicangle(sec[40] | (((uint32_t)sec[39]) << 8) |
					  (((uint32_t)sec[38]) << 16) | (((uint32_t)sec[37]) << 24));
		int32_t lat1(sec[44] | (((uint32_t)sec[43]) << 8) |
			     (((uint32_t)sec[42]) << 16) | (((uint32_t)sec[41]) << 24));
		int32_t lon1(sec[48] | (((uint32_t)sec[47]) << 8) |
			     (((uint32_t)sec[46]) << 16) | (((uint32_t)sec[45]) << 24));
		int32_t lat2(sec[53] | (((uint32_t)sec[52]) << 8) |
			     (((uint32_t)sec[51]) << 16) | (((uint32_t)sec[50]) << 24));
		int32_t lon2(sec[57] | (((uint32_t)sec[56]) << 8) |
			     (((uint32_t)sec[55]) << 16) | (((uint32_t)sec[54]) << 24));
		int32_t Di(sec[61] | (((uint32_t)sec[60]) << 8) |
			   (((uint32_t)sec[59]) << 16) | (((uint32_t)sec[58]) << 24));
		int32_t Dj(sec[65] | (((uint32_t)sec[64]) << 8) |
			   (((uint32_t)sec[63]) << 16) | (((uint32_t)sec[62]) << 24));
		if (lat1 & 0x80000000) {
			lat1 &= 0x7fffffff;
			lat1 = -lat1;
		}
		if (lon1 & 0x80000000) {
			lon1 &= 0x7fffffff;
			lon1 = -lon1;
		}
		if (lat2 & 0x80000000) {
			lat2 &= 0x7fffffff;
			lat2 = -lat2;
		}
		if (lon2 & 0x80000000) {
			lon2 &= 0x7fffffff;
			lon2 = -lon2;
		}
		if (Di & 0x80000000) {
			Di &= 0x7fffffff;
			Di = -Di;
		}
		if (Dj & 0x80000000) {
			Dj &= 0x7fffffff;
			Dj = -Dj;
		}
		std::cerr << "Shape of the Earth " << (unsigned int)sec[9] << " (" << (esid->str ? esid->str : "?") << ')' << std::endl
			  << "Radius of Spherical Earth " << radiusearth << ',' << (unsigned int)sec[10] << std::endl
			  << "Major Axis of Oblate Spheroid Earth " << majoraxis << ',' << (unsigned int)sec[15] << std::endl
			  << "Minor Axis of Oblate Spheroid Earth " << minoraxis << ',' << (unsigned int)sec[20] << std::endl
			  << "Ni " << Ni << std::endl
			  << "Nj " << Nj << std::endl
			  << "Basic angle of the initial production domain " << basicangle << std::endl
			  << "Subdivisions of basic angle used to define extreme lon/lat " << subdivbasicangle << std::endl
			  << "Latitude of first Grid Point " << lat1 << std::endl
			  << "Longitude of first Grid Point " << lon1 << std::endl
			  << "Resolution/Component Flags " << (unsigned int)sec[49] << std::endl
			  << "  " << ((sec[49] & 32) ? "i direction increments given" : "i direction increments not given") << std::endl
			  << "  " << ((sec[49] & 16) ? "j direction increments given" : "j direction increments not given") << std::endl
			  << "  " << ((sec[49] & 8) ? "Resolved u and v components of vector quantities relative to the defined grid in the direction of increasing x and y (or i and j) coordinates, respectively" : "Resolved u and v components of vector quantities relative to easterly and northerly directions") << std::endl
			  << "Latitude of last Grid Point " << lat2 << std::endl
			  << "Longitude of last Grid Point " << lon2 << std::endl
			  << "Di " << Di << std::endl
			  << "Dj " << Dj << std::endl
			  << "Scanning Mode " << (unsigned int)sec[66] << std::endl
			  << "  " << ((sec[66] & 128) ? "Points in the first row or column scan in the -i (-x) direction" : "Points in the first row or column scan in the +i (+x) direction") << std::endl
			  << "  " << ((sec[66] & 64) ? "Points in the first row or column scan in the +j (+y) direction" : "Points in the first row or column scan in the -j (-y) direction") << std::endl
			  << "  " << ((sec[66] & 32) ? "Adjacent points in the j (y) direction are consecutive" : "Adjacent points in the i (x) direction are consecutive") << std::endl
			  << "  " << ((sec[66] & 16) ? "Adjacent rows scan in the opposite direction" : "All rows scan in the same direction") << std::endl;
		break;
	}

	default:
		break;
	}
	std::cerr << std::endl;
	return 0;
}

int GRIB2File::section4(const uint8_t *sec, unsigned int len)
{
	if (len < 4)
		return -2;
	uint16_t nrcoordval(sec[1] | (((unsigned int)sec[0]) << 8));
	uint16_t proddeftempl(sec[3] | (((unsigned int)sec[2]) << 8));
	const struct simple16_id *pdid(proddeftempl_id);
	while (pdid->str && pdid->id != proddeftempl)
		++pdid;
	std::cerr << "Section 4: Product Definition Section" << std::endl
		  << "Number of coordinate values after template " << nrcoordval << std::endl
		  << "Product definition template " << proddeftempl << " (" << (pdid->str ? pdid->str : "?") << ')' << std::endl;
	switch (proddeftempl) {
	case 0:
	case 8:
	{
		if (len < 29)
			return -2;
		uint16_t hrcutoff(sec[10] | (((unsigned int)sec[9]) << 8));
		uint32_t fcsttime(sec[16] | (((uint32_t)sec[15]) << 8) |
				  (((uint32_t)sec[14]) << 16) | (((uint32_t)sec[13]) << 24));
		uint32_t fixsfc1(sec[22] | (((uint32_t)sec[21]) << 8) |
				 (((uint32_t)sec[20]) << 16) | (((uint32_t)sec[19]) << 24));
		uint32_t fixsfc2(sec[28] | (((uint32_t)sec[27]) << 8) |
				 (((uint32_t)sec[26]) << 16) | (((uint32_t)sec[25]) << 24));
		const ParamDiscipline *pardisc(find_discipline(m_productdiscipline));
		const ParamCategory *parcat(find_paramcategory(pardisc, sec[4]));
		const Parameter *param(find_parameter(parcat, sec[5]));
		const struct simple_id *truid(timerangeunit_id);
		while (truid->str && truid->id != sec[12])
			++truid;
		std::cerr << "Parameter category " << (unsigned int)sec[4] << " (" << parcat->get_str("?") << ')' << std::endl
			  << "Parameter number " << (unsigned int)sec[5] << " (" << param->get_str("?") << ','
			  << param->get_unit("-") << ',' << param->get_abbrev("?") << ')' << std::endl
			  << "Type of generating process " << (unsigned int)sec[6] << " (" << find_genprocess_str(sec[6], "?") << ')' << std::endl
			  << "Background generating process identifier " << (unsigned int)sec[7] << std::endl
			  << "Analysis or forecast generating process identifier " << (unsigned int)sec[8]
			  << " (" << find_genprocesstype_str(m_centerid, sec[8], "?") << ')' << std::endl
			  << "Hours of observational data cutoff after reference time " << hrcutoff << std::endl
			  << "Minutes of observational data cutoff after reference time " << (unsigned int)sec[11] << std::endl
			  << "Indicator of unit of time range " << (unsigned int)sec[12] << " (" << (truid->str ? truid->str : "?") << ')' << std::endl
			  << "Forecast time " << fcsttime << std::endl
			  << "Type of first fixed surface " << (unsigned int)sec[17] << " (" << find_surfacetype_str(sec[17], "?")
			  << ", " << find_surfaceunit_str(sec[17], "?") << ')' << std::endl
			  << "Scale factor of first fixed surface " << fixsfc1 << ',' << (unsigned int)sec[18] << " = " << (fixsfc1 * pow(0.1, (int8_t)sec[18])) << std::endl
			  << "Type of second fixed surface " << (unsigned int)sec[23] << " (" << find_surfacetype_str(sec[23], "?")
			  << ", " << find_surfaceunit_str(sec[23], "?") << ')' << std::endl
			  << "Scale factor of second fixed surface " << fixsfc2 << ',' << (unsigned int)sec[24] << " = " << (fixsfc2 * pow(0.1, (int8_t)sec[24])) << std::endl;
		if (!proddeftempl)
			break;
		if (len < 41)
			return -2;
		Glib::DateTime intvlend(Glib::DateTime::create_utc((((unsigned int)sec[29]) << 8) | sec[30], sec[31], sec[32],
								   sec[33], sec[34], sec[35]));
		uint32_t valmissing(sec[40] | (((uint32_t)sec[39]) << 8) |
				 (((uint32_t)sec[38]) << 16) | (((uint32_t)sec[37]) << 24));
		std::cerr << "Time of end of overall time interval " << intvlend.format("%F %H:%M:%S") << std::endl
			  << "Number of time ranges describing the time intervals used to calculate the statistically-processed field " <<  (unsigned int)sec[36] << std::endl
			  << "Total number of data values missing in statistical process " << valmissing  << std::endl;
		if (len < 41 + 12 * sec[36])
			return -2;
		for (unsigned int j = 0; j < sec[36]; ++j) {
			const struct simple_id *spid(statprocess_id);
			while (spid->str && spid->id != sec[41 + j*12])
				++spid;
			const struct simple_id *stid(stattimeincr_id);
			while (stid->str && stid->id != sec[42 + j*12])
				++stid;
			const struct simple_id *truid(timerangeunit_id);
			while (truid->str && truid->id != sec[43 + j*12])
				++truid;
			uint32_t timerange(sec[47 + j*12] | (((uint32_t)sec[46 + j*12]) << 8) |
					   (((uint32_t)sec[45 + j*12]) << 16) | (((uint32_t)sec[44 + j*12]) << 24));
			uint32_t timeincr(sec[52 + j*12] | (((uint32_t)sec[51 + j*12]) << 8) |
					  (((uint32_t)sec[50 + j*12]) << 16) | (((uint32_t)sec[49 + j*12]) << 24));
			std::cerr << "Time Range Specification " << j << std::endl
				  << "  Statistical process used to calculate the processed field from the field at each time increment during the time range "
				  << (unsigned int)sec[41 + j*12] << " (" << (spid->str ? spid->str : "?") << ')' << std::endl
				  << "  Type of time increment between successive fields used in the statistical processing "
				  << (unsigned int)sec[42 + j*12] << " (" << (stid->str ? stid->str : "?") << ')' << std::endl
				  << "  Indicator of unit of time for time range over which statistical processing is done "
				  << (unsigned int)sec[43 + j*12] << " (" << (truid->str ? truid->str : "?") << ')' << std::endl
				  << "  Length of the time range over which statistical processing is done " << timerange << std::endl
				  << "  Indicator of unit of time for the increment between the successive fields used " << (unsigned int)sec[48 + j*12] << std::endl
				  << "  Time increment between successive fields " << timeincr << std::endl;
		}
		break;
	}

	default:
		break;
	}
	std::cerr << std::endl;
	return 0;
}

int GRIB2File::section5(const uint8_t *sec, unsigned int len)
{
	m_scaler = 0;
	m_scalee = 0;
	m_scaled = 0;
	if (len < 6)
		return -2;
	uint32_t nrdp(sec[3] | (((unsigned int)sec[2]) << 8) |
		      (((uint32_t)sec[1]) << 16) | (((uint32_t)sec[0]) << 24));
	uint16_t datareprtempl(sec[5] | (((unsigned int)sec[4]) << 8));
	m_datarepresentation = datareprtempl;
	const struct simple16_id *drid(datareprtempl_id);
	while (drid->str && drid->id != datareprtempl)
		++drid;
	std::cerr << "Section 5: Data Representation Section" << std::endl
		  << "Number of data points " << nrdp << std::endl
		  << "Data representation template " << datareprtempl << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
	switch (datareprtempl) {
	case 2:
	{
		if (len < 42)
			return -2;
		union {
			float f;
			uint32_t w;
			uint8_t b[4];
		} r;
		r.w = sec[9] | (((uint32_t)sec[8]) << 8) |
			(((uint32_t)sec[7]) << 16) | (((uint32_t)sec[6]) << 24);
		int16_t e(sec[11] | (((unsigned int)sec[10]) << 8));
		if (e & 0x8000) {
			e &= 0x7fff;
			e = -e;
		}
		int16_t d(sec[13] | (((unsigned int)sec[12]) << 8));
		if (d & 0x8000) {
			d &= 0x7fff;
			d = -d;
		}
		m_scaler = r.f;
		m_scalee = e;
		m_scaled = d;
		std::cerr << "Reference value (R) " << r.f << std::endl
			  << "Binary scale factor (E) " << e << std::endl
			  << "Decimal scale factor (D) " << d << std::endl
			  << "Number of bits used for each group reference value for complex packing or spatial differencing " << (unsigned int)sec[14] << std::endl;
		{
			const struct simple_id *drid(cplxpkgorigfieldtype_id);
			while (drid->str && drid->id != sec[15])
				++drid;
			std::cerr << "Type of original field values " << (unsigned int)sec[15] << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
		}
		{
			const struct simple_id *drid(cplxpkggrpsplit_id);
			while (drid->str && drid->id != sec[16])
				++drid;
			std::cerr << "Group splitting method " << (unsigned int)sec[16] << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
		}
		{
			const struct simple_id *drid(cplxpkgmissvalmgmt_id);
			while (drid->str && drid->id != sec[17])
				++drid;
			std::cerr << "Missing value management " << (unsigned int)sec[17] << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
		}
		uint32_t primmiss(sec[21] | (((uint32_t)sec[20]) << 8) | (((uint32_t)sec[19]) << 16) | (((uint32_t)sec[18]) << 24));
		uint32_t secmiss(sec[25] | (((uint32_t)sec[24]) << 8) | (((uint32_t)sec[23]) << 16) | (((uint32_t)sec[22]) << 24));
		uint32_t ng(sec[29] | (((uint32_t)sec[28]) << 8) | (((uint32_t)sec[27]) << 16) | (((uint32_t)sec[26]) << 24));
		uint32_t reggl(sec[35] | (((uint32_t)sec[34]) << 8) | (((uint32_t)sec[33]) << 16) | (((uint32_t)sec[32]) << 24));
		uint32_t lastgrplen(sec[40] | (((uint32_t)sec[39]) << 8) | (((uint32_t)sec[38]) << 16) | (((uint32_t)sec[37]) << 24));
		std::cerr << "Primary missing value substitute " << primmiss << std::endl
			  << "Secondary missing value substitute " << secmiss << std::endl
			  << "Number of groups of data values " << ng << std::endl
			  << "Reference for group widths " << (unsigned int)sec[30] << std::endl
			  << "Number of bits used for the group widths " << (unsigned int)sec[31] << std::endl
			  << "Reference for group lengths " << reggl << std::endl
			  << "Length increment for the group lengths " << (unsigned int)sec[36] << std::endl
			  << "True length of last group " << lastgrplen << std::endl
			  << "Number of bits used for the scaled group lengths " << (unsigned int)sec[41] << std::endl;
		break;
	}

	case 3:
	{
		if (len < 44)
			return -2;
		union {
			float f;
			uint32_t w;
			uint8_t b[4];
		} r;
		r.w = sec[9] | (((uint32_t)sec[8]) << 8) |
			(((uint32_t)sec[7]) << 16) | (((uint32_t)sec[6]) << 24);
		int16_t e(sec[11] | (((unsigned int)sec[10]) << 8));
		if (e & 0x8000) {
			e &= 0x7fff;
			e = -e;
		}
		int16_t d(sec[13] | (((unsigned int)sec[12]) << 8));
		if (d & 0x8000) {
			d &= 0x7fff;
			d = -d;
		}
		m_scaler = r.f;
		m_scalee = e;
		m_scaled = d;
		std::cerr << "Reference value (R) " << r.f << std::endl
			  << "Binary scale factor (E) " << e << std::endl
			  << "Decimal scale factor (D) " << d << std::endl
			  << "Number of bits used for each group reference value for complex packing or spatial differencing " << (unsigned int)sec[14] << std::endl;
		{
			const struct simple_id *drid(cplxpkgorigfieldtype_id);
			while (drid->str && drid->id != sec[15])
				++drid;
			std::cerr << "Type of original field values " << (unsigned int)sec[15] << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
		}
		{
			const struct simple_id *drid(cplxpkggrpsplit_id);
			while (drid->str && drid->id != sec[16])
				++drid;
			std::cerr << "Group splitting method " << (unsigned int)sec[16] << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
		}
		{
			const struct simple_id *drid(cplxpkgmissvalmgmt_id);
			while (drid->str && drid->id != sec[17])
				++drid;
			std::cerr << "Missing value management " << (unsigned int)sec[17] << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
		}
		uint32_t primmiss(sec[21] | (((uint32_t)sec[20]) << 8) | (((uint32_t)sec[19]) << 16) | (((uint32_t)sec[18]) << 24));
		uint32_t secmiss(sec[25] | (((uint32_t)sec[24]) << 8) | (((uint32_t)sec[23]) << 16) | (((uint32_t)sec[22]) << 24));
		uint32_t ng(sec[29] | (((uint32_t)sec[28]) << 8) | (((uint32_t)sec[27]) << 16) | (((uint32_t)sec[26]) << 24));
		uint32_t reggl(sec[35] | (((uint32_t)sec[34]) << 8) | (((uint32_t)sec[33]) << 16) | (((uint32_t)sec[32]) << 24));
		uint32_t lastgrplen(sec[40] | (((uint32_t)sec[39]) << 8) | (((uint32_t)sec[38]) << 16) | (((uint32_t)sec[37]) << 24));
		std::cerr << "Primary missing value substitute " << primmiss << std::endl
			  << "Secondary missing value substitute " << secmiss << std::endl
			  << "Number of groups of data values " << ng << std::endl
			  << "Reference for group widths " << (unsigned int)sec[30] << std::endl
			  << "Number of bits used for the group widths " << (unsigned int)sec[31] << std::endl
			  << "Reference for group lengths " << reggl << std::endl
			  << "Length increment for the group lengths " << (unsigned int)sec[36] << std::endl
			  << "True length of last group " << lastgrplen << std::endl
			  << "Number of bits used for the scaled group lengths " << (unsigned int)sec[41] << std::endl;
		{
			const struct simple_id *drid(cplxpkgspatdifforder_id);
			while (drid->str && drid->id != sec[42])
				++drid;
			std::cerr << "Order of spatial difference " << (unsigned int)sec[42] << " (" << (drid->str ? drid->str : "?") << ')' << std::endl;
		}
		std::cerr << "Number of octets required in the data section to specify extra descriptors " << (unsigned int)sec[43] << std::endl;
		break;
	}
		
	case 40:
	{
		if (len < 18)
			return -2;
		union {
			float f;
			uint32_t w;
			uint8_t b[4];
		} r;
		r.w = sec[9] | (((uint32_t)sec[8]) << 8) |
			(((uint32_t)sec[7]) << 16) | (((uint32_t)sec[6]) << 24);
		int16_t e(sec[11] | (((unsigned int)sec[10]) << 8));
		if (e & 0x8000) {
			e &= 0x7fff;
			e = -e;
		}
		int16_t d(sec[13] | (((unsigned int)sec[12]) << 8));
		if (d & 0x8000) {
			d &= 0x7fff;
			d = -d;
		}
		m_scaler = r.f;
		m_scalee = e;
		m_scaled = d;
		std::cerr << "Reference value (R) " << r.f << std::endl
			  << "Binary scale factor (E) " << e << std::endl
			  << "Decimal scale factor (D) " << d << std::endl
			  << "Number of bits required to hold the resulting scaled and referenced data values " << (unsigned int)sec[14] << std::endl
			  << "Type of original field values " << (unsigned int)sec[15] << std::endl
			  << "Type of Compression used " << (unsigned int)sec[16] << std::endl
			  << "Target compression ratio " << (unsigned int)sec[17] << ":1" << std::endl;
		break;		
	}

	default:
		break;
	}
	std::cerr << std::endl;
	return 0;
}

int GRIB2File::section6(const uint8_t *sec, unsigned int len)
{
	if (len < 1)
		return -2;
	const struct simple_id *bmid(bitmapind_id);
	while (bmid->str && bmid->id != sec[0])
		++bmid;
	std::cerr << "Section 6: Bitmap Section" << std::endl
		  << "Bitmap indicator " << (unsigned int)sec[0] << " (" << (bmid->str ? bmid->str : "?") << ')';
	if (!sec[0]) {
		unsigned int len1(len - 1);
		if (len1 > 256)
			len1 = 256;
		std::ostringstream oss;
		oss << std::hex;
		for (unsigned int i = 0; i < len1; ++i) {
			if (!(i & 15))
				oss << std::endl << std::setfill('0') << std::setw(4) << i << ':';
			oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)sec[i + 1];
		}
		std::cerr << oss.str();
	}
	std::cerr << std::endl << std::endl;
	return 0;
}

int GRIB2File::section7(Glib::RefPtr<Gio::FileInputStream> instream, unsigned int len)
{
	if (!len) {
		std::cerr << "Empty" << std::endl << std::endl;
		return 0;
	}
	std::vector<uint8_t> buf;
	buf.resize(len);
	{
		gssize r(instream->read(&buf[0], buf.size()));
		if (r < buf.size())
			return -3;
	}
	{
		std::ostringstream oss;
		oss << std::hex;
		for (unsigned int i = 0; i < 8 && i < len; ++i)
			oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)buf[i];
		std::cerr << "Data" << oss.str() << std::endl;
	}
	switch (m_datarepresentation) {
	case 40:
	{
#ifdef HAVE_OPENJPEG
		opj_common_struct_t cinfo;
		opj_codestream_info_t cstr_info;
		memset(&cinfo, 0, sizeof(cinfo));
		memset(&cstr_info, 0, sizeof(cstr_info));
		opj_cio_t *cio(opj_cio_open(&cinfo, &buf[0], buf.size()));
		opj_dinfo_t *dinfo(opj_create_decompress(CODEC_J2K));
		opj_dparameters_t param;
		opj_set_default_decoder_parameters(&param);
		opj_setup_decoder(dinfo, &param);
		opj_image_t *img(opj_decode_with_info(dinfo, cio, &cstr_info));
		opj_destroy_decompress(dinfo);
		opj_cio_close(cio);
		std::cerr << "J2K x0 " << img->x0 << std::endl
			  << "J2K y0 " << img->y0 << std::endl
			  << "J2K x1 " << img->x1 << std::endl
			  << "J2K y1 " << img->y1 << std::endl
			  << "J2K num comps " << img->numcomps << std::endl
			  << "J2K colspc " << (int)img->color_space << std::endl
			  << "J2K image_w " << cstr_info.image_w << std::endl
			  << "J2K image_h " << cstr_info.image_h << std::endl;
		for (unsigned int i = 0; i < img->numcomps; ++i) {
			std::cerr << "J2K comp[" << i << "] x0 " << img->comps[i].x0 << std::endl
				  << "J2K comp[" << i << "] y0 " << img->comps[i].y0 << std::endl
				  << "J2K comp[" << i << "] dx " << img->comps[i].dx << std::endl
				  << "J2K comp[" << i << "] dy " << img->comps[i].dy << std::endl
				  << "J2K comp[" << i << "] w " << img->comps[i].w << std::endl
				  << "J2K comp[" << i << "] h " << img->comps[i].h << std::endl
				  << "J2K comp[" << i << "] prec " << img->comps[i].prec << std::endl
				  << "J2K comp[" << i << "] bpp " << img->comps[i].bpp << std::endl
				  << "J2K comp[" << i << "] sgnd " << img->comps[i].sgnd << std::endl
				  << "J2K comp[" << i << "] resno_decoded " << img->comps[i].resno_decoded << std::endl
				  << "J2K comp[" << i << "] factor " << img->comps[i].factor << std::endl
				  << "J2K comp[" << i << "] data";
			double scaled(std::pow(10, -m_scaled));
			for (unsigned int j = 0; j < 4; ++j) {
				int v(img->comps[i].data[j]);
				double y(m_scaler + std::ldexp(v, m_scalee));
				y *= scaled;
				std::cerr << ' ' << y << '(' << v << ')';
			}
			std::cerr << std::endl;
			if (img->comps[i].w > 16 && img->comps[i].h > 200) {
				int v(img->comps[i].data[174 * img->comps[i].w + 16]);
				double y(m_scaler + std::ldexp(v, m_scalee));
				y *= scaled;
				std::cerr << "J2K comp[" << i << "] data 16,174: " << y << '(' << v << ')';
			}
		}
		opj_image_destroy(img);
		opj_destroy_cstr_info(&cstr_info);
#endif
		break;
	}
	}
	std::cerr << std::endl;
	return 0;
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
		{ "files", required_argument, 0, 'f' },
		{ "dirs", required_argument, 0, 'd' },
		{ 0, 0, 0, 0 }
        };
        int c, err(0);

	Glib::init();
	Gio::init();
	Glib::thread_init();
	Glib::set_application_name("grib2dump");
  	std::vector<std::string> paths;
        while ((c = getopt_long(argc, argv, "f:d:", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
		case 'f':
			paths.push_back(optarg);
			break;

 		default:
			err++;
			break;
                }
        }
        if (err || ((optind + 1 > argc) && paths.empty())) {
                std::cerr << "usage: grib2dump <input>" << std::endl;
                return EX_USAGE;
        }
	try {
		{
			GRIB2 grib2;
			unsigned int count(grib2.expire_cache());
			if (count)
				std::cout << "Expired " << count << " cached GRIB2 Layers" << std::endl;
		}
		if (paths.empty()) {
			GRIB2File gf;
			if (gf.parse(argv[optind])) {
				std::cerr << "Error parsing file " << argv[optind] << std::endl;
				return EX_DATAERR;
			}
		} else {
			GRIB2 grib2;
			GRIB2::Parser parser(grib2);
			for (std::vector<std::string>::const_iterator i(paths.begin()), e(paths.end()); i != e; ++i) {
				int r(parser.parse(*i));
				if (r < 0)
					std::cerr << "Error " << r << " parsing path " << *i << std::endl;
				else
					std::cout << "Path " << *i << ": " << r << " layers" << std::endl;
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Cannot parse file " << argv[optind] << ": " << e.what() << std::endl;
		return EX_DATAERR;
	}
        return EX_OK;
}
