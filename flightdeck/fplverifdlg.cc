//
// C++ Implementation: FlightDeckWindow
//
// Description: Flight Deck Windows, Altitude Dialog
//
//
// Author: Thomas Sailer <t.sailer@alumni.ethz.ch>, (C) 2011, 2012, 2013
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "sysdeps.h"
#include "sysdepsgui.h"
#include <iomanip>

#include "flightdeckwindow.h"
#include "icaofpl.h"

FlightDeckWindow::FPLVerificationDialog::FPLVerificationDialog(BaseObjectType *castitem, const Glib::RefPtr<Builder>& refxml)
	: Gtk::Window(castitem), m_buttondone(0), m_buttonvalidate(0), m_buttonvalidateeurofpl(0), m_buttonvalidatelocal(0),
	  m_buttonfixup(0), m_comboboxawymode(0), m_textviewfpl(0), m_textviewresult(0),
	  m_sensors(0), m_childrun(false), m_clearresulttext(false)
{
#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        signal_delete_event().connect(sigc::mem_fun(*this, &FPLVerificationDialog::on_delete_event));
        signal_show().connect(sigc::mem_fun(*this, &FPLVerificationDialog::on_show));
        signal_hide().connect(sigc::mem_fun(*this, &FPLVerificationDialog::on_hide));
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
	get_widget(refxml, "fplverifbuttondone", m_buttondone);
	get_widget(refxml, "fplverifbuttonvalidate", m_buttonvalidate);
	get_widget(refxml, "fplverifbuttonvalidateeurofpl", m_buttonvalidateeurofpl);
	get_widget(refxml, "fplverifbuttonvalidatelocal", m_buttonvalidatelocal);
	get_widget(refxml, "fplverifbuttonfixup", m_buttonfixup);
	get_widget(refxml, "fplveriftextviewfpl", m_textviewfpl);
	get_widget(refxml, "fplveriftextviewresult", m_textviewresult);
	get_widget(refxml, "fplverifcomboboxawymode", m_comboboxawymode);

	if (m_buttondone)
		m_buttondone->signal_clicked().connect(sigc::mem_fun(*this, &FPLVerificationDialog::done_clicked));
	if (m_buttonvalidate)
		m_buttonvalidate->signal_clicked().connect(sigc::mem_fun(*this, &FPLVerificationDialog::validate_clicked));
	if (m_buttonvalidateeurofpl)
		m_buttonvalidateeurofpl->signal_clicked().connect(sigc::mem_fun(*this, &FPLVerificationDialog::validateeurofpl_clicked));
	if (m_buttonvalidatelocal)
		m_buttonvalidatelocal->signal_clicked().connect(sigc::mem_fun(*this, &FPLVerificationDialog::validatelocal_clicked));
	if (m_buttonfixup)
		m_buttonfixup->signal_clicked().connect(sigc::mem_fun(*this, &FPLVerificationDialog::fixup_clicked));
	if (m_comboboxawymode)
		m_comboboxawymode->signal_changed().connect(sigc::mem_fun(*this, &FPLVerificationDialog::awycollapsed_changed));
	if (m_textviewfpl) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfpl->get_buffer());
                if (buf)
			buf->signal_changed().connect(sigc::mem_fun(*this, &FPLVerificationDialog::fpl_changed));
	} 
}

FlightDeckWindow::FPLVerificationDialog::~FPLVerificationDialog()
{
	child_close();
}

void FlightDeckWindow::FPLVerificationDialog::on_show(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::Window::on_show();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

void FlightDeckWindow::FPLVerificationDialog::on_hide(void)
{
#ifdef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
        Gtk::Window::on_hide();
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
}

bool FlightDeckWindow::FPLVerificationDialog::on_delete_event(GdkEventAny* event)
{
	child_close();
        hide();
	m_sensors = 0;
        return true;
}

void FlightDeckWindow::FPLVerificationDialog::done_clicked(void)
{
	child_close();
	hide();
	m_sensors = 0;
}

void FlightDeckWindow::FPLVerificationDialog::validate_clicked(void)
{
	if (m_textviewresult) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
                if (buf)
                        buf->set_text("");
	}
	child_run();
	if (!child_is_running())
		return;
	if (m_textviewfpl) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfpl->get_buffer());
                if (buf)
			child_send("validate*:cfmu\n" + buf->get_text() + "\n");
	}
}

void FlightDeckWindow::FPLVerificationDialog::validateeurofpl_clicked(void)
{
	if (m_textviewresult) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
                if (buf)
                        buf->set_text("");
	}
	child_run();
	if (!child_is_running())
		return;
	if (m_textviewfpl) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfpl->get_buffer());
                if (buf)
			child_send("validate*:eurofpl\n" + buf->get_text() + "\n");
	}
}

void FlightDeckWindow::FPLVerificationDialog::validatelocal_clicked(void)
{
	if (m_textviewresult) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
                if (buf)
                        buf->set_text("");
	}
	child_run();
	if (!child_is_running())
		return;
	if (m_textviewfpl) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfpl->get_buffer());
                if (buf)
			child_send("validate*:local\n" + buf->get_text() + "\n");
	}
}

void FlightDeckWindow::FPLVerificationDialog::fixup_clicked(void)
{
	if (!m_sensors)
		return;
	FPlanRoute& route(m_sensors->get_route());
	bool chg(false);
	{
		IcaoFlightPlan fpl(*m_sensors->get_engine());
		chg = fpl.mark_vfrroute_begin(route) || chg;
		chg = fpl.mark_vfrroute_end(route) || chg;
	}
	if (!chg)
		chg = route.enforce_pathcode_vfrdeparr() || chg;
	chg = route.enforce_pathcode_vfrifr() || chg;
	if (chg)
		m_sensors->nav_fplan_modified();
}

bool FlightDeckWindow::FPLVerificationDialog::child_is_running(void) const
{
	return m_childrun || m_childsock;
}

void FlightDeckWindow::FPLVerificationDialog::child_run(void)
{
	if (child_is_running())
		return;
	if (m_textviewresult) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
		if (buf) {
			buf->set_text("Connecting to validation server...");
			m_clearresulttext = true;
		}
	}
	try {
#ifdef G_OS_UNIX
		Glib::RefPtr<Gio::UnixSocketAddress> childsockaddr(Gio::UnixSocketAddress::create(PACKAGE_RUN_DIR "/validator/socket", Gio::UNIX_SOCKET_ADDRESS_PATH));
#else
		Glib::RefPtr<Gio::InetSocketAddress> childsockaddr(Gio::InetSocketAddress::create(Gio::InetAddress::create_loopback(Gio::SOCKET_FAMILY_IPV4), 53181));
#endif
		m_childsock = Gio::Socket::create(childsockaddr->get_family(), Gio::SOCKET_TYPE_STREAM, Gio::SOCKET_PROTOCOL_DEFAULT);
		m_childsock->connect(childsockaddr);
	} catch (...) {
		m_childsock.reset();
	}
	if (m_childsock) {
		m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &FPLVerificationDialog::child_socket_handler), m_childsock->get_fd(), Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
		return;
	}
	int childstdin(-1), childstdout(-1);
	try {
		std::vector<std::string> argv, env;
#ifdef __MINGW32__
		std::string workdir;
		{
			gchar *instdir(g_win32_get_package_installation_directory_of_module(0));
			if (instdir) {
				workdir = instdir;
				argv.push_back(Glib::build_filename(instdir, "bin", "cfmuvalidateserver.exe"));
			} else {
				argv.push_back(Glib::build_filename(PACKAGE_PREFIX_DIR "/bin", "cfmuvalidateserver.exe"));
				workdir = PACKAGE_PREFIX_DIR;
			}
		}
#else
		std::string workdir(PACKAGE_DATA_DIR);
		argv.push_back(Glib::build_filename(PACKAGE_LIBEXEC_DIR, "cfmuvalidateserver"));
#endif
		env.push_back("PATH=/bin:/usr/bin");
		{
			bool found(false);
			std::string disp(Glib::getenv("DISPLAY", found));
			if (found)
				env.push_back("DISPLAY=" + disp);
		}
#ifdef __MINGW32__
                // inherit environment; otherwise, this may fail on WindowsXP with STATUS_SXS_ASSEMBLY_NOT_FOUND (C0150004)
                Glib::spawn_async_with_pipes(workdir, argv,
                                             Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
                                             sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#else
		Glib::spawn_async_with_pipes(workdir, argv, env,
					     Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_STDERR_TO_DEV_NULL,
					     sigc::slot<void>(), &m_childpid, &childstdin, &childstdout, 0);
#endif
		m_childrun = true;
	} catch (Glib::SpawnError& e) {
		m_childrun = false;
		if (m_textviewresult) {
			Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
			if (buf)
				buf->set_text("Cannot run validator: " + e.what());
		}
		return;
	}
	m_connchildwatch = Glib::signal_child_watch().connect(sigc::mem_fun(this, &FPLVerificationDialog::child_watch), m_childpid);
	m_childchanstdin = Glib::IOChannel::create_from_fd(childstdin);
	m_childchanstdout = Glib::IOChannel::create_from_fd(childstdout);
	m_childchanstdin->set_encoding();
	m_childchanstdout->set_encoding();
	m_childchanstdin->set_close_on_unref(true);
	m_childchanstdout->set_close_on_unref(true);
	m_connchildstdout = Glib::signal_io().connect(sigc::mem_fun(this, &FPLVerificationDialog::child_stdout_handler), m_childchanstdout,
						      Glib::IO_IN | Glib::IO_ERR | Glib::IO_HUP);
}

bool FlightDeckWindow::FPLVerificationDialog::child_stdout_handler(Glib::IOCondition iocond)
{
        Glib::ustring line;
        Glib::IOStatus iostat(m_childchanstdout->read_line(line));
        if (iostat == Glib::IO_STATUS_ERROR ||
            iostat == Glib::IO_STATUS_EOF) {
                child_close();
                return true;
        }
	if (m_textviewresult) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
		if (buf) {
			if (m_clearresulttext) {
				buf->set_text(line);
				m_clearresulttext = false;
			} else {
				buf->insert(buf->end(), line);
			}
		}
	}
        return true;
}

bool FlightDeckWindow::FPLVerificationDialog::child_socket_handler(Glib::IOCondition iocond)
{
	char buf[4096];
	gssize r(m_childsock->receive_with_blocking(buf, sizeof(buf), false));
	if (r < 0) {
		child_close();
		return true;
	}
	if (!r)
		return true;
	std::string txt(buf, buf + r);
	if (m_textviewresult) {
		Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
		if (buf) {
			if (m_clearresulttext) {
				buf->set_text(txt);
				m_clearresulttext = false;
			} else {
				buf->insert(buf->end(), txt);
			}
		}
	}
        return true;
}

void FlightDeckWindow::FPLVerificationDialog::child_watch(GPid pid, int child_status)
{
	if (!m_childrun)
		return;
	if (m_childpid != pid) {
		Glib::spawn_close_pid(pid);
		return;
	}
	Glib::spawn_close_pid(pid);
	m_childrun = false;
	m_connchildwatch.disconnect();
	m_connchildstdout.disconnect();
	child_close();
}

void FlightDeckWindow::FPLVerificationDialog::child_close(void)
{
	m_connchildstdout.disconnect();
	if (m_childsock) {
		m_childsock->close();
		m_childsock.reset();
	}
	if (m_childchanstdin) {
		m_childchanstdin->close();
		m_childchanstdin.reset();
	}
	if (m_childchanstdout) {
		m_childchanstdout->close();
		m_childchanstdout.reset();
	}
	if (!m_childrun)
		return;
	m_childrun = false;
	m_connchildwatch.disconnect();
	Glib::signal_child_watch().connect(sigc::hide(sigc::ptr_fun(&Glib::spawn_close_pid)), m_childpid);
}

void FlightDeckWindow::FPLVerificationDialog::child_send(const std::string& tx)
{
	if (m_childsock) {
		gssize r(m_childsock->send_with_blocking(const_cast<char *>(tx.c_str()), tx.size(), true));
		if (r == -1) {
			child_close();
		}
	} else if (m_childchanstdin) {
		m_childchanstdin->write(tx);
		m_childchanstdin->flush();
	}
}

void FlightDeckWindow::FPLVerificationDialog::fpl_changed(void)
{
	if (m_textviewresult) {
                Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewresult->get_buffer());
                if (buf)
                        buf->set_text("");
	}
}

void FlightDeckWindow::FPLVerificationDialog::awycollapsed_changed(void)
{
	sensors_change(Sensors::change_all);
}

void FlightDeckWindow::FPLVerificationDialog::open(Sensors *sensors)
{
	m_sensors = sensors;
	if (!m_sensors) {
		hide();
		return;
	}
	show();
	sensors_change(Sensors::change_all);
}

void FlightDeckWindow::FPLVerificationDialog::sensors_change(Sensors::change_t changemask)
{
	if (!m_sensors)
		return;
       if (changemask & Sensors::change_fplan) {
	       {
		       FPlanRoute route(m_sensors->get_route());
		       bool chg(route.enforce_pathcode_vfrdeparr());
		       chg = chg || route.enforce_pathcode_vfrifr();
		       if (m_buttonfixup)
			       m_buttonfixup->set_visible(chg);
	       }
	       if (m_textviewfpl) {
		       Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfpl->get_buffer());
		       if (buf)
			       buf->set_text("Please wait...");
	       }
	       IcaoFlightPlan fpl(*m_sensors->get_engine());
	       IcaoFlightPlan::awymode_t awymode(IcaoFlightPlan::awymode_keep);
	       if (m_comboboxawymode) {
		       switch (m_comboboxawymode->get_active_row_number()) {
		       default:
		       case 0:
			       awymode = IcaoFlightPlan::awymode_keep;
			       break;

		       case 1:
			       awymode = IcaoFlightPlan::awymode_collapse;
			       break;

		       case 2:
			       awymode = IcaoFlightPlan::awymode_collapse_all;
			       break;
		       } 
	       }
	       fpl.populate(m_sensors->get_route(), awymode, std::numeric_limits<double>::max());
	       fpl.set_aircraft(m_sensors->get_aircraft(), m_sensors->get_acft_cruise_params());
	       fpl.set_personsonboard(0);
	       {
		       time_t dept(fpl.get_departuretime());
		       if (dept < time(0)) {
			       struct tm tm;
			       if (gmtime_r(&dept, &tm))
				       fpl.set_departuretime(((tm.tm_hour * 60) + tm.tm_min) * 60 + tm.tm_sec);
		       }
	       }
	       {
		       Glib::ustring pic(Glib::get_real_name());
		       if (!pic.empty())
			       fpl.set_picname(pic.uppercase());
	       }
	       if (m_textviewfpl) {
		       Glib::RefPtr<Gtk::TextBuffer> buf(m_textviewfpl->get_buffer());
		       if (buf)
			       buf->set_text(fpl.get_fpl());
	       }
       }
}
