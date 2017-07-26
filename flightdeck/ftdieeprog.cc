#include "config.h"

#include "getopt.h"

#include <inttypes.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <glibmm.h>

#if defined(HAVE_LIBFTDI) || defined(HAVE_LIBFTDI1)
#include <ftdi.h>
#elif WIN32
#include <windows.h>
#include <ftdi/ftd2xx.h>

#ifndef EOF
#define EOF -1
#endif
#else
#error No FTDI Access Library
#endif

// use default FTDI VID/PID
static const uint16_t vendor_id = 0x0403;
static const uint16_t device_id = 0x6001;

static const char *dev_manufacturer = "FTDI";
static const char *dev_product = "FlightDeck";
static const char *dev_serial = "HBPBX";

static const char *dev_orig_manufacturer = "FTDI";
static const char *dev_orig_product = "UC232R";

/*

Original EEPROM Contents:
0x00,0x40,0x03,0x04,0x01,0x60,0x00,0x06,0xa0,0x32,0x08,0x00,0x00,0x02,0x98,0x0a,
0xa2,0x0e,0xb0,0x12,0x11,0x11,0x05,0x00,0x0a,0x03,0x46,0x00,0x54,0x00,0x44,0x00,
0x49,0x00,0x0e,0x03,0x55,0x00,0x43,0x00,0x32,0x00,0x33,0x00,0x32,0x00,0x52,0x00,
0x12,0x03,0x46,0x00,0x54,0x00,0x46,0x00,0x42,0x00,0x39,0x00,0x4d,0x00,0x54,0x00,
0x30,0x00,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x09

*/

#if defined(HAVE_LIBFTDI)

class FtdiProg {
public:
        FtdiProg(void);
        ~FtdiProg();
	void program_all(const char *manufacturer, const char *product, const char *serial, bool force);

protected:
        struct ftdi_context m_ctx;
        struct ftdi_device_list *m_devlist;
        bool m_init;
	struct ftdi_eeprom m_eeprom;

	void chk(int r, const std::string& n);
};

FtdiProg::FtdiProg(void)
        : m_devlist(0), m_init(false)
{
	ftdi_eeprom_initdefaults(&m_eeprom);
        if (ftdi_init(&m_ctx) < 0)
                throw std::runtime_error("ftdi_init failed");
        m_init = true;
        if (ftdi_usb_find_all(&m_ctx, &m_devlist, vendor_id, device_id) < 0)
                throw std::runtime_error("ftdi_usb_find_all failed");
}

FtdiProg::~FtdiProg()
{
        if (m_devlist)
                ftdi_list_free(&m_devlist);
        m_devlist = 0;
        if (m_init)
                ftdi_deinit(&m_ctx);
        m_init = false;
	ftdi_eeprom_free(&m_eeprom);
}

void FtdiProg::chk(int r, const std::string& n)
{
	if (r >= 0)
		return;
	std::ostringstream oss;
	oss << n << " failed (" << r << ", " << ftdi_get_error_string(&m_ctx) << ")";
	throw std::runtime_error(oss.str());
}

void FtdiProg::program_all(const char *manufacturer, const char *product, const char *serial, bool force)
{
	if (!serial)
		serial = dev_serial;
	ftdi_eeprom_initdefaults(&m_eeprom);
        for (struct ftdi_device_list *dev(m_devlist); dev; dev = dev->next) {
                struct usb_device *udev(dev->dev);
                if (!udev)
                        continue;
                char mfg[128], desc[128], ser[128];
		mfg[0] = desc[0] = ser[0] = 0;
                if (ftdi_usb_get_strings(&m_ctx, udev, mfg, sizeof mfg, desc, sizeof desc, ser, sizeof ser) && !force)
                        continue;
                mfg[sizeof(mfg)-1] = 0;
                desc[sizeof(desc)-1] = 0;
                ser[sizeof(ser)-1] = 0;
		if (!force && (std::string(mfg) != dev_orig_manufacturer || std::string(desc) != dev_orig_product)) {
			std::cout << "Skipping Adapter: MFG \"" << mfg << "\" Desc \""
				  << desc << "\" Serial \"" << ser << "\"" << std::endl;
                        continue;
		}
		std::cout << "Reprogramming Adapter: MFG \"" << mfg << "\" Desc \""
			  << desc << "\" Serial \"" << ser << "\"" << std::endl;
		ftdi_eeprom_initdefaults(&m_eeprom);
		m_eeprom.vendor_id = vendor_id;
		m_eeprom.product_id = device_id;
		m_eeprom.self_powered = 0;
		m_eeprom.remote_wakeup = 0;
		//m_eeprom.BM_type_chip = 1;
		m_eeprom.chip_type = TYPE_R;
		m_eeprom.in_is_isochronous = 0;
		m_eeprom.out_is_isochronous = 0;
		m_eeprom.suspend_pull_downs = 0;
		m_eeprom.use_serial = 1;
		m_eeprom.change_usb_version = 0;
		m_eeprom.usb_version = 0x0200;
		m_eeprom.max_power = 50;
		m_eeprom.manufacturer = strdup(manufacturer);
		m_eeprom.product = strdup(product);
		m_eeprom.serial = strdup(serial);
		m_eeprom.cbus_function[0] = CBUS_IOMODE;
		m_eeprom.cbus_function[1] = CBUS_IOMODE;
		m_eeprom.cbus_function[2] = CBUS_IOMODE;
		m_eeprom.cbus_function[3] = CBUS_IOMODE;
		m_eeprom.cbus_function[4] = CBUS_SLEEP;
		//m_eeprom.size = FTDI_DEFAULT_EEPROM_SIZE;
		uint8_t eebuf[128];
		chk(ftdi_eeprom_build(&m_eeprom, eebuf), "ftdi_eeprom_build");
		ftdi_eeprom_free(&m_eeprom);
		chk(ftdi_usb_open_dev(&m_ctx, udev), "ftdi_usb_open_dev");
		chk(ftdi_write_eeprom(&m_ctx, eebuf), "ftdi_write_eeprom");
		chk(ftdi_usb_close(&m_ctx), "ftdi_usb_close");
        }
}

class FtdiDump {
public:
	FtdiDump(void);
	~FtdiDump();
	std::vector<uint8_t> dump(void);

protected:
        struct ftdi_context m_ctx;
        bool m_init;

	void chk(int r, const std::string& n);
};

FtdiDump::FtdiDump(void)
        : m_init(false)
{
        if (ftdi_init(&m_ctx) < 0)
                throw std::runtime_error("ftdi_init failed");
        m_init = true;
}

FtdiDump::~FtdiDump()
{
	ftdi_usb_close(&m_ctx);
        if (m_init)
                ftdi_deinit(&m_ctx);
        m_init = false;
}

void FtdiDump::chk(int r, const std::string& n)
{
	if (r >= 0)
		return;
	std::ostringstream oss;
	oss << n << " failed (" << r << ", " << ftdi_get_error_string(&m_ctx) << ")";
	throw std::runtime_error(oss.str());
}

std::vector<uint8_t> FtdiDump::dump(void)
{
	ftdi_usb_close(&m_ctx);
	chk(ftdi_usb_open(&m_ctx, vendor_id, device_id), "ftdi_usb_open");
	std::vector<uint8_t> r(m_ctx.eeprom_size, 0);
	chk(ftdi_read_eeprom(&m_ctx, &r[0]),"ftdi_read_eeprom");
	chk(ftdi_usb_close(&m_ctx), "ftdi_usb_close");
	return r;
}

#elif defined(HAVE_LIBFTDI1)

class FtdiProg {
public:
        FtdiProg(void);
        ~FtdiProg();
	void program_all(const char *manufacturer, const char *product, const char *serial, bool force);

protected:
        struct ftdi_context m_ctx;
        struct ftdi_device_list *m_devlist;
        bool m_init;

	void chk(int r, const std::string& n);
};

FtdiProg::FtdiProg(void)
        : m_devlist(0), m_init(false)
{
	if (ftdi_init(&m_ctx) < 0)
                throw std::runtime_error("ftdi_init failed");
        m_init = true;
        if (ftdi_usb_find_all(&m_ctx, &m_devlist, vendor_id, device_id) < 0)
                throw std::runtime_error("ftdi_usb_find_all failed");
}

FtdiProg::~FtdiProg()
{
        if (m_devlist)
                ftdi_list_free(&m_devlist);
        m_devlist = 0;
        if (m_init)
                ftdi_deinit(&m_ctx);
        m_init = false;
}

void FtdiProg::chk(int r, const std::string& n)
{
	if (r >= 0)
		return;
	std::ostringstream oss;
	oss << n << " failed (" << r << ", " << ftdi_get_error_string(&m_ctx) << ")";
	throw std::runtime_error(oss.str());
}

void FtdiProg::program_all(const char *manufacturer, const char *product, const char *serial, bool force)
{
	if (!serial)
		serial = dev_serial;
	for (struct ftdi_device_list *dev(m_devlist); dev; dev = dev->next) {
                struct libusb_device *udev(dev->dev);
                if (!udev)
                        continue;
		char mfg[128], desc[128], ser[128];
		mfg[0] = desc[0] = ser[0] = 0;
                if (ftdi_usb_get_strings(&m_ctx, udev, mfg, sizeof mfg, desc, sizeof desc, ser, sizeof ser) && !force)
                        continue;
                mfg[sizeof(mfg)-1] = 0;
                desc[sizeof(desc)-1] = 0;
                ser[sizeof(ser)-1] = 0;
		if (!force && (std::string(mfg) != dev_orig_manufacturer || std::string(desc) != dev_orig_product)) {
			std::cout << "Skipping Adapter: MFG \"" << mfg << "\" Desc \""
				  << desc << "\" Serial \"" << ser << "\"" << std::endl;
                        continue;
		}
		if (ftdi_usb_open_dev(&m_ctx, udev) < 0)
			continue;
		std::cout << "Reprogramming Adapter: MFG \"" << mfg << "\" Desc \""
			  << desc << "\" Serial \"" << ser << "\"" << std::endl;
		ftdi_eeprom_initdefaults(&m_ctx, const_cast<char *>(manufacturer), const_cast<char *>(product), const_cast<char *>(serial));
		chk(ftdi_set_eeprom_value(&m_ctx, VENDOR_ID, vendor_id), "ftdi_set_eeprom_value: VENDOR_ID");
		chk(ftdi_set_eeprom_value(&m_ctx, PRODUCT_ID, device_id), "ftdi_set_eeprom_value: PRODUCT_ID");
		chk(ftdi_set_eeprom_value(&m_ctx, SELF_POWERED, 0), "ftdi_set_eeprom_value: SELF_POWERED");
		chk(ftdi_set_eeprom_value(&m_ctx, REMOTE_WAKEUP, 0), "ftdi_set_eeprom_value: REMOTE_WAKEUP");
		chk(ftdi_set_eeprom_value(&m_ctx, IN_IS_ISOCHRONOUS, 0), "ftdi_set_eeprom_value: IN_IS_ISOCHRONOUS");
		chk(ftdi_set_eeprom_value(&m_ctx, OUT_IS_ISOCHRONOUS, 0), "ftdi_set_eeprom_value: OUT_IS_ISOCHRONOUS");
		chk(ftdi_set_eeprom_value(&m_ctx, SUSPEND_PULL_DOWNS, 0), "ftdi_set_eeprom_value: SUSPEND_PULL_DOWNS");
		chk(ftdi_set_eeprom_value(&m_ctx, USE_SERIAL, 1), "ftdi_set_eeprom_value: USE_SERIAL");
		chk(ftdi_set_eeprom_value(&m_ctx, USE_USB_VERSION, 0), "ftdi_set_eeprom_value: USE_USB_VERSION");
		chk(ftdi_set_eeprom_value(&m_ctx, USB_VERSION, 0x0200), "ftdi_set_eeprom_value: USB_VERSION");
		chk(ftdi_set_eeprom_value(&m_ctx, MAX_POWER, 50), "ftdi_set_eeprom_value: MAX_POWER");
		chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_0, CBUS_IOMODE), "ftdi_set_eeprom_value: CBUS_FUNCTION_0");
		chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_1, CBUS_IOMODE), "ftdi_set_eeprom_value: CBUS_FUNCTION_1");
		chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_2, CBUS_IOMODE), "ftdi_set_eeprom_value: CBUS_FUNCTION_2");
		chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_3, CBUS_IOMODE), "ftdi_set_eeprom_value: CBUS_FUNCTION_3");
		chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_4, CBUS_SLEEP), "ftdi_set_eeprom_value: CBUS_FUNCTION_4");
		chk(ftdi_set_eeprom_value(&m_ctx, CHIP_TYPE, TYPE_R), "ftdi_set_eeprom_value: CHIP_TYPE");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHIP_SIZE, FTDI_DEFAULT_EEPROM_SIZE), "ftdi_set_eeprom_value: CHIP_SIZE");
		//chk(ftdi_set_eeprom_value(&m_ctx, IS_NOT_PNP, ), "ftdi_set_eeprom_value: IS_NOT_PNP");
		//chk(ftdi_set_eeprom_value(&m_ctx, SUSPEND_DBUS7, ), "ftdi_set_eeprom_value: SUSPEND_DBUS7");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_A_TYPE, ), "ftdi_set_eeprom_value: CHANNEL_A_TYPE");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_B_TYPE, ), "ftdi_set_eeprom_value: CHANNEL_B_TYPE");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_A_DRIVER, ), "ftdi_set_eeprom_value: CHANNEL_A_DRIVER");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_B_DRIVER, ), "ftdi_set_eeprom_value: CHANNEL_B_DRIVER");
		//chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_5, ), "ftdi_set_eeprom_value: CBUS_FUNCTION_5");
		//chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_6, ), "ftdi_set_eeprom_value: CBUS_FUNCTION_6");
		//chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_7, ), "ftdi_set_eeprom_value: CBUS_FUNCTION_7");
		//chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_8, ), "ftdi_set_eeprom_value: CBUS_FUNCTION_8");
		//chk(ftdi_set_eeprom_value(&m_ctx, CBUS_FUNCTION_9, ), "ftdi_set_eeprom_value: CBUS_FUNCTION_9");
		//chk(ftdi_set_eeprom_value(&m_ctx, HIGH_CURRENT, ), "ftdi_set_eeprom_value: HIGH_CURRENT");
		//chk(ftdi_set_eeprom_value(&m_ctx, HIGH_CURRENT_A, ), "ftdi_set_eeprom_value: HIGH_CURRENT_A");
		//chk(ftdi_set_eeprom_value(&m_ctx, HIGH_CURRENT_B, ), "ftdi_set_eeprom_value: HIGH_CURRENT_B");
		//chk(ftdi_set_eeprom_value(&m_ctx, INVERT, ), "ftdi_set_eeprom_value: INVERT");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP0_DRIVE, ), "ftdi_set_eeprom_value: GROUP0_DRIVE");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP0_SCHMITT, ), "ftdi_set_eeprom_value: GROUP0_SCHMITT");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP0_SLEW, ), "ftdi_set_eeprom_value: GROUP0_SLEW");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP1_DRIVE, ), "ftdi_set_eeprom_value: GROUP1_DRIVE");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP1_SCHMITT, ), "ftdi_set_eeprom_value: GROUP1_SCHMITT");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP1_SLEW, ), "ftdi_set_eeprom_value: GROUP1_SLEW");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP2_DRIVE, ), "ftdi_set_eeprom_value: GROUP2_DRIVE");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP2_SCHMITT, ), "ftdi_set_eeprom_value: GROUP2_SCHMITT");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP2_SLEW, ), "ftdi_set_eeprom_value: GROUP2_SLEW");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP3_DRIVE, ), "ftdi_set_eeprom_value: GROUP3_DRIVE");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP3_SCHMITT, ), "ftdi_set_eeprom_value: GROUP3_SCHMITT");
		//chk(ftdi_set_eeprom_value(&m_ctx, GROUP3_SLEW, ), "ftdi_set_eeprom_value: GROUP3_SLEW");
		//chk(ftdi_set_eeprom_value(&m_ctx, POWER_SAVE, ), "ftdi_set_eeprom_value: POWER_SAVE");
		//chk(ftdi_set_eeprom_value(&m_ctx, CLOCK_POLARITY, ), "ftdi_set_eeprom_value: CLOCK_POLARITY");
		//chk(ftdi_set_eeprom_value(&m_ctx, DATA_ORDER, ), "ftdi_set_eeprom_value: DATA_ORDER");
		//chk(ftdi_set_eeprom_value(&m_ctx, FLOW_CONTROL, ), "ftdi_set_eeprom_value: FLOW_CONTROL");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_C_DRIVER, ), "ftdi_set_eeprom_value: CHANNEL_C_DRIVER");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_D_DRIVER, ), "ftdi_set_eeprom_value: CHANNEL_D_DRIVER");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_A_RS485, ), "ftdi_set_eeprom_value: CHANNEL_A_RS485");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_B_RS485, ), "ftdi_set_eeprom_value: CHANNEL_B_RS485");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_C_RS485, ), "ftdi_set_eeprom_value: CHANNEL_C_RS485");
		//chk(ftdi_set_eeprom_value(&m_ctx, CHANNEL_D_RS485, ), "ftdi_set_eeprom_value: CHANNEL_D_RS485");
		//chk(ftdi_set_eeprom_value(&m_ctx, RELEASE_NUMBER, ), "ftdi_set_eeprom_value: RELEASE_NUMBER");
		//m_eeprom.BM_type_chip = 1;
		chk(ftdi_eeprom_build(&m_ctx), "ftdi_eeprom_build");
		chk(ftdi_write_eeprom(&m_ctx), "ftdi_write_eeprom");
		chk(ftdi_usb_close(&m_ctx), "ftdi_usb_close");
        }
}

class FtdiDump {
public:
	FtdiDump(void);
	~FtdiDump();
	std::vector<uint8_t> dump(void);

protected:
        struct ftdi_context m_ctx;
        bool m_init;

	void chk(int r, const std::string& n);
};

FtdiDump::FtdiDump(void)
        : m_init(false)
{
        if (ftdi_init(&m_ctx) < 0)
                throw std::runtime_error("ftdi_init failed");
        m_init = true;
}

FtdiDump::~FtdiDump()
{
	ftdi_usb_close(&m_ctx);
        if (m_init)
                ftdi_deinit(&m_ctx);
        m_init = false;
}

void FtdiDump::chk(int r, const std::string& n)
{
	if (r >= 0)
		return;
	std::ostringstream oss;
	oss << n << " failed (" << r << ", " << ftdi_get_error_string(&m_ctx) << ")";
	throw std::runtime_error(oss.str());
}

std::vector<uint8_t> FtdiDump::dump(void)
{
	ftdi_usb_close(&m_ctx);
	chk(ftdi_usb_open(&m_ctx, vendor_id, device_id), "ftdi_usb_open");
	chk(ftdi_read_eeprom(&m_ctx), "ftdi_read_eeprom");
	chk(ftdi_eeprom_decode(&m_ctx, 0), "ftdi_eeprom_decode");
	int rsize(0);
	chk(ftdi_get_eeprom_value(&m_ctx, CHIP_SIZE, &rsize), "ftdi_get_eeprom_value");
	std::vector<uint8_t> r;
	if (rsize > 0 && rsize <= 65536) {
		r.resize(rsize, 0);
		chk(ftdi_get_eeprom_buf(&m_ctx, &r[0], rsize), "ftdi_get_eeprom_buf");
	}
	chk(ftdi_usb_close(&m_ctx), "ftdi_usb_close");
	return r;
}

#else

class FtdiProg {
public:
        FtdiProg(void);
        ~FtdiProg();
	void program_all(const char *manufacturer, const char *product, const char *serial, bool force);

	static std::string to_str(FT_STATUS stat);
	static void err(FT_STATUS stat, const std::string& str);

protected:
	FT_HANDLE m_fth;
};

FtdiProg::FtdiProg(void)
	: m_fth(0)
{
}

FtdiProg::~FtdiProg()
{
	if (m_fth)
		FT_Close(m_fth);
	m_fth = 0;
}

std::string FtdiProg::to_str(FT_STATUS stat)
{
	switch (stat) {
        case FT_OK: return "OK";
        case FT_INVALID_HANDLE: return "INVALID_HANDLE";
        case FT_DEVICE_NOT_FOUND: return "DEVICE_NOT_FOUND";
        case FT_DEVICE_NOT_OPENED: return "DEVICE_NOT_OPENED";
        case FT_IO_ERROR: return "IO_ERROR";
        case FT_INSUFFICIENT_RESOURCES: return "INSUFFICIENT_RESOURCES";
        case FT_INVALID_PARAMETER: return "INVALID_PARAMETER";
        case FT_INVALID_BAUD_RATE: return "INVALID_BAUD_RATE";
        case FT_DEVICE_NOT_OPENED_FOR_ERASE: return "DEVICE_NOT_OPENED_FOR_ERASE";
        case FT_DEVICE_NOT_OPENED_FOR_WRITE: return "DEVICE_NOT_OPENED_FOR_WRITE";
        case FT_FAILED_TO_WRITE_DEVICE: return "FAILED_TO_WRITE_DEVICE";
        case FT_EEPROM_READ_FAILED: return "EEPROM_READ_FAILED";
        case FT_EEPROM_WRITE_FAILED: return "EEPROM_WRITE_FAILED";
        case FT_EEPROM_ERASE_FAILED: return "EEPROM_ERASE_FAILED";
        case FT_EEPROM_NOT_PRESENT: return "EEPROM_NOT_PRESENT";
        case FT_EEPROM_NOT_PROGRAMMED: return "EEPROM_NOT_PROGRAMMED";
        case FT_INVALID_ARGS: return "INVALID_ARGS";
        case FT_NOT_SUPPORTED: return "NOT_SUPPORTED";
        case FT_OTHER_ERROR: return "OTHER_ERROR";
        case FT_DEVICE_LIST_NOT_READY: return "DEVICE_LIST_NOT_READY";
	default:
		break;
	}
	{
		std::ostringstream oss;
		oss << "unknown FTDI error: " << stat;
		return oss.str();
	}
}

void FtdiProg::err(FT_STATUS stat, const std::string& str)
{
	if (stat == FT_OK)
		return;
	throw std::runtime_error(str + ": " + to_str(stat));
}

void FtdiProg::program_all(const char *manufacturer, const char *product, const char *serial, bool force)
{
	if (!serial)
		serial = dev_serial;
	static const char *dev_mfgid = "FT";
	if (m_fth)
		FT_Close(m_fth);
	m_fth = 0;
        FT_STATUS ftStatus;
        DWORD numDevs;
        ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
        if (ftStatus != FT_OK)
                return;
        for (DWORD devIndex = 0; devIndex < numDevs; ++devIndex) {
                char desc[64];
                ftStatus = FT_ListDevices((PVOID)devIndex, desc, FT_LIST_BY_INDEX|FT_OPEN_BY_DESCRIPTION);
                if (!FT_SUCCESS(ftStatus))
                        continue;
                desc[sizeof(desc)-1] = 0;
                char oldserial[64];
                ftStatus = FT_ListDevices((PVOID)devIndex, oldserial, FT_LIST_BY_INDEX|FT_OPEN_BY_SERIAL_NUMBER);
                if (!FT_SUCCESS(ftStatus))
                        continue;
                oldserial[sizeof(oldserial)-1] = 0;
		if (false) {
			FT_STATUS ftStatus = FT_Open(devIndex, &m_fth);
			err(ftStatus, "FT_Open failed");
			FT_PROGRAM_DATA eedata;
			memset(&eedata, 0, sizeof(eedata));
			char ManufacturerBuf[64];
			char ManufacturerIdBuf[16];
			char DescriptionBuf[64];
			char SerialNumberBuf[16];
			eedata.Signature1 = 0x00000000;
			eedata.Signature2 = 0xffffffff;
			eedata.Version = 0x00000004;
			eedata.Manufacturer = ManufacturerBuf;
			eedata.ManufacturerId = ManufacturerIdBuf;
			eedata.Description = DescriptionBuf;
			eedata.SerialNumber = SerialNumberBuf;
			ftStatus = FT_EE_Read(m_fth, &eedata);
			FT_Close(m_fth);
			err(ftStatus, "FT_EE_Read");
			std::cout << "Adapter: Desc \"" << desc << "\" Serial \"" << oldserial << "\"" << std::endl
				  << "Signature1 0x" << std::hex << eedata.Signature1
				  << " Signature2 0x" << eedata.Signature2
				  << " Version " << std::dec << eedata.Version << std::endl
				  << "VendorId 0x" << std::hex << eedata.VendorId
				  << " ProductId 0x" << eedata.ProductId << std::endl
				  << "Manufacturer " << std::dec << eedata.Manufacturer
				  << " ManufacturerId " << eedata.ManufacturerId << std::endl
				  << "Description " << eedata.Description
				  << " SerialNumber " << eedata.SerialNumber << std::endl
				  << "MaxPower " << eedata.MaxPower
				  << " PnP " << eedata.PnP
				  << " SelfPowered " << eedata.SelfPowered
				  << " RemoteWakeup " << eedata.RemoteWakeup << std::endl
				  << "Rev4 (FT232B) extensions" << std::endl
				  << "Rev4 " << (unsigned int)eedata.Rev4
				  << " IsoIn " << (unsigned int)eedata.IsoIn
				  << " IsoOut " << (unsigned int)eedata.IsoOut
				  << " PullDownEnable " << (unsigned int)eedata.PullDownEnable
				  << " SerNumEnable " << (unsigned int)eedata.SerNumEnable
				  << " USBVersionEnable " << (unsigned int)eedata.USBVersionEnable
				  << " USBVersion 0x" << std::hex << eedata.USBVersion << std::dec << std::endl
				  << "Rev 5 (FT2232) extensions" << std::endl
				  << "Rev5 " << (unsigned int)eedata.Rev5
				  << " IsoInA " << (unsigned int)eedata.IsoInA
				  << " IsoInB " << (unsigned int)eedata.IsoInB
				  << " IsoOutA " << (unsigned int)eedata.IsoOutA
				  << " IsoOutB " << (unsigned int)eedata.IsoOutB
				  << " PullDownEnable5 " << (unsigned int)eedata.PullDownEnable5
				  << " SerNumEnable5 " << (unsigned int)eedata.SerNumEnable5
				  << " USBVersionEnable5 " << (unsigned int)eedata.USBVersionEnable5
				  << " USBVersion5 0x" << std::hex << eedata.USBVersion5 << std::dec << std::endl
				  << "AIsHighCurrent " << (unsigned int)eedata.AIsHighCurrent
				  << " BIsHighCurrent " << (unsigned int)eedata.BIsHighCurrent
				  << " IFAIsFifo " << (unsigned int)eedata.IFAIsFifo
				  << " IFAIsFifoTar " << (unsigned int)eedata.IFAIsFifoTar
				  << " IFAIsFastSer " << (unsigned int)eedata.IFAIsFastSer
				  << " AIsVCP " << (unsigned int)eedata.AIsVCP
				  << " IFBIsFifo " << (unsigned int)eedata.IFBIsFifo
				  << " IFBIsFifoTar " << (unsigned int)eedata.IFBIsFifoTar
				  << " IFBIsFastSer " << (unsigned int)eedata.IFBIsFastSer
				  << " BIsVCP " << (unsigned int)eedata.BIsVCP << std::endl
				  << "Rev 6 (FT232R) extensions" << std::endl
				  << "UseExtOsc " << (unsigned int)eedata.UseExtOsc
				  << " HighDriveIOs " << (unsigned int)eedata.HighDriveIOs
				  << " EndpointSize " << (unsigned int)eedata.EndpointSize
				  << " PullDownEnableR " << (unsigned int)eedata.PullDownEnableR
				  << " SerNumEnableR " << (unsigned int)eedata.SerNumEnableR
				  << " InvertTXD " << (unsigned int)eedata.InvertTXD
				  << " InvertRXD " << (unsigned int)eedata.InvertRXD
				  << " InvertRTS " << (unsigned int)eedata.InvertRTS
				  << " InvertCTS " << (unsigned int)eedata.InvertCTS
				  << " InvertDTR " << (unsigned int)eedata.InvertDTR
				  << " InvertDSR " << (unsigned int)eedata.InvertDSR
				  << " InvertDCD " << (unsigned int)eedata.InvertDCD
				  << " InvertRI " << (unsigned int)eedata.InvertRI
				  << " Cbus0 " << (unsigned int)eedata.Cbus0
				  << " Cbus1 " << (unsigned int)eedata.Cbus1
				  << " Cbus2 " << (unsigned int)eedata.Cbus2
				  << " Cbus3 " << (unsigned int)eedata.Cbus3
				  << " Cbus4 " << (unsigned int)eedata.Cbus4
				  << " RIsD2XX " << (unsigned int)eedata.RIsD2XX << std::endl
				  << "Rev 7 (FT2232H) Extensions" << std::endl
				  << "PullDownEnable7 " << (unsigned int)eedata.PullDownEnable7
				  << " SerNumEnable7 " << (unsigned int)eedata.SerNumEnable7
				  << " ALSlowSlew " << (unsigned int)eedata.ALSlowSlew
				  << " ALSchmittInput " << (unsigned int)eedata.ALSchmittInput
				  << " ALDriveCurrent " << (unsigned int)eedata.ALDriveCurrent
				  << " AHSlowSlew " << (unsigned int)eedata.AHSlowSlew
				  << " AHSchmittInput " << (unsigned int)eedata.AHSchmittInput
				  << " AHDriveCurrent " << (unsigned int)eedata.AHDriveCurrent
				  << " BLSlowSlew " << (unsigned int)eedata.BLSlowSlew
				  << " BLSchmittInput " << (unsigned int)eedata.BLSchmittInput
				  << " BLDriveCurrent " << (unsigned int)eedata.BLDriveCurrent
				  << " BHSlowSlew " << (unsigned int)eedata.BHSlowSlew
				  << " BHSchmittInput " << (unsigned int)eedata.BHSchmittInput
				  << " BHDriveCurrent " << (unsigned int)eedata.BHDriveCurrent
				  << " IFAIsFifo7 " << (unsigned int)eedata.IFAIsFifo7
				  << " IFAIsFifoTar7 " << (unsigned int)eedata.IFAIsFifoTar7
				  << " IFAIsFastSer7 " << (unsigned int)eedata.IFAIsFastSer7
				  << " AIsVCP7 " << (unsigned int)eedata.AIsVCP7
				  << " IFBIsFifo7 " << (unsigned int)eedata.IFBIsFifo7
				  << " IFBIsFifoTar7 " << (unsigned int)eedata.IFBIsFifoTar7
				  << " IFBIsFastSer7 " << (unsigned int)eedata.IFBIsFastSer7
				  << " BIsVCP7 " << (unsigned int)eedata.BIsVCP7
				  << " PowerSaveEnable" << (unsigned int)eedata.PowerSaveEnable << std::endl
				  << "Rev 8 (FT4232H) Extensions" << std::endl
				  << "PullDownEnable8 " << (unsigned int)eedata.PullDownEnable8
				  << " SerNumEnable8 " << (unsigned int)eedata.SerNumEnable8
				  << " ASlowSlew " << (unsigned int)eedata.ASlowSlew
				  << " ASchmittInput " << (unsigned int)eedata.ASchmittInput
				  << " ADriveCurrent " << (unsigned int)eedata.ADriveCurrent
				  << " BSlowSlew " << (unsigned int)eedata.BSlowSlew
				  << " BSchmittInput " << (unsigned int)eedata.BSchmittInput
				  << " BDriveCurrent " << (unsigned int)eedata.BDriveCurrent
				  << " CSlowSlew " << (unsigned int)eedata.CSlowSlew
				  << " CSchmittInput " << (unsigned int)eedata.CSchmittInput
				  << " CDriveCurrent " << (unsigned int)eedata.CDriveCurrent
				  << " DSlowSlew " << (unsigned int)eedata.DSlowSlew
				  << " DSchmittInput " << (unsigned int)eedata.DSchmittInput
				  << " DDriveCurrent " << (unsigned int)eedata.DDriveCurrent
				  << " ARIIsTXDEN " << (unsigned int)eedata.ARIIsTXDEN
				  << " BRIIsTXDEN " << (unsigned int)eedata.BRIIsTXDEN
				  << " CRIIsTXDEN " << (unsigned int)eedata.CRIIsTXDEN
				  << " DRIIsTXDEN " << (unsigned int)eedata.DRIIsTXDEN
				  << " AIsVCP8 " << (unsigned int)eedata.AIsVCP8
				  << " BIsVCP8 " << (unsigned int)eedata.BIsVCP8
				  << " CIsVCP8 " << (unsigned int)eedata.CIsVCP8
				  << " DIsVCP8 " << (unsigned int)eedata.DIsVCP8 << std::endl;
		}
		if (!force && std::string(desc) != dev_orig_product) {
			std::cout << "Skipping Adapter: Desc \"" << desc << "\" Serial \"" << oldserial << "\"" << std::endl;
                        continue;
		}
		std::cout << "Programming Adapter: Desc \"" << desc << "\" Serial \"" << oldserial << "\"" << std::endl;
		FT_STATUS ftStatus = FT_Open(devIndex, &m_fth);
		err(ftStatus, "FT_Open failed");
		if (ftStatus != FT_OK)
			break;
		FT_PROGRAM_DATA eedata;
		memset(&eedata, 0, sizeof(eedata));
		eedata.Signature1 = 0x00000000;			// Header - must be 0x00000000
		eedata.Signature2 = 0xffffffff;			// Header - must be 0xffffffff
		eedata.Version = 4;			  	// Header - FT_PROGRAM_DATA version
		//			0 = original
		//			1 = FT2232C extensions
		//			2 = FT232R extensions
		//			3 = FT2232H extensions
		//			4 = FT4232H extensions
		eedata.VendorId = vendor_id;			// 0x0403
		eedata.ProductId = device_id;			// 0x6001
		eedata.Manufacturer = (char *)manufacturer;	// "FTDI"
		eedata.ManufacturerId = (char *)dev_mfgid;	// "FT"
		eedata.Description = (char *)product;		// "USB HS Serial Converter"
		eedata.SerialNumber = (char *)serial;		// "FT000001" if fixed, or NULL
		eedata.MaxPower = 50;				// 0 < MaxPower <= 500
		eedata.PnP = 1;					// 0 = disabled, 1 = enabled
		eedata.SelfPowered = 0;				// 0 = bus powered, 1 = self powered
		eedata.RemoteWakeup = 0;			// 0 = not capable, 1 = capable
		//
		// Rev4 (FT232B) extensions
		//
		eedata.Rev4 = 0;				// non-zero if Rev4 chip, zero otherwise
		eedata.IsoIn = 0;				// non-zero if in endpoint is isochronous
		eedata.IsoOut = 0;				// non-zero if out endpoint is isochronous
		eedata.PullDownEnable = 0;			// non-zero if pull down enabled
		eedata.SerNumEnable = 0;			// non-zero if serial number to be used
		eedata.USBVersionEnable = 0;			// non-zero if chip uses USBVersion
		eedata.USBVersion = 0x0000;			// BCD (0x0200 => USB2)
		//
		// Rev 5 (FT2232) extensions
		//
		eedata.Rev5 = 0;				// non-zero if Rev5 chip, zero otherwise
		eedata.IsoInA = 0;				// non-zero if in endpoint is isochronous
		eedata.IsoInB = 0;				// non-zero if in endpoint is isochronous
		eedata.IsoOutA = 0;				// non-zero if out endpoint is isochronous
		eedata.IsoOutB = 0;				// non-zero if out endpoint is isochronous
		eedata.PullDownEnable5 = 0;			// non-zero if pull down enabled
		eedata.SerNumEnable5 = 0;			// non-zero if serial number to be used
		eedata.USBVersionEnable5 = 0;			// non-zero if chip uses USBVersion
		eedata.USBVersion5 = 0x0000;			// BCD (0x0200 => USB2)
		eedata.AIsHighCurrent = 0;			// non-zero if interface is high current
		eedata.BIsHighCurrent = 0;			// non-zero if interface is high current
		eedata.IFAIsFifo = 0;				// non-zero if interface is 245 FIFO
		eedata.IFAIsFifoTar = 0;			// non-zero if interface is 245 FIFO CPU target
		eedata.IFAIsFastSer = 0;			// non-zero if interface is Fast serial
		eedata.AIsVCP = 0;				// non-zero if interface is to use VCP drivers
		eedata.IFBIsFifo = 0;				// non-zero if interface is 245 FIFO
		eedata.IFBIsFifoTar = 0;			// non-zero if interface is 245 FIFO CPU target
		eedata.IFBIsFastSer = 0;			// non-zero if interface is Fast serial
		eedata.BIsVCP = 0;				// non-zero if interface is to use VCP drivers
		//
		// Rev 6 (FT232R) extensions
		//
		eedata.UseExtOsc = 0;				// Use External Oscillator
		eedata.HighDriveIOs = 0;			// High Drive I/Os
		eedata.EndpointSize = 0;			// Endpoint size
		eedata.PullDownEnableR = 0;			// non-zero if pull down enabled
		eedata.SerNumEnableR = 0;			// non-zero if serial number to be used
		eedata.InvertTXD = 0;				// non-zero if invert TXD
		eedata.InvertRXD = 0;				// non-zero if invert RXD
		eedata.InvertRTS = 0;				// non-zero if invert RTS
		eedata.InvertCTS = 0;				// non-zero if invert CTS
		eedata.InvertDTR = 0;				// non-zero if invert DTR
		eedata.InvertDSR = 0;				// non-zero if invert DSR
		eedata.InvertDCD = 0;				// non-zero if invert DCD
		eedata.InvertRI = 0;				// non-zero if invert RI
		eedata.Cbus0 = FT_232R_CBUS_IOMODE;		// Cbus Mux control
		eedata.Cbus1 = FT_232R_CBUS_IOMODE;		// Cbus Mux control
		eedata.Cbus2 = FT_232R_CBUS_IOMODE;		// Cbus Mux control
		eedata.Cbus3 = FT_232R_CBUS_IOMODE;		// Cbus Mux control
		eedata.Cbus4 = FT_232R_CBUS_SLEEP;		// Cbus Mux control
		eedata.RIsD2XX = 0;				// non-zero if using D2XX driver
		//
		// Rev 7 (FT2232H) Extensions
		//
		eedata.PullDownEnable7 = 0;			// non-zero if pull down enabled
		eedata.SerNumEnable7 = 1;			// non-zero if serial number to be used
		eedata.ALSlowSlew = 0;				// non-zero if AL pins have slow slew
		eedata.ALSchmittInput = 1;			// non-zero if AL pins are Schmitt input
		eedata.ALDriveCurrent = 8;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.AHSlowSlew = 0;				// non-zero if AH pins have slow slew
		eedata.AHSchmittInput = 1;			// non-zero if AH pins are Schmitt input
		eedata.AHDriveCurrent = 8;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.BLSlowSlew = 0;				// non-zero if BL pins have slow slew
		eedata.BLSchmittInput = 1;			// non-zero if BL pins are Schmitt input
		eedata.BLDriveCurrent = 8;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.BHSlowSlew = 0;				// non-zero if BH pins have slow slew
		eedata.BHSchmittInput = 1;			// non-zero if BH pins are Schmitt input
		eedata.BHDriveCurrent = 8;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.IFAIsFifo7 = 0;				// non-zero if interface is 245 FIFO
		eedata.IFAIsFifoTar7 = 0;			// non-zero if interface is 245 FIFO CPU target
		eedata.IFAIsFastSer7 = 0;			// non-zero if interface is Fast serial
		eedata.AIsVCP7 = 0;				// non-zero if interface is to use VCP drivers
		eedata.IFBIsFifo7 = 0;				// non-zero if interface is 245 FIFO
		eedata.IFBIsFifoTar7 = 0;			// non-zero if interface is 245 FIFO CPU target
		eedata.IFBIsFastSer7 = 0;			// non-zero if interface is Fast serial
		eedata.BIsVCP7 = 0;				// non-zero if interface is to use VCP drivers
		eedata.PowerSaveEnable = 0;			// non-zero if using BCBUS7 to save power for self-powered designs
		//
		// Rev 8 (FT4232H) Extensions
		//
		eedata.PullDownEnable8 = 0;			// non-zero if pull down enabled
		eedata.SerNumEnable8 = 0;			// non-zero if serial number to be used
		eedata.ASlowSlew = 0;				// non-zero if AL pins have slow slew
		eedata.ASchmittInput = 0;			// non-zero if AL pins are Schmitt input
		eedata.ADriveCurrent = 0;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.BSlowSlew = 0;				// non-zero if AH pins have slow slew
		eedata.BSchmittInput = 0;			// non-zero if AH pins are Schmitt input
		eedata.BDriveCurrent = 0;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.CSlowSlew = 0;				// non-zero if BL pins have slow slew
		eedata.CSchmittInput = 0;			// non-zero if BL pins are Schmitt input
		eedata.CDriveCurrent = 0;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.DSlowSlew = 0;				// non-zero if BH pins have slow slew
		eedata.DSchmittInput = 0;			// non-zero if BH pins are Schmitt input
		eedata.DDriveCurrent = 0;			// valid values are 4mA, 8mA, 12mA, 16mA
		eedata.ARIIsTXDEN = 0;				// non-zero if port A uses RI as RS485 TXDEN
		eedata.BRIIsTXDEN = 0;				// non-zero if port B uses RI as RS485 TXDEN
		eedata.CRIIsTXDEN = 0;				// non-zero if port C uses RI as RS485 TXDEN
		eedata.DRIIsTXDEN = 0;				// non-zero if port D uses RI as RS485 TXDEN
		eedata.AIsVCP8 = 0;				// non-zero if interface is to use VCP drivers
		eedata.BIsVCP8 = 0;				// non-zero if interface is to use VCP drivers
		eedata.CIsVCP8 = 0;				// non-zero if interface is to use VCP drivers
		eedata.DIsVCP8 = 0;				// non-zero if interface is to use VCP drivers
		ftStatus = FT_EE_Program(m_fth, &eedata);
		FT_Close(m_fth);
		err(ftStatus, "FT_EE_Program failed");
        }		
}

class FtdiDump {
public:
	FtdiDump(void);
	~FtdiDump();
	std::vector<uint8_t> dump(void);

	static std::string to_str(FT_STATUS stat) { return FtdiProg::to_str(stat); }
	static void err(FT_STATUS stat, const std::string& str) { FtdiProg::err(stat, str); }

protected:
        FT_HANDLE m_fth;
};

FtdiDump::FtdiDump(void)
        : m_fth(0)
{
}

FtdiDump::~FtdiDump()
{
	if (m_fth)
		FT_Close(m_fth);
	m_fth = 0;
}

std::vector<uint8_t> FtdiDump::dump(void)
{
	if (m_fth)
		FT_Close(m_fth);
	m_fth = 0;
	FT_STATUS ftStatus = FT_Open(0, &m_fth);
	err(ftStatus, "FT_Open failed");
	std::vector<uint8_t> r(128, 0);
	for (unsigned int i = 0; i < r.size(); i += 2) {
		WORD w;
		ftStatus = FT_ReadEE(m_fth, i >> 1, &w);
		err(ftStatus, "FT_ReadEE failed");
		r[i] = w;
		r[i + 1] = w >> 8;
	}
	ftStatus = FT_Close(m_fth);
	m_fth = 0;
	err(ftStatus, "FT_Close failed");
	return r;
}

#endif

int main(int argc, char *argv[])
{
        static const struct option long_options[] = {
                { "manufacturer", required_argument, 0, 'M' },
		{ "product", required_argument, 0, 'P' },
		{ "program", no_argument, 0, 'p' },
                { "dump", no_argument, 0, 'd' },
                { "force", no_argument, 0, 'f' },
                { "serial", required_argument, 0, 's' },
                { "standard-serial", no_argument, 0, 'S' },
		{ 0, 0, 0, 0 }
        };
	std::cout << "FlightDeck RS232 Adapter EEPROM Writer" << std::endl;
	int c, err = 0;
	int mode = 0;
	bool force(false);
	std::string manufacturer(dev_manufacturer);
	std::string product(dev_product);
	std::string serial;
        while ((c = getopt_long(argc, argv, "M:P:pdfSs:ABCc", long_options, NULL)) != EOF) {
                switch (c) {
		case 'M':
			manufacturer = optarg;
			break;

		case 'P':
			product = optarg;
			break;

		case 'p':
			mode = 1;
			break;

		case 'd':
			mode = 2;
			break;

		case 'f':
			force = true;
			break;

		case 'S':
			serial = dev_serial;
			break;

		case 's':
			serial = optarg;
			break;

		default:
                        ++err;
                        break;
                }
        }
        if (err || !mode) {
                std::cerr << "usage: [-p] [-d] [-f] [-s <serial>] [-S]" << std::endl;
                return 1;
        }
	try {
		switch (mode) {
		case 1:
		{
			if (serial.empty()) {
				Glib::Checksum chk(Glib::Checksum::CHECKSUM_SHA256);
				if (!chk)
					throw std::runtime_error("serial mixer error");
				{
					Glib::TimeVal tv;
					tv.assign_current_time();
					uint8_t sec[4];
					uint8_t usec[4];
					for (unsigned int i = 0; i < 4; ++i) {
						sec[i] = tv.tv_sec >> (i << 3);
						usec[i] = tv.tv_usec >> (i << 3);
					}
					chk.update(sec, sizeof(sec)/sizeof(sec[0]));
					chk.update(usec, sizeof(usec)/sizeof(usec[0]));
				}
				chk.update(Glib::getenv("HOSTNAME"));
				serial = chk.get_string();
				if (serial.size() > 8) {
					serial.erase(0, serial.size() - 8);
				} else if (serial.size() < 8) {
					serial.insert(0, 8 - serial.size(), '0');
				}
				for (std::string::iterator i(serial.begin()), e(serial.end()); i != e; ++i) {
					if (*i < 'a' || *i > 'f')
						continue;
					*i += 'A' - 'a';
				}
			}
			FtdiProg p;
			std::cout << "Programming manufacturer " << manufacturer << " product " << product
				  << " serial " << serial << std::endl;
			p.program_all(manufacturer.c_str(), product.c_str(), serial.c_str(), force);
			break;
		}

		case 2:
		{
			FtdiDump d;
			std::vector<uint8_t> r(d.dump());
			std::cout << "EEPROM Contents:";
			for (unsigned int i = 0; i < r.size(); ++i) {
				if (i)
					std::cout << ',';
				if (!(i & 15))
					std::cout << std::endl << "\t";
				std::ostringstream oss;
				oss << "0x" << std::hex << std::setfill('0') << std::setw(2)
				    << (unsigned int)r[i];
				std::cout << oss.str();
			}
			std::cout << std::endl;
			break;
		}

		default:
			break;
		}
	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
