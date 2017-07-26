#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxml++/libxml++.h>
#include <iomanip>

#include "tfr.hh"

class TrafficFlowRestrictions::Loader : public xmlpp::SaxParser {
public:
	Loader(TrafficFlowRestrictions& tfrs, AirportsDb& airportdb, NavaidsDb& navaiddb,
	       WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb);
	virtual ~Loader();

	void error(const std::string& text) const;
	void error(const std::string& child, const std::string& text) const;
	void warning(const std::string& text) const;
	void warning(const std::string& child, const std::string& text) const;
	void info(const std::string& text) const;
	void info(const std::string& child, const std::string& text) const;
	void message(Message::type_t mt, const std::string& child, const std::string& text) const;

	TrafficFlowRestrictions& get_tfrs(void) const { return m_dbldr.get_tfrs(); }
	AirportsDb& get_airportdb(void) const { return m_dbldr.get_airportdb(); }
	NavaidsDb& get_navaiddb(void) const { return m_dbldr.get_navaiddb(); }
	WaypointsDb& get_waypointdb(void) const { return m_dbldr.get_waypointdb(); }
	AirwaysDb& get_airwaydb(void) const { return m_dbldr.get_airwaydb(); }
	AirspacesDb& get_airspacedb(void) const { return m_dbldr.get_airspacedb(); }

	TFRAirspace::const_ptr_t find_airspace(const std::string& id, const std::string& type) { return m_dbldr.find_airspace(id, type); }
	TFRAirspace::const_ptr_t find_airspace(const std::string& id, char bdryclass, uint8_t typecode = AirspacesDb::Airspace::typecode_ead) { return m_dbldr.find_airspace(id, bdryclass, typecode); }
	const AirportsDb::Airport& find_airport(const std::string& icao) { return m_dbldr.find_airport(icao); }
	void fill_airport_cache(void) { m_dbldr.fill_airport_cache(); }

	void disable_rule(const std::string& ruleid);
	void disable_rule(const TrafficFlowRule::const_ptr_t& p) { if (p) disable_rule(p->get_codeid()); }

protected:
        virtual void on_start_document(void);
        virtual void on_end_document(void);
        virtual void on_start_element(const Glib::ustring& name, const AttributeList& properties);
        virtual void on_end_element(const Glib::ustring& name);
        virtual void on_characters(const Glib::ustring& characters);
        virtual void on_comment(const Glib::ustring& text);
        virtual void on_warning(const Glib::ustring& text);
        virtual void on_error(const Glib::ustring& text);
        virtual void on_fatal_error(const Glib::ustring& text);

	class PNodeIgnore;
	class PNodeText;
	class PNodeRteUid;
	class PNodeAhpUid;
	class PNodeAseUid;
	class PNodeSidUid;
	class PNodeSiaUid;
	class PNodeTfrUid;
	class PNodeTimsh;
	class PNodeTft;
	class PNodeFcl;
	class PNodeTfl;
	class PNodePoint;
	class PNodeDct;
	class PNodeRpn;
	class PNodeTfe;
	class PNodeTre;
	class PNodeDfl;
	class PNodeAbc;
	class PNodeAcs;
	class PNodeFcs;
	class PNodeFcc;
	class PNodeFce;
	class PNodeTfr;

	class PNode {
	public:
		typedef Glib::RefPtr<PNode> ptr_t;
		typedef Glib::RefPtr<const PNode> const_ptr_t;

		PNode(Loader& loader, unsigned int childnum);
		virtual ~PNode();

		void reference(void) const;
		void unreference(void) const;

		virtual void on_characters(const Glib::ustring& characters);
		virtual void on_subelement(const ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeIgnore>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeRteUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeAhpUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeAseUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeSidUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeSiaUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTfrUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTimsh>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTft>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeFcl>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTfl>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodePoint>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeDct>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeRpn>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTfe>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTre>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeDfl>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeAbc>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeAcs>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeFcs>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeFcc>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeFce>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTfr>& el);
		virtual const std::string& get_element_name(void) const = 0;

		virtual bool is_ignore(void) const { return false; }

		void error(const std::string& text) const;
		void error(const_ptr_t child, const std::string& text) const;
		void warning(const std::string& text) const;
		void warning(const_ptr_t child, const std::string& text) const;
		void info(const std::string& text) const;
		void info(const_ptr_t child, const std::string& text) const;

		unsigned int get_childnum(void) const { return m_childnum; }
		unsigned int get_childcount(void) const { return m_childcount; }

	protected:
		Loader& m_loader;
		mutable gint m_refcount;
		unsigned int m_childnum;
		unsigned int m_childcount;
	};

	class PNodeIgnore : public PNode {
	public:
		typedef Glib::RefPtr<PNodeIgnore> ptr_t;

		PNodeIgnore(Loader& loader, unsigned int childnum, const std::string& name);

		virtual void on_characters(const Glib::ustring& characters) {}
		virtual void on_subelement(const PNode::ptr_t& el) {}
		virtual const std::string& get_element_name(void) const { return m_elementname; }
		virtual bool is_ignore(void) const { return true; }

	protected:
		std::string m_elementname;
	};

	class PNodeNoIgnore : public PNode {
	public:
		PNodeNoIgnore(Loader& loader, unsigned int childnum) : PNode(loader, childnum) {}

		virtual void on_subelement(const Glib::RefPtr<PNodeIgnore>& el);
	};

	class PNodeText : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeText> ptr_t;

		typedef enum {
			tt_first,
			tt_codeid = tt_first,
			tt_codetype,
			tt_txtoprgoal,
			tt_txtdescr,
			tt_codeworkhr,
			tt_timewef,
			tt_timetil,
			tt_datevalidwef,
			tt_datevalidtil,
			tt_codeday,
			tt_codetimeref,
			tt_codeeventwef,
			tt_codeeventtil,
			tt_timereleventwef,
			tt_timereleventtil,
			tt_valdistverlower,
			tt_uomdistverlower,
			tt_codedistverlower,
			tt_valdistverupper,
			tt_uomdistverupper,
			tt_codedistverupper,
			tt_geolat,
			tt_geolong,
			tt_txtdesig,
			tt_txtlocdesig,
			tt_valexceedlen,
			tt_uomlen,
			tt_codetypeengine,
			tt_codeengineno,
			tt_codeicaoacfttype,
			tt_codecapability,
			tt_codepurpose,
			tt_codemil,
			tt_constant,
			tt_invalid
		} texttype_t;

		PNodeText(Loader& loader, unsigned int childnum, const std::string& name);
		PNodeText(Loader& loader, unsigned int childnum, texttype_t tt = tt_invalid);

		virtual void on_characters(const Glib::ustring& characters);

		virtual const std::string& get_element_name(void) const { return get_static_element_name(m_texttype); }
		static const std::string& get_static_element_name(texttype_t tt);
		static texttype_t find_type(const std::string& name);
		texttype_t get_type(void) const { return m_texttype; }
		const std::string& get_text(void) const { return m_text; }
		long get_long(void) const;
		unsigned long get_ulong(void) const;
		double get_double(void) const;
		Point::coord_t get_lat(void) const;
		Point::coord_t get_lon(void) const;

	protected:
		std::string m_text;
		texttype_t m_texttype;
	};

	class PNodeRteUid : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeRteUid> ptr_t;

		PNodeRteUid(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_ident(void) const { return m_ident; }

	protected:
		static const std::string elementname;
		std::string m_ident;
	};

	class PNodeAhpUid : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeAhpUid> ptr_t;

		PNodeAhpUid(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_ident(void) const { return m_ident; }

	protected:
		static const std::string elementname;
		std::string m_ident;
	};

	class PNodeAseUid : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeAseUid> ptr_t;

		typedef enum {
			asetype_first,
			asetype_normal = asetype_first,
			asetype_from,
			asetype_into,
			asetype_invalid
		} asetype_t;

		PNodeAseUid(Loader& loader, unsigned int childnum, const std::string& name);
		PNodeAseUid(Loader& loader, unsigned int childnum, asetype_t t = asetype_invalid);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return get_static_element_name(get_asetype()); }
		static const std::string& get_static_element_name(asetype_t t);
		static asetype_t find_type(const std::string& name);
		asetype_t get_asetype(void) const { return m_asetype; }

		const std::string& get_ident(void) const { return m_ident; }
		const std::string& get_type(void) const { return m_type; }

	protected:
		std::string m_ident;
		std::string m_type;
		asetype_t m_asetype;
	};

	class PNodeSidUid : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeSidUid> ptr_t;

		PNodeSidUid(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual void on_subelement(const PNodeAhpUid::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_ident(void) const { return m_ident; }
		const std::string& get_airport(void) const { return m_airport; }

	protected:
		static const std::string elementname;
		std::string m_ident;
		std::string m_airport;
	};

	class PNodeSiaUid : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeSiaUid> ptr_t;

		PNodeSiaUid(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual void on_subelement(const PNodeAhpUid::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_ident(void) const { return m_ident; }
		const std::string& get_airport(void) const { return m_airport; }

	protected:
		static const std::string elementname;
		std::string m_ident;
		std::string m_airport;
	};

	class PNodeUid : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeUid> ptr_t;

		PNodeUid(Loader& loader, unsigned int childnum, const AttributeList& properties);
		
		virtual void on_subelement(const PNode::ptr_t& el);

		const std::string& get_codeid(void) const { return m_codeid; }
		uint64_t get_mid(void) const { return m_mid; }

	protected:
		std::string m_codeid;
		uint64_t m_mid;
	};

	class PNodeTfrUid : public PNodeUid {
	public:
		typedef Glib::RefPtr<PNodeTfrUid> ptr_t;
		PNodeTfrUid(Loader& loader, unsigned int childnum, const AttributeList& properties) : PNodeUid(loader, childnum, properties) {}
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }
	protected:
		static const std::string elementname;
	};

	class PNodeTimsh : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeTimsh> ptr_t;

		PNodeTimsh(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		Timesheet::Element& get_element(void) { return m_element; }
		const Timesheet::Element& get_element(void) const { return m_element; }

	protected:
		static const std::string elementname;
		Timesheet::Element m_element;
	};

	class PNodeTft : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeTft> ptr_t;

		PNodeTft(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual void on_subelement(const PNodeTimsh::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		Timesheet& get_timesheet(void) { return m_timesheet; }
		const Timesheet& get_timesheet(void) const { return m_timesheet; }

	protected:
		static const std::string elementname;
		Timesheet m_timesheet;
	};

	class PNodeAltRange : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeAltRange> ptr_t;

		PNodeAltRange(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);

		AltRange get_altrange(void) const;
		bool is_lower_valid(void) const;
		bool is_upper_valid(void) const;
		bool is_valid(void) const;

	protected:
		long m_lower;
		long m_upper;
		AltRange::alt_t m_lowermode;
		AltRange::alt_t m_uppermode;
		double m_lowerscale;
		double m_upperscale;
	};

	class PNodeFcl : public PNodeAltRange {
	public:
		typedef Glib::RefPtr<PNodeFcl> ptr_t;

		PNodeFcl(Loader& loader, unsigned int childnum) : PNodeAltRange(loader, childnum) {}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class PNodeTfl : public PNodeAltRange {
	public:
		typedef Glib::RefPtr<PNodeTfl> ptr_t;

		PNodeTfl(Loader& loader, unsigned int childnum) : PNodeAltRange(loader, childnum) {}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class PNodePoint : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodePoint> ptr_t;

		typedef enum {
			pt_first,
			pt_sta = pt_first,
			pt_end,
			pt_spn,
			pt_invalid
		} pt_t;

		typedef enum {
			nav_first,
			nav_intersection = nav_first,
			nav_vor,
			nav_dme,
			nav_ndb,
			nav_tacan,
			nav_invalid,
		} nav_t;

		typedef std::pair<pt_t,nav_t> type_t;

		PNodePoint(Loader& loader, unsigned int childnum, const std::string& name);
		PNodePoint(Loader& loader, unsigned int childnum, pt_t pt = pt_invalid, nav_t nt = nav_invalid);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return get_static_element_name(get_ptype(), get_ntype()); }
		static const std::string& get_static_element_name(pt_t pt, nav_t nt);
		static type_t find_type(const std::string& name);
		pt_t get_ptype(void) const { return m_ptype; }
		nav_t get_ntype(void) const { return m_ntype; }
		RuleWpt& get_point(void) { return m_pt; }
		const RuleWpt& get_point(void) const { return m_pt; }

	protected:
		static const std::string elementname;
		RuleWpt m_pt;
		pt_t m_ptype;
		nav_t m_ntype;

		void set_pttype(void);
	};

	class PNodeDct : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeDct> ptr_t;

	        PNodeDct(Loader& loader, unsigned int childnum, bool seg = false);

		virtual void on_subelement(const PNodePoint::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return get_static_element_name(m_seg); }
		static const std::string& get_static_element_name(bool seg) { return elementname[!!seg]; }

		const RuleWpt& get_point0(void) const { return m_point[0]; }
		const RuleWpt& get_point1(void) const { return m_point[1]; }
		const RuleWpt& operator[](unsigned int idx) const { return m_point[!!idx]; }

		virtual const std::string& get_ident(void) const { return empty; }
		virtual bool is_valid(void) const;
		bool is_seg(void) const { return m_seg; }

	protected:
		static const std::string elementname[2];
		static const std::string empty;
		RuleWpt m_point[2];
		bool m_seg;
	};

	class PNodeRpn : public PNodeDct {
	public:
		typedef Glib::RefPtr<PNodeRpn> ptr_t;

		PNodeRpn(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeRteUid::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_ident(void) const { return m_ident; }
		bool is_valid(void) const { return PNodeDct::is_valid() && !get_ident().empty(); }

	protected:
		static const std::string elementname;
		std::string m_ident;
	};

	class PNodeTfe : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeTfe> ptr_t;

		PNodeTfe(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const Glib::RefPtr<PNodeAseUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeSidUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeSiaUid>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeTfl>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodePoint>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeDct>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeRpn>& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeAhpUid>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		RestrictionElement::ptr_t get_restriction(void) const;

	protected:
		static const std::string elementname;
		AltRange m_alt;
		RuleWpt m_point[2];
		std::string m_id;
		std::string m_id2;

		typedef enum {
			mode_unset,
			mode_route,
			mode_dct,
			mode_seg,
			mode_point,
			mode_sid,
			mode_star,
			mode_airspace,
			mode_airport
		} mode_t;
		mode_t m_mode;
	};

	class PNodeTre : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeTre> ptr_t;

		PNodeTre(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeTfe::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }
		RestrictionSequence& get_sequence(void) { return m_seq; }
		const RestrictionSequence& get_sequence(void) const { return m_seq; }

	protected:
		static const std::string elementname;
		RestrictionSequence m_seq;
	};

	class PNodeDfl : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeDfl> ptr_t;

		PNodeDfl(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		double get_limit(void);

	protected:
		static const std::string elementname;
		double m_length;
		double m_scale;
	};

	class PNodeAbc : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeAbc> ptr_t;

		PNodeAbc(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeAseUid::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_ident(unsigned int index) const { return m_ident[!!index]; }
		const std::string& get_type(unsigned int index) const { return m_type[!!index]; }

	protected:
		static const std::string elementname;
		std::string m_ident[2];
		std::string m_type[2];
	};

	class PNodeAcs : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeAcs> ptr_t;

		PNodeAcs(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		Condition::ptr_t get_cond(void);

	protected:
		static const std::string elementname;
		std::string m_type;
		unsigned int m_nrengines;
		ConditionPropulsion::kind_t m_kind;
		ConditionPropulsion::propulsion_t m_propulsion;
		ConditionEquipment::equipment_t m_equipment;
	};

	class PNodeFcs : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeFcs> ptr_t;

		PNodeFcs(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		Condition::ptr_t get_cond(void);

	protected:
		static const std::string elementname;
		ConditionFlight::flight_t m_flight;
		ConditionCivMil::civmil_t m_civmil;
		bool m_gat;
		typedef enum {
			constant_false,
			constant_true,
			constant_undefined
		} constant_t;
		constant_t m_constant;
	};

	class PNodeFcc : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeFcc> ptr_t;

		PNodeFcc(Loader& loader, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const PNodeFcc::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<PNodeFce>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		Condition::ptr_t get_cond(void);

	protected:
		static const std::string elementname;
		typedef std::vector<Condition::ptr_t> conds_t;
		conds_t m_conds;
		typedef enum {
			mode_and,
			mode_or,
			mode_andnot,
			mode_seq,
			mode_none,
			mode_invalid
		} mode_t;
		mode_t m_mode;
	};

	class PNodeFce : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeFce> ptr_t;

		PNodeFce(Loader& loader, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const PNodeFcl::ptr_t& el);
		virtual void on_subelement(const PNodeAhpUid::ptr_t& el);
		virtual void on_subelement(const PNodeAseUid::ptr_t& el);
		virtual void on_subelement(const PNodeAbc::ptr_t& el);
		virtual void on_subelement(const PNodeDfl::ptr_t& el);
		virtual void on_subelement(const PNodeDct::ptr_t& el);
		virtual void on_subelement(const PNodeRpn::ptr_t& el);
		virtual void on_subelement(const PNodePoint::ptr_t& el);
		virtual void on_subelement(const PNodeAcs::ptr_t& el);
		virtual void on_subelement(const PNodeFcs::ptr_t& el);
		virtual void on_subelement(const PNodeSidUid::ptr_t& el);
		virtual void on_subelement(const PNodeSiaUid::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		Condition::ptr_t get_cond(void);

	protected:
		static const std::string elementname;
		AltRange m_altrange;
		std::string m_ident;
		std::string m_airport;
		Condition::ptr_t m_cond;
		TFRAirspace::const_ptr_t m_airspace[2];
		RuleWpt m_point[2];
		double m_dctlimit;
		bool m_refloc;
		typedef enum {
			mode_dep,
			mode_arr,
			mode_depaspc,
			mode_arraspc,
			mode_aspcactive,
			mode_crossingaspc1,
			mode_crossingaspc2,
			mode_crossingawy,
			mode_awyavailable,
			mode_crossingdct,
			mode_crossingpoint,
			mode_dctlimit,
			mode_cond,
			mode_sid,
			mode_star,
			mode_unspec
		} mode_t;
		mode_t m_mode;
	};

	class PNodeTfr : public PNodeNoIgnore {
	public:
		typedef Glib::RefPtr<PNodeTfr> ptr_t;

		PNodeTfr(Loader& loader, unsigned int childnum);

		virtual void on_subelement(const PNodeText::ptr_t& el);
		virtual void on_subelement(const PNodeTfrUid::ptr_t& el);
		virtual void on_subelement(const PNodeTft::ptr_t& el);
		virtual void on_subelement(const PNodeTre::ptr_t& el);
		virtual void on_subelement(const PNodeFcc::ptr_t& el);
		virtual void on_subelement(const PNodeFce::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		TrafficFlowRule::ptr_t get_tfr(void) { return m_tfr; }
	        TrafficFlowRule::const_ptr_t get_tfr(void) const { return m_tfr; }

	protected:
		static const std::string elementname;
		TrafficFlowRule::ptr_t m_tfr;
	};

	class PNodeAIXMSnapshot : public PNode {
	public:
		typedef Glib::RefPtr<PNodeAIXMSnapshot> ptr_t;

		PNodeAIXMSnapshot(Loader& loader, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const PNodeTfr::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	DbLoader m_dbldr;

	typedef std::vector<PNode::ptr_t > parsestack_t;
	parsestack_t m_parsestack;

	std::string get_stackpath(void) const;
};

TrafficFlowRestrictions::Loader::PNode::PNode(Loader& loader, unsigned int childnum)
	: m_loader(loader), m_refcount(1), m_childnum(childnum), m_childcount(0)
{
}

TrafficFlowRestrictions::Loader::PNode::~PNode()
{
}

void TrafficFlowRestrictions::Loader::PNode::reference(void) const
{
        g_atomic_int_inc(&m_refcount);
}

void TrafficFlowRestrictions::Loader::PNode::unreference(void) const
{
        if (!g_atomic_int_dec_and_test(&m_refcount))
                return;
        delete this;
}

void TrafficFlowRestrictions::Loader::PNode::error(const std::string& text) const
{
	m_loader.error(text);
}

void TrafficFlowRestrictions::Loader::PNode::error(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		error(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	m_loader.error(oss.str(), text);
}

void TrafficFlowRestrictions::Loader::PNode::warning(const std::string& text) const
{
	m_loader.warning(text);
}

void TrafficFlowRestrictions::Loader::PNode::warning(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		warning(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	m_loader.warning(oss.str(), text);
}

void TrafficFlowRestrictions::Loader::PNode::info(const std::string& text) const
{
	m_loader.info(text);
}

void TrafficFlowRestrictions::Loader::PNode::info(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		info(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	m_loader.info(oss.str(), text);
}

void TrafficFlowRestrictions::Loader::PNode::on_characters(const Glib::ustring& characters)
{
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const ptr_t& el)
{
	++m_childcount;
	{
		PNodeText::ptr_t elx(PNodeText::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeRteUid::ptr_t elx(PNodeRteUid::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeAhpUid::ptr_t elx(PNodeAhpUid::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeAseUid::ptr_t elx(PNodeAseUid::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeSidUid::ptr_t elx(PNodeSidUid::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeSiaUid::ptr_t elx(PNodeSiaUid::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeTfrUid::ptr_t elx(PNodeTfrUid::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeTimsh::ptr_t elx(PNodeTimsh::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeTft::ptr_t elx(PNodeTft::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeFcl::ptr_t elx(PNodeFcl::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeTfl::ptr_t elx(PNodeTfl::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodePoint::ptr_t elx(PNodePoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeRpn::ptr_t elx(PNodeRpn::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeDct::ptr_t elx(PNodeDct::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeTfe::ptr_t elx(PNodeTfe::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeTre::ptr_t elx(PNodeTre::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeDfl::ptr_t elx(PNodeDfl::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeAbc::ptr_t elx(PNodeAbc::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeAcs::ptr_t elx(PNodeAcs::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeFcs::ptr_t elx(PNodeFcs::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeFcc::ptr_t elx(PNodeFcc::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeFce::ptr_t elx(PNodeFce::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeTfr::ptr_t elx(PNodeTfr::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		PNodeIgnore::ptr_t elx(PNodeIgnore::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	error(el, "Element " + get_element_name() + " does not support unknown subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeIgnore::ptr_t& el)
{
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeText::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support text subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeRteUid::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support RteUid subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeAhpUid::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support AhpUid subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeAseUid::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support AseUid subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeSidUid::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support SidUid subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeSiaUid::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support SiaUid subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeTfrUid::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support TfrUid subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeTimsh::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Timsh subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeTft::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Tft subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeFcl::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Fcl subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeTfl::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Tfl subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodePoint::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support " + el->get_element_name() + " subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeDct::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support " + el->get_element_name() + " subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeRpn::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Rpn subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeTfe::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Tfe subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeTre::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Tre subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeDfl::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Dfl subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeAbc::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Abc subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeAcs::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Acs subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeFcs::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Fcs subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeFcc::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Fcc subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeFce::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Fce subelements");
}

void TrafficFlowRestrictions::Loader::PNode::on_subelement(const PNodeTfr::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support Tfr subelements");
}

TrafficFlowRestrictions::Loader::PNodeIgnore::PNodeIgnore(Loader& loader, unsigned int childnum, const std::string& name)
	: PNode(loader, childnum), m_elementname(name)
{
}

void TrafficFlowRestrictions::Loader::PNodeNoIgnore::on_subelement(const PNodeIgnore::ptr_t& el)
{
	if (true)
		std::cerr << "TFR Loader: Element " + get_element_name() + " does not support ignored subelements" << std::endl;
	error("Element " + get_element_name() + " does not support ignored subelements");
}

TrafficFlowRestrictions::Loader::PNodeText::PNodeText(Loader& loader, unsigned int childnum, const std::string& name)
	: PNodeNoIgnore(loader, childnum), m_texttype(find_type(name))
{
}

TrafficFlowRestrictions::Loader::PNodeText::PNodeText(Loader& loader, unsigned int childnum, texttype_t tt)
	: PNodeNoIgnore(loader, childnum), m_texttype(tt)
{
}

void TrafficFlowRestrictions::Loader::PNodeText::on_characters(const Glib::ustring& characters)
{
	m_text += characters;
}

const std::string& TrafficFlowRestrictions::Loader::PNodeText::get_static_element_name(texttype_t tt)
{
	switch (tt) {
	case tt_codeid:
	{
		static const std::string r("codeId");
		return r;
	}

	case tt_codetype:
	{
		static const std::string r("codeType");
		return r;
	}

	case tt_txtoprgoal:
	{
		static const std::string r("txtOprGoal");
		return r;
	}

	case tt_txtdescr:
	{
		static const std::string r("txtDescr");
		return r;
	}

	case tt_codeworkhr:
	{
		static const std::string r("codeWorkHr");
		return r;
	}

	case tt_timewef:
	{
		static const std::string r("timeWef");
		return r;
	}

	case tt_timetil:
	{
		static const std::string r("timeTil");
		return r;
	}

	case tt_datevalidwef:
	{
		static const std::string r("dateValidWef");
		return r;
	}

	case tt_datevalidtil:
	{
		static const std::string r("dateValidTil");
		return r;
	}

	case tt_codeday:
	{
		static const std::string r("codeDay");
		return r;
	}

	case tt_codetimeref:
	{
		static const std::string r("codeTimeRef");
		return r;
	}

	case tt_codeeventwef:
	{
		static const std::string r("codeEventWef");
		return r;
	}

	case tt_codeeventtil:
	{
		static const std::string r("codeEventTil");
		return r;
	}

	case tt_timereleventwef:
	{
		static const std::string r("timeRelEventWef");
		return r;
	}

	case tt_timereleventtil:
	{
		static const std::string r("timeRelEventTil");
		return r;
	}

	case tt_valdistverlower:
	{
		static const std::string r("valDistVerLower");
		return r;
	}

	case tt_uomdistverlower:
	{
		static const std::string r("uomDistVerLower");
		return r;
	}

	case tt_codedistverlower:
	{
		static const std::string r("codeDistVerLower");
		return r;
	}

	case tt_valdistverupper:
	{
		static const std::string r("valDistVerUpper");
		return r;
	}

	case tt_uomdistverupper:
	{
		static const std::string r("uomDistVerUpper");
		return r;
	}

	case tt_codedistverupper:
	{
		static const std::string r("codeDistVerUpper");
		return r;
	}

	case tt_geolat:
	{
		static const std::string r("geoLat");
		return r;
	}

	case tt_geolong:
	{
		static const std::string r("geoLong");
		return r;
	}

	case tt_txtdesig:
	{
		static const std::string r("txtDesig");
		return r;
	}

	case tt_txtlocdesig:
	{
		static const std::string r("txtLocDesig");
		return r;
	}

	case tt_valexceedlen:
	{
		static const std::string r("valExceedLen");
		return r;
	}

	case tt_uomlen:
	{
		static const std::string r("uomLen");
		return r;
	}

	case tt_codetypeengine:
	{
		static const std::string r("codeTypeEngine");
		return r;
	}

	case tt_codeengineno:
	{
		static const std::string r("codeEngineNo");
		return r;
	}

	case tt_codeicaoacfttype:
	{
		static const std::string r("codeIcaoAcftType");
		return r;
	}

	case tt_codecapability:
	{
		static const std::string r("codeCapability");
		return r;
	}

	case tt_codepurpose:
	{
		static const std::string r("codePurpose");
		return r;
	}

	case tt_codemil:
	{
		static const std::string r("codeMil");
		return r;
	}

	case tt_constant:
	{
		static const std::string r("constant");
		return r;
	}

	default:
	case tt_invalid:
	{
		static const std::string r("?");
		return r;
	}
	}
}

TrafficFlowRestrictions::Loader::PNodeText::texttype_t TrafficFlowRestrictions::Loader::PNodeText::find_type(const std::string& name)
{
	for (texttype_t tt(tt_first); tt < tt_invalid; tt = (texttype_t)(tt + 1))
		if (name == get_static_element_name(tt))
			return tt;
	return tt_invalid;
}

long TrafficFlowRestrictions::Loader::PNodeText::get_long(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	long v(strtol(cp, &ep, 10));
	if (!*ep && ep != cp)
		return v;
	error("invalid signed number " + get_text());
	return 0;
}

unsigned long TrafficFlowRestrictions::Loader::PNodeText::get_ulong(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	unsigned long v(strtoul(cp, &ep, 10));
	if (!*ep && ep != cp)
		return v;
	error("invalid unsigned number " + get_text());
	return 0;
}

double TrafficFlowRestrictions::Loader::PNodeText::get_double(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	double v(strtod(cp, &ep));
	if (!*ep && ep != cp)
		return v;
	error("invalid floating point number " + get_text());
	return std::numeric_limits<double>::quiet_NaN();
}

Point::coord_t TrafficFlowRestrictions::Loader::PNodeText::get_lat(void) const
{
	Point pt;
	if (!(pt.set_str(get_text()) & Point::setstr_lat)) {
		error("invalid latitude " + get_text());
		pt.set_invalid();
	}
	return pt.get_lat();
}

Point::coord_t TrafficFlowRestrictions::Loader::PNodeText::get_lon(void) const
{
	Point pt;
	if (!(pt.set_str(get_text()) & Point::setstr_lon)) {
		error("invalid longitude " + get_text());
		pt.set_invalid();
	}
	return pt.get_lon();
}

const std::string TrafficFlowRestrictions::Loader::PNodeRteUid::elementname("RteUid");

TrafficFlowRestrictions::Loader::PNodeRteUid::PNodeRteUid(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeRteUid::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_txtdesig:
		m_ident = el->get_text();
		break;

	case PNodeText::tt_txtlocdesig:
		break;

	default:
		error("RteUid node has invalid text subelement " + el->get_element_name());
	}
}

const std::string TrafficFlowRestrictions::Loader::PNodeAhpUid::elementname("AhpUid");

TrafficFlowRestrictions::Loader::PNodeAhpUid::PNodeAhpUid(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeAhpUid::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_codeid:
		m_ident = el->get_text();
		break;

	default:
		error("AhpUid node has invalid text subelement " + el->get_element_name());
	}
}

TrafficFlowRestrictions::Loader::PNodeAseUid::PNodeAseUid(Loader& loader, unsigned int childnum, const std::string& name)
	: PNodeNoIgnore(loader, childnum), m_asetype(asetype_invalid)
{
	m_asetype = find_type(name);
}

TrafficFlowRestrictions::Loader::PNodeAseUid::PNodeAseUid(Loader& loader, unsigned int childnum, asetype_t t)
	: PNodeNoIgnore(loader, childnum), m_asetype(t)
{
}

const std::string& TrafficFlowRestrictions::Loader::PNodeAseUid::get_static_element_name(asetype_t t)
{
	switch (t) {
	case asetype_normal:
	{
		static const std::string r("AseUid");
		return r;
	}

	case asetype_from:
	{
		static const std::string r("AseUidFrom");
		return r;
	}

	case asetype_into:
	{
		static const std::string r("AseUidInto");
		return r;
	}

	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

TrafficFlowRestrictions::Loader::PNodeAseUid::asetype_t TrafficFlowRestrictions::Loader::PNodeAseUid::find_type(const std::string& name)
{
	for (asetype_t t(asetype_first); t < asetype_invalid; t = (asetype_t)(t + 1))
		if (name == get_static_element_name(t))
			return t;
	return asetype_invalid;
}

void TrafficFlowRestrictions::Loader::PNodeAseUid::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_codeid:
		m_ident = el->get_text();
		break;

	case PNodeText::tt_codetype:
		m_type = el->get_text();
		break;

	default:
		error("AseUid node has invalid text subelement " + el->get_element_name());
	}
}

const std::string TrafficFlowRestrictions::Loader::PNodeSidUid::elementname("SidUid");

TrafficFlowRestrictions::Loader::PNodeSidUid::PNodeSidUid(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeSidUid::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_txtdesig:
		m_ident = el->get_text();
		break;

	default:
		error("SidUid node has invalid text subelement " + el->get_element_name());
	}
}

void TrafficFlowRestrictions::Loader::PNodeSidUid::on_subelement(const PNodeAhpUid::ptr_t& el)
{
	m_airport = el->get_ident();
}

const std::string TrafficFlowRestrictions::Loader::PNodeSiaUid::elementname("SiaUid");

TrafficFlowRestrictions::Loader::PNodeSiaUid::PNodeSiaUid(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeSiaUid::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_txtdesig:
		m_ident = el->get_text();
		break;

	default:
		error("SiaUid node has invalid text subelement " + el->get_element_name());
	}
}

void TrafficFlowRestrictions::Loader::PNodeSiaUid::on_subelement(const PNodeAhpUid::ptr_t& el)
{
	m_airport = el->get_ident();
}

TrafficFlowRestrictions::Loader::PNodeUid::PNodeUid(Loader& loader, unsigned int childnum, const AttributeList& properties)
	: PNodeNoIgnore(loader, childnum), m_mid(~0ULL)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "mid")
			continue;
		const char *cp(i->value.c_str());
		char *ep;
		m_mid = strtoull(cp, &ep, 10);
		if (*ep || cp == ep)
			m_mid = ~0ULL;
	}
}
		
void TrafficFlowRestrictions::Loader::PNodeUid::on_subelement(const PNode::ptr_t& el)
{
	PNodeText::ptr_t elx(PNodeText::ptr_t::cast_dynamic(el));
	if (!elx || elx->get_type() != PNodeText::tt_codeid)
		error("Uid node: invalid subelement " + el->get_element_name());
	m_codeid = elx->get_text();	
}

const std::string TrafficFlowRestrictions::Loader::PNodeTfrUid::elementname("TfrUid");

const std::string TrafficFlowRestrictions::Loader::PNodeTimsh::elementname("Timsh");

TrafficFlowRestrictions::Loader::PNodeTimsh::PNodeTimsh(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeTimsh::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_timewef:
		m_element.set_timewef(el->get_text());
		break;

	case PNodeText::tt_timetil:
		m_element.set_timetil(el->get_text());
		break;

	case PNodeText::tt_datevalidwef:
		m_element.set_datewef(el->get_text());
		break;

	case PNodeText::tt_datevalidtil:
		m_element.set_datetil(el->get_text());
		break;

	case PNodeText::tt_codeday:
		m_element.set_day(el->get_text());
		break;

	case PNodeText::tt_codetimeref:
		m_element.set_timeref(el->get_text());
		break;

	case PNodeText::tt_codeeventwef:
		if (el->get_text() != "SR")
			error("unsupported codeEventWef event " + el->get_text());
		m_element.set_sunrise(true);
		break;

	case PNodeText::tt_codeeventtil:
		if (el->get_text() != "SS")
			error("unsupported codeEventTil event " + el->get_text());
		m_element.set_sunset(true);
		break;

	case PNodeText::tt_timereleventwef:
		m_element.set_timewef(el->get_long() * 60);
		break;

	case PNodeText::tt_timereleventtil:
		m_element.set_timetil(el->get_long() * 60);
		break;

	default:
		error("Tft node has invalid text subelement " + el->get_element_name());
	}
}

const std::string TrafficFlowRestrictions::Loader::PNodeTft::elementname("Tft");

TrafficFlowRestrictions::Loader::PNodeTft::PNodeTft(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeTft::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_codeworkhr:
		m_timesheet.set_workhr(el->get_text());
		break;

	default:
		error("Tft node has invalid text subelement " + el->get_element_name());
	}
}

void TrafficFlowRestrictions::Loader::PNodeTft::on_subelement(const PNodeTimsh::ptr_t& el)
{
	if (!el->get_element().is_valid())
		error("Tft: invalid Timsh element" + el->get_element().to_ustr());
	m_timesheet.add(el->get_element());
}

TrafficFlowRestrictions::Loader::PNodeAltRange::PNodeAltRange(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum),
	  m_lower(std::numeric_limits<long>::max()),
	  m_upper(std::numeric_limits<long>::min()),
	  m_lowermode(AltRange::alt_invalid), m_uppermode(AltRange::alt_invalid),
	  m_lowerscale(std::numeric_limits<double>::quiet_NaN()),
	  m_upperscale(std::numeric_limits<double>::quiet_NaN())
{
}

void TrafficFlowRestrictions::Loader::PNodeAltRange::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_valdistverlower:
	{
		double val(el->get_double());
		if (!std::isnan(val))
			m_lower = Point::round<long,double>(val);
		break;
	}

	case PNodeText::tt_valdistverupper:
	{
		double val(el->get_double());
		if (!std::isnan(val))
			m_upper = Point::round<long,double>(val);
		break;
	}

	case PNodeText::tt_uomdistverlower:
		if (el->get_text() == "M")
			m_lowerscale = Point::m_to_ft_dbl;
		else if (el->get_text() == "FT")
			m_lowerscale = 1;
		else if (el->get_text() == "FL")
			m_lowerscale = 100;
		else 
			m_lowerscale = std::numeric_limits<double>::quiet_NaN();
		break;

	case PNodeText::tt_uomdistverupper:
		if (el->get_text() == "M")
			m_upperscale = Point::m_to_ft_dbl;
		else if (el->get_text() == "FT")
			m_upperscale = 1;
		else if (el->get_text() == "FL")
			m_upperscale = 100;
		else 
			m_upperscale = std::numeric_limits<double>::quiet_NaN();
		break;

	case PNodeText::tt_codedistverlower:
		m_lowermode = AltRange::alt_invalid;
		for (AltRange::alt_t a(AltRange::alt_first); a < AltRange::alt_invalid; a = (AltRange::alt_t)(a + 1))
			if (el->get_text() == AltRange::get_mode_string(a)) {
				m_lowermode = a;
				break;
			}
		break;

	case PNodeText::tt_codedistverupper:
		m_uppermode = AltRange::alt_invalid;
		for (AltRange::alt_t a(AltRange::alt_first); a < AltRange::alt_invalid; a = (AltRange::alt_t)(a + 1))
			if (el->get_text() == AltRange::get_mode_string(a)) {
				m_uppermode = a;
				break;
			}
		break;

	default:
		error(get_element_name() + " node has invalid text subelement " + el->get_element_name());
	}
}

TrafficFlowRestrictions::AltRange TrafficFlowRestrictions::Loader::PNodeAltRange::get_altrange(void) const
{
	return AltRange(Point::round<int32_t,double>(m_lower * m_lowerscale), is_lower_valid() ? m_lowermode : AltRange::alt_invalid,
			Point::round<int32_t,double>(m_upper * m_upperscale), is_upper_valid() ? m_uppermode : AltRange::alt_invalid);
}

bool TrafficFlowRestrictions::Loader::PNodeAltRange::is_lower_valid(void) const
{
	return (m_lower != std::numeric_limits<long>::max() &&
		m_lowermode < AltRange::alt_invalid &&
		!std::isnan(m_lowerscale));
}

bool TrafficFlowRestrictions::Loader::PNodeAltRange::is_upper_valid(void) const
{
	return (m_upper != std::numeric_limits<long>::min() &&
		m_uppermode < AltRange::alt_invalid &&
		!std::isnan(m_upperscale));
}

bool TrafficFlowRestrictions::Loader::PNodeAltRange::is_valid(void) const
{
	return is_lower_valid() && is_upper_valid();
}

const std::string TrafficFlowRestrictions::Loader::PNodeFcl::elementname("Fcl");

const std::string TrafficFlowRestrictions::Loader::PNodeTfl::elementname("Tfl");


TrafficFlowRestrictions::Loader::PNodePoint::PNodePoint(Loader& loader, unsigned int childnum, const std::string& name)
	: PNodeNoIgnore(loader, childnum), m_ptype(pt_invalid), m_ntype(nav_invalid)
{
	type_t t(find_type(name));
	m_ptype = t.first;
	m_ntype = t.second;
	set_pttype();
	{
		Point pt;
		pt.set_invalid();
		m_pt.set_coord(pt);
	}
}

TrafficFlowRestrictions::Loader::PNodePoint::PNodePoint(Loader& loader, unsigned int childnum, pt_t pt, nav_t nt)
	: PNodeNoIgnore(loader, childnum), m_ptype(pt), m_ntype(nt)
{
	set_pttype();
	{
		Point pt;
		pt.set_invalid();
		m_pt.set_coord(pt);
	}
}

void TrafficFlowRestrictions::Loader::PNodePoint::set_pttype(void)
{
	switch (get_ntype()) {
	case nav_intersection:
		m_pt.set_type(Vertex::type_intersection);
		m_pt.set_vor(false);
		m_pt.set_ndb(false);
		break;

	case nav_vor:
		m_pt.set_type(Vertex::type_navaid);
		m_pt.set_vor(true);
		m_pt.set_ndb(false);
		break;

	case nav_dme:
		m_pt.set_type(Vertex::type_navaid);
		m_pt.set_vor(false);
		m_pt.set_ndb(false);
		break;

	case nav_ndb:
		m_pt.set_type(Vertex::type_navaid);
		m_pt.set_vor(false);
		m_pt.set_ndb(true);
		break;

	case nav_tacan:
		m_pt.set_type(Vertex::type_navaid);
		m_pt.set_vor(false);
		m_pt.set_ndb(false);
		break;

	default:
		m_pt.set_type(Vertex::type_undefined);
		m_pt.set_vor(false);
		m_pt.set_ndb(false);
		break;
	};
}

void TrafficFlowRestrictions::Loader::PNodePoint::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_codeid:
		m_pt.set_ident(el->get_text());
		break;

	case PNodeText::tt_geolat:
		m_pt.set_lat(el->get_lat());
		break;

	case PNodeText::tt_geolong:
		m_pt.set_lon(el->get_lon());
		break;

	default:
		error(get_element_name() + " node has invalid text subelement " + el->get_element_name());
	}
}

const std::string& TrafficFlowRestrictions::Loader::PNodePoint::get_static_element_name(pt_t pt, nav_t nt)
{
	static const std::string invalid("?");
	static const std::string elementnames[pt_invalid][nav_invalid] = {
		{ "DpnUidSta", "VorUidSta", "DmeUidSta", "NdbUidSta", "TcnUidSta" },
		{ "DpnUidEnd", "VorUidEnd", "DmeUidEnd", "NdbUidEnd", "TcnUidEnd" },
		{ "DpnUidSpn", "VorUidSpn", "DmeUidSpn", "NdbUidSpn", "TcnUidSpn" }
	};

	if (pt >= pt_invalid || nt >= nav_invalid)
		return invalid;
	return elementnames[pt][nt];
}

TrafficFlowRestrictions::Loader::PNodePoint::type_t TrafficFlowRestrictions::Loader::PNodePoint::find_type(const std::string& name)
{
	for (pt_t p(pt_first); p < pt_invalid; p = (pt_t)(p + 1))
		for (nav_t n(nav_first); n < nav_invalid; n = (nav_t)(n + 1))
			if (get_static_element_name(p, n) == name)
				return type_t(p, n);
	return type_t(pt_invalid, nav_invalid);
}

const std::string TrafficFlowRestrictions::Loader::PNodeDct::elementname[2] = { "Dct" , "Seg" };
const std::string TrafficFlowRestrictions::Loader::PNodeDct::empty;

TrafficFlowRestrictions::Loader::PNodeDct::PNodeDct(Loader& loader, unsigned int childnum, bool seg)
	: PNodeNoIgnore(loader, childnum), m_seg(seg)
{
	Point pt;
	pt.set_invalid();
	m_point[0].set_coord(pt);
	m_point[1].set_coord(pt);
}

void TrafficFlowRestrictions::Loader::PNodeDct::on_subelement(const PNodePoint::ptr_t& el)
{
	unsigned int index(0);

	switch (el->get_ptype()) {
	case PNodePoint::pt_sta:
		index = 0;
		break;

	case PNodePoint::pt_end:
		index = 1;
		break;

	default:
		error(get_element_name() + " node has invalid point subelement " + el->get_element_name());
	}
	if (m_point[index].get_type() != Vertex::type_undefined)
		error(get_element_name() + " node has duplicate points");
	if (el->get_point().get_type() == Vertex::type_undefined)
		error(get_element_name() + " node has invalid point subelement " + el->get_element_name());
	m_point[index] = el->get_point();
}

bool TrafficFlowRestrictions::Loader::PNodeDct::is_valid(void) const
{
	for (unsigned int index = 0; index < 2; ++index)
		if (m_point[index].get_type() == Vertex::type_undefined)
			return false;
	return true;
}

const std::string TrafficFlowRestrictions::Loader::PNodeRpn::elementname("Rpn");

TrafficFlowRestrictions::Loader::PNodeRpn::PNodeRpn(Loader& loader, unsigned int childnum)
	: PNodeDct(loader, false)
{
}

void TrafficFlowRestrictions::Loader::PNodeRpn::on_subelement(const PNodeRteUid::ptr_t& el)
{
	m_ident = el->get_ident();
}

const std::string TrafficFlowRestrictions::Loader::PNodeTfe::elementname("Tfe");

TrafficFlowRestrictions::Loader::PNodeTfe::PNodeTfe(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum), m_mode(mode_unset)
{
	Point pt;
	pt.set_invalid();
	m_point[0].set_coord(pt);
	m_point[1].set_coord(pt);
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodeAseUid>& el)
{
	if (m_mode != mode_unset || el->get_ident().empty() || el->get_type().empty()) {
		if (true)
			std::cerr << "TFR Loader: " << get_element_name() << " node is invalid" << std::endl;
		error(get_element_name() + " node is invalid");
	}
	m_mode = mode_airspace;
	m_id = el->get_ident();
	m_id2 = el->get_type();
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodeSidUid>& el)
{
	if (m_mode != mode_unset || el->get_ident().empty() || el->get_airport().empty())
		error(get_element_name() + " node is invalid");
	m_mode = mode_sid;
	m_id = el->get_ident();
	m_id2 = el->get_airport();
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodeSiaUid>& el)
{
	if (m_mode != mode_unset || el->get_ident().empty() || el->get_airport().empty()) {
		if (true)
			std::cerr << "TFR Loader: " << get_element_name() << " node is invalid" << std::endl;
		error(get_element_name() + " node is invalid");
	}
	m_mode = mode_star;
	m_id = el->get_ident();
	m_id2 = el->get_airport();
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodeTfl>& el)
{
	if (m_alt.is_valid())
		error(get_element_name() + " node has duplicate Tfl subelement");
	if (!el->is_lower_valid() && !el->is_upper_valid())
		error(get_element_name() + " node has invalid Tfl subelement");
	m_alt = el->get_altrange();
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodePoint>& el)
{
	if (m_mode != mode_unset || el->get_point().get_coord().is_invalid())
		error(get_element_name() + " node is invalid");
	m_mode = mode_point;
	m_point[0] = el->get_point();
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodeDct>& el)
{
	if (m_mode != mode_unset || el->get_point0().get_coord().is_invalid() || el->get_point1().get_coord().is_invalid())
		error(get_element_name() + " node is invalid");
	m_mode = el->is_seg() ? mode_seg : mode_dct;
	m_point[0] = el->get_point0();
	m_point[1] = el->get_point1();
	m_id = el->get_ident();
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodeRpn>& el)
{
	if (m_mode != mode_unset || el->get_point0().get_coord().is_invalid() || el->get_point1().get_coord().is_invalid())
		error(get_element_name() + " node is invalid");
	m_mode = mode_route;
	m_point[0] = el->get_point0();
	m_point[1] = el->get_point1();
	m_id = el->get_ident();
}

void TrafficFlowRestrictions::Loader::PNodeTfe::on_subelement(const Glib::RefPtr<PNodeAhpUid>& el)
{
	if (m_mode != mode_unset || el->get_ident().empty())
		error(get_element_name() + " node is invalid");
	m_mode = mode_airport;
	m_id = el->get_ident();
}

TrafficFlowRestrictions::RestrictionElement::ptr_t TrafficFlowRestrictions::Loader::PNodeTfe::get_restriction(void) const
{
	switch (m_mode) {
	default:
		return RestrictionElement::ptr_t();

	case mode_route:
		return RestrictionElement::ptr_t(new RestrictionElementRoute(m_alt, m_point[0], m_point[1], m_id, RestrictionElementRoute::match_awy));

	case mode_dct:
		return RestrictionElement::ptr_t(new RestrictionElementRoute(m_alt, m_point[0], m_point[1], m_id, RestrictionElementRoute::match_dct));

	case mode_seg:
		return RestrictionElement::ptr_t(new RestrictionElementRoute(m_alt, m_point[0], m_point[1], m_id, RestrictionElementRoute::match_all));

	case mode_point:
		return RestrictionElement::ptr_t(new RestrictionElementPoint(m_alt, m_point[0]));

	case mode_sid:
	case mode_star:
	{
		Point coord;
		coord.set_invalid();
		AirportsDb::Airport arpt(m_loader.find_airport(m_id2));
		if (arpt.is_valid())
			coord = arpt.get_coord();
		else
			warning("Airport " + m_id2 + " not found");
		return RestrictionElement::ptr_t(new RestrictionElementSidStar(m_alt, m_id, RuleWpt(Vertex::type_airport, m_id2, coord, false, false),
									       m_mode == mode_star));
	}

	case mode_airspace:
	{
		TFRAirspace::const_ptr_t aspc(m_loader.find_airspace(m_id, m_id2));
		if (!aspc) {
			warning("cannot find airspace " + m_id + "/" + m_id2);
			return RestrictionElement::ptr_t();
		}
		return RestrictionElement::ptr_t(new RestrictionElementAirspace(m_alt, aspc));
	}

	case mode_airport:
	{
		AirportsDb::Airport arpt(m_loader.find_airport(m_id));
		if (!arpt.is_valid()) {
		        warning("cannot find airport " + m_id);
			return RestrictionElement::ptr_t();
		}
		if (m_alt.is_lower_valid() + m_alt.is_upper_valid() == 1)
			error(get_element_name() + " node has invalid Tfl subelement");
		RuleWpt pt(Vertex::type_airport, m_id, arpt.get_coord());
		return RestrictionElement::ptr_t(new RestrictionElementPoint(m_alt, pt));
       	}
	}
}

const std::string TrafficFlowRestrictions::Loader::PNodeTre::elementname("Tre");

TrafficFlowRestrictions::Loader::PNodeTre::PNodeTre(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeTre::on_subelement(const PNodeTfe::ptr_t& el)
{
	m_seq.add(el->get_restriction());
}


const std::string TrafficFlowRestrictions::Loader::PNodeDfl::elementname("Dfl");

TrafficFlowRestrictions::Loader::PNodeDfl::PNodeDfl(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum), m_length(std::numeric_limits<double>::quiet_NaN()),
	  m_scale(std::numeric_limits<double>::quiet_NaN())
{
}

void TrafficFlowRestrictions::Loader::PNodeDfl::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_valexceedlen:
		m_length = el->get_double();
		break;

	case PNodeText::tt_uomlen:
		if (el->get_text() == "NM")
			m_scale = 1;
		else if (el->get_text() == "KM")
			m_scale = Point::km_to_nmi_dbl;
		else if (el->get_text() == "M")
			m_scale = 1e-3 * Point::km_to_nmi_dbl;
		else
			m_scale = std::numeric_limits<double>::quiet_NaN();
		break;

	default:
		error("Dfl node has invalid text subelement " + el->get_element_name());
	}
}

double TrafficFlowRestrictions::Loader::PNodeDfl::get_limit(void)
{
	return m_length * m_scale;
}

const std::string TrafficFlowRestrictions::Loader::PNodeAbc::elementname("Abc");

TrafficFlowRestrictions::Loader::PNodeAbc::PNodeAbc(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum)
{
}

void TrafficFlowRestrictions::Loader::PNodeAbc::on_subelement(const PNodeAseUid::ptr_t& el)
{
	unsigned int index(0);
	switch (el->get_asetype()) {
	case PNodeAseUid::asetype_from:
		index = 0;
		break;

	case PNodeAseUid::asetype_into:
		index = 1;
		break;

	default:
		error(get_element_name() + " node with invalid " + el->get_element_name() + " child");
	}
	m_ident[index] = el->get_ident();
	m_type[index] = el->get_type();
}

const std::string TrafficFlowRestrictions::Loader::PNodeAcs::elementname("Acs");

TrafficFlowRestrictions::Loader::PNodeAcs::PNodeAcs(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum), m_nrengines(0), m_kind(ConditionPropulsion::kind_invalid),
	  m_propulsion(ConditionPropulsion::propulsion_invalid), m_equipment(ConditionEquipment::equipment_invalid)
{
}

void TrafficFlowRestrictions::Loader::PNodeAcs::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_codetype:
	{
		const std::string& txt(el->get_text());
		if (txt == "OTHER") {
			m_kind = ConditionPropulsion::kind_any;
			break;
		}
		if (txt.size() != 1)
			error(el, "Aircraft Kind must be one character length: " + txt);
		switch (txt[0]) {
		case 'L':
			m_kind = ConditionPropulsion::kind_landplane;
			break;

		case 'S':
			m_kind = ConditionPropulsion::kind_seaplane;
			break;

		case 'A':
			m_kind = ConditionPropulsion::kind_amphibian;
			break;

		case 'H':
			m_kind = ConditionPropulsion::kind_helicopter;
			break;

		case 'G':
			m_kind = ConditionPropulsion::kind_gyrocopter;
			break;

		case 'T':
			m_kind = ConditionPropulsion::kind_tiltwing;
			break;

		default:
			error(el, "invalid Aircraft Kind: " + txt);	
			break;
		}
		break;
	}

	case PNodeText::tt_codetypeengine:
	{
		const std::string& txt(el->get_text());
		if (txt.size() != 1)
			error(el, "Engine Type must be one character length: " + txt);
		switch (txt[0]) {
		case 'P':
			m_propulsion = ConditionPropulsion::propulsion_piston;
			break;

		case 'T':
			m_propulsion = ConditionPropulsion::propulsion_turboprop;
			break;

		case 'J':
			m_propulsion = ConditionPropulsion::propulsion_jet;
			break;

		default:
			error(el, "invalid Engine Type: " + txt);
			break;
		}
		break;
	}

	case PNodeText::tt_codeengineno:
	{
		const std::string& txt(el->get_text());
		if (txt == "OTHER") {
			m_nrengines = 0;
		} else {
			m_nrengines = el->get_ulong();
			if (!m_nrengines)
				error(el, "invalid Engine Number: " + txt);
		}
		break;
	}

	case PNodeText::tt_codeicaoacfttype:
		m_type = el->get_text();
		if (m_type.empty())
			error(el, "empty Aircraft Type");
		break;

	case PNodeText::tt_codecapability:
	{
		const std::string& txt(el->get_text());
		if (txt == "RNAV") {
			m_equipment = ConditionEquipment::equipment_rnav;
		} else if (txt == "RVSM") {
			m_equipment = ConditionEquipment::equipment_rvsm;
		} else if (txt == "8.33kHz") {
			m_equipment = ConditionEquipment::equipment_833;
		} else if (txt == "MLS") {
			m_equipment = ConditionEquipment::equipment_mls;
		} else if (txt == "ILS") {
			m_equipment = ConditionEquipment::equipment_ils;
		} else {
			error(el, "invalid Capability: " + txt);
		}
		break;
	}

	default:
		error("Acs node has invalid text subelement " + el->get_element_name());
	}
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Loader::PNodeAcs::get_cond(void)
{
	if (!m_type.empty() && m_kind == ConditionPropulsion::kind_invalid && m_propulsion == ConditionPropulsion::propulsion_invalid &&
	    m_equipment == ConditionEquipment::equipment_invalid && !m_nrengines)
		return Condition::ptr_t(new ConditionAircraftType(get_childnum(), m_type));
	if (m_type.empty() && m_kind != ConditionPropulsion::kind_invalid && m_propulsion != ConditionPropulsion::propulsion_invalid &&
	    m_equipment == ConditionEquipment::equipment_invalid)
		return Condition::ptr_t(new ConditionPropulsion(get_childnum(), m_kind, m_propulsion, m_nrengines));
	if (m_type.empty() && m_kind == ConditionPropulsion::kind_invalid && m_propulsion == ConditionPropulsion::propulsion_invalid &&
	    m_equipment != ConditionEquipment::equipment_invalid && !m_nrengines)
		return Condition::ptr_t(new ConditionEquipment(get_childnum(), m_equipment));
	error("invalid Acs");
	return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
}

const std::string TrafficFlowRestrictions::Loader::PNodeFcs::elementname("Fcs");

TrafficFlowRestrictions::Loader::PNodeFcs::PNodeFcs(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum), m_flight(ConditionFlight::flight_invalid), m_civmil(ConditionCivMil::civmil_invalid),
	  m_gat(false), m_constant(constant_undefined)
{
}

void TrafficFlowRestrictions::Loader::PNodeFcs::on_subelement(const PNodeText::ptr_t& el)
{
	switch (el->get_type()) {
	case PNodeText::tt_codetype:
	{
		const std::string& txt(el->get_text());
		if (txt == "GAT")
			m_gat = true;
		else
			error(el, "invalid codetype: " + txt);
		break;
	}

	case PNodeText::tt_codepurpose:
	{
		const std::string& txt(el->get_text());
		if (txt == "S") {
			m_flight = ConditionFlight::flight_scheduled;
		} else if (txt == "NS") {
			m_flight = ConditionFlight::flight_nonscheduled;
		} else if (txt == "OTHER") {
			m_flight = ConditionFlight::flight_other;
		} else {
			error(el, "invalid flight purpose: " + txt);
		}
		break;
	}

	case PNodeText::tt_codemil:
	{
		const std::string& txt(el->get_text());
		if (txt == "CIV") {
			m_civmil = ConditionCivMil::civmil_civ;
		} else if (txt == "MIL") {
			m_civmil = ConditionCivMil::civmil_mil;
		} else {
			error(el, "invalid military indication: " + txt);
		}
		break;
	}

	case PNodeText::tt_constant:
	{
		const std::string& txt(el->get_text());
		constant_t c(constant_undefined);
		if (txt == "true" || txt == "True" || txt == "TRUE") {
			c = constant_true;
		} else if (txt == "false" || txt == "False" || txt == "FALSE") {
		        c = constant_false;
		} else {
			error(el, "invalid condition constant: " + txt);
		}
		if (m_constant != constant_undefined && m_constant != c)
			warning(el, "duplicate different condition constants");
		m_constant = c;
		break;
	}

	default:
		error("Fcs node has invalid text subelement " + el->get_element_name());
	}
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Loader::PNodeFcs::get_cond(void)
{
	if (m_flight != ConditionFlight::flight_invalid && m_civmil == ConditionCivMil::civmil_invalid && !m_gat && m_constant == constant_undefined)
		return Condition::ptr_t(new ConditionFlight(get_childnum(), m_flight));
	if (m_flight == ConditionFlight::flight_invalid && m_civmil != ConditionCivMil::civmil_invalid && !m_gat && m_constant == constant_undefined)
		return Condition::ptr_t(new ConditionCivMil(get_childnum(), m_civmil));
	if (m_flight == ConditionFlight::flight_invalid && m_civmil == ConditionCivMil::civmil_invalid && m_gat && m_constant == constant_undefined)
		return Condition::ptr_t(new ConditionGeneralAviation(get_childnum()));
	if (m_flight == ConditionFlight::flight_invalid && m_civmil == ConditionCivMil::civmil_invalid && !m_gat && m_constant != constant_undefined)
		return Condition::ptr_t(new ConditionConstant(get_childnum(), m_constant == constant_true));
	error("invalid Fcs");
	return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
}

const std::string TrafficFlowRestrictions::Loader::PNodeFcc::elementname("Fcc");

TrafficFlowRestrictions::Loader::PNodeFcc::PNodeFcc(Loader& loader, unsigned int childnum, const AttributeList& properties)
	: PNodeNoIgnore(loader, childnum), m_mode(mode_invalid)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "codeOpr")
			continue;
		if (i->value == "AND") {
			m_mode = mode_and;
		} else if (i->value == "OR") {
			m_mode = mode_or;
		} else if (i->value == "ANDNOT") {
			m_mode = mode_andnot;
		} else if (i->value == "NONE") {
			m_mode = mode_none;
		} else if (i->value == "SEQ") {
			m_mode = mode_seq;
		} else {
			error(get_element_name() + " node with invalid codeOpr attribute: " + i->value);
		}
	}
	if (m_mode == mode_invalid)
		error(get_element_name() + " node with missing codeOpr attribute");
}

void TrafficFlowRestrictions::Loader::PNodeFcc::on_subelement(const PNodeFcc::ptr_t& el)
{
	m_conds.push_back(el->get_cond());
}

void TrafficFlowRestrictions::Loader::PNodeFcc::on_subelement(const PNodeFce::ptr_t& el)
{
	m_conds.push_back(el->get_cond());
}

  
TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Loader::PNodeFcc::get_cond(void)
{
	switch (m_mode) {
	case mode_none:
		if (m_conds.size() != 1)
			error(get_element_name() + " node with codeOpr NONE and not exactly 1 child");
		break;

	case mode_andnot:
		if (m_conds.size() > 2)
			warning(get_element_name() + " node with codeOpr ANDNOT and more than 2 children");
		/* fall through */

	case mode_and:
	case mode_or:
	case mode_seq:
		if (m_conds.size() < 2)
			error(get_element_name() + " node with codeOpr logic or SEQ and less than 2 children");
		break;

	default:
		error(get_element_name() + " node with missing codeOpr attribute");
	}
	bool resinv(false), oprinv(false), oprinv2(false);
       	switch (m_mode) {
	case mode_none:
		for (conds_t::const_iterator ci(m_conds.begin()), ce(m_conds.end()); ci != ce; ++ci) {
			if (*ci)
				return *ci;
			warning(get_element_name() + " node NONE: ignoring null condition");
		}
		return Condition::ptr_t();

	case mode_seq:
	{
		Glib::RefPtr<ConditionSequence> cc(new ConditionSequence(get_childnum()));
		Condition::ptr_t c1;
		unsigned int ccount(0);
		for (conds_t::const_iterator ci(m_conds.begin()), ce(m_conds.end()); ci != ce; ++ci) {
			if (!*ci) {
				warning(get_element_name() + " node SEQ: ignoring null condition");
				continue;
			}
			if (!c1)
				c1 = *ci;
			++ccount;
			cc->add(*ci);
		}
		if (ccount < 2)
			return c1;
		return cc;
	}
	case mode_and:
		resinv = false;
		oprinv = false;
		oprinv2 = false;
		break;

	case mode_or:
		resinv = true;
		oprinv = true;
		oprinv2 = true;
		break;

	case mode_andnot:
		resinv = false;
		oprinv = false;
		oprinv2 = true;
		break;

	default:
		return Condition::ptr_t();
	}
	{
		Glib::RefPtr<ConditionAnd> cc(new ConditionAnd(get_childnum(), resinv));
		Condition::ptr_t c1;
		unsigned int ccount(0);
		for (conds_t::const_iterator ci(m_conds.begin()), ce(m_conds.end()); ci != ce; ++ci, oprinv = oprinv2) {
			if (!*ci) {
				warning(get_element_name() + " node logic: ignoring null condition");
				continue;
			}
			if (!c1)
				c1 = *ci;
			++ccount;
			cc->add(*ci, oprinv);
		}
		if (!ccount)
			return c1;
		if (ccount == 1 && m_mode != mode_andnot)
			return c1;
		return cc;
	}
}

const std::string TrafficFlowRestrictions::Loader::PNodeFce::elementname("Fce");

TrafficFlowRestrictions::Loader::PNodeFce::PNodeFce(Loader& loader, unsigned int childnum, const AttributeList& properties)
	: PNodeNoIgnore(loader, childnum), m_dctlimit(std::numeric_limits<double>::quiet_NaN()), m_refloc(false), m_mode(mode_unspec)
{
	{
		Point pt;
		pt.set_invalid();
		m_point[0].set_coord(pt);
		m_point[1].set_coord(pt);
	}
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name == "codeRefLoc") {
			m_refloc = i->value == "Y";
			if (!m_refloc && i->value != "N")
				error("invalid codeRefLoc attribute: " + i->value);
			continue;
		}
		if (i->name != "codeRelWithLoc")
			continue;
		if (i->value == "DEP") {
			m_mode = mode_dep;
		} else if (i->value == "ARR") {
			m_mode = mode_arr;
		} else if (i->value == "XNG") {
			m_mode = mode_crossingaspc1;
		} else if (i->value == "ACT") {
			m_mode = mode_aspcactive;
		} else if (i->value == "AVB") {
			m_mode = mode_awyavailable;
		} else {
			error("invalid codeRelWithLoc attribute: " + i->value);
		}
	}
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeFcl::ptr_t& el)
{
	if (!el->is_lower_valid() && !el->is_upper_valid()) {
		error(get_element_name() + " node with invalid Fcl");
	}
	if (m_altrange.is_valid()) {
		error(get_element_name() + " node with duplicate Fcl");
	}
	m_altrange = el->get_altrange();
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeAhpUid::ptr_t& el)
{
	if (m_mode == mode_crossingaspc1) {
		warning(get_element_name() + " node with nonsensical crossing AhpUid child");
		AirportsDb::Airport arpt(m_loader.find_airport(el->get_ident()));
                m_point[0] = RuleWpt(Vertex::type_airport, el->get_ident(), arpt.get_coord());
		if (!arpt.is_valid()) {
			Point pt;
			pt.set_invalid();
			m_point[0].set_coord(pt);
                        error("cannot find airport " + el->get_ident());
		}
		m_mode = mode_crossingpoint;
                return;
	}
	if (m_mode != mode_arr && m_mode != mode_dep)
		error(get_element_name() + " node with AhpUid child but not DEP/ARR mode");
	if (!m_ident.empty())
		error(get_element_name() + " node with duplicate AhpUid");
	m_ident = el->get_ident();
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeAseUid::ptr_t& el)
{
	if (el->get_asetype() != PNodeAseUid::asetype_normal)
		error(get_element_name() + " node with invalid " + el->get_element_name() + " child");
	if (m_mode == mode_dep || m_mode == mode_depaspc) {
		m_mode = mode_depaspc;
	} else if (m_mode == mode_arr || m_mode == mode_arraspc) {
		m_mode = mode_arraspc;
	} else if (m_mode != mode_crossingaspc1 && m_mode != mode_aspcactive) {
		error(get_element_name() + " node with AseUid child but invalid mode");
	}
	if (m_airspace[0])
		error(get_element_name() + " node with duplicate AseUid children");
	m_airspace[0] = m_loader.find_airspace(el->get_ident(), el->get_type());
	if (!m_airspace[0])
		warning("cannot find airspace " + el->get_ident() + "/" + el->get_type());
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeAbc::ptr_t& el)
{
	if (m_mode == mode_crossingaspc1) {
		m_mode = mode_crossingaspc2;
	} else if (m_mode != mode_crossingaspc2) {
		error(get_element_name() + " node with Abc child but invalid mode");
	}
	for (unsigned int i = 0; i < 2; ++i) {
		if (m_airspace[i])
			error(get_element_name() + " node with duplicate Abc children");
		m_airspace[i] = m_loader.find_airspace(el->get_ident(i), el->get_type(i));
		if (!m_airspace[i])
			warning("cannot find airspace " + el->get_ident(i) + "/" + el->get_type(i));
	}
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeDfl::ptr_t& el)
{
	if (m_mode != mode_unspec)
		error(get_element_name() + " node with Dfl child but invalid mode");
	m_mode = mode_dctlimit;
	m_dctlimit = el->get_limit();
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeDct::ptr_t& el)
{
	if (m_mode == mode_crossingaspc1) {
		m_mode = mode_crossingdct;
	} else {
		error(get_element_name() + " node with Dct child but invalid mode");
	}
	for (unsigned int i = 0; i < 2; ++i)
		m_point[i] = el->operator[](i);
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeRpn::ptr_t& el)
{
	if (m_mode == mode_crossingaspc1) {
		m_mode = mode_crossingawy;
	} else if (m_mode != mode_awyavailable) {
		error(get_element_name() + " node with Rpn child but invalid mode");
	}
	if (!m_ident.empty())
		error(get_element_name() + " node with duplicate Rpn children");
	m_ident = el->get_ident();
	for (unsigned int i = 0; i < 2; ++i)
		m_point[i] = el->operator[](i);
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodePoint::ptr_t& el)
{
	if (el->get_ptype() != PNodePoint::pt_spn)
		error(get_element_name() + " node with invalid " + el->get_element_name() + " child");
	if (m_mode == mode_crossingaspc1) {
		m_mode = mode_crossingpoint;
	} else {
		error(get_element_name() + " node with *UidSpn child but invalid mode");
	}
	m_point[0] = el->get_point();
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeAcs::ptr_t& el)
{
	if (m_mode == mode_unspec) {
		m_mode = mode_cond;
	} else {
		error(get_element_name() + " node with Acs child but invalid mode");
	}
	m_cond = el->get_cond();
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeFcs::ptr_t& el)
{
	if (m_mode == mode_unspec) {
		m_mode = mode_cond;
	} else {
		error(get_element_name() + " node with Fcs child but invalid mode");
	}
	m_cond = el->get_cond();
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeSidUid::ptr_t& el)
{
	if (m_mode == mode_crossingaspc1) {
		m_mode = mode_sid;
	} else {
		error(get_element_name() + " node with SidUid child but invalid mode");
	}
	m_ident = el->get_ident();
	m_airport = el->get_airport();
}

void TrafficFlowRestrictions::Loader::PNodeFce::on_subelement(const PNodeSiaUid::ptr_t& el)
{
	if (m_mode == mode_crossingaspc1) {
		m_mode = mode_star;
	} else {
		error(get_element_name() + " node with SiaUid child but invalid mode");
	}
	m_ident = el->get_ident();
	m_airport = el->get_airport();
}

TrafficFlowRestrictions::Condition::ptr_t TrafficFlowRestrictions::Loader::PNodeFce::get_cond(void)
{
	switch (m_mode) {
	case mode_dep:
	case mode_arr:
	case mode_depaspc:
	case mode_arraspc:
	case mode_dctlimit:
	case mode_cond:
	case mode_aspcactive:
	case mode_sid:
	case mode_star:
		if (m_altrange.is_valid())
			error(get_element_name() + " node DEP/ARR/DFL/ACS with altitude range");
		break;

	case mode_crossingaspc1:
	case mode_crossingaspc2:
		break;

	case mode_crossingawy:
	case mode_awyavailable:
	case mode_crossingdct:
	case mode_crossingpoint:
		if (m_altrange.is_lower_valid() + m_altrange.is_upper_valid() == 1)
			error(get_element_name() + " node XNG/AVB with invalid altitude range");
		break;

	default:
	case mode_unspec:
		error(get_element_name() + " node without codeRelWithLoc attribute: cannot determine mode");
	}
	if (m_refloc) {
		switch (m_mode) {
		case mode_dep:
		case mode_arr:
		case mode_depaspc:
		case mode_arraspc:
		case mode_sid:
		case mode_star:
		case mode_crossingaspc1:
		case mode_crossingaspc2:
		case mode_crossingawy:
		case mode_crossingdct:
		case mode_crossingpoint:
			break;

		default:
			error(get_element_name() + " node cannot be used as reference location");
		}
	}
	switch (m_mode) {
	case mode_dep:
	case mode_arr:
	{
		if (m_ident.empty())
			error(get_element_name() + " node DEP/ARR but no airport");
		AirportsDb::Airport arpt(m_loader.find_airport(m_ident));
		Point pt(arpt.get_coord());
		if (!arpt.is_valid()) {
			pt.set_invalid();
			warning("Airport " + m_ident + " not found");
		} else if (Condition::ifr_refloc && m_refloc) {
			bool alwaysfalse(false);
			if (m_mode == mode_dep &&
			    (arpt.get_flightrules() & (AirportsDb::Airport::flightrules_dep_vfr | AirportsDb::Airport::flightrules_dep_ifr)) ==
			    AirportsDb::Airport::flightrules_dep_vfr) {
				warning("VFR only Airport " + m_ident + " departure RefLoc");
				alwaysfalse = true;
			}
			if (m_mode == mode_arr &&
			    (arpt.get_flightrules() & (AirportsDb::Airport::flightrules_arr_vfr | AirportsDb::Airport::flightrules_arr_ifr)) ==
			    AirportsDb::Airport::flightrules_arr_vfr) {
				warning("VFR only Airport " + m_ident + " arrival RefLoc");
				alwaysfalse = true;
			}
			if (alwaysfalse)
				return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionDepArr(get_childnum(), RuleWpt(Vertex::type_airport, m_ident, pt, false, false), m_mode == mode_arr, m_refloc));
	}

	case mode_depaspc:
	case mode_arraspc:
		if (!m_airspace[0]) {
			warning(get_element_name() + " node DEP/ARR but no airspace");
			//error(get_element_name() + " node DEP/ARR but no airspace");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionDepArrAirspace(get_childnum(), m_airspace[0], m_mode == mode_arr, m_refloc));

	case mode_crossingaspc1:
		if (!m_airspace[0]) {
			warning(get_element_name() + " node XNG but no airspace");
			//error(get_element_name() + " node XNG but no airspace");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionCrossingAirspace1(get_childnum(), m_altrange, m_airspace[0], m_refloc));

	case mode_crossingaspc2:
		if (!m_airspace[0] || !m_airspace[1]) {
			warning(get_element_name() + " node XNG but no airspaces");
			//error(get_element_name() + " node XNG but no airspaces");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionCrossingAirspace2(get_childnum(), m_altrange, m_airspace[0], m_airspace[1], m_refloc));

	case mode_aspcactive:
		if (!m_airspace[0]) {
			warning(get_element_name() + " node XNG but no airspace");
			//error(get_element_name() + " node XNG but no airspace");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionCrossingAirspaceActive(get_childnum(), m_airspace[0]));

	case mode_crossingawy:
		if (m_point[0].get_coord().is_invalid() || m_point[1].get_coord().is_invalid() || m_ident.empty()) {
			warning(get_element_name() + " node XNG but no airway");
			//error(get_element_name() + " node XNG but no airway");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionCrossingAirway(get_childnum(), m_altrange, m_point[0], m_point[1], m_ident, m_refloc));

	case mode_awyavailable:
		if (m_point[0].get_coord().is_invalid() || m_point[1].get_coord().is_invalid() || m_ident.empty()) {
			warning(get_element_name() + " node AVB but no airway");
			//error(get_element_name() + " node AVB but no airway");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionCrossingAirwayAvailable(get_childnum(), m_altrange, m_point[0], m_point[1], m_ident));

	case mode_crossingdct:
		if (m_point[0].get_coord().is_invalid() || m_point[1].get_coord().is_invalid()) {
			warning(get_element_name() + " node XNG but no Dct points");
			//error(get_element_name() + " node XNG but no Dct points");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionCrossingDct(get_childnum(), m_altrange, m_point[0], m_point[1], m_refloc));

	case mode_crossingpoint:
		if (m_point[0].get_coord().is_invalid()) {
			warning(get_element_name() + " node XNG but no point");
			//error(get_element_name() + " node XNG but no point");
			return Condition::ptr_t(new ConditionConstant(get_childnum(), false));
		}
		return Condition::ptr_t(new ConditionCrossingPoint(get_childnum(), m_altrange, m_point[0], m_refloc));

	case mode_dctlimit:
		if (std::isnan(m_dctlimit))
			error(get_element_name() + " node Dfl but no direct-to limit");
		return Condition::ptr_t(new ConditionDctLimit(get_childnum(), m_dctlimit));

	case mode_sid:
	case mode_star:
	{
		AirportsDb::Airport arpt(m_loader.find_airport(m_airport));
		Point pt(arpt.get_coord());
		if (!arpt.is_valid()) {
			pt.set_invalid();
			warning("Airport " + m_airport + " not found");
		}
		return Condition::ptr_t(new ConditionSidStar(get_childnum(), RuleWpt(Vertex::type_airport, m_airport, pt, false, false), m_ident, m_mode == mode_star, m_refloc));
	}

	case mode_cond:
		if (!m_cond) {
			warning(get_element_name() + " node Acs with invalid condition");
			//error(get_element_name() + " node Acs with invalid condition");
		}
		return m_cond;

	default:
		return Condition::ptr_t();
	}
}

const std::string TrafficFlowRestrictions::Loader::PNodeTfr::elementname("Tfr");

TrafficFlowRestrictions::Loader::PNodeTfr::PNodeTfr(Loader& loader, unsigned int childnum)
	: PNodeNoIgnore(loader, childnum), m_tfr(new TrafficFlowRule())
{
}

void TrafficFlowRestrictions::Loader::PNodeTfr::on_subelement(const PNodeText::ptr_t& el)
{
	if (!m_tfr)
		error("Tfr node has no TrafficFlowRule");
	switch (el->get_type()) {
	case PNodeText::tt_codetype:
		m_tfr->set_codetype(el->get_text());
		break;

	case PNodeText::tt_txtoprgoal:
		m_tfr->set_oprgoal(el->get_text());
		break;

	case PNodeText::tt_txtdescr:
		m_tfr->set_descr(el->get_text());
		break;

	default:
		error("Tfr node has invalid text subelement " + el->get_element_name());
	}
}

void TrafficFlowRestrictions::Loader::PNodeTfr::on_subelement(const PNodeTfrUid::ptr_t& el)
{
	if (!m_tfr)
		error("Tfr node has no TrafficFlowRule");
	m_tfr->set_mid(el->get_mid());
	m_tfr->set_codeid(el->get_codeid());
}

void TrafficFlowRestrictions::Loader::PNodeTfr::on_subelement(const PNodeTft::ptr_t& el)
{
	if (!m_tfr)
		error("Tfr node has no TrafficFlowRule");
	if (!el->get_timesheet().is_valid())
		error("Tfr node has invalid Tft subelement " + el->get_timesheet().to_str());
	m_tfr->set_timesheet(el->get_timesheet());
}

void TrafficFlowRestrictions::Loader::PNodeTfr::on_subelement(const PNodeTre::ptr_t& el)
{
	if (!m_tfr)
		error("Tfr node has no TrafficFlowRule");
	const RestrictionSequence& seq(el->get_sequence());
	if (!seq.size())
		return;
	for (unsigned int i = 0; i < seq.size(); ++i)
		if (!seq[i])
			return;
	m_tfr->add_restriction(seq);
}

void TrafficFlowRestrictions::Loader::PNodeTfr::on_subelement(const PNodeFcc::ptr_t& el)
{
	if (!m_tfr)
		error("Tfr node has no TrafficFlowRule");
	Condition::ptr_t c(el->get_cond());
	if (!c)
		return;
	if (m_tfr->get_condition())
		error("Tfr node has duplicate conditions");
	m_tfr->set_condition(c);
}

void TrafficFlowRestrictions::Loader::PNodeTfr::on_subelement(const PNodeFce::ptr_t& el)
{
	if (!m_tfr)
		error("Tfr node has no TrafficFlowRule");
	Condition::ptr_t c(el->get_cond());
	if (!c)
		return;
	if (m_tfr->get_condition())
		error("Tfr node has duplicate conditions");
	m_tfr->set_condition(c);
}

const std::string TrafficFlowRestrictions::Loader::PNodeAIXMSnapshot::elementname("AIXM-Snapshot");

TrafficFlowRestrictions::Loader::PNodeAIXMSnapshot::PNodeAIXMSnapshot(Loader& loader, unsigned int childnum, const AttributeList& properties)
	: PNode(loader, childnum)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (!i->value.empty()) {
			if (i->name == "origin") {
				m_loader.get_tfrs().set_origin(i->value);
				continue;
			}
			if (i->name == "created") {
				m_loader.get_tfrs().set_created(i->value);
				continue;
			}
			if (i->name == "effective") {
				m_loader.get_tfrs().set_effective(i->value);
				continue;
			}
		}
	}
}

void TrafficFlowRestrictions::Loader::PNodeAIXMSnapshot::on_subelement(const PNodeTfr::ptr_t& el)
{
	TrafficFlowRule::ptr_t tfr(el->get_tfr());
	if (!tfr)
		error(el, "No rule");
	switch (tfr->get_codetype()) {
	case TrafficFlowRule::codetype_closed:
		if (!tfr->get_restrictions().empty())
			warning(el, tfr->get_codeid() + ": rule is closed for cruising, but has restriction elements");
		tfr->get_restrictions().clear();
		break;

	case TrafficFlowRule::codetype_mandatory:
		if (tfr->get_restrictions().empty() && !tfr->is_dct()) {
			warning(el, tfr->get_codeid() + ": rule is mandatory, but has no restriction elements");
			m_loader.disable_rule(tfr);
		}
		break;

	case TrafficFlowRule::codetype_forbidden:
		if (tfr->get_restrictions().empty()) {
			warning(el, tfr->get_codeid() + ": rule is forbidden, but has no restriction elements");
			m_loader.disable_rule(tfr);
		}
		break;

	default:
		warning(el, tfr->get_codeid() + ": rule has invalid codetype");
		return;
	}
	if (!tfr->get_condition()) {
		warning(el, tfr->get_codeid() + ": rule has no condition");
		tfr->set_condition(Condition::ptr_t(new ConditionConstant(get_childnum(), true)));
	}
	m_loader.get_tfrs().m_loadedtfr.push_back(tfr);
	//std::cerr << "PNodeAIXMSnapshot::on_subelement: Tfr: " << tfr->get_codeid() << std::endl;
}

TrafficFlowRestrictions::Loader::Loader(TrafficFlowRestrictions& tfrs, AirportsDb& airportdb, NavaidsDb& navaiddb,
					WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
	: m_dbldr(tfrs, sigc::mem_fun(*this, &Loader::message), airportdb, navaiddb, waypointdb, airwaydb, airspacedb)
{
}

TrafficFlowRestrictions::Loader::~Loader()
{
}

std::string TrafficFlowRestrictions::Loader::get_stackpath(void) const
{
	std::string r;
	for (parsestack_t::const_reverse_iterator pi(m_parsestack.rbegin()), pe(m_parsestack.rend()); pi != pe; ) {
		std::ostringstream oss;
		oss << (*pi)->get_element_name();
		PNodeTfr::ptr_t tfr(PNodeTfr::ptr_t::cast_dynamic(*pi));
		++pi;
		if (pi != pe && (*pi)->get_childcount() != 1)
			oss << '[' << ((int)(*pi)->get_childcount() - 1) << ']';
		if (tfr && tfr->get_tfr() && !tfr->get_tfr()->get_codeid().empty())
			oss << '{' << tfr->get_tfr()->get_codeid() << '}';
		if (!r.empty())
			oss << '/' << r;
		r = oss.str();
	}
	return r;
}

void TrafficFlowRestrictions::Loader::message(Message::type_t mt, const std::string& child, const std::string& text) const
{
	std::string str("TFR Loader: ");
	std::string strm(str);
	std::string ruleid;
	{
		std::string tos(get_stackpath());
		if (!tos.empty()) {
			str += tos + ": ";
			std::string::size_type n1(tos.find('{'));
			if (n1 != std::string::npos) {
				std::string::size_type n2(tos.find('}', n1 + 1));
				if (n2 != std::string::npos) {
					ruleid = tos.substr(n1 + 1, n2 - n1 - 1);
					tos.erase(n1, n2 - n1 + 1);
				}
			}
			strm += tos + ": ";
		}
	}
	if (!child.empty()) {
		str += child + ": ";
		strm += child + ": ";
	}
	str += text;
	strm += text;
	get_tfrs().message(strm, mt, ruleid);
	if (true)
		std::cerr << str << std::endl;
	if (mt == Message::type_error)
		throw std::runtime_error(str);	
}

void TrafficFlowRestrictions::Loader::error(const std::string& text) const
{
	error("", text);
}

void TrafficFlowRestrictions::Loader::error(const std::string& child, const std::string& text) const
{
	message(Message::type_error, child, text);
}

void TrafficFlowRestrictions::Loader::warning(const std::string& text) const
{
	warning("", text);
}

void TrafficFlowRestrictions::Loader::warning(const std::string& child, const std::string& text) const
{
	message(Message::type_warning, child, text);
}

void TrafficFlowRestrictions::Loader::info(const std::string& text) const
{
	info("", text);
}

void TrafficFlowRestrictions::Loader::info(const std::string& child, const std::string& text) const
{
	message(Message::type_info, child, text);
}

void TrafficFlowRestrictions::Loader::disable_rule(const std::string& ruleid)
{
	if (!ruleid.empty())
		get_tfrs().disabledrules_add(ruleid);
}

void TrafficFlowRestrictions::Loader::on_start_document(void)
{
	m_parsestack.clear();
}

void TrafficFlowRestrictions::Loader::on_end_document(void)
{
	if (!m_parsestack.empty()) {
		if (true)
			std::cerr << "TFR Loader: parse stack not empty at end of document" << std::endl;
		error("parse stack not empty at end of document");
	}
}

void TrafficFlowRestrictions::Loader::on_start_element(const Glib::ustring& name, const AttributeList& properties)
{
	unsigned int childnum(0);
	if (m_parsestack.empty()) {
		if (name == PNodeAIXMSnapshot::get_static_element_name()) {
			if (false)
				std::cerr << ">> " << PNodeAIXMSnapshot::get_static_element_name() << std::endl;
			PNode::ptr_t p(new PNodeAIXMSnapshot(*this, childnum, properties));
			m_parsestack.push_back(p);
			return;
		}
		error("document must begin with " + PNodeAIXMSnapshot::get_static_element_name());
	}
	childnum = m_parsestack.back()->get_childcount();
	if (m_parsestack.back()->is_ignore()) {
		if (false)
			std::cerr << ">> ignore: " << name << std::endl;
		PNode::ptr_t p(new PNodeIgnore(*this, childnum, name));
		m_parsestack.push_back(p);
		return;
	}
	// PNode factory
	{
		PNodeText::texttype_t tt(PNodeText::find_type(name));
		if (tt != PNodeText::tt_invalid) {
			if (false)
				std::cerr << ">> " << PNodeText::get_static_element_name(tt) << std::endl;
			PNode::ptr_t p(new PNodeText(*this, childnum, tt));
			m_parsestack.push_back(p);
			return;
		}
	}

	if (name == PNodeRteUid::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeRteUid::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeRteUid(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeAhpUid::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeAhpUid::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeAhpUid(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	{
		PNodeAseUid::asetype_t at(PNodeAseUid::find_type(name));
		if (at != PNodeAseUid::asetype_invalid) {
			if (false)
				std::cerr << ">> " << PNodeAseUid::get_static_element_name(at) << std::endl;
			PNode::ptr_t p(new PNodeAseUid(*this, childnum, at));
			m_parsestack.push_back(p);
			return;
		}
	}
	if (name == PNodeSidUid::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeSidUid::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeSidUid(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeSiaUid::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeSiaUid::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeSiaUid(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeTfrUid::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeTfrUid::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeTfrUid(*this, childnum, properties));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeTimsh::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeTimsh::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeTimsh(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeTft::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeTft::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeTft(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeFcl::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeFcl::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeFcl(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeTfl::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeTfl::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeTfl(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	{
		PNodePoint::type_t t(PNodePoint::find_type(name));
		if (t.first != PNodePoint::pt_invalid && t.second != PNodePoint::nav_invalid) {
			if (false)
				std::cerr << ">> " << PNodePoint::get_static_element_name(t.first, t.second) << std::endl;
			PNode::ptr_t p(new PNodePoint(*this, childnum, t.first, t.second));
			m_parsestack.push_back(p);
			return;
		}
	}
	if (name == PNodeDct::get_static_element_name(false)) {
		if (false)
			std::cerr << ">> " << PNodeDct::get_static_element_name(false) << std::endl;
		PNode::ptr_t p(new PNodeDct(*this, childnum, false));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeDct::get_static_element_name(true)) {
		if (false)
			std::cerr << ">> " << PNodeDct::get_static_element_name(true) << std::endl;
		PNode::ptr_t p(new PNodeDct(*this, childnum, true));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeRpn::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeRpn::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeRpn(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeTfe::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeTfe::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeTfe(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeTre::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeTre::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeTre(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeDfl::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeDfl::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeDfl(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeAbc::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeAbc::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeAbc(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeAcs::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeAcs::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeAcs(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeFcs::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeFcs::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeFcs(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeFcc::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeFcc::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeFcc(*this, childnum, properties));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeFce::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeFce::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeFce(*this, childnum, properties));
		m_parsestack.push_back(p);
		return;
	}
	if (name == PNodeTfr::get_static_element_name()) {
		if (false)
			std::cerr << ">> " << PNodeTfr::get_static_element_name() << std::endl;
		PNode::ptr_t p(new PNodeTfr(*this, childnum));
		m_parsestack.push_back(p);
		return;
	}
	// finally: ignoring element
	{
		if (false)
			std::cerr << ">> ignore: " << name << std::endl;
		PNode::ptr_t p(new PNodeIgnore(*this, childnum, name));
		m_parsestack.push_back(p);
		return;
	}
}

void TrafficFlowRestrictions::Loader::on_end_element(const Glib::ustring& name)
{
	if (m_parsestack.empty())
		error("parse stack underflow on end element");
	PNode::ptr_t p(m_parsestack.back());
	m_parsestack.pop_back();
	if (false)
		std::cerr << "<< " << p->get_element_name() << std::endl;
	if (p->get_element_name() != name)
		error("end element name " + name + " does not match expected name " + p->get_element_name());
	if (!m_parsestack.empty()) {
		m_parsestack.back()->on_subelement(p);
		return;
	}
	{
		PNodeAIXMSnapshot::ptr_t px(PNodeAIXMSnapshot::ptr_t::cast_dynamic(p));
		if (!px)
			error("top node is not an AIXMSnapshot");
	}
}

void TrafficFlowRestrictions::Loader::on_characters(const Glib::ustring& characters)
{
	if (!m_parsestack.empty())
		m_parsestack.back()->on_characters(characters);
}

void TrafficFlowRestrictions::Loader::on_comment(const Glib::ustring& text)
{
}

void TrafficFlowRestrictions::Loader::on_warning(const Glib::ustring& text)
{
	warning("Parser: Warning: " + text);
}

void TrafficFlowRestrictions::Loader::on_error(const Glib::ustring& text)
{
        warning("Parser: Error: " + text);
}

void TrafficFlowRestrictions::Loader::on_fatal_error(const Glib::ustring& text)
{
        error("Parser: Fatal Error: " + text);
}

bool TrafficFlowRestrictions::add_rules(std::vector<Message>& msg, const std::string& fname, AirportsDb& airportdb,
					NavaidsDb& navaiddb, WaypointsDb& waypointdb, AirwaysDb& airwaydb, AirspacesDb& airspacedb)
{
	msg.clear();
        try {
		TrafficFlowRestrictions::Loader ldr(*this, airportdb, navaiddb, waypointdb, airwaydb, airspacedb);
		//ldr.fill_airport_cache();
		ldr.set_validate(false); // Do not validate, we do not have a DTD
		ldr.set_substitute_entities(true);
		ldr.parse_file(fname);
        } catch (const xmlpp::exception& ex) {
		message(std::string("libxml++ exception: ") + ex.what(), Message::type_error);
 		msg.swap(m_messages);
                return false;
        } catch (const std::exception& ex) {
		message(std::string("exception: ") + ex.what(), Message::type_error);
		msg.swap(m_messages);
                return false;
        }
	msg.swap(m_messages);
	return true;
}
