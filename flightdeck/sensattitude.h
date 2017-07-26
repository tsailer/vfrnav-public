//
// C++ Interface: SensAttitude
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SENSATTITUDE_H
#define SENSATTITUDE_H

#include "sensors.h"

class Sensors::SensorAttitude : public SensorInstance {
  public:
	SensorAttitude(Sensors& sensors, const Glib::ustring& configgroup);
	virtual ~SensorAttitude();

  protected:
	enum {
		parnrstart = SensorInstance::parnrend,
		parnrend = parnrstart
	};
};

#endif /* SENSATTITUDE_H */
