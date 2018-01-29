/***************************************************************************
 *   Copyright (C) 2007, 2009, 2012, 2013, 2017 by Thomas Sailer           *
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

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include "RouteEditUi.h"
#include "Navigate.h"
#include "icaofpl.h"

int main(int argc, char *argv[])
{
        try {
                std::string dir_main(""), dir_aux("");
                Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs;
                bool dis_aux(false);
		Glib::init();
                Glib::OptionGroup optgroup("vfrnav", "VFR Navigation Application Options", "Options controlling the VFR Navigation Application");
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
                osso_context_t* osso_context = osso_initialize("vfrnav", VERSION, TRUE /* deprecated parameter */, 0 /* Use default Glib main loop context */);
                if (!osso_context) {
                        std::cerr << "osso_initialize() failed." << std::endl;
                        return OSSO_ERROR;
                }
#endif
#ifdef HAVE_HILDONMM
                Hildon::init();
#endif
#ifdef HAVE_CURL
		curl_global_init(CURL_GLOBAL_ALL);
#endif
                Glib::set_application_name("VFR Navigation");
                Glib::thread_init();
                Engine engine(dir_main, auxdbmode, dir_aux, false, false);
		for (int ind = 1; ind < argc; ++ind) {
			if (strncmp(argv[ind], "garminpilot://", 14))
				continue;
			// handle garminpilot
			IcaoFlightPlan fpl(engine);
			{
				IcaoFlightPlan::errors_t err(fpl.parse_garminpilot(argv[ind], true));
				if (!err.empty()) {
					std::cerr << "Error parsing garminpilot flightplan: " << argv[ind] << std::endl;
						for (IcaoFlightPlan::errors_t::const_iterator ei(err.begin()), ee(err.end()); ei != ee; ++ei)
							std::cerr << *ei << std::endl;
						continue;
				}
			}
			if (false)
				std::cerr << "Successfully parsed garminpilot flightplan: " << argv[ind] << std::endl;
			FPlanRoute route(engine.get_fplan_db());
			route.new_fp();
			fpl.set_route(route);
			route.save_fp();
			if (false)
				std::cerr << "Successfully saved garminpilot flightplan: " << route.get_id() << std::endl;
			engine.get_prefs().curplan = route.get_id();
		}
		{
                        Glib::RefPtr<PrefsWindow> prefswindow(PrefsWindow::create(engine.get_prefs()));
                        RouteEditUi routeeditui(engine, prefswindow);
                        Navigate navui(engine, prefswindow);

                        routeeditui.signal_startnav().connect(sigc::mem_fun(navui, &Navigate::start_nav));
                        routeeditui.signal_updateroute().connect(sigc::mem_fun(navui, &Navigate::update_route));

                        navui.signal_endnav().connect(sigc::mem_fun(routeeditui, &RouteEditUi::end_navigate));
                        navui.signal_updateroute().connect(sigc::mem_fun(routeeditui, &RouteEditUi::update_route));

                        Gtk::Main::run(*routeeditui.get_routeeditwindow());
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
        return 0;
}
