#ifndef ADRAUPPARSE_H
#define ADRAUPPARSE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <libxml++/libxml++.h>

#include "adr.hh"
#include "adraup.hh"
#include "adrdb.hh"
#include "adrpoint.hh"
#include "adrroute.hh"
#include "adrairspace.hh"

#include "adraerodrome.hh"
#include "adrunit.hh"
#include "adrrestriction.hh"

namespace ADR {

class ParseAUP : public xmlpp::SaxParser {
public:
	ParseAUP(Database& db, bool verbose = true);
	virtual ~ParseAUP();

	void error(const std::string& text);
	void error(const std::string& child, const std::string& text);
	void warning(const std::string& text);
	void warning(const std::string& child, const std::string& text);
	void info(const std::string& text);
	void info(const std::string& child, const std::string& text);

	unsigned int get_errorcnt(void) const { return m_errorcnt; }
	unsigned int get_warncnt(void) const { return m_warncnt; }

	Database& get_db(void) { return m_db; }

	void process(bool verbose);
	void save(AUPDatabase& aupdb);

	class AUPRouteSegmentTimeSlice;
        class AUPIdentTimeSlice;
        class AUPRouteTimeSlice;
        class AUPDesignatedPointTimeSlice;
        class AUPNavaidTimeSlice;
        class AUPAirspaceTimeSlice;

	class AUPLink {
	public:
		AUPLink(const std::string& id = "", const Link& l = Link()) : m_link(l), m_id(id) {}

		const Link& get_link(void) const { return m_link; }
		void set_link(const Link& l) { m_link = l; }

		const std::string& get_id(void) const { return m_id; }
		void set_id(const std::string& id) { m_id = id; }

	protected:
		Link m_link;
		std::string m_id;
	};

	class AUPSegmentLink : public Link {
	public:
		AUPSegmentLink(const Link& l = Link(), bool backward = false) : Link(l), m_backward(backward) {}
		bool is_forward(void) const { return !m_backward; }
		bool is_backward(void) const { return m_backward; }
		void set_backward(bool b) { m_backward = b; }
		void set_forward(bool f) { set_backward(!f); }

	protected:
		bool m_backward;
	};

	class AUPTimeSlice {
	public:
		AUPTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);
		virtual ~AUPTimeSlice() {}
		timetype_t get_starttime(void) const { return m_time.first; }
		timetype_t get_endtime(void) const { return m_time.second; }
		const timepair_t& get_timeinterval(void) const { return m_time; }
		void set_starttime(timetype_t t) { m_time.first = t; }
		void set_endtime(timetype_t t) { m_time.second = t; }
		bool is_snapshot(void) const { return get_starttime() && get_starttime() == get_endtime(); }
		bool is_valid(void) const { return get_starttime() < get_endtime(); }
		bool is_inside(timetype_t tm) const { return get_starttime() <= tm && tm < get_endtime(); }
		bool is_overlap(timetype_t tm0, timetype_t tm1) const;
		bool is_overlap(const TimeSlice& ts) const { return is_overlap(ts.get_starttime(), ts.get_endtime()); }
		bool is_overlap(const timepair_t& tm) const { return is_overlap(tm.first, tm.second); }
		timetype_t get_overlap(timetype_t tm0, timetype_t tm1) const;
		timetype_t get_overlap(const TimeSlice& ts) const { return get_overlap(ts.get_starttime(), ts.get_endtime()); }
		timetype_t get_overlap(const timepair_t& tm) const { return get_overlap(tm.first, tm.second); }

		virtual AUPRouteSegmentTimeSlice& as_routesegment(void);
		virtual const AUPRouteSegmentTimeSlice& as_routesegment(void) const;
		virtual AUPIdentTimeSlice& as_ident(void);
		virtual const AUPIdentTimeSlice& as_ident(void) const;
		virtual AUPRouteTimeSlice& as_route(void);
		virtual const AUPRouteTimeSlice& as_route(void) const;
		virtual AUPDesignatedPointTimeSlice& as_designatedpoint(void);
		virtual const AUPDesignatedPointTimeSlice& as_designatedpoint(void) const;
		virtual AUPNavaidTimeSlice& as_navaid(void);
		virtual const AUPNavaidTimeSlice& as_navaid(void) const;
		virtual AUPAirspaceTimeSlice& as_airspace(void);
		virtual const AUPAirspaceTimeSlice& as_airspace(void) const;

		virtual std::ostream& print(std::ostream& os) const;

	protected:
		timepair_t m_time;
	};

	class AUPObject {
	public:
		typedef Glib::RefPtr<AUPObject> ptr_t;
		typedef Glib::RefPtr<const AUPObject> const_ptr_t;

		AUPObject(const std::string& id = "", const Link& link = Link());
		virtual ~AUPObject();

		typedef enum {
			type_first,
			type_routesegment = type_first,
			type_route,
			type_navaid,
			type_designatedpoint,
			type_airspace,
			type_invalid,
			type_last
		} type_t;
		virtual type_t get_type(void) const { return type_invalid; }

		void reference(void) const;
		void unreference(void) const;
		gint get_refcount(void) const { return m_refcount; }
		ptr_t get_ptr(void) { reference(); return ptr_t(this); }
		const_ptr_t get_ptr(void) const { reference(); return const_ptr_t(this); }

		const AUPLink& get_auplink(void) const { return m_link; }
		AUPLink& get_auplink(void) { return m_link; }
		void set_auplink(const AUPLink& l) { m_link = l; }

		const Link& get_link(void) const { return get_auplink().get_link(); }
		void set_link(const Link& l) { get_auplink().set_link(l); }

		const std::string& get_id(void) const { return get_auplink().get_id(); }
		void set_id(const std::string& id) { get_auplink().set_id(id); }

		bool empty(void) const;
		unsigned int size(void) const;
		AUPTimeSlice& operator[](unsigned int idx);
		const AUPTimeSlice& operator[](unsigned int idx) const;
		// find timeslice containing given time
		AUPTimeSlice& operator()(timetype_t tm);
		const AUPTimeSlice& operator()(timetype_t tm) const;
		// find best overlap
		AUPTimeSlice& operator()(timetype_t tm0, timetype_t tm1);
		const AUPTimeSlice& operator()(timetype_t tm0, timetype_t tm1) const;
		AUPTimeSlice& operator()(const AUPTimeSlice& ts);
		const AUPTimeSlice& operator()(const AUPTimeSlice& ts) const;

		virtual std::ostream& print(std::ostream& os) const;

		static const AUPTimeSlice invalid_timeslice;

	protected:
		AUPLink m_link;
		mutable gint m_refcount;

		virtual bool ts_empty(void) const { return true; }
		virtual unsigned int ts_size(void) const { return 0; }
		virtual AUPTimeSlice& ts_get(unsigned int idx);
		virtual const AUPTimeSlice& ts_get(unsigned int idx) const;
	};

	template<class TS = AUPTimeSlice> class AUPObjectImpl : public AUPObject {
	public:
		typedef TS TimeSliceImpl;
		typedef Glib::RefPtr<AUPObjectImpl> ptr_t;
		typedef Glib::RefPtr<const AUPObjectImpl> const_ptr_t;
		typedef AUPObjectImpl objtype_t;
		typedef AUPObject objbase_t;

		AUPObjectImpl(const std::string& id = "", const Link& link = Link());

		virtual void clean_timeslices(timetype_t cutoff = 0);
		void add_timeslice(const TimeSliceImpl& ts);

		static const TimeSliceImpl invalid_timeslice;

	protected:
		class SortTimeSlice;

		typedef std::vector<TimeSliceImpl> timeslice_t;
		timeslice_t m_timeslice;

		virtual bool ts_empty(void) const { return m_timeslice.empty(); }
		virtual unsigned int ts_size(void) const { return m_timeslice.size(); }
		virtual AUPTimeSlice& ts_get(unsigned int idx);
		virtual const AUPTimeSlice& ts_get(unsigned int idx) const;
	};

	class AUPRouteSegmentTimeSlice : public AUPTimeSlice {
	public:
		class Availability {
		public:
			Availability(void) : m_flags(0) {}

			const AltRange& get_altrange(void) const { return m_altrange; }
			AltRange& get_altrange(void) { return m_altrange; }
			void set_altrange(const AltRange& ar) { m_altrange = ar; }

			typedef std::vector<AUPLink> hostairspaces_t;
			const hostairspaces_t& get_hostairspaces(void) const { return m_hostairspaces; }
			hostairspaces_t& get_hostairspaces(void) { return m_hostairspaces; }
			void set_hostairspaces(const hostairspaces_t& ar) { m_hostairspaces = ar; }

			unsigned int get_cdr(void) const { return ((m_flags & 0x0C) >> 2); }
			void set_cdr(unsigned int c) { m_flags ^= (m_flags ^ (c << 2)) & 0x0C; }

			operator AUPCDR::Availability(void) const;

			std::ostream& print(std::ostream& os, const timepair_t& tp) const;

		protected:
			AltRange m_altrange;
			hostairspaces_t m_hostairspaces;
			uint8_t m_flags;
		};

		AUPRouteSegmentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

		virtual AUPRouteSegmentTimeSlice& as_routesegment(void);
		virtual const AUPRouteSegmentTimeSlice& as_routesegment(void) const;

		const AUPLink& get_route(void) const { return m_route; }
		AUPLink& get_route(void) { return m_route; }
		void set_route(const AUPLink& route) { m_route = route; }

		const AUPLink& get_start(void) const { return m_start; }
		AUPLink& get_start(void) { return m_start; }
		void set_start(const AUPLink& start) { m_start = start; }

		const AUPLink& get_end(void) const { return m_end; }
		AUPLink& get_end(void) { return m_end; }
		void set_end(const AUPLink& end) { m_end = end; }

		typedef std::vector<Availability> availability_t;
		const availability_t& get_availability(void) const { return m_availability; }
		availability_t& get_availability(void) { return m_availability; }
		void set_availability(const availability_t& a) { m_availability = a; }

		typedef std::vector<AUPSegmentLink> segments_t;
		const segments_t& get_segments(void) const { return m_segments; }
		segments_t& get_segments(void) { return m_segments; }
		void set_segments(segments_t& s) { m_segments = s; }

		virtual std::ostream& print(std::ostream& os) const;

	protected:
		availability_t m_availability;
		segments_t m_segments;
		AUPLink m_route;
		AUPLink m_start;
		AUPLink m_end;
	};

	class AUPRouteSegment : public AUPObjectImpl<AUPRouteSegmentTimeSlice> {
	public:
		typedef Glib::RefPtr<AUPRouteSegment> ptr_t;
		typedef Glib::RefPtr<const AUPRouteSegment> const_ptr_t;
		typedef AUPObjectImpl<AUPRouteSegmentTimeSlice> objbase_t;

		AUPRouteSegment(const std::string& id = "", const Link& link = Link());

		virtual type_t get_type(void) const { return type_routesegment; }
	};

	class AUPIdentTimeSlice : public AUPTimeSlice {
	public:
		AUPIdentTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

		virtual AUPIdentTimeSlice& as_ident(void);
		virtual const AUPIdentTimeSlice& as_ident(void) const;

		const std::string& get_ident(void) const { return m_ident; }
		void set_ident(const std::string& x) { m_ident = x; }

		virtual std::ostream& print(std::ostream& os) const;

		static const AUPIdentTimeSlice invalid_timeslice;

	protected:
		std::string m_ident;
	};

	class AUPRouteTimeSlice : public AUPIdentTimeSlice {
	public:
		AUPRouteTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

		virtual AUPRouteTimeSlice& as_route(void);
		virtual const AUPRouteTimeSlice& as_route(void) const;

		virtual std::ostream& print(std::ostream& os) const;
	};

	class AUPRoute : public AUPObjectImpl<AUPRouteTimeSlice> {
	public:
		typedef Glib::RefPtr<AUPRoute> ptr_t;
		typedef Glib::RefPtr<const AUPRoute> const_ptr_t;
		typedef AUPObjectImpl<AUPRouteTimeSlice> objbase_t;

		AUPRoute(const std::string& id = "", const Link& link = Link());

		virtual type_t get_type(void) const { return type_route; }
	};

	class AUPDesignatedPointTimeSlice : public AUPIdentTimeSlice {
	public:
		typedef ADR::DesignatedPointTimeSlice::type_t type_t;

		AUPDesignatedPointTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

		virtual AUPDesignatedPointTimeSlice& as_designatedpoint(void);
		virtual const AUPDesignatedPointTimeSlice& as_designatedpoint(void) const;

		type_t get_type(void) const { return m_type; }
		void set_type(type_t t) { m_type = t; }

		virtual std::ostream& print(std::ostream& os) const;

	protected:
		type_t m_type;
	};

	class AUPDesignatedPoint : public AUPObjectImpl<AUPDesignatedPointTimeSlice> {
	public:
		typedef Glib::RefPtr<AUPDesignatedPoint> ptr_t;
		typedef Glib::RefPtr<const AUPDesignatedPoint> const_ptr_t;
		typedef AUPObjectImpl<AUPDesignatedPointTimeSlice> objbase_t;

		AUPDesignatedPoint(const std::string& id = "", const Link& link = Link());

		virtual type_t get_type(void) const { return type_designatedpoint; }
	};

	class AUPNavaidTimeSlice : public AUPIdentTimeSlice {
	public:
		AUPNavaidTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

		virtual AUPNavaidTimeSlice& as_navaid(void);
		virtual const AUPNavaidTimeSlice& as_navaid(void) const;

		virtual std::ostream& print(std::ostream& os) const;
	};

	class AUPNavaid : public AUPObjectImpl<AUPNavaidTimeSlice> {
	public:
		typedef Glib::RefPtr<AUPNavaid> ptr_t;
		typedef Glib::RefPtr<const AUPNavaid> const_ptr_t;
		typedef AUPObjectImpl<AUPNavaidTimeSlice> objbase_t;

		AUPNavaid(const std::string& id = "", const Link& link = Link());

		virtual type_t get_type(void) const { return type_navaid; }
	};

	class AUPAirspaceTimeSlice : public AUPIdentTimeSlice {
	public:
		class Activation {
		public:
			typedef ADR::AUPRSA::Activation::status_t status_t;

			Activation(void) : m_status(ADR::AUPRSA::Activation::status_invalid) {}

			const AltRange& get_altrange(void) const { return m_altrange; }
			AltRange& get_altrange(void) { return m_altrange; }
			void set_altrange(const AltRange& ar) { m_altrange = ar; }

			typedef std::vector<AUPLink> hostairspaces_t;
			const hostairspaces_t& get_hostairspaces(void) const { return m_hostairspaces; }
			hostairspaces_t& get_hostairspaces(void) { return m_hostairspaces; }
			void set_hostairspaces(const hostairspaces_t& ar) { m_hostairspaces = ar; }

			status_t get_status(void) const { return m_status; }
			void set_status(status_t s) { m_status = s; }

			operator AUPRSA::Activation(void) const;

			std::ostream& print(std::ostream& os, const timepair_t& tp) const;

		protected:
			AltRange m_altrange;
			hostairspaces_t m_hostairspaces;
			status_t m_status;
		};

		typedef ADR::AirspaceTimeSlice::type_t type_t;

		AUPAirspaceTimeSlice(timetype_t starttime = 0, timetype_t endtime = 0);

		virtual AUPAirspaceTimeSlice& as_airspace(void);
		virtual const AUPAirspaceTimeSlice& as_airspace(void) const;

		const Activation& get_activation(void) const { return m_activation; }
		Activation& get_activation(void) { return m_activation; }
		void set_activation(const Activation& ar) { m_activation = ar; }

		type_t get_type(void) const { return m_type; }
		void set_type(type_t t) { m_type = t; }

		bool is_icao(void) const { return !!(m_flags & 0x04); }
		void set_icao(bool icao = true) { if (icao) m_flags |= 0x04; else m_flags &= ~0x04; }

		bool is_level(unsigned int l) const { return !!(m_flags & 0x70 & (0x08 << l)); }
		void set_level(unsigned int l, bool x = true) { m_flags ^= (m_flags ^ -!!x) & 0x70 & (0x08 << l); }

		virtual std::ostream& print(std::ostream& os) const;

 	protected:
		Activation m_activation;
		type_t m_type;
		uint8_t m_flags;
	};

	class AUPAirspace : public AUPObjectImpl<AUPAirspaceTimeSlice> {
	public:
		typedef Glib::RefPtr<AUPAirspace> ptr_t;
		typedef Glib::RefPtr<const AUPAirspace> const_ptr_t;
		typedef AUPObjectImpl<AUPAirspaceTimeSlice> objbase_t;

		AUPAirspace(const std::string& id = "", const Link& link = Link());

		virtual type_t get_type(void) const { return type_airspace; }
	};

protected:
	class NodeIgnore;
	class NodeText;
	class NodeLink;
	class NodeAIXMExtensionBase;
	class NodeAIXMAvailabilityBase;
	class NodeADRAirspaceExtension;
	class NodeADRAirspaceActivationExtension;
	class NodeADRRouteAvailabilityExtension;
	class NodeAIXMExtension;
	class NodeAIXMAirspaceActivation;
	class NodeAIXMActivation;
	class NodeGMLValidTime;
	class NodeGMLTimePosition;
	class NodeGMLBeginPosition;
	class NodeGMLEndPosition;
	class NodeGMLTimePeriod;
	class NodeGMLTimeInstant;
	class NodeAIXMEnRouteSegmentPoint;
	class NodeAIXMStart;
	class NodeAIXMEnd;
	class NodeAIXMAirspaceLayer;
	class NodeAIXMLevels;
	class NodeAIXMRouteAvailability;
	class NodeAIXMProcedureAvailability;
	class NodeAIXMAvailability;
	class NodeAIXMAnyTimeSlice;
	class NodeAIXMRouteSegmentTimeSlice;
	class NodeAIXMRouteTimeSlice;
	class NodeAIXMDesignatedPointTimeSlice;
	class NodeAIXMNavaidTimeSlice;
	class NodeAIXMAirspaceTimeSlice;
	class NodeAIXMTimeSlice;
	class NodeAIXMObject;
	template <typename Obj> class NodeAIXMObjectImpl;
	class NodeADRMessageMember;
	class NodeAllocations;
	class NodeData;

	class Node {
	public:
		typedef Glib::RefPtr<Node> ptr_t;
		typedef Glib::RefPtr<const Node> const_ptr_t;

		Node(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		virtual ~Node();

		void reference(void) const;
		void unreference(void) const;

		virtual void on_characters(const Glib::ustring& characters);
		virtual void on_subelement(const ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeIgnore>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtensionBase>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceActivation>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLValidTime>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLBeginPosition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLEndPosition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimePeriod>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimePosition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimeInstant>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMStart>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailabilityBase>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAnyTimeSlice>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTimeSlice>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMObject>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRMessageMember>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAllocations>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeData>& el);
		virtual const std::string& get_element_name(void) const = 0;

		virtual bool is_ignore(void) const { return false; }

		void error(const std::string& text) const;
		void error(const_ptr_t child, const std::string& text) const;
		void warning(const std::string& text) const;
		void warning(const_ptr_t child, const std::string& text) const;
		void info(const std::string& text) const;
		void info(const_ptr_t child, const std::string& text) const;

		const std::string& get_tagname(void) const { return m_tagname; }
		unsigned int get_childnum(void) const { return m_childnum; }
		unsigned int get_childcount(void) const { return m_childcount; }

		const std::string& get_id(void) const { return m_id; }

	protected:
		ParseAUP& m_parser;
		std::string m_tagname;
		std::string m_id;
		mutable gint m_refcount;
		unsigned int m_childnum;
		unsigned int m_childcount;
	};

	class NodeIgnore : public Node {
	public:
		typedef Glib::RefPtr<NodeIgnore> ptr_t;

		NodeIgnore(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name);

		virtual void on_characters(const Glib::ustring& characters) {}
		virtual void on_subelement(const Node::ptr_t& el) {}
		virtual const std::string& get_element_name(void) const { return m_elementname; }
		virtual bool is_ignore(void) const { return true; }

	protected:
		std::string m_elementname;
	};

	class NodeNoIgnore : public Node {
	public:
		NodeNoIgnore(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) : Node(parser, tagname, childnum, properties) {}

		virtual void on_subelement(const Glib::RefPtr<NodeIgnore>& el);
	};

	class NodeText : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeText> ptr_t;

		typedef enum {
			tt_first,
			tt_adrconditionalroutetype = tt_first,
			tt_adrlevel1,
			tt_aixmdesignator,
		        tt_aixmdesignatoricao,
			tt_aixmdesignatornumber,
			tt_aixmdesignatorprefix,
			tt_aixmdesignatorsecondletter,
			tt_aixminterpretation,
			tt_aixmlowerlimit,
			tt_aixmlowerlimitreference,
			tt_aixmmultipleidentifier,
			tt_aixmstatus,
			tt_aixmtype,
			tt_aixmupperlimit,
		        tt_aixmupperlimitreference,
			tt_requestid,
			tt_requestreceptiontime,
			tt_sendtime,
			tt_status,
			tt_invalid
		} texttype_t;

		NodeText(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name);
		NodeText(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, texttype_t tt = tt_invalid);

		virtual void on_characters(const Glib::ustring& characters);

		virtual const std::string& get_element_name(void) const { return get_static_element_name(m_texttype); }
		static const std::string& get_static_element_name(texttype_t tt);
		static void check_type_order(void);
		static texttype_t find_type(const std::string& name);
		texttype_t get_type(void) const { return m_texttype; }
		const std::string& get_text(void) const { return m_text; }
		long get_long(void) const;
		unsigned long get_ulong(void) const;
		double get_double(void) const;
		Point::coord_t get_lat(void) const;
		Point::coord_t get_lon(void) const;
		UUID get_uuid(void) const;
		Point get_coord_deg(void) const;
		std::pair<Point,int32_t> get_coord_3d(void) const;
		int get_timehhmm(void) const; // in minutes
		timetype_t get_dateyyyymmdd(void) const;
		int get_daynr(void) const;
		int get_yesno(void) const;
		void update(AltRange& ar);

	protected:
		std::string m_text;
		double m_factor;
		unsigned long m_factorl;
		texttype_t m_texttype;

		void check_attr(const AttributeList& properties);
	};

	class NodeLink : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeLink> ptr_t;

		typedef enum {
			lt_first,
			lt_adrhostairspace = lt_first,
			lt_aixmpointchoiceairportreferencepoint,
			lt_aixmpointchoicefixdesignatedpoint,
			lt_aixmpointchoicenavaidsystem,
			lt_aixmrouteformed,
			lt_invalid
		} linktype_t;

		NodeLink(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name);
		NodeLink(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, linktype_t lt = lt_invalid);

		virtual const std::string& get_element_name(void) const { return get_static_element_name(m_linktype); }
		static const std::string& get_static_element_name(linktype_t lt);
		static void check_type_order(void);
		static linktype_t find_type(const std::string& name);
		linktype_t get_type(void) const { return m_linktype; }
		const UUID& get_uuid(void) const { return m_uuid; }
		const std::string& get_id(void) const { return m_id; }

	protected:
		UUID m_uuid;
		std::string m_id;
		linktype_t m_linktype;

		void process_attr(const AttributeList& properties);
	};

	class NodeAIXMExtensionBase : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMExtensionBase> ptr_t;

		NodeAIXMExtensionBase(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
	};

	class NodeAIXMAvailabilityBase : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAvailabilityBase> ptr_t;

		NodeAIXMAvailabilityBase(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
	};

	class NodeADRDefaultApplicability : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRDefaultApplicability> ptr_t;

		NodeADRDefaultApplicability(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRDefaultApplicability(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		TimeTable get_timetable(void) const;

	protected:
		static const std::string elementname;
		TimeTableElement m_timetable;
	};

	class NodeADRAirspaceExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRAirspaceExtension> ptr_t;

		NodeADRAirspaceExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRAirspaceExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		bool is_level1(void) const { return m_level1; }

	protected:
		static const std::string elementname;
		bool m_level1;
	};

	class NodeADRAirspaceActivationExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRAirspaceActivationExtension> ptr_t;

		NodeADRAirspaceActivationExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRAirspaceActivationExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPAirspaceTimeSlice::Activation::hostairspaces_t& get_hostairspaces(void) const { return m_hostairspaces; }

	protected:
		static const std::string elementname;
		AUPAirspaceTimeSlice::Activation::hostairspaces_t m_hostairspaces;
	};

	class NodeADRNonDefaultApplicability : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRNonDefaultApplicability> ptr_t;

		NodeADRNonDefaultApplicability(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRNonDefaultApplicability(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
	};

	class NodeADRRouteAvailabilityExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRRouteAvailabilityExtension> ptr_t;

		NodeADRRouteAvailabilityExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRRouteAvailabilityExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPRouteSegmentTimeSlice::Availability::hostairspaces_t& get_hostairspaces(void) const { return m_hostairspaces; }
		unsigned int get_cdr(void) const { return m_cdr; }

	protected:
		static const std::string elementname;
		AUPRouteSegmentTimeSlice::Availability::hostairspaces_t m_hostairspaces;
		unsigned int m_cdr;
	};

	class NodeAIXMExtension : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMExtension> ptr_t;

		NodeAIXMExtension(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtensionBase>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Glib::RefPtr<NodeAIXMExtensionBase>& get_el(void) const { return m_el; }
		template<typename T> Glib::RefPtr<T> get(void) const { return Glib::RefPtr<T>::cast_dynamic(get_el()); }

	protected:
		static const std::string elementname;
		Glib::RefPtr<NodeAIXMExtensionBase> m_el;
	};

	class NodeAIXMAirspaceActivation : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceActivation> ptr_t;

		NodeAIXMAirspaceActivation(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceActivation(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPAirspaceTimeSlice::Activation& get_activation(void) const { return m_activation; }

	protected:
		static const std::string elementname;
		AUPAirspaceTimeSlice::Activation m_activation;
	};

	class NodeAIXMActivation : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMActivation> ptr_t;

		NodeAIXMActivation(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMActivation(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceActivation>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Glib::RefPtr<Node>& get_el(void) const { return m_el; }
		template<typename T> Glib::RefPtr<T> get(void) const { return Glib::RefPtr<T>::cast_dynamic(get_el()); }

	protected:
		static const std::string elementname;
		Glib::RefPtr<Node> m_el;
	};

	class NodeGMLValidTime : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLValidTime> ptr_t;

		NodeGMLValidTime(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLValidTime(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimePeriod>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimeInstant>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_start(void) const { return m_start; }
		long get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		long m_start;
		long m_end;
	};

	class NodeGMLTimePosition : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLTimePosition> ptr_t;
		static const long invalid;
		static const long unknown;

		NodeGMLTimePosition(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLTimePosition(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		virtual void on_characters(const Glib::ustring& characters);

		long get_time(void) const;

	protected:
		static const std::string elementname;
		std::string m_text;
		bool m_unknown;
	};

	class NodeGMLBeginPosition : public NodeGMLTimePosition {
	public:
		typedef Glib::RefPtr<NodeGMLBeginPosition> ptr_t;

		NodeGMLBeginPosition(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLBeginPosition(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_time(void) const;

	protected:
		static const std::string elementname;
	};

	class NodeGMLEndPosition : public NodeGMLTimePosition {
	public:
		typedef Glib::RefPtr<NodeGMLEndPosition> ptr_t;

		NodeGMLEndPosition(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLEndPosition(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_time(void) const;

	protected:
		static const std::string elementname;
	};

	class NodeGMLTimePeriod : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLTimePeriod> ptr_t;

		NodeGMLTimePeriod(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLTimePeriod(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeGMLBeginPosition::ptr_t& el);
		virtual void on_subelement(const NodeGMLEndPosition::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_start(void) const { return m_start; }
		long get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		long m_start;
		long m_end;
	};

	class NodeGMLTimeInstant : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLTimeInstant> ptr_t;

		NodeGMLTimeInstant(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLTimeInstant(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeGMLTimePosition::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_time(void) const { return m_time; }

	protected:
		static const std::string elementname;
		long m_time;
	};

	class NodeAIXMEnRouteSegmentPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMEnRouteSegmentPoint> ptr_t;

		NodeAIXMEnRouteSegmentPoint(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMEnRouteSegmentPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_id(void) const { return m_id; }

	protected:
		static const std::string elementname;
		std::string m_id;
	};

	class NodeAIXMStart : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMStart> ptr_t;

		NodeAIXMStart(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStart(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_id(void) const { return m_id; }

	protected:
		static const std::string elementname;
		std::string m_id;
	};

	class NodeAIXMEnd : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMEnd> ptr_t;

		NodeAIXMEnd(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMEnd(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_id(void) const { return m_id; }

	protected:
		static const std::string elementname;
		std::string m_id;
	};

	class NodeAIXMAirspaceLayer : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceLayer> ptr_t;

		NodeAIXMAirspaceLayer(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceLayer(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AltRange& get_altrange(void) const { return m_altrange; }

	protected:
		static const std::string elementname;
		AltRange m_altrange;
	};

	class NodeAIXMLevels : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMLevels> ptr_t;

		NodeAIXMLevels(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMLevels(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AltRange& get_altrange(void) const { return m_altrange; }

	protected:
		static const std::string elementname;
		AltRange m_altrange;
	};

	class NodeAIXMRouteAvailability : public NodeAIXMAvailabilityBase {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteAvailability> ptr_t;

		NodeAIXMRouteAvailability(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteAvailability(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPRouteSegmentTimeSlice::Availability& get_availability(void) const { return m_availability; }

	protected:
		static const std::string elementname;
		AUPRouteSegmentTimeSlice::Availability m_availability;
	};

	class NodeAIXMAvailability : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAvailability> ptr_t;

		NodeAIXMAvailability(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAvailability(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailabilityBase>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Glib::RefPtr<NodeAIXMAvailabilityBase>& get_el(void) const { return m_el; }
		template<typename T> Glib::RefPtr<T> get(void) const { return Glib::RefPtr<T>::cast_dynamic(get_el()); }

	protected:
		static const std::string elementname;
		Glib::RefPtr<NodeAIXMAvailabilityBase> m_el;
	};

	class NodeAIXMAnyTimeSlice : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAnyTimeSlice> ptr_t;

		NodeAIXMAnyTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeGMLValidTime::ptr_t& el);

		typedef enum {
			interpretation_baseline,
			interpretation_permdelta,
			interpretation_tempdelta,
			interpretation_snapshot,
			interpretation_invalid
		} interpretation_t;

		interpretation_t get_interpretation(void) const { return m_interpretation; }
		long get_validstart(void) const { return m_validstart; }
		long get_validend(void) const { return m_validend; }
		long get_featurestart(void) const { return m_featurestart; }
		long get_featureend(void) const { return m_featureend; }

		virtual AUPTimeSlice& get_tsbase(void) { return *(AUPTimeSlice *)0; }

	protected:
		long m_validstart;
		long m_validend;
		long m_featurestart;
		long m_featureend;
		interpretation_t m_interpretation;
	};

	class NodeAIXMRouteSegmentTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteSegmentTimeSlice> ptr_t;

		NodeAIXMRouteSegmentTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteSegmentTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMStart>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPRouteSegmentTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AUPRouteSegmentTimeSlice m_timeslice;

		virtual AUPTimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMRouteTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteTimeSlice> ptr_t;

		NodeAIXMRouteTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPRouteTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AUPRouteTimeSlice m_timeslice;
		std::string m_desigprefix;
		std::string m_desigsecondletter;
		std::string m_designumber;
		std::string m_multiid;

		virtual AUPTimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMDesignatedPointTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMDesignatedPointTimeSlice> ptr_t;

		NodeAIXMDesignatedPointTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDesignatedPointTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPDesignatedPointTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AUPDesignatedPointTimeSlice m_timeslice;

		virtual AUPTimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMNavaidTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMNavaidTimeSlice> ptr_t;

		NodeAIXMNavaidTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMNavaidTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPNavaidTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AUPNavaidTimeSlice m_timeslice;

		virtual AUPTimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMAirspaceTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceTimeSlice> ptr_t;

		NodeAIXMAirspaceTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AUPAirspaceTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AUPAirspaceTimeSlice m_timeslice;

		virtual AUPTimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMTimeSlice : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMTimeSlice> ptr_t;

		NodeAIXMTimeSlice(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMAnyTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		NodeAIXMAnyTimeSlice::ptr_t operator[](unsigned int i) { return m_nodes[i]; }
		unsigned int size(void) const { return m_nodes.size(); }
		bool empty(void) const { return m_nodes.empty(); }

	protected:
		static const std::string elementname;
		typedef std::vector<NodeAIXMAnyTimeSlice::ptr_t> nodes_t;
		nodes_t m_nodes;
	};

	class NodeAIXMObject : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMObject> ptr_t;

		NodeAIXMObject(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		virtual AUPObject::ptr_t get_obj(void) const = 0;
	};

	template <typename Obj> class NodeAIXMObjectImpl : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMObjectImpl> ptr_t;
		typedef Obj Object;

		NodeAIXMObjectImpl(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);

		virtual AUPObject::ptr_t get_obj(void) const { return m_obj; }

	protected:
		typename Object::ptr_t m_obj;
	};

	class NodeAIXMRouteSegment : public NodeAIXMObjectImpl<AUPRouteSegment> {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteSegment> ptr_t;

		NodeAIXMRouteSegment(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteSegment(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMRoute : public NodeAIXMObjectImpl<AUPRoute> {
	public:
		typedef Glib::RefPtr<NodeAIXMRoute> ptr_t;

		NodeAIXMRoute(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRoute(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMDesignatedPoint : public NodeAIXMObjectImpl<AUPDesignatedPoint> {
	public:
		typedef Glib::RefPtr<NodeAIXMDesignatedPoint> ptr_t;

		NodeAIXMDesignatedPoint(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDesignatedPoint(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMNavaid : public NodeAIXMObjectImpl<AUPNavaid> {
	public:
		typedef Glib::RefPtr<NodeAIXMNavaid> ptr_t;

		NodeAIXMNavaid(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMNavaid(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMAirspace : public NodeAIXMObjectImpl<AUPAirspace> {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspace> ptr_t;

		NodeAIXMAirspace(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspace(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeADRMessageMember : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRMessageMember> ptr_t;

		NodeADRMessageMember(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRMessageMember(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMObject::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		NodeAIXMObject::ptr_t get_obj(void) const { return m_obj; }

	protected:
		static const std::string elementname;
		NodeAIXMObject::ptr_t m_obj;
	};

	class NodeAllocations : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAllocations> ptr_t;

		NodeAllocations(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const NodeADRMessageMember::ptr_t& el);

	protected:
	};

	class NodeCDRAllocations : public NodeAllocations {
	public:
		typedef Glib::RefPtr<NodeCDRAllocations> ptr_t;

		NodeCDRAllocations(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeCDRAllocations(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeRSAAllocations : public NodeAllocations {
	public:
		typedef Glib::RefPtr<NodeRSAAllocations> ptr_t;

		NodeRSAAllocations(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeRSAAllocations(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeData : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeData> ptr_t;

		NodeData(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeData(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAllocations::ptr_t& el);

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAirspaceReply : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAirspaceReply> ptr_t;

		NodeAirspaceReply(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const NodeData::ptr_t& el);

	protected:
		std::string m_requesttime;
		std::string m_requestid;
		std::string m_sendtime;
		std::string m_status;
	};

	class NodeAirspaceEAUPCDRReply : public NodeAirspaceReply {
	public:
		typedef Glib::RefPtr<NodeAirspaceEAUPCDRReply> ptr_t;

		NodeAirspaceEAUPCDRReply(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAirspaceEAUPCDRReply(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAirspaceEAUPRSAReply : public NodeAirspaceReply {
	public:
		typedef Glib::RefPtr<NodeAirspaceEAUPRSAReply> ptr_t;

		NodeAirspaceEAUPRSAReply(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseAUP& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAirspaceEAUPRSAReply(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	virtual void on_start_document(void);
	virtual void on_end_document(void);
	virtual void on_start_element(const Glib::ustring& name, const AttributeList& properties);
	virtual void on_end_element(const Glib::ustring& name);
	virtual void on_characters(const Glib::ustring& characters);
	virtual void on_comment(const Glib::ustring& text);
	virtual void on_warning(const Glib::ustring& text);
	virtual void on_error(const Glib::ustring& text);
	virtual void on_fatal_error(const Glib::ustring& text);

	static const bool tracestack = false;

	Database& m_db;

	typedef Node::ptr_t (*factoryfunc_t)(ParseAUP&, const std::string&, unsigned int, const AttributeList&);
	typedef std::map<std::string,factoryfunc_t> factory_t;
	factory_t m_factory;

	typedef std::map<std::string,std::string> namespacemap_t;
	namespacemap_t m_namespacemap;
	namespacemap_t m_namespaceshort;

	typedef std::vector<Node::ptr_t > parsestack_t;
	parsestack_t m_parsestack;

	struct AUPObjectIDSorter {
		bool operator()(const AUPObject& a, const AUPObject& b) const { return a.get_id() < b.get_id(); }
		bool operator()(const AUPObject::const_ptr_t& a, const AUPObject::const_ptr_t& b) {
			if (!b)
				return false;
			if (!a)
				return true;
			return a->get_id() < b->get_id();
		}
	};
	typedef std::set<AUPObject::ptr_t, AUPObjectIDSorter> objects_t;
	objects_t m_objects;

	unsigned int m_errorcnt;
	unsigned int m_warncnt;

	bool m_verbose;
};

};

const std::string& to_str(ADR::ParseAUP::AUPObject::type_t t);
inline std::ostream& operator<<(std::ostream& os, ADR::ParseAUP::AUPObject::type_t t) { return os << to_str(t); }

#endif /* ADRAUPPARSE_H */
