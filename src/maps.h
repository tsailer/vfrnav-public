//
// C++ Interface: maps
//
// Description: Maps Drawing
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2008, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MAPS_H
#define MAPS_H

#include "sysdeps.h"

#ifdef HAVE_MULTITHREADED_MAPS
#include "mapst.h"
#else
#include "mapsnt.h"
#endif

#endif /* MAPS_H */
