//
// C++ Implementation: wmmconv
//
// Description: 
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <sqlite3.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <cmath>
#include <cctype>
#include <cstring>
#include <limits>

class WMMConverter {
        public:
                WMMConverter(void);
                void parse_cof(const std::string& fn);
                std::ostream& print(std::ostream& os) const;

        private:
                static const unsigned int maxord = 12;

                // file read data
                float epoch;
                std::string name;
                std::string cofdate;
                float c[maxord+1][maxord+1];
                float cd[maxord+1][maxord+1];
                // precomputed
                float snorm[(maxord+1) * (maxord+1)];
                float fm[maxord+1];
                float fn[maxord+1];
                float k[maxord+1][maxord+1];
                std::string clsname;

                void precompute(void);
                void print(std::ostream& os, const std::string& symname, float val) const;
                void print(std::ostream& os, const std::string& symname, const std::string& val) const;
                void print(std::ostream& os, const std::string& symname, const float *val, unsigned int dim1, unsigned int dim2 = 1) const;
                template <int i> void print(std::ostream& os, const std::string& symname, const float val[i]) const { print(os, symname, &val[0], i); }
                template <int i, int j> void print(std::ostream& os, const std::string& symname, const float val[i][j]) const { print(os, symname, &val[0][0], i, j); }
};

std::ostream& operator<<(std::ostream& os, const WMMConverter& wmm)
{
        return wmm.print(os);
}

WMMConverter::WMMConverter(void)
{
        memset(c, 0, sizeof(c));
        memset(cd, 0, sizeof(cd));
}

std::string getline(std::istream& is)
{
        std::string line;
	std::getline(is, line, '\n');
        if (!line.empty() && line[line.size()-1] == '\r')
                line.resize(line.size()-1);
        return line;
}

void WMMConverter::parse_cof(const std::string& fn)
{
        for (unsigned int i = 0; i <= maxord; i++)
                for (unsigned int j = 0; j <= maxord; j++)
                        c[i][j] = cd[i][j] = (j > i || !i && !j) ? 0.0f : std::numeric_limits<float>::quiet_NaN();
        std::ifstream ifs(fn.c_str());
        if (!ifs.good())
                throw std::runtime_error("WMMConverter::parse_cof: Cannot open file " + fn);
        {
                std::string line(getline(ifs));
                if (!ifs.good())
                        throw std::runtime_error("WMMConverter::parse_cof: Cannot read line");
                std::istringstream iss(line.c_str());
                iss >> epoch;
                if (!ifs.good())
                        throw std::runtime_error("WMMConverter::parse_cof: cannot extract epoch from " + line);
                iss >> name;
                if (!ifs.good())
                        throw std::runtime_error("WMMConverter::parse_cof: cannot extract name from " + line);
                iss >> cofdate;
                if (!ifs.good())
                        throw std::runtime_error("WMMConverter::parse_cof: cannot extract COF date from " + line);
        }
        while (!ifs.eof()) {
                std::string line(getline(ifs));
                if (ifs.bad())
                        throw std::runtime_error("WMMConverter::parse_cof: read error in file  " + fn);
                if (!line.compare(4, 4, "9999"))
                        break;
                //std::cerr << "parsing line " << line << std::endl;
                std::istringstream iss(line.c_str());
                unsigned int i, j;
                iss >> i;
                iss >> j;
                float gnm, hnm, dgnm, dhnm;
                iss >> gnm;
                iss >> hnm;
                iss >> dgnm;
                iss >> dhnm;
                if (iss.bad())
                        throw std::runtime_error("WMMConverter::parse_cof: cannot parse line " + line);
                if (i > maxord || j > i)
                        throw std::runtime_error("WMMConverter::parse_cof: invalid coeff coordinates in line " + line);
                //std::cerr << "parsing line i=" << i << " j=" << j << " gnm=" << gnm << " dgnm=" << dgnm << " hnm=" << hnm << " dhnm=" << dhnm << std::endl;
                c[j][i] = gnm;
                cd[j][i] = dgnm;
                if (j > 0) {
                        c[i][j-1] = hnm;
                        cd[i][j-1] = dhnm;
                }
                //std::cerr << "c[" << i << "][" << j << "]=" << c[i][j] << std::endl;
                //std::cerr << "cd[" << i << "][" << j << "]=" << cd[i][j] << std::endl;
        }
        for (unsigned int i = 0; i <= maxord; i++)
                for (unsigned int j = 0; j <= maxord; j++) {
                        if (std::isnan(c[i][j])) {
                                std::cerr << "warning: c[" << i << "][" << j << "] not read, setting to 0" << std::endl;
                                c[i][j] = 0;
                        }
                        if (std::isnan(cd[i][j])) {
                                std::cerr << "warning: cd[" << i << "][" << j << "] not read, setting to 0" << std::endl;
                                cd[i][j] = 0;
                        }
                }
        //print<maxord+1,maxord+1>(std::cerr, "c", c);
        //print<maxord+1,maxord+1>(std::cerr, "cd", cd);
        precompute();
}

void WMMConverter::precompute( void )
{
        memset(snorm, 0, sizeof(snorm));
        memset(k, 0, sizeof(k));
        memset(fn, 0, sizeof(fn));
        memset(fm, 0, sizeof(fm));
        snorm[0] = 1.0;
        for (unsigned int n = 1; n <= maxord; n++) {
                snorm[n] = snorm[n-1] * ((float)(2*n-1)/(float)n);
                int j = 2;
                for (unsigned int m = 0, D1 = 1, D2=n + 1; D2 > 0; D2--, m += D1) {
                        k[m][n] = (float)(((n-1)*(n-1))-(m*m))/(float)((2*n-1)*(2*n-3));
                        if (m > 0) {
                                float flnmj = (float)((n-m+1)*j)/(float)(n+m);
                                snorm[n+m*(maxord+1)] = snorm[n+(m-1)*(maxord+1)]*sqrt(flnmj);
                                j = 1;
                                c[n][m-1] = snorm[n+m*(maxord+1)]*c[n][m-1];
                                cd[n][m-1] = snorm[n+m*(maxord+1)]*cd[n][m-1];
                        }
                        c[m][n] = snorm[n+m*(maxord+1)]*c[m][n];
                        cd[m][n] = snorm[n+m*(maxord+1)]*cd[m][n];
                }
                fn[n] = (float)(n+1);
                fm[n] = (float)n;
        }
        k[1][1] = 0.0;
        clsname = "";
        for (std::string::const_iterator i1(name.begin()), ie(name.end()); i1 != ie; i1++)
                if (std::isalnum(*i1))
                        clsname += *i1;
}

void WMMConverter::print( std::ostream& os, const std::string& symname, float val ) const
{
        os << "/* " << symname << " */ " << val;
}

void WMMConverter::print( std::ostream& os, const std::string& symname, const std::string & val ) const
{
        std::string s;
        for (std::string::const_iterator i1(val.begin()), ie(val.end()); i1 != ie; i1++) {
                if ((*i1) == '"')
                        s += '\\';
                s += *i1;
        }
        os << "/* " << symname << " */ \"" << s << "\"";
}

void WMMConverter::print( std::ostream& os, const std::string& symname, const float * val, unsigned int dim1, unsigned int dim2 ) const
{
        os << "/* " << symname << " */ {";
        if (dim2 <= 1) {
                for (unsigned int i = 0;;) {
                        if (!(i & 3))
                                os << std::endl << "                ";
                        os << val[i];
                        i++;
                        if (i >= dim1)
                                break;
                        os << ", ";
                }
                os << std::endl << "        }";
                return;
        }
        for (unsigned int i = 0;;) {
                os << std::endl << "                {";
                for (unsigned int j = 0;;) {
                        if (!(j & 3))
                                os << std::endl << "                        ";
                        os << val[i * dim2 + j];
                        j++;
                        if (j >= dim2)
                                break;
                        os << ", ";
                }
                os << std::endl << "                }";
                i++;
                if (i >= dim1)
                        break;
                os << ',';
        }
        os << std::endl << "        }";
}

std::ostream & WMMConverter::print( std::ostream & os ) const
{
        std::ios_base::fmtflags ff = os.flags();
        std::streamsize ss = os.precision();
        os << std::scientific << std::setprecision(10);
        os << "#include \"wmm.h\"" << std::endl << std::endl
                        << "const WMM::WMMModel WMM::" << clsname << " = {" << std::endl << "        ";
        print(os, "name", name);
        os << ',' << std::endl << "        ";
        print(os, "cofdate", cofdate);
        os << ',' << std::endl << "        ";
        print(os, "epoch", epoch);
        os << ',' << std::endl << "        ";
        print<maxord+1,maxord+1>(os, "c", c);
        os << ',';
        print<maxord+1,maxord+1>(os, "cd", cd);
        os << ',';
        print<(maxord+1) * (maxord+1)>(os, "snorm", snorm);
        os << ',';
        if (false) {
                print<maxord+1>(os, "fm", fm);
                os << ',';
                print<maxord+1>(os, "fn", fn);
                os << ',';
        }
        print<maxord+1,maxord+1>(os, "k", k);
        os << std::endl << "};" << std::endl;
        os.flags(ff);
        os.precision(ss);
        return os;
}

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
                { "import", optional_argument, 0, 'i' },
                { 0, 0, 0, 0 }
        };
        std::string filename;
        int mode(0), c, err(0);

        while ((c = getopt_long (argc, argv, "i:", long_options, 0)) != EOF) {
                switch (c) {
                        case 'i':
                                if (optarg)
                                        filename = optarg;
                                mode = 1;
                                break;

                        default:
                                err++;
                                break;
                }
        }
        if (err) {
                std::cerr << "usage: wmmconv -i <fn>" << std::endl;
                return 1;
        }
        switch (mode) {
                case 1:
                {
                        try {
                                WMMConverter conv;
                                conv.parse_cof(filename);
                                std::cout << conv;
                        } catch (const std::exception& ex) {
                                std::cerr << "exception: " << ex.what() << std::endl;
                        }
                        break;
                }

                default:
                {
                        std::cerr << "must select import or export" << std::endl;
                        return 3;
                }
        }
        return 0;
}
