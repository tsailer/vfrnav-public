/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014, 2016 by Thomas Sailer                 *
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

#include <iostream>
#include <fstream>
#include <glibmm.h>
#include <glibmm/datetime.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "simulator.h"

static Simulator *simptr = 0;
static Glib::RefPtr<Glib::MainLoop> main_loop;

#ifndef WIN32

static int replay(const std::string& replayfile, const std::string& file = "", bool dounlink = true)
{
	int slave(-1), master(-1);
        char ttyname[32] = { 0, };
#ifdef HAVE_PTY_H
        if (openpty(&master, &slave, ttyname, 0, 0)) {
		std::ostringstream oss;
		oss << "openpty: cannot create terminal: " << strerror(errno);
                throw std::runtime_error(oss.str());
	}
#else
	throw std::runtime_error("win32: cannot create terminal");
#endif
        /* set mode to raw */
#ifdef HAVE_TERMIOS_H
        struct termios tm;
        memset(&tm, 0, sizeof(tm));
        tm.c_cflag = CS8 | CREAD | CLOCAL;
        if (tcsetattr(master, TCSANOW, &tm))
		std::cerr << "cannot set master terminal attributes: " << strerror(errno) << std::endl;
	memset(&tm, 0, sizeof(tm));
        tm.c_cflag = CS8 | CREAD | CLOCAL;
        if (tcsetattr(slave, TCSANOW, &tm))
		std::cerr << "cannot set slave terminal attributes: " << strerror(errno) << std::endl;
        fchmod(slave, 0600);
	if (!file.empty()) {
		if (dounlink)
			unlink(file.c_str());
		dounlink = true;
		if (symlink(ttyname, file.c_str())) {
			std::cerr << "symlink error: " << ttyname << " -> " << file << std::endl;
			dounlink = false;
		}
	} else {
		dounlink = false;
	}
	close(slave);
	//fcntl(master, F_SETFL, fcntl(master, F_GETFL, 0) | O_NONBLOCK);
#endif
	int fd(open(replayfile.c_str(), O_RDONLY));
	if (fd == -1) {
		std::cerr << "cannot open replay file " << replayfile << ": " << strerror(errno) << std::endl;
		if (dounlink)
			unlink(file.c_str());
		return 1;
	}
	for (;;) {
		char c;
		int r(read(fd, &c, 1));
		if (r <= 0)
			break;
		write(master, &c, 1);
		if (true) {
			if (c == 2)
				std::cout << "<STX>";
			else if (c == 3)
				std::cout << "<ETX>";
			else if (c == 13)
				std::cout;
			else if (c == 10)
				std::cout << std::endl;
			else if (c >= 32 && c < 127)
				std::cout << c;
			else
				std::cout << '.';
		}
		if (c == 3)
			sleep(1);
	}
	if (dounlink)
		unlink(file.c_str());
	return 0;
}
#endif

static void line_input(char *line)
{
	if (!line)
		return;
	if (simptr && *line) {
		add_history(line);
		if (simptr->command(line))
			main_loop->quit();
	}
	free(line);
}

int main(int argc, char *argv[])
{
        try {
                std::string dir_main(""), dir_aux(""), devname("");
                Engine::auxdb_mode_t auxdbmode = Engine::auxdb_prefs;
                bool dis_aux(false), devunlink(true), trimble(false);
                Glib::init();
                Glib::OptionGroup optgroup("kingsim", "King Simulator Options", "Options controlling the King GPS Protocol Simulator");
                Glib::OptionEntry optmaindir, optauxdir, optdisableaux, optdevname, optdevunlink, opttrimble;
#ifndef WIN32
 		std::string replayfile("");
		Glib::OptionEntry optreplay;
#endif
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
		optdevname.set_short_name('d');
                optdevname.set_long_name("devname");
                optdevname.set_description("Device name");
                optdevname.set_arg_description("FILE");
                optdevunlink.set_short_name('U');
                optdevunlink.set_long_name("unlinkdev");
                optdevunlink.set_description("Unlink Device Name");
                optdevunlink.set_arg_description("BOOL");
		opttrimble.set_short_name('T');
		opttrimble.set_long_name("trimble");
                opttrimble.set_description("Trimble Approach2000 Protocol");
                opttrimble.set_arg_description("BOOL");
#ifndef WIN32
		optreplay.set_short_name('r');
                optreplay.set_long_name("replay");
                optreplay.set_description("Replay File Name");
                optreplay.set_arg_description("FILE");
#endif
		optgroup.add_entry_filename(optmaindir, dir_main);
                optgroup.add_entry_filename(optauxdir, dir_aux);
                optgroup.add_entry(optdisableaux, dis_aux);
                optgroup.add_entry_filename(optdevname, devname);
                optgroup.add_entry(optdevunlink, devunlink);
                optgroup.add_entry(opttrimble, trimble);
#ifndef WIN32
		optgroup.add_entry_filename(optreplay, replayfile);
		Glib::OptionContext optctx("[--maindir=<dir>] [--auxdir=<dir>] [--disableaux] [--devname=<file>] [--devunlink] [--trimble] [--replay=<file>]");
#else
		Glib::OptionContext optctx("[--maindir=<dir>] [--auxdir=<dir>] [--disableaux] [--devname=<file>] [--devunlink] [--trimble]");
#endif
                optctx.set_help_enabled(true);
                optctx.set_ignore_unknown_options(false);
                optctx.set_main_group(optgroup);
		if (!optctx.parse(argc, argv)) {
			std::cerr << "Command Line Option Error: Usage: " << Glib::get_prgname() << std::endl
				  << optctx.get_help(false);
			return 1;
		}
                if (dis_aux)
                        auxdbmode = Engine::auxdb_none;
                else if (!dir_aux.empty())
                        auxdbmode = Engine::auxdb_override;
                Glib::set_application_name("Flight Deck");
                Glib::thread_init();
#ifndef WIN32
		if (!replayfile.empty())
			return replay(replayfile, devname, devunlink);
#endif		    
                Engine engine(dir_main, auxdbmode, dir_aux, false);
		Simulator sim(engine, devname, devunlink, trimble ? Simulator::protocol_trimble : Simulator::protocol_garmin);
#if 0
		for (;;) {
			char *txt(readline("GPS>"));
			if (!txt)
				break;
			if (*txt)
				add_history(txt);
			std::string txt1(txt);
			free(txt);
			if (sim.command(txt1))
				break;
		}
#else
		main_loop = Glib::MainLoop::create();
		simptr = &sim;
		rl_callback_handler_install("GPS>", &line_input);
		sigc::connection conn = Glib::signal_io().connect(sigc::bind_return(sigc::hide(sigc::ptr_fun(&rl_callback_read_char)), true), 0, Glib::IO_IN);
		main_loop->run();
		conn.disconnect();
		rl_callback_handler_remove();
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
