/*****************************************************************************/

/*
 *      grib2.cc  --  Gridded Binary weather files.
 *
 *      Copyright (C) 2012, 2013, 2014, 2015, 2016, 2017, 2018  Thomas Sailer (t.sailer@alumni.ethz.ch)
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
#include "fplan.h"
#include "SunriseSunset.h"
#include "metgraph.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <set>
#include <giomm.h>
#ifdef HAVE_OPENJPEG
#include <openjpeg.h>
#endif
#ifdef HAVE_EIGEN3
#include <Eigen/Dense>
#endif
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

const uint8_t GRIB2::surface_ground_or_water;
const uint8_t GRIB2::surface_cloud_base;
const uint8_t GRIB2::surface_cloud_top;
const uint8_t GRIB2::surface_0degc_isotherm;
const uint8_t GRIB2::surface_lifted_adiabatic_condensation;
const uint8_t GRIB2::surface_maximum_wind;
const uint8_t GRIB2::surface_tropopause;
const uint8_t GRIB2::surface_nominal_atmosphere_top;
const uint8_t GRIB2::surface_sea_bottom;
const uint8_t GRIB2::surface_entire_atmosphere;
const uint8_t GRIB2::surface_cumulonimbus_base;
const uint8_t GRIB2::surface_cumulonimbus_top;
const uint8_t GRIB2::surface_isothermal_level;
const uint8_t GRIB2::surface_isobaric_surface;
const uint8_t GRIB2::surface_mean_sea_level;
const uint8_t GRIB2::surface_specific_altitude_amsl;
const uint8_t GRIB2::surface_specific_height_gnd;
const uint8_t GRIB2::surface_sigma_level;
const uint8_t GRIB2::surface_hybrid_level;
const uint8_t GRIB2::surface_depth_below_land_surface;
const uint8_t GRIB2::surface_isentropic_level;
const uint8_t GRIB2::surface_level_specific_pressure_difference;
const uint8_t GRIB2::surface_potential_vorticity_surface;
const uint8_t GRIB2::surface_eta_level;
const uint8_t GRIB2::surface_logarithmic_hybrid_coordinate;
const uint8_t GRIB2::surface_mixed_layer_depth;
const uint8_t GRIB2::surface_hybrid_height_level;
const uint8_t GRIB2::surface_hybrid_pressure_level;
const uint8_t GRIB2::surface_pressure_thickness;
const uint8_t GRIB2::surface_generalized_vertical_height_coordinate;
const uint8_t GRIB2::surface_depth_below_sealevel;
const uint8_t GRIB2::surface_depth_below_water_surface;
const uint8_t GRIB2::surface_lake_bottom;
const uint8_t GRIB2::surface_sediment_bottom;
const uint8_t GRIB2::surface_thermally_active_sediment_bottom;
const uint8_t GRIB2::surface_thermal_wave_penetrated_sediment_bottom;
const uint8_t GRIB2::surface_maxing_layer;
const uint8_t GRIB2::surface_ionospheric_d_level;
const uint8_t GRIB2::surface_ionospheric_e_level;
const uint8_t GRIB2::surface_ionospheric_f1_level;
const uint8_t GRIB2::surface_ionospheric_f2_level;
const uint8_t GRIB2::surface_entire_atmosphere_as_single_layer;
const uint8_t GRIB2::surface_entire_ocean_as_single_layer;
const uint8_t GRIB2::surface_highest_tropospheric_freezing_level;
const uint8_t GRIB2::surface_grid_scale_cloud_bottom;
const uint8_t GRIB2::surface_grid_scale_cloud_top;
const uint8_t GRIB2::surface_boundary_layer_cloud_bottom;
const uint8_t GRIB2::surface_boundary_layer_cloud_top;
const uint8_t GRIB2::surface_boundary_layer_cloud;
const uint8_t GRIB2::surface_low_cloud_bottom;
const uint8_t GRIB2::surface_low_cloud_top;
const uint8_t GRIB2::surface_low_cloud;
const uint8_t GRIB2::surface_cloud_ceiling;
const uint8_t GRIB2::surface_planetary_boundary_layer;
const uint8_t GRIB2::surface_layer_between_hybrid_levels;
const uint8_t GRIB2::surface_middle_cloud_bottom;
const uint8_t GRIB2::surface_middle_cloud_top;
const uint8_t GRIB2::surface_middle_cloud;
const uint8_t GRIB2::surface_top_cloud_bottom;
const uint8_t GRIB2::surface_top_cloud_top;
const uint8_t GRIB2::surface_top_cloud;
const uint8_t GRIB2::surface_ocean_isotherm_level;
const uint8_t GRIB2::surface_layer_between_depths_below_ocean_surface;
const uint8_t GRIB2::surface_ocean_mixing_layer_bottom;
const uint8_t GRIB2::surface_ocean_isothermal_layer_bottom;
const uint8_t GRIB2::surface_ocean_surface_26c_isothermal;
const uint8_t GRIB2::surface_ocean_mixing_layer;
const uint8_t GRIB2::surface_ordered_sequence_of_data;
const uint8_t GRIB2::surface_convective_cloud_bottom;
const uint8_t GRIB2::surface_convective_cloud_top;
const uint8_t GRIB2::surface_convective_cloud;
const uint8_t GRIB2::surface_lowest_wet_bulb_zero;
const uint8_t GRIB2::surface_maximum_equivalent_potential_temperature;
const uint8_t GRIB2::surface_equilibrium_level;
const uint8_t GRIB2::surface_shallow_convective_cloud_bottom;
const uint8_t GRIB2::surface_shallow_convective_cloud_top;
const uint8_t GRIB2::surface_shallow_convective_cloud;
const uint8_t GRIB2::surface_deep_convective_cloud_top;
const uint8_t GRIB2::surface_supercooled_liquid_water_bottom;
const uint8_t GRIB2::surface_supercooled_liquid_water_top;
const uint8_t GRIB2::surface_missing;

const uint8_t GRIB2::discipline_meteorology;
const uint8_t GRIB2::discipline_hydrology;
const uint8_t GRIB2::discipline_landsurface;
const uint8_t GRIB2::discipline_space;
const uint8_t GRIB2::discipline_spaceweather;
const uint8_t GRIB2::discipline_oceanography;

const uint16_t GRIB2::paramcat_meteorology_temperature;
const uint16_t GRIB2::paramcat_meteorology_moisture;
const uint16_t GRIB2::paramcat_meteorology_momentum;
const uint16_t GRIB2::paramcat_meteorology_mass;
const uint16_t GRIB2::paramcat_meteorology_shortwave_radiation;
const uint16_t GRIB2::paramcat_meteorology_longwave_radiation;
const uint16_t GRIB2::paramcat_meteorology_cloud;
const uint16_t GRIB2::paramcat_meteorology_thermodynamic_stability;
const uint16_t GRIB2::paramcat_meteorology_kinematic_stability;
const uint16_t GRIB2::paramcat_meteorology_temperature_prob;
const uint16_t GRIB2::paramcat_meteorology_moisture_prob;
const uint16_t GRIB2::paramcat_meteorology_momentum_prob;
const uint16_t GRIB2::paramcat_meteorology_mass_probabilities;
const uint16_t GRIB2::paramcat_meteorology_aerosols;
const uint16_t GRIB2::paramcat_meteorology_trace_gases;
const uint16_t GRIB2::paramcat_meteorology_radar;
const uint16_t GRIB2::paramcat_meteorology_forecast_radar;
const uint16_t GRIB2::paramcat_meteorology_electrodynamics;
const uint16_t GRIB2::paramcat_meteorology_nuclear;
const uint16_t GRIB2::paramcat_meteorology_atmosphere_physics;
const uint16_t GRIB2::paramcat_meteorology_atmosphere_chemistry;
const uint16_t GRIB2::paramcat_meteorology_string;
const uint16_t GRIB2::paramcat_meteorology_miscellaneous;
const uint16_t GRIB2::paramcat_meteorology_covariance;
const uint16_t GRIB2::paramcat_meteorology_missing;
const uint16_t GRIB2::paramcat_hydrology_basic;
const uint16_t GRIB2::paramcat_hydrology_prob;
const uint16_t GRIB2::paramcat_hydrology_inlandwater_sediment;
const uint16_t GRIB2::paramcat_hydrology_missing;
const uint16_t GRIB2::paramcat_landsurface_vegetation;
const uint16_t GRIB2::paramcat_landsurface_agriaquaculture;
const uint16_t GRIB2::paramcat_landsurface_transportation;
const uint16_t GRIB2::paramcat_landsurface_soil;
const uint16_t GRIB2::paramcat_landsurface_fire;
const uint16_t GRIB2::paramcat_landsurface_missing;
const uint16_t GRIB2::paramcat_space_image;
const uint16_t GRIB2::paramcat_space_quantitative;
const uint16_t GRIB2::paramcat_space_forecast;
const uint16_t GRIB2::paramcat_space_missing;
const uint16_t GRIB2::paramcat_spaceweather_temperature;
const uint16_t GRIB2::paramcat_spaceweather_momentum;
const uint16_t GRIB2::paramcat_spaceweather_charged_particles;
const uint16_t GRIB2::paramcat_spaceweather_fields;
const uint16_t GRIB2::paramcat_spaceweather_energetic_particles;
const uint16_t GRIB2::paramcat_spaceweather_waves;
const uint16_t GRIB2::paramcat_spaceweather_solar;
const uint16_t GRIB2::paramcat_spaceweather_terrestrial;
const uint16_t GRIB2::paramcat_spaceweather_imagery;
const uint16_t GRIB2::paramcat_spaceweather_ionneutral_coupling;
const uint16_t GRIB2::paramcat_spaceweather_missing;
const uint16_t GRIB2::paramcat_oceanography_waves;
const uint16_t GRIB2::paramcat_oceanography_currents;
const uint16_t GRIB2::paramcat_oceanography_ice;
const uint16_t GRIB2::paramcat_oceanography_surface;
const uint16_t GRIB2::paramcat_oceanography_subsurface;
const uint16_t GRIB2::paramcat_oceanography_miscellaneous;
const uint16_t GRIB2::paramcat_oceanography_missing;

const uint32_t GRIB2::param_meteorology_temperature_tmp;
const uint32_t GRIB2::param_meteorology_temperature_vtmp;
const uint32_t GRIB2::param_meteorology_temperature_pot;
const uint32_t GRIB2::param_meteorology_temperature_epot;
const uint32_t GRIB2::param_meteorology_temperature_tmax;
const uint32_t GRIB2::param_meteorology_temperature_tmin;
const uint32_t GRIB2::param_meteorology_temperature_dpt;
const uint32_t GRIB2::param_meteorology_temperature_depr;
const uint32_t GRIB2::param_meteorology_temperature_lapr;
const uint32_t GRIB2::param_meteorology_temperature_tmpa;
const uint32_t GRIB2::param_meteorology_temperature_lhtfl;
const uint32_t GRIB2::param_meteorology_temperature_shtfl;
const uint32_t GRIB2::param_meteorology_temperature_heatx;
const uint32_t GRIB2::param_meteorology_temperature_wcf;
const uint32_t GRIB2::param_meteorology_temperature_mindpd;
const uint32_t GRIB2::param_meteorology_temperature_vptmp;
const uint32_t GRIB2::param_meteorology_temperature_snohf_2;
const uint32_t GRIB2::param_meteorology_temperature_skint;
const uint32_t GRIB2::param_meteorology_temperature_snot;
const uint32_t GRIB2::param_meteorology_temperature_ttcht;
const uint32_t GRIB2::param_meteorology_temperature_tdcht;
const uint32_t GRIB2::param_meteorology_temperature_snohf;
const uint32_t GRIB2::param_meteorology_temperature_ttrad;
const uint32_t GRIB2::param_meteorology_temperature_rev;
const uint32_t GRIB2::param_meteorology_temperature_lrghr;
const uint32_t GRIB2::param_meteorology_temperature_cnvhr;
const uint32_t GRIB2::param_meteorology_temperature_thflx;
const uint32_t GRIB2::param_meteorology_temperature_ttdia;
const uint32_t GRIB2::param_meteorology_temperature_ttphy;
const uint32_t GRIB2::param_meteorology_temperature_tsd1d;
const uint32_t GRIB2::param_meteorology_temperature_shahr;
const uint32_t GRIB2::param_meteorology_temperature_vdfhr;
const uint32_t GRIB2::param_meteorology_temperature_thz0;
const uint32_t GRIB2::param_meteorology_temperature_tchp;
const uint32_t GRIB2::param_meteorology_moisture_spfh;
const uint32_t GRIB2::param_meteorology_moisture_rh;
const uint32_t GRIB2::param_meteorology_moisture_mixr;
const uint32_t GRIB2::param_meteorology_moisture_pwat;
const uint32_t GRIB2::param_meteorology_moisture_vapp;
const uint32_t GRIB2::param_meteorology_moisture_satd;
const uint32_t GRIB2::param_meteorology_moisture_evp;
const uint32_t GRIB2::param_meteorology_moisture_prate;
const uint32_t GRIB2::param_meteorology_moisture_apcp;
const uint32_t GRIB2::param_meteorology_moisture_ncpcp;
const uint32_t GRIB2::param_meteorology_moisture_acpcp;
const uint32_t GRIB2::param_meteorology_moisture_snod;
const uint32_t GRIB2::param_meteorology_moisture_srweq;
const uint32_t GRIB2::param_meteorology_moisture_weasd;
const uint32_t GRIB2::param_meteorology_moisture_snoc;
const uint32_t GRIB2::param_meteorology_moisture_snol;
const uint32_t GRIB2::param_meteorology_moisture_snom;
const uint32_t GRIB2::param_meteorology_moisture_snoag;
const uint32_t GRIB2::param_meteorology_moisture_absh;
const uint32_t GRIB2::param_meteorology_moisture_ptype;
const uint32_t GRIB2::param_meteorology_moisture_iliqw;
const uint32_t GRIB2::param_meteorology_moisture_tcond;
const uint32_t GRIB2::param_meteorology_moisture_clwmr;
const uint32_t GRIB2::param_meteorology_moisture_icmr;
const uint32_t GRIB2::param_meteorology_moisture_rwmr;
const uint32_t GRIB2::param_meteorology_moisture_snmr;
const uint32_t GRIB2::param_meteorology_moisture_mconv;
const uint32_t GRIB2::param_meteorology_moisture_maxrh;
const uint32_t GRIB2::param_meteorology_moisture_maxah;
const uint32_t GRIB2::param_meteorology_moisture_asnow;
const uint32_t GRIB2::param_meteorology_moisture_pwcat;
const uint32_t GRIB2::param_meteorology_moisture_hail;
const uint32_t GRIB2::param_meteorology_moisture_grle;
const uint32_t GRIB2::param_meteorology_moisture_crain;
const uint32_t GRIB2::param_meteorology_moisture_cfrzr;
const uint32_t GRIB2::param_meteorology_moisture_cicep;
const uint32_t GRIB2::param_meteorology_moisture_csnow;
const uint32_t GRIB2::param_meteorology_moisture_cprat;
const uint32_t GRIB2::param_meteorology_moisture_mdiv;
const uint32_t GRIB2::param_meteorology_moisture_cpofp;
const uint32_t GRIB2::param_meteorology_moisture_pevap;
const uint32_t GRIB2::param_meteorology_moisture_pevpr;
const uint32_t GRIB2::param_meteorology_moisture_snowc;
const uint32_t GRIB2::param_meteorology_moisture_frain;
const uint32_t GRIB2::param_meteorology_moisture_rime;
const uint32_t GRIB2::param_meteorology_moisture_tcolr;
const uint32_t GRIB2::param_meteorology_moisture_tcols;
const uint32_t GRIB2::param_meteorology_moisture_lswp;
const uint32_t GRIB2::param_meteorology_moisture_cwp;
const uint32_t GRIB2::param_meteorology_moisture_twatp;
const uint32_t GRIB2::param_meteorology_moisture_tsnowp;
const uint32_t GRIB2::param_meteorology_moisture_tcwat;
const uint32_t GRIB2::param_meteorology_moisture_tprate;
const uint32_t GRIB2::param_meteorology_moisture_tsrwe;
const uint32_t GRIB2::param_meteorology_moisture_lsprate;
const uint32_t GRIB2::param_meteorology_moisture_csrwe;
const uint32_t GRIB2::param_meteorology_moisture_lssrwe;
const uint32_t GRIB2::param_meteorology_moisture_tsrate;
const uint32_t GRIB2::param_meteorology_moisture_csrate;
const uint32_t GRIB2::param_meteorology_moisture_lssrate;
const uint32_t GRIB2::param_meteorology_moisture_sdwe;
const uint32_t GRIB2::param_meteorology_moisture_sden;
const uint32_t GRIB2::param_meteorology_moisture_sevap;
const uint32_t GRIB2::param_meteorology_moisture_tciwv;
const uint32_t GRIB2::param_meteorology_moisture_rprate;
const uint32_t GRIB2::param_meteorology_moisture_sprate;
const uint32_t GRIB2::param_meteorology_moisture_fprate;
const uint32_t GRIB2::param_meteorology_moisture_iprate;
const uint32_t GRIB2::param_meteorology_moisture_tcolw;
const uint32_t GRIB2::param_meteorology_moisture_tcoli;
const uint32_t GRIB2::param_meteorology_moisture_hailmxr;
const uint32_t GRIB2::param_meteorology_moisture_tcolh;
const uint32_t GRIB2::param_meteorology_moisture_hailpr;
const uint32_t GRIB2::param_meteorology_moisture_tcolg;
const uint32_t GRIB2::param_meteorology_moisture_gprate;
const uint32_t GRIB2::param_meteorology_moisture_crrate;
const uint32_t GRIB2::param_meteorology_moisture_lsrrate;
const uint32_t GRIB2::param_meteorology_moisture_tcolwa;
const uint32_t GRIB2::param_meteorology_moisture_evarate;
const uint32_t GRIB2::param_meteorology_moisture_totcon;
const uint32_t GRIB2::param_meteorology_moisture_tcicon;
const uint32_t GRIB2::param_meteorology_moisture_cimixr;
const uint32_t GRIB2::param_meteorology_moisture_scllwc;
const uint32_t GRIB2::param_meteorology_moisture_scliwc;
const uint32_t GRIB2::param_meteorology_moisture_srainw;
const uint32_t GRIB2::param_meteorology_moisture_ssnoww;
const uint32_t GRIB2::param_meteorology_moisture_tkmflx;
const uint32_t GRIB2::param_meteorology_moisture_ukmflx;
const uint32_t GRIB2::param_meteorology_moisture_vkmflx;
const uint32_t GRIB2::param_meteorology_moisture_crain_2;
const uint32_t GRIB2::param_meteorology_moisture_cfrzr_2;
const uint32_t GRIB2::param_meteorology_moisture_cicep_2;
const uint32_t GRIB2::param_meteorology_moisture_csnow_2;
const uint32_t GRIB2::param_meteorology_moisture_cprat_2;
const uint32_t GRIB2::param_meteorology_moisture_mdiv_2;
const uint32_t GRIB2::param_meteorology_moisture_minrh;
const uint32_t GRIB2::param_meteorology_moisture_pevap_2;
const uint32_t GRIB2::param_meteorology_moisture_pevpr_2;
const uint32_t GRIB2::param_meteorology_moisture_snowc_2;
const uint32_t GRIB2::param_meteorology_moisture_frain_2;
const uint32_t GRIB2::param_meteorology_moisture_rime_2;
const uint32_t GRIB2::param_meteorology_moisture_tcolr_2;
const uint32_t GRIB2::param_meteorology_moisture_tcols_2;
const uint32_t GRIB2::param_meteorology_moisture_tipd;
const uint32_t GRIB2::param_meteorology_moisture_ncip;
const uint32_t GRIB2::param_meteorology_moisture_snot;
const uint32_t GRIB2::param_meteorology_moisture_tclsw;
const uint32_t GRIB2::param_meteorology_moisture_tcolm;
const uint32_t GRIB2::param_meteorology_moisture_emnp;
const uint32_t GRIB2::param_meteorology_moisture_sbsno;
const uint32_t GRIB2::param_meteorology_moisture_cnvmr;
const uint32_t GRIB2::param_meteorology_moisture_shamr;
const uint32_t GRIB2::param_meteorology_moisture_vdfmr;
const uint32_t GRIB2::param_meteorology_moisture_condp;
const uint32_t GRIB2::param_meteorology_moisture_lrgmr;
const uint32_t GRIB2::param_meteorology_moisture_qz0;
const uint32_t GRIB2::param_meteorology_moisture_qmax;
const uint32_t GRIB2::param_meteorology_moisture_qmin;
const uint32_t GRIB2::param_meteorology_moisture_arain;
const uint32_t GRIB2::param_meteorology_moisture_snowt;
const uint32_t GRIB2::param_meteorology_moisture_apcpn;
const uint32_t GRIB2::param_meteorology_moisture_acpcpn;
const uint32_t GRIB2::param_meteorology_moisture_frzr;
const uint32_t GRIB2::param_meteorology_moisture_pwther;
const uint32_t GRIB2::param_meteorology_moisture_frozr;
const uint32_t GRIB2::param_meteorology_moisture_tsnow;
const uint32_t GRIB2::param_meteorology_moisture_rhpw;
const uint32_t GRIB2::param_meteorology_momentum_wdir;
const uint32_t GRIB2::param_meteorology_momentum_wind;
const uint32_t GRIB2::param_meteorology_momentum_ugrd;
const uint32_t GRIB2::param_meteorology_momentum_vgrd;
const uint32_t GRIB2::param_meteorology_momentum_strm;
const uint32_t GRIB2::param_meteorology_momentum_vpot;
const uint32_t GRIB2::param_meteorology_momentum_mntsf;
const uint32_t GRIB2::param_meteorology_momentum_sgcvv;
const uint32_t GRIB2::param_meteorology_momentum_vvel;
const uint32_t GRIB2::param_meteorology_momentum_dzdt;
const uint32_t GRIB2::param_meteorology_momentum_absv;
const uint32_t GRIB2::param_meteorology_momentum_absd;
const uint32_t GRIB2::param_meteorology_momentum_relv;
const uint32_t GRIB2::param_meteorology_momentum_reld;
const uint32_t GRIB2::param_meteorology_momentum_pvort;
const uint32_t GRIB2::param_meteorology_momentum_vucsh;
const uint32_t GRIB2::param_meteorology_momentum_vvcsh;
const uint32_t GRIB2::param_meteorology_momentum_uflx;
const uint32_t GRIB2::param_meteorology_momentum_vflx;
const uint32_t GRIB2::param_meteorology_momentum_wmixe;
const uint32_t GRIB2::param_meteorology_momentum_blydp;
const uint32_t GRIB2::param_meteorology_momentum_maxgust;
const uint32_t GRIB2::param_meteorology_momentum_gust;
const uint32_t GRIB2::param_meteorology_momentum_ugust;
const uint32_t GRIB2::param_meteorology_momentum_vgust;
const uint32_t GRIB2::param_meteorology_momentum_vwsh;
const uint32_t GRIB2::param_meteorology_momentum_mflx;
const uint32_t GRIB2::param_meteorology_momentum_ustm;
const uint32_t GRIB2::param_meteorology_momentum_vstm;
const uint32_t GRIB2::param_meteorology_momentum_cd;
const uint32_t GRIB2::param_meteorology_momentum_fricv;
const uint32_t GRIB2::param_meteorology_momentum_tdcmom;
const uint32_t GRIB2::param_meteorology_momentum_etacvv;
const uint32_t GRIB2::param_meteorology_momentum_windf;
const uint32_t GRIB2::param_meteorology_momentum_vwsh_2;
const uint32_t GRIB2::param_meteorology_momentum_mflx_2;
const uint32_t GRIB2::param_meteorology_momentum_ustm_2;
const uint32_t GRIB2::param_meteorology_momentum_vstm_2;
const uint32_t GRIB2::param_meteorology_momentum_cd_2;
const uint32_t GRIB2::param_meteorology_momentum_fricv_2;
const uint32_t GRIB2::param_meteorology_momentum_lauv;
const uint32_t GRIB2::param_meteorology_momentum_louv;
const uint32_t GRIB2::param_meteorology_momentum_lavv;
const uint32_t GRIB2::param_meteorology_momentum_lovv;
const uint32_t GRIB2::param_meteorology_momentum_lapp;
const uint32_t GRIB2::param_meteorology_momentum_lopp;
const uint32_t GRIB2::param_meteorology_momentum_vedh;
const uint32_t GRIB2::param_meteorology_momentum_covmz;
const uint32_t GRIB2::param_meteorology_momentum_covtz;
const uint32_t GRIB2::param_meteorology_momentum_covtm;
const uint32_t GRIB2::param_meteorology_momentum_vdfua;
const uint32_t GRIB2::param_meteorology_momentum_vdfva;
const uint32_t GRIB2::param_meteorology_momentum_gwdu;
const uint32_t GRIB2::param_meteorology_momentum_gwdv;
const uint32_t GRIB2::param_meteorology_momentum_cnvu;
const uint32_t GRIB2::param_meteorology_momentum_cnvv;
const uint32_t GRIB2::param_meteorology_momentum_wtend;
const uint32_t GRIB2::param_meteorology_momentum_omgalf;
const uint32_t GRIB2::param_meteorology_momentum_cngwdu;
const uint32_t GRIB2::param_meteorology_momentum_cngwdv;
const uint32_t GRIB2::param_meteorology_momentum_lmv;
const uint32_t GRIB2::param_meteorology_momentum_pvmww;
const uint32_t GRIB2::param_meteorology_momentum_maxuvv;
const uint32_t GRIB2::param_meteorology_momentum_maxdvv;
const uint32_t GRIB2::param_meteorology_momentum_maxuw;
const uint32_t GRIB2::param_meteorology_momentum_maxvw;
const uint32_t GRIB2::param_meteorology_momentum_vrate;
const uint32_t GRIB2::param_meteorology_mass_pres;
const uint32_t GRIB2::param_meteorology_mass_prmsl;
const uint32_t GRIB2::param_meteorology_mass_ptend;
const uint32_t GRIB2::param_meteorology_mass_icaht;
const uint32_t GRIB2::param_meteorology_mass_gp;
const uint32_t GRIB2::param_meteorology_mass_hgt;
const uint32_t GRIB2::param_meteorology_mass_dist;
const uint32_t GRIB2::param_meteorology_mass_hstdv;
const uint32_t GRIB2::param_meteorology_mass_presa;
const uint32_t GRIB2::param_meteorology_mass_gpa;
const uint32_t GRIB2::param_meteorology_mass_den;
const uint32_t GRIB2::param_meteorology_mass_alts;
const uint32_t GRIB2::param_meteorology_mass_thick;
const uint32_t GRIB2::param_meteorology_mass_presalt;
const uint32_t GRIB2::param_meteorology_mass_denalt;
const uint32_t GRIB2::param_meteorology_mass_5wavh;
const uint32_t GRIB2::param_meteorology_mass_hpbl;
const uint32_t GRIB2::param_meteorology_mass_5wava;
const uint32_t GRIB2::param_meteorology_mass_sdsgso;
const uint32_t GRIB2::param_meteorology_mass_aosgso;
const uint32_t GRIB2::param_meteorology_mass_ssgso;
const uint32_t GRIB2::param_meteorology_mass_gwd;
const uint32_t GRIB2::param_meteorology_mass_asgso;
const uint32_t GRIB2::param_meteorology_mass_nlpres;
const uint32_t GRIB2::param_meteorology_mass_mslet;
const uint32_t GRIB2::param_meteorology_mass_5wavh_2;
const uint32_t GRIB2::param_meteorology_mass_hpbl_2;
const uint32_t GRIB2::param_meteorology_mass_5wava_2;
const uint32_t GRIB2::param_meteorology_mass_mslma;
const uint32_t GRIB2::param_meteorology_mass_tslsa;
const uint32_t GRIB2::param_meteorology_mass_plpl;
const uint32_t GRIB2::param_meteorology_mass_lpsx;
const uint32_t GRIB2::param_meteorology_mass_lpsy;
const uint32_t GRIB2::param_meteorology_mass_hgtx;
const uint32_t GRIB2::param_meteorology_mass_hgty;
const uint32_t GRIB2::param_meteorology_mass_layth;
const uint32_t GRIB2::param_meteorology_mass_nlgsp;
const uint32_t GRIB2::param_meteorology_mass_cnvumf;
const uint32_t GRIB2::param_meteorology_mass_cnvdmf;
const uint32_t GRIB2::param_meteorology_mass_cnvdemf;
const uint32_t GRIB2::param_meteorology_mass_lmh;
const uint32_t GRIB2::param_meteorology_mass_hgtn;
const uint32_t GRIB2::param_meteorology_mass_presn;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_nswrs;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_nswrt;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_swavr;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_grad;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_brtmp;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_lwrad;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_swrad;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_dswrf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_uswrf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_nswrf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_photar;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_nswrfcs;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_dwuvr;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_uviucs;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_uvi;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_dswrf_2;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_uswrf_2;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_duvb;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_cduvb;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_csdsf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_swhr;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_csusf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_cfnsf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_vbdsf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_vddsf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_nbdsf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_nddsf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_dtrf;
const uint32_t GRIB2::param_meteorology_shortwave_radiation_utrf;
const uint32_t GRIB2::param_meteorology_longwave_radiation_nlwrs;
const uint32_t GRIB2::param_meteorology_longwave_radiation_nlwrt;
const uint32_t GRIB2::param_meteorology_longwave_radiation_lwavr;
const uint32_t GRIB2::param_meteorology_longwave_radiation_dlwrf;
const uint32_t GRIB2::param_meteorology_longwave_radiation_ulwrf;
const uint32_t GRIB2::param_meteorology_longwave_radiation_nlwrf;
const uint32_t GRIB2::param_meteorology_longwave_radiation_nlwrcs;
const uint32_t GRIB2::param_meteorology_longwave_radiation_dlwrf_2;
const uint32_t GRIB2::param_meteorology_longwave_radiation_ulwrf_2;
const uint32_t GRIB2::param_meteorology_longwave_radiation_lwhr;
const uint32_t GRIB2::param_meteorology_longwave_radiation_csulf;
const uint32_t GRIB2::param_meteorology_longwave_radiation_csdlf;
const uint32_t GRIB2::param_meteorology_longwave_radiation_cfnlf;
const uint32_t GRIB2::param_meteorology_cloud_cice;
const uint32_t GRIB2::param_meteorology_cloud_tcdc;
const uint32_t GRIB2::param_meteorology_cloud_cdcon;
const uint32_t GRIB2::param_meteorology_cloud_lcdc;
const uint32_t GRIB2::param_meteorology_cloud_mcdc;
const uint32_t GRIB2::param_meteorology_cloud_hcdc;
const uint32_t GRIB2::param_meteorology_cloud_cwat;
const uint32_t GRIB2::param_meteorology_cloud_cdca;
const uint32_t GRIB2::param_meteorology_cloud_cdcty;
const uint32_t GRIB2::param_meteorology_cloud_tmaxt;
const uint32_t GRIB2::param_meteorology_cloud_thunc;
const uint32_t GRIB2::param_meteorology_cloud_cdcb;
const uint32_t GRIB2::param_meteorology_cloud_cdct;
const uint32_t GRIB2::param_meteorology_cloud_ceil;
const uint32_t GRIB2::param_meteorology_cloud_cdlyr;
const uint32_t GRIB2::param_meteorology_cloud_cwork;
const uint32_t GRIB2::param_meteorology_cloud_cuefi;
const uint32_t GRIB2::param_meteorology_cloud_tcond;
const uint32_t GRIB2::param_meteorology_cloud_tcolw;
const uint32_t GRIB2::param_meteorology_cloud_tcoli;
const uint32_t GRIB2::param_meteorology_cloud_tcolc;
const uint32_t GRIB2::param_meteorology_cloud_fice;
const uint32_t GRIB2::param_meteorology_cloud_cdcc;
const uint32_t GRIB2::param_meteorology_cloud_cdcimr;
const uint32_t GRIB2::param_meteorology_cloud_suns;
const uint32_t GRIB2::param_meteorology_cloud_cbhe;
const uint32_t GRIB2::param_meteorology_cloud_hconcb;
const uint32_t GRIB2::param_meteorology_cloud_hconct;
const uint32_t GRIB2::param_meteorology_cloud_nconcd;
const uint32_t GRIB2::param_meteorology_cloud_nccice;
const uint32_t GRIB2::param_meteorology_cloud_ndencd;
const uint32_t GRIB2::param_meteorology_cloud_ndcice;
const uint32_t GRIB2::param_meteorology_cloud_fraccc;
const uint32_t GRIB2::param_meteorology_cloud_sunsd;
const uint32_t GRIB2::param_meteorology_cloud_cdlyr_2;
const uint32_t GRIB2::param_meteorology_cloud_cwork_2;
const uint32_t GRIB2::param_meteorology_cloud_cuefi_2;
const uint32_t GRIB2::param_meteorology_cloud_tcond_2;
const uint32_t GRIB2::param_meteorology_cloud_tcolw_2;
const uint32_t GRIB2::param_meteorology_cloud_tcoli_2;
const uint32_t GRIB2::param_meteorology_cloud_tcolc_2;
const uint32_t GRIB2::param_meteorology_cloud_fice_2;
const uint32_t GRIB2::param_meteorology_cloud_mflux;
const uint32_t GRIB2::param_meteorology_cloud_sunsd_2;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_pli;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_bli;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_kx;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_kox;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_totalx;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_sx;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_cape;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_cin;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_hlcy;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_ehlx;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_lftx;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_4lftx;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_ri;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_shwinx;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_uphl;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_lftx_2;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_4lftx_2;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_ri_2;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_cwdi;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_uvi;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_uphl_2;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_lai;
const uint32_t GRIB2::param_meteorology_thermodynamic_stability_mxuphl;
const uint32_t GRIB2::param_meteorology_aerosols_aerot;
const uint32_t GRIB2::param_meteorology_aerosols_pmtc;
const uint32_t GRIB2::param_meteorology_aerosols_pmtf;
const uint32_t GRIB2::param_meteorology_aerosols_lpmtf;
const uint32_t GRIB2::param_meteorology_aerosols_lipmf;
const uint32_t GRIB2::param_meteorology_trace_gases_tozne;
const uint32_t GRIB2::param_meteorology_trace_gases_o3mr;
const uint32_t GRIB2::param_meteorology_trace_gases_tcioz;
const uint32_t GRIB2::param_meteorology_trace_gases_o3mr_2;
const uint32_t GRIB2::param_meteorology_trace_gases_ozcon;
const uint32_t GRIB2::param_meteorology_trace_gases_ozcat;
const uint32_t GRIB2::param_meteorology_trace_gases_vdfoz;
const uint32_t GRIB2::param_meteorology_trace_gases_poz;
const uint32_t GRIB2::param_meteorology_trace_gases_toz;
const uint32_t GRIB2::param_meteorology_trace_gases_pozt;
const uint32_t GRIB2::param_meteorology_trace_gases_pozo;
const uint32_t GRIB2::param_meteorology_trace_gases_ozmax1;
const uint32_t GRIB2::param_meteorology_trace_gases_ozmax8;
const uint32_t GRIB2::param_meteorology_trace_gases_pdmax1;
const uint32_t GRIB2::param_meteorology_trace_gases_pdmax24;
const uint32_t GRIB2::param_meteorology_radar_bswid;
const uint32_t GRIB2::param_meteorology_radar_bref;
const uint32_t GRIB2::param_meteorology_radar_brvel;
const uint32_t GRIB2::param_meteorology_radar_vil;
const uint32_t GRIB2::param_meteorology_radar_lmaxbr;
const uint32_t GRIB2::param_meteorology_radar_prec;
const uint32_t GRIB2::param_meteorology_radar_rdsp1;
const uint32_t GRIB2::param_meteorology_radar_rdsp2;
const uint32_t GRIB2::param_meteorology_radar_rdsp3;
const uint32_t GRIB2::param_meteorology_radar_rfcd;
const uint32_t GRIB2::param_meteorology_radar_rfci;
const uint32_t GRIB2::param_meteorology_radar_rfsnow;
const uint32_t GRIB2::param_meteorology_radar_rfrain;
const uint32_t GRIB2::param_meteorology_radar_rfgrpl;
const uint32_t GRIB2::param_meteorology_radar_rfhail;
const uint32_t GRIB2::param_meteorology_forecast_radar_refzr;
const uint32_t GRIB2::param_meteorology_forecast_radar_refzi;
const uint32_t GRIB2::param_meteorology_forecast_radar_refzc;
const uint32_t GRIB2::param_meteorology_forecast_radar_retop;
const uint32_t GRIB2::param_meteorology_forecast_radar_refd;
const uint32_t GRIB2::param_meteorology_forecast_radar_refc;
const uint32_t GRIB2::param_meteorology_forecast_radar_refzr_2;
const uint32_t GRIB2::param_meteorology_forecast_radar_refzi_2;
const uint32_t GRIB2::param_meteorology_forecast_radar_refzc_2;
const uint32_t GRIB2::param_meteorology_forecast_radar_refd_2;
const uint32_t GRIB2::param_meteorology_forecast_radar_refc_2;
const uint32_t GRIB2::param_meteorology_forecast_radar_retop_2;
const uint32_t GRIB2::param_meteorology_forecast_radar_maxref;
const uint32_t GRIB2::param_meteorology_electrodynamics_ltng;
const uint32_t GRIB2::param_meteorology_nuclear_acces;
const uint32_t GRIB2::param_meteorology_nuclear_aciod;
const uint32_t GRIB2::param_meteorology_nuclear_acradp;
const uint32_t GRIB2::param_meteorology_nuclear_gdces;
const uint32_t GRIB2::param_meteorology_nuclear_gdiod;
const uint32_t GRIB2::param_meteorology_nuclear_gdradp;
const uint32_t GRIB2::param_meteorology_nuclear_tiaccp;
const uint32_t GRIB2::param_meteorology_nuclear_tiacip;
const uint32_t GRIB2::param_meteorology_nuclear_tiacrp;
const uint32_t GRIB2::param_meteorology_nuclear_aircon;
const uint32_t GRIB2::param_meteorology_nuclear_wetdep;
const uint32_t GRIB2::param_meteorology_nuclear_drydep;
const uint32_t GRIB2::param_meteorology_nuclear_totlwd;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_vis;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_albdo;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_tstm;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_mixht;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_volash;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_icit;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_icib;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_ici;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_turbt;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_turbb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_turb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_tke;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_pblreg;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_conti;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_contet;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_contt;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_contb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_mxsalb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_snfalb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_salbd;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_icip;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_ctp;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_cat;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_sldp;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_contke;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_wiww;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_convo;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_mxsalb_2;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_snfalb_2;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_srcono;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_mrcono;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_hrcono;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_torprob;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_hailprob;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_windprob;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_storprob;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_shailpro;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_swindpro;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_tstmc;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_mixly;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_flght;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_cicel;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_civis;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_ciflt;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_lavni;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_havni;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_sbsalb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_swsalb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_nbsalb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_nwsalb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_prsvr;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_prsigsvr;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_sipd;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_epsr;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_tpfi;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_svrts;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_procon;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_convp;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_vaftd;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_icprb;
const uint32_t GRIB2::param_meteorology_atmosphere_physics_icsev;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_massden;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_colmd;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_massmr;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_aemflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_anpmflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_anpemflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_sddmflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_swdmflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_aremflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_wlsmflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_wdcpmflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_sedmflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_ddmflx;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_tranhh;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_trsds;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_aia;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_conair;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_vmxr;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_cgprc;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_cgdrc;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_sflux;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_coaia;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_tyaba;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_tyaal;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_ancon;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_saden;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_atmtk;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_aotk;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_ssalbk;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_asysfk;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_aecoef;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_aacoef;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_albsat;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_albgrd;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_alesat;
const uint32_t GRIB2::param_meteorology_atmosphere_chemistry_alegrd;
const uint32_t GRIB2::param_meteorology_string_atext;
const uint32_t GRIB2::param_meteorology_miscellaneous_tsec;
const uint32_t GRIB2::param_meteorology_miscellaneous_geolat;
const uint32_t GRIB2::param_meteorology_miscellaneous_geolon;
const uint32_t GRIB2::param_meteorology_miscellaneous_nlat;
const uint32_t GRIB2::param_meteorology_miscellaneous_elon;
const uint32_t GRIB2::param_meteorology_miscellaneous_tsec_2;
const uint32_t GRIB2::param_meteorology_miscellaneous_mlyno;
const uint32_t GRIB2::param_meteorology_miscellaneous_nlatn;
const uint32_t GRIB2::param_meteorology_miscellaneous_elonn;
const uint32_t GRIB2::param_meteorology_covariance_covmz;
const uint32_t GRIB2::param_meteorology_covariance_covtz;
const uint32_t GRIB2::param_meteorology_covariance_covtm;
const uint32_t GRIB2::param_meteorology_covariance_covtw;
const uint32_t GRIB2::param_meteorology_covariance_covzz;
const uint32_t GRIB2::param_meteorology_covariance_covmm;
const uint32_t GRIB2::param_meteorology_covariance_covqz;
const uint32_t GRIB2::param_meteorology_covariance_covqm;
const uint32_t GRIB2::param_meteorology_covariance_covtvv;
const uint32_t GRIB2::param_meteorology_covariance_covqvv;
const uint32_t GRIB2::param_meteorology_covariance_covpsps;
const uint32_t GRIB2::param_meteorology_covariance_covqq;
const uint32_t GRIB2::param_meteorology_covariance_covvvvv;
const uint32_t GRIB2::param_meteorology_covariance_covtt;
const uint32_t GRIB2::param_hydrology_basic_ffldg;
const uint32_t GRIB2::param_hydrology_basic_ffldro;
const uint32_t GRIB2::param_hydrology_basic_rssc;
const uint32_t GRIB2::param_hydrology_basic_esct;
const uint32_t GRIB2::param_hydrology_basic_swepon;
const uint32_t GRIB2::param_hydrology_basic_bgrun;
const uint32_t GRIB2::param_hydrology_basic_ssrun;
const uint32_t GRIB2::param_hydrology_basic_bgrun_2;
const uint32_t GRIB2::param_hydrology_basic_ssrun_2;
const uint32_t GRIB2::param_hydrology_prob_cppop;
const uint32_t GRIB2::param_hydrology_prob_pposp;
const uint32_t GRIB2::param_hydrology_prob_pop;
const uint32_t GRIB2::param_hydrology_prob_cpozp;
const uint32_t GRIB2::param_hydrology_prob_cpofp;
const uint32_t GRIB2::param_hydrology_prob_ppffg;
const uint32_t GRIB2::param_hydrology_prob_cwr;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_wdpthil;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_wtmpil;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_wfract;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_sedtk;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_sedtmp;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_ictkil;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_icetil;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_icecil;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_landil;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_sfsal;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_sftmp;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_acwsr;
const uint32_t GRIB2::param_hydrology_inlandwater_sediment_saltil;
const uint32_t GRIB2::param_landsurface_vegetation_land;
const uint32_t GRIB2::param_landsurface_vegetation_sfcr;
const uint32_t GRIB2::param_landsurface_vegetation_tsoil;
const uint32_t GRIB2::param_landsurface_vegetation_soilmc;
const uint32_t GRIB2::param_landsurface_vegetation_veg;
const uint32_t GRIB2::param_landsurface_vegetation_watr;
const uint32_t GRIB2::param_landsurface_vegetation_evapt;
const uint32_t GRIB2::param_landsurface_vegetation_mterh;
const uint32_t GRIB2::param_landsurface_vegetation_landu;
const uint32_t GRIB2::param_landsurface_vegetation_soilw;
const uint32_t GRIB2::param_landsurface_vegetation_gflux;
const uint32_t GRIB2::param_landsurface_vegetation_mstav;
const uint32_t GRIB2::param_landsurface_vegetation_sfexc;
const uint32_t GRIB2::param_landsurface_vegetation_cnwat;
const uint32_t GRIB2::param_landsurface_vegetation_bmixl;
const uint32_t GRIB2::param_landsurface_vegetation_ccond;
const uint32_t GRIB2::param_landsurface_vegetation_rsmin;
const uint32_t GRIB2::param_landsurface_vegetation_wilt;
const uint32_t GRIB2::param_landsurface_vegetation_rcs;
const uint32_t GRIB2::param_landsurface_vegetation_rct;
const uint32_t GRIB2::param_landsurface_vegetation_rcq;
const uint32_t GRIB2::param_landsurface_vegetation_rcsol;
const uint32_t GRIB2::param_landsurface_vegetation_soilm;
const uint32_t GRIB2::param_landsurface_vegetation_cisoilw;
const uint32_t GRIB2::param_landsurface_vegetation_hflux;
const uint32_t GRIB2::param_landsurface_vegetation_vsoilm;
const uint32_t GRIB2::param_landsurface_vegetation_wilt_3;
const uint32_t GRIB2::param_landsurface_vegetation_vwiltp;
const uint32_t GRIB2::param_landsurface_vegetation_leainx;
const uint32_t GRIB2::param_landsurface_vegetation_everf;
const uint32_t GRIB2::param_landsurface_vegetation_decf;
const uint32_t GRIB2::param_landsurface_vegetation_ndvinx;
const uint32_t GRIB2::param_landsurface_vegetation_rdveg;
const uint32_t GRIB2::param_landsurface_vegetation_soilw_2;
const uint32_t GRIB2::param_landsurface_vegetation_gflux_2;
const uint32_t GRIB2::param_landsurface_vegetation_mstav_2;
const uint32_t GRIB2::param_landsurface_vegetation_sfexc_2;
const uint32_t GRIB2::param_landsurface_vegetation_cnwat_2;
const uint32_t GRIB2::param_landsurface_vegetation_bmixl_2;
const uint32_t GRIB2::param_landsurface_vegetation_vgtyp;
const uint32_t GRIB2::param_landsurface_vegetation_ccond_2;
const uint32_t GRIB2::param_landsurface_vegetation_rsmin_2;
const uint32_t GRIB2::param_landsurface_vegetation_wilt_2;
const uint32_t GRIB2::param_landsurface_vegetation_rcs_2;
const uint32_t GRIB2::param_landsurface_vegetation_rct_2;
const uint32_t GRIB2::param_landsurface_vegetation_rcq_2;
const uint32_t GRIB2::param_landsurface_vegetation_rcsol_2;
const uint32_t GRIB2::param_landsurface_vegetation_rdrip;
const uint32_t GRIB2::param_landsurface_vegetation_icwat;
const uint32_t GRIB2::param_landsurface_vegetation_akhs;
const uint32_t GRIB2::param_landsurface_vegetation_akms;
const uint32_t GRIB2::param_landsurface_vegetation_vegt;
const uint32_t GRIB2::param_landsurface_vegetation_sstor;
const uint32_t GRIB2::param_landsurface_vegetation_lsoil;
const uint32_t GRIB2::param_landsurface_vegetation_ewatr;
const uint32_t GRIB2::param_landsurface_vegetation_gwrec;
const uint32_t GRIB2::param_landsurface_vegetation_qrec;
const uint32_t GRIB2::param_landsurface_vegetation_sfcrh;
const uint32_t GRIB2::param_landsurface_vegetation_ndvi;
const uint32_t GRIB2::param_landsurface_vegetation_landn;
const uint32_t GRIB2::param_landsurface_vegetation_amixl;
const uint32_t GRIB2::param_landsurface_vegetation_wvinc;
const uint32_t GRIB2::param_landsurface_vegetation_wcinc;
const uint32_t GRIB2::param_landsurface_vegetation_wvconv;
const uint32_t GRIB2::param_landsurface_vegetation_wcconv;
const uint32_t GRIB2::param_landsurface_vegetation_wvuflx;
const uint32_t GRIB2::param_landsurface_vegetation_wvvflx;
const uint32_t GRIB2::param_landsurface_vegetation_wcuflx;
const uint32_t GRIB2::param_landsurface_vegetation_wcvflx;
const uint32_t GRIB2::param_landsurface_vegetation_acond;
const uint32_t GRIB2::param_landsurface_vegetation_evcw;
const uint32_t GRIB2::param_landsurface_vegetation_trans;
const uint32_t GRIB2::param_landsurface_agriaquaculture_canl;
const uint32_t GRIB2::param_landsurface_soil_sotyp;
const uint32_t GRIB2::param_landsurface_soil_uplst;
const uint32_t GRIB2::param_landsurface_soil_uplsm;
const uint32_t GRIB2::param_landsurface_soil_lowlsm;
const uint32_t GRIB2::param_landsurface_soil_botlst;
const uint32_t GRIB2::param_landsurface_soil_soill;
const uint32_t GRIB2::param_landsurface_soil_rlyrs;
const uint32_t GRIB2::param_landsurface_soil_smref;
const uint32_t GRIB2::param_landsurface_soil_smdry;
const uint32_t GRIB2::param_landsurface_soil_poros;
const uint32_t GRIB2::param_landsurface_soil_liqvsm;
const uint32_t GRIB2::param_landsurface_soil_voltso;
const uint32_t GRIB2::param_landsurface_soil_transo;
const uint32_t GRIB2::param_landsurface_soil_voldec;
const uint32_t GRIB2::param_landsurface_soil_direc;
const uint32_t GRIB2::param_landsurface_soil_soilp;
const uint32_t GRIB2::param_landsurface_soil_vsosm;
const uint32_t GRIB2::param_landsurface_soil_satosm;
const uint32_t GRIB2::param_landsurface_soil_soiltmp;
const uint32_t GRIB2::param_landsurface_soil_soilmoi;
const uint32_t GRIB2::param_landsurface_soil_cisoilm;
const uint32_t GRIB2::param_landsurface_soil_soilice;
const uint32_t GRIB2::param_landsurface_soil_cisice;
const uint32_t GRIB2::param_landsurface_soil_soill_2;
const uint32_t GRIB2::param_landsurface_soil_rlyrs_2;
const uint32_t GRIB2::param_landsurface_soil_sltyp;
const uint32_t GRIB2::param_landsurface_soil_smref_2;
const uint32_t GRIB2::param_landsurface_soil_smdry_2;
const uint32_t GRIB2::param_landsurface_soil_poros_2;
const uint32_t GRIB2::param_landsurface_soil_evbs;
const uint32_t GRIB2::param_landsurface_soil_lspa;
const uint32_t GRIB2::param_landsurface_soil_baret;
const uint32_t GRIB2::param_landsurface_soil_avsft;
const uint32_t GRIB2::param_landsurface_soil_radt;
const uint32_t GRIB2::param_landsurface_soil_fldcp;
const uint32_t GRIB2::param_landsurface_fire_fireolk;
const uint32_t GRIB2::param_landsurface_fire_fireodt;
const uint32_t GRIB2::param_landsurface_fire_hindex;
const uint32_t GRIB2::param_space_image_srad;
const uint32_t GRIB2::param_space_image_salbedo;
const uint32_t GRIB2::param_space_image_sbtmp;
const uint32_t GRIB2::param_space_image_spwat;
const uint32_t GRIB2::param_space_image_slfti;
const uint32_t GRIB2::param_space_image_sctpres;
const uint32_t GRIB2::param_space_image_sstmp;
const uint32_t GRIB2::param_space_image_cloudm;
const uint32_t GRIB2::param_space_image_pixst;
const uint32_t GRIB2::param_space_image_firedi;
const uint32_t GRIB2::param_space_quantitative_estp;
const uint32_t GRIB2::param_space_quantitative_irrate;
const uint32_t GRIB2::param_space_quantitative_ctoph;
const uint32_t GRIB2::param_space_quantitative_ctophqi;
const uint32_t GRIB2::param_space_quantitative_estugrd;
const uint32_t GRIB2::param_space_quantitative_estvgrd;
const uint32_t GRIB2::param_space_quantitative_npixu;
const uint32_t GRIB2::param_space_quantitative_solza;
const uint32_t GRIB2::param_space_quantitative_raza;
const uint32_t GRIB2::param_space_quantitative_rfl06;
const uint32_t GRIB2::param_space_quantitative_rfl08;
const uint32_t GRIB2::param_space_quantitative_rfl16;
const uint32_t GRIB2::param_space_quantitative_rfl39;
const uint32_t GRIB2::param_space_quantitative_atmdiv;
const uint32_t GRIB2::param_space_quantitative_cbtmp;
const uint32_t GRIB2::param_space_quantitative_csbtmp;
const uint32_t GRIB2::param_space_quantitative_cldrad;
const uint32_t GRIB2::param_space_quantitative_cskyrad;
const uint32_t GRIB2::param_space_quantitative_winds;
const uint32_t GRIB2::param_space_quantitative_aot06;
const uint32_t GRIB2::param_space_quantitative_aot08;
const uint32_t GRIB2::param_space_quantitative_aot16;
const uint32_t GRIB2::param_space_quantitative_angcoe;
const uint32_t GRIB2::param_space_quantitative_usct;
const uint32_t GRIB2::param_space_quantitative_vsct;
const uint32_t GRIB2::param_space_forecast_sbt122;
const uint32_t GRIB2::param_space_forecast_sbt123;
const uint32_t GRIB2::param_space_forecast_sbt124;
const uint32_t GRIB2::param_space_forecast_sbt126;
const uint32_t GRIB2::param_space_forecast_sbc123;
const uint32_t GRIB2::param_space_forecast_sbc124;
const uint32_t GRIB2::param_space_forecast_sbt112;
const uint32_t GRIB2::param_space_forecast_sbt113;
const uint32_t GRIB2::param_space_forecast_sbt114;
const uint32_t GRIB2::param_space_forecast_sbt115;
const uint32_t GRIB2::param_space_forecast_amsre9;
const uint32_t GRIB2::param_space_forecast_amsre10;
const uint32_t GRIB2::param_space_forecast_amsre11;
const uint32_t GRIB2::param_space_forecast_amsre12;
const uint32_t GRIB2::param_spaceweather_temperature_tmpswp;
const uint32_t GRIB2::param_spaceweather_temperature_electmp;
const uint32_t GRIB2::param_spaceweather_temperature_prottmp;
const uint32_t GRIB2::param_spaceweather_temperature_iontmp;
const uint32_t GRIB2::param_spaceweather_temperature_pratmp;
const uint32_t GRIB2::param_spaceweather_temperature_prptmp;
const uint32_t GRIB2::param_spaceweather_momentum_speed;
const uint32_t GRIB2::param_spaceweather_momentum_vel1;
const uint32_t GRIB2::param_spaceweather_momentum_vel2;
const uint32_t GRIB2::param_spaceweather_momentum_vel3;
const uint32_t GRIB2::param_spaceweather_charged_particles_plsmden;
const uint32_t GRIB2::param_spaceweather_charged_particles_elcden;
const uint32_t GRIB2::param_spaceweather_charged_particles_protden;
const uint32_t GRIB2::param_spaceweather_charged_particles_ionden;
const uint32_t GRIB2::param_spaceweather_charged_particles_vtec;
const uint32_t GRIB2::param_spaceweather_charged_particles_absfrq;
const uint32_t GRIB2::param_spaceweather_charged_particles_absrb;
const uint32_t GRIB2::param_spaceweather_charged_particles_sprdf;
const uint32_t GRIB2::param_spaceweather_charged_particles_hprimf;
const uint32_t GRIB2::param_spaceweather_charged_particles_crtfrq;
const uint32_t GRIB2::param_spaceweather_charged_particles_scint;
const uint32_t GRIB2::param_spaceweather_fields_btot;
const uint32_t GRIB2::param_spaceweather_fields_bvec1;
const uint32_t GRIB2::param_spaceweather_fields_bvec2;
const uint32_t GRIB2::param_spaceweather_fields_bvec3;
const uint32_t GRIB2::param_spaceweather_fields_etot;
const uint32_t GRIB2::param_spaceweather_fields_evec1;
const uint32_t GRIB2::param_spaceweather_fields_evec2;
const uint32_t GRIB2::param_spaceweather_fields_evec3;
const uint32_t GRIB2::param_spaceweather_energetic_particles_difpflux;
const uint32_t GRIB2::param_spaceweather_energetic_particles_intpflux;
const uint32_t GRIB2::param_spaceweather_energetic_particles_difeflux;
const uint32_t GRIB2::param_spaceweather_energetic_particles_inteflux;
const uint32_t GRIB2::param_spaceweather_energetic_particles_dififlux;
const uint32_t GRIB2::param_spaceweather_energetic_particles_intiflux;
const uint32_t GRIB2::param_spaceweather_energetic_particles_ntrnflux;
const uint32_t GRIB2::param_spaceweather_solar_tsi;
const uint32_t GRIB2::param_spaceweather_solar_xlong;
const uint32_t GRIB2::param_spaceweather_solar_xshrt;
const uint32_t GRIB2::param_spaceweather_solar_euvirr;
const uint32_t GRIB2::param_spaceweather_solar_specirr;
const uint32_t GRIB2::param_spaceweather_solar_f107;
const uint32_t GRIB2::param_spaceweather_solar_solrf;
const uint32_t GRIB2::param_spaceweather_terrestrial_lmbint;
const uint32_t GRIB2::param_spaceweather_terrestrial_dskint;
const uint32_t GRIB2::param_spaceweather_terrestrial_dskday;
const uint32_t GRIB2::param_spaceweather_terrestrial_dskngt;
const uint32_t GRIB2::param_spaceweather_imagery_xrayrad;
const uint32_t GRIB2::param_spaceweather_imagery_euvrad;
const uint32_t GRIB2::param_spaceweather_imagery_harad;
const uint32_t GRIB2::param_spaceweather_imagery_whtrad;
const uint32_t GRIB2::param_spaceweather_imagery_caiirad;
const uint32_t GRIB2::param_spaceweather_imagery_whtcor;
const uint32_t GRIB2::param_spaceweather_imagery_helcor;
const uint32_t GRIB2::param_spaceweather_imagery_mask;
const uint32_t GRIB2::param_spaceweather_ionneutral_coupling_sigped;
const uint32_t GRIB2::param_spaceweather_ionneutral_coupling_sighal;
const uint32_t GRIB2::param_spaceweather_ionneutral_coupling_sigpar;
const uint32_t GRIB2::param_oceanography_waves_wvsp1;
const uint32_t GRIB2::param_oceanography_waves_wvsp2;
const uint32_t GRIB2::param_oceanography_waves_wvsp3;
const uint32_t GRIB2::param_oceanography_waves_htsgw;
const uint32_t GRIB2::param_oceanography_waves_wvdir;
const uint32_t GRIB2::param_oceanography_waves_wvhgt;
const uint32_t GRIB2::param_oceanography_waves_wvper;
const uint32_t GRIB2::param_oceanography_waves_swdir;
const uint32_t GRIB2::param_oceanography_waves_swell;
const uint32_t GRIB2::param_oceanography_waves_swper;
const uint32_t GRIB2::param_oceanography_waves_dirpw;
const uint32_t GRIB2::param_oceanography_waves_perpw;
const uint32_t GRIB2::param_oceanography_waves_dirsw;
const uint32_t GRIB2::param_oceanography_waves_persw;
const uint32_t GRIB2::param_oceanography_waves_wwsdir;
const uint32_t GRIB2::param_oceanography_waves_mwsper;
const uint32_t GRIB2::param_oceanography_waves_cdww;
const uint32_t GRIB2::param_oceanography_waves_fricv;
const uint32_t GRIB2::param_oceanography_waves_wstr;
const uint32_t GRIB2::param_oceanography_waves_nwstr;
const uint32_t GRIB2::param_oceanography_waves_mssw;
const uint32_t GRIB2::param_oceanography_waves_ussd;
const uint32_t GRIB2::param_oceanography_waves_vssd;
const uint32_t GRIB2::param_oceanography_waves_pmaxwh;
const uint32_t GRIB2::param_oceanography_waves_maxwh;
const uint32_t GRIB2::param_oceanography_waves_imwf;
const uint32_t GRIB2::param_oceanography_waves_imfww;
const uint32_t GRIB2::param_oceanography_waves_imftsw;
const uint32_t GRIB2::param_oceanography_waves_mzwper;
const uint32_t GRIB2::param_oceanography_waves_mzpww;
const uint32_t GRIB2::param_oceanography_waves_mzptsw;
const uint32_t GRIB2::param_oceanography_waves_wdirw;
const uint32_t GRIB2::param_oceanography_waves_dirwww;
const uint32_t GRIB2::param_oceanography_waves_dirwts;
const uint32_t GRIB2::param_oceanography_waves_pwper;
const uint32_t GRIB2::param_oceanography_waves_pperww;
const uint32_t GRIB2::param_oceanography_waves_pperts;
const uint32_t GRIB2::param_oceanography_waves_altwh;
const uint32_t GRIB2::param_oceanography_waves_alcwh;
const uint32_t GRIB2::param_oceanography_waves_alrrc;
const uint32_t GRIB2::param_oceanography_waves_mnwsow;
const uint32_t GRIB2::param_oceanography_waves_mwdirw;
const uint32_t GRIB2::param_oceanography_waves_wesp;
const uint32_t GRIB2::param_oceanography_waves_kssedw;
const uint32_t GRIB2::param_oceanography_waves_beninx;
const uint32_t GRIB2::param_oceanography_waves_spftr;
const uint32_t GRIB2::param_oceanography_waves_2dsed;
const uint32_t GRIB2::param_oceanography_waves_fseed;
const uint32_t GRIB2::param_oceanography_waves_dirsed;
const uint32_t GRIB2::param_oceanography_waves_hsign;
const uint32_t GRIB2::param_oceanography_waves_pkdir;
const uint32_t GRIB2::param_oceanography_waves_mnstp;
const uint32_t GRIB2::param_oceanography_waves_dmspr;
const uint32_t GRIB2::param_oceanography_waves_wffrac;
const uint32_t GRIB2::param_oceanography_waves_temm1;
const uint32_t GRIB2::param_oceanography_waves_dir11;
const uint32_t GRIB2::param_oceanography_waves_dir22;
const uint32_t GRIB2::param_oceanography_waves_dspr11;
const uint32_t GRIB2::param_oceanography_waves_dspr22;
const uint32_t GRIB2::param_oceanography_waves_wlen;
const uint32_t GRIB2::param_oceanography_waves_rdsxx;
const uint32_t GRIB2::param_oceanography_waves_rdsyy;
const uint32_t GRIB2::param_oceanography_waves_rdsxy;
const uint32_t GRIB2::param_oceanography_waves_wstp;
const uint32_t GRIB2::param_oceanography_currents_dirc;
const uint32_t GRIB2::param_oceanography_currents_spc;
const uint32_t GRIB2::param_oceanography_currents_uogrd;
const uint32_t GRIB2::param_oceanography_currents_vogrd;
const uint32_t GRIB2::param_oceanography_currents_omlu;
const uint32_t GRIB2::param_oceanography_currents_omlv;
const uint32_t GRIB2::param_oceanography_currents_ubaro;
const uint32_t GRIB2::param_oceanography_currents_vbaro;
const uint32_t GRIB2::param_oceanography_ice_icec;
const uint32_t GRIB2::param_oceanography_ice_icetk;
const uint32_t GRIB2::param_oceanography_ice_diced;
const uint32_t GRIB2::param_oceanography_ice_siced;
const uint32_t GRIB2::param_oceanography_ice_uice;
const uint32_t GRIB2::param_oceanography_ice_vice;
const uint32_t GRIB2::param_oceanography_ice_iceg;
const uint32_t GRIB2::param_oceanography_ice_iced;
const uint32_t GRIB2::param_oceanography_ice_icetmp;
const uint32_t GRIB2::param_oceanography_ice_iceprs;
const uint32_t GRIB2::param_oceanography_surface_wtmp;
const uint32_t GRIB2::param_oceanography_surface_dslm;
const uint32_t GRIB2::param_oceanography_surface_surge;
const uint32_t GRIB2::param_oceanography_surface_etsrg;
const uint32_t GRIB2::param_oceanography_surface_elev;
const uint32_t GRIB2::param_oceanography_surface_sshg;
const uint32_t GRIB2::param_oceanography_surface_p2omlt;
const uint32_t GRIB2::param_oceanography_surface_aohflx;
const uint32_t GRIB2::param_oceanography_surface_ashfl;
const uint32_t GRIB2::param_oceanography_surface_sstt;
const uint32_t GRIB2::param_oceanography_surface_ssst;
const uint32_t GRIB2::param_oceanography_surface_keng;
const uint32_t GRIB2::param_oceanography_surface_sltfl;
const uint32_t GRIB2::param_oceanography_surface_tcsrg20;
const uint32_t GRIB2::param_oceanography_surface_tcsrg30;
const uint32_t GRIB2::param_oceanography_surface_tcsrg40;
const uint32_t GRIB2::param_oceanography_surface_tcsrg50;
const uint32_t GRIB2::param_oceanography_surface_tcsrg60;
const uint32_t GRIB2::param_oceanography_surface_tcsrg70;
const uint32_t GRIB2::param_oceanography_surface_tcsrg80;
const uint32_t GRIB2::param_oceanography_surface_tcsrg90;
const uint32_t GRIB2::param_oceanography_surface_etcwl;
const uint32_t GRIB2::param_oceanography_subsurface_mthd;
const uint32_t GRIB2::param_oceanography_subsurface_mtha;
const uint32_t GRIB2::param_oceanography_subsurface_tthdp;
const uint32_t GRIB2::param_oceanography_subsurface_salty;
const uint32_t GRIB2::param_oceanography_subsurface_ovhd;
const uint32_t GRIB2::param_oceanography_subsurface_ovsd;
const uint32_t GRIB2::param_oceanography_subsurface_ovmd;
const uint32_t GRIB2::param_oceanography_subsurface_bathy;
const uint32_t GRIB2::param_oceanography_subsurface_sfsalp;
const uint32_t GRIB2::param_oceanography_subsurface_sftmpp;
const uint32_t GRIB2::param_oceanography_subsurface_acwsrd;
const uint32_t GRIB2::param_oceanography_subsurface_wdepth;
const uint32_t GRIB2::param_oceanography_subsurface_wtmpss;
const uint32_t GRIB2::param_oceanography_subsurface_wtmpc;
const uint32_t GRIB2::param_oceanography_subsurface_salin;
const uint32_t GRIB2::param_oceanography_subsurface_bkeng;
const uint32_t GRIB2::param_oceanography_subsurface_dbss;
const uint32_t GRIB2::param_oceanography_subsurface_intfd;
const uint32_t GRIB2::param_oceanography_subsurface_ohc;
const uint32_t GRIB2::param_oceanography_miscellaneous_tsec;
const uint32_t GRIB2::param_oceanography_miscellaneous_mosf;

class GRIB2::LayerAllocator {
public:
	static inline std::size_t pagesize(void);
	static inline void readonly(const void *ptr);
	static inline void readwrite(const void *ptr);
};

const GRIB2::LayerAllocator GRIB2::LayerAlloc;

inline std::size_t GRIB2::LayerAllocator::pagesize(void)
{
#if defined(HAVE_SYSCONF)
	return sysconf(_SC_PAGE_SIZE);
#else
	return 4096;
#endif
}

#undef DEBUG_LAYERALLOC

#if !defined(HAVE_MMAP)
#undef DEBUG_LAYERALLOC
#endif

inline void *operator new(std::size_t sz, const GRIB2::LayerAllocator& a) throw (std::bad_alloc)
{
#if defined(DEBUG_LAYERALLOC)
	const std::size_t pgsz(a.pagesize());
	const std::size_t objsz((sz + 15 + pgsz) & ~pgsz);
	void *ptr(mmap(0, objsz, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
	if (ptr == MAP_FAILED)
		throw std::bad_alloc();
	*(std::size_t *)ptr = objsz;
	return ptr + 16;
#else
	return ::operator new(sz);
#endif
}

inline void *operator new[](std::size_t sz, const GRIB2::LayerAllocator& a) throw (std::bad_alloc)
{
#if defined(DEBUG_LAYERALLOC)
	return ::operator new(sz, a);
#else
	return ::operator new[](sz);
#endif
}

inline void operator delete(void *ptr, const GRIB2::LayerAllocator& a) throw ()
{
#if defined(DEBUG_LAYERALLOC)
	//const std::size_t pgsz(a.pagesize());
	void *ptr1(ptr - 16);
	std::size_t objsz(*(std::size_t *)ptr1);

	if (((uint8_t *)ptr1)[8]) {
		std::cerr << "Double delete" << std::endl;
		*(int *)0 = 0;
	}
	((uint8_t *)ptr1)[8] = 1;
	mprotect(ptr1, objsz, PROT_READ);

	//munmap(ptr1, objsz);
#else
	::operator delete(ptr);
#endif
}

inline void operator delete[](void *ptr, const GRIB2::LayerAllocator& a) throw ()
{
#if defined(DEBUG_LAYERALLOC)
	::operator delete(ptr, a);
#else
	::operator delete[](ptr);
#endif
}

inline void GRIB2::LayerAllocator::readonly(const void *ptr)
{
#if defined(DEBUG_LAYERALLOC)
	//const std::size_t pgsz(a.pagesize());
	void *ptr1(const_cast<void *>(ptr) - 16);
	std::size_t objsz(*(std::size_t *)ptr1);
	mprotect(ptr1, objsz, PROT_READ);
#endif
}

inline void GRIB2::LayerAllocator::readwrite(const void *ptr)
{
#if defined(DEBUG_LAYERALLOC)
	//const std::size_t pgsz(a.pagesize());
	void *ptr1(const_cast<void *>(ptr) - 16);
	std::size_t objsz(*(std::size_t *)ptr1);
	mprotect(ptr1, objsz, PROT_READ | PROT_WRITE);
#endif
}

GRIB2::Grid::Grid()
	: m_refcount(1)
{
}

GRIB2::Grid::~Grid()
{
}

void GRIB2::Grid::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void GRIB2::Grid::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

GRIB2::GridLatLon::GridLatLon(const Point& origin, const Point& ptsz, unsigned int usz, unsigned int vsz, int scu, int scv, int offs)
	: m_origin(origin), m_pointsize(ptsz), m_usize(usz), m_vsize(vsz), m_scaleu(scu), m_scalev(scv), m_offset(offs)
{
}

unsigned int GRIB2::GridLatLon::operator()(int u, int v) const
{
	if (u < 0)
		u = 0;
	else if ((unsigned int)u >= m_usize)
		u = m_usize - 1;
	if (v < 0)
		v = 0;
	else if ((unsigned int)v >= m_vsize)
		v = m_vsize - 1;
	return m_offset + u * m_scaleu + v * m_scalev;
}

Point GRIB2::GridLatLon::get_center(int u, int v) const
{
	if (u < 0)
		u = 0;
	else if ((unsigned int)u >= m_usize)
		u = m_usize - 1;
	if (v < 0)
		v = 0;
	else if ((unsigned int)v >= m_vsize)
		v = m_vsize - 1;
	return Point(m_origin.get_lon() + u * m_pointsize.get_lon(),
		     m_origin.get_lat() + v * m_pointsize.get_lat());
}

bool GRIB2::GridLatLon::operator==(const Grid& x) const
{
	const GridLatLon *xx(dynamic_cast<const GridLatLon *>(&x));
	if (!xx)
		return false;
	if (m_origin != xx->m_origin)
		return false;
	if (m_pointsize != xx->m_pointsize)
		return false;
	if (m_usize != xx->m_usize)
		return false;
	if (m_vsize != xx->m_vsize)
		return false;
	if (m_scaleu != xx->m_scaleu)
		return false;
	if (m_scalev != xx->m_scalev)
		return false;
	if (m_offset != xx->m_offset)
		return false;
	return true;
}

std::pair<float,float> GRIB2::GridLatLon::transform_axes(float u, float v) const
{
	std::pair<float,float> r(0, 0);
	if (m_scaleu > 0)
		r.first = u;
	else if (m_scaleu < 0)
		r.first = -u;
	if (m_scalev > 0)
		r.second = v;
	else if (m_scalev < 0)
		r.second = -v;
	return r;
}

#ifdef HAVE_LIBCRYPTO
GRIB2::Cache::Cache(const std::vector<uint8_t>& filedata)
{
	if (!filedata.size()) {
		memset(m_hash, 0, sizeof(m_hash));
		return;
	}
	MD4(&filedata[0], filedata.size(), m_hash);
}

GRIB2::Cache::Cache(const uint8_t *ptr, unsigned int len)
{
	if (!ptr || !len) {
		memset(m_hash, 0, sizeof(m_hash));
		return;
	}
	MD4(ptr, len, m_hash);
}

std::string GRIB2::Cache::get_filename(const std::string& cachedir) const
{
	if (cachedir.empty())
		return cachedir;
	std::ostringstream oss;
	oss << "jpeg2000." << std::hex;
	bool nz(false);
	for (unsigned int i(0); i < sizeof(m_hash); ++i) {
		nz = nz || m_hash[i];
		oss << std::setw(2) << std::setfill('0') << (unsigned int)m_hash[i];
	}
	if (!nz)
		return "";
	return Glib::build_filename(cachedir, oss.str());
}

bool GRIB2::Cache::load(const std::string& cachedir, std::vector<uint8_t>& data) const
{
	std::string filename(get_filename(cachedir));
	if (filename.empty())
		return false;
	int fd(open(filename.c_str(), O_RDONLY));
	if (fd == -1)
		return false;
	struct stat st;
	if (fstat(fd, &st)) {
		close(fd);
		return false;
	}
	if (!st.st_size || st.st_size > 1024*1024*128)
		return false;
	data.resize(st.st_size, 0);
	ssize_t r(read(fd, &data[0], data.size()));
	close(fd);
	return r == data.size();
}

void GRIB2::Cache::save(const std::string& cachedir, const uint8_t *ptr, unsigned int len)
{
	if (!ptr || !len)
		return;
	std::string filename(get_filename(cachedir));
	if (filename.empty())
		return;
	int fd(open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0755));
	if (fd == -1)
		return;
	ssize_t r(write(fd, ptr, len));
	close(fd);
	if (r != len)
		unlink(filename.c_str());
}

template <typename T> bool GRIB2::Cache::load(const std::string& cachedir, std::vector<T>& data, unsigned int typesz) const
{
	std::string filename(get_filename(cachedir));
	if (filename.empty())
		return false;
	int fd(open(filename.c_str(), O_RDONLY));
	if (fd == -1)
		return false;
	struct stat st;
	if (fstat(fd, &st)) {
		close(fd);
		return false;
	}
	if (!st.st_size || st.st_size > 1024*1024*128)
		return false;
	unsigned int sz(st.st_size / typesz);
	uint8_t d[st.st_size];
	ssize_t r(read(fd, d, st.st_size));
	close(fd);
	if (r != st.st_size)
		return false;
	data.resize(sz, 0);
	const uint8_t *dp = d;
	for (unsigned int i = 0; i < sz; ++i) {
		T d(0);
		for (unsigned int sh(0); sh < CHAR_BIT*typesz; sh += CHAR_BIT)
			d |= ((T)*dp++) << sh;
		if (typesz < sizeof(T) && (T)-1 < (T)0)
			d |= -(d & (((T)1) << (typesz * CHAR_BIT - 1)));
		data[i] = d;
	}
	return true;
}

template <typename T> void GRIB2::Cache::save(const std::string& cachedir, const T *ptr, unsigned int len, unsigned int typesz)
{
	if (!ptr || !len)
		return;
	std::string filename(get_filename(cachedir));
	if (filename.empty())
		return;
	int fd(open(filename.c_str(), O_CREAT | O_RDWR | O_TRUNC | O_EXCL, 0755));
	if (fd == -1)
		return;
	unsigned int blen(len * typesz);
	uint8_t d[blen];
	uint8_t *dp = d;
	for (unsigned int i = 0; i < len; ++i) {
		T d(ptr[i]);
		for (unsigned int j(0); j < typesz; ++j) {
			*dp++ = d;
			d >>= CHAR_BIT;
		}
	}
	ssize_t r(write(fd, d, blen));
	close(fd);
	if (r != blen)
		unlink(filename.c_str());
}

unsigned int GRIB2::Cache::expire(const std::string& cachedir, unsigned int maxdays, off_t maxbytes)
{
	if (cachedir.empty())
		return 0;
	time_t texp;
	time(&texp);
	texp -= maxdays*24*60*60;
	Glib::Dir d(cachedir);
	unsigned int ret(0);
	off_t bytes(0);
	typedef std::pair<std::string,off_t> file_t;
	typedef std::multimap<time_t,file_t> files_t;
	files_t files;
        for (;;) {
		std::string filename(d.read_name());
		if (filename.empty())
			break;
		filename = Glib::build_filename(cachedir, filename);
		struct stat st;
		if (stat(filename.c_str(), &st))
			continue;
		if (st.st_atime >= texp) {
			bytes += st.st_size;
			files.insert(files_t::value_type(st.st_atime, file_t(filename, st.st_size)));
			continue;
		}
		unlink(filename.c_str());
		++ret;
	}
	while (bytes > maxbytes && !files.empty()) {
		bytes -= std::min(bytes, files.begin()->second.second);
		unlink(files.begin()->second.first.c_str());
		files.erase(files.begin());
	}
	return ret;
}
#endif

GRIB2::Layer::Layer(const Parameter *param, const Glib::RefPtr<Grid const>& grid,
		    gint64 reftime, gint64 efftime, uint16_t centerid, uint16_t subcenterid,
		    uint8_t productionstatus, uint8_t datatype, uint8_t genprocess, uint8_t genprocesstype,
		    uint8_t surface1type, double surface1value, uint8_t surface2type, double surface2value)
	: m_refcount(1), m_grid(grid), m_reftime(reftime), m_efftime(efftime), m_cachetime(0),
	  m_parameter(param), m_surface1value(surface1value), m_surface2value(surface2value),
	  m_centerid(centerid), m_subcenterid(subcenterid), m_productionstatus(productionstatus), m_datatype(datatype),
	  m_genprocess(genprocess), m_genprocesstype(genprocesstype), m_surface1type(surface1type), m_surface2type(surface2type)
{
}

GRIB2::Layer::~Layer()
{
	m_expire.disconnect();
}

void GRIB2::Layer::reference(void) const
{
	readwrite();
        g_atomic_int_inc(&m_refcount);
	readonly();
}

void GRIB2::Layer::unreference(void) const
{
	readwrite();
        if (!g_atomic_int_dec_and_test(&m_refcount)) {
		readonly();
                return;
	}
	this->~Layer();
	operator delete(const_cast<GRIB2::Layer *>(this), LayerAlloc);
        //delete this;
}

void GRIB2::Layer::readonly(void) const
{
	LayerAllocator().readonly(this);
}

void GRIB2::Layer::readwrite(void) const
{
	LayerAllocator().readwrite(this);
}

void GRIB2::Layer::expire_time(long curtime)
{
	readwrite();
	Glib::Mutex::Lock lock(m_mutex);
	if (curtime < m_cachetime) {
		readonly();
		return;
	}
	m_cachetime = 0;
	m_data.clear();
	m_expire.disconnect();
	readonly();
}

void GRIB2::Layer::expire_now(void)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	expire_time(tv.tv_sec);
}

Glib::RefPtr<GRIB2::LayerResult> GRIB2::Layer::get_results(const Rect& bbox)
{
	if (!m_grid) {
		if (true)
			std::cerr << "GRIB2: layer has no grid" << std::endl;
		return Glib::RefPtr<LayerResult>();
	}
	readwrite();
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_expire.disconnect();
	}
	if (m_data.empty()) {
		load();
		if (m_data.empty()) {
			if (true)
				std::cerr << "GRIB2: cannot load layer" << std::endl;
			readonly();
			return Glib::RefPtr<LayerResult>();
		}
	}
	{
		Glib::TimeVal tv;
		tv.assign_current_time();
		tv.add_seconds(60);
		m_cachetime = tv.tv_sec;
	}
#if !defined(DEBUG_LAYERALLOC)
	{
		Glib::Mutex::Lock lock(m_mutex);
		m_expire = Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(*this, &Layer::expire_now), false), 65);
	}
#endif
	static const int64_t twopower32(((int64_t)1) << 32);
	Point ptsz(m_grid->get_pointsize());
	Point pthalfsz(ptsz.get_lon() / 2, ptsz.get_lat() / 2);
	Point ptorigin(m_grid->get_center(0, 0));
	unsigned int usize(m_grid->get_usize());
	unsigned int vsize(m_grid->get_vsize());
	if (false)
		std::cerr << "GRIB2: Grid: (" << usize << 'x' << vsize << ") pointsz " << ptsz.get_lat_str2()
			  << ' ' << ptsz.get_lon_str2() << " origin " << ptorigin.get_lat_str2()
			  << ' ' << ptorigin.get_lon_str2() << std::endl;
	int umin, umax, vmin, vmax;
	{
		int64_t by1((Point::coord_t)(bbox.get_south() - (uint32_t)(ptorigin.get_lat() - (uint32_t)pthalfsz.get_lat())));
		int64_t by2(bbox.get_north() - (int64_t)bbox.get_south());
		by2 += by1;
		vmin = by1 / ptsz.get_lat();
		vmax = (by2 + ptsz.get_lat() - 1) / ptsz.get_lat();
		if (vmin < 0)
			vmin = 0;
		else if (vmin >= (int)vsize)
			vmin = vsize - 1;
		if (vmax >= (int)vsize)
			vmax = vsize - 1;
		else if (vmax < vmin)
			vmax = vmin;
	}
	{
		int64_t bx1((Point::coord_t)(bbox.get_west() - (uint32_t)(ptorigin.get_lon() - (uint32_t)pthalfsz.get_lon())));
		if (bx1 < 0)
			bx1 += twopower32;
		int64_t bx2(bx1);
		bx2 += (uint32_t)(bbox.get_east() - (uint32_t)bbox.get_west());
		int64_t xw(ptsz.get_lon());
		xw *= usize;
		if (xw >= twopower32) {
			umin = bx1 / ptsz.get_lon();
			umax = (bx2 - bx1 + ptsz.get_lon() - 1) / ptsz.get_lon();
		} else {
			int64_t cx1(bx1);
			int64_t cx2(std::min(xw, bx2));
			int64_t dx1(std::max(twopower32, bx1));
			int64_t dx2(std::min(twopower32 + xw, bx2));
			if ((dx2 - dx1) > (cx2 - cx1)) {
				cx1 = dx1;
				cx2 = dx2;
			}
			umin = cx1 / ptsz.get_lon();
			umax = (cx2 - cx1 + ptsz.get_lon() - 1) / ptsz.get_lon();
		}
		if (umin < 0)
			umin = 0;
		else if (umin >= (int)usize)
			umin = usize - 1;
		if (umax < 0)
			umax = 0;
		else if (umax >= (int)usize)
			umax = usize - 1;
	}
	Rect laybbox(m_grid->get_center(umin, vmin) - pthalfsz, Point());
	laybbox.set_north(laybbox.get_south() + (vmax + 1 - vmin) * ptsz.get_lat());
	{
		int64_t x(ptsz.get_lon());
		x *= (umax + 1);
		if (x >= twopower32)
			x = twopower32 - 1;
		laybbox.set_east(laybbox.get_west() + x);
	}
	Glib::RefPtr<Layer const> thislayer(this);
	reference();
	Glib::RefPtr<LayerResult> lr(new (LayerAlloc) LayerResult(thislayer, laybbox, umax + 1, vmax + 1 - vmin, get_efftime(), get_reftime(), get_reftime(), get_surface1value()));
	umax += umin;
	if (umax >= (int)usize)
		umax -= usize;
	if (false)
		std::cerr << "GRIB2: query " << bbox << " result (" << umin << ".." << umax << ',' << vmin << ".." << vmax
			  << ") " << laybbox << std::endl;
	unsigned int vv(0);
	for (int v = vmin; v <= vmax; ++vv, ++v) {
		unsigned int uu(0);
		for (int u = umin; ; ++uu) {
			lr->operator()(uu, vv) = m_data[m_grid->operator()(u, v)];
			if (u == umax)
				break;
			++u;
			if (u >= (int)usize)
				u = 0;
		}
	}
	readonly();
	lr->readonly();
	return lr;
}

unsigned int GRIB2::Layer::extract(const std::vector<uint8_t>& filedata, unsigned int offs, unsigned int width)
{
	unsigned int r(0);
	while (width > 0) {
		unsigned int bi(offs >> 3);
		if (bi >= filedata.size())
			return 0;
		unsigned int bo(offs & 7);
		unsigned int w(std::min(width, 8 - bo));
		unsigned int m((1U << w) - 1U);
		r <<= w;
		r |= ((filedata[bi] << bo) >> (8 - w)) & m;
		offs += w;
		width -= w;
	}
	return r;
}

std::ostream& GRIB2::Layer::print_param(std::ostream& os)
{
	if (!m_parameter)
		return os;
	os << (unsigned int)m_parameter->get_id() << ':'
	   << (unsigned int)m_parameter->get_category_id() << ':'
	   << (unsigned int)m_parameter->get_discipline_id();
	if (m_parameter->get_str())
		os << ' ' << m_parameter->get_str();
	if (m_parameter->get_abbrev())
		os << " (" << m_parameter->get_abbrev() << ')';
	if (m_parameter->get_unit())
		os << " [" << m_parameter->get_unit() << ']';
	return os;
}

GRIB2::LayerJ2KParam::LayerJ2KParam(void)
	: m_datascale(std::numeric_limits<double>::quiet_NaN()), m_dataoffset(std::numeric_limits<double>::quiet_NaN())
{
}

GRIB2::LayerSimplePackingParam::LayerSimplePackingParam(void)
	: m_nbitsgroupref(0), m_fieldvaluetype(255)
{
}

GRIB2::LayerComplexPackingParam::LayerComplexPackingParam(void)
	: m_ngroups(0), m_refgroupwidth(0), m_nbitsgroupwidth(0),
	  m_refgrouplength(0), m_incrgrouplength(0), m_lastgrouplength(0), m_nbitsgrouplength(0),
	  m_primarymissingvalue(0), m_secondarymissingvalue(0), m_groupsplitmethod(255), m_missingvaluemgmt(255)
{
}

bool GRIB2::LayerComplexPackingParam::is_primarymissingvalue(void) const
{
	switch (get_missingvaluemgmt()) {
	case 1:
	case 2:
		return true;

	default:
		return false;
	}
}

bool GRIB2::LayerComplexPackingParam::is_secondarymissingvalue(void) const
{
	switch (get_missingvaluemgmt()) {
	case 2:
		return true;

	default:
		return false;
	}
}

unsigned int GRIB2::LayerComplexPackingParam::get_primarymissingvalue_raw(void) const
{
	if (is_primarymissingvalue())
		return (1U << get_nbitsgroupref()) - 1U;
	return 0;
}

unsigned int GRIB2::LayerComplexPackingParam::get_secondarymissingvalue_raw(void) const
{
	if (is_secondarymissingvalue())
		return (1U << get_nbitsgroupref()) - 2U;
	return 0;
}

double GRIB2::LayerComplexPackingParam::get_primarymissingvalue_float(void) const
{
	if (!is_fieldvalue_float())
		return get_primarymissingvalue();
	union {
		uint32_t u;
		float f;
	} u;
	u.u = get_primarymissingvalue();
	return u.f;
}

double GRIB2::LayerComplexPackingParam::get_secondarymissingvalue_float(void) const
{
	if (!is_fieldvalue_float())
		return get_secondarymissingvalue();
	union {
		uint32_t u;
		float f;
	} u;
	u.u = get_secondarymissingvalue();
	return u.f;
}

GRIB2::LayerComplexPackingSpatialDiffParam::LayerComplexPackingSpatialDiffParam(void)
	: m_spatialdifforder(0), m_extradescroctets(0)
{
}

#if defined(HAVE_OPENJPEG) && !defined(HAVE_OPENJPEG1)

namespace {

class OpjMemStream {
public:
	OpjMemStream(const std::vector<uint8_t>& filedata);
	~OpjMemStream();
	opj_stream_t *get_stream(void) { return m_stream; }

protected:
	const std::vector<uint8_t>& m_filedata;
	opj_stream_t *m_stream;
	OPJ_OFF_T m_offs;

	OPJ_SIZE_T opj_mem_stream_read(void *p_buffer, OPJ_SIZE_T p_nb_bytes);
	OPJ_SIZE_T opj_mem_stream_write(void *p_buffer, OPJ_SIZE_T p_nb_bytes);
	OPJ_OFF_T opj_mem_stream_skip(OPJ_OFF_T p_nb_bytes);
	OPJ_BOOL opj_mem_stream_seek(OPJ_OFF_T p_nb_bytes);
	void opj_mem_stream_free_user_data(void);

	static OPJ_SIZE_T opj_mem_stream_read_1(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data) {
		return ((OpjMemStream *)p_user_data)->opj_mem_stream_read(p_buffer, p_nb_bytes);
	}

	static OPJ_SIZE_T opj_mem_stream_write_1(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data) {
		return ((OpjMemStream *)p_user_data)->opj_mem_stream_write(p_buffer, p_nb_bytes);
	}

	static OPJ_OFF_T opj_mem_stream_skip_1(OPJ_OFF_T p_nb_bytes, void *p_user_data) {
		return ((OpjMemStream *)p_user_data)->opj_mem_stream_skip(p_nb_bytes);
	}

	static OPJ_BOOL opj_mem_stream_seek_1(OPJ_OFF_T p_nb_bytes, void *p_user_data) {
		return ((OpjMemStream *)p_user_data)->opj_mem_stream_seek(p_nb_bytes);
	}

	static void opj_mem_stream_free_user_data_1(void *p_user_data) {
		return ((OpjMemStream *)p_user_data)->opj_mem_stream_free_user_data();
	}
};

OpjMemStream::OpjMemStream(const std::vector<uint8_t>& filedata)
	: m_filedata(filedata), m_stream(0), m_offs(0)
{
	m_stream = opj_stream_create(m_filedata.size(), 1);
	if (!m_stream)
		return;
	opj_stream_set_user_data(m_stream, this, opj_mem_stream_free_user_data_1);
	opj_stream_set_user_data_length(m_stream, m_filedata.size());
	opj_stream_set_read_function(m_stream, opj_mem_stream_read_1);
	opj_stream_set_write_function(m_stream, opj_mem_stream_write_1);
	opj_stream_set_skip_function(m_stream, opj_mem_stream_skip_1);
	opj_stream_set_seek_function(m_stream, opj_mem_stream_seek_1);
}

OpjMemStream::~OpjMemStream()
{
	if (m_stream)
		opj_stream_destroy(m_stream);
}

OPJ_SIZE_T OpjMemStream::opj_mem_stream_read(void *p_buffer, OPJ_SIZE_T p_nb_bytes)
{
	OPJ_SIZE_T r = p_nb_bytes;
	if (m_offs + r > m_filedata.size())
		r = m_filedata.size() - m_offs;
	memcpy(p_buffer, &m_filedata[m_offs], r);
	m_offs += r;
	return r;
}

OPJ_SIZE_T OpjMemStream::opj_mem_stream_write(void *p_buffer, OPJ_SIZE_T p_nb_bytes)
{
	return 0;
}

OPJ_OFF_T OpjMemStream::opj_mem_stream_skip(OPJ_OFF_T p_nb_bytes)
{
	OPJ_SIZE_T r = p_nb_bytes;
	if (m_offs + r > m_filedata.size())
		r = m_filedata.size() - m_offs;
	m_offs += r;
	return r;
}

OPJ_BOOL OpjMemStream::opj_mem_stream_seek(OPJ_OFF_T p_nb_bytes)
{
	m_offs = p_nb_bytes;
	if (m_offs <= m_filedata.size())
		return OPJ_TRUE;
	m_offs = m_filedata.size();
	return OPJ_FALSE;
}

void OpjMemStream::opj_mem_stream_free_user_data(void)
{
}

};

#endif

GRIB2::LayerJ2K::LayerJ2K(const Parameter *param, const Glib::RefPtr<Grid const>& grid,
			  gint64 reftime, gint64 efftime, uint16_t centerid, uint16_t subcenterid,
			  uint8_t productionstatus, uint8_t datatype, uint8_t genprocess, uint8_t genprocesstype,
			  uint8_t surface1type, double surface1value, uint8_t surface2type, double surface2value,
			  const LayerJ2KParam& layerparam, goffset bitmapoffs, bool bitmap,
			  goffset fileoffset, gsize filesize, const std::string& filename,
			  const std::string& cachedir)
	: Layer(param, grid, reftime, efftime, centerid, subcenterid,
		productionstatus, datatype, genprocess, genprocesstype, surface1type, surface1value, surface2type, surface2value),
	  m_filename(filename), m_cachedir(cachedir), m_param(layerparam), m_bitmapoffset(bitmapoffs),
	  m_fileoffset(fileoffset), m_filesize(filesize), m_bitmap(bitmap)
{
}

void GRIB2::LayerJ2K::load(void)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	Glib::Mutex::Lock lock(m_mutex);
	if (m_cachetime > tv.tv_sec) {
		if (true)
			std::cerr << "GRIB2: do not load layer due to negative cache" << std::endl;
		return;
	}
	if (false)
		std::cerr << "Layer " << get_parameter()->get_abbrev_nonnull() << " load" << std::endl;
	m_data.clear();
	m_cachetime = std::numeric_limits<long>::max();
	if (!m_grid) {
		if (true)
			std::cerr << "GRIB2: layer has no grid" << std::endl;
		return;
	}
	if (!m_filesize)
		return;
	unsigned int usize(m_grid->get_usize());
        unsigned int vsize(m_grid->get_vsize());
	unsigned int uvsize(usize * vsize);
	if (!uvsize)
		return;
	std::vector<uint8_t> bitmap;
	std::vector<uint8_t> filedata;
	Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(m_filename));
	Glib::RefPtr<Gio::FileInputStream> instream(file->read());
	if (m_bitmap) {
		bitmap.resize((uvsize + 7) >> 3, 0);
		if (!instream->seek(m_bitmapoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to bitmap" << std::endl;
			return;
		}
		int r(instream->read(&bitmap[0], bitmap.size()));
		if (r != (int)bitmap.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read bitmap" << std::endl;
			return;
		}
	}
	{
		filedata.resize(m_filesize, 0);
		if (!instream->seek(m_fileoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to data" << std::endl;
			return;
		}
		int r(instream->read(&filedata[0], filedata.size()));
		if (r != (int)filedata.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read data" << std::endl;
			return;
		}
	}
	Cache cache(filedata);
	{
		std::vector<int> data;
		if (cache.load(m_cachedir, data)) {
			m_data.resize(uvsize, std::numeric_limits<float>::quiet_NaN());
			if (m_bitmap) {
				unsigned int imgsz(data.size());
				unsigned int imgptr(0);
				for (unsigned int i = 0; i < uvsize; ++i) {
					if (!((bitmap[i >> 3] << (i & 7)) & 0x80))
						continue;
					if (imgptr >= imgsz)
						break;
					m_data[i] = m_param.scale(data[imgptr++]);
				}
			} else {
				for (unsigned int i = 0, n = std::min(uvsize, (unsigned int)data.size()); i < n; ++i)
					m_data[i] = m_param.scale(data[i]);
			}
			if (false)
				std::cerr << "LayerJ2K: load (cached): dataoffset " << m_param.get_dataoffset()
					  << " datascale " << m_param.get_datascale() << " data " << m_param.scale(data[0])
					  << '(' << data[0] << ')' << std::endl;
			tv.add_seconds(60);
			m_cachetime = tv.tv_sec;
			return;
		}
	}
#ifdef HAVE_OPENJPEG
#ifdef HAVE_OPENJPEG1
	opj_common_struct_t cinfo;
	opj_codestream_info_t cstr_info;
	memset(&cinfo, 0, sizeof(cinfo));
	memset(&cstr_info, 0, sizeof(cstr_info));
	opj_cio_t *cio(opj_cio_open(&cinfo, &filedata[0], filedata.size()));
	opj_dinfo_t *dinfo(opj_create_decompress(CODEC_J2K));
	opj_dparameters_t param;
	opj_set_default_decoder_parameters(&param);
	opj_setup_decoder(dinfo, &param);
	opj_image_t *img(opj_decode_with_info(dinfo, cio, &cstr_info));
	opj_destroy_decompress(dinfo);
	opj_cio_close(cio);
	if (img->numcomps == 1 && (m_bitmap || (img->comps[0].w == cstr_info.image_w && img->comps[0].h == cstr_info.image_h))) {
		cache.save(m_cachedir, img->comps[0].data, cstr_info.image_w * cstr_info.image_h);
		m_data.resize(uvsize, std::numeric_limits<float>::quiet_NaN());
		if (m_bitmap) {
			unsigned int imgsz(cstr_info.image_w * cstr_info.image_h);
			unsigned int imgptr(0);
			for (unsigned int i = 0; i < uvsize; ++i) {
				if (!((bitmap[i >> 3] << (i & 7)) & 0x80))
					continue;
				if (imgptr >= imgsz)
					break;
				m_data[i] = m_param.scale(img->comps[0].data[imgptr++]);
			}
		} else {
			for (unsigned int i = 0; i < uvsize; ++i)
				m_data[i] = m_param.scale(img->comps[0].data[i]);
		}
		if (false)
			std::cerr << "LayerJ2K: load: dataoffset " << m_param.get_dataoffset()
				  << " datascale " << m_param.get_datascale() << " data " << m_param.scale(img->comps[0].data[0])
				  << '(' << img->comps[0].data[0] << ')' << std::endl;
	}
	opj_image_destroy(img);
	opj_destroy_cstr_info(&cstr_info);
#else
	opj_image_t *img(0);
	{
		OpjMemStream streamdata(filedata);
		opj_codec_t *codec(opj_create_decompress(OPJ_CODEC_J2K));
		if (codec) {
			opj_dparameters_t param;
			opj_set_default_decoder_parameters(&param);
			if (opj_setup_decoder(codec, &param)) {
				if (opj_read_header(streamdata.get_stream(), codec, &img)) {
					if (!opj_decode(codec, streamdata.get_stream(), img)) {
						opj_image_destroy(img);
						img = 0;
					}
				}
			}
			opj_destroy_codec(codec);
		}
	}
	if (img) {
		if (img->numcomps == 1) {
			cache.save(m_cachedir, img->comps[0].data, img->comps[0].w * img->comps[0].h);
			m_data.resize(uvsize, std::numeric_limits<float>::quiet_NaN());
			if (m_bitmap) {
				unsigned int imgsz(img->comps[0].w * img->comps[0].h);
				unsigned int imgptr(0);
				for (unsigned int i = 0; i < uvsize; ++i) {
					if (!((bitmap[i >> 3] << (i & 7)) & 0x80))
						continue;
					if (imgptr >= imgsz)
						break;
					m_data[i] = m_param.scale(img->comps[0].data[imgptr++]);
				}
			} else {
				for (unsigned int i = 0; i < uvsize; ++i)
					m_data[i] = m_param.scale(img->comps[0].data[i]);
			}
			if (false)
				std::cerr << "LayerJ2K: load: dataoffset " << m_param.get_dataoffset()
					  << " datascale " << m_param.get_datascale() << " data " << m_param.scale(img->comps[0].data[0])
					  << '(' << img->comps[0].data[0] << ')' << std::endl;
		}
		opj_image_destroy(img);
	}
#endif
#endif
	if (m_data.empty()) {
		if (true)
			std::cerr << "GRIB2: JPEG2000 data format error" << std::endl;
		return;
	}
	tv.add_seconds(60);
	m_cachetime = tv.tv_sec;
}

bool GRIB2::LayerJ2K::check_load(void)
{
	if (!m_filesize)
		return false;
	return Glib::file_test(m_filename, Glib::FILE_TEST_EXISTS) && !Glib::file_test(m_filename, Glib::FILE_TEST_IS_DIR);
}


GRIB2::LayerSimplePacking::LayerSimplePacking(const Parameter *param, const Glib::RefPtr<Grid const>& grid,
					      gint64 reftime, gint64 efftime, uint16_t centerid, uint16_t subcenterid,
					      uint8_t productionstatus, uint8_t datatype, uint8_t genprocess, uint8_t genprocesstype,
					      uint8_t surface1type, double surface1value, uint8_t surface2type, double surface2value,
					      const LayerSimplePackingParam& layerparam, goffset bitmapoffs, bool bitmap,
					      goffset fileoffset, gsize filesize, const std::string& filename)
	: Layer(param, grid, reftime, efftime, centerid, subcenterid,
		productionstatus, datatype, genprocess, genprocesstype, surface1type, surface1value, surface2type, surface2value),
	  m_filename(filename), m_param(layerparam), m_bitmapoffset(bitmapoffs),
	  m_fileoffset(fileoffset), m_filesize(filesize), m_bitmap(bitmap)
{
}

void GRIB2::LayerSimplePacking::load(void)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	Glib::Mutex::Lock lock(m_mutex);
	if (m_cachetime > tv.tv_sec) {
		if (true)
			std::cerr << "GRIB2: do not load layer due to negative cache" << std::endl;
		return;
	}
	if (false)
		std::cerr << "Layer " << get_parameter()->get_abbrev_nonnull() << " load" << std::endl;
	m_data.clear();
	m_cachetime = std::numeric_limits<long>::max();
	if (!m_grid) {
		if (true)
			std::cerr << "GRIB2: layer has no grid" << std::endl;
		return;
	}
	if (!m_filesize)
		return;
	unsigned int usize(m_grid->get_usize());
        unsigned int vsize(m_grid->get_vsize());
	unsigned int uvsize(usize * vsize);
	if (!uvsize)
		return;
	std::vector<uint8_t> bitmap;
	std::vector<uint8_t> filedata;
	Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(m_filename));
	Glib::RefPtr<Gio::FileInputStream> instream(file->read());
	if (m_bitmap) {
		bitmap.resize((uvsize + 7) >> 3, 0);
		if (!instream->seek(m_bitmapoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to bitmap" << std::endl;
			return;
		}
		int r(instream->read(&bitmap[0], bitmap.size()));
		if (r != (int)bitmap.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read bitmap" << std::endl;
			return;
		}
	}
	{
		filedata.resize(m_filesize, 0);
		if (!instream->seek(m_fileoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to data" << std::endl;
			return;
		}
		int r(instream->read(&filedata[0], filedata.size()));
		if (r != (int)filedata.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read data" << std::endl;
			return;
		}
	}
	m_data.resize(uvsize, std::numeric_limits<float>::quiet_NaN());
	{
		unsigned int ptr(0), width(m_param.get_nbitsgroupref());
		for (unsigned int i = 0; i < uvsize; ++i) {
			if (m_bitmap && !((bitmap[i >> 3] << (i & 7)) & 0x80))
				continue;
			m_data[i] = m_param.scale(extract(filedata, ptr, width));
			ptr += width;
		}
	}
}

bool GRIB2::LayerSimplePacking::check_load(void)
{
	if (!m_filesize)
		return false;
	if (!m_param.get_nbitsgroupref())
		return false;
	return Glib::file_test(m_filename, Glib::FILE_TEST_EXISTS) && !Glib::file_test(m_filename, Glib::FILE_TEST_IS_DIR);
}

GRIB2::LayerComplexPacking::LayerComplexPacking(const Parameter *param, const Glib::RefPtr<Grid const>& grid,
						gint64 reftime, gint64 efftime, uint16_t centerid, uint16_t subcenterid,
						uint8_t productionstatus, uint8_t datatype, uint8_t genprocess, uint8_t genprocesstype,
						uint8_t surface1type, double surface1value, uint8_t surface2type, double surface2value,
						const LayerComplexPackingParam& layerparam, goffset bitmapoffs, bool bitmap,
						goffset fileoffset, gsize filesize, const std::string& filename)
	: Layer(param, grid, reftime, efftime, centerid, subcenterid,
		productionstatus, datatype, genprocess, genprocesstype, surface1type, surface1value, surface2type, surface2value),
	  m_filename(filename), m_param(layerparam), m_bitmapoffset(bitmapoffs),
	  m_fileoffset(fileoffset), m_filesize(filesize), m_bitmap(bitmap)
{
}

void GRIB2::LayerComplexPacking::load(void)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	Glib::Mutex::Lock lock(m_mutex);
	if (m_cachetime > tv.tv_sec) {
		if (true)
			std::cerr << "GRIB2: do not load layer due to negative cache" << std::endl;
		return;
	}
	if (false)
		std::cerr << "Layer " << get_parameter()->get_abbrev_nonnull() << " load" << std::endl;
	m_data.clear();
	m_cachetime = std::numeric_limits<long>::max();
	if (!m_grid) {
		if (true)
			std::cerr << "GRIB2: layer has no grid" << std::endl;
		return;
	}
	if (!m_filesize)
		return;
	unsigned int usize(m_grid->get_usize());
        unsigned int vsize(m_grid->get_vsize());
	unsigned int uvsize(usize * vsize);
	if (!uvsize)
		return;
	std::vector<uint8_t> bitmap;
	std::vector<uint8_t> filedata;
	Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(m_filename));
	Glib::RefPtr<Gio::FileInputStream> instream(file->read());
	if (m_bitmap) {
		bitmap.resize((uvsize + 7) >> 3, 0);
		if (!instream->seek(m_bitmapoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to bitmap" << std::endl;
			return;
		}
		int r(instream->read(&bitmap[0], bitmap.size()));
		if (r != (int)bitmap.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read bitmap" << std::endl;
			return;
		}
	}
	{
		filedata.resize(m_filesize, 0);
		if (!instream->seek(m_fileoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to data" << std::endl;
			return;
		}
		int r(instream->read(&filedata[0], filedata.size()));
		if (r != (int)filedata.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read data" << std::endl;
			return;
		}
	}
	if (!m_param.is_gengroupsplit() || !m_param.get_ngroups())
		return;
	std::vector<unsigned int> grpref(m_param.get_ngroups(), 0), grpwidth(m_param.get_ngroups(), 0), grplength(m_param.get_ngroups(), 0);
	unsigned int ptr(0);
	for (unsigned int i(0), ng(m_param.get_ngroups()), grw(m_param.get_nbitsgroupref()); i < ng; ++i, ptr += grw)
		grpref[i] = extract(filedata, ptr, grw);
	ptr = (ptr + 7U) & ~7U;
	for (unsigned int i(0), ng(m_param.get_ngroups()), gww(m_param.get_nbitsgroupwidth()),
		     gwr(m_param.get_refgroupwidth()); i < ng; ++i, ptr += gww)
		grpwidth[i] = extract(filedata, ptr, gww) + gwr;
	ptr = (ptr + 7U) & ~7U;
	for (unsigned int i(0), ng(m_param.get_ngroups()), glw(m_param.get_nbitsgrouplength()),
		     glr(m_param.get_refgrouplength()), gli(m_param.get_incrgrouplength()); i < ng; ++i, ptr += glw)
		grplength[i] = extract(filedata, ptr, glw) * gli + glr;
	if (!grplength.empty())
		grplength.back() = m_param.get_lastgrouplength();
	ptr = (ptr + 7U) & ~7U;
	if (false) {
		unsigned int totlen(0), bmsz(0);
		for (std::vector<unsigned int>::const_iterator gi(grplength.begin()), ge(grplength.end()); gi != ge; ++gi)
			totlen += *gi;
		if (m_bitmap)
			for (unsigned int i = 0; i < uvsize; ++i)
				if ((bitmap[i >> 3] << (i & 7)) & 0x80)
					++bmsz;
		print_param(std::cerr << "GRIB2: LayerComplexPacking::load: param ")
			<< " surface " << (unsigned int)get_surface1type() << ' ' << get_surface1value()
			<< " / " << (unsigned int)get_surface2type() << ' ' << get_surface2value()
			<< " group length sum " << totlen << " bitmap " << bmsz << " uv " << uvsize << std::endl;
	}
	m_data.resize(uvsize, std::numeric_limits<float>::quiet_NaN());
	{
		const unsigned int primiss(m_param.get_primarymissingvalue_raw());
		const unsigned int secmiss(m_param.get_secondarymissingvalue_raw());
		const double primissfloat(m_param.get_primarymissingvalue_float());
		const double secmissfloat(m_param.get_secondarymissingvalue_float());
		unsigned int grpidx(0), grpcnt(1U), grpr(0U), grpw(1U), grpprimiss(~0U), grpsecmiss(~0U);
		if (grpidx < grplength.size()) {
			grpcnt = grplength[grpidx];
			grpr = grpref[grpidx];
			grpw = grpwidth[grpidx];
			if (grpw > 0 && m_param.is_primarymissingvalue())
				grpprimiss = (1U << grpw) - 1U;
			if (grpw > 1 && m_param.is_secondarymissingvalue())
				grpsecmiss = (1U << grpw) - 2U;
		}
		for (unsigned int i = 0; i < uvsize; ++i) {
			if (m_bitmap && !((bitmap[i >> 3] << (i & 7)) & 0x80))
				continue;
			unsigned int v(extract(filedata, ptr, grpw));
			if (v == grpprimiss) {
				m_data[i] = primissfloat;
				goto next;
			}
			if (v == grpsecmiss) {
				m_data[i] = secmissfloat;
				goto next;
			}
			v += grpr;
			if (v) {
				if (v == primiss) {
					m_data[i] = primissfloat;
					goto next;
				}
				if (v == secmiss) {
					m_data[i] = secmissfloat;
					goto next;
				}
			}
			m_data[i] = m_param.scale(v);
		  next:
			ptr += grpw;
			--grpcnt;
			if (grpcnt)
				continue;
			++grpidx;
			grpcnt = 1U;
			grpr = 0U;
			grpw = 1U;
			if (grpidx < grplength.size()) {
				grpcnt = grplength[grpidx];
				grpr = grpref[grpidx];
				grpw = grpwidth[grpidx];
			}
		}
	}
}

bool GRIB2::LayerComplexPacking::check_load(void)
{
	if (!m_filesize)
		return false;
	if (!m_param.is_gengroupsplit() || !m_param.get_ngroups())
		return false;
	return Glib::file_test(m_filename, Glib::FILE_TEST_EXISTS) && !Glib::file_test(m_filename, Glib::FILE_TEST_IS_DIR);
}

GRIB2::LayerComplexPackingSpatialDiff::LayerComplexPackingSpatialDiff(const Parameter *param, const Glib::RefPtr<Grid const>& grid,
								      gint64 reftime, gint64 efftime, uint16_t centerid, uint16_t subcenterid,
								      uint8_t productionstatus, uint8_t datatype, uint8_t genprocess, uint8_t genprocesstype,
								      uint8_t surface1type, double surface1value, uint8_t surface2type, double surface2value,
								      const LayerComplexPackingSpatialDiffParam& layerparam, goffset bitmapoffs, bool bitmap,
								      goffset fileoffset, gsize filesize, const std::string& filename)
	: Layer(param, grid, reftime, efftime, centerid, subcenterid,
		productionstatus, datatype, genprocess, genprocesstype, surface1type, surface1value, surface2type, surface2value),
	  m_filename(filename), m_param(layerparam), m_bitmapoffset(bitmapoffs),
	  m_fileoffset(fileoffset), m_filesize(filesize), m_bitmap(bitmap)
{
}

void GRIB2::LayerComplexPackingSpatialDiff::load(void)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
	Glib::Mutex::Lock lock(m_mutex);
	if (m_cachetime > tv.tv_sec) {
		if (true)
			std::cerr << "GRIB2: do not load layer due to negative cache" << std::endl;
		return;
	}
	if (false)
		std::cerr << "Layer " << get_parameter()->get_abbrev_nonnull() << " load" << std::endl;
	m_data.clear();
	m_cachetime = std::numeric_limits<long>::max();
	if (!m_grid) {
		if (true)
			std::cerr << "GRIB2: layer has no grid" << std::endl;
		return;
	}
	if (!m_filesize)
		return;
	unsigned int usize(m_grid->get_usize());
        unsigned int vsize(m_grid->get_vsize());
	unsigned int uvsize(usize * vsize);
	if (uvsize < m_param.get_spatialdifforder())
		return;
	std::vector<uint8_t> bitmap;
	std::vector<uint8_t> filedata;
	Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(m_filename));
	Glib::RefPtr<Gio::FileInputStream> instream(file->read());
	if (m_bitmap) {
		bitmap.resize((uvsize + 7) >> 3, 0);
		if (!instream->seek(m_bitmapoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to bitmap" << std::endl;
			return;
		}
		int r(instream->read(&bitmap[0], bitmap.size()));
		if (r != (int)bitmap.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read bitmap" << std::endl;
			return;
		}
	}
	{
		filedata.resize(m_filesize, 0);
		if (!instream->seek(m_fileoffset, Glib::SEEK_TYPE_SET)) {
			if (true)
				std::cerr << "GRIB2: cannot seek to data" << std::endl;
			return;
		}
		int r(instream->read(&filedata[0], filedata.size()));
		if (r != (int)filedata.size()) {
			if (true)
				std::cerr << "GRIB2: cannot read data" << std::endl;
			return;
		}
	}
	if (!m_param.is_gengroupsplit() || !m_param.get_ngroups() || m_param.get_spatialdifforder() > 2U)
		return;
	std::vector<unsigned int> grpref(m_param.get_ngroups(), 0), grpwidth(m_param.get_ngroups(), 0), grplength(m_param.get_ngroups(), 0);
	unsigned int ptr(0);
	std::vector<int> ddata;
	unsigned int diffinit[2] = { 0, 0 };
	{
		unsigned int w(m_param.get_extradescroctets() << 3);
		for (unsigned int i(0), n(m_param.get_spatialdifforder()); i < n; ++i) {
			diffinit[i] = extract(filedata, ptr, w);
			ptr += w;
		}
		unsigned int sgn(extract(filedata, ptr, 1));
		int ghmin(extract(filedata, ptr + 1, w - 1));
		if (sgn)
			ghmin = -ghmin;
		ptr += w;
		ddata.resize(uvsize, ghmin);
	}
	for (unsigned int i(0), ng(m_param.get_ngroups()), grw(m_param.get_nbitsgroupref()); i < ng; ++i, ptr += grw)
		grpref[i] = extract(filedata, ptr, grw);
	ptr = (ptr + 7U) & ~7U;
	for (unsigned int i(0), ng(m_param.get_ngroups()), gww(m_param.get_nbitsgroupwidth()),
		     gwr(m_param.get_refgroupwidth()); i < ng; ++i, ptr += gww)
		grpwidth[i] = extract(filedata, ptr, gww) + gwr;
	ptr = (ptr + 7U) & ~7U;
	for (unsigned int i(0), ng(m_param.get_ngroups()), glw(m_param.get_nbitsgrouplength()),
		     glr(m_param.get_refgrouplength()), gli(m_param.get_incrgrouplength()); i < ng; ++i, ptr += glw)
		grplength[i] = extract(filedata, ptr, glw) * gli + glr;
	if (!grplength.empty())
		grplength.back() = m_param.get_lastgrouplength();
	ptr = (ptr + 7U) & ~7U;
	if (false) {
		unsigned int totlen(0), bmsz(0);
		for (std::vector<unsigned int>::const_iterator gi(grplength.begin()), ge(grplength.end()); gi != ge; ++gi)
			totlen += *gi;
		if (m_bitmap)
			for (unsigned int i = 0; i < uvsize; ++i)
				if ((bitmap[i >> 3] << (i & 7)) & 0x80)
					++bmsz;
		print_param(std::cerr << "GRIB2: LayerComplexPackingSpatialDiff::load: param ")
			<< " surface " << (unsigned int)get_surface1type() << ' ' << get_surface1value()
			<< " / " << (unsigned int)get_surface2type() << ' ' << get_surface2value()
			<< " group length sum " << totlen << " bitmap " << bmsz << " uv " << uvsize << std::endl;
	}
	static const int ddataprimiss(std::numeric_limits<int>::min());
	static const int ddatasecmiss(std::numeric_limits<int>::min() + 1);
	static const int ddatamiss(std::numeric_limits<int>::min() + 2);
	{
		const unsigned int primiss(m_param.get_primarymissingvalue_raw());
		const unsigned int secmiss(m_param.get_secondarymissingvalue_raw());
		unsigned int grpidx(0), grpcnt(1U), grpr(0U), grpw(1U), grpprimiss(~0U), grpsecmiss(~0U);
		if (grpidx < grplength.size()) {
			grpcnt = grplength[grpidx];
			grpr = grpref[grpidx];
			grpw = grpwidth[grpidx];
			if (grpw > 0 && m_param.is_primarymissingvalue())
				grpprimiss = (1U << grpw) - 1U;
			if (grpw > 1 && m_param.is_secondarymissingvalue())
				grpsecmiss = (1U << grpw) - 2U;
		}
		for (unsigned int i = 0; i < uvsize; ++i) {
			if (m_bitmap && !((bitmap[i >> 3] << (i & 7)) & 0x80)) {
				ddata[i] = ddatamiss;
				continue;
			}
			unsigned int v(extract(filedata, ptr, grpw));
			if (v == grpprimiss) {
				ddata[i] = ddataprimiss;
				goto next;
			}
			if (v == grpsecmiss) {
				ddata[i] = ddatasecmiss;
				goto next;
			}
			v += grpr;
			if (v) {
				if (v == primiss) {
					ddata[i] = ddataprimiss;
					goto next;
				}
				if (v == secmiss) {
					ddata[i] = ddatasecmiss;
					goto next;
				}
			}
			ddata[i] += v;
		  next:
			ptr += grpw;
			--grpcnt;
			if (grpcnt)
				continue;
			++grpidx;
			grpcnt = 1U;
			grpr = 0U;
			grpw = 1U;
			grpprimiss = grpsecmiss = ~0U;
			if (grpidx < grplength.size()) {
				grpcnt = grplength[grpidx];
				grpr = grpref[grpidx];
				grpw = grpwidth[grpidx];
				if (grpw > 0 && m_param.is_primarymissingvalue())
					grpprimiss = (1U << grpw) - 1U;
				if (grpw > 1 && m_param.is_secondarymissingvalue())
					grpsecmiss = (1U << grpw) - 2U;
			}
		}
	}
	switch (m_param.get_spatialdifforder()) {
	case 1:
		if (ddata[0] > ddatamiss)
			ddata[0] = diffinit[0];
		for (unsigned int i = 1; i < uvsize; ++i) {
			int v(ddata[i]);
			if (v <= ddatamiss)
				continue;
			v += diffinit[0];
			diffinit[0] = v;
			ddata[i] = v;
		}
		break;

	case 2:
		if (ddata[0] > ddatamiss)
			ddata[0] = diffinit[0];
		if (ddata[1] > ddatamiss)
			ddata[1] = diffinit[1];
		for (unsigned int i = 2; i < uvsize; ++i) {
			int v(ddata[i]);
			if (v <= ddatamiss)
				continue;
			v += 2 * diffinit[1] - diffinit[0];
			diffinit[0] = diffinit[1];
			diffinit[1] = v;
			ddata[i] = v;
		}
		break;

	default:
		break;
	}
	m_data.resize(uvsize, std::numeric_limits<float>::quiet_NaN());
	const double primissfloat(m_param.get_primarymissingvalue_float());
	const double secmissfloat(m_param.get_secondarymissingvalue_float());
	for (unsigned int i = 0; i < uvsize; ++i) {
		switch (ddata[i]) {
		case ddataprimiss:
			m_data[i] = primissfloat;
			break;

		case ddatasecmiss:
			m_data[i] = secmissfloat;
			break;

		case ddatamiss:
			break;

		default:
			m_data[i] = m_param.scale(ddata[i]);
			break;
		}
	}
}

bool GRIB2::LayerComplexPackingSpatialDiff::check_load(void)
{
	if (!m_filesize)
		return false;
	if (!m_param.is_gengroupsplit() || !m_param.get_ngroups() || m_param.get_spatialdifforder() > 2U)
		return false;
	return Glib::file_test(m_filename, Glib::FILE_TEST_EXISTS) && !Glib::file_test(m_filename, Glib::FILE_TEST_IS_DIR);
}

const float GRIB2::LayerResult::nan(std::numeric_limits<float>::quiet_NaN());

GRIB2::LayerResult::LayerResult(const Glib::RefPtr<Layer const>& layer, const Rect& bbox, unsigned int w, unsigned int h, gint64 efftime, gint64 minreftime, gint64 maxreftime, double sfc1value)
	: m_refcount(1), m_layer(layer), m_bbox(bbox), m_width(w), m_height(h), m_efftime(efftime), m_minreftime(minreftime), m_maxreftime(maxreftime), m_surface1value(sfc1value)
{
	unsigned int sz(m_width * m_height);
	m_data.resize(sz, nan);
}

GRIB2::LayerResult::~LayerResult()
{
}

void GRIB2::LayerResult::reference(void) const
{
	readwrite();
        g_atomic_int_inc(&m_refcount);
	readonly();
}

void GRIB2::LayerResult::unreference(void) const
{
	readwrite();
        if (!g_atomic_int_dec_and_test(&m_refcount)) {
		readonly();
                return;
	}
	this->~LayerResult();
	operator delete(const_cast<GRIB2::LayerResult *>(this), LayerAlloc);
        //delete this;
}

void GRIB2::LayerResult::readonly(void) const
{
	LayerAllocator().readonly(this);
}

void GRIB2::LayerResult::readwrite(void) const
{
	LayerAllocator().readwrite(this);
}

const float& GRIB2::LayerResult::operator()(unsigned int x, unsigned int y) const
{
	if (x < m_width && y < m_height)
		return m_data[x + y * m_width];
	return nan;
}

float& GRIB2::LayerResult::operator()(unsigned int x, unsigned int y)
{
	if (x < m_width && y < m_height)
		return m_data[x + y * m_width];
	return const_cast<float&>(nan);
}

const float& GRIB2::LayerResult::operator[](unsigned int x) const
{
	if (x < m_data.size())
		return m_data[x];
	return nan;
}

float& GRIB2::LayerResult::operator[](unsigned int x)
{
	if (x < m_data.size())
		return m_data[x];
	return const_cast<float&>(nan);
}

float GRIB2::LayerResult::operator()(const Point& pt) const
{
	Point ptsz(get_pixelsize());
	uint32_t latd(pt.get_lat() - m_bbox.get_south());
	uint32_t lond(pt.get_lon() - m_bbox.get_west());
	unsigned int x(lond / ptsz.get_lon());
	unsigned int y(latd / ptsz.get_lat());
	lond -= x * ptsz.get_lon();
	latd -= y * ptsz.get_lat();
	float v[2][2];
	for (unsigned int xx = 0; xx < 2; ++xx) {
		uint32_t xxx(x + xx);
		if (xxx >= m_width) {
			for (unsigned int yy = 0; yy < 2; ++yy)
				v[xx][yy] = nan;
			continue;
		}
		for (unsigned int yy = 0; yy < 2; ++yy) {
			uint32_t yyy(y + yy);
			if (yyy >= m_height) {
				v[xx][yy] = nan;
				continue;
			}
			v[xx][yy] = operator()(xxx, yyy);
		}
	}
	double mx[2], my[2];
	mx[1] = lond / (float)ptsz.get_lon();
	mx[0] = 1 - mx[1];
	my[1] = latd / (float)ptsz.get_lat();
	my[0] = 1 - my[1];
	for (unsigned int xx = 0; xx < 2; ++xx) {
		if (mx[xx] < 0.5)
			continue;
		for (unsigned int yy = 0; yy < 2; ++yy)
			if (std::isnan(v[1 - xx][yy]))
				v[1 - xx][yy] = v[xx][yy];
	}
	for (unsigned int yy = 0; yy < 2; ++yy) {
		if (my[yy] < 0.5)
			continue;
		for (unsigned int xx = 0; xx < 2; ++xx)
			if (std::isnan(v[xx][1 - yy]))
				v[xx][1 - yy] = v[xx][yy];
	}
	float z(0);
	for (unsigned int xx = 0; xx < 2; ++xx)
		for (unsigned int yy = 0; yy < 2; ++yy) {
			if (std::isnan(v[xx][yy]))
				return nan;
			z += v[xx][yy] * mx[xx] * my[yy];
		}
	return z;
}

Point GRIB2::LayerResult::get_center(unsigned int x, unsigned int y) const
{
	Point ptsz(get_pixelsize());
	Point pt(ptsz.get_lon() / 2 + x * ptsz.get_lon(), ptsz.get_lat() / 2 + y * ptsz.get_lat());
	pt += m_bbox.get_southwest();
	return pt;
}

Point GRIB2::LayerResult::get_pixelsize(void) const
{
	if (!m_width || !m_height)
		return Point(0, 0);
	Point pt(m_bbox.get_northeast() - m_bbox.get_southwest());
	pt.set_lat(((uint32_t)pt.get_lat()) / m_height);
	pt.set_lon(((uint32_t)pt.get_lon()) / m_width);
	return pt;
}

GRIB2::LayerInterpolateResult::LinInterp::LinInterp(float p0, float p1, float p2, float p3)
{
	m_p[0] = p0;
	m_p[1] = p1;
	m_p[2] = p2;
	m_p[3] = p3;
}

float GRIB2::LayerInterpolateResult::LinInterp::operator()(const InterpIndex& idx) const
{
	return m_p[0] + m_p[1] * idx.get_idxtime() + m_p[2] * idx.get_idxsfc1value() + m_p[3] * idx.get_idxtime() * idx.get_idxsfc1value();
}

float GRIB2::LayerInterpolateResult::LinInterp::operator[](unsigned int idx) const
{
	if (idx >= 4)
		return 0;
	return m_p[idx];
}

GRIB2::LayerInterpolateResult::LinInterp& GRIB2::LayerInterpolateResult::LinInterp::operator+=(const LinInterp& x)
{
	for (unsigned int i = 0; i < 4; ++i)
		m_p[i] += x.m_p[i];
	return *this;
}

GRIB2::LayerInterpolateResult::LinInterp& GRIB2::LayerInterpolateResult::LinInterp::operator*=(float m)
{
	for (unsigned int i = 0; i < 4; ++i)
		m_p[i] *= m;
	return *this;
}

GRIB2::LayerInterpolateResult::LinInterp GRIB2::LayerInterpolateResult::LinInterp::operator+(const LinInterp& x) const
{
	LinInterp z(*this);
	z += x;
	return z;
}

GRIB2::LayerInterpolateResult::LinInterp GRIB2::LayerInterpolateResult::LinInterp::operator*(float m) const
{
	LinInterp z(*this);
	z *= m;
	return z;
}

bool GRIB2::LayerInterpolateResult::LinInterp::is_nan(void) const
{
	for (unsigned int i = 0; i < 4; ++i)
		if (std::isnan(m_p[i]))
			return true;
	return false;
}

const GRIB2::LayerInterpolateResult::LinInterp GRIB2::LayerInterpolateResult::nan(std::numeric_limits<float>::quiet_NaN(),
										  std::numeric_limits<float>::quiet_NaN(),
										  std::numeric_limits<float>::quiet_NaN(),
										  std::numeric_limits<float>::quiet_NaN());

GRIB2::LayerInterpolateResult::LayerInterpolateResult(const Glib::RefPtr<Layer const>& layer, const Rect& bbox, unsigned int w, unsigned int h,
						      gint64 minefftime, gint64 maxefftime, gint64 minreftime, gint64 maxreftime, double minsfc1value, double maxsfc1value)
	: m_refcount(1), m_layer(layer), m_bbox(bbox), m_width(w), m_height(h), m_minefftime(minefftime), m_maxefftime(maxefftime), m_minreftime(minreftime), m_maxreftime(maxreftime),
	  m_efftimemul(0), m_minsurface1value(minsfc1value), m_maxsurface1value(maxsfc1value), m_surface1valuemul(0)
{
	unsigned int sz(m_width * m_height);
	m_data.resize(sz, nan);
	if (m_maxefftime > m_minefftime)
		m_efftimemul = 1.0 / (double)(m_maxefftime - m_minefftime);
	{
		double x(m_maxsurface1value - m_minsurface1value);
		if (x > 0 && !std::isnan(x))
			m_surface1valuemul = 1.0 / x;
	}
}

GRIB2::LayerInterpolateResult::~LayerInterpolateResult()
{
}

void GRIB2::LayerInterpolateResult::reference(void) const
{
	readwrite();
        g_atomic_int_inc(&m_refcount);
	readonly();
}

void GRIB2::LayerInterpolateResult::unreference(void) const
{
	readwrite();
        if (!g_atomic_int_dec_and_test(&m_refcount)) {
		readonly();
                return;
	}
	this->~LayerInterpolateResult();
	operator delete(const_cast<GRIB2::LayerInterpolateResult *>(this), LayerAlloc);
        //delete this;
}

void GRIB2::LayerInterpolateResult::readonly(void) const
{
	LayerAllocator().readonly(this);
}

void GRIB2::LayerInterpolateResult::readwrite(void) const
{
	LayerAllocator().readwrite(this);
}

GRIB2::LayerInterpolateResult::InterpIndex GRIB2::LayerInterpolateResult::get_index(gint64 efftime, double sfc1value) const
{
	efftime = std::min(std::max(efftime, m_minefftime), m_maxefftime) - m_minefftime;
	sfc1value = std::min(std::max(sfc1value, m_minsurface1value), m_maxsurface1value) - m_minsurface1value;
	if (std::isnan(sfc1value))
		sfc1value = 0;
	InterpIndex idx(efftime * m_efftimemul, sfc1value * m_surface1valuemul);
	if (false)
		std::cerr << "get_index: efftime " << efftime << " sfc1value " << sfc1value
			  << " idxtime " << idx.get_idxtime() << " idxsfc1value " << idx.get_idxsfc1value() << std::endl;
	return idx;
}

GRIB2::LayerInterpolateResult::InterpIndex GRIB2::LayerInterpolateResult::get_index_efftime(gint64 efftime) const
{
	efftime = std::min(std::max(efftime, m_minefftime), m_maxefftime) - m_minefftime;
	InterpIndex idx(efftime * m_efftimemul, 0);
	if (false)
		std::cerr << "get_index: efftime " << efftime
			  << " idxtime " << idx.get_idxtime() << " idxsfc1value " << idx.get_idxsfc1value() << std::endl;
	return idx;
}

GRIB2::LayerInterpolateResult::InterpIndex GRIB2::LayerInterpolateResult::get_index_surface1value(double sfc1value) const
{
	sfc1value = std::min(std::max(sfc1value, m_minsurface1value), m_maxsurface1value) - m_minsurface1value;
	if (std::isnan(sfc1value))
		sfc1value = 0;
	InterpIndex idx(0, sfc1value * m_surface1valuemul);
	if (false)
		std::cerr << "get_index: sfc1value " << sfc1value
			  << " idxtime " << idx.get_idxtime() << " idxsfc1value " << idx.get_idxsfc1value() << std::endl;
	return idx;
}

float GRIB2::LayerInterpolateResult::operator()(unsigned int x, unsigned int y, gint64 efftime, double sfc1value) const
{
	return operator()(x, y, get_index(efftime, sfc1value));
}

float GRIB2::LayerInterpolateResult::operator()(unsigned int x, unsigned int y, const InterpIndex& idx) const
{
	if (false) {
		std::cerr << "(): x " << x << " y " << y
			  << " idxtime " << idx.get_idxtime() << " idxsfc1value " << idx.get_idxsfc1value();
		if (x < m_width && y < m_height) {
			std::cerr << " poly";
			for (unsigned int i = 0; i < 4; ++i)
				std::cerr << ' ' << m_data[x + y * m_width][i];
		}
		std::cerr << std::endl;
	}
	if (x < m_width && y < m_height)
		return m_data[x + y * m_width](idx);
       	return std::numeric_limits<float>::quiet_NaN();
}

const GRIB2::LayerInterpolateResult::LinInterp& GRIB2::LayerInterpolateResult::operator()(unsigned int x, unsigned int y) const
{
	if (x < m_width && y < m_height)
		return m_data[x + y * m_width];
	return nan;
}

GRIB2::LayerInterpolateResult::LinInterp& GRIB2::LayerInterpolateResult::operator()(unsigned int x, unsigned int y)
{
	if (x < m_width && y < m_height)
		return m_data[x + y * m_width];
	return const_cast<LinInterp&>(nan);
}

const GRIB2::LayerInterpolateResult::LinInterp& GRIB2::LayerInterpolateResult::operator[](unsigned int x) const
{
	if (x < m_data.size())
		return m_data[x];
	return nan;
}

GRIB2::LayerInterpolateResult::LinInterp& GRIB2::LayerInterpolateResult::operator[](unsigned int x)
{
	if (x < m_data.size())
		return m_data[x];
	return const_cast<LinInterp&>(nan);
}

GRIB2::LayerInterpolateResult::LinInterp GRIB2::LayerInterpolateResult::operator()(const Point& pt) const
{
	Point ptsz(get_pixelsize());
	uint32_t latd(pt.get_lat() - m_bbox.get_south());
	uint32_t lond(pt.get_lon() - m_bbox.get_west());
	unsigned int x(lond / ptsz.get_lon());
	unsigned int y(latd / ptsz.get_lat());
	lond -= x * ptsz.get_lon();
	latd -= y * ptsz.get_lat();
	LinInterp v[2][2];
	for (unsigned int xx = 0; xx < 2; ++xx) {
		uint32_t xxx(x + xx);
		if (xxx >= m_width) {
			for (unsigned int yy = 0; yy < 2; ++yy)
				v[xx][yy] = nan;
			continue;
		}
		for (unsigned int yy = 0; yy < 2; ++yy) {
			uint32_t yyy(y + yy);
			if (yyy >= m_height) {
				v[xx][yy] = nan;
				continue;
			}
			v[xx][yy] = operator()(xxx, yyy);
		}
	}
	double mx[2], my[2];
	mx[1] = lond / (float)ptsz.get_lon();
	mx[0] = 1 - mx[1];
	my[1] = latd / (float)ptsz.get_lat();
	my[0] = 1 - my[1];
	for (unsigned int xx = 0; xx < 2; ++xx) {
		if (mx[xx] < 0.5)
			continue;
		for (unsigned int yy = 0; yy < 2; ++yy)
			if (v[1 - xx][yy].is_nan())
				v[1 - xx][yy] = v[xx][yy];
	}
	for (unsigned int yy = 0; yy < 2; ++yy) {
		if (my[yy] < 0.5)
			continue;
		for (unsigned int xx = 0; xx < 2; ++xx)
			if (v[xx][1 - yy].is_nan())
				v[xx][1 - yy] = v[xx][yy];
	}
	LinInterp z(0);
	for (unsigned int xx = 0; xx < 2; ++xx)
		for (unsigned int yy = 0; yy < 2; ++yy) {
			if (v[xx][yy].is_nan())
				return nan;
			z += v[xx][yy] * (mx[xx] * my[yy]);
		}
	if (false && z.is_nan()) {
		std::cerr << "LayerInterpolateResult: " << z[0] << ' ' << z[1] << ' ' << z[2] << ' ' << z[3]
			  << " x " << x << " y " << y << " mx " << mx[1] << " my " << my[1] << std::endl;
	        for (unsigned int xx = 0; xx < 2; ++xx)
			for (unsigned int yy = 0; yy < 2; ++yy)
				std::cerr << "  " << xx << ',' << yy << ": " << v[xx][yy][0] << ' ' << v[xx][yy][1]
					  << ' ' << v[xx][yy][2] << ' ' << v[xx][yy][3] << std::endl;
	}
	return z;
}

float GRIB2::LayerInterpolateResult::operator()(const Point& pt, gint64 efftime, double sfc1value) const
{
	return operator()(pt, get_index(efftime, sfc1value));
}

float GRIB2::LayerInterpolateResult::operator()(const Point& pt, const InterpIndex& idx) const
{
	LinInterp z(this->operator()(pt));
	if (false) {
		std::cerr << "(): pt " << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
			  << " idxtime " << idx.get_idxtime() << " idxsfc1value " << idx.get_idxsfc1value() << " poly";
			for (unsigned int i = 0; i < 4; ++i)
				std::cerr << ' ' << z[i];
		std::cerr << std::endl;
	}
	return z(idx);
}

Point GRIB2::LayerInterpolateResult::get_center(unsigned int x, unsigned int y) const
{
	Point ptsz(get_pixelsize());
	Point pt(ptsz.get_lon() / 2 + x * ptsz.get_lon(), ptsz.get_lat() / 2 + y * ptsz.get_lat());
	pt += m_bbox.get_southwest();
	return pt;
}

Point GRIB2::LayerInterpolateResult::get_pixelsize(void) const
{
	if (!m_width || !m_height)
		return Point(0, 0);
	Point pt(m_bbox.get_northeast() - m_bbox.get_southwest());
	pt.set_lat(((uint32_t)pt.get_lat()) / m_height);
	pt.set_lon(((uint32_t)pt.get_lon()) / m_width);
	return pt;
}

Glib::RefPtr<GRIB2::LayerResult> GRIB2::LayerInterpolateResult::get_results(gint64 efftime, double sfc1value)
{
	efftime = std::min(std::max(efftime, m_minefftime), m_maxefftime);
	sfc1value = std::min(std::max(sfc1value, m_minsurface1value), m_maxsurface1value);
	InterpIndex idx(get_index(efftime, sfc1value));
	Glib::RefPtr<LayerResult> r(new (LayerAlloc) LayerResult(get_layer(), get_bbox(), get_width(), get_height(), efftime, get_minreftime(), get_maxreftime(), sfc1value));
	for (data_t::size_type i(0), sz(m_data.size()); i < sz; ++i)
		r->operator[](i) = m_data[i](idx);
	return r;
}

int GRIB2::LayerPtr::compare(const LayerPtr& x) const
{
	if (m_layer) {
		if (!x.m_layer)
			return -1;
	} else {
		if (x.m_layer)
			return 1;
		return 0;
	}
	{
		const Parameter *a(m_layer->get_parameter());
		const Parameter *b(x.m_layer->get_parameter());
		if (a) {
			if (!b)
				return -1;
			int c(a->compareid(*b));
			if (c)
				return c;
		} else {
			if (b)
				return 1;
		}
	}
	{
		uint8_t a(m_layer->get_surface1type());
		uint8_t b(x.m_layer->get_surface1type());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	{
		double a(m_layer->get_surface1value());
		double b(x.m_layer->get_surface1value());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	{
		uint8_t a(m_layer->get_surface2type());
		uint8_t b(x.m_layer->get_surface2type());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	{
		double a(m_layer->get_surface2value());
		double b(x.m_layer->get_surface2value());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	{
		gint64 a(m_layer->get_efftime());
		gint64 b(x.m_layer->get_efftime());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	{
		gint64 a(m_layer->get_reftime());
		gint64 b(x.m_layer->get_reftime());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	return 0;
}

int GRIB2::LayerPtrSurface::compare(const LayerPtrSurface& x) const
{
	if (m_layer) {
		if (!x.m_layer)
			return -1;
	} else {
		if (x.m_layer)
			return 1;
		return 0;
	}
	{
		const Parameter *a(m_layer->get_parameter());
		const Parameter *b(x.m_layer->get_parameter());
		if (a) {
			if (!b)
				return -1;
			int c(a->compareid(*b));
			if (c)
				return c;
		} else {
			if (b)
				return 1;
		}
	}
	{
		uint8_t a(m_layer->get_surface1type());
		uint8_t b(x.m_layer->get_surface1type());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	{
		double a(m_layer->get_surface1value());
		double b(x.m_layer->get_surface1value());
		if (a < b)
			return -1;
		if (b < a)
			return 1;
	}
	return 0;
}

GRIB2::Parser::Parser(GRIB2& gr2)
	: m_grib2(gr2),
	  m_surface1value(std::numeric_limits<double>::quiet_NaN()),
	  m_surface2value(std::numeric_limits<double>::quiet_NaN()),
	  m_bitmapoffs(0), m_bitmapsize(0), m_nrdatapoints(0),
	  m_centerid(0xffff), m_subcenterid(0xffff), m_datarepresentation(0xffff), m_productionstatus(0xff),
	  m_datatype(0xff), m_productdiscipline(0xff), m_paramcategory(0xff), m_paramnumber(0xff),
	  m_genprocess(0xff), m_genprocesstype(0xff), m_surface1type(0xff), m_surface2type(0xff), m_bitmap(false)
{
}

int GRIB2::Parser::parse_file(const std::string& filename)
{
       	Glib::RefPtr<Gio::File> file(Gio::File::create_for_path(filename));
	Glib::RefPtr<Gio::FileInputStream> instream(file->read());
	uint8_t buf[4096];
	unsigned int bufsz(0);
	int layers(0);
	int err(0);
	do {
		{
			gssize r(instream->read(&buf[bufsz], sizeof(buf) - bufsz));
			if (r > 0)
				bufsz += r;
		}
		if (!bufsz)
			break;
		// check section0 size
		if (bufsz < 16) {
			err = -1;
			break;
		}
		// check signature
		if (memcmp(&buf[0], &grib2_header[0], sizeof(grib2_header))) {
			err = -2;
			break;
		}
		// check GRIB version
		if (buf[7] != 0x02) {
			err = -3;
			break;
		}
		uint64_t file_len(buf[15] | (((uint32_t)buf[14]) << 8) |
				  (((uint32_t)buf[13]) << 16) | (((uint32_t)buf[12]) << 24) |
				  (((uint64_t)buf[11]) << 32) | (((uint64_t)buf[10]) << 40) |
				  (((uint64_t)buf[9]) << 48) | (((uint64_t)buf[8]) << 56));
		m_productdiscipline = buf[6];
		bufsz -= 16;
		if (bufsz)
			memmove(&buf[0], &buf[16], bufsz);
		m_grid.reset();
		m_reftime = 0;
		m_efftime = 0;
		m_surface1value = std::numeric_limits<double>::quiet_NaN();
		m_surface2value = std::numeric_limits<double>::quiet_NaN();
		m_layerparam = LayerComplexPackingSpatialDiffParam();
		m_centerid = 0xffff;
		m_subcenterid = 0xffff;
		m_datarepresentation = 0xffff;
		m_bitmapoffs = 0;
		m_bitmapsize = 0;
		m_bitmap = false;
		m_nrdatapoints = 0;
		m_productionstatus = 0xff;
		m_datatype = 0xff;
		m_paramcategory = 0xff;
		m_paramnumber = 0xff;
		m_genprocess = 0xff;
		m_genprocesstype = 0xff;
		m_surface1type = 0xff;
		m_surface2type = 0xff;
		do {
			{
				gssize r(instream->read(&buf[bufsz], sizeof(buf) - bufsz));
				if (r > 0)
					bufsz += r;
			}
			if (bufsz < 4) {
				err = -4;
				break;
			}
			uint32_t len(buf[3] | (((uint32_t)buf[2]) << 8) |
				     (((uint32_t)buf[1]) << 16) | (((uint32_t)buf[0]) << 24));
			if (len == grib2_terminator) {
				bufsz -= 4;
				if (bufsz)
					memmove(&buf[0], &buf[4], bufsz);
				break;
			}
			if (bufsz < 5) {
				err = -4;
				break;
			}
			switch (buf[4]) {
			case 1:
			{
				err = section1(buf + 5, len - 5);
				if (err >= 0) {
					layers += err;
					err = 0;
				}
				break;
			}

			case 3:
		        {
				err = section3(buf + 5, len - 5);
				if (err >= 0) {
					layers += err;
					err = 0;
				}
				break;
			}

			case 4:
			{
				err = section4(buf + 5, len - 5);
				if (err >= 0) {
					layers += err;
					err = 0;
				}
				break;
			}

			case 5:
			{
				err = section5(buf + 5, len - 5);
				if (err >= 0) {
					layers += err;
					err = 0;
				}
				break;
			}

			case 6:
			{
				err = section6(buf + 5, len - 5, instream->tell() + 5 - bufsz);
				if (err >= 0) {
					layers += err;
					err = 0;
				}
				break;
			}

			case 7:
			{
				err = section7(buf + 5, len - 5, instream->tell() + 5 - bufsz, filename);
				if (err >= 0) {
					layers += err;
					err = 0;
				}
				break;
			}

			default:
				break;
			}
			if (true && err) {
				unsigned int l1(len);
				l1 = std::min(l1, bufsz);
				l1 = std::min(l1, 64U);
				std::ostringstream oss;
				oss << std::hex;
				for (unsigned int i = 0; i < l1; ++i) {
					if (!(i & 15))
						oss << std::endl << std::setfill('0') << std::setw(4) << i << ':';
					oss << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)buf[i];
				}
				std::cerr << "GRIB2: " << filename << ':' << (instream->tell() + 5 - bufsz)
					  << ": error " << err << " in section " << (unsigned int)buf[4]
					  << "(section length " << len << ")" << oss.str() << std::endl;
			}
			if (len > bufsz) {
				len -= bufsz;
				bufsz = 0;
				instream->skip(len);
			} else {
				bufsz -= len;
				if (bufsz)
					memmove(&buf[0], &buf[len], bufsz);
			}
		} while (!err);
	} while (!err);
	if (layers > 0)
		return layers;
	return err;
}

int GRIB2::Parser::section1(const uint8_t *buf, uint32_t len)
{
	m_reftime = 0;
	m_centerid = 0xffff;
	m_subcenterid = 0xffff;
	m_productionstatus = 0xff;
	m_datatype = 0xff;
	if (len < 16)
		return -101;
	m_centerid = buf[1] | (((unsigned int)buf[0]) << 8);
	m_subcenterid = buf[3] | (((unsigned int)buf[2]) << 8);
	m_productionstatus = buf[14];
	m_datatype = buf[15];
	m_reftime = Glib::DateTime::create_utc((((unsigned int)buf[7]) << 8) | buf[8], buf[9], buf[10], buf[11], buf[12], buf[13]).to_unix();
	return 0;
}

int GRIB2::Parser::section3(const uint8_t *buf, uint32_t len)
{
	m_grid.reset();
	if (len < 10)
		return -301;
	if (buf[0]) {
		std::cerr << "Invalid Grid Source" << std::endl;
		return 0;
	}
	if (buf[5] || buf[6]) {
		std::cerr << "Quasi-Regular Grids not supported" << std::endl;
		return 0;
	}
	uint32_t ndata(buf[4] | (((unsigned int)buf[3]) << 8) |
		       (((unsigned int)buf[2]) << 16) | (((unsigned int)buf[1]) << 24));
	uint16_t gridtempl(buf[8] | (((unsigned int)buf[7]) << 8));
	switch (gridtempl) {
	default:
		std::cerr << "Unsupported Grid " << gridtempl << " - only Lat/Lon Grids supported" << std::endl;
		return 0;

	case 0:
	{
		if (len < 67)
			return -301;
		uint32_t radiusearth(buf[14] | (((uint32_t)buf[13]) << 8) |
				     (((uint32_t)buf[12]) << 16) | (((uint32_t)buf[11]) << 24));
		uint32_t majoraxis(buf[19] | (((uint32_t)buf[18]) << 8) |
				   (((uint32_t)buf[17]) << 16) | (((uint32_t)buf[16]) << 24));
		uint32_t minoraxis(buf[24] | (((uint32_t)buf[23]) << 8) |
				   (((uint32_t)buf[22]) << 16) | (((uint32_t)buf[21]) << 24));
		uint32_t Ni(buf[28] | (((uint32_t)buf[27]) << 8) |
			    (((uint32_t)buf[26]) << 16) | (((uint32_t)buf[25]) << 24));
		uint32_t Nj(buf[32] | (((uint32_t)buf[31]) << 8) |
			    (((uint32_t)buf[30]) << 16) | (((uint32_t)buf[29]) << 24));
		uint32_t basicangle(buf[36] | (((uint32_t)buf[35]) << 8) |
				    (((uint32_t)buf[34]) << 16) | (((uint32_t)buf[33]) << 24));
		uint32_t subdivbasicangle(buf[40] | (((uint32_t)buf[39]) << 8) |
					  (((uint32_t)buf[38]) << 16) | (((uint32_t)buf[37]) << 24));
		int32_t lat1(buf[44] | (((uint32_t)buf[43]) << 8) |
			     (((uint32_t)buf[42]) << 16) | (((uint32_t)buf[41]) << 24));
		int32_t lon1(buf[48] | (((uint32_t)buf[47]) << 8) |
			     (((uint32_t)buf[46]) << 16) | (((uint32_t)buf[45]) << 24));
		int32_t lat2(buf[53] | (((uint32_t)buf[52]) << 8) |
			     (((uint32_t)buf[51]) << 16) | (((uint32_t)buf[50]) << 24));
		int32_t lon2(buf[57] | (((uint32_t)buf[56]) << 8) |
			     (((uint32_t)buf[55]) << 16) | (((uint32_t)buf[54]) << 24));
		int32_t Di(buf[61] | (((uint32_t)buf[60]) << 8) |
			   (((uint32_t)buf[59]) << 16) | (((uint32_t)buf[58]) << 24));
		int32_t Dj(buf[65] | (((uint32_t)buf[64]) << 8) |
			   (((uint32_t)buf[63]) << 16) | (((uint32_t)buf[62]) << 24));
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
		// we do not (yet) care about the exact earth definition
		// FIXME: subdivbasicangle should not be zero, old GFS data
		if (basicangle || (subdivbasicangle != 0 && subdivbasicangle != (uint32_t)~0UL)) {
			std::cerr << "Grid: basic angle/subdivisions not supported" << std::endl;
			return 0;
		}
		if (Ni * Nj != ndata) {
			std::cerr << "Grid: Ni * Nj does not match number of data points: " << Ni << '*' << Nj << "!=" << ndata << std::endl;
			return 0;
		}
		if (Ni < 2 || Nj < 2) {
			std::cerr << "Grid: Ni / Nj must be at least 2" << std::endl;
			return 0;
		}
		if (buf[66] & 16) {
			std::cerr << "Grid: opposite row scanning not supported" << std::endl;
			return 0;
		}
		if (buf[49] & 8) {
			std::cerr << "Grid: u/v vector components not aligned to easterly/northerly direction" << std::endl;
			return 0;
		}
		Point origin;
		Point ptsz;
		int scu((buf[66] & 32) ? Nj : 1);
		int scv((buf[66] & 32) ? 1 : Ni);
		int offs(0);
		origin.set_lat_deg_dbl(((buf[66] & 64) ? lat1 : lat2) * 1e-6);
		origin.set_lon_deg_dbl(((buf[66] & 128) ? lon2 : lon1) * 1e-6);
		if (!(buf[49] & 32)) {
			int32_t ld(lon2 - lon1);
			if (ld < 0)
				ld += 360000000;
			if (ld >= 360000000)
				ld -= 360000000;
			Di = (ld + (Ni - 1)/2) / (Ni - 1);
		}
		if (!(buf[49] & 16))
			Dj = ((buf[66] & 64) ? lat2 - lat1 : lat1 - lat2) / (Nj - 1);
		ptsz.set_lat(static_cast<Point::coord_t>(ceil(abs(Dj) * (1e-6 * Point::from_deg_dbl))));
		ptsz.set_lon(static_cast<Point::coord_t>(ceil(abs(Di) * (1e-6 * Point::from_deg_dbl))));
		if (buf[66] & 128) {
			offs += scu * (Ni - 1);
			scu = -scu;
		}
		if (!(buf[66] & 64)) {
			offs += scv * (Nj - 1);
			scv = -scv;
		}
		m_grid = Glib::RefPtr<GridLatLon>(new GridLatLon(origin, ptsz, Ni, Nj, scu, scv, offs));
		if (m_lastgrid && *(m_lastgrid.operator->()) == *(m_grid.operator->()))
			m_grid = m_lastgrid;
		m_lastgrid = m_grid;
		break;
	}
	}
	return 0;
}

int GRIB2::Parser::section4(const uint8_t *buf, uint32_t len)
{
	m_efftime = 0;
	m_paramcategory = 0xff;
	m_paramnumber = 0xff;
	m_genprocess = 0xff;
	m_genprocesstype = 0xff;
	m_surface1type = 0xff;
	m_surface2type = 0xff;
	m_surface1value = std::numeric_limits<double>::quiet_NaN();
	m_surface2value = std::numeric_limits<double>::quiet_NaN();
	if (len < 4)
		return -401;
	uint16_t nrcoordval(buf[1] | (((unsigned int)buf[0]) << 8));
	if (nrcoordval) {
		std::cerr << "Product Definition: Coordinate Values not supported" << std::endl;
		return 0;
	}
	uint16_t proddeftempl(buf[3] | (((unsigned int)buf[2]) << 8));
	switch (proddeftempl) {
	default:
		std::cerr << "Unsupported Product definition template " << proddeftempl << std::endl;
		return 0;

	case 0:
	case 8:
	{
		if (len < 29)
			return -401;
		uint16_t hrcutoff(buf[10] | (((unsigned int)buf[9]) << 8));
		uint32_t fcsttime(buf[16] | (((uint32_t)buf[15]) << 8) |
				  (((uint32_t)buf[14]) << 16) | (((uint32_t)buf[13]) << 24));
		uint32_t fixsfc1(buf[22] | (((uint32_t)buf[21]) << 8) |
				 (((uint32_t)buf[20]) << 16) | (((uint32_t)buf[19]) << 24));
		uint32_t fixsfc2(buf[28] | (((uint32_t)buf[27]) << 8) |
				 (((uint32_t)buf[26]) << 16) | (((uint32_t)buf[25]) << 24));
		switch (buf[12]) {
		case 0: // Minute
			m_efftime = m_reftime + 60 * fcsttime;
			break;

		case 1: // Hour
			m_efftime = m_reftime + (60 * 60) * fcsttime;
			break;

		case 2: // Day
			m_efftime = m_reftime + (24 * 60 * 60) * fcsttime;
			break;

		case 3: // Month
			m_efftime = Glib::DateTime::create_now_utc(m_reftime).add_months(fcsttime).to_unix();
			break;

		case 4: // Year
			m_efftime = Glib::DateTime::create_now_utc(m_reftime).add_years(fcsttime).to_unix();
			break;

		case 5: // Decade
			m_efftime = Glib::DateTime::create_now_utc(m_reftime).add_years(10 * fcsttime).to_unix();
			break;

		case 6: // Normal (30 Years)
			m_efftime = Glib::DateTime::create_now_utc(m_reftime).add_years(30 * fcsttime).to_unix();
			break;

		case 7: // Century
			m_efftime = Glib::DateTime::create_now_utc(m_reftime).add_years(100 * fcsttime).to_unix();
			break;

		case 10: // 3 Hours
			m_efftime = m_reftime + (3 * 60 * 60) * fcsttime;
			break;

		case 11: // 6 Hours
			m_efftime = m_reftime + (6 * 60 * 60) * fcsttime;
			break;

		case 12: // 12 Hours
			m_efftime = m_reftime + (12 * 60 * 60) * fcsttime;
			break;

		case 13: // Second
			m_efftime = m_reftime + fcsttime;
			break;

		default:
			std::cerr << "Unsupported Forecast Time Unit Indicator " << (unsigned int)buf[12] << std::endl;
			return 0;
		}
		m_paramcategory = buf[4];
		m_paramnumber = buf[5];
		m_genprocess = buf[6];
		m_genprocesstype = buf[8];
		m_surface1type = buf[17];
		m_surface2type = buf[23];
		m_surface1value = fixsfc1 * exp((-M_LN10) * (int8_t)buf[18]);
		m_surface2value = fixsfc2 * exp((-M_LN10) * (int8_t)buf[24]);
		if (m_surface1type == surface_missing)
			m_surface1value = 0;
		if (m_surface2type == surface_missing)
			m_surface2value = 0;
		if (!proddeftempl)
			break;
		if (len < 41)
			return -401;
		Glib::DateTime intvlend(Glib::DateTime::create_utc((((unsigned int)buf[29]) << 8) | buf[30], buf[31], buf[32],
								   buf[33], buf[34], buf[35]));
		uint32_t valmissing(buf[40] | (((uint32_t)buf[39]) << 8) |
				    (((uint32_t)buf[38]) << 16) | (((uint32_t)buf[37]) << 24));
		if (len < 41U + 12U * buf[36])
			return -401;
		for (unsigned int j = 0; j < buf[36]; ++j) {
			uint32_t timerange(buf[47 + j*12] | (((uint32_t)buf[46 + j*12]) << 8) |
					   (((uint32_t)buf[45 + j*12]) << 16) | (((uint32_t)buf[44 + j*12]) << 24));
			uint32_t timeincr(buf[52 + j*12] | (((uint32_t)buf[51 + j*12]) << 8) |
					  (((uint32_t)buf[50 + j*12]) << 16) | (((uint32_t)buf[49 + j*12]) << 24));
		}
		break;
	}
	}
	return 0;
}

int GRIB2::Parser::section5(const uint8_t *buf, uint32_t len)
{
	m_nrdatapoints = 0;
	m_layerparam = LayerComplexPackingSpatialDiffParam();
	m_datarepresentation = 0xffff;
	if (len < 6)
		return -501;
        m_nrdatapoints = buf[3] | (((unsigned int)buf[2]) << 8) |
		(((uint32_t)buf[1]) << 16) | (((uint32_t)buf[0]) << 24);
        m_datarepresentation = buf[5] | (((unsigned int)buf[4]) << 8);
	switch (m_datarepresentation) {
	case 0:
	{
		if (len < 16)
			return -501;
		union {
			float f;
			uint32_t w;
			uint8_t b[4];
		} r;
		r.w = buf[9] | (((uint32_t)buf[8]) << 8) |
			(((uint32_t)buf[7]) << 16) | (((uint32_t)buf[6]) << 24);
		int16_t e(buf[11] | (((unsigned int)buf[10]) << 8));
		if (e & 0x8000) {
			e &= 0x7fff;
			e = -e;
		}
		int16_t d(buf[13] | (((unsigned int)buf[12]) << 8));
		if (d & 0x8000) {
			d &= 0x7fff;
			d = -d;
		}
		double sc(exp((-M_LN10) * d));
		m_layerparam.set_datascale(std::ldexp(sc, e));
		m_layerparam.set_dataoffset(r.f * sc);
		m_layerparam.set_nbitsgroupref(buf[14]);
		m_layerparam.set_fieldvaluetype(buf[15]);
		break;
	}

	case 2:
	{
		if (len < 42)
			return -501;
		union {
			float f;
			uint32_t w;
			uint8_t b[4];
		} r;
		r.w = buf[9] | (((uint32_t)buf[8]) << 8) |
			(((uint32_t)buf[7]) << 16) | (((uint32_t)buf[6]) << 24);
		int16_t e(buf[11] | (((unsigned int)buf[10]) << 8));
		if (e & 0x8000) {
			e &= 0x7fff;
			e = -e;
		}
		int16_t d(buf[13] | (((unsigned int)buf[12]) << 8));
		if (d & 0x8000) {
			d &= 0x7fff;
			d = -d;
		}
		double sc(exp((-M_LN10) * d));
		m_layerparam.set_datascale(std::ldexp(sc, e));
		m_layerparam.set_dataoffset(r.f * sc);
		m_layerparam.set_nbitsgroupref(buf[14]);
		m_layerparam.set_fieldvaluetype(buf[15]);
		m_layerparam.set_groupsplitmethod(buf[16]);
		m_layerparam.set_missingvaluemgmt(buf[17]);
		m_layerparam.set_primarymissingvalue(buf[21] | (((uint32_t)buf[20]) << 8) | (((uint32_t)buf[19]) << 16) | (((uint32_t)buf[18]) << 24));
	        m_layerparam.set_secondarymissingvalue(buf[25] | (((uint32_t)buf[24]) << 8) | (((uint32_t)buf[23]) << 16) | (((uint32_t)buf[22]) << 24));
		m_layerparam.set_ngroups(buf[29] | (((uint32_t)buf[28]) << 8) | (((uint32_t)buf[27]) << 16) | (((uint32_t)buf[26]) << 24));
		m_layerparam.set_refgroupwidth(buf[30]);
		m_layerparam.set_nbitsgroupwidth(buf[31]);
		m_layerparam.set_refgrouplength(buf[35] | (((uint32_t)buf[34]) << 8) | (((uint32_t)buf[33]) << 16) | (((uint32_t)buf[32]) << 24));
		m_layerparam.set_incrgrouplength(buf[36]);
		m_layerparam.set_lastgrouplength(buf[40] | (((uint32_t)buf[39]) << 8) | (((uint32_t)buf[38]) << 16) | (((uint32_t)buf[37]) << 24));
		m_layerparam.set_nbitsgrouplength(buf[41]);
		break;
	}

	case 3:
	{
		if (len < 44)
			return -501;
		union {
			float f;
			uint32_t w;
			uint8_t b[4];
		} r;
		r.w = buf[9] | (((uint32_t)buf[8]) << 8) |
			(((uint32_t)buf[7]) << 16) | (((uint32_t)buf[6]) << 24);
		int16_t e(buf[11] | (((unsigned int)buf[10]) << 8));
		if (e & 0x8000) {
			e &= 0x7fff;
			e = -e;
		}
		int16_t d(buf[13] | (((unsigned int)buf[12]) << 8));
		if (d & 0x8000) {
			d &= 0x7fff;
			d = -d;
		}
		double sc(exp((-M_LN10) * d));
		m_layerparam.set_datascale(std::ldexp(sc, e));
		m_layerparam.set_dataoffset(r.f * sc);
		m_layerparam.set_nbitsgroupref(buf[14]);
		m_layerparam.set_fieldvaluetype(buf[15]);
		m_layerparam.set_groupsplitmethod(buf[16]);
		m_layerparam.set_missingvaluemgmt(buf[17]);
	        m_layerparam.set_primarymissingvalue(buf[21] | (((uint32_t)buf[20]) << 8) | (((uint32_t)buf[19]) << 16) | (((uint32_t)buf[18]) << 24));
	        m_layerparam.set_secondarymissingvalue(buf[25] | (((uint32_t)buf[24]) << 8) | (((uint32_t)buf[23]) << 16) | (((uint32_t)buf[22]) << 24));
		m_layerparam.set_ngroups(buf[29] | (((uint32_t)buf[28]) << 8) | (((uint32_t)buf[27]) << 16) | (((uint32_t)buf[26]) << 24));
		m_layerparam.set_refgroupwidth(buf[30]);
		m_layerparam.set_nbitsgroupwidth(buf[31]);
		m_layerparam.set_refgrouplength(buf[35] | (((uint32_t)buf[34]) << 8) | (((uint32_t)buf[33]) << 16) | (((uint32_t)buf[32]) << 24));
		m_layerparam.set_incrgrouplength(buf[36]);
		m_layerparam.set_lastgrouplength(buf[40] | (((uint32_t)buf[39]) << 8) | (((uint32_t)buf[38]) << 16) | (((uint32_t)buf[37]) << 24));
		m_layerparam.set_nbitsgrouplength(buf[41]);
		m_layerparam.set_spatialdifforder(buf[42]);
		m_layerparam.set_extradescroctets(buf[43]);
		break;
	}

	case 40:
	{
		if (len < 18)
			return -501;
		union {
			float f;
			uint32_t w;
			uint8_t b[4];
		} r;
		r.w = buf[9] | (((uint32_t)buf[8]) << 8) |
			(((uint32_t)buf[7]) << 16) | (((uint32_t)buf[6]) << 24);
		int16_t e(buf[11] | (((unsigned int)buf[10]) << 8));
		if (e & 0x8000) {
			e &= 0x7fff;
			e = -e;
		}
		int16_t d(buf[13] | (((unsigned int)buf[12]) << 8));
		if (d & 0x8000) {
			d &= 0x7fff;
			d = -d;
		}
		double sc(exp((-M_LN10) * d));
		m_layerparam.set_datascale(std::ldexp(sc, e));
		m_layerparam.set_dataoffset(r.f * sc);
		break;
	}

	default:
	{
		const Parameter *param(find_parameter(m_productdiscipline, m_paramcategory, m_paramnumber));
		const char *sfc(find_surfacetype_str(m_surface1type, "?"));
		std::cerr << "Unsupported Data Representation " << m_datarepresentation
			  << " (param " << param->get_str_nonnull() << ' ' << param->get_unit_nonnull()
			  << ' ' << param->get_abbrev_nonnull() << " surface " << sfc
			  << " reftime " << Glib::TimeVal(m_reftime, 0).as_iso8601()
			  << " efftime " << Glib::TimeVal(m_efftime, 0).as_iso8601() << ')' << std::endl;
		return 0;
	}
	}
	return 0;
}

int GRIB2::Parser::section6(const uint8_t *buf, uint32_t len, goffset offs)
{
	m_bitmap = false;
	if (len < 1)
		return -601;
	switch (buf[0]) {
	case 255:
		break;

	case 254:
		m_bitmap = true;
		break;

	case 0:
		m_bitmapoffs = offs + 1;
		m_bitmapsize = len - 1;
		m_bitmap = true;
		break;

	default:
		std::cerr << "Bitmap Section: unsupported bitmap type " << (unsigned int)buf[0] << std::endl;
		return 0;
	}
	if (!m_bitmap)
		return 0;

	return 0;
}

int GRIB2::Parser::section7(const uint8_t *buf, uint32_t len, goffset offs, const std::string& filename)
{
	if (false)
		std::cout << "section7: grid " << (!m_grid ? "no" : "yes")
			  << " reftime " << (m_reftime > 0 ? "no" : "yes") << ' '
			  << Glib::DateTime::create_now_utc(m_reftime).format("%F %H:%M:%S")
			  << " efftime " << (m_efftime > 0 ? "no" : "yes") << ' '
			  << Glib::DateTime::create_now_utc(m_efftime).format("%F %H:%M:%S")
			  << " discipline " << (unsigned int)m_productdiscipline << " category " << (unsigned int)m_paramcategory
			  << " parameter " << (unsigned int)m_paramnumber << std::endl;
	if (!m_grid || !m_reftime || !m_efftime ||
	    m_productdiscipline == 0xff || m_paramcategory == 0xff || m_paramnumber == 0xff)
		return 0;
	const Parameter *param(find_parameter(m_productdiscipline, m_paramcategory, m_paramnumber));
	if (!param) {
		std::cerr << "Layer Parameter " << (unsigned int)m_productdiscipline << ':'
			  << (unsigned int)m_paramcategory << ':'
			  << (unsigned int)m_paramnumber << " not found" << std::endl;
		return 0;
	}
	if (m_grid) {
		uint32_t griddp(m_grid->get_usize() * m_grid->get_vsize());
		if (m_bitmap) {
			if (m_bitmapsize * 8 < griddp) {
				std::cerr << "Data Representation: not enough bitmap bits for grid: " << (m_bitmapsize * 8) << '<' << griddp << std::endl;
				return 0;
			}
		} else {
			if (m_nrdatapoints < griddp) {
				std::cerr << "Data Representation: not enough datapoints for grid: " << m_nrdatapoints << '<' << griddp << std::endl;
				return 0;
			}
		}
	}
	Glib::RefPtr<Layer> layer;
	switch (m_datarepresentation) {
	case 0:
		if (!len || filename.empty()) {
			if (false)
				std::cerr << "Invalid layer " << param->get_category()->get_discipline()->get_str("?")
					  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?")
					  << " filename " << filename << " pos " << offs << " len " << len << std::endl;
			break;
		}
		if (false)
			std::cout << "Adding layer " << param->get_category()->get_discipline()->get_str("?")
				  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?") << std::endl;
		layer = Glib::RefPtr<LayerSimplePacking>(new (LayerAlloc) LayerSimplePacking(param, m_grid, m_reftime, m_efftime, m_centerid, m_subcenterid,
											     m_productionstatus, m_datatype, m_genprocess, m_genprocesstype,
											     m_surface1type, m_surface1value, m_surface2type, m_surface2value,
											     m_layerparam, m_bitmapoffs, m_bitmap, offs, len, filename));
		break;

	case 2:
		if (!len || filename.empty()) {
			if (false)
				std::cerr << "Invalid layer " << param->get_category()->get_discipline()->get_str("?")
					  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?")
					  << " filename " << filename << " pos " << offs << " len " << len << std::endl;
			break;
		}
		if (false)
			std::cout << "Adding layer " << param->get_category()->get_discipline()->get_str("?")
				  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?") << std::endl;
		layer = Glib::RefPtr<LayerComplexPacking>(new (LayerAlloc) LayerComplexPacking(param, m_grid, m_reftime, m_efftime, m_centerid, m_subcenterid,
											       m_productionstatus, m_datatype, m_genprocess, m_genprocesstype,
											       m_surface1type, m_surface1value, m_surface2type, m_surface2value,
											       m_layerparam, m_bitmapoffs, m_bitmap, offs, len, filename));
		break;

	case 3:
		if (!len || filename.empty()) {
			if (false)
				std::cerr << "Invalid layer " << param->get_category()->get_discipline()->get_str("?")
					  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?")
					  << " filename " << filename << " pos " << offs << " len " << len << std::endl;
			break;
		}
		if (false)
			std::cout << "Adding layer " << param->get_category()->get_discipline()->get_str("?")
				  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?") << std::endl;
		layer = Glib::RefPtr<LayerComplexPackingSpatialDiff>(new (LayerAlloc) LayerComplexPackingSpatialDiff(param, m_grid, m_reftime, m_efftime, m_centerid, m_subcenterid,
														     m_productionstatus, m_datatype, m_genprocess, m_genprocesstype,
														     m_surface1type, m_surface1value, m_surface2type, m_surface2value,
														     m_layerparam, m_bitmapoffs, m_bitmap, offs, len, filename));
		break;

	case 40:
		if (!len || filename.empty()) {
			if (false)
				std::cerr << "Invalid layer " << param->get_category()->get_discipline()->get_str("?")
					  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?")
					  << " filename " << filename << " pos " << offs << " len " << len << std::endl;
			break;
		}
		if (false)
			std::cout << "Adding layer " << param->get_category()->get_discipline()->get_str("?")
				  << ':' << param->get_category()->get_str("?") << ':' << param->get_str("?") << std::endl;
		layer = Glib::RefPtr<LayerJ2K>(new (LayerAlloc) LayerJ2K(param, m_grid, m_reftime, m_efftime, m_centerid, m_subcenterid,
									 m_productionstatus, m_datatype, m_genprocess, m_genprocesstype,
									 m_surface1type, m_surface1value, m_surface2type, m_surface2value,
									 m_layerparam, m_bitmapoffs, m_bitmap, offs, len, filename,
									 m_grib2.get_cachedir()));
		break;

	default:
		break;
	}
	if (!layer)
		return 0;
	m_grib2.add_layer(layer);
	return 1;
}

int GRIB2::Parser::parse_directory(const std::string& dir)
{
	int err(0), lay(0);
	Glib::Dir d(dir);
	for (;;) {
		std::string f(d.read_name());
		if (f.empty())
			break;
		int r(parse(Glib::build_filename(dir, f)));
		if (r < 0)
			err = r;
		else
			lay += r;
	}
	if (lay)
		return lay;
	return err;
}

int GRIB2::Parser::parse(const std::string& p)
{
	if (Glib::file_test(p, Glib::FILE_TEST_IS_DIR))
		return parse_directory(p);
	if (Glib::file_test(p, Glib::FILE_TEST_IS_REGULAR))
		return parse_file(p);
	return -1;
}

int GRIB2::ParamDiscipline::compareid(const ParamDiscipline& x) const
{
	if (get_id() < x.get_id())
		return -1;
	if (x.get_id() < get_id())
		return 1;
	return 0;
}

int GRIB2::ParamCategory::compareid(const ParamCategory& x) const
{
	if (get_discipline_id() < x.get_discipline_id())
		return -1;
	if (x.get_discipline_id() < get_discipline_id())
		return 1;
	if (get_id() < x.get_id())
		return -1;
	if (x.get_id() < get_id())
		return 1;
	return 0;
}

int GRIB2::Parameter::compareid(const Parameter& x) const
{
	if (get_discipline_id() < x.get_discipline_id())
		return -1;
	if (x.get_discipline_id() < get_discipline_id())
		return 1;
	if (get_category_id() < x.get_category_id())
		return -1;
	if (x.get_category_id() < get_category_id())
		return 1;
	if (get_id() < x.get_id())
		return -1;
	if (x.get_id() < get_id())
		return 1;
	return 0;
}

int GRIB2::ID8String::compareid(const ID8String& x) const
{
	if (get_id() < x.get_id())
		return -1;
	if (x.get_id() < get_id())
		return 1;
	return 0;
}

int GRIB2::ID16String::compareid(const ID16String& x) const
{
	if (get_id() < x.get_id())
		return -1;
	if (x.get_id() < get_id())
		return 1;
	return 0;
}

int GRIB2::CenterTable::compareid(const CenterTable& x) const
{
	if (get_id() < x.get_id())
		return -1;
	if (x.get_id() < get_id())
		return 1;
	return 0;
}

int GRIB2::SurfaceTable::compareid(const SurfaceTable& x) const
{
	if (get_id() < x.get_id())
		return -1;
	if (x.get_id() < get_id())
		return 1;
	return 0;
}

GRIB2::WeatherProfilePoint::SoundingSurfaceTemp::SoundingSurfaceTemp(float press, float temp, float dewpt)
	: m_press(press), m_temp(temp), m_dewpt(dewpt)
{
}

int GRIB2::WeatherProfilePoint::SoundingSurfaceTemp::compare(const SoundingSurfaceTemp& x) const
{
	if (std::isnan(get_press()))
		return std::isnan(x.get_press()) ? 0 : -1;
	if (std::isnan(x.get_press()))
		return 1;
	if (get_press() < x.get_press())
		return 1;
	if (x.get_press() < get_press())
		return -1;
	return 0;
}

GRIB2::WeatherProfilePoint::SoundingSurface::SoundingSurface(float press, float temp, float dewpt, float winddir, float windspd)
	: SoundingSurfaceTemp(press, temp, dewpt), m_winddir(winddir), m_windspeed(windspd)
{
}

constexpr float GRIB2::WeatherProfilePoint::Surface::magnus_b;
constexpr float GRIB2::WeatherProfilePoint::Surface::magnus_c;

GRIB2::WeatherProfilePoint::Surface::Surface(float uwind, float vwind, float temp, float rh, float hwsh, float vwsh)
	: m_uwind(uwind), m_vwind(vwind), m_temp(temp), m_rh(rh), m_hwsh(hwsh), m_vwsh(vwsh)
{
}

float GRIB2::WeatherProfilePoint::Surface::get_turbulenceindex(void) const
{
	float x(get_vwsh() * 1e3);
	x *= x;
	x += 42 + 5e5 * get_hwsh();
	return x * 0.25;
}

float GRIB2::WeatherProfilePoint::Surface::get_dewpoint(void) const
{
	if (std::isnan(get_rh()) || std::isnan(get_temp()))
		return std::numeric_limits<float>::quiet_NaN();
	if (get_rh() <= 0)
		return 0;
	float gamma(magnus_gamma());
	float dewpt(magnus_c * gamma / (magnus_b - gamma) + IcaoAtmosphere<float>::degc_to_kelvin);
	if (false)
		std::cerr << "get_dewpoint: rh " << get_rh() << " temp " << get_temp()
			  << " gamma " << gamma << " dewpt " << dewpt << std::endl;
	return dewpt;
}

float GRIB2::WeatherProfilePoint::Surface::magnus_gamma(float temp, float rh)
{
	temp -= IcaoAtmosphere<float>::degc_to_kelvin;
	return logf(rh * 0.01) + magnus_b * temp / (magnus_c + temp);
}

GRIB2::WeatherProfilePoint::Surface::operator SoundingSurface(void) const
{
	SoundingSurface s;
	{
		float wu(get_uwind()), wv(get_vwind());
		wu *= (-1e-3f * Point::km_to_nmi * 3600);
		wv *= (-1e-3f * Point::km_to_nmi * 3600);
		s.set_winddir(atan2f(wu, wv) * (180.0 / M_PI));
		s.set_windspeed(sqrtf(wu * wu + wv * wv));
	}
	s.set_temp(get_temp());
	s.set_dewpt(get_dewpoint());
	return s;
}

GRIB2::WeatherProfilePoint::Stability::Stability(const soundingsurfaces_t& surf)
	: m_lcl_press(std::numeric_limits<float>::quiet_NaN()),
	  m_lcl_temp(std::numeric_limits<float>::quiet_NaN()),
	  m_lfc_press(std::numeric_limits<float>::quiet_NaN()),
	  m_lfc_temp(std::numeric_limits<float>::quiet_NaN()),
	  m_el_press(std::numeric_limits<float>::quiet_NaN()),
	  m_el_temp(std::numeric_limits<float>::quiet_NaN()),
	  m_liftedindex(std::numeric_limits<float>::quiet_NaN()),
	  m_cape(std::numeric_limits<float>::quiet_NaN()),
	  m_cin(std::numeric_limits<float>::quiet_NaN())
{
	if (surf.empty())
		return;
	const SoundingSurfaceTemp& gnd(*surf.begin());
	if (std::isnan(gnd.get_press()) || std::isnan(gnd.get_temp()) || std::isnan(gnd.get_dewpt()) ||
	    gnd.get_press() <= 0 || gnd.get_temp() < gnd.get_dewpt())
		return;
	// find LCL
	{
		double mixr(1);
		for (int i = 0; i < 12; ++i) {
			double t(SkewTChart::mixing_ratio(mixr, gnd.get_press()));
			double td(SkewTChart::mixing_ratio(mixr + 0.01, gnd.get_press()));
			if (!false)
				std::cerr << "Stability: " << gnd.get_press() << "hPa, dewpt " << gnd.get_dewpt()
					  << " t " << t << " td " << td << " mixr " << mixr << std::endl;
			mixr += (gnd.get_dewpt() - t) / (td - t) * 0.01;
		}
		double tdry(gnd.get_temp() / SkewTChart::dry_adiabat(1., gnd.get_press()));
		if (!false)
			std::cerr << "Stability: mixr " << mixr << " tdry " << tdry << std::endl;
		SoundingSurfaceTemp x[2];
		x[0].set_press(1023);
		x[1].set_press(10);
		for (unsigned int j = 0; j < 2; ++j) {
			x[j].set_temp(SkewTChart::dry_adiabat(tdry, x[j].get_press()));
			x[j].set_dewpt(SkewTChart::mixing_ratio(mixr, x[j].get_press()));
		}
		if (x[0].get_dewpt() > x[0].get_temp() || x[1].get_dewpt() <= x[1].get_temp())
			return;
		for (int i = 0; i < 12; ++i) {
			SoundingSurfaceTemp y;
			y.set_press(0.5 * (x[0].get_press() + x[1].get_press()));
			y.set_temp(SkewTChart::dry_adiabat(tdry, y.get_press()));
			y.set_dewpt(SkewTChart::mixing_ratio(mixr, y.get_press()));
			x[(y.get_dewpt() > y.get_temp())] = y;
		}
		double t(x[1].get_temp() - x[0].get_temp() - x[1].get_dewpt() + x[0].get_dewpt());
		if (fabs(t) < 1e-10) {
			t = 0;
		} else {
			t = (x[0].get_dewpt() - x[0].get_temp()) / t;
			t = std::min(std::max(t, 0.), 1.);
		}
		m_lcl_press = x[0].get_press() + t * (x[1].get_press() - x[0].get_press());
		m_lcl_temp = x[0].get_temp() + t * (x[1].get_temp() - x[0].get_temp());
		if (!false)
			std::cerr << "Stability: t " << t << " p " << m_lcl_press << " t " << m_lcl_temp
				  << ' ' << x[0].get_dewpt() + t * (x[1].get_dewpt() - x[0].get_dewpt()) << std::endl;
		double tsat(m_lcl_temp);
		for (int i = 0; i < 12; ++i) {
			double t(SkewTChart::sat_adiabat(tsat, m_lcl_press));
			double td(SkewTChart::sat_adiabat(tsat + 1, m_lcl_press));
			if (!false)
				std::cerr << "Stability: " << m_lcl_press << "hPa, temp " << m_lcl_temp
					  << " t " << t << " td " << td << " tsat " << tsat << std::endl;
			tsat += (m_lcl_temp - t) / (td - t);
		}
		for (soundingsurfaces_t::const_iterator si(surf.begin()), se(surf.end()); si != se; ++si) {
			if (std::isnan(si->get_press()) || std::isnan(si->get_temp()) || si->get_press() <= 0)
				continue;
			double t;
			if (si->get_press() >= m_lcl_press)
				t = SkewTChart::dry_adiabat(tdry, si->get_press());
			else
				t = SkewTChart::sat_adiabat(tsat, si->get_press());
			m_tempcurve.insert(SoundingSurfaceTemp(si->get_press(), si->get_temp(), t));
		}
		// insert LCL
		{
			tempcurve_t::const_iterator i1(m_tempcurve.lower_bound(SoundingSurfaceTemp(m_lcl_press)));
			if (i1 != m_tempcurve.end() && i1 != m_tempcurve.begin() && m_lcl_press < i1->get_press()) {
				tempcurve_t::const_iterator i2(i1);
				--i1;
				double t((m_lcl_press - i1->get_press()) / (i2->get_press() - i1->get_press()));
				t *= i2->get_temp() - i1->get_temp();
				t += i1->get_temp();
				m_tempcurve.insert(SoundingSurfaceTemp(m_lcl_press, t, m_lcl_temp));
			}
		}
	}
	// insert crossover points
	for (tempcurve_t::iterator i1(m_tempcurve.begin()), ie(m_tempcurve.end()); i1 != ie; ) {
		tempcurve_t::iterator i2(i1);
		++i2;
		if (i2 == ie)
			break;
		if (!((i1->get_dewpt() < i1->get_temp() && i2->get_dewpt() > i2->get_temp()) ||
		      (i1->get_dewpt() > i1->get_temp() && i2->get_dewpt() < i2->get_temp()))) {
			i1 = i2;
			continue;
		}
		double t(i2->get_temp() - i1->get_temp() - i2->get_dewpt() + i1->get_dewpt());
		if (fabs(t) < 1e-10) {
			t = 0;
		} else {
			t = (i1->get_dewpt() - i1->get_temp()) / t;
			t = std::min(std::max(t, 0.), 1.);
		}
		float pr(i1->get_press() + t * (i2->get_press() - i1->get_press()));
		float te(i1->get_temp() + t * (i2->get_temp() - i1->get_temp()));
		std::pair<tempcurve_t::iterator,bool> ins(m_tempcurve.insert(SoundingSurfaceTemp(pr, te, te)));
		i1 = ins.first;
		if (!ins.second) {
			SoundingSurfaceTemp& x(const_cast<SoundingSurfaceTemp&>(*i1));
			x.set_temp(te);
			x.set_dewpt(te);
		}
	}
	// lifted index
	{
		static const float li_press = 500.0;
		tempcurve_t::const_iterator i1(m_tempcurve.lower_bound(SoundingSurfaceTemp(li_press)));
		if (i1 != m_tempcurve.end()) {
			if (i1->get_press() == li_press || i1 == m_tempcurve.begin()) {
				m_liftedindex = i1->get_temp() - i1->get_dewpt();
			} else {
				tempcurve_t::const_iterator i2(i1);
				--i1;
				double t((m_lcl_press - i1->get_press()) / (i2->get_press() - i1->get_press()));
				t *= i2->get_temp() - i1->get_temp() - i2->get_dewpt() + i1->get_dewpt();
				t += i1->get_temp() - i1->get_dewpt();
				m_liftedindex = t;
			}
		}
	}
	// CIN and CAPE
	// FIXME: CAPE/CIN should use virtual temperatures
	// http://journals.ametsoc.org/doi/pdf/10.1175/1520-0434%281994%29009%3C0625%3ATEONTV%3E2.0.CO%3B2
	if (!m_tempcurve.empty()) {
		tempcurve_t::const_iterator ie(m_tempcurve.end()), i1(m_tempcurve.lower_bound(SoundingSurfaceTemp(m_lcl_press)));
		--ie;
		if (i1 != m_tempcurve.begin() && i1->get_press() > m_lcl_press)
			--i1;
		for (; i1 != ie; ) {
			tempcurve_t::const_iterator i2(i1);
			++i2;
			while (i2 != ie && i2->get_temp() != i2->get_dewpt())
				++i2;
			float pot(0);
			{
				tempcurve_t::const_iterator i0(i1);
				while (i0 != i2) {
					tempcurve_t::const_iterator i3(i0);
					++i3;
					float alt0(0), alt1(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt0, 0, i0->get_press());
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt1, 0, i3->get_press());
					float t0((i0->get_dewpt() - i0->get_temp()) / i0->get_temp());
					float t1((i3->get_dewpt() - i3->get_temp()) / i3->get_temp());
					pot += 0.5 * (t0 + t1) * (alt1 - alt0);
					i0 = i3;
				}
			}
			pot *= IcaoAtmosphere<float>::const_g;
			if (!std::isnan(m_cape))
				break;
			if (pot < 0 && std::isnan(m_cin))
				m_cin = -pot;
			if (pot > 0) {
				m_cape = pot;
				m_lfc_press = i1->get_press();
				m_lfc_temp = i1->get_temp();
				m_el_press = i2->get_press();
				m_el_temp = i2->get_temp();
				break;
			}
			i1 = i2;
		}
	}
}

const int32_t GRIB2::WeatherProfilePoint::invalidalt = std::numeric_limits<int32_t>::min();
const float GRIB2::WeatherProfilePoint::invalidcover = std::numeric_limits<float>::quiet_NaN();
const int16_t GRIB2::WeatherProfilePoint::isobaric_levels[27] = {
	-1,
	1000,
	975,
	950,
	925,
	900,
	850,
	800,
	750,
	700,
	650,
	600,
	550,
	500,
	450,
	400,
	350,
	300,
	250,
	200,
	150,
	100,
	70,
	50,
	30,
	20,
	10
};

namespace {

	inline int32_t grib2_std_pressure_to_altitude(float press)
	{
		float alt;
		IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, press);
		return Point::round<int32_t,float>(alt * Point::m_to_ft);
	};

};

const int32_t GRIB2::WeatherProfilePoint::altitudes[sizeof(isobaric_levels)/sizeof(isobaric_levels[0])] = {
	0,
	grib2_std_pressure_to_altitude(1000),
	grib2_std_pressure_to_altitude(975),
	grib2_std_pressure_to_altitude(950),
	grib2_std_pressure_to_altitude(925),
	grib2_std_pressure_to_altitude(900),
	grib2_std_pressure_to_altitude(850),
	grib2_std_pressure_to_altitude(800),
	grib2_std_pressure_to_altitude(750),
	grib2_std_pressure_to_altitude(700),
	grib2_std_pressure_to_altitude(650),
	grib2_std_pressure_to_altitude(600),
	grib2_std_pressure_to_altitude(550),
	grib2_std_pressure_to_altitude(500),
	grib2_std_pressure_to_altitude(450),
	grib2_std_pressure_to_altitude(400),
	grib2_std_pressure_to_altitude(350),
	grib2_std_pressure_to_altitude(300),
	grib2_std_pressure_to_altitude(250),
	grib2_std_pressure_to_altitude(200),
	grib2_std_pressure_to_altitude(150),
	grib2_std_pressure_to_altitude(100),
	grib2_std_pressure_to_altitude(70),
	grib2_std_pressure_to_altitude(50),
	grib2_std_pressure_to_altitude(30),
	grib2_std_pressure_to_altitude(20),
	grib2_std_pressure_to_altitude(10)
};

const GRIB2::WeatherProfilePoint::Surface GRIB2::WeatherProfilePoint::invalid_surface(std::numeric_limits<float>::quiet_NaN(),
										      std::numeric_limits<float>::quiet_NaN(),
										      std::numeric_limits<float>::quiet_NaN(),
										      std::numeric_limits<float>::quiet_NaN(),
										      std::numeric_limits<float>::quiet_NaN(),
										      std::numeric_limits<float>::quiet_NaN());

GRIB2::WeatherProfilePoint::WeatherProfilePoint(double dist, double routedist, unsigned int routeindex,
						const Point& pt, gint64 efftime, int32_t alt,
						int32_t zerodegisotherm, int32_t tropopause,
						float cldbdrycover, int32_t bdrylayerheight,
						float cldlowcover, int32_t cldlowbase, int32_t cldlowtop,
						float cldmidcover, int32_t cldmidbase, int32_t cldmidtop,
						float cldhighcover, int32_t cldhighbase, int32_t cldhightop,
						float cldconvcover, int32_t cldconvbase, int32_t cldconvtop,
						float precip, float preciprate, float convprecip, float convpreciprate,
						float liftedindex, float cape, float cin, uint16_t flags)
	: m_pt(pt), m_efftime(efftime), m_dist(dist), m_routedist(routedist), m_routeindex(routeindex), m_alt(alt),
	  m_zerodegisotherm(zerodegisotherm), m_tropopause(tropopause),
	  m_cldbdrycover(cldbdrycover), m_bdrylayerheight(bdrylayerheight),
	  m_cldlowcover(cldlowcover), m_cldlowbase(cldlowbase), m_cldlowtop(cldlowtop),
	  m_cldmidcover(cldmidcover), m_cldmidbase(cldmidbase), m_cldmidtop(cldmidtop),
	  m_cldhighcover(cldhighcover), m_cldhighbase(cldhighbase), m_cldhightop(cldhightop),
	  m_cldconvcover(cldconvcover), m_cldconvbase(cldconvbase), m_cldconvtop(cldconvtop),
	  m_precip(precip), m_preciprate(preciprate), m_convprecip(convprecip), m_convpreciprate(convpreciprate),
	  m_liftedindex(liftedindex), m_cape(cape), m_cin(cin), m_flags(flags)
{
}

int32_t GRIB2::WeatherProfilePoint::get_bdrylayerbase(void) const
{
	// https://en.wikipedia.org/wiki/Lapse_rate#Dry_adiabatic_lapse_rate
	// 9.8°C/km
	static const float dryadiabaticlapserate(9.8f / 1000.f * Point::ft_to_m);
	float dewpt(operator[](0).get_dewpoint()), temp(operator[](0).get_temp());
	if (std::isnan(dewpt) || std::isnan(temp) || dewpt > temp)
		return invalidalt;
	return (temp - dewpt) * (1.f / dryadiabaticlapserate);
}

GRIB2::WeatherProfilePoint::Surface& GRIB2::WeatherProfilePoint::operator[](unsigned int i)
{
	if (i >= sizeof(m_surfaces)/sizeof(m_surfaces[0]))
		return const_cast<Surface&>(invalid_surface);
	return m_surfaces[i];
}

const GRIB2::WeatherProfilePoint::Surface& GRIB2::WeatherProfilePoint::operator[](unsigned int i) const
{
	if (i >= sizeof(m_surfaces)/sizeof(m_surfaces[0]))
		return invalid_surface;
	return m_surfaces[i];
}

GRIB2::WeatherProfilePoint::operator soundingsurfaces_t(void) const
{
	soundingsurfaces_t s;
	for (unsigned int i(0); i <= sizeof(m_surfaces)/sizeof(m_surfaces[0]); ++i) {
		SoundingSurface x(m_surfaces[i]);
		if (!x.is_temp_valid())
			continue;
		x.set_press(isobaric_levels[i]);
		s.insert(x);
	}
	return s;
}

bool GRIB2::WeatherProfilePoint::is_pressure_valid(float press_pa)
{
	return !std::isnan(press_pa) && press_pa >= 0.38f && press_pa <= 200000.0f;
}

GRIB2::WeatherProfile::WeatherProfile(void)
	: m_minefftime(std::numeric_limits<gint64>::max()), m_maxefftime(std::numeric_limits<gint64>::min()),
	  m_minreftime(std::numeric_limits<gint64>::max()), m_maxreftime(std::numeric_limits<gint64>::min())
{
}

void GRIB2::WeatherProfile::add_reftime(gint64 t)
{
	m_minreftime = std::min(t, m_minreftime);
	m_maxreftime = std::max(t, m_maxreftime);
}

void GRIB2::WeatherProfile::add_efftime(gint64 t)
{
	m_minefftime = std::min(t, m_minefftime);
	m_maxefftime = std::max(t, m_maxefftime);
}

double GRIB2::WeatherProfile::get_dist(void) const
{
        if (this->empty())
                return 0;
        return this->operator[](this->size() - 1).get_routedist();
}

GRIB2::GRIB2(void)
{
	set_default_cachedir();
}

GRIB2::GRIB2(const std::string& cachedir)
	: m_cachedir(cachedir)
{
}

GRIB2::~GRIB2(void)
{
}

void GRIB2::set_default_cachedir(void)
{
	m_cachedir.clear();
	std::string dir(FPlan::get_userdbpath());
	if (Glib::file_test(dir, Glib::FILE_TEST_EXISTS) && Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
		dir = Glib::build_filename(dir, "gfscache");
		if (!Glib::file_test(dir, Glib::FILE_TEST_EXISTS))
			g_mkdir_with_parents(dir.c_str(), 0755);
		if (Glib::file_test(dir, Glib::FILE_TEST_EXISTS) && Glib::file_test(dir, Glib::FILE_TEST_IS_DIR))
			m_cachedir = dir;
	}
}

void GRIB2::add_layer(const Glib::RefPtr<Layer>& layer)
{
	if (!layer)
		return;
	std::pair<layers_t::iterator,bool> ins;
	{
		Glib::Mutex::Lock lock(m_mutex);
		ins = m_layers.insert(LayerPtr(layer));
	}
	if (false && !ins.second)
		std::cerr << "duplicate layer " << layer->get_parameter()->get_abbrev_nonnull()
			  << " sfc1 " << find_surfacetype_str(layer->get_surface1type())
			  << ' ' << layer->get_surface1value()
			  << " sfc2 " << find_surfacetype_str(layer->get_surface2type())
			  << ' ' << layer->get_surface2value()
			  << " eff " << layer->get_efftime()
			  << " ref " << layer->get_reftime()
			  << " not inserted" << std::endl;
}

unsigned int GRIB2::remove_missing_layers(void)
{
	unsigned int ret(0);
	{
		Glib::Mutex::Lock lock(m_mutex);
		for (layers_t::iterator li(m_layers.begin()), le(m_layers.end()); li != le;) {
			layers_t::iterator li1(li);
			++li;
			if (li1->get_layer()->check_load())
				continue;
			m_layers.erase(li1);
			++ret;
		}
	}
	return ret;
}

unsigned int GRIB2::remove_obsolete_layers(void)
{
	unsigned int ret(0);
	{
		Glib::Mutex::Lock lock(m_mutex);
		for (layers_t::iterator li(m_layers.begin()), le(m_layers.end()); li != le;) {
			layers_t::iterator li1(li);
			++li;
			if (li == le)
				break;
			const Glib::RefPtr<Layer>& lay1(li1->get_layer());
			const Glib::RefPtr<Layer>& lay2(li->get_layer());
			if (lay1->get_parameter() != lay2->get_parameter())
				continue;
			if (lay1->get_surface1type() != lay2->get_surface1type())
				continue;
			if (lay1->get_surface1value() != lay2->get_surface1value())
				continue;
			if (lay1->get_surface2type() != lay2->get_surface2type())
				continue;
			if (lay1->get_surface2value() != lay2->get_surface2value())
				continue;
			if (lay1->get_efftime() != lay2->get_efftime())
				continue;
			m_layers.erase(li1);
			++ret;
		}
	}
	return ret;
}

unsigned int GRIB2::expire_cache(unsigned int maxdays, off_t maxbytes)
{
	return Cache::expire(get_cachedir(), maxdays, maxbytes);
}

struct GRIB2::parameter_cmp
{
	bool operator()(const GRIB2::ParamDiscipline& a, uint8_t b) { return a.get_id() < b; }
	bool operator()(const GRIB2::ParamCategory& a, uint8_t b) { return a.get_id() < b; }
	bool operator()(const GRIB2::Parameter& a, uint8_t b) { return a.get_id() < b; }
	bool operator()(const GRIB2::ID8String& a, uint8_t b) { return a.get_id() < b; }
	bool operator()(const GRIB2::ID16String& a, uint8_t b) { return a.get_id() < b; }
	bool operator()(const GRIB2::CenterTable& a, uint8_t b) { return a.get_id() < b; }
	bool operator()(const GRIB2::SurfaceTable& a, uint8_t b) { return a.get_id() < b; }
	bool operator()(const char *a, const char *b) {
		if (a) {
			if (!b)
				return false;
			return strcmp(a, b) < 0;
		}
		return !!b;
	}
	bool operator()(const GRIB2::ParamDiscipline& a, const char *b) { return (*this)(a.get_str(), b); }
	bool operator()(const GRIB2::ParamCategory& a, const char *b) { return (*this)(a.get_str(), b); }
	bool operator()(const GRIB2::Parameter& a, const char *b) { return (*this)(a.get_str(), b); }
	bool operator()(const GRIB2::ParamDiscipline *a, const char *b) { if (!a) return false; return (*this)(*a, b); }
	bool operator()(const GRIB2::ParamCategory *a, const char *b) { if (!a) return false; return (*this)(*a, b); }
	bool operator()(const GRIB2::Parameter *a, const char *b) { if (!a) return false; return (*this)(*a, b); }
};

struct GRIB2::parameter_abbrev_cmp
{
	bool operator()(const GRIB2::Parameter& a, const char *b) { return parameter_cmp()(a.get_abbrev(), b); }
	bool operator()(const GRIB2::Parameter *a, const char *b) { if (!a) return false; return (*this)(*a, b); }
};

const GRIB2::ParamDiscipline *GRIB2::find_discipline(uint8_t id)
{
	const ParamDiscipline *b(&paramdiscipline[0]);
	const ParamDiscipline *e(&paramdiscipline[nr_paramdiscipline]);
	const ParamDiscipline *i(std::lower_bound(b, e, id, parameter_cmp()));
	if (i == e)
		return 0;
	return i;
}

const GRIB2::ParamCategory *GRIB2::find_paramcategory(const ParamDiscipline *disc, uint8_t id)
{
	if (!disc)
		return 0;
	const ParamCategory *b(disc[0].get_category());
	const ParamCategory *e(disc[1].get_category());
	const ParamCategory *i(std::lower_bound(b, e, id, parameter_cmp()));
	if (i == e)
		return 0;
	return i;
}

const GRIB2::Parameter *GRIB2::find_parameter(const ParamCategory *cat, uint8_t id)
{
	if (!cat)
		return 0;
	const Parameter *b(cat[0].get_parameter());
	const Parameter *e(cat[1].get_parameter());
	const Parameter *i(std::lower_bound(b, e, id, parameter_cmp()));
	if (i == e)
		return 0;
	return i;
}

const GRIB2::ParamDiscipline * const *GRIB2::find_discipline(const char *str)
{
	if (!str)
		return 0;
	const ParamDiscipline * const *b(&paramdisciplineindex[0]);
	const ParamDiscipline * const *e(&paramdisciplineindex[nr_paramdisciplineindex]);
	const ParamDiscipline * const *i(std::lower_bound(b, e, str, parameter_cmp()));
	if (i == e)
		return 0;
	if (!*i)
		return 0;
	if (!(*i)->get_str())
		return 0;
	if (strcmp((*i)->get_str(), str))
		return 0;
	return i;
}

const GRIB2::ParamCategory * const *GRIB2::find_paramcategory(const char *str)
{
	if (!str)
		return 0;
	const ParamCategory * const *b(&paramcategoryindex[0]);
	const ParamCategory * const *e(&paramcategoryindex[nr_paramcategoryindex]);
	const ParamCategory * const *i(std::lower_bound(b, e, str, parameter_cmp()));
	if (i == e)
		return 0;
	if (!*i)
		return 0;
	if (!(*i)->get_str())
		return 0;
	if (strcmp((*i)->get_str(), str))
		return 0;
	return i;
}

const GRIB2::Parameter * const *GRIB2::find_parameter(const char *str)
{
	if (!str)
		return 0;
	const Parameter * const *b(&parameternameindex[0]);
	const Parameter * const *e(&parameternameindex[nr_parameternameindex]);
	const Parameter * const *i(std::lower_bound(b, e, str, parameter_cmp()));
	if (i == e)
		return 0;
	if (!*i)
		return 0;
	if (!(*i)->get_str())
		return 0;
	if (strcmp((*i)->get_str(), str))
		return 0;
	return i;
}

const GRIB2::Parameter * const *GRIB2::find_parameter_by_abbrev(const char *str)
{
	if (!str)
		return 0;
	const Parameter * const *b(&parameterabbrevindex[0]);
	const Parameter * const *e(&parameterabbrevindex[nr_parameterabbrevindex]);
	const Parameter * const *i(std::lower_bound(b, e, str, parameter_abbrev_cmp()));
	if (i == e)
		return 0;
	if (!*i)
		return 0;
	if (strcmp((*i)->get_abbrev(), str))
		return 0;
	return i;
}

const GRIB2::CenterTable *GRIB2::find_centerid_table(uint16_t cid)
{
	const CenterTable *b(&centertable[0]);
	const CenterTable *e(&centertable[nr_centertable]);
	const CenterTable *i(std::lower_bound(b, e, cid, parameter_cmp()));
	if (i == e)
		return 0;
	return i;
}

const char *GRIB2::find_centerid_str(uint16_t cid, const char *dflt)
{
	const CenterTable *ct(find_centerid_table(cid));
	if (!ct)
		return dflt;
	return ct->get_str(dflt);
}

const char *GRIB2::find_subcenterid_str(uint16_t cid, uint16_t sid, const char *dflt)
{
	const CenterTable *ct(find_centerid_table(cid));
	if (!ct)
		return dflt;
	const ID16String *b(ct->subcenter_begin());
	const ID16String *e(ct->subcenter_end());
	const ID16String *i(std::lower_bound(b, e, sid, parameter_cmp()));
	if (i == e)
		return dflt;
	return i->get_str(dflt);
}

const char *GRIB2::find_genprocesstype_str(uint16_t cid, uint8_t genproct, const char *dflt)
{
	const CenterTable *ct(find_centerid_table(cid));
	if (!ct)
		return dflt;
	const ID8String *b(ct->model_begin());
	const ID8String *e(ct->model_end());
	const ID8String *i(std::lower_bound(b, e, genproct, parameter_cmp()));
	if (i == e)
		return dflt;
	return i->get_str(dflt);
}

const char *GRIB2::find_productionstatus_str(uint8_t pstat, const char *dflt)
{
	const ID8String *b(&prodstatustable[0]);
	const ID8String *e(&prodstatustable[nr_prodstatustable]);
	const ID8String *i(std::lower_bound(b, e, pstat, parameter_cmp()));
	if (i == e)
		return dflt;
	return i->get_str(dflt);
}

const char *GRIB2::find_datatype_str(uint8_t dtype, const char *dflt)
{
	const ID8String *b(&typeofdatatable[0]);
	const ID8String *e(&typeofdatatable[nr_typeofdatatable]);
	const ID8String *i(std::lower_bound(b, e, dtype, parameter_cmp()));
	if (i == e)
		return dflt;
	return i->get_str(dflt);
}

const char *GRIB2::find_genprocess_str(uint8_t genproc, const char *dflt)
{
	const ID8String *b(&genprocesstable[0]);
	const ID8String *e(&genprocesstable[nr_genprocesstable]);
	const ID8String *i(std::lower_bound(b, e, genproc, parameter_cmp()));
	if (i == e)
		return dflt;
	return i->get_str(dflt);
}

const char *GRIB2::find_surfacetype_str(uint8_t sfctype, const char *dflt)
{
	const SurfaceTable *b(&surfacetable[0]);
	const SurfaceTable *e(&surfacetable[nr_surfacetable]);
	const SurfaceTable *i(std::lower_bound(b, e, sfctype, parameter_cmp()));
	if (i == e)
		return dflt;
	return i->get_str(dflt);
}

const char *GRIB2::find_surfaceunit_str(uint8_t sfctype, const char *dflt)
{
	const SurfaceTable *b(&surfacetable[0]);
	const SurfaceTable *e(&surfacetable[nr_surfacetable]);
	const SurfaceTable *i(std::lower_bound(b, e, sfctype, parameter_cmp()));
	if (i == e)
		return dflt;
	return i->get_unit(dflt);
}

GRIB2::layerlist_t GRIB2::find_layers(void)
{
	layerlist_t l;
	Glib::Mutex::Lock lock(m_mutex);
	for (layers_t::const_iterator i(m_layers.begin()), e(m_layers.end()); i != e; ++i)
		l.push_back(i->get_layer());
	return l;
}

GRIB2::layerlist_t GRIB2::find_layers(const Parameter *param, gint64 efftime)
{
	if (!param)
		return layerlist_t();
	gint64 eff0(0);
	gint64 eff1(std::numeric_limits<gint64>::max());
	typedef std::set<LayerPtrSurface> set_t;
	set_t lay0, lay1;
	LayerPtr layptr(Glib::RefPtr<Layer>(new (LayerAlloc) LayerJ2K(param, Glib::RefPtr<Grid>(),
								      std::numeric_limits<gint64>::min(),
								      std::numeric_limits<gint64>::min(),
								      0xffff, 0xffff, 0xff, 0xff, 0xff, 0xff,
								      0, std::numeric_limits<double>::min(),
								      0, std::numeric_limits<double>::min())));
	Glib::Mutex::Lock lock(m_mutex);
	for (layers_t::const_iterator i(m_layers.lower_bound(layptr)), e(m_layers.end()); i != e; ++i) {
		const Glib::RefPtr<Layer>& layer(i->get_layer());
		if (layer->get_parameter() != param)
			break;
		if (layer->get_efftime() < efftime) {
			if (layer->get_efftime() > eff0) {
				lay0.clear();
				eff0 = layer->get_efftime();
			}
			std::pair<set_t::iterator,bool> ins(lay0.insert(LayerPtrSurface(layer)));
			if (!ins.second) {
				if (layer->get_reftime() > ins.first->get_layer()->get_reftime()) {
					lay0.erase(ins.first);
					ins = lay0.insert(LayerPtrSurface(layer));
				}
			}
		} else {
			if (layer->get_efftime() < eff1) {
				lay1.clear();
				eff1 = layer->get_efftime();
			}
			std::pair<set_t::iterator,bool> ins(lay1.insert(LayerPtrSurface(layer)));
			if (!ins.second) {
				if (layer->get_reftime() > ins.first->get_layer()->get_reftime()) {
					lay1.erase(ins.first);
					ins = lay1.insert(LayerPtrSurface(layer));
				}
			}
		}
	}
	layerlist_t ll;
	for (set_t::const_iterator i(lay0.begin()), e(lay0.end()); i != e; ++i)
		ll.push_back(i->get_layer());
	for (set_t::const_iterator i(lay1.begin()), e(lay1.end()); i != e; ++i)
		ll.push_back(i->get_layer());
	return ll;
}

GRIB2::layerlist_t GRIB2::find_layers(const Parameter *param, gint64 efftime, uint8_t sfc1type, double sfc1value)
{
	if (!param)
		return layerlist_t();
	gint64 eff0(0);
	gint64 eff1(0);
	gint64 eff2(std::numeric_limits<gint64>::max());
	gint64 eff3(std::numeric_limits<gint64>::max());
	Glib::RefPtr<Layer> lay0, lay1, lay2, lay3;
	LayerPtr layptr(Glib::RefPtr<Layer>(new (LayerAlloc) LayerJ2K(param, Glib::RefPtr<Grid>(),
								      std::numeric_limits<gint64>::min(),
								      std::numeric_limits<gint64>::min(),
								      0xffff, 0xffff, 0xff, 0xff, 0xff, 0xff,
								      0, std::numeric_limits<double>::min(),
								      0, std::numeric_limits<double>::min())));
	Glib::Mutex::Lock lock(m_mutex);
	for (layers_t::const_iterator i(m_layers.lower_bound(layptr)), e(m_layers.end()); i != e; ++i) {
		const Glib::RefPtr<Layer>& layer(i->get_layer());
		if (layer->get_parameter() != param)
			break;
		if (layer->get_surface1type() != sfc1type)
			continue;
		if (layer->get_efftime() < efftime) {
			if (layer->get_surface1value() < sfc1value) {
				if (!lay0 || layer->get_efftime() > eff0 ||
				    (layer->get_efftime() == eff0 && layer->get_reftime() > lay0->get_reftime()) ||
				    (layer->get_efftime() == eff0 && layer->get_reftime() == lay0->get_reftime() &&
				     layer->get_surface1value() > lay0->get_surface1value())) {
					lay0 = layer;
					eff0 = layer->get_efftime();
				}
			} else {
				if (!lay1 || layer->get_efftime() > eff1 ||
				    (layer->get_efftime() == eff1 && layer->get_reftime() > lay1->get_reftime()) ||
				    (layer->get_efftime() == eff1 && layer->get_reftime() == lay1->get_reftime() &&
				     layer->get_surface1value() < lay1->get_surface1value())) {
					lay1 = layer;
					eff1 = layer->get_efftime();
				}
			}
		} else {
			if (layer->get_surface1value() < sfc1value) {
				if (!lay2 || layer->get_efftime() < eff2 ||
				    (layer->get_efftime() == eff2 && layer->get_reftime() > lay2->get_reftime()) ||
				    (layer->get_efftime() == eff2 && layer->get_reftime() == lay2->get_reftime() &&
				     layer->get_surface1value() > lay2->get_surface1value())) {
					lay2 = layer;
					eff2 = layer->get_efftime();
				}
			} else {
				if (!lay3 || layer->get_efftime() < eff3 ||
				    (layer->get_efftime() == eff3 && layer->get_reftime() > lay3->get_reftime()) ||
				    (layer->get_efftime() == eff3 && layer->get_reftime() == lay3->get_reftime() &&
				     layer->get_surface1value() < lay3->get_surface1value())) {
					lay3 = layer;
					eff3 = layer->get_efftime();
				}
			}
		}
	}
	layerlist_t ll;
	if (lay0)
		ll.push_back(lay0);
	if (lay1)
		ll.push_back(lay1);
	if (lay2)
		ll.push_back(lay2);
	if (lay3)
		ll.push_back(lay3);
	return ll;
}

class GRIB2::Interpolate {
public:
	Interpolate(const Rect& bbox, const layerlist_t& layers);
	Glib::RefPtr<LayerInterpolateResult> interpolate(gint64 efftime) const;
	Glib::RefPtr<LayerInterpolateResult> interpolate(double sfc1value) const;
	Glib::RefPtr<LayerInterpolateResult> interpolate(gint64 efftime, double sfc1value) const;
	Glib::RefPtr<LayerInterpolateResult> nointerp(unsigned int idx) const;
	std::ostream& dumplayers(std::ostream& os, unsigned int indent = 0) const;

protected:
	typedef std::vector<Glib::RefPtr<LayerResult> > lr_t;
	lr_t m_lr;
	typedef std::vector<gint64> efftimes_t;
	efftimes_t m_efftimes;
	typedef std::vector<double> sfc1values_t;
	sfc1values_t m_sfc1values;
	gint64 m_minefftime;
	gint64 m_maxefftime;
	gint64 m_minreftime;
	gint64 m_maxreftime;
	double m_minsfc1value;
	double m_maxsfc1value;
	bool m_samesize;
};

GRIB2::Interpolate::Interpolate(const Rect& bbox, const layerlist_t& layers)
	: m_minefftime(std::numeric_limits<gint64>::max()),
	  m_maxefftime(std::numeric_limits<gint64>::min()),
	  m_minreftime(std::numeric_limits<gint64>::max()),
	  m_maxreftime(std::numeric_limits<gint64>::min()),
	  m_minsfc1value(std::numeric_limits<double>::max()),
	  m_maxsfc1value(std::numeric_limits<double>::min()),
	  m_samesize(true)
{
	for (layerlist_t::const_iterator i(layers.begin()), e(layers.end()); i != e; ++i) {
		if (!*i)
			continue;
		Glib::RefPtr<GRIB2::LayerResult> r((*i)->get_results(bbox));
		if (!r)
			continue;
		m_lr.push_back(r);
		gint64 efftime(r->get_efftime());
		gint64 minreftime(r->get_minreftime());
		gint64 maxreftime(r->get_maxreftime());
		double sfc1value(r->get_surface1value());
		m_efftimes.push_back(efftime);
		m_sfc1values.push_back(sfc1value);
		m_minefftime = std::min(m_minefftime, efftime);
		m_maxefftime = std::max(m_maxefftime, efftime);
		m_minreftime = std::min(m_minreftime, minreftime);
		m_maxreftime = std::max(m_maxreftime, maxreftime);
		m_minsfc1value = std::min(m_minsfc1value, sfc1value);
		m_maxsfc1value = std::max(m_maxsfc1value, sfc1value);
		if (m_lr.size() == 1)
			continue;
		m_samesize = m_samesize && m_lr.front()->get_height() == m_lr.back()->get_height() &&
			m_lr.front()->get_width() == m_lr.back()->get_width() && m_lr.front()->get_bbox() == m_lr.back()->get_bbox();
	}
}

std::ostream& GRIB2::Interpolate::dumplayers(std::ostream& os, unsigned int indent) const
{
	for (lr_t::const_iterator i(m_lr.begin()), e(m_lr.end()); i != e; ++i) {
		os << std::string(indent, ' ') << (i - m_lr.begin()) << ": ";
		const Glib::RefPtr<const LayerResult>& lr(*i);
		if (!lr) {
			os << '-' << std::endl;
			continue;
		}
		os << lr->get_layer()->get_parameter()->get_abbrev_nonnull()
		   << ' ' << lr->get_width() << 'x' << lr->get_height() << ' '
		   << Glib::TimeVal(lr->get_efftime(), 0).as_iso8601() << ' '
		   << GRIB2::find_surfacetype_str(lr->get_layer()->get_surface1type(), "?") << ' '
		   << lr->get_surface1value() << std::endl;
	}
	return os;
}

Glib::RefPtr<GRIB2::LayerInterpolateResult> GRIB2::Interpolate::nointerp(unsigned int idx) const
{
	if (false)
		dumplayers(std::cerr << "GRIB2::Interpolate::nointerp: " << idx << std::endl, 2);
	if (idx >= m_lr.size())
		return Glib::RefPtr<LayerInterpolateResult>();
	const Glib::RefPtr<LayerResult const>& lay(m_lr[idx]);
	gint64 efftime(lay->get_layer()->get_efftime());
	gint64 reftime(lay->get_layer()->get_reftime());
	double sfc1value(lay->get_layer()->get_surface1value());
	unsigned int w(lay->get_width());
	unsigned int h(lay->get_height());
	Glib::RefPtr<LayerInterpolateResult> r(new (LayerAlloc) LayerInterpolateResult(lay->get_layer(), lay->get_bbox(), w, h,
										       efftime, efftime, reftime, reftime, sfc1value, sfc1value));
	unsigned int sz(r->get_size());
	for (unsigned int i = 0; i < sz; ++i)
		r->operator[](i) = GRIB2::LayerInterpolateResult::LinInterp(lay->operator[](i), 0, 0, 0);
	return r;
}

Glib::RefPtr<GRIB2::LayerInterpolateResult> GRIB2::Interpolate::interpolate(gint64 efftime) const
{
	if (false)
		dumplayers(std::cerr << "GRIB2::Interpolate::interpolate: "
			   << Glib::TimeVal(efftime, 0).as_iso8601() << std::endl, 2);
	if (m_lr.empty())
		return Glib::RefPtr<LayerInterpolateResult>();
	if (m_lr.size() == 1)
		return nointerp(0);
	if (m_minefftime >= m_maxefftime)
		return nointerp(0);
	double efftimemul(1.0 / (double)(m_maxefftime - m_minefftime));
	efftime = std::max(std::min(efftime, m_maxefftime), m_minefftime);
	double sfc1value(m_lr.front()->get_layer()->get_surface1value());
	unsigned int lrsz(m_lr.size());
	Eigen::MatrixXd A(lrsz, 2);
	for (unsigned int j = 0; j < lrsz; ++j) {
		A(j, 0) = 1;
		A(j, 1) = (m_efftimes[j] - m_minefftime) * efftimemul;
	}
	Eigen::MatrixXd M((A.transpose() * A).inverse() * A.transpose());
	unsigned int w(m_lr.front()->get_width());
	unsigned int h(m_lr.front()->get_height());
	if (false)
		std::cerr << "GRIB2 Interpolate: A " << A << std::endl << "GRIB2 Interpolate: M " << M << std::endl;
	if (false) {
		std::cerr << "GRIB2 Interpolate: " << m_lr.front()->get_layer()->get_parameter()->get_abbrev_nonnull()
			  << ' ' << w << 'x' << h << " eff " << m_minefftime << ".." << m_maxefftime
			  << " eff " << Glib::DateTime::create_now_utc(efftime).format("%F %H:%M:%S") << " (" << efftime << ')';
		if (m_samesize)
			std::cerr << " (same)";
		for (unsigned int j = 0; j < lrsz; ++j)
			std::cerr << " Layer " << M(0, j) << ' ' << M(1, j)
				  << " eff " << Glib::DateTime::create_now_utc(m_lr[j]->get_efftime()).format("%F %H:%M:%S")
				  << " (" << m_lr[j]->get_efftime() << ')';
		std::cerr << std::endl;
	}
	Glib::RefPtr<LayerInterpolateResult> r(new (LayerAlloc) LayerInterpolateResult(m_lr.front()->get_layer(), m_lr.front()->get_bbox(), w, h,
										       m_minefftime, m_maxefftime, m_minreftime, m_maxreftime, sfc1value, sfc1value));
	if (m_samesize) {
		unsigned int sz(r->get_size());
		for (unsigned int i = 0; i < sz; ++i) {
			GRIB2::LayerInterpolateResult::LinInterp z(0, 0, 0, 0);
			for (unsigned int j = 0; j < lrsz; ++j) {
				float y(m_lr[j]->operator[](i));
				z += GRIB2::LayerInterpolateResult::LinInterp(M(0, j) * y, M(1, j) * y, 0, 0);
			}
			r->operator[](i) = z;
		}
		return r;
	}
	for (unsigned int u = 0; u < w; ++u) {
		for (unsigned int v = 0; v < h; ++v) {
			GRIB2::LayerInterpolateResult::LinInterp z;
			{
				float y(m_lr.front()->operator()(u, v));
				z = GRIB2::LayerInterpolateResult::LinInterp(M(0, 0) * y, M(1, 0) * y, 0, 0);
			}
			Point pt(r->get_center(u, v));
			for (unsigned int j = 1; j < lrsz; ++j) {
				float y(m_lr[j]->operator()(pt));
				z += GRIB2::LayerInterpolateResult::LinInterp(M(0, j) * y, M(1, j) * y, 0, 0);
			}
			r->operator()(u, v) = z;
		}
	}
	return r;
}

Glib::RefPtr<GRIB2::LayerInterpolateResult> GRIB2::Interpolate::interpolate(double sfc1value) const
{
	if (false)
		dumplayers(std::cerr << "GRIB2::Interpolate::interpolate: " << sfc1value << std::endl, 2);
	if (m_lr.empty())
		return Glib::RefPtr<LayerInterpolateResult>();
	if (m_lr.size() == 1)
		return nointerp(0);
	if (m_minsfc1value >= m_maxsfc1value || (m_maxsfc1value - m_minsfc1value) < 1e-100)
		return nointerp(0);
	double sfc1valuemul(1.0 / (m_maxsfc1value - m_minsfc1value));
	sfc1value = std::max(std::min(sfc1value, m_maxsfc1value), m_minsfc1value);
	gint64 efftime(m_lr.front()->get_layer()->get_efftime());
	unsigned int lrsz(m_lr.size());
	Eigen::MatrixXd A(lrsz, 2);
	for (unsigned int j = 0; j < lrsz; ++j) {
		A(j, 0) = 1;
		A(j, 1) = (m_sfc1values[j] - m_minsfc1value) * sfc1valuemul;
	}
	Eigen::MatrixXd M((A.transpose() * A).inverse() * A.transpose());
	unsigned int w(m_lr.front()->get_width());
	unsigned int h(m_lr.front()->get_height());
	if (false)
		std::cerr << "GRIB2 Interpolate: A " << A << std::endl << "GRIB2 Interpolate: M " << M << std::endl;
	if (false) {
		std::cerr << "GRIB2 Interpolate: " << m_lr.front()->get_layer()->get_parameter()->get_abbrev_nonnull()
			  << ' ' << w << 'x' << h << " sfc1 " << m_minsfc1value << ".." << m_maxsfc1value
			  << " sfc1 " << sfc1value;
		if (m_samesize)
			std::cerr << " (same)";
		for (unsigned int j = 0; j < lrsz; ++j)
			std::cerr << " Layer " << M(0, j) << ' ' << M(1, j)
				  << " sfc1 " << m_lr[j]->get_surface1value();
		std::cerr << std::endl;
	}
	Glib::RefPtr<LayerInterpolateResult> r(new (LayerAlloc) LayerInterpolateResult(m_lr.front()->get_layer(), m_lr.front()->get_bbox(), w, h,
										       efftime, efftime, m_minreftime, m_maxreftime, m_minsfc1value, m_maxsfc1value));
	if (m_samesize) {
		unsigned int sz(r->get_size());
		for (unsigned int i = 0; i < sz; ++i) {
			GRIB2::LayerInterpolateResult::LinInterp z(0, 0, 0, 0);
			for (unsigned int j = 0; j < lrsz; ++j) {
				float y(m_lr[j]->operator[](i));
				z += GRIB2::LayerInterpolateResult::LinInterp(M(0, j) * y, 0, M(1, j) * y, 0);
			}
			r->operator[](i) = z;
		}
		return r;
	}
	for (unsigned int u = 0; u < w; ++u) {
		for (unsigned int v = 0; v < h; ++v) {
			GRIB2::LayerInterpolateResult::LinInterp z;
			{
				float y(m_lr.front()->operator()(u, v));
				z = GRIB2::LayerInterpolateResult::LinInterp(M(0, 0) * y, 0, M(1, 0) * y, 0);
			}
			Point pt(r->get_center(u, v));
			for (unsigned int j = 1; j < lrsz; ++j) {
				float y(m_lr[j]->operator()(pt));
				z += GRIB2::LayerInterpolateResult::LinInterp(M(0, j) * y, 0, M(1, j) * y, 0);
			}
			r->operator()(u, v) = z;
		}
	}
	return r;
}

Glib::RefPtr<GRIB2::LayerInterpolateResult> GRIB2::Interpolate::interpolate(gint64 efftime, double sfc1value) const
{
	if (false)
		dumplayers(std::cerr << "GRIB2::Interpolate::interpolate: "
			   << Glib::TimeVal(efftime, 0).as_iso8601() << ' ' << sfc1value << std::endl, 2);
	if (m_lr.empty())
		return Glib::RefPtr<LayerInterpolateResult>();
	if (m_lr.size() == 1)
		return nointerp(0);
	if (m_minefftime >= m_maxefftime)
		return interpolate(sfc1value);
	if (m_minsfc1value >= m_maxsfc1value || (m_maxsfc1value - m_minsfc1value) < 1e-100)
		return interpolate(efftime);
	double efftimemul(1.0 / (double)(m_maxefftime - m_minefftime));
	double sfc1valuemul(1.0 / (m_maxsfc1value - m_minsfc1value));
	efftime = std::max(std::min(efftime, m_maxefftime), m_minefftime);
	sfc1value = std::max(std::min(sfc1value, m_maxsfc1value), m_minsfc1value);
	unsigned int lrsz(m_lr.size());
	Eigen::MatrixXd A(lrsz, 4);
	for (unsigned int j = 0; j < lrsz; ++j) {
		A(j, 0) = 1;
		A(j, 1) = (m_efftimes[j] - m_minefftime) * efftimemul;
		A(j, 2) = (m_sfc1values[j] - m_minsfc1value) * sfc1valuemul;
		A(j, 3) = A(j, 1) * A(j, 2);
	}
	Eigen::MatrixXd M;
	if (lrsz >= 4) {
		M = (A.transpose() * A).inverse() * A.transpose();
	} else {
		A.conservativeResize(Eigen::NoChange_t(), 3);
		M = (A.transpose() * A).inverse() * A.transpose();
		M.conservativeResize(4, Eigen::NoChange_t());
		for (unsigned int j = 0; j < lrsz; ++j)
			M(3, j) = 0;
	}
	unsigned int w(m_lr.front()->get_width());
	unsigned int h(m_lr.front()->get_height());
	if (false)
		std::cerr << "GRIB2 Interpolate: A " << A << std::endl << "GRIB2 Interpolate: M " << M << std::endl;
	if (false) {
		std::cerr << "GRIB2 Interpolate: " << m_lr.front()->get_layer()->get_parameter()->get_abbrev_nonnull()
			  << ' ' << w << 'x' << h << " eff " << m_minefftime << ".." << m_maxefftime
			  << " sfc1 " << m_minsfc1value << ".." << m_maxsfc1value
			  << " eff " << Glib::DateTime::create_now_utc(efftime).format("%F %H:%M:%S") << " (" << efftime << ')'
			  << " sfc1 " << sfc1value;
		if (m_samesize)
			std::cerr << " (same)";
		for (unsigned int j = 0; j < lrsz; ++j)
			std::cerr << " Layer " << M(0, j) << ' ' << M(1, j) << ' ' << M(2, j) << ' ' << M(3, j)
				  << " eff " << Glib::DateTime::create_now_utc(m_lr[j]->get_efftime()).format("%F %H:%M:%S")
				  << " (" << m_lr[j]->get_efftime() << ')'
				  << " sfc1 " << m_lr[j]->get_surface1value();
		std::cerr << std::endl;
	}
	Glib::RefPtr<LayerInterpolateResult> r(new (LayerAlloc) LayerInterpolateResult(m_lr.front()->get_layer(), m_lr.front()->get_bbox(), w, h,
										       m_minefftime, m_maxefftime, m_minreftime, m_maxreftime, m_minsfc1value, m_maxsfc1value));
	if (m_samesize) {
		unsigned int sz(r->get_size());
		for (unsigned int i = 0; i < sz; ++i) {
			GRIB2::LayerInterpolateResult::LinInterp z(0, 0, 0, 0);
			for (unsigned int j = 0; j < lrsz; ++j) {
				float y(m_lr[j]->operator[](i));
				z += GRIB2::LayerInterpolateResult::LinInterp(M(0, j) * y, M(1, j) * y, M(2, j) * y, M(3, j) * y);
			}
			r->operator[](i) = z;
		}
		return r;
	}
	for (unsigned int u = 0; u < w; ++u) {
		for (unsigned int v = 0; v < h; ++v) {
			GRIB2::LayerInterpolateResult::LinInterp z;
			{
				float y(m_lr.front()->operator()(u, v));
				z = GRIB2::LayerInterpolateResult::LinInterp(M(0, 0) * y, M(1, 0) * y, M(2, 0) * y, M(3, 0) * y);
			}
			Point pt(r->get_center(u, v));
			for (unsigned int j = 1; j < lrsz; ++j) {
				float y(m_lr[j]->operator()(pt));
				z += GRIB2::LayerInterpolateResult::LinInterp(M(0, j) * y, M(1, j) * y, M(2, j) * y, M(3, j) * y);
			}
			r->operator()(u, v) = z;
		}
	}
	return r;
}

Glib::RefPtr<GRIB2::LayerInterpolateResult> GRIB2::interpolate_results(const Rect& bbox, const layerlist_t& layers, gint64 efftime)
{
	Interpolate interp(bbox, layers);
	return interp.interpolate(efftime);
}

Glib::RefPtr<GRIB2::LayerInterpolateResult> GRIB2::interpolate_results(const Rect& bbox, const layerlist_t& layers, gint64 efftime, double sfc1value)
{
	Interpolate interp(bbox, layers);
	return interp.interpolate(efftime, sfc1value);
}

GRIB2::WeatherProfile GRIB2::get_profile(const FPlanRoute& fpl)
{
	static const unsigned int nrsfc(sizeof(WeatherProfilePoint::isobaric_levels)/sizeof(WeatherProfilePoint::isobaric_levels[0]));
	static const bool trace_loads(false);
	static const bool trace_crain(false);
	Glib::RefPtr<LayerInterpolateResult> wxzerodegisotherm;
	Glib::RefPtr<LayerInterpolateResult> wxtropopause;
	Glib::RefPtr<LayerInterpolateResult> wxcldbdrycover;
	Glib::RefPtr<LayerInterpolateResult> wxbdrylayerheight;
	Glib::RefPtr<LayerInterpolateResult> wxcldlowcover;
	Glib::RefPtr<LayerInterpolateResult> wxcldlowbase;
	Glib::RefPtr<LayerInterpolateResult> wxcldlowtop;
	Glib::RefPtr<LayerInterpolateResult> wxcldmidcover;
	Glib::RefPtr<LayerInterpolateResult> wxcldmidbase;
	Glib::RefPtr<LayerInterpolateResult> wxcldmidtop;
	Glib::RefPtr<LayerInterpolateResult> wxcldhighcover;
	Glib::RefPtr<LayerInterpolateResult> wxcldhighbase;
	Glib::RefPtr<LayerInterpolateResult> wxcldhightop;
	Glib::RefPtr<LayerInterpolateResult> wxcldconvcover;
	Glib::RefPtr<LayerInterpolateResult> wxcldconvbase;
	Glib::RefPtr<LayerInterpolateResult> wxcldconvtop;
	Glib::RefPtr<LayerInterpolateResult> wxprecip;
	Glib::RefPtr<LayerInterpolateResult> wxpreciprate;
	Glib::RefPtr<LayerInterpolateResult> wxconvprecip;
	Glib::RefPtr<LayerInterpolateResult> wxconvpreciprate;
	Glib::RefPtr<LayerInterpolateResult> wxcrain;
	Glib::RefPtr<LayerInterpolateResult> wxcfrzr;
	Glib::RefPtr<LayerInterpolateResult> wxcicep;
	Glib::RefPtr<LayerInterpolateResult> wxcsnow;
	Glib::RefPtr<LayerInterpolateResult> wxlft;
	Glib::RefPtr<LayerInterpolateResult> wxcape;
	Glib::RefPtr<LayerInterpolateResult> wxcin;
	Glib::RefPtr<LayerInterpolateResult> wxtemp[nrsfc];
	Glib::RefPtr<LayerInterpolateResult> wxugrd[nrsfc];
	Glib::RefPtr<LayerInterpolateResult> wxvgrd[nrsfc];
	Glib::RefPtr<LayerInterpolateResult> wxrelhum[nrsfc];
	Rect bbox(fpl.get_bbox().oversize_nmi(100.f));
	double dist(0);
	WeatherProfile wxprof;
	for (unsigned int wptnr = 1U, nrwpt = fpl.get_nrwpt(); wptnr < nrwpt; ++wptnr) {
		const FPlanWaypoint& wpt0(fpl[wptnr - 1U]);
		const FPlanWaypoint& wpt1(fpl[wptnr]);
		double distleg(wpt0.get_coord().spheric_distance_nmi_dbl(wpt1.get_coord()));
		gint64 timeorig(fpl[0].get_time_unix() + wpt0.get_flighttime());
		gint64 timediff(wpt1.get_flighttime() - (gint64)wpt0.get_flighttime());
		double tinc(1.0);
		if (distleg > 0)
			tinc = 1.0 / distleg;
		else if (timediff > 0)
			tinc = 600.0 / timediff;
		else
			continue;
		tinc = std::max(tinc, 1e-3);
		Point ptorig(wpt0.get_coord());
		Point ptdiff(wpt1.get_coord() - ptorig);
		int32_t altorig(wpt0.get_altitude());
		int32_t altdiff(wpt1.get_altitude() - wpt0.get_altitude());
		for (double t = 0;; t += tinc) {
			if (t >= 1.0) {
				if (wptnr + 1 < nrwpt)
					break;
				t = 1.0;
			}
			if (false)
				std::cerr << "WX profile: wpt " << wptnr << '/' << nrwpt << " t " << t << std::endl;
			Point pt(Point(ptdiff.get_lon() * t, ptdiff.get_lat() * t) + ptorig);
			gint64 efftime(timeorig + t * timediff);
			int32_t alt(altorig + t * altdiff);
			bool first(wptnr == 1U && t == 0);
			if (first ||
			    (wxzerodegisotherm && (efftime < wxzerodegisotherm->get_minefftime() ||
						   efftime > wxzerodegisotherm->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_hgt),
							   efftime, surface_0degc_isotherm, 0));
                                wxzerodegisotherm = interpolate_results(bbox, ll, efftime);
				if (wxzerodegisotherm) {
					wxprof.add_efftime(wxzerodegisotherm->get_minefftime());
					wxprof.add_efftime(wxzerodegisotherm->get_maxefftime());
					wxprof.add_reftime(wxzerodegisotherm->get_minreftime());
					wxprof.add_reftime(wxzerodegisotherm->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "0degC Isotherm: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxzerodegisotherm ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxzerodegisotherm)
						std::cerr << " (" << Glib::TimeVal(wxzerodegisotherm->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxzerodegisotherm->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxtropopause && (efftime < wxtropopause->get_minefftime() ||
					      efftime > wxtropopause->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_hgt),
							   efftime, surface_tropopause, 0));
				wxtropopause = interpolate_results(bbox, ll, efftime);
				if (wxtropopause) {
					wxprof.add_efftime(wxtropopause->get_minefftime());
					wxprof.add_efftime(wxtropopause->get_maxefftime());
					wxprof.add_reftime(wxtropopause->get_minreftime());
					wxprof.add_reftime(wxtropopause->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Tropopause: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxtropopause ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxtropopause)
						std::cerr << " (" << Glib::TimeVal(wxtropopause->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxtropopause->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldbdrycover && (efftime < wxcldbdrycover->get_minefftime() ||
						efftime > wxcldbdrycover->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_cloud_tcdc),
							   efftime, surface_boundary_layer_cloud, 0));
                                wxcldbdrycover = interpolate_results(bbox, ll, efftime);
				if (wxcldbdrycover) {
					wxprof.add_efftime(wxcldbdrycover->get_minefftime());
					wxprof.add_efftime(wxcldbdrycover->get_maxefftime());
					wxprof.add_reftime(wxcldbdrycover->get_minreftime());
					wxprof.add_reftime(wxcldbdrycover->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Cloud Boundary Cover: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldbdrycover ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldbdrycover)
						std::cerr << " (" << Glib::TimeVal(wxcldbdrycover->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldbdrycover->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxbdrylayerheight && (efftime < wxbdrylayerheight->get_minefftime() ||
					      efftime > wxbdrylayerheight->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_hpbl_2),
							   efftime, surface_ground_or_water, 0));
                                wxbdrylayerheight = interpolate_results(bbox, ll, efftime);
				if (wxbdrylayerheight) {
					wxprof.add_efftime(wxbdrylayerheight->get_minefftime());
					wxprof.add_efftime(wxbdrylayerheight->get_maxefftime());
					wxprof.add_reftime(wxbdrylayerheight->get_minreftime());
					wxprof.add_reftime(wxbdrylayerheight->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Boundary Layer Height: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxbdrylayerheight ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxbdrylayerheight)
						std::cerr << " (" << Glib::TimeVal(wxbdrylayerheight->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxbdrylayerheight->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldlowcover && (efftime < wxcldlowcover->get_minefftime() ||
					       efftime > wxcldlowcover->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_cloud_tcdc),
							   efftime, surface_low_cloud, 0));
                                wxcldlowcover = interpolate_results(bbox, ll, efftime);
				if (wxcldlowcover) {
					wxprof.add_efftime(wxcldlowcover->get_minefftime());
					wxprof.add_efftime(wxcldlowcover->get_maxefftime());
					wxprof.add_reftime(wxcldlowcover->get_minreftime());
					wxprof.add_reftime(wxcldlowcover->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Low Cloud Cover: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldlowcover ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldlowcover)
						std::cerr << " (" << Glib::TimeVal(wxcldlowcover->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldlowcover->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldlowbase && (efftime < wxcldlowbase->get_minefftime() ||
					      efftime > wxcldlowbase->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_low_cloud_bottom, 0));
                                wxcldlowbase = interpolate_results(bbox, ll, efftime);
				if (wxcldlowbase) {
					wxprof.add_efftime(wxcldlowbase->get_minefftime());
					wxprof.add_efftime(wxcldlowbase->get_maxefftime());
					wxprof.add_reftime(wxcldlowbase->get_minreftime());
					wxprof.add_reftime(wxcldlowbase->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Low Cloud Base: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldlowbase ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldlowbase)
						std::cerr << " (" << Glib::TimeVal(wxcldlowbase->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldlowbase->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldlowtop && (efftime < wxcldlowtop->get_minefftime() ||
					     efftime > wxcldlowtop->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_low_cloud_top, 0));
                                wxcldlowtop = interpolate_results(bbox, ll, efftime);
				if (wxcldlowtop) {
					wxprof.add_efftime(wxcldlowtop->get_minefftime());
					wxprof.add_efftime(wxcldlowtop->get_maxefftime());
					wxprof.add_reftime(wxcldlowtop->get_minreftime());
					wxprof.add_reftime(wxcldlowtop->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Low Cloud Top: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldlowtop ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldlowtop)
						std::cerr << " (" << Glib::TimeVal(wxcldlowtop->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldlowtop->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldmidcover && (efftime < wxcldmidcover->get_minefftime() ||
					       efftime > wxcldmidcover->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_cloud_tcdc),
							   efftime, surface_middle_cloud, 0));
                                wxcldmidcover = interpolate_results(bbox, ll, efftime);
				if (wxcldmidcover) {
					wxprof.add_efftime(wxcldmidcover->get_minefftime());
					wxprof.add_efftime(wxcldmidcover->get_maxefftime());
					wxprof.add_reftime(wxcldmidcover->get_minreftime());
					wxprof.add_reftime(wxcldmidcover->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Mid Cloud Cover: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldmidcover ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldmidcover)
						std::cerr << " (" << Glib::TimeVal(wxcldmidcover->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldmidcover->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldmidbase && (efftime < wxcldmidbase->get_minefftime() ||
					      efftime > wxcldmidbase->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_middle_cloud_bottom, 0));
                                wxcldmidbase = interpolate_results(bbox, ll, efftime);
				if (wxcldmidbase) {
					wxprof.add_efftime(wxcldmidbase->get_minefftime());
					wxprof.add_efftime(wxcldmidbase->get_maxefftime());
					wxprof.add_reftime(wxcldmidbase->get_minreftime());
					wxprof.add_reftime(wxcldmidbase->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Mid Cloud Base: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldmidbase ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldmidbase)
						std::cerr << " (" << Glib::TimeVal(wxcldmidbase->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldmidbase->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldmidtop && (efftime < wxcldmidtop->get_minefftime() ||
					     efftime > wxcldmidtop->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_middle_cloud_top, 0));
                                wxcldmidtop = interpolate_results(bbox, ll, efftime);
				if (wxcldmidtop) {
					wxprof.add_efftime(wxcldmidtop->get_minefftime());
					wxprof.add_efftime(wxcldmidtop->get_maxefftime());
					wxprof.add_reftime(wxcldmidtop->get_minreftime());
					wxprof.add_reftime(wxcldmidtop->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Mid Cloud Top: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldmidtop ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldmidtop)
						std::cerr << " (" << Glib::TimeVal(wxcldmidtop->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldmidtop->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldhighcover && (efftime < wxcldhighcover->get_minefftime() ||
						efftime > wxcldhighcover->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_cloud_tcdc),
							   efftime, surface_top_cloud, 0));
                                wxcldhighcover = interpolate_results(bbox, ll, efftime);
				if (wxcldhighcover) {
					wxprof.add_efftime(wxcldhighcover->get_minefftime());
					wxprof.add_efftime(wxcldhighcover->get_maxefftime());
					wxprof.add_reftime(wxcldhighcover->get_minreftime());
					wxprof.add_reftime(wxcldhighcover->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "High Cloud Cover: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldhighcover ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldhighcover)
						std::cerr << " (" << Glib::TimeVal(wxcldhighcover->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldhighcover->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldhighbase && (efftime < wxcldhighbase->get_minefftime() ||
					       efftime > wxcldhighbase->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_top_cloud_bottom, 0));
                                wxcldhighbase = interpolate_results(bbox, ll, efftime);
				if (wxcldhighbase) {
					wxprof.add_efftime(wxcldhighbase->get_minefftime());
					wxprof.add_efftime(wxcldhighbase->get_maxefftime());
					wxprof.add_reftime(wxcldhighbase->get_minreftime());
					wxprof.add_reftime(wxcldhighbase->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "High Cloud Base: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldhighbase ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldhighbase)
						std::cerr << " (" << Glib::TimeVal(wxcldhighbase->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldhighbase->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldhightop && (efftime < wxcldhightop->get_minefftime() ||
					      efftime > wxcldhightop->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_top_cloud_top, 0));
                                wxcldhightop = interpolate_results(bbox, ll, efftime);
				if (wxcldhightop) {
					wxprof.add_efftime(wxcldhightop->get_minefftime());
					wxprof.add_efftime(wxcldhightop->get_maxefftime());
					wxprof.add_reftime(wxcldhightop->get_minreftime());
					wxprof.add_reftime(wxcldhightop->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "High Cloud Top: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldhightop ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldhightop)
						std::cerr << " (" << Glib::TimeVal(wxcldhightop->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldhightop->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldconvcover && (efftime < wxcldconvcover->get_minefftime() ||
						efftime > wxcldconvcover->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_cloud_tcdc),
							   efftime, surface_convective_cloud, 0));
                                wxcldconvcover = interpolate_results(bbox, ll, efftime);
				if (wxcldconvcover) {
					wxprof.add_efftime(wxcldconvcover->get_minefftime());
					wxprof.add_efftime(wxcldconvcover->get_maxefftime());
					wxprof.add_reftime(wxcldconvcover->get_minreftime());
					wxprof.add_reftime(wxcldconvcover->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Convective Cloud Cover: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldconvcover ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldconvcover)
						std::cerr << " (" << Glib::TimeVal(wxcldconvcover->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldconvcover->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldconvbase && (efftime < wxcldconvbase->get_minefftime() ||
					       efftime > wxcldconvbase->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_convective_cloud_bottom, 0));
                                wxcldconvbase = interpolate_results(bbox, ll, efftime);
				if (wxcldconvbase) {
					wxprof.add_efftime(wxcldconvbase->get_minefftime());
					wxprof.add_efftime(wxcldconvbase->get_maxefftime());
					wxprof.add_reftime(wxcldconvbase->get_minreftime());
					wxprof.add_reftime(wxcldconvbase->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Convective Cloud Base: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldconvbase ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldconvbase)
						std::cerr << " (" << Glib::TimeVal(wxcldconvbase->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldconvbase->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcldconvtop && (efftime < wxcldconvtop->get_minefftime() ||
					      efftime > wxcldconvtop->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_mass_pres),
							   efftime, surface_convective_cloud_top, 0));
                                wxcldconvtop = interpolate_results(bbox, ll, efftime);
				if (wxcldconvtop) {
					wxprof.add_efftime(wxcldconvtop->get_minefftime());
					wxprof.add_efftime(wxcldconvtop->get_maxefftime());
					wxprof.add_reftime(wxcldconvtop->get_minreftime());
					wxprof.add_reftime(wxcldconvtop->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Convective Cloud Top: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcldconvtop ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcldconvtop)
						std::cerr << " (" << Glib::TimeVal(wxcldconvtop->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcldconvtop->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxprecip && (efftime < wxprecip->get_minefftime() ||
					  efftime > wxprecip->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_apcp),
							   efftime, surface_ground_or_water, 0));
                                wxprecip = interpolate_results(bbox, ll, efftime);
				if (wxprecip) {
					wxprof.add_efftime(wxprecip->get_minefftime());
					wxprof.add_efftime(wxprecip->get_maxefftime());
					wxprof.add_reftime(wxprecip->get_minreftime());
					wxprof.add_reftime(wxprecip->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Precipitation: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxprecip ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxprecip)
						std::cerr << " (" << Glib::TimeVal(wxprecip->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxprecip->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxpreciprate && (efftime < wxpreciprate->get_minefftime() ||
					      efftime > wxpreciprate->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_prate),
							   efftime, surface_ground_or_water, 0));
                                wxpreciprate = interpolate_results(bbox, ll, efftime);
				if (wxpreciprate) {
					wxprof.add_efftime(wxpreciprate->get_minefftime());
					wxprof.add_efftime(wxpreciprate->get_maxefftime());
					wxprof.add_reftime(wxpreciprate->get_minreftime());
					wxprof.add_reftime(wxpreciprate->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Precipitation Rate: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxpreciprate ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxpreciprate)
						std::cerr << " (" << Glib::TimeVal(wxpreciprate->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxpreciprate->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxconvprecip && (efftime < wxconvprecip->get_minefftime() ||
					      efftime > wxconvprecip->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_acpcp),
							   efftime, surface_ground_or_water, 0));
                                wxconvprecip = interpolate_results(bbox, ll, efftime);
				if (wxconvprecip) {
					wxprof.add_efftime(wxconvprecip->get_minefftime());
					wxprof.add_efftime(wxconvprecip->get_maxefftime());
					wxprof.add_reftime(wxconvprecip->get_minreftime());
					wxprof.add_reftime(wxconvprecip->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Convective Precipitation: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxconvprecip ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxconvprecip)
						std::cerr << " (" << Glib::TimeVal(wxconvprecip->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxconvprecip->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxconvpreciprate && (efftime < wxconvpreciprate->get_minefftime() ||
						  efftime > wxconvpreciprate->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_cprat_2),
							   efftime, surface_ground_or_water, 0));
                                wxconvpreciprate = interpolate_results(bbox, ll, efftime);
				if (wxconvpreciprate) {
					wxprof.add_efftime(wxconvpreciprate->get_minefftime());
					wxprof.add_efftime(wxconvpreciprate->get_maxefftime());
					wxprof.add_reftime(wxconvpreciprate->get_minreftime());
					wxprof.add_reftime(wxconvpreciprate->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Convective Precipitation Rate: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxconvpreciprate ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxconvpreciprate)
						std::cerr << " (" << Glib::TimeVal(wxconvpreciprate->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxconvpreciprate->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcrain && (efftime < wxcrain->get_minefftime() ||
					 efftime > wxcrain->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_crain_2),
							   efftime, surface_ground_or_water, 0));
				wxcrain = interpolate_results(bbox, ll, efftime);
				if (wxcrain) {
					wxprof.add_efftime(wxcrain->get_minefftime());
					wxprof.add_efftime(wxcrain->get_maxefftime());
					wxprof.add_reftime(wxcrain->get_minreftime());
					wxprof.add_reftime(wxcrain->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Categorical Rain: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcrain ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcrain)
						std::cerr << " (" << Glib::TimeVal(wxcrain->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcrain->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
				if (trace_crain) {
					for (layerlist_t::iterator li(ll.begin()), le(ll.end()); li != le; ++li) {
						Glib::RefPtr<LayerResult> r((*li)->get_results(bbox));
						std::cerr << "crain " << Glib::TimeVal((*li)->get_efftime(), 0).as_iso8601()
							  << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
							  << " value " << r->operator()(pt) << std::endl;
						unsigned int w(r->get_width()), h(r->get_height());
						for (unsigned int hh(0); hh < h; ++hh)
							for (unsigned int ww(0); ww < w; ++ww) {
								Point pt(r->get_center(ww, hh));
								std::cerr << "  h " << hh << " w " << ww
									  << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
									  << " value " << r->operator()(ww, hh) << std::endl;
							}
					}
					if (wxcrain)
						std::cerr << "crain " << Glib::TimeVal(efftime, 0).as_iso8601()
							  << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2()
							  << " value " << wxcrain->operator()(pt, efftime, 0) << std::endl;
				}
			}
			if (first ||
			    (wxcfrzr && (efftime < wxcfrzr->get_minefftime() ||
					 efftime > wxcfrzr->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_cfrzr_2),
							   efftime, surface_ground_or_water, 0));
                                wxcfrzr = interpolate_results(bbox, ll, efftime);
				if (wxcfrzr) {
					wxprof.add_efftime(wxcfrzr->get_minefftime());
					wxprof.add_efftime(wxcfrzr->get_maxefftime());
					wxprof.add_reftime(wxcfrzr->get_minreftime());
					wxprof.add_reftime(wxcfrzr->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Categorical Freezing Rain: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcfrzr ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcfrzr)
						std::cerr << " (" << Glib::TimeVal(wxcfrzr->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcfrzr->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcicep && (efftime < wxcicep->get_minefftime() ||
					 efftime > wxcicep->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_cicep_2),
							   efftime, surface_ground_or_water, 0));
                                wxcicep = interpolate_results(bbox, ll, efftime);
				if (wxcicep) {
					wxprof.add_efftime(wxcicep->get_minefftime());
					wxprof.add_efftime(wxcicep->get_maxefftime());
					wxprof.add_reftime(wxcicep->get_minreftime());
					wxprof.add_reftime(wxcicep->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Categorical Ice Pellets: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcicep ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcicep)
						std::cerr << " (" << Glib::TimeVal(wxcicep->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcicep->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcsnow && (efftime < wxcsnow->get_minefftime() ||
					 efftime > wxcsnow->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_csnow_2),
							   efftime, surface_ground_or_water, 0));
                                wxcsnow = interpolate_results(bbox, ll, efftime);
				if (wxcsnow) {
					wxprof.add_efftime(wxcsnow->get_minefftime());
					wxprof.add_efftime(wxcsnow->get_maxefftime());
					wxprof.add_reftime(wxcsnow->get_minreftime());
					wxprof.add_reftime(wxcsnow->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Categorical Snow: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcsnow ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxlft)
						std::cerr << " (" << Glib::TimeVal(wxcsnow->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcsnow->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxlft && (efftime < wxlft->get_minefftime() ||
				       efftime > wxlft->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_thermodynamic_stability_lftx_2),
							   efftime, surface_ground_or_water, 0));
                                wxlft = interpolate_results(bbox, ll, efftime);
				if (wxlft) {
					wxprof.add_efftime(wxlft->get_minefftime());
					wxprof.add_efftime(wxlft->get_maxefftime());
					wxprof.add_reftime(wxlft->get_minreftime());
					wxprof.add_reftime(wxlft->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "Lifted Index: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxlft ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxlft)
						std::cerr << " (" << Glib::TimeVal(wxlft->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxlft->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcape && (efftime < wxcape->get_minefftime() ||
					efftime > wxcape->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_thermodynamic_stability_cape),
							   efftime, surface_ground_or_water, 0));
                                wxcape = interpolate_results(bbox, ll, efftime);
				if (wxcape) {
					wxprof.add_efftime(wxcape->get_minefftime());
					wxprof.add_efftime(wxcape->get_maxefftime());
					wxprof.add_reftime(wxcape->get_minreftime());
					wxprof.add_reftime(wxcape->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "CAPE: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcape ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcape)
						std::cerr << " (" << Glib::TimeVal(wxcape->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcape->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			if (first ||
			    (wxcin && (efftime < wxcin->get_minefftime() ||
				       efftime > wxcin->get_maxefftime()))) {
				layerlist_t ll(find_layers(find_parameter(param_meteorology_thermodynamic_stability_cin),
							   efftime, surface_ground_or_water, 0));
                                wxcin = interpolate_results(bbox, ll, efftime);
				if (wxcin) {
					wxprof.add_efftime(wxcin->get_minefftime());
					wxprof.add_efftime(wxcin->get_maxefftime());
					wxprof.add_reftime(wxcin->get_minreftime());
					wxprof.add_reftime(wxcin->get_maxreftime());
				}
				if (trace_loads) {
					std::cerr << "CIN: wptnr " << wptnr << " t " << t << " ll " << ll.size()
						  << " interp " << (wxcin ? "yes" : "no")
						  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601();
					if (wxcin)
						std::cerr << " (" << Glib::TimeVal(wxcin->get_minefftime(), 0).as_iso8601()
							  << ".." << Glib::TimeVal(wxcin->get_maxefftime(), 0).as_iso8601() << ')';
					std::cerr << std::endl;
				}
			}
			int32_t zerodegisotherm = WeatherProfilePoint::invalidalt;
			if (wxzerodegisotherm) {
				float x(wxzerodegisotherm->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					zerodegisotherm = x * Point::m_to_ft;
			}
			int32_t tropopause = WeatherProfilePoint::invalidalt;
			if (wxtropopause) {
				float x(wxtropopause->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					tropopause = x * Point::m_to_ft;
			}
			float cldbdrycover = WeatherProfilePoint::invalidcover;
			if (wxcldbdrycover) {
				float x(wxcldbdrycover->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					cldbdrycover = 0.01 * x;
			}
			int32_t bdrylayerheight = WeatherProfilePoint::invalidalt;
			if (wxbdrylayerheight) {
				float x(wxbdrylayerheight->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					bdrylayerheight = Point::round<int,float>(x * Point::m_to_ft);
			}
			float cldlowcover = WeatherProfilePoint::invalidcover;
			if (wxcldlowcover) {
				float x(wxcldlowcover->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					cldlowcover = 0.01 * x;
			}
			int32_t cldlowbase = WeatherProfilePoint::invalidalt;
			if (wxcldlowbase) {
				float x(wxcldlowbase->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldlowbase = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			int32_t cldlowtop = WeatherProfilePoint::invalidalt;
			if (wxcldlowtop) {
				float x(wxcldlowtop->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldlowtop = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			float cldmidcover = WeatherProfilePoint::invalidcover;
			if (wxcldmidcover) {
				float x(wxcldmidcover->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					cldmidcover = 0.01 * x;
			}
			int32_t cldmidbase = WeatherProfilePoint::invalidalt;
			if (wxcldmidbase) {
				float x(wxcldmidbase->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldmidbase = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			int32_t cldmidtop = WeatherProfilePoint::invalidalt;
			if (wxcldmidtop) {
				float x(wxcldmidtop->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldmidtop = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			float cldhighcover = WeatherProfilePoint::invalidcover;
			if (wxcldhighcover) {
				float x(wxcldhighcover->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					cldhighcover = 0.01 * x;
			}
			int32_t cldhighbase = WeatherProfilePoint::invalidalt;
			if (wxcldhighbase) {
				float x(wxcldhighbase->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldhighbase = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			int32_t cldhightop = WeatherProfilePoint::invalidalt;
			if (wxcldhightop) {
				float x(wxcldhightop->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldhightop = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			float cldconvcover = WeatherProfilePoint::invalidcover;
			if (wxcldconvcover) {
				float x(wxcldconvcover->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					cldconvcover = 0.01 * x;
			}
			int32_t cldconvbase = WeatherProfilePoint::invalidalt;
			if (wxcldconvbase) {
				float x(wxcldconvbase->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldconvbase = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			int32_t cldconvtop = WeatherProfilePoint::invalidalt;
			if (wxcldconvtop) {
				float x(wxcldconvtop->operator()(pt, efftime, 0));
				if (WeatherProfilePoint::is_pressure_valid(x)) {
					float alt(0);
					IcaoAtmosphere<float>::std_pressure_to_altitude(&alt, 0, x * 0.01);
					cldconvtop = Point::round<int,float>(alt * Point::m_to_ft);
				}
			}
			float precip = WeatherProfilePoint::invalidcover;
			if (wxprecip) {
				float x(wxprecip->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					precip = x;
			}
			float preciprate = WeatherProfilePoint::invalidcover;
			if (wxpreciprate) {
				float x(wxpreciprate->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					preciprate = x;
			}
			float convprecip = WeatherProfilePoint::invalidcover;
			if (wxconvprecip) {
				float x(wxconvprecip->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					convprecip = x;
			}
			float convpreciprate = WeatherProfilePoint::invalidcover;
			if (wxconvpreciprate) {
				float x(wxconvpreciprate->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					convpreciprate = x;
			}
			uint16_t flags = 0;
			if (wxcrain) {
				float x(wxcrain->operator()(pt, efftime, 0));
				if (!std::isnan(x) && x >= 0.5 && x <= 10)
					flags |= WeatherProfilePoint::flags_rain;
			}
			if (wxcfrzr) {
				float x(wxcfrzr->operator()(pt, efftime, 0));
				if (!std::isnan(x) && x >= 0.5 && x <= 10)
					flags |= WeatherProfilePoint::flags_freezingrain;
			}
			if (wxcicep) {
				float x(wxcicep->operator()(pt, efftime, 0));
				if (!std::isnan(x) && x >= 0.5 && x <= 10)
					flags |= WeatherProfilePoint::flags_icepellets;
			}
			if (wxcsnow) {
				float x(wxcsnow->operator()(pt, efftime, 0));
				if (!std::isnan(x) && x >= 0.5 && x <= 10)
					flags |= WeatherProfilePoint::flags_snow;
			}
			float lft = WeatherProfilePoint::invalidcover;
			if (wxlft) {
				float x(wxlft->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					lft = x;
			}
			float cape = WeatherProfilePoint::invalidcover;
			if (wxcape) {
				float x(wxcape->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					cape = x;
			}
			float cin = WeatherProfilePoint::invalidcover;
			if (wxcin) {
				float x(wxcin->operator()(pt, efftime, 0));
				if (!std::isnan(x))
					cin = x;
			}
			// day/night
			{
				Glib::DateTime dt(Glib::DateTime::create_now_utc(efftime));
				double sr, ss, twr, tws;
				int rss(SunriseSunset::sun_rise_set(dt.get_year(), dt.get_month(), dt.get_day_of_month(), pt, sr, ss));
				int rtwil(SunriseSunset::civil_twilight(dt.get_year(), dt.get_month(), dt.get_day_of_month(), pt, twr, tws));
				if (rss || rtwil) {
					int r(rtwil ? rtwil : rss);
					if (r < 0)
						flags |= WeatherProfilePoint::flags_night;
					else
						flags |= WeatherProfilePoint::flags_day;
				} else {
					int xtwr(twr * 3600), xtws(tws * 3600), xsr(sr * 3600), xss(ss * 3600);
					int x(dt.get_second() + 60 * (dt.get_minute() + 60 * dt.get_hour()));
					while (xtwr < 0)
						xtwr += 24 * 60 * 60;
					while (xsr < xtwr)
						xsr += 24 * 60 * 60;
					while (xss < xsr)
						xss += 24 * 60 * 60;
					while (xtws < xss)
						xtws += 24 * 60 * 60;
					while (x < xtwr)
						x += 24 * 60 * 60;
					if (x >= xsr && x <= xss)
						flags |= WeatherProfilePoint::flags_day;
					else if (x < xsr)
						flags |= WeatherProfilePoint::flags_dusk;
					else if (x < xtws)
						flags |= WeatherProfilePoint::flags_dawn;
					else
						flags |= WeatherProfilePoint::flags_night;
					if (false)
						std::cerr << "SR/SS: time (" << (x / (24*60*60)) << ')'
							  << std::setw(2) << std::setfill('0') << ((x / (60*60)) % 24) << ':'
							  << std::setw(2) << std::setfill('0') << ((x / 60) % 60) << ':'
							  << std::setw(2) << std::setfill('0') << (x % 60) << " day ("
							  << (xtwr / (24*60*60)) << ')'
							  << std::setw(2) << std::setfill('0') << ((xtwr / (60*60)) % 24) << ':'
							  << std::setw(2) << std::setfill('0') << ((xtwr / 60) % 60) << ':'
							  << std::setw(2) << std::setfill('0') << (xtwr % 60) << " ("
							  << (xsr / (24*60*60)) << ')'
							  << std::setw(2) << std::setfill('0') << ((xsr / (60*60)) % 24) << ':'
							  << std::setw(2) << std::setfill('0') << ((xsr / 60) % 60) << ':'
							  << std::setw(2) << std::setfill('0') << (xsr % 60) << " ("
							  << (xss / (24*60*60)) << ')'
							  << std::setw(2) << std::setfill('0') << ((xss / (60*60)) % 24) << ':'
							  << std::setw(2) << std::setfill('0') << ((xss / 60) % 60) << ':'
							  << std::setw(2) << std::setfill('0') << (xss % 60) << " ("
							  << (xtws / (24*60*60)) << ')'
							  << std::setw(2) << std::setfill('0') << ((xtws / (60*60)) % 24) << ':'
							  << std::setw(2) << std::setfill('0') << ((xtws / 60) % 60) << ':'
							  << std::setw(2) << std::setfill('0') << (xtws % 60) << " flags "
							  << (flags & WeatherProfilePoint::flags_daymask) << std::endl;
				}
			}
			double ldist(t * distleg);
			double rdist(dist + ldist);
			unsigned int wnr(wptnr - 1);
			if (t == 1.0) {
				ldist = 0;
				++wnr;
			}
			wxprof.push_back(WeatherProfilePoint(ldist, rdist, wnr, pt, efftime, alt,
							     zerodegisotherm, tropopause,
							     cldbdrycover, bdrylayerheight,
							     cldlowcover, cldlowbase, cldlowtop,
							     cldmidcover, cldmidbase, cldmidtop,
							     cldhighcover, cldhighbase, cldhightop,
							     cldconvcover, cldconvbase, cldconvtop,
							     precip, preciprate, convprecip, convpreciprate,
							     lft, cape, cin, flags));
			for (unsigned int i = 0; i < nrsfc; ++i) {
				const int16_t isobarlvl(WeatherProfilePoint::isobaric_levels[i]);
				const uint8_t sfc((isobarlvl < 0) ? surface_specific_height_gnd : surface_isobaric_surface);
				if (first ||
				    (wxtemp[i] && (efftime < wxtemp[i]->get_minefftime() ||
						   efftime > wxtemp[i]->get_maxefftime()))) {
					double sfcval((isobarlvl < 0) ? 2 : isobarlvl * 100.0);
					layerlist_t ll(find_layers(find_parameter(param_meteorology_temperature_tmp),
								   efftime, sfc, sfcval));
					wxtemp[i] = interpolate_results(bbox, ll, efftime, sfcval);
					if (wxtemp[i]) {
						wxprof.add_efftime(wxtemp[i]->get_minefftime());
						wxprof.add_efftime(wxtemp[i]->get_maxefftime());
						wxprof.add_reftime(wxtemp[i]->get_minreftime());
						wxprof.add_reftime(wxtemp[i]->get_maxreftime());
					}
					if (trace_loads) {
						std::cerr << "Temperature: wptnr " << wptnr << " t " << t << " sfc " << i
							  << " ll " << ll.size() << " interp " << (wxtemp[i] ? "yes" : "no")
							  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601()
							  << "  " << Glib::TimeVal(efftime, 0).as_iso8601();
						if (wxtemp[i])
							std::cerr << " (" << Glib::TimeVal(wxtemp[i]->get_minefftime(), 0).as_iso8601()
								  << ".." << Glib::TimeVal(wxtemp[i]->get_maxefftime(), 0).as_iso8601() << ')';
						std::cerr << std::endl;
					}
				}
				if (first ||
				    (wxugrd[i] && (efftime < wxugrd[i]->get_minefftime() ||
						   efftime > wxugrd[i]->get_maxefftime()))) {
					double sfcval((isobarlvl < 0) ? 10 : isobarlvl * 100.0);
					layerlist_t ll(find_layers(find_parameter(param_meteorology_momentum_ugrd),
								   efftime, sfc, sfcval));
					wxugrd[i] = interpolate_results(bbox, ll, efftime, sfcval);
					if (wxugrd[i]) {
						wxprof.add_efftime(wxugrd[i]->get_minefftime());
						wxprof.add_efftime(wxugrd[i]->get_maxefftime());
						wxprof.add_reftime(wxugrd[i]->get_minreftime());
						wxprof.add_reftime(wxugrd[i]->get_maxreftime());
					}
					if (trace_loads) {
						std::cerr << "U Gradient Wind: wptnr " << wptnr << " t " << t << " sfc " << i
							  << " ll " << ll.size() << " interp " << (wxugrd[i] ? "yes" : "no")
							  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601()
							  << "  " << Glib::TimeVal(efftime, 0).as_iso8601();
						if (wxugrd[i])
							std::cerr << " (" << Glib::TimeVal(wxugrd[i]->get_minefftime(), 0).as_iso8601()
								  << ".." << Glib::TimeVal(wxugrd[i]->get_maxefftime(), 0).as_iso8601() << ')';
						std::cerr << std::endl;
					}
				}
				if (first ||
				    (wxvgrd[i] && (efftime < wxvgrd[i]->get_minefftime() ||
						   efftime > wxvgrd[i]->get_maxefftime()))) {
					double sfcval((isobarlvl < 0) ? 10 : isobarlvl * 100.0);
					layerlist_t ll(find_layers(find_parameter(param_meteorology_momentum_vgrd),
								   efftime, sfc, sfcval));
					wxvgrd[i] = interpolate_results(bbox, ll, efftime, sfcval);
					if (wxvgrd[i]) {
						wxprof.add_efftime(wxvgrd[i]->get_minefftime());
						wxprof.add_efftime(wxvgrd[i]->get_maxefftime());
						wxprof.add_reftime(wxvgrd[i]->get_minreftime());
						wxprof.add_reftime(wxvgrd[i]->get_maxreftime());
					}
					if (trace_loads) {
						std::cerr << "V Gradient Wind: wptnr " << wptnr << " t " << t << " sfc " << i
							  << " ll " << ll.size() << " interp " << (wxvgrd[i] ? "yes" : "no")
							  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601()
							  << "  " << Glib::TimeVal(efftime, 0).as_iso8601();
						if (wxvgrd[i])
							std::cerr << " (" << Glib::TimeVal(wxvgrd[i]->get_minefftime(), 0).as_iso8601()
								  << ".." << Glib::TimeVal(wxvgrd[i]->get_maxefftime(), 0).as_iso8601() << ')';
						std::cerr << std::endl;
					}
				}
				if (first ||
				    (wxrelhum[i] && (efftime < wxrelhum[i]->get_minefftime() ||
						     efftime > wxrelhum[i]->get_maxefftime()))) {
					double sfcval((isobarlvl < 0) ? 2 : isobarlvl * 100.0);
					layerlist_t ll(find_layers(find_parameter(param_meteorology_moisture_rh),
								   efftime, sfc, sfcval));
					wxrelhum[i] = interpolate_results(bbox, ll, efftime, sfcval);
					if (wxrelhum[i]) {
						wxprof.add_efftime(wxrelhum[i]->get_minefftime());
						wxprof.add_efftime(wxrelhum[i]->get_maxefftime());
						wxprof.add_reftime(wxrelhum[i]->get_minreftime());
						wxprof.add_reftime(wxrelhum[i]->get_maxreftime());
					}
					if (trace_loads) {
						std::cerr << "Relative Humidity: wptnr " << wptnr << " t " << t << " sfc " << i
							  << " ll " << ll.size() << " interp " << (wxrelhum[i] ? "yes" : "no")
							  << " efftime " << Glib::TimeVal(efftime, 0).as_iso8601()
							  << "  " << Glib::TimeVal(efftime, 0).as_iso8601();
						if (wxrelhum[i])
							std::cerr << " (" << Glib::TimeVal(wxrelhum[i]->get_minefftime(), 0).as_iso8601()
								  << ".." << Glib::TimeVal(wxrelhum[i]->get_maxefftime(), 0).as_iso8601() << ')';
						std::cerr << std::endl;
					}
				}
				float temp = WeatherProfilePoint::invalidcover;
				if (wxtemp[i]) {
					float x(wxtemp[i]->operator()(pt, efftime, (isobarlvl < 0) ? 2 : isobarlvl * 100.0));
					if (!std::isnan(x))
						temp = x;
				}
				float ugrd = WeatherProfilePoint::invalidcover, vgrd = WeatherProfilePoint::invalidcover;
				if (wxugrd[i] && wxvgrd[i]) {
					float u(wxugrd[i]->operator()(pt, efftime, (isobarlvl < 0) ? 10 : isobarlvl * 100.0));
					float v(wxvgrd[i]->operator()(pt, efftime, (isobarlvl < 0) ? 10 : isobarlvl * 100.0));
					if (!std::isnan(u) && !std::isnan(v)) {
						std::pair<float,float> wnd(wxugrd[i]->get_layer()->get_grid()->transform_axes(u, v));
						if (!std::isnan(wnd.first) && !std::isnan(wnd.second)) {
							ugrd = wnd.first;
							vgrd = wnd.second;
						}
					}
				}
				;		float rh = WeatherProfilePoint::invalidcover;
				if (wxrelhum[i]) {
					float x(wxrelhum[i]->operator()(pt, efftime, (isobarlvl < 0) ? 2 : isobarlvl * 100.0));
					if (!std::isnan(x))
						rh = x;
				}
				float hwsh = WeatherProfilePoint::invalidcover;
				if (!std::isnan(ugrd) && !std::isnan(vgrd) && wxugrd[i] && wxvgrd[i]) {
					float w0(sqrtf(ugrd * ugrd + vgrd * vgrd));
					hwsh = 0;
					unsigned int hwshcnt(0);
					static const double hwshdist_nmi = 50;
					static const double hwshdist_m = hwshdist_nmi * (1000.0 / Point::km_to_nmi_dbl);
					for (unsigned int j = 0; j < 4; ++j) {
						Point pt1(pt.spheric_course_distance_nmi(j * 90, hwshdist_nmi));
						if (!wxugrd[i] || !wxvgrd[i])
							continue;
						float ugrd1(wxugrd[i]->operator()(pt, efftime, (isobarlvl < 0) ? 10 : isobarlvl * 100.0));
						float vgrd1(wxvgrd[i]->operator()(pt, efftime, (isobarlvl < 0) ? 10 : isobarlvl * 100.0));
						if (std::isnan(ugrd1) || std::isnan(vgrd1))
							continue;
						{
							std::pair<float,float> wnd(wxugrd[i]->get_layer()->get_grid()->transform_axes(ugrd1, vgrd1));
							ugrd1 = wnd.first;
							vgrd1 = wnd.second;
						}
						if (std::isnan(ugrd1) || std::isnan(vgrd1))
							continue;
						float w1(sqrtf(ugrd1 * ugrd1 + vgrd1 * vgrd1));
						hwsh += fabsf(w0 - w1) * (1.0 / hwshdist_m);
						++hwshcnt;
					}
					if (hwshcnt)
						hwsh /= hwshcnt;
					else
						hwsh = WeatherProfilePoint::invalidcover;
				}
				wxprof.back()[i] = WeatherProfilePoint::Surface(ugrd, vgrd, temp, rh, hwsh, WeatherProfilePoint::invalidcover);
			}
			for (unsigned int i = 0; i < nrsfc; ++i) {
				if (WeatherProfilePoint::isobaric_levels[i] < 0)
					continue;
				float vwsh(0);
				unsigned int vwshcnt(0);
				float w0(wxprof.back()[i].get_wind());
				if (std::isnan(w0))
					continue;
				for (int k = -1; k <= 1; k += 2) {
					if (k < 0 && !i)
						continue;
					if (i + k >= nrsfc)
						continue;
					if (WeatherProfilePoint::isobaric_levels[i + k] < 0)
						continue;
					float w1(wxprof.back()[i + k].get_wind());
					if (std::isnan(w1))
						continue;
					vwsh += fabsf(w0 - w1) / abs(WeatherProfilePoint::altitudes[i] - WeatherProfilePoint::altitudes[i + k]) * Point::m_to_ft;
					++vwshcnt;
				}
				if (vwshcnt)
					vwsh /= vwshcnt;
				else
					vwsh = WeatherProfilePoint::invalidcover;
				wxprof.back()[i].set_vwsh(vwsh);
			}
			if (t == 1.0)
				break;
		}
		dist += distleg;
	}
	if (!false) {
		for (WeatherProfile::const_iterator pi(wxprof.begin()), pe(wxprof.end()); pi != pe; ++pi) {
			std::cerr << "D " << pi->get_routedist() << " W" << pi->get_routeindex() << " CLD B ";
			if (std::isnan(pi->get_cldbdrycover()) ||
			    pi->get_bdrylayerheight() == GRIB2::WeatherProfilePoint::invalidalt)
				std::cerr << "--";
			else
				std::cerr << pi->get_cldbdrycover() << ' ' << pi->get_bdrylayerheight();
			std::cerr << " L ";
			if (std::isnan(pi->get_cldlowcover()) ||
			    pi->get_cldlowbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldlowtop() == GRIB2::WeatherProfilePoint::invalidalt)
				std::cerr << "--";
			else
				std::cerr << pi->get_cldlowcover() << ' ' << pi->get_cldlowbase() << ' ' << pi->get_cldlowtop();
			std::cerr << " M ";
			if (std::isnan(pi->get_cldmidcover()) ||
			    pi->get_cldmidbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldmidtop() == GRIB2::WeatherProfilePoint::invalidalt)
				std::cerr << "--";
			else
				std::cerr << pi->get_cldmidcover() << ' ' << pi->get_cldmidbase() << ' ' << pi->get_cldmidtop();
			std::cerr << " H ";
			if (std::isnan(pi->get_cldhighcover()) ||
			    pi->get_cldhighbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldhightop() == GRIB2::WeatherProfilePoint::invalidalt)
				std::cerr << "--";
			else
				std::cerr << pi->get_cldhighcover() << ' ' << pi->get_cldhighbase() << ' ' << pi->get_cldhightop();
			std::cerr << " C ";
			if (std::isnan(pi->get_cldconvcover()) ||
			    pi->get_cldconvbase() == GRIB2::WeatherProfilePoint::invalidalt ||
			    pi->get_cldconvtop() == GRIB2::WeatherProfilePoint::invalidalt)
				std::cerr << "--";
			else
				std::cerr << pi->get_cldconvcover() << ' ' << pi->get_cldconvbase() << ' ' << pi->get_cldconvtop();
			std::cerr << " precip " << (pi->get_preciprate() * 86400.0)
				  << "mm/day conv " << (pi->get_convpreciprate() * 86400.0) << "mm/day";
			switch (pi->get_flags() & WeatherProfilePoint::flags_daymask) {
			default:
			case WeatherProfilePoint::flags_day:
				std::cerr << " day";
				break;

			case WeatherProfilePoint::flags_dusk:
				std::cerr << " dusk";
				break;

			case WeatherProfilePoint::flags_night:
				std::cerr << " night";
				break;

			case WeatherProfilePoint::flags_dawn:
				std::cerr << " dawn";
				break;
			}
			if (pi->get_flags() & WeatherProfilePoint::flags_rain)
				std::cerr << " rain";
			if (pi->get_flags() & WeatherProfilePoint::flags_freezingrain)
				std::cerr << " freezingrain";
			if (pi->get_flags() & WeatherProfilePoint::flags_icepellets)
				std::cerr << " icepellets";
			if (pi->get_flags() & WeatherProfilePoint::flags_snow)
				std::cerr << " snow";
			std::cerr << std::endl;
		}
	}
	return wxprof;
}
