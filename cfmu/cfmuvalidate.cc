#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <iomanip>
#include <gdkmm.h>
#include <gtkmm.h>

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#else
#define EX_OK           0
#define EX_USAGE        64
#define EX_DATAERR      65
#define EX_NOINPUT      66
#define EX_UNAVAILABLE  69
#endif  

#include "cfmuvalidate.hh"
#include "icaofpl.h"

CFMUValidate::MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Builder>& builder)
	: Gtk::Window(cobject), m_engine(0), m_builder(builder), m_aboutwindow(0),
	  m_textviewresults(0), m_validationwidget(0),
	  m_buttonabout(0), m_buttonclear(0), m_buttonvalidate(0), m_buttonvalidateadr(0),
	  m_buttonquit(0), m_app(0), m_tfrloaded(false), m_honourprofilerules(true)
{
	m_builder->get_widget("cfmuaboutdialog", m_aboutwindow);
	m_builder->get_widget("textviewfplan", m_textviewfplan);
	m_builder->get_widget("textviewresults", m_textviewresults);
	m_builder->get_widget_derived("validationwidget", m_validationwidget);
	m_builder->get_widget("buttonabout", m_buttonabout);
	m_builder->get_widget("buttonclear", m_buttonclear);
	m_builder->get_widget("buttonvalidate", m_buttonvalidate);
	m_builder->get_widget("buttonvalidateadr", m_buttonvalidateadr);
	m_builder->get_widget("buttonquit", m_buttonquit);

	if (m_buttonvalidate)
		m_buttonvalidate->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::validate_clicked));
	if (m_buttonvalidateadr)
		m_buttonvalidateadr->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::validate_adr_clicked));
	if (m_buttonquit)
		m_buttonquit->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::quit_clicked));
	if (m_buttonabout)
		m_buttonabout->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::about_clicked));
	if (m_buttonclear)
		m_buttonclear->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::clear_clicked));
	if (m_aboutwindow)
		m_aboutwindow->signal_response().connect(sigc::mem_fun(*this, &MainWindow::about_done));
	if (m_textviewfplan) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfplan->get_buffer());
		if (buf) {
			buf->set_text("-(FPL-HBPBX-IG -1P28R/L -SDFGLO/S -LSZH1235 -N0135F070 ZUE -EDNY0026 LSZR -)");
			buf->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::fpl_changed));
		}
	}

	if (m_textviewresults) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresults->get_buffer());
		if (buf) {
			m_txttagmsginfo = buf->create_tag();
			m_txttagmsginfo->property_weight() = 400;
			m_txttagmsginfo->property_weight_set() = true;
			m_txttagmsginfo->property_style() = Pango::STYLE_NORMAL;
			m_txttagmsginfo->property_style_set() = true;
			m_txttagmsgwarn = buf->create_tag();
			m_txttagmsgwarn->property_weight() = 400;
			m_txttagmsgwarn->property_weight_set() = true;
			m_txttagmsgwarn->property_style() = Pango::STYLE_ITALIC;
			m_txttagmsgwarn->property_style_set() = true;
			m_txttagrule = buf->create_tag();
			m_txttagrule->property_weight() = 600;
			m_txttagrule->property_weight_set() = true;
			m_txttagrule->property_style() = Pango::STYLE_NORMAL;
			m_txttagrule->property_style_set() = true;
			m_txttagdesc = buf->create_tag();
			m_txttagdesc->property_weight() = 400;
			m_txttagdesc->property_weight_set() = true;
			m_txttagdesc->property_style() = Pango::STYLE_NORMAL;
			m_txttagdesc->property_style_set() = true;
			m_txttagoprgoal = buf->create_tag();
			m_txttagoprgoal->property_weight() = 400;
			m_txttagoprgoal->property_weight_set() = true;
			m_txttagoprgoal->property_style() = Pango::STYLE_ITALIC;
			m_txttagoprgoal->property_style_set() = true;
			m_txttagcond = buf->create_tag();
			m_txttagcond->property_weight() = 300;
			m_txttagcond->property_weight_set() = true;
			m_txttagcond->property_style() = Pango::STYLE_NORMAL;
			m_txttagcond->property_style_set() = true;
			m_txttagrestrict = buf->create_tag();
			m_txttagrestrict->property_weight() = 400;
			m_txttagrestrict->property_weight_set() = true;
			m_txttagrestrict->property_style() = Pango::STYLE_NORMAL;
			m_txttagrestrict->property_style_set() = true;
			m_txttagfpl = buf->create_tag();
			m_txttagfpl->property_weight() = 400;
			m_txttagfpl->property_weight_set() = true;
			m_txttagfpl->property_style() = Pango::STYLE_NORMAL;
			m_txttagfpl->property_style_set() = true;
		}
	}

	if (m_validationwidget) {
		m_validationwidget->set_cookiefile(Glib::build_filename(FPlan::get_userdbpath(), "cookies.txt"));
		m_validationwidget->start();
	}

	add_resulttext("Note: the first local validation will take some time due to loading the eRAD restrictions; "
		       "after the first local validation, failed CFMU validation rules will display a rule explanation\n", m_txttagcond);
}

CFMUValidate::MainWindow::~MainWindow()
{
	delete m_aboutwindow;
	delete m_engine;
}

void CFMUValidate::MainWindow::set_app(CFMUValidate *app)
{
	m_app = app;
}

//<button type="button" class="portal_buttonToolsButton_enabled" id="FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.VALIDATE_ACTION_LABEL">Validate</button>
//<textarea class="gwt-TextArea ifpuv_freeTextPanel_fplTextArea" id="FREE_TEXT_EDITOR.FLIGHT_DATA_AREA.GENERAL_DATA_ENTRY.INTRODUCE_FLIGHT_PLAN_FIELD"></textarea>
//<table cellspacing="0" cellpadding="0" class="cfmu_DisclosurePanel cfmu_DisclosurePanel-closed" id="FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA"><tbody><tr><td align="left" style="vertical-align: top; "><a href="javascript:void(0);" style="display: block; " class="header"><table cellspacing="0" cellpadding="0" class="portal_fullWidthStyle"><tbody><tr><td align="left" style="vertical-align: top; " width="" height="" rowspan="1"><div class="cfmu_DisclosurePanel_Label cfmu_DisclosurePanel_Enabled">Validation&nbsp;Results</div></td><td align="left" style="vertical-align: top; width: 100%; " width="" height=""><div class="gwt-HTML">&nbsp;</div></td><td align="left" style="vertical-align: top; " width="" height="" rowspan="1"><div class="portal_disclosurePanelRightTextStyle_0"></div></td></tr></tbody></table></a></td></tr><tr><td align="left" style="vertical-align: top; "><div style="overflow-x: hidden; overflow-y: hidden; display: none; padding-top: 0px; padding-right: 0px; padding-bottom: 0px; padding-left: 12px; "><table cellspacing="0" cellpadding="0" class="portal_cfmuDisclosurePanel_contentContainer content"><tbody><tr><td align="left" style="vertical-align: top; "><table cellspacing="0" cellpadding="0" class="portal_fullSizeStyle portal_fullWidthStyle" style="display: none; "><tbody><tr><td align="left" style="vertical-align: top; "><div class="portal_warningLabel" style="display: none; "></div></td></tr><tr><td align="left" style="vertical-align: top; "><div class="portal_dataValue" style="text-align: left; display: none; ">NO ERRORS</div></td></tr><tr><td align="left" style="vertical-align: top; "><div style="padding-top: 0px; padding-right: 0px; padding-bottom: 0px; padding-left: 0px; position: relative; display: none; " id="FREE_TEXT_EDITOR.VALIDATION_RESULTS_AREA.ERRORS_DATA_TABLE" class="portal_fullWidthStyle"><div tabindex="0" style="display: none; position: relative; " class="portal_sliderBar"><input type="text" tabindex="-1" style="opacity: 0; height: 1px; width: 1px; z-index: -1; overflow-x: hidden; overflow-y: hidden; position: absolute; "><div style="position: absolute; left: 0px; " class="portal_sliderBarLine"></div><div class="portal_sliderBarKnob" style="position: absolute; left: -1px; "></div><div style="position: absolute; left: -1px; visibility: visible; " class="portal_sliderBarTick"></div><div style="position: absolute; left: -1px; visibility: visible; " class="portal_sliderBarLabel">1</div><div style="position: absolute; left: 0px; visibility: visible; " class="portal_sliderBarTick"></div><div style="position: absolute; left: 0px; visibility: visible; " class="portal_sliderBarLabel">1</div></div><div></div><table class="portal_noScreen portal_autopagetablecontainer_headerbackground portal_noPrint" cellpadding="0" cellspacing="0"><colgroup><col></colgroup><tbody><tr class="portal_autoPageTableContainer_headerRowStyle"><td class="portal_autoPageTableContainer_headerCellStyle"><div class="gwt-HTML">Errors</div></td></tr></tbody></table><div style="width: 100%; padding-top: 0px; padding-right: 0px; padding-bottom: 0px; padding-left: 0px; margin-top: 0px; margin-right: 0px; margin-bottom: 0px; margin-left: 0px; border-top-style: none; border-right-style: none; border-bottom-style: none; border-left-style: none; border-width: initial; border-color: initial; border-image: initial; position: relative; "><table cellpadding="0" cellspacing="0" class="portal_decotable"><colgroup><col></colgroup><tbody></tbody></table></div></div></td></tr><tr><td align="left" style="vertical-align: top; "><table cellspacing="0" cellpadding="0" class="portal_fullWidthStyle"><tbody><tr><td align="left" style="vertical-align: top; "><div class="portal_dataValue" style="display: none; ">No routes found.</div></td></tr><tr><td align="left" style="vertical-align: top; "><table cellspacing="0" cellpadding="0" id="FREE_TEXT_EDITOR.PROPOSE_ROUTES_RESULTS_AREA.ORIGINAL_ROUTE_DATA_TABLE"><tbody><tr><td align="left" style="vertical-align: top; "><table class="portal_headerTableDecorator_headerbackground" cellpadding="0" cellspacing="0"><colgroup><col></colgroup><tbody><tr class="portal_headerTableDecorator_headerRowStyle"></tr></tbody></table></td></tr><tr><td align="left" style="vertical-align: top; "><table cellpadding="0" cellspacing="0" class="portal_decotable"><colgroup><col></colgroup><tbody></tbody></table></td></tr></tbody></table></td></tr></tbody></table></td></tr></tbody></table></td></tr></tbody></table></div></td></tr></tbody></table>
//<div class="portal_dataValue" style="text-align: left; ">NO ERRORS</div>
//<td class="portal_decotableCell ">ROUTE126: FLIGHT RULES Y WITH NO VFR PART.</td>
//<td class="portal_decotableCell ">ROUTE130: UNKNOWN DESIGNATOR HOC3Z</td>

void CFMUValidate::MainWindow::validate_clicked(void)
{
	if (!m_validationwidget || !m_textviewfplan)
		return;
	Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfplan->get_buffer());
	if (!buf)
		return;
	const Glib::ustring& txt(buf->get_text());
	if (txt.empty())
		return;
	m_validationwidget->validate(txt, sigc::mem_fun(*this, &CFMUValidate::MainWindow::validate_results));
}

void CFMUValidate::MainWindow::clear_clicked(void)
{
	if (!m_textviewfplan)
		return;
	Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfplan->get_buffer());
	if (!buf)
		return;
	buf->set_text("");
}

void CFMUValidate::MainWindow::quit_clicked(void)
{
	if (m_app)
		m_app->quit();
}

void CFMUValidate::MainWindow::about_clicked(void)
{
	if (m_aboutwindow)
		m_aboutwindow->show();
}

void CFMUValidate::MainWindow::about_done(int resp)
{
	if (m_aboutwindow)
		m_aboutwindow->hide();
}

void CFMUValidate::MainWindow::fpl_changed(void)
{
	if (!m_textviewresults)
		return;
	Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresults->get_buffer());
	if (!buf)
		return;
	buf->set_text("");
}

void CFMUValidate::MainWindow::validate_results(const CFMUValidateWidget::validate_result_t& results)
{
	if (!m_textviewresults)
		return;
	Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresults->get_buffer());
	if (!buf)
		return;
	buf->set_text("");
	// locally look up failing rules and get a description
	for (CFMUValidateWidget::validate_result_t::const_iterator ri(results.begin()), re(results.end()); ri != re; ++ri) {
		add_resulttext(*ri + "\n", m_txttagmsginfo);
		if (!m_tfrloaded)
			continue;
		Glib::ustring::size_type n(ri->rfind('['));
		if (n == Glib::ustring::npos)
			continue;
		Glib::ustring::size_type ne(ri->find(']', n + 1));
		if (ne == Glib::ustring::npos || ne - n < 2 || ne - n > 11)
			continue;
		std::string ruleid(ri->substr(n + 1, ne - n - 1));
		{
			uint64_t tm(time(0));
			ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end());
			for (; ri != re; ++ri) {
				const ADR::Object::ptr_t& p(*ri);
				ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
				if (!ts.is_valid())
					continue;
				if (ts.get_ident() != ruleid)
					continue;
				add_resulttext(p, tm);
				break;
			}
			if (ri != re)
				continue;
		}
		// no rule known
	}
}

void CFMUValidate::MainWindow::set_dbdirs(const std::string& dir_main, Engine::auxdb_mode_t auxdbmode, const std::string& dir_aux)
{
	m_dir_main = dir_main;
	m_dir_aux = dir_aux;
	m_auxdbmode = auxdbmode;
}

void CFMUValidate::MainWindow::add_resulttext(const Glib::ustring& txt, const Glib::RefPtr<Gtk::TextTag>& tag)
{
	if (!m_textviewresults)
		return;
	Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresults->get_buffer());
	if (!buf)
		return;
	buf->insert_with_tag(buf->end(), txt, tag);
}

void CFMUValidate::MainWindow::print_adr_tracerules(void)
{
	std::string r;
	bool subseq(false);
	uint64_t tm(time(0));
	for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
		const ADR::Object::ptr_t& p(*ri);
		const ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (!ts.is_trace())
			continue;
		if (subseq)
			r += ",";
		subseq = true;
		r += ts.get_ident();
	}
	add_resulttext("trace:" + r + "\n", m_txttagmsginfo);
}

void CFMUValidate::MainWindow::print_adr_disabledrules(void)
{
	std::string r;
	bool subseq(false);
	uint64_t tm(time(0));
	for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
		const ADR::Object::ptr_t& p(*ri);
		const ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
		if (!ts.is_valid())
			continue;
		if (ts.is_enabled())
			continue;
		if (subseq)
			r += ",";
		subseq = true;
		r += ts.get_ident();
	}
	add_resulttext("disabled:" + r + "\n", m_txttagmsginfo);
}

void CFMUValidate::MainWindow::add_resulttext(const ADR::Message& m)
{
	std::ostringstream oss;
	oss << m.get_type_char() << ':';
	{
		const std::string& id(m.get_ident());
		if (!id.empty())
			oss << " R:" << id;
	}
	if (!m.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(m.get_vertexset().begin()), e(m.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!m.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (ADR::Message::set_t::const_iterator i(m.get_edgeset().begin()), e(m.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	oss << ' ' << m.get_text();
	add_resulttext(oss.str() + "\n", (m.get_type() <= ADR::Message::type_warning) ? m_txttagmsgwarn : m_txttagmsginfo);
}

void CFMUValidate::MainWindow::add_resulttext(const ADR::RestrictionResult& res)
{
	const ADR::FlightRestrictionTimeSlice& tsrule(res.get_rule()->operator()(m_reval.get_departuretime()).as_flightrestriction());
	if (!tsrule.is_valid()) {
		add_resulttext("Rule: ?""?\n", m_txttagrule);
		return;
	}
	std::ostringstream oss;
	oss << "Rule: " << tsrule.get_ident() << ' ';
	switch (tsrule.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
		oss << "Mandatory";
		break;

	case ADR::FlightRestrictionTimeSlice::type_forbidden:
		oss << "Forbidden";
		break;

	case ADR::FlightRestrictionTimeSlice::type_closed:
		oss << "ClosedForCruising";
		break;

	case ADR::FlightRestrictionTimeSlice::type_allowed:
		oss << "Allowed";
		break;

	default:
		oss << '?';
		break;
	}
	if (tsrule.is_dct() || tsrule.is_unconditional() || tsrule.is_routestatic() ||
	    tsrule.is_mandatoryinbound() || tsrule.is_mandatoryoutbound() || !tsrule.is_enabled()) {
		oss << " (";
		if (!tsrule.is_enabled())
			oss << '-';
		if (tsrule.is_dct())
			oss << 'D';
		if (tsrule.is_unconditional())
			oss << 'U';
		if (tsrule.is_routestatic())
			oss << 'S';
		if (tsrule.is_mandatoryinbound())
			oss << 'I';
		if (tsrule.is_mandatoryoutbound())
			oss << 'O';
		oss << ')';
	}
	if (!res.get_vertexset().empty()) {
		oss << " V:";
		bool subseq(false);
		for (ADR::RestrictionResult::set_t::const_iterator i(res.get_vertexset().begin()), e(res.get_vertexset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!res.get_edgeset().empty()) {
		oss << " E:";
		bool subseq(false);
		for (ADR::RestrictionResult::set_t::const_iterator i(res.get_edgeset().begin()), e(res.get_edgeset().end()); i != e; ++i) {
			if (subseq)
				oss << ',';
			subseq = true;
			oss << *i;
		}
	}
	if (!res.has_refloc())
		oss << " L:" << res.get_refloc();
	add_resulttext(oss.str() + "\n", m_txttagrule);
	add_resulttext("Desc: " + tsrule.get_instruction() + "\n", m_txttagdesc);
	//add_resulttext("OprGoal: \n", m_txttagoprgoal);
	if (tsrule.get_condition())
		add_resulttext("Cond: " + tsrule.get_condition()->to_str(m_reval.get_departuretime()) + "\n", m_txttagcond);
	for (unsigned int i = 0, n = tsrule.get_restrictions().size(); i < n; ++i) {
		const ADR::RestrictionSequence& rs(tsrule.get_restrictions()[i]);
		const ADR::RestrictionSequenceResult& rsr(res[i]);
		{
			std::ostringstream oss;
			oss << "  Alternative " << i;
			if (!rsr.get_vertexset().empty()) {
				oss << " V:";
				bool subseq(false);
				for (ADR::RestrictionResult::set_t::const_iterator i(rsr.get_vertexset().begin()), e(rsr.get_vertexset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			if (!rsr.get_edgeset().empty()) {
				oss << " E:";
				bool subseq(false);
				for (ADR::RestrictionResult::set_t::const_iterator i(rsr.get_edgeset().begin()), e(rsr.get_edgeset().end()); i != e; ++i) {
					if (subseq)
						oss << ',';
					subseq = true;
					oss << *i;
				}
			}
			add_resulttext(oss.str() + "\n", m_txttagrestrict);
		}
		add_resulttext(rs.to_str(m_reval.get_departuretime()) + "\n", m_txttagrestrict);
	}
}

void CFMUValidate::MainWindow::add_resulttext(const ADR::Object::ptr_t& p, uint64_t tm)
{
	ADR::FlightRestrictionTimeSlice& tsrule(p->operator()(tm).as_flightrestriction());
	if (!tsrule.is_valid()) {
		add_resulttext("Rule: ?""?\n", m_txttagrule);
		return;
	}
	std::ostringstream oss;
	oss << "Rule: " << tsrule.get_ident() << ' ';
	switch (tsrule.get_type()) {
	case ADR::FlightRestrictionTimeSlice::type_mandatory:
		oss << "Mandatory";
		break;

	case ADR::FlightRestrictionTimeSlice::type_forbidden:
		oss << "Forbidden";
		break;

	case ADR::FlightRestrictionTimeSlice::type_closed:
		oss << "ClosedForCruising";
		break;

	case ADR::FlightRestrictionTimeSlice::type_allowed:
		oss << "Allowed";
		break;

	default:
		oss << '?';
		break;
	}
	if (tsrule.is_dct() || tsrule.is_unconditional() || tsrule.is_routestatic() || tsrule.is_mandatoryinbound() || !tsrule.is_enabled()) {
		oss << " (";
		if (!tsrule.is_enabled())
			oss << '-';
		if (tsrule.is_dct())
			oss << 'D';
		if (tsrule.is_unconditional())
			oss << 'U';
		if (tsrule.is_routestatic())
			oss << 'S';
		if (tsrule.is_mandatoryinbound())
			oss << 'I';
		oss << ')';
	}
	add_resulttext(oss.str() + "\n", m_txttagrule);
	add_resulttext("Desc: " + tsrule.get_instruction() + "\n", m_txttagdesc);
	//add_resulttext("OprGoal: \n", m_txttagoprgoal);
	if (tsrule.get_condition())
		add_resulttext("Cond: " + tsrule.get_condition()->to_str(m_reval.get_departuretime()) + "\n", m_txttagcond);
	for (unsigned int i = 0, n = tsrule.get_restrictions().size(); i < n; ++i) {
		const ADR::RestrictionSequence& rs(tsrule.get_restrictions()[i]);
		std::ostringstream oss;
		oss << "  Alternative " << i << ' ' << rs.to_str(m_reval.get_departuretime());
		add_resulttext(oss.str() + "\n", m_txttagrestrict);
	}
}

void CFMUValidate::MainWindow::validate_adr_clicked(void)
{
	if (m_textviewresults) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresults->get_buffer());
		if (buf)
			buf->set_text("");
	}
	if (!m_textviewfplan)
		return;
	Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfplan->get_buffer());
	if (!buf)
		return;
	const Glib::ustring& txt(buf->get_text());
	if (txt.empty())
		return;
	if (m_reval.get_rules().empty()) {
		// first open the databases
		m_adrdb.set_path(m_dir_aux.empty() ? Engine::get_default_aux_dir() : m_dir_aux);
		if (!m_topodb.is_open())
			m_topodb.open(m_dir_aux.empty() ? Engine::get_default_aux_dir() : m_dir_aux);
		m_reval.set_db(&m_adrdb);
		Glib::TimeVal tv;
		tv.assign_current_time();
		m_reval.load_rules();
		{
			Glib::TimeVal tv1;
			tv1.assign_current_time();
			tv = tv1 - tv;
		}
		std::ostringstream oss;
		oss << "Loaded " << m_reval.count_rules() << " Restrictions, "
		    << std::fixed << std::setprecision(3) << tv.as_double() << 's' << std::endl;
		add_resulttext(oss.str(), m_txttagmsginfo);
	}
	if (!txt.compare(0, 6, "trace?")) {
		print_adr_tracerules();
		return;
	}
	if (!txt.compare(0, 6, "trace:") || !txt.compare(0, 6, "trace+")) {
		Glib::ustring txt1(txt);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		txt1 = txt1.substr(6);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_trace(true);
		}
		print_adr_tracerules();
		return;
	}
	if (!txt.compare(0, 6, "trace-")) {
		Glib::ustring txt1(txt);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		txt1 = txt1.substr(6);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_trace(false);
		}
		print_adr_tracerules();
		return;
	}
	if (!txt.compare(0, 6, "trace*")) {
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			ts.set_trace(false);
		}
		print_adr_tracerules();
		return;
	}
	if (!txt.compare(0, 9, "disabled?")) {
		print_adr_disabledrules();
		return;
	}
	if (!txt.compare(0, 9, "disabled:") || !txt.compare(0, 9, "disabled+")) {
		Glib::ustring txt1(txt);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		txt1 = txt1.substr(9);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_enabled(false);
		}
		print_adr_disabledrules();
		return;
	}
	if (!txt.compare(0, 9, "disabled-")) {
		Glib::ustring txt1(txt);
		Glib::ustring::size_type n(txt1.find('\n'));
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		n = txt1.find(' ');
		if (n != Glib::ustring::npos)
			txt1.erase(n);
		txt1 = txt1.substr(9);
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			if (ts.get_ident() != txt1)
				continue;
			ts.set_enabled(true);
		}
		print_adr_disabledrules();
		return;
	}
	if (!txt.compare(0, 9, "disabled*")) {
		uint64_t tm(time(0));
		for (ADR::RestrictionEval::rules_t::const_iterator ri(m_reval.get_rules().begin()), re(m_reval.get_rules().end()); ri != re; ++ri) {
			const ADR::Object::ptr_t& p(*ri);
			ADR::FlightRestrictionTimeSlice& ts(p->operator()(tm).as_flightrestriction());
			if (!ts.is_valid())
				continue;
			ts.set_enabled(true);
		}
		print_adr_disabledrules();
		return;
	}

	if (!txt.compare(0, 4, "fpr?")) {
		add_resulttext(m_honourprofilerules ? "fpr+\n" : "fpr-\n", m_txttagmsginfo);
		return;
	}
	if (!txt.compare(0, 4, "fpr+")) {
		m_honourprofilerules = true;
		add_resulttext("fpr+\n", m_txttagmsginfo);
		return;
	}
	if (!txt.compare(0, 4, "fpr-")) {
		m_honourprofilerules = false;
		add_resulttext("fpr-\n", m_txttagmsginfo);
		return;
	}
	{
		ADR::FlightPlan adrfpl;
		ADR::FlightPlan::errors_t err(adrfpl.parse(m_adrdb, txt, true));
		if (!err.empty()) {
			std::string er("EFPL: ");
			bool subseq(false);
			for (ADR::FlightPlan::errors_t::const_iterator i(err.begin()), e(err.end()); i != e; ++i) {
				if (subseq)
					er += "; ";
				subseq = true;
				er += *i;
			}
			add_resulttext(er + "\n", m_txttagmsgwarn);
			return;
		}
		adrfpl.fix_invalid_altitudes(m_topodb, m_adrdb);
		m_reval.set_fplan(adrfpl);
		if (true) {
			std::ostringstream oss;
			oss << "I: FPL Parsed, " << adrfpl.size() << " waypoints";
			add_resulttext(oss.str() + "\n", m_txttagfpl);
		}
		// print Parsed Flight Plan
		if (false) {
			for (unsigned int i(0), n(adrfpl.size()); i < n; ++i) {
				const ADR::FPLWaypoint& wpt(adrfpl[i]);
				std::ostringstream oss;
				oss << "PFPL[" << i << "]: " << wpt.get_icao() << '/' << wpt.get_name() << ' ';
				switch (wpt.get_pathcode()) {
				case FPlanWaypoint::pathcode_sid:
				case FPlanWaypoint::pathcode_star:
				case FPlanWaypoint::pathcode_airway:
					oss << wpt.get_pathname();
					break;

				case FPlanWaypoint::pathcode_directto:
					oss << "DCT";
					break;

				default:
					oss << '-';
					break;
				}
				oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
				    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
				    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
				    << ' ' << wpt.get_fpl_altstr() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
				if (wpt.get_ptobj())
					oss << " ptobj " << wpt.get_ptobj()->get_uuid();
				if (wpt.get_pathobj())
					oss << " pathobj " << wpt.get_pathobj()->get_uuid();
				if (wpt.is_expanded())
					oss << " (E)";
				add_resulttext(oss.str() + "\n", m_txttagfpl);
			}
		}
	}
	bool res(m_reval.check_fplan(m_honourprofilerules));
	// print Flight Plan
	{
		ADR::FlightPlan f(m_reval.get_fplan());
 		{
			std::ostringstream oss;
			oss << "FR: " << f.get_aircrafttype() << ' ' << Aircraft::get_aircrafttypeclass(f.get_aircrafttype()) << ' '
			    << f.get_equipment() << " PBN/" << f.get_pbn_string() << ' ' << f.get_flighttype();
			add_resulttext(oss.str() + "\n", m_txttagfpl);
		}
		for (unsigned int i(0), n(f.size()); i < n; ++i) {
			const ADR::FPLWaypoint& wpt(f[i]);
			std::ostringstream oss;
			oss << "F[" << i << '/' << wpt.get_wptnr() << "]: " << wpt.get_ident() << ' ' << wpt.get_type_string() << ' ';
			switch (wpt.get_pathcode()) {
			case FPlanWaypoint::pathcode_sid:
			case FPlanWaypoint::pathcode_star:
			case FPlanWaypoint::pathcode_airway:
				oss << wpt.get_pathname();
				break;

			case FPlanWaypoint::pathcode_directto:
				oss << "DCT";
				break;

			default:
				oss << '-';
				break;
			}
			oss << ' ' << wpt.get_coord().get_lat_str2() << ' ' << wpt.get_coord().get_lon_str2()
			    << " D" << std::fixed << std::setprecision(1) << wpt.get_dist_nmi()
			    << " T" << std::fixed << std::setprecision(0) << wpt.get_truetrack_deg()
			    << ' ' << wpt.get_fpl_altstr()
			    << ' ' << wpt.get_icao() << '/' << wpt.get_name() << ' ' << (wpt.is_ifr() ? 'I' : 'V') << "FR";
			{
				time_t t(wpt.get_time_unix());
				struct tm tm;
				if (gmtime_r(&t, &tm)) {
					oss << ' ' << std::setw(2) << std::setfill('0') << tm.tm_hour
					    << ':' << std::setw(2) << std::setfill('0') << tm.tm_min;
					if (t >= 48 * 60 * 60)
						oss << ' ' << std::setw(4) << std::setfill('0') << (tm.tm_year + 1900)
						    << '-' << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
						    << '-' << std::setw(2) << std::setfill('0') << tm.tm_mday;
				}
			}
			if (wpt.get_ptobj())
				oss << " ptobj " << wpt.get_ptobj()->get_uuid();
			if (wpt.get_pathobj())
				oss << " pathobj " << wpt.get_pathobj()->get_uuid();
			if (wpt.is_expanded())
				oss << " (E)";
			add_resulttext(oss.str() + "\n", m_txttagfpl);
		}
	}
	for (ADR::RestrictionEval::messages_const_iterator mi(m_reval.messages_begin()), me(m_reval.messages_end()); mi != me; ++mi)
		add_resulttext(*mi);
	add_resulttext(res ? "OK\n" : "FAIL\n", m_txttagrule);
	for (ADR::RestrictionEval::results_const_iterator ri(m_reval.results_begin()), re(m_reval.results_end()); ri != re; ++ri)
		add_resulttext(*ri);
}


CFMUValidate::CFMUValidate(int& argc, char**& argv, Glib::OptionContext& option_context)
	: Gtk::Main(argc, argv, option_context), m_mainwindow(0)
{
	m_builder = load_glade_file("cfmuvalidate" UIEXT);
	m_builder->get_widget_derived("cfmuvalidatewindow", m_mainwindow);
	if (m_mainwindow)
		m_mainwindow->set_app(this);
#ifdef HAVE_WEBKIT1
	{
		guint nsigids(0);
		guint *sigids(g_signal_list_ids(WEBKIT_TYPE_DOM_HTML_ELEMENT, &nsigids));
		std::cerr << "DOM HTML Element signal list: " << nsigids << std::endl;
		for (guint i(0); i < nsigids; ++i) {
			GSignalQuery q;
			g_signal_query(sigids[i], &q);
			std::cerr << "  " << q.signal_name << std::endl;
		}
	}
#endif
}

CFMUValidate::~CFMUValidate()
{
	delete m_mainwindow;
}

void CFMUValidate::run(void)
{
	if (m_mainwindow)
		Gtk::Main::run(*m_mainwindow);
}

void CFMUValidate::set_dbdirs(const std::string& dir_main, Engine::auxdb_mode_t auxdbmode, const std::string& dir_aux)
{
	if (!m_mainwindow)
		return;
	m_mainwindow->set_dbdirs(dir_main, auxdbmode, dir_aux);
}

int main(int argc, char *argv[])
{
	try {
		std::string dir_main(""), dir_aux("");
		Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs;
		bool dis_aux(false);
		Glib::init();
		Glib::OptionGroup optgroup("cfmuvalidate", "CFMU Validate Application Options", "Options controlling the CFMU Validate Application");
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
		CFMUValidate app(argc, argv, optctx);
		if (dis_aux)
			auxdbmode = Engine::auxdb_none;
		else if (!dir_aux.empty())
			auxdbmode = Engine::auxdb_override;
		Glib::set_application_name("CFMU Validate");
		Glib::thread_init();
 		app.set_dbdirs(dir_main, auxdbmode, dir_aux);
		app.run();
		return EX_OK;
	} catch (const Glib::FileError& ex) {
		std::cerr << "FileError: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const Gtk::BuilderError& ex) {
		std::cerr << "BuilderError: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		return EX_DATAERR;
	} catch (const Glib::Exception& ex) {
		std::cerr << "Glib Error: " << ex.what() << std::endl;
		return EX_DATAERR;
	}
	return EX_DATAERR;
}
