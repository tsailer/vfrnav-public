//
// C++ Implementation: Windows Sensors API GPS Sensor
//
// Description: Windows Sensors API GPS Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <iomanip>

#include "senswingps.h"

class Sensors::SensorWinGPS::SensorManagerEvents : public ISensorManagerEvents {
public:
	SensorManagerEvents(SensorWinGPS *gps) : m_gps(gps), m_cRef(0) {}
	virtual ~SensorManagerEvents() {}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
	{
		if (ppv == NULL)
			return E_POINTER;
		if (iid == __uuidof(IUnknown)) {
			*ppv = static_cast<IUnknown*>(this);
		} else if (iid == __uuidof(ISensorManagerEvents)) {
			*ppv = static_cast<ISensorManagerEvents*>(this);
		} else {
			*ppv = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef); 
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG count = InterlockedDecrement(&m_cRef);
		if (!count) {
			delete this;
			return 0;
		}
		return count;
	}

	STDMETHODIMP OnSensorEnter(ISensor *pSensor, SensorState state)
	{
		if (NULL == pSensor)
			return E_INVALIDARG;
		m_gps->try_connect();
		return S_OK;
	}

private:
	SensorWinGPS *m_gps;
        long m_cRef;
};

class Sensors::SensorWinGPS::SensorEvents : public ISensorEvents {
public:
	SensorEvents(SensorWinGPS *gps) : m_gps(gps), m_cRef(0) {}
	virtual ~SensorEvents() {}

	STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
	{
		if (ppv == NULL)
			return E_POINTER;
		if (iid == __uuidof(IUnknown)) {
			*ppv = static_cast<IUnknown*>(this);
		} else if (iid == __uuidof(ISensorEvents)) {
			*ppv = static_cast<ISensorEvents*>(this);
		} else {
			*ppv = NULL;
			return E_NOINTERFACE;
		}
		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef); 
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG count = InterlockedDecrement(&m_cRef);
		if (!count) {
			delete this;
			return 0;
		}
		return count;
	}

	//
	// ISensorEvents methods.
	//

	STDMETHODIMP OnEvent(ISensor *pSensor, REFGUID eventID, IPortableDeviceValues *pEventData)
	{
		HRESULT hr = S_OK;
		// Handle custom events here.
		return hr;
	}

	STDMETHODIMP OnDataUpdated(ISensor *pSensor, ISensorDataReport *pNewData)
	{
		if (NULL == pNewData || NULL == pSensor)
			return E_INVALIDARG;
		m_gps->gps_newdata(pSensor, pNewData);
		return S_OK;
	}

	STDMETHODIMP OnLeave(REFSENSOR_ID sensorID)
	{
		m_gps->close();
		return S_OK;
	}

	STDMETHODIMP OnStateChanged(ISensor* pSensor, SensorState state)
	{
		if (NULL == pSensor)
			return E_INVALIDARG;
		m_gps->gps_statechange(pSensor, state);
		return S_OK;
	}

private:
	SensorWinGPS *m_gps;
        long m_cRef;
};


Sensors::SensorWinGPS::SensorWinGPS(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorGPS(sensors, configgroup), m_sensmgr(0), m_gpssens(0)
{

	HRESULT hr(::CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_sensmgr)));
	if (FAILED(hr)) {
		get_sensors().log(Sensors::loglevel_warning, "wingps: cannot create sensor manager");
		m_sensmgr = 0;
		return;
	}
	SensorManagerEvents *pSensorManagerEventClass(new SensorManagerEvents(this));
	ISensorManagerEvents *pSensorManagerEvents(0);
	hr = pSensorManagerEventClass->QueryInterface(IID_PPV_ARGS(&pSensorManagerEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "wingps: cannot get sensor manager events interface");
		delete pSensorManagerEventClass;
		m_sensmgr->Release();
		m_sensmgr = 0;
		return;
	}
	hr = m_sensmgr->SetEventSink(pSensorManagerEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "wingps: cannot set sensor manager event sink");
		pSensorManagerEvents->Release();
		m_sensmgr->Release();
		m_sensmgr = 0;
		return;
	}
        try_connect();
}

Sensors::SensorWinGPS::~SensorWinGPS()
{
	close();
	if (m_sensmgr)
		m_sensmgr->Release();
}

bool Sensors::SensorWinGPS::close(void)
{
	bool was_open(!!m_gpssens);
	if (was_open)
		m_gpssens->SetEventSink(0);
	m_gpssens = 0;
	m_fixtype = fixtype_nofix;
	m_postime = m_alttime = m_crstime = Glib::TimeVal();
        m_pos = Point();
        m_alt = m_altrate = m_crs = m_groundspeed = 
		m_pdop = m_hdop = m_vdop = m_errl = m_errv = std::numeric_limits<double>::quiet_NaN();
	if (was_open) {
		ParamChanged pc;
		pc.set_changed(parnrstart, parnrend - 1);
		update(pc);
	}
        return was_open;
}

bool Sensors::SensorWinGPS::try_connect(void)
{
        if (close()) {
                ParamChanged pc;
                pc.set_changed(parnrfixtype, parnrgroundspeed);
                pc.set_changed(parnrstart, parnrend - 1);
                update(pc);
        }
	if (!m_sensmgr)
		return true;
	ISensorCollection *pSensorCollection(0);
	HRESULT hr(m_sensmgr->GetSensorsByType(SENSOR_TYPE_LOCATION_GPS, &pSensorCollection));
	if (FAILED(hr))
		return true;
	ULONG senscount(0);
	hr = pSensorCollection->GetCount(&senscount);
	if (FAILED(hr)) {
		if (pSensorCollection)
			pSensorCollection->Release();
		return true;
	}
	ISensor *gpssens(0);
	for (ULONG i = 0; i < senscount; ++i) {
		ISensor *pSensor(0);
		hr = pSensorCollection->GetAt(i, &pSensor);
		if (FAILED(hr))
			continue;
		VARIANT_BOOL bSupported(VARIANT_FALSE);
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_LATITUDE_DEGREES, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		bSupported = VARIANT_FALSE;
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_LONGITUDE_DEGREES, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		gpssens = pSensor;
		break;
	}
	if (pSensorCollection)
		pSensorCollection->Release();
	if (!gpssens) {
                get_sensors().log(Sensors::loglevel_notice, "wingps: no suitable GPS sensor found");
		return true;
	}
	SensorState sensstate(SENSOR_STATE_ERROR);
	hr = gpssens->GetState(&sensstate);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "wingps: cannot get sensor state");
		return true;
	}
	if (sensstate == SENSOR_STATE_ACCESS_DENIED) {
		// Make a SensorCollection with only the sensors we want to get permission to access.
		ISensorCollection *senscoll(0);
		hr = ::CoCreateInstance(IID_ISensorCollection, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&senscoll));
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "wingps: cannot create a sensor collection");
			return true;
		}
		senscoll->Clear();
		senscoll->Add(gpssens);
		hr = m_sensmgr->RequestPermissions(NULL, senscoll, TRUE);
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "wingps: no permission to access the GPS sensor");
			return true;
		}
	}
	// set sampling rate
	{
		IPortableDeviceValues *propsset(0);
		hr = CoCreateInstance(__uuidof(PortableDeviceValues), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&propsset));
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "wingps: cannot create device values");
			return true;
		}
		hr = propsset->SetUnsignedIntegerValue(SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, 500);
		if (FAILED(hr)) {
			propsset->Release();
			get_sensors().log(Sensors::loglevel_warning, "wingps: cannot set reporting interval");
			return true;
		}
		IPortableDeviceValues *propsrecv(0);
		hr = gpssens->SetProperties(propsset, &propsrecv);
		if (FAILED(hr)) {
			propsset->Release();
			if (propsrecv)
				propsrecv->Release();
			get_sensors().log(Sensors::loglevel_warning, "wingps: cannot set default reporting interval");
			return true;
		}
		propsrecv->Release();
		propsset->Release();
		PROPVARIANT pv = {};
		hr = gpssens->GetProperty(SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, &pv);
		if (FAILED(hr) || pv.vt != VT_UI4) {
			get_sensors().log(Sensors::loglevel_warning, "wingps: cannot get reporting interval");
			return true;
		}
		ULONG interval(pv.ulVal);
		PropVariantClear(&pv);
		std::ostringstream oss;
		oss << "wingps: reporting interval " << interval << "ms";
		get_sensors().log(Sensors::loglevel_notice, oss.str());
	}
	SensorEvents *pSensorEventClass(new SensorEvents(this));
	ISensorEvents *pSensorEvents(0);
	hr = pSensorEventClass->QueryInterface(IID_PPV_ARGS(&pSensorEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "wingps: cannot get sensor events interface");
		delete pSensorEventClass;
		return true;
	}
	hr = gpssens->SetEventSink(pSensorEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "wingps: cannot set sensor event sink");
		pSensorEvents->Release();
		return true;
	}
	m_gpssens = gpssens;
	return false;
}

// Parameter Documentation: http://msdn.microsoft.com/en-us/library/windows/desktop/dd318981%28v=vs.85%29.aspx

void Sensors::SensorWinGPS::gps_newdata(ISensor *pSensor, ISensorDataReport *pNewData)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
        ParamChanged pc;
	PROPVARIANT pv = {};
        HRESULT hr(pNewData->GetSensorValue(SENSOR_DATA_TYPE_LATITUDE_DEGREES, &pv));
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_pos.set_lat_deg_dbl(pv.dblVal);
		m_postime = tv;
		pc.set_changed(parnrlat);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_LONGITUDE_DEGREES, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_pos.set_lon_deg_dbl(pv.dblVal);
		m_postime = tv;
		pc.set_changed(parnrlon);
 	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ALTITUDE_SEALEVEL_METERS, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_alt = pv.dblVal * Point::m_to_ft_dbl;
		m_alttime = tv;
                pc.set_changed(parnralt);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_SPEED_KNOTS, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_groundspeed = pv.dblVal;
		pc.set_changed(parnrgroundspeed);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_TRUE_HEADING_DEGREES, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_crs = pv.dblVal;
		m_crstime = tv;
		pc.set_changed(parnrcourse);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_FIX_TYPE, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_I4) {
		switch (pv.lVal) {
		default:
			m_fixtype = fixtype_nofix;
			break;

		case 1:
		case 3:
			m_fixtype = fixtype_3d;
			break;

		case 2:
			m_fixtype = fixtype_3d_diff;
			break;
		}
		pc.set_changed(parnrfixtype, parnrsatellites);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_POSITION_DILUTION_OF_PRECISION, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_pdop = pv.dblVal;
		pc.set_changed(parnrpdop);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_HORIZONAL_DILUTION_OF_PRECISION, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_hdop = pv.dblVal;
		pc.set_changed(parnrhdop);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_VERTICAL_DILUTION_OF_PRECISION, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_vdop = pv.dblVal;
		pc.set_changed(parnrvdop);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ERROR_RADIUS_METERS, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_errl = pv.dblVal;
		pc.set_changed(parnrerrl);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ALTITUDE_SEALEVEL_ERROR_METERS, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_errv = pv.dblVal;
		pc.set_changed(parnrerrv);
	}
	PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_SATELLITES_IN_VIEW_PRNS, &pv);
	if (SUCCEEDED(hr) && pv.vt == (VT_UI1 | VT_VECTOR)) {
		unsigned int nrelem(pv.caub.cElems / sizeof(uint32_t));
		const uint32_t *elem((uint32_t *)pv.caub.pElems);
		nrelem = std::min(nrelem, 64U);
		m_sat.resize(nrelem, Satellite(0, 0, 0, false));
		for (unsigned int i = 0; i < nrelem; ++i) {
			Satellite& s(m_sat[i]);
			s = Satellite(elem[i], s.get_azimuth(), s.get_elevation(), s.get_snr(), s.is_used());
		}
		pc.set_changed(parnrsatellites);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_SATELLITES_IN_VIEW_ELEVATION, &pv);
	if (SUCCEEDED(hr) && pv.vt == (VT_UI1 | VT_VECTOR)) {
		unsigned int nrelem(pv.caub.cElems / sizeof(double));
		const double *elem((double *)pv.caub.pElems);
		if (nrelem > m_sat.size())
			nrelem = m_sat.size();
		for (unsigned int i = 0; i < nrelem; ++i) {
			Satellite& s(m_sat[i]);
			s = Satellite(s.get_prn(), s.get_azimuth(), elem[i], s.get_snr(), s.is_used());
		}
		pc.set_changed(parnrsatellites);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_SATELLITES_IN_VIEW_AZIMUTH, &pv);
	if (SUCCEEDED(hr) && pv.vt == (VT_UI1 | VT_VECTOR)) {
		unsigned int nrelem(pv.caub.cElems / sizeof(double));
		const double *elem((double *)pv.caub.pElems);
		if (nrelem > m_sat.size())
			nrelem = m_sat.size();
		for (unsigned int i = 0; i < nrelem; ++i) {
			Satellite& s(m_sat[i]);
			s = Satellite(s.get_prn(), elem[i], s.get_elevation(), s.get_snr(), s.is_used());
		}
		pc.set_changed(parnrsatellites);
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_SATELLITES_IN_VIEW_STN_RATIO, &pv);
	if (SUCCEEDED(hr) && pv.vt == (VT_UI1 | VT_VECTOR)) {
		unsigned int nrelem(pv.caub.cElems / sizeof(double));
		const double *elem((double *)pv.caub.pElems);
		if (nrelem > m_sat.size())
			nrelem = m_sat.size();
		for (unsigned int i = 0; i < nrelem; ++i) {
			Satellite& s(m_sat[i]);
			s = Satellite(s.get_prn(), s.get_azimuth(), s.get_elevation(), elem[i], s.is_used());
		}
		pc.set_changed(parnrsatellites);
	}
	PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_SATELLITES_USED_PRNS, &pv);
	if (SUCCEEDED(hr) && pv.vt == (VT_UI1 | VT_VECTOR)) {
		unsigned int nrelem(pv.caub.cElems / sizeof(uint32_t));
		const uint32_t *elem((uint32_t *)pv.caub.pElems);
		std::set<uint32_t> prns;
		for (unsigned int i = 0; i < nrelem; ++i)
			if (elem[i])
				prns.insert(elem[i]);
		for (satellites_t::iterator i(m_sat.begin()), e(m_sat.end()); i != e; ++i) {
			if (!i->get_prn())
				continue;
			*i = Satellite(i->get_prn(), i->get_azimuth(), i->get_elevation(), i->get_snr(),
				       prns.find((uint32_t)i->get_prn()) != prns.end());
		}
		pc.set_changed(parnrsatellites);
	}
        PropVariantClear(&pv);
	for (satellites_t::iterator i(m_sat.begin()), e(m_sat.end()); i != e; ) {
		if (i->get_prn()) {
			++i;
			continue;
		}
		i = m_sat.erase(i);
		e = m_sat.end();
		pc.set_changed(parnrsatellites);
	}
        update(pc);
}

void Sensors::SensorWinGPS::gps_statechange(ISensor* pSensor, SensorState state)
{
	if (state == SENSOR_STATE_READY) {
                get_sensors().log(Sensors::loglevel_warning, "wingps: sensor ready");
		return;
	}
	if (state == SENSOR_STATE_ACCESS_DENIED) {
		get_sensors().log(Sensors::loglevel_warning, "wingps: sensor permission denied");
		close();
		return;
	}
}

bool Sensors::SensorWinGPS::get_position(Point& pt) const
{
        if (m_fixtype < fixtype_2d) {
                pt = Point();
                return false;
        }
        pt = m_pos;
        return true;
}

Glib::TimeVal Sensors::SensorWinGPS::get_position_time(void) const
{
        return m_postime;
}

void Sensors::SensorWinGPS::get_truealt(double& alt, double& altrate) const
{
        alt = altrate = std::numeric_limits<double>::quiet_NaN();
        if (m_fixtype < fixtype_3d)
                return;
        alt = m_alt;
        altrate = m_altrate;
}

Glib::TimeVal Sensors::SensorWinGPS::get_truealt_time(void) const
{
        return m_alttime;
}

void Sensors::SensorWinGPS::get_course(double& crs, double& gs) const
{
        crs = gs = std::numeric_limits<double>::quiet_NaN();
        if (m_fixtype < fixtype_2d)
                return;
        crs = m_crs;
        gs = m_groundspeed;
}

Glib::TimeVal Sensors::SensorWinGPS::get_course_time(void) const
{
        return m_crstime;
}

void Sensors::SensorWinGPS::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
        SensorGPS::get_param_desc(pagenr, pd);
	{
                paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
                if (pdi != pde)
                        ++pdi;
                for (; pdi != pde; ++pdi)
                        if (pdi->get_type() == ParamDesc::type_section)
                                break;
                pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrdevname, "Device Name", "Device Name", ""));
                ++pdi;
                pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrdevmfg, "Manufacturer", "Device Manufacturer", ""));
                ++pdi;
                pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrdevdesc, "Description", "Device Description", ""));
                ++pdi;
                pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrdevserial, "Serial", "Device Serial Number", ""));
        }
        pd.push_back(ParamDesc(ParamDesc::type_satellites, ParamDesc::editable_readonly, parnrsatellites, "Satellites", "Satellite Azimuth/Elevation and Signal Strengths", ""));

        pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Uncertainties", "Uncertainties", ""));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrpdop, "PDOP", "Positional Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrhdop, "HDOP", "Horizontal Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrvdop, "VDOP", "Vertical Dilution of Precision", "", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrvdop, "Lateral Error", "Estimated Lateral Error", "m", 3, -999999, 999999, 0.0, 0.0));
	pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrvdop, "Vertical Error", "Estimated Vertical Error", "m", 3, -999999, 999999, 0.0, 0.0));
}

Sensors::SensorWinGPS::paramfail_t Sensors::SensorWinGPS::get_param(unsigned int nr, Glib::ustring& v) const
{
        switch (nr) {
        default:
                break;

	case parnrdevname:
	{
		v.clear();
		if (!m_gpssens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gpssens->GetProperty(SENSOR_PROPERTY_FRIENDLY_NAME, &pv));
		if (FAILED(hr))
			return paramfail_fail;
		if (pv.vt != VT_LPWSTR)
			return paramfail_fail;
		gchar *p(g_utf16_to_utf8((gunichar2 *)pv.pwszVal, -1, NULL, NULL, NULL));
		PropVariantClear(&pv);
		if (!p)
			return paramfail_fail;
		v = p;
		g_free(p);
		return paramfail_ok;
	}

	case parnrdevmfg:
	{
		v.clear();
		if (!m_gpssens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gpssens->GetProperty(SENSOR_PROPERTY_MANUFACTURER, &pv));
		if (FAILED(hr))
			return paramfail_fail;
		if (pv.vt != VT_LPWSTR)
			return paramfail_fail;
		gchar *p(g_utf16_to_utf8((gunichar2 *)pv.pwszVal, -1, NULL, NULL, NULL));
		PropVariantClear(&pv);
		if (!p)
			return paramfail_fail;
		v = p;
		g_free(p);
		return paramfail_ok;
	}

	case parnrdevdesc:
	{
		v.clear();
		if (!m_gpssens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gpssens->GetProperty(SENSOR_PROPERTY_DESCRIPTION, &pv));
		if (FAILED(hr))
			return paramfail_fail;
		if (pv.vt != VT_LPWSTR)
			return paramfail_fail;
		gchar *p(g_utf16_to_utf8((gunichar2 *)pv.pwszVal, -1, NULL, NULL, NULL));
		PropVariantClear(&pv);
		if (!p)
			return paramfail_fail;
		v = p;
		g_free(p);
		return paramfail_ok;
	}

	case parnrdevserial:
	{
		v.clear();
		if (!m_gpssens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gpssens->GetProperty(SENSOR_PROPERTY_SERIAL_NUMBER, &pv));
		if (FAILED(hr))
			return paramfail_fail;
		if (pv.vt != VT_LPWSTR)
			return paramfail_fail;
		gchar *p(g_utf16_to_utf8((gunichar2 *)pv.pwszVal, -1, NULL, NULL, NULL));
		PropVariantClear(&pv);
		if (!p)
			return paramfail_fail;
		v = p;
		g_free(p);
		return paramfail_ok;
	}

        }
        return SensorGPS::get_param(nr, v);
}

Sensors::SensorWinGPS::paramfail_t Sensors::SensorWinGPS::get_param(unsigned int nr, double& v) const
{
        switch (nr) {
        default:
                break;

	case parnrpdop:
		v = m_pdop;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrhdop:
		v = m_hdop;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrvdop:
		v = m_vdop;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrerrl:
		v = m_errl;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;

	case parnrerrv:
		v = m_errv;
		return m_fixtype < fixtype_2d ? paramfail_fail : paramfail_ok;
        }
        return SensorGPS::get_param(nr, v);
}

Sensors::SensorWinGPS::paramfail_t Sensors::SensorWinGPS::get_param(unsigned int nr, int& v) const
{
        switch (nr) {
        default:
                break;

        }
        return SensorGPS::get_param(nr, v);
}

Sensors::SensorWinGPS::paramfail_t Sensors::SensorWinGPS::get_param(unsigned int nr, satellites_t& sat) const
{
        sat = m_sat;
        return m_gpssens ? paramfail_ok : paramfail_fail;
}

void Sensors::SensorWinGPS::set_param(unsigned int nr, const Glib::ustring& v)
{
        switch (nr) {
        default:
                SensorGPS::set_param(nr, v);
                return;

	}
        ParamChanged pc;
        pc.set_changed(nr);
        update(pc);
}

std::string Sensors::SensorWinGPS::logfile_header(void) const
{
	return SensorGPS::logfile_header() + ",FixTime,Lat,Lon,Alt,VS,CRS,GS,PDOP,HDOP,VDOP,ErrL,ErrV,NrSat";
}

std::string Sensors::SensorWinGPS::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorGPS::logfile_records() << std::fixed << std::setprecision(6) << ',';
	if (m_postime.valid() && m_fixtype >= fixtype_2d) {
                Glib::DateTime dt(Glib::DateTime::create_now_utc(m_postime));
                oss << dt.format("%Y-%m-%d %H:%M:%S");
	}
	oss << ',';
	if (m_fixtype < fixtype_2d || m_pos.is_invalid())
		oss << ',';
	else
		oss << m_pos.get_lat_deg_dbl() << ',' << m_pos.get_lon_deg_dbl();
	oss << ',';
	if (!std::isnan(m_alt))
		oss << m_alt;
	oss << ',';
	if (!std::isnan(m_altrate))
		oss << m_altrate;
	oss << ',';
	if (!std::isnan(m_crs))
		oss << m_crs;
	oss << ',';
	if (!std::isnan(m_groundspeed))
		oss << m_groundspeed;
	oss << ',';
	if (!std::isnan(m_pdop))
		oss << m_pdop;
	oss << ',';
	if (!std::isnan(m_hdop))
		oss << m_hdop;
	oss << ',';
	if (!std::isnan(m_vdop))
		oss << m_vdop;
	oss << ',';
	if (!std::isnan(m_errl))
		oss << m_errl;
	oss << ',';
	if (!std::isnan(m_errv))
		oss << m_errv;
	oss << ',' << m_sat.size();
	return oss.str();
}
