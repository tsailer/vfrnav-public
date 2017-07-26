/*****************************************************************************/

/*
 *      gengrib2paramtbl.cc  --  Generate GRIB2 Parameter Tables.
 *
 *      Copyright (C) 2013  Thomas Sailer (t.sailer@alumni.ethz.ch)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*****************************************************************************/

#include "grib2.h"
#include <iostream>
#include <vector>
#include <set>

struct parameter_id {
	uint8_t id;
	const char *str;
	const char *unit;
	const char *abbrev;
};

struct paramcategory_id {
	uint8_t id;
	const char *str;
	const struct parameter_id *par;
};

struct paramdiscipline_id {
	uint8_t id;
	const char *str;
	const struct paramcategory_id *cat;
};

// grib2_table4-2-0-0.shtml

const struct parameter_id parameter_0_0[] = {
	{ 0, "Temperature", "K", "TMP" },
	{ 1, "Virtual Temperature", "K", "VTMP" },
	{ 2, "Potential Temperature", "K", "POT" },
	{ 3, "Pseudo-Adiabatic Potential", "K", "EPOT" },
	{ 4, "Maximum Temperature", "K", "TMAX" },
	{ 5, "Minimum Temperature", "K", "TMIN" },
	{ 6, "Dew Point Temperature", "K", "DPT" },
	{ 7, "Dew Point Depression (or Deficit)", "K", "DEPR" },
	{ 8, "Lapse Rate", "K m^-1", "LAPR" },
	{ 9, "Temperature Anomaly", "K", "TMPA" },
	{ 10, "Latent Heat Net Flux", "W m^-2", "LHTFL" },
	{ 11, "Sensible Heat Net Flux", "W m^-2", "SHTFL" },
	{ 12, "Heat Index", "K", "HEATX" },
	{ 13, "Wind Chill Factor", "K", "WCF" },
	{ 14, "Minimum Dew Point Depression", "K", "MINDPD" },
	{ 15, "Virtual Potential Temperature", "K", "VPTMP" },
	{ 16, "Snow Phase Change Heat Flux", "W m^-2", "SNOHF" },
	{ 17, "Skin Temperature", "K", "SKINT" },
	{ 18, "Snow Temperature (top of snow)", "K", "SNOT" },
	{ 19, "Turbulent Transfer Coefficient for Heat", 0, "TTCHT" },
	{ 20, "Turbulent Diffusion Coefficient for Heat", "m^2 s^-1", "TDCHT" },
	{ 192, "Snow Phase Change Heat Flux", "W m^-2", "SNOHF" },
	{ 193, "Temperature Tendency by All Radiation", "K s^-1", "TTRAD" },
	{ 194, "Relative Error Variance", 0, "REV" },
	{ 195, "Large Scale Condensate Heating Rate", "K s^-1", "LRGHR" },
	{ 196, "Deep Convective Heating Rate", "K s^-1", "CNVHR" },
	{ 197, "Total Downward Heat Flux at Surface", "W m^-2", "THFLX" },
	{ 198, "Temperature Tendency by All Physics", "K s^-1", "TTDIA" },
	{ 199, "Temperature Tendency by Non-radiation Physics", "K s^-1", "TTPHY" },
	{ 200, "Standard Dev. of IR Temp. over 1x1 deg. area", "K", "TSD1D" },
	{ 201, "Shallow Convective Heating Rate", "K s^-1", "SHAHR" },
	{ 202, "Vertical Diffusion Heating rate", "K s^-1", "VDFHR" },
	{ 203, "Potential Temperature at Top of Viscous Sublayer", "K", "THZ0" },
	{ 204, "Tropical Cyclone Heat Potential", "J m^-2 K^-1", "TCHP" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-1.shtml

const struct parameter_id parameter_0_1[] = {
	{ 0, "Specific Humidity", "kg kg^-1", "SPFH" },
	{ 1, "Relative Humidity", "%", "RH" },
	{ 2, "Humidity Mixing Ratio", "kg kg^-1", "MIXR" },
	{ 3, "Precipitable Water", "kg m^-2", "PWAT" },
	{ 4, "Vapour Pressure", "Pa", "VAPP" },
	{ 5, "Saturation Deficit", "Pa", "SATD" },
	{ 6, "Evaporation", "kg m^-2", "EVP" },
	{ 7, "Precipitation Rate", "kg m^-2 s^-1", "PRATE" },
	{ 8, "Total Precipitation", "kg m^-2", "APCP" },
	{ 9, "Large-Scale Precipitation", "kg m^-2", "NCPCP" },
	{ 10, "Convective Precipitation", "kg m^-2", "ACPCP" },
	{ 11, "Snow Depth", "m", "SNOD" },
	{ 12, "Snowfall Rate Water Equivalent", "kg m^-2 s^-1", "SRWEQ" },
	{ 13, "Water Equivalent of Accumulated", "kg m^-2", "WEASD" },
	{ 14, "Convective Snow", "kg m^-2", "SNOC" },
	{ 15, "Large-Scale Snow", "kg m^-2", "SNOL" },
	{ 16, "Snow Melt", "kg m^-2", "SNOM" },
	{ 17, "Snow Age", "day", "SNOAG" },
	{ 18, "Absolute Humidity", "kg m^-3", "ABSH" },
	{ 19, "Precipitation Type", "1=RA,2=TS,3=FZRA,4=IC,5=SN", "PTYPE" },
	{ 20, "Integrated Liquid Water", "kg m^-2", "ILIQW" },
	{ 21, "Condensate", "kg kg^-1", "TCOND" },
	{ 22, "Cloud Mixing Ratio", "kg kg^-1", "CLWMR" },
	{ 23, "Ice Water Mixing Ratio", "kg kg^-1", "ICMR" },
	{ 24, "Rain Mixing Ratio", "kg kg^-1", "RWMR" },
	{ 25, "Snow Mixing Ratio", "kg kg^-1", "SNMR" },
	{ 26, "Horizontal Moisture Convergence", "kg kg^-1 s^-1", "MCONV" },
	{ 27, "Maximum Relative Humidity", "%", "MAXRH" },
	{ 28, "Maximum Absolute Humidity", "kg m^-3", "MAXAH" },
	{ 29, "Total Snowfall", "m", "ASNOW" },
	{ 30, "Precipitable Water Category", 0, "PWCAT" },
	{ 31, "Hail", "m", "HAIL" },
	{ 32, "Graupel", "kg kg^-1", "GRLE" },
	{ 33, "Categorical Rain", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CRAIN" },
	{ 34, "Categorical Freezing Rain", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CFRZR" },
	{ 35, "Categorical Ice Pellets", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CICEP" },
	{ 36, "Categorical Snow", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CSNOW" },
	{ 37, "Convective Precipitation Rate", "kg m^-2 s^-1", "CPRAT" },
	{ 38, "Horizontal Moisture Divergence", "kg kg^-1 s^-1", "MDIV" },
	{ 39, "Percent frozen precipitation", "%", "CPOFP" },
	{ 40, "Potential Evaporation", "kg m^-2", "PEVAP" },
	{ 41, "Potential Evaporation Rate", "W m^-2", "PEVPR" },
	{ 42, "Snow Cover", "%", "SNOWC" },
	{ 43, "Rain Fraction of Total Cloud", "Proportion", "FRAIN" },
	{ 44, "Rime Factor", 0, "RIME" },
	{ 45, "Total Column Integrated Rain", "kg m^-2", "TCOLR" },
	{ 46, "Total Column Integrated Snow", "kg m^-2", "TCOLS" },
	{ 47, "Large Scale Water Precipitation (Non-Convective)", "kg m^-2", "LSWP" },
	{ 48, "Convective Water Precipitation", "kg m^-2", "CWP" },
	{ 49, "Total Water Precipitation", "kg m^-2", "TWATP" },
	{ 50, "Total Snow Precipitation", "kg m^-2", "TSNOWP" },
	{ 51, "Total Column Water", "kg m", "TCWAT" },
	{ 52, "Total Precipitation Rate", "kg m^-2 s^-1", "TPRATE" },
	{ 53, "Total Snowfall Rate Water Equivalent", "kg m^-2 s^-1", "TSRWE" },
	{ 54, "Large Scale Precipitation Rate", "kg m^-2 s^-1", "LSPRATE" },
	{ 55, "Convective Snowfall Rate Water Equivalent", "kg m^-2 s^-1", "CSRWE" },
	{ 56, "Large Scale Snowfall Rate Water Equivalent", "kg m^-2 s^-1", "LSSRWE" },
	{ 57, "Total Snowfall Rate", "m s^-1", "TSRATE" },
	{ 58, "Convective Snowfall Rate", "m s^-1", "CSRATE" },
	{ 59, "Large Scale Snowfall Rate", "m s^-1", "LSSRATE" },
	{ 60, "Snow Depth Water Equivalent", "kg m^-2", "SDWE" },
	{ 61, "Snow Density", "kg m^-3", "SDEN" },
	{ 62, "Snow Evaporation", "kg m^-2", "SEVAP" },
	{ 64, "Total Column Integrated Water Vapour", "kg m^-2", "TCIWV" },
	{ 65, "Rain Precipitation Rate", "kg m^-2 s^-1", "RPRATE" },
	{ 66, "Snow Precipitation Rate", "kg m^-2 s^-1", "SPRATE" },
	{ 67, "Freezing Rain Precipitation Rate", "kg m^-2 s^-1", "FPRATE" },
	{ 68, "Ice Pellets Precipitation Rate", "kg m^-2 s^-1", "IPRATE" },
	{ 69, "Total Column Integrate Cloud Water", "kg m^-2", "TCOLW" },
	{ 70, "Total Column Integrate Cloud Ice", "kg m^-2", "TCOLI" },
	{ 71, "Hail Mixing Ratio", "kg kg^-1", "HAILMXR" },
	{ 72, "Total Column Integrate Hail", "kg m^-2", "TCOLH" },
	{ 73, "Hail Prepitation Rate", "kg m^-2 s^-1", "HAILPR" },
	{ 74, "Total Column Integrate Graupel", "kg m^-2", "TCOLG" },
	{ 75, "Graupel (Snow Pellets) Prepitation Rate", "kg m^-2 s^-1", "GPRATE" },
	{ 76, "Convective Rain Rate", "kg m^-2 s^-1", "CRRATE" },
	{ 77, "Large Scale Rain Rate", "kg m^-2 s^-1", "LSRRATE" },
	{ 78, "Total Column Integrate Water (All", "kg m^-2", "TCOLWA" },
	{ 79, "Evaporation Rate", "kg m^-2 s^-1", "EVARATE" },
	{ 80, "Total Condensate", "kg kg^-1", "TOTCON" },
	{ 81, "Total Column-Integrate Condensate", "kg m^-2", "TCICON" },
	{ 82, "Cloud Ice Mixing Ratio", "kg kg", "CIMIXR" },
	{ 83, "Specific Cloud Liquid Water Content", "kg kg^-1", "SCLLWC" },
	{ 84, "Specific Cloud Ice Water Content", "kg kg^-1", "SCLIWC" },
	{ 85, "Specific Rain Water Content", "kg kg^-1", "SRAINW" },
	{ 86, "Specific Snow Water Content", "kg kg^-1", "SSNOWW" },
	{ 90, "Total Kinematic Moisture Flux", "kg kg^-1 m s^-1", "TKMFLX" },
	{ 91, "U-component (zonal) Kinematic Moisture Flux", "kg kg^-1 m s^-1", "UKMFLX" },
	{ 92, "V-component (meridional) Kinematic Moisture Flux", "kg kg^-1 m s^-1", "VKMFLX" },
	{ 192, "Categorical Rain", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CRAIN" },
	{ 193, "Categorical Freezing Rain", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CFRZR" },
	{ 194, "Categorical Ice Pellets", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CICEP" },
	{ 195, "Categorical Snow", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CSNOW" },
	{ 196, "Convective Precipitation Rate", "kg m^-2 s^-1", "CPRAT" },
	{ 197, "Horizontal Moisture Divergence", "kg kg^-1", "MDIV" },
	{ 198, "Minimum Relative Humidity", "%", "MINRH" },
	{ 199, "Potential Evaporation", "kg m^-2", "PEVAP" },
	{ 200, "Potential Evaporation Rate", "W m^-2", "PEVPR" },
	{ 201, "Snow Cover", "%", "SNOWC" },
	{ 202, "Rain Fraction of Total Liquid Water", 0, "FRAIN" },
	{ 203, "Rime Factor", 0, "RIME" },
	{ 204, "Total Column Integrated Rain", "kg m^-2", "TCOLR" },
	{ 205, "Total Column Integrated Snow", "kg m^-2", "TCOLS" },
	{ 206, "Total Icing Potential Diagnostic", 0, "TIPD" },
	{ 207, "Number concentration for ice particles", 0, "NCIP" },
	{ 208, "Snow temperature", "K", "SNOT" },
	{ 209, "Total column-integrated supercooled liquid water", "kg m^-2", "TCLSW" },
	{ 210, "Total column-integrated melting ice", "kg m^-2", "TCOLM" },
	{ 211, "Evaporation - Precipitation", "cm day^-1", "EMNP" },
	{ 212, "Sublimation (evaporation from snow)", "W m^-2", "SBSNO" },
	{ 213, "Deep  Convective Moistening Rate", "kg kg^-1 s^-1", "CNVMR" },
	{ 214, "Shallow Convective Moistening Rate", "kg kg^-1 s^-1", "SHAMR" },
	{ 215, "Vertical Diffusion Moistening Rate", "kg kg^-1 s^-1", "VDFMR" },
	{ 216, "Condensation Pressure of Parcali", "Pa", "CONDP" },
	{ 217, "Large scale moistening rate", "kg kg^-1 s^-1", "LRGMR" },
	{ 218, "Specific humidity at top of viscous sublayer", "kg kg^-1", "QZ0" },
	{ 219, "Maximum specific humidity at 2m", "kg kg^-1", "QMAX" },
	{ 220, "Minimum specific humidity at 2m", "kg kg^-1", "QMIN" },
	{ 221, "Liquid precipitation (rainfall)", "kg m^-2", "ARAIN" },
	{ 222, "Snow temperature, depth-avg", "K", "SNOWT" },
	{ 223, "Total precipitation (nearest grid point)", "kg m^-2", "APCPN" },
	{ 224, "Convective precipitation (nearest grid point)", "kg m^-2", "ACPCPN" },
	{ 225, "Freezing Rain", "kg m^-2", "FRZR" },
	{ 226, "Predominant Weather", 0, "PWTHER" },
	{ 227, "Frozen Rain", "kg m^-2", "FROZR" },
	{ 241, "Total Snow", "kg m^-2", "TSNOW" },
	{ 242, "Relative Humidity with Respect to Precipitable Water", "%", "RHPW" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-2.shtml

const struct parameter_id parameter_0_2[] = {
	{ 0, "Wind Direction (from which blowing)", "deg true", "WDIR" },
	{ 1, "Wind Speed", "m s^-1", "WIND" },
	{ 2, "U-Component of Wind", "m s^-1", "UGRD" },
	{ 3, "V-Component of Wind", "m s^-1", "VGRD" },
	{ 4, "Stream Function", "m^2 s^-1", "STRM" },
	{ 5, "Velocity Potential", "m^2 s^-1", "VPOT" },
	{ 6, "Montgomery Stream Function", "m^2 s^-1", "MNTSF" },
	{ 7, "Sigma Coordinate Vertical", "s^-1", "SGCVV" },
	{ 8, "Vertical Velocity (Pressure)", "Pa s^-1", "VVEL" },
	{ 9, "Vertical Velocity (Geometric)", "m s^-1", "DZDT" },
	{ 10, "Absolute Vorticity", "s^-1", "ABSV" },
	{ 11, "Absolute Divergence", "s^-1", "ABSD" },
	{ 12, "Relative Vorticity", "s^-1", "RELV" },
	{ 13, "Relative Divergence", "s^-1", "RELD" },
	{ 14, "Potential Vorticity", "K m^2 kg^-1 s^-1", "PVORT" },
	{ 15, "Vertical U-Component Shear", "s^-1", "VUCSH" },
	{ 16, "Vertical V-Component Shear", "s^-1", "VVCSH" },
	{ 17, "Momentum Flux, U-Component", "N m^-2", "UFLX" },
	{ 18, "Momentum Flux, V-Component", "N m^-2", "VFLX" },
	{ 19, "Wind Mixing Energy", "J", "WMIXE" },
	{ 20, "Boundary Layer Dissipation", "W m^-2", "BLYDP" },
	{ 21, "Maximum Wind Speed", "m s^-1", "MAXGUST" },
	{ 22, "Wind Speed (Gust)", "m s^-1", "GUST" },
	{ 23, "U-Component of Wind (Gust)", "m s^-1", "UGUST" },
	{ 24, "V-Component of Wind (Gust)", "m s^-1", "VGUST" },
	{ 25, "Vertical Speed Shear", "s^-1", "VWSH" },
	{ 26, "Horizontal Momentum Flux", "N m^-2", "MFLX" },
	{ 27, "U-Component Storm Motion", "m s^-1", "USTM" },
	{ 28, "V-Component Storm Motion", "m s^-1", "VSTM" },
	{ 29, "Drag Coefficient", 0, "CD" },
	{ 30, "Frictional Velocity", "m s^-1", "FRICV" },
	{ 31, "Turbulent Diffusion Coefficient for Momentum", "m^2 s^-1", "TDCMOM" },
	{ 32, "Eta Coordinate Vertical Velocity", "s^-1", "ETACVV" },
	{ 33, "Wind Fetch", "m", "WINDF" },
	{ 192, "Vertical Speed Shear", "s^-1", "VWSH" },
	{ 193, "Horizontal Momentum Flux", "N m^-2", "MFLX" },
	{ 194, "U-Component Storm Motion", "m s^-1", "USTM" },
	{ 195, "V-Component Storm Motion", "m s^-1", "VSTM" },
	{ 196, "Drag Coefficient", 0, "CD" },
	{ 197, "Frictional Velocity", "m s^-1", "FRICV" },
	{ 198, "Latitude of U Wind Component of", "deg", "LAUV" },
	{ 199, "Longitude of U Wind Component of", "deg", "LOUV" },
	{ 200, "Latitude of V Wind Component of Velocity", "deg", "LAVV" },
	{ 201, "Longitude of V Wind Component of", "deg", "LOVV" },
	{ 202, "Latitude of Presure Point", "deg", "LAPP" },
	{ 203, "Longitude of Presure Point", "deg", "LOPP" },
	{ 204, "Vertical Eddy Diffusivity Heat exchange", "m^2 s^-1", "VEDH" },
	{ 205, "Covariance between Meridional and Zonal", "m^2 s^-2", "COVMZ" },
	{ 206, "Covariance between Temperature and Zonal", "K m s^-1", "COVTZ" },
	{ 207, "Covariance between Temperature and Meridional", "K m s^-1", "COVTM" },
	{ 208, "Vertical Diffusion Zonal Acceleration", "m s^-2", "VDFUA" },
	{ 209, "Vertical Diffusion Meridional Acceleration", "m s^-2", "VDFVA" },
	{ 210, "Gravity wave drag zonal acceleration", "m s^-2", "GWDU" },
	{ 211, "Gravity wave drag meridional acceleration", "m s^-2", "GWDV" },
	{ 212, "Convective zonal momentum mixing acceleration", "m s^-2", "CNVU" },
	{ 213, "Convective meridional momentum mixing acceleration", "m s^-2", "CNVV" },
	{ 214, "Tendency of vertical velocity", "m s^-2", "WTEND" },
	{ 215, "Omega (Dp/Dt) divide by density", "K", "OMGALF" },
	{ 216, "Convective Gravity wave drag zonal acceleration", "m s^-2", "CNGWDU" },
	{ 217, "Convective Gravity wave drag meridional acceleration", "m s^-2", "CNGWDV" },
	{ 218, "Velocity Point Model Surface", 0, "LMV" },
	{ 219, "Potential Vorticity (Mass-Weighted)", "s^-1 m^-1", "PVMWW" },
	{ 220, "Hourly Maximum of Upward Vertical Velocity", "m s^-1", "MAXUVV" },
	{ 221, "Hourly Maximum of Downward Vertical Velocity", "m s^-1", "MAXDVV" },
	{ 222, "U Component of Hourly Maximum 10m Wind Speed", "m s^-1", "MAXUW" },
	{ 223, "V Component of Hourly Maximum 10m Wind Speed", "m s^-1", "MAXVW" },
	{ 224, "Ventilation Rate", "m^2 s^-1", "VRATE" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-3.shtml

const struct parameter_id parameter_0_3[] = {
	{ 0, "Pressure", "Pa", "PRES" },
	{ 1, "Pressure Reduced to MSL", "Pa", "PRMSL" },
	{ 2, "Pressure Tendency", "Pa s^-1", "PTEND" },
	{ 3, "ICAO Standard Atmosphere", "m", "ICAHT" },
	{ 4, "Geopotential", "m^2 s^-1", "GP" },
	{ 5, "Geopotential Height", "gpm", "HGT" },
	{ 6, "Geometric Height", "m", "DIST" },
	{ 7, "Standard Deviation of Height", "m", "HSTDV" },
	{ 8, "Pressure Anomaly", "Pa", "PRESA" },
	{ 9, "Geopotential Height Anomaly", "gpm", "GPA" },
	{ 10, "Density", "kg m^-3", "DEN" },
	{ 11, "Altimeter Setting", "Pa", "ALTS" },
	{ 12, "Thickness", "m", "THICK" },
	{ 13, "Pressure Altitude", "m", "PRESALT" },
	{ 14, "Density Altitude", "m", "DENALT" },
	{ 15, "5-Wave Geopotential Height", "gpm", "5WAVH" },
	{ 16, "Zonal Flux of Gravity Wave Stress", "N m^-2", "U-GWD" },
	{ 17, "Meridional Flux of Gravity Wave", "N m^-2", "V-GWD" },
	{ 18, "Planetary Boundary Layer Height", "m", "HPBL" },
	{ 19, "5-Wave Geopotential Height", "gpm", "5WAVA" },
	{ 20, "Standard Deviation of Sub-Grid Scale Orography", "m", "SDSGSO" },
	{ 21, "Angle of Sub-Grid Scale Orography", "rad", "AOSGSO" },
	{ 22, "Slope of Sub-Grid Scale Orography", 0, "SSGSO" },
	{ 23, "Gravity Wave Dissipation", "W m^-2", "GWD" },
	{ 24, "Anisotropy of Sub-Grid Scale Orography", 0, "ASGSO" },
	{ 25, "Natural Logarithm of Pressure in Pa", 0, "NLPRES" },
	{ 192, "MSLP (Eta model reduction)", "Pa", "MSLET" },
	{ 193, "5-Wave Geopotential Height", "gpm", "5WAVH" },
	{ 194, "Zonal Flux of Gravity Wave Stress", "N m^-2", "U-GWD" },
	{ 195, "Meridional Flux of Gravity Wave", "N m^-2", "V-GWD" },
	{ 196, "Planetary Boundary Layer Height", "m", "HPBL" },
	{ 197, "5-Wave Geopotential Height", "gpm", "5WAVA" },
	{ 198, "MSLP (MAPS System Reduction)", "Pa", "MSLMA" },
	{ 199, "3-hr pressure tendency (Std. Atmos. Reduction)", "Pa s^-1", "TSLSA" },
	{ 200, "Pressure of level from which parcel was lifted", "Pa", "PLPL" },
	{ 201, "X-gradient of Log Pressure", "m^-1", "LPSX" },
	{ 202, "Y-gradient of Log Pressure", "m^-1", "LPSY" },
	{ 203, "X-gradient of Height", "m^-1", "HGTX" },
	{ 204, "Y-gradient of Height", "m^-1", "HGTY" },
	{ 205, "Layer Thickness", "m", "LAYTH" },
	{ 206, "Natural Log of Surface Pressure", "ln (kPa)", "NLGSP" },
	{ 207, "Convective updraft mass flux", "kg m^2 s^-1", "CNVUMF" },
	{ 208, "Convective downdraft mass flux", "kg m^2 s^-1", "CNVDMF" },
	{ 209, "Convective detrainment mass flux", "kg m^2 s^-1", "CNVDEMF" },
	{ 210, "Mass Point Model Surface", 0, "LMH" },
	{ 211, "Geopotential Height (nearest grid point)", "gpm", "HGTN" },
	{ 212, "Pressure (nearest grid point)", "Pa", "PRESN" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-4.shtml

const struct parameter_id parameter_0_4[] = {
	{ 0, "Net Short-Wave Radiation Flux (Surface)", "W m^-2", "NSWRS" },
	{ 1, "Net Short-Wave Radiation Flux", "W m^-2", "NSWRT" },
	{ 2, "Short-Wave Radiation Flux", "W m^-2", "SWAVR" },
	{ 3, "Global Radiation Flux", "W m^-2", "GRAD" },
	{ 4, "Brightness Temperature", "K", "BRTMP" },
	{ 5, "Radiance (with respect to wave number)", "W m^-1 sr^-1", "LWRAD" },
	{ 6, "Radiance (with respect to wavelength)", "W m^-3 sr^-1", "SWRAD" },
	{ 7, "Downward Short-Wave Radiation Flux", "W m^-2", "DSWRF" },
	{ 8, "Upward Short-Wave Radiation Flux", "W m^-2", "USWRF" },
	{ 9, "Net Short Wave Radiation Flux", "W m^-2", "NSWRF" },
	{ 10, "Photosynthetically Active Radiation", "W m^-2", "PHOTAR" },
	{ 11, "Net Short-Wave Radiation Flux, Clear Sky", "W m^-2", "NSWRFCS" },
	{ 12, "Downward UV Radiation", "W m^-2", "DWUVR" },
	{ 50, "UV Index (Under Clear Sky)", 0, "UVIUCS" },
	{ 51, "UV Index", "W m^-2", "UVI" },
	{ 192, "Downward Short-Wave Radiation Flux", "W m^-2", "DSWRF" },
	{ 193, "Upward Short-Wave Radiation Flux", "W m^-2", "USWRF" },
	{ 194, "UV-B Downward Solar Flux", "W m^-2", "DUVB" },
	{ 195, "Clear Sky UV-B Downward Solar Flux", "W m^-2", "CDUVB" },
	{ 196, "Clear Sky Downward Solar Flux", "W m^-2", "CSDSF" },
	{ 197, "Solar Radiative Heating Rate", "K s^-1", "SWHR" },
	{ 198, "Clear Sky Upward Solar Flux", "W m^-2", "CSUSF" },
	{ 199, "Cloud Forcing Net Solar Flux", "W m^-2", "CFNSF" },
	{ 200, "Visible Beam Downward Solar Flux", "W m^-2", "VBDSF" },
	{ 201, "Visible Diffuse Downward Solar Flux", "W m^-2", "VDDSF" },
	{ 202, "Near IR Beam Downward Solar Flux", "W m^-2", "NBDSF" },
	{ 203, "Near IR Diffuse Downward Solar Flux", "W m^-2", "NDDSF" },
	{ 204, "Downward Total Radiation Flux", "W m^-2", "DTRF" },
	{ 205, "Upward Total Radiation Flux", "W m^-2", "UTRF" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-5.shtml

const struct parameter_id parameter_0_5[] = {
	{ 0, "Net Long-Wave Radiation Flux (Surface)", "W m^-2", "NLWRS" },
	{ 1, "Net Long-Wave Radiation Flux (Top of Atmosphere)", "W m^-2", "NLWRT" },
	{ 2, "Long-Wave Radiation Flux", "W m^-2", "LWAVR" },
	{ 3, "Downward Long-Wave Rad. Flux", "W m^-2", "DLWRF" },
	{ 4, "Upward Long-Wave Rad. Flux", "W m^-2", "ULWRF" },
	{ 5, "Net Long-Wave Radiation Flux", "W m^-2", "NLWRF" },
	{ 6, "Net Long-Wave Radiation Flux, Clear Sky", "W m^-2", "NLWRCS" },
	{ 192, "Downward Long-Wave Rad. Flux", "W m^-2", "DLWRF" },
	{ 193, "Upward Long-Wave Rad. Flux", "W m^-2", "ULWRF" },
	{ 194, "Long-Wave Radiative Heating Rate", "K s^-1", "LWHR" },
	{ 195, "Clear Sky Upward Long Wave Flux", "W m^-2", "CSULF" },
	{ 196, "Clear Sky Downward Long Wave Flux", "W m^-2", "CSDLF" },
	{ 197, "Cloud Forcing Net Long Wave Flux", "W m^-2", "CFNLF" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-6.shtml

const struct parameter_id parameter_0_6[] = {
	{ 0, "Cloud Ice", "kg m^-2", "CICE" },
	{ 1, "Total Cloud Cover", "%", "TCDC" },
	{ 2, "Convective Cloud Cover", "%", "CDCON" },
	{ 3, "Low Cloud Cover", "%", "LCDC" },
	{ 4, "Medium Cloud Cover", "%", "MCDC" },
	{ 5, "High Cloud Cover", "%", "HCDC" },
	{ 6, "Cloud Water", "kg m^-2", "CWAT" },
	{ 7, "Cloud Amount", "%", "CDCA" },
	{ 8, "Cloud Type", "0=SKC,1=CB,2=ST,3=SC,4=CU,5=AS,6=NS,7=AC,8=CS,9=CC,10=CI,11=CB+Fog,12=ST+Fog,13=SC+Fog,14=CU+Fog,15=AS+Fog,16=NS+Fog,17=AC+Fog,18=CS+Fog,19=CC+Fog,20=CI+Fog,255=Missing", "CDCT" },
	{ 9, "Thunderstorm Maximum Tops", "m", "TMAXT" },
	{ 10, "Thunderstorm Coverage", "0=None,1=Isolated(1-2%),2=Few(3-5%),3=Scattered(16-45%),4=Numerous(>45%),255=Missing", "THUNC" },
	{ 11, "Cloud Base", "m", "CDCB" },
	{ 12, "Cloud Top", "m", "CDCT" },
	{ 13, "Ceiling", "m", "CEIL" },
	{ 14, "Non-Convective Cloud Cover", "%", "CDLYR" },
	{ 15, "Cloud Work Function", "J kg^-1", "CWORK" },
	{ 16, "Convective Cloud Efficiency", "Proportion", "CUEFI" },
	{ 17, "Total Condensate", "kg kg^-1", "TCOND" },
	{ 18, "Total Column-Integrated Cloud", "kg m^-2", "TCOLW" },
	{ 19, "Total Column-Integrated Cloud Ice", "kg m^-2", "TCOLI" },
	{ 20, "Total Column-Integrated", "kg m^-2", "TCOLC" },
	{ 21, "Ice fraction of total condensate", "Proportion", "FICE" },
	{ 22, "Cloud Cover", "%", "CDCC" },
	{ 23, "Cloud Ice Mixing Ratio", "kg kg^-1", "CDCIMR" },
	{ 24, "Sunshine", 0, "SUNS" },
	{ 25, "Horizontal Extent of Cumulonimbus (CB)", "%", "CBHE" },
	{ 26, "Height of Convective Cloud Base", "m", "HCONCB" },
	{ 27, "Height of Convective Cloud Top", "m", "HCONCT" },
	{ 28, "Number Concentration of Cloud Droplets", "kg^-1", "NCONCD" },
	{ 29, "Number Concentration of Cloud Ice", "kg^-1", "NCCICE" },
	{ 30, "Number Density of Cloud Droplets", "m^-3", "NDENCD" },
	{ 31, "Number Density of Cloud Ice", "m^-3", "NDCICE" },
	{ 32, "Fraction of Cloud Cover", 0, "FRACCC" },
	{ 33, "Sunshine Duration", "s", "SUNSD" },
	{ 192, "Non-Convective Cloud Cover", "%", "CDLYR" },
	{ 193, "Cloud Work Function", "J kg^-1", "CWORK" },
	{ 194, "Convective Cloud Efficiency", 0, "CUEFI" },
	{ 195, "Total Condensate", "kg kg^-1", "TCOND" },
	{ 196, "Total Column-Integrated Cloud", "kg m^-2", "TCOLW" },
	{ 197, "Total Column-Integrated Cloud Ice", "kg m^-2", "TCOLI" },
	{ 198, "Total Column-Integrated Condensate", "kg m^-2", "TCOLC" },
	{ 199, "Ice fraction of total condensate", 0, "FICE" },
	{ 200, "Convective Cloud Mass Flux", "Pa s^-1", "MFLUX" },
	{ 201, "Sunshine Duration", "s", "SUNSD" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-7.shtml

const struct parameter_id parameter_0_7[] = {
	{ 0, "Parcel Lifted Index (to 500 hPa)", "K", "PLI" },
	{ 1, "Best Lifted Index (to 500 hPa)", "K", "BLI" },
	{ 2, "K Index", "K", "KX" },
	{ 3, "KO Index", "K", "KOX" },
	{ 4, "Total Totals Index", "K", "TOTALX" },
	{ 5, "Sweat Index", 0, "SX" },
	{ 6, "Convective Available Potential", "J kg^-1", "CAPE" },
	{ 7, "Convective Inhibition", "J kg^-1", "CIN" },
	{ 8, "Storm Relative Helicity", "m^2 s^-2", "HLCY" },
	{ 9, "Energy Helicity Index", 0, "EHLX" },
	{ 10, "Surface Lifted Index", "K", "LFT X" },
	{ 11, "Best (4 layer) Lifted Index", "K", "4LFTX" },
	{ 12, "Richardson Number", 0, "RI" },
	{ 13, "Showalter Index", "K", "SHWINX" },
	{ 15, "Updraft Helicity", "m^2 s^-2", "UPHL" },
	{ 192, "Surface Lifted Index", "K", "LFT X" },
	{ 193, "Best (4 layer) Lifted Index", "K", "4LFTX" },
	{ 194, "Richardson Number", 0, "RI" },
	{ 195, "Convective Weather Detection Index", 0, "CWDI" },
	{ 196, "Ultra Violet Index", "W m^-2", "UVI" },
	{ 197, "Updraft Helicity", "m^2 s^-2", "UPHL" },
	{ 198, "Leaf Area Index", 0, "LAI" },
	{ 199, "Hourly Maximum of Updraft Helicity over Layer 2km to 5 km AGL", "m^2 s^-2", "MXUPHL" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-13.shtml

const struct parameter_id parameter_0_13[] = {
	{ 0, "Aerosol Type", "0=not present,1=present,255=missing", "AEROT" },
	{ 192, "Particulate matter (coarse)", "ug m^-3", "PMTC" },
	{ 193, "Particulate matter (fine)", "ug m^-3", "PMTF" },
	{ 194, "Particulate matter (fine)", "log10 (ug m^-3)", "LPMTF" },
	{ 195, "Integrated column particulate matter (fine)", "log10 (ug m^-3)", "LIPMF" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-14.shtml

const struct parameter_id parameter_0_14[] = {
	{ 0, "Total Ozone", "DU", "TOZNE" },
	{ 1, "Ozone Mixing Ratio", "kg kg^-1", "O3MR" },
	{ 2, "Total Column Integrated Ozone", "DU", "TCIOZ" },
	{ 192, "Ozone Mixing Ratio", "kg kg^-1", "O3MR" },
	{ 193, "Ozone Concentration", "ppb", "OZCON" },
	{ 194, "Categorical Ozone Concentration", 0, "OZCAT" },
	{ 195, "Ozone Vertical Diffusion", "kg kg^-1 s^-1", "VDFOZ" },
	{ 196, "Ozone Production", "kg kg^-1 s^-1", "POZ" },
	{ 197, "Ozone Tendency", "kg kg^-1 s^-1", "TOZ" },
	{ 198, "Ozone Production from Temperature Term", "kg kg^-1 s^-1", "POZT" },
	{ 199, "Ozone Production from Column Ozone Term", "kg kg^-1 s^-1", "POZO" },
	{ 200, "Ozone Daily Max from 1-hour Average", "ppbV", "OZMAX1" },
	{ 201, "Ozone Daily Max from 8-hour Average", "ppbV", "OZMAX8" },
	{ 202, "PM 2.5 Daily Max from 1-hour Average", "ug m^-3", "PDMAX1" },
	{ 203, "PM 2.5 Daily Max from 24-hour Average", "ug m^-3", "PDMAX24" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-15.shtml

const struct parameter_id parameter_0_15[] = {
	{ 0, "Base Spectrum Width", "m s^-1", "BSWID" },
	{ 1, "Base Reflectivity", "dB", "BREF" },
	{ 2, "Base Radial Velocity", "m s^-1", "BRVEL" },
	{ 3, "Vertically Integrated Liquid", "kg m^-2", "VIL" },
	{ 4, "Layer Maximum Base Reflectivity", "dB", "LMAXBR" },
	{ 5, "Precipitation", "kg m^-2", "PREC" },
	{ 6, "Radar Spectra (1)", 0, "RDSP1" },
	{ 7, "Radar Spectra (2)", 0, "RDSP2" },
	{ 8, "Radar Spectra (3)", 0, "RDSP3" },
	{ 9, "Reflectivity of Cloud Droplets", "dB", "RFCD" },
	{ 10, "Reflectivity of Cloud Ice", "dB", "RFCI" },
	{ 11, "Reflectivity of Snow", "dB", "RFSNOW" },
	{ 12, "Reflectivity of Rain", "dB", "RFRAIN" },
	{ 13, "Reflectivity of Graupel", "dB", "RFGRPL" },
	{ 14, "Reflectivity of Hail", "dB", "RFHAIL" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-16.shtml

const struct parameter_id parameter_0_16[] = {
	{ 0, "Equivalent radar reflectivity factor for rain", "m m^6 m^-3", "REFZR" },
	{ 1, "Equivalent radar reflectivity factor for snow", "m m^6 m^-3", "REFZI" },
	{ 2, "Equivalent radar reflectivity factor for parameterized convection", "m m^6 m^-3", "REFZC" },
	{ 3, "Echo Top", "m", "RETOP" },
	{ 4, "Reflectivity", "dB", "REFD" },
	{ 5, "Composite reflectivity", "dB", "REFC" },
	{ 192, "Equivalent radar reflectivity factor for rain", "m m^6 m^-3", "REFZR" },
	{ 193, "Equivalent radar reflectivity factor for snow", "m m^6 m^-3", "REFZI" },
	{ 194, "Equivalent radar reflectivity factor for parameterized convection", "m m^6 m^-3", "REFZC" },
	{ 195, "Reflectivity", "dB", "REFD" },
	{ 196, "Composite reflectivity", "dB", "REFC" },
	{ 197, "Echo Top    (See Note 1)", "m", "RETOP" },
	{ 198, "Hourly Maximum of Simulated Reflectivity at 1 km AGL", "dB", "MAXREF" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-17.shtml

const struct parameter_id parameter_0_17[] = {
	{ 192, "Lightning", 0, "LTNG" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-18.shtml

const struct parameter_id parameter_0_18[] = {
	{ 0, "Air Concentration of Caesium 137", "Bq m^-3", "ACCES" },
	{ 1, "Air Concentration of Iodine 131", "Bq m^-3", "ACIOD" },
	{ 2, "Air Concentration of Radioactive", "Bq m^-3", "ACRADP" },
	{ 3, "Ground Deposition of Caesium 137", "Bq m^-2", "GDCES" },
	{ 4, "Ground Deposition of Iodine 131", "Bq m^-2", "GDIOD" },
	{ 5, "Ground Deposition of Radioactive", "Bq m^-2", "GDRADP" },
	{ 6, "Time Integrated Air", "Bq s m^-3", "TIACCP" },
	{ 7, "Time Integrated Air", "Bq s m^-3", "TIACIP" },
	{ 8, "Time Integrated Air", "Bq s m^-3", "TIACRP" },
	{ 10, "Air Concentration", "Bq m^-3", "AIRCON" },
	{ 11, "Wet Deposition", "Bq m^-3", "WETDEP" },
	{ 12, "Dry Deposition", "Bq m^-3", "DRYDEP" },
	{ 13, "Total Deposition (Wet + Dry)", "Bq m^-3", "TOTLWD" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-19.shtml

const struct parameter_id parameter_0_19[] = {
	{ 0, "Visibility", "m", "VIS" },
	{ 1, "Albedo", "%", "ALBDO" },
	{ 2, "Thunderstorm Probability", "%", "TSTM" },
	{ 3, "Mixed Layer Depth", "m", "MIXHT" },
	{ 4, "Volcanic Ash", "0=not present,1=present,255=missing", "VOLASH" },
	{ 5, "Icing Top", "m", "ICIT" },
	{ 6, "Icing Base", "m", "ICIB" },
	{ 7, "Icing", "0=None,1=Light,2=Moderate,3=Severe,4=Trace,5=Heavy,255=Missing", "ICI" },
	{ 8, "Turbulence Top", "m", "TURBT" },
	{ 9, "Turbulence Base", "m", "TURBB" },
	{ 10, "Turbulence", "0=None,1=Light,2=Moderate,3=Severe,4=Extreme,255=Missing", "TURB" },
	{ 11, "Turbulent Kinetic Energy", "J kg^-1", "TKE" },
	{ 12, "Planetary Boundary Layer Regime", "1=Stable,2=Mechanically-Driven Turbulence,3=Force Convection,4=Free Convection,255=Missing", "PBLREG" },
	{ 13, "Contrail Intensity", "0=not present,1=present,255=missing", "CONTI" },
	{ 14, "Contrail Engine Type", "0=Low Bypass,1=High Bypass,2=Non Bypass,255=Missing", "CONTET" },
	{ 15, "Contrail Top", "m", "CONTT" },
	{ 16, "Contrail Base", "m", "CONTB" },
	{ 17, "Maximum Snow Albedo", "%", "MXSALB" },
	{ 18, "Snow-Free Albedo", "%", "SNFALB" },
	{ 19, "Snow Albedo", "%", "SALBD" },
	{ 20, "Icing", "%", "ICIP" },
	{ 21, "In-Cloud Turbulence", "%", "CTP" },
	{ 22, "Clear Air Turbulence (CAT)", "%", "CAT" },
	{ 23, "Supercooled Large Droplet (SLD)", "%", "SLDP" },
	{ 24, "Convective Turbulent Kinetic Energy", "J kg^-1", "CONTKE" },
	{ 25, "Weather Interpretation ww (WMO)", 0, "WIWW" },
	{ 26, "Convective Outlook", "0=No Risk,2=General Thunderstorm,4=Slight Risk,6=Moderate Risk,8=High Risk,11=Dry Thunderstorm (Dry Lightning),14=Critical Risk,18=Extremely Critical Risk,255=Missing", "CONVO" },
	{ 192, "Maximum Snow Albedo", 0, "MXSALB" },
	{ 193, "Snow-Free Albedo", "%", "SNFALB" },
	{ 194, "Slight risk convective outlook", 0, "SRCONO" },
	{ 195, "Moderate risk convective outlook", 0, "MRCONO" },
	{ 196, "High risk convective outlook ", 0, "HRCONO" },
	{ 197, "Tornado probability", "%", "TORPROB" },
	{ 198, "Hail probability", "%", "HAILPROB" },
	{ 199, "Wind probability", "%", "WINDPROB" },
	{ 200, "Significant Tornado probability", "%", "STORPROB" },
	{ 201, "Significant Hail probability", "%", "SHAILPRO" },
	{ 202, "Significant Wind probability", "%", "SWINDPRO" },
	{ 203, "Categorical Thunderstorm", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "TSTMC" },
	{ 204, "Number of mixed layers next to surface", 0, "MIXLY" },
	{ 205, "Flight Category", 0, "FLGHT" },
	{ 206, "Confidence - Ceiling", 0, "CICEL" },
	{ 207, "Confidence - Visibility", 0, "CIVIS" },
	{ 208, "Confidence - Flight Category", 0, "CIFLT" },
	{ 209, "Low-Level aviation interest", 0, "LAVNI" },
	{ 210, "High-Level aviation interest", 0, "HAVNI" },
	{ 211, "Visible, Black Sky Albedo", "%", "SBSALB" },
	{ 212, "Visible, White Sky Albedo", "%", "SWSALB" },
	{ 213, "Near IR, Black Sky Albedo", "%", "NBSALB" },
	{ 214, "Near IR, White Sky Albedo", "%", "NWSALB" },
	{ 215, "Total Probability of Severe Thunderstorms (Days 2,3)", "%", "PRSVR" },
	{ 216, "Total Probability of Extreme Severe Thunderstorms (Days 2,3)", "%", "PRSIGSVR" },
	{ 217, "Supercooled Large Droplet (SLD) Icing", "0=None,1=Light,2=Moderate,3=Severe,4=Trace,5=Heavy,255=Missing", "SIPD" },
	{ 218, "Radiative emissivity", 0, "EPSR" },
	{ 219, "Turbulence Potential Forecast Index", 0, "TPFI" },
	{ 220, "Categorical Severe Thunderstorm", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "SVRTS" },
	{ 221, "Probability of Convection", "%", "PROCON" },
	{ 222, "Convection Potential", "0=No,1=Yes,4=Low,6=Medium,8=High,255=Missing", "CONVP" },
	{ 232, "Volcanic Ash Forecast Transport and Dispersion", "log10 (kg m^-3)", "VAFTD" },
	{ 233, "Icing probability", 0, "ICPRB" },
	{ 234, "Icing severity", 0, "ICSEV" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-20.shtml

const struct parameter_id parameter_0_20[] = {
	{ 0, "Mass Density (Concentration)", "kg m^-3", "MASSDEN" },
	{ 1, "Column-Integrated Mass Density", "kg m^-2", "COLMD" },
	{ 2, "Mass Mixing Ratio (Mass Fraction in Air)", "kg kg^-1", "MASSMR" },
	{ 3, "Atmosphere Emission Mass Flux", "kg m^-2 s^-1", "AEMFLX" },
	{ 4, "Atmosphere Net Production Mass Flux", "kg m^-2 s^-1", "ANPMFLX" },
	{ 5, "Atmosphere Net Production And Emision Mass Flux", "kg m^-2 s^-1", "ANPEMFLX" },
	{ 6, "Surface Dry Deposition Mass Flux", "kg m^-2 s^-1", "SDDMFLX" },
	{ 7, "Surface Wet Deposition Mass Flux", "kg m^-2 s^-1", "SWDMFLX" },
	{ 8, "Atmosphere Re-Emission Mass Flux", "kg m^-2 s^-1", "AREMFLX" },
	{ 9, "Wet Deposition by Large-Scale Precipitation Mass Flux", "kg m^-2 s^-1", "WLSMFLX" },
	{ 10, "Wet Deposition by Convective Precipitation Mass Flux", "kg m^-2 s^-1", "WDCPMFLX" },
	{ 11, "Sedimentation Mass Flux", "kg m^-2 s^-1", "SEDMFLX" },
	{ 12, "Dry Deposition Mass Flux", "kg m^-2 s^-1", "DDMFLX" },
	{ 13, "Transfer From Hydrophobic to Hydrophilic", "kg kg^-1 s^-1", "TRANHH" },
	{ 14, "Transfer From SO2 (Sulphur Dioxide) to SO4 (Sulphate)", "kg kg^-1 s^-1", "TRSDS" },
	{ 50, "Amount in Atmosphere", "mol", "AIA" },
	{ 51, "Concentration In Air", "mol m^-3", "CONAIR" },
	{ 52, "Volume Mixing Ratio (Fraction in Air)", "mol mol^-1", "VMXR" },
	{ 53, "Chemical Gross Production Rate of Concentration", "mol m^-3 s^-1", "CGPRC" },
	{ 54, "Chemical Gross Destruction Rate of Concentration", "mol m^-3 s^-1", "CGDRC" },
	{ 55, "Surface Flux", "mol m^-2 s^-1", "SFLUX" },
	{ 56, "Changes Of Amount in Atmosphere", "mol s^-1", "COAIA" },
	{ 57, "Total Yearly Average Burden of The Atmosphere", "mol", "TYABA" },
	{ 58, "Total Yearly Average Atmospheric Loss", "mol s^-1", "TYAAL" },
	{ 59, "Aerosol Number Concentration", "m^-3", "ANCON" },
	{ 100, "Surface Area Density (Aerosol)", "m^-1", "SADEN" },
	{ 101, "Atmosphere Optical Thickness", "m", "ATMTK" },
	{ 102, "Aerosol Optical Thickness", 0, "AOTK" },
	{ 103, "Single Scattering Albedo", 0, "SSALBK" },
	{ 104, "Asymmetry Factor", 0, "ASYSFK" },
	{ 105, "Aerosol Extinction Coefficient", "m^-1", "AECOEF" },
	{ 106, "Aerosol Absorption Coefficient", "m^-1", "AACOEF" },
	{ 107, "Aerosol Lidar Backscatter from Satellite", "m^-1 sr^-1", "ALBSAT" },
	{ 108, "Aerosol Lidar Backscatter from the Ground", "m^-1 sr^-1", "ALBGRD" },
	{ 109, "Aerosol Lidar Extinction from Satellite", "m^-1", "ALESAT" },
	{ 110, "Aerosol Lidar Extinction from the Ground", "m^-1", "ALEGRD" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-190.shtml

const struct parameter_id parameter_0_190[] = {
	{ 0, "Arbitrary Text String", "CCITTIA5", "ATEXT" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-191.shtml

const struct parameter_id parameter_0_191[] = {
	{ 0, "Seconds prior to initial reference time (defined in Section 1)", "s", "TSEC" },
	{ 1, "Geographical Latitude", "deg N", "GEOLAT" },
	{ 2, "Geographical Longitude", "deg E", "GEOLON" },
	{ 192, "Latitude (-90 to 90)", "deg", "NLAT" },
	{ 193, "East Longitude (0 to 360)", "deg", "ELON" },
	{ 194, "Seconds prior to initial", "s", "TSEC" },
	{ 195, "Model Layer number (From bottom up)", 0, "MLYNO" },
	{ 196, "Latitude (nearest neighbor) (-90 to 90)", "deg", "NLATN" },
	{ 197, "East Longitude (nearest neighbor) (0 to 360)", "deg", "ELONN" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-0-192.shtml

const struct parameter_id parameter_0_192[] = {
	{ 1, "Covariance between zonal and meridional components of", "m^2 m^-2", "COVMZ" },
	{ 2, "Covariance between zonal component of the wind and temperature.", "K m s^-1", "COVTZ" },
	{ 3, "Covariance between meridional component of the wind and temperature.", "K m s^-1", "COVTM" },
	{ 4, "Covariance between temperature and vertical component of", "K m s^-1", "COVTW" },
	{ 5, "Covariance between zonal and zonal", "m^2 s^-2", "COVZZ" },
	{ 6, "Covariance between meridional and meridional", "m^2 s^-2", "COVMM" },
	{ 7, "Covariance between specific humidity and zonal", "kg kg^-1 m s^-1", "COVQZ" },
	{ 8, "Covariance between specific humidity and meridional", "kg kg^-1 m s^-1", "COVQM" },
	{ 9, "Covariance between temperature and vertical", "K Pa s^-1", "COVTVV" },
	{ 10, "Covariance between specific humidity and vertical", "kg kg^-1 Pa s^-1", "COVQVV" },
	{ 11, "Covariance between surface pressure and surface pressure.", "Pa Pa", "COVPSPS" },
	{ 12, "Covariance between specific humidity and specific", "kg kg^-1 kg kg^-1", "COVQQ" },
	{ 13, "Covariance between vertical and vertical", "Pa", "COVVVVV" },
	{ 14, "Covariance between temperature and temperature.", "K K", "COVTT" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-1-0.shtml

const struct parameter_id parameter_1_0[] = {
	{ 0, "Flash Flood Guidance", "kg m^-2", "FFLDG" },
	{ 1, "Flash Flood Runoff", "kg m^-2", "FFLDRO" },
	{ 2, "Remotely Sensed Snow Cover", "50=No Snow/No Cloud,100=Clouds,250=Snow,255=Missing", "RSSC" },
	{ 3, "Elevation of Snow Covered Terrain", "0-90=Elevation in 100m,254=Clouds,255=Missing", "ESCT" },
	{ 4, "Snow Water Equivalent Percent of", "%", "SWEPON" },
	{ 5, "Baseflow-Groundwater Runoff", "kg m^-2", "BGRUN" },
	{ 6, "Storm Surface Runoff", "kg m^-2", "SSRUN" },
	{ 192, "Baseflow-Groundwater Runoff", "kg m^-2", "BGRUN" },
	{ 193, "Storm Surface Runoff", "kg m^-2", "SSRUN" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-1-1.shtml

const struct parameter_id parameter_1_1[] = {
	{ 0, "Conditional percent precipitation amount fractile for an", "kg m^-2", "CPPOP" },
	{ 1, "Percent Precipitation in a", "%", "PPOSP" },
	{ 2, "Probability of 0.01 inch of", "%", "POP" },
	{ 192, "Probability of Freezing", "%", "CPOZP" },
	{ 193, "Probability of Frozen", "%", "CPOFP" },
	{ 194, "Probability of precipitation exceeding flash flood guidance values", "%", "PPFFG" },
	{ 195, "Probability of Wetting Rain, exceeding in 0.10in in a given time period", "%", "CWR" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-1-2.shtml

const struct parameter_id parameter_1_2[] = {
	{ 0, "Water Depth", "m", "WDPTHIL" },
	{ 1, "Water Temperature", "K", "WTMPIL" },
	{ 2, "Water Fraction", 0, "WFRACT" },
	{ 3, "Sediment Thickness", "m", "SEDTK" },
	{ 4, "Sediment Temperature", "K", "SEDTMP" },
	{ 5, "Ice Thickness", "m", "ICTKIL" },
	{ 6, "Ice Temperature", "K", "ICETIL" },
	{ 7, "Ice Cover", 0, "ICECIL" },
	{ 8, "Land Cover (0=water, 1=land)", 0, "LANDIL" },
	{ 9, "Shape Factor with Respect to Salinity Profile", 0, "SFSAL" },
	{ 10, "Shape Factor with Respect to Temperature Profile in Thermocline", 0, "SFTMP" },
	{ 11, "Attenuation Coefficient of Water with Respect to Solar", "m^-1", "ACWSR" },
	{ 12, "Salinity", "kg kg^-1", "SALTIL" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-2-0.shtml

const struct parameter_id parameter_2_0[] = {
	{ 0, "Land Cover (0=sea, 1=land)", 0, "LAND" },
	{ 1, "Surface Roughness", "m", "SFCR" },
	{ 2, "Soil Temperature", "K", "TSOIL" },
	{ 3, "Soil Moisture Content", "kg m^-2", "SOILM" },
	{ 4, "Vegetation", "%", "VEG" },
	{ 5, "Water Runoff", "kg m^-2", "WATR" },
	{ 6, "Evapotranspiration", "kg^-2 s^-1", "EVAPT" },
	{ 7, "Model Terrain Height", "m", "MTERH" },
	{ 8, "Land Use", "1=Urban Land,2=Agricultural,3=Range Land,4=Deciduous Forest,5=Coniferous Forest,6=Forest/Wetland,7=Water,8=Wetlands,9=Desert,10=Tundra,11=Ice,12=Tropical Forest,13=Savannah,255=Missing", "LANDU" },
	{ 9, "Volumetric Soil Moisture Content", 0, "SOILW" },
	{ 10, "Ground Heat Flux", "W m^-2", "GFLUX" },
	{ 11, "Moisture Availability", "%", "MSTAV" },
	{ 12, "Exchange Coefficient", "kg m^-2 s^-1", "SFEXC" },
	{ 13, "Plant Canopy Surface Water", "kg m^-2", "CNWAT" },
	{ 14, "Blackadar's Mixing Length Scale", "m", "BMIXL" },
	{ 15, "Canopy Conductance", "m s^-1", "CCOND" },
	{ 16, "Minimal Stomatal Resistance", "s m^-1", "RSMIN" },
	{ 17, "Wilting Point", 0, "WILT" },
	{ 18, "Solar parameter in canopy", 0, "RCS" },
	{ 19, "Temperature parameter in canopy", 0, "RCT" },
	{ 20, "Humidity parameter in canopy conductance", 0, "RCQ" },
	{ 21, "Soil moisture parameter in canopy conductance", 0, "RCSOL" },
	{ 22, "Soil Moisture", "kg m^-3", "SOILM" },
	{ 23, "Column-Integrated Soil Water", "kg m^-2", "CISOILW" },
	{ 24, "Heat Flux", "W m^-2", "HFLUX" },
	{ 25, "Volumetric Soil Moisture", "m^3 m^-3", "VSOILM" },
	{ 26, "Wilting Point", "kg m^-3", "WILT" },
	{ 27, "Volumetric Wilting Point", "m^3 m^-3", "VWILTP" },
	{ 28, "Leaf Area Index", 0, "LEAINX" },
	{ 29, "Evergreen Forest", 0, "EVERF" },
	{ 30, "Deciduous Forest", 0, "DECF" },
	{ 31, "Normalized Differential Vegetation Index (NDVI)", 0, "NDVINX" },
	{ 32, "Root Depth of Vegetation", "m", "RDVEG" },
	{ 192, "Volumetric Soil Moisture Content", 0, "SOILW" },
	{ 193, "Ground Heat Flux", "W m^-2", "GFLUX" },
	{ 194, "Moisture Availability", "%", "MSTAV" },
	{ 195, "Exchange Coefficient", "kg m^-3 m s^-1", "SFEXC" },
	{ 196, "Plant Canopy Surface Water", "kg m^-2", "CNWAT" },
	{ 197, "Blackadar's Mixing Length Scale", "m", "BMIXL" },
	{ 198, "Vegetation Type", 0, "VGTYP" },
	{ 199, "Canopy Conductance", "m s^-1", "CCOND" },
	{ 200, "Minimal Stomatal Resistance", "s m^-1", "RSMIN" },
	{ 201, "Wilting Point", 0, "WILT" },
	{ 202, "Solar parameter in canopy conductance", 0, "RCS" },
	{ 203, "Temperature parameter in canopy conductance", 0, "RCT" },
	{ 204, "Humidity parameter in canopy conductance", 0, "RCQ" },
	{ 205, "Soil moisture parameter in canopy conductance", 0, "RCSOL" },
	{ 206, "Rate of water dropping from canopy to ground", 0, "RDRIP" },
	{ 207, "Ice-free water surface", "%", "ICWAT" },
	{ 208, "Surface exchange coefficients for T and Q divided by delta z", "m s^-1", "AKHS" },
	{ 209, "Surface exchange coefficients for U and V divided by delta z", "m s^-1", "AKMS" },
	{ 210, "Vegetation canopy temperature", "K", "VEGT" },
	{ 211, "Surface water storage", "kg m^-2", "SSTOR" },
	{ 212, "Liquid soil moisture content (non-frozen)", "kg m^-2", "LSOIL" },
	{ 213, "Open water evaporation (standing water)", "W m^-2", "EWATR" },
	{ 214, "Groundwater recharge", "kg m^-2", "GWREC" },
	{ 215, "Flood plain recharge", "kg m^-2", "QREC" },
	{ 216, "Roughness length for heat", "m", "SFCRH" },
	{ 217, "Normalized Difference Vegetation Index", 0, "NDVI" },
	{ 218, "Land-sea coverage (nearest neighbor) [land=1,sea=0]", 0, "LANDN" },
	{ 219, "Asymptotic mixing length scale", "m", "AMIXL" },
	{ 220, "Water vapor added by precip assimilation", "kg m^-2", "WVINC" },
	{ 221, "Water condensate added by precip assimilation", "kg m^-2", "WCINC" },
	{ 222, "Water Vapor Flux Convergance (Vertical Int)", "kg m^-2", "WVCONV" },
	{ 223, "Water Condensate Flux Convergance (Vertical Int)", "kg m^-2", "WCCONV" },
	{ 224, "Water Vapor Zonal Flux (Vertical Int)", "kg m^-2", "WVUFLX" },
	{ 225, "Water Vapor Meridional Flux (Vertical Int)", "kg m^-2", "WVVFLX" },
	{ 226, "Water Condensate Zonal Flux (Vertical Int)", "kg m^-2", "WCUFLX" },
	{ 227, "Water Condensate Meridional Flux (Vertical Int)", "kg m^-2", "WCVFLX" },
	{ 228, "Aerodynamic conductance", "m s-1", "ACOND" },
	{ 229, "Canopy water evaporation", "W m-2", "EVCW" },
	{ 230, "Transpiration", "W m-2", "TRANS" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-2-1.shtml

const struct parameter_id parameter_2_1[] = {
	{ 192, "Cold Advisory for Newborn Livestock", 0, "CANL" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-2-3.shtml

const struct parameter_id parameter_2_3[] = {
	{ 0, "Soil Type", "1=Sand,2=Loamy Sand,3=Sandy Loam,4=Silt Loam,5=Organic,6=Sandy Clay Loam,7=Silt Clay Loam,8=Clay Loam,9=Sandy Clay,10=Silty Clay,11=Clay,12=Loam,13=Peat,14=Rock,15=Ice,16=Water,255=Missing", "SOTYP" },
	{ 1, "Upper Layer Soil Temperature", "K", "UPLST" },
	{ 2, "Upper Layer Soil Moisture", "kg m^-3", "UPLSM" },
	{ 3, "Lower Layer Soil Moisture", "kg m^-3", "LOWLSM" },
	{ 4, "Bottom Layer Soil Temperature", "K", "BOTLST" },
	{ 5, "Liquid Volumetric Soil Moisture (non-frozen)", 0, "SOILL" },
	{ 6, "Number of Soil Layers in Root Zone", 0, "RLYRS" },
	{ 7, "Transpiration Stress-onset (soil moisture)", 0, "SMREF" },
	{ 8, "Direct Evaporation Cease (soil moisture)", 0, "SMDRY" },
	{ 9, "Soil Porosity", "Proportion", "POROS" },
	{ 10, "Liquid Volumetric Soil Moisture (Non-Frozen)", "m^3 m^-3", "LIQVSM" },
	{ 11, "Volumetric Transpiration Stree-Onset(Soil Moisture)", "m^3 m^-3", "VOLTSO" },
	{ 12, "Transpiration Stree-Onset(Soil Moisture)", "kg m^3", "TRANSO" },
	{ 13, "Volumetric Direct Evaporation Cease(Soil Moisture)", "m^3 m^-3", "VOLDEC" },
	{ 14, "Direct Evaporation Cease(Soil Moisture)", "kg m^3", "DIREC" },
	{ 15, "Soil Porosity", "m^3 m^-3", "SOILP" },
	{ 16, "Volumetric Saturation Of Soil Moisture", "m^3 m^-3", "VSOSM" },
	{ 17, "Saturation Of Soil Moisture", "kg m^-3", "SATOSM" },
	{ 18, "Soil Temperature", "K", "SOILTMP" },
	{ 19, "Soil Moisture", "kg m^-3", "SOILMOI" },
	{ 20, "Column-Integrated Soil Moisture", "kg m^-2", "CISOILM" },
	{ 21, "Soil Ice", "kg m^-3", "SOILICE" },
	{ 22, "Column-Integrated Soil Ice", "kg m^-2", "CISICE" },
	{ 192, "Liquid Volumetric Soil Moisture", 0, "SOILL" },
	{ 193, "Number of Soil Layers in Root Zone", 0, "RLYRS" },
	{ 194, "Surface Slope Type", 0, "SLTYP" },
	{ 195, "Transpiration Stress-onset (soil moisture)", 0, "SMREF" },
	{ 196, "Direct Evaporation Cease (soil moisture)", 0, "SMDRY" },
	{ 197, "Soil Porosity", 0, "POROS" },
	{ 198, "Direct Evaporation from Bare Soil", "W m^-2", "EVBS" },
	{ 199, "Land Surface Precipitation Accumulation", "kg m^-2", "LSPA" },
	{ 200, "Bare Soil Surface Skin temperature", "K", "BARET" },
	{ 201, "Average Surface Skin Temperature", "K", "AVSFT" },
	{ 202, "Effective Radiative Skin Temperature", "K", "RADT" },
	{ 203, "Field Capacity", 0, "FLDCP" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-2-4.shtml

const struct parameter_id parameter_2_4[] = {
	{ 0, "Fire Outlook", "0=No Risk,2=General Thunderstorm,4=Slight Risk,6=Moderate Risk,8=High Risk,11=Dry Thunderstorm (Dry Lightning),14=Critical Risk,18=Extremely Critical Risk,255=Missing", "FIREOLK" },
	{ 1, "Fire Outlook Due to Dry Thunderstorm", "0=No Risk,2=General Thunderstorm,4=Slight Risk,6=Moderate Risk,8=High Risk,11=Dry Thunderstorm (Dry Lightning),14=Critical Risk,18=Extremely Critical Risk,255=Missing", "FIREODT" },
	{ 2, "Haines Index", 0, "HINDEX" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-3-0.shtml

const struct parameter_id parameter_3_0[] = {
	{ 0, "Scaled Radiance", 0, "SRAD" },
	{ 1, "Scaled Albedo", 0, "SALBEDO" },
	{ 2, "Scaled Brightness Temperature", 0, "SBTMP" },
	{ 3, "Scaled Precipitable Water", 0, "SPWAT" },
	{ 4, "Scaled Lifted Index", 0, "SLFTI" },
	{ 5, "Scaled Cloud Top Pressure", 0, "SCTPRES" },
	{ 6, "Scaled Skin Temperature", 0, "SSTMP" },
	{ 7, "Cloud Mask", "0=Clear over Water,1=Clear over Land,2=Cloud,3=No Data,255=Missing", "CLOUDM" },
	{ 8, "Pixel scene type", "0=No Scene Identified,1=Green Needle-Leafed Forest,2=Green Broad-Leafed Forest,3=Deciduous Needle-Leafed Forest,4=Deciduous Broad-Leafed Forest,5=Deciduous Mixed Forest,6=Closed Shrub-Land,7=Open Shrub-Land,8=Woody Savannah,9=Savannah,10=Grassland,11=Permanent Wetland,12=Cropland,13=Urban,14=Vegetation/Crops,15=Permanent Snow/Ice,16=Barren Desert,17=Water Bodies,18=Tundra,97=Snow/Ice on Land,98=Snow/Ice on Water,99=Sun-Glint,100=General Cloud,101=Low Cloud/Fog/Stratus,102=Low Cloud/Stratocumulus,103=Low Cloud/Unknown Type,104=Medium Cloud/Nimbostratus,105=Medium Cloud/Altostratus,106=Medium Cloud/Unknown Type,107=High Cloud/Cumulus,108=High Cloud/Cirrus,109=High Cloud/Unknown Type,110=Unknown Cloud Type,255=Missing", "PIXST" },
	{ 9, "Fire Detection Indicator", "0=No Fire,1=Possible Fire,2=Probable Fire,3=Missing,255=Missing", "FIREDI" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-3-1.shtml

const struct parameter_id parameter_3_1[] = {
	{ 0, "Estimated Precipitation", "kg m^-2", "ESTP" },
	{ 1, "Instantaneous Rain Rate", "kg m^-2 s^-1", "IRRATE" },
	{ 2, "Cloud Top Height", "m", "CTOPH" },
	{ 3, "Cloud Top Height Quality Indicator", "0=Nominal Cloud Top Height Quality,1=Fog In Segment,2=Poor Quality Height Estimation,3=Fog In Segment and Poor Quality Height Estimation,255=Missing", "CTOPHQI" },
	{ 4, "Estimated u-Component of Wind", "m s^-1", "ESTUGRD" },
	{ 5, "Estimated v-Component of Wind", "m s^-1", "ESTVGRD" },
	{ 6, "Number Of Pixels Used", 0, "NPIXU" },
	{ 7, "Solar Zenith Angle", "deg", "SOLZA" },
	{ 8, "Relative Azimuth Angle", "deg", "RAZA" },
	{ 9, "Reflectance in 0.6 Micron Channel", "%", "RFL06" },
	{ 10, "Reflectance in 0.8 Micron Channel", "%", "RFL08" },
	{ 11, "Reflectance in 1.6 Micron Channel", "%", "RFL16" },
	{ 12, "Reflectance in 3.9 Micron Channel", "%", "RFL39" },
	{ 13, "Atmospheric Divergence", "s^-1", "ATMDIV" },
	{ 14, "Cloudy Brightness Temperature", "K", "CBTMP" },
	{ 15, "Clear Sky Brightness Temperature", "K", "CSBTMP" },
	{ 16, "Cloudy Radiance (with respect to wave number)", "W m^-1 sr^-1", "CLDRAD" },
	{ 17, "Clear Sky Radiance (with respect to wave number)", "W m^-1 sr^-1", "CSKYRAD" },
	{ 19, "Wind Speed", "m s^-1", "WINDS" },
	{ 20, "Aerosol Optical Thickness at 0.635um", 0, "AOT06" },
	{ 21, "Aerosol Optical Thickness at 0.810um", 0, "AOT08" },
	{ 22, "Aerosol Optical Thickness at 1.640um", 0, "AOT16" },
	{ 23, "Angstrom Coefficient", 0, "ANGCOE" },
	{ 192, "Scatterometer Estimated U Wind", "m s^-1", "USCT" },
	{ 193, "Scatterometer Estimated V Wind", "m s^-1", "VSCT" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-3-192.shtml

const struct parameter_id parameter_3_192[] = {
	{ 0, "Simulated Brightness Temperature for GOES 12, Channel 2", "K", "SBT122" },
	{ 1, "Simulated Brightness Temperature for GOES 12, Channel 3", "K", "SBT123" },
	{ 2, "Simulated Brightness Temperature for GOES 12, Channel 4", "K", "SBT124" },
	{ 3, "Simulated Brightness Temperature for GOES 12, Channel 6", "K", "SBT126" },
	{ 4, "Simulated Brightness Counts for GOES 12, Channel 3", "Byte", "SBC123" },
	{ 5, "Simulated Brightness Counts for GOES 12, Channel 4", "Byte", "SBC124" },
	{ 6, "Simulated Brightness Temperature for GOES 11, Channel 2", "K", "SBT112" },
	{ 7, "Simulated Brightness Temperature for GOES 11, Channel 3", "K", "SBT113" },
	{ 8, "Simulated Brightness Temperature for GOES 11, Channel 4", "K", "SBT114" },
	{ 9, "Simulated Brightness Temperature for GOES 11, Channel 5", "K", "SBT115" },
	{ 10, "Simulated Brightness Temperature for AMSRE on Aqua, Channel 9", "K", "AMSRE9" },
	{ 11, "Simulated Brightness Temperature for AMSRE on Aqua, Channel 10", "K", "AMSRE10" },
	{ 12, "Simulated Brightness Temperature for AMSRE on Aqua, Channel 11", "K", "AMSRE11" },
	{ 13, "Simulated Brightness Temperature for AMSRE on Aqua, Channel 12", "K", "AMSRE12" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-0.shtml

const struct parameter_id parameter_4_0[] = {
	{ 0, "Temperature", "K", "TMPSWP" },
	{ 1, "Electron Temperature", "K", "ELECTMP" },
	{ 2, "Proton Temperature", "K", "PROTTMP" },
	{ 3, "Ion Temperature", "K", "IONTMP" },
	{ 4, "Parallel Temperature", "K", "PRATMP" },
	{ 5, "Perpendicular Temperature", "K", "PRPTMP" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-1.shtml

const struct parameter_id parameter_4_1[] = {
	{ 0, "Velocity Magnitude (Speed)", "m s^-1", "SPEED" },
	{ 1, "1st Vector Component of Velocity", "m s^-1", "VEL1" },
	{ 2, "2nd Vector Component of Velocity", "m s^-1", "VEL2" },
	{ 3, "3rd Vector Component of Velocity", "m s^-1", "VEL3" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-2.shtml

const struct parameter_id parameter_4_2[] = {
	{ 0, "Particle Number Density", "m^-3", "PLSMDEN" },
	{ 1, "Electron Density", "m^-3", "ELCDEN" },
	{ 2, "Proton Density", "m^-3", "PROTDEN" },
	{ 3, "Ion Density", "m^-3", "IONDEN" },
	{ 4, "Vertical Electron Content", "m^-2", "VTEC" },
	{ 5, "HF Absorption Frequency", "Hz", "ABSFRQ" },
	{ 6, "HF Absorption", "dB", "ABSRB" },
	{ 7, "Spread F", "m", "SPRDF" },
	{ 8, "h'F", "m", "HPRIMF" },
	{ 9, "Critical Frequency", "Hz", "CRTFRQ" },
	{ 10, "Scintillation", 0, "SCINT" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-3.shtml

const struct parameter_id parameter_4_3[] = {
	{ 0, "Magnetic Field Magnitude", "T", "BTOT" },
	{ 1, "1st Vector Component of Magnetic Field", "T", "BVEC1" },
	{ 2, "2nd Vector Component of Magnetic Field", "T", "BVEC2" },
	{ 3, "3rd Vector Component of Magnetic Field", "T", "BVEC3" },
	{ 4, "Electric Field Magnitude", "V m^-1", "ETOT" },
	{ 5, "1st Vector Component of Electric Field", "V m^-1", "EVEC1" },
	{ 6, "2nd Vector Component of Electric Field", "V m^-1", "EVEC2" },
	{ 7, "3rd Vector Component of Electric Field", "V m^-1", "EVEC3" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-4.shtml

const struct parameter_id parameter_4_4[] = {
	{ 0, "Proton Flux (Differential)", "(m^2 s sr eV)^-1", "DIFPFLUX" },
	{ 1, "Proton Flux (Integral)", "(m^2 s sr)^-1", "INTPFLUX" },
	{ 2, "Electron Flux (Differential)", "(m^2 s sr eV)^-1", "DIFEFLUX" },
	{ 3, "Electron Flux (Integral)", "(m^2 s sr)^-1", "INTEFLUX" },
	{ 4, "Heavy Ion Flux (Differential)", "(m^2 s sr eV nuc^-1)^-1", "DIFIFLUX" },
	{ 5, "Heavy Ion Flux (iIntegral)", "(m^2 s sr)^-1", "INTIFLUX" },
	{ 6, "Cosmic Ray Neutron Flux", "h^-1", "NTRNFLUX" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-5.shtml

const struct parameter_id parameter_4_5[] = {
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-6.shtml

const struct parameter_id parameter_4_6[] = {
	{ 0, "Integrated Solar Irradiance", "W m^-2", "TSI" },
	{ 1, "Solar X-ray Flux (XRS Long)", "W m^-2", "XLONG" },
	{ 2, "Solar X-ray Flux (XRS Short)", "W m^-2", "XSHRT" },
	{ 3, "Solar EUV Irradiance", "W m^-2", "EUVIRR" },
	{ 4, "Solar Spectral Irradiance", "W m^-2 nm^-1", "SPECIRR" },
	{ 5, "F10.7", "W m^-2 Hz^-1", "F107" },
	{ 6, "Solar Radio Emissions", "W m^-2 Hz^-1", "SOLRF" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-7.shtml

const struct parameter_id parameter_4_7[] = {
	{ 0, "Limb Intensity", "m^-2 s^-1", "LMBINT" },
	{ 1, "Disk Intensity", "m^-2 s^-1", "DSKINT" },
	{ 2, "Disk Intensity Day", "m^-2 s^-1", "DSKDAY" },
	{ 3, "Disk Intensity Night", "m^-2 s^-1", "DSKNGT" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-8.shtml

const struct parameter_id parameter_4_8[] = {
	{ 0, "X-Ray Radiance", "W sr^-1 m^-2", "XRAYRAD" },
	{ 1, "EUV Radiance", "W sr^-1 m^-2", "EUVRAD" },
	{ 2, "H-Alpha Radiance", "W sr^-1 m^-2", "HARAD" },
	{ 3, "White Light Radiance", "W sr^-1 m^-2", "WHTRAD" },
	{ 4, "CaII-K Radiance", "W sr^-1 m^-2", "CAIIRAD" },
	{ 5, "White Light Coronagraph Radiance", "W sr^-1 m^-2", "WHTCOR" },
	{ 6, "Heliospheric Radiance", "W sr^-1 m^-2", "HELCOR" },
	{ 7, "Thematic Mask", 0, "MASK" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-4-9.shtml

const struct parameter_id parameter_4_9[] = {
	{ 0, "Pedersen Conductivity", "S m^-1", "SIGPED" },
	{ 1, "Hall Conductivity", "S m^-1", "SIGHAL" },
	{ 2, "Parallel Conductivity", "S m^-1", "SIGPAR" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-10-0.shtml

const struct parameter_id parameter_10_0[] = {
	{ 0, "Wave Spectra (1)", 0, "WVSP1" },
	{ 1, "Wave Spectra (2)", 0, "WVSP2" },
	{ 2, "Wave Spectra (3)", 0, "WVSP3" },
	{ 3, "Significant Height of Combined", "m", "HTSGW" },
	{ 4, "Direction of Wind Waves", "degree true", "WVDIR" },
	{ 5, "Significant Height of Wind Waves", "m", "WVHGT" },
	{ 6, "Mean Period of Wind Waves", "s", "WVPER" },
	{ 7, "Direction of Swell Waves", "degree true", "SWDIR" },
	{ 8, "Significant Height of Swell Waves", "m", "SWELL" },
	{ 9, "Mean Period of Swell Waves", "s", "SWPER" },
	{ 10, "Primary Wave Direction", "degree true", "DIRPW" },
	{ 11, "Primary Wave Mean Period", "s", "PERPW" },
	{ 12, "Secondary Wave Direction", "degree true", "DIRSW" },
	{ 13, "Secondary Wave Mean Period", "s", "PERSW" },
	{ 14, "Direction of Combined Wind Waves and Swell", "degree true", "WWSDIR" },
	{ 15, "Mean Period of Combined Wind Waves and Swell", "s", "MWSPER" },
	{ 16, "Coefficient of Drag With Waves", 0, "CDWW" },
	{ 17, "Friction Velocity", "m s^-1", "FRICV" },
	{ 18, "Wave Stress", "N m^-2", "WSTR" },
	{ 19, "Normalised Waves Stress", 0, "NWSTR" },
	{ 20, "Mean Square Slope of Waves", 0, "MSSW" },
	{ 21, "U-component Surface Stokes Drift", "m s^-1", "USSD" },
	{ 22, "V-component Surface Stokes Drift", "m s^-1", "VSSD" },
	{ 23, "Period of Maximum Individual Wave Height", "s", "PMAXWH" },
	{ 24, "Maximum Individual Wave Height", "m", "MAXWH" },
	{ 25, "Inverse Mean Wave Frequency", "s", "IMWF" },
	{ 26, "Inverse Mean Frequency of The Wind Waves", "s", "IMFWW" },
	{ 27, "Inverse Mean Frequency of The Total Swell", "s", "IMFTSW" },
	{ 28, "Mean Zero-Crossing Wave Period", "s", "MZWPER" },
	{ 29, "Mean Zero-Crossing Period of The Wind Waves", "s", "MZPWW" },
	{ 30, "Mean Zero-Crossing Period of The Total Swell", "s", "MZPTSW" },
	{ 31, "Wave Directional Width", 0, "WDIRW" },
	{ 32, "Directional Width of The Wind Waves", 0, "DIRWWW" },
	{ 33, "Directional Width of The Total Swell", 0, "DIRWTS" },
	{ 34, "Peak Wave Period", "s", "PWPER" },
	{ 35, "Peak Period of The Wind Waves", "s", "PPERWW" },
	{ 36, "Peak Period of The Total Swell", "s", "PPERTS" },
	{ 37, "Altimeter Wave Height", "m", "ALTWH" },
	{ 38, "Altimeter Corrected Wave Height", "m", "ALCWH" },
	{ 39, "Altimeter Range Relative Correction", 0, "ALRRC" },
	{ 40, "10 Metre Neutral Wind Speed Over Waves", "m s^-1", "MNWSOW" },
	{ 41, "10 Metre Wind Direction Over Waves", "degree true", "MWDIRW" },
	{ 42, "Wave Engery Spectrum", "m^-2 s rad^-1", "WESP" },
	{ 43, "Kurtosis of The Sea Surface Elevation Due to Waves", 0, "KSSEDW" },
	{ 44, "Benjamin-Feir Index", 0, "BENINX" },
	{ 45, "Spectral Peakedness Factor", "s^-1", "SPFTR" },
	{ 46, "2-Dimension Spectral Energy Density E(f,theta)", "m s^-2", "2DSED" },
	{ 47, "Frequency Spectral Energy Density", "m s^-2", "FSEED" },
	{ 48, "Directional Spectral Energy Density", "m s^-2", "DIRSED" },
	{ 50, "Significant Wave Height", "m", "HSIGN" },
	{ 51, "Peak Direction", "degree true", "PKDIR" },
	{ 52, "Wave Steepness", 0, "MNSTP" },
	{ 53, "Mean Wave Directional Spread", "degree", "DMSPR" },
	{ 54, "Wind-Forced Fraction of the Wave Spectrum", 0, "WFFRAC" },
	{ 55, "Energy Mean Wave Period (TMM1)", "s", "TEMM1" },
	{ 56, "First Directional Moments Mean Wave Direction", "degree true", "DIR11" },
	{ 57, "Second Directional Moments Mean Wave Direction", "degree true", "DIR22" },
	{ 58, "First Directional Moments Mean Directional Spread", "degree", "DSPR11" },
	{ 59, "Second Directional Moments Mean Directional Spread", "degree", "DSPR22" },
	{ 60, "Mean Wave Length", "m", "WLEN" },
	{ 61, "Sxx Component Radiation Stress", "N m^-2", "RDSXX" },
	{ 62, "Syy Component Radiation Stress", "N m^-2", "RDSYY" },
	{ 63, "Sxy Component Radiation Stress", "N m^-2", "RDSXY" },
	{ 192, "Wave Steepness", 0, "WSTP" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-10-1.shtml

const struct parameter_id parameter_10_1[] = {
	{ 0, "Current Direction", "degree True", "DIRC" },
	{ 1, "Current Speed", "m s^-1", "SPC" },
	{ 2, "U-Component of Current", "m s^-1", "UOGRD" },
	{ 3, "V-Component of Current", "m s^-1", "VOGRD" },
	{ 192, "Ocean Mixed Layer U Velocity", "m s^-1", "OMLU" },
	{ 193, "Ocean Mixed Layer V Velocity", "m s^-1", "OMLV" },
	{ 194, "Barotropic U velocity", "m s^-1", "UBARO" },
	{ 195, "Barotropic V velocity", "m s^-1", "VBARO" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-10-2.shtml

const struct parameter_id parameter_10_2[] = {
	{ 0, "Ice Cover", 0, "ICEC" },
	{ 1, "Ice Thickness", "m", "ICETK" },
	{ 2, "Direction of Ice Drift", "degree True", "DICED" },
	{ 3, "Speed of Ice Drift", "m s^-1", "SICED" },
	{ 4, "U-Component of Ice Drift", "m s^-1", "UICE" },
	{ 5, "V-Component of Ice Drift", "m s^-1", "VICE" },
	{ 6, "Ice Growth Rate", "m s^-1", "ICEG" },
	{ 7, "Ice Divergence", "s^-1", "ICED" },
	{ 8, "Ice Temperature", "K", "ICETMP" },
	{ 9, "Ice Internal Pressure", "Pa m", "ICEPRS" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-10-3.shtml

const struct parameter_id parameter_10_3[] = {
	{ 0, "Water Temperature", "K", "WTMP" },
	{ 1, "Deviation of Sea Level from Mean", "m", "DSLM" },
	{ 192, "Hurricane Storm Surge", "m", "SURGE" },
	{ 193, "Extra Tropical Storm Surge", "m", "ETSRG" },
	{ 194, "Ocean Surface Elevation Relative to Geoid", "m", "ELEV" },
	{ 195, "Sea Surface Height Relative to Geoid", "m", "SSHG" },
	{ 196, "Ocean Mixed Layer Potential Density (Reference 2000m)", "kg m^-3", "P2OMLT" },
	{ 197, "Net Air-Ocean Heat Flux", "W m^-2", "AOHFLX" },
	{ 198, "Assimilative Heat Flux", "W m^-2", "ASHFL" },
	{ 199, "Surface Temperature Trend", "degree day^-1", "SSTT" },
	{ 200, "Surface Salinity Trend", "psu day^-1", "SSST" },
	{ 201, "Kinetic Energy", "J kg^-1", "KENG" },
	{ 202, "Salt Flux", "kg m^-2 s^-1", "SLTFL" },
	{ 242, "20% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG20" },
	{ 243, "30% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG30" },
	{ 244, "40% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG40" },
	{ 245, "50% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG50" },
	{ 246, "60% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG60" },
	{ 247, "70% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG70" },
	{ 248, "80% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG80" },
	{ 249, "90% Tropical Cyclone Storm Surge Exceedance", "m", "TCSRG90" },
	{ 250, "Extra Tropical Storm Surge Combined Surge and Tide", "m", "ETCWL" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-10-4.shtml

const struct parameter_id parameter_10_4[] = {
	{ 0, "Main Thermocline Depth", "m", "MTHD" },
	{ 1, "Main Thermocline Anomaly", "m", "MTHA" },
	{ 2, "Transient Thermocline Depth", "m", "TTHDP" },
	{ 3, "Salinity", "kg kg^-1", "SALTY" },
	{ 4, "Ocean Vertical Heat Diffusivity", "m^2 s^-1", "OVHD" },
	{ 5, "Ocean Vertical Salt Diffusivity", "m^2 s^-1", "OVSD" },
	{ 6, "Ocean Vertical Momentum Diffusivity", "m^2 s^-1", "OVMD" },
	{ 7, "Bathymetry", "m", "BATHY" },
	{ 11, "Shape Factor With Respect To Salinity Profile", 0, "SFSALP" },
	{ 12, "Shape Factor With Respect To Temperature Profile In", 0, "SFTMPP" },
	{ 13, "Attenuation Coefficient Of Water With Respect to Solar Radiation", "m^-1", "ACWSRD" },
	{ 14, "Water Depth", "m", "WDEPTH" },
	{ 15, "Water Temperature", "K", "WTMPSS" },
	{ 192, "3-D Temperature", "degC", "WTMPC" },
	{ 193, "3-D Salinity", "psu", "SALIN" },
	{ 194, "Barotropic Kinectic Energy", "J kg^-1", "BKENG" },
	{ 195, "Geometric Depth Below Sea Surface", "m", "DBSS" },
	{ 196, "Interface Depths", "m", "INTFD" },
	{ 197, "Ocean Heat Content", "J m^-2", "OHC" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

// grib2_table4-2-10-191.shtml

const struct parameter_id parameter_10_191[] = {
	{ 0, "Seconds Prior To Initial Reference Time (Defined In Section 1)", "s", "TSEC" },
	{ 1, "Meridional Overturning Stream Function", "m^3 s^-1", "MOSF" },
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

const struct parameter_id parameter_missing[] = {
	{ 255, "Missing", 0, 0 },
	{ 0, 0, 0, 0 }
};

const struct paramcategory_id paramcategory_0[] = {
	{ 0, "Temperature", parameter_0_0 },
	{ 1, "Moisture", parameter_0_1 },
	{ 2, "Momentum", parameter_0_2 },
	{ 3, "Mass", parameter_0_3 },
	{ 4, "Short-wave radiation", parameter_0_4 },
	{ 5, "Long-wave radiation", parameter_0_5 },
	{ 6, "Cloud", parameter_0_6 },
	{ 7, "Thermodynamic Stability indicies", parameter_0_7 },
	{ 8, "Kinematic Stability indicies", 0 },
	{ 9, "Temperature Probabilities (deprecated)", 0 },
	{ 10, "Moisture Probabilities (deprecated)", 0 },
	{ 11, "Momentum Probabilities (deprecated)", 0 },
	{ 12, "Mass Probabilities (deprecated)", 0 },
	{ 13, "Aerosols", parameter_0_13 },
	{ 14, "Trace gases", parameter_0_14 },
	{ 15, "Radar", parameter_0_15 },
	{ 16, "Forecast Radar Imagery", parameter_0_16 },
	{ 17, "Electrodynamics", parameter_0_17 },
	{ 18, "Nuclear/radiology", parameter_0_18 },
	{ 19, "Physical atmospheric properties", parameter_0_19 },
	{ 20 , "Atmospheric chemical Constituents", parameter_0_20 },
	{ 190, "CCITT IA5 string", parameter_0_190 },
	{ 191, "Miscellaneous", parameter_0_191 },
	{ 192, "Covariance", parameter_0_192 },
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const struct paramcategory_id paramcategory_1[] = {
	{ 0, "Hydrology basic products", parameter_1_0 },
	{ 1, "Hydrology probabilities", parameter_1_1 },
	{ 2, "Inland water and sediment properties", parameter_1_2 },
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const struct paramcategory_id paramcategory_2[] = {
	{ 0, "Vegetation/Biomass", parameter_2_0 },
	{ 1, "Agricultural/Aquacultural Special Products", parameter_2_1 },
	{ 2, "Transportation-related Products", 0 },
	{ 3, "Soil Products", parameter_2_3 },
	{ 4, "Fire Weather", parameter_2_4 },
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const struct paramcategory_id paramcategory_3[] = {
	{ 0, "Image format products", parameter_3_0 },
	{ 1, "Quantitative products", parameter_3_1 },
	{ 192, "Forecast Satellite Imagery", parameter_3_192 },
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const struct paramcategory_id paramcategory_4[] = {
	{ 0, "Temperature", parameter_4_0 },
	{ 1, "Momentum", parameter_4_1 },
	{ 2, "Charged Particle Mass and Number", parameter_4_2 },
	{ 3, "Electric and Magnetic Fields", parameter_4_3 },
	{ 4, "Energetic Particles", parameter_4_4 },
	{ 5, "Waves", parameter_4_5 },
	{ 6, "Solar Electromagnetic Emissions", parameter_4_6 },
	{ 7, "Terrestrial Electromagnetic Emissions", parameter_4_7 },
	{ 8, "Imagery", parameter_4_8 },
	{ 9, "Ion-Neutral Coupling", parameter_4_9 },
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const struct paramcategory_id paramcategory_10[] = {
	{ 0, "Waves", parameter_10_0 },
	{ 1, "Currents", parameter_10_1 },
	{ 2, "Ice", parameter_10_2 },
	{ 3, "Surface Properties", parameter_10_3 },
	{ 4, "Sub-surface Properties", parameter_10_4 },
	{ 191, "Miscellaneous", parameter_10_191 },
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const struct paramcategory_id paramcategory_missing[] = {
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const struct paramdiscipline_id paramdiscipline_id[] = {
	{ 0, "Meteorological Products", paramcategory_0 },
	{ 1, "Hydrological Products", paramcategory_1 },
	{ 2, "Land Surface Products", paramcategory_2 },
	{ 3, "Space Products", paramcategory_3 },
	{ 4, "Space Weather Products", paramcategory_4 },
	{ 10, "Oceanographic Products", paramcategory_10 },
	{ 0, 0 }
};

class Gen {
public:
	Gen(void);
	std::ostream& print(std::ostream& os) const;

protected:
	class Parameter {
	public:
		Parameter(const struct paramdiscipline_id *pdisc, 
			  const struct paramcategory_id *pcat,
			  const struct parameter_id *par) : 
			m_discipline(pdisc), m_paramcategory(pcat), m_parameter(par),
			m_disciplineindex(0), m_paramcategoryindex(0), m_parameterindex(0) {}
		uint8_t get_disciplineid(void) const { return m_discipline ? m_discipline->id : 0xff; }
		const char *get_disciplinestr(void) const { return m_discipline ? m_discipline->str : 0; }
		uint8_t get_paramcategoryid(void) const { return m_paramcategory ? m_paramcategory->id : 0xff; }
		const char *get_paramcategorystr(void) const { return m_paramcategory ? m_paramcategory->str : 0; }
		uint8_t get_parameterid(void) const { return m_parameter ? m_parameter->id : 0xff; }
		const char *get_parameterstr(void) const { return m_parameter ? m_parameter->str : 0; }
		const char *get_parameterunit(void) const { return m_parameter ? m_parameter->unit : 0; }
		const char *get_parameterabbrev(void) const { return m_parameter ? m_parameter->abbrev : 0; }
		unsigned int get_disciplineindex(void) const { return m_disciplineindex; }
		unsigned int get_paramcategoryindex(void) const { return m_paramcategoryindex; }
		unsigned int get_parameterindex(void) const { return m_parameterindex; }
		void set_indices(unsigned int disciplineindex, unsigned int paramcategoryindex, unsigned int parameterindex)
			{ m_disciplineindex = disciplineindex; m_paramcategoryindex = paramcategoryindex; m_parameterindex = parameterindex;  }
		bool operator<(const Parameter& x) const;

	protected:
		const struct paramdiscipline_id *m_discipline;
		const struct paramcategory_id *m_paramcategory;
		const struct parameter_id *m_parameter;
		unsigned int m_disciplineindex;
		unsigned int m_paramcategoryindex;
		unsigned int m_parameterindex;
	};

	class Index {
	public:
		Index(const Parameter *par = 0) : m_par(par) {}
		unsigned int get_disciplineindex(void) const { return m_par ? m_par->get_disciplineindex() : 0; }
		unsigned int get_paramcategoryindex(void) const { return m_par ? m_par->get_paramcategoryindex() : 0; }
		unsigned int get_parameterindex(void) const { return m_par ? m_par->get_parameterindex() : 0; }

	protected:
		const Parameter *m_par;
	};

	class DiscIndex : public Index {
	public:
		DiscIndex(const Parameter *par = 0) : Index(par) {}
		bool operator<(const DiscIndex& x) const;
	};

	class CatIndex : public Index {
	public:
		CatIndex(const Parameter *par = 0) : Index(par) {}
		bool operator<(const CatIndex& x) const;
	};

	class ParIndex : public Index {
	public:
		ParIndex(const Parameter *par = 0) : Index(par) {}
		bool operator<(const ParIndex& x) const;
	};

	class AbbrevIndex : public Index {
	public:
		AbbrevIndex(const Parameter *par = 0) : Index(par) {}
		bool operator<(const AbbrevIndex& x) const;
	};

	static std::string to_cname(const std::string& x);

	typedef std::set<Parameter> params_t;
	params_t m_params;
	typedef std::set<std::string> units_t;
	units_t m_units;

	typedef std::set<DiscIndex> discindex_t;
	discindex_t m_discindex;
	typedef std::set<CatIndex> catindex_t;
	catindex_t m_catindex;
	typedef std::set<ParIndex> parindex_t;
	parindex_t m_parindex;
	typedef std::set<AbbrevIndex> abbrevindex_t;
	abbrevindex_t m_abbrevindex;

	unsigned int m_nrdiscipline;
	unsigned int m_nrparamcategory;
	unsigned int m_nrparameter;
	unsigned int m_maxabbrevlen;
};

bool Gen::Parameter::operator<(const Parameter& x) const
{
	if (get_disciplineid() < x.get_disciplineid())
		return true;
	if (x.get_disciplineid() < get_disciplineid())
		return false;
	if (get_paramcategoryid() < x.get_paramcategoryid())
		return true;
	if (x.get_paramcategoryid() < get_paramcategoryid())
		return false;
	return get_parameterid() < x.get_parameterid();
}

bool Gen::DiscIndex::operator<(const DiscIndex& x) const
{
	if (m_par) {
		if (!x.m_par)
			return false;
		const char *a(m_par->get_disciplinestr());
		const char *b(x.m_par->get_disciplinestr());
		if (a) {
			if (!b)
				return false;
			int c(strcmp(a, b));
			if (c)
				return c < 0;
		} else if (b) {
			return true;
		}
	} else {
		return !!x.m_par;
	}
	return m_par->get_disciplineindex() < x.m_par->get_disciplineindex();
}

bool Gen::CatIndex::operator<(const CatIndex& x) const
{
	if (m_par) {
		if (!x.m_par)
			return false;
		const char *a(m_par->get_paramcategorystr());
		const char *b(x.m_par->get_paramcategorystr());
		if (a) {
			if (!b)
				return false;
			int c(strcmp(a, b));
			if (c)
				return c < 0;
		} else if (b) {
			return true;
		}
	} else {
		return !!x.m_par;
	}
        return m_par->get_paramcategoryindex() < x.m_par->get_paramcategoryindex();
}

bool Gen::ParIndex::operator<(const ParIndex& x) const
{
	if (m_par) {
		if (!x.m_par)
			return false;
		const char *a(m_par->get_parameterstr());
		const char *b(x.m_par->get_parameterstr());
		if (a) {
			if (!b)
				return false;
			int c(strcmp(a, b));
			if (c)
				return c < 0;
		} else if (b) {
			return true;
		}
	} else {
		return !!x.m_par;
	}
	return m_par->get_parameterindex() < x.m_par->get_parameterindex();
}

bool Gen::AbbrevIndex::operator<(const AbbrevIndex& x) const
{
	if (m_par) {
		if (!x.m_par)
			return false;
		const char *a(m_par->get_parameterabbrev());
		const char *b(x.m_par->get_parameterabbrev());
		if (a) {
			if (!b)
				return false;
			int c(strcmp(a, b));
			if (c)
				return c < 0;
		} else if (b) {
			return true;
		}
	} else {
		return !!x.m_par;
	}
	return m_par->get_parameterindex() < x.m_par->get_parameterindex();
}

Gen::Gen(void)
	: m_nrdiscipline(0), m_nrparamcategory(0), m_nrparameter(0), m_maxabbrevlen(0)
{
	for (const struct paramdiscipline_id *pdisc(paramdiscipline_id); pdisc->str; ++pdisc) {
		for (const struct paramcategory_id *pcat(pdisc->cat ? pdisc->cat : paramcategory_missing); pcat->str; ++pcat) {
			for (const struct parameter_id *par(pcat->par ? pcat->par : parameter_missing); par->str; ++par) {
				Parameter p(pdisc, pcat, par);
				if (!p.get_parameterabbrev() && p.get_parameterid() != 0xff) {
					std::cerr << "Skipping " << pdisc->str << ":" << pcat->str << ":" << par->str
						  << " due to no abbreviation" << std::endl;
					continue;
				}
				std::pair<params_t::iterator,bool> ins(m_params.insert(p));
				if (!ins.second) {
					std::cerr << "Skipping " << pdisc->str << ":" << pcat->str << ":" << par->str
						  << " due to duplicate IDs" << std::endl;
					continue;
				}
				if (p.get_parameterunit())
					m_units.insert(std::string(p.get_parameterunit()));
			}
		}
	}
	for (params_t::const_iterator i(m_params.begin()), e(m_params.end()); i != e;) {
		const_cast<Parameter&>(*i).set_indices(m_nrdiscipline, m_nrparamcategory, m_nrparameter);
		if (i->get_parameterstr())
			++m_nrparameter;
		params_t::iterator i0(i);
		++i;
		if (i == e)
			break;
		if (i0->get_disciplineid() != i->get_disciplineid()) {
			++m_nrdiscipline;
			if (!strcmp(i0->get_disciplinestr(), i->get_disciplinestr())) {
				std::cerr << "Warning: disciplines " << (unsigned int)i0->get_disciplineid()
					  << " and " << (unsigned int)i->get_disciplineid()
					  << " have the same name" << std::endl;
			}
		}
		if (i0->get_paramcategoryid() != i->get_paramcategoryid()) {
			++m_nrparamcategory;
			if (i0->get_disciplineid() == i->get_disciplineid() &&
			    !strcmp(i0->get_paramcategorystr(), i->get_paramcategorystr())) {
				std::cerr << "Warning: parameter categories " << (unsigned int)i0->get_paramcategoryid()
					  << " and " << (unsigned int)i->get_paramcategoryid()
					  << " (discipline " << (unsigned int)i0->get_disciplineid()
					  << ") have the same name" << std::endl;
			}
		}
	}
	if (m_nrparameter) {
		++m_nrdiscipline;
		++m_nrparamcategory;
	}
	for (params_t::const_iterator i(m_params.begin()), e(m_params.end()); i != e; ++i) {
		if (i->get_disciplinestr())
			m_discindex.insert(DiscIndex(&*i));
		if (i->get_paramcategorystr())
			m_catindex.insert(CatIndex(&*i));
		if (i->get_parameterstr())
			m_parindex.insert(ParIndex(&*i));
		if (i->get_parameterabbrev())
			m_abbrevindex.insert(AbbrevIndex(&*i));
	}
}

std::string Gen::to_cname(const std::string& x)
{
	std::string r;
	for (std::string::const_iterator i(x.begin()), e(x.end()); i != e; ++i) {
		if (std::isalnum(*i)) {
			r.push_back(*i);
			continue;
		}
		if (*i == '^') {
			r += "sup";
			continue;
		}
		if (*i == '%') {
			r += "percent";
			continue;
		}
		if (*i == '-') {
			r.push_back('m');
			continue;
		}
		if (*i == ' ') {
			r.push_back('_');
			continue;
		}
		continue;
	}
	return r;
}

std::ostream& Gen::print(std::ostream& os) const
{
	os << "// maximum abbreviation length " << m_maxabbrevlen << std::endl << std::endl
	   << "namespace {" << std::endl;
	for (units_t::const_iterator ui(m_units.begin()), ue(m_units.end()); ui != ue; ++ui)
		os << "static const char unit_" << to_cname(*ui) << "[] = \"" << *ui << "\";" << std::endl;
	os << "};" << std::endl << std::endl
	   << "const GRIB2::Parameter GRIB2::parameters[" << (m_nrparameter + 1) << "] = {";
	{
		unsigned int index(~0U);
		for (params_t::const_iterator i(m_params.begin()), e(m_params.end()); i != e; ++i) {
			if (!i->get_parameterstr())
				continue;
			if (index != ~0U)
				os << ", // " << index;
			index = i->get_parameterindex();
			os << std::endl << "\t{ &paramcategory[" << i->get_paramcategoryindex()
			   << "], ";
			if (i->get_parameterstr())
				os << '\"' << i->get_parameterstr() << '\"';
			else
				os << '0';
			os << ", ";
			if (i->get_parameterunit())
				os << "unit_" << to_cname(i->get_parameterunit());
			else
				os << '0';
			os << ", \"";
			if (i->get_parameterabbrev())
				os << i->get_parameterabbrev();
			os << "\", " << (unsigned int)i->get_parameterid()
			   << ", " << (unsigned int)i->get_paramcategoryid()
			   << ", " << (unsigned int)i->get_disciplineid() << " }";
		}
		if (index != 0U)
			os << ", // " << index;
		os << "\t{ 0, 0, 0, \"\", 255, 255, 255 }";
	}
	os << std::endl << "};" << std::endl << std::endl
	   << "const GRIB2::ParamCategory GRIB2::paramcategory[" << (m_nrparamcategory + 1) << "] = {";
	{
		unsigned int index(~0U);
		for (params_t::const_iterator i(m_params.begin()), e(m_params.end()); i != e; ++i) {
			if (i->get_paramcategoryindex() == index)
				continue;
			if (index != ~0U)
				os << ", // " << index;
			index = i->get_paramcategoryindex();
			os << std::endl << "\t{ &paramdiscipline[" << i->get_disciplineindex()
			   << "], &parameters[" << i->get_parameterindex() << "], ";
			if (i->get_paramcategorystr())
				os << '\"' << i->get_paramcategorystr() << '\"';
			else
				os << '0';
			os << ", " << (unsigned int)i->get_paramcategoryid()
			   << ", " << (unsigned int)i->get_disciplineid() << " }";
		}
		if (index != ~0U)
			os << ", // " << index;
	}
	os << std::endl << "\t{ 0, &parameters[" << m_nrparameter << "], 0 }"
	   << std::endl << "};" << std::endl << std::endl
	   << "const GRIB2::ParamDiscipline GRIB2::paramdiscipline[" << (m_nrdiscipline + 1) << "] = {";
	{
		unsigned int index(~0U);
		for (params_t::const_iterator i(m_params.begin()), e(m_params.end()); i != e; ++i) {
			if (i->get_disciplineindex() == index)
				continue;
			if (index != ~0U)
				os << ", // " << index;
			index = i->get_disciplineindex();
			os << std::endl << "\t{ &paramcategory[" << i->get_paramcategoryindex() << "], ";
			if (i->get_disciplinestr())
				os << '\"' << i->get_disciplinestr() << '\"';
			else
				os << '0';
			os << ", " << (unsigned int)i->get_disciplineid() << " }";
		}
		if (index != ~0U)
			os << ", // " << index;
	}
	os << std::endl << "\t{ &paramcategory[" << m_nrparamcategory << "], 0 }"
	   << std::endl << "};" << std::endl << std::endl
	   << "const GRIB2::ParamDiscipline *GRIB2::paramdisciplineindex[" << (m_discindex.size() + 1) << "] = {";
	{
		bool subseq(false);
		for (discindex_t::const_iterator i(m_discindex.begin()), e(m_discindex.end()); i != e; ++i) {
			if (subseq)
				os << ',';
			subseq = true;
			os << std::endl << "\t&paramdiscipline[" << i->get_disciplineindex() << ']';
		}
		if (subseq)
			os << ',';
	}
	os << std::endl << "\t0" << std::endl<< "};" << std::endl << std::endl
	   << "const GRIB2::ParamCategory *GRIB2::paramcategoryindex[" << (m_catindex.size() + 1) << "] = {";
	{
		bool subseq(false);
		for (catindex_t::const_iterator i(m_catindex.begin()), e(m_catindex.end()); i != e; ++i) {
			if (subseq)
				os << ',';
			subseq = true;
			os << std::endl << "\t&paramcategory[" << i->get_paramcategoryindex() << ']';
		}
		if (subseq)
			os << ',';
	}
	os << std::endl << "\t0" << std::endl << "};" << std::endl << std::endl
	   << "const GRIB2::Parameter *GRIB2::parameternameindex[" << (m_parindex.size() + 1) << "] = {";
	{
		bool subseq(false);
		for (parindex_t::const_iterator i(m_parindex.begin()), e(m_parindex.end()); i != e; ++i) {
			if (subseq)
				os << ',';
			subseq = true;
			os << std::endl << "\t&parameters[" << i->get_parameterindex() << ']';
		}
		if (subseq)
			os << ',';
	}
	os << std::endl << "\t0" << std::endl << "};" << std::endl << std::endl
	   << "const GRIB2::Parameter *GRIB2::parameterabbrevindex[" << (m_abbrevindex.size() + 1) << "] = {";
	{
		bool subseq(false);
		for (abbrevindex_t::const_iterator i(m_abbrevindex.begin()), e(m_abbrevindex.end()); i != e; ++i) {
			if (subseq)
				os << ',';
			subseq = true;
			os << std::endl << "\t&parameters[" << i->get_parameterindex() << ']';
		}
		if (subseq)
			os << ',';
	}
	os << std::endl << "\t0" << std::endl << "};" << std::endl << std::endl
	   << "const unsigned int GRIB2::nr_paramdiscipline = " << m_nrdiscipline << ';' << std::endl
	   << "const unsigned int GRIB2::nr_paramcategory = " << m_nrparamcategory << ';' << std::endl
	   << "const unsigned int GRIB2::nr_parameters = " << m_nrparameter << ';' << std::endl
	   << "const unsigned int GRIB2::nr_paramdisciplineindex = " << m_discindex.size() << ';' << std::endl
	   << "const unsigned int GRIB2::nr_paramcategoryindex = " << m_catindex.size() << ';' << std::endl
	   << "const unsigned int GRIB2::nr_parameternameindex = " << m_parindex.size() << ';' << std::endl
	   << "const unsigned int GRIB2::nr_parameterabbrevindex = " << m_abbrevindex.size() << ';' << std::endl;
	return os;
};

std::ostream& operator<<(std::ostream& os, const Gen& gen)
{
        return gen.print(os);
}

int main(int argc, char *argv[])
{
	Gen g;
        std::cout << "// do not edit, automatically generated by gengrib2paramtbl.cc" << std::endl << std::endl
		  << "#include \"grib2.h\"" << std::endl << std::endl << g << std::endl;
        return 0;
}
