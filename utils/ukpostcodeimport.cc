//
// C++ Implementation: ukpostcodeimport
//
// Description: UK Postal Code importer (from Code-Point open)
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2015, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

//#include <unistd.h>
//#include <stdio.h>
#include <getopt.h>
//#include <iomanip>
#include <iostream>
#include <fstream>
//#include <sstream>
//#include <stdlib.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
//#include <stdarg.h>
#include <cmath>
#include <ogr_spatialref.h>
#include <sqlite3x.hpp>
#include <glibmm.h>
#include "geom.h"

#ifdef HAVE_PQXX
#include <pqxx/connection.hxx>
#include <pqxx/transactor.hxx>
#include <pqxx/result.hxx>
#include <pqxx/except.hxx>
#endif

class UKPostCodeImport {
public:
	UKPostCodeImport(void);
	~UKPostCodeImport();

	static std::vector<std::string> tokenize(const std::string& s, char delim = ',');
	int process_csv(const std::string& fn);
	void clear(void);
	void close(void);

protected:
	OGRSpatialReference m_srefosgb36;
	OGRSpatialReference m_srefwgs84;
	bool m_dbopen;

	virtual void db_start(void) = 0;
	virtual void db_end(void) = 0;
	virtual void db_delete(void) = 0;
	virtual void db_insert(const std::string& postcode, unsigned int quality, const Point& pt) = 0;
};


UKPostCodeImport::UKPostCodeImport(void)
	: m_dbopen(false)
{
	if (m_srefosgb36.importFromEPSG(27700) != OGRERR_NONE)
		throw std::runtime_error("Cannot set projection EPSG::27700 (OSGB36)");
	if (m_srefwgs84.importFromEPSG(4326) != OGRERR_NONE)
		throw std::runtime_error("Cannot set projection EPSG::4326 (WGS84)");
}

UKPostCodeImport::~UKPostCodeImport()
{
	close();
}

void UKPostCodeImport::close(void)
{
	if (m_dbopen)
		db_end();
	m_dbopen = false;
}

void UKPostCodeImport::clear(void)
{
	if (!m_dbopen) {
		db_start();
		m_dbopen = true;
	}
	db_delete();
}

std::vector<std::string> UKPostCodeImport::tokenize(const std::string& s, char delim)
{
	std::vector<std::string> r;
	for (std::string::const_iterator si(s.begin()), se(s.end()); si != se; ) {
		if (*si == delim) {
			++si;
			continue;
		}
		bool inquote(*si == '"');
		std::string::const_iterator si2(si);
		for (++si2; si2 != se; ++si2) {
			if (*si2 == '"') {
				inquote = !inquote;
				continue;
			}
			if (inquote)
				continue;
			if (*si2 == delim)
				break;
		}
		{
			std::string::const_iterator si3(si2);
			if (*si == '"') {
				++si;
				if (si != si3)
					--si3;
			}
			r.push_back(std::string(si, si3));
		}
		si = si2;
	}
	return r;
}

int UKPostCodeImport::process_csv(const std::string& fn)
{
	std::ifstream is(fn.c_str());
	if (!is.is_open()) {
		std::cerr << "cannot open input file " << fn << std::endl;
		return EX_DATAERR;
	}
	if (!m_dbopen) {
		db_start();
		m_dbopen = true;
	}
	OGRCoordinateTransformation *trans(OGRCreateCoordinateTransformation(&m_srefosgb36, &m_srefwgs84));
	while (is.good()) {
		std::string line;
		getline(is, line);
		std::vector<std::string> tok(tokenize(line));
		if (tok.size() < 4)
			continue;
		if (tok[0].empty())
			continue;
		double lat(0), lon(0);
		{
			const char *cp(tok[2].c_str());
			char *cp1(0);
			lon = strtod(cp, &cp1);
			if (std::isnan(lon) || cp1 == cp || *cp1)
				continue;
		}
		{
			const char *cp(tok[3].c_str());
			char *cp1(0);
			lat = strtod(cp, &cp1);
			if (std::isnan(lat) || cp1 == cp || *cp1)
				continue;
		}
		unsigned int quality(0);
		{
			const char *cp(tok[1].c_str());
			char *cp1(0);
			quality = strtoul(cp, &cp1, 10);
			if (cp1 == cp || *cp1)
				continue;
		}
		trans->Transform(1, &lon, &lat);
		Point pt;
		pt.set_lat_deg_dbl(lat);
		pt.set_lon_deg_dbl(lon);
		if (!false)
			std::cout << tok[0] << ',' << quality << ',' << lat << ',' << lon << ' ' << pt.get_lat_str2() << ' ' << pt.get_lon_str2() << std::endl;
		db_insert(tok[0], quality, pt);
	}
	OGRFree(trans);
	return 0;
}

class UKPostCodeImportSQLite : public UKPostCodeImport {
public:
	UKPostCodeImportSQLite(const std::string& path);

protected:
	sqlite3x::sqlite3_connection m_db;

	void db_start(void);
	void db_end(void);
	void db_delete(void);
	void db_insert(const std::string& postcode, unsigned int quality, const Point& pt);
};

UKPostCodeImportSQLite::UKPostCodeImportSQLite(const std::string& path)
{
	m_db.open(Glib::build_filename(path, "ukpostcodes.db"));
	{
		sqlite3x::sqlite3_command cmd(m_db, "CREATE TABLE IF NOT EXISTS ukpostcodes (POSTCODE TEXT PRIMARY KEY NOT NULL, "
					      "QUALITY INTEGER NOT NULL,"
					      "LAT INTEGER NOT NULL,"
					      "LON INTEGER NOT NULL);");
		cmd.executenonquery();
	}
	{ sqlite3x::sqlite3_command cmd(m_db, "CREATE INDEX IF NOT EXISTS ukpostcodes_latlon ON ukpostcodes(LAT,LON);"); cmd.executenonquery(); }
}

void UKPostCodeImportSQLite::db_start(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "PRAGMA journal_mode=WAL;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "BEGIN;");
		cmd.executenonquery();
	}
}

void UKPostCodeImportSQLite::db_end(void)
{
	{
		sqlite3x::sqlite3_command cmd(m_db, "COMMIT;");
		cmd.executenonquery();
	}
	{
		sqlite3x::sqlite3_command cmd(m_db, "PRAGMA journal_mode=DELETE;");
		cmd.executenonquery();
	}
}

void UKPostCodeImportSQLite::db_delete(void)
{
	sqlite3x::sqlite3_command cmd(m_db, "DELETE FROM ukpostcodes;");
	cmd.executenonquery();
}

void UKPostCodeImportSQLite::db_insert(const std::string& postcode, unsigned int quality, const Point& pt)
{
	sqlite3x::sqlite3_command cmd(m_db, "INSERT OR REPLACE INTO ukpostcodes (POSTCODE,QUALITY,LAT,LON) VALUES (?,?,?,?);");
	cmd.bind(1, postcode);
	cmd.bind(2, (int)quality);
	cmd.bind(3, pt.get_lat());
	cmd.bind(4, pt.get_lon());
	cmd.executenonquery();
}

#ifdef HAVE_PQXX

class UKPostCodeImportPG : public UKPostCodeImport {
public:
	UKPostCodeImportPG(const std::string& conn, const std::string& schema = "aviationdb");

protected:
	pqxx::connection m_conn;
	std::unique_ptr<pqxx::work> m_work;
	std::string m_schema;

	void db_start(void);
	void db_end(void);
	void db_delete(void);
	void db_insert(const std::string& postcode, unsigned int quality, const Point& pt);
};

UKPostCodeImportPG::UKPostCodeImportPG(const std::string& conn, const std::string& schema)
	: m_conn(conn), m_schema(schema)
{
	pqxx::work w(m_conn);
	w.exec("CREATE SCHEMA IF NOT EXISTS " + m_schema + ";");
	w.exec("CREATE TABLE IF NOT EXISTS " + m_schema + ".ukpostcodes ("
	       "POSTCODE TEXT PRIMARY KEY NOT NULL, "
	       "QUALITY INTEGER NOT NULL,"
	       "LAT INTEGER NOT NULL,"
	       "LON INTEGER NOT NULL"
	       ");");
	w.exec("CREATE INDEX IF NOT EXISTS ukpostcodes_latlon ON " + m_schema + ".ukpostcodes(LAT,LON);");
	w.commit();
}

void UKPostCodeImportPG::db_start(void)
{
	m_work.reset(new pqxx::work(m_conn, "UK Postcode DB Update"));
}

void UKPostCodeImportPG::db_end(void)
{
	if (!m_work)
		return;
	m_work->commit();
	m_work.reset();
}

void UKPostCodeImportPG::db_delete(void)
{
	m_work->exec("DELETE FROM " + m_schema + ".ukpostcodes;");
}

void UKPostCodeImportPG::db_insert(const std::string& postcode, unsigned int quality, const Point& pt)
{
	m_work->exec("INSERT INTO " + m_schema + ".ukpostcodes (POSTCODE,QUALITY,LAT,LON) VALUES (" +
		     m_work->quote(postcode) + "," + m_work->quote(quality) + "," +
		     m_work->quote(pt.get_lat()) + "," + m_work->quote(pt.get_lon()) +
		     ") ON CONFLICT (POSTCODE) DO UPDATE SET (QUALITY,LAT,LON) = (EXCLUDED.QUALITY,EXCLUDED.LAT,EXCLUDED.LON);");
}

#endif

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
		{ "directory", required_argument, 0, 'd' },
		{ "clear",     no_argument,       0, 'c' },
#ifdef HAVE_PQXX
		{ "pgsql",     required_argument, 0, 'p' },
		{ "schema",    required_argument, 0, 'S' },
#endif
		{ 0, 0, 0, 0 }
        };
        int c, err(0);
	bool pgsql(false), doclear(false);
	std::string dir("."), schema("aviationdb");

        while ((c = getopt_long(argc, argv, "d:cp:S:", long_options, 0)) != EOF) {
                switch (c) {
		case 'd':
			if (optarg) {
				dir = optarg;
				pgsql = false;
			}
			break;

		case 'c':
			doclear = true;
			break;

#ifdef HAVE_PQXX
		case 'p':
			if (optarg) {
				dir = optarg;
				pgsql = true;
			}
			break;

		case 'S':
			if (!optarg)
				break;
			schema = optarg;
			break;
#endif

 		default:
			err++;
			break;
                }
        }
        if (err) {
                std::cerr << "usage: vfrdbukpostcodeimport [-o <dir>] [<csv files>]" << std::endl;
                return EX_USAGE;
        }
	try {
		std::unique_ptr<UKPostCodeImport> imp;
#ifdef HAVE_PQXX
		if (pgsql)
			imp.reset(new UKPostCodeImportPG(dir, schema));
		else
#endif
			imp.reset(new UKPostCodeImportSQLite(dir));
		if (doclear)
			imp->clear();
		{
			int ret = 0;
			for (; optind < argc; ++optind) {
				int r = imp->process_csv(argv[optind]);
				if (!ret)
					ret = r;
			}
			imp->close();
			return ret;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EX_DATAERR;
	} catch (const Glib::Exception& e) {
		std::cerr << "Glib Error: " << e.what() << std::endl;
		return EX_DATAERR;
	}
}
