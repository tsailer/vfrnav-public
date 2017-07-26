/*****************************************************************************/

/*
 *      grib2tables.cc  --  Gridded Binary weather files, tables.
 *
 *      Copyright (C) 2012, 2013  Thomas Sailer (t.sailer@alumni.ethz.ch)
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
#include "baro.h"

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <set>

const char GRIB2::grib2_header[4] = { 'G', 'R', 'I', 'B' };
const uint32_t GRIB2::grib2_terminator;
const char GRIB2::charstr_terminator;

const struct GRIB2::ID8String GRIB2::genprocesstable[] = {
	{ 0, "Analysis" },
	{ 1, "Initialization" },
	{ 2, "Forecast" },
	{ 3, "Bias Corrected Forecast" },
	{ 4, "Ensemble Forecast" },
	{ 5, "Probability Forecast" },
	{ 6, "Forecast Error" },
	{ 7, "Analysis Error" },
	{ 8, "Observation" },
	{ 9, "Climatological" },
	{ 10, "Probability-Weighted Forecast" },
	{ 11, "Bias-Corrected Ensemble Forecast" },
	{ 192, "Forecast Confidence Indicator" },
	{ 255, "Missing" }
};

const unsigned int GRIB2::nr_genprocesstable = sizeof(genprocesstable)/sizeof(genprocesstable[0]);

const struct GRIB2::ID8String GRIB2::prodstatustable[] = {
	{ 0, "Operational Products" },
	{ 1, "Operational Test Products" },
	{ 2, "Research Products" },
	{ 3, "Re-Analysis Products" },
	{ 4, "THORPEX Interactive Grand Global Ensemble (TIGGE)" },
	{ 5, "THORPEX Interactive Grand Global Ensemble (TIGGE) test" },
	{ 255, "Missing" }
};

const unsigned int GRIB2::nr_prodstatustable = sizeof(prodstatustable)/sizeof(prodstatustable[0]);

const struct GRIB2::ID8String GRIB2::typeofdatatable[] = {
	{ 0, "Analysis Products" },
	{ 1, "Forecast Products" },
	{ 2, "Analysis and Forecast Products" },
	{ 3, "Control Forecast Products" },
	{ 4, "Perturbed Forecast Products" },
	{ 5, "Control and Perturbed Forecast Products" },
	{ 6, "Processed Satellite Observations" },
	{ 7, "Processed Radar Observations" },
	{ 8, "Event Probability" },
	{ 255, "Missing" }
};

const unsigned int GRIB2::nr_typeofdatatable = sizeof(typeofdatatable)/sizeof(typeofdatatable[0]);

const struct GRIB2::SurfaceTable GRIB2::surfacetable[] = {
	{ 1, "Ground or Water Surface", 0 },
	{ 2, "Cloud Base Level", 0 },
	{ 3, "Level of Cloud Tops", 0 },
	{ 4, "Level of 0degC Isotherm", 0 },
	{ 5, "Level of Adiabatic Condensation Lifted from the Surface", 0 },
	{ 6, "Maximum Wind Level", 0 },
	{ 7, "Tropopause", 0 },
	{ 8, "Nominal Top of the Atmosphere", 0 },
	{ 9, "Sea Bottom", 0 },
	{ 10, "Entire Atmosphere", 0 },
	{ 11, "Cumulonimbus Base (CB)", "m" },
	{ 12, "Cumulonimbus Top (CT)", "m" },
	{ 20, "Isothermal Level", "K" },
	{ 100, "Isobaric Surface", "Pa" },
	{ 101, "Mean Sea Level", 0 },
	{ 102, "Specific Altitude Above Mean Sea Level", "m" },
	{ 103, "Specified Height Level Above Ground", "m" },
	{ 104, "Sigma Level", 0 },
	{ 105, "Hybrid Level", 0 },
	{ 106, "Depth Below Land Surface", "m" },
	{ 107, "Isentropic (theta) Level", "K" },
	{ 108, "Level at Specified Pressure Difference from Ground to Level", "Pa" },
	{ 109, "Potential Vorticity Surface", "K m^2 kg^-1 s^-1" },
	{ 111, "Eta Level", 0 },
	{ 113, "Logarithmic Hybrid Coordinate", 0 },
	{ 117, "Mixed Layer Depth", "m" },
	{ 118, "Hybrid Height Level", 0 },
	{ 119, "Hybrid Pressure Level", 0 },
	{ 120, "Pressure Thickness", "Pa" },
	{ 150, "Generalized Vertical Height Coordinate", 0 },
	{ 160, "Depth Below Sea Level", "m" },
	{ 161, "Depth Below Water Surface", "m" },
	{ 162, "Lake or River Bottom", 0 },
	{ 163, "Bottom Of Sediment Layer", 0 },
	{ 164, "Bottom Of Thermally Active Sediment Layer", 0 },
	{ 165, "Bottom Of Sediment Layer Penetrated By Thermal Wave", 0 },
	{ 166, "Maxing Layer", 0 },
	{ 170, "Ionospheric D-region Level", 0 },
	{ 171, "Ionospheric E-region Level", 0 },
	{ 172, "Ionospheric F1-region Level", 0 },
	{ 173, "Ionospheric F2-region Level", 0 },
	{ 200, "Entire atmosphere (considered as a single layer)", 0 },
	{ 201, "Entire ocean (considered as a single layer)", 0 },
	{ 204, "Highest tropospheric freezing level", 0 },
	{ 206, "Grid scale cloud bottom level", 0 },
	{ 207, "Grid scale cloud top level", 0 },
	{ 209, "Boundary layer cloud bottom level", 0 },
	{ 210, "Boundary layer cloud top level", 0 },
	{ 211, "Boundary layer cloud layer", 0 },
	{ 212, "Low cloud bottom level", 0 },
	{ 213, "Low cloud top level", 0 },
	{ 214, "Low cloud layer", 0 },
	{ 215, "Cloud ceiling", 0 },
	{ 220, "Planetary Boundary Layer", 0 },
	{ 221, "Layer Between Two Hybrid Levels", 0 },
	{ 222, "Middle cloud bottom level", 0 },
	{ 223, "Middle cloud top level", 0 },
	{ 224, "Middle cloud layer", 0 },
	{ 232, "High cloud bottom level", 0 },
	{ 233, "High cloud top level", 0 },
	{ 234, "High cloud layer", 0 },
	{ 235, "Ocean Isotherm Level", "1/10degC" },
	{ 236, "Layer between two depths below ocean surface", 0 },
	{ 237, "Bottom of Ocean Mixed Layer", "m" },
	{ 238, "Bottom of Ocean Isothermal Layer", "m" },
	{ 239, "Layer Ocean Surface and 26C Ocean Isothermal Level", 0 },
	{ 240, "Ocean Mixed Layer", 0 },
	{ 241, "Ordered Sequence of Data", 0 },
	{ 242, "Convective cloud bottom level", 0 },
	{ 243, "Convective cloud top level", 0 },
	{ 244, "Convective cloud layer", 0 },
	{ 245, "Lowest level of the wet bulb zero", 0 },
	{ 246, "Maximum equivalent potential temperature level", 0 },
	{ 247, "Equilibrium level", 0 },
	{ 248, "Shallow convective cloud bottom level", 0 },
	{ 249, "Shallow convective cloud top level", 0 },
	{ 251, "Deep convective cloud bottom level", 0 },
	{ 252, "Deep convective cloud top level", 0 },
	{ 253, "Lowest bottom level of supercooled liquid water layer", 0 },
	{ 254, "Highest top level of supercooled liquid water layer", 0 },
	{ 255, "Missing", 0 },
	{ 0, 0, 0 }
};

const unsigned int GRIB2::nr_surfacetable = sizeof(surfacetable)/sizeof(surfacetable[0]);

const struct GRIB2::ID16String GRIB2::subcentertable_7[] = {
	{ 1, "NCEP Re-Analysis Project" },
	{ 2, "NCEP Ensemble Products" },
	{ 3, "NCEP Central Operations" },
	{ 4, "Environmental Modeling Center" },
	{ 5, "Hydrometeorological Prediction Center" },
	{ 6, "Marine Prediction Center" },
	{ 7, "Climate Prediction Center" },
	{ 8, "Aviation Weather Center" },
	{ 9, "Storm Prediction Center" },
	{ 10, "National Hurricane Prediction Center" },
	{ 11, "NWS Techniques Development Laboratory" },
	{ 12, "NESDIS Office of Research and Applications" },
	{ 13, "Federal Aviation Administration" },
	{ 14, "NWS Meteorological Development Laboratory" },
	{ 15, "North American Regional Reanalysis Project" },
	{ 16, "Space Weather Prediction Center" }
};

const struct GRIB2::ID8String GRIB2::genprocesstype_7[] = {
	{ 02, "Ultra Violet Index Model" },
	{ 03, "NCEP/ARL Transport and Dispersion Model" },
	{ 04, "NCEP/ARL Smoke Model" },
	{ 05, "Satellite Derived Precipitation and temperatures, from IR (See PDS Octet 41... for specific satellite ID)" },
	{ 06, "NCEP/ARL Dust Model" },
	{ 10, "Global Wind-Wave Forecast Model" },
	{ 11, "Global Multi-Grid Wave Model (Static Grids)" },
	{ 12, "Probabilistic Storm Surge" },
	{ 13, "Hurricane Multi-Grid Wave Model" },
	{ 14, "Extratropical Storm Surge Model" },
	{ 19, "Limited-area Fine Mesh (LFM) analysis" },
	{ 25, "Snow Cover Analysis" },
	{ 30, "Forecaster generated field" },
	{ 31, "Value added post processed field" },
	{ 39, "Nested Grid forecast Model (NGM)" },
	{ 42, "Global Optimum Interpolation Analysis (GOI) from GFS model" },
	{ 43, "Global Optimum Interpolation Analysis (GOI) from \"Final\" run" },
	{ 44, "Sea Surface Temperature Analysis" },
	{ 45, "Coastal Ocean Circulation Model" },
	{ 46, "HYCOM - Global" },
	{ 47, "HYCOM - North Pacific basin" },
	{ 48, "HYCOM - North Atlantic basin" },
	{ 49, "Ozone Analysis from TIROS Observations" },
	{ 52, "Ozone Analysis from Nimbus 7 Observations" },
	{ 53, "LFM-Fourth Order Forecast Model" },
	{ 64, "Regional Optimum Interpolation Analysis (ROI)" },
	{ 68, "80 wave triangular, 18-layer Spectral model from GFS model" },
	{ 69, "80 wave triangular, 18 layer Spectral model from \"Medium Range Forecast\" run" },
	{ 70, "Quasi-Lagrangian Hurricane Model (QLM)" },
	{ 73, "Fog Forecast model - Ocean Prod. Center" },
	{ 74, "Gulf of Mexico Wind/Wave" },
	{ 75, "Gulf of Alaska Wind/Wave" },
	{ 76, "Bias corrected Medium Range Forecast" },
	{ 77, "126 wave triangular, 28 layer Spectral model from GFS model" },
	{ 78, "126 wave triangular, 28 layer Spectral model from \"Medium Range Forecast\" run" },
	{ 79, "Backup from the previous run" },
	{ 80, "62 wave triangular, 28 layer Spectral model from \"Medium Range Forecast\" run" },
	{ 81, "Analysis from GFS (Global Forecast System)" },
	{ 82, "Analysis from GDAS (Global Data Assimilation System)" },
	{ 84, "MESO NAM Model (currently 12 km)" },
	{ 85, "Real Time Ocean Forecast System (RTOFS)" },
	{ 86, "RUC Model, from Forecast Systems Lab (isentropic; scale: 60km at 40N)" },
	{ 87, "CAC Ensemble Forecasts from Spectral (ENSMB)" },
	{ 88, "NOAA Wave Watch III (NWW3) Ocean Wave Model" },
	{ 89, "Non-hydrostatic Meso Model (NMM) (Currently 8 km)" },
	{ 90, "62 wave triangular, 28 layer spectral model extension of the \"Medium Range Forecast\" run" },
	{ 91, "62 wave triangular, 28 layer spectral model extension of the GFS model" },
	{ 92, "62 wave triangular, 28 layer spectral model run from the \"Medium Range Forecast\" final analysis" },
	{ 93, "62 wave triangular, 28 layer spectral model run from the T62 GDAS analysis of the \"Medium Range Forecast\" run" },
	{ 94, "T170/L42 Global Spectral Model from MRF run" },
	{ 95, "T126/L42 Global Spectral Model from MRF run" },
	{ 96, "Global Forecast System Model, T574 - Forecast hours 00-192, T190 - Forecast hours 204 - 384" },
	{ 98, "Climate Forecast System Model -- Atmospheric model (GFS) coupled to a multi level ocean model. Currently GFS spectral model at T62, 64 levels coupled to 40 level MOM3 ocean model." },
	{ 99, "Miscellaneous Test ID" },
	{ 100, "RUC Surface Analysis (scale: 60km at 40N)" },
	{ 101, "RUC Surface Analysis (scale: 40km at 40N)" },
	{ 105, "RUC Model from FSL (isentropic; scale: 20km at 40N)" },
	{ 107, "Global Ensemble Forecast System (GEFS)" },
	{ 108, "LAMP" },
	{ 109, "RTMA (Real Time Mesoscale Analysis)" },
	{ 110, "NAM Model - 15km version" },
	{ 111, "NAM model, generic resolution (Used in SREF processing)" },
	{ 112, "WRF-NMM model, generic resolution (Used in various runs) NMM=Nondydrostatic Mesoscale Model (NCEP)" },
	{ 113, "Products from NCEP SREF processing" },
	{ 114, "NAEFS Products from joined NCEP, CMC global ensembles" },
	{ 115, "Downscaled GFS from NAM eXtension" },
	{ 116, "WRF-EM model, generic resolution (Used in various runs) EM - Eulerian Mass-core (NCAR - aka Advanced Research WRF)" },
	{ 117, "NEMS GFS Aerosol Component" },
	{ 120, "Ice Concentration Analysis" },
	{ 121, "Western North Atlantic Regional Wave Model" },
	{ 122, "Alaska Waters Regional Wave Model" },
	{ 123, "North Atlantic Hurricane Wave Model" },
	{ 124, "Eastern North Pacific Regional Wave Model" },
	{ 125, "North Pacific Hurricane Wave Model" },
	{ 126, "Sea Ice Forecast Model" },
	{ 127, "Lake Ice Forecast Model" },
	{ 128, "Global Ocean Forecast Model" },
	{ 129, "Global Ocean Data Analysis System (GODAS)" },
	{ 130, "Merge of fields from the RUC, NAM, and Spectral Model" },
	{ 131, "Great Lakes Wave Model" },
	{ 140, "North American Regional Reanalysis (NARR)" },
	{ 141, "Land Data Assimilation and Forecast System" },
	{ 150, "NWS River Forecast System (NWSRFS)" },
	{ 151, "NWS Flash Flood Guidance System (NWSFFGS)" },
	{ 152, "WSR-88D Stage II Precipitation Analysis" },
	{ 153, "WSR-88D Stage III Precipitation Analysis" },
	{ 180, "Quantitative Precipitation Forecast generated by NCEP" },
	{ 181, "River Forecast Center Quantitative Precipitation Forecast mosaic generated by NCEP" },
	{ 182, "River Forecast Center Quantitative Precipitation estimate mosaic generated by NCEP" },
	{ 183, "NDFD product generated by NCEP/HPC" },
	{ 184, "Climatological Calibrated Precipitation Analysis - CCPA" },
	{ 190, "National Convective Weather Diagnostic generated by NCEP/AWC" },
	{ 191, "Current Icing Potential automated product genterated by NCEP/AWC" },
	{ 192, "Analysis product from NCEP/AWC" },
	{ 193, "Forecast product from NCEP/AWC" },
	{ 195, "Climate Data Assimilation System 2 (CDAS2)" },
	{ 196, "Climate Data Assimilation System 2 (CDAS2) - used for regeneration runs" },
	{ 197, "Climate Data Assimilation System (CDAS)" },
	{ 198, "Climate Data Assimilation System (CDAS) - used for regeneration runs" },
	{ 199, "Climate Forecast System Reanalysis (CFSR) -- Atmospheric model (GFS) coupled to a multi level ocean, land and seaice model. Currently GFS spectral model at T382, 64 levels coupled to 40 level MOM4 ocean model." },
	{ 200, "CPC Manual Forecast Product" },
	{ 201, "CPC Automated Product" },
	{ 210, "EPA Air Quality Forecast - Currently North East US domain" },
	{ 211, "EPA Air Quality Forecast - Currently Eastern US domain" },
	{ 215, "SPC Manual Forecast Product" },
	{ 220, "NCEP/OPC automated product" },
	{ 255, "Missing" },
};

const struct GRIB2::CenterTable GRIB2::centertable[] = {
	{ 1, "Melbourne (WMC)" },
	{ 2, "Melbourne (WMC)" },
	{ 3, "Melbourne (WMC)" },
	{ 4, "Moscow (WMC)" },
	{ 5, "Moscow (WMC)" },
	{ 6, "Moscow (WMC)" },
	{ 7, "US National Weather Service - NCEP (WMC)",
	  subcentertable_7, sizeof(subcentertable_7)/sizeof(subcentertable_7[0]),
	  genprocesstype_7, sizeof(genprocesstype_7)/sizeof(genprocesstype_7[0]) },
	{ 8, "US National Weather Service - NWSTG (WMC)" },
	{ 9, "US National Weather Service - Other (WMC)" },
	{ 10, "Cairo (RSMC/RAFC)" },
	{ 11, "Cairo (RSMC/RAFC)" },
	{ 12, "Dakar (RSMC/RAFC)" },
	{ 13, "Dakar (RSMC/RAFC)" },
	{ 14, "Nairobi (RSMC/RAFC)" },
	{ 15, "Nairobi (RSMC/RAFC)" },
	{ 16, "Casablanca (RSMC)" },
	{ 17, "Tunis (RSMC)" },
	{ 18, "Tunis-Casablanca (RSMC)" },
	{ 19, "Tunis-Casablanca (RSMC)" },
	{ 20, "Las Palmas (RAFC)" },
	{ 21, "Algiers (RSMC)" },
	{ 22, "ACMAD" },
	{ 23, "Mozambique (NMC)" },
	{ 24, "Pretoria (RSMC)" },
	{ 25, "La Reunion (RSMC)" },
	{ 26, "Khabarovsk (RSMC)" },
	{ 27, "Khabarovsk (RSMC)" },
	{ 28, "New Delhi (RSMC/RAFC)" },
	{ 29, "New Delhi (RSMC/RAFC)" },
	{ 30, "Novosibirsk (RSMC)" },
	{ 31, "Novosibirsk (RSMC)" },
	{ 32, "Tashkent (RSMC)" },
	{ 33, "Jeddah (RSMC)" },
	{ 34, "Tokyo (RSMC), Japanese Meteorological Agency" },
	{ 35, "Tokyo (RSMC), Japanese Meteorological Agency" },
	{ 36, "Bankok" },
	{ 37, "Ulan Bator" },
	{ 38, "Beijing (RSMC)" },
	{ 39, "Beijing (RSMC)" },
	{ 40, "Seoul" },
	{ 41, "Buenos Aires (RSMC/RAFC)" },
	{ 42, "Buenos Aires (RSMC/RAFC)" },
	{ 43, "Brasilia (RSMC/RAFC)" },
	{ 44, "Brasilia (RSMC/RAFC)" },
	{ 45, "Santiago" },
	{ 46, "Brazilian Space Agency - INPE" },
	{ 47, "Columbia (NMC)" },
	{ 48, "Ecuador (NMC)" },
	{ 49, "Peru (NMC)" },
	{ 50, "Venezuela (NMC)" },
	{ 51, "Miami (RSMC/RAFC)" },
	{ 52, "Miami (RSMC), National Hurricane Center" },
	{ 53, "Canadian Meteorological Service - Montreal (RSMC)" },
	{ 54, "Canadian Meteorological Service - Montreal (RSMC)" },
	{ 55, "San Francisco" },
	{ 56, "ARINC Center" },
	{ 57, "US Air Force - Air Force Global Weather Center" },
	{ 58, "Fleet Numerical Meteorology and Oceanography Center,Monterey,CA,USA" },
	{ 59, "The NOAA Forecast Systems Lab, Boulder, CO, USA" },
	{ 60, "National Center for Atmospheric Research (NCAR), Boulder, CO" },
	{ 61, "Service ARGOS - Landover, MD, USA" },
	{ 62, "US Naval Oceanographic Office" },
	{ 63, "International Research Institude for Climate and Society" },
	{ 64, "Honolulu" },
	{ 65, "Darwin (RSMC)" },
	{ 66, "Darwin (RSMC)" },
	{ 67, "Melbourne (RSMC)" },
	{ 69, "Wellington (RSMC/RAFC)" },
	{ 70, "Wellington (RSMC/RAFC)" },
	{ 71, "Nadi (RSMC)" },
	{ 72, "Singapore" },
	{ 73, "Malaysia (NMC)" },
	{ 74, "U.K. Met Office - Exeter (RSMC)" },
	{ 75, "U.K. Met Office - Exeter (RSMC)" },
	{ 76, "Moscow (RSMC/RAFC)" },
	{ 78, "Offenbach (RSMC)" },
	{ 79, "Offenbach (RSMC)" },
	{ 80, "Rome (RSMC)" },
	{ 81, "Rome (RSMC)" },
	{ 82, "Norrkoping" },
	{ 83, "Norrkoping" },
	{ 84, "French Weather Service - Toulouse" },
	{ 85, "French Weather Service - Toulouse" },
	{ 86, "Helsinki" },
	{ 87, "Belgrade" },
	{ 88, "Oslo" },
	{ 89, "Prague" },
	{ 90, "Episkopi" },
	{ 91, "Ankara" },
	{ 92, "Frankfurt/Main (RAFC)" },
	{ 93, "London (WAFC)" },
	{ 94, "Copenhagen" },
	{ 95, "Rota" },
	{ 96, "Athens" },
	{ 97, "European Space Agency (ESA)" },
	{ 98, "European Center for Medium-Range Weather Forecasts (RSMC)" },
	{ 99, "De Bilt, Netherlands" },
	{ 100, "Brazzaville" },
	{ 101, "Abidjan" },
	{ 102, "Libyan Arab Jamahiriya (NMC)" },
	{ 103, "Madagascar (NMC)" },
	{ 104, "Mauritius (NMC)" },
	{ 105, "Niger (NMC)" },
	{ 106, "Seychelles (NMC)" },
	{ 107, "Uganda (NMC)" },
	{ 108, "United Republic of Tanzania (NMC)" },
	{ 109, "Zimbabwe (NMC)" },
	{ 110, "Hong-Kong" },
	{ 111, "Afghanistan (NMC)" },
	{ 112, "Bahrain (NMC)" },
	{ 113, "Bangladesh (NMC)" },
	{ 114, "Bhutan (NMC)" },
	{ 115, "Cambodia (NMC)" },
	{ 116, "Democratic People's Republic of Korea (NMC)" },
	{ 117, "Islamic Republic of Iran (NMC)" },
	{ 118, "Iraq (NMC)" },
	{ 119, "Kazakhstan (NMC)" },
	{ 120, "Kuwait (NMC)" },
	{ 121, "Kyrgyz Republic (NMC)" },
	{ 122, "Lao People's Democratic Republic (NMC)" },
	{ 123, "Macao, China" },
	{ 124, "Maldives (NMC)" },
	{ 125, "Myanmar (NMC)" },
	{ 126, "Nepal (NMC)" },
	{ 127, "Oman (NMC)" },
	{ 128, "Pakistan (NMC)" },
	{ 129, "Qatar (NMC)" },
	{ 130, "Yemen (NMC)" },
	{ 131, "Sri Lanka (NMC)" },
	{ 132, "Tajikistan (NMC)" },
	{ 133, "Turkmenistan (NMC)" },
	{ 134, "United Arab Emirates (NMC)" },
	{ 135, "Uzbekistan (NMC)" },
	{ 136, "Viet Nam (NMC)" },
	{ 140, "Bolivia (NMC)" },
	{ 141, "Guyana (NMC)" },
	{ 142, "Paraguay (NMC)" },
	{ 143, "Suriname (NMC)" },
	{ 144, "Uruguay (NMC)" },
	{ 145, "French Guyana" },
	{ 146, "Brazilian Navy Hydrographic Center" },
	{ 147, "National Commission on Space Activities - Argentina" },
	{ 150, "Antigua and Barbuda (NMC)" },
	{ 151, "Bahamas (NMC)" },
	{ 152, "Barbados (NMC)" },
	{ 153, "Belize (NMC)" },
	{ 154, "British Caribbean Territories Center" },
	{ 155, "San Jose" },
	{ 156, "Cuba (NMC)" },
	{ 157, "Dominica (NMC)" },
	{ 158, "Dominican Republic (NMC)" },
	{ 159, "El Salvador (NMC)" },
	{ 160, "US NOAA/NESDIS" },
	{ 161, "US NOAA Office of Oceanic and Atmospheric Research" },
	{ 162, "Guatemala (NMC)" },
	{ 163, "Haiti (NMC)" },
	{ 164, "Honduras (NMC)" },
	{ 165, "Jamaica (NMC)" },
	{ 166, "Mexico City" },
	{ 167, "Netherlands Antilles and Aruba (NMC)" },
	{ 168, "Nicaragua (NMC)" },
	{ 169, "Panama (NMC)" },
	{ 170, "Saint Lucia (NMC)" },
	{ 171, "Trinidad and Tobago (NMC)" },
	{ 172, "French Departments in RA IV" },
	{ 173, "US National Aeronautics and Space Administration (NASA)" },
	{ 174, "Integrated System Data Management/Marine Environmental Data Service (ISDM/MEDS) - Canada" },
	{ 176, "US Cooperative Institude for Meteorological Satellite Studies" },
	{ 190, "Cook Islands (NMC)" },
	{ 191, "French Polynesia (NMC)" },
	{ 192, "Tonga (NMC)" },
	{ 193, "Vanuatu (NMC)" },
	{ 194, "Brunei (NMC)" },
	{ 195, "Indonesia (NMC)" },
	{ 196, "Kiribati (NMC)" },
	{ 197, "Federated States of Micronesia (NMC)" },
	{ 198, "New Caledonia (NMC)" },
	{ 199, "Niue" },
	{ 200, "Papua New Guinea (NMC)" },
	{ 201, "Philippines (NMC)" },
	{ 202, "Samoa (NMC)" },
	{ 203, "Solomon Islands (NMC)" },
	{ 204, "National Institude of Water and Atmospheric Research - New Zealand" },
	{ 210, "Frascati (ESA/ESRIN)" },
	{ 211, "Lanion" },
	{ 212, "Lisbon" },
	{ 213, "Reykjavik" },
	{ 214, "Madrid" },
	{ 215, "Zurich" },
	{ 216, "Service ARGOS - Toulouse" },
	{ 217, "Bratislava" },
	{ 218, "Budapest" },
	{ 219, "Ljubljana" },
	{ 220, "Warsaw" },
	{ 221, "Zagreb" },
	{ 222, "Albania (NMC)" },
	{ 223, "Armenia (NMC)" },
	{ 224, "Austria (NMC)" },
	{ 225, "Azerbaijan (NMC)" },
	{ 226, "Belarus (NMC)" },
	{ 227, "Belgium (NMC)" },
	{ 228, "Bosnia and Herzegovina (NMC)" },
	{ 229, "Bulgaria (NMC)" },
	{ 230, "Cyprus (NMC)" },
	{ 231, "Estonia (NMC)" },
	{ 232, "Georgia (NMC)" },
	{ 233, "Dublin" },
	{ 234, "Israel (NMC)" },
	{ 235, "Jordan (NMC)" },
	{ 236, "Latvia (NMC)" },
	{ 237, "Lebanon (NMC)" },
	{ 238, "Lithuania (NMC)" },
	{ 239, "Luxembourg" },
	{ 240, "Malta (NMC)" },
	{ 241, "Monaco" },
	{ 242, "Romania (NMC)" },
	{ 243, "Syrian Arab Republic (NMC)" },
	{ 244, "The former Yugoslav Republic of Macedonia (NMC)" },
	{ 245, "Ukraine (NMC)" },
	{ 246, "Republic of Moldova (NMC)" },
	{ 247, "Operational Programme for the Exchange of Weather RAdar Information (OPERA) - EUMETNET" },
	{ 250, "COnsortium for Small scale MOdelling (COSMO)" },
	{ 254, "EUMETSAT Operations Center" },
	{ 255, "Missing Value" }
};

const unsigned int GRIB2::nr_centertable = sizeof(centertable)/sizeof(centertable[0]);
