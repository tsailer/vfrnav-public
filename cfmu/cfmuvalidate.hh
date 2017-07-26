#ifndef CFMUVALIDATE_H
#define CFMUVALIDATE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdepsgui.h"

#include <gtkmm.h>
#include <sigc++/sigc++.h>

#include "adr.hh"
#include "adrdb.hh"
#include "adrfplan.hh"
#include "adrrestriction.hh"
#include "cfmuvalidatewidget.hh"

class CFMUValidate : public Gtk::Main {
protected:
	class MainWindow : public Gtk::Window {
	public:
		explicit MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Builder>& builder);
		virtual ~MainWindow();
		void set_app(CFMUValidate *app);
		void set_dbdirs(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");

	protected:
		std::unique_ptr<AirportsDbQueryInterface> m_airportdb;
		std::unique_ptr<NavaidsDbQueryInterface> m_navaiddb;
		std::unique_ptr<WaypointsDbQueryInterface> m_waypointdb;
		std::unique_ptr<AirwaysDbQueryInterface> m_airwaydb;
		std::unique_ptr<AirspacesDbQueryInterface> m_airspacedb;
		TopoDb30 m_topodb;
		Engine *m_engine;
		ADR::Database m_adrdb;
		ADR::RestrictionEval m_reval;
		std::string m_dir_main;
		std::string m_dir_aux;
		Glib::RefPtr<Gtk::TextTag> m_txttagmsginfo;
		Glib::RefPtr<Gtk::TextTag> m_txttagmsgwarn;
		Glib::RefPtr<Gtk::TextTag> m_txttagrule;
		Glib::RefPtr<Gtk::TextTag> m_txttagdesc;
		Glib::RefPtr<Gtk::TextTag> m_txttagoprgoal;
		Glib::RefPtr<Gtk::TextTag> m_txttagcond;
		Glib::RefPtr<Gtk::TextTag> m_txttagrestrict;
		Glib::RefPtr<Gtk::TextTag> m_txttagfpl;
		Glib::RefPtr<Builder> m_builder;
		Gtk::AboutDialog *m_aboutwindow;
		Gtk::TextView *m_textviewfplan;
		Gtk::TextView *m_textviewresults;
		CFMUValidateWidget *m_validationwidget;
		Gtk::Button *m_buttonabout;
		Gtk::Button *m_buttonclear;
		Gtk::Button *m_buttonvalidate;
		Gtk::Button *m_buttonvalidateadr;
		Gtk::Button *m_buttonquit;
		CFMUValidate *m_app;
		Engine::auxdb_mode_t m_auxdbmode;
		bool m_tfrloaded;
		bool m_honourprofilerules;

		void validate_clicked(void);
		void validate_adr_clicked(void);
		void clear_clicked(void);
		void quit_clicked(void);
		void about_clicked(void);
		void about_done(int resp);
		void fpl_changed(void);
		void validate_results(const CFMUValidateWidget::validate_result_t& results);

		void add_resulttext(const Glib::ustring& txt, const Glib::RefPtr<Gtk::TextTag>& tag);
		void add_resulttext(const ADR::Message& m);
		void add_resulttext(const ADR::RestrictionResult& res);
		void add_resulttext(const ADR::Object::ptr_t& p, uint64_t tm);
		void print_adr_tracerules(void);
		void print_adr_disabledrules(void);
	};

public:
	CFMUValidate(int& argc, char**& argv, Glib::OptionContext& option_context);
	~CFMUValidate();
	void run(void);
	void set_dbdirs(const std::string& dir_main = "", Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs, const std::string& dir_aux = "");

protected:
	Glib::RefPtr<Builder> m_builder;
	MainWindow *m_mainwindow;
};

#endif /* CFMUVALIDATE_H */
