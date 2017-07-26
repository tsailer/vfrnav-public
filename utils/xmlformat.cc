//
// C++ Implementation: xmlformat
//
// Description: Simple XML Formatter
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"

#include <libxml++/libxml++.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdexcept>
#include <stdarg.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

int main(int argc, char *argv[])
{
        static struct option long_options[] = {
		{0, 0, 0, 0}
        };
        int c, err(0);

        while ((c = getopt_long(argc, argv, "", long_options, 0)) != EOF) {
                switch (c) {
 		default:
			err++;
			break;
                }
        }
        if (err || optind + 2 > argc) {
                std::cerr << "usage: xmlformat <input> <output>" << std::endl;
                return EX_USAGE;
        }
	if (false) {
		xmlpp::DomParser p(argv[optind]);
		if (!p)
			return EX_DATAERR;
		xmlpp::Document *doc(p.get_document());
		doc->write_to_file_formatted(argv[optind+1]);
		return EX_OK;
	}
	xmlDocPtr doc(xmlReadFile(argv[optind], NULL, XML_PARSE_NOBLANKS));
	if (doc == NULL)
		return EX_DATAERR;
	xmlKeepBlanksDefault(0);
	xmlIndentTreeOutput = 1;
	err = xmlSaveFormatFile(argv[optind+1], doc, 1);
	xmlFreeDoc(doc);
	return err == -1 ? EX_DATAERR : EX_OK;
}
