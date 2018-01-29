//
// C++ Interface: modes
//
// Description: Mode-S Transponder Message
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2013, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MODES_H
#define MODES_H

#include "sysdeps.h"

#include <limits>
#include <set>
#include <glibmm.h>

#include "geom.h"

class ModeSMessage {
  protected:

  public:
	static const uint32_t crc_poly = ((1 << 24) ^ (1 << 23) ^ (1 << 22) ^ (1 << 21) ^ (1 << 20) ^ (1 << 19) ^
					  (1 << 18) ^ (1 << 17) ^ (1 << 16) ^ (1 << 15) ^ (1 << 14) ^ (1 << 13) ^
					  (1 << 12) ^ (1 << 10) ^ (1 << 3) ^ (1 << 0));
	static const int NZ = 15;

	ModeSMessage(void);
	unsigned int get_length(void) const { return m_length ? 14 : 7; }
	bool is_long(void) const { return m_length; }
	uint8_t get_format(void) const { return (m_message[0] >> 3) & 0x1f; }
	uint8_t operator[](unsigned int idx) const;
	void set(unsigned int idx, uint8_t val);
	static uint32_t crc(const uint8_t *data, unsigned int length, uint32_t initial = 0U);
	uint32_t crc(void) const { return crc(m_message, get_length()); }
	uint32_t addr(void) const;
	static uint32_t addr_to_crc(uint32_t addr);
	static uint32_t crc_to_addr(uint32_t crc);
	int correct(uint32_t addr = 0U);
	std::string::const_iterator parse_raw(std::string::const_iterator si, std::string::const_iterator se);
	std::string::const_iterator parse_line(std::string::const_iterator si, std::string::const_iterator se);
	std::string get_raw_string(void) const;
	std::string get_msg_string(bool add_crc = true) const;

	bool is_adsb(void) const;
	bool is_adsb_ident_category(void) const;
	bool is_adsb_position(void) const;
	bool is_adsb_baroalt(void) const;
	bool is_adsb_gnssheight(void) const;
	bool is_adsb_velocity_cartesian(void) const;
	bool is_adsb_velocity_polar(void) const;
	bool is_adsb_modea(void) const;
	uint8_t get_adsb_type(void) const { return (m_message[4] >> 3) & 31; }
	bool get_adsb_timebit(void) const { return (m_message[6] >> 3) & 1; }
	bool get_adsb_cprformat(void) const { return (m_message[6] >> 2) & 1; }
	uint32_t get_adsb_cprlat(void) const;
	uint32_t get_adsb_cprlon(void) const;
	uint16_t get_adsb_altitude(void) const;
	uint8_t get_adsb_emergencystate(void) const { return (m_message[5] >> 5) & 7; }
	uint16_t get_adsb_modea(void) const;
	uint8_t get_adsb_emittercategory(void) const;
	std::string get_adsb_ident(void) const;
	int get_adsb_heading(void) const;
	int16_t get_adsb_velocity_east(void) const;
	int16_t get_adsb_velocity_north(void) const;
	int32_t get_adsb_vertical_rate(void) const;
	bool get_adsb_vertical_source_baro(void) const;
	int16_t get_adsb_airspeed(void) const;
	bool get_adsb_airspeed_tas(void) const;

	static Point decode_local_cpr17(uint32_t cprlat, uint32_t cprlon, bool cprformat, const Point& ref);
	Point decode_local_cpr17(const Point& ref) const;
	static std::pair<Point,Point> decode_global_cpr17(uint32_t cprlat0, uint32_t cprlon0, uint32_t cprlat1, uint32_t cprlon1);

	static int32_t decode_alt13(uint16_t altcode);
	static int32_t decode_alt12(uint16_t altcode);
	static int32_t decode_gilham(uint16_t altcode); // D1 D2 D4 A1 A2 A4 B1 B2 B4 D1 D2 D4
	static uint16_t decode_identity(uint16_t id);
	static std::string decode_registration(uint32_t icaoaddr);
	static std::string decode_registration_country(uint32_t icaoaddr);
	static uint32_t decode_registration(std::string::const_iterator i, std::string::const_iterator e);

  protected:
	static const uint32_t crc_table[256];
	static const uint32_t rev_crc_table[256];
#if 0
	static const uint32_t msg56_crc_residue[56+56*55/2];
	static const uint8_t msg56_crc_errloc[56+56*55/2][2];
#endif
	static const uint32_t msg112_crc_residue[112+112*111/2];
	static const uint8_t msg112_crc_errloc[112+112*111/2][2];
	static const double Dlat[2];
	static const double NLtable[4*NZ-1];
	static double NL(double lat);
	static unsigned int NLtbl(double lat);

	typedef std::string (*regdecode_t)(uint32_t icaoaddr);
	struct RegistrationEntry {
		uint32_t m_icaoaddr;
		const char *m_country;
		const char *m_regprefix;
		regdecode_t m_regdecode;

		bool operator<(const RegistrationEntry& x) const { return m_icaoaddr < x.m_icaoaddr; }
	};
	static const RegistrationEntry regtable[];
	static std::string regdecode_26(uint32_t icaoaddr, const char *pfx);
	static std::string regdecode_hb(uint32_t icaoaddr);
	static std::string regdecode_oh(uint32_t icaoaddr);
	static std::string regdecode_n(uint32_t icaoaddr);
	static std::string regdecode_n_1letter(uint32_t icaoaddr);
	static std::string regdecode_n_2letter(uint32_t icaoaddr);
	static std::string regdecode_5bit(uint32_t icaoaddr, const char *pfx, uint8_t choffs);
	static std::string regdecode_tc(uint32_t icaoaddr);
	static std::string regdecode_oy(uint32_t icaoaddr);
	static std::string regdecode_yu(uint32_t icaoaddr);
	static std::string regdecode_oo(uint32_t icaoaddr);
	static uint32_t decode_registration_hex(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_oct(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_bin(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_n(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_n_2letter(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_n_1letter(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_26(std::string::const_iterator i, std::string::const_iterator e, uint32_t baseaddr);
	static uint32_t decode_registration_hb(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_oh(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_5bit(std::string::const_iterator i, std::string::const_iterator e, uint32_t baseaddr, uint8_t choffs);
	static uint32_t decode_registration_tc(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_oy(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_yu(std::string::const_iterator i, std::string::const_iterator e);
	static uint32_t decode_registration_oo(std::string::const_iterator i, std::string::const_iterator e);

	uint8_t m_message[14];
	bool m_length;
};

#endif /* MODES_H */
