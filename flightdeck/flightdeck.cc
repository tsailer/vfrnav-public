/***************************************************************************
 *   Copyright (C) 2007, 2009, 2012, 2013 by Thomas Sailer                 *
 *   t.sailer@alumni.ethz.ch                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "sysdeps.h"

#include <gtkmm/main.h>

#include "engine.h"
#include "sensors.h"
#include "flightdeckwindow.h"
#include "sysdepsgui.h"

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_OBJBASE_H
#include <objbase.h>
#endif

#if 0

// disable screen saver stuff

/*
 * XScreensaver stuff
 */

static int screensaver_off;
static unsigned int time_last;


static int xss_suspend(Bool suspend)
{
#ifndef CONFIG_XSS
    return 0;
#else
    int event, error, major, minor;
    if (XScreenSaverQueryExtension(mDisplay, &event, &error) != True ||
        XScreenSaverQueryVersion(mDisplay, &major, &minor) != True)
        return 0;
    if (major < 1 || (major == 1 && minor < 1))
        return 0;
    XScreenSaverSuspend(mDisplay, suspend);
    return 1;
#endif
}

/*
 * End of XScreensaver stuff
 */

void saver_on(Display * mDisplay)
{

    if (!screensaver_off)
        return;
    screensaver_off = 0;
    if (xss_suspend(False))
        return;
#ifdef CONFIG_XDPMS
    if (dpms_disabled)
    {
        int nothing;
        if (DPMSQueryExtension(mDisplay, &nothing, &nothing))
        {
            if (!DPMSEnable(mDisplay))
            {                   // restoring power saving settings
                mp_msg(MSGT_VO, MSGL_WARN, "DPMS not available?\n");
            } else
            {
                // DPMS does not seem to be enabled unless we call DPMSInfo
                BOOL onoff;
                CARD16 state;

                DPMSForceLevel(mDisplay, DPMSModeOn);
                DPMSInfo(mDisplay, &state, &onoff);
                if (onoff)
                {
                    mp_msg(MSGT_VO, MSGL_V,
                           "Successfully enabled DPMS\n");
                } else
                {
                    mp_msg(MSGT_VO, MSGL_WARN, "Could not enable DPMS\n");
                }
            }
        }
        dpms_disabled = 0;
    }
#endif
}

void saver_off(Display * mDisplay)
{
    int nothing;

    if (!stop_xscreensaver || screensaver_off)
        return;
    screensaver_off = 1;
    if (xss_suspend(True))
        return;
#ifdef CONFIG_XDPMS
    if (DPMSQueryExtension(mDisplay, &nothing, &nothing))
    {
        BOOL onoff;
        CARD16 state;

        DPMSInfo(mDisplay, &state, &onoff);
        if (onoff)
        {
            Status stat;

            mp_msg(MSGT_VO, MSGL_V, "Disabling DPMS\n");
            dpms_disabled = 1;
            stat = DPMSDisable(mDisplay);       // monitor powersave off
            mp_msg(MSGT_VO, MSGL_V, "DPMSDisable stat: %d\n", stat);
        }
    }
#endif
}


#endif



int main(int argc, char *argv[])
{
	try {
                std::string dir_main(""), dir_aux("");
                Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs;
                bool dis_aux(false);
		Glib::init();
		Gio::init();
               Glib::OptionGroup optgroup("vfrnav", "Flight Deck Application Options", "Options controlling the Flight Deck Application");
                Glib::OptionEntry optmaindir, optauxdir, optdisableaux;
                optmaindir.set_short_name('m');
                optmaindir.set_long_name("maindir");
                optmaindir.set_description("Directory containing the main database");
                optmaindir.set_arg_description("DIR");
                optauxdir.set_short_name('a');
                optauxdir.set_long_name("auxdir");
                optauxdir.set_description("Directory containing the auxilliary database");
                optauxdir.set_arg_description("DIR");
                optdisableaux.set_short_name('A');
                optdisableaux.set_long_name("disableaux");
                optdisableaux.set_description("Disable the auxilliary database");
                optdisableaux.set_arg_description("BOOL");
                optgroup.add_entry_filename(optmaindir, dir_main);
                optgroup.add_entry_filename(optauxdir, dir_aux);
                optgroup.add_entry(optdisableaux, dis_aux);
                Glib::OptionContext optctx("[--maindir=<dir>] [--auxdir=<dir>] [--disableaux]");
                optctx.set_help_enabled(true);
                optctx.set_ignore_unknown_options(false);
                optctx.set_main_group(optgroup);
                Gtk::Main m(argc, argv, optctx);
                if (dis_aux)
                        auxdbmode = Engine::auxdb_none;
                else if (!dir_aux.empty())
                        auxdbmode = Engine::auxdb_override;
#ifdef HAVE_OSSO
                osso_context_t* osso_context = osso_initialize("flightdeck", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
                if (!osso_context) {
                        std::cerr << "osso_initialize() failed." << std::endl;
                        return OSSO_ERROR;
                }
#endif
#ifdef HAVE_HILDONMM
                Hildon::init();
#endif
#ifdef HAVE_OBJBASE_H
		{
			// COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE
			HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			if (!SUCCEEDED(hr)) {
				std::cerr << "Cannot initialize OLE" << std::endl;
				return 1;
			}
		}
#endif
#ifdef HAVE_CURL
		curl_global_init(CURL_GLOBAL_ALL);
#endif
                Glib::set_application_name("Flight Deck");
                Glib::thread_init();
                Engine engine(dir_main, auxdbmode, dir_aux, true, true);
		Sensors sensors;
		sensors.set_engine(engine);
#ifdef HAVE_GTKMM3
		{
			Glib::RefPtr<Gtk::CssProvider> css(Gtk::CssProvider::create());
#ifdef __MINGW32__
			typedef std::vector<Glib::ustring> dirs_t;
			dirs_t dirs;
			dirs.push_back(PACKAGE_DATA_DIR);
			{
				gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
				if (instdir) {
					dirs.push_back(Glib::build_filename(instdir, "share\\vfrnav"));
					dirs.push_back(Glib::build_filename(instdir, "share"));
					dirs.push_back(Glib::build_filename(instdir, "vfrnav"));
					dirs.push_back(instdir);
					dirs.push_back(Glib::build_filename(instdir, "..\\share\\vfrnav"));
					dirs.push_back(Glib::build_filename(instdir, "..\\share"));
					dirs.push_back(Glib::build_filename(instdir, "..\\vfrnav"));
					g_free(instdir);
				}
			}
			dirs_t::const_iterator di(dirs.begin()), de(dirs.end());
			for (; di != de; ++di) {
				Glib::ustring fn(Glib::build_filename(*di, "themes", "gtk-3.0", "flightdeck.css"));
				if (true)
					std::cerr << "load theme: trying " << fn << std::endl;
				if (!Glib::file_test(fn, Glib::FILE_TEST_IS_REGULAR))
					continue;
				css->load_from_path(fn);
				break;
			}
			if (di == de)
				throw std::runtime_error("theme file flightdeck.css not found");
#else /* __MINGW32__ */
			Glib::ustring fn(Glib::build_filename(PACKAGE_DATA_DIR, "themes", "gtk-3.0", "flightdeck.css"));
			if (true)
				std::cerr << "load theme: " << fn << std::endl;
			css->load_from_path(fn);
#endif /* __MINGW32__ */
			if (true)
				std::cerr << "theme loaded: " << css->to_string() << std::endl;
			Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), css, GTK_STYLE_PROVIDER_PRIORITY_USER);
		}
#endif
		Glib::RefPtr<Builder> refxml(load_glade_file("flightdeck" UIEXT));
		FlightDeckWindow::SplashWindow *splashwindow(0);
		get_widget_derived(refxml, "splashwindow", splashwindow);
		FlightDeckWindow *flightdeckwindow(0);
		get_widget_derived(refxml, "flightdeckwindow", flightdeckwindow);
		if (splashwindow)
			splashwindow->hide();
		if (flightdeckwindow)
			flightdeckwindow->hide();
		{
			std::set<Sensors::ConfigFile> cfgfiles(Sensors::find_configs(dir_main, engine.get_aux_dir(auxdbmode, dir_aux)));
			std::set<Sensors::ConfigFile>::size_type cfgsz(cfgfiles.size());
			if (!cfgsz || (cfgsz > 1 && !splashwindow)) {
				Glib::RefPtr<Gtk::MessageDialog> dlg(new Gtk::MessageDialog("Flight Deck: No Configuration found", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true));
				dlg->signal_response().connect(sigc::hide(sigc::mem_fun(*dlg.operator->(), &Gtk::MessageDialog::hide)));
				Gtk::Main::run(*dlg.operator->());
#ifdef HAVE_OSSO
				osso_deinitialize(osso_context);
#endif
				return 1;
			}
			std::set<Sensors::ConfigFile>::const_iterator cfgres(cfgfiles.begin());
			if (cfgsz > 1) {
				splashwindow->set_configs(cfgfiles.begin(), cfgfiles.end());
				Gtk::Main::run(*splashwindow);
				if (splashwindow->get_result().empty()) {
#ifdef HAVE_OSSO
					osso_deinitialize(osso_context);
#endif
					return 1;
				}
				cfgres = cfgfiles.find(Sensors::ConfigFile(splashwindow->get_result()));
			}
			if (cfgres == cfgfiles.end()) {
#ifdef HAVE_OSSO
				osso_deinitialize(osso_context);
#endif
				return 1;
			}
			sensors.init(const_cast<Sensors::ConfigFile&>(*cfgres).to_data(), cfgres->get_filename());
		}
		if (flightdeckwindow) {
			flightdeckwindow->set_engine(engine);
			flightdeckwindow->set_sensors(sensors);
			Gtk::Main::run(*flightdeckwindow);
		}
#ifdef HAVE_OSSO
                osso_deinitialize(osso_context);
#endif
        } catch (const Glib::Exception& e) {
                std::cerr << "glib exception: " << e.what() << std::endl;
                return 1;
        } catch (const std::exception& e) {
                std::cerr << "exception: " << e.what() << std::endl;
                return 1;
        }
#ifdef HAVE_OBJBASE_H
        ::CoUninitialize();
#endif
        return 0;
}
