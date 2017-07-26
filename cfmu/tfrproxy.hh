#ifndef TFRPROXY_H
#define TFRPROXY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysdeps.h"
#include "fplan.h"
#include "dbobj.h"

#include <glibmm.h>
#include <sigc++/sigc++.h>

class FPlanCheckerProxy : public sigc::trackable {
public:
	typedef enum {
		proxystate_stopped,
		proxystate_started,
		proxystate_error
	} proxystate_t;

	class Command {
	public:
		Command(void);
		Command(const std::string& cmd);

		const std::string& get_cmdname(void) const { return m_cmdname; }
		void set_cmdname(const std::string cmdname) { m_cmdname = cmdname; }

		std::string get_cmdstring(void) const;

		typedef std::vector<uint8_t> bytearray_t;
		typedef std::vector<int> intarray_t;
		typedef std::vector<std::string> stringarray_t;
		typedef std::vector<unsigned long long> ulongarray_t;

		void unset_option(const std::string& name, bool require = false);
		void set_option(const std::string& name, const std::string& value, bool replace = false);
		void set_option(const std::string& name, const Glib::ustring& value, bool replace = false);
		void set_option(const std::string& name, const bytearray_t& value, bool replace = false);
		template<typename T> void set_option(const std::string& name, T value, bool replace = false);
		template<typename T> void set_option(const std::string& name, T b, T e, bool replace = false);
		void set_option(const std::string& name, proxystate_t pss, bool replace = false);
		void set_option(const std::string& name, const Point& value, bool replace = false);

		std::pair<const std::string&,bool> get_option(const std::string& name) const;
		bool is_option(const std::string& name) const { return get_option(name).second; }
		std::pair<long,bool> get_option_int(const std::string& name) const;
		std::pair<unsigned long,bool> get_option_uint(const std::string& name) const;
		std::pair<long long,bool> get_option_long(const std::string& name) const;
		std::pair<unsigned long long,bool> get_option_ulong(const std::string& name) const;
		std::pair<double,bool> get_option_float(const std::string& name) const;
		std::pair<bytearray_t,bool> get_option_bytearray(const std::string& name) const;
		std::pair<intarray_t,bool> get_option_intarray(const std::string& name) const;
		std::pair<ulongarray_t,bool> get_option_ulongarray(const std::string& name) const;
		std::pair<stringarray_t,bool> get_option_stringarray(const std::string& name) const;
		std::pair<proxystate_t,bool> get_option_proxystate(const std::string& name) const;
		std::pair<Point,bool> get_option_coord(const std::string& name) const;

		static void error(const std::string& text);

		bool empty(void) const { return m_opts.empty(); }

	protected:
		static const std::string empty_string;
		std::string m_cmdname;
		typedef std::map<std::string,std::string> opts_t;
		opts_t m_opts;

		static bool is_str_needs_quotes(const std::string& s, bool nocomma = false);
		static std::string quote_str(const std::string& s);
		static bool unquote_str(std::string& res, const std::string& s, std::string::size_type& pos, bool nocomma = false);
	};
 
	FPlanCheckerProxy();
	~FPlanCheckerProxy();
	void start_checker(void) { child_run(); }
	void stop_checker(void) { child_close(); }
	bool is_checker_running(void) const { return m_childrun; }

	void clear_fplan(void);
	FPlanRoute& get_fplan(void) { return m_route; }
	const FPlanRoute& get_fplan(void) const { return m_route; }

	void sendcmd(const Command& cmd);
	sigc::signal<void,const Command&>& signal_recvcmd(void) { return m_signal_recvcmd; }

protected:
	FPlanRoute m_route;
	sigc::signal<void,const Command&> m_signal_recvcmd;
	sigc::connection m_connchildwatch;
	sigc::connection m_connchildstdout;
	Glib::RefPtr<Glib::IOChannel> m_childchanstdin;
	Glib::RefPtr<Glib::IOChannel> m_childchanstdout;
	Glib::Pid m_childpid;
	bool m_childrun;

	void child_watch(GPid pid, int child_status);
	void child_run(void);
	void child_close(void);
	bool child_stdout_handler(Glib::IOCondition iocond);
	void recvcmd(const Command& cmd);
};

extern const Glib::ustring& to_str(FPlanCheckerProxy::proxystate_t ps);

#endif /* TFRPROXY_H */
