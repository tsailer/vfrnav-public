//
// C++ Implementation: Attitude Sensor
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sensattitude.h"

Sensors::SensorAttitude::SensorAttitude(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorInstance(sensors, configgroup)
{
}

Sensors::SensorAttitude::~SensorAttitude()
{
}
