//
// C++ Implementation: Windows Sensors API Baro Sensor
//
// Description: Windows Sensors API Baro Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <iomanip>

#include "senswinbaro.h"

class Sensors::SensorWinBaro::SensorManagerEvents : public ISensorManagerEvents {
public:
	SensorManagerEvents(SensorWinBaro *baro) : m_baro(baro), m_cRef(0) {}
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
		m_baro->try_connect();
		return S_OK;
	}

private:
	SensorWinBaro *m_baro;
        long m_cRef;
};

class Sensors::SensorWinBaro::SensorEvents : public ISensorEvents {
public:
	SensorEvents(SensorWinBaro *baro) : m_baro(baro), m_cRef(0) {}
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
		m_baro->baro_newdata(pSensor, pNewData);
		return S_OK;
	}

	STDMETHODIMP OnLeave(REFSENSOR_ID sensorID)
	{
		m_baro->close();
		return S_OK;
	}

	STDMETHODIMP OnStateChanged(ISensor* pSensor, SensorState state)
	{
		if (NULL == pSensor)
			return E_INVALIDARG;
		m_baro->baro_statechange(pSensor, state);
		return S_OK;
	}

private:
	SensorWinBaro *m_baro;
        long m_cRef;
};


Sensors::SensorWinBaro::SensorWinBaro(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorInstance(sensors, configgroup), m_sensmgr(0), m_barosens(0),
	  m_rawalt(std::numeric_limits<double>::quiet_NaN()), m_altfilter(1.0), m_altratefilter(0.05),
	  m_press(std::numeric_limits<double>::quiet_NaN()),  m_temp(std::numeric_limits<double>::quiet_NaN()),
	  m_alt(std::numeric_limits<double>::quiet_NaN()), m_altrate(std::numeric_limits<double>::quiet_NaN()),
	  m_priority(2)
{
        // get configuration
        const Glib::KeyFile& cf(get_sensors().get_configfile());
        if (!cf.has_key(get_configgroup(), "priority"))
                get_sensors().get_configfile().set_integer(get_configgroup(), "priority", m_priority);
        m_priority = cf.get_integer(get_configgroup(), "priority");
        if (!cf.has_key(get_configgroup(), "altfilter"))
                get_sensors().get_configfile().set_double(get_configgroup(), "altfilter", m_altfilter);
        m_altfilter = cf.get_double(get_configgroup(), "altfilter");
        if (!cf.has_key(get_configgroup(), "vsfilter"))
                get_sensors().get_configfile().set_double(get_configgroup(), "vsfilter", m_altratefilter);
        m_altratefilter = cf.get_double(get_configgroup(), "vsfilter");
	// find sensor manager
	HRESULT hr(::CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_sensmgr)));
	if (FAILED(hr)) {
		get_sensors().log(Sensors::loglevel_warning, "winbaro: cannot create sensor manager");
		m_sensmgr = 0;
		return;
	}
	SensorManagerEvents *pSensorManagerEventClass(new SensorManagerEvents(this));
	ISensorManagerEvents *pSensorManagerEvents(0);
	hr = pSensorManagerEventClass->QueryInterface(IID_PPV_ARGS(&pSensorManagerEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winbaro: cannot get sensor manager events interface");
		delete pSensorManagerEventClass;
		m_sensmgr->Release();
		m_sensmgr = 0;
		return;
	}
	hr = m_sensmgr->SetEventSink(pSensorManagerEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winbaro: cannot set sensor manager event sink");
		pSensorManagerEvents->Release();
		m_sensmgr->Release();
		m_sensmgr = 0;
		return;
	}
        try_connect();
}

Sensors::SensorWinBaro::~SensorWinBaro()
{
	close();
	if (m_sensmgr)
		m_sensmgr->Release();
}

bool Sensors::SensorWinBaro::close(void)
{
	bool was_open(!!m_barosens);
	if (was_open)
		m_barosens->SetEventSink(0);
	m_barosens = 0;
	m_rawalt = m_press = m_temp = m_alt = m_altrate = std::numeric_limits<double>::quiet_NaN();
	m_time = Glib::TimeVal();
	if (was_open) {
		ParamChanged pc;
		pc.set_changed(parnrstart, parnrdevserial);
		pc.set_changed(parnralt, parnrend - 1);
		update(pc);
	}
        return was_open;
}

bool Sensors::SensorWinBaro::try_connect(void)
{
        if (close()) {
                ParamChanged pc;
                pc.set_changed(parnralt, parnrtemp);
                pc.set_changed(parnrstart, parnrend - 1);
                update(pc);
        }
	if (!m_sensmgr)
		return true;
	ISensorCollection *pSensorCollection(0);
	HRESULT hr(m_sensmgr->GetSensorsByCategory(SENSOR_CATEGORY_ENVIRONMENTAL, &pSensorCollection));
	if (FAILED(hr))
		return true;
	ULONG senscount(0);
	hr = pSensorCollection->GetCount(&senscount);
	if (FAILED(hr)) {
		if (pSensorCollection)
			pSensorCollection->Release();
		return true;
	}
	ISensor *barosens(0);
	for (ULONG i = 0; i < senscount; ++i) {
		ISensor *pSensor(0);
		hr = pSensorCollection->GetAt(i, &pSensor);
		if (FAILED(hr))
			continue;
		VARIANT_BOOL bSupported(VARIANT_FALSE);
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_ATMOSPHERIC_PRESSURE_BAR, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		barosens = pSensor;
		break;
	}
	if (pSensorCollection)
		pSensorCollection->Release();
	if (!barosens) {
                get_sensors().log(Sensors::loglevel_notice, "winbaro: no suitable barometric pressure sensor found");
		return true;
	}
	SensorState sensstate(SENSOR_STATE_ERROR);
	hr = barosens->GetState(&sensstate);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winbaro: cannot get sensor state");
		return true;
	}
	if (sensstate == SENSOR_STATE_ACCESS_DENIED) {
		// Make a SensorCollection with only the sensors we want to get permission to access.
		ISensorCollection *senscoll(0);
		hr = ::CoCreateInstance(IID_ISensorCollection, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&senscoll));
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "winbaro: cannot create a sensor collection");
			return true;
		}
		senscoll->Clear();
		senscoll->Add(barosens);
		hr = m_sensmgr->RequestPermissions(NULL, senscoll, TRUE);
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "winbaro: no permission to access the BARO sensor");
			return true;
		}
	}
	SensorEvents *pSensorEventClass(new SensorEvents(this));
	ISensorEvents *pSensorEvents(0);
	hr = pSensorEventClass->QueryInterface(IID_PPV_ARGS(&pSensorEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winbaro: cannot get sensor events interface");
		delete pSensorEventClass;
		return true;
	}
	hr = barosens->SetEventSink(pSensorEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winbaro: cannot set sensor event sink");
		pSensorEvents->Release();
		return true;
	}
	m_barosens = barosens;
	return false;
}

// Parameter Documentation: http://msdn.microsoft.com/en-us/library/windows/desktop/dd318981%28v=vs.85%29.aspx

void Sensors::SensorWinBaro::baro_newdata(ISensor *pSensor, ISensorDataReport *pNewData)
{
	Glib::TimeVal tv;
	tv.assign_current_time();
        ParamChanged pc;
	PROPVARIANT pv = {};
        HRESULT hr(pNewData->GetSensorValue(SENSOR_DATA_TYPE_TEMPERATURE_CELSIUS, &pv));
	if (SUCCEEDED(hr) && pv.vt == VT_R4) {
		m_temp = pv.fltVal;
		pc.set_changed(parnrtemp);
 	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ATMOSPHERIC_PRESSURE_BAR, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R4) {
		m_press = pv.fltVal * 0.001f;
		Glib::TimeVal tdiff(m_time);
		m_time = tv;
		tdiff -= m_time;
		if (true)
			std::cerr << "winbaro: new result: press " << m_press
				  << " temp " << m_temp << " time since last " << -tdiff.as_double() << std::endl;
		{
			Glib::TimeVal tdiff2(tdiff);
			tdiff2.add_seconds(10);
			float a(0);
			m_icao.pressure_to_altitude(&a, 0, m_press);
			a *= FPlanWaypoint::m_to_ft;
			if (std::isnan(m_alt) || tdiff2.negative())
				m_alt = a;
			else
				m_alt += (a - m_alt) * m_altfilter;
			double adiff(a - m_rawalt);
			m_rawalt = a;
			if (tdiff.negative()) {
				adiff *= (-60.0) / tdiff.as_double();
				if (std::isnan(m_altrate) || tdiff2.negative())
					m_altrate = adiff;
				else
					m_altrate += (adiff - m_altrate) * m_altratefilter;
			} else {
				m_altrate = std::numeric_limits<double>::quiet_NaN();
			}
		}
		pc.set_changed(parnralt, parnrpress);
	}
        PropVariantClear(&pv);
}

void Sensors::SensorWinBaro::baro_statechange(ISensor* pSensor, SensorState state)
{
	if (state == SENSOR_STATE_READY) {
                get_sensors().log(Sensors::loglevel_warning, "winbaro: sensor ready");
		return;
	}
	if (state == SENSOR_STATE_ACCESS_DENIED) {
		get_sensors().log(Sensors::loglevel_warning, "winbaro: sensor permission denied");
		close();
		return;
	}
}

void Sensors::SensorWinBaro::get_baroalt(double& alt, double& altrate) const
{
        alt = m_alt;
        altrate = m_altrate;
}

unsigned int Sensors::SensorWinBaro::get_baroalt_priority(void) const
{
        return m_priority;
}

Glib::TimeVal Sensors::SensorWinBaro::get_baroalt_time(void) const
{
        return m_time;
}

void Sensors::SensorWinBaro::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
        SensorInstance::get_param_desc(pagenr, pd);
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

        pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Measurement", "Current Measurement", ""));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnralt, "Altitude", "Baro Altitude", "ft", 1, -99999.0, 99999.0, 1.0, 10.0));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnraltrate, "Climb", "Climb Rate", "ft/min", 1, -9999.0, 9999.0, 1.0, 10.0));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrpress, "Pressure", "Sensor Pressure", "hPa", 2, 0.0, 1999.0, 1.0, 10.0));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrtemp, "Temperature", "Sensor Temperature", "°C", 1, -99.0, 99.0, 1.0, 10.0));

        pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Priorities", "Priority Values", ""));
        pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrprio, "Alt Prio", "Altitude Priority", "", 0, 0, 9999, 1, 10));

        pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Filter", "Filter Coefficients", ""));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltfilter, "Alt Filter", "Altitude Filter", "", 3, 0.0, 1.0, 0.01, 0.1));
        pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnraltratefilter, "VS Filter", "Vertical Speed (Altitude Rate) Filter", "", 3, 0.0, 1.0, 0.01, 0.1));
}

Sensors::SensorWinBaro::paramfail_t Sensors::SensorWinBaro::get_param(unsigned int nr, Glib::ustring& v) const
{
        switch (nr) {
        default:
                break;

	case parnrdevname:
	{
		v.clear();
		if (!m_barosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_barosens->GetProperty(SENSOR_PROPERTY_FRIENDLY_NAME, &pv));
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
		if (!m_barosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_barosens->GetProperty(SENSOR_PROPERTY_MANUFACTURER, &pv));
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
		if (!m_barosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_barosens->GetProperty(SENSOR_PROPERTY_DESCRIPTION, &pv));
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
		if (!m_barosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_barosens->GetProperty(SENSOR_PROPERTY_SERIAL_NUMBER, &pv));
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
        return SensorInstance::get_param(nr, v);
}

Sensors::SensorWinBaro::paramfail_t Sensors::SensorWinBaro::get_param(unsigned int nr, double& v) const
{
        switch (nr) {
        default:
                break;

        case parnraltfilter:
                v = m_altfilter;
                return paramfail_ok;

        case parnraltratefilter:
                v = m_altratefilter;
                return paramfail_ok;

        case parnralt:
        {
                if (!is_baroalt_ok()) {
                        v = std::numeric_limits<double>::quiet_NaN();
                        return paramfail_fail;
                }
                double v1;
                get_baroalt(v, v1);
                return std::isnan(v) ? paramfail_fail : paramfail_ok;
        }

        case parnraltrate:
        {
                if (!is_baroalt_ok()) {
                        v = std::numeric_limits<double>::quiet_NaN();
                        return paramfail_fail;
                }
                double v1;
                get_baroalt(v1, v);
                return std::isnan(v) ? paramfail_fail : paramfail_ok;
        }

        case parnrpress:
        {
                if (!is_baroalt_ok()) {
                        v = std::numeric_limits<double>::quiet_NaN();
                        return paramfail_fail;
                }
                v = m_press;
                return std::isnan(v) ? paramfail_fail : paramfail_ok;
        }

        case parnrtemp:
        {
                if (!is_baroalt_ok()) {
                        v = std::numeric_limits<double>::quiet_NaN();
                        return paramfail_fail;
                }
                v = m_temp;
                return std::isnan(v) ? paramfail_fail : paramfail_ok;
        }

	}
        return SensorInstance::get_param(nr, v);
}

Sensors::SensorWinBaro::paramfail_t Sensors::SensorWinBaro::get_param(unsigned int nr, int& v) const
{
        switch (nr) {
        default:
                break;

        case parnrprio:
                v = m_priority;
                return paramfail_ok;

        }
        return SensorInstance::get_param(nr, v);
}

void Sensors::SensorWinBaro::set_param(unsigned int nr, const Glib::ustring& v)
{
        switch (nr) {
        default:
                SensorInstance::set_param(nr, v);
                return;

	}
        ParamChanged pc;
        pc.set_changed(nr);
        update(pc);
}

void Sensors::SensorWinBaro::set_param(unsigned int nr, int v)
{
        switch (nr) {
        default:
                SensorInstance::set_param(nr, v);
                return;

        case parnrprio:
                if (m_priority == (unsigned int)v)
                        return;
                m_priority = v;
                get_sensors().get_configfile().set_integer(get_configgroup(), "priority", m_priority);
                break;
        }
        ParamChanged pc;
        pc.set_changed(nr);
        update(pc);
}

void Sensors::SensorWinBaro::set_param(unsigned int nr, double v)
{
        switch (nr) {
        default:
                SensorInstance::set_param(nr, v);
                return;

        case parnraltfilter:
                if (m_altfilter == v)
                        return;
                m_altfilter = v;
                get_sensors().get_configfile().set_double(get_configgroup(), "altfilter", m_altfilter);
                break;

        case parnraltratefilter:
                if (m_altratefilter == v)
                        return;
                m_altratefilter = v;
                get_sensors().get_configfile().set_double(get_configgroup(), "vsfilter", m_altratefilter);
                break;
        }
        ParamChanged pc;
        pc.set_changed(nr);
        update(pc);
}

std::string Sensors::SensorWinBaro::logfile_header(void) const
{
	return SensorInstance::logfile_header() + ",Alt,VS,Press,Temp";
}

std::string Sensors::SensorWinBaro::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorInstance::logfile_records() << std::fixed << std::setprecision(1) << ',';
	if (!std::isnan(m_alt))
		oss << m_alt;
	oss << ',';
	if (!std::isnan(m_altrate))
		oss << m_altrate;
	oss << ',';
	if (!std::isnan(m_press))
		oss << m_press;
	oss << ',';
	if (!std::isnan(m_temp))
		oss << m_temp;
	return oss.str();
}
