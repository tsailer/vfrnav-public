//
// C++ Interface: grib2
//
// Description: Gridded Binary weather files
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013, 2014, 2016, 2017
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef GRIB2_H
#define GRIB2_H

#include "sysdeps.h"
#include "geom.h"

#include <set>
#include <limits>
#include <stdint.h>
#include <glibmm.h>
#include <gdkmm.h>
#include <glibmm/datetime.h>
#ifdef HAVE_LIBCRYPTO
#include <openssl/md4.h>
#endif

class FPlanRoute;

class GRIB2 {
public:
	static const uint8_t surface_ground_or_water = 1;
	static const uint8_t surface_cloud_base = 2;
	static const uint8_t surface_cloud_top = 3;
	static const uint8_t surface_0degc_isotherm = 4;
	static const uint8_t surface_lifted_adiabatic_condensation = 5;
	static const uint8_t surface_maximum_wind = 6;
	static const uint8_t surface_tropopause = 7;
	static const uint8_t surface_nominal_atmosphere_top = 8;
	static const uint8_t surface_sea_bottom = 9;
	static const uint8_t surface_entire_atmosphere = 10;
	static const uint8_t surface_cumulonimbus_base = 11;
	static const uint8_t surface_cumulonimbus_top = 12;
	static const uint8_t surface_isothermal_level = 20;
	static const uint8_t surface_isobaric_surface = 100;
	static const uint8_t surface_mean_sea_level = 101;
	static const uint8_t surface_specific_altitude_amsl = 102;
	static const uint8_t surface_specific_height_gnd = 103;
	static const uint8_t surface_sigma_level = 104;
	static const uint8_t surface_hybrid_level = 105;
	static const uint8_t surface_depth_below_land_surface = 106;
	static const uint8_t surface_isentropic_level = 107;
	static const uint8_t surface_level_specific_pressure_difference = 108;
	static const uint8_t surface_potential_vorticity_surface = 109;
	static const uint8_t surface_eta_level = 111;
	static const uint8_t surface_logarithmic_hybrid_coordinate = 113;
	static const uint8_t surface_mixed_layer_depth = 117;
	static const uint8_t surface_hybrid_height_level = 118;
	static const uint8_t surface_hybrid_pressure_level = 119;
	static const uint8_t surface_pressure_thickness = 120;
	static const uint8_t surface_generalized_vertical_height_coordinate = 150;
	static const uint8_t surface_depth_below_sealevel = 160;
	static const uint8_t surface_depth_below_water_surface = 161;
	static const uint8_t surface_lake_bottom = 162;
	static const uint8_t surface_sediment_bottom = 163;
	static const uint8_t surface_thermally_active_sediment_bottom = 164;
	static const uint8_t surface_thermal_wave_penetrated_sediment_bottom = 165;
	static const uint8_t surface_maxing_layer = 166;
	static const uint8_t surface_ionospheric_d_level = 170;
	static const uint8_t surface_ionospheric_e_level = 171;
	static const uint8_t surface_ionospheric_f1_level = 172;
	static const uint8_t surface_ionospheric_f2_level = 173;
	static const uint8_t surface_entire_atmosphere_as_single_layer = 200;
	static const uint8_t surface_entire_ocean_as_single_layer = 201;
	static const uint8_t surface_highest_tropospheric_freezing_level = 204;
	static const uint8_t surface_grid_scale_cloud_bottom = 206;
	static const uint8_t surface_grid_scale_cloud_top = 207;
	static const uint8_t surface_boundary_layer_cloud_bottom = 209;
	static const uint8_t surface_boundary_layer_cloud_top = 210;
	static const uint8_t surface_boundary_layer_cloud = 211;
	static const uint8_t surface_low_cloud_bottom = 212;
	static const uint8_t surface_low_cloud_top = 213;
	static const uint8_t surface_low_cloud = 214;
	static const uint8_t surface_cloud_ceiling = 215;
	static const uint8_t surface_planetary_boundary_layer = 220;
	static const uint8_t surface_layer_between_hybrid_levels = 221;
	static const uint8_t surface_middle_cloud_bottom = 222;
	static const uint8_t surface_middle_cloud_top = 223;
	static const uint8_t surface_middle_cloud = 224;
	static const uint8_t surface_top_cloud_bottom = 232;
	static const uint8_t surface_top_cloud_top = 233;
	static const uint8_t surface_top_cloud = 234;
	static const uint8_t surface_ocean_isotherm_level = 235;
	static const uint8_t surface_layer_between_depths_below_ocean_surface = 236;
	static const uint8_t surface_ocean_mixing_layer_bottom = 237;
	static const uint8_t surface_ocean_isothermal_layer_bottom = 238;
	static const uint8_t surface_ocean_surface_26c_isothermal = 239;
	static const uint8_t surface_ocean_mixing_layer = 240;
	static const uint8_t surface_ordered_sequence_of_data = 241;
	static const uint8_t surface_convective_cloud_bottom = 242;
	static const uint8_t surface_convective_cloud_top = 243;
	static const uint8_t surface_convective_cloud = 244;
	static const uint8_t surface_lowest_wet_bulb_zero = 245;
	static const uint8_t surface_maximum_equivalent_potential_temperature = 246;
	static const uint8_t surface_equilibrium_level = 247;
	static const uint8_t surface_shallow_convective_cloud_bottom = 248;
	static const uint8_t surface_shallow_convective_cloud_top = 249;
	static const uint8_t surface_shallow_convective_cloud = 251;
	static const uint8_t surface_deep_convective_cloud_top = 252;
	static const uint8_t surface_supercooled_liquid_water_bottom = 253;
	static const uint8_t surface_supercooled_liquid_water_top = 254;
	static const uint8_t surface_missing = 255;

	static const uint8_t discipline_meteorology = 0;
	static const uint8_t discipline_hydrology = 1;
	static const uint8_t discipline_landsurface = 2;
	static const uint8_t discipline_space = 3;
	static const uint8_t discipline_spaceweather = 4;
	static const uint8_t discipline_oceanography = 10;

	/*
	  #!/usr/bin/perl

	  my @paramcat;

	  open FH, "<", "grib2.h" || die "cannot open grib2.h";
	  while (<FH>) {
	  if (/^\s*static\s+const\s+uint16_t\s+paramcat_([A-Za-z0-9_]+)\s+=/) {
	  push @paramcat, $1;
	  }
	  }
	  close FH;

	  open FH, "<", "grib2paramtbl.cc" || die "cannot open grib2paramtbl.cc";
	  while (<FH>) {
	  if (/^\s*{\s+&paramcategory\s*\[\s*(\d+)\s*\]\s*,\s*"[^"]*"\s*,\s*[A-Za-z0-9_]+\s*,\s*"([A-Za-z0-9_]+)"\s*,\s*(\d+)\s*,/) {
	  my $catidx = $1;
	  my $abbrev = $2;
	  my $id = $3;
	  if ($catidx >= scalar @paramcat) {
	  print STDERR "Parameter Category $1 ($2) out of range\n";
	  next;
	  }
	  my $cat = $paramcat[$catidx];
	  my $lcabbrev = lc $abbrev;
	  print "\tstatic const uint32_t param_${cat}_${lcabbrev} = ${id} | (paramcat_${cat} << 8);\n";
	  }
	  }
	  close FH;
	*/

	static const uint16_t paramcat_meteorology_temperature = 0 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_moisture = 1 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_momentum = 2 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_mass = 3 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_shortwave_radiation = 4 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_longwave_radiation = 5 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_cloud = 6 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_thermodynamic_stability = 7 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_kinematic_stability = 8 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_temperature_prob = 9 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_moisture_prob = 10 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_momentum_prob = 11 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_mass_probabilities = 12 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_aerosols = 13 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_trace_gases = 14 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_radar = 15 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_forecast_radar = 16 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_electrodynamics = 17 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_nuclear = 18 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_atmosphere_physics = 19 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_atmosphere_chemistry = 20 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_string = 190 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_miscellaneous = 191 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_covariance = 192 | (discipline_meteorology << 8);
	static const uint16_t paramcat_meteorology_missing = 255 | (discipline_meteorology << 8);
	static const uint16_t paramcat_hydrology_basic = 0 | (discipline_hydrology << 8);
	static const uint16_t paramcat_hydrology_prob = 1 | (discipline_hydrology << 8);
	static const uint16_t paramcat_hydrology_inlandwater_sediment = 2 | (discipline_hydrology << 8);
	static const uint16_t paramcat_hydrology_missing = 255 | (discipline_hydrology << 8);
	static const uint16_t paramcat_landsurface_vegetation = 0 | (discipline_landsurface << 8);
	static const uint16_t paramcat_landsurface_agriaquaculture = 1 | (discipline_landsurface << 8);
	static const uint16_t paramcat_landsurface_transportation = 2 | (discipline_landsurface << 8);
	static const uint16_t paramcat_landsurface_soil = 3 | (discipline_landsurface << 8);
	static const uint16_t paramcat_landsurface_fire = 4 | (discipline_landsurface << 8);
	static const uint16_t paramcat_landsurface_missing = 255 | (discipline_landsurface << 8);
	static const uint16_t paramcat_space_image = 0 | (discipline_space << 8);
	static const uint16_t paramcat_space_quantitative = 1 | (discipline_space << 8);
	static const uint16_t paramcat_space_forecast = 192 | (discipline_space << 8);
	static const uint16_t paramcat_space_missing = 255 | (discipline_space << 8);
	static const uint16_t paramcat_spaceweather_temperature = 0 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_momentum = 1 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_charged_particles = 2 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_fields = 3 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_energetic_particles = 4 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_waves = 5 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_solar = 6 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_terrestrial = 7 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_imagery = 8 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_ionneutral_coupling = 9 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_spaceweather_missing = 255 | (discipline_spaceweather << 8);
	static const uint16_t paramcat_oceanography_waves = 0 | (discipline_oceanography << 8);
	static const uint16_t paramcat_oceanography_currents = 1 | (discipline_oceanography << 8);
	static const uint16_t paramcat_oceanography_ice = 2 | (discipline_oceanography << 8);
	static const uint16_t paramcat_oceanography_surface = 3 | (discipline_oceanography << 8);
	static const uint16_t paramcat_oceanography_subsurface = 4 | (discipline_oceanography << 8);
	static const uint16_t paramcat_oceanography_miscellaneous = 191 | (discipline_oceanography << 8);
	static const uint16_t paramcat_oceanography_missing = 255 | (discipline_oceanography << 8);

	static const uint32_t param_meteorology_temperature_tmp = 0 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_vtmp = 1 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_pot = 2 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_epot = 3 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_tmax = 4 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_tmin = 5 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_dpt = 6 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_depr = 7 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_lapr = 8 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_tmpa = 9 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_lhtfl = 10 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_shtfl = 11 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_heatx = 12 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_wcf = 13 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_mindpd = 14 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_vptmp = 15 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_snohf = 16 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_skint = 17 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_snot = 18 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_ttcht = 19 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_tdcht = 20 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_snohf_2 = 192 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_ttrad = 193 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_rev = 194 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_lrghr = 195 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_cnvhr = 196 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_thflx = 197 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_ttdia = 198 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_ttphy = 199 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_tsd1d = 200 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_shahr = 201 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_vdfhr = 202 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_thz0 = 203 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_temperature_tchp = 204 | (paramcat_meteorology_temperature << 8);
	static const uint32_t param_meteorology_moisture_spfh = 0 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_rh = 1 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_mixr = 2 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_pwat = 3 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_vapp = 4 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_satd = 5 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_evp = 6 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_prate = 7 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_apcp = 8 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_ncpcp = 9 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_acpcp = 10 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snod = 11 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_srweq = 12 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_weasd = 13 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snoc = 14 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snol = 15 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snom = 16 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snoag = 17 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_absh = 18 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_ptype = 19 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_iliqw = 20 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcond = 21 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_clwmr = 22 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_icmr = 23 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_rwmr = 24 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snmr = 25 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_mconv = 26 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_maxrh = 27 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_maxah = 28 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_asnow = 29 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_pwcat = 30 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_hail = 31 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_grle = 32 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_crain = 33 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cfrzr = 34 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cicep = 35 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_csnow = 36 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cprat = 37 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_mdiv = 38 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cpofp = 39 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_pevap = 40 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_pevpr = 41 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snowc = 42 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_frain = 43 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_rime = 44 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcolr = 45 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcols = 46 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_lswp = 47 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cwp = 48 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_twatp = 49 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tsnowp = 50 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcwat = 51 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tprate = 52 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tsrwe = 53 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_lsprate = 54 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_csrwe = 55 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_lssrwe = 56 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tsrate = 57 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_csrate = 58 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_lssrate = 59 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_sdwe = 60 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_sden = 61 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_sevap = 62 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tciwv = 64 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_rprate = 65 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_sprate = 66 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_fprate = 67 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_iprate = 68 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcolw = 69 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcoli = 70 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_hailmxr = 71 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcolh = 72 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_hailpr = 73 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcolg = 74 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_gprate = 75 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_crrate = 76 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_lsrrate = 77 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcolwa = 78 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_evarate = 79 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_totcon = 80 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcicon = 81 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cimixr = 82 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_scllwc = 83 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_scliwc = 84 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_srainw = 85 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_ssnoww = 86 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tkmflx = 90 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_ukmflx = 91 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_vkmflx = 92 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_crain_2 = 192 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cfrzr_2 = 193 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cicep_2 = 194 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_csnow_2 = 195 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cprat_2 = 196 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_mdiv_2 = 197 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_minrh = 198 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_pevap_2 = 199 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_pevpr_2 = 200 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snowc_2 = 201 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_frain_2 = 202 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_rime_2 = 203 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcolr_2 = 204 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcols_2 = 205 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tipd = 206 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_ncip = 207 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snot = 208 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tclsw = 209 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tcolm = 210 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_emnp = 211 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_sbsno = 212 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_cnvmr = 213 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_shamr = 214 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_vdfmr = 215 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_condp = 216 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_lrgmr = 217 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_qz0 = 218 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_qmax = 219 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_qmin = 220 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_arain = 221 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_snowt = 222 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_apcpn = 223 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_acpcpn = 224 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_frzr = 225 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_pwther = 226 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_frozr = 227 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_tsnow = 241 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_moisture_rhpw = 242 | (paramcat_meteorology_moisture << 8);
	static const uint32_t param_meteorology_momentum_wdir = 0 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_wind = 1 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_ugrd = 2 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vgrd = 3 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_strm = 4 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vpot = 5 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_mntsf = 6 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_sgcvv = 7 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vvel = 8 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_dzdt = 9 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_absv = 10 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_absd = 11 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_relv = 12 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_reld = 13 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_pvort = 14 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vucsh = 15 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vvcsh = 16 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_uflx = 17 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vflx = 18 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_wmixe = 19 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_blydp = 20 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_maxgust = 21 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_gust = 22 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_ugust = 23 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vgust = 24 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vwsh = 25 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_mflx = 26 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_ustm = 27 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vstm = 28 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_cd = 29 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_fricv = 30 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_tdcmom = 31 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_etacvv = 32 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_windf = 33 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vwsh_2 = 192 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_mflx_2 = 193 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_ustm_2 = 194 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vstm_2 = 195 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_cd_2 = 196 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_fricv_2 = 197 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_lauv = 198 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_louv = 199 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_lavv = 200 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_lovv = 201 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_lapp = 202 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_lopp = 203 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vedh = 204 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_covmz = 205 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_covtz = 206 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_covtm = 207 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vdfua = 208 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vdfva = 209 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_gwdu = 210 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_gwdv = 211 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_cnvu = 212 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_cnvv = 213 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_wtend = 214 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_omgalf = 215 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_cngwdu = 216 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_cngwdv = 217 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_lmv = 218 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_pvmww = 219 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_maxuvv = 220 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_maxdvv = 221 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_maxuw = 222 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_maxvw = 223 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_momentum_vrate = 224 | (paramcat_meteorology_momentum << 8);
	static const uint32_t param_meteorology_mass_pres = 0 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_prmsl = 1 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_ptend = 2 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_icaht = 3 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_gp = 4 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_hgt = 5 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_dist = 6 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_hstdv = 7 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_presa = 8 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_gpa = 9 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_den = 10 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_alts = 11 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_thick = 12 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_presalt = 13 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_denalt = 14 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_5wavh = 15 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_hpbl = 18 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_5wava = 19 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_sdsgso = 20 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_aosgso = 21 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_ssgso = 22 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_gwd = 23 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_asgso = 24 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_nlpres = 25 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_mslet = 192 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_5wavh_2 = 193 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_hpbl_2 = 196 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_5wava_2 = 197 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_mslma = 198 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_tslsa = 199 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_plpl = 200 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_lpsx = 201 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_lpsy = 202 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_hgtx = 203 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_hgty = 204 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_layth = 205 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_nlgsp = 206 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_cnvumf = 207 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_cnvdmf = 208 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_cnvdemf = 209 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_lmh = 210 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_hgtn = 211 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_mass_presn = 212 | (paramcat_meteorology_mass << 8);
	static const uint32_t param_meteorology_shortwave_radiation_nswrs = 0 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_nswrt = 1 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_swavr = 2 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_grad = 3 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_brtmp = 4 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_lwrad = 5 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_swrad = 6 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_dswrf = 7 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_uswrf = 8 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_nswrf = 9 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_photar = 10 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_nswrfcs = 11 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_dwuvr = 12 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_uviucs = 50 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_uvi = 51 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_dswrf_2 = 192 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_uswrf_2 = 193 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_duvb = 194 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_cduvb = 195 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_csdsf = 196 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_swhr = 197 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_csusf = 198 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_cfnsf = 199 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_vbdsf = 200 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_vddsf = 201 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_nbdsf = 202 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_nddsf = 203 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_dtrf = 204 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_shortwave_radiation_utrf = 205 | (paramcat_meteorology_shortwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_nlwrs = 0 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_nlwrt = 1 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_lwavr = 2 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_dlwrf = 3 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_ulwrf = 4 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_nlwrf = 5 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_nlwrcs = 6 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_dlwrf_2 = 192 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_ulwrf_2 = 193 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_lwhr = 194 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_csulf = 195 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_csdlf = 196 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_longwave_radiation_cfnlf = 197 | (paramcat_meteorology_longwave_radiation << 8);
	static const uint32_t param_meteorology_cloud_cice = 0 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcdc = 1 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdcon = 2 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_lcdc = 3 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_mcdc = 4 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_hcdc = 5 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cwat = 6 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdca = 7 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdcty = 8 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tmaxt = 9 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_thunc = 10 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdcb = 11 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdct = 12 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_ceil = 13 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdlyr = 14 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cwork = 15 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cuefi = 16 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcond = 17 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcolw = 18 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcoli = 19 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcolc = 20 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_fice = 21 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdcc = 22 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdcimr = 23 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_suns = 24 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cbhe = 25 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_hconcb = 26 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_hconct = 27 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_nconcd = 28 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_nccice = 29 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_ndencd = 30 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_ndcice = 31 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_fraccc = 32 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_sunsd = 33 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cdlyr_2 = 192 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cwork_2 = 193 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_cuefi_2 = 194 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcond_2 = 195 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcolw_2 = 196 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcoli_2 = 197 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_tcolc_2 = 198 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_fice_2 = 199 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_mflux = 200 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_cloud_sunsd_2 = 201 | (paramcat_meteorology_cloud << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_pli = 0 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_bli = 1 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_kx = 2 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_kox = 3 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_totalx = 4 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_sx = 5 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_cape = 6 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_cin = 7 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_hlcy = 8 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_ehlx = 9 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_lftx = 10 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_4lftx = 11 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_ri = 12 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_shwinx = 13 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_uphl = 15 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_lftx_2 = 192 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_4lftx_2 = 193 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_ri_2 = 194 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_cwdi = 195 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_uvi = 196 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_uphl_2 = 197 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_lai = 198 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_thermodynamic_stability_mxuphl = 199 | (paramcat_meteorology_thermodynamic_stability << 8);
	static const uint32_t param_meteorology_aerosols_aerot = 0 | (paramcat_meteorology_aerosols << 8);
	static const uint32_t param_meteorology_aerosols_pmtc = 192 | (paramcat_meteorology_aerosols << 8);
	static const uint32_t param_meteorology_aerosols_pmtf = 193 | (paramcat_meteorology_aerosols << 8);
	static const uint32_t param_meteorology_aerosols_lpmtf = 194 | (paramcat_meteorology_aerosols << 8);
	static const uint32_t param_meteorology_aerosols_lipmf = 195 | (paramcat_meteorology_aerosols << 8);
	static const uint32_t param_meteorology_trace_gases_tozne = 0 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_o3mr = 1 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_tcioz = 2 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_o3mr_2 = 192 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_ozcon = 193 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_ozcat = 194 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_vdfoz = 195 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_poz = 196 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_toz = 197 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_pozt = 198 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_pozo = 199 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_ozmax1 = 200 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_ozmax8 = 201 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_pdmax1 = 202 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_trace_gases_pdmax24 = 203 | (paramcat_meteorology_trace_gases << 8);
	static const uint32_t param_meteorology_radar_bswid = 0 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_bref = 1 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_brvel = 2 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_vil = 3 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_lmaxbr = 4 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_prec = 5 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rdsp1 = 6 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rdsp2 = 7 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rdsp3 = 8 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rfcd = 9 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rfci = 10 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rfsnow = 11 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rfrain = 12 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rfgrpl = 13 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_radar_rfhail = 14 | (paramcat_meteorology_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refzr = 0 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refzi = 1 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refzc = 2 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_retop = 3 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refd = 4 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refc = 5 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refzr_2 = 192 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refzi_2 = 193 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refzc_2 = 194 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refd_2 = 195 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_refc_2 = 196 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_retop_2 = 197 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_forecast_radar_maxref = 198 | (paramcat_meteorology_forecast_radar << 8);
	static const uint32_t param_meteorology_electrodynamics_ltng = 192 | (paramcat_meteorology_electrodynamics << 8);
	static const uint32_t param_meteorology_nuclear_acces = 0 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_aciod = 1 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_acradp = 2 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_gdces = 3 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_gdiod = 4 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_gdradp = 5 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_tiaccp = 6 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_tiacip = 7 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_tiacrp = 8 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_aircon = 10 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_wetdep = 11 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_drydep = 12 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_nuclear_totlwd = 13 | (paramcat_meteorology_nuclear << 8);
	static const uint32_t param_meteorology_atmosphere_physics_vis = 0 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_albdo = 1 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_tstm = 2 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_mixht = 3 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_volash = 4 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_icit = 5 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_icib = 6 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_ici = 7 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_turbt = 8 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_turbb = 9 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_turb = 10 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_tke = 11 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_pblreg = 12 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_conti = 13 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_contet = 14 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_contt = 15 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_contb = 16 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_mxsalb = 17 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_snfalb = 18 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_salbd = 19 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_icip = 20 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_ctp = 21 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_cat = 22 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_sldp = 23 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_contke = 24 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_wiww = 25 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_convo = 26 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_mxsalb_2 = 192 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_snfalb_2 = 193 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_srcono = 194 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_mrcono = 195 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_hrcono = 196 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_torprob = 197 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_hailprob = 198 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_windprob = 199 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_storprob = 200 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_shailpro = 201 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_swindpro = 202 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_tstmc = 203 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_mixly = 204 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_flght = 205 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_cicel = 206 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_civis = 207 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_ciflt = 208 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_lavni = 209 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_havni = 210 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_sbsalb = 211 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_swsalb = 212 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_nbsalb = 213 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_nwsalb = 214 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_prsvr = 215 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_prsigsvr = 216 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_sipd = 217 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_epsr = 218 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_tpfi = 219 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_svrts = 220 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_procon = 221 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_convp = 222 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_vaftd = 232 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_icprb = 233 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_physics_icsev = 234 | (paramcat_meteorology_atmosphere_physics << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_massden = 0 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_colmd = 1 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_massmr = 2 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_aemflx = 3 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_anpmflx = 4 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_anpemflx = 5 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_sddmflx = 6 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_swdmflx = 7 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_aremflx = 8 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_wlsmflx = 9 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_wdcpmflx = 10 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_sedmflx = 11 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_ddmflx = 12 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_tranhh = 13 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_trsds = 14 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_aia = 50 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_conair = 51 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_vmxr = 52 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_cgprc = 53 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_cgdrc = 54 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_sflux = 55 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_coaia = 56 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_tyaba = 57 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_tyaal = 58 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_ancon = 59 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_saden = 100 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_atmtk = 101 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_aotk = 102 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_ssalbk = 103 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_asysfk = 104 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_aecoef = 105 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_aacoef = 106 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_albsat = 107 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_albgrd = 108 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_alesat = 109 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_atmosphere_chemistry_alegrd = 110 | (paramcat_meteorology_atmosphere_chemistry << 8);
	static const uint32_t param_meteorology_string_atext = 0 | (paramcat_meteorology_string << 8);
	static const uint32_t param_meteorology_miscellaneous_tsec = 0 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_geolat = 1 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_geolon = 2 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_nlat = 192 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_elon = 193 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_tsec_2 = 194 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_mlyno = 195 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_nlatn = 196 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_miscellaneous_elonn = 197 | (paramcat_meteorology_miscellaneous << 8);
	static const uint32_t param_meteorology_covariance_covmz = 1 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covtz = 2 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covtm = 3 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covtw = 4 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covzz = 5 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covmm = 6 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covqz = 7 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covqm = 8 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covtvv = 9 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covqvv = 10 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covpsps = 11 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covqq = 12 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covvvvv = 13 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_meteorology_covariance_covtt = 14 | (paramcat_meteorology_covariance << 8);
	static const uint32_t param_hydrology_basic_ffldg = 0 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_ffldro = 1 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_rssc = 2 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_esct = 3 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_swepon = 4 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_bgrun = 5 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_ssrun = 6 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_bgrun_2 = 192 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_basic_ssrun_2 = 193 | (paramcat_hydrology_basic << 8);
	static const uint32_t param_hydrology_prob_cppop = 0 | (paramcat_hydrology_prob << 8);
	static const uint32_t param_hydrology_prob_pposp = 1 | (paramcat_hydrology_prob << 8);
	static const uint32_t param_hydrology_prob_pop = 2 | (paramcat_hydrology_prob << 8);
	static const uint32_t param_hydrology_prob_cpozp = 192 | (paramcat_hydrology_prob << 8);
	static const uint32_t param_hydrology_prob_cpofp = 193 | (paramcat_hydrology_prob << 8);
	static const uint32_t param_hydrology_prob_ppffg = 194 | (paramcat_hydrology_prob << 8);
	static const uint32_t param_hydrology_prob_cwr = 195 | (paramcat_hydrology_prob << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_wdpthil = 0 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_wtmpil = 1 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_wfract = 2 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_sedtk = 3 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_sedtmp = 4 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_ictkil = 5 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_icetil = 6 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_icecil = 7 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_landil = 8 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_sfsal = 9 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_sftmp = 10 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_acwsr = 11 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_hydrology_inlandwater_sediment_saltil = 12 | (paramcat_hydrology_inlandwater_sediment << 8);
	static const uint32_t param_landsurface_vegetation_land = 0 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_sfcr = 1 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_tsoil = 2 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_soilmc = 3 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_veg = 4 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_watr = 5 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_evapt = 6 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_mterh = 7 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_landu = 8 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_soilw = 9 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_gflux = 10 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_mstav = 11 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_sfexc = 12 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_cnwat = 13 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_bmixl = 14 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_ccond = 15 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rsmin = 16 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wilt = 17 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rcs = 18 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rct = 19 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rcq = 20 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rcsol = 21 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_soilm = 22 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_cisoilw = 23 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_hflux = 24 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_vsoilm = 25 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wilt_3 = 26 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_vwiltp = 27 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_leainx = 28 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_everf = 29 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_decf = 30 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_ndvinx = 31 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rdveg = 32 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_soilw_2 = 192 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_gflux_2 = 193 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_mstav_2 = 194 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_sfexc_2 = 195 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_cnwat_2 = 196 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_bmixl_2 = 197 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_vgtyp = 198 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_ccond_2 = 199 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rsmin_2 = 200 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wilt_2 = 201 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rcs_2 = 202 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rct_2 = 203 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rcq_2 = 204 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rcsol_2 = 205 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_rdrip = 206 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_icwat = 207 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_akhs = 208 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_akms = 209 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_vegt = 210 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_sstor = 211 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_lsoil = 212 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_ewatr = 213 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_gwrec = 214 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_qrec = 215 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_sfcrh = 216 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_ndvi = 217 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_landn = 218 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_amixl = 219 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wvinc = 220 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wcinc = 221 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wvconv = 222 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wcconv = 223 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wvuflx = 224 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wvvflx = 225 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wcuflx = 226 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_wcvflx = 227 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_acond = 228 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_evcw = 229 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_vegetation_trans = 230 | (paramcat_landsurface_vegetation << 8);
	static const uint32_t param_landsurface_agriaquaculture_canl = 192 | (paramcat_landsurface_agriaquaculture << 8);
	static const uint32_t param_landsurface_soil_sotyp = 0 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_uplst = 1 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_uplsm = 2 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_lowlsm = 3 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_botlst = 4 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_soill = 5 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_rlyrs = 6 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_smref = 7 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_smdry = 8 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_poros = 9 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_liqvsm = 10 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_voltso = 11 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_transo = 12 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_voldec = 13 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_direc = 14 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_soilp = 15 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_vsosm = 16 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_satosm = 17 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_soiltmp = 18 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_soilmoi = 19 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_cisoilm = 20 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_soilice = 21 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_cisice = 22 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_soill_2 = 192 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_rlyrs_2 = 193 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_sltyp = 194 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_smref_2 = 195 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_smdry_2 = 196 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_poros_2 = 197 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_evbs = 198 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_lspa = 199 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_baret = 200 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_avsft = 201 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_radt = 202 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_soil_fldcp = 203 | (paramcat_landsurface_soil << 8);
	static const uint32_t param_landsurface_fire_fireolk = 0 | (paramcat_landsurface_fire << 8);
	static const uint32_t param_landsurface_fire_fireodt = 1 | (paramcat_landsurface_fire << 8);
	static const uint32_t param_landsurface_fire_hindex = 2 | (paramcat_landsurface_fire << 8);
	static const uint32_t param_space_image_srad = 0 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_salbedo = 1 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_sbtmp = 2 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_spwat = 3 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_slfti = 4 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_sctpres = 5 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_sstmp = 6 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_cloudm = 7 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_pixst = 8 | (paramcat_space_image << 8);
	static const uint32_t param_space_image_firedi = 9 | (paramcat_space_image << 8);
	static const uint32_t param_space_quantitative_estp = 0 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_irrate = 1 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_ctoph = 2 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_ctophqi = 3 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_estugrd = 4 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_estvgrd = 5 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_npixu = 6 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_solza = 7 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_raza = 8 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_rfl06 = 9 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_rfl08 = 10 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_rfl16 = 11 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_rfl39 = 12 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_atmdiv = 13 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_cbtmp = 14 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_csbtmp = 15 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_cldrad = 16 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_cskyrad = 17 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_winds = 19 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_aot06 = 20 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_aot08 = 21 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_aot16 = 22 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_angcoe = 23 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_usct = 192 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_quantitative_vsct = 193 | (paramcat_space_quantitative << 8);
	static const uint32_t param_space_forecast_sbt122 = 0 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbt123 = 1 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbt124 = 2 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbt126 = 3 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbc123 = 4 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbc124 = 5 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbt112 = 6 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbt113 = 7 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbt114 = 8 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_sbt115 = 9 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_amsre9 = 10 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_amsre10 = 11 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_amsre11 = 12 | (paramcat_space_forecast << 8);
	static const uint32_t param_space_forecast_amsre12 = 13 | (paramcat_space_forecast << 8);
	static const uint32_t param_spaceweather_temperature_tmpswp = 0 | (paramcat_spaceweather_temperature << 8);
	static const uint32_t param_spaceweather_temperature_electmp = 1 | (paramcat_spaceweather_temperature << 8);
	static const uint32_t param_spaceweather_temperature_prottmp = 2 | (paramcat_spaceweather_temperature << 8);
	static const uint32_t param_spaceweather_temperature_iontmp = 3 | (paramcat_spaceweather_temperature << 8);
	static const uint32_t param_spaceweather_temperature_pratmp = 4 | (paramcat_spaceweather_temperature << 8);
	static const uint32_t param_spaceweather_temperature_prptmp = 5 | (paramcat_spaceweather_temperature << 8);
	static const uint32_t param_spaceweather_momentum_speed = 0 | (paramcat_spaceweather_momentum << 8);
	static const uint32_t param_spaceweather_momentum_vel1 = 1 | (paramcat_spaceweather_momentum << 8);
	static const uint32_t param_spaceweather_momentum_vel2 = 2 | (paramcat_spaceweather_momentum << 8);
	static const uint32_t param_spaceweather_momentum_vel3 = 3 | (paramcat_spaceweather_momentum << 8);
	static const uint32_t param_spaceweather_charged_particles_plsmden = 0 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_elcden = 1 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_protden = 2 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_ionden = 3 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_vtec = 4 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_absfrq = 5 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_absrb = 6 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_sprdf = 7 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_hprimf = 8 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_crtfrq = 9 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_charged_particles_scint = 10 | (paramcat_spaceweather_charged_particles << 8);
	static const uint32_t param_spaceweather_fields_btot = 0 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_fields_bvec1 = 1 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_fields_bvec2 = 2 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_fields_bvec3 = 3 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_fields_etot = 4 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_fields_evec1 = 5 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_fields_evec2 = 6 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_fields_evec3 = 7 | (paramcat_spaceweather_fields << 8);
	static const uint32_t param_spaceweather_energetic_particles_difpflux = 0 | (paramcat_spaceweather_energetic_particles << 8);
	static const uint32_t param_spaceweather_energetic_particles_intpflux = 1 | (paramcat_spaceweather_energetic_particles << 8);
	static const uint32_t param_spaceweather_energetic_particles_difeflux = 2 | (paramcat_spaceweather_energetic_particles << 8);
	static const uint32_t param_spaceweather_energetic_particles_inteflux = 3 | (paramcat_spaceweather_energetic_particles << 8);
	static const uint32_t param_spaceweather_energetic_particles_dififlux = 4 | (paramcat_spaceweather_energetic_particles << 8);
	static const uint32_t param_spaceweather_energetic_particles_intiflux = 5 | (paramcat_spaceweather_energetic_particles << 8);
	static const uint32_t param_spaceweather_energetic_particles_ntrnflux = 6 | (paramcat_spaceweather_energetic_particles << 8);
	static const uint32_t param_spaceweather_solar_tsi = 0 | (paramcat_spaceweather_solar << 8);
	static const uint32_t param_spaceweather_solar_xlong = 1 | (paramcat_spaceweather_solar << 8);
	static const uint32_t param_spaceweather_solar_xshrt = 2 | (paramcat_spaceweather_solar << 8);
	static const uint32_t param_spaceweather_solar_euvirr = 3 | (paramcat_spaceweather_solar << 8);
	static const uint32_t param_spaceweather_solar_specirr = 4 | (paramcat_spaceweather_solar << 8);
	static const uint32_t param_spaceweather_solar_f107 = 5 | (paramcat_spaceweather_solar << 8);
	static const uint32_t param_spaceweather_solar_solrf = 6 | (paramcat_spaceweather_solar << 8);
	static const uint32_t param_spaceweather_terrestrial_lmbint = 0 | (paramcat_spaceweather_terrestrial << 8);
	static const uint32_t param_spaceweather_terrestrial_dskint = 1 | (paramcat_spaceweather_terrestrial << 8);
	static const uint32_t param_spaceweather_terrestrial_dskday = 2 | (paramcat_spaceweather_terrestrial << 8);
	static const uint32_t param_spaceweather_terrestrial_dskngt = 3 | (paramcat_spaceweather_terrestrial << 8);
	static const uint32_t param_spaceweather_imagery_xrayrad = 0 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_imagery_euvrad = 1 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_imagery_harad = 2 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_imagery_whtrad = 3 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_imagery_caiirad = 4 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_imagery_whtcor = 5 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_imagery_helcor = 6 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_imagery_mask = 7 | (paramcat_spaceweather_imagery << 8);
	static const uint32_t param_spaceweather_ionneutral_coupling_sigped = 0 | (paramcat_spaceweather_ionneutral_coupling << 8);
	static const uint32_t param_spaceweather_ionneutral_coupling_sighal = 1 | (paramcat_spaceweather_ionneutral_coupling << 8);
	static const uint32_t param_spaceweather_ionneutral_coupling_sigpar = 2 | (paramcat_spaceweather_ionneutral_coupling << 8);
	static const uint32_t param_oceanography_waves_wvsp1 = 0 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wvsp2 = 1 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wvsp3 = 2 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_htsgw = 3 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wvdir = 4 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wvhgt = 5 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wvper = 6 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_swdir = 7 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_swell = 8 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_swper = 9 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dirpw = 10 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_perpw = 11 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dirsw = 12 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_persw = 13 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wwsdir = 14 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mwsper = 15 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_cdww = 16 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_fricv = 17 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wstr = 18 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_nwstr = 19 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mssw = 20 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_ussd = 21 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_vssd = 22 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_pmaxwh = 23 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_maxwh = 24 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_imwf = 25 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_imfww = 26 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_imftsw = 27 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mzwper = 28 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mzpww = 29 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mzptsw = 30 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wdirw = 31 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dirwww = 32 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dirwts = 33 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_pwper = 34 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_pperww = 35 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_pperts = 36 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_altwh = 37 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_alcwh = 38 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_alrrc = 39 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mnwsow = 40 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mwdirw = 41 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wesp = 42 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_kssedw = 43 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_beninx = 44 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_spftr = 45 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_2dsed = 46 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_fseed = 47 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dirsed = 48 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_hsign = 50 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_pkdir = 51 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_mnstp = 52 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dmspr = 53 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wffrac = 54 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_temm1 = 55 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dir11 = 56 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dir22 = 57 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dspr11 = 58 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_dspr22 = 59 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wlen = 60 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_rdsxx = 61 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_rdsyy = 62 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_rdsxy = 63 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_waves_wstp = 192 | (paramcat_oceanography_waves << 8);
	static const uint32_t param_oceanography_currents_dirc = 0 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_currents_spc = 1 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_currents_uogrd = 2 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_currents_vogrd = 3 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_currents_omlu = 192 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_currents_omlv = 193 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_currents_ubaro = 194 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_currents_vbaro = 195 | (paramcat_oceanography_currents << 8);
	static const uint32_t param_oceanography_ice_icec = 0 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_icetk = 1 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_diced = 2 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_siced = 3 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_uice = 4 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_vice = 5 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_iceg = 6 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_iced = 7 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_icetmp = 8 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_ice_iceprs = 9 | (paramcat_oceanography_ice << 8);
	static const uint32_t param_oceanography_surface_wtmp = 0 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_dslm = 1 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_surge = 192 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_etsrg = 193 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_elev = 194 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_sshg = 195 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_p2omlt = 196 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_aohflx = 197 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_ashfl = 198 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_sstt = 199 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_ssst = 200 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_keng = 201 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_sltfl = 202 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg20 = 242 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg30 = 243 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg40 = 244 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg50 = 245 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg60 = 246 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg70 = 247 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg80 = 248 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_tcsrg90 = 249 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_surface_etcwl = 250 | (paramcat_oceanography_surface << 8);
	static const uint32_t param_oceanography_subsurface_mthd = 0 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_mtha = 1 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_tthdp = 2 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_salty = 3 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_ovhd = 4 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_ovsd = 5 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_ovmd = 6 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_bathy = 7 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_sfsalp = 11 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_sftmpp = 12 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_acwsrd = 13 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_wdepth = 14 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_wtmpss = 15 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_wtmpc = 192 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_salin = 193 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_bkeng = 194 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_dbss = 195 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_intfd = 196 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_subsurface_ohc = 197 | (paramcat_oceanography_subsurface << 8);
	static const uint32_t param_oceanography_miscellaneous_tsec = 0 | (paramcat_oceanography_miscellaneous << 8);
	static const uint32_t param_oceanography_miscellaneous_mosf = 1 | (paramcat_oceanography_miscellaneous << 8);

	class ParamCategory;
	class Parameter;

	class ParamDiscipline {
	public:
		const ParamCategory *get_category(void) const { return m_cat; }
		const char *get_str(const char *dflt = 0) const { return this && m_str ? m_str : dflt; }
		const char *get_str_nonnull(void) const { return get_str(&charstr_terminator); }
		uint8_t get_id(void) const { return this ? m_id : 0xff; }
		int compareid(const ParamDiscipline& x) const;

	public:
		const ParamCategory *m_cat;
		const char *m_str;
		uint8_t m_id;
	};

	class ParamCategory {
	public:
		const ParamDiscipline *get_discipline(void) const { return this ? m_disc : 0; }
		const Parameter *get_parameter(void) const { return m_par; }
		const char *get_str(const char *dflt = 0) const { return this && m_str ? m_str : dflt; }
		const char *get_str_nonnull(void) const { return get_str(&charstr_terminator); }
		uint8_t get_id(void) const { return this ? m_id : 0xff; }
		uint8_t get_discipline_id(void) const { return this ? m_discid : 0xff; }
		uint16_t get_fullid(void) const { return this ? m_id | (m_discid << 8) : 0xffff; }
		int compareid(const ParamCategory& x) const;

	public:
		const ParamDiscipline *m_disc;
		const Parameter *m_par;
		const char *m_str;
		uint8_t m_id;
		uint8_t m_discid;
	};

	class Parameter {
	public:
		const ParamCategory *get_category(void) const { return this ? m_cat : 0; }
		const char *get_str(const char *dflt = 0) const { return this && m_str ? m_str : dflt; }
		const char *get_str_nonnull(void) const { return get_str(&charstr_terminator); }
		const char *get_unit(const char *dflt = 0) const { return this && m_unit ? m_unit : dflt; }
		const char *get_unit_nonnull(void) const { return get_unit(&charstr_terminator); }
		const char *get_abbrev(const char *dflt = 0) const { return this ? m_abbrev : dflt; }
		const char *get_abbrev_nonnull(void) const { return get_abbrev(&charstr_terminator); }
		uint8_t get_id(void) const { return this ? m_id : 0xff; }
		uint8_t get_category_id(void) const { return this ? m_catid : 0xff; }
		uint8_t get_discipline_id(void) const { return this ? m_discid : 0xff; }
		uint32_t get_fullid(void) const { return this ? m_id | (m_catid << 8) | (m_discid << 16) : 0xffffff; }
		int compareid(const Parameter& x) const;

	public:
		const ParamCategory *m_cat;
		const char *m_str;
		const char *m_unit;
		char m_abbrev[13];
		uint8_t m_id;
		uint8_t m_catid;
		uint8_t m_discid;
	};

	static const ParamDiscipline *find_discipline(uint8_t id);
	static const ParamCategory *find_paramcategory(const ParamDiscipline *disc, uint8_t id);
	static const ParamCategory *find_paramcategory(uint8_t discid, uint8_t id) { return find_paramcategory(find_discipline(discid), id); }
	static const ParamCategory *find_paramcategory(uint16_t fullid) { return find_paramcategory(fullid >> 8, fullid); }
	static const Parameter *find_parameter(const ParamCategory *cat, uint8_t id);
	static const Parameter *find_parameter(const ParamDiscipline *disc, uint8_t catid, uint8_t id) { return find_parameter(find_paramcategory(disc, catid), id); }
	static const Parameter *find_parameter(uint8_t discid, uint8_t catid, uint8_t id) { return find_parameter(find_paramcategory(find_discipline(discid), catid), id); }
	static const Parameter *find_parameter(uint32_t fullid) { return find_parameter(fullid >> 16, fullid >> 8, fullid); }
	static const ParamDiscipline * const *find_discipline(const char *str);
	static const ParamDiscipline * const *find_discipline(const std::string& str) { return find_discipline(str.c_str()); }
	static const ParamCategory * const *find_paramcategory(const char *str);
	static const ParamCategory * const *find_paramcategory(const std::string& str) { return find_paramcategory(str.c_str()); }
	static const Parameter * const *find_parameter(const char *str);
	static const Parameter * const *find_parameter(const std::string& str) { return find_parameter(str.c_str()); }
	static const Parameter * const *find_parameter_by_abbrev(const char *str);
	static const Parameter * const *find_parameter_by_abbrev(const std::string& str) { return find_parameter_by_abbrev(str.c_str()); }

	static const char *find_centerid_str(uint16_t cid, const char *dflt = 0);
	static const char *find_subcenterid_str(uint16_t cid, uint16_t sid, const char *dflt = 0);
	static const char *find_genprocesstype_str(uint16_t cid, uint8_t genproct, const char *dflt = 0);
	static const char *find_productionstatus_str(uint8_t pstat, const char *dflt = 0);
	static const char *find_datatype_str(uint8_t dtype, const char *dflt = 0);
	static const char *find_genprocess_str(uint8_t genproc, const char *dflt = 0);
	static const char *find_surfacetype_str(uint8_t sfctype, const char *dflt = 0);
	static const char *find_surfaceunit_str(uint8_t sfctype, const char *dflt = 0);

	class Grid {
	public:
		Grid(void);
		virtual ~Grid();
                void reference(void) const;
                void unreference(void) const;
		virtual unsigned int get_usize(void) const = 0;
		virtual unsigned int get_vsize(void) const = 0;
		virtual unsigned int operator()(int u, int v) const = 0;
		virtual Point get_center(int u, int v) const = 0;
		virtual Point get_pointsize(void) const = 0;
		virtual bool operator==(const Grid& x) const { return false; }
		virtual bool operator!=(const Grid& x) const { return !operator==(x); }
		virtual std::pair<float,float> transform_axes(float u, float v) const = 0;

	protected:
                mutable gint m_refcount;
	};

	class GridLatLon : public Grid {
	public:
		GridLatLon(const Point& origin, const Point& ptsz, unsigned int usz, unsigned int vsz, int scu, int scv, int offs);
		virtual unsigned int get_usize(void) const { return m_usize; }
		virtual unsigned int get_vsize(void) const { return m_vsize; }
		virtual unsigned int operator()(int u, int v) const;
		virtual Point get_center(int u, int v) const;
		virtual Point get_pointsize(void) const { return m_pointsize; }
		virtual bool operator==(const Grid& x) const;
		virtual std::pair<float,float> transform_axes(float u, float v) const;

	protected:
		Point m_origin;
		Point m_pointsize;
		unsigned int m_usize;
		unsigned int m_vsize;
		int m_scaleu;
		int m_scalev;
		int m_offset;
		bool m_fulllon;
	};

	class LayerResult;

	class Layer {
	public:
		Layer(const Parameter *param = 0, const Glib::RefPtr<Grid const>& grid = Glib::RefPtr<Grid>(),
		      gint64 reftime = 0, gint64 efftime = 0, uint16_t centerid = 0xffff, uint16_t subcenterid = 0xffff,
		      uint8_t productionstatus = 0xff, uint8_t datatype = 0xff,
		      uint8_t genprocess = 0xff, uint8_t genprocesstype = 0xff,
		      uint8_t surface1type = 0xff, double surface1value = std::numeric_limits<double>::quiet_NaN(),
		      uint8_t surface2type = 0xff, double surface2value = std::numeric_limits<double>::quiet_NaN());
		virtual ~Layer();
                void reference(void) const;
                void unreference(void) const;
		gint64 get_reftime(void) const { return m_reftime; }
		gint64 get_efftime(void) const { return m_efftime; }
		const Parameter *get_parameter(void) const { return m_parameter; }
		uint8_t get_surface1type(void) const { return m_surface1type; }
		double get_surface1value(void) const { return m_surface1value; }
		uint8_t get_surface2type(void) const { return m_surface2type; }
		double get_surface2value(void) const { return m_surface2value; }
		uint16_t get_centerid(void) const { return m_centerid; }
		uint16_t get_subcenterid(void) const { return m_subcenterid; }
		uint8_t get_productionstatus(void) const { return m_productionstatus; }
		uint8_t get_datatype(void) const { return m_datatype; }
		uint8_t get_genprocess(void) const { return m_genprocess; }
		uint8_t get_genprocesstype(void) const { return m_genprocesstype; }
		const Glib::RefPtr<Grid const>& get_grid(void) const { return m_grid; }
		void expire_time(long curtime);
		void expire_now(void);
		Glib::RefPtr<LayerResult> get_results(const Rect& bbox);
		void readonly(void) const;
		void readwrite(void) const;
		virtual bool check_load(void) = 0;
		std::ostream& print_param(std::ostream& os);

		static unsigned int extract(const std::vector<uint8_t>& filedata, unsigned int offs, unsigned int width);

	protected:
                mutable gint m_refcount;
		Glib::Mutex m_mutex;
		sigc::connection m_expire;
		typedef std::vector<float> data_t;
		data_t m_data;
		Glib::RefPtr<Grid const> m_grid;
		gint64 m_reftime;
		gint64 m_efftime;
		gint64 m_cachetime;
		const Parameter *m_parameter;
		double m_surface1value;
		double m_surface2value;
		uint16_t m_centerid;
		uint16_t m_subcenterid;
		uint8_t m_productionstatus;
		uint8_t m_datatype;
		uint8_t m_genprocess;
		uint8_t m_genprocesstype;
		uint8_t m_surface1type;
		uint8_t m_surface2type;

		virtual void load(void) = 0;
	};

	class LayerJ2KParam {
	public:
		LayerJ2KParam(void);
		double get_datascale(void) const { return m_datascale; }
		void set_datascale(double d) { m_datascale = d; }
		double get_dataoffset(void) const { return m_dataoffset; }
		void set_dataoffset(double d) { m_dataoffset = d; }
		double scale(int v) const { return m_dataoffset + m_datascale * v; }

	protected:
		double m_datascale;
		double m_dataoffset;
	};

	class LayerSimplePackingParam : public LayerJ2KParam {
	public:
		LayerSimplePackingParam(void);

		unsigned int get_nbitsgroupref(void) const { return m_nbitsgroupref; }
		void set_nbitsgroupref(unsigned int nbgr) { m_nbitsgroupref = nbgr; }
		uint8_t get_fieldvaluetype(void) const { return m_fieldvaluetype; }
		void set_fieldvaluetype(uint8_t vt) { m_fieldvaluetype = vt; }
		bool is_fieldvalue_float(void) const { return !get_fieldvaluetype(); }

	protected:
		unsigned int m_nbitsgroupref;
		uint8_t m_fieldvaluetype;
	};

	class LayerComplexPackingParam : public LayerSimplePackingParam {
	public:
		LayerComplexPackingParam(void);

		unsigned int get_ngroups(void) const { return m_ngroups; }
		void set_ngroups(unsigned int ng) { m_ngroups = ng; }
		unsigned int get_refgroupwidth(void) const { return m_refgroupwidth; }
		void set_refgroupwidth(unsigned int rgw) { m_refgroupwidth = rgw; }
		unsigned int get_nbitsgroupwidth(void) const { return m_nbitsgroupwidth; }
		void set_nbitsgroupwidth(unsigned int nbgw) { m_nbitsgroupwidth = nbgw; }
		unsigned int get_refgrouplength(void) const { return m_refgrouplength; }
		void set_refgrouplength(unsigned int rgl) { m_refgrouplength = rgl; }
		unsigned int get_incrgrouplength(void) const { return m_incrgrouplength; }
		void set_incrgrouplength(unsigned int igl) { m_incrgrouplength = igl; }
		unsigned int get_lastgrouplength(void) const { return m_lastgrouplength; }
		void set_lastgrouplength(unsigned int lgl) { m_lastgrouplength = lgl; }
		unsigned int get_nbitsgrouplength(void) const { return m_nbitsgrouplength; }
		void set_nbitsgrouplength(unsigned int nbgl) { m_nbitsgrouplength = nbgl; }

		uint8_t get_groupsplitmethod(void) const { return m_groupsplitmethod; }
		void set_groupsplitmethod(uint8_t gsm) { m_groupsplitmethod = gsm; }
		bool is_gengroupsplit(void) const { return get_groupsplitmethod() == 1; }

		uint8_t get_missingvaluemgmt(void) const { return m_missingvaluemgmt; }
		void set_missingvaluemgmt(uint8_t mvm) { m_missingvaluemgmt = mvm; }
		bool is_primarymissingvalue(void) const;
		bool is_secondarymissingvalue(void) const;
		int32_t get_primarymissingvalue(void) const { return m_primarymissingvalue; }
		void set_primarymissingvalue(int32_t v) { m_primarymissingvalue = v; }
		int32_t get_secondarymissingvalue(void) const { return m_secondarymissingvalue; }
		void set_secondarymissingvalue(int32_t v) { m_secondarymissingvalue = v; }
		unsigned int get_primarymissingvalue_raw(void) const;
		unsigned int get_secondarymissingvalue_raw(void) const;
		double get_primarymissingvalue_float(void) const;
		double get_secondarymissingvalue_float(void) const;

	protected:
		unsigned int m_ngroups;
		unsigned int m_refgroupwidth;
		unsigned int m_nbitsgroupwidth;
		unsigned int m_refgrouplength;
		unsigned int m_incrgrouplength;
		unsigned int m_lastgrouplength;
		unsigned int m_nbitsgrouplength;
		int32_t m_primarymissingvalue;
		int32_t m_secondarymissingvalue;
		uint8_t m_groupsplitmethod;
		uint8_t m_missingvaluemgmt;
	};

	class LayerComplexPackingSpatialDiffParam : public LayerComplexPackingParam {
	public:
		LayerComplexPackingSpatialDiffParam(void);

		unsigned int get_spatialdifforder(void) const { return m_spatialdifforder; }
		void set_spatialdifforder(unsigned int o) { m_spatialdifforder = o; }
		unsigned int get_extradescroctets(void) const { return m_extradescroctets; }
		void set_extradescroctets(unsigned int e) { m_extradescroctets = e; }

	protected:
		unsigned int m_spatialdifforder;
		unsigned int m_extradescroctets;
	};

	class LayerJ2K : public Layer {
	public:
		LayerJ2K(const Parameter *param = 0, const Glib::RefPtr<Grid const>& grid = Glib::RefPtr<Grid>(),
			 gint64 reftime = 0, gint64 efftime = 0, uint16_t centerid = 0xffff, uint16_t subcenterid = 0xffff,
			 uint8_t productionstatus = 0xff, uint8_t datatype = 0xff,
			 uint8_t genprocess = 0xff, uint8_t genprocesstype = 0xff,
			 uint8_t surface1type = 0xff, double surface1value = std::numeric_limits<double>::quiet_NaN(),
			 uint8_t surface2type = 0xff, double surface2value = std::numeric_limits<double>::quiet_NaN(),
			 const LayerJ2KParam& layerparam = LayerJ2KParam(),
			 goffset bitmapoffs = 0, bool bitmap = false,
			 goffset fileoffset = 0, gsize filesize = 0, const std::string& filename = "",
			 const std::string& cachedir = "");
		virtual bool check_load(void);

	protected:
		std::string m_filename;
		std::string m_cachedir;
		LayerJ2KParam m_param;
		goffset m_bitmapoffset;
		goffset m_fileoffset;
		gsize m_filesize;
		bool m_bitmap;

		virtual void load(void);
	};

	class LayerSimplePacking : public Layer {
	public:
		LayerSimplePacking(const Parameter *param = 0, const Glib::RefPtr<Grid const>& grid = Glib::RefPtr<Grid>(),
				   gint64 reftime = 0, gint64 efftime = 0, uint16_t centerid = 0xffff, uint16_t subcenterid = 0xffff,
				   uint8_t productionstatus = 0xff, uint8_t datatype = 0xff,
				   uint8_t genprocess = 0xff, uint8_t genprocesstype = 0xff,
				   uint8_t surface1type = 0xff, double surface1value = std::numeric_limits<double>::quiet_NaN(),
				   uint8_t surface2type = 0xff, double surface2value = std::numeric_limits<double>::quiet_NaN(),
				   const LayerSimplePackingParam& layerparam = LayerSimplePackingParam(),
				   goffset bitmapoffs = 0, bool bitmap = false,
				   goffset fileoffset = 0, gsize filesize = 0, const std::string& filename = "");
		virtual bool check_load(void);

	protected:
		std::string m_filename;
	        LayerSimplePackingParam m_param;
		goffset m_bitmapoffset;
		goffset m_fileoffset;
		gsize m_filesize;
		bool m_bitmap;

		virtual void load(void);
	};

	class LayerComplexPacking : public Layer {
	public:
		LayerComplexPacking(const Parameter *param = 0, const Glib::RefPtr<Grid const>& grid = Glib::RefPtr<Grid>(),
				    gint64 reftime = 0, gint64 efftime = 0, uint16_t centerid = 0xffff, uint16_t subcenterid = 0xffff,
				    uint8_t productionstatus = 0xff, uint8_t datatype = 0xff,
				    uint8_t genprocess = 0xff, uint8_t genprocesstype = 0xff,
				    uint8_t surface1type = 0xff, double surface1value = std::numeric_limits<double>::quiet_NaN(),
				    uint8_t surface2type = 0xff, double surface2value = std::numeric_limits<double>::quiet_NaN(),
				    const LayerComplexPackingParam& layerparam = LayerComplexPackingParam(),
				    goffset bitmapoffs = 0, bool bitmap = false,
				    goffset fileoffset = 0, gsize filesize = 0, const std::string& filename = "");
		virtual bool check_load(void);

	protected:
		std::string m_filename;
		LayerComplexPackingParam m_param;
		goffset m_bitmapoffset;
		goffset m_fileoffset;
		gsize m_filesize;
		bool m_bitmap;

		virtual void load(void);
	};

	class LayerComplexPackingSpatialDiff : public Layer {
	public:
		LayerComplexPackingSpatialDiff(const Parameter *param = 0, const Glib::RefPtr<Grid const>& grid = Glib::RefPtr<Grid>(),
					       gint64 reftime = 0, gint64 efftime = 0, uint16_t centerid = 0xffff, uint16_t subcenterid = 0xffff,
					       uint8_t productionstatus = 0xff, uint8_t datatype = 0xff,
					       uint8_t genprocess = 0xff, uint8_t genprocesstype = 0xff,
					       uint8_t surface1type = 0xff, double surface1value = std::numeric_limits<double>::quiet_NaN(),
					       uint8_t surface2type = 0xff, double surface2value = std::numeric_limits<double>::quiet_NaN(),
					       const LayerComplexPackingSpatialDiffParam& layerparam = LayerComplexPackingSpatialDiffParam(),
					       goffset bitmapoffs = 0, bool bitmap = false,
					       goffset fileoffset = 0, gsize filesize = 0, const std::string& filename = "");
		virtual bool check_load(void);

	protected:
		std::string m_filename;
		LayerComplexPackingSpatialDiffParam m_param;
		goffset m_bitmapoffset;
		goffset m_fileoffset;
		gsize m_filesize;
		bool m_bitmap;

		virtual void load(void);
	};

	class LayerResult {
	public:
		LayerResult(const Glib::RefPtr<Layer const>& layer = Glib::RefPtr<Layer const>(), const Rect& bbox = Rect(), unsigned int w = 0, unsigned int h = 0,
			    gint64 efftime = 0, gint64 minreftime = 0, gint64 maxreftime = 0, double sfc1value = std::numeric_limits<double>::quiet_NaN());
		~LayerResult();
                void reference(void) const;
                void unreference(void) const;
		const Glib::RefPtr<Layer const>& get_layer(void) const { return m_layer; }
		const Rect& get_bbox(void) const { return m_bbox; }
		unsigned int get_width(void) const { return m_width; }
		unsigned int get_height(void) const { return m_height; }
		unsigned int get_size(void) const { return m_data.size(); }
		const float& operator()(unsigned int x, unsigned int y) const;
		float& operator()(unsigned int x, unsigned int y);
		const float& operator[](unsigned int x) const;
		float& operator[](unsigned int x);
		float operator()(const Point& pt) const;
		Point get_center(unsigned int x, unsigned int y) const;
		Point get_pixelsize(void) const;
		gint64 get_efftime(void) const { return m_efftime; }
		gint64 get_minreftime(void) const { return m_minreftime; }
		gint64 get_maxreftime(void) const { return m_maxreftime; }
		double get_surface1value(void) const { return m_surface1value; }
		void readonly(void) const;
		void readwrite(void) const;

	protected:
		static const float nan;
                mutable gint m_refcount;
		Glib::RefPtr<Layer const> m_layer;
		Rect m_bbox;
		unsigned int m_width;
		unsigned int m_height;
		gint64 m_efftime;
		gint64 m_minreftime;
		gint64 m_maxreftime;
		double m_surface1value;
		typedef std::vector<float> data_t;
		data_t m_data;
	};

	class LayerInterpolateResult {
	public:
		class InterpIndex {
		public:
			InterpIndex(float idxtime, float idxsfc1value) : m_idxtime(idxtime), m_idxsfc1value(idxsfc1value) {}
			float get_idxtime(void) const { return m_idxtime; }
			float get_idxsfc1value(void) const { return m_idxsfc1value; }

		protected:
			float m_idxtime;
			float m_idxsfc1value;
		};

		class LinInterp {
		public:
			LinInterp(float p0 = 0, float p1 = 0, float p2 = 0, float p3 = 0);
			float operator()(const InterpIndex& idx) const;
			float operator[](unsigned int idx) const;
			LinInterp& operator+=(const LinInterp& x);
			LinInterp& operator*=(float m);
			LinInterp operator+(const LinInterp& x) const;
			LinInterp operator*(float m) const;
			bool is_nan(void) const;

		protected:
			float m_p[4];
		};

		LayerInterpolateResult(const Glib::RefPtr<Layer const>& layer = Glib::RefPtr<Layer const>(), const Rect& bbox = Rect(), unsigned int w = 0, unsigned int h = 0,
				       gint64 minefftime = 0, gint64 maxefftime = 0, gint64 minreftime = 0, gint64 maxreftime = 0,
				       double minsfc1value = std::numeric_limits<double>::quiet_NaN(), double maxsfc1value = std::numeric_limits<double>::quiet_NaN());
		~LayerInterpolateResult();
                void reference(void) const;
                void unreference(void) const;
		const Glib::RefPtr<Layer const>& get_layer(void) const { return m_layer; }
		const Rect& get_bbox(void) const { return m_bbox; }
		unsigned int get_width(void) const { return m_width; }
		unsigned int get_height(void) const { return m_height; }
		unsigned int get_size(void) const { return m_data.size(); }
		float operator()(unsigned int x, unsigned int y, const InterpIndex& idx) const;
		float operator()(unsigned int x, unsigned int y, gint64 efftime, double sfc1value) const;
		const LinInterp& operator()(unsigned int x, unsigned int y) const;
		LinInterp& operator()(unsigned int x, unsigned int y);
		const LinInterp& operator[](unsigned int x) const;
		LinInterp& operator[](unsigned int x);
		LinInterp operator()(const Point& pt) const;
		float operator()(const Point& pt, const InterpIndex& idx) const;
		float operator()(const Point& pt, gint64 efftime, double sfc1value) const;
		Point get_center(unsigned int x, unsigned int y) const;
		Point get_pixelsize(void) const;
		gint64 get_minefftime(void) const { return m_minefftime; }
		gint64 get_maxefftime(void) const { return m_maxefftime; }
		gint64 get_minreftime(void) const { return m_minreftime; }
		gint64 get_maxreftime(void) const { return m_maxreftime; }
		double get_minsurface1value(void) const { return m_minsurface1value; }
		double get_maxsurface1value(void) const { return m_maxsurface1value; }
		Glib::RefPtr<LayerResult> get_results(gint64 efftime, double sfc1value = 0);
		InterpIndex get_index(gint64 efftime, double sfc1value) const;
		InterpIndex get_index_efftime(gint64 efftime) const;
		InterpIndex get_index_surface1value(double sfc1value) const;
		void readonly(void) const;
		void readwrite(void) const;

	protected:
		static const LinInterp nan;
		mutable gint m_refcount;
		Glib::RefPtr<Layer const> m_layer;
		Rect m_bbox;
		unsigned int m_width;
		unsigned int m_height;
		gint64 m_minefftime;
		gint64 m_maxefftime;
		gint64 m_minreftime;
		gint64 m_maxreftime;
		double m_efftimemul;
		double m_minsurface1value;
		double m_maxsurface1value;
		double m_surface1valuemul;
		typedef std::vector<LinInterp> data_t;
		data_t m_data;
	};

	class Parser {
	public:
		Parser(GRIB2& gr2);
		int parse_file(const std::string& filename);
		int parse_directory(const std::string& dir);
		int parse(const std::string& p);

	protected:
		GRIB2& m_grib2;
		Glib::RefPtr<Grid> m_lastgrid;
		Glib::RefPtr<Grid> m_grid;
		gint64 m_reftime;
		gint64 m_efftime;
		double m_surface1value;
		double m_surface2value;
		LayerComplexPackingSpatialDiffParam m_layerparam;
		goffset m_bitmapoffs;
		uint32_t m_bitmapsize;
		uint32_t m_nrdatapoints;
		uint16_t m_centerid;
		uint16_t m_subcenterid;
		uint16_t m_datarepresentation;
		uint8_t m_productionstatus;
		uint8_t m_datatype;
		uint8_t m_productdiscipline;
		uint8_t m_paramcategory;
		uint8_t m_paramnumber;
		uint8_t m_genprocess;
		uint8_t m_genprocesstype;
		uint8_t m_surface1type;
		uint8_t m_surface2type;
		bool m_bitmap;

		int section1(const uint8_t *buf, uint32_t len);
		int section3(const uint8_t *buf, uint32_t len);
		int section4(const uint8_t *buf, uint32_t len);
		int section5(const uint8_t *buf, uint32_t len);
		int section6(const uint8_t *buf, uint32_t len, goffset offs);
		int section7(const uint8_t *buf, uint32_t len, goffset offs, const std::string& filename);
	};

	class WeatherProfilePoint {
	public:
		static const int32_t invalidalt;
		static const float invalidcover;
		static const int16_t isobaric_levels[27];
		static const int32_t altitudes[sizeof(isobaric_levels)/sizeof(isobaric_levels[0])];
		static const uint16_t flags_daymask      = 0x03;
		static const uint16_t flags_day          = 0x00;
		static const uint16_t flags_dusk         = 0x01;
		static const uint16_t flags_night        = 0x02;
		static const uint16_t flags_dawn         = 0x03;
		static const uint16_t flags_rain         = 0x04;
		static const uint16_t flags_freezingrain = 0x08;
		static const uint16_t flags_icepellets   = 0x10;
		static const uint16_t flags_snow         = 0x20;

		class SoundingSurfaceTemp {
		public:
			SoundingSurfaceTemp(float press = std::numeric_limits<float>::quiet_NaN(),    /* hpa */
					    float temp = std::numeric_limits<float>::quiet_NaN(),     /* kelvin */
					    float dewpt = std::numeric_limits<float>::quiet_NaN());   /* kelvin */

			float get_press(void) const { return m_press; }
			void set_press(float x) { m_press = x; }
			float get_temp(void) const { return m_temp; }
			void set_temp(float x) { m_temp = x; }
			bool is_temp_valid(void) const { return !std::isnan(get_temp()); }
			float get_dewpt(void) const { return m_dewpt; }
			void set_dewpt(float x) { m_dewpt = x; }
			bool is_dewpt_valid(void) const { return !std::isnan(get_dewpt()); }

			int compare(const SoundingSurfaceTemp& x) const;
			bool operator==(const SoundingSurfaceTemp& x) const { return compare(x) == 0; }
			bool operator!=(const SoundingSurfaceTemp& x) const { return compare(x) != 0; }
			bool operator<(const SoundingSurfaceTemp& x) const { return compare(x) < 0; }
			bool operator<=(const SoundingSurfaceTemp& x) const { return compare(x) <= 0; }
			bool operator>(const SoundingSurfaceTemp& x) const { return compare(x) > 0; }
			bool operator>=(const SoundingSurfaceTemp& x) const { return compare(x) >= 0; }

		protected:
			float m_press;
			float m_temp;
			float m_dewpt;
		};

		class SoundingSurface : public SoundingSurfaceTemp {
		public:
			SoundingSurface(float press = std::numeric_limits<float>::quiet_NaN(),    /* hpa */
					float temp = std::numeric_limits<float>::quiet_NaN(),     /* kelvin */
					float dewpt = std::numeric_limits<float>::quiet_NaN(),    /* kelvin */
					float winddir = std::numeric_limits<float>::quiet_NaN(),  /* deg */
					float windspd = std::numeric_limits<float>::quiet_NaN()); /* kts */

			float get_winddir(void) const { return m_winddir; }
			void set_winddir(float x) { m_winddir = x; }
			float get_windspeed(void) const { return m_windspeed; }
			void set_windspeed(float x) { m_windspeed = x; }
			bool is_wind_valid(void) const {
				return !std::isnan(get_winddir()) && !std::isnan(get_windspeed());
			}

		protected:
			float m_winddir;
			float m_windspeed;
		};

		typedef std::set<SoundingSurface> soundingsurfaces_t;

		class Surface {
		public:
			Surface(float uwind = 0, float vwind = 0, float temp = 0, float rh = 0, float hwsh = 0, float vwsh = 0);

			float get_uwind(void) const { return m_uwind; }
			float get_vwind(void) const { return m_vwind; }
			float get_wind(void) const { return sqrtf(get_uwind() * get_uwind() + get_vwind() * get_vwind()); }
			float get_temp(void) const { return m_temp; }
			float get_rh(void) const { return m_rh; }
			float get_hwsh(void) const { return m_hwsh; }
			float get_vwsh(void) const { return m_vwsh; }
			float get_turbulenceindex(void) const;
			float get_dewpoint(void) const;

			void set_uwind(float u) { m_uwind = u; }
			void set_vwind(float v) { m_vwind = v; }
			void set_wind(float u, float v) { m_uwind = u; m_vwind = v; }
			void set_temp(float x) { m_temp = x; }
			void set_rh(float x) { m_rh = x; }
			void set_hwsh(float x) { m_hwsh = x; }
			void set_vwsh(float x) { m_vwsh = x; }

			operator SoundingSurface(void) const;

		protected:
			//static constexpr float magnus_b = 17.62;
			//static constexpr float magnus_c = 243.12;
			// WMO
			//static constexpr float magnus_b = 17.502;
			//static constexpr float magnus_c = 240.97;
			// Bolton
			static constexpr float magnus_b = 17.67;
			static constexpr float magnus_c = 243.5;
			float m_uwind;
			float m_vwind;
			float m_temp;
			float m_rh;
			float m_hwsh;
			float m_vwsh;

			static float magnus_gamma(float temp, float rh);
			float magnus_gamma(void) const { return magnus_gamma(get_temp(), get_rh()); }
		};

		class Stability {
		public:
			Stability(const soundingsurfaces_t& surf = soundingsurfaces_t());
			float get_lcl_press(void) const { return m_lcl_press; }
			float get_lcl_temp(void) const { return m_lcl_temp; }
			float get_lfc_press(void) const { return m_lfc_press; }
			float get_lfc_temp(void) const { return m_lfc_temp; }
			float get_el_press(void) const { return m_el_press; }
			float get_el_temp(void) const { return m_el_temp; }
			bool is_lcl(void) const { return !std::isnan(get_lcl_press()) && !std::isnan(get_lcl_temp()); }
			bool is_lfc(void) const { return !std::isnan(get_lfc_press()) && !std::isnan(get_lfc_temp()); }
			bool is_el(void) const { return !std::isnan(get_el_press()) && !std::isnan(get_el_temp()); }
			float get_liftedindex(void) const { return m_liftedindex; }
			float get_cape(void) const { return m_cape; }
			float get_cin(void) const { return m_cin; }
			typedef std::set<SoundingSurfaceTemp> tempcurve_t;
			const tempcurve_t& get_tempcurve(void) const { return m_tempcurve; }

		protected:
			tempcurve_t m_tempcurve;
			float m_lcl_press;
			float m_lcl_temp;
			float m_lfc_press;
			float m_lfc_temp;
			float m_el_press;
			float m_el_temp;
			float m_liftedindex;
			float m_cape;
			float m_cin;
		};

		WeatherProfilePoint(double dist = 0, double routedist = 0, unsigned int routeindex = 0,
				    const Point& pt = Point::invalid, gint64 efftime = 0, int32_t alt = std::numeric_limits<int32_t>::min(),
				    int32_t zerodegisotherm = invalidalt, int32_t tropopause = invalidalt,
				    float cldbdrycover = invalidcover, int32_t bdrylayerheight = invalidalt,
				    float cldlowcover = invalidcover, int32_t cldlowbase = invalidalt, int32_t cldlowtop = invalidalt,
				    float cldmidcover = invalidcover, int32_t cldmidbase = invalidalt, int32_t cldmidtop = invalidalt,
				    float cldhighcover = invalidcover, int32_t cldhighbase = invalidalt, int32_t cldhightop = invalidalt,
				    float cldconvcover = invalidcover, int32_t cldconvbase = invalidalt, int32_t cldconvtop = invalidalt,
				    float precip = 0, float preciprate = 0, float convprecip = 0, float convpreciprate = 0,
				    float liftedindex = 0, float cape = 0, float cin = 0, uint16_t flags = 0);

		const Point& get_pt(void) const { return m_pt; }
		gint64 get_efftime(void) const { return m_efftime; }
		int32_t get_alt(void) const { return m_alt; }
		double get_dist(void) const { return m_dist; }
		double get_routedist(void) const { return m_routedist; }
		unsigned int get_routeindex(void) const { return m_routeindex; }
		int32_t get_zerodegisotherm(void) const { return m_zerodegisotherm; }
		int32_t get_tropopause(void) const { return m_tropopause; }
		float get_cldbdrycover(void) const { return m_cldbdrycover; }
		int32_t get_bdrylayerbase(void) const;
		int32_t get_bdrylayerheight(void) const { return m_bdrylayerheight; }
		float get_cldlowcover(void) const { return m_cldlowcover; }
		int32_t get_cldlowbase(void) const { return m_cldlowbase; }
		int32_t get_cldlowtop(void) const { return m_cldlowtop; }
		float get_cldmidcover(void) const { return m_cldmidcover; }
		int32_t get_cldmidbase(void) const { return m_cldmidbase; }
		int32_t get_cldmidtop(void) const { return m_cldmidtop; }
		float get_cldhighcover(void) const { return m_cldhighcover; }
		int32_t get_cldhighbase(void) const { return m_cldhighbase; }
		int32_t get_cldhightop(void) const { return m_cldhightop; }
		float get_cldconvcover(void) const { return m_cldconvcover; }
		int32_t get_cldconvbase(void) const { return m_cldconvbase; }
		int32_t get_cldconvtop(void) const { return m_cldconvtop; }
		float get_precip(void) const { return m_precip; }
		float get_preciprate(void) const { return m_preciprate; }
		float get_convprecip(void) const { return m_convprecip; }
		float get_convpreciprate(void) const { return m_convpreciprate; }
		float get_liftedindex(void) const { return m_liftedindex; }
		float get_cape(void) const { return m_cape; }
		float get_cin(void) const { return m_cin; }
		uint16_t get_flags(void) const { return m_flags; }
		Surface& operator[](unsigned int i);
		const Surface& operator[](unsigned int i) const;
		operator soundingsurfaces_t(void) const;
		static bool is_pressure_valid(float press_pa);

	protected:
		static const Surface invalid_surface;
		Surface m_surfaces[sizeof(isobaric_levels)/sizeof(isobaric_levels[0])];
		Point m_pt;
		gint64 m_efftime;
		double m_dist;
		double m_routedist;
		uint32_t m_routeindex;
		int32_t m_alt;
		int32_t m_zerodegisotherm;
		int32_t m_tropopause;
		float m_cldbdrycover;
		int32_t m_bdrylayerheight;
		float m_cldlowcover;
		int32_t m_cldlowbase;
		int32_t m_cldlowtop;
		float m_cldmidcover;
		int32_t m_cldmidbase;
		int32_t m_cldmidtop;
		float m_cldhighcover;
		int32_t m_cldhighbase;
		int32_t m_cldhightop;
		float m_cldconvcover;
		int32_t m_cldconvbase;
		int32_t m_cldconvtop;
		float m_precip;
		float m_preciprate;
		float m_convprecip;
		float m_convpreciprate;
		float m_liftedindex;
		float m_cape;
		float m_cin;
		uint16_t m_flags;
	};

	class WeatherProfile : public std::vector<WeatherProfilePoint> {
	protected:
		typedef std::vector<WeatherProfilePoint> base_t;

	public:
		WeatherProfile(void);
		double get_dist(void) const;
		gint64 get_minefftime(void) const { return m_minefftime; }
		gint64 get_maxefftime(void) const { return m_maxefftime; }
		gint64 get_minreftime(void) const { return m_minreftime; }
		gint64 get_maxreftime(void) const { return m_maxreftime; }
		void add_reftime(gint64 t);
		void add_efftime(gint64 t);

	protected:
		gint64 m_minefftime;
		gint64 m_maxefftime;
		gint64 m_minreftime;
		gint64 m_maxreftime;
	};

	GRIB2(void);
	GRIB2(const std::string& cachedir);
	~GRIB2(void);
	const std::string& get_cachedir(void) const { return m_cachedir; }
	void set_cachedir(const std::string& cachedir = "") { m_cachedir = cachedir; }
	void set_default_cachedir(void);
	void add_layer(const Glib::RefPtr<Layer>& layer);
	unsigned int remove_missing_layers(void);
	unsigned int remove_obsolete_layers(void);
	unsigned int expire_cache(unsigned int maxdays = 14, off_t maxbytes = 1024*1024*1024);

	typedef std::vector<Glib::RefPtr<Layer> > layerlist_t;
	layerlist_t find_layers(void);
	layerlist_t find_layers(const Parameter *param, gint64 efftime);
	layerlist_t find_layers(const Parameter *param, gint64 efftime, uint8_t sfc1type, double sfc1value);
	static Glib::RefPtr<LayerInterpolateResult> interpolate_results(const Rect& bbox, const layerlist_t& layers, gint64 efftime);
	static Glib::RefPtr<LayerInterpolateResult> interpolate_results(const Rect& bbox, const layerlist_t& layers, gint64 efftime, double sfc1value);

	WeatherProfile get_profile(const FPlanRoute& fpl);

	class LayerAllocator;
	static const LayerAllocator LayerAlloc;

protected:
	class LayerPtr {
	public:
		LayerPtr(const Glib::RefPtr<Layer>& layer) : m_layer(layer) {}
		const Glib::RefPtr<Layer>& get_layer(void) const { return m_layer; }
		int compare(const LayerPtr& x) const;
		bool operator<(const LayerPtr& x) const { return compare(x) < 0; }

	protected:
		Glib::RefPtr<Layer> m_layer;
	};

	class LayerPtrSurface {
	public:
		LayerPtrSurface(const Glib::RefPtr<Layer>& layer) : m_layer(layer) {}
		const Glib::RefPtr<Layer>& get_layer(void) const { return m_layer; }
		int compare(const LayerPtrSurface& x) const;
		bool operator<(const LayerPtrSurface& x) const { return compare(x) < 0; }

	protected:
		Glib::RefPtr<Layer> m_layer;
	};

	class Interpolate;

	class ID8String {
	public:
		ID8String(uint8_t id, const char *str) : m_str(str), m_id(id) {}
		const char *get_str(const char *dflt = 0) const { return this && m_str ? m_str : dflt; }
		const char *get_str_nonnull(void) const { return get_str(&charstr_terminator); }
		uint8_t get_id(void) const { return this ? m_id : 0xff; }
		int compareid(const ID8String& x) const;

	protected:
		const char *m_str;
		uint8_t m_id;
	};

	class ID16String {
	public:
		ID16String(uint16_t id, const char *str) : m_str(str), m_id(id) {}
		const char *get_str(const char *dflt = 0) const { return this && m_str ? m_str : dflt; }
		const char *get_str_nonnull(void) const { return get_str(&charstr_terminator); }
		uint16_t get_id(void) const { return this ? m_id : 0xff; }
		int compareid(const ID16String& x) const;

	protected:
		const char *m_str;
		uint16_t m_id;
	};

	class CenterTable {
	public:
		CenterTable(uint16_t id, const char *str, const ID16String *subcenter = 0, uint16_t subcentersz = 0, const ID8String *model = 0, uint16_t modelsz = 0)
			: m_str(str), m_subcenter(subcenter), m_model(model), m_id(id), m_subcentersz(subcentersz), m_modelsz(modelsz) {}
		const char *get_str(const char *dflt = 0) const { return this && m_str ? m_str : dflt; }
		const char *get_str_nonnull(void) const { return get_str(&charstr_terminator); }
		uint16_t get_id(void) const { return this ? m_id : 0xff; }
		int compareid(const CenterTable& x) const;
		const ID16String *subcenter_begin(void) const { return m_subcenter; }
		const ID16String *subcenter_end(void) const { return m_subcenter + m_subcentersz; }
		const ID8String *model_begin(void) const { return m_model; }
		const ID8String *model_end(void) const { return m_model + m_modelsz; }

	protected:
		const char *m_str;
		const ID16String *m_subcenter;
		const ID8String *m_model;
		uint16_t m_id;
		uint16_t m_subcentersz;
		uint16_t m_modelsz;
	};

	class SurfaceTable {
	public:
		SurfaceTable(uint8_t id, const char *str, const char *unit) : m_str(str), m_unit(unit), m_id(id) {}
		const char *get_str(const char *dflt = 0) const { return this && m_str ? m_str : dflt; }
		const char *get_str_nonnull(void) const { return get_str(&charstr_terminator); }
		const char *get_unit(const char *dflt = 0) const { return this && m_unit ? m_unit : dflt; }
		const char *get_unit_nonnull(void) const { return get_unit(&charstr_terminator); }
		uint8_t get_id(void) const { return this ? m_id : 0xff; }
		int compareid(const SurfaceTable& x) const;

	protected:
		const char *m_str;
		const char *m_unit;
		uint8_t m_id;
	};


#ifdef HAVE_LIBCRYPTO
	class Cache {
	public:
		Cache(const std::vector<uint8_t>& filedata);
		Cache(const uint8_t *ptr = 0, unsigned int len = 0);
		std::string get_filename(const std::string& cachedir) const;
		bool load(const std::string& cachedir, std::vector<uint8_t>& data) const;
		void save(const std::string& cachedir, const std::vector<uint8_t>& filedata) { save(cachedir, &filedata[0], filedata.size()); }
		void save(const std::string& cachedir, const uint8_t *ptr = 0, unsigned int len = 0);
		template <typename T> bool load(const std::string& cachedir, std::vector<T>& data, unsigned int typesz = sizeof(T)) const;
		template <typename T> void save(const std::string& cachedir, const std::vector<T>& filedata, unsigned int typesz = sizeof(T)) { save(cachedir, &filedata[0], filedata.size(), typesz); }
		template <typename T> void save(const std::string& cachedir, const T *ptr = 0, unsigned int len = 0, unsigned int typesz = sizeof(T));
		static unsigned int expire(const std::string& cachedir, unsigned int maxdays = 14, off_t maxbytes = 1024*1024*1024);

	protected:
		uint8_t m_hash[MD4_DIGEST_LENGTH];
	};
#else
	class Cache {
	public:
		Cache(const std::vector<uint8_t>& filedata) {}
		Cache(const uint8_t *ptr = 0, unsigned int len = 0) {}
		std::string get_filename(const std::string& cachedir) const { return ""; }
		bool load(const std::string& cachedir, std::vector<uint8_t>& data) const { return false; }
		void save(const std::string& cachedir, const std::vector<uint8_t>& filedata) {}
		void save(const std::string& cachedir, const uint8_t *ptr = 0, unsigned int len = 0) {}
		template <typename T> bool load(const std::string& cachedir, std::vector<T>& data, unsigned int typesz = sizeof(T)) const { return false; }
		template <typename T> void save(const std::string& cachedir, const std::vector<T>& filedata, unsigned int typesz = sizeof(T)) { save(&filedata[0], filedata.size(), typesz); }
		template <typename T> void save(const std::string& cachedir, const T *ptr = 0, unsigned int len = 0, unsigned int typesz = sizeof(T)) {}
		static unsigned int expire(const std::string& cachedir, unsigned int maxdays = 14, off_t maxbytes = 1024*1024*1024) { return 0; }
	};
#endif

	static const char grib2_header[4];
	static const uint32_t grib2_terminator = (((uint32_t)'7') << 24) | (((uint32_t)'7') << 16) | (((uint32_t)'7') << 8) | ((uint32_t)'7');

	static const char charstr_terminator = 0;

	static const ParamDiscipline paramdiscipline[];
	static const ParamCategory paramcategory[];
	static const Parameter parameters[];

	static const ParamDiscipline *paramdisciplineindex[];
	static const ParamCategory *paramcategoryindex[];
	static const Parameter *parameternameindex[];
	static const Parameter *parameterabbrevindex[];

	static const unsigned int nr_paramdiscipline;
	static const unsigned int nr_paramcategory;
	static const unsigned int nr_parameters;
	static const unsigned int nr_paramdisciplineindex;
	static const unsigned int nr_paramcategoryindex;
	static const unsigned int nr_parameternameindex;
	static const unsigned int nr_parameterabbrevindex;

	static const struct ID8String genprocesstable[];
	static const unsigned int nr_genprocesstable;
	static const struct ID8String prodstatustable[];
	static const unsigned int nr_prodstatustable;
	static const struct ID8String typeofdatatable[];
	static const unsigned int nr_typeofdatatable;
	static const struct SurfaceTable surfacetable[];
	static const unsigned int nr_surfacetable;
	static const struct ID16String subcentertable_7[];
	static const struct ID8String genprocesstype_7[];
	static const struct CenterTable centertable[];
	static const unsigned int nr_centertable;

	struct parameter_cmp;
	struct parameter_abbrev_cmp;

	static const CenterTable *find_centerid_table(uint16_t cid);

	std::string m_cachedir;
	typedef std::set<LayerPtr> layers_t;
	layers_t m_layers;
	Glib::Mutex m_mutex;
};

#endif /* GRIB2_H */
