//
// C++ Implementation: Navigate
//
// Description: Navigation Mode Windows
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2011, 2012, 2016
//
// Copyright: See COPYING file that comes with this distribution
//
//

// MN - TN = VAR
// MN = TN + VAR
// (MT - MN) = (TT - TN) -> MT = TT - TN + MN = TT - VAR


#include "sysdeps.h"

#include "Navigate.h"
#include "RouteEditUi.h"
#include "wmm.h"
#include "baro.h"
#include "sysdepsgui.h"

#include <sstream>
#include <libintl.h>

#ifdef HAVE_LIBLOCATION
extern "C" {
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>
}
#endif

#ifdef HAVE_GYPSY
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-accuracy.h>
#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-course.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-satellite.h>
#include <gypsy/gypsy-time.h>
#endif

bool Navigate::GPSComm::SV::operator==(const SV& sv) const
{
        return m_prn == sv.m_prn && m_az == sv.m_az && m_elev == sv.m_elev && m_ss == sv.m_ss && m_used == sv.m_used;
}

bool Navigate::GPSComm::SVVector::operator==( const SVVector & v ) const
{
        if (size() != v.size())
                return false;
        for (size_type i = 0; i < size(); i++)
                if (operator[](i) != v[i])
                        return false;
        return true;
}

Navigate::GPSComm::GPSComm(void)
{
}

Navigate::GPSComm::~GPSComm( )
{
}

Navigate::GPSComm::fixtype_t Navigate::GPSComm::get_fixtype(void) const
{
        return fixtype_noconn;
}

Navigate::GPSComm::fixstatus_t Navigate::GPSComm::get_fixstatus(void) const
{
        return fixstatus_nofix;
}

double Navigate::GPSComm::get_fixtime(void) const
{
        return 0;
}

Point Navigate::GPSComm::get_coord(void) const
{
        return Point();
}

float Navigate::GPSComm::get_coord_uncertainty(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

bool Navigate::GPSComm::is_coord_valid(void) const
{
        return (get_fixtype() >= fixtype_2d) && (get_fixstatus() >= fixstatus_fix);
}

float Navigate::GPSComm::get_altitude(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_altitude_uncertainty(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

bool Navigate::GPSComm::is_altitude_valid(void) const
{
        return (get_fixtype() >= fixtype_3d) && (get_fixstatus() >= fixstatus_fix);
}

float Navigate::GPSComm::get_track(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_track_uncertainty(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_velocity(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_velocity_uncertainty(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_verticalvelocity(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_verticalvelocity_uncertainty(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_pdop(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_hdop(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_vdop(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_tdop(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

float Navigate::GPSComm::get_gdop(void) const
{
        return std::numeric_limits<float>::quiet_NaN();
}

unsigned int Navigate::GPSComm::get_nrsv(void) const
{
        return 0;
}

Navigate::GPSComm::SV Navigate::GPSComm::get_sv( unsigned int idx ) const
{
        return SV();
}

Navigate::GPSComm::SVVector Navigate::GPSComm::get_svvector(void) const
{
        return SVVector();
}

#if defined(HAVE_LIBLOCATION)

class Navigate::GPSCommGPSD : public Navigate::GPSComm {
        private:
                GPSCommGPSD(void);
                virtual ~GPSCommGPSD();

        public:
                static Navigate::GPSCommGPSD *create(const Glib::ustring& server = "localhost", const Glib::ustring& port = "2947");
                virtual fixtype_t get_fixtype(void) const;
                virtual fixstatus_t get_fixstatus(void) const;
                virtual double get_fixtime(void) const;
                virtual Point get_coord(void) const;
                virtual float get_coord_uncertainty(void) const;
                virtual bool is_coord_valid(void) const;
                virtual float get_altitude(void) const;
                virtual float get_altitude_uncertainty(void) const;
                virtual bool is_altitude_valid(void) const;
                virtual float get_track(void) const;
                virtual float get_track_uncertainty(void) const;
                virtual float get_velocity(void) const;
                virtual float get_velocity_uncertainty(void) const;
                virtual float get_verticalvelocity(void) const;
                virtual float get_verticalvelocity_uncertainty(void) const;
                virtual float get_pdop(void) const;
                virtual float get_hdop(void) const;
                virtual float get_vdop(void) const;
                virtual float get_tdop(void) const;
                virtual float get_gdop(void) const;
                virtual unsigned int get_nrsv(void) const;
                virtual SV get_sv(unsigned int idx) const;
                virtual SVVector get_svvector(void) const;

        private:
                LocationGPSDControl *m_control;
                LocationGPSDevice *m_device;
                gulong m_sighandler;

                static void location_changed(LocationGPSDevice *device, gpointer userdata);
                void update_gps(void);
};

Navigate::GPSCommGPSD *Navigate::GPSCommGPSD::create( const Glib::ustring & server, const Glib::ustring & port )
{
        return new Navigate::GPSCommGPSD();
}

Navigate::GPSCommGPSD::GPSCommGPSD(void)
        : m_device(0), m_sighandler(0)
{
        m_control = (LocationGPSDControl *)g_object_new (LOCATION_TYPE_GPSD_CONTROL, NULL);
        location_gpsd_control_start(m_control);
        m_device = (LocationGPSDevice *)g_object_new (LOCATION_TYPE_GPS_DEVICE, NULL);
        m_sighandler = g_signal_connect(m_device, "changed", G_CALLBACK (Navigate::GPSCommGPSD::location_changed), this);
        location_gps_device_start(m_device);
}

Navigate::GPSCommGPSD::~GPSCommGPSD( )
{
        location_gps_device_stop(m_device);
        g_signal_handler_disconnect(m_device, m_sighandler);
        g_object_unref(m_device);
        m_device = 0;
        m_sighandler = 0;
        location_gpsd_control_stop(m_control);
        g_object_unref(m_control);
        m_control = 0;
}

void Navigate::GPSCommGPSD::location_changed(LocationGPSDevice *device, gpointer userdata)
{
        Navigate::GPSCommGPSD *obj((Navigate::GPSCommGPSD *)userdata);
        obj->update_gps();
}

void Navigate::GPSCommGPSD::update_gps(void)
{
        m_update();
}

Navigate::GPSCommGPSD::fixtype_t Navigate::GPSCommGPSD::get_fixtype(void) const
{
        switch (m_device->fix->mode) {
                default:
                case LOCATION_GPS_DEVICE_MODE_NOT_SEEN:
                        return fixtype_noconn;

                case LOCATION_GPS_DEVICE_MODE_NO_FIX:
                        return fixtype_nofix;

                case LOCATION_GPS_DEVICE_MODE_2D:
                        return fixtype_2d;

                case LOCATION_GPS_DEVICE_MODE_3D:
                        return fixtype_3d;
        }
}

Navigate::GPSCommGPSD::fixstatus_t Navigate::GPSCommGPSD::get_fixstatus(void) const
{
        switch (m_device->status) {
                default:
                case LOCATION_GPS_DEVICE_STATUS_NO_FIX:
                        return fixstatus_nofix;

                case LOCATION_GPS_DEVICE_STATUS_FIX:
                        return fixstatus_fix;

                case LOCATION_GPS_DEVICE_STATUS_DGPS_FIX:
                        return fixstatus_dgpsfix;
        }
}

double Navigate::GPSCommGPSD::get_fixtime(void) const
{
        return m_device->fix->time;
}

Point Navigate::GPSCommGPSD::get_coord(void) const
{
        return Point((int32_t)(m_device->fix->longitude * Point::from_deg),
                     (int32_t)(m_device->fix->latitude * Point::from_deg));
}

float Navigate::GPSCommGPSD::get_coord_uncertainty(void) const
{
        return m_device->fix->ept;
}

bool Navigate::GPSCommGPSD::is_coord_valid(void) const
{
        return Navigate::GPSComm::is_coord_valid() && (!std::isnan(m_device->fix->longitude)) && (!std::isnan(m_device->fix->latitude));
}

float Navigate::GPSCommGPSD::get_altitude(void) const
{
        return m_device->fix->altitude;
}

float Navigate::GPSCommGPSD::get_altitude_uncertainty(void) const
{
        return m_device->fix->epv;
}

bool Navigate::GPSCommGPSD::is_altitude_valid(void) const
{
        return Navigate::GPSComm::is_altitude_valid() && !std::isnan(m_device->fix->altitude);
}

float Navigate::GPSCommGPSD::get_track(void) const
{
        return m_device->fix->track;
}

float Navigate::GPSCommGPSD::get_track_uncertainty(void) const
{
        return m_device->fix->epd;
}

float Navigate::GPSCommGPSD::get_velocity(void) const
{
        return m_device->fix->speed;
}

float Navigate::GPSCommGPSD::get_velocity_uncertainty(void) const
{
        return m_device->fix->eps;
}

float Navigate::GPSCommGPSD::get_verticalvelocity(void) const
{
        return m_device->fix->climb;
}

float Navigate::GPSCommGPSD::get_verticalvelocity_uncertainty(void) const
{
        return m_device->fix->epc;
}

float Navigate::GPSCommGPSD::get_pdop(void) const
{
        return m_device->fix->eph;
}

float Navigate::GPSCommGPSD::get_hdop(void) const
{
        return m_device->fix->eph;
}

float Navigate::GPSCommGPSD::get_vdop(void) const
{
        return m_device->fix->epv;
}

float Navigate::GPSCommGPSD::get_tdop(void) const
{
        return m_device->fix->ept;
}

float Navigate::GPSCommGPSD::get_gdop(void) const
{
        return m_device->fix->eph;
}

unsigned int Navigate::GPSCommGPSD::get_nrsv(void) const
{
        return m_device->satellites_in_view;
}

Navigate::GPSCommGPSD::SV Navigate::GPSCommGPSD::get_sv( unsigned int idx ) const
{
        if (idx >= (unsigned int)m_device->satellites_in_view)
                return SV();
        LocationGPSDeviceSatellite *sat((LocationGPSDeviceSatellite *)g_ptr_array_index(m_device->satellites, idx));
        return SV(sat->prn, sat->azimuth, sat->elevation, sat->signal_strength, !!sat->in_use);
}

Navigate::GPSCommGPSD::SVVector Navigate::GPSCommGPSD::get_svvector(void) const
{
        SVVector v;
        for (int i = 0; i < m_device->satellites_in_view; i++) {
                LocationGPSDeviceSatellite *sat((LocationGPSDeviceSatellite *)g_ptr_array_index(m_device->satellites, i));
                v.push_back(SV(sat->prn, sat->azimuth, sat->elevation, sat->signal_strength, !!sat->in_use));
        }
        return v;
}

#elif defined(HAVE_GYPSY)

class Navigate::GPSCommGPSD : public Navigate::GPSComm {
        private:
                GPSCommGPSD(const Glib::ustring& port);
                virtual ~GPSCommGPSD();

        public:
                static Navigate::GPSCommGPSD *create(const Glib::ustring& server = "", const Glib::ustring& port = "/dev/ttyUSB0");
                virtual fixtype_t get_fixtype(void) const;
                virtual fixstatus_t get_fixstatus(void) const;
                virtual double get_fixtime(void) const;
                virtual Point get_coord(void) const;
                virtual float get_coord_uncertainty(void) const;
                virtual bool is_coord_valid(void) const;
                virtual float get_altitude(void) const;
                virtual float get_altitude_uncertainty(void) const;
                virtual bool is_altitude_valid(void) const;
                virtual float get_track(void) const;
                virtual float get_track_uncertainty(void) const;
                virtual float get_velocity(void) const;
                virtual float get_velocity_uncertainty(void) const;
                virtual float get_verticalvelocity(void) const;
                virtual float get_verticalvelocity_uncertainty(void) const;
                virtual float get_pdop(void) const;
                virtual float get_hdop(void) const;
                virtual float get_vdop(void) const;
                virtual float get_tdop(void) const;
                virtual float get_gdop(void) const;
                virtual unsigned int get_nrsv(void) const;
                virtual SV get_sv(unsigned int idx) const;
                virtual SVVector get_svvector(void) const;

        private:
                GypsyControl *m_control;
                char *m_path;
                GypsyDevice *m_device;
                GypsyPosition *m_position;
                gulong m_position_sighandler;
                GypsyCourse *m_course;
                gulong m_course_sighandler;
                GypsyAccuracy *m_accuracy;
                gulong m_accuracy_sighandler;
                GypsySatellite *m_satellite;
                gulong m_satellite_sighandler;

                static void position_changed(GypsyPosition *position, GypsyPositionFields fields_set, int timestamp,
                                             double latitude, double longitude, double altitude, gpointer userdata);
                static void course_changed(GypsyCourse *course, GypsyCourseFields fields, int timestamp,
                                           double speed, double direction, double climb, gpointer userdata);
                static void accuracy_changed(GypsyAccuracy *accuracy, GypsyAccuracyFields fields,
                                             double position, double horizontal, double vertical, gpointer userdata);
                static void satellite_changed(GypsySatellite *satellite, GPtrArray *satellites, gpointer userdata);

                void update_gps(void);
};

Navigate::GPSCommGPSD *Navigate::GPSCommGPSD::create( const Glib::ustring & server, const Glib::ustring & port )
{
        try {
                return new Navigate::GPSCommGPSD(port);
        } catch (std::exception& e) {
                std::cerr << "GPS: cannot create device: " << e.what() << std::endl;
                return 0;
        }
}

Navigate::GPSCommGPSD::GPSCommGPSD(const Glib::ustring& port)
        : m_control(0), m_device(0), m_position(0), m_position_sighandler(0), m_course(0), m_course_sighandler(0),
          m_accuracy(0), m_accuracy_sighandler(0), m_satellite(0), m_satellite_sighandler(0)
{
        GError *error = NULL;
        char *path;
        m_control = gypsy_control_get_default();

        // Tell gypsy-daemon to create a GPS object 
        // The GPS to use is passed in as a command line argument
        path = gypsy_control_create(m_control, port.c_str(), &error);
        if (!path) {
                std::ostringstream oss;
                oss << "GPS: cannot create client for " << port << ": " << error->message;
                g_error_free(error);
                throw std::runtime_error(oss.str());
        }
        m_position = gypsy_position_new(path);
        m_position_sighandler = g_signal_connect(m_position, "position-changed", G_CALLBACK(Navigate::GPSCommGPSD::position_changed), this);
        m_course = gypsy_course_new(path);
        m_course_sighandler = g_signal_connect(m_course, "course-changed", G_CALLBACK(Navigate::GPSCommGPSD::course_changed), this);
        m_accuracy = gypsy_accuracy_new(path);
        m_accuracy_sighandler = g_signal_connect(m_accuracy, "accuracy-changed", G_CALLBACK(Navigate::GPSCommGPSD::accuracy_changed), this);
        m_satellite = gypsy_satellite_new(path);
        m_satellite_sighandler = g_signal_connect(m_satellite, "satellites-changed", G_CALLBACK(Navigate::GPSCommGPSD::satellite_changed), this);
        m_device = gypsy_device_new(path);
        gypsy_device_start(m_device, &error);
        if (error != NULL) {
                std::ostringstream oss;
                oss << "GPS: cannot start device " << port << ": " << error->message;
                g_error_free(error);
                throw std::runtime_error(oss.str());
        }
}

Navigate::GPSCommGPSD::~GPSCommGPSD()
{
        if (m_position) {
                g_signal_handler_disconnect(m_position, m_position_sighandler);
                g_object_unref(m_position);
                m_position = 0;
                m_position_sighandler = 0;
        }
        if (m_course) {
                g_signal_handler_disconnect(m_course, m_course_sighandler);
                g_object_unref(m_course);
                m_course = 0;
                m_course_sighandler = 0;
        }
        if (m_accuracy) {
                g_signal_handler_disconnect(m_accuracy, m_accuracy_sighandler);
                g_object_unref(m_accuracy);
                m_accuracy = 0;
                m_accuracy_sighandler = 0;
        }
        if (m_satellite) {
                g_signal_handler_disconnect(m_satellite, m_satellite_sighandler);
                g_object_unref(m_satellite);
                m_satellite = 0;
                m_satellite_sighandler = 0;
        }
        if (m_device) {
                g_object_unref(m_device);
                m_device = 0;
        }
        m_control = 0;
}

void Navigate::GPSCommGPSD::position_changed(GypsyPosition *position, GypsyPositionFields fields_set, int timestamp,
                                             double latitude, double longitude, double altitude, gpointer userdata)
{
        Navigate::GPSCommGPSD *obj((Navigate::GPSCommGPSD *)userdata);
        obj->update_gps();
        if (true) {
                std::cerr << "GPS: position:time " << timestamp << " lat ";
                if (fields_set & GYPSY_POSITION_FIELDS_LATITUDE)
                        std::cerr << latitude;
                else
                        std::cerr << '?';
                std::cerr << " lon ";
                if (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE)
                        std::cerr << longitude;
                else
                        std::cerr << '?';
                std::cerr << " alt ";
                if (fields_set & GYPSY_POSITION_FIELDS_ALTITUDE)
                        std::cerr << altitude;
                else
                        std::cerr << '?';
                std::cerr << std::endl;
        }
}

void Navigate::GPSCommGPSD::course_changed(GypsyCourse *course, GypsyCourseFields fields, int timestamp,
                                           double speed, double direction, double climb, gpointer userdata)
{
        Navigate::GPSCommGPSD *obj((Navigate::GPSCommGPSD *)userdata);
        obj->update_gps();
}

void Navigate::GPSCommGPSD::accuracy_changed(GypsyAccuracy *accuracy, GypsyAccuracyFields fields,
                                             double position, double horizontal, double vertical, gpointer userdata)
{
        Navigate::GPSCommGPSD *obj((Navigate::GPSCommGPSD *)userdata);
        obj->update_gps();
}

void Navigate::GPSCommGPSD::satellite_changed(GypsySatellite *satellite, GPtrArray *satellites, gpointer userdata)
{
        Navigate::GPSCommGPSD *obj((Navigate::GPSCommGPSD *)userdata);
        obj->update_gps();
}

void Navigate::GPSCommGPSD::update_gps(void)
{
        m_update();
}

Navigate::GPSCommGPSD::fixtype_t Navigate::GPSCommGPSD::get_fixtype(void) const
{
        if (!m_device)
                return fixtype_noconn;
        GError *error = 0;
        GypsyDeviceFixStatus st = gypsy_device_get_fix_status(m_device, &error);
        if (error) {
                std::cerr << "gypsy_device_get_fix_status: error: " << error->message << std::endl;
                g_error_free(error);
                return fixtype_noconn;
        }
        switch (st) {
                default:
                case GYPSY_DEVICE_FIX_STATUS_INVALID:
                        return fixtype_noconn;

                case GYPSY_DEVICE_FIX_STATUS_NONE:
                        return fixtype_nofix;

                case GYPSY_DEVICE_FIX_STATUS_2D:
                        return fixtype_2d;

                case GYPSY_DEVICE_FIX_STATUS_3D:
                        return fixtype_3d;
        }
}

Navigate::GPSCommGPSD::fixstatus_t Navigate::GPSCommGPSD::get_fixstatus(void) const
{
        if (!m_device)
                return fixstatus_nofix;
        GError *error = 0;
        GypsyDeviceFixStatus st = gypsy_device_get_fix_status(m_device, &error);
        if (error) {
                std::cerr << "gypsy_device_get_fix_status: error: " << error->message << std::endl;
                g_error_free(error);
                return fixstatus_nofix;
        }
        switch (st) {
                default:
                case GYPSY_DEVICE_FIX_STATUS_INVALID:
                case GYPSY_DEVICE_FIX_STATUS_NONE:
                        return fixstatus_nofix;

                case GYPSY_DEVICE_FIX_STATUS_2D:
                case GYPSY_DEVICE_FIX_STATUS_3D:
                        return fixstatus_fix;
        }
}

double Navigate::GPSCommGPSD::get_fixtime(void) const
{
        if (!m_position)
                return 0;
        int timestamp;
        GError *error = 0;
        GypsyPositionFields flds = gypsy_position_get_position(m_position, &timestamp, 0, 0, 0, &error);
        if (error) {
                std::cerr << "gypsy_position_get_position: error: " << error->message << std::endl;
                g_error_free(error);
                return 0;
        }
        return timestamp;
}

Point Navigate::GPSCommGPSD::get_coord(void) const
{
        if (!m_position)
                return Point();
        double lat, lon;
        GError *error = 0;
        GypsyPositionFields flds = gypsy_position_get_position(m_position, 0, &lat, &lon, 0, &error);
        if (error) {
                std::cerr << "gypsy_position_get_position: error: " << error->message << std::endl;
                g_error_free(error);
                return Point();
        }
        if (!(flds & GYPSY_POSITION_FIELDS_LATITUDE))
                lat = 0;
        if (!(flds & GYPSY_POSITION_FIELDS_LONGITUDE))
                lon = 0;
        return Point((int32_t)(lon * Point::from_deg),
                     (int32_t)(lat * Point::from_deg));
}

float Navigate::GPSCommGPSD::get_coord_uncertainty(void) const
{
        if (!m_accuracy)
                return std::numeric_limits<float>::quiet_NaN();
        GError *error = 0;
        double hdop;
        GypsyAccuracyFields fld = gypsy_accuracy_get_accuracy(m_accuracy, 0, &hdop, 0, &error);
        if (error) {
                std::cerr << "gypsy_accuracy_get_accuracy: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(fld & GYPSY_ACCURACY_FIELDS_HORIZONTAL))
                return std::numeric_limits<float>::quiet_NaN();
        return hdop;
}

bool Navigate::GPSCommGPSD::is_coord_valid(void) const
{
        if (!m_position)
                return false;
        GError *error = 0;
        GypsyPositionFields flds = gypsy_position_get_position(m_position, 0, 0, 0, 0, &error);
        if (error) {
                std::cerr << "gypsy_position_get_position: error: " << error->message << std::endl;
                g_error_free(error);
                return false;
        }
        return Navigate::GPSComm::is_coord_valid() && (flds & GYPSY_POSITION_FIELDS_LATITUDE) && (flds & GYPSY_POSITION_FIELDS_LONGITUDE);
}

float Navigate::GPSCommGPSD::get_altitude(void) const
{
        if (!m_position)
                return 0;
        double alt;
        GError *error = 0;
        GypsyPositionFields flds = gypsy_position_get_position(m_position, 0, 0, 0, &alt, &error);
        if (error) {
                std::cerr << "gypsy_position_get_position: error: " << error->message << std::endl;
                g_error_free(error);
                return 0;
        }
        if (!(flds & GYPSY_POSITION_FIELDS_ALTITUDE))
                alt = 0;
        return alt;
}

float Navigate::GPSCommGPSD::get_altitude_uncertainty(void) const
{
        if (!m_accuracy)
                return std::numeric_limits<float>::quiet_NaN();
        GError *error = 0;
        double vdop;
        GypsyAccuracyFields fld = gypsy_accuracy_get_accuracy(m_accuracy, 0, 0, &vdop, &error);
        if (error) {
                std::cerr << "gypsy_accuracy_get_accuracy: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(fld & GYPSY_ACCURACY_FIELDS_VERTICAL))
                return std::numeric_limits<float>::quiet_NaN();
        return vdop;
}

bool Navigate::GPSCommGPSD::is_altitude_valid(void) const
{
        if (!m_position)
                return false;
        GError *error = 0;
        GypsyPositionFields flds = gypsy_position_get_position(m_position, 0, 0, 0, 0, &error);
        if (error) {
                std::cerr << "gypsy_position_get_position: error: " << error->message << std::endl;
                g_error_free(error);
                return false;
        }
        return Navigate::GPSComm::is_altitude_valid() && (flds & GYPSY_POSITION_FIELDS_ALTITUDE);
}

float Navigate::GPSCommGPSD::get_track(void) const
{
        if (!m_course)
                return std::numeric_limits<float>::quiet_NaN();
        double direction;
        GError *error = 0;
        GypsyCourseFields flds = gypsy_course_get_course(m_course, 0, 0, &direction, 0, &error);
        if (error) {
                std::cerr << "gypsy_course_get_course: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(flds & GYPSY_COURSE_FIELDS_DIRECTION))
                return std::numeric_limits<float>::quiet_NaN();
        return direction;
}

float Navigate::GPSCommGPSD::get_track_uncertainty(void) const
{
        // no separate track uncertainty
        return get_hdop();
}

float Navigate::GPSCommGPSD::get_velocity(void) const
{
        if (!m_course)
                return std::numeric_limits<float>::quiet_NaN();
        double speed;
        GError *error = 0;
        GypsyCourseFields flds = gypsy_course_get_course(m_course, 0, &speed, 0, 0, &error);
        if (error) {
                std::cerr << "gypsy_course_get_course: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(flds & GYPSY_COURSE_FIELDS_SPEED))
                return std::numeric_limits<float>::quiet_NaN();
        return speed;
}

float Navigate::GPSCommGPSD::get_velocity_uncertainty(void) const
{
        // no separate velocity uncertainty
        return get_hdop();
}

float Navigate::GPSCommGPSD::get_verticalvelocity(void) const
{
        if (!m_course)
                return std::numeric_limits<float>::quiet_NaN();
        double climb;
        GError *error = 0;
        GypsyCourseFields flds = gypsy_course_get_course(m_course, 0, 0, 0, &climb, &error);
        if (error) {
                std::cerr << "gypsy_course_get_course: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(flds & GYPSY_COURSE_FIELDS_CLIMB))
                return std::numeric_limits<float>::quiet_NaN();
        return climb;
}

float Navigate::GPSCommGPSD::get_verticalvelocity_uncertainty(void) const
{
        // no separate vertical velocity uncertainty
        return get_vdop();
}

float Navigate::GPSCommGPSD::get_pdop(void) const
{
        if (!m_accuracy)
                return std::numeric_limits<float>::quiet_NaN();
        GError *error = 0;
        double pdop;
        GypsyAccuracyFields fld = gypsy_accuracy_get_accuracy(m_accuracy, &pdop, 0, 0, &error);
        if (error) {
                std::cerr << "gypsy_accuracy_get_accuracy: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(fld & GYPSY_ACCURACY_FIELDS_POSITION))
                return std::numeric_limits<float>::quiet_NaN();
        return pdop;
}

float Navigate::GPSCommGPSD::get_hdop(void) const
{
        if (!m_accuracy)
                return std::numeric_limits<float>::quiet_NaN();
        GError *error = 0;
        double hdop;
        GypsyAccuracyFields fld = gypsy_accuracy_get_accuracy(m_accuracy, 0, &hdop, 0, &error);
        if (error) {
                std::cerr << "gypsy_accuracy_get_accuracy: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(fld & GYPSY_ACCURACY_FIELDS_HORIZONTAL))
                return std::numeric_limits<float>::quiet_NaN();
        return hdop;
}

float Navigate::GPSCommGPSD::get_vdop(void) const
{
        if (!m_accuracy)
                return std::numeric_limits<float>::quiet_NaN();
        GError *error = 0;
        double vdop;
        GypsyAccuracyFields fld = gypsy_accuracy_get_accuracy(m_accuracy, 0, 0, &vdop, &error);
        if (error) {
                std::cerr << "gypsy_accuracy_get_accuracy: error: " << error->message << std::endl;
                g_error_free(error);
                return std::numeric_limits<float>::quiet_NaN();
        }
        if (!(fld & GYPSY_ACCURACY_FIELDS_VERTICAL))
                return std::numeric_limits<float>::quiet_NaN();
        return vdop;
}

float Navigate::GPSCommGPSD::get_tdop(void) const
{
        return 0;
}

float Navigate::GPSCommGPSD::get_gdop(void) const
{
        // no separate general dilution of precision
        return get_hdop();
}

unsigned int Navigate::GPSCommGPSD::get_nrsv(void) const
{
        if (!m_satellite)
                return 0;
        GError *error = 0;
        GPtrArray *satellites = gypsy_satellite_get_satellites(m_satellite, &error);
        if (!satellites) {
                std::cerr << "gypsy_satellite_get_satellites: error: " << error->message << std::endl;
                g_error_free(error);
                return 0;
        }
        unsigned int nr = satellites->len;
        gypsy_satellite_free_satellite_array(satellites);
        return nr;
}

Navigate::GPSCommGPSD::SV Navigate::GPSCommGPSD::get_sv(unsigned int idx) const
{
        SV sv;
        if (!m_satellite)
                return sv;
        GError *error = 0;
        GPtrArray *satellites = gypsy_satellite_get_satellites(m_satellite, &error);
        if (!satellites) {
                std::cerr << "gypsy_satellite_get_satellites: error: " << error->message << std::endl;
                g_error_free(error);
                return sv;
        }
        if (idx < satellites->len) {
                GypsySatelliteDetails *details = static_cast<GypsySatelliteDetails *>(satellites->pdata[idx]);
                sv = SV(details->satellite_id, details->azimuth, details->elevation, details->snr, !!(details->in_use));
        }
        gypsy_satellite_free_satellite_array(satellites);
        return sv;
}

Navigate::GPSCommGPSD::SVVector Navigate::GPSCommGPSD::get_svvector(void) const
{
        SVVector v;
        if (!m_satellite)
                return v;
        GError *error = 0;
        GPtrArray *satellites = gypsy_satellite_get_satellites(m_satellite, &error);
        if (!satellites) {
                std::cerr << "gypsy_satellite_get_satellites: error: " << error->message << std::endl;
                g_error_free(error);
                return v;
        }
        for (unsigned int i = 0; i < satellites->len; i++) {
                GypsySatelliteDetails *details = static_cast<GypsySatelliteDetails *>(satellites->pdata[i]);
                v.push_back(SV(details->satellite_id, details->azimuth, details->elevation, details->snr, !!(details->in_use)));
        }
        gypsy_satellite_free_satellite_array(satellites);
        return v;
}

#elif defined(HAVE_LIBGPS)

#if GPSD_API_MAJOR_VERSION >= 5
#define gps_open_r gps_open
#endif

class Navigate::GPSCommGPSD : public Navigate::GPSComm {
        private:
                GPSCommGPSD(struct gps_data_t& gd);
                virtual ~GPSCommGPSD();

        public:
                static Navigate::GPSCommGPSD *create(const Glib::ustring& server = "localhost", const Glib::ustring& port = "2947");
                virtual fixtype_t get_fixtype(void) const;
                virtual fixstatus_t get_fixstatus(void) const;
                virtual double get_fixtime(void) const;
                virtual Point get_coord(void) const;
                virtual float get_coord_uncertainty(void) const;
                virtual bool is_coord_valid(void) const;
                virtual float get_altitude(void) const;
                virtual float get_altitude_uncertainty(void) const;
                virtual bool is_altitude_valid(void) const;
                virtual float get_track(void) const;
                virtual float get_track_uncertainty(void) const;
                virtual float get_velocity(void) const;
                virtual float get_velocity_uncertainty(void) const;
                virtual float get_verticalvelocity(void) const;
                virtual float get_verticalvelocity_uncertainty(void) const;
                virtual float get_pdop(void) const;
                virtual float get_hdop(void) const;
                virtual float get_vdop(void) const;
                virtual float get_tdop(void) const;
                virtual float get_gdop(void) const;
                virtual unsigned int get_nrsv(void) const;
                virtual SV get_sv(unsigned int idx) const;
                virtual SVVector get_svvector(void) const;

        private:
                struct gps_data_t m_gpsdata;
                sigc::connection m_gpssigioconn;
#if GPSD_API_MAJOR_VERSION < 5
                typedef std::map<struct gps_data_t *, GPSCommGPSD *> gpsmap_t;
                static gpsmap_t m_gpsmap;

                static void gps_hook(struct gps_data_t *, char *, size_t, int);
#endif
                bool gps_poll(Glib::IOCondition iocond);
                void update_gps(void);
                void gps_send(const std::string& s);
};

#if GPSD_API_MAJOR_VERSION < 5
Navigate::GPSCommGPSD::gpsmap_t Navigate::GPSCommGPSD::m_gpsmap;
#endif

Navigate::GPSCommGPSD *Navigate::GPSCommGPSD::create( const Glib::ustring & server, const Glib::ustring & port )
{
	struct gps_data_t gpsdata;
	if (gps_open_r(server.c_str(), port.c_str(), &gpsdata)) {
                std::cerr << "open_gps: error opening GPS" << std::endl;
                return 0;
        }
        Navigate::GPSCommGPSD *ngps = new Navigate::GPSCommGPSD(gpsdata);
        return ngps;
}

Navigate::GPSCommGPSD::GPSCommGPSD(struct gps_data_t& gd)
        : m_gpsdata(gd)
{
#if GPSD_API_MAJOR_VERSION < 5
        m_gpsmap[&m_gpsdata] = this;
        gps_set_raw_hook(m_gpsdata, gps_hook);
#endif
        m_gpssigioconn = Glib::signal_io().connect(sigc::mem_fun(*this, &Navigate::GPSCommGPSD::gps_poll), m_gpsdata.gps_fd, Glib::IO_IN);
        gps_send("w1\n");
}

Navigate::GPSCommGPSD::~GPSCommGPSD( )
{
        m_gpssigioconn.disconnect();
 #if GPSD_API_MAJOR_VERSION < 5
       gpsmap_t::iterator i(m_gpsmap.find(m_gpsdata));
        if (i != m_gpsmap.end())
                m_gpsmap.erase(i);
#endif
        gps_close(&m_gpsdata);
}

void Navigate::GPSCommGPSD::gps_send(const std::string& s)
{
        int r = write(m_gpsdata.gps_fd, s.c_str(), s.size());
        if (r != s.size())
                std::cerr << "gps_send: error " << r << std::endl;
}

#if GPSD_API_MAJOR_VERSION < 5
void Navigate::GPSCommGPSD::gps_hook( struct gps_data_t *p, char *, size_t, int )
{
        gpsmap_t::iterator i(m_gpsmap.find(p));
        if (i != m_gpsmap.end()) {
                (*i).second->update_gps();
                return;
        }
}
#endif

bool Navigate::GPSCommGPSD::gps_poll(Glib::IOCondition iocond)
{
        if (!(iocond & Glib::IO_IN))
                return true;
#if GPSD_API_MAJOR_VERSION >= 5
	if (gps_read(&m_gpsdata) >= 0)
		update_gps();
#else
        ::gps_poll(m_gpsdata);
#endif
        std::cerr << "status " << m_gpsdata.status << " fixtype " << m_gpsdata.fix.mode << std::endl;
        return true;
}

void Navigate::GPSCommGPSD::update_gps(void)
{
        m_update();
}

Navigate::GPSCommGPSD::fixtype_t Navigate::GPSCommGPSD::get_fixtype(void) const
{
        switch (m_gpsdata.fix.mode) {
                default:
                case MODE_NOT_SEEN:
                        return fixtype_noconn;

                case MODE_NO_FIX:
                        return fixtype_nofix;

                case MODE_2D:
                        return fixtype_2d;

                case MODE_3D:
                        return fixtype_3d;
        }
}

Navigate::GPSCommGPSD::fixstatus_t Navigate::GPSCommGPSD::get_fixstatus(void) const
{
        switch (m_gpsdata.status) {
                default:
                case STATUS_NO_FIX:
                        return fixstatus_nofix;

                case STATUS_FIX:
                        return fixstatus_fix;

#if GPSD_API_MAJOR_VERSION < 6
                case STATUS_DGPS_FIX:
                        return fixstatus_dgpsfix;
#endif
        }
}

double Navigate::GPSCommGPSD::get_fixtime(void) const
{
        return m_gpsdata.fix.time;
}

Point Navigate::GPSCommGPSD::get_coord(void) const
{
        return Point((int32_t)(m_gpsdata.fix.longitude * Point::from_deg),
                     (int32_t)(m_gpsdata.fix.latitude * Point::from_deg));
}

float Navigate::GPSCommGPSD::get_coord_uncertainty(void) const
{
        return m_gpsdata.fix.ept;
}

bool Navigate::GPSCommGPSD::is_coord_valid(void) const
{
        return Navigate::GPSComm::is_coord_valid() && (!std::isnan(m_gpsdata.fix.longitude)) && (!std::isnan(m_gpsdata.fix.latitude));
}

float Navigate::GPSCommGPSD::get_altitude(void) const
{
        return m_gpsdata.fix.altitude;
}

float Navigate::GPSCommGPSD::get_altitude_uncertainty(void) const
{
        return m_gpsdata.fix.epv;
}

bool Navigate::GPSCommGPSD::is_altitude_valid(void) const
{
        return Navigate::GPSComm::is_altitude_valid() && !std::isnan(m_gpsdata.fix.altitude);
}

float Navigate::GPSCommGPSD::get_track(void) const
{
        return m_gpsdata.fix.track;
}

float Navigate::GPSCommGPSD::get_track_uncertainty(void) const
{
        return m_gpsdata.fix.epd;
}

float Navigate::GPSCommGPSD::get_velocity(void) const
{
        return m_gpsdata.fix.speed;
}

float Navigate::GPSCommGPSD::get_velocity_uncertainty(void) const
{
        return m_gpsdata.fix.eps;
}

float Navigate::GPSCommGPSD::get_verticalvelocity(void) const
{
        return m_gpsdata.fix.climb;
}

float Navigate::GPSCommGPSD::get_verticalvelocity_uncertainty(void) const
{
        return m_gpsdata.fix.epc;
}

float Navigate::GPSCommGPSD::get_pdop(void) const
{
#if GPSD_API_MAJOR_VERSION < 6
        return m_gpsdata.pdop;
#else
        return m_gpsdata.dop.pdop;
#endif
}

float Navigate::GPSCommGPSD::get_hdop(void) const
{
#if GPSD_API_MAJOR_VERSION < 6
        return m_gpsdata.hdop;
#else
        return m_gpsdata.dop.pdop;
#endif
}

float Navigate::GPSCommGPSD::get_vdop(void) const
{
#if GPSD_API_MAJOR_VERSION < 6
        return m_gpsdata.vdop;
#else
        return m_gpsdata.dop.vdop;
#endif
}

float Navigate::GPSCommGPSD::get_tdop(void) const
{
#if GPSD_API_MAJOR_VERSION < 6
        return m_gpsdata.tdop;
#else
        return m_gpsdata.dop.tdop;
#endif
}

float Navigate::GPSCommGPSD::get_gdop(void) const
{
#if GPSD_API_MAJOR_VERSION < 6
        return m_gpsdata.gdop;
#else
        return m_gpsdata.dop.gdop;
#endif
}

unsigned int Navigate::GPSCommGPSD::get_nrsv(void) const
{
	if (!(m_gpsdata.set & SATELLITE_SET))
		return 0;
        return m_gpsdata.satellites_visible;
}

Navigate::GPSCommGPSD::SV Navigate::GPSCommGPSD::get_sv( unsigned int idx ) const
{
	if (!(m_gpsdata.set & SATELLITE_SET))
		return SV();
        if (idx >= (unsigned int)m_gpsdata.satellites_visible)
		return SV();
#if GPSD_API_MAJOR_VERSION >= 6
	std::set<int> satused;
	for (int i = 0; i < m_gpsdata.satellites_used; ++i)
		satused.insert(m_gpsdata.skyview[i].used);
	return SV(m_gpsdata.skyview[idx].PRN, m_gpsdata.skyview[idx].azimuth, m_gpsdata.skyview[idx].elevation,
		  m_gpsdata.skyview[idx].ss, satused.find(m_gpsdata.skyview[idx].PRN) != satused.end());
#else
	std::set<int> satused;
	for (int i = 0; i < m_gpsdata.satellites_used; ++i)
		satused.insert(m_gpsdata.used[i]);
        return SV(m_gpsdata.PRN[idx], m_gpsdata.azimuth[idx], m_gpsdata.elevation[idx],
		  m_gpsdata.ss[idx], satused.find(m_gpsdata.PRN[idx]) != satused.end());
#endif
}

Navigate::GPSCommGPSD::SVVector Navigate::GPSCommGPSD::get_svvector(void) const
{
        SVVector v;
#if GPSD_API_MAJOR_VERSION >= 6
	std::set<int> satused;
	for (int i = 0; i < m_gpsdata.satellites_used; ++i)
		satused.insert(m_gpsdata.skyview[i].used);
        for (int i = 0; i < m_gpsdata.satellites_visible; ++i)
                v.push_back(SV(m_gpsdata.skyview[i].PRN, m_gpsdata.skyview[i].azimuth, m_gpsdata.skyview[i].elevation,
			       m_gpsdata.skyview[i].ss, satused.find(m_gpsdata.skyview[i].PRN) != satused.end()));
#else
	std::set<int> satused;
	for (int i = 0; i < m_gpsdata.satellites_used; ++i)
		satused.insert(m_gpsdata.used[i]);
        for (int i = 0; i < m_gpsdata.satellites_visible; ++i)
                v.push_back(SV(m_gpsdata.PRN[i], m_gpsdata.azimuth[i], m_gpsdata.elevation[i],
			       m_gpsdata.ss[i], satused.find(m_gpsdata.PRN[idx]) != satused.end()));
#endif
        return v;
}

#endif

Navigate::NavInfoWindow::NavInfoWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : toplevel_window_t(castitem)
{
}

Navigate::NavInfoWindow::~NavInfoWindow()
{
}

Navigate::NavPageWindow::NavPageWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : toplevel_window_t(castitem), m_fullscreen(false)
{
        signal_window_state_event().connect(sigc::mem_fun(*this, &Navigate::NavPageWindow::on_my_window_state_event));
}

Navigate::NavPageWindow::~NavPageWindow()
{
}

bool Navigate::NavPageWindow::on_my_window_state_event(GdkEventWindowState *state)
{
        if (!state)
                return false;
        m_fullscreen = !!(Gdk::WINDOW_STATE_FULLSCREEN & state->new_window_state);
        return false;
}

void Navigate::NavPageWindow::toggle_fullscreen(void)
{
        if (m_fullscreen)
                unfullscreen();
        else
                fullscreen();
}

Navigate::NavPOIWindow::POIModelColumns::POIModelColumns(void)
{
        add(m_col_name);
        add(m_col_anglename);
        add(m_col_angle);
        add(m_col_dist);
}

Glib::ustring Navigate::NavPOIWindow::POI::get_angletext(void ) const
{
        uint16_t angle = m_angle + 0x0800;
        if (angle < 0x1000)
                return "N";
        if (angle < 0x2000)
                return "NNE";
        if (angle < 0x3000)
                return "NE";
        if (angle < 0x4000)
                return "ENE";
        if (angle < 0x5000)
                return "E";
        if (angle < 0x6000)
                return "ESE";
        if (angle < 0x7000)
                return "SE";
        if (angle < 0x8000)
                return "SSE";
        if (angle < 0x9000)
                return "S";
        if (angle < 0xa000)
                return "SSW";
        if (angle < 0xb000)
                return "SW";
        if (angle < 0xc000)
                return "WSW";
        if (angle < 0xd000)
                return "W";
        if (angle < 0xe000)
                return "WNW";
        if (angle < 0xf000)
                return "NW";
        return "NNW";
}
Navigate::NavPOIWindow::NavPOIWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : Navigate::NavPageWindow(castitem, refxml), m_engine(0), m_coord(0, 0), m_queryrect(Point(~0, ~0), Point(~0, ~0))
{
        m_dispatch.connect(sigc::mem_fun(*this, &Navigate::NavPOIWindow::async_done));
        get_widget(refxml, "navpoiairports", m_navpoiairports);
        get_widget(refxml, "navpoinavaids", m_navpoinavaids);
        get_widget(refxml, "navpoimapelements", m_navpoimapelements);
        get_widget(refxml, "navpoiwaypoints", m_navpoiwaypoints);
        // Airports List
        m_airportsstore = Gtk::ListStore::create(m_poimodelcolumns);
        m_navpoiairports->set_model(m_airportsstore);
        m_navpoiairports->append_column(_("Name"), m_poimodelcolumns.m_col_name);
        m_navpoiairports->append_column(_("Angle"), m_poimodelcolumns.m_col_anglename);
        m_navpoiairports->append_column(_("Angle"), m_poimodelcolumns.m_col_angle);
        m_navpoiairports->append_column(_("Dist"), m_poimodelcolumns.m_col_dist);
        //Gtk::TreeViewColumn *dist_column = m_navpoiairports->get_column(m_navpoiairports->append_column(_("Dist"), m_poimodelcolumns.m_col_dist) - 1);
        //dist_column->set_cell_data_func(*dist_column->get_first_cell_renderer(), sigc::mem_fun(*this, &Navigate::NavPOIWindow::distance_cell_func));
        for (unsigned int i = 0; i < 4; ++i) {
                Gtk::TreeViewColumn *col = m_navpoiairports->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                const Gtk::TreeModelColumnBase *cols[4] = { &m_poimodelcolumns.m_col_name, &m_poimodelcolumns.m_col_anglename,
                                                            &m_poimodelcolumns.m_col_angle, &m_poimodelcolumns.m_col_dist };
                col->set_sort_column(*cols[i]);
                col->set_expand(i == 0);
        }
        m_navpoiairports->set_headers_visible(false);
        {
                Glib::RefPtr<Gtk::TreeSelection> sel(m_navpoiairports->get_selection());
                sel->set_mode(Gtk::SELECTION_NONE);
        }
        m_navpoiairports->set_enable_search(false);
        m_navpoiairports->set_reorderable(false);
        // Navaids List
        m_navaidsstore = Gtk::ListStore::create(m_poimodelcolumns);
        m_navpoinavaids->set_model(m_navaidsstore);
        m_navpoinavaids->append_column(_("Name"), m_poimodelcolumns.m_col_name);
        m_navpoinavaids->append_column(_("Angle"), m_poimodelcolumns.m_col_anglename);
        m_navpoinavaids->append_column(_("Angle"), m_poimodelcolumns.m_col_angle);
        m_navpoinavaids->append_column(_("Dist"), m_poimodelcolumns.m_col_dist);
        //Gtk::TreeViewColumn *dist_column = m_navpoinavaids->get_column(m_navpoinavaids->append_column(_("Dist"), m_poimodelcolumns.m_col_dist) - 1);
        //dist_column->set_cell_data_func(*dist_column->get_first_cell_renderer(), sigc::mem_fun(*this, &Navigate::NavPOIWindow::distance_cell_func));
        for (unsigned int i = 0; i < 4; ++i) {
                Gtk::TreeViewColumn *col = m_navpoinavaids->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                const Gtk::TreeModelColumnBase *cols[4] = { &m_poimodelcolumns.m_col_name, &m_poimodelcolumns.m_col_anglename,
                                                            &m_poimodelcolumns.m_col_angle, &m_poimodelcolumns.m_col_dist };
                col->set_sort_column(*cols[i]);
                col->set_expand(i == 0);
        }
        m_navpoinavaids->set_headers_visible(false);
        {
                Glib::RefPtr<Gtk::TreeSelection> sel(m_navpoinavaids->get_selection());
                sel->set_mode(Gtk::SELECTION_NONE);
        }
        m_navpoinavaids->set_enable_search(false);
        m_navpoinavaids->set_reorderable(false);
        // Waypoints List
        m_waypointsstore = Gtk::ListStore::create(m_poimodelcolumns);
        m_navpoiwaypoints->set_model(m_waypointsstore);
        m_navpoiwaypoints->append_column(_("Name"), m_poimodelcolumns.m_col_name);
        m_navpoiwaypoints->append_column(_("Angle"), m_poimodelcolumns.m_col_anglename);
        m_navpoiwaypoints->append_column(_("Angle"), m_poimodelcolumns.m_col_angle);
        m_navpoiwaypoints->append_column(_("Dist"), m_poimodelcolumns.m_col_dist);
        //Gtk::TreeViewColumn *dist_column = m_navpoiwaypoints->get_column(m_navpoiwaypoints->append_column(_("Dist"), m_poimodelcolumns.m_col_dist) - 1);
        //dist_column->set_cell_data_func(*dist_column->get_first_cell_renderer(), sigc::mem_fun(*this, &Navigate::NavPOIWindow::distance_cell_func));
        for (unsigned int i = 0; i < 4; ++i) {
                Gtk::TreeViewColumn *col = m_navpoiwaypoints->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                const Gtk::TreeModelColumnBase *cols[4] = { &m_poimodelcolumns.m_col_name, &m_poimodelcolumns.m_col_anglename,
                                                            &m_poimodelcolumns.m_col_angle, &m_poimodelcolumns.m_col_dist };
                col->set_sort_column(*cols[i]);
                col->set_expand(i == 0);
        }
        m_navpoiwaypoints->set_headers_visible(false);
        {
                Glib::RefPtr<Gtk::TreeSelection> sel(m_navpoiwaypoints->get_selection());
                sel->set_mode(Gtk::SELECTION_NONE);
        }
        m_navpoiwaypoints->set_enable_search(false);
        m_navpoiwaypoints->set_reorderable(false);
        // Mapelements List
        m_mapelementsstore = Gtk::ListStore::create(m_poimodelcolumns);
        m_navpoimapelements->set_model(m_mapelementsstore);
        m_navpoimapelements->append_column(_("Name"), m_poimodelcolumns.m_col_name);
        m_navpoimapelements->append_column(_("Angle"), m_poimodelcolumns.m_col_anglename);
        m_navpoimapelements->append_column(_("Angle"), m_poimodelcolumns.m_col_angle);
        m_navpoimapelements->append_column(_("Dist"), m_poimodelcolumns.m_col_dist);
        //Gtk::TreeViewColumn *dist_column = m_navpoimapelements->get_column(m_navpoimapelements->append_column(_("Dist"), m_poimodelcolumns.m_col_dist) - 1);
        //dist_column->set_cell_data_func(*dist_column->get_first_cell_renderer(), sigc::mem_fun(*this, &Navigate::NavPOIWindow::distance_cell_func));
        for (unsigned int i = 0; i < 4; ++i) {
                Gtk::TreeViewColumn *col = m_navpoimapelements->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                const Gtk::TreeModelColumnBase *cols[4] = { &m_poimodelcolumns.m_col_name, &m_poimodelcolumns.m_col_anglename,
                                                            &m_poimodelcolumns.m_col_angle, &m_poimodelcolumns.m_col_dist };
                col->set_sort_column(*cols[i]);
                col->set_expand(i == 0);
        }
        m_navpoimapelements->set_headers_visible(false);
        {
                Glib::RefPtr<Gtk::TreeSelection> sel(m_navpoimapelements->get_selection());
                sel->set_mode(Gtk::SELECTION_NONE);
        }
        m_navpoimapelements->set_enable_search(false);
        m_navpoimapelements->set_reorderable(false);
}

Navigate::NavPOIWindow::~NavPOIWindow()
{
        async_cancel();
}

void Navigate::NavPOIWindow::set_coord(const Point & coord)
{
        m_coord = coord;
        update_display();
        if (!m_engine || m_queryrect.is_inside(m_coord))
                return;
        async_cancel();
        m_queryrect = m_coord.simple_box_nmi(2.0);
        Rect bbox = m_coord.simple_box_nmi(20.0);
        m_airportquery = m_engine->async_airport_find_bbox(bbox, ~0, AirportsDb::element_t::subtables_none);
        m_navaidquery = m_engine->async_navaid_find_bbox(bbox, ~0, NavaidsDb::element_t::subtables_none);
        m_waypointquery = m_engine->async_waypoint_find_bbox(bbox, ~0, WaypointsDb::element_t::subtables_none);
        m_mapelementquery = m_engine->async_mapelement_find_bbox(bbox, ~0, MapelementsDb::element_t::subtables_none);
        if (m_airportquery)
                m_airportquery->connect(sigc::mem_fun(*this, &Navigate::NavPOIWindow::async_done_dispatch));
        if (m_navaidquery)
                m_navaidquery->connect(sigc::mem_fun(*this, &Navigate::NavPOIWindow::async_done_dispatch));
        if (m_waypointquery)
                m_waypointquery->connect(sigc::mem_fun(*this, &Navigate::NavPOIWindow::async_done_dispatch));
        if (m_mapelementquery)
                m_mapelementquery->connect(sigc::mem_fun(*this, &Navigate::NavPOIWindow::async_done_dispatch));
}

void Navigate::NavPOIWindow::async_cancel(void)
{
        Engine::MapelementResult::cancel(m_mapelementquery);
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::WaypointResult::cancel(m_waypointquery);
        Engine::AirportResult::cancel(m_airportquery);
}

void Navigate::NavPOIWindow::async_done_dispatch(void)
{
        m_dispatch();
}

void Navigate::NavPOIWindow::async_done(void)
{
        if (m_airportquery && m_airportquery->is_done()) {
                if (!m_airportquery->is_error()) {
                        AirportsDb::elementvector_t& ev(m_airportquery->get_result());
                        m_poiairports.clear();
                        for (AirportsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                Glib::ustring name(ei->get_icao_name());
                                if (name.empty())
                                        continue;
                                m_poiairports.push_back(POI(name, ei->get_coord()));
                        }
                }
                m_airportquery = Glib::RefPtr<Engine::AirportResult>();
        }
        if (m_navaidquery && m_navaidquery->is_done()) {
                if (!m_navaidquery->is_error()) {
                        NavaidsDb::elementvector_t& ev(m_navaidquery->get_result());
                        m_poinavaids.clear();
                        for (NavaidsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                Glib::ustring name(ei->get_icao_name());
                                if (name.empty())
                                        continue;
                                m_poinavaids.push_back(POI(name, ei->get_coord()));
                        }
                }
                m_navaidquery = Glib::RefPtr<Engine::NavaidResult>();
        }
        if (m_waypointquery && m_waypointquery->is_done()) {
                if (!m_waypointquery->is_error()) {
                        WaypointsDb::elementvector_t& ev(m_waypointquery->get_result());
                        m_poiwaypoints.clear();
                        for (WaypointsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                Glib::ustring name(ei->get_icao_name());
                                if (name.empty())
                                        continue;
                                m_poiwaypoints.push_back(POI(name, ei->get_coord()));
                        }
                }
                m_waypointquery = Glib::RefPtr<Engine::WaypointResult>();
        }
        if (m_mapelementquery && m_mapelementquery->is_done()) {
                if (!m_mapelementquery->is_error()) {
                        MapelementsDb::elementvector_t& ev(m_mapelementquery->get_result());
                        m_poimapelements.clear();
                        for (MapelementsDb::elementvector_t::const_iterator ei(ev.begin()), ee(ev.end()); ei != ee; ei++) {
                                Glib::ustring name(ei->get_name());
                                if (name.empty())
                                        continue;
                                m_poimapelements.push_back(POI(name, ei->get_coord()));
                        }
                }
                m_mapelementquery = Glib::RefPtr<Engine::MapelementResult>();
        }
        if (m_airportquery || m_navaidquery || m_waypointquery || m_mapelementquery)
                return;
        update_display();
}

void Navigate::NavPOIWindow::update_display(void)
{
        for (std::vector<POI>::iterator pi(m_poiairports.begin()), pe(m_poiairports.end()); pi != pe; pi++)
                pi->update(m_coord);
        sort(m_poiairports.begin(), m_poiairports.end());
        if (m_poiairports.size() > 10)
                m_poiairports.resize(10);
        for (std::vector<POI>::iterator pi(m_poimapelements.begin()), pe(m_poimapelements.end()); pi != pe; pi++)
                pi->update(m_coord);
        sort(m_poimapelements.begin(), m_poimapelements.end());
        if (m_poimapelements.size() > 10)
                m_poimapelements.resize(10);
        for (std::vector<POI>::iterator pi(m_poinavaids.begin()), pe(m_poinavaids.end()); pi != pe; pi++)
                pi->update(m_coord);
        sort(m_poinavaids.begin(), m_poinavaids.end());
        if (m_poinavaids.size() > 10)
                m_poinavaids.resize(10);
        for (std::vector<POI>::iterator pi(m_poiwaypoints.begin()), pe(m_poiwaypoints.end()); pi != pe; pi++)
                pi->update(m_coord);
        sort(m_poiwaypoints.begin(), m_poiwaypoints.end());
        if (m_poiwaypoints.size() > 10)
                m_poiwaypoints.resize(10);
        // Update list stores
        m_airportsstore->clear();
        unsigned int i = 0;
        for (std::vector<POI>::const_iterator pi(m_poiairports.begin()), pe(m_poiairports.end()); i < 5 && pi != pe; pi++, i++) {
                Gtk::TreeModel::Row row(*(m_airportsstore->append()));
                row[m_poimodelcolumns.m_col_name] = pi->get_name();
                row[m_poimodelcolumns.m_col_anglename] = pi->get_angletext();
                row[m_poimodelcolumns.m_col_angle] = Conversions::track_str(pi->get_angle());
                row[m_poimodelcolumns.m_col_dist] = Conversions::dist_str(pi->get_dist());
        }
        m_mapelementsstore->clear();
        i = 0;
        for (std::vector<POI>::const_iterator pi(m_poimapelements.begin()), pe(m_poimapelements.end()); i < 5 && pi != pe; pi++, i++) {
                Gtk::TreeModel::Row row(*(m_mapelementsstore->append()));
                row[m_poimodelcolumns.m_col_name] = pi->get_name();
                row[m_poimodelcolumns.m_col_anglename] = pi->get_angletext();
                row[m_poimodelcolumns.m_col_angle] = Conversions::track_str(pi->get_angle());
                row[m_poimodelcolumns.m_col_dist] = Conversions::dist_str(pi->get_dist());
        }
        m_navaidsstore->clear();
        i = 0;
        for (std::vector<POI>::const_iterator pi(m_poinavaids.begin()), pe(m_poinavaids.end()); i < 5 && pi != pe; pi++, i++) {
                Gtk::TreeModel::Row row(*(m_navaidsstore->append()));
                row[m_poimodelcolumns.m_col_name] = pi->get_name();
                row[m_poimodelcolumns.m_col_anglename] = pi->get_angletext();
                row[m_poimodelcolumns.m_col_angle] = Conversions::track_str(pi->get_angle());
                row[m_poimodelcolumns.m_col_dist] = Conversions::dist_str(pi->get_dist());
        }
        m_waypointsstore->clear();
        i = 0;
        for (std::vector<POI>::const_iterator pi(m_poiwaypoints.begin()), pe(m_poiwaypoints.end()); i < 5 && pi != pe; pi++, i++) {
                Gtk::TreeModel::Row row(*(m_waypointsstore->append()));
                row[m_poimodelcolumns.m_col_name] = pi->get_name();
                row[m_poimodelcolumns.m_col_anglename] = pi->get_angletext();
                row[m_poimodelcolumns.m_col_angle] = Conversions::track_str(pi->get_angle());
                row[m_poimodelcolumns.m_col_dist] = Conversions::dist_str(pi->get_dist());
        }
}

Navigate::NavVectorMapArea::NavVectorMapArea( BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml )
        : VectorMapArea(castitem, refxml)
{
}

Navigate::NavVectorMapArea::~NavVectorMapArea( )
{
}

void Navigate::NavVectorMapArea::set_cursor( const Point & pt )
{
        set_cursor(pt, 0.0, false);
}

void Navigate::NavVectorMapArea::set_cursor( const Point & pt, float track_deg, bool trackvalid )
{
        m_cursortrack = track_deg * (M_PI / 180.0);
        m_cursortrackvalid = trackvalid;
        VectorMapArea::set_cursor(pt);
        if (trackvalid)
                VectorMapArea::set_cursor_angle(track_deg);
        else
                VectorMapArea::invalidate_cursor_angle();
}

Navigate::FindWindow::FindWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : toplevel_window_t(castitem), m_refxml(refxml), m_engine(0), m_treeview(0), m_buttonok(0)
{
        signal_hide().connect(sigc::mem_fun(*this, &Navigate::FindWindow::on_my_hide));
        signal_delete_event().connect(sigc::mem_fun(*this, &Navigate::FindWindow::on_my_delete_event));
        m_querydispatch.connect(sigc::mem_fun(*this, &Navigate::FindWindow::async_done));
}

Navigate::FindWindow::~FindWindow()
{
        async_cancel();
}

void Navigate::FindWindow::set_engine(Engine & eng)
{
        m_engine = &eng;
}

void Navigate::FindWindow::set_center(const Point & pt)
{
        m_center = pt;
}

void Navigate::FindWindow::init_list(void)
{
        // Create Tree Model
        m_findstore = Gtk::TreeStore::create(m_findcolumns);
        m_treeview->set_model(m_findstore);
        Gtk::CellRendererText *display_renderer = Gtk::manage(new Gtk::CellRendererText());
        int display_col = m_treeview->append_column(_("Waypoint"), *display_renderer) - 1;
        Gtk::CellRendererText *dist_renderer = Gtk::manage(new Gtk::CellRendererText());
        int dist_col = m_treeview->append_column(_("Dist"), *dist_renderer) - 1;
        {
                Gtk::TreeViewColumn *display_column = m_treeview->get_column(display_col);
                if (display_column) {
                        display_column->add_attribute(*display_renderer, "text", m_findcolumns.m_col_display);
                        display_column->add_attribute(*display_renderer, "foreground-gdk", m_findcolumns.m_col_display_color);
                }
        }
        {
                Gtk::TreeViewColumn *dist_column = m_treeview->get_column(dist_col);
                if (dist_column) {
                        dist_column->add_attribute(*dist_renderer, "text", m_findcolumns.m_col_dist);
                        //dist_column->set_cell_data_func(*dist_renderer, sigc::mem_fun(*this, &Navigate::FindWindow::distance_cell_func));
                }
                dist_renderer->set_property("xalign", 1.0);
        }
        for (unsigned int i = 0; i < 2; ++i) {
                Gtk::TreeViewColumn *col = m_treeview->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                const Gtk::TreeModelColumnBase *cols[2] = { &m_findcolumns.m_col_display, &m_findcolumns.m_col_dist };
                col->set_sort_column(*cols[i]);
                col->set_expand(i == 0);
        }
        m_findstore->set_sort_func(m_findcolumns.m_col_dist, sigc::mem_fun(*this, &Navigate::FindWindow::sort_dist));
        m_findstore->set_sort_column(Gtk::TreeSortable::DEFAULT_SORT_COLUMN_ID, Gtk::SORT_ASCENDING);
#if 0
        for (unsigned int i = 1; i < 2; ++i) {
                Gtk::CellRenderer *renderer = m_treeview->get_column_cell_renderer(i);
                if (!renderer)
                        continue;
                renderer->set_property("xalign", 1.0);
        }
#endif
        //m_treeview->columns_autosize();
        //m_treeview->set_headers_clickable();
        // selection
        Glib::RefPtr<Gtk::TreeSelection> find_selection = m_treeview->get_selection();
        find_selection->set_mode(Gtk::SELECTION_SINGLE);
        find_selection->signal_changed().connect(sigc::mem_fun(*this, &FindWindow::find_selection_changed));
        find_selection->set_select_function(sigc::mem_fun(*this, &FindWindow::find_select_function));
        m_buttonok->set_sensitive(false);
        // Insert Headers
        Gdk::Color black;
        black.set_grey(0);
        Gtk::TreeModel::Row row(*(m_findstore->append()));
        row[m_findcolumns.m_col_display] = "Flightplan Waypoints";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Airports";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Navaids";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Waypoints";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Map Elements";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Airspaces";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
        row = *(m_findstore->append());
        row[m_findcolumns.m_col_display] = "Airways";
        row[m_findcolumns.m_col_display_color] = black;
        row[m_findcolumns.m_col_dist] = "";
}

int Navigate::FindWindow::sort_dist(const Gtk::TreeModel::iterator& ia, const Gtk::TreeModel::iterator& ib)
{
        //Gtk::TreeModel::Path pa(m_findstore->get_path(ia)), pb(m_findstore->get_path(ib));
        const Gtk::TreeModel::Row rowa(*ia), rowb(*ib);
        return RouteEditUi::compare_dist(rowa.get_value(m_findcolumns.m_col_dist), rowb.get_value(m_findcolumns.m_col_dist));
}

template<> const unsigned int Navigate::FindWindow::FindObjectInfo<FPlanWaypoint>::parent_row = 0;
template<> const unsigned int Navigate::FindWindow::FindObjectInfo<AirportsDb::Airport>::parent_row = 1;
template<> const unsigned int Navigate::FindWindow::FindObjectInfo<NavaidsDb::Navaid>::parent_row = 2;
template<> const unsigned int Navigate::FindWindow::FindObjectInfo<WaypointsDb::Waypoint>::parent_row = 3;
template<> const unsigned int Navigate::FindWindow::FindObjectInfo<MapelementsDb::Mapelement>::parent_row = 4;
template<> const unsigned int Navigate::FindWindow::FindObjectInfo<AirspacesDb::Airspace>::parent_row = 5;
template<> const unsigned int Navigate::FindWindow::FindObjectInfo<AirwaysDb::Airway>::parent_row = 6;

template<typename T> void Navigate::FindWindow::set_objects(const T& v1, const T& v2)
{
        Gtk::TreeModel::Row parentrow(m_findstore->children()[FindObjectInfo<typename T::value_type>::parent_row]);
        Gtk::TreeModel::Path parentpath(m_findstore->get_path(parentrow));
        bool exp(m_treeview->row_expanded(parentpath));
        {
                Gtk::TreeIter iter;
                while (iter = parentrow->children().begin())
                        m_findstore->erase(iter);
        }
        for (typename T::const_iterator i1(v1.begin()), ie(v1.end()); i1 != ie; i1++) {
                const typename T::value_type& nn(*i1);
                Gtk::TreeModel::Row row(*(m_findstore->append(parentrow.children())));
                set_row(row, nn, false);
        }
        for (typename T::const_iterator i1(v2.begin()), ie(v2.end()); i1 != ie; i1++) {
                const typename T::value_type& nn(*i1);
                Gtk::TreeModel::Row row(*(m_findstore->append(parentrow.children())));
                set_row(row, nn, true);
        }
        if (exp)
                m_treeview->expand_row(parentpath, false);
        else
                m_treeview->collapse_row(parentpath);
        Gdk::Color black;
        black.set_grey(0);
        parentrow[m_findcolumns.m_col_display_color] = black;
}

void Navigate::FindWindow::clear_objects(void)
{
        Gdk::Color blue;
        blue.set_rgb(0, 0, 0xffff);
        for (Gtk::TreeIter i1(m_findstore->children().begin()); i1; i1++) {
                Gtk::TreeModel::Row parentrow(*i1);
                Gtk::TreeIter iter;
                while (iter = parentrow->children().begin())
                        m_findstore->erase(iter);
                parentrow[m_findcolumns.m_col_display_color] = blue;
        }
}

void Navigate::FindWindow::find_selection_changed(void)
{
        Glib::RefPtr<Gtk::TreeSelection> find_selection = m_treeview->get_selection();
        Gtk::TreeModel::iterator iter = find_selection->get_selected();
        if (iter) {
                Gtk::TreeModel::Row row = *iter;
                //Do something with the row.
                std::cerr << "selected row: " << row[m_findcolumns.m_col_display] << std::endl;
                m_buttonok->set_sensitive(true);
                return;
        }
        std::cerr << "selection cleared" << std::endl;
        m_buttonok->set_sensitive(false);
}

bool Navigate::FindWindow::find_select_function(const Glib::RefPtr< Gtk::TreeModel > & model, const Gtk::TreeModel::Path & path, bool)
{
        return (path.size() > 1);
}

void Navigate::FindWindow::buttonok_clicked(void)
{
        Glib::RefPtr<Gtk::TreeSelection> find_selection = m_treeview->get_selection();
        Gtk::TreeModel::iterator iter = find_selection->get_selected();
        if (!iter)
                return;
        Gtk::TreeModel::Row row = *iter;
        switch (row[m_findcolumns.m_col_type]) {
                case 1:
                {
                        std::vector<FPlanWaypoint> wpts;
                        iter = row.children().begin();
                        while (iter) {
                                row = *iter;
                                std::cerr << "set route point: " << row[m_findcolumns.m_col_display] << std::endl;
                                iter++;
                                FPlanWaypoint wpt;
                                wpt.set_icao(row[m_findcolumns.m_col_icao]);
                                wpt.set_name(row[m_findcolumns.m_col_name]);
                                wpt.set_note(row[m_findcolumns.m_col_comment]);
                                wpt.set_frequency(row[m_findcolumns.m_col_freq]);
                                wpt.set_coord(row[m_findcolumns.m_col_coord]);
                                wpt.set_altitude(row[m_findcolumns.m_col_alt]);
                                wpt.set_flags(row[m_findcolumns.m_col_flags]);
                                wpts.push_back(wpt);
                        }
                        signal_setwaypoints()(wpts);
                        break;
                }

                default:
                {
                        std::cerr << "set point: " << row[m_findcolumns.m_col_display] << std::endl;
                        FPlanWaypoint wpt;
                        wpt.set_icao(row[m_findcolumns.m_col_icao]);
                        wpt.set_name(row[m_findcolumns.m_col_name]);
                        wpt.set_note(row[m_findcolumns.m_col_comment]);
                        wpt.set_frequency(row[m_findcolumns.m_col_freq]);
                        wpt.set_coord(row[m_findcolumns.m_col_coord]);
                        wpt.set_altitude(row[m_findcolumns.m_col_alt]);
                        wpt.set_flags(row[m_findcolumns.m_col_flags]);
                        std::vector<FPlanWaypoint> wpts;
                        wpts.push_back(wpt);
                        signal_setwaypoints()(wpts);
                        break;
                }
        }
        hide();
}

void Navigate::FindWindow::buttoncancel_clicked(void)
{
        hide();
        async_cancel();
}

void Navigate::FindWindow::on_my_hide(void)
{
        clear_objects();
        std::cerr << "find: " << this << " hide" << std::endl;
}

bool Navigate::FindWindow::on_my_delete_event(GdkEventAny * event)
{
        hide();
        return true;
}

Navigate::FindWindow::FindModelColumns::FindModelColumns(void)
{
        add(m_col_display);
        add(m_col_display_color);
        add(m_col_dist);
        add(m_col_icao);
        add(m_col_name);
        add(m_col_coord);
        add(m_col_alt);
        add(m_col_flags);
        add(m_col_freq);
        add(m_col_comment);
        add(m_col_type);
}

void Navigate::FindWindow::set_row_display(Gtk::TreeModel::Row & row, const Glib::ustring & comp1, const Glib::ustring & comp2)
{
        Glib::ustring s(comp1);
        if (!comp1.empty() && !comp2.empty())
                s += ' ';
        s += comp2;
        row[m_findcolumns.m_col_display] = s;
        Gdk::Color black;
        black.set_grey(0);
        row[m_findcolumns.m_col_display_color] = black;
}

void Navigate::FindWindow::set_row(Gtk::TreeModel::Row & row, const FPlanWaypoint & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name());
        Point pt(nn.get_coord());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = nn.get_altitude();
        row[m_findcolumns.m_col_flags] = nn.get_flags();
        row[m_findcolumns.m_col_freq] = nn.get_frequency();
        row[m_findcolumns.m_col_comment] = nn.get_note();
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
}

void Navigate::FindWindow::set_row(Gtk::TreeModel::Row & row, const AirportsDb::Airport & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name()), comment;
        Point pt(nn.get_coord());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = nn.get_elevation();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
        if (nn.get_nrcomm())
                row[m_findcolumns.m_col_freq] = nn.get_comm(0)[0];
        row[m_findcolumns.m_col_comment] = comment;
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
        for (unsigned int rtenr = 0; rtenr < nn.get_nrvfrrte(); rtenr++) {
                const AirportsDb::Airport::VFRRoute& rte(nn.get_vfrrte(rtenr));
                Gtk::TreeModel::Row rterow(*(m_findstore->append(row.children())));
                Point pt1(pt);
                if (rte.size() > 0) {
                        const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtep(rte[0]);
                        pt1 = rtep.get_coord();
                }
                rterow[m_findcolumns.m_col_dist] = Conversions::dist_str(pt1.spheric_distance_nmi(m_center));
                rterow[m_findcolumns.m_col_icao] = icao;
                rterow[m_findcolumns.m_col_name] = name;
                rterow[m_findcolumns.m_col_coord] = pt;
                rterow[m_findcolumns.m_col_alt] = nn.get_elevation();
                rterow[m_findcolumns.m_col_flags] = 0;
                rterow[m_findcolumns.m_col_freq] = 0;
                if (nn.get_nrcomm())
                        rterow[m_findcolumns.m_col_freq] = nn.get_comm(0)[0];
                rterow[m_findcolumns.m_col_comment] = comment;
                rterow[m_findcolumns.m_col_type] = 1;
                rterow[m_findcolumns.m_col_display] = rte.get_name();
                for (unsigned int rtepnr = 0; rtepnr < rte.size(); rtepnr++) {
                        const AirportsDb::Airport::VFRRoute::VFRRoutePoint& rtep(rte[rtepnr]);
                        Gtk::TreeModel::Row rteprow(*(m_findstore->append(rterow.children())));
                        Point pt2(rtep.get_coord());
                        rteprow[m_findcolumns.m_col_dist] = Conversions::dist_str(pt2.spheric_distance_nmi(m_center));
                        rteprow[m_findcolumns.m_col_icao] = icao;
                        rteprow[m_findcolumns.m_col_coord] = pt2;
                        if (rtep.is_at_airport()) {
                                rteprow[m_findcolumns.m_col_name] = name;
                                rteprow[m_findcolumns.m_col_alt] = nn.get_elevation();
                        } else {
                                rteprow[m_findcolumns.m_col_name] = rtep.get_name();
                                rteprow[m_findcolumns.m_col_alt] = rtep.get_altitude();
                        }
                        rteprow[m_findcolumns.m_col_flags] = 0;
                        rteprow[m_findcolumns.m_col_freq] = 0;
                        if (nn.get_nrcomm())
                                rteprow[m_findcolumns.m_col_freq] = nn.get_comm(0)[0];
                        rteprow[m_findcolumns.m_col_comment] = comment;
                        rteprow[m_findcolumns.m_col_type] = 0;
                        rteprow[m_findcolumns.m_col_display] = rtep.get_name();
                }
        }
}

void Navigate::FindWindow::set_row(Gtk::TreeModel::Row & row, const NavaidsDb::Navaid & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name()), comment(nn.get_navaid_typename());
        Point pt(nn.get_coord());
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", nn.get_range());
        comment += " Range: ";
        comment += buf;
        comment += "\n";
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = nn.get_elev();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = nn.get_frequency();
        row[m_findcolumns.m_col_comment] = comment;
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
}

void Navigate::FindWindow::set_row(Gtk::TreeModel::Row & row, const WaypointsDb::Waypoint & nn, bool dispreverse)
{
        Glib::ustring name(nn.get_name());
        Point pt(nn.get_coord());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(pt.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = "";
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = std::numeric_limits<int>::min();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
        row[m_findcolumns.m_col_comment] = "";
        row[m_findcolumns.m_col_type] = 0;
        set_row_display(row, name);
}

void Navigate::FindWindow::set_row(Gtk::TreeModel::Row & row, const AirwaysDb::Airway & nn, bool dispreverse)
{
        Glib::ustring name(nn.get_name()), nameb(nn.get_begin_name()), namee(nn.get_end_name());
        Point ptb(nn.get_begin_coord()), pte(nn.get_end_coord());
        float db(ptb.spheric_distance_nmi(m_center)), de(pte.spheric_distance_nmi(m_center));
        if (db > de) {
                std::swap(db, de);
                std::swap(ptb, pte);
                std::swap(nameb, namee);
        }
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(db);
        row[m_findcolumns.m_col_icao] = "";
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = ptb;
        row[m_findcolumns.m_col_alt] = nn.get_base_level() * 100;
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
        row[m_findcolumns.m_col_comment] = "";
        row[m_findcolumns.m_col_type] = 1;
        {
                std::ostringstream oss;
                oss << '(' << nameb << '-' << namee
                                << ", FL" << nn.get_base_level() << "-FL" << nn.get_top_level()
                                << ", " << nn.get_type_name() << ')';
                set_row_display(row, name, oss.str());
        }
        std::ostringstream oss;
        oss << name << ", " << nn.get_type_name() << ", FL" << nn.get_base_level() << "-FL" << nn.get_top_level();
        Gtk::TreeModel::Row rowb(*(m_findstore->append(row.children())));
        Gtk::TreeModel::Row rowe(*(m_findstore->append(row.children())));
        rowb[m_findcolumns.m_col_dist] = Conversions::dist_str(db);
        rowb[m_findcolumns.m_col_icao] = "";
        rowb[m_findcolumns.m_col_name] = nameb;
        rowb[m_findcolumns.m_col_coord] = ptb;
        rowb[m_findcolumns.m_col_alt] = nn.get_base_level() * 100;
        rowb[m_findcolumns.m_col_flags] = 0;
        rowb[m_findcolumns.m_col_freq] = 0;
        rowb[m_findcolumns.m_col_comment] = oss.str();
        rowb[m_findcolumns.m_col_type] = 0;
        set_row_display(rowb, nameb);
        rowe[m_findcolumns.m_col_dist] = Conversions::dist_str(de);
        rowe[m_findcolumns.m_col_icao] = "";
        rowe[m_findcolumns.m_col_name] = namee;
        rowe[m_findcolumns.m_col_coord] = pte;
        rowe[m_findcolumns.m_col_alt] = nn.get_base_level() * 100;
        rowe[m_findcolumns.m_col_flags] = 0;
        rowe[m_findcolumns.m_col_freq] = 0;
        rowe[m_findcolumns.m_col_comment] = "";
        rowe[m_findcolumns.m_col_type] = 0;
        set_row_display(rowe, namee);
}

void Navigate::FindWindow::set_row(Gtk::TreeModel::Row & row, const AirspacesDb::Airspace & nn, bool dispreverse)
{
        Glib::ustring icao(nn.get_icao()), name(nn.get_name());
        Point pt(nn.get_labelcoord());
        Rect r(nn.get_bbox());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(r.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = icao;
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = (nn.get_altlwr() + nn.get_altupr()) / 2;
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = nn.get_commfreq(0);
        row[m_findcolumns.m_col_comment] = nn.get_commname() + "\n" +
                        nn.get_classexcept() + "\n" + nn.get_ident() + "\n" +
                        nn.get_efftime() + "\n" + nn.get_note() + "\n";
        row[m_findcolumns.m_col_type] = 0;
        if (dispreverse)
                set_row_display(row, name, icao);
        else
                set_row_display(row, icao, name);
}

void Navigate::FindWindow::set_row(Gtk::TreeModel::Row & row, const MapelementsDb::Mapelement & nn, bool dispreverse)
{
        Glib::ustring name(nn.get_name());
        Point pt(nn.get_labelcoord());
        Rect r(nn.get_bbox());
        row[m_findcolumns.m_col_dist] = Conversions::dist_str(r.spheric_distance_nmi(m_center));
        row[m_findcolumns.m_col_icao] = "";
        row[m_findcolumns.m_col_name] = name;
        row[m_findcolumns.m_col_coord] = pt;
        row[m_findcolumns.m_col_alt] = std::numeric_limits<int>::min();
        row[m_findcolumns.m_col_flags] = 0;
        row[m_findcolumns.m_col_freq] = 0;
        row[m_findcolumns.m_col_comment] = "";
        row[m_findcolumns.m_col_type] = 0;
        set_row_display(row, name);
}

void Navigate::FindWindow::async_dispatchdone(void)
{
        m_querydispatch();
}

void Navigate::FindWindow::async_done(void)
{
        if ((m_airportquery || m_airportquery1) && (!m_airportquery || m_airportquery->is_done()) && (!m_airportquery1 || m_airportquery1->is_done())) {
                Engine::AirportResult::elementvector_t empty;
                const Engine::AirportResult::elementvector_t *v1 = &empty, *v2 = &empty;

                if (m_airportquery)
                        std::cerr << "Airportquery: error: " << m_airportquery->is_error() << std::endl;
                if (m_airportquery1)
                        std::cerr << "Airportquery1: error: " << m_airportquery1->is_error() << std::endl;

                if (m_airportquery && !m_airportquery->is_error())
                        v1 = &m_airportquery->get_result();
                if (m_airportquery1 && !m_airportquery1->is_error())
                        v2 = &m_airportquery1->get_result();
                set_objects(*v1, *v2);
                m_airportquery = Glib::RefPtr<Engine::AirportResult>();
                m_airportquery1 = Glib::RefPtr<Engine::AirportResult>();
        }
        if ((m_airspacequery || m_airspacequery1) && (!m_airspacequery || m_airspacequery->is_done()) && (!m_airspacequery1 || m_airspacequery1->is_done())) {
                Engine::AirspaceResult::elementvector_t empty;
                const Engine::AirspaceResult::elementvector_t *v1 = &empty, *v2 = &empty;
                if (m_airspacequery && !m_airspacequery->is_error())
                        v1 = &m_airspacequery->get_result();
                if (m_airspacequery1 && !m_airspacequery1->is_error())
                        v2 = &m_airspacequery1->get_result();
                set_objects(*v1, *v2);
                m_airspacequery = Glib::RefPtr<Engine::AirspaceResult>();
                m_airspacequery1 = Glib::RefPtr<Engine::AirspaceResult>();
        }
        if ((m_navaidquery || m_navaidquery1) && (!m_navaidquery || m_navaidquery->is_done()) && (!m_navaidquery1 || m_navaidquery1->is_done())) {
                Engine::NavaidResult::elementvector_t empty;
                const Engine::NavaidResult::elementvector_t *v1 = &empty, *v2 = &empty;
                if (m_navaidquery && !m_navaidquery->is_error())
                        v1 = &m_navaidquery->get_result();
                if (m_navaidquery1 && !m_navaidquery1->is_error())
                        v2 = &m_navaidquery1->get_result();
                set_objects(*v1, *v2);
                m_navaidquery = Glib::RefPtr<Engine::NavaidResult>();
                m_navaidquery1 = Glib::RefPtr<Engine::NavaidResult>();
        }
        if (m_waypointquery && m_waypointquery->is_done()) {
                if (!m_waypointquery->is_error())
                        set_objects(m_waypointquery->get_result());
                m_waypointquery = Glib::RefPtr<Engine::WaypointResult>();
        }
        if (m_airwayquery && m_airwayquery->is_done()) {
                if (!m_airwayquery->is_error())
                        set_objects(m_airwayquery->get_result());
                m_airwayquery = Glib::RefPtr<Engine::AirwayResult>();
        }
        if (m_mapelementquery && m_mapelementquery->is_done()) {
                if (!m_mapelementquery->is_error())
                        set_objects(m_mapelementquery->get_result());
                m_mapelementquery = Glib::RefPtr<Engine::MapelementResult>();
        }
}

void Navigate::FindWindow::async_cancel(void)
{
        Engine::AirportResult::cancel(m_airportquery);
        Engine::AirportResult::cancel(m_airportquery1);
        Engine::AirspaceResult::cancel(m_airspacequery);
        Engine::AirspaceResult::cancel(m_airspacequery1);
        Engine::NavaidResult::cancel(m_navaidquery);
        Engine::NavaidResult::cancel(m_navaidquery1);
        Engine::WaypointResult::cancel(m_waypointquery);
        Engine::AirwayResult::cancel(m_airwayquery);
        Engine::MapelementResult::cancel(m_mapelementquery);
}

Navigate::FindNameWindow::FindNameWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : FindWindow(castitem, refxml), m_entry(0), m_searchtype(0)
{
        get_widget(m_refxml, "findname_treeview", m_treeview);
        Gtk::Button *bcancel, *bclear, *bfind;
        get_widget(m_refxml, "findname_buttonok", m_buttonok);
        get_widget(m_refxml, "findname_buttoncancel", bcancel);
        get_widget(m_refxml, "findname_buttonclear", bclear);
        get_widget(m_refxml, "findname_buttonfind", bfind);
        get_widget(m_refxml, "findname_searchtype", m_searchtype);
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttonok_clicked));
        bcancel->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttoncancel_clicked));
        bclear->signal_clicked().connect(sigc::mem_fun(*this, &FindNameWindow::buttonclear_clicked));
        bfind->signal_clicked().connect(sigc::mem_fun(*this, &FindNameWindow::buttonfind_clicked));
        m_searchtype->signal_changed().connect(sigc::mem_fun(*this, &FindNameWindow::searchtype_changed));
        m_searchtype->set_active(0);
        get_widget(m_refxml, "findname_entry", m_entry);
        m_entry->signal_changed().connect(sigc::mem_fun(*this, &FindNameWindow::entry_changed));
        init_list();
}

Navigate::FindNameWindow::~FindNameWindow()
{
}

void Navigate::FindNameWindow::set_center(const Point & pt)
{
        FindWindow::set_center(pt);
        entry_changed();
}

void Navigate::FindNameWindow::buttonclear_clicked(void)
{
        m_entry->delete_text(0, std::numeric_limits<int>::max());
}

void Navigate::FindNameWindow::buttonfind_clicked(void)
{
        if (!m_engine)
                return;
        DbQueryInterfaceCommon::comp_t comp;
        FPlan::comp_t fpcomp;
        std::cerr << "Search type: " << m_searchtype->get_active_row_number() << std::endl;
        switch (m_searchtype->get_active_row_number()) {
	default:
	case 0:
		comp = DbQueryInterfaceCommon::comp_startswith;
		fpcomp = FPlan::comp_startswith;
		break;

	case 1:
		comp = DbQueryInterfaceCommon::comp_exact;
		fpcomp = FPlan::comp_exact;
		break;

	case 2:
		comp = DbQueryInterfaceCommon::comp_contains;
		fpcomp = FPlan::comp_contains;
		break;
        }
        clear_objects();
        Glib::ustring text(m_entry->get_text());
        {
                std::vector<FPlanWaypoint> wpts(m_engine->get_fplan_db().find_by_icao(text, FindWindow::line_limit / 2, 0, fpcomp));
                std::vector<FPlanWaypoint> wpts1(m_engine->get_fplan_db().find_by_name(text, FindWindow::line_limit / 2, 0, fpcomp));
                set_objects(wpts, wpts1);
        }
#if 1
        async_cancel();
        m_airportquery = m_engine->async_airport_find_by_icao(text, FindWindow::line_limit / 2, 0, comp, AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_comms);
        m_airportquery1 = m_engine->async_airport_find_by_name(text, FindWindow::line_limit / 2, 0, comp, AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_comms);
        m_navaidquery = m_engine->async_navaid_find_by_icao(text, FindWindow::line_limit / 2, 0, comp, NavaidsDb::element_t::subtables_none);
        m_navaidquery1 = m_engine->async_navaid_find_by_name(text, FindWindow::line_limit / 2, 0, comp, NavaidsDb::element_t::subtables_none);
        m_waypointquery = m_engine->async_waypoint_find_by_name(text, FindWindow::line_limit, 0, comp, WaypointsDb::element_t::subtables_none);
        m_airspacequery = m_engine->async_airspace_find_by_icao(text, FindWindow::line_limit / 2, 0, comp, AirspacesDb::element_t::subtables_none);
        m_airspacequery1 = m_engine->async_airspace_find_by_name(text, FindWindow::line_limit / 2, 0, comp, AirspacesDb::element_t::subtables_none);
        m_mapelementquery = m_engine->async_mapelement_find_by_name(text, FindWindow::line_limit, 0, comp, MapelementsDb::element_t::subtables_none);
        m_airwayquery = m_engine->async_airway_find_by_text(text, FindWindow::line_limit, 0, comp, AirwaysDb::element_t::subtables_none);
        if (m_airportquery) {
                m_airportquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airportquery1) {
                m_airportquery1->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airspacequery) {
                m_airspacequery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airspacequery1) {
                m_airspacequery1->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_navaidquery) {
                m_navaidquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_navaidquery1) {
                m_navaidquery1->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_waypointquery) {
                m_waypointquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_mapelementquery) {
                m_mapelementquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
        if (m_airwayquery) {
                m_airwayquery->connect(sigc::mem_fun(*this, &FindNameWindow::async_dispatchdone));
        }
#else
        {
                AirportsDb::elementvector_t palmairports(m_engine->airport_find_by_icao(text, FindWindow::line_limit / 2, 0, comp));
                AirportsDb::elementvector_t palmairports1(m_engine->airport_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmairports, palmairports1);
        }
        {
                NavaidsDb::elementvector_t palmnavaids(m_engine->navaid_find_by_icao(text, FindWindow::line_limit / 2, 0, comp));
                NavaidsDb::elementvector_t palmnavaids1(m_engine->navaid_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmnavaids, palmnavaids1);
        }
        {
                WaypointsDb::elementvector_t palmwaypoints(m_engine->waypoint_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmwaypoints);
        }
        {
                AirspacesDb::elementvector_t palmairspaces(m_engine->airspace_find_by_icao(text, FindWindow::line_limit / 2, 0, comp));
                AirspacesDb::elementvector_t palmairspaces1(m_engine->airspace_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmairspaces, palmairspaces1);
        }
        {
                MapelementsDb::elementvector_t palmmapelements(m_engine->mapelement_find_by_name(text, FindWindow::line_limit / 2, 0, comp));
                set_objects(palmmapelements);
        }
#endif
}

void Navigate::FindNameWindow::searchtype_changed(void)
{
        // do nothing
}

void Navigate::FindNameWindow::entry_changed(void)
{
        //buttonfind_clicked();
}

Navigate::FindNearestWindow::FindNearestWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : FindWindow(castitem, refxml), m_distance(0)
{
        get_widget(m_refxml, "findnearest_treeview", m_treeview);
        Gtk::Button *bcancel, *bfind;
        get_widget(m_refxml, "findnearest_buttonok", m_buttonok);
        get_widget(m_refxml, "findnearest_buttoncancel", bcancel);
        get_widget(m_refxml, "findnearest_buttonfind", bfind);
        get_widget(m_refxml, "findnearest_distance", m_distance);
        m_buttonok->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttonok_clicked));
        bcancel->signal_clicked().connect(sigc::mem_fun(*this, &FindWindow::buttoncancel_clicked));
        bfind->signal_clicked().connect(sigc::mem_fun(*this, &FindNearestWindow::buttonfind_clicked));
        m_distance->set_value(distance_limit);
        init_list();
}

Navigate::FindNearestWindow::~FindNearestWindow()
{
}


void Navigate::FindNearestWindow::buttonfind_clicked(void)
{
        if (!m_engine)
                return;
        std::cerr << "Distance: " << m_distance->get_value() << std::endl;
        Rect bbox(m_center.simple_box_nmi(m_distance->get_value()));
        clear_objects();
        {
                std::vector<FPlanWaypoint> wpts(m_engine->get_fplan_db().find_nearest(m_center, FindWindow::line_limit, 0, bbox));
                set_objects(wpts);
        }
#if 1
        async_cancel();
        m_airportquery = m_engine->async_airport_find_nearest(m_center, FindWindow::line_limit, bbox, AirportsDb::element_t::subtables_vfrroutes | AirportsDb::element_t::subtables_comms);
        m_airportquery1 = Glib::RefPtr<Engine::AirportResult>();
        m_navaidquery = m_engine->async_navaid_find_nearest(m_center, FindWindow::line_limit, bbox, NavaidsDb::element_t::subtables_none);
        m_navaidquery1 = Glib::RefPtr<Engine::NavaidResult>();
        m_waypointquery = m_engine->async_waypoint_find_nearest(m_center, FindWindow::line_limit, bbox, WaypointsDb::element_t::subtables_none);
        m_airspacequery = m_engine->async_airspace_find_nearest(m_center, FindWindow::line_limit, bbox, AirspacesDb::element_t::subtables_none);
        m_airspacequery1 = Glib::RefPtr<Engine::AirspaceResult>();
        m_mapelementquery = m_engine->async_mapelement_find_nearest(m_center, FindWindow::line_limit, bbox, MapelementsDb::element_t::subtables_none);
        m_airwayquery = m_engine->async_airway_find_nearest(m_center, FindWindow::line_limit, bbox, WaypointsDb::element_t::subtables_none);
        if (m_airportquery) {
                m_airportquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_airspacequery) {
                m_airspacequery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_navaidquery) {
                m_navaidquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_waypointquery) {
                m_waypointquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_mapelementquery) {
                m_mapelementquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
        if (m_airwayquery) {
                m_airwayquery->connect(sigc::mem_fun(*this, &FindNearestWindow::async_dispatchdone));
        }
#else
        {
                AirportsDb::elementvector_t palmairports(m_engine->airport_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmairports);
        }
        {
                NavaidsDb::elementvector_t palmnavaids(m_engine->navaid_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmnavaids);
        }
        {
                WaypointsDb::elementvector_t palmwaypoints(m_engine->waypoint_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmwaypoints);
        }
        {
                AirspacesDb::elementvector_t palmairspaces(m_engine->airspace_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmairspaces);
        }
        {
                MapelementsDb::elementvector_t palmmapelements(m_engine->mapelement_find_nearest(m_center, FindWindow::line_limit, bbox));
                set_objects(palmmapelements);
        }
#endif
}

void Navigate::FindNearestWindow::set_center(const Point & pt)
{
        FindWindow::set_center(pt);
}

Navigate::ConfirmUpdateWindow::WaypointListModelColumns::WaypointListModelColumns(void)
{
        add(m_col_icao);
        add(m_col_name);
        add(m_col_freq);
        add(m_col_lon);
        add(m_col_lat);
        add(m_col_alt);
        add(m_col_mt);
        add(m_col_tt);
        add(m_col_dist);
        add(m_col_vertspeed);
        add(m_col_truealt);
        add(m_col_nr);
        add(m_col_weight);
        add(m_col_color);
        add(m_col_style);
}

Navigate::ConfirmUpdateWindow::ConfirmUpdateWindow(BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml)
        : toplevel_window_t(castitem), m_button_duplicate(0), m_button_up(0), m_button_down(0), m_engine(0), m_route(0), m_confirmupdatewaypointlist(0), m_wptnr(0), m_wptdelete(false)
{
        get_widget(refxml, "confirmupdatewaypointlist", m_confirmupdatewaypointlist);
        Gtk::Button *bok, *bcancel;
        get_widget(refxml, "confirmupdatebuttonok", bok);
        get_widget(refxml, "confirmupdatebuttonduplicate", m_button_duplicate);
        get_widget(refxml, "confirmupdatebuttoncancel", bcancel);
        get_widget(refxml, "confirmupdatebuttonup", m_button_up);
        get_widget(refxml, "confirmupdatebuttondown", m_button_down);
        bok->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmUpdateWindow::buttonok_clicked));
        m_button_duplicate->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmUpdateWindow::buttonduplicate_clicked));
        bcancel->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmUpdateWindow::buttoncancel_clicked));
        m_button_up->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmUpdateWindow::buttonup_clicked));
        m_button_down->signal_clicked().connect(sigc::mem_fun(*this, &ConfirmUpdateWindow::buttondown_clicked));
        signal_hide().connect(sigc::mem_fun(*this, &Navigate::ConfirmUpdateWindow::on_my_hide));
        signal_show().connect(sigc::mem_fun(*this, &Navigate::ConfirmUpdateWindow::on_my_show));
        signal_delete_event().connect(sigc::mem_fun(*this, &Navigate::ConfirmUpdateWindow::on_my_delete_event));
        // Waypoint List
        m_waypointliststore = Gtk::ListStore::create(m_waypointlistcolumns);
        m_confirmupdatewaypointlist->set_model(m_waypointliststore);
        m_confirmupdatewaypointlist->append_column(_("ICAO"), m_waypointlistcolumns.m_col_icao);
        m_confirmupdatewaypointlist->append_column(_("Name"), m_waypointlistcolumns.m_col_name);
        m_confirmupdatewaypointlist->append_column(_("Freq"), m_waypointlistcolumns.m_col_freq);
        m_confirmupdatewaypointlist->append_column(_("Lon"), m_waypointlistcolumns.m_col_lon);
        m_confirmupdatewaypointlist->append_column(_("Lat"), m_waypointlistcolumns.m_col_lat);
        m_confirmupdatewaypointlist->append_column(_("Alt"), m_waypointlistcolumns.m_col_alt);
        m_confirmupdatewaypointlist->append_column(_("MT"), m_waypointlistcolumns.m_col_mt);
        m_confirmupdatewaypointlist->append_column(_("TT"), m_waypointlistcolumns.m_col_tt);
        m_confirmupdatewaypointlist->append_column(_("Dist"), m_waypointlistcolumns.m_col_dist);
        m_confirmupdatewaypointlist->append_column(_("VertS"), m_waypointlistcolumns.m_col_vertspeed);
        m_confirmupdatewaypointlist->append_column(_("TrueAlt"), m_waypointlistcolumns.m_col_truealt);
        for (unsigned int i = 0; i < 11; ++i) {
                Gtk::TreeViewColumn *col = m_confirmupdatewaypointlist->get_column(i);
                if (!col)
                        continue;
                col->set_resizable(true);
                col->set_reorderable(true);
                col->set_expand(i == 1);
                Gtk::CellRenderer *renderer = m_confirmupdatewaypointlist->get_column_cell_renderer(i);
                if (!renderer)
                        continue;
                col->add_attribute(*renderer, "weight", m_waypointlistcolumns.m_col_weight);
                col->add_attribute(*renderer, "foreground-gdk", m_waypointlistcolumns.m_col_color);
                col->add_attribute(*renderer, "style", m_waypointlistcolumns.m_col_style);
        }
        for (unsigned int i = 5; i < 11; ++i) {
                Gtk::CellRenderer *renderer = m_confirmupdatewaypointlist->get_column_cell_renderer(i);
                if (!renderer)
                        continue;
                renderer->set_property("xalign", 1.0);
        }
        // selection
        {
                Glib::RefPtr<Gtk::TreeSelection> waypointlist_selection = m_confirmupdatewaypointlist->get_selection();
                waypointlist_selection->set_mode(Gtk::SELECTION_NONE);
        }
}

Navigate::ConfirmUpdateWindow::~ConfirmUpdateWindow()
{
}

void Navigate::ConfirmUpdateWindow::set_engine(Engine & eng)
{
        m_engine = &eng;
}

void Navigate::ConfirmUpdateWindow::on_my_hide()
{
        m_waypointliststore->clear();
}

void Navigate::ConfirmUpdateWindow::on_my_show()
{
        m_waypointliststore->clear();
        if (!m_engine || !m_route || !m_route->get_id()) {
                m_button_up->set_sensitive(false);
                m_button_down->set_sensitive(false);
                return;
        }
        m_button_up->set_sensitive(m_wptnr >= 2);
        m_button_down->set_sensitive(m_route && ((m_wptnr + 1 < m_route->get_nrwpt()) || ((m_wptnr < m_route->get_nrwpt()) && !m_wptdelete)));
        Gdk::Color black, red, green;
        black.set_grey(0);
        red.set_rgb(0xffff, 0, 0);
        green.set_rgb(0, 0xffff, 0);
#ifdef HAVE_GTKMM2
        {
                Glib::RefPtr<Gdk::Colormap> colmap(Gdk::Colormap::get_system());
                colmap->alloc_color(black);
                colmap->alloc_color(red);
                colmap->alloc_color(green);
        }
#endif
        Gtk::TreeModel::Row row(*(m_waypointliststore->append()));
        row[m_waypointlistcolumns.m_col_nr] = 0;
        row[m_waypointlistcolumns.m_col_icao] = "";
        row[m_waypointlistcolumns.m_col_name] = "Off Block";
        row[m_waypointlistcolumns.m_col_freq] = "";
        row[m_waypointlistcolumns.m_col_lon] = "";
        row[m_waypointlistcolumns.m_col_lat] = "";
        row[m_waypointlistcolumns.m_col_alt] = "";
        row[m_waypointlistcolumns.m_col_mt] = "";
        row[m_waypointlistcolumns.m_col_tt] = "";
        row[m_waypointlistcolumns.m_col_dist] = "";
        row[m_waypointlistcolumns.m_col_vertspeed] = "";
        row[m_waypointlistcolumns.m_col_truealt] = "";
        row[m_waypointlistcolumns.m_col_weight] = 0;
        row[m_waypointlistcolumns.m_col_color] = black;
        row[m_waypointlistcolumns.m_col_style] = Pango::STYLE_NORMAL;
        WMM wmm;
        time_t curtime = time(0);
        Preferences& prefs(m_engine->get_prefs());
        {
                std::vector<FPlanWaypoint> wpts;
                for (unsigned int nr = 0; nr < m_route->get_nrwpt(); nr++)
                        wpts.push_back((*m_route)[nr]);
                m_wptnr = std::min(m_wptnr, (uint16_t)wpts.size());
                if (!m_wptdelete)
                        wpts.insert(wpts.begin() + m_wptnr, m_newwpts.begin(), m_newwpts.end());
                for (unsigned int nr = 0; nr < wpts.size(); nr++) {
                        const FPlanWaypoint& wpt(wpts[nr]);
			IcaoAtmosphere<float> qnh(wpt.get_qff_hpa(), wpt.get_isaoffset_kelvin() + IcaoAtmosphere<float>::std_sealevel_temperature);
                        row = *(m_waypointliststore->append());
                        row[m_waypointlistcolumns.m_col_nr] = nr + 1;
                        row[m_waypointlistcolumns.m_col_icao] = wpt.get_icao();
                        row[m_waypointlistcolumns.m_col_name] = wpt.get_name();
                        row[m_waypointlistcolumns.m_col_freq] = Conversions::freq_str(wpt.get_frequency());
                        row[m_waypointlistcolumns.m_col_lon] = wpt.get_coord().get_lon_str();
                        row[m_waypointlistcolumns.m_col_lat] = wpt.get_coord().get_lat_str();
                        row[m_waypointlistcolumns.m_col_alt] = Conversions::alt_str(wpt.get_altitude(), wpt.get_flags());
                        int16_t wpttruealt = FPlanLeg::baro_correction(wpt.get_qff_hpa(), wpt.get_isaoffset_kelvin(), wpt.get_altitude(), wpt.get_flags());
                        row[m_waypointlistcolumns.m_col_truealt] = Conversions::alt_str(wpttruealt, 0);
                        unsigned int nr2(nr + 1);
                        if (m_wptdelete && nr == m_wptnr)
                                ++nr2;
                        if (nr2 < wpts.size()) {
                                const FPlanWaypoint& wpt2(wpts[nr2]);
                                Point pt1(wpt.get_coord()), pt2(wpt2.get_coord());
                                float dist = pt1.spheric_distance_nmi(pt2);
                                float tt = pt1.spheric_true_course(pt2);
                                wmm.compute((wpt.get_altitude() + wpt2.get_altitude()) * (FPlanWaypoint::ft_to_m / 2000.0), pt1.halfway(pt2), curtime);
                                float mt = tt;
                                if (wmm)
                                        mt -= wmm.get_dec();
                                int16_t wpt2truealt = FPlanLeg::baro_correction(wpt2.get_qff_hpa(), wpt2.get_isaoffset_kelvin(), wpt2.get_altitude(), wpt2.get_flags());
                                float legtime = 60.0f * dist / std::max(1.0f, (float)prefs.vcruise);
                                float vspeed = (wpt2truealt - wpttruealt) / std::max(1.0f / 60.0f, legtime);
                                row[m_waypointlistcolumns.m_col_mt] = Conversions::track_str(mt);
                                row[m_waypointlistcolumns.m_col_tt] = Conversions::track_str(tt);
                                row[m_waypointlistcolumns.m_col_dist] = Conversions::dist_str(dist);
                                row[m_waypointlistcolumns.m_col_vertspeed] = Conversions::verticalspeed_str(vspeed);
                        } else {
                                row[m_waypointlistcolumns.m_col_mt] = "";
                                row[m_waypointlistcolumns.m_col_tt] = "";
                                row[m_waypointlistcolumns.m_col_dist] = "";
                                row[m_waypointlistcolumns.m_col_vertspeed] = "";
                        }
                        if (m_wptdelete && nr == m_wptnr) {
                                row[m_waypointlistcolumns.m_col_weight] = 1;
                                row[m_waypointlistcolumns.m_col_color] = red;
                                row[m_waypointlistcolumns.m_col_style] = Pango::STYLE_ITALIC;
                        } else if (!m_wptdelete && nr >= m_wptnr && nr < m_wptnr + m_newwpts.size()) {
                                row[m_waypointlistcolumns.m_col_weight] = 1;
                                row[m_waypointlistcolumns.m_col_color] = green;
                                row[m_waypointlistcolumns.m_col_style] = Pango::STYLE_ITALIC;
                        } else {
                                row[m_waypointlistcolumns.m_col_weight] = 0;
                                row[m_waypointlistcolumns.m_col_color] = black;
                                row[m_waypointlistcolumns.m_col_style] = Pango::STYLE_NORMAL;
                        }
                }
                row = *(m_waypointliststore->append());
                row[m_waypointlistcolumns.m_col_nr] = wpts.size() + 1;
                row[m_waypointlistcolumns.m_col_icao] = "";
                row[m_waypointlistcolumns.m_col_name] = "On Block";
                row[m_waypointlistcolumns.m_col_freq] = "";
                row[m_waypointlistcolumns.m_col_lon] = "";
                row[m_waypointlistcolumns.m_col_lat] = "";
                row[m_waypointlistcolumns.m_col_alt] = "";
                row[m_waypointlistcolumns.m_col_mt] = "";
                row[m_waypointlistcolumns.m_col_tt] = "";
                row[m_waypointlistcolumns.m_col_dist] = "";
                row[m_waypointlistcolumns.m_col_vertspeed] = "";
                row[m_waypointlistcolumns.m_col_truealt] = "";
                row[m_waypointlistcolumns.m_col_weight] = 0;
                row[m_waypointlistcolumns.m_col_color] = black;
                row[m_waypointlistcolumns.m_col_style] = Pango::STYLE_NORMAL;
        }
}

bool Navigate::ConfirmUpdateWindow::on_my_delete_event(GdkEventAny *event)
{
        hide();
        return true;
}

void Navigate::ConfirmUpdateWindow::buttonok_clicked(void)
{
        hide();
        signal_update()(false, m_wptnr, m_wptdelete, m_newwpts);
}

void Navigate::ConfirmUpdateWindow::buttonduplicate_clicked(void)
{
        hide();
        signal_update()(true, m_wptnr, m_wptdelete, m_newwpts);
}

void Navigate::ConfirmUpdateWindow::buttoncancel_clicked(void)
{
        hide();
}

void Navigate::ConfirmUpdateWindow::buttonup_clicked(void)
{
        if (m_wptnr <= 1)
                return;
        --m_wptnr;
        on_my_show();
}

void Navigate::ConfirmUpdateWindow::buttondown_clicked(void)
{
        if (!m_route)
                return;
        if ((m_wptdelete && (m_wptnr + 2 >= m_route->get_nrwpt())) || (m_wptnr + 1 >= m_route->get_nrwpt()))
                return;
        ++m_wptnr;
        on_my_show();
}

void Navigate::ConfirmUpdateWindow::set_enableduplicate(bool dupl)
{
        if (dupl)
                m_button_duplicate->show();
        else
                m_button_duplicate->hide();
}

bool Navigate::ConfirmUpdateWindow::get_enableduplicate(void) const
{
#ifdef HAVE_GTKMM2
        return m_button_duplicate->is_visible();
#else
        return m_button_duplicate->get_visible();
#endif
}

Navigate::Navigate(Engine& eng, Glib::RefPtr<PrefsWindow>& prefswindow)
        : m_engine(eng), m_route(m_engine.get_fplan_db()), m_route_unmodified(true), m_prefswindow(prefswindow), m_lastgpstime(0), m_gps(new GPSComm()),
          m_navwaypointwindow(0), m_navlegwindow(0), m_navvmapwindow(0), m_navterrainwindow(0), m_navairportdiagramwindow(0), m_navpoiwindow(0),
          m_gpsinfowindow(0), m_navinfowindow(0), m_findnamewindow(0), m_findnearestwindow(0), m_confirmupdatewindow(0), m_navwaypointtreeview(0),
          m_gpsinfopane(0), m_gpsinfoazelev(0), m_gpsinfosigstrength(0), m_gpsinfoalt(0), m_gpsinfolon(0), m_gpsinfolat(0),
          m_gpsinfovgnd(0), m_gpsinfohdop(0), m_gpsinfovdop(0), m_gpsinfodgps(0), m_gpsinfogps(0),
          m_navvmaparea(0), m_navvmapmenuzoomin(0), m_navvmapmenuzoomout(0),
          m_navvmapinfohdg(0), m_navvmapinfodist(0), m_navvmapinfovs(0), m_navvmapinfoname(0),
          m_navvmapinfotarghdg(0), m_navvmapinfotargdist(0), m_navvmapinfotargvs(0), m_navvmapinfogps(0), m_navvmapinfovgnd(0),
          m_navlegfromname(0), m_navlegfromalt(0), m_navlegtoname(0), m_navlegtoalt(0),
          m_navlegcurrenttt(0), m_navlegcurrentmt(0), m_navlegcurrenthdg(0), m_navlegcurrentdist(0),
          m_navlegnexttt(0), m_navlegnextmt(0), m_navlegnexthdg(0), m_navlegnextdist(0),
          m_navlegmaplon(0), m_navlegmaplat(0), m_navlegalt(0), m_navleggpshdg(0), m_navlegvgnd(0),
          m_navlegcrosstrack(0), m_navlegcoursetodest(0), m_navlegreferencealt(0), m_navlegaltdiff(0), m_navleghdgtodest(0), m_navlegdisttodest(0),
          m_navlegprogress(0), m_navlegcdi(0), m_navlegmaplonlabel(0), m_navlegmaplatlabel(0), m_navlegaltlabel(0),
          m_navterraininfohdg(0), m_navterraininfodist(0), m_navterraininfovs(0), m_navterraininfoname(0),
          m_navterraininfotarghdg(0), m_navterraininfotargdist(0), m_navterraininfotargvs(0), m_navterraininfogps(0), m_navterraininfovgnd(0),
          m_navairportdiagramarea(0), m_navairportdiagrammenuzoomin(0), m_navairportdiagrammenuzoomout(0),
          m_navairportdiagraminfohdg(0), m_navairportdiagraminfodist(0), m_navairportdiagraminfovs(0), m_navairportdiagraminfoname(0), 
          m_navairportdiagraminfotarghdg(0), m_navairportdiagraminfotargdist(0), m_navairportdiagraminfotargvs(0), 
          m_navairportdiagraminfogps(0), m_navairportdiagraminfovgnd(0),
          m_navpoiinfohdg(0), m_navpoiinfodist(0), m_navpoiinfovs(0), m_navpoiinfoname(0),
          m_navpoiinfotarghdg(0), m_navpoiinfotargdist(0), m_navpoiinfotargvs(0), m_navpoiinfogps(0), m_navpoiinfovgnd(0),
          m_aboutdialog(0)
{
        m_gps->connect(sigc::mem_fun(*this, &Navigate::update_gps));
}

Navigate::~Navigate()
{
        end_nav();
        delete m_gps;
}

#if defined(HAVE_LIBGPS) || defined(HAVE_GYPSY) || defined(HAVE_LIBLOCATION)

void Navigate::open_gps(void)
{
        if (!m_engine.get_prefs().gps) {
                close_gps();
                return;
        }
        GPSComm *g = GPSCommGPSD::create(m_engine.get_prefs().gpshost, m_engine.get_prefs().gpsport);
        if (g) {
                delete m_gps;
                m_gps = g;
                m_gps->connect(sigc::mem_fun(*this, &Navigate::update_gps));
                update_gps();
        }
}

void Navigate::close_gps(void)
{
        delete m_gps;
        m_gps = new GPSComm();
        m_gps->connect(sigc::mem_fun(*this, &Navigate::update_gps));
        update_gps();
}

#else

void Navigate::open_gps(void)
{
}

void Navigate::close_gps(void)
{
}

#endif

void Navigate::update_gps(void)
{
        //std::cerr << "update_gps time " << time(0) << std::endl;
        uint64_t fixtime((uint64_t)ldexp(m_gps->get_fixtime(), 8));
        if (abs((int64_t)(m_lastgpstime - fixtime)) < 192)
                return;
        m_lastgpstime = fixtime;
        bool coordvalid(m_gps->is_coord_valid() && (fixtime < (2050000000ULL << 8)));
        bool altvalid(m_gps->is_altitude_valid() && coordvalid);
        Point coord;
        if (coordvalid)
                coord = m_gps->get_coord();
        float alt(0);
        if (altvalid)
                alt = m_gps->get_altitude() * FPlanWaypoint::m_to_ft;
        if (coordvalid && m_track.is_valid()) {
                TracksDb::Track::TrackPoint tp;
                tp.set_timefrac8(fixtime);
                tp.set_coord(coord);
                if (altvalid)
                        tp.set_alt((TracksDb::Track::TrackPoint::alt_t)alt);
                else
                        tp.set_alt_invalid();
                m_track.append(tp);
                if (m_track.get_unsaved_points() >= 60)
                        m_engine.get_tracks_db().save_appendlog(m_track);
        }
        if (m_gpsinfowindow) {
                bool stale = (fixtime + 10) < time(0);
                if (coordvalid) {
                        m_gpsinfolon->set_text(coord.get_lon_str());
                        m_gpsinfolat->set_text(coord.get_lat_str());
                        char buf1[16], buf2[16];
                        snprintf(buf1, sizeof(buf1), "%f", m_gps->get_velocity() * Point::mps_to_kts);
                        snprintf(buf2, sizeof(buf2), "%f", m_gps->get_hdop());
                        m_gpsinfovgnd->set_text(buf1);
                        m_gpsinfohdop->set_text(buf2);
                } else {
                        m_gpsinfolon->set_text(stale ? "STALE" : "--");
                        m_gpsinfolat->set_text(stale ? "STALE" : "--");
                        m_gpsinfovgnd->set_text(stale ? "STALE" : "--");
                        m_gpsinfohdop->set_text(stale ? "STALE" : "--");
                }
                if (altvalid) {
                        char buf1[16], buf2[16];
                        snprintf(buf1, sizeof(buf1), "%f", alt);
                        snprintf(buf2, sizeof(buf2), "%f", m_gps->get_vdop() * FPlanWaypoint::m_to_ft);
                        m_gpsinfoalt->set_text(buf1);
                        m_gpsinfovdop->set_text(buf2);
                } else {
                        m_gpsinfoalt->set_text(stale ? "STALE" : "--");
                        m_gpsinfovdop->set_text(stale ? "STALE" : "--");
                }
                switch (m_gps->get_fixtype()) {
                        default:
                        case GPSComm::fixtype_noconn:
                                m_gpsinfogps->set_text("NOCONN");
                                break;

                        case GPSComm::fixtype_nofix:
                                m_gpsinfogps->set_text("NOFIX");
                                break;

                        case GPSComm::fixtype_2d:
                                m_gpsinfogps->set_text("2D");
                                break;

                        case GPSComm::fixtype_3d:
                                m_gpsinfogps->set_text("3D");
                                break;
                }
                switch (m_gps->get_fixstatus()) {
                        default:
                        case GPSComm::fixstatus_nofix:
                                m_gpsinfodgps->set_text("NOFIX");
                                break;

                        case GPSComm::fixstatus_fix:
                                m_gpsinfodgps->set_text("FIX");
                                break;

                        case GPSComm::fixstatus_dgpsfix:
                                m_gpsinfodgps->set_text("DGPS");
                                break;
                }
                GPSComm::SVVector sv = m_gps->get_svvector();
                m_gpsinfoazelev->update(sv);
                m_gpsinfosigstrength->update(sv);
        }
}

void Navigate::bind_menubar(const Glib::ustring& wndname, Gtk::Window *wnd)
{
        Gtk::MenuItem *mi;
        mi = 0;
        get_widget(m_refxml, wndname + "next", mi);
        if (mi) {
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_nextwpt));
                m_menu_nextwpt.push_back(mi);
        }
        mi = 0;
        get_widget(m_refxml, wndname + "prev", mi);
        if (mi) {
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_prevwpt));
                m_menu_prevwpt.push_back(mi);
        }
        mi = 0;
        get_widget(m_refxml, wndname + "insertname", mi);
        if (mi) {
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_insertbyname));
                m_menu_insertbyname.push_back(mi);
        }
        mi = 0;
        get_widget(m_refxml, wndname + "insertnearest", mi);
        if (mi) {
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_insertnearest));
                m_menu_insertnearest.push_back(mi);
        }
        mi = 0;
        get_widget(m_refxml, wndname + "insertcurrentpos", mi);
        if (mi) {
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_insertcurrentpos));
                m_menu_insertcurrentpos.push_back(mi);
        }
        mi = 0;
        get_widget(m_refxml, wndname + "deletenextwpt", mi);
        if (mi) {
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_deletenext));
                m_menu_deletenextwpt.push_back(mi);
        }
        mi = 0;
        get_widget(m_refxml, wndname + "restart", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_restartnav));
        mi = 0;
        get_widget(m_refxml, wndname + "endnav", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::end_nav));
        mi = 0;
        get_widget(m_refxml, wndname + "viewwptinfo", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_viewwptinfo));
        mi = 0;
        get_widget(m_refxml, wndname + "vectormap", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_viewvmap));
        mi = 0;
        get_widget(m_refxml, wndname + "nearestpoi", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_viewnearestpoi));
        mi = 0;
        get_widget(m_refxml, wndname + "gpsinfo", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_viewgpsinfo));
        mi = 0;
        get_widget(m_refxml, wndname + "leg", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_viewcurrentleg));
        mi = 0;
        get_widget(m_refxml, wndname + "terrain", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_viewterrain));
        mi = 0;
        get_widget(m_refxml, wndname + "prefs", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_viewprefs));
        mi = 0;
        get_widget(m_refxml, wndname + "cut", mi);
        if (mi)
                mi->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::menu_cut), m_navwaypointwindow));
        mi = 0;
        get_widget(m_refxml, wndname + "copy", mi);
        if (mi)
                mi->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::menu_copy), m_navwaypointwindow));
        mi = 0;
        get_widget(m_refxml, wndname + "paste", mi);
        if (mi)
                mi->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::menu_paste), m_navwaypointwindow));
        mi = 0;
        get_widget(m_refxml, wndname + "delete", mi);
        if (mi)
                mi->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::menu_delete), m_navwaypointwindow));
        mi = 0;
        get_widget(m_refxml, wndname + "createwaypoint", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_createwaypoint));
        mi = 0;
        get_widget(m_refxml, wndname + "about", mi);
        if (mi)
                mi->signal_activate().connect(sigc::mem_fun(*this, &Navigate::menu_about));
}

void Navigate::start_nav(FPlanRoute::id_t id)
{
        if (!m_refxml) {
                // load glade file
                m_refxml = load_glade_file("navigate" UIEXT);
                // construct windows
                get_widget_derived(m_refxml, "navinfowindow", m_navinfowindow);
                // Find Windows
                get_widget_derived(m_refxml, "findnamewindow", m_findnamewindow);
                m_findnamewindow->set_engine(m_engine);
                m_findnamewindow->signal_setwaypoints().connect(sigc::mem_fun(*this, &Navigate::find_setwpts));
                m_findnamewindow->hide();
                get_widget_derived(m_refxml, "findnearestwindow", m_findnearestwindow);
                m_findnearestwindow->set_engine(m_engine);
                m_findnearestwindow->signal_setwaypoints().connect(sigc::mem_fun(*this, &Navigate::find_setwpts));
                m_findnearestwindow->hide();
                // Confirm Update Window
                get_widget_derived(m_refxml, "confirmupdatewindow", m_confirmupdatewindow);
                m_confirmupdatewindow->hide();
                m_confirmupdatewindow->set_engine(m_engine);
                m_confirmupdatewindow->set_route(m_route);
                m_confirmupdatewindow->signal_update().connect(sigc::mem_fun(*this, &Navigate::update_save_route));
                get_widget_derived(m_refxml, "navwaypointwindow", m_navwaypointwindow);
                get_widget(m_refxml, "navwaypointtreeview", m_navwaypointtreeview);
                get_widget_derived(m_refxml, "gpsinfowindow", m_gpsinfowindow);
                //get_widget(m_refxml, "gpsinfopane", m_gpsinfopane);
                get_widget_derived(m_refxml, "gpsinfoazelev", m_gpsinfoazelev);
                get_widget_derived(m_refxml, "gpsinfosigstrength", m_gpsinfosigstrength);
                get_widget(m_refxml, "gpsinfoalt", m_gpsinfoalt);
                get_widget(m_refxml, "gpsinfolon", m_gpsinfolon);
                get_widget(m_refxml, "gpsinfolat", m_gpsinfolat);
                get_widget(m_refxml, "gpsinfovgnd", m_gpsinfovgnd);
                get_widget(m_refxml, "gpsinfohdop", m_gpsinfohdop);
                get_widget(m_refxml, "gpsinfovdop", m_gpsinfovdop);
                get_widget(m_refxml, "gpsinfodgps", m_gpsinfodgps);
                get_widget(m_refxml, "gpsinfogps", m_gpsinfogps);
                get_widget_derived(m_refxml, "navvmapwindow", m_navvmapwindow);
                get_widget_derived(m_refxml, "navvmaparea", m_navvmaparea);
                get_widget(m_refxml, "navvmapmenuzoomin", m_navvmapmenuzoomin);
                get_widget(m_refxml, "navvmapmenuzoomout", m_navvmapmenuzoomout);
                get_widget(m_refxml, "navvmapinfohdg", m_navvmapinfohdg);
                get_widget(m_refxml, "navvmapinfodist", m_navvmapinfodist);
                get_widget(m_refxml, "navvmapinfovs", m_navvmapinfovs);
                get_widget(m_refxml, "navvmapinfoname", m_navvmapinfoname);
                get_widget(m_refxml, "navvmapinfotarghdg", m_navvmapinfotarghdg);
                get_widget(m_refxml, "navvmapinfotargdist", m_navvmapinfotargdist);
                get_widget(m_refxml, "navvmapinfotargvs", m_navvmapinfotargvs);
                get_widget(m_refxml, "navvmapinfogps", m_navvmapinfogps);
                get_widget(m_refxml, "navvmapinfovgnd", m_navvmapinfovgnd);
                get_widget(m_refxml, "aboutdialog", m_aboutdialog);
                get_widget_derived(m_refxml, "navlegwindow", m_navlegwindow);
                get_widget(m_refxml, "navlegfromname", m_navlegfromname);
                get_widget(m_refxml, "navlegfromalt", m_navlegfromalt);
                get_widget(m_refxml, "navlegtoname", m_navlegtoname);
                get_widget(m_refxml, "navlegtoalt", m_navlegtoalt);
                get_widget(m_refxml, "navlegcurrenttt", m_navlegcurrenttt);
                get_widget(m_refxml, "navlegcurrentmt", m_navlegcurrentmt);
                get_widget(m_refxml, "navlegcurrenthdg", m_navlegcurrenthdg);
                get_widget(m_refxml, "navlegcurrentdist", m_navlegcurrentdist);
                get_widget(m_refxml, "navlegnexttt", m_navlegnexttt);
                get_widget(m_refxml, "navlegnextmt", m_navlegnextmt);
                get_widget(m_refxml, "navlegnexthdg", m_navlegnexthdg);
                get_widget(m_refxml, "navlegnextdist", m_navlegnextdist);
                get_widget(m_refxml, "navlegmaplon", m_navlegmaplon);
                get_widget(m_refxml, "navlegmaplat", m_navlegmaplat);
                get_widget(m_refxml, "navlegalt", m_navlegalt);
                get_widget(m_refxml, "navleggpshdg", m_navleggpshdg);
                get_widget(m_refxml, "navlegvgnd", m_navlegvgnd);
                get_widget(m_refxml, "navlegcrosstrack", m_navlegcrosstrack);
                get_widget(m_refxml, "navlegcoursetodest", m_navlegcoursetodest);
                get_widget(m_refxml, "navlegreferencealt", m_navlegreferencealt);
                get_widget(m_refxml, "navlegaltdiff", m_navlegaltdiff);
                get_widget(m_refxml, "navleghdgtodest", m_navleghdgtodest);
                get_widget(m_refxml, "navlegdisttodest", m_navlegdisttodest);
                get_widget(m_refxml, "navlegprogress", m_navlegprogress);
                get_widget_derived(m_refxml, "navlegcdi", m_navlegcdi);
                get_widget(m_refxml, "navlegmaplonlabel", m_navlegmaplonlabel);
                get_widget(m_refxml, "navlegmaplatlabel", m_navlegmaplatlabel);
                get_widget(m_refxml, "navlegaltlabel", m_navlegaltlabel);
                get_widget_derived(m_refxml, "navterrainwindow", m_navterrainwindow);
                get_widget_derived(m_refxml, "navterrainarea", m_navterrainarea);
                get_widget(m_refxml, "navterrainmenuzoomin", m_navterrainmenuzoomin);
                get_widget(m_refxml, "navterrainmenuzoomout", m_navterrainmenuzoomout);
                get_widget(m_refxml, "navterraininfohdg", m_navterraininfohdg);
                get_widget(m_refxml, "navterraininfodist", m_navterraininfodist);
                get_widget(m_refxml, "navterraininfovs", m_navterraininfovs);
                get_widget(m_refxml, "navterraininfoname", m_navterraininfoname);
                get_widget(m_refxml, "navterraininfotarghdg", m_navterraininfotarghdg);
                get_widget(m_refxml, "navterraininfotargdist", m_navterraininfotargdist);
                get_widget(m_refxml, "navterraininfotargvs", m_navterraininfotargvs);
                get_widget(m_refxml, "navterraininfogps", m_navterraininfogps);
                get_widget(m_refxml, "navterraininfovgnd", m_navterraininfovgnd);
                get_widget_derived(m_refxml, "navairportdiagramwindow", m_navairportdiagramwindow);
                get_widget_derived(m_refxml, "navairportdiagramarea", m_navairportdiagramarea);
                get_widget(m_refxml, "navairportdiagrammenuzoomin", m_navairportdiagrammenuzoomin);
                get_widget(m_refxml, "navairportdiagrammenuzoomout", m_navairportdiagrammenuzoomout);
                get_widget(m_refxml, "navairportdiagraminfohdg", m_navairportdiagraminfohdg);
                get_widget(m_refxml, "navairportdiagraminfodist", m_navairportdiagraminfodist);
                get_widget(m_refxml, "navairportdiagraminfovs", m_navairportdiagraminfovs);
                get_widget(m_refxml, "navairportdiagraminfoname", m_navairportdiagraminfoname);
                get_widget(m_refxml, "navairportdiagraminfotarghdg", m_navairportdiagraminfotarghdg);
                get_widget(m_refxml, "navairportdiagraminfotargdist", m_navairportdiagraminfotargdist);
                get_widget(m_refxml, "navairportdiagraminfotargvs", m_navairportdiagraminfotargvs);
                get_widget(m_refxml, "navairportdiagraminfogps", m_navairportdiagraminfogps);
                get_widget(m_refxml, "navairportdiagraminfovgnd", m_navairportdiagraminfovgnd);
                get_widget_derived(m_refxml, "navpoiwindow", m_navpoiwindow);
                get_widget(m_refxml, "navpoiinfohdg", m_navpoiinfohdg);
                get_widget(m_refxml, "navpoiinfodist", m_navpoiinfodist);
                get_widget(m_refxml, "navpoiinfovs", m_navpoiinfovs);
                get_widget(m_refxml, "navpoiinfoname", m_navpoiinfoname);
                get_widget(m_refxml, "navpoiinfotarghdg", m_navpoiinfotarghdg);
                get_widget(m_refxml, "navpoiinfotargdist", m_navpoiinfotargdist);
                get_widget(m_refxml, "navpoiinfotargvs", m_navpoiinfotargvs);
                get_widget(m_refxml, "navpoiinfogps", m_navpoiinfogps);
                get_widget(m_refxml, "navpoiinfovgnd", m_navpoiinfovgnd);
                // delete handlers etc.
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->add_window(*m_navinfowindow);
                Hildon::Program::get_instance()->add_window(*m_findnamewindow);
                Hildon::Program::get_instance()->add_window(*m_findnearestwindow);
                Hildon::Program::get_instance()->add_window(*m_confirmupdatewindow);
                Hildon::Program::get_instance()->add_window(*m_navlegwindow);
                Hildon::Program::get_instance()->add_window(*m_navvmapwindow);
                Hildon::Program::get_instance()->add_window(*m_gpsinfowindow);
                Hildon::Program::get_instance()->add_window(*m_navwaypointwindow);
                Hildon::Program::get_instance()->add_window(*m_navterrainwindow);
                Hildon::Program::get_instance()->add_window(*m_navairportdiagramwindow);
                Hildon::Program::get_instance()->add_window(*m_navpoiwindow);
#endif
                m_navinfowindow->hide();
                m_findnamewindow->hide();
                m_findnearestwindow->hide();
                m_confirmupdatewindow->hide();
                m_navwaypointwindow->hide();
                m_gpsinfowindow->hide();
                m_navvmapwindow->hide();
                m_navlegwindow->hide();
                m_navterrainwindow->hide();
                m_navairportdiagramwindow->hide();
                m_navpoiwindow->hide();
                m_navinfowindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_navinfowindow));
                m_navwaypointwindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_navwaypointwindow));
                m_gpsinfowindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_gpsinfowindow));
                m_navvmapwindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_navvmapwindow));
                m_navlegwindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_navlegwindow));
                m_navterrainwindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_navterrainwindow));
                m_navairportdiagramwindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_navairportdiagramwindow));
                m_navpoiwindow->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&Navigate::window_delete_event), m_navpoiwindow));
                m_navwaypointwindow->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::window_key_press_event), m_navwaypointwindow));
                m_gpsinfowindow->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::window_key_press_event), m_gpsinfowindow));
                m_navvmapwindow->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::window_key_press_event), m_navvmapwindow));
                m_navlegwindow->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::window_key_press_event), m_navlegwindow));
                m_navterrainwindow->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::window_key_press_event), m_navterrainwindow));
                m_navairportdiagramwindow->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::window_key_press_event), m_navairportdiagramwindow));
                m_navpoiwindow->signal_key_press_event().connect(sigc::bind(sigc::mem_fun(*this, &Navigate::window_key_press_event), m_navpoiwindow));
                m_aboutdialog->hide();
                m_aboutdialog->signal_response().connect(sigc::mem_fun(*this, &Navigate::aboutdialog_response));
                m_navpoiwindow->set_engine(m_engine);
                m_navvmaparea->set_engine(m_engine);
                m_navvmaparea->set_route(m_route);
                m_navvmaparea->set_track(&m_track);
                m_navvmaparea->set_renderer(VectorMapArea::renderer_vmap);
                m_navvmaparea->set_map_scale(m_engine.get_prefs().mapscale);
                m_navvmapwindow->signal_hide().connect(sigc::mem_fun(*m_navvmaparea, &Navigate::NavVectorMapArea::hide));
                m_navvmapwindow->signal_show().connect(sigc::mem_fun(*m_navvmaparea, &Navigate::NavVectorMapArea::show));
                m_navvmapwindow->signal_zoomin().connect(sigc::bind_return(sigc::mem_fun(*m_navvmaparea, &Navigate::NavVectorMapArea::zoom_in), true));
                m_navvmapwindow->signal_zoomout().connect(sigc::bind_return(sigc::mem_fun(*m_navvmaparea, &Navigate::NavVectorMapArea::zoom_out), true));
                m_navterrainarea->set_engine(m_engine);
                m_navterrainarea->set_route(m_route);
                m_navterrainarea->set_renderer(VectorMapArea::renderer_terrain);
                m_navterrainarea->set_map_scale(m_engine.get_prefs().mapscale);
                m_navterrainwindow->signal_hide().connect(sigc::mem_fun(*m_navterrainarea, &Navigate::NavVectorMapArea::hide));
                m_navterrainwindow->signal_show().connect(sigc::mem_fun(*m_navterrainarea, &Navigate::NavVectorMapArea::show));
                m_navairportdiagramarea->set_engine(m_engine);
                m_navairportdiagramarea->set_route(m_route);
                m_navairportdiagramarea->set_renderer(VectorMapArea::renderer_airportdiagram);
                m_navairportdiagramarea->set_map_scale(m_engine.get_prefs().mapscaleairportdiagram);
                m_navairportdiagramwindow->signal_hide().connect(sigc::mem_fun(*m_navairportdiagramarea, &Navigate::NavVectorMapArea::hide));
                m_navairportdiagramwindow->signal_show().connect(sigc::mem_fun(*m_navairportdiagramarea, &Navigate::NavVectorMapArea::show));
                m_engine.get_prefs().mapflags.signal_change().connect(sigc::mem_fun(*this, &Navigate::prefschange_mapflags));
                prefschange_mapflags(m_engine.get_prefs().mapflags);
                // Fixups
                if (0) {
                        int maxpos = m_gpsinfopane->get_height();
                        int minpos = std::min(maxpos, m_gpsinfopane->get_width() / 2);
                        int pos = m_gpsinfopane->get_position();
                        if (pos > maxpos)
                                m_gpsinfopane->set_position(maxpos);
                        else if (pos < minpos)
                                m_gpsinfopane->set_position(minpos);
                }
                // connect menus
                bind_menubar("navwaypointmenu", m_navwaypointwindow);
                bind_menubar("gpsinfomenu", m_gpsinfowindow);
                bind_menubar("navvmapmenu", m_navvmapwindow);
                bind_menubar("navlegmenu", m_navlegwindow);
                bind_menubar("navterrainmenu", m_navterrainwindow);
                bind_menubar("navairportdiagrammenu", m_navairportdiagramwindow);
                bind_menubar("navpoimenu", m_navpoiwindow);
                // VMap status line
                m_navvmapinfohdg->set_alignment(1.0);
                m_navvmapinfodist->set_alignment(1.0);
                m_navvmapinfovs->set_alignment(1.0);
                m_navvmapinfotarghdg->set_alignment(1.0);
                m_navvmapinfotargdist->set_alignment(1.0);
                m_navvmapinfotargvs->set_alignment(1.0);
                m_navvmapinfovgnd->set_alignment(1.0);
                m_navterraininfohdg->set_alignment(1.0);
                m_navterraininfodist->set_alignment(1.0);
                m_navterraininfovs->set_alignment(1.0);
                m_navterraininfotarghdg->set_alignment(1.0);
                m_navterraininfotargdist->set_alignment(1.0);
                m_navterraininfotargvs->set_alignment(1.0);
                m_navterraininfovgnd->set_alignment(1.0);
                m_navairportdiagraminfohdg->set_alignment(1.0);
                m_navairportdiagraminfodist->set_alignment(1.0);
                m_navairportdiagraminfovs->set_alignment(1.0);
                m_navairportdiagraminfotarghdg->set_alignment(1.0);
                m_navairportdiagraminfotargdist->set_alignment(1.0);
                m_navairportdiagraminfotargvs->set_alignment(1.0);
                m_navairportdiagraminfovgnd->set_alignment(1.0);
                m_navpoiinfohdg->set_alignment(1.0);
                m_navpoiinfodist->set_alignment(1.0);
                m_navpoiinfovs->set_alignment(1.0);
                m_navpoiinfotarghdg->set_alignment(1.0);
                m_navpoiinfotargdist->set_alignment(1.0);
                m_navpoiinfotargvs->set_alignment(1.0);
                m_navpoiinfovgnd->set_alignment(1.0);
                // VMap Menus
                m_navvmapmenuzoomin->signal_activate().connect(sigc::mem_fun(*this, &Navigate::navvmapmenuview_zoomin));
                m_navvmapmenuzoomout->signal_activate().connect(sigc::mem_fun(*this, &Navigate::navvmapmenuview_zoomout));
                // Terrain Menus
                m_navterrainmenuzoomin->signal_activate().connect(sigc::mem_fun(*this, &Navigate::navterrainmenuview_zoomin));
                m_navterrainmenuzoomout->signal_activate().connect(sigc::mem_fun(*this, &Navigate::navterrainmenuview_zoomout));
                // Airport Diagram Menus
                m_navairportdiagrammenuzoomin->signal_activate().connect(sigc::mem_fun(*this, &Navigate::navairportdiagrammenuview_zoomin));
                m_navairportdiagrammenuzoomout->signal_activate().connect(sigc::mem_fun(*this, &Navigate::navairportdiagrammenuview_zoomout));
                // Nav Waypoint List
                m_navwaypointliststore = Gtk::ListStore::create(m_navwaypointlistcolumns);
                m_navwaypointtreeview->set_model(m_navwaypointliststore);
                m_navwaypointtreeview->append_column(_("ICAO"), m_navwaypointlistcolumns.m_col_icao);
                m_navwaypointtreeview->append_column(_("Name"), m_navwaypointlistcolumns.m_col_name);
                m_navwaypointtreeview->append_column(_("Freq"), m_navwaypointlistcolumns.m_col_freq);
                Gtk::CellRendererText *time_renderer = Gtk::manage(new Gtk::CellRendererText());
                int time_col = m_navwaypointtreeview->append_column(_("Time"), *time_renderer) - 1;
                Gtk::TreeViewColumn *time_column = m_navwaypointtreeview->get_column(time_col);
                if (time_column) {
                        time_column->add_attribute(*time_renderer, "text", m_navwaypointlistcolumns.m_col_time);
                        time_column->add_attribute(*time_renderer, "weight", m_navwaypointlistcolumns.m_col_time_weight);
                        time_column->add_attribute(*time_renderer, "foreground-gdk", m_navwaypointlistcolumns.m_col_time_color);
                        time_column->add_attribute(*time_renderer, "style", m_navwaypointlistcolumns.m_col_time_style);
                }
                time_renderer->set_property("weight-set", true);
                m_navwaypointtreeview->append_column(_("Dist"), m_navwaypointlistcolumns.m_col_dist);
                m_navwaypointtreeview->append_column(_("Hdg"), m_navwaypointlistcolumns.m_col_hdg);
                m_navwaypointtreeview->append_column(_("TT"), m_navwaypointlistcolumns.m_col_tt);
                m_navwaypointtreeview->append_column(_("Alt"), m_navwaypointlistcolumns.m_col_alt);
                m_navwaypointtreeview->append_column(_("TrueAlt"), m_navwaypointlistcolumns.m_col_truealt);
                m_navwaypointtreeview->append_column(_("VertS"), m_navwaypointlistcolumns.m_col_vspeed);
                m_navwaypointtreeview->append_column(_("Lon"), m_navwaypointlistcolumns.m_col_lon);
                m_navwaypointtreeview->append_column(_("Lat"), m_navwaypointlistcolumns.m_col_lat);
                m_navwaypointtreeview->append_column(_("Var"), m_navwaypointlistcolumns.m_col_var);
                m_navwaypointtreeview->append_column(_("WCA"), m_navwaypointlistcolumns.m_col_wca);
                for (unsigned int i = 0; i < 14; ++i) {
                        Gtk::TreeViewColumn *col = m_navwaypointtreeview->get_column(i);
                        if (!col)
                                continue;
                        col->set_resizable(true);
                        col->set_reorderable(true);
                }
                for (unsigned int i = 4; i < 14; ++i) {
                        if (i >= 10 && i <= 11)
                                continue;
                        Gtk::CellRenderer *renderer = m_navwaypointtreeview->get_column_cell_renderer(i);
                        if (!renderer)
                                continue;
                        renderer->set_property("xalign", 1.0);
                }
                {
                        Gtk::TreeViewColumn *col = m_navwaypointtreeview->get_column(1);
                        if (col)
                                col->set_expand(true);
                }
                {
                        Glib::RefPtr<Gtk::TreeSelection> sel(m_navwaypointtreeview->get_selection());
                        sel->set_mode(Gtk::SELECTION_NONE);
                }
                m_navwaypointtreeview->set_enable_search(false);
                m_navwaypointtreeview->set_reorderable(false);
                open_gps();
        }
        m_updatetimer.disconnect();
        m_route.load_fp(id);
        m_route_unmodified = true;
        m_directdestleg = m_route.get_directleg();
        m_track = TracksDb::Track();
        m_track.make_valid();
        m_track.set_sourceid(NewWaypointWindow::make_sourceid());
        if (m_route.get_nrwpt()) {
                const FPlanWaypoint& wpt0(m_route[0]);
                const FPlanWaypoint& wpt1(m_route[m_route.get_nrwpt()-1]);
                m_track.set_fromicao(wpt0.get_icao());
                m_track.set_fromname(wpt0.get_name());
                m_track.set_toicao(wpt1.get_icao());
                m_track.set_toname(wpt1.get_name());
        }
        m_track.set_notes(m_route.get_note());
        {
                time_t t = time(0);
                m_track.set_offblocktime(t);
                m_track.set_takeofftime(t);
                m_track.set_landingtime(t);
                m_track.set_onblocktime(t);
                m_track.set_modtime(t);
        }
        newleg();
        update_current_route();
        m_navwaypointwindow->show();
        m_updatetimer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Navigate::timer_handler), 1000);
}

void Navigate::end_nav(void)
{
        m_updatetimer.disconnect();
        if (m_navinfowindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_navinfowindow);
#endif
                m_navinfowindow->hide();
                m_navinfowindow = 0;
        }
        if (m_findnamewindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_findnamewindow);
#endif
                m_findnamewindow->hide();
                m_findnamewindow = 0;
        }
        if (m_findnearestwindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_findnearestwindow);
#endif
                m_findnearestwindow->hide();
                m_findnearestwindow = 0;
        }
        if (m_confirmupdatewindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_confirmupdatewindow);
#endif
                m_confirmupdatewindow->hide();
                m_confirmupdatewindow = 0;
        }
        if (m_navwaypointwindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_navwaypointwindow);
#endif
                m_navwaypointwindow->hide();
                m_navwaypointwindow = 0;
        }
        if (m_gpsinfowindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_gpsinfowindow);
#endif
                m_gpsinfowindow->hide();
                m_gpsinfowindow = 0;
        }
        if (m_navvmapwindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_navvmapwindow);
#endif
                m_navvmapwindow->hide();
                m_navvmapwindow = 0;
        }
        if (m_navlegwindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_navlegwindow);
#endif
                m_navlegwindow->hide();
                m_navlegwindow = 0;
        }
        if (m_navterrainwindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_navterrainwindow);
#endif
                m_navterrainwindow->hide();
                m_navterrainwindow = 0;
        }
        if (m_navairportdiagramwindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_navairportdiagramwindow);
#endif
                m_navairportdiagramwindow->hide();
                m_navairportdiagramwindow = 0;
        }
        if (m_navpoiwindow) {
#ifdef HAVE_HILDONMM
                Hildon::Program::get_instance()->remove_window(*m_navpoiwindow);
#endif
                m_navpoiwindow->hide();
                m_navpoiwindow = 0;
        }
        if (m_aboutdialog) {
                m_aboutdialog->hide();
                m_aboutdialog = 0;
        }
        m_menu_prevwpt.clear();
        m_menu_nextwpt.clear();
        m_menu_insertbyname.clear();
        m_menu_insertnearest.clear();
        m_menu_insertcurrentpos.clear();
        m_menu_deletenextwpt.clear();
        m_refxml = Glib::RefPtr<Builder>();
        signal_endnav()();
        close_gps();
        if (m_track.is_valid()) {
                m_track.set_offblocktime(m_route.get_time_offblock_unix());
                m_track.set_onblocktime(m_route.get_time_onblock_unix());
                if (m_route.get_nrwpt()) {
                        m_track.set_takeofftime(m_route.get_time_unix(0));
                        m_track.set_landingtime(m_route.get_time_unix(m_route.get_nrwpt()-1));
                } else {
                        m_track.set_takeofftime(m_route.get_time_offblock_unix());
                        m_track.set_landingtime(m_route.get_time_onblock_unix());
                }
                m_track.set_modtime(time(0));
                m_engine.get_tracks_db().save(m_track);
        }
        m_track = TracksDb::Track();
}

void Navigate::menu_cut(Gtk::Window *wnd)
{
        Gtk::Widget *focus = wnd->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->cut_clipboard();
}

void Navigate::menu_copy(Gtk::Window *wnd)
{
        Gtk::Widget *focus = wnd->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->copy_clipboard();
}

void Navigate::menu_paste(Gtk::Window *wnd)
{
        Gtk::Widget *focus = wnd->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->paste_clipboard();
}

void Navigate::menu_delete(Gtk::Window *wnd)
{
        Gtk::Widget *focus = wnd->get_focus();
        if (!focus)
                return;
        Gtk::Editable *edit = dynamic_cast<Gtk::Editable *>(focus);
        if (!edit)
                return;
        edit->delete_selection();
}

void Navigate::menu_nextwpt(void)
{
        unsigned int curwpt(m_route.get_curwpt()), nrwpt(m_route.get_nrwpt());
        if (curwpt <= nrwpt) {
                curwpt++;
                m_route.set_curwpt(curwpt);
        }
        m_route.save_time(curwpt);
        m_route.set_time_unix(curwpt, time(0));
        newleg();
}

void Navigate::menu_prevwpt(void)
{
        unsigned int curwpt(m_route.get_curwpt()), nrwpt(m_route.get_nrwpt());
        if (curwpt > 0) {
                curwpt--;
                m_route.set_curwpt(curwpt);
        }
        m_route.restore_time(curwpt);
        newleg();
}

void Navigate::find_setwpts(const std::vector<FPlanWaypoint>& wpts)
{
        m_findnamewindow->hide();
        m_findnearestwindow->hide();
        if (!wpts.size())
                return;
        m_confirmupdatewindow->set_newwpts(wpts);
        m_confirmupdatewindow->show();
}

void Navigate::menu_insertbyname(void)
{
        m_findnamewindow->set_center(m_currentleg.get_mapcoord());
        m_findnamewindow->show();
        m_findnearestwindow->hide();
        m_confirmupdatewindow->set_enableduplicate(m_route_unmodified);
        m_confirmupdatewindow->set_wptnr(m_route.get_curwpt());
        m_confirmupdatewindow->set_wptdelete(false);
}

void Navigate::menu_insertnearest(void)
{
        m_findnearestwindow->set_center(m_currentleg.get_mapcoord());
        m_findnearestwindow->show();
        m_findnamewindow->hide();
        m_confirmupdatewindow->set_enableduplicate(m_route_unmodified);
        m_confirmupdatewindow->set_wptnr(m_route.get_curwpt());
        m_confirmupdatewindow->set_wptdelete(false);
}

void Navigate::menu_insertcurrentpos(void)
{
        m_findnamewindow->hide();
        m_findnearestwindow->hide();
        m_confirmupdatewindow->set_enableduplicate(m_route_unmodified);
        m_confirmupdatewindow->set_wptnr(m_route.get_curwpt());
        m_confirmupdatewindow->set_wptdelete(false);
        {
                FPlanWaypoint wpt;
                wpt.set_name("<current position>");
                wpt.set_coord(m_currentleg.get_mapcoord());
                wpt.set_altitude(m_currentleg.get_mapalt());
                m_confirmupdatewindow->set_newwpt(wpt);
        }
        m_confirmupdatewindow->show();
}

void Navigate::menu_deletenext(void)
{
        m_findnamewindow->hide();
        m_findnearestwindow->hide();
        m_confirmupdatewindow->set_enableduplicate(m_route_unmodified);
        m_confirmupdatewindow->set_wptnr(m_route.get_curwpt());
        m_confirmupdatewindow->set_wptdelete(true);
        m_confirmupdatewindow->show();
}

void Navigate::update_save_route(bool duplicate, uint16_t wptnr, bool wptdelete, const std::vector<FPlanWaypoint>& wpts)
{
        if (duplicate)
                m_route.duplicate_fp();
        if (wptdelete) {
                if (wptnr <= m_route.get_nrwpt())
                        m_route.delete_wpt(wptnr);
        } else {
                for (std::vector<FPlanWaypoint>::const_iterator wi(wpts.begin()), we(wpts.end()); wi != we; ++wi, ++wptnr)
                        m_route.insert_wpt(wptnr, *wi);
        }
        m_route.save_fp();
        update_time_estimates();
        m_route_unmodified = false;
        updateeditmenu();
}

void Navigate::menu_restartnav(void)
{
        m_route.set_curwpt(0);
        m_route.save_time(0);
        m_route.set_time_unix(0, time(0));
        newleg();
}

void Navigate::menu_viewwptinfo(void)
{
        m_navwaypointwindow->show();
}

void Navigate::menu_viewvmap(void)
{
        m_navvmapwindow->show();
}

void Navigate::menu_viewnearestpoi(void)
{
        m_navpoiwindow->show();
}

void Navigate::menu_viewgpsinfo(void)
{
        m_gpsinfowindow->show();
}

void Navigate::menu_viewprefs(void)
{
        m_prefswindow->show();
}

void Navigate::menu_viewcurrentleg(void)
{
        m_navlegwindow->show();
}

void Navigate::menu_viewterrain(void)
{
        m_navterrainwindow->show();
}

void Navigate::menu_viewairportdiagram(void)
{
        m_navairportdiagramwindow->show();
}

void Navigate::menu_createwaypoint(void)
{
        Glib::ustring name(m_currentleg.get_fromname());
        Glib::ustring icao(m_currentleg.get_fromicao());
        if (!m_currentleg.get_fromname().empty() && !m_currentleg.get_toname().empty())
                name += " - ";
        if (!m_currentleg.get_fromicao().empty() && !m_currentleg.get_toicao().empty())
                icao += " - ";
        name += m_currentleg.get_toname();
        icao += m_currentleg.get_toicao();
        if (!m_currentleg.is_gpsvalid())
                name = "(NOGPS) " + name;
        Glib::RefPtr<NewWaypointWindow> wnd(NewWaypointWindow::create(m_engine, m_currentleg.get_mapcoord(), name, icao));
        wnd->present();
}

void Navigate::menu_about(void)
{
        m_aboutdialog->show();
}

void Navigate::update_route(FPlanRoute::id_t id)
{
        if (false)
                std::cerr << "Update route: id=" << id << std::endl;
        if (!id || id != m_route.get_id())
                return;
        m_route.load_fp(id);
        updateeditmenu();
        update_time_estimates();
        if (m_confirmupdatewindow)
                m_confirmupdatewindow->update_route();
}

void Navigate::update_current_route(void)
{
        // Delete old contents
        m_navwaypointliststore->clear();
        // Populate Nav Waypoint Window
        Gdk::Color black, red;
        black.set_grey(0);
        red.set_rgb(0xffff, 0, 0);
#ifdef HAVE_GTKMM2
        {
                Glib::RefPtr<Gdk::Colormap> colmap(Gdk::Colormap::get_system());
                colmap->alloc_color(black);
                colmap->alloc_color(red);
        }
#endif
        Preferences& prefs(m_engine.get_prefs());
        for (unsigned int nr = 0;; nr++) {
                FPlanLeg leg = m_route.get_leg(nr);
                if (leg.get_legtype() == FPlanLeg::legtype_invalid)
                        break;
                leg.wind_correction(prefs.vcruise);
                Gtk::TreeModel::Row row(*(m_navwaypointliststore->append()));
                row[m_navwaypointlistcolumns.m_col_nr] = nr;
                row[m_navwaypointlistcolumns.m_col_icao] = leg.get_fromicao();
                row[m_navwaypointlistcolumns.m_col_name] = leg.get_fromname();
                row[m_navwaypointlistcolumns.m_col_freq] = "";
                row[m_navwaypointlistcolumns.m_col_time] = Conversions::time_str(leg.get_fromtime_unix());
                row[m_navwaypointlistcolumns.m_col_time_weight] = 0;
                row[m_navwaypointlistcolumns.m_col_time_color] = black;
                row[m_navwaypointlistcolumns.m_col_time_style] = Pango::STYLE_NORMAL;
                row[m_navwaypointlistcolumns.m_col_lon] = "";
                row[m_navwaypointlistcolumns.m_col_lat] = "";
                row[m_navwaypointlistcolumns.m_col_alt] = "";
                row[m_navwaypointlistcolumns.m_col_truealt] = "";
                row[m_navwaypointlistcolumns.m_col_hdg] = "";
                row[m_navwaypointlistcolumns.m_col_tt] = "";
                row[m_navwaypointlistcolumns.m_col_var] = "";
                row[m_navwaypointlistcolumns.m_col_wca] = "";
                row[m_navwaypointlistcolumns.m_col_dist] = "";
                row[m_navwaypointlistcolumns.m_col_vspeed] = "";
                if (leg.get_legtype() == FPlanLeg::legtype_offblock_takeoff)
                        continue;
                row[m_navwaypointlistcolumns.m_col_freq] = Conversions::freq_str(leg.get_fromfrequency());
                row[m_navwaypointlistcolumns.m_col_time] = Conversions::time_str(leg.get_fromtime_unix());
                row[m_navwaypointlistcolumns.m_col_time_weight] = nr; //leg.is_future() ? 1 : 0;
                row[m_navwaypointlistcolumns.m_col_time_color] = leg.is_future() ? red : black;
                row[m_navwaypointlistcolumns.m_col_time_style] = leg.is_future() ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
                row[m_navwaypointlistcolumns.m_col_lon] = leg.get_fromcoord().get_lon_str();
                row[m_navwaypointlistcolumns.m_col_lat] = leg.get_fromcoord().get_lat_str();
                row[m_navwaypointlistcolumns.m_col_alt] = Conversions::alt_str(leg.get_fromaltitude(), leg.get_fromflags());
                row[m_navwaypointlistcolumns.m_col_truealt] = Conversions::alt_str(leg.get_fromtruealtitude(), 0);
                row[m_navwaypointlistcolumns.m_col_hdg] = Conversions::track_str(leg.get_heading());
                row[m_navwaypointlistcolumns.m_col_tt] = Conversions::track_str(leg.get_truetrack());
                row[m_navwaypointlistcolumns.m_col_var] = Conversions::track_str(leg.get_variance());
                row[m_navwaypointlistcolumns.m_col_wca] = Conversions::track_str(leg.get_wca());
                row[m_navwaypointlistcolumns.m_col_dist] = Conversions::dist_str(leg.get_dist());
                row[m_navwaypointlistcolumns.m_col_vspeed] = Conversions::verticalspeed_str(leg.get_verticalspeed());
                if (leg.get_legtype() != FPlanLeg::legtype_landing_onblock)
                        continue;
                row = *(m_navwaypointliststore->append());
                row[m_navwaypointlistcolumns.m_col_nr] = nr + 1;
                row[m_navwaypointlistcolumns.m_col_icao] = leg.get_toicao();
                row[m_navwaypointlistcolumns.m_col_name] = leg.get_toname();
                row[m_navwaypointlistcolumns.m_col_freq] = "";
                row[m_navwaypointlistcolumns.m_col_time] = Conversions::time_str(leg.get_totime_unix());
                row[m_navwaypointlistcolumns.m_col_time_weight] = 0;
                row[m_navwaypointlistcolumns.m_col_time_color] = (nr + 1 > m_route.get_curwpt()) ? red : black;
                row[m_navwaypointlistcolumns.m_col_time_style] = (nr + 1 > m_route.get_curwpt()) ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL;
                row[m_navwaypointlistcolumns.m_col_lon] = "";
                row[m_navwaypointlistcolumns.m_col_lat] = "";
                row[m_navwaypointlistcolumns.m_col_alt] = "";
                row[m_navwaypointlistcolumns.m_col_truealt] = "";
                row[m_navwaypointlistcolumns.m_col_hdg] = "";
                row[m_navwaypointlistcolumns.m_col_tt] = "";
                row[m_navwaypointlistcolumns.m_col_var] = "";
                row[m_navwaypointlistcolumns.m_col_wca] = "";
                row[m_navwaypointlistcolumns.m_col_dist] = "";
                row[m_navwaypointlistcolumns.m_col_vspeed] = "";
                break;
        }
}

Navigate::GPSAzimuthElevation::GPSAzimuthElevation( BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml )
        : Gtk::DrawingArea(castitem)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_expose_event));
        signal_size_request().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_size_request));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &GPSAzimuthElevation::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

Navigate::GPSAzimuthElevation::~GPSAzimuthElevation( )
{
}

void Navigate::GPSAzimuthElevation::update( const GPSComm::SVVector & svs )
{
        if (svs == m_sv)
                return;
        m_sv = svs;
        if (get_is_drawable())
                queue_draw();
}

bool Navigate::GPSAzimuthElevation::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
        static const double prn_font = 12;
        static const double windrose_font = 12;
        static const char *windrose_text[4] = { "N", "E", "S", "W" };
        int width, height;
	{
		Gtk::Allocation allocation(get_allocation());
		width = allocation.get_width();
		height = allocation.get_height();
	}
	cr->save();
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        // draw windrose
        cr->translate(0.5 * width, 0.5 * height);
        double radius = 0.5 * std::min(width, height);
        cr->set_font_size(windrose_font);
        Cairo::TextExtents ext;
        cr->get_text_extents("W", ext);
        radius -= 0.5 * std::max(ext.width, ext.height);
        cr->set_source_rgb(0.85, 0.85, 0.85);
        cr->set_line_width(1.0);
        for (int i = 1; i <= 3; i++) {
                cr->arc(0, 0, radius * i * (1.0 / 3.0), 0, 2.0 * M_PI);
                cr->stroke();
        }
        for (int i = 0; i < 12; i++) {
                double a = i * (M_PI/ 6.0);
                cr->move_to(0, 0);
                cr->line_to(radius * cos(a), radius * sin(a));
                cr->stroke();
        }
        cr->set_source_rgb(0.75, 0.75, 0.75);
        for (int i = 0; i < 4; i++) {
                double a = i * (M_PI/ 2.0);
                cr->save();
                cr->translate(radius * sin(a), -radius * cos(a));
                cr->save();
                cr->set_source_rgb(1.0, 1.0, 1.0);
                cr->rectangle(-0.5 * ext.width, -0.5 * ext.height, ext.width, ext.height);
                cr->fill();
                cr->restore();
                Cairo::TextExtents ext1;
                cr->get_text_extents(windrose_text[i], ext1);
                cr->move_to(-0.5 * ext1.width, 0.5 * ext1.height);
                cr->show_text(windrose_text[i]);
                cr->restore();
        }
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->set_line_width(1.0);
        cr->set_font_size(prn_font);
        cr->get_text_extents("88", ext);
        double satradius = sqrt(ext.width * ext.width + ext.height * ext.height) * (0.5 * 1.2);
        double satsail = 1.2 * satradius;
        for (GPSComm::SVVector::size_type i = 0; i < m_sv.size(); i++) {
                const GPSComm::SV& sv(m_sv[i]);
                cr->save();
                double a = sv.get_azimuth() * (M_PI / 180.0);
                double r = (90 - sv.get_elevation()) * (1.0 / 90.0) * radius;
                cr->translate(r * sin(a), -r * cos(a));
                cr->begin_new_path();
                cr->save();
                svcolor(cr, sv);
                cr->arc(0, 0, satradius, 0, 2.0 * M_PI);
                if (sv.is_used())
                        cr->fill();
                else
                        cr->stroke();
                cr->move_to(-satsail, 0);
                cr->line_to(-satradius, 0);
                cr->stroke();
                cr->move_to(satsail, 0);
                cr->line_to(satradius, 0);
                cr->stroke();
                cr->move_to(-satsail, -satradius);
                cr->line_to(-satsail, satradius);
                cr->stroke();
                cr->move_to(satsail, -satradius);
                cr->line_to(satsail, satradius);
                cr->stroke();
                cr->restore();
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", sv.get_prn());
                Cairo::TextExtents ext1;
                cr->get_text_extents(buf, ext1);
                cr->move_to(-0.5 * ext1.width, 0.5 * ext1.height);
                cr->show_text(buf);
                cr->restore();
        }
	cr->restore();
        return true;
}

#ifdef HAVE_GTKMM2
bool Navigate::GPSAzimuthElevation::on_expose_event(GdkEventExpose *event)
{
        Glib::RefPtr<Gdk::Window> wnd(get_window());
        if (!wnd)
                return false;
        Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	if (event) {
		GdkRectangle *rects;
		int n_rects;
		gdk_region_get_rectangles(event->region, &rects, &n_rects);
		for (int i = 0; i < n_rects; i++)
			cr->rectangle(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cr->clip();
		g_free(rects);
	}
        return on_draw(cr);
}

void Navigate::GPSAzimuthElevation::on_size_request(Gtk::Requisition * requisition)
{
        if (!requisition)
                return;
        requisition->width = 64;
        requisition->height = 64;
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode Navigate::GPSAzimuthElevation::get_request_mode_vfunc() const
{
	return Gtk::DrawingArea::get_request_mode_vfunc();
}

void Navigate::GPSAzimuthElevation::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = 64;
	natural_width = 128;
}

void Navigate::GPSAzimuthElevation::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	minimum_height = natural_height = width;
}

void Navigate::GPSAzimuthElevation::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = 64;
	natural_height = 128;
}

void Navigate::GPSAzimuthElevation::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	minimum_width = natural_width = height;
}
#endif

void Navigate::GPSAzimuthElevation::svcolor(const Cairo::RefPtr<Cairo::Context>& cr, const GPSComm::SV& sv)
{
        int ss = sv.get_signalstrength();
        if (ss < 20) {
                cr->set_source_rgb(0.7, 0.7, 0.7);
                return;
        }
        if (ss < 40) {
                cr->set_source_rgb(1.0, 0.9, 0.0);
                return;
        }
        cr->set_source_rgb(0.0, 1.0, 0.0);
}

Navigate::GPSSignalStrength::GPSSignalStrength( BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml )
        : Gtk::DrawingArea(castitem)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_expose_event));
        signal_size_request().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_size_request));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &GPSSignalStrength::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

Navigate::GPSSignalStrength::~GPSSignalStrength( )
{
}

void Navigate::GPSSignalStrength::update( const GPSComm::SVVector & svs )
{
        if (svs == m_sv)
                return;
        m_sv = svs;
        if (get_is_drawable())
                queue_draw();
}

bool Navigate::GPSSignalStrength::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	static const double prn_font = 12;
        static const int max_sat = 14;
        int width, height;
	{
		Gtk::Allocation allocation(get_allocation());
		width = allocation.get_width();
		height = allocation.get_height();
	}
	cr->save();
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        cr->set_font_size(prn_font);
        Cairo::TextExtents ext;
        cr->get_text_extents("88", ext);
        double gap = ext.width * 0.2;
        double ygap = ext.height * 0.2;
        double barwidth = (width - (max_sat + 1) * gap) * (1.0 / max_sat);
        barwidth = std::max(barwidth, ext.width);
        double bary = ext.height + 2.0 * ygap;
        double barscale = std::max(height - ygap - bary, 10.0) * (1.0 / 60.0);
        bary = height - bary;
        cr->set_source_rgb(0.85, 0.85, 0.85);
        cr->set_line_width(1.0);
        for (int i = 0; i <= 60; i += 20) {
                cr->move_to(0, bary - i * barscale);
                cr->rel_line_to(width, 0);
                cr->stroke();
        }
        for (GPSComm::SVVector::size_type i = 0; i < m_sv.size(); i++) {
                const GPSComm::SV& sv(m_sv[i]);
                cr->save();
                //std::cerr << "w " << width << " h " << height << " x " << (i * barwidth + (i + 1) * gap) << " y " << (height - ext.height) << std::endl;
                cr->translate(i * barwidth + (i + 1) * gap, 0);
                cr->set_source_rgb(0.0, 0.0, 0.0);
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", sv.get_prn());
                Cairo::TextExtents ext1;
                cr->get_text_extents(buf, ext1);
                cr->move_to(0.5 * (barwidth - ext1.width), height - ygap);
                cr->show_text(buf);
                GPSAzimuthElevation::svcolor(cr, sv);
                double h = sv.get_signalstrength() * barscale;
                cr->rectangle(0, bary - h, barwidth, h);
                if (sv.is_used())
                        cr->fill();
                else
                        cr->stroke();
                cr->restore();
        }
	cr->restore();
        return true;
}

#ifdef HAVE_GTKMM2
bool Navigate::GPSSignalStrength::on_expose_event(GdkEventExpose *event)
{
        Glib::RefPtr<Gdk::Window> wnd(get_window());
        if (!wnd)
                return false;
        Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	if (event) {
		GdkRectangle *rects;
		int n_rects;
		gdk_region_get_rectangles(event->region, &rects, &n_rects);
		for (int i = 0; i < n_rects; i++)
			cr->rectangle(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cr->clip();
		g_free(rects);
	}
        return on_draw(cr);
}

void Navigate::GPSSignalStrength::on_size_request(Gtk::Requisition * requisition)
{
        if (!requisition)
                return;
        requisition->width = 64;
        requisition->height = 32;
}
#endif

#ifdef HAVE_GTKMM3
Gtk::SizeRequestMode Navigate::GPSSignalStrength::get_request_mode_vfunc() const
{
	return Gtk::DrawingArea::get_request_mode_vfunc();
}

void Navigate::GPSSignalStrength::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const
{
	minimum_width = 64;
	natural_width = 128;
}

void Navigate::GPSSignalStrength::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const
{
	minimum_height = width / 3;
	natural_height = width / 2;
}

void Navigate::GPSSignalStrength::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const
{
	minimum_height = 32;
	natural_height = 64;
}

void Navigate::GPSSignalStrength::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const
{
	minimum_width = height + height / 2;
	natural_width = height * 2;
}
#endif

Navigate::NavWaypointModelColumns::NavWaypointModelColumns(void)
{
        add(m_col_icao);
        add(m_col_name);
        add(m_col_freq);
        add(m_col_time);
        add(m_col_time_weight);
        add(m_col_time_color);
        add(m_col_time_style);
        add(m_col_lon);
        add(m_col_lat);
        add(m_col_alt);
        add(m_col_truealt);
        add(m_col_hdg);
        add(m_col_tt);
        add(m_col_var);
        add(m_col_wca);
        add(m_col_dist);
        add(m_col_vspeed);
        add(m_col_nr);
}

void Navigate::update_time_estimates(void)
{
        Preferences& p(m_engine.get_prefs());
        unsigned int curwpt(m_route.get_curwpt()), nrwpt(m_route.get_nrwpt());
        for (unsigned int nr = curwpt; nr <= nrwpt; nr++) {
                FPlanLeg leg = m_route.get_leg(nr);
                leg.wind_correction(p.vcruise);
                if (nr >= nrwpt) {
                        const FPlanWaypoint& wpt(m_route[nrwpt-1]);
                        m_route.set_time_onblock(wpt.get_time() + 5 * 60);
                        continue;
                }
                FPlanWaypoint& wpt(m_route[nr]);
                if (!nr) {
                        wpt.set_time(m_route.get_time_offblock() + 5 * 60);
                        continue;
                }
                const FPlanWaypoint& wptp(m_route[nr-1]);
                wpt.set_time(wptp.get_time() + leg.get_legtime());
        }
        if (m_route.get_id()) {
                m_route.save_fp();
                signal_updateroute()(m_route.get_id());
        }
        update_current_route();
}

void Navigate::newleg(void)
{
        unsigned int curwpt(m_route.get_curwpt());
        m_currentleg = m_route.get_leg(curwpt);
        m_nextleg = m_route.get_leg(curwpt + 1);
        Preferences& p(m_engine.get_prefs());
        m_currentleg.wind_correction(p.vcruise);
        m_nextleg.wind_correction(p.vcruise);
        m_directdestleg.wind_correction(p.vcruise);
        update_time_estimates();
        updateleg();
        updateeditmenu();
}

void Navigate::updateeditmenu(void)
{
        unsigned int curwpt(m_route.get_curwpt()), nrwpt(m_route.get_nrwpt());
        for (menuitemvector_t::const_iterator mi(m_menu_prevwpt.begin()), me(m_menu_prevwpt.end()); mi != me; ++mi)
                (*mi)->set_sensitive(curwpt > 0);
        for (menuitemvector_t::const_iterator mi(m_menu_nextwpt.begin()), me(m_menu_nextwpt.end()); mi != me; ++mi)
                (*mi)->set_sensitive(curwpt <= nrwpt);
        for (menuitemvector_t::const_iterator mi(m_menu_insertbyname.begin()), me(m_menu_insertbyname.end()); mi != me; ++mi)
                (*mi)->set_sensitive(curwpt > 0 && curwpt < nrwpt);
        for (menuitemvector_t::const_iterator mi(m_menu_insertnearest.begin()), me(m_menu_insertnearest.end()); mi != me; ++mi)
                (*mi)->set_sensitive(curwpt > 0 && curwpt < nrwpt);
        for (menuitemvector_t::const_iterator mi(m_menu_insertcurrentpos.begin()), me(m_menu_insertcurrentpos.end()); mi != me; ++mi)
                (*mi)->set_sensitive(curwpt > 0 && curwpt < nrwpt);
        for (menuitemvector_t::const_iterator mi(m_menu_deletenextwpt.begin()), me(m_menu_deletenextwpt.end()); mi != me; ++mi)
                (*mi)->set_sensitive(curwpt > 0 && curwpt + 1 < nrwpt);
}

void Navigate::updateleg(void)
{
        bool gpsvalid = m_gps->is_coord_valid(), gpsaltvalid = m_gps->is_altitude_valid();
        time_t tm(time(0));
        Point gpscoord(m_gps->get_coord());
        int16_t gpsalt((int16_t)(m_gps->get_altitude() * FPlanWaypoint::m_to_ft));
        float vgnd;
        if (gpsvalid) {
                vgnd = m_gps->get_velocity() * Point::mps_to_kts;
        } else {
                vgnd = m_currentleg.get_vdist();
        }
        m_currentleg.update(tm, gpscoord, gpsvalid, gpsalt, gpsaltvalid, vgnd);
        m_directdestleg.update(tm, m_currentleg.get_mapcoord(), true, m_currentleg.get_mapalt(), true, vgnd);
        Preferences& p(m_engine.get_prefs());
        if (p.autowpt) {
                bool nextwpt = false;
                switch (m_currentleg.get_legtype()) {
                        default:
                                break;

                        case FPlanLeg::legtype_offblock_takeoff:
                                if (p.vblock <= 0)
                                        break;
                                if (!gpsvalid)
                                        break;
                                if (vgnd < p.vblock)
                                        break;
                                nextwpt = true;
                                break;

                        case FPlanLeg::legtype_landing_onblock:
                                if (p.vblock <= 0)
                                        break;
                                if (!gpsvalid)
                                        break;
                                if (vgnd >= p.vblock)
                                        break;
                                nextwpt = true;
                                break;

                        case FPlanLeg::legtype_normal:
                                if (!gpsvalid)
                                        break;
                                m_nextleg.update(tm, gpscoord, gpsvalid, gpsalt, gpsaltvalid, vgnd);
                                if (fabsf(m_currentleg.get_crosstrack()) <= fabsf(m_nextleg.get_crosstrack()))
                                        break;
                                nextwpt = true;
                                break;
                }
                if (nextwpt) {
                        menu_nextwpt();
                        return;
                }
        }
        // update forms
        float maptrack(m_gps->get_track());
        m_navvmaparea->set_center(m_currentleg.get_mapcoord(), m_currentleg.get_mapalt());
        m_navvmaparea->set_cursor(m_currentleg.get_mapcoord(), maptrack, gpsvalid);
        m_navterrainarea->set_center(m_currentleg.get_mapcoord(), m_currentleg.get_mapalt());
        m_navterrainarea->set_cursor(m_currentleg.get_mapcoord(), maptrack, gpsvalid);
        m_navairportdiagramarea->set_center(m_currentleg.get_mapcoord(), m_currentleg.get_mapalt());
        m_navairportdiagramarea->set_cursor(m_currentleg.get_mapcoord(), maptrack, gpsvalid);
        m_navlegfromname->set_text(m_currentleg.get_fromicaoname());
        m_navlegfromalt->set_text(Conversions::alt_str(m_currentleg.get_fromaltitude(), m_currentleg.get_fromflags()));
        m_navlegtoname->set_text(m_currentleg.get_toicaoname());
        m_navlegtoalt->set_text(Conversions::alt_str(m_currentleg.get_toaltitude(), m_currentleg.get_toflags()));
        m_navlegcurrenttt->set_text(Conversions::track_str(m_currentleg.get_truetrack()));
        m_navlegcurrentmt->set_text(Conversions::track_str(m_currentleg.get_truetrack() - m_currentleg.get_variance()));
        m_navlegcurrenthdg->set_text(Conversions::track_str(m_currentleg.get_heading()));
        m_navlegcurrentdist->set_text(Conversions::dist_str(m_currentleg.get_dist()));
        m_navlegnexttt->set_text(Conversions::track_str(m_nextleg.get_truetrack()));
        m_navlegnextmt->set_text(Conversions::track_str(m_nextleg.get_truetrack() - m_currentleg.get_variance()));
        m_navlegnexthdg->set_text(Conversions::track_str(m_nextleg.get_heading()));
        m_navlegnextdist->set_text(Conversions::dist_str(m_nextleg.get_dist()));
        m_navlegmaplon->set_text(m_currentleg.get_mapcoord().get_lon_str());
        m_navlegmaplat->set_text(m_currentleg.get_mapcoord().get_lat_str());
        m_navlegalt->set_text(Conversions::alt_str(m_currentleg.get_mapalt(), 0));
        m_navlegmaplonlabel->set_text(m_currentleg.is_gpsvalid() ? _("GPS Lon") : _("Map Lon"));
        m_navlegmaplatlabel->set_text(m_currentleg.is_gpsvalid() ? _("GPS Lat") : _("Map Lat"));
        m_navlegaltlabel->set_text(m_currentleg.is_gpsaltvalid() ? _("GPS Alt") : _("Alt"));
        if (gpsvalid) {
                m_navleggpshdg->set_text(Conversions::track_str(m_gps->get_track()));
                m_navlegvgnd->set_text(Conversions::track_str(vgnd));
        } else {
                m_navleggpshdg->set_text("--");
                m_navlegvgnd->set_text("--");
        }
        m_navlegcrosstrack->set_text(Conversions::dist_str(m_currentleg.get_crosstrack() * (Point::km_to_nmi * 1e-3)));
        m_navlegcoursetodest->set_text(Conversions::track_str(m_currentleg.get_coursetodest()));
        m_navlegreferencealt->set_text(Conversions::alt_str(m_currentleg.get_referencealt(), 0));
        m_navlegaltdiff->set_text(Conversions::alt_str(m_currentleg.get_altdiff(), 0));
        Glib::ustring htd(Conversions::track_str(m_currentleg.get_coursetodest() - m_currentleg.get_variance() + m_currentleg.get_wca()));
        Glib::ustring dtd(Conversions::dist_str(m_currentleg.get_disttodest()));
        m_navleghdgtodest->set_text(htd);
        m_navlegdisttodest->set_text(dtd);
        {
                uint32_t tleg(m_currentleg.get_legtime()), tdiff(time(0) - m_currentleg.get_fromtime_unix());
                uint32_t min(tdiff / 60), sec(tdiff - 60 * min);
                uint32_t legmin(tleg / 60), legsec(tleg - 60 * legmin);
                char buf[16];
                snprintf(buf, sizeof(buf), "%02u:%02u / %02u:%02u", min, sec, legmin, legsec);
                m_navlegprogress->set_text(buf);
                m_navlegprogress->set_fraction(std::min(tdiff / (float)std::max(tleg, (uint32_t)1U), 1.0f));
        }
        m_navlegcdi->update(m_currentleg.get_crosstrack() * (Point::km_to_nmi * 1e-3), m_currentleg.get_altdiff());
        m_navpoiwindow->set_coord(m_currentleg.get_mapcoord());
        Glib::ustring vstd(Conversions::verticalspeed_str(m_currentleg.get_verticalspeedtodest()));
        Glib::ustring htd2(Conversions::track_str(m_directdestleg.get_coursetodest() - m_currentleg.get_variance() + m_currentleg.get_wca()));
        Glib::ustring dtd2(Conversions::dist_str(m_directdestleg.get_disttodest()));
        Glib::ustring vstd2(Conversions::verticalspeed_str(m_directdestleg.get_verticalspeedtodest()));
        Glib::ustring gpss;
        {
                GPSComm::fixtype_t fixt(m_gps->get_fixtype());
                GPSComm::fixstatus_t fixs(m_gps->get_fixstatus());
                if (fixs == GPSComm::fixstatus_dgpsfix)
                        gpss = "D";
                if (fixt == GPSComm::fixtype_3d)
                        gpss += "3D";
                else if (fixt == GPSComm::fixtype_2d)
                        gpss += "2D";
                if (fixs == GPSComm::fixstatus_nofix || fixt == GPSComm::fixtype_nofix)
                        gpss = "NOFIX";
                if (fixt == GPSComm::fixtype_noconn)
                        gpss = "NOCONN";
        }
        Glib::ustring svgnd(gpsvalid ? Conversions::velocity_str(vgnd) : "--");
        m_navvmapinfohdg->set_text(htd);
        m_navvmapinfodist->set_text(dtd);
        m_navvmapinfovs->set_text(vstd);
        m_navvmapinfoname->set_text(m_currentleg.get_toicaoname());
        m_navvmapinfotarghdg->set_text(htd2);
        m_navvmapinfotargdist->set_text(dtd2);
        m_navvmapinfotargvs->set_text(vstd2);
        m_navvmapinfogps->set_text(gpss);
        m_navvmapinfovgnd->set_text(svgnd);
        m_navterraininfohdg->set_text(htd);
        m_navterraininfodist->set_text(dtd);
        m_navterraininfovs->set_text(vstd);
        m_navterraininfoname->set_text(m_currentleg.get_toicaoname());
        m_navterraininfotarghdg->set_text(htd2);
        m_navterraininfotargdist->set_text(dtd2);
        m_navterraininfotargvs->set_text(vstd2);
        m_navterraininfogps->set_text(gpss);
        m_navterraininfovgnd->set_text(svgnd);
        m_navairportdiagraminfohdg->set_text(htd);
        m_navairportdiagraminfodist->set_text(dtd);
        m_navairportdiagraminfovs->set_text(vstd);
        m_navairportdiagraminfoname->set_text(m_currentleg.get_toicaoname());
        m_navairportdiagraminfotarghdg->set_text(htd2);
        m_navairportdiagraminfotargdist->set_text(dtd2);
        m_navairportdiagraminfotargvs->set_text(vstd2);
        m_navairportdiagraminfogps->set_text(gpss);
        m_navairportdiagraminfovgnd->set_text(svgnd);
        m_navpoiinfohdg->set_text(htd);
        m_navpoiinfodist->set_text(dtd);
        m_navpoiinfovs->set_text(vstd);
        m_navpoiinfoname->set_text(m_currentleg.get_toicaoname());
        m_navpoiinfotarghdg->set_text(htd2);
        m_navpoiinfotargdist->set_text(dtd2);
        m_navpoiinfotargvs->set_text(vstd2);
        m_navpoiinfogps->set_text(gpss);
        m_navpoiinfovgnd->set_text(svgnd);
#ifdef HAVE_GTKMM2
        {
                Glib::RefPtr<Gtk::RcStyle> rcs(Gtk::RcStyle::create());
                Glib::RefPtr<Gtk::RcStyle> rcsa(Gtk::RcStyle::create());
                Gdk::Color col0, col1;
                col0.set_rgb(0, 0, 0xffff);
                col1.set_rgb(0, 0, 0);
                rcs->set_color_flags(Gtk::STATE_NORMAL, Gtk::RC_TEXT);
                rcs->set_text(Gtk::STATE_NORMAL, gpsvalid ? col1 : col0);
                rcsa->set_color_flags(Gtk::STATE_NORMAL, Gtk::RC_TEXT);
                rcsa->set_text(Gtk::STATE_NORMAL, gpsaltvalid ? col1 : col0);
                m_navvmapinfohdg->modify_style(rcs);
                m_navvmapinfodist->modify_style(rcs);
                m_navvmapinfovs->modify_style(rcsa);
                m_navvmapinfotarghdg->modify_style(rcs);
                m_navvmapinfotargdist->modify_style(rcs);
                m_navvmapinfotargvs->modify_style(rcsa);
                m_navterraininfohdg->modify_style(rcs);
                m_navterraininfodist->modify_style(rcs);
                m_navterraininfovs->modify_style(rcsa);
                m_navterraininfotarghdg->modify_style(rcs);
                m_navterraininfotargdist->modify_style(rcs);
                m_navterraininfotargvs->modify_style(rcsa);
                m_navairportdiagraminfohdg->modify_style(rcs);
                m_navairportdiagraminfodist->modify_style(rcs);
                m_navairportdiagraminfovs->modify_style(rcsa);
                m_navairportdiagraminfotarghdg->modify_style(rcs);
                m_navairportdiagraminfotargdist->modify_style(rcs);
                m_navairportdiagraminfotargvs->modify_style(rcsa);
                m_navpoiinfohdg->modify_style(rcs);
                m_navpoiinfodist->modify_style(rcs);
                m_navpoiinfovs->modify_style(rcsa);
                m_navpoiinfotarghdg->modify_style(rcs);
                m_navpoiinfotargdist->modify_style(rcs);
                m_navpoiinfotargvs->modify_style(rcsa);
        }
#endif
#ifdef HAVE_GTKMM3
        {
                Gdk::RGBA col, cola;
		{
			Gdk::RGBA col0, col1;
			col0.set_rgba_u(0, 0, 0xffff);
			col1.set_rgba_u(0, 0, 0);
			col = gpsvalid ? col1 : col0;
			cola = gpsaltvalid ? col1 : col0;
		}
		m_navvmapinfohdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navvmapinfodist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navvmapinfovs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
                m_navvmapinfotarghdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navvmapinfotargdist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navvmapinfotargvs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
                m_navterraininfohdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navterraininfodist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navterraininfovs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
                m_navterraininfotarghdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navterraininfotargdist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navterraininfotargvs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
                m_navairportdiagraminfohdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navairportdiagraminfodist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navairportdiagraminfovs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
                m_navairportdiagraminfotarghdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navairportdiagraminfotargdist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navairportdiagraminfotargvs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
                m_navpoiinfohdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navpoiinfodist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navpoiinfovs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
                m_navpoiinfotarghdg->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navpoiinfotargdist->override_color(col, Gtk::STATE_FLAG_NORMAL);
                m_navpoiinfotargvs->override_color(cola, Gtk::STATE_FLAG_NORMAL);
        }
#endif
}

void Navigate::prefschange_mapflags(int f)
{
        *(VectorMapArea *)m_navvmaparea = (MapRenderer::DrawFlags)f;
        *(VectorMapArea *)m_navterrainarea = (MapRenderer::DrawFlags)f;
        *(VectorMapArea *)m_navairportdiagramarea = (MapRenderer::DrawFlags)f;
}

bool Navigate::timer_handler(void)
{
        updateleg();
        std::cerr << "timer_handler" << std::endl;
        return true;
}

void Navigate::aboutdialog_response(int response)
{
        if (response == Gtk::RESPONSE_CLOSE || response == Gtk::RESPONSE_CANCEL)
                m_aboutdialog->hide();
}

bool Navigate::window_delete_event(GdkEventAny * event, Gtk::Window * window)
{
        window->hide();
        return true;
}

bool Navigate::window_key_press_event(GdkEventKey * event, NavPageWindow * window)
{
        if(!event)
                return false;
        std::cerr << "key press: type " << event->type << " keyval " << event->keyval << std::endl;
        if (event->type != GDK_KEY_PRESS)
                return false;
        static const unsigned int nrpages = sizeof(m_pagewindows) / sizeof(m_pagewindows[0]);
        unsigned int page;
        switch (event->keyval) {
                case GDK_KEY_Left:
                        for (page = 0; page < nrpages; page++)
                                if (m_pagewindows[page] == window)
                                        break;
                        std::cerr << "page number " << page << std::endl;
                        if (page >= nrpages)
                                return false;
                        m_pagewindows[page]->hide();
                        page = (page + nrpages - 1) % nrpages;
                        m_pagewindows[page]->show();
                        m_pagewindows[page]->present();
                        return true;

                case GDK_KEY_Right:
                case GDK_KEY_Escape:
                        for (page = 0; page < nrpages; page++)
                                if (m_pagewindows[page] == window)
                                        break;
                        if (page >= nrpages)
                                return false;
                        m_pagewindows[page]->hide();
                        page = (page + 1) % nrpages;
                        m_pagewindows[page]->show();
                        m_pagewindows[page]->present();
                        return true;

                case GDK_KEY_F6:
                        window->toggle_fullscreen();
                        return true;

                case GDK_KEY_F7:
                        return window->zoom_in();

                case GDK_KEY_F8:
                        return window->zoom_out();

                case GDK_KEY_r:
                        menu_restartnav();
                        return true;

                case GDK_KEY_p:
                        menu_prevwpt();
                        return true;

                case GDK_KEY_n:
                        menu_nextwpt();
                        return true;

                default:
                        break;
        }
        return false;
}

void Navigate::navvmapmenuview_zoomin(void)
{
        m_navvmaparea->zoom_in();
        m_engine.get_prefs().mapscale = m_navvmaparea->get_map_scale();
}

void Navigate::navvmapmenuview_zoomout(void)
{
        m_navvmaparea->zoom_out();
        m_engine.get_prefs().mapscale = m_navvmaparea->get_map_scale();
}

void Navigate::navterrainmenuview_zoomin(void)
{
        m_navterrainarea->zoom_in();
        m_engine.get_prefs().mapscale = m_navterrainarea->get_map_scale();
}

void Navigate::navterrainmenuview_zoomout(void)
{
        m_navterrainarea->zoom_out();
        m_engine.get_prefs().mapscale = m_navterrainarea->get_map_scale();
}

void Navigate::navairportdiagrammenuview_zoomin(void)
{
        m_navairportdiagramarea->zoom_in();
        m_engine.get_prefs().mapscaleairportdiagram = m_navairportdiagramarea->get_map_scale();
}

void Navigate::navairportdiagrammenuview_zoomout(void)
{
        m_navairportdiagramarea->zoom_out();
        m_engine.get_prefs().mapscaleairportdiagram = m_navairportdiagramarea->get_map_scale();
}

Navigate::CDI::CDI( BaseObjectType * castitem, const Glib::RefPtr< Builder > & refxml )
        : Gtk::DrawingArea(castitem), m_crosstrack(0), m_altdiff(0)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
#ifdef HAVE_GTKMM2
        signal_expose_event().connect(sigc::mem_fun(*this, &CDI::on_expose_event));
#endif
#ifdef HAVE_GTKMM3
        signal_draw().connect(sigc::mem_fun(*this, &CDI::on_draw));
#endif
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

Navigate::CDI::~CDI( )
{
}

void Navigate::CDI::update( float crosstrack, int16_t altdiff )
{
        m_crosstrack = crosstrack;
        m_altdiff = altdiff;
}

#ifdef HAVE_GTKMM2
bool Navigate::CDI::on_expose_event(GdkEventExpose *event)
{
        Glib::RefPtr<Gdk::Window> wnd(get_window());
        if (!wnd)
                return false;
        Cairo::RefPtr<Cairo::Context> cr(wnd->create_cairo_context());
	if (event) {
		GdkRectangle *rects;
		int n_rects;
		gdk_region_get_rectangles(event->region, &rects, &n_rects);
		for (int i = 0; i < n_rects; i++)
			cr->rectangle(rects[i].x, rects[i].y, rects[i].width, rects[i].height);
		cr->clip();
		g_free(rects);
	}
        return on_draw(cr);
}
#endif

bool Navigate::CDI::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	int width, height;
	{
		Gtk::Allocation allocation(get_allocation());
		width = allocation.get_width();
		height = allocation.get_height();
	}
	cr->save();
        cr->set_source_rgb(1.0, 1.0, 1.0);
        cr->paint();
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->set_line_width(1.0);
        cr->translate(width * 0.5, height * 0.5);
        cr->move_to(-width / 2 + 1, -(m_altdiff * height * (1.0 / 2000.0)));
        cr->rel_line_to(width + 2, 0);
        cr->move_to(m_crosstrack * width * (1.0 / 4.0), -height / 2 + 1);
        cr->rel_line_to(0, height + 2);
        cr->stroke();
	cr->restore();
        return true;
}

