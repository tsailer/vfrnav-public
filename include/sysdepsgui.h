//
// C++ Interface: sysdepsgui
//
// Description: GUI System Dependency "equalization"
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2007, 2009, 2012
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef SYSDEPSGUI_H
#define SYSDEPSGUI_H

#include "sysdeps.h"

#include <gtkmm.h>
#include <stdexcept>
#include <iostream>

#ifdef HAVE_GTKMM2
#include <libglademm.h>
typedef Gnome::Glade::Xml Builder;
#endif

#ifdef HAVE_GTKMM3
typedef Gtk::Builder Builder;
#endif

inline Glib::RefPtr<Builder> load_glade_file_nosearch(const Glib::ustring& fname, const Glib::ustring& wname = "")
{
#ifdef HAVE_GTKMM3
        Glib::RefPtr<Builder> refxml;
	// hack: load full file anyway, otherwise references to liststores or adjustments may be missing
	if (true || wname.empty())
		refxml = Builder::create_from_file(fname);
	else
		refxml = Builder::create_from_file(fname, wname);
#elif defined(GLIBMM_EXCEPTIONS_ENABLED)
        Glib::RefPtr<Builder> refxml = Builder::create(fname, wname);
#else /* GLIBMM_EXCEPTIONS_ENABLED */
        std::auto_ptr<Gnome::Glade::XmlError> ex;
        Glib::RefPtr<Builder> refxml = Builder::create(fname, wname, Glib::ustring(), ex);
        if (ex.get())
                throw *ex;
#endif /* GLIBMM_EXCEPTIONS_ENABLED */
        return refxml;
}

inline Glib::RefPtr<Builder> load_glade_file(const Glib::ustring& fname, const Glib::ustring& wname = "")
{
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
        for (dirs_t::const_iterator di(dirs.begin()), de(dirs.end()); di != de; ++di) {
                Glib::ustring fn(Glib::build_filename(*di, fname));
                if (true)
                        std::cerr << "load_glade_file: trying " << fn << std::endl;
                if (!Glib::file_test(fn, Glib::FILE_TEST_IS_REGULAR))
                        continue;
                return load_glade_file_nosearch(fn, wname);
        }
        throw std::runtime_error("glade file " + fname + " not found");
#else /* __MINGW32__ */
        return load_glade_file_nosearch(Glib::build_filename(PACKAGE_DATA_DIR, fname), wname);
#endif /* __MINGW32__ */
}

template<class T_Widget>
inline void get_widget(const Glib::RefPtr<Builder>& refxml, const Glib::ustring& name, T_Widget*& widget)
{
	refxml->get_widget(name, widget);
	if (widget)
		widget->set_name(name);
}

template<class T_Widget>
inline void get_widget_derived(const Glib::RefPtr<Builder>& refxml, const Glib::ustring& name, T_Widget*& widget)
{
	refxml->get_widget_derived(name, widget);
	if (widget)
		widget->set_name(name);
}

#endif /* SYSDEPSGUI_H */
