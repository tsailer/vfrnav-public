#ifndef ADRPARSE_H
#define ADRPARSE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <libxml++/libxml++.h>

#include "adr.hh"
#include "adrdb.hh"
#include "adraerodrome.hh"
#include "adrunit.hh"
#include "adrpoint.hh"
#include "adrroute.hh"
#include "adrairspace.hh"
#include "adrrestriction.hh"

namespace ADR {

class ParseXML : public xmlpp::SaxParser {
public:
	ParseXML(Database& db, bool verbose = true);
	virtual ~ParseXML();

	void error(const std::string& text);
	void error(const std::string& child, const std::string& text);
	void warning(const std::string& text);
	void warning(const std::string& child, const std::string& text);
	void info(const std::string& text);
	void info(const std::string& child, const std::string& text);

	unsigned int get_errorcnt(void) const { return m_errorcnt; }
	unsigned int get_warncnt(void) const { return m_warncnt; }

	Database& get_db(void) { return m_db; }

protected:
	class NodeIgnore;
	class NodeText;
	class NodeLink;
	class NodeAIXMElementBase;
	class NodeAIXMExtensionBase;
	class NodeAIXMAvailabilityBase;
	class NodeADRWeekDayPattern;
	class NodeADRSpecialDayPattern;
	class NodeADRTimePatternCombination;
	class NodeADRTimeTable1;
	class NodeADRTimeTable;
	class NodeADRLevels;
	class NodeADRPropertiesWithScheduleExtension;
	class NodeADRDefaultApplicability;
	class NodeADRAirspaceExtension;
	class NodeADRNonDefaultApplicability;
	class NodeADRRouteAvailabilityExtension;
	class NodeADRProcedureAvailabilityExtension;
	class NodeADRRouteSegmentExtension;
	class NodeADRIntermediateSignificantPoint;
	class NodeADRIntermediatePoint;
	class NodeADRRoutePortionExtension;
	class NodeADRStandardInstrumentDepartureExtension;
	class NodeADRStandardInstrumentArrivalExtension;
	class NodeAIXMExtension;
	class NodeAIXMAirspaceActivation;
	class NodeAIXMActivation;
	class NodeAIXMElevatedPoint;
	class NodeAIXMPoint;
	class NodeAIXMARP;
	class NodeAIXMLocation;
	class NodeAIXMCity;
	class NodeAIXMServedCity;
	class NodeGMLValidTime;
	class NodeAIXMFeatureLifetime;
	class NodeGMLPosition;
	class NodeGMLBeginPosition;
	class NodeGMLEndPosition;
	class NodeGMLTimePeriod;
	class NodeGMLLinearRing;
	class NodeGMLInterior;
	class NodeGMLExterior;
	class NodeGMLPolygonPatch;
	class NodeGMLPatches;
	class NodeAIXMSurface;
	class NodeAIXMHorizontalProjection;
	class NodeAIXMAirspaceVolumeDependency;
	class NodeAIXMContributorAirspace;
	class NodeAIXMAirspaceVolume;
	class NodeAIXMTheAirspaceVolume;
	class NodeAIXMAirspaceGeometryComponent;
	class NodeAIXMGeometryComponent;
	class NodeAIXMEnRouteSegmentPoint;
	class NodeAIXMTerminalSegmentPoint;
	class NodeAIXMStart;
	class NodeAIXMEnd;
	class NodeAIXMStartPoint;
	class NodeAIXMEndPoint;
	class NodeADRConnectingPoint;
	class NodeADRInitialApproachFix;
	class NodeAIXMAirspaceLayer;
	class NodeAIXMLevels;
	class NodeAIXMRouteAvailability;
	class NodeAIXMProcedureAvailability;
	class NodeAIXMAvailability;
	class NodeAIXMClientRoute;
	class NodeAIXMRoutePortion;
	class NodeAIXMDirectFlightClass;
	class NodeAIXMFlightConditionCircumstance;
	class NodeAIXMOperationalCondition;
	class NodeAIXMFlightRestrictionLevel;
	class NodeAIXMFlightLevel;
	class NodeADRFlightConditionCombinationExtension;
	class NodeAIXMDirectFlightSegment;
	class NodeAIXMElementRoutePortionElement;
	class NodeAIXMElementDirectFlightElement;
	class NodeAIXMFlightRoutingElement;
	class NodeAIXMRouteElement;
	class NodeAIXMFlightRestrictionRoute;
	class NodeAIXMRegulatedRoute;
	class NodeAIXMAircraftCharacteristic;
	class NodeAIXMFlightConditionAircraft;
	class NodeAIXMFlightCharacteristic;
	class NodeAIXMFlightConditionFlight;
	class NodeAIXMFlightConditionOperand;
	class NodeAIXMFlightConditionRoutePortionCondition;
	class NodeAIXMFlightConditionDirectFlightCondition;
	class NodeADRAirspaceBorderCrossingObject;
	class NodeADRBorderCrossingCondition;
	class NodeADRFlightConditionElementExtension;
	class NodeAIXMFlightConditionElement;
	class NodeAIXMElement;
	class NodeAIXMFlightConditionCombination;
	class NodeAIXMFlight;
	class NodeAIXMAnyTimeSlice;
	class NodeAIXMAirportHeliportTimeSlice;
	class NodeAIXMAirportHeliportCollocationTimeSlice;
	class NodeAIXMDesignatedPointTimeSlice;
	class NodeAIXMNavaidTimeSlice;
	class NodeAIXMAngleIndicationTimeSlice;
	class NodeAIXMDistanceIndicationTimeSlice;
	class NodeAIXMAirspaceTimeSlice;
	class NodeAIXMStandardLevelTableTimeSlice;
	class NodeAIXMStandardLevelColumnTimeSlice;
	class NodeAIXMRouteSegmentTimeSlice;
	class NodeAIXMRouteTimeSlice;
	class NodeAIXMStandardInstrumentDepartureTimeSlice;
	class NodeAIXMStandardInstrumentArrivalTimeSlice;
	class NodeAIXMDepartureLegTimeSlice;
	class NodeAIXMArrivalLegTimeSlice;
	class NodeAIXMOrganisationAuthorityTimeSlice;
	class NodeAIXMSpecialDateTimeSlice;
	class NodeAIXMUnitTimeSlice;
	class NodeAIXMAirTrafficManagementServiceTimeSlice;
	class NodeAIXMFlightRestrictionTimeSlice;
	class NodeAIXMTimeSlice;
	class NodeAIXMObject;
	class NodeAIXMAirportHeliport;
	class NodeAIXMAirportHeliportCollocation;
	class NodeAIXMDesignatedPoint;
	class NodeAIXMNavaid;
	class NodeAIXMAngleIndication;
	class NodeAIXMDistanceIndication;
	class NodeAIXMAirspace;
	class NodeAIXMStandardLevelTable;
	class NodeAIXMStandardLevelColumn;
	class NodeAIXMRouteSegment;
	class NodeAIXMRoute;
	class NodeAIXMStandardInstrumentArrival;
	class NodeAIXMStandardInstrumentDeparture;
	class NodeAIXMDepartureLeg;
	class NodeAIXMArrivalLeg;
	class NodeAIXMOrganisationAuthority;
	class NodeAIXMSpecialDate;
	class NodeAIXMUnit;
	class NodeAIXMAirTrafficManagementService;
	class NodeAIXMFlightRestriction;
	class NodeADRMessageMember;

	class Node {
	public:
		typedef Glib::RefPtr<Node> ptr_t;
		typedef Glib::RefPtr<const Node> const_ptr_t;

		Node(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		virtual ~Node();

		void reference(void) const;
		void unreference(void) const;

		virtual void on_characters(const Glib::ustring& characters);
		virtual void on_subelement(const ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeIgnore>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRWeekDayPattern>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRSpecialDayPattern>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRTimePatternCombination>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable1>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRLevels>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRDefaultApplicability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRNonDefaultApplicability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRIntermediateSignificantPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRIntermediatePoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtensionBase>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceActivation>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElevatedPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMARP>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMLocation>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMCity>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMServedCity>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLValidTime>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFeatureLifetime>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLBeginPosition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLEndPosition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimePeriod>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLLinearRing>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLInterior>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLExterior>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLPolygonPatch>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLPatches>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMSurface>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMHorizontalProjection>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolumeDependency>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMContributorAirspace>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolume>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTheAirspaceVolume>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceGeometryComponent>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMGeometryComponent>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMStart>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMStartPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEndPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRConnectingPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRInitialApproachFix>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailabilityBase>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMClientRoute>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightClass>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCircumstance>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMOperationalCondition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionLevel>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightLevel>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightSegment>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElementRoutePortionElement>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElementDirectFlightElement>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightRoutingElement>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRouteElement>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionRoute>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRegulatedRoute>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAircraftCharacteristic>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionAircraft>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightCharacteristic>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionFlight>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionOperand>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionRoutePortionCondition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionDirectFlightCondition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRAirspaceBorderCrossingObject>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRBorderCrossingCondition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElementBase>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElement>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCombination>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlight>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAnyTimeSlice>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTimeSlice>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMObject>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRMessageMember>& el);
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
		ParseXML& m_parser;
		std::string m_tagname;
		std::string m_id;
		mutable gint m_refcount;
		unsigned int m_childnum;
		unsigned int m_childcount;
	};

	class NodeIgnore : public Node {
	public:
		typedef Glib::RefPtr<NodeIgnore> ptr_t;

		NodeIgnore(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name);

		virtual void on_characters(const Glib::ustring& characters) {}
		virtual void on_subelement(const Node::ptr_t& el) {}
		virtual const std::string& get_element_name(void) const { return m_elementname; }
		virtual bool is_ignore(void) const { return true; }

	protected:
		std::string m_elementname;
	};

	class NodeNoIgnore : public Node {
	public:
		NodeNoIgnore(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) : Node(parser, tagname, childnum, properties) {}

		virtual void on_subelement(const Glib::RefPtr<NodeIgnore>& el);
	};

	class NodeText : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeText> ptr_t;

		typedef enum {
			tt_first,
			tt_adrconditionalroutetype = tt_first,
			tt_adrday,
			tt_adrenabled,
			tt_adrenddate,
			tt_adrendtime,
			tt_adrexclude,
			tt_adrflexibleuse,
			tt_adrlevel1,
			tt_adrlevel2,
			tt_adrlevel3,
			tt_adrprocessingindicator,
			tt_adrspecialday,
			tt_adrstartdate,
			tt_adrstarttime,
			tt_adrtimeoperand,
			tt_adrtimereference,
			tt_aixmangle,
			tt_aixmangletype,
			tt_aixmcontroltype,
			tt_aixmdateday,
			tt_aixmdateyear,
			tt_aixmdependency,
			tt_aixmdesignator,
			tt_aixmdesignatoriata,
			tt_aixmdesignatoricao,
			tt_aixmdesignatornumber,
			tt_aixmdesignatorprefix,
			tt_aixmdesignatorsecondletter,
			tt_aixmdirection,
			tt_aixmdistance,
			tt_aixmengine,
			tt_aixmexceedlength,
			tt_aixmindex,
			tt_aixminstruction,
			tt_aixminterpretation,
			tt_aixmlocaltype,
			tt_aixmlocationindicatoricao,
			tt_aixmlogicaloperator,
			tt_aixmlowerlevel,
			tt_aixmlowerlevelreference,
			tt_aixmlowerlimit,
			tt_aixmlowerlimitaltitude,
			tt_aixmlowerlimitreference,
			tt_aixmmilitary,
			tt_aixmmultipleidentifier,
			tt_aixmname,
			tt_aixmnavigationspecification,
			tt_aixmnumberengine,
			tt_aixmoperation,
			tt_aixmordernumber,
			tt_aixmpurpose,
			tt_aixmreferencelocation,
			tt_aixmrelationwithlocation,
			tt_aixmseries,
			tt_aixmstandardicao,
			tt_aixmstatus,
			tt_aixmtype,
			tt_aixmtypeaircrafticao,
			tt_aixmupperlevel,
			tt_aixmupperlevelreference,
			tt_aixmupperlimit,
			tt_aixmupperlimitaltitude,
			tt_aixmupperlimitreference,
			tt_aixmverticalseparationcapability,
			tt_gmlidentifier,
			tt_gmlpos,
			tt_invalid
		} texttype_t;

		NodeText(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name);
		NodeText(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, texttype_t tt = tt_invalid);

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
			lt_adrenteredairspace = lt_first,
			lt_adrexitedairspace,
			lt_adrpointchoicefixdesignatedpoint,
			lt_adrpointchoicenavaidsystem,
			lt_adrreferencedprocedure,
			lt_aixmairportheliport,
			lt_aixmarrival,
			lt_aixmauthority,
			lt_aixmclientairspace,
			lt_aixmdeparture,
			lt_aixmdependentairport,
			lt_aixmdiscretelevelseries,
			lt_aixmelementairspaceelement,
			lt_aixmelementstandardinstrumentarrivalelement,
			lt_aixmelementstandardinstrumentdepartureelement,
			lt_aixmendairportreferencepoint,
			lt_aixmendfixdesignatedpoint,
			lt_aixmendnavaidsystem,
			lt_aixmfix,
			lt_aixmflightconditionairportheliportcondition,
			lt_aixmflightconditionairspacecondition,
			lt_aixmflightconditionstandardinstrumentarrivalcondition,
			lt_aixmflightconditionstandardinstrumentdeparturecondition,
			lt_aixmhostairport,
			lt_aixmleveltable,
			lt_aixmpointchoiceairportreferencepoint,
			lt_aixmpointchoicefixdesignatedpoint,
			lt_aixmpointchoicenavaidsystem,
			lt_aixmpointelementfixdesignatedpoint,
			lt_aixmpointelementnavaidsystem,
			lt_aixmreferencedroute,
			lt_aixmrouteformed,
			lt_aixmserviceprovider,
			lt_aixmsignificantpointconditionfixdesignatedpoint,
			lt_aixmsignificantpointconditionnavaidsystem,
			lt_aixmstartairportreferencepoint,
			lt_aixmstartfixdesignatedpoint,
			lt_aixmstartnavaidsystem,
			lt_aixmtheairspace,
			lt_gmlpointproperty,
			lt_invalid
		} linktype_t;

		NodeLink(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name);
		NodeLink(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, linktype_t lt = lt_invalid);

		virtual const std::string& get_element_name(void) const { return get_static_element_name(m_linktype); }
		static const std::string& get_static_element_name(linktype_t lt);
		static void check_type_order(void);
		static linktype_t find_type(const std::string& name);
		linktype_t get_type(void) const { return m_linktype; }
		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		UUID m_uuid;
		linktype_t m_linktype;

		void process_attr(const AttributeList& properties);
	};

	class NodeAIXMElementBase : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMElementBase> ptr_t;

		NodeAIXMElementBase(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
	};

	class NodeAIXMExtensionBase : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMExtensionBase> ptr_t;

		NodeAIXMExtensionBase(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
	};

	class NodeAIXMAvailabilityBase : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAvailabilityBase> ptr_t;

		NodeAIXMAvailabilityBase(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
	};

	class NodeADRWeekDayPattern : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRWeekDayPattern> ptr_t;

		NodeADRWeekDayPattern(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRWeekDayPattern(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimePattern& get_timepat(void) const { return m_timepat; }

	protected:
		static const std::string elementname;
		TimePattern m_timepat;
	};

	class NodeADRSpecialDayPattern : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRSpecialDayPattern> ptr_t;

		NodeADRSpecialDayPattern(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRSpecialDayPattern(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimePattern& get_timepat(void) const { return m_timepat; }

	protected:
		static const std::string elementname;
		TimePattern m_timepat;
	};

	class NodeADRTimePatternCombination : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRTimePatternCombination> ptr_t;

		NodeADRTimePatternCombination(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRTimePatternCombination(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRWeekDayPattern>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRSpecialDayPattern>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		TimeTableElement get_timetable(void) const;

	protected:
		static const std::string elementname;
		TimeTableElement m_timetable;
	};

	class NodeADRTimeTable1 : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRTimeTable1> ptr_t;

		NodeADRTimeTable1(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRTimeTable1(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRTimePatternCombination>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTableElement& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTableElement m_timetable;
	};

	class NodeADRTimeTable : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRTimeTable> ptr_t;

		NodeADRTimeTable(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRTimeTable(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable1>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
	};

	class NodeADRLevels : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRLevels> ptr_t;

		NodeADRLevels(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRLevels(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }
		const AltRange& get_altrange(void) const { return m_altrange; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
		AltRange m_altrange;
	};

	class NodeADRPropertiesWithScheduleExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRPropertiesWithScheduleExtension> ptr_t;

		NodeADRPropertiesWithScheduleExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRPropertiesWithScheduleExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
	};

	class NodeADRDefaultApplicability : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRDefaultApplicability> ptr_t;

		NodeADRDefaultApplicability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

		NodeADRAirspaceExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRAirspaceExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		bool is_flexibleuse(void) const { return m_flexibleuse; }
		bool is_level1(void) const { return m_level1; }
		bool is_level2(void) const { return m_level2; }
		bool is_level3(void) const { return m_level3; }

	protected:
		static const std::string elementname;
		bool m_flexibleuse;
		bool m_level1;
		bool m_level2;
		bool m_level3;
	};

	class NodeADRNonDefaultApplicability : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRNonDefaultApplicability> ptr_t;

		NodeADRNonDefaultApplicability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRNonDefaultApplicability(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el);
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

		NodeADRRouteAvailabilityExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRRouteAvailabilityExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRNonDefaultApplicability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRDefaultApplicability>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }
		unsigned int get_cdr(void) const { return m_cdr; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
		unsigned int m_cdr;
	};

	class NodeADRProcedureAvailabilityExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRProcedureAvailabilityExtension> ptr_t;

		NodeADRProcedureAvailabilityExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRProcedureAvailabilityExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
	};

	class NodeADRRouteSegmentExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRRouteSegmentExtension> ptr_t;

		NodeADRRouteSegmentExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRRouteSegmentExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRLevels>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }
		const AltRange& get_altrange(void) const { return m_altrange; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
		AltRange m_altrange;
	};

	class NodeADRIntermediateSignificantPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRIntermediateSignificantPoint> ptr_t;

		NodeADRIntermediateSignificantPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRIntermediateSignificantPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeADRIntermediatePoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRIntermediatePoint> ptr_t;

		NodeADRIntermediatePoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRIntermediatePoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRIntermediateSignificantPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeADRRoutePortionExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRRoutePortionExtension> ptr_t;

		NodeADRRoutePortionExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRRoutePortionExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRIntermediatePoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_refproc(void) const { return m_refproc; }
		typedef std::vector<UUID> intermpt_t;
		const intermpt_t& get_intermpt(void) const { return m_intermpt; }


	protected:
		static const std::string elementname;
		UUID m_refproc;
		intermpt_t m_intermpt;
	};
 
	class NodeADRStandardInstrumentDepartureExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRStandardInstrumentDepartureExtension> ptr_t;

		NodeADRStandardInstrumentDepartureExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRStandardInstrumentDepartureExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRConnectingPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		typedef std::set<UUID> connpt_t;
		const connpt_t& get_connpt(void) const { return m_connpt; }

	protected:
		static const std::string elementname;
		connpt_t m_connpt;
	};

	class NodeADRStandardInstrumentArrivalExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRStandardInstrumentArrivalExtension> ptr_t;

		NodeADRStandardInstrumentArrivalExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRStandardInstrumentArrivalExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRConnectingPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeADRInitialApproachFix>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		typedef std::set<UUID> connpt_t;
		const connpt_t& get_connpt(void) const { return m_connpt; }
		const UUID& get_iaf(void) const { return m_iaf; }

	protected:
		static const std::string elementname;
		connpt_t m_connpt;
		UUID m_iaf;
	};

	class NodeAIXMExtension : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMExtension> ptr_t;

		NodeAIXMExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

		NodeAIXMAirspaceActivation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceActivation(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
	};

	class NodeAIXMActivation : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMActivation> ptr_t;

		NodeAIXMActivation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

	class NodeAIXMElevatedPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMElevatedPoint> ptr_t;

		NodeAIXMElevatedPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMElevatedPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Point& get_coord(void) const { return m_coord; }
		int32_t get_elev(void) const { return m_elev; }

	protected:
		static const std::string elementname;
		Point m_coord;
		int32_t m_elev;
	};

	class NodeAIXMPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMPoint> ptr_t;

		NodeAIXMPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Point& get_coord(void) const { return m_coord; }

	protected:
		static const std::string elementname;
		Point m_coord;
	};

	class NodeAIXMARP : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMARP> ptr_t;

		NodeAIXMARP(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMARP(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMElevatedPoint::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Point& get_coord(void) const { return m_coord; }
		int32_t get_elev(void) const { return m_elev; }

	protected:
		static const std::string elementname;
		Point m_coord;
		int32_t m_elev;
	};

	class NodeAIXMLocation : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMLocation> ptr_t;

		NodeAIXMLocation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMLocation(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMElevatedPoint::ptr_t& el);
		virtual void on_subelement(const NodeAIXMPoint::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Point& get_coord(void) const { return m_coord; }
		int32_t get_elev(void) const { return m_elev; }

	protected:
		static const std::string elementname;
		Point m_coord;
		int32_t m_elev;
	};

	class NodeAIXMCity : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMCity> ptr_t;

		NodeAIXMCity(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMCity(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const std::string& get_city(void) const { return m_city; }

	protected:
		static const std::string elementname;
		std::string m_city;
	};

	class NodeAIXMServedCity : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMServedCity> ptr_t;

		NodeAIXMServedCity(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMServedCity(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMCity::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		typedef AirportTimeSlice::servedcities_t servedcities_t;
		const servedcities_t get_cities(void) const { return m_cities; }

	protected:
		static const std::string elementname;
		servedcities_t m_cities;
	};

	class NodeGMLValidTime : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLValidTime> ptr_t;

		NodeGMLValidTime(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLValidTime(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimePeriod>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_start(void) const { return m_start; }
		long get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		long m_start;
		long m_end;
	};

	class NodeAIXMFeatureLifetime : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFeatureLifetime> ptr_t;

		NodeAIXMFeatureLifetime(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFeatureLifetime(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLTimePeriod>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_start(void) const { return m_start; }
		long get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		long m_start;
		long m_end;
	};

	class NodeGMLPosition : public NodeNoIgnore {
	public:
		static const long invalid;
		static const long unknown;

		NodeGMLPosition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_characters(const Glib::ustring& characters);

		long get_time(void) const;

	protected:
		std::string m_text;
		bool m_unknown;
	};

	class NodeGMLBeginPosition : public NodeGMLPosition {
	public:
		typedef Glib::RefPtr<NodeGMLBeginPosition> ptr_t;

		NodeGMLBeginPosition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLBeginPosition(parser, tagname, childnum, properties));
		}

		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		long get_time(void) const;

	protected:
		static const std::string elementname;
	};

	class NodeGMLEndPosition : public NodeGMLPosition {
	public:
		typedef Glib::RefPtr<NodeGMLEndPosition> ptr_t;

		NodeGMLEndPosition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

		NodeGMLTimePeriod(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

	class NodeGMLLinearRing : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLLinearRing> ptr_t;

		NodeGMLLinearRing(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLLinearRing(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const PolygonSimple& get_poly(void) const { return m_poly; }
		const AirspaceTimeSlice::Component::pointlink_t& get_pointlink(void) const { return m_pointlink; }

	protected:
		static const std::string elementname;
		PolygonSimple m_poly;
		AirspaceTimeSlice::Component::pointlink_t m_pointlink;
	};

	class NodeGMLInterior : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLInterior> ptr_t;

		NodeGMLInterior(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLExterior(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLLinearRing>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const PolygonSimple& get_poly(void) const { return m_poly; }
		const AirspaceTimeSlice::Component::pointlink_t& get_pointlink(void) const { return m_pointlink; }

	protected:
		static const std::string elementname;
		PolygonSimple m_poly;
		AirspaceTimeSlice::Component::pointlink_t m_pointlink;
	};

	class NodeGMLExterior : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLExterior> ptr_t;

		NodeGMLExterior(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLExterior(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLLinearRing>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const PolygonSimple& get_poly(void) const { return m_poly; }
		const AirspaceTimeSlice::Component::pointlink_t& get_pointlink(void) const { return m_pointlink; }

	protected:
		static const std::string elementname;
		PolygonSimple m_poly;
		AirspaceTimeSlice::Component::pointlink_t m_pointlink;
	};

	class NodeGMLPolygonPatch : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLPolygonPatch> ptr_t;

		NodeGMLPolygonPatch(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLPolygonPatch(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLInterior>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeGMLExterior>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const PolygonHole& get_poly(void) const { return m_poly; }
		const AirspaceTimeSlice::Component::pointlink_t& get_pointlink(void) const { return m_pointlink; }

	protected:
		static const std::string elementname;
		PolygonHole m_poly;
		AirspaceTimeSlice::Component::pointlink_t m_pointlink;
	};

	class NodeGMLPatches : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeGMLPatches> ptr_t;

		NodeGMLPatches(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeGMLPatches(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLPolygonPatch>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const MultiPolygonHole& get_poly(void) const { return m_poly; }
		const AirspaceTimeSlice::Component::pointlink_t& get_pointlink(void) const { return m_pointlink; }

	protected:
		static const std::string elementname;
		MultiPolygonHole m_poly;
		AirspaceTimeSlice::Component::pointlink_t m_pointlink;
	};

	class NodeAIXMSurface : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMSurface> ptr_t;

		NodeAIXMSurface(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMSurface(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeGMLPatches>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const MultiPolygonHole& get_poly(void) const { return m_poly; }
		const AirspaceTimeSlice::Component::pointlink_t& get_pointlink(void) const { return m_pointlink; }

	protected:
		static const std::string elementname;
		MultiPolygonHole m_poly;
		AirspaceTimeSlice::Component::pointlink_t m_pointlink;
	};

	class NodeAIXMHorizontalProjection : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMHorizontalProjection> ptr_t;

		NodeAIXMHorizontalProjection(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMHorizontalProjection(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMSurface>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const MultiPolygonHole& get_poly(void) const { return m_poly; }
		const AirspaceTimeSlice::Component::pointlink_t& get_pointlink(void) const { return m_pointlink; }

	protected:
		static const std::string elementname;
		MultiPolygonHole m_poly;
		AirspaceTimeSlice::Component::pointlink_t m_pointlink;
	};

	class NodeAIXMAirspaceVolumeDependency : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceVolumeDependency> ptr_t;

		NodeAIXMAirspaceVolumeDependency(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceVolumeDependency(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirspaceTimeSlice::Component& get_comp(void) const { return m_comp; }

	protected:
		static const std::string elementname;
		AirspaceTimeSlice::Component m_comp;
	};

	class NodeAIXMContributorAirspace : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMContributorAirspace> ptr_t;

		NodeAIXMContributorAirspace(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMContributorAirspace(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolumeDependency>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirspaceTimeSlice::Component& get_comp(void) const { return m_comp; }

	protected:
		static const std::string elementname;
		AirspaceTimeSlice::Component m_comp;
	};

	class NodeAIXMAirspaceVolume : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceVolume> ptr_t;

		NodeAIXMAirspaceVolume(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceVolume(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMHorizontalProjection>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMContributorAirspace>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirspaceTimeSlice::Component& get_comp(void) const { return m_comp; }

	protected:
		static const std::string elementname;
		AirspaceTimeSlice::Component m_comp;
	};

	class NodeAIXMTheAirspaceVolume : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMTheAirspaceVolume> ptr_t;

		NodeAIXMTheAirspaceVolume(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMTheAirspaceVolume(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolume>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirspaceTimeSlice::Component& get_comp(void) const { return m_comp; }

	protected:
		static const std::string elementname;
		AirspaceTimeSlice::Component m_comp;
	};

	class NodeAIXMAirspaceGeometryComponent : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceGeometryComponent> ptr_t;

		NodeAIXMAirspaceGeometryComponent(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceGeometryComponent(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTheAirspaceVolume>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirspaceTimeSlice::Component& get_comp(void) const { return m_comp; }

	protected:
		static const std::string elementname;
		AirspaceTimeSlice::Component m_comp;
	};

	class NodeAIXMGeometryComponent : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMGeometryComponent> ptr_t;

		NodeAIXMGeometryComponent(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMGeometryComponent(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceGeometryComponent>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirspaceTimeSlice::Component& get_comp(void) const { return m_comp; }

	protected:
		static const std::string elementname;
		AirspaceTimeSlice::Component m_comp;
	};

	class NodeAIXMEnRouteSegmentPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMEnRouteSegmentPoint> ptr_t;

		NodeAIXMEnRouteSegmentPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMEnRouteSegmentPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeAIXMTerminalSegmentPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMTerminalSegmentPoint> ptr_t;

		NodeAIXMTerminalSegmentPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMTerminalSegmentPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeAIXMStart : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMStart> ptr_t;

		NodeAIXMStart(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStart(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeAIXMEnd : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMEnd> ptr_t;

		NodeAIXMEnd(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMEnd(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeAIXMStartPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMStartPoint> ptr_t;

		NodeAIXMStartPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStartPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeAIXMEndPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMEndPoint> ptr_t;

		NodeAIXMEndPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMEndPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeADRConnectingPoint : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRConnectingPoint> ptr_t;

		NodeADRConnectingPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRConnectingPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeADRInitialApproachFix : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRInitialApproachFix> ptr_t;

		NodeADRInitialApproachFix(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRInitialApproachFix(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_uuid(void) const { return m_uuid; }

	protected:
		static const std::string elementname;
		UUID m_uuid;
	};

	class NodeAIXMAirspaceLayer : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceLayer> ptr_t;

		NodeAIXMAirspaceLayer(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceLayer(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const RouteSegmentTimeSlice::Availability& get_availability(void) const { return m_availability; }

	protected:
		static const std::string elementname;
		RouteSegmentTimeSlice::Availability m_availability;
	};

	class NodeAIXMLevels : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMLevels> ptr_t;

		NodeAIXMLevels(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMLevels(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const RouteSegmentTimeSlice::Availability& get_availability(void) const { return m_availability; }

	protected:
		static const std::string elementname;
		RouteSegmentTimeSlice::Availability m_availability;
	};

	class NodeAIXMRouteAvailability : public NodeAIXMAvailabilityBase {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteAvailability> ptr_t;

		NodeAIXMRouteAvailability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteAvailability(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const RouteSegmentTimeSlice::Availability& get_availability(void) const { return m_availability; }

	protected:
		static const std::string elementname;
		RouteSegmentTimeSlice::Availability m_availability;
	};

	class NodeAIXMProcedureAvailability : public NodeAIXMAvailabilityBase {
	public:
		typedef Glib::RefPtr<NodeAIXMProcedureAvailability> ptr_t;

		NodeAIXMProcedureAvailability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMProcedureAvailability(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }
		StandardInstrumentTimeSlice::status_t get_status(void) const { return m_status; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
		StandardInstrumentTimeSlice::status_t m_status;
	};

	class NodeAIXMAvailability : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAvailability> ptr_t;

		NodeAIXMAvailability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

	class NodeAIXMClientRoute : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMClientRoute> ptr_t;

		NodeAIXMClientRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMClientRoute(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_route(void) const { return m_route; }
		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_route;
		UUID m_start;
		UUID m_end;
	};

	class NodeAIXMRoutePortion : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMRoutePortion> ptr_t;

		NodeAIXMRoutePortion(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRoutePortion(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_route(void) const { return m_route; }
		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_route;
		UUID m_start;
		UUID m_end;
	};

	class NodeAIXMDirectFlightClass : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMDirectFlightClass> ptr_t;

		NodeAIXMDirectFlightClass(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDirectFlightClass(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		double get_length(void) const { return m_length; }

	protected:
		static const std::string elementname;
		double m_length;
	};

	class NodeAIXMFlightConditionCircumstance : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionCircumstance> ptr_t;

		NodeAIXMFlightConditionCircumstance(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionCircumstance(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		bool is_refloc(void) const { return m_refloc; }

		typedef enum {
			locrel_dep,
			locrel_arr,
			locrel_act,
			locrel_avbl,
			locrel_xng,
			locrel_invalid
		} locrel_t;
		locrel_t get_locrel(void) const { return m_locrel; }
		static const std::string& get_locrel_string(locrel_t lr);
		const std::string& get_locrel_string(void) const { return get_locrel_string(get_locrel()); }

	protected:
		static const std::string elementname;
		bool m_refloc;
		locrel_t m_locrel;
	};

	class NodeAIXMOperationalCondition : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMOperationalCondition> ptr_t;

		NodeAIXMOperationalCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMOperationalCondition(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCircumstance>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		bool is_refloc(void) const { return m_refloc; }
		NodeAIXMFlightConditionCircumstance::locrel_t get_locrel(void) const { return m_locrel; }

	protected:
		static const std::string elementname;
		bool m_refloc;
		NodeAIXMFlightConditionCircumstance::locrel_t m_locrel;
	};

	class NodeAIXMFlightRestrictionLevel : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightRestrictionLevel> ptr_t;

		NodeAIXMFlightRestrictionLevel(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightRestrictionLevel(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AltRange& get_altrange(void) const { return m_altrange; }

	protected:
		static const std::string elementname;
		AltRange m_altrange;
	};

	class NodeAIXMFlightLevel : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightLevel> ptr_t;

		NodeAIXMFlightLevel(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightLevel(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionLevel>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AltRange& get_altrange(void) const { return m_altrange; }

	protected:
		static const std::string elementname;
		AltRange m_altrange;
	};

	class NodeADRFlightConditionCombinationExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRFlightConditionCombinationExtension> ptr_t;

		NodeADRFlightConditionCombinationExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRFlightConditionCombinationExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
	};

	class NodeAIXMDirectFlightSegment : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMDirectFlightSegment> ptr_t;

		NodeAIXMDirectFlightSegment(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDirectFlightSegment(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_start;
		UUID m_end;
	};

	class NodeAIXMElementRoutePortionElement : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMElementRoutePortionElement> ptr_t;

		NodeAIXMElementRoutePortionElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMElementRoutePortionElement(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_route(void) const { return m_route; }
		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_route;
		UUID m_start;
		UUID m_end;
	};

	class NodeAIXMElementDirectFlightElement : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMElementDirectFlightElement> ptr_t;

		NodeAIXMElementDirectFlightElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMElementDirectFlightElement(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightSegment>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_start;
		UUID m_end;
	};

	class NodeAIXMFlightRoutingElement : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightRoutingElement> ptr_t;

		NodeAIXMFlightRoutingElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightRoutingElement(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightLevel>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElementRoutePortionElement>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElementDirectFlightElement>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		unsigned int get_ordernumber(void) const { return m_ordernumber; }
		RestrictionElement::ptr_t get_elem(void) const;

	protected:
		static const std::string elementname;
		unsigned int m_ordernumber;
		AltRange m_altrange;
		UUID m_uuid;
		UUID m_start;
		UUID m_end;
		RestrictionElement::type_t m_type;
		bool m_star;
	};

	class NodeAIXMRouteElement : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteElement> ptr_t;

		NodeAIXMRouteElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteElement(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightRoutingElement>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		unsigned int get_ordernumber(void) const { return m_ordernumber; }
		const RestrictionElement::ptr_t& get_elem(void) const { return m_elem; }

	protected:
		static const std::string elementname;
		RestrictionElement::ptr_t m_elem;
		unsigned int m_ordernumber;
	};

	class NodeAIXMFlightRestrictionRoute : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightRestrictionRoute> ptr_t;

		NodeAIXMFlightRestrictionRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightRestrictionRoute(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRouteElement>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		RestrictionSequence get_seq(void) const;

	protected:
		static const std::string elementname;
		typedef std::pair<RestrictionElement::ptr_t,unsigned int> elem_t;
		typedef std::vector<elem_t> elems_t;
		elems_t m_elems;
	};

	class NodeAIXMRegulatedRoute : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMRegulatedRoute> ptr_t;

		NodeAIXMRegulatedRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRegulatedRoute(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionRoute>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Restrictions& get_res(void) const { return m_res; }

	protected:
		static const std::string elementname;
		Restrictions m_res;
	};

	class NodeAIXMAircraftCharacteristic : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAircraftCharacteristic> ptr_t;

		NodeAIXMAircraftCharacteristic(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAircraftCharacteristic(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		ConditionAircraft::acfttype_t get_acfttype(void) const { return m_acfttype; }
		ConditionAircraft::engine_t get_engine(void) const { return m_engine; }
		ConditionAircraft::navspec_t get_navspec(void) const { return m_navspec; }
		ConditionAircraft::vertsep_t get_vertsep(void) const { return m_vertsep; }
		const std::string& get_icaotype(void) const { return m_icaotype; }
		unsigned int get_engines(void) const { return m_engines; }

	protected:
		static const std::string elementname;
		std::string m_icaotype;
		unsigned int m_engines;
		ConditionAircraft::acfttype_t m_acfttype;
		ConditionAircraft::engine_t m_engine;
		ConditionAircraft::navspec_t m_navspec;
		ConditionAircraft::vertsep_t m_vertsep;
	};

	class NodeAIXMFlightConditionAircraft : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionAircraft> ptr_t;

		NodeAIXMFlightConditionAircraft(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionAircraft(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAircraftCharacteristic>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		ConditionAircraft::acfttype_t get_acfttype(void) const { return m_acfttype; }
		ConditionAircraft::engine_t get_engine(void) const { return m_engine; }
		ConditionAircraft::navspec_t get_navspec(void) const { return m_navspec; }
		ConditionAircraft::vertsep_t get_vertsep(void) const { return m_vertsep; }
		const std::string& get_icaotype(void) const { return m_icaotype; }
		unsigned int get_engines(void) const { return m_engines; }

	protected:
		static const std::string elementname;
		std::string m_icaotype;
		unsigned int m_engines;
		ConditionAircraft::acfttype_t m_acfttype;
		ConditionAircraft::engine_t m_engine;
		ConditionAircraft::navspec_t m_navspec;
		ConditionAircraft::vertsep_t m_vertsep;
	};

	class NodeAIXMFlightCharacteristic : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightCharacteristic> ptr_t;

		NodeAIXMFlightCharacteristic(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightCharacteristic(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		ConditionFlight::civmil_t get_civmil(void) const { return m_civmil; }
		ConditionFlight::purpose_t get_purpose(void) const { return m_purpose; }

	protected:
		static const std::string elementname;
		ConditionFlight::civmil_t m_civmil;
		ConditionFlight::purpose_t m_purpose;
	};

	class NodeAIXMFlightConditionFlight : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionFlight> ptr_t;

		NodeAIXMFlightConditionFlight(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionFlight(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightCharacteristic>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		ConditionFlight::civmil_t get_civmil(void) const { return m_civmil; }
		ConditionFlight::purpose_t get_purpose(void) const { return m_purpose; }

	protected:
		static const std::string elementname;
		ConditionFlight::civmil_t m_civmil;
		ConditionFlight::purpose_t m_purpose;
	};

	class NodeAIXMFlightConditionOperand : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionOperand> ptr_t;

		NodeAIXMFlightConditionOperand(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionOperand(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCombination>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Condition::ptr_t& get_cond(void) const { return m_cond; }

	protected:
		static const std::string elementname;
		Condition::ptr_t m_cond;
	};

	class NodeAIXMFlightConditionStandardInstrumentArrivalCondition : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionStandardInstrumentArrivalCondition> ptr_t;

		NodeAIXMFlightConditionStandardInstrumentArrivalCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionStandardInstrumentArrivalCondition(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightClass>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMFlightConditionStandardInstrumentDepartureCondition : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionStandardInstrumentDepartureCondition> ptr_t;

		NodeAIXMFlightConditionStandardInstrumentDepartureCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionStandardInstrumentDepartureCondition(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightClass>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMFlightConditionRoutePortionCondition : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionRoutePortionCondition> ptr_t;

		NodeAIXMFlightConditionRoutePortionCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionRoutePortionCondition(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_route(void) const { return m_route; }
		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_route;
		UUID m_start;
		UUID m_end;
	};

	class NodeAIXMFlightConditionDirectFlightCondition : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionDirectFlightCondition> ptr_t;

		NodeAIXMFlightConditionDirectFlightCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionDirectFlightCondition(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightClass>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightSegment>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }
		double get_length(void) const { return m_length; }

	protected:
		static const std::string elementname;
		UUID m_start;
		UUID m_end;
		double m_length;
	};

	class NodeADRAirspaceBorderCrossingObject : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRAirspaceBorderCrossingObject> ptr_t;

		NodeADRAirspaceBorderCrossingObject(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRAirspaceBorderCrossingObject(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_start;
		UUID m_end;
	};

	class NodeADRBorderCrossingCondition : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRBorderCrossingCondition> ptr_t;

		NodeADRBorderCrossingCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRBorderCrossingCondition(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRAirspaceBorderCrossingObject>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_start;
		UUID m_end;
	};

	class NodeADRFlightConditionElementExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRFlightConditionElementExtension> ptr_t;

		NodeADRFlightConditionElementExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRFlightConditionElementExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeADRBorderCrossingCondition>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UUID& get_start(void) const { return m_start; }
		const UUID& get_end(void) const { return m_end; }

	protected:
		static const std::string elementname;
		UUID m_start;
		UUID m_end;
	};

	class NodeAIXMFlightConditionElement : public NodeAIXMElementBase {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionElement> ptr_t;

		NodeAIXMFlightConditionElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionElement(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionOperand>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionDirectFlightCondition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionAircraft>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionRoutePortionCondition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionFlight>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMOperationalCondition>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightLevel>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		unsigned int get_index(void) const { return m_index; }
		const Condition::ptr_t& get_cond(void);

		bool is_refloc(void) const { return m_refloc; }
		NodeAIXMFlightConditionCircumstance::locrel_t get_locrel(void) const { return m_locrel; }

	protected:
		static const std::string elementname;
		AltRange m_altrange;
		UUID m_uuid;
		UUID m_start;
		UUID m_end;
		Condition::ptr_t m_cond;
		double m_length;
		unsigned int m_index;
		bool m_refloc;
		NodeAIXMFlightConditionCircumstance::locrel_t m_locrel;
		Condition::type_t m_type;
		bool m_arr;
		std::string m_icaotype;
		unsigned int m_engines;
		ConditionAircraft::acfttype_t m_acfttype;
		ConditionAircraft::engine_t m_engine;
		ConditionAircraft::navspec_t m_navspec;
		ConditionAircraft::vertsep_t m_vertsep;
		ConditionFlight::civmil_t m_civmil;
		ConditionFlight::purpose_t m_purpose;
	};

	class NodeAIXMElement : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMElement> ptr_t;

		NodeAIXMElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMElement(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElementBase>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Glib::RefPtr<NodeAIXMElementBase>& get_el(void) const { return m_el; }
		template<typename T> Glib::RefPtr<T> get(void) const { return Glib::RefPtr<T>::cast_dynamic(get_el()); }

	protected:
		static const std::string elementname;
		Glib::RefPtr<NodeAIXMElementBase> m_el;
 	};

	class NodeAIXMFlightConditionCombination : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightConditionCombination> ptr_t;

		NodeAIXMFlightConditionCombination(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightConditionCombination(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMElement>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		Condition::ptr_t get_cond(void) const;
		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		TimeTable m_timetable;
		typedef std::pair<Condition::ptr_t,unsigned int> element_t;
		typedef std::vector<element_t> elements_t;
		elements_t m_elements;
		typedef enum {
			operator_and,
			operator_andnot,
			operator_or,
			operator_seq,
			operator_invalid
		} operator_t;
		operator_t m_operator;

		operator_t get_operator(void) const { return m_operator; }
	};

	class NodeAIXMFlight : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMFlight> ptr_t;

		NodeAIXMFlight(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlight(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCombination>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const Condition::ptr_t& get_cond(void) const { return m_cond; }
		const TimeTable& get_timetable(void) const { return m_timetable; }

	protected:
		static const std::string elementname;
		Condition::ptr_t m_cond;
		TimeTable m_timetable;
	};

	class NodeADRFlightRestrictionExtension : public NodeAIXMExtensionBase {
	public:
		typedef Glib::RefPtr<NodeADRFlightRestrictionExtension> ptr_t;

		NodeADRFlightRestrictionExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeADRFlightRestrictionExtension(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		bool is_enabled(void) const { return m_enabled; }
		FlightRestrictionTimeSlice::procind_t get_procind(void) const { return m_procind; }

	protected:
		static const std::string elementname;
		bool m_enabled;
		FlightRestrictionTimeSlice::procind_t m_procind;
	};

	class NodeAIXMAnyTimeSlice : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMAnyTimeSlice> ptr_t;

		NodeAIXMAnyTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeGMLValidTime::ptr_t& el);
		virtual void on_subelement(const NodeAIXMFeatureLifetime::ptr_t& el);

		typedef enum {
			interpretation_baseline,
			interpretation_permdelta,
			interpretation_tempdelta,
			interpretation_invalid
		} interpretation_t;

		interpretation_t get_interpretation(void) const { return m_interpretation; }
		long get_validstart(void) const { return m_validstart; }
		long get_validend(void) const { return m_validend; }
		long get_featurestart(void) const { return m_featurestart; }
		long get_featureend(void) const { return m_featureend; }

	protected:
		long m_validstart;
		long m_validend;
		long m_featurestart;
		long m_featureend;
		interpretation_t m_interpretation;

		virtual TimeSlice& get_tsbase(void) { return *(TimeSlice *)0; }
	};

	class NodeAIXMAirportHeliportTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMAirportHeliportTimeSlice> ptr_t;

		NodeAIXMAirportHeliportTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirportHeliportTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeAIXMServedCity::ptr_t& el);
		virtual void on_subelement(const NodeAIXMARP::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirportTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AirportTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMAirportHeliportCollocationTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMAirportHeliportCollocationTimeSlice> ptr_t;

		NodeAIXMAirportHeliportCollocationTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirportHeliportCollocationTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirportCollocationTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AirportCollocationTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMDesignatedPointTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMDesignatedPointTimeSlice> ptr_t;

		NodeAIXMDesignatedPointTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDesignatedPointTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeAIXMLocation::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const DesignatedPointTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		DesignatedPointTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMNavaidTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMNavaidTimeSlice> ptr_t;

		NodeAIXMNavaidTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMNavaidTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeAIXMLocation::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const NavaidTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		NavaidTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMAngleIndicationTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMAngleIndicationTimeSlice> ptr_t;

		NodeAIXMAngleIndicationTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAngleIndicationTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AngleIndicationTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AngleIndicationTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMDistanceIndicationTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMDistanceIndicationTimeSlice> ptr_t;

		NodeAIXMDistanceIndicationTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDistanceIndicationTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const NodeLink::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const DistanceIndicationTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		DistanceIndicationTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMAirspaceTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspaceTimeSlice> ptr_t;

		NodeAIXMAirspaceTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspaceTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeText::ptr_t& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMGeometryComponent>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirspaceTimeSlice& get_timeslice(void) const;

	protected:
		static const std::string elementname;
		AirspaceTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMStandardLevelTableTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardLevelTableTimeSlice> ptr_t;

		NodeAIXMStandardLevelTableTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardLevelTableTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const StandardLevelTableTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		StandardLevelTableTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMStandardLevelColumnTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardLevelColumnTimeSlice> ptr_t;

		NodeAIXMStandardLevelColumnTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardLevelColumnTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const StandardLevelColumnTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		StandardLevelColumnTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMRouteTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteTimeSlice> ptr_t;

		NodeAIXMRouteTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const RouteTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		RouteTimeSlice m_timeslice;
		std::string m_desigprefix;
		std::string m_desigsecondletter;
		std::string m_designumber;
		std::string m_multiid;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMRouteSegmentTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteSegmentTimeSlice> ptr_t;

		NodeAIXMRouteSegmentTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteSegmentTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMStart>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const RouteSegmentTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		RouteSegmentTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMStandardInstrumentDepartureTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardInstrumentDepartureTimeSlice> ptr_t;

		NodeAIXMStandardInstrumentDepartureTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardInstrumentDepartureTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const StandardInstrumentDepartureTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		StandardInstrumentDepartureTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMStandardInstrumentArrivalTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardInstrumentArrivalTimeSlice> ptr_t;

		NodeAIXMStandardInstrumentArrivalTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardInstrumentArrivalTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const StandardInstrumentArrivalTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		StandardInstrumentArrivalTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMDepartureLegTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMDepartureLegTimeSlice> ptr_t;

		NodeAIXMDepartureLegTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDepartureLegTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMStartPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEndPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const DepartureLegTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		DepartureLegTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMArrivalLegTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMArrivalLegTimeSlice> ptr_t;

		NodeAIXMArrivalLegTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMArrivalLegTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMStartPoint>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMEndPoint>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const ArrivalLegTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		ArrivalLegTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMOrganisationAuthorityTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMOrganisationAuthorityTimeSlice> ptr_t;

		NodeAIXMOrganisationAuthorityTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMOrganisationAuthorityTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const OrganisationAuthorityTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		OrganisationAuthorityTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMSpecialDateTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMSpecialDateTimeSlice> ptr_t;

		NodeAIXMSpecialDateTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMSpecialDateTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const SpecialDateTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		SpecialDateTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMUnitTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMUnitTimeSlice> ptr_t;

		NodeAIXMUnitTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMUnitTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const UnitTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		UnitTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMAirTrafficManagementServiceTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMAirTrafficManagementServiceTimeSlice> ptr_t;

	        NodeAIXMAirTrafficManagementServiceTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirTrafficManagementServiceTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeLink>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMClientRoute>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const AirTrafficManagementServiceTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		AirTrafficManagementServiceTimeSlice m_timeslice;

                virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMFlightRestrictionTimeSlice : public NodeAIXMAnyTimeSlice {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightRestrictionTimeSlice> ptr_t;

	        NodeAIXMFlightRestrictionTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightRestrictionTimeSlice(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const Glib::RefPtr<NodeText>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMFlight>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMRegulatedRoute>& el);
		virtual void on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

		const FlightRestrictionTimeSlice& get_timeslice(void) const { return m_timeslice; }

	protected:
		static const std::string elementname;
		FlightRestrictionTimeSlice m_timeslice;

		virtual TimeSlice& get_tsbase(void) { return m_timeslice; }
	};

	class NodeAIXMTimeSlice : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeAIXMTimeSlice> ptr_t;

		NodeAIXMTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

		NodeAIXMObject(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const NodeText::ptr_t& el);

	protected:
		UUID m_uuid;

		template<typename T> void fake_uuid(const T& el);
		template<typename BT, typename TS> void on_timeslice(const NodeAIXMTimeSlice::ptr_t& el);
	};

	class NodeAIXMAirportHeliport : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMAirportHeliport> ptr_t;

		NodeAIXMAirportHeliport(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirportHeliport(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMAirportHeliportCollocation : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMAirportHeliportCollocation> ptr_t;

		NodeAIXMAirportHeliportCollocation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirportHeliportCollocation(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMDesignatedPoint : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMDesignatedPoint> ptr_t;

		NodeAIXMDesignatedPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDesignatedPoint(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMNavaid : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMNavaid> ptr_t;

		NodeAIXMNavaid(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMNavaid(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMAngleIndication : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMAngleIndication> ptr_t;

		NodeAIXMAngleIndication(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAngleIndication(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMDistanceIndication : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMDistanceIndication> ptr_t;

		NodeAIXMDistanceIndication(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDistanceIndication(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMAirspace : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMAirspace> ptr_t;

		NodeAIXMAirspace(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirspace(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMStandardLevelTable : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardLevelTable> ptr_t;

		NodeAIXMStandardLevelTable(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardLevelTable(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMStandardLevelColumn : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardLevelColumn> ptr_t;

		NodeAIXMStandardLevelColumn(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardLevelColumn(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMRoute : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMRoute> ptr_t;

		NodeAIXMRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRoute(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMRouteSegment : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMRouteSegment> ptr_t;

		NodeAIXMRouteSegment(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMRouteSegment(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMStandardInstrumentDeparture : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardInstrumentDeparture> ptr_t;

		NodeAIXMStandardInstrumentDeparture(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardInstrumentDeparture(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMStandardInstrumentArrival : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMStandardInstrumentArrival> ptr_t;

		NodeAIXMStandardInstrumentArrival(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMStandardInstrumentArrival(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMDepartureLeg : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMDepartureLeg> ptr_t;

		NodeAIXMDepartureLeg(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMDepartureLeg(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMArrivalLeg : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMArrivalLeg> ptr_t;

		NodeAIXMArrivalLeg(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMArrivalLeg(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMOrganisationAuthority : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMOrganisationAuthority> ptr_t;

		NodeAIXMOrganisationAuthority(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMOrganisationAuthority(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMSpecialDate : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMSpecialDate> ptr_t;

		NodeAIXMSpecialDate(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMSpecialDate(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMUnit : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMUnit> ptr_t;

		NodeAIXMUnit(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMUnit(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMAirTrafficManagementService : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMAirTrafficManagementService> ptr_t;

		NodeAIXMAirTrafficManagementService(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMAirTrafficManagementService(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeAIXMFlightRestriction : public NodeAIXMObject {
	public:
		typedef Glib::RefPtr<NodeAIXMFlightRestriction> ptr_t;

		NodeAIXMFlightRestriction(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
			return Node::ptr_t(new NodeAIXMFlightRestriction(parser, tagname, childnum, properties));
		}

		virtual void on_subelement(const NodeAIXMTimeSlice::ptr_t& el);
		virtual const std::string& get_element_name(void) const { return elementname; }
		static const std::string& get_static_element_name(void) { return elementname; }

	protected:
		static const std::string elementname;
	};

	class NodeADRMessageMember : public NodeNoIgnore {
	public:
		typedef Glib::RefPtr<NodeADRMessageMember> ptr_t;

		NodeADRMessageMember(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);
		static Node::ptr_t create(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties) {
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

	class NodeADRMessage : public Node {
	public:
		typedef Glib::RefPtr<NodeADRMessage> ptr_t;

		NodeADRMessage(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties);

		virtual void on_subelement(const NodeADRMessageMember::ptr_t& el);
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

	typedef Node::ptr_t (*factoryfunc_t)(ParseXML&, const std::string&, unsigned int, const AttributeList&);
	typedef std::map<std::string,factoryfunc_t> factory_t;
	factory_t m_factory;

	typedef std::map<std::string,std::string> namespacemap_t;
	namespacemap_t m_namespacemap;
	namespacemap_t m_namespaceshort;

	typedef std::vector<Node::ptr_t > parsestack_t;
	parsestack_t m_parsestack;

	unsigned int m_errorcnt;
	unsigned int m_warncnt;

	bool m_verbose;
};

};

#endif /* ADRPARSE_H */
