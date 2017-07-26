/*****************************************************************************/

/*
 *      modes.cc  --  Mode-S Transponder Messages.
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

#include "modes.h"

#include <cmath>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

const uint32_t ModeSMessage::crc_poly;
const int ModeSMessage::NZ;

const double ModeSMessage::Dlat[2] = {
	2.0 * M_PI / (4 * NZ),
	2.0 * M_PI / (4 * NZ - 1)
};

ModeSMessage::ModeSMessage(void)
	: m_length(false)
{
	memset(m_message, 0, sizeof(m_message));
}

uint8_t ModeSMessage::operator[](unsigned int idx) const
{
	if (idx >= get_length())
		return 0;
	return m_message[idx];
}

void ModeSMessage::set(unsigned int idx, uint8_t val)
{
	if (idx >= get_length())
		return;
	m_message[idx] = val;
}

uint32_t ModeSMessage::crc(const uint8_t *data, unsigned int length, uint32_t initial)
{
	for (; length > 0; --length, ++data) {
		uint8_t b((initial >> 16) ^ *data);
		initial <<= 8;
		initial ^= crc_table[b];
		initial &= 0xffffff;
	}
	return initial;
}

uint32_t ModeSMessage::addr(void) const
{
	unsigned int len(get_length() - 3);
	uint32_t c(crc(m_message, len));
	c ^= ((uint32_t)m_message[len++]) << 16;
	c ^= ((uint32_t)m_message[len++]) << 8;
	c ^= ((uint32_t)m_message[len]);
	return c;
}

uint32_t ModeSMessage::addr_to_crc(uint32_t addr)
{
	uint8_t a[3] = { (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff };
	return crc(a, 3, 0);
}

uint32_t ModeSMessage::crc_to_addr(uint32_t crc)
{
	for (unsigned int i = 0; i < 3; ++i)
		crc = (crc >> 8) ^ rev_crc_table[crc & 0xff];
	return crc;
}

int ModeSMessage::correct(uint32_t addr)
{
	uint32_t syn;
	{
		unsigned int len(get_length() - 3);
		syn = crc(m_message, len);
		uint8_t p[3];
		p[0] = m_message[len++] ^ (addr >> 16);
		p[1] = m_message[len++] ^ (addr >> 8);
		p[2] = m_message[len++] ^ addr;
		syn = crc(p, sizeof(p), syn);
	}
	if (!syn)
		return 0;
	unsigned int idx;
	{
		const uint32_t *p0(msg112_crc_residue);
		const uint32_t *p1(msg112_crc_residue + sizeof(msg112_crc_residue)/sizeof(msg112_crc_residue[0]));
		std::pair<const uint32_t *, const uint32_t *> ptr(std::equal_range(p0, p1, syn));
		if (ptr.first == ptr.second)
			return -1;
		idx = ptr.first - msg112_crc_residue;
	}
	if (!is_long() && (msg112_crc_errloc[idx][0] < 56 || msg112_crc_errloc[idx][1] < 56))
		return -1;
	int r(0);
	for (unsigned int i = 0; i < 2; ++i) {
		unsigned int b(msg112_crc_errloc[idx][i]);
		if (b == 255)
			continue;
		if (!is_long())
			b -= 56;
		if (b >= 8*sizeof(m_message))
			continue;
		m_message[b >> 3] ^= 1 << (b & 7);
		++r;
	}
	return r;
}

std::string::const_iterator ModeSMessage::parse_raw(std::string::const_iterator si, std::string::const_iterator se)
{
	std::string::const_iterator si1(si);
	unsigned int idx(0);
	memset(m_message, 0, sizeof(m_message));
	m_length = false;
	while (idx < 2 * sizeof(m_message) && si1 != se) {
		uint8_t ch(*si1++);
		if (ch >= '0' && ch <= '9') {
			ch -= '0';
		} else if (ch >= 'A' && ch <= 'F') {
			ch -= 'A' - 10;
		} else if (ch >= 'a' && ch <= 'f') {
			ch -= 'a' - 10;
		} else {
			break;
		}
		if (!(idx & 1))
			ch <<= 4;
		m_message[idx >> 1] |= ch;
		++idx;
		if (idx == sizeof(m_message)) {
			si = si1;
		} else if (idx == 2 * sizeof(m_message)) {
			si = si1;
			m_length = true;
		}
	}
	return si;
}

std::string::const_iterator ModeSMessage::parse_line(std::string::const_iterator si, std::string::const_iterator se)
{
	std::string::const_iterator si1(si);
	while (std::isspace(*si1) && si1 != se)
		++si1;
	if (si1 == se)
		return si;
	if (*si1++ != '*')
		return si;
	std::string::const_iterator si2(si1);
	si1 = parse_raw(si1, se);
	if (si1 == si2)
		return si;
	if (si1 == se)
		return si;
	if (*si1++ != ';')
		return si;
	return si1;
}

std::string ModeSMessage::get_raw_string(void) const
{
	std::ostringstream oss;
	oss << std::hex;
	for (unsigned int i = 0, len = get_length(); i < len; ++i)
		oss << std::setfill('0') << std::setw(2) << (unsigned int)m_message[i] << ' ';
	oss << "CRC " << std::setfill('0') << std::setw(6) << crc();
	return oss.str();
}

std::string ModeSMessage::get_msg_string(bool add_crc) const
{
	std::ostringstream oss;
	oss << '*' << std::hex;
	for (unsigned int i = 0, len = get_length(); i < len; ++i)
		oss << std::setfill('0') << std::setw(2) << (unsigned int)m_message[i];
	oss << ';';
	if (add_crc)
		oss << " CRC " << std::setfill('0') << std::setw(6) << crc();
	return oss.str();
}

bool ModeSMessage::is_adsb(void) const
{
	if (!m_length)
		return false;
	switch (get_format()) {
	case 17:
	case 18:
		break;

	default:
		return false;
	}
	return !crc();
}

bool ModeSMessage::is_adsb_ident_category(void) const
{
	if (!is_adsb())
		return false;
	uint8_t type((m_message[4] >> 3) & 0x1f);
	if (type < 1 || type > 4)
		return false;
	return true;
}

bool ModeSMessage::is_adsb_position(void) const
{
	if (!is_adsb())
		return false;
	uint8_t type((m_message[4] >> 3) & 0x1f);
	if (type < 5 || type == 19 || type > 22)
		return false;
	return true;
}

bool ModeSMessage::is_adsb_baroalt(void) const
{
	if (!is_adsb())
		return false;
	uint8_t type((m_message[4] >> 3) & 0x1f);
	if (!type)
		return !!get_adsb_altitude();
	if (type < 5 || type > 18)
		return false;
	return true;
}

bool ModeSMessage::is_adsb_gnssheight(void) const
{
	if (!is_adsb())
		return false;
	uint8_t type((m_message[4] >> 3) & 0x1f);
	if (type < 20 || type > 22)
		return false;
	return true;
}

bool ModeSMessage::is_adsb_velocity_cartesian(void) const
{
	if (!is_adsb())
		return false;
	if (m_message[4] >= ((19 << 3) | 1) && m_message[4] <= ((19 << 3) | 2))
		return true;
	return false;
}

bool ModeSMessage::is_adsb_velocity_polar(void) const
{
	if (!is_adsb())
		return false;
	if (m_message[4] >= ((19 << 3) | 3) && m_message[4] <= ((19 << 3) | 4))
		return true;
	return false;
}

bool ModeSMessage::is_adsb_modea(void) const
{
	if (!is_adsb())
		return false;
	return m_message[4] == ((28 << 3) | 1);
}

uint32_t ModeSMessage::get_adsb_cprlat(void) const
{
	uint32_t lat((m_message[6] << 16) | (m_message[7] << 8) | m_message[8]);
	return (lat >> 1) & 0x1ffff;
}

uint32_t ModeSMessage::get_adsb_cprlon(void) const
{
	uint32_t lon((m_message[8] << 16) | (m_message[9] << 8) | m_message[10]);
	return lon & 0x1ffff;
}

uint16_t ModeSMessage::get_adsb_altitude(void) const
{
	uint16_t alt((m_message[5] << 8) | m_message[6]);
	return alt >> 4;
}

uint16_t ModeSMessage::get_adsb_modea(void) const
{
	uint16_t modea((m_message[5] << 8) | m_message[6]);
	return modea & 0x1fff;
}

uint8_t ModeSMessage::get_adsb_emittercategory(void) const
{
	return (0x20 - (m_message[4] & 38)) | (m_message[4] & 0x07);
}

std::string ModeSMessage::get_adsb_ident(void) const
{
	std::string r;
	unsigned int idx(5 << 3);
	for (unsigned int i = 0; i < 8; ++i, idx += 6) {
		uint16_t ch((m_message[idx >> 3] << 8) | m_message[(idx >> 3) + 1]);
		ch <<= idx & 7;
		ch >>= 10;
		ch &= 0x3f;
		if (ch == 0x20) {
			r += ' ';
		} else if (ch >= 0x30 && ch <= 0x39) {
			r += (char)(ch + ('0' - 0x30));
		} else if (ch >= 1 && ch <= 26) {
			r += (char)(ch + ('A' - 1));
		} else {
			r += '?';
		}
	}
	return r;
}

int ModeSMessage::get_adsb_heading(void) const
{
	uint8_t type((m_message[4] >> 3) & 0x1f);
	uint16_t hdg((m_message[5] << 8) | m_message[6]);
	if (type == 19) {
		return hdg << 6;
	} else if (type >= 5 && type <= 8) {
		if (m_message[5] & 0x08)
			return (hdg << 5) & 0xfe00;
		return -1;
	}
	return -1;
}

int16_t ModeSMessage::get_adsb_velocity_east(void) const
{
	if (m_message[4] >= ((19 << 3) | 1) && m_message[4] <= ((19 << 3) | 2)) {
		int16_t v((m_message[5] << 8) | m_message[6]);
		v &= 0x3ff;
		if (!v)
			return std::numeric_limits<int16_t>::min();
		--v;
		v |= -(v & 0x200);
		if (m_message[5] & 0x04)
			v = -v;
		if (m_message[4] == ((19 << 3) | 2))
			v *= 4;
		return v;
	}
	return std::numeric_limits<int16_t>::min();
}

int16_t ModeSMessage::get_adsb_velocity_north(void) const
{
	if (m_message[4] >= ((19 << 3) | 1) && m_message[4] <= ((19 << 3) | 2)) {
		int16_t v((m_message[7] << 8) | m_message[8]);
		v >>= 5;
		v &= 0x3ff;
		if (!v)
			return std::numeric_limits<int16_t>::min();
		--v;
		v |= -(v & 0x200);
		if (m_message[7] & 0x80)
			v = -v;
		if (m_message[4] == ((19 << 3) | 2))
			v *= 4;
		return v;
	}
	return std::numeric_limits<int16_t>::min();
}

int32_t ModeSMessage::get_adsb_vertical_rate(void) const
{
	uint8_t type((m_message[4] >> 3) & 0x1f);
	if (type != 19)
		return std::numeric_limits<int32_t>::min();
	int16_t v((m_message[8] << 8) | m_message[9]);
	v >>= 2;
	v &= 0x1ff;
	if (!v)
		return std::numeric_limits<int32_t>::min();
	--v;
	v |= -(v & 0x100);
	v *= 64;
	return v;
}

bool ModeSMessage::get_adsb_vertical_source_baro(void) const
{
	uint8_t type((m_message[4] >> 3) & 0x1f);
	if (type != 19)
		return std::numeric_limits<int32_t>::min();
	return (m_message[8] >> 4) & 1;
}

int16_t ModeSMessage::get_adsb_airspeed(void) const
{
	if (m_message[4] >= ((19 << 3) | 3) && m_message[4] <= ((19 << 3) | 4)) {
		int16_t v((m_message[7] << 8) | m_message[8]);
		v >>= 5;
		v &= 0x3ff;
		if (!v)
			return std::numeric_limits<int16_t>::min();
		--v;
		v |= -(v & 0x200);
		if (m_message[4] == ((19 << 3) | 4))
			v *= 4;
		return v;
	}
	return std::numeric_limits<int16_t>::min();
}

bool ModeSMessage::get_adsb_airspeed_tas(void) const
{
	if (m_message[4] >= ((19 << 3) | 3) && m_message[4] <= ((19 << 3) | 4))
		return !!(m_message[7] & 0x80);
	return false;
}

double ModeSMessage::NL(double lat)
{
	double c(cos(fabs(lat)));
	return floor(2 * M_PI / acos(1 - (1 - cos(M_PI * 0.5 / NZ)) / (c * c)));
}

unsigned int ModeSMessage::NLtbl(double lat)
{
	lat = fabs(lat);
	if (std::isnan(lat))
		return 1;
	unsigned int i(0), j(sizeof(NLtable)/sizeof(NLtable[0]) - 1);
	while (i + 1 < j) {
		unsigned int k((i + j) >> 1);
		if (lat > NLtable[k])
			j = k;
		else
			i = k;
	}
	while (i && lat > NLtable[i - 1])
		--i;
	while (i < (sizeof(NLtable)/sizeof(NLtable[0]) - 1) && !(lat > NLtable[i]))
		++i;
	return i + 1;
}

Point ModeSMessage::decode_local_cpr17(uint32_t cprlat, uint32_t cprlon, bool cprformat, const Point& ref)
{
	static const double scale(1.0 / (1U << 17));
	double dlati(Dlat[cprformat & 1]);
	double cprlatscaled(cprlat * scale);
	double cprlonscaled(cprlon * scale);
	double t1(ref.get_lat_rad_dbl() / dlati);
	double t2(floor(t1));
	double j(t2 + floor(0.5 + (t1 - t2) - cprlatscaled));
	double rlati(dlati * (j + cprlatscaled));
	double dloni(2.0 * M_PI);
	t1 = NLtbl(rlati) - (cprformat & 1);
	if (t1 > 0)
		dloni /= t1;
	t1 = ref.get_lon_rad_dbl() / dloni;
	t2 = floor(t1);
	double m(t2 + floor(0.5 + (t1 - t2) - cprlonscaled));
	double rloni(dloni * (m + cprlonscaled));
	Point pt;
	pt.set_lat_rad_dbl(rlati);
	pt.set_lon_rad_dbl(rloni);
	return pt;
}

Point ModeSMessage::decode_local_cpr17(const Point& ref) const
{
	return decode_local_cpr17(get_adsb_cprlat(), get_adsb_cprlon(), get_adsb_cprformat(), ref);
}

std::pair<Point,Point> ModeSMessage::decode_global_cpr17(uint32_t cprlat0, uint32_t cprlon0, uint32_t cprlat1, uint32_t cprlon1)
{
	static const double scale(1.0 / (1U << 17));
	std::pair<Point,Point> r;
	r.first.set_invalid();
	r.second.set_invalid();
	double cprlat0scaled(cprlat0 * scale);
	double cprlon0scaled(cprlon0 * scale);
	double cprlat1scaled(cprlat1 * scale);
	double cprlon1scaled(cprlon1 * scale);
	double j(floor((4.0 * NZ - 1.0) * cprlat0scaled - (4.0 * NZ) * cprlat1scaled + 0.5));
	double rlat0(Dlat[0] * (j - (4.0 * NZ) * floor(j * (1.0 / (4.0 * NZ))) + cprlat0scaled));
	double rlat1(Dlat[1] * (j - (4.0 * NZ - 1.0) * floor(j * (1.0 / (4.0 * NZ - 1.0))) + cprlat1scaled));
	unsigned int nl0(NLtbl(rlat0));
	unsigned int nl1(NLtbl(rlat1));
	if (nl0 != nl1)
		return r;
	unsigned int n0(std::max(1U, nl0));
	unsigned int n1(std::max(1U, nl1 - 1U));
	double Dlon0(2.0 * M_PI / (double)n0);
	double Dlon1(2.0 * M_PI / (double)n1);
	double m(floor(cprlon0scaled * (nl0 - 1) - cprlon1scaled * nl0 + 0.5));
	double rlon0(Dlon0 * (m - n0 * floor(m / (double)n0) + cprlon0scaled));
	double rlon1(Dlon1 * (m - n1 * floor(m / (double)n1) + cprlon1scaled));
	r.first.set_lat_rad_dbl(rlat0);
	r.first.set_lon_rad_dbl(rlon0);
	r.second.set_lat_rad_dbl(rlat1);
	r.second.set_lon_rad_dbl(rlon1);
	return r;
}

int32_t ModeSMessage::decode_alt13(uint16_t altcode)
{
	if (!altcode)
		return std::numeric_limits<int32_t>::min();
	// M=1 (metric) not supported yet
	if (altcode & 0x0040)
		return std::numeric_limits<int32_t>::min();
	return decode_alt12(((altcode >> 1) & 0xFC0) | (altcode & 0x03F));
}

int32_t ModeSMessage::decode_alt12(uint16_t altcode)
{
	if (!altcode)
		return std::numeric_limits<int32_t>::min();
	// Q Bit
	if (altcode & 0x0010)
		return (((altcode >> 1) & 0xFF0) | (altcode & 0x00F)) * 25 - 1000;
	// Gilham Code: C1, A1, C2, A2, C4, A4, ZERO, B1, ZERO, B2, D2, B4, D4
	return decode_gilham(((altcode >> 8) & 0x001) |
			     ((altcode >> 9) & 0x002) |
			     ((altcode >> 10) & 0x004) |
			     ((altcode << 2) & 0x008) |
			     ((altcode << 1) & 0x010) |
			     (altcode & 0x020) |
			     ((altcode >> 1) & 0x040) |
			     ((altcode >> 2) & 0x080) |
			     ((altcode >> 3) & 0x100) |
			     ((altcode << 9) & 0x200) |
			     ((altcode << 8) & 0x400));
}

int32_t ModeSMessage::decode_gilham(uint16_t altcode)
{
	static const uint8_t dectbl[8] = { 0xff, 0, 2, 1, 4, 0xff, 3, 0xff };
	// D1 D2 D4 A1 A2 A4 B1 B2 B4 C1 C2 C4
	// D1-B4 is a gray code in 500ft increments
	unsigned int graycode((altcode >> 3) & 0x1FF);
	for (unsigned int mask = graycode >> 1; mask != 0; mask = mask >> 1)
		graycode ^= mask;
	uint8_t lowbits(dectbl[altcode & 0x007]);
	if (lowbits > 4)
		return std::numeric_limits<int32_t>::min();
	if (graycode & 1)
		lowbits = 4 - lowbits;
	return (graycode * 5 + lowbits) * 100 - 1200;
}

// input: C1-A1-C2-A2-C4-A4-ZERO-B1-D1-B2-D2-B4-D4
// bitnr: 12 11 10  9  8  7    6  5  4  3  2  1  0
// output:   A4 A2 A1 B4 B2   B1 C4 C2 C1 D4 D2 D1
uint16_t ModeSMessage::decode_identity(uint16_t id)
{
	return (((id << 4) & 0x800) |
		((id << 1) & 0x400) |
		((id >> 2) & 0x200) |
		((id << 7) & 0x100) |
		((id << 4) & 0x080) |
		((id << 1) & 0x040) |
		((id >> 3) & 0x020) |
		((id >> 6) & 0x010) |
		((id >> 9) & 0x008) |
		((id << 2) & 0x004) |
		((id >> 1) & 0x002) |
		((id >> 4) & 0x001));
}

// From ICAO_Annex_10_Volume_III.pdf

const ModeSMessage::RegistrationEntry ModeSMessage::regtable[] = {
	{ 0x004000, "Zimbabwe", "Z", 0 }, // 004000 - 0043FF Zimbabwe
	{ 0x004400, 0, 0, 0 },
	{ 0x006000, "Mozambique", "C9", 0 }, // 006000 - 006FFF Mozambique
	{ 0x007000, 0, 0, 0 },
	{ 0x008000, "South Africa", "ZS", 0 }, // 008000 - 00FFFF South Africa
	{ 0x010000, "Egypt", "SU", 0 }, // 010000 - 017FFF Egypt
	{ 0x018000, "Libyan Arab Jamahiriya", "5A", 0 }, // 018000 - 01FFFF Libyan Arab Jamahiriya
	{ 0x020000, "Morocco", "CN", 0 }, // 020000 - 027FFF Morocco
	{ 0x028000, "Tunisia", "TS", 0 }, // 028000 - 02FFFF Tunisia
	{ 0x030000, "Botswana", "A2", 0 }, // 030000 - 0303FF Botswana
	{ 0x030400, 0, 0, 0 },
	{ 0x032000, "Burundi", "9U", 0 }, // 032000 - 032FFF Burundi
	{ 0x033000, 0, 0, 0 },
	{ 0x034000, "Cameroon", "TJ", 0 }, // 034000 - 034FFF Cameroon
	{ 0x035000, "Comoros", "D6", 0 }, // 035000 - 0353FF Comoros
	{ 0x035400, 0, 0, 0 },
	{ 0x036000, "Congo", "TN", 0 }, // 036000 - 036FFF Congo
	{ 0x037000, 0, 0, 0 },
	{ 0x038000, "Cote d Ivoire", "TU", 0 }, // 038000 - 038FFF Cote d Ivoire
	{ 0x039000, 0, 0, 0 },
	{ 0x03e000, "Gabon", "TR", 0 }, // 03E000 - 03EFFF Gabon
	{ 0x03f000, 0, 0, 0 },
	{ 0x040000, "Ethiopia", "ET", 0 }, // 040000 - 040FFF Ethiopia
	{ 0x041000, 0, 0, 0 },
	{ 0x042000, "Equatorial Guinea", "3C", 0 }, // 042000 - 042FFF Equatorial Guinea
	{ 0x043000, 0, 0, 0 },
	{ 0x044000, "Ghana", "9G", 0 }, // 044000 - 044FFF Ghana
	{ 0x045000, 0, 0, 0 },
	{ 0x046000, "Guinea", "3X", 0 }, // 046000 - 046FFF Guinea
	{ 0x047000, 0, 0, 0 },
	{ 0x048000, "Guinea-Bissau", "J5", 0 }, // 048000 - 0483FF Guinea-Bissau
	{ 0x048400, 0, 0, 0 },
	{ 0x04a000, "Lesotho", "7P", 0 }, // 04A000 - 04A3FF Lesotho
	{ 0x04a400, 0, 0, 0 },
	{ 0x04c000, "Kenya", "5Y", 0 }, // 04C000 - 04CFFF Kenya
	{ 0x04d000, 0, 0, 0 },
	{ 0x050000, "Liberia", "A8", 0 }, // 050000 - 050FFF Liberia
	{ 0x051000, 0, 0, 0 },
	{ 0x054000, "Madagascar", "5R", 0 }, // 054000 - 054FFF Madagascar
	{ 0x055000, 0, 0, 0 },
	{ 0x058000, "Malawi", "7Q", 0 }, // 058000 - 058FFF Malawi
	{ 0x059000, 0, 0, 0 },
	{ 0x05a000, "Maldives", "8Q", 0 }, // 05A000 - 05A3FF Maldives
	{ 0x05a400, 0, 0, 0 },
	{ 0x05c000, "Mali", "TZ", 0 }, // 05C000 - 05CFFF Mali
	{ 0x05d000, 0, 0, 0 },
	{ 0x05e000, "Mauritania", "5T", 0 }, // 05E000 - 05E3FF Mauritania
	{ 0x05e400, 0, 0, 0 },
	{ 0x060000, "Mauritius", "3B", 0 }, // 060000 - 0603FF Mauritius
	{ 0x060400, 0, 0, 0 },
	{ 0x062000, "Niger", "5U", 0 }, // 062000 - 062FFF Niger
	{ 0x063000, 0, 0, 0 },
	{ 0x064000, "Nigeria", "5N", 0 }, // 064000 - 064FFF Nigeria
	{ 0x065000, 0, 0, 0 },
	{ 0x068000, "Uganda", "5X", 0 }, // 068000 - 068FFF Uganda
	{ 0x069000, 0, 0, 0 },
	{ 0x06a000, "Qatar", "A7", 0 }, // 06A000 - 06A3FF Qatar
	{ 0x06a400, 0, 0, 0 },
	{ 0x06c000, "Central African Republic", "TL", 0 }, // 06C000 - 06CFFF Central African Republic
	{ 0x06d000, 0, 0, 0 },
	{ 0x06e000, "Rwanda", "9XR", 0 }, // 06E000 - 06EFFF Rwanda
	{ 0x06f000, 0, 0, 0 },
	{ 0x070000, "Senegal", "6V", 0 }, // 070000 - 070FFF Senegal
	{ 0x071000, 0, 0, 0 },
	{ 0x074000, "Seychelles", "S7", 0 }, // 074000 - 0743FF Seychelles
	{ 0x074400, 0, 0, 0 },
	{ 0x076000, "Sierra Leone", "9L", 0 }, // 076000 - 0763FF Sierra Leone
	{ 0x076400, 0, 0, 0 },
	{ 0x078000, "Somalia", "6O", 0 }, // 078000 - 078FFF Somalia
	{ 0x079000, 0, 0, 0 },
	{ 0x07a000, "Swaziland", "3D", 0 }, // 07A000 - 07A3FF Swaziland
	{ 0x07a400, 0, 0, 0 },
	{ 0x07c000, "Sudan", "ST", 0 }, // 07C000 - 07CFFF Sudan
	{ 0x07d000, 0, 0, 0 },
	{ 0x080000, "United Republic of Tanzania", "5H", 0 }, // 080000 - 080FFF United Republic of Tanzania
	{ 0x081000, 0, 0, 0 },
	{ 0x084000, "Chad", "TT", 0 }, // 084000 - 084FFF Chad
	{ 0x085000, 0, 0, 0 },
	{ 0x088000, "Togo", "5V", 0 }, // 088000 - 088FFF Togo
	{ 0x089000, 0, 0, 0 },
	{ 0x08a000, "Zambia", "9J", 0 }, // 08A000 - 08AFFF Zambia
	{ 0x08b000, 0, 0, 0 },
	{ 0x08c000, "Democratic Republic of the Congo", "TN", 0 }, // 08C000 - 08CFFF Democratic Republic of the Congo
	{ 0x08d000, 0, 0, 0 },
	{ 0x090000, "Angola", "D2", 0 }, // 090000 - 090FFF Angola
	{ 0x091000, 0, 0, 0 },
	{ 0x094000, "Benin", "TY", 0 }, // 094000 - 0943FF Benin
	{ 0x095000, 0, 0, 0 },
	{ 0x096000, "Cape Verde", "D4", 0 }, // 096000 - 0963FF Cape Verde
	{ 0x096400, 0, 0, 0 },
	{ 0x098000, "Djibouti", "J2", 0 }, // 098000 - 0983FF Djibouti
	{ 0x098400, 0, 0, 0 },
	{ 0x09a000, "Gambia", "C5", 0 }, // 09A000 - 09AFFF Gambia
	{ 0x09b000, 0, 0, 0 },
	{ 0x09c000, "Burkina Faso", "XT", 0 }, // 09C000 - 09CFFF Burkina Faso
	{ 0x09d000, 0, 0, 0 },
	{ 0x09e000, "Sao Tome and Principe", "S9", 0 }, // 09E000 - 09E3FF Sao Tome and Principe
	{ 0x09e400, 0, 0, 0 },
	{ 0x0a0000, "Algeria", "7T", 0 }, // 0A0000 - 0A7FFF Algeria
	{ 0x0a8000, 0, 0, 0 },
	{ 0x0a8000, "Bahamas", "C6", 0 }, // 0A8000 - 0A8FFF Bahamas
	{ 0x0a9000, 0, 0, 0 },
	{ 0x0aa000, "Barbados", "8P", 0 }, // 0AA000 - 0AA3FF Barbados
	{ 0x0aa400, 0, 0, 0 },
	{ 0x0ab000, "Belize", "V3", 0 }, // 0AB000 - 0AB3FF Belize
	{ 0x0ab400, 0, 0, 0 },
	{ 0x0ac000, "Colombia", "HJ", 0 }, // 0AC000 - 0ACFFF Colombia
	{ 0x0ad000, 0, 0, 0 },
	{ 0x0ae000, "Costa Rica", "TI", 0 }, // 0AE000 - 0AEFFF Costa Rica
	{ 0x0af000, 0, 0, 0 },
	{ 0x0b0000, "Cuba", "CU", 0 }, // 0B0000 - 0B0FFF Cuba
	{ 0x0b1000, 0, 0, 0 },
	{ 0x0b2000, "El Salvador", "YS", 0 }, // 0B2000 - 0B2FFF El Salvador
	{ 0x0b3000, 0, 0, 0 },
	{ 0x0b4000, "Guatemala", "TG", 0 }, // 0B4000 - 0B4FFF Guatemala
	{ 0x0b5000, 0, 0, 0 },
	{ 0x0b6000, "Guyana", "8R", 0 }, // 0B6000 - 0B6FFF Guyana
	{ 0x0b7000, 0, 0, 0 },
	{ 0x0b8000, "Haiti", "HH", 0 }, // 0B8000 - 0B8FFF Haiti
	{ 0x0b9000, 0, 0, 0 },
	{ 0x0ba000, "Honduras", "HR", 0 }, // 0BA000 - 0BAFFF Honduras
	{ 0x0bb000, 0, 0, 0 },
	{ 0x0bc000, "Saint Vincent and the Grenadines", "J8", 0 }, // 0BC000 - 0BC3FF Saint Vincent and the Grenadines
	{ 0x0bc400, 0, 0, 0 },
	{ 0x0be000, "Jamaica", "6Y", 0 }, // 0BE000 - 0BEFFF Jamaica
	{ 0x0bf000, 0, 0, 0 },
	{ 0x0c0000, "Nicaragua", "YN", 0 }, // 0C0000 - 0C0FFF Nicaragua
	{ 0x0c1000, 0, 0, 0 },
	{ 0x0c2000, "Panama", "HP", 0 }, // 0C2000 - 0C2FFF Panama
	{ 0x0c3000, 0, 0, 0 },
	{ 0x0c4000, "Dominican Republic", "HI", 0 }, // 0C4000 - 0C4FFF Dominican Republic
	{ 0x0c5000, 0, 0, 0 },
	{ 0x0c6000, "Trinidad and Tobago", "9Y", 0 }, // 0C6000 - 0C6FFF Trinidad and Tobago
	{ 0x0c7000, 0, 0, 0 },
	{ 0x0c8000, "Suriname", "PZ", 0 }, // 0C8000 - 0C8FFF Suriname
	{ 0x0c9000, 0, 0, 0 },
	{ 0x0ca000, "Antigua and Barbuda", "V2", 0 }, // 0CA000 - 0CA3FF Antigua and Barbuda
	{ 0x0ca400, 0, 0, 0 },
	{ 0x0cc000, "Grenada", "J3", 0 }, // 0CC000 - 0CC3FF Grenada
	{ 0x0cc400, 0, 0, 0 },
	{ 0x0d0000, "Mexico", "XA", 0 }, // 0D0000 - 0D7FFF Mexico
	{ 0x0d8000, 0, 0, 0 },
	{ 0x0d8000, "Venezuela", "YV", 0 }, // 0D8000 - 0DFFFF Venezuela
	{ 0x0e0000, 0, 0, 0 },
	{ 0x100000, "Russian Federation", "RA", 0 }, // 100000 - 1FFFFF Russian Federation
	{ 0x200000, 0, 0, 0 },
	{ 0x201000, "Namibia", "V5", 0 }, // 201000 - 2013FF Namibia
	{ 0x201400, 0, 0, 0 },
	{ 0x202000, "Eritrea", "E3", 0 }, // 202000 - 2023FF Eritrea
	{ 0x202400, 0, 0, 0 },
	{ 0x300000, "Italy", "I", 0 }, // 300000 - 33FFFF Italy
	{ 0x340000, "Spain", "EC", 0 }, // 340000 - 37FFFF Spain
	{ 0x380000, "France", "F", 0 },	// 380000 - 3BFFFF France
	{ 0x3c0000, "Germany", "D", 0 }, // 3C0000 - 3FFFFF Germany
	{ 0x400000, "United Kingdom", "G", 0 }, // 400000 - 43FFFF United Kingdom
	{ 0x440000, "Austria", "OE", 0 }, // 440000 - 447FFF Austria
	{ 0x448000, "Belgium", "OO", &regdecode_oo }, // 448000 - 44FFFF Belgium
	{ 0x450000, "Bulgaria", "LZ", 0 }, // 450000 - 457FFF Bulgaria
	{ 0x458000, "Denmark", "OY", &regdecode_oy }, // 458000 - 45FFFF Denmark
	{ 0x460000, "Finland", "OH", &regdecode_oh }, // 460000 - 467FFF Finland
	{ 0x468000, "Greece", "SX", 0 }, // 468000 - 46FFFF Greece
	{ 0x470000, "Hungary", "HA", 0 }, // 470000 - 477FFF Hungary
	{ 0x478000, "Norway", "LN", 0 }, // 478000 - 47FFFF Norway
	{ 0x480000, "Kingdom of the Netherlands", "PA", 0 }, // 480000 - 487FFF Kingdom of the Netherlands
	{ 0x488000, "Poland", "SP", 0 }, // 488000 - 48FFFF Poland
	{ 0x490000, "Portugal", "CR", 0 }, // 490000 - 497FFF Portugal
	{ 0x498000, "Czech Republic", "OK", 0 }, // 498000 - 49FFFF Czech Republic
	{ 0x4a0000, "Romania", "YR", 0 }, // 4A0000 - 4A7FFF Romania
	{ 0x4a8000, "Sweden", "SE", 0 }, // 4A8000 - 4AFFFF Sweden
	{ 0x4b0000, "Switzerland", "HB", &regdecode_hb }, // 4B0000 - 4B7FFF Switzerland
	{ 0x4b8000, "Turkey", "TC", &regdecode_tc }, // 4B8000 - 4BFFFF Turkey
	{ 0x4c0000, "Yugoslavia", "YU", &regdecode_yu }, // 4C0000 - 4C7FFF Yugoslavia
	{ 0x4c8000, "Cyprus", "5B", 0 }, // 4C8000 - 4C83FF Cyprus
	{ 0x4c8400, 0, 0, 0 },
	{ 0x4ca000, "Ireland", "EI", 0 }, // 4CA000 - 4CAFFF Ireland
	{ 0x4cb000, 0, 0, 0 },
	{ 0x4cc000, "Iceland", "TF", 0 }, // 4CC000 - 4CCFFF Iceland
	{ 0x4cd000, 0, 0, 0 },
	{ 0x4d0000, "Luxembourg", "LX", 0 }, // 4D0000 - 4D03FF Luxembourg
	{ 0x4d0400, 0, 0, 0 },
	{ 0x4d2000, "Malta", "9H", 0 }, // 4D2000 - 4D23FF Malta
	{ 0x4d2400, 0, 0, 0 },
	{ 0x4d4000, "Monaco", "3A", 0 }, // 4D4000 - 4D43FF Monaco
	{ 0x4d4400, 0, 0, 0 },
	{ 0x500000, "San Marino", "T7", 0 }, // 500000 - 5003FF San Marino
	{ 0x500400, 0, 0, 0 },
	{ 0x501000, "Albania", "ZA", 0 }, // 501000 - 5013FF Albania
	{ 0x501400, 0, 0, 0 },
	{ 0x501c00, "Croatia", "9A", 0 }, // 501C00 - 501FFF Croatia
	{ 0x502000, 0, 0, 0 },
	{ 0x502c00, "Latvia", "YL", 0 }, // 502C00 - 502FFF Latvia
	{ 0x503000, 0, 0, 0 },
	{ 0x503c00, "Lithuania", "YL", 0 }, // 503C00 - 503FFF Lithuania
	{ 0x504000, 0, 0, 0 },
	{ 0x504c00, "Republic of Moldova", "ER", 0 }, // 504C00 - 504FFF Republic of Moldova
	{ 0x505000, 0, 0, 0 },
	{ 0x505c00, "Slovakia", "OM", 0 }, // 505C00 - 505FFF Slovakia
	{ 0x506000, 0, 0, 0 },
	{ 0x506c00, "Slovenia", "S5", 0 }, // 506C00 - 506FFF Slovenia
	{ 0x507000, 0, 0, 0 },
	{ 0x507c00, "Uzbekistan", "UK", 0 }, // 507C00 - 507FFF Uzbekistan
	{ 0x508000, 0, 0, 0 },
	{ 0x508000, "Ukraine", "UR", 0 }, // 508000 - 50FFFF Ukraine
	{ 0x510000, "Belarus", "EW", 0 }, // 510000 - 5103FF Belarus
	{ 0x510400, 0, 0, 0 },
	{ 0x511000, "Estonia", "ES", 0 }, // 511000 - 5113FF Estonia
	{ 0x511400, 0, 0, 0 },
	{ 0x512000, "The former Yugoslav Republic of Macedonia", "Z3", 0 }, // 512000 - 5123FF The former Yugoslav Republic of Macedonia
	{ 0x512400, 0, 0, 0 },
	{ 0x513000, "Bosnia and Herzegovina", "T9", 0 }, // 513000 - 5133FF Bosnia and Herzegovina
	{ 0x513400, 0, 0, 0 },
	{ 0x514000, "Georgia", "4L", 0 }, // 514000 - 5143FF Georgia
	{ 0x514400, 0, 0, 0 },
	{ 0x515000, "Tajikistan", "EY", 0 }, // 515000 - 5153FF Tajikistan
	{ 0x515400, 0, 0, 0 },
	{ 0x600000, "Armenia", "EK", 0 }, // 600000 - 6003FF Armenia
	{ 0x600400, 0, 0, 0 },
	{ 0x600800, "Azerbaijan", "4K", 0 }, // 600800 - 600BFF Azerbaijan
	{ 0x600c00, 0, 0, 0 },
	{ 0x601000, "Kyrgyzstan", "EX", 0 }, // 601000 - 6013FF Kyrgyzstan
	{ 0x601400, 0, 0, 0 },
	{ 0x601800, "Turkmenistan", "EZ", 0 }, // 601800 - 601BFF Turkmenistan
	{ 0x601c00, 0, 0, 0 },
	{ 0x680000, "Bhutan", "A5", 0 }, // 680000 - 6803FF Bhutan
	{ 0x680400, 0, 0, 0 },
	{ 0x681000, "Federated States of Micronesia", "V6", 0 }, // 681000 - 6813FF Federated States of Micronesia
	{ 0x681400, 0, 0, 0 },
	{ 0x682000, "Mongolia", "JU", 0 }, // 682000 - 6823FF Mongolia
	{ 0x682400, 0, 0, 0 },
	{ 0x683000, "Kazakhstan", "UP", 0 }, // 683000 - 6833FF Kazakhstan
	{ 0x683400, 0, 0, 0 },
	{ 0x684000, "Palau", "", 0 }, // 684000 - 6843FF Palau
	{ 0x684400, 0, 0, 0 },
	{ 0x700000, "Afghanistan", "YA", 0 }, // 700000 - 700FFF Afghanistan
	{ 0x701000, 0, 0, 0 },
	{ 0x702000, "Bangladesh", "S2", 0 }, // 702000 - 702FFF Bangladesh
	{ 0x703000, 0, 0, 0 },
	{ 0x704000, "Myanmar", "XY", 0 }, // 704000 - 704FFF Myanmar
	{ 0x705000, 0, 0, 0 },
	{ 0x706000, "Kuwait", "9K", 0 }, // 706000 - 706FFF Kuwait
	{ 0x707000, 0, 0, 0 },
	{ 0x708000, "Lao People's Democratic Republic", "RDPL", 0 }, // 708000 - 708FFF Lao People's Democratic Republic
	{ 0x709000, 0, 0, 0 },
	{ 0x70a000, "Nepal", "9N", 0 }, // 70A000 - 70AFFF Nepal
	{ 0x70b000, 0, 0, 0 },
	{ 0x70c000, "Oman", "A4O", 0 }, // 70C000 - 70C3FF Oman
	{ 0x70d000, 0, 0, 0 },
	{ 0x70e000, "Cambodia", "XU", 0 }, // 70E000 - 70EFFF Cambodia
	{ 0x70f000, 0, 0, 0 },
	{ 0x710000, "Saudi Arabia", "HZ", 0 }, // 710000 - 717FFF 
	{ 0x718000, "Republic of Korea", "DS", 0 }, // 718000 - 71FFFF Republic of Korea
	{ 0x720000, "Democratic People's Republic of Korea", "P", 0 }, // 720000 - 727FFF Democratic People's Republic of Korea
	{ 0x728000, "Iraq", "YI", 0 }, // 728000 - 72FFFF Iraq
	{ 0x730000, "Islamic Republic of Iran", "EP", 0 }, // 730000 - 737FFF Islamic Republic of Iran
	{ 0x738000, "Israel", "4X", 0 }, // 738000 - 73FFFF Israel
	{ 0x740000, "Jordan", "JY", 0 }, // 740000 - 747FFF Jordan
	{ 0x748000, "Lebanon", "OD", 0 }, // 748000 - 74F000 Lebanon
	{ 0x750000, "Malaysia", "9M", 0 }, // 750000 - 757FFF Malaysia
	{ 0x758000, "Philippines", "RPC", 0 }, // 758000 - 758FFF Philippines
	{ 0x760000, "Pakistan", "AP", 0 }, // 760000 - 767FFF Pakistan
	{ 0x768000, "Singapore", "9V", 0 }, // 768000 - 76FFFF Singapore
	{ 0x770000, "Sri Lanka", "4R", 0 }, // 770000 - 777FFF Sri Lanka
	{ 0x778000, "Syrian Arab Republic", "YK", 0 }, // 778000 - 77FFFF Syrian Arab Republic
	{ 0x780000, "China", "B", 0 }, // 780000 - 7BFFFF China
	{ 0x7c0000, "Australia", "VH", 0 }, // 7C0000 - 7FFFFF Australia
	{ 0x800000, "India", "VT", 0 }, // 800000 - 83FFFF India
	{ 0x840000, "Japan", "JA", 0 }, // 840000 - 87FFFF Japan
	{ 0x880000, "Thailand", "HS", 0 }, // 880000 - 887FFF Thailand
	{ 0x888000, "Viet Nam", "VN", 0 }, // 888000 - 88FFFF Viet Nam
	{ 0x890000, "Yemen", "7O", 0 }, // 890000 - 890FFF Yemen
	{ 0x891000, 0, 0, 0 },
	{ 0x894000, "Bahrain", "A9C", 0 }, // 894000 - 894FFF Bahrain
	{ 0x895000, "Brunei Darussalam", "V8", 0 }, // 895000 - 8953FF Brunei Darussalam
	{ 0x895400, 0, 0, 0 },
	{ 0x896000, "United Arab Emirates", "A6", 0 }, // 896000 - 896FFF United Arab Emirates
	{ 0x897000, "Solomon Islands", "H4", 0 }, // 897000 - 8973FF Solomon Islands
	{ 0x897400, 0, 0, 0 },
	{ 0x898000, "Papua New Guinea", "P2", 0 }, // 898000 - 898FFF Papua New Guinea
	{ 0x899000, "Taiwan", "BV", 0 }, // 899000 - 8993FF ICAO (2) (Taiwan)
	{ 0x899400, 0, 0, 0 },
	{ 0x8a0000, "Indonesia", "PK", 0 }, // 8A0000 - 8A7FFF Indonesia
	{ 0x8a8000, 0, 0, 0 },
	{ 0x900000, "Marshall Islands", "V7", 0 }, // 900000 - 9003FF Marshall Islands
	{ 0x900400, 0, 0, 0 },
	{ 0x901000, "Cook Islands", "E5", 0 }, // 901000 - 9013FF Cook Islands
	{ 0x901400, 0, 0, 0 },
	{ 0x902000, "Samoa", "5W", 0 }, // 902000 - 9023FF Samoa
	{ 0x902400, 0, 0, 0 },
	{ 0xa00000, "United States of America", "N", &regdecode_n }, // A00000 - AFFFFF USA
	{ 0xb00000, 0, 0, 0 },
	{ 0xc00000, "Canada", "CF", 0 }, // C00000 - C3FFFF Canada
	{ 0xc40000, 0, 0, 0 },
	{ 0xc80000, "New Zealand", "ZK", 0 }, // C80000 - C87FFF New Zealand
	{ 0xc88000, "Fiji", "DQ", 0 }, // C88000 - C88FFF Fiji
	{ 0xc89000, 0, 0, 0 },
	{ 0xc8a000, "Nauru", "C2", 0 }, // C8A000 - C8A3FF Nauru
	{ 0xc8a400, 0, 0, 0 },
	{ 0xc8c000, "Saint Lucia", "J6", 0 }, // C8C000 - C8C3FF Saint Lucia
	{ 0xc8c400, 0, 0, 0 },
	{ 0xc8d000, "Tonga", "A3", 0 }, // C8D000 - C8D3FF Tonga
	{ 0xc8d400, 0, 0, 0 },
	{ 0xc8e000, "Kiribati", "T3", 0 }, // C8E000 - C8E3FF Kiribati
	{ 0xc8e400, 0, 0, 0 },
	{ 0xc90000, "Vanuatu", "YJ", 0 }, // C90000 - C903FF Vanuatu
	{ 0xc90400, 0, 0, 0 },
	{ 0xe00000, "Argentina", "LV", 0 }, // E00000 - E3FFFF Argentina
	{ 0xe40000, "Brazil", "PP", 0 }, // E40000 - E7FFFF Brazil
	{ 0xe80000, "Chile", "CC", 0 }, // E80000 - E80FFF Chile
	{ 0xe81000, 0, 0, 0 },
	{ 0xe84000, "Ecuador", "HC", 0 }, // E84000 - E84FFF Ecuador
	{ 0xe85000, 0, 0, 0 },
	{ 0xe88000, "Paraguay", "ZP", 0 }, // E88000 - E88FFF Paraguay
	{ 0xe89000, 0, 0, 0 },
	{ 0xe8c000, "Peru", "OB", 0 }, // E8C000 - E8CFFF Peru
	{ 0xe8d000, 0, 0, 0 },
	{ 0xe90000, "Uruguay", "CX", 0 }, // E90000 - E90FFF Uruguay
	{ 0xe91000, 0, 0, 0 },
	{ 0xe94000, "Bolivia", "CP", 0 }, // E94000 - E94FFF Bolivia
	{ 0xe95000, 0, 0, 0 },
	{ 0xf00000, "ICAO (1)", "", 0 }, // F00000 - F07FFF ICAO (1)
	{ 0xf08000, 0, 0, 0 },
	{ 0xf09000, "ICAO (3)", "", 0 }, // F09000 - F093FF ICAO (3)
	{ 0xf09400, 0, 0, 0 }
};

std::string ModeSMessage::regdecode_26(uint32_t icaoaddr, const char *pfx)
{
	std::string r(pfx);
	if (icaoaddr >= 26*26*26)
		return r;
	unsigned int x(icaoaddr / (26*26));
	icaoaddr -= x * (26 * 26);
	r += (char)('A' + x);
	x = icaoaddr / 26;
	icaoaddr -= x * 26;
	r += (char)('A' + x);
	r += (char)('A' + icaoaddr);
	return r;
}

std::string ModeSMessage::regdecode_hb(uint32_t icaoaddr)
{
	if (icaoaddr >= 26*26*26) {
		std::string r("HB");
		icaoaddr -= 26*26*26 - 1;
		if (icaoaddr <= 9999) {
			std::ostringstream oss;
			oss << icaoaddr;
			r += oss.str();
			return r;
		}
		return r;
	}
	return regdecode_26(icaoaddr, "HB");
}

std::string ModeSMessage::regdecode_oh(uint32_t icaoaddr)
{
	return regdecode_26(icaoaddr, "OH");
}

std::string ModeSMessage::regdecode_n_1letter(uint32_t icaoaddr)
{
	if (!icaoaddr)
		return "";
	std::ostringstream oss;
	if (icaoaddr >= 25) {
		oss << (char)('0' - 25 + icaoaddr);
		return oss.str();
	}
	// skip I, O
	if (icaoaddr >= 9)
		++icaoaddr;
	if (icaoaddr >= 15)
		++icaoaddr;
	oss << (char)('A' - 1 + icaoaddr);
	return oss.str();
}

std::string ModeSMessage::regdecode_n_2letter(uint32_t icaoaddr)
{
	if (!icaoaddr)
		return "";
	--icaoaddr;
	unsigned int x(icaoaddr / 25U);
	icaoaddr -= x * 25U;
	if (x >= 8)
		++x;
	if (x >= 14)
		++x;
	std::ostringstream oss;
	oss << (char)('A'  + x);
	if (!icaoaddr)
		return oss.str();
	if (icaoaddr >= 9)
		++icaoaddr;
	if (icaoaddr >= 15)
		++icaoaddr;
	oss << (char)('A' - 1 + icaoaddr);
	return oss.str();
}

std::string ModeSMessage::regdecode_n(uint32_t icaoaddr)
{
	std::ostringstream oss;
	oss << 'N';
	if (icaoaddr < 1)
		return oss.str();
	--icaoaddr;
#if 0
	for (unsigned int d1 = 1; d1 <= 9; ++d1) {
		if (icaoaddr < 601) {
			oss << (char)('0' + d1) << regdecode_n_2letter(icaoaddr);
			return oss.str();
		}
		icaoaddr -= 601;
		for (unsigned int d2 = 0; d2 <= 9; ++d2) {
			if (icaoaddr < 601) {
				oss << (char)('0' + d1) << (char)('0' + d2) << regdecode_n_2letter(icaoaddr);
				return oss.str();
			}
			icaoaddr -= 601;
			for (unsigned int d3 = 0; d3 <= 9; ++d3) {
				if (icaoaddr < 601) {
					oss << (char)('0' + d1) << (char)('0' + d2) << (char)('0' + d3) << regdecode_n_2letter(icaoaddr);
					return oss.str();
				}
				icaoaddr -= 601;
				for (unsigned int d4 = 0; d4 <= 9; ++d4) {
					if (icaoaddr < 35) {
						oss << (char)('0' + d1) << (char)('0' + d2) << (char)('0' + d3) << (char)('0' + d4) << regdecode_n_1letter(icaoaddr);
						return oss.str();
					}
					icaoaddr -= 35;
				}
			}
		}
	}
	return oss.str();
#endif
	// first digit
	{
		unsigned int d(icaoaddr / 101711U);
		if (d >= 9)
			return oss.str();
		icaoaddr -= d * 101711U;
		oss << (char)('1' + d);
	}
	if (icaoaddr < 601)
		return oss.str() + regdecode_n_2letter(icaoaddr);
	icaoaddr -= 601;
	// second digit
	{
		unsigned int d(icaoaddr / 10111U);
		if (d >= 10)
			return oss.str();
		icaoaddr -= d * 10111U;
		oss << (char)('0' + d);
	}
	if (icaoaddr < 601)
		return oss.str() + regdecode_n_2letter(icaoaddr);
	icaoaddr -= 601;
	// third digit
	{
		unsigned int d(icaoaddr / 951U);
		if (d >= 10)
			return oss.str();
		icaoaddr -= d * 951U;
		oss << (char)('0' + d);
	}
	if (icaoaddr < 601)
		return oss.str() + regdecode_n_2letter(icaoaddr);
	icaoaddr -= 601;
	// fourth digit
	{
		unsigned int d(icaoaddr / 35U);
		if (d >= 10)
			return oss.str();
		icaoaddr -= d * 35U;
		oss << (char)('0' + d);
	}
	return oss.str() + regdecode_n_1letter(icaoaddr);
}

std::string ModeSMessage::regdecode_5bit(uint32_t icaoaddr, const char *pfx, uint8_t choffs)
{
	std::string r(pfx);
	for (unsigned int i = 0; i < 3; ++i) {
		uint8_t ch(icaoaddr >> 10);
		icaoaddr <<= 5;
		ch -= choffs;
		ch &= 0x1f;
		if (ch < 0 || ch > 25)
			return pfx;
		r += (char)(ch + 'A');
	}
	return r;
}

std::string ModeSMessage::regdecode_tc(uint32_t icaoaddr)
{
	return regdecode_5bit(icaoaddr, "TC", 1);
}

std::string ModeSMessage::regdecode_oy(uint32_t icaoaddr)
{
	return regdecode_5bit(icaoaddr, "OY", 1);
}

std::string ModeSMessage::regdecode_yu(uint32_t icaoaddr)
{
	return regdecode_5bit(icaoaddr, "YU", 0);
}

std::string ModeSMessage::regdecode_oo(uint32_t icaoaddr)
{
	return regdecode_5bit(icaoaddr, "OO", 1);
}

std::string ModeSMessage::decode_registration(uint32_t icaoaddr)
{
	icaoaddr &= 0xffffff;
	RegistrationEntry re;
	re.m_icaoaddr = icaoaddr;
	const RegistrationEntry *p(std::upper_bound(regtable, &regtable[sizeof(regtable)/sizeof(regtable[0])], re));
	if (p == regtable)
		return "";
	--p;
	if (p->m_regdecode)
		return (*p->m_regdecode)(icaoaddr - p->m_icaoaddr);
	if (p->m_regprefix)
		return p->m_regprefix;
	return "";
}

std::string ModeSMessage::decode_registration_country(uint32_t icaoaddr)
{
	icaoaddr &= 0xffffff;
	RegistrationEntry re;
	re.m_icaoaddr = icaoaddr;
	const RegistrationEntry *p(std::upper_bound(regtable, &regtable[sizeof(regtable)/sizeof(regtable[0])], re));
	if (p == regtable)
		return "";
	--p;
	if (p->m_country)
		return p->m_country;
	return "";
}

uint32_t ModeSMessage::decode_registration(std::string::const_iterator i, std::string::const_iterator e)
{
	// remove leading/trailing space
	while (i != e && (std::isspace(*i) || *i == '\n' || *i == '\r'))
		++i;
	while (i != e) {
		std::string::const_iterator e2(e);
		--e2;
		if (std::isspace(*e2) || *e2 == '\n' || *e2 == '\r') {
			e = e2;
			continue;
		}
		break;
	}
	unsigned int len(0);
	bool hexonly(true);
	bool octonly(true);
	bool binonly(true);
	for (std::string::const_iterator i2(i); i2 != e; ++i2, ++len) {
		hexonly = hexonly && std::isxdigit(*i2);
		octonly = octonly && (*i2 >= '0' && *i2 <= '7');
		binonly = binonly && (*i2 >= '0' && *i2 <= '1');
	}
	if (len == 6 && hexonly) {
		uint32_t r(decode_registration_hex(i, e));
		if (r != std::numeric_limits<uint32_t>::max())
			return r;
	}
	if (len == 8 && octonly) {
		uint32_t r(decode_registration_oct(i, e));
		if (r != std::numeric_limits<uint32_t>::max())
			return r;
	}
	if (len == 24 && binonly) {
		uint32_t r(decode_registration_bin(i, e));
		if (r != std::numeric_limits<uint32_t>::max())
			return r;
	}
	switch (*i) {
	case 'n':
	case 'N':
	{
		uint32_t r(decode_registration_n(i + 1, e));
		if (r != std::numeric_limits<uint32_t>::max())
			return r;
		break;
	}

	case 'h':
	case 'H':
		switch (*(i + 1)) {
		case 'b':
		case 'B':
		{
			uint32_t r(decode_registration_hb(i + 2, e));
			if (r != std::numeric_limits<uint32_t>::max())
				return r;
			break;
		}

		default:
			break;
		}
		break;

	case 'o':
	case 'O':
		switch (*(i + 1)) {
		case 'h':
		case 'H':
		{
			uint32_t r(decode_registration_oh(i + 2, e));
			if (r != std::numeric_limits<uint32_t>::max())
				return r;
			break;
		}

		case 'o':
		case 'O':
		{
			uint32_t r(decode_registration_oo(i + 2, e));
			if (r != std::numeric_limits<uint32_t>::max())
				return r;
			break;
		}

		case 'y':
		case 'Y':
		{
			uint32_t r(decode_registration_oy(i + 2, e));
			if (r != std::numeric_limits<uint32_t>::max())
				return r;
			break;
		}

		default:
			break;
		}
		break;

	case 't':
	case 'T':
		switch (*(i + 1)) {
		case 'c':
		case 'C':
		{
			uint32_t r(decode_registration_tc(i + 2, e));
			if (r != std::numeric_limits<uint32_t>::max())
				return r;
			break;
		}

		default:
			break;
		}
		break;

	case 'y':
	case 'Y':
		switch (*(i + 1)) {
		case 'u':
		case 'U':
		{
			uint32_t r(decode_registration_yu(i + 2, e));
			if (r != std::numeric_limits<uint32_t>::max())
				return r;
			break;
		}

		default:
			break;
		}
		break;

	default:
		break;
	}
	if (len <= 6 && hexonly) {
		uint32_t r(decode_registration_hex(i, e));
		if (r != std::numeric_limits<uint32_t>::max())
			return r;
	}
	if (len <= 8 && octonly) {
		uint32_t r(decode_registration_oct(i, e));
		if (r != std::numeric_limits<uint32_t>::max())
			return r;
	}
	if (len <= 24 && binonly) {
		uint32_t r(decode_registration_bin(i, e));
		if (r != std::numeric_limits<uint32_t>::max())
			return r;
	}
	return std::numeric_limits<uint32_t>::max();
}

uint32_t ModeSMessage::decode_registration_hex(std::string::const_iterator i, std::string::const_iterator e)
{
	if (i == e)
		return std::numeric_limits<uint32_t>::max();
	uint32_t r(0);
	for (; i != e; ++i) {
		uint8_t ch(*i);
		if (ch >= '0' && ch <= '9')
			ch -= '0';
		else if (ch >= 'A' && ch <= 'F')
			ch -= 'A' - 10;
		else if (ch >= 'a' && ch <= 'f')
			ch -= 'a' - 10;
		else
			return std::numeric_limits<uint32_t>::max();
		r <<= 4;
		r |= ch;
	}
	return r;
}

uint32_t ModeSMessage::decode_registration_oct(std::string::const_iterator i, std::string::const_iterator e)
{
	if (i == e)
		return std::numeric_limits<uint32_t>::max();
	uint32_t r(0);
	for (; i != e; ++i) {
		uint8_t ch(*i);
		if (ch >= '0' && ch <= '7')
			ch -= '0';
		else
			return std::numeric_limits<uint32_t>::max();
		r <<= 3;
		r |= ch;
	}
	return r;
}

uint32_t ModeSMessage::decode_registration_bin(std::string::const_iterator i, std::string::const_iterator e)
{
	if (i == e)
		return std::numeric_limits<uint32_t>::max();
	uint32_t r(0);
	for (; i != e; ++i) {
		uint8_t ch(*i);
		if (ch >= '0' && ch <= '1')
			ch -= '0';
		else
			return std::numeric_limits<uint32_t>::max();
		r <<= 1;
		r |= ch;
	}
	return r;
}

uint32_t ModeSMessage::decode_registration_n(std::string::const_iterator i, std::string::const_iterator e)
{
	if (i == e)
		return std::numeric_limits<uint32_t>::max();
	// first digit
	uint32_t r(0xa00001);
	{
		char ch(*i++);
		if (ch == 'I' || ch == 'i')
			ch = '1';
		else if (ch == 'O' || ch == 'o')
			ch = '0';
		if (ch < '1' || ch > '9')
			return std::numeric_limits<uint32_t>::max();
		r += (ch - '1') * 101711U;
	}
	{
		uint32_t r1(decode_registration_n_2letter(i, e));
		if (r1 != std::numeric_limits<uint32_t>::max())
			return r + r1;
	}
	r += 601;
	// second digit
	{
		char ch(*i++);
		if (ch == 'I' || ch == 'i')
			ch = '1';
		else if (ch == 'O' || ch == 'o')
			ch = '0';
		if (ch < '0' || ch > '9')
			return std::numeric_limits<uint32_t>::max();
		r += (ch - '0') * 10111U;
	}
	{
		uint32_t r1(decode_registration_n_2letter(i, e));
		if (r1 != std::numeric_limits<uint32_t>::max())
			return r + r1;
	}
	r += 601;
	// third digit
	{
		char ch(*i++);
		if (ch == 'I' || ch == 'i')
			ch = '1';
		else if (ch == 'O' || ch == 'o')
			ch = '0';
		if (ch < '0' || ch > '9')
			return std::numeric_limits<uint32_t>::max();
		r += (ch - '0') * 951U;
	}
	{
		uint32_t r1(decode_registration_n_2letter(i, e));
		if (r1 != std::numeric_limits<uint32_t>::max())
			return r + r1;
	}
	r += 601;
	// fourth digit
	{
		char ch(*i++);
		if (ch == 'I' || ch == 'i')
			ch = '1';
		else if (ch == 'O' || ch == 'o')
			ch = '0';
		if (ch < '0' || ch > '9')
			return std::numeric_limits<uint32_t>::max();
		r += (ch - '0') * 35U;
	}
	{
		uint32_t r1(decode_registration_n_1letter(i, e));
		if (r1 != std::numeric_limits<uint32_t>::max())
			return r + r1;
	}
	return std::numeric_limits<uint32_t>::max();
}

uint32_t ModeSMessage::decode_registration_n_2letter(std::string::const_iterator i, std::string::const_iterator e)
{
	if (i == e)
		return 0;
	char ch1(*i);
	if (ch1 >= 'a' && ch1 <= 'z')
		ch1 -= 'a' - 'A';
	if (ch1 < 'A' || ch1 > 'Z' || ch1 == 'I' || ch1 == 'O')
		return std::numeric_limits<uint32_t>::max();
	if (ch1 >= 'P')
		--ch1;
	if (ch1 >= 'J')
		--ch1;
	++i;
	if (i == e)
		return 1U + 25U * (ch1 - 'A');
	char ch2(*i);
	if (ch2 >= 'a' && ch2 <= 'z')
		ch2 -= 'a' - 'A';
	if (ch2 < 'A' || ch2 > 'Z' || ch2 == 'I' || ch2 == 'O')
		return std::numeric_limits<uint32_t>::max();
	if (ch2 >= 'P')
		--ch2;
	if (ch2 >= 'J')
		--ch2;
	++i;
	if (i != e)
		return std::numeric_limits<uint32_t>::max();
	return 1U + 25U * (ch1 - 'A') + (ch2 - ('A' - 1));
}

uint32_t ModeSMessage::decode_registration_n_1letter(std::string::const_iterator i, std::string::const_iterator e)
{
	if (i == e)
		return 0;
	char ch(*i);
	++i;
	if (i != e)
		return std::numeric_limits<uint32_t>::max();
	if (ch >= 'a' && ch <= 'z')
		ch -= 'a' - 'A';
	if (ch == 'I')
		ch = '1';
	else if (ch == 'O')
		ch = '0';
	if (ch >= '0' && ch <= '9')
		return ch - ('0' - 25);
	if (ch < 'A' || ch > 'Z')
		return std::numeric_limits<uint32_t>::max();
	if (ch >= 'P')
		--ch;
	if (ch >= 'J')
		--ch;
	return ch - ('A' - 1);
}

uint32_t ModeSMessage::decode_registration_26(std::string::const_iterator i, std::string::const_iterator e, uint32_t baseaddr)
{
	unsigned int nr(0), len(0);
	for (; i != e; ++i, ++len) {
		char ch(*i);
		if (ch >= 'A' && ch <= 'Z')
			ch -= 'A';
		else if (ch >= 'a' && ch <= 'z')
			ch -= 'a';
		else
			return std::numeric_limits<uint32_t>::max();
		nr = nr * 26 + ch;
	}
	if (len != 3)
		return std::numeric_limits<uint32_t>::max();
	return baseaddr + nr;
}

uint32_t ModeSMessage::decode_registration_hb(std::string::const_iterator i, std::string::const_iterator e)
{
	if (i == e)
		return std::numeric_limits<uint32_t>::max();
	// glider / motorglider
	if (std::isdigit(*i)) {
		unsigned int nr(0);
		for (; i != e; ++i) {
			if (!isdigit(*i))
				return std::numeric_limits<uint32_t>::max();
			nr = nr * 10 + *i - '0';
			if (nr > 0x7fff - 26*26*26 + 1)
				return std::numeric_limits<uint32_t>::max();
		}
		if (!nr)
			return std::numeric_limits<uint32_t>::max();
		return 0x4b0000 + 26 * 26 * 26 - 1 + nr;
	}
	return decode_registration_26(i, e, 0x4b0000);
}

uint32_t ModeSMessage::decode_registration_oh(std::string::const_iterator i, std::string::const_iterator e)
{
	return decode_registration_26(i, e, 0x460000);
}

uint32_t ModeSMessage::decode_registration_5bit(std::string::const_iterator i, std::string::const_iterator e, uint32_t baseaddr, uint8_t choffs)
{
	if (i == e)
		return std::numeric_limits<uint32_t>::max();
	unsigned int nr(0), len(0);
	for (; i != e; ++i, ++len) {
		char ch(*i);
		if (ch >= 'A' && ch <= 'Z')
			ch -= 'A';
		else if (ch >= 'a' && ch <= 'z')
			ch -= 'a';
		else
			return std::numeric_limits<uint32_t>::max();
		nr = (nr << 5) + ((ch + choffs) & 0x1f);
	}
	if (len != 3)
		return std::numeric_limits<uint32_t>::max();
	return baseaddr + nr;
}

uint32_t ModeSMessage::decode_registration_tc(std::string::const_iterator i, std::string::const_iterator e)
{
	return decode_registration_5bit(i, e, 0x4b8000, 1);
}

uint32_t ModeSMessage::decode_registration_oy(std::string::const_iterator i, std::string::const_iterator e)
{
	return decode_registration_5bit(i, e, 0x458000, 1);
}

uint32_t ModeSMessage::decode_registration_yu(std::string::const_iterator i, std::string::const_iterator e)
{
	return decode_registration_5bit(i, e, 0x4c0000, 0);
}

uint32_t ModeSMessage::decode_registration_oo(std::string::const_iterator i, std::string::const_iterator e)
{
	return decode_registration_5bit(i, e, 0x448000, 1);
}
