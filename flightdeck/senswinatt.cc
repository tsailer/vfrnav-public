//
// C++ Implementation: Attitude Sensor: Windows Sensors API
//
// Description: Attitude Sensor Handling
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE 1

#include "sysdeps.h"

#include <iostream>
#include <iomanip>

#include "senswinatt.h"

class Sensors::SensorWinAttitude::SensorManagerEvents : public ISensorManagerEvents {
public:
	SensorManagerEvents(SensorWinAttitude *att) : m_att(att), m_cRef(0) {}
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
		m_att->try_connect();
		return S_OK;
	}

private:
	SensorWinAttitude *m_att;
        long m_cRef;
};

class Sensors::SensorWinAttitude::SensorAccelEvents : public ISensorEvents {
public:
	SensorAccelEvents(SensorWinAttitude *att) : m_att(att), m_cRef(0) {}
	virtual ~SensorAccelEvents() {}

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
		m_att->accel_newdata(pSensor, pNewData);
		return S_OK;
	}

	STDMETHODIMP OnLeave(REFSENSOR_ID sensorID)
	{
		m_att->close();
		return S_OK;
	}

	STDMETHODIMP OnStateChanged(ISensor* pSensor, SensorState state)
	{
		if (NULL == pSensor)
			return E_INVALIDARG;
		m_att->accel_statechange(pSensor, state);
		return S_OK;
	}

private:
	SensorWinAttitude *m_att;
        long m_cRef;
};

class Sensors::SensorWinAttitude::SensorGyroEvents : public ISensorEvents {
public:
	SensorGyroEvents(SensorWinAttitude *att) : m_att(att), m_cRef(0) {}
	virtual ~SensorGyroEvents() {}

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
		m_att->gyro_newdata(pSensor, pNewData);
		return S_OK;
	}

	STDMETHODIMP OnLeave(REFSENSOR_ID sensorID)
	{
		m_att->close();
		return S_OK;
	}

	STDMETHODIMP OnStateChanged(ISensor* pSensor, SensorState state)
	{
		if (NULL == pSensor)
			return E_INVALIDARG;
		m_att->gyro_statechange(pSensor, state);
		return S_OK;
	}

private:
	SensorWinAttitude *m_att;
        long m_cRef;
};

class Sensors::SensorWinAttitude::SensorMagEvents : public ISensorEvents {
public:
	SensorMagEvents(SensorWinAttitude *att) : m_att(att), m_cRef(0) {}
	virtual ~SensorMagEvents() {}

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
		m_att->mag_newdata(pSensor, pNewData);
		return S_OK;
	}

	STDMETHODIMP OnLeave(REFSENSOR_ID sensorID)
	{
		m_att->close();
		return S_OK;
	}

	STDMETHODIMP OnStateChanged(ISensor* pSensor, SensorState state)
	{
		if (NULL == pSensor)
			return E_INVALIDARG;
		m_att->mag_statechange(pSensor, state);
		return S_OK;
	}

private:
	SensorWinAttitude *m_att;
        long m_cRef;
};

inline HRESULT InitPropVariantFromUInt32(ULONG ulVal, PROPVARIANT *ppropvar)
{
	if (!ppropvar)
		return E_POINTER;
	ppropvar->vt = VT_UI4;
	ppropvar->ulVal = ulVal;
	return S_OK;
}

Sensors::SensorWinAttitude::SensorWinAttitude(Sensors& sensors, const Glib::ustring& configgroup)
	: SensorAttitude(sensors, configgroup), m_sensmgr(0), m_accelsens(0), m_gyrosens(0), m_magsens(0), m_ahrs(50, "", AHRS::init_standard), 
	  m_pitch(std::numeric_limits<double>::quiet_NaN()), m_bank(std::numeric_limits<double>::quiet_NaN()),
	  m_slip(std::numeric_limits<double>::quiet_NaN()), m_rate(std::numeric_limits<double>::quiet_NaN()),
	  m_hdg(std::numeric_limits<double>::quiet_NaN()), m_dirty(false), m_samplecount(0),
	  m_attitudeprio(0), m_headingprio(0)
{
        // get configuration
	const Glib::KeyFile& cf(get_sensors().get_configfile());
	if (!cf.has_key(get_configgroup(), "attitudepriority"))
		get_sensors().get_configfile().set_uint64(get_configgroup(), "attitudepriority", m_attitudeprio);
	m_attitudeprio = cf.get_uint64(get_configgroup(), "attitudepriority");
	if (!cf.has_key(get_configgroup(), "headingpriority"))
		get_sensors().get_configfile().set_uint64(get_configgroup(), "headingpriority", m_headingprio);
	m_headingprio = cf.get_uint64(get_configgroup(), "headingpriority");
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
	// find sensor manager
	HRESULT hr(::CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_sensmgr)));
	if (FAILED(hr)) {
		get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot create sensor manager");
		m_sensmgr = 0;
		return;
	}
	SensorManagerEvents *pSensorManagerEventClass(new SensorManagerEvents(this));
	ISensorManagerEvents *pSensorManagerEvents(0);
	hr = pSensorManagerEventClass->QueryInterface(IID_PPV_ARGS(&pSensorManagerEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get sensor manager events interface");
		delete pSensorManagerEventClass;
		m_sensmgr->Release();
		m_sensmgr = 0;
		return;
	}
	hr = m_sensmgr->SetEventSink(pSensorManagerEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set sensor manager event sink");
		pSensorManagerEvents->Release();
		m_sensmgr->Release();
		m_sensmgr = 0;
		return;
	}
        try_connect();
}

Sensors::SensorWinAttitude::~SensorWinAttitude()
{
	// prevent calling the update event handler, which would call into get_params
	close_internal();
	if (m_sensmgr)
		m_sensmgr->Release();
}

bool Sensors::SensorWinAttitude::close_internal(void)
{
	bool wasopen(is_open());
	if (m_accelsens)
		m_accelsens->SetEventSink(0);	
	m_accelsens = 0;
	if (m_gyrosens)
		m_gyrosens->SetEventSink(0);	
	m_gyrosens = 0;
	if (m_magsens)
		m_magsens->SetEventSink(0);	
	m_magsens = 0;
	if (wasopen) {
		m_ahrs.save(get_sensors().get_configfile(), get_configgroup());
		m_ahrs.save();
	}
	m_ahrs.set_id("");
	m_attitudetime = Glib::TimeVal();
	m_attitudereporttime = Glib::TimeVal();
	m_pitch = m_bank = m_slip = m_rate = m_hdg = std::numeric_limits<double>::quiet_NaN();
	m_dirty = false;
	m_samplecount = 0;
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = 0;
	return wasopen;
}

bool Sensors::SensorWinAttitude::close(void)
{
	if (close_internal()) {
		ParamChanged pc;
		pc.set_changed(parnracceldevname, parnrmagdevserial);
		pc.set_changed(parnrmode, parnrgyro);
		update(pc);
		return true;
	}
	return false;
}

bool Sensors::SensorWinAttitude::try_connect(void)
{
	if (close()) {
                ParamChanged pc;
		pc.set_changed(parnracceldevname, parnrmagdevserial);
		pc.set_changed(parnrmode, parnrgyro);
                update(pc);
        }
	if (!m_sensmgr)
		return true;
	ISensorCollection *pSensorCollection(0);
	HRESULT hr(m_sensmgr->GetSensorsByType(SENSOR_TYPE_ACCELEROMETER_3D, &pSensorCollection));
	if (FAILED(hr))
		return true;
	ULONG senscount(0);
	hr = pSensorCollection->GetCount(&senscount);
	if (FAILED(hr)) {
		if (pSensorCollection)
			pSensorCollection->Release();
		return true;
	}
	ISensor *accelsens(0);
	for (ULONG i = 0; i < senscount; ++i) {
		ISensor *pSensor(0);
		hr = pSensorCollection->GetAt(i, &pSensor);
		if (FAILED(hr))
			continue;
		VARIANT_BOOL bSupported(VARIANT_FALSE);
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_ACCELERATION_X_G, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		bSupported = VARIANT_FALSE;
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_ACCELERATION_Y_G, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		bSupported = VARIANT_FALSE;
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_ACCELERATION_Z_G, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		accelsens = pSensor;
		break;
	}
	if (pSensorCollection)
		pSensorCollection->Release();
	if (!accelsens) {
                get_sensors().log(Sensors::loglevel_notice, "winattitude: no suitable acceleration sensor found");
		return true;
	}
	pSensorCollection = 0;
	hr = m_sensmgr->GetSensorsByType(SENSOR_TYPE_GYROMETER_3D, &pSensorCollection);
	if (FAILED(hr))
		return true;
	senscount = 0;
	hr = pSensorCollection->GetCount(&senscount);
	if (FAILED(hr)) {
		if (pSensorCollection)
			pSensorCollection->Release();
		return true;
	}
	ISensor *gyrosens(0);
	for (ULONG i = 0; i < senscount; ++i) {
		ISensor *pSensor(0);
		hr = pSensorCollection->GetAt(i, &pSensor);
		if (FAILED(hr))
			continue;
		VARIANT_BOOL bSupported(VARIANT_FALSE);
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_X_DEGREES_PER_SECOND, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		bSupported = VARIANT_FALSE;
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Y_DEGREES_PER_SECOND, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		bSupported = VARIANT_FALSE;
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Z_DEGREES_PER_SECOND, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		gyrosens = pSensor;
		break;
	}
	if (pSensorCollection)
		pSensorCollection->Release();
	if (!gyrosens) {
                get_sensors().log(Sensors::loglevel_notice, "winattitude: no suitable gyro sensor found");
		return true;
	}
	pSensorCollection = 0;
	hr = m_sensmgr->GetSensorsByType(SENSOR_TYPE_COMPASS_3D, &pSensorCollection);
	if (FAILED(hr))
		return true;
	senscount = 0;
	hr = pSensorCollection->GetCount(&senscount);
	if (FAILED(hr)) {
		if (pSensorCollection)
			pSensorCollection->Release();
		return true;
	}
	ISensor *magsens(0);
	for (ULONG i = 0; i < senscount; ++i) {
		ISensor *pSensor(0);
		hr = pSensorCollection->GetAt(i, &pSensor);
		if (FAILED(hr))
			continue;
		VARIANT_BOOL bSupported(VARIANT_FALSE);
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_X_MILLIGAUSS, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		bSupported = VARIANT_FALSE;
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_Y_MILLIGAUSS, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		bSupported = VARIANT_FALSE;
		hr = pSensor->SupportsDataField(SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_Z_MILLIGAUSS, &bSupported);
		if (FAILED(hr) || bSupported != VARIANT_TRUE)
			continue;
		magsens = pSensor;
		break;
	}
	if (pSensorCollection)
		pSensorCollection->Release();
	if (!magsens) {
                get_sensors().log(Sensors::loglevel_notice, "winattitude: no suitable magnetic field sensor found");
		return true;
	}
	SensorState sensstateaccel(SENSOR_STATE_ERROR);
	hr = accelsens->GetState(&sensstateaccel);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get acceleration sensor state");
		return true;
	}
	SensorState sensstategyro(SENSOR_STATE_ERROR);
	hr = gyrosens->GetState(&sensstategyro);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get gyro sensor state");
		return true;
	}
	SensorState sensstatemag(SENSOR_STATE_ERROR);
	hr = magsens->GetState(&sensstatemag);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get magnetic field sensor state");
		return true;
	}
	if (sensstateaccel == SENSOR_STATE_ACCESS_DENIED ||
	    sensstategyro == SENSOR_STATE_ACCESS_DENIED ||
	    sensstatemag == SENSOR_STATE_ACCESS_DENIED) {
		// Make a SensorCollection with only the sensors we want to get permission to access.
		ISensorCollection *senscoll(0);
		hr = ::CoCreateInstance(IID_ISensorCollection, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&senscoll));
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot create a sensor collection");
			return true;
		}
		senscoll->Clear();
		if (sensstateaccel == SENSOR_STATE_ACCESS_DENIED)
			senscoll->Add(accelsens);
		if (sensstategyro == SENSOR_STATE_ACCESS_DENIED)
			senscoll->Add(gyrosens);
		if (sensstatemag == SENSOR_STATE_ACCESS_DENIED)
			senscoll->Add(magsens);
		hr = m_sensmgr->RequestPermissions(NULL, senscoll, TRUE);
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "winattitude: no permission to access the sensors");
			return true;
		}
	}
	// get sampling rate
	{
		IPortableDeviceValues *propsset(0);
		hr = CoCreateInstance(__uuidof(PortableDeviceValues), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&propsset));
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot create device values");
			return true;
		}
		hr = propsset->SetUnsignedIntegerValue(SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, 0);
		if (FAILED(hr)) {
			propsset->Release();
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set reporting interval");
			return true;
		}
		IPortableDeviceValues *propsrecv(0);
		hr = gyrosens->SetProperties(propsset, &propsrecv);
		if (FAILED(hr)) {
			propsset->Release();
			if (propsrecv)
				propsrecv->Release();
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set default reporting interval of gyro sensor");
			return true;
		}
		propsrecv->Release();
		propsrecv = 0;
		PROPVARIANT pv = {};
		hr = gyrosens->GetProperty(SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, &pv);
		if (FAILED(hr) || pv.vt != VT_UI4) {
			propsset->Release();
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get gyro sensor reporting interval");
			return true;
		}
		ULONG interval(pv.ulVal);
		PropVariantClear(&pv);
		propsset->SetUnsignedIntegerValue(SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, interval);
		hr = accelsens->SetProperties(propsset, &propsrecv);
		if (FAILED(hr)) {
			propsset->Release();
			if (propsrecv)
				propsrecv->Release();
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set reporting interval of acceleration sensor");
			return true;
		}
		propsrecv->Release();
		propsrecv = 0;
		hr = magsens->SetProperties(propsset, &propsrecv);
		if (FAILED(hr)) {
			propsset->Release();
			if (propsrecv)
				propsrecv->Release();
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set reporting interval of magnetic field sensor");
			return true;
		}
		propsrecv->Release();
		propsrecv = 0;
		propsset->Release();
		// get name of gyro sensor
		hr = gyrosens->GetProperty(SENSOR_PROPERTY_FRIENDLY_NAME, &pv);
		if (FAILED(hr)) {
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get the name of the gyro sensor");
			return true;
		}
		if (pv.vt != VT_LPWSTR) {
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get the name of the gyro sensor");
			PropVariantClear(&pv);
			return true;
		}
		gchar *p(g_utf16_to_utf8((gunichar2 *)pv.pwszVal, -1, NULL, NULL, NULL));
		PropVariantClear(&pv);
		if (!p) {
			get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get the name of the gyro sensor");
			return true;
		}
		m_ahrs.set_id(p);
		g_free(p);
		m_ahrs.set_sampletime(interval * 0.001);
		if (!m_ahrs.init(AHRS::init_standard))
			m_ahrs.load(get_sensors().get_configfile(), get_configgroup());
		std::ostringstream oss;
		oss << "winattitude: reporting interval " << interval << "ms";
		get_sensors().log(Sensors::loglevel_notice, oss.str());
	}
	SensorAccelEvents *pSensorAccelEventClass(new SensorAccelEvents(this));
	ISensorEvents *pSensorAccelEvents(0);
	hr = pSensorAccelEventClass->QueryInterface(IID_PPV_ARGS(&pSensorAccelEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get acceleration sensor events interface");
		delete pSensorAccelEventClass;
		return true;
	}
	hr = accelsens->SetEventSink(pSensorAccelEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set acceleration sensor event sink");
		pSensorAccelEvents->Release();
		return true;
	}
	m_accelsens = accelsens;
	SensorGyroEvents *pSensorGyroEventClass(new SensorGyroEvents(this));
	ISensorEvents *pSensorGyroEvents(0);
	hr = pSensorGyroEventClass->QueryInterface(IID_PPV_ARGS(&pSensorGyroEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get gyro sensor events interface");
		delete pSensorGyroEventClass;
		close_internal();
		return true;
	}
	hr = gyrosens->SetEventSink(pSensorGyroEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set gyro sensor event sink");
		pSensorGyroEvents->Release();
		close_internal();
		return true;
	}
	m_gyrosens = gyrosens;
	SensorMagEvents *pSensorMagEventClass(new SensorMagEvents(this));
	ISensorEvents *pSensorMagEvents(0);
	hr = pSensorMagEventClass->QueryInterface(IID_PPV_ARGS(&pSensorMagEvents));
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot get magnetic field sensor events interface");
		delete pSensorMagEventClass;
		close_internal();
		return true;
	}
	hr = magsens->SetEventSink(pSensorMagEvents);
	if (FAILED(hr)) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: cannot set magnetic field sensor event sink");
		pSensorMagEvents->Release();
		close_internal();
		return true;
	}
	m_magsens = magsens;
	return false;
}

// Parameter Documentation: http://msdn.microsoft.com/en-us/library/windows/desktop/dd318985%28v=vs.85%29.aspx

// Our convention is NED (North, East, Down)
// Microsoft uses ENU (East, North, Up)

void Sensors::SensorWinAttitude::accel_newdata(ISensor *pSensor, ISensorDataReport *pNewData)
{
	m_rawaccel[0] = m_rawaccel[1] = m_rawaccel[2] = std::numeric_limits<double>::quiet_NaN();
	unsigned int flg(0);
	PROPVARIANT pv = {};
        HRESULT hr(pNewData->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_X_G, &pv));
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawaccel[1] = pv.dblVal;
		flg |= 1;
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Y_G, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawaccel[0] = pv.dblVal;
		flg |= 2;
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Z_G, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawaccel[2] = -pv.dblVal;
		flg |= 4;
	}
        PropVariantClear(&pv);
	if (flg == 7 && !std::isnan(m_rawaccel[0]) && !std::isnan(m_rawaccel[1]) && !std::isnan(m_rawaccel[2]))
		m_ahrs.new_accel_samples(m_rawaccel);
}

void Sensors::SensorWinAttitude::accel_statechange(ISensor* pSensor, SensorState state)
{
	if (state == SENSOR_STATE_READY) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: acceleration sensor ready");
		return;
	}
	if (state == SENSOR_STATE_ACCESS_DENIED) {
		get_sensors().log(Sensors::loglevel_warning, "winattitude: acceleration sensor permission denied");
		close();
		return;
	}
}

void Sensors::SensorWinAttitude::gyro_newdata(ISensor *pSensor, ISensorDataReport *pNewData)
{
	m_rawgyro[0] = m_rawgyro[1] = m_rawgyro[2] = std::numeric_limits<double>::quiet_NaN();
	unsigned int flg(0);
	PROPVARIANT pv = {};
        HRESULT hr(pNewData->GetSensorValue(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_X_DEGREES_PER_SECOND, &pv));
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawgyro[1] = pv.dblVal;
		flg |= 1;
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Y_DEGREES_PER_SECOND, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawgyro[0] = pv.dblVal;
		flg |= 2;
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Z_DEGREES_PER_SECOND, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawgyro[2] = -pv.dblVal;
 		flg |= 4;
	}
       PropVariantClear(&pv);
       if (flg == 7 && !std::isnan(m_rawgyro[0]) && !std::isnan(m_rawgyro[1]) && !std::isnan(m_rawgyro[2])) {
	       m_ahrs.new_gyro_samples(1, m_rawgyro);
	       ++m_samplecount;
	       if (false) {
		       std::cout << "Accel " << std::setw(6) << m_rawaccel[0]
				 << ' ' << std::setw(6) << m_rawaccel[1]
				 << ' ' << std::setw(6) << m_rawaccel[2]
				 << " Gyro " << std::setw(6) << m_rawgyro[0]
				 << ' ' << std::setw(6) << m_rawgyro[1]
				 << ' ' << std::setw(6) << m_rawgyro[2]
				 << " Mag " << std::setw(6) << m_rawmag[0]
				 << ' ' << std::setw(6) << m_rawmag[1]
				 << ' ' << std::setw(6) << m_rawmag[2]
				 << " scnt " << m_samplecount << std::endl;
	       }
	       m_attitudetime.assign_current_time();
	       m_dirty = true;
	       {
		       Glib::TimeVal tv(m_attitudetime);
		       tv.subtract(m_attitudereporttime);
		       tv.subtract_milliseconds(50);
		       if (!m_attitudereporttime.valid() || !tv.negative()) {
			       m_attitudereporttime = m_attitudetime;
			       ParamChanged pc;
			       pc.set_changed(parnrattituderoll, parnrattitudeheading);
			       pc.set_changed(parnrrawgyrox, parnrattitudequatw);
			       pc.set_changed(parnrcalgyrobiasx, parnrcalgyrobiasz);
			       pc.set_changed(parnrcalaccelscale2, parnrgyro);
			       pc.set_changed(parnrrawsamplemagx, parnrrawsamplemagz);
			       update(pc);
		       }
	       }
       }
}

void Sensors::SensorWinAttitude::gyro_statechange(ISensor* pSensor, SensorState state)
{
	if (state == SENSOR_STATE_READY) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: gyro sensor ready");
		return;
	}
	if (state == SENSOR_STATE_ACCESS_DENIED) {
		get_sensors().log(Sensors::loglevel_warning, "winattitude: gyro sensor permission denied");
		close();
		return;
	}
}

void Sensors::SensorWinAttitude::mag_newdata(ISensor *pSensor, ISensorDataReport *pNewData)
{
	m_rawmag[0] = m_rawmag[1] = m_rawmag[2] = std::numeric_limits<double>::quiet_NaN();
	unsigned int flg(0);
	PROPVARIANT pv = {};
        HRESULT hr(pNewData->GetSensorValue(SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_X_MILLIGAUSS, &pv));
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawmag[1] = pv.dblVal;
		flg |= 1;
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_Y_MILLIGAUSS, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawmag[0] = pv.dblVal;
		flg |= 2;
	}
        PropVariantClear(&pv);
	hr = pNewData->GetSensorValue(SENSOR_DATA_TYPE_MAGNETIC_FIELD_STRENGTH_Z_MILLIGAUSS, &pv);
	if (SUCCEEDED(hr) && pv.vt == VT_R8) {
		m_rawmag[2] = -pv.dblVal;
		flg |= 4;
	}
        PropVariantClear(&pv);
	if (flg == 7 && !std::isnan(m_rawmag[0]) && !std::isnan(m_rawmag[1]) && !std::isnan(m_rawmag[2]))
		m_ahrs.new_mag_samples(m_rawmag);
}

void Sensors::SensorWinAttitude::mag_statechange(ISensor* pSensor, SensorState state)
{
	if (state == SENSOR_STATE_READY) {
                get_sensors().log(Sensors::loglevel_warning, "winattitude: magnetic field sensor ready");
		return;
	}
	if (state == SENSOR_STATE_ACCESS_DENIED) {
		get_sensors().log(Sensors::loglevel_warning, "winattitude: magnetic field sensor permission denied");
		close();
		return;
	}
}

void Sensors::SensorWinAttitude::update_values(void) const
{
	if (!m_dirty)
		return;
	m_dirty = false;
	m_attitudereporttime = m_attitudetime;
	Eigen::Vector3d bodye1(m_ahrs.e1_to_body());
	Eigen::Vector3d bodye3(m_ahrs.e3_to_body());
	if (false)
		std::cerr << "bodye1 " << bodye1.transpose() << " bodye3 " << bodye3.transpose() << std::endl;
	m_pitch = (180.0 / M_PI) * atan2(bodye3[0], bodye3[2]);
	m_bank = (-180.0 / M_PI) * atan2(bodye3[1], bodye3[2]);
	m_hdg = (-180.0 / M_PI) * atan2(bodye1[1], bodye1[0]) + m_ahrs.get_magdeviation();
	Eigen::Vector3d filtaccel(m_ahrs.get_filtered_accelerometer());
	Eigen::Vector3d filtgyro(m_ahrs.get_filtered_gyroscope());
	m_rate = filtgyro.z() * (180.0 / M_PI / 3.0);
	m_slip = (180.0 / M_PI) * atan2(filtaccel.z(), filtaccel.y()) - 90.0;
}
	
void Sensors::SensorWinAttitude::get_heading(double& hdg) const
{
	update_values();
	hdg = m_hdg;
}

unsigned int Sensors::SensorWinAttitude::get_heading_priority(void) const
{
	return m_headingprio;
}

Glib::TimeVal Sensors::SensorWinAttitude::get_heading_time(void) const
{
	return m_attitudetime;
}

bool Sensors::SensorWinAttitude::is_heading_true(void) const
{
	return false;
}

bool Sensors::SensorWinAttitude::is_heading_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_heading_ok(tv);
}

void Sensors::SensorWinAttitude::get_attitude(double& pitch, double& bank, double& slip, double& rate) const
{
	update_values();
	pitch = m_pitch;
	bank = m_bank;
	slip = m_slip;
	rate = m_rate;
}

unsigned int Sensors::SensorWinAttitude::get_attitude_priority(void) const
{
	return m_attitudeprio;
}

Glib::TimeVal Sensors::SensorWinAttitude::get_attitude_time(void) const
{
	return m_attitudetime;
}

bool Sensors::SensorWinAttitude::is_attitude_ok(const Glib::TimeVal& tv) const
{
	return is_open() && SensorAttitude::is_attitude_ok(tv);
}

const AHRS::mode_t  Sensors::SensorWinAttitude::ahrsmodemap[4] = {
	AHRS::mode_slow, AHRS::mode_normal, AHRS::mode_fast, AHRS::mode_gyroonly
};

void Sensors::SensorWinAttitude::get_param_desc(unsigned int pagenr, paramdesc_t& pd)
{
	SensorAttitude::get_param_desc(pagenr, pd);

	switch (pagenr) {
	default:
		{
			paramdesc_t::iterator pdi(pd.begin()), pde(pd.end());
			if (pdi != pde)
				++pdi;
			for (; pdi != pde; ++pdi)
				if (pdi->get_type() == ParamDesc::type_section)
					break;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevname, "Accel Device Name", "Acceleration Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevmfg, "Accel Manufacturer", "Acceleration Sensor Device Manufacturer", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevdesc, "Accel Description", "Acceleration Sensor Device Description", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnracceldevserial, "Accel Serial", "Acceleration Sensor Device Serial Number", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevname, "Gyro Device Name", "Gyro Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevmfg, "Gyro Manufacturer", "Gyro Sensor Device Manufacturer", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevdesc, "Gyro Description", "Gyro Sensor Device Description", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrgyrodevserial, "Gyro Serial", "Gyro Sensor Device Serial Number", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevname, "Mag Device Name", "Magnetic Field Sensor Device Name", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevmfg, "Mag Manufacturer", "Magnetic Field Sensor Device Manufacturer", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevdesc, "Mag Description", "Magnetic Field Sensor Device Description", ""));
			++pdi;
			pdi = pd.insert(pdi, ParamDesc(ParamDesc::type_string, ParamDesc::editable_readonly, parnrmagdevserial, "Mag Serial", "Magnetic Field Sensor Device Serial Number", ""));
		}
		pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrattitudeprio, "Attitude Priority", "Attitude Priority; higher values mean this sensor is preferred to other sensors delivering attitude information", "", 0, 0, 9999, 1, 10));
		pd.push_back(ParamDesc(ParamDesc::type_integer, ParamDesc::editable_admin, parnrheadingprio, "Heading Priority", "Heading Priority; higher values mean this sensor is preferred to other sensors delivering heading information", "", 0, 0, 9999, 1, 10));
		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Parameters", "AHRS Parameters", ""));
		{
			ParamDesc::choices_t ch;
			for (int i = 0; i < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0])); ++i)
				ch.push_back(AHRS::to_str(ahrsmodemap[i]));
			pd.push_back(ParamDesc(ParamDesc::type_choice, ParamDesc::editable_user, parnrmode, "Mode", "Controls the amount of coupling of the gyro to the accelerometer and magnetometer", "", ch.begin(), ch.end()));
		}
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffla, "la", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoefflc, "lc", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffld, "ld", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffsigma, "sigma", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffn, "n", "", "", 3, 0.0, 99.999, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcoeffo, "o", "", "", 3, 0.0, 99.999, 0.01, 0.1));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Attitude", "Aircraft Attitude", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattituderoll, "Roll", "Roll Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudepitch, "Pitch", "Pitch Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudeheading, "Heading", "Heading Angle", "°", 1, 0.0, 360.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Orientation", "Sensor Orientation", ""));
		{
			ParamDesc::choices_t ch;
			ch.push_back("$gtk-go-back");
			ch.push_back("$gtk-go-forward");
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrorientationroll, "Roll", "Roll Orientation Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientcoarseadjustroll, "Coarse Adj.", "Coarse Adjust Roll Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientfineadjustroll, "Fine Adj.", "Fine Adjust Roll Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrorientationpitch, "Pitch", "Pitch Orientation Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientcoarseadjustpitch, "Coarse Adj.", "Coarse Adjust Pitch Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientfineadjustpitch, "Fine Adj.", "Fine Adjust Pitch Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrorientationheading, "Heading", "Heading Orientation Angle", "°", 1, -180.0, 360.0, 0.1, 1.0));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientcoarseadjustheading, "Coarse Adj.", "Coarse Adjust Heading Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorientfineadjustheading, "Fine Adj.", "Fine Adjust Heading Angle", "", ch.begin(), ch.end()));
			pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationdeviation, "Deviation", "Deviation Angle", "°", 1, -180.0, 540.0, 0.1, 1.0));
		}
		{
			ParamDesc::choices_t ch;
			ch.push_back("Erect");
			ch.push_back("Default");
			pd.push_back(ParamDesc(ParamDesc::type_button, ParamDesc::editable_admin, parnrorienterectdefault, "Quick Adjust", "Quick Adjust of Sensor Orientation", "", ch.begin(), ch.end()));
		}

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Attitudes", "Attitude Quaternions", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequatx, "Attitude X", "Attitude Quaternion X Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequaty, "Attitude Y", "Attitude Quaternion Y Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequatz, "Attitude Z", "Attitude Quaternion Z Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrattitudequatw, "Attitude W", "Attitude Quaternion W Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquatx, "Orientation X", "Sensor Orientation Quaternion X Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquaty, "Orientation Y", "Sensor Orientation Quaternion Y Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquatz, "Orientation Z", "Sensor Orientation Quaternion Z Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrorientationquatw, "Orientation W", "Sensor Orientation Quaternion W Value", "", 4, -1.0, 1.0, 0.01, 0.1));
		break;

	case 1:
		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Raw Values", "Raw Sensor Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawgyrox, "Gyro X", "Gyro Sensor X Value", "rad/s", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawgyroy, "Gyro Y", "Gyro Sensor Y Value", "rad/s", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawgyroz, "Gyro Z", "Gyro Sensor Z Value", "rad/s", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawaccelx, "Accel X", "Accelerometer Sensor X Value", "g", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawaccely, "Accel Y", "Accelerometer Sensor Y Value", "g", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawaccelz, "Accel Z", "Accelerometer Sensor Z Value", "g", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawmagx, "Mag X", "Magnetometer Sensor X Value", "B", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawmagy, "Mag Y", "Magnetometer Sensor Y Value", "B", 3, -100.0, 100.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrrawmagz, "Mag Z", "Magnetometer Sensor Z Value", "B", 3, -100.0, 100.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Gyro Calibration", "Gyro Calibration Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalgyrobiasx, "Gyro Bias X", "Gyro Sensor Bias (Offset) X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalgyrobiasy, "Gyro Bias Y", "Gyro Sensor Bias (Offset) Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalgyrobiasz, "Gyro Bias Z", "Gyro Sensor Bias (Offset) Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalgyroscalex, "Gyro Scale X", "Gyro Sensor Scaling X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalgyroscaley, "Gyro Scale Y", "Gyro Sensor Scaling Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalgyroscalez, "Gyro Scale Z", "Gyro Sensor Scaling Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Accelerometer Calibration", "Accelerometer Calibration Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelbiasx, "Accel Bias X", "Accelerometer Sensor Bias (Offset) X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelbiasy, "Accel Bias Y", "Accelerometer Sensor Bias (Offset) Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelbiasz, "Accel Bias Z", "Accelerometer Sensor Bias (Offset) Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelscalex, "Accel Scale X", "Accelerometer Sensor Scaling X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelscaley, "Accel Scale Y", "Accelerometer Sensor Scaling Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalaccelscalez, "Accel Scale Z", "Accelerometer Sensor Scaling Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalaccelscale2, "Accel Scale 2", "Accelerometer Sensor Scalar Scaling", "", 6, -10000.0, 10000.0, 0.1, 1.0));

		pd.push_back(ParamDesc(ParamDesc::type_section, ParamDesc::editable_readonly, ~0U, "Magnetometer Calibration", "Magnetometer Calibration Values", ""));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagbiasx, "Mag Bias X", "Magnetometer Sensor Bias (Offset) X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagbiasy, "Mag Bias Y", "Magnetometer Sensor Bias (Offset) Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagbiasz, "Mag Bias Z", "Magnetometer Sensor Bias (Offset) Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagscalex, "Mag Scale X", "Magnetometer Sensor Scaling X Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagscaley, "Mag Scale Y", "Magnetometer Sensor Scaling Y Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_admin, parnrcalmagscalez, "Mag Scale Z", "Magnetometer Sensor Scaling Z Value", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		pd.push_back(ParamDesc(ParamDesc::type_double, ParamDesc::editable_readonly, parnrcalmagscale2, "Mag Scale 2", "Magnetometer Sensor Scalar Scaling", "", 6, -10000.0, 10000.0, 0.1, 1.0));
		{
			static const Glib::ustring bname("$gtk-preferences");
			pd.push_back(ParamDesc(ParamDesc::type_magcalib, ParamDesc::editable_admin, parnrrawsamplemag, "Magnetometer Calib", "Opens a Dialog to Calibrate the Magnetometer; requires rotating the Magnetometer around all axes", "", &bname, &bname + 1));
		}
		break;
	}
	pd.push_back(ParamDesc(ParamDesc::type_gyro, ParamDesc::editable_readonly, parnrgyro, ""));
}

Sensors::SensorWinAttitude::paramfail_t Sensors::SensorWinAttitude::get_param(unsigned int nr, Glib::ustring& v) const
{
	switch (nr) {
	default:
		break;

	case parnracceldevname:
	{
		v.clear();
		if (!m_accelsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_accelsens->GetProperty(SENSOR_PROPERTY_FRIENDLY_NAME, &pv));
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

	case parnracceldevmfg:
	{
		v.clear();
		if (!m_accelsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_accelsens->GetProperty(SENSOR_PROPERTY_MANUFACTURER, &pv));
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

	case parnracceldevdesc:
	{
		v.clear();
		if (!m_accelsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_accelsens->GetProperty(SENSOR_PROPERTY_DESCRIPTION, &pv));
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

	case parnracceldevserial:
	{
		v.clear();
		if (!m_accelsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_accelsens->GetProperty(SENSOR_PROPERTY_SERIAL_NUMBER, &pv));
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

	case parnrgyrodevname:
	{
		v.clear();
		if (!m_gyrosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gyrosens->GetProperty(SENSOR_PROPERTY_FRIENDLY_NAME, &pv));
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

	case parnrgyrodevmfg:
	{
		v.clear();
		if (!m_gyrosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gyrosens->GetProperty(SENSOR_PROPERTY_MANUFACTURER, &pv));
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

	case parnrgyrodevdesc:
	{
		v.clear();
		if (!m_gyrosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gyrosens->GetProperty(SENSOR_PROPERTY_DESCRIPTION, &pv));
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

	case parnrgyrodevserial:
	{
		v.clear();
		if (!m_gyrosens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_gyrosens->GetProperty(SENSOR_PROPERTY_SERIAL_NUMBER, &pv));
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

	case parnrmagdevname:
	{
		v.clear();
		if (!m_magsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_magsens->GetProperty(SENSOR_PROPERTY_FRIENDLY_NAME, &pv));
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

	case parnrmagdevmfg:
	{
		v.clear();
		if (!m_magsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_magsens->GetProperty(SENSOR_PROPERTY_MANUFACTURER, &pv));
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

	case parnrmagdevdesc:
	{
		v.clear();
		if (!m_magsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_magsens->GetProperty(SENSOR_PROPERTY_DESCRIPTION, &pv));
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

	case parnrmagdevserial:
	{
		v.clear();
		if (!m_magsens)
			return paramfail_fail;
		PROPVARIANT pv = {};
		HRESULT hr(m_magsens->GetProperty(SENSOR_PROPERTY_SERIAL_NUMBER, &pv));
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
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorWinAttitude::paramfail_t Sensors::SensorWinAttitude::get_param(unsigned int nr, int& v) const
{
	switch (nr) {
	default:
		break;

	case parnrmode:
	{
		v = 1;
		AHRS::mode_t m(m_ahrs.get_mode());
		for (int i = 0; i < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0])); ++i) {
			if (ahrsmodemap[i] != m)
				continue;
			v = i;
			break;
		}
		return get_paramfail_open();
	}

	case parnrattitudeprio:
		v = m_attitudeprio;
		return paramfail_ok;

	case parnrheadingprio:
		v = m_headingprio;
		return paramfail_ok;
		
	case parnrrawsamplemagx:
	case parnrrawsamplemagy:
	case parnrrawsamplemagz:
		v = m_rawmag[nr - parnrrawsamplemag];
		return get_paramfail_open();

	case parnrorientcoarseadjustroll:
	case parnrorientfineadjustroll:
	case parnrorientcoarseadjustpitch:
	case parnrorientfineadjustpitch:
	case parnrorientcoarseadjustheading:
	case parnrorientfineadjustheading:
	case parnrorienterectdefault:
		v = 0;
		return get_paramfail_open();
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorWinAttitude::paramfail_t Sensors::SensorWinAttitude::get_param(unsigned int nr, double& v) const
{
	switch (nr) {
	default:
		break;

	case parnrcoeffla:
		v = m_ahrs.get_coeff_la();
		return get_paramfail_open();

	case parnrcoefflc:
		v = m_ahrs.get_coeff_lc();
		return get_paramfail_open();

	case parnrcoeffld:
		v = m_ahrs.get_coeff_ld();
		return get_paramfail_open();

	case parnrcoeffsigma:
		v = m_ahrs.get_coeff_sigma();
		return get_paramfail_open();

	case parnrcoeffn:
		v = m_ahrs.get_coeff_n();
		return get_paramfail_open();

	case parnrcoeffo:
		v = m_ahrs.get_coeff_o();
		return get_paramfail_open();

	case parnrattituderoll:
	case parnrattitudepitch:
	case parnrattitudeheading:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation() * m_ahrs.get_attitude().conjugate());
		AHRS::EulerAnglesd ea(AHRS::to_euler(q));
		v = ea[nr - parnrattitude] * (180.0 / M_PI);
		return get_paramfail();
	}

	case parnrorientationroll:
	case parnrorientationpitch:
	case parnrorientationheading:
	{
		AHRS::EulerAnglesd ea(AHRS::to_euler(m_ahrs.get_sensororientation()));
		v = ea[nr - parnrorientation] * (180.0 / M_PI);
		return get_paramfail_open();
	}

	case parnrorientationdeviation:
		v = m_ahrs.get_magdeviation();
		return get_paramfail_open();

	case parnrrawgyrox:
	case parnrrawgyroy:
	case parnrrawgyroz:
		v = m_ahrs.get_gyroscope()(nr - parnrrawgyro);
		return get_paramfail();

	case parnrrawaccelx:
	case parnrrawaccely:
	case parnrrawaccelz:
	     	v = m_ahrs.get_accelerometer()(nr - parnrrawaccel);
	     	return get_paramfail();

	case parnrrawmagx:
	case parnrrawmagy:
	case parnrrawmagz:
		v = m_ahrs.get_magnetometer()(nr - parnrrawmag);
		return get_paramfail();

	case parnrattitudequatx:
		v = m_ahrs.get_attitude().x();
		return get_paramfail();

	case parnrattitudequaty:
		v = m_ahrs.get_attitude().y();
		return get_paramfail();

	case parnrattitudequatz:
		v = m_ahrs.get_attitude().z();
		return get_paramfail();

	case parnrattitudequatw:
		v = m_ahrs.get_attitude().w();
		return get_paramfail();

	case parnrorientationquatx:
		v = m_ahrs.get_sensororientation().x();
		return get_paramfail_open();

	case parnrorientationquaty:
		v = m_ahrs.get_sensororientation().y();
		return get_paramfail_open();

	case parnrorientationquatz:
		v = m_ahrs.get_sensororientation().z();
		return get_paramfail_open();

	case parnrorientationquatw:
		v = m_ahrs.get_sensororientation().w();
		return get_paramfail_open();

	case parnrcalgyrobiasx:
	case parnrcalgyrobiasy:
	case parnrcalgyrobiasz:
	     	v = m_ahrs.get_gyrobias()(nr - parnrcalgyrobias);
	     	return get_paramfail();

	case parnrcalgyroscalex:
	case parnrcalgyroscaley:
	case parnrcalgyroscalez:
	     	v = m_ahrs.get_gyroscale()(nr - parnrcalgyroscale);
	     	return get_paramfail_open();

	case parnrcalaccelbiasx:
	case parnrcalaccelbiasy:
	case parnrcalaccelbiasz:
	     	v = m_ahrs.get_accelbias()(nr - parnrcalaccelbias);
	     	return get_paramfail_open();

	case parnrcalaccelscalex:
	case parnrcalaccelscaley:
	case parnrcalaccelscalez:
	     	v = m_ahrs.get_accelscale()(nr - parnrcalaccelscale);
	     	return get_paramfail_open();

	case parnrcalmagbiasx:
	case parnrcalmagbiasy:
	case parnrcalmagbiasz:
	     	v = m_ahrs.get_magbias()(nr - parnrcalmagbias);
	     	return get_paramfail_open();

	case parnrcalmagscalex:
	case parnrcalmagscaley:
	case parnrcalmagscalez:
	     	v = m_ahrs.get_magscale()(nr - parnrcalmagscale);
	     	return get_paramfail_open();

	case parnrcalaccelscale2:
	     	v = m_ahrs.get_accelscale2();
	     	return get_paramfail();

	case parnrcalmagscale2:
		v = m_ahrs.get_magscale2();
		return get_paramfail();
		
	case parnrrawsamplemagx:
	case parnrrawsamplemagy:
	case parnrrawsamplemagz:
		v = m_rawmag[nr - parnrrawsamplemag];
		return get_paramfail_open();
	}
	return SensorAttitude::get_param(nr, v);
}

Sensors::SensorWinAttitude::paramfail_t Sensors::SensorWinAttitude::get_paramfail_open(void) const
{
	return is_open() ? paramfail_ok : paramfail_fail;
}

Sensors::SensorWinAttitude::paramfail_t Sensors::SensorWinAttitude::get_paramfail(void) const
{
	if (!is_open())
		return paramfail_fail;
	Glib::TimeVal tv;
        tv.assign_current_time();
	tv.subtract(m_attitudetime);
	tv.add_seconds(-1);
	return tv.negative() ? paramfail_ok : paramfail_fail;
}

Sensors::SensorWinAttitude::paramfail_t Sensors::SensorWinAttitude::get_param(unsigned int nr, double& pitch, double& bank, double& slip, double& hdg, double& rate) const
{
	update_values();
	pitch = m_pitch;
	bank = m_bank;
	slip = m_slip;
	hdg = m_hdg;
	rate = m_rate;
	if (!is_open() || std::isnan(m_pitch) || std::isnan(m_bank) || std::isnan(m_slip) || std::isnan(m_hdg) || std::isnan(m_rate))
		return paramfail_fail;
	return get_paramfail();
}

void Sensors::SensorWinAttitude::set_param(unsigned int nr, const Glib::ustring& v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorWinAttitude::set_param(unsigned int nr, int v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	case parnrattitudeprio:
		m_attitudeprio = v;
		get_sensors().get_configfile().set_uint64(get_configgroup(), "attitudepriority", m_attitudeprio);
		return;

	case parnrheadingprio:
		m_headingprio = v;
		get_sensors().get_configfile().set_uint64(get_configgroup(), "headingpriority", m_headingprio);
		return;

	case parnrmode:
	{
		int v1(v);
		if (v1 >= 0 && v1 < (int)(sizeof(ahrsmodemap)/sizeof(ahrsmodemap[0])))
			m_ahrs.set_mode(ahrsmodemap[v1]);
		break;
	}

	case parnrorientcoarseadjustroll:
	case parnrorientfineadjustroll:
	case parnrorientcoarseadjustpitch:
	case parnrorientfineadjustpitch:
	case parnrorientcoarseadjustheading:
	case parnrorientfineadjustheading:
	{
		if (!(v & 0x0F))
			break;
		int i = nr - parnrorientadjust;
		double angle((i & 1) ? (2.0*M_PI/180.0*0.5) : (0.1*M_PI/180.0*0.5));
		i >>= 1;
		for (int j = 0; j < 2; ++j) {
			if (!(v & (1 << j)))
				continue;
			double a((2 * j - 1) * angle);
			Eigen::Quaterniond q(cos(a), 0, 0, 0);
			q.coeffs()[i] = sin(a);
			Eigen::Quaterniond q1(m_ahrs.get_sensororientation());
			q1 = q * q1;
			m_ahrs.set_sensororientation(q1);
		}
		ParamChanged pc;
		pc.set_changed(nr);
		pc.set_changed(parnrattituderoll, parnrattitudeheading);
		pc.set_changed(parnrorientationroll, parnrorientationheading);
		pc.set_changed(parnrorientationquatx, parnrorientationquatw);
		pc.set_changed(parnrgyro);
		update(pc);
		return;
	}

	case parnrorienterectdefault:
	{
		if (!(v & 0x03))
			break;
		if (v & 0x01) {
			// Erect
			Eigen::Quaterniond q(m_ahrs.get_attitude());
			q.normalize();
			m_ahrs.set_sensororientation(q);
		}
		if (v & 0x02) {
			// Default
			m_ahrs.set_sensororientation(Eigen::Quaterniond(1, 0, 0, 0));
		}
		ParamChanged pc;
		pc.set_changed(nr);
		pc.set_changed(parnrattituderoll, parnrattitudeheading);
		pc.set_changed(parnrorientationroll, parnrorientationheading);
		pc.set_changed(parnrorientationquatx, parnrorientationquatw);
		pc.set_changed(parnrgyro);
		update(pc);
		return;
	}

	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

void Sensors::SensorWinAttitude::set_param(unsigned int nr, double v)
{
	switch (nr) {
	default:
		SensorAttitude::set_param(nr, v);
		return;

	case parnrcoeffla:
		m_ahrs.set_coeff_la(v);
		break;

	case parnrcoefflc:
		m_ahrs.set_coeff_lc(v);
		break;

	case parnrcoeffld:
		m_ahrs.set_coeff_ld(v);
		break;

	case parnrcoeffsigma:
		m_ahrs.set_coeff_sigma(v);
		break;

	case parnrcoeffn:
		m_ahrs.set_coeff_n(v);
		break;

	case parnrcoeffo:
		m_ahrs.set_coeff_o(v);
		break;

	case parnrorientationdeviation:
		while (v < 0)
			v += 360;
		while (v >= 360)
			v -= 360;
		m_ahrs.set_magdeviation(v);
		break;

	case parnrorientationquatx:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.x() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquaty:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.y() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquatz:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.z() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrorientationquatw:
	{
		Eigen::Quaterniond q(m_ahrs.get_sensororientation());
		q.w() = v;
		q.normalize();
		m_ahrs.set_sensororientation(q);
		ParamChanged pc;
		pc.set_changed(parnrorientationquatx, parnrorientationquatz);
		update(pc);
		return;
	}

	case parnrcalgyroscalex:
	case parnrcalgyroscaley:
	case parnrcalgyroscalez:
	{
		Eigen::Vector3d x(m_ahrs.get_gyroscale());
	     	x(nr - parnrcalgyroscale) = v;
		m_ahrs.set_gyroscale(x);
		break;
	}

	case parnrcalaccelbiasx:
	case parnrcalaccelbiasy:
	case parnrcalaccelbiasz:
	{
		Eigen::Vector3d x(m_ahrs.get_accelbias());
	     	x(nr - parnrcalaccelbias) = v;
		m_ahrs.set_accelbias(x);
		break;
	}

	case parnrcalaccelscalex:
	case parnrcalaccelscaley:
	case parnrcalaccelscalez:
	{
		Eigen::Vector3d x(m_ahrs.get_accelscale());
	     	x(nr - parnrcalaccelscale) = v;
		m_ahrs.set_accelscale(x);
		break;
	}

	case parnrcalmagbiasx:
	case parnrcalmagbiasy:
	case parnrcalmagbiasz:
	{
		Eigen::Vector3d x(m_ahrs.get_magbias());
	     	x(nr - parnrcalmagbias) = v;
		m_ahrs.set_magbias(x);
		break;
	}

	case parnrcalmagscalex:
	case parnrcalmagscaley:
	case parnrcalmagscalez:
	{
		Eigen::Vector3d x(m_ahrs.get_magscale());
	     	x(nr - parnrcalmagscale) = v;
		m_ahrs.set_magscale(x);
		break;
	}
	}
	ParamChanged pc;
	pc.set_changed(nr);
	update(pc);
}

std::string Sensors::SensorWinAttitude::logfile_header(void) const
{
	return SensorAttitude::logfile_header() + ",GyroX,GyroY,GyroZ,AccelX,AccelY,AccelZ,MagX,MagY,MagZ,AttX,AttY,AttZ,AttW,GyroBX,GyroBY,GyroBZ,Accel2,Mag2,RawMagX,RawMagY,RawMagZ,Mode";
}

std::string Sensors::SensorWinAttitude::logfile_records(void) const
{
	std::ostringstream oss;
	oss << SensorAttitude::logfile_records() << std::fixed << std::setprecision(6) << ',';
	{
		Eigen::Vector3d v(m_ahrs.get_gyroscope());
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		Eigen::Vector3d v(m_ahrs.get_accelerometer());
		oss << ',';
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		Eigen::Vector3d v(m_ahrs.get_magnetometer());
		oss << ',';
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		Eigen::Quaterniond v(m_ahrs.get_attitude());
		oss << ',';
		if (!std::isnan(v.x()))
			oss << v.x();
		oss << ',';
		if (!std::isnan(v.y()))
			oss << v.y();
		oss << ',';
		if (!std::isnan(v.z()))
			oss << v.z();
		oss << ',';
		if (!std::isnan(v.w()))
			oss << v.w();
	}
	{
		Eigen::Vector3d v(m_ahrs.get_gyrobias());
		oss << ',';
		if (!std::isnan(v(0)))
			oss << v(0);
		oss << ',';
		if (!std::isnan(v(1)))
			oss << v(1);
		oss << ',';
		if (!std::isnan(v(2)))
			oss << v(2);
	}
	{
		double v(m_ahrs.get_accelscale2());
		oss << ',';
		if (!std::isnan(v))
			oss << v;
	}
	{
		double v(m_ahrs.get_magscale2());
		oss << ',';
		if (!std::isnan(v))
			oss << v;
	}
	oss << ',';
	if (!std::isnan(m_rawmag[0]))
		oss << m_rawmag[0];
	oss << ',';
	if (!std::isnan(m_rawmag[1]))
		oss << m_rawmag[1];
	oss << ',';
	if (!std::isnan(m_rawmag[2]))
		oss << m_rawmag[2];
	oss << ',' << AHRS::to_str(m_ahrs.get_mode());
	return oss.str();
}
