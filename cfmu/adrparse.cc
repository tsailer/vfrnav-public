#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iomanip>

#include "adrparse.hh"

using namespace ADR;

ParseXML::Node::Node(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: m_parser(parser), m_tagname(tagname), m_refcount(1), m_childnum(childnum), m_childcount(0)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "gml:id")
			continue;
		m_id = i->value;
	}
}

ParseXML::Node::~Node()
{
}

void ParseXML::Node::reference(void) const
{
	g_atomic_int_inc(&m_refcount);
}

void ParseXML::Node::unreference(void) const
{
	if (!g_atomic_int_dec_and_test(&m_refcount))
		return;
	delete this;
}

void ParseXML::Node::error(const std::string& text) const
{
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.error(text1);
}

void ParseXML::Node::error(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		error(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.error(oss.str(), text1);
}

void ParseXML::Node::warning(const std::string& text) const
{
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.warning(text1);
}

void ParseXML::Node::warning(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		warning(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.warning(oss.str(), text1);
}

void ParseXML::Node::info(const std::string& text) const
{
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.info(text1);
}

void ParseXML::Node::info(const_ptr_t child, const std::string& text) const
{
	if (!child) {
		info(text);
		return;
	}
	std::ostringstream oss;
	oss << child->get_element_name();
	if (get_childcount() != 1)
		oss << '[' << ((int)get_childcount() - 1) << ']';
	std::string text1(text);
	if (!get_id().empty())
		text1 += " [" + get_id() + "]";
	m_parser.info(oss.str(), text1);
}

void ParseXML::Node::on_characters(const Glib::ustring& characters)
{
}

void ParseXML::Node::on_subelement(const ptr_t& el)
{
	++m_childcount;
	{
		NodeText::ptr_t elx(NodeText::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeLink::ptr_t elx(NodeLink::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeIgnore::ptr_t elx(NodeIgnore::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRMessageMember::ptr_t elx(NodeADRMessageMember::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRWeekDayPattern::ptr_t elx(NodeADRWeekDayPattern::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRSpecialDayPattern::ptr_t elx(NodeADRSpecialDayPattern::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRTimePatternCombination::ptr_t elx(NodeADRTimePatternCombination::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRTimeTable1::ptr_t elx(NodeADRTimeTable1::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRTimeTable::ptr_t elx(NodeADRTimeTable::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRLevels::ptr_t elx(NodeADRLevels::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRDefaultApplicability::ptr_t elx(NodeADRDefaultApplicability::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRNonDefaultApplicability::ptr_t elx(NodeADRNonDefaultApplicability::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRIntermediateSignificantPoint::ptr_t elx(NodeADRIntermediateSignificantPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRIntermediatePoint::ptr_t elx(NodeADRIntermediatePoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMExtensionBase::ptr_t elx(NodeAIXMExtensionBase::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMExtension::ptr_t elx(NodeAIXMExtension::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAirspaceActivation::ptr_t elx(NodeAIXMAirspaceActivation::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMActivation::ptr_t elx(NodeAIXMActivation::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMElevatedPoint::ptr_t elx(NodeAIXMElevatedPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMPoint::ptr_t elx(NodeAIXMPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMARP::ptr_t elx(NodeAIXMARP::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMLocation::ptr_t elx(NodeAIXMLocation::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMCity::ptr_t elx(NodeAIXMCity::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMServedCity::ptr_t elx(NodeAIXMServedCity::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLValidTime::ptr_t elx(NodeGMLValidTime::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFeatureLifetime::ptr_t elx(NodeAIXMFeatureLifetime::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLBeginPosition::ptr_t elx(NodeGMLBeginPosition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLEndPosition::ptr_t elx(NodeGMLEndPosition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLTimePeriod::ptr_t elx(NodeGMLTimePeriod::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLLinearRing::ptr_t elx(NodeGMLLinearRing::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLInterior::ptr_t elx(NodeGMLInterior::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLExterior::ptr_t elx(NodeGMLExterior::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLPolygonPatch::ptr_t elx(NodeGMLPolygonPatch::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeGMLPatches::ptr_t elx(NodeGMLPatches::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMSurface::ptr_t elx(NodeAIXMSurface::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMHorizontalProjection::ptr_t elx(NodeAIXMHorizontalProjection::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAirspaceVolumeDependency::ptr_t elx(NodeAIXMAirspaceVolumeDependency::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMContributorAirspace::ptr_t elx(NodeAIXMContributorAirspace::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAirspaceVolume::ptr_t elx(NodeAIXMAirspaceVolume::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMTheAirspaceVolume::ptr_t elx(NodeAIXMTheAirspaceVolume::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAirspaceGeometryComponent::ptr_t elx(NodeAIXMAirspaceGeometryComponent::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMGeometryComponent::ptr_t elx(NodeAIXMGeometryComponent::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMEnRouteSegmentPoint::ptr_t elx(NodeAIXMEnRouteSegmentPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMTerminalSegmentPoint::ptr_t elx(NodeAIXMTerminalSegmentPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMStart::ptr_t elx(NodeAIXMStart::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMEnd::ptr_t elx(NodeAIXMEnd::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMStartPoint::ptr_t elx(NodeAIXMStartPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMEndPoint::ptr_t elx(NodeAIXMEndPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRConnectingPoint::ptr_t elx(NodeADRConnectingPoint::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRInitialApproachFix::ptr_t elx(NodeADRInitialApproachFix::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAirspaceLayer::ptr_t elx(NodeAIXMAirspaceLayer::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMLevels::ptr_t elx(NodeAIXMLevels::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAvailabilityBase::ptr_t elx(NodeAIXMAvailabilityBase::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAvailability::ptr_t elx(NodeAIXMAvailability::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMClientRoute::ptr_t elx(NodeAIXMClientRoute::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMRoutePortion::ptr_t elx(NodeAIXMRoutePortion::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMDirectFlightClass::ptr_t elx(NodeAIXMDirectFlightClass::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightConditionCircumstance::ptr_t elx(NodeAIXMFlightConditionCircumstance::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMOperationalCondition::ptr_t elx(NodeAIXMOperationalCondition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightRestrictionLevel::ptr_t elx(NodeAIXMFlightRestrictionLevel::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightLevel::ptr_t elx(NodeAIXMFlightLevel::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMDirectFlightSegment::ptr_t elx(NodeAIXMDirectFlightSegment::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMElementRoutePortionElement::ptr_t elx(NodeAIXMElementRoutePortionElement::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMElementDirectFlightElement::ptr_t elx(NodeAIXMElementDirectFlightElement::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightRoutingElement::ptr_t elx(NodeAIXMFlightRoutingElement::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMRouteElement::ptr_t elx(NodeAIXMRouteElement::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightRestrictionRoute::ptr_t elx(NodeAIXMFlightRestrictionRoute::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMRegulatedRoute::ptr_t elx(NodeAIXMRegulatedRoute::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAircraftCharacteristic::ptr_t elx(NodeAIXMAircraftCharacteristic::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightConditionAircraft::ptr_t elx(NodeAIXMFlightConditionAircraft::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightCharacteristic::ptr_t elx(NodeAIXMFlightCharacteristic::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightConditionFlight::ptr_t elx(NodeAIXMFlightConditionFlight::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightConditionOperand::ptr_t elx(NodeAIXMFlightConditionOperand::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightConditionRoutePortionCondition::ptr_t elx(NodeAIXMFlightConditionRoutePortionCondition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightConditionDirectFlightCondition::ptr_t elx(NodeAIXMFlightConditionDirectFlightCondition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRAirspaceBorderCrossingObject::ptr_t elx(NodeADRAirspaceBorderCrossingObject::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeADRBorderCrossingCondition::ptr_t elx(NodeADRBorderCrossingCondition::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMElementBase::ptr_t elx(NodeAIXMElementBase::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMElement::ptr_t elx(NodeAIXMElement::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlightConditionCombination::ptr_t elx(NodeAIXMFlightConditionCombination::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMFlight::ptr_t elx(NodeAIXMFlight::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMAnyTimeSlice::ptr_t elx(NodeAIXMAnyTimeSlice::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMTimeSlice::ptr_t elx(NodeAIXMTimeSlice::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	{
		NodeAIXMObject::ptr_t elx(NodeAIXMObject::ptr_t::cast_dynamic(el));
		if (elx) {
			on_subelement(elx);
			return;
		}
	}
	error(el, "Element " + get_element_name() + " does not support unknown subelements");
}

void ParseXML::Node::on_subelement(const NodeIgnore::ptr_t& el)
{
}

void ParseXML::Node::on_subelement(const NodeText::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support text subelements");
}

void ParseXML::Node::on_subelement(const NodeLink::ptr_t& el)
{
	error(el, "Element " + get_element_name() + " does not support link subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRWeekDayPattern>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRWeekDayPattern::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRSpecialDayPattern>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRSpecialDayPattern::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRTimePatternCombination>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRTimePatternCombination::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRTimeTable1>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRTimeTable1::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRTimeTable::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRLevels>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRLevels::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRDefaultApplicability>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRDefaultApplicability::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRNonDefaultApplicability>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRNonDefaultApplicability::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRIntermediateSignificantPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRIntermediateSignificantPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRIntermediatePoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRIntermediatePoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMExtensionBase>& el)
{
	error(el, "Element " + get_element_name() + " does not support AIXM Extension subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMExtension::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceActivation>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAirspaceActivation::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMActivation::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMElevatedPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMElevatedPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMARP>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMARP::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMLocation>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMLocation::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMCity>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMCity::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMServedCity>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMServedCity::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLValidTime>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLValidTime::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFeatureLifetime>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFeatureLifetime::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLBeginPosition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLBeginPosition::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLEndPosition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLEndPosition::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLTimePeriod>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLTimePeriod::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLLinearRing>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLLinearRing::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLInterior>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLInterior::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLExterior>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLExterior::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLPolygonPatch>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLPolygonPatch::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeGMLPatches>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeGMLPatches::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMSurface>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMSurface::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMHorizontalProjection>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMHorizontalProjection::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolumeDependency>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAirspaceVolumeDependency::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMContributorAirspace>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMContributorAirspace::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolume>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAirspaceVolume::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMTheAirspaceVolume>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMTheAirspaceVolume::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceGeometryComponent>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAirspaceGeometryComponent::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMGeometryComponent>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMGeometryComponent::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMEnRouteSegmentPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMTerminalSegmentPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMStart>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMStart::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMEnd::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMStartPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMStartPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMEndPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMEndPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRConnectingPoint>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRConnectingPoint::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRInitialApproachFix>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRInitialApproachFix::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAirspaceLayer::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMLevels::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAvailabilityBase>& el)
{
	error(el, "Element " + get_element_name() + " does not support AIXM Availability subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAvailability::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMClientRoute>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMClientRoute::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMRoutePortion::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightClass>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMDirectFlightClass::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCircumstance>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightConditionCircumstance::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMOperationalCondition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMOperationalCondition::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionLevel>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightRestrictionLevel::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightLevel>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightLevel::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightSegment>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMDirectFlightSegment::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMElementRoutePortionElement>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMElementRoutePortionElement::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMElementDirectFlightElement>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMElementDirectFlightElement::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightRoutingElement>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightRoutingElement::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMRouteElement>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMRouteElement::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionRoute>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightRestrictionRoute::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMRegulatedRoute>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMRegulatedRoute::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAircraftCharacteristic>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMAircraftCharacteristic::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionAircraft>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightConditionAircraft::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightCharacteristic>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightCharacteristic::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionFlight>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightConditionFlight::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionOperand>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightConditionOperand::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionRoutePortionCondition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightConditionRoutePortionCondition::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionDirectFlightCondition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightConditionDirectFlightCondition::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRAirspaceBorderCrossingObject>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRAirspaceBorderCrossingObject::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRBorderCrossingCondition>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRBorderCrossingCondition::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMElementBase>& el)
{
	error(el, "Element " + get_element_name() + " does not support AIXM Element subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMElement>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMElement::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCombination>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlightConditionCombination::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMFlight>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMFlight::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMAnyTimeSlice>& el)
{
	error(el, "Element " + get_element_name() + " does not support aixm time slice subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMTimeSlice>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeAIXMTimeSlice::get_static_element_name() + " subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeAIXMObject>& el)
{
	error(el, "Element " + get_element_name() + " does not support aixm object subelements");
}

void ParseXML::Node::on_subelement(const Glib::RefPtr<NodeADRMessageMember>& el)
{
	error(el, "Element " + get_element_name() + " does not support " + NodeADRMessageMember::get_static_element_name() + " subelements");
}

ParseXML::NodeIgnore::NodeIgnore(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name)
	: Node(parser, tagname, childnum, properties), m_elementname(name)
{
}

void ParseXML::NodeNoIgnore::on_subelement(const NodeIgnore::ptr_t& el)
{
	if (true)
		std::cerr << "ADR Parser: Element " + get_element_name() + " does not support ignored subelements" << std::endl;
	error("Element " + get_element_name() + " does not support ignored subelements");
}

ParseXML::NodeText::NodeText(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_factor(1), m_factorl(1), m_texttype(find_type(name))
{
	check_attr(properties);
}

ParseXML::NodeText::NodeText(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, texttype_t tt)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_factor(1), m_factorl(1), m_texttype(tt)
{
	check_attr(properties);
}

void ParseXML::NodeText::check_attr(const AttributeList& properties)
{
	switch (get_type()) {
	case tt_gmlpos:
		for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
			if (i->name != "srsName")
				continue;
			if (i->value != "urn:ogc:def:crs:EPSG::4326")
				warning(get_element_name() + ": unknown coordinate system " + i->value);
		}
		break;

	case tt_aixmdistance:
	case tt_aixmexceedlength:
		for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
			if (i->name != "uom")
				continue;
			std::string val(Glib::ustring(i->value).uppercase());
			if (val == "NM")
				m_factor = 1;
			else if (val == "M")
				m_factor = Point::km_to_nmi_dbl * 1e-3;
			else if (val == "KM")
				m_factor = Point::km_to_nmi_dbl;
			else if (val == "FT")
				m_factor = Point::ft_to_m_dbl * 1e-3 * Point::km_to_nmi_dbl;
			else
				error(get_element_name() + ": unknown unit of measure: " + val);
		}
		break;

	case tt_aixmupperlimit:
	case tt_aixmlowerlimit:
	case tt_aixmupperlimitaltitude:
	case tt_aixmlowerlimitaltitude:
	case tt_aixmupperlevel:
	case tt_aixmlowerlevel:
		for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
			if (i->name != "uom")
				continue;
			std::string val(Glib::ustring(i->value).uppercase());
			if (val == "FT") {
				m_factor = 1;
				m_factorl = 1;
			} else if (val == "FL") {
				m_factor = 100;
				m_factorl = 100;
			} else {
				error(get_element_name() + ": unknown unit of measure: " + val);
			}
		}
		break;

	default:
		break;
	}
}

void ParseXML::NodeText::on_characters(const Glib::ustring& characters)
{
	m_text += characters;
}

const std::string& ParseXML::NodeText::get_static_element_name(texttype_t tt)
{
	switch (tt) {
	case tt_adrconditionalroutetype:
	{
		static const std::string r("adr:conditionalRouteType");
		return r;
	}

	case tt_adrday:
	{
		static const std::string r("adr:day");
		return r;
	}

	case tt_adrenabled:
	{
		static const std::string r("adr:enabled");
		return r;
	}

	case tt_adrenddate:
	{
		static const std::string r("adr:endDate");
		return r;
	}

	case tt_adrendtime:
	{
		static const std::string r("adr:endTime");
		return r;
	}

	case tt_adrexclude:
	{
		static const std::string r("adr:exclude");
		return r;
	}

	case tt_adrflexibleuse:
	{
		static const std::string r("adr:flexibleUse");
		return r;
	}

	case tt_adrlevel1:
	{
		static const std::string r("adr:level1");
		return r;
	}

	case tt_adrlevel2:
	{
		static const std::string r("adr:level2");
		return r;
	}

	case tt_adrlevel3:
	{
		static const std::string r("adr:level3");
		return r;
	}

	case tt_adrprocessingindicator:
	{
		static const std::string r("adr:processingIndicator");
		return r;
	}

	case tt_adrspecialday:
	{
		static const std::string r("adr:specialDay");
		return r;
	}

	case tt_adrstartdate:
	{
		static const std::string r("adr:startDate");
		return r;
	}

	case tt_adrstarttime:
	{
		static const std::string r("adr:startTime");
		return r;
	}

	case tt_adrtimeoperand:
	{
		static const std::string r("adr:timeOperand");
		return r;
	}

	case tt_adrtimereference:
	{
		static const std::string r("adr:timeReference");
		return r;
	}

	case tt_aixmangle:
	{
		static const std::string r("aixm:angle");
		return r;
	}

	case tt_aixmangletype:
	{
		static const std::string r("aixm:angleType");
		return r;
	}

	case tt_aixmcontroltype:
	{
		static const std::string r("aixm:controlType");
		return r;
	}

	case tt_aixmdateday:
	{
		static const std::string r("aixm:dateDay");
		return r;
	}

	case tt_aixmdateyear:
	{
		static const std::string r("aixm:dateYear");
		return r;
	}

	case tt_aixmdependency:
	{
		static const std::string r("aixm:dependency");
		return r;
	}

	case tt_aixmdesignator:
	{
		static const std::string r("aixm:designator");
		return r;
	}

	case tt_aixmdesignatoriata:
	{
		static const std::string r("aixm:designatorIATA");
		return r;
	}

	case tt_aixmdesignatoricao:
	{
		static const std::string r("aixm:designatorICAO");
		return r;
	}

	case tt_aixmdesignatornumber:
	{
		static const std::string r("aixm:designatorNumber");
		return r;
	}

	case tt_aixmdesignatorprefix:
	{
		static const std::string r("aixm:designatorPrefix");
		return r;
	}

	case tt_aixmdesignatorsecondletter:
	{
		static const std::string r("aixm:designatorSecondLetter");
		return r;
	}

	case tt_aixmdirection:
	{
		static const std::string r("aixm:direction");
		return r;
	}

	case tt_aixmdistance:
	{
		static const std::string r("aixm:distance");
		return r;
	}

	case tt_aixmengine:
	{
		static const std::string r("aixm:engine");
		return r;
	}

	case tt_aixmexceedlength:
	{
		static const std::string r("aixm:exceedLength");
		return r;
	}

	case tt_aixmindex:
	{
		static const std::string r("aixm:index");
		return r;
	}

	case tt_aixminstruction:
	{
		static const std::string r("aixm:instruction");
		return r;
	}

	case tt_aixminterpretation:
	{
		static const std::string r("aixm:interpretation");
		return r;
	}

	case tt_aixmlocaltype:
	{
		static const std::string r("aixm:localType");
		return r;
	}

	case tt_aixmlocationindicatoricao:
	{
		static const std::string r("aixm:locationIndicatorICAO");
		return r;
	}

	case tt_aixmlogicaloperator:
	{
		static const std::string r("aixm:logicalOperator");
		return r;
	}

	case tt_aixmlowerlevel:
	{
		static const std::string r("aixm:lowerLevel");
		return r;
	}

	case tt_aixmlowerlevelreference:
	{
		static const std::string r("aixm:lowerLevelReference");
		return r;
	}

	case tt_aixmlowerlimit:
	{
		static const std::string r("aixm:lowerLimit");
		return r;
	}

	case tt_aixmlowerlimitaltitude:
	{
		static const std::string r("aixm:lowerLimitAltitude");
		return r;
	}

	case tt_aixmlowerlimitreference:
	{
		static const std::string r("aixm:lowerLimitReference");
		return r;
	}

	case tt_aixmmilitary:
	{
		static const std::string r("aixm:military");
		return r;
	}

	case tt_aixmmultipleidentifier:
	{
		static const std::string r("aixm:multipleIdentifier");
		return r;
	}

	case tt_aixmname:
	{
		static const std::string r("aixm:name");
		return r;
	}

	case tt_aixmnavigationspecification:
	{
		static const std::string r("aixm:navigationSpecification");
		return r;
	}

	case tt_aixmnumberengine:
	{
		static const std::string r("aixm:numberEngine");
		return r;
	}

	case tt_aixmoperation:
	{
		static const std::string r("aixm:operation");
		return r;
	}

	case tt_aixmordernumber:
	{
		static const std::string r("aixm:orderNumber");
		return r;
	}

	case tt_aixmpurpose:
	{
		static const std::string r("aixm:purpose");
		return r;
	}

	case tt_aixmreferencelocation:
	{
		static const std::string r("aixm:referenceLocation");
		return r;
	}

	case tt_aixmrelationwithlocation:
	{
		static const std::string r("aixm:relationWithLocation");
		return r;
	}

	case tt_aixmseries:
	{
		static const std::string r("aixm:series");
		return r;
	}

	case tt_aixmstandardicao:
	{
		static const std::string r("aixm:standardICAO");
		return r;
	}

	case tt_aixmstatus:
	{
		static const std::string r("aixm:status");
		return r;
	}

	case tt_aixmtype:
	{
		static const std::string r("aixm:type");
		return r;
	}

	case tt_aixmtypeaircrafticao:
	{
		static const std::string r("aixm:typeAircraftICAO");
		return r;
	}

	case tt_aixmupperlevel:
	{
		static const std::string r("aixm:upperLevel");
		return r;
	}

	case tt_aixmupperlevelreference:
	{
		static const std::string r("aixm:upperLevelReference");
		return r;
	}

	case tt_aixmupperlimit:
	{
		static const std::string r("aixm:upperLimit");
		return r;
	}

	case tt_aixmupperlimitaltitude:
	{
		static const std::string r("aixm:upperLimitAltitude");
		return r;
	}

	case tt_aixmupperlimitreference:
	{
		static const std::string r("aixm:upperLimitReference");
		return r;
	}

	case tt_aixmverticalseparationcapability:
	{
		static const std::string r("aixm:verticalSeparationCapability");
		return r;
	}

	case tt_gmlidentifier:
	{
		static const std::string r("gml:identifier");
		return r;
	}

	case tt_gmlpos:
	{
		static const std::string r("gml:pos");
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

void ParseXML::NodeText::check_type_order(void)
{
	for (texttype_t tt(tt_first); tt < tt_invalid; ) {
		texttype_t ttp(tt);
		tt = (texttype_t)(tt + 1);
		if (!(tt < tt_invalid))
			break;
		if (get_static_element_name(ttp) < get_static_element_name(tt))
			continue;
		throw std::runtime_error("NodeText ordering error: " + get_static_element_name(ttp) +
					 " >= " + get_static_element_name(tt));
	}
}

ParseXML::NodeText::texttype_t ParseXML::NodeText::find_type(const std::string& name)
{
	if (false) {
		for (texttype_t tt(tt_first); tt < tt_invalid; tt = (texttype_t)(tt + 1))
			if (name == get_static_element_name(tt))
				return tt;
		return tt_invalid;
	}
	texttype_t tb(tt_first), tt(tt_invalid);
	for (;;) {
		texttype_t tm((texttype_t)((tb + tt) >> 1));
		if (tm == tb) {
			if (name == get_static_element_name(tb))
				return tb;
			return tt_invalid;
		}
		int c(name.compare(get_static_element_name(tm)));
		if (!c)
			return tm;
		if (c < 0)
			tt = tm;
		else
			tb = tm;
	}
}

long ParseXML::NodeText::get_long(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	long v(strtol(cp, &ep, 10));
	if (!*ep && ep != cp)
		return v * (long)m_factorl;
	error("invalid signed number " + get_text());
	return 0;
}

unsigned long ParseXML::NodeText::get_ulong(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	unsigned long v(strtoul(cp, &ep, 10));
	if (!*ep && ep != cp)
		return v * m_factorl;
	error("invalid unsigned number " + get_text());
	return 0;
}

double ParseXML::NodeText::get_double(void) const
{
	const char *cp(m_text.c_str());
	char *ep;
	double v(strtod(cp, &ep));
	if (!*ep && ep != cp)
		return v * m_factor;
	error("invalid floating point number " + get_text());
	return std::numeric_limits<double>::quiet_NaN();
}

Point::coord_t ParseXML::NodeText::get_lat(void) const
{
	Point pt;
	if (!(pt.set_str(get_text()) & Point::setstr_lat)) {
		error("invalid latitude " + get_text());
		pt.set_invalid();
	}
	return pt.get_lat();
}

Point::coord_t ParseXML::NodeText::get_lon(void) const
{
	Point pt;
	if (!(pt.set_str(get_text()) & Point::setstr_lon)) {
		error("invalid longitude " + get_text());
		pt.set_invalid();
	}
	return pt.get_lon();
}

Point ParseXML::NodeText::get_coord_deg(void) const
{
	Point pt;
	pt.set_invalid();
	const char *cp(get_text().c_str());
	char *cp1;
	double lat(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || *cp1 != ' ')
		return pt;
	cp = cp1 + 1;
	double lon(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return pt;
	pt.set_lat_deg_dbl(lat);
	pt.set_lon_deg_dbl(lon);
	return pt;
}

std::pair<Point,int32_t> ParseXML::NodeText::get_coord_3d(void) const
{
	std::pair<Point,int32_t> r;
	r.first.set_invalid();
	r.second = std::numeric_limits<int32_t>::min();
	const char *cp(get_text().c_str());
	char *cp1;
	double lat(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || *cp1 != ' ')
		return r;
	cp = cp1 + 1;
	double lon(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return r;
	r.first.set_lat_deg_dbl(lat);
	r.first.set_lon_deg_dbl(lon);
	if (!*cp1)
		return r;
	cp = cp1 + 1;
	double alt(strtod(cp, &cp1));
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return r;
	r.second = Point::round<int32_t,double>(alt * Point::m_to_ft_dbl);
	return r;
}

UUID ParseXML::NodeText::get_uuid(void) const
{
	return UUID::from_str(get_text());
}

int ParseXML::NodeText::get_timehhmm(void) const
{
	const char *cp(get_text().c_str());
	char *cp1;
	int t(strtoul(cp, &cp1, 10) * 60);
	if (cp1 == cp || !cp1 || *cp1 != ':')
		return -1;
	cp = cp1 + 1;
	t += strtoul(cp, &cp1, 10);
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return -1;
	return t;
}

timetype_t ParseXML::NodeText::get_dateyyyymmdd(void) const
{
	const char *cp(get_text().c_str());
	char *cp1;
	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = strtoul(cp, &cp1, 10) - 1900;
	if (cp1 == cp || !cp1 || *cp1 != '-')
		return std::numeric_limits<timetype_t>::max();
	cp = cp1 + 1;
	tm.tm_mon = strtoul(cp, &cp1, 10) - 1;
	if (cp1 == cp || !cp1 || *cp1 != '-')
		return std::numeric_limits<timetype_t>::max();
	cp = cp1 + 1;
	tm.tm_mday = strtoul(cp, &cp1, 10);
	if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' '))
		return std::numeric_limits<timetype_t>::max();
	time_t t(timegm(&tm));
	if (t != (time_t)-1)
		return t;
	return std::numeric_limits<timetype_t>::max();
}

int ParseXML::NodeText::get_daynr(void) const
{
	if (get_text() == "SUN")
		return 0;
	if (get_text() == "MON")
		return 1;
	if (get_text() == "TUE")
		return 2;
	if (get_text() == "WED")
		return 3;
	if (get_text() == "THU")
		return 4;
	if (get_text() == "FRI")
		return 5;
	if (get_text() == "SAT")
		return 6;
	return -1;
}

int ParseXML::NodeText::get_yesno(void) const
{
	if (get_text() == "YES")
		return 1;
	if (get_text() == "NO")
		return 0;
	return -1;
}

void ParseXML::NodeText::update(AltRange& ar)
{
	switch (get_type()) {
	case tt_aixmupperlimit:
	case tt_aixmupperlevel:
	case tt_aixmupperlimitaltitude:
		if (get_text() == "GND") {
			ar.set_upper_alt(0);
			ar.set_upper_mode(AltRange::alt_height);
		} else if (get_text() == "FLOOR") {
			ar.set_upper_alt(0);
			ar.set_upper_mode(AltRange::alt_floor);
		} else if (get_text() == "CEILING") {
			ar.set_upper_alt(AltRange::altmax);
			ar.set_upper_mode(AltRange::alt_ceiling);
		} else if (get_text() == "UNL") {
			ar.set_upper_alt(AltRange::altmax);
		} else {
			ar.set_upper_alt(get_long());
		}
		break;

	case tt_aixmlowerlimit:
	case tt_aixmlowerlevel:
	case tt_aixmlowerlimitaltitude:
		if (get_text() == "GND") {
			ar.set_lower_alt(0);
			ar.set_lower_mode(AltRange::alt_height);
		} else if (get_text() == "FLOOR") {
			ar.set_lower_alt(0);
			ar.set_lower_mode(AltRange::alt_floor);
		} else if (get_text() == "CEILING") {
			ar.set_lower_alt(AltRange::altmax);
			ar.set_lower_mode(AltRange::alt_ceiling);
		} else if (get_text() == "UNL") {
			ar.set_lower_alt(AltRange::altmax);
		} else {
			ar.set_lower_alt(get_long());
		}
		break;

	case tt_aixmupperlevelreference:
	case tt_aixmupperlimitreference:
		if ((ar.get_upper_mode() == AltRange::alt_height && !ar.get_upper_alt()) ||
		    ar.get_upper_mode() == AltRange::alt_floor ||
		    ar.get_upper_mode() == AltRange::alt_ceiling)
			break;
		if (get_text() == "STD")
			ar.set_upper_mode(AltRange::alt_std);
		else if (get_text() == "MSL")
			ar.set_upper_mode(AltRange::alt_qnh);
		else
			error(get_element_name() + ": unknown upper limit reference " + get_text());
		break;

	case tt_aixmlowerlevelreference:
	case tt_aixmlowerlimitreference:
		if ((ar.get_lower_mode() == AltRange::alt_height && !ar.get_lower_alt()) ||
		    ar.get_lower_mode() == AltRange::alt_floor ||
		    ar.get_lower_mode() == AltRange::alt_ceiling)
			break;
		if (get_text() == "STD")
			ar.set_lower_mode(AltRange::alt_std);
		else if (get_text() == "MSL")
			ar.set_lower_mode(AltRange::alt_qnh);
		else
			error(get_element_name() + ": unknown lower limit reference " + get_text());
		break;

	default:
		error(get_element_name() + ": cannot update altrange with " + get_text());
		break;
	}
}

ParseXML::NodeLink::NodeLink(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, const std::string& name)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_linktype(find_type(name))
{
	process_attr(properties);
}

ParseXML::NodeLink::NodeLink(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties, linktype_t lt)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_linktype(lt)
{
	process_attr(properties);
}

void ParseXML::NodeLink::process_attr(const AttributeList& properties)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "xlink:href")
			continue;
		m_uuid = UUID::from_str(i->value);
		if (m_uuid.is_nil())
			warning(get_element_name() + " invalid UUID " + i->value);
	}
}

const std::string& ParseXML::NodeLink::get_static_element_name(linktype_t lt)
{
	switch (lt) {
	case lt_adrenteredairspace:
	{
		static const std::string r("adr:enteredAirspace");
		return r;
	}

	case lt_adrexitedairspace:
	{
		static const std::string r("adr:exitedAirspace");
		return r;
	}

	case lt_adrpointchoicefixdesignatedpoint:
	{
		static const std::string r("adr:pointChoice_fixDesignatedPoint");
		return r;
	}

	case lt_adrpointchoicenavaidsystem:
	{
		static const std::string r("adr:pointChoice_navaidSystem");
		return r;
	}

	case lt_adrreferencedprocedure:
	{
		static const std::string r("adr:referencedProcedure");
		return r;
	}

	case lt_aixmairportheliport:
	{
		static const std::string r("aixm:airportHeliport");
		return r;
	}

	case lt_aixmarrival:
	{
		static const std::string r("aixm:arrival");
		return r;
	}

	case lt_aixmauthority:
	{
		static const std::string r("aixm:authority");
		return r;
	}

	case lt_aixmclientairspace:
	{
		static const std::string r("aixm:clientAirspace");
		return r;
	}

	case lt_aixmdeparture:
	{
		static const std::string r("aixm:departure");
		return r;
	}

	case lt_aixmdependentairport:
	{
		static const std::string r("aixm:dependentAirport");
		return r;
	}

	case lt_aixmdiscretelevelseries:
	{
		static const std::string r("aixm:discreteLevelSeries");
		return r;
	}

	case lt_aixmelementairspaceelement:
	{
		static const std::string r("aixm:element_airspaceElement");
		return r;
	}

	case lt_aixmelementstandardinstrumentarrivalelement:
	{
		static const std::string r("aixm:element_standardInstrumentArrivalElement");
		return r;
	}

	case lt_aixmelementstandardinstrumentdepartureelement:
	{
		static const std::string r("aixm:element_standardInstrumentDepartureElement");
		return r;
	}

	case lt_aixmendairportreferencepoint:
	{
		static const std::string r("aixm:end_airportReferencePoint");
		return r;
	}

	case lt_aixmendfixdesignatedpoint:
	{
		static const std::string r("aixm:end_fixDesignatedPoint");
		return r;
	}

	case lt_aixmendnavaidsystem:
	{
		static const std::string r("aixm:end_navaidSystem");
		return r;
	}

	case lt_aixmfix:
	{
		static const std::string r("aixm:fix");
		return r;
	}

	case lt_aixmflightconditionairportheliportcondition:
	{
		static const std::string r("aixm:flightCondition_airportHeliportCondition");
		return r;
	}

	case lt_aixmflightconditionairspacecondition:
	{
		static const std::string r("aixm:flightCondition_airspaceCondition");
		return r;
	}

	case lt_aixmflightconditionstandardinstrumentarrivalcondition:
	{
		static const std::string r("aixm:flightCondition_standardInstrumentArrivalCondition");
		return r;
	}

	case lt_aixmflightconditionstandardinstrumentdeparturecondition:
	{
		static const std::string r("aixm:flightCondition_standardInstrumentDepartureCondition");
		return r;
	}

	case lt_aixmhostairport:
	{
		static const std::string r("aixm:hostAirport");
		return r;
	}

	case lt_aixmleveltable:
	{
		static const std::string r("aixm:levelTable");
		return r;
	}

	case lt_aixmpointchoiceairportreferencepoint:
	{
		static const std::string r("aixm:pointChoice_airportReferencePoint");
		return r;
	}

	case lt_aixmpointchoicefixdesignatedpoint:
	{
		static const std::string r("aixm:pointChoice_fixDesignatedPoint");
		return r;
	}

	case lt_aixmpointchoicenavaidsystem:
	{
		static const std::string r("aixm:pointChoice_navaidSystem");
		return r;
	}

	case lt_aixmpointelementfixdesignatedpoint:
	{
		static const std::string r("aixm:pointElement_fixDesignatedPoint");
		return r;
	}

	case lt_aixmpointelementnavaidsystem:
	{
		static const std::string r("aixm:pointElement_navaidSystem");
		return r;
	}

	case lt_aixmreferencedroute:
	{
		static const std::string r("aixm:referencedRoute");
		return r;
	}

	case lt_aixmrouteformed:
	{
		static const std::string r("aixm:routeFormed");
		return r;
	}

	case lt_aixmserviceprovider:
	{
		static const std::string r("aixm:serviceProvider");
		return r;
	}

	case lt_aixmsignificantpointconditionfixdesignatedpoint:
	{
		static const std::string r("aixm:significantPointCondition_fixDesignatedPoint");
		return r;
	}

	case lt_aixmsignificantpointconditionnavaidsystem:
	{
		static const std::string r("aixm:significantPointCondition_navaidSystem");
		return r;
	}

	case lt_aixmstartairportreferencepoint:
	{
		static const std::string r("aixm:start_airportReferencePoint");
		return r;
	}

	case lt_aixmstartfixdesignatedpoint:
	{
		static const std::string r("aixm:start_fixDesignatedPoint");
		return r;
	}

	case lt_aixmstartnavaidsystem:
	{
		static const std::string r("aixm:start_navaidSystem");
		return r;
	}

	case lt_aixmtheairspace:
	{
		static const std::string r("aixm:theAirspace");
		return r;
	}

	case lt_gmlpointproperty:
	{
		static const std::string r("gml:pointProperty");
		return r;
	}

	default:
	case lt_invalid:
	{
		static const std::string r("?");
		return r;
	}
	}
}

void ParseXML::NodeLink::check_type_order(void)
{
	for (linktype_t lt(lt_first); lt < lt_invalid; ) {
		linktype_t ltp(lt);
		lt = (linktype_t)(lt + 1);
		if (!(lt < lt_invalid))
			break;
		if (get_static_element_name(ltp) < get_static_element_name(lt))
			continue;
		throw std::runtime_error("NodeLink ordering error: " + get_static_element_name(ltp) +
					 " >= " + get_static_element_name(lt));
	}
}

ParseXML::NodeLink::linktype_t ParseXML::NodeLink::find_type(const std::string& name)
{
	if (false) {
		for (linktype_t lt(lt_first); lt < lt_invalid; lt = (linktype_t)(lt + 1))
			if (name == get_static_element_name(lt))
				return lt;
		return lt_invalid;
	}
	linktype_t lb(lt_first), lt(lt_invalid);
	for (;;) {
		linktype_t lm((linktype_t)((lb + lt) >> 1));
		if (lm == lb) {
			if (name == get_static_element_name(lb))
				return lb;
			return lt_invalid;
		}
		int c(name.compare(get_static_element_name(lm)));
		if (!c)
			return lm;
		if (c < 0)
			lt = lm;
		else
			lb = lm;
	}
}

ParseXML::NodeAIXMElementBase::NodeAIXMElementBase(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

ParseXML::NodeAIXMExtensionBase::NodeAIXMExtensionBase(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

ParseXML::NodeAIXMAvailabilityBase::NodeAIXMAvailabilityBase(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

const std::string ParseXML::NodeADRWeekDayPattern::elementname("adr:weekDayPattern");

ParseXML::NodeADRWeekDayPattern::NodeADRWeekDayPattern(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
	m_timepat.set_type(TimePattern::type_weekday);
}

void ParseXML::NodeADRWeekDayPattern::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrday:
	{
		int d(el->get_daynr());
		if (d == -1)
			error(get_static_element_name() + ": invalid day " + el->get_text());
		else
			m_timepat.set_daymask(m_timepat.get_daymask() | (1 << d));
		break;
	}

	case NodeText::tt_adrstarttime:
	{
		int t(el->get_timehhmm());
		if (t == -1)
			error(get_static_element_name() + ": invalid starttime " + el->get_text());
		else
			m_timepat.set_starttime_min(t);
		break;
	}

	case NodeText::tt_adrendtime:
	{
		int t(el->get_timehhmm());
		if (t == -1)
			error(get_static_element_name() + ": invalid endtime " + el->get_text());
		else
			m_timepat.set_endtime_min(t);
		break;
	}

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeADRSpecialDayPattern::elementname("adr:specialDayPattern");

ParseXML::NodeADRSpecialDayPattern::NodeADRSpecialDayPattern(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRSpecialDayPattern::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrspecialday:
		if (el->get_text() == "AFT_HOL")
			m_timepat.set_type(TimePattern::type_afterhol);
		else if (el->get_text() == "BEF_HOL")
			m_timepat.set_type(TimePattern::type_beforehol);
		else if (el->get_text() == "HOL")
			m_timepat.set_type(TimePattern::type_hol);
		else if (el->get_text() == "BUSY_FRI")
			m_timepat.set_type(TimePattern::type_busyfri);
		else
			error(get_static_element_name() + ": unknown special day " + el->get_text());
		break;

	case NodeText::tt_adrstarttime:
	{
		int t(el->get_timehhmm());
		if (t == -1)
			error(get_static_element_name() + ": invalid starttime " + el->get_text());
		else
			m_timepat.set_starttime_min(t);
		break;
	}

	case NodeText::tt_adrendtime:
	{
		int t(el->get_timehhmm());
		if (t == -1)
			error(get_static_element_name() + ": invalid endtime " + el->get_text());
		else
			m_timepat.set_endtime_min(t);
		break;
	}

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeADRTimePatternCombination::elementname("adr:timePatternCombination");

ParseXML::NodeADRTimePatternCombination::NodeADRTimePatternCombination(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRTimePatternCombination::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrtimeoperand:
		if (m_timetable.empty()) {
			error(get_static_element_name() + ": timeOperand before Pattern");
			break;
		}
		if (el->get_text() == "ADD")
			m_timetable.back().set_operator(TimePattern::operator_add);
		else if (el->get_text() == "SUBTRACT")
			m_timetable.back().set_operator(TimePattern::operator_sub);
		else
			error(get_static_element_name() + "/" + el->get_element_name() + ": unknown text " + el->get_text());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeADRTimePatternCombination::on_subelement(const Glib::RefPtr<NodeADRWeekDayPattern>& el)
{
	if (!el)
		return;
	m_timetable.push_back(el->get_timepat());
}

void ParseXML::NodeADRTimePatternCombination::on_subelement(const Glib::RefPtr<NodeADRSpecialDayPattern>& el)
{
	if (!el)
		return;
	m_timetable.push_back(el->get_timepat());
}

TimeTableElement ParseXML::NodeADRTimePatternCombination::get_timetable(void) const
{
	TimeTableElement r(m_timetable);
	for (unsigned int i(1), n(r.size()); i < n; ++i)
		r[i].set_operator(r[i-1].get_operator());
	if (!r.empty())
		r[0].set_operator(TimePattern::operator_set);
	return r;
}

const std::string ParseXML::NodeADRTimeTable1::elementname("adr:TimeTable");

ParseXML::NodeADRTimeTable1::NodeADRTimeTable1(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRTimeTable1::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrtimereference:
		if (el->get_text() != "UTC")
			error(get_static_element_name() + "/" + el->get_element_name() + ": unknown time reference " + el->get_text());
		break;

	case NodeText::tt_adrexclude:
	{
		int b(el->get_yesno());
		if (b == -1) {
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid boolean " + el->get_text());
			break;
		}
		m_timetable.set_exclude(!!b);
		break;
	}

	case NodeText::tt_adrstartdate:
	{
		timetype_t t(el->get_dateyyyymmdd());
		if (t != std::numeric_limits<timetype_t>::max())
			m_timetable.set_start(t);
		else
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid date (yyyy-mm-dd) " + el->get_text());
		break;
	}

	case NodeText::tt_adrenddate:
	{
		timetype_t t(el->get_dateyyyymmdd());
		if (t != std::numeric_limits<timetype_t>::max())
			m_timetable.set_end(t);
		else
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid date (yyyy-mm-dd) " + el->get_text());
		break;
	}

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeADRTimeTable1::on_subelement(const Glib::RefPtr<NodeADRTimePatternCombination>& el)
{
	if (!el)
		return;
	TimeTableElement t(el->get_timetable());
	m_timetable.swap(t);
}

const std::string ParseXML::NodeADRTimeTable::elementname("adr:timeTable");

ParseXML::NodeADRTimeTable::NodeADRTimeTable(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRTimeTable::on_subelement(const Glib::RefPtr<NodeADRTimeTable1>& el)
{
	m_timetable.push_back(el->get_timetable());
}

const std::string ParseXML::NodeADRLevels::elementname("adr:levels");

ParseXML::NodeADRLevels::NodeADRLevels(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRLevels::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlimit:
	case NodeText::tt_aixmlowerlimit:
	case NodeText::tt_aixmupperlimitreference:
	case NodeText::tt_aixmlowerlimitreference:
		el->update(m_altrange);
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeADRLevels::on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el)
{
	if (!el)
		return;
	const TimeTable& tt(el->get_timetable());
	m_timetable.insert(m_timetable.end(), tt.begin(), tt.end());
	m_timetable.simplify();
}

const std::string ParseXML::NodeADRPropertiesWithScheduleExtension::elementname("adr:PropertiesWithScheduleExtension");

ParseXML::NodeADRPropertiesWithScheduleExtension::NodeADRPropertiesWithScheduleExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRPropertiesWithScheduleExtension::on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el)
{
	if (!el)
		return;
	const TimeTable& tt(el->get_timetable());
	m_timetable.insert(m_timetable.end(), tt.begin(), tt.end());
	m_timetable.simplify();
}

const std::string ParseXML::NodeADRDefaultApplicability::elementname("adr:defaultApplicability");

ParseXML::NodeADRDefaultApplicability::NodeADRDefaultApplicability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

TimeTable ParseXML::NodeADRDefaultApplicability::get_timetable(void) const
{
	TimeTable r;
	r.push_back(m_timetable);
	return r;
}

void ParseXML::NodeADRDefaultApplicability::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrtimereference:
		if (el->get_text() != "UTC")
			error(get_static_element_name() + ": unknown time reference " + el->get_text());
		break;

	case NodeText::tt_adrstartdate:
	{
		timetype_t t(el->get_dateyyyymmdd());
		if (t != std::numeric_limits<timetype_t>::max())
			m_timetable.set_start(t);
		else
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid date (yyyy-mm-dd) " + el->get_text());
		break;
	}

	case NodeText::tt_adrenddate:
	{
		timetype_t t(el->get_dateyyyymmdd());
		if (t != std::numeric_limits<timetype_t>::max())
			m_timetable.set_end(t);
		else
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid date (yyyy-mm-dd) " + el->get_text());
		break;
	}

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeADRAirspaceExtension::elementname("adr:AirspaceExtension");

ParseXML::NodeADRAirspaceExtension::NodeADRAirspaceExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties), m_flexibleuse(false), m_level1(false), m_level2(false), m_level3(false) 
{
}

void ParseXML::NodeADRAirspaceExtension::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrflexibleuse:
	{
		int b(el->get_yesno());
		if (b == -1)
			error(get_static_element_name() + ": invalid flexibleuse " + el->get_text());
		else
			m_flexibleuse = !!b;
		break;
	}

	case NodeText::tt_adrlevel1:
	{
		int b(el->get_yesno());
		if (b == -1)
			error(get_static_element_name() + ": invalid level1 " + el->get_text());
		else
			m_level1 = !!b;
		break;
	}

	case NodeText::tt_adrlevel2:
	{
		int b(el->get_yesno());
		if (b == -1)
			error(get_static_element_name() + ": invalid level2 " + el->get_text());
		else
			m_level2 = !!b;
		break;
	}

	case NodeText::tt_adrlevel3:
	{
		int b(el->get_yesno());
		if (b == -1)
			error(get_static_element_name() + ": invalid level3 " + el->get_text());
		else
			m_level3 = !!b;
		break;
	}

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeADRNonDefaultApplicability::elementname("adr:nonDefaultApplicability");

ParseXML::NodeADRNonDefaultApplicability::NodeADRNonDefaultApplicability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRNonDefaultApplicability::on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el)
{
	if (!el)
		return;
	const TimeTable& tt(el->get_timetable());
	m_timetable.insert(m_timetable.end(), tt.begin(), tt.end());
	m_timetable.simplify();
}

const std::string ParseXML::NodeADRRouteAvailabilityExtension::elementname("adr:RouteAvailabilityExtension");

ParseXML::NodeADRRouteAvailabilityExtension::NodeADRRouteAvailabilityExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties), m_cdr(0)
{
}

void ParseXML::NodeADRRouteAvailabilityExtension::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrconditionalroutetype:
		if (el->get_text() == "CDR_1")
			m_cdr = 1;
		else if (el->get_text() == "CDR_2")
			m_cdr = 2;
		else if (el->get_text() == "CDR_3")
			m_cdr = 3;
		else
			error(get_static_element_name() + ": unknown conditional route type " + el->get_element_name());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeADRRouteAvailabilityExtension::on_subelement(const Glib::RefPtr<NodeADRNonDefaultApplicability>& el)
{
	if (el)
		m_timetable = el->get_timetable();
}

void ParseXML::NodeADRRouteAvailabilityExtension::on_subelement(const Glib::RefPtr<NodeADRDefaultApplicability>& el)
{
	if (el)
		m_timetable = el->get_timetable();
}

const std::string ParseXML::NodeADRProcedureAvailabilityExtension::elementname("adr:ProcedureAvailabilityExtension");

ParseXML::NodeADRProcedureAvailabilityExtension::NodeADRProcedureAvailabilityExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRProcedureAvailabilityExtension::on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el)
{
	if (!el)
		return;
	const TimeTable& tt(el->get_timetable());
	m_timetable.insert(m_timetable.end(), tt.begin(), tt.end());
	m_timetable.simplify();
}

const std::string ParseXML::NodeADRRouteSegmentExtension::elementname("adr:RouteSegmentExtension");

ParseXML::NodeADRRouteSegmentExtension::NodeADRRouteSegmentExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRRouteSegmentExtension::on_subelement(const Glib::RefPtr<NodeADRLevels>& el)
{
	if (!el)
		return;
	m_timetable = el->get_timetable();
	m_altrange = el->get_altrange();
}

const std::string ParseXML::NodeADRIntermediateSignificantPoint::elementname("adr:IntermediateSignificantPoint");

ParseXML::NodeADRIntermediateSignificantPoint::NodeADRIntermediateSignificantPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRIntermediateSignificantPoint::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_adrpointchoicefixdesignatedpoint:
		m_uuid = el->get_uuid();
		break;

	case NodeLink::lt_adrpointchoicenavaidsystem:
		m_uuid = el->get_uuid();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeADRIntermediatePoint::elementname("adr:intermediatePoint");

ParseXML::NodeADRIntermediatePoint::NodeADRIntermediatePoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRIntermediatePoint::on_subelement(const Glib::RefPtr<NodeADRIntermediateSignificantPoint>& el)
{
	if (!el)
		return;
	m_uuid = el->get_uuid();
}

const std::string ParseXML::NodeADRRoutePortionExtension::elementname("adr:RoutePortionExtension");

ParseXML::NodeADRRoutePortionExtension::NodeADRRoutePortionExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRRoutePortionExtension::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_adrreferencedprocedure:
		m_refproc = el->get_uuid();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeADRRoutePortionExtension::on_subelement(const Glib::RefPtr<NodeADRIntermediatePoint>& el)
{
	if (!el)
		return;
	m_intermpt.push_back(el->get_uuid());
}

const std::string ParseXML::NodeADRStandardInstrumentDepartureExtension::elementname("adr:StandardInstrumentDepartureExtension");

ParseXML::NodeADRStandardInstrumentDepartureExtension::NodeADRStandardInstrumentDepartureExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRStandardInstrumentDepartureExtension::on_subelement(const Glib::RefPtr<NodeADRConnectingPoint>& el)
{
	if (el)
		m_connpt.insert(el->get_uuid());
}

const std::string ParseXML::NodeADRStandardInstrumentArrivalExtension::elementname("adr:StandardInstrumentArrivalExtension");

ParseXML::NodeADRStandardInstrumentArrivalExtension::NodeADRStandardInstrumentArrivalExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRStandardInstrumentArrivalExtension::on_subelement(const Glib::RefPtr<NodeADRConnectingPoint>& el)
{
	if (el)
		m_connpt.insert(el->get_uuid());
}

void ParseXML::NodeADRStandardInstrumentArrivalExtension::on_subelement(const Glib::RefPtr<NodeADRInitialApproachFix>& el)
{
	if (el)
		m_iaf = el->get_uuid();
}

const std::string ParseXML::NodeAIXMExtension::elementname("aixm:extension");

ParseXML::NodeAIXMExtension::NodeAIXMExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMExtension::on_subelement(const Glib::RefPtr<NodeAIXMExtensionBase>& el)
{
	if (!el)
		return;
	if (m_el)
		error(get_static_element_name() + ": multiple subelements");
	m_el = el;
}

const std::string ParseXML::NodeAIXMAirspaceActivation::elementname("aixm:AirspaceActivation");

ParseXML::NodeAIXMAirspaceActivation::NodeAIXMAirspaceActivation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirspaceActivation::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmstatus:
		if (el->get_text() != "AVBL_FOR_ACTIVATION")
			error(get_static_element_name() + ": invalid status " + el->get_text());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMAirspaceActivation::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRPropertiesWithScheduleExtension::ptr_t elx(el->get<NodeADRPropertiesWithScheduleExtension>());
	if (!elx) {
		error(get_static_element_name() + ": invalid activation type " + el->get_element_name());
		return;
	}
	m_timetable = elx->get_timetable();
}

const std::string ParseXML::NodeAIXMActivation::elementname("aixm:activation");

ParseXML::NodeAIXMActivation::NodeAIXMActivation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMActivation::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceActivation>& el)
{
	if (!el)
		return;
	if (m_el)
		error(get_static_element_name() + ": multiple subelements");
	m_el = el;
}

const std::string ParseXML::NodeAIXMElevatedPoint::elementname("aixm:ElevatedPoint");

ParseXML::NodeAIXMElevatedPoint::NodeAIXMElevatedPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_elev(std::numeric_limits<int32_t>::min())
{
	m_coord.set_invalid();
}

void ParseXML::NodeAIXMElevatedPoint::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	if (el->get_type() != NodeText::tt_gmlpos) {
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		return;
	}
	std::pair<Point,int32_t> c(el->get_coord_3d());
	m_coord = c.first;
	m_elev = c.second;
}

const std::string ParseXML::NodeAIXMPoint::elementname("aixm:Point");

ParseXML::NodeAIXMPoint::NodeAIXMPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
	m_coord.set_invalid();
}

void ParseXML::NodeAIXMPoint::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	if (el->get_type() != NodeText::tt_gmlpos) {
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		return;
	}
	m_coord = el->get_coord_deg();
}

const std::string ParseXML::NodeAIXMARP::elementname("aixm:ARP");

ParseXML::NodeAIXMARP::NodeAIXMARP(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_elev(std::numeric_limits<int32_t>::min())
{
	m_coord.set_invalid();
}

void ParseXML::NodeAIXMARP::on_subelement(const NodeAIXMElevatedPoint::ptr_t& el)
{
	if (!el)
		return;
	m_coord = el->get_coord();
	m_elev = el->get_elev();
}

const std::string ParseXML::NodeAIXMLocation::elementname("aixm:location");

ParseXML::NodeAIXMLocation::NodeAIXMLocation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_elev(std::numeric_limits<int32_t>::min())

{
	m_coord.set_invalid();
}

void ParseXML::NodeAIXMLocation::on_subelement(const NodeAIXMElevatedPoint::ptr_t& el)
{
	if (el) {
		m_coord = el->get_coord();
		m_elev = el->get_elev();
	}
}

void ParseXML::NodeAIXMLocation::on_subelement(const NodeAIXMPoint::ptr_t& el)
{
	if (el)
		m_coord = el->get_coord();
}

const std::string ParseXML::NodeAIXMCity::elementname("aixm:City");

ParseXML::NodeAIXMCity::NodeAIXMCity(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMCity::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	if (el->get_type() != NodeText::tt_aixmname) {
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		return;
	}
	m_city = el->get_text();
}

const std::string ParseXML::NodeAIXMServedCity::elementname("aixm:servedCity");

ParseXML::NodeAIXMServedCity::NodeAIXMServedCity(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMServedCity::on_subelement(const NodeAIXMCity::ptr_t& el)
{
	if (el)
		m_cities.push_back(el->get_city());
}

const std::string ParseXML::NodeGMLValidTime::elementname("gml:validTime");

ParseXML::NodeGMLValidTime::NodeGMLValidTime(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_start(0), m_end(std::numeric_limits<long>::max())
{
}

void ParseXML::NodeGMLValidTime::on_subelement(const NodeGMLTimePeriod::ptr_t& el)
{
	if (el) {
		m_start = el->get_start();
		m_end = el->get_end();
	}
}

const std::string ParseXML::NodeAIXMFeatureLifetime::elementname("aixm:featureLifetime");

ParseXML::NodeAIXMFeatureLifetime::NodeAIXMFeatureLifetime(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_start(0), m_end(std::numeric_limits<long>::max())
{
}

void ParseXML::NodeAIXMFeatureLifetime::on_subelement(const NodeGMLTimePeriod::ptr_t& el)
{
	if (el) {
		m_start = el->get_start();
		m_end = el->get_end();
	}
}

const long ParseXML::NodeGMLPosition::invalid = std::numeric_limits<long>::min();
const long ParseXML::NodeGMLPosition::unknown = std::numeric_limits<long>::min() + 1;

ParseXML::NodeGMLPosition::NodeGMLPosition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_unknown(false)
{
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name != "indeterminatePosition")
			continue;
		m_unknown = i->value == "unknown";
	}
}

void ParseXML::NodeGMLPosition::on_characters(const Glib::ustring& characters)
{
	m_text += characters;
}

long ParseXML::NodeGMLPosition::get_time(void) const
{
	if (m_unknown)
		return unknown;
	Glib::TimeVal tv;
	if (!tv.assign_from_iso8601(m_text + "Z"))
		return invalid;
	return tv.tv_sec;
}

const std::string ParseXML::NodeGMLBeginPosition::elementname("gml:beginPosition");

ParseXML::NodeGMLBeginPosition::NodeGMLBeginPosition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeGMLPosition(parser, tagname, childnum, properties)
{
}

long ParseXML::NodeGMLBeginPosition::get_time(void) const
{
	long t(NodeGMLPosition::get_time());
	if (t == invalid)
		t = 0;
	else if (t < 0 || t == unknown)
		t = 0;
	return t;
}

const std::string ParseXML::NodeGMLEndPosition::elementname("gml:endPosition");

ParseXML::NodeGMLEndPosition::NodeGMLEndPosition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeGMLPosition(parser, tagname, childnum, properties)
{
}

long ParseXML::NodeGMLEndPosition::get_time(void) const
{
	long t(NodeGMLPosition::get_time());
	if (t == invalid)
		t = 0;
	else if (t < 0 || t == unknown)
		t = std::numeric_limits<long>::max();
	return t;
}

const std::string ParseXML::NodeGMLTimePeriod::elementname("gml:TimePeriod");

ParseXML::NodeGMLTimePeriod::NodeGMLTimePeriod(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_start(0), m_end(std::numeric_limits<long>::max())
{
}

void ParseXML::NodeGMLTimePeriod::on_subelement(const NodeGMLBeginPosition::ptr_t& el)
{
	if (el)
		m_start = el->get_time();
}

void ParseXML::NodeGMLTimePeriod::on_subelement(const NodeGMLEndPosition::ptr_t& el)
{
	if (el)
		m_end = el->get_time();
}

const std::string ParseXML::NodeGMLLinearRing::elementname("gml:LinearRing");

ParseXML::NodeGMLLinearRing::NodeGMLLinearRing(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeGMLLinearRing::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_gmlpos:
		m_poly.push_back(el->get_coord_deg());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeGMLLinearRing::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_gmlpointproperty:
		m_pointlink.push_back(AirspaceTimeSlice::Component::PointLink(el->get_uuid(), 0, AirspaceTimeSlice::Component::PointLink::exterior, m_poly.size()));
		m_poly.push_back(Point::invalid);
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeGMLInterior::elementname("gml:interior");

ParseXML::NodeGMLInterior::NodeGMLInterior(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeGMLInterior::on_subelement(const Glib::RefPtr<NodeGMLLinearRing>& el)
{
	if (!el)
		return;
	if (!m_poly.empty())
		error(get_static_element_name() + ": duplicate " + el->get_static_element_name());
	m_poly = el->get_poly();
	m_pointlink = el->get_pointlink();
}

const std::string ParseXML::NodeGMLExterior::elementname("gml:exterior");

ParseXML::NodeGMLExterior::NodeGMLExterior(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeGMLExterior::on_subelement(const Glib::RefPtr<NodeGMLLinearRing>& el)
{
	if (!el)
		return;
	if (!m_poly.empty())
		error(get_static_element_name() + ": duplicate " + el->get_static_element_name());
	m_poly = el->get_poly();
	m_pointlink = el->get_pointlink();
}

const std::string ParseXML::NodeGMLPolygonPatch::elementname("gml:PolygonPatch");

ParseXML::NodeGMLPolygonPatch::NodeGMLPolygonPatch(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeGMLPolygonPatch::on_subelement(const Glib::RefPtr<NodeGMLInterior>& el)
{
	if (!el)
		return;
	unsigned int ring(m_poly.get_nrinterior());
	const AirspaceTimeSlice::Component::pointlink_t& pl(el->get_pointlink());
	for (AirspaceTimeSlice::Component::pointlink_t::const_iterator i(pl.begin()), e(pl.end()); i != e; ++i) {
		m_pointlink.push_back(*i);
		m_pointlink.back().set_ring(ring);
	}
	m_poly.add_interior(el->get_poly());
}

void ParseXML::NodeGMLPolygonPatch::on_subelement(const Glib::RefPtr<NodeGMLExterior>& el)
{
	if (!el)
		return;
	if (!m_poly.get_exterior().empty())
		error(get_static_element_name() + ": duplicate " + el->get_static_element_name());
	for (AirspaceTimeSlice::Component::pointlink_t::iterator i(m_pointlink.begin()), e(m_pointlink.end()); i != e; ) {
		if (!i->is_exterior()) {
			++i;
			continue;
		}
		i = m_pointlink.erase(i);
		e = m_pointlink.end();
	}
	const AirspaceTimeSlice::Component::pointlink_t& pl(el->get_pointlink());
	m_pointlink.insert(m_pointlink.begin(), pl.begin(), pl.end());
	m_poly.set_exterior(el->get_poly());
}

const std::string ParseXML::NodeGMLPatches::elementname("gml:patches");

ParseXML::NodeGMLPatches::NodeGMLPatches(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeGMLPatches::on_subelement(const Glib::RefPtr<NodeGMLPolygonPatch>& el)
{
	if (!el)
		return;
	unsigned int poly(m_poly.size());
	const AirspaceTimeSlice::Component::pointlink_t& pl(el->get_pointlink());
	for (AirspaceTimeSlice::Component::pointlink_t::const_iterator i(pl.begin()), e(pl.end()); i != e; ++i) {
		m_pointlink.push_back(*i);
		m_pointlink.back().set_poly(poly);
	}
	m_poly.push_back(el->get_poly());
}

const std::string ParseXML::NodeAIXMSurface::elementname("aixm:Surface");

ParseXML::NodeAIXMSurface::NodeAIXMSurface(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMSurface::on_subelement(const Glib::RefPtr<NodeGMLPatches>& el)
{
	if (!el)
		return;
	if (!m_poly.empty())
		error(get_static_element_name() + ": duplicate " + el->get_static_element_name());
	m_poly = el->get_poly();
	m_pointlink = el->get_pointlink();
}

const std::string ParseXML::NodeAIXMHorizontalProjection::elementname("aixm:horizontalProjection");

ParseXML::NodeAIXMHorizontalProjection::NodeAIXMHorizontalProjection(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMHorizontalProjection::on_subelement(const Glib::RefPtr<NodeAIXMSurface>& el)
{
	if (!el)
		return;
	if (!m_poly.empty())
		error(get_static_element_name() + ": duplicate " + el->get_static_element_name());
	m_poly = el->get_poly();
	m_pointlink = el->get_pointlink();
}

const std::string ParseXML::NodeAIXMAirspaceVolumeDependency::elementname("aixm:AirspaceVolumeDependency");

ParseXML::NodeAIXMAirspaceVolumeDependency::NodeAIXMAirspaceVolumeDependency(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirspaceVolumeDependency::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdependency:
		if (el->get_text() == "FULL_GEOMETRY")
			m_comp.set_full_geometry(true);
		else if (el->get_text() == "HORZ_PROJECTION")
			m_comp.set_full_geometry(false);
		else
			error(get_static_element_name() + ": invalid dependency " + el->get_text());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMAirspaceVolumeDependency::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmtheairspace:
		m_comp.set_airspace(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMContributorAirspace::elementname("aixm:contributorAirspace");

ParseXML::NodeAIXMContributorAirspace::NodeAIXMContributorAirspace(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMContributorAirspace::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolumeDependency>& el)
{
	if (!el)
		return;
	m_comp = el->get_comp();
}

const std::string ParseXML::NodeAIXMAirspaceVolume::elementname("aixm:AirspaceVolume");

ParseXML::NodeAIXMAirspaceVolume::NodeAIXMAirspaceVolume(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirspaceVolume::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlimit:
	case NodeText::tt_aixmlowerlimit:
	case NodeText::tt_aixmupperlimitreference:
	case NodeText::tt_aixmlowerlimitreference:
		el->update(m_comp.get_altrange());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMAirspaceVolume::on_subelement(const Glib::RefPtr<NodeAIXMHorizontalProjection>& el)
{
	if (!el)
		return;
	if (!m_comp.get_poly().empty())
		error(get_static_element_name() + ": duplicate " + el->get_static_element_name());
	m_comp.set_poly(el->get_poly());
	m_comp.set_pointlink(el->get_pointlink());
}

void ParseXML::NodeAIXMAirspaceVolume::on_subelement(const Glib::RefPtr<NodeAIXMContributorAirspace>& el)
{
	if (!el)
		return;
	const AirspaceTimeSlice::Component& comp(el->get_comp());
	m_comp.set_airspace(comp.get_airspace());
	m_comp.set_full_geometry(comp.is_full_geometry());
}

const std::string ParseXML::NodeAIXMTheAirspaceVolume::elementname("aixm:theAirspaceVolume");

ParseXML::NodeAIXMTheAirspaceVolume::NodeAIXMTheAirspaceVolume(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMTheAirspaceVolume::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceVolume>& el)
{
	if (!el)
		return;
	m_comp = el->get_comp();
}

const std::string ParseXML::NodeAIXMAirspaceGeometryComponent::elementname("aixm:AirspaceGeometryComponent");

ParseXML::NodeAIXMAirspaceGeometryComponent::NodeAIXMAirspaceGeometryComponent(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirspaceGeometryComponent::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmoperation:
		if (el->get_text() == "BASE")
			m_comp.set_operator(AirspaceTimeSlice::Component::operator_base);
		else if (el->get_text() == "UNION")
			m_comp.set_operator(AirspaceTimeSlice::Component::operator_union);
		else
			error(get_static_element_name() + ": invalid operation " + el->get_text());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMAirspaceGeometryComponent::on_subelement(const Glib::RefPtr<NodeAIXMTheAirspaceVolume>& el)
{
	if (!el)
		return;
	const AirspaceTimeSlice::Component& comp(el->get_comp());
	m_comp.set_airspace(comp.get_airspace());
	m_comp.set_full_geometry(comp.is_full_geometry());
	m_comp.set_altrange(comp.get_altrange());
	m_comp.set_poly(comp.get_poly());
	m_comp.set_pointlink(comp.get_pointlink());
	m_comp.set_gndelevmin(comp.get_gndelevmin());
	m_comp.set_gndelevmax(comp.get_gndelevmax());
}

const std::string ParseXML::NodeAIXMGeometryComponent::elementname("aixm:geometryComponent");

ParseXML::NodeAIXMGeometryComponent::NodeAIXMGeometryComponent(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMGeometryComponent::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceGeometryComponent>& el)
{
	if (!el)
		return;
	m_comp = el->get_comp();
}

const std::string ParseXML::NodeAIXMEnRouteSegmentPoint::elementname("aixm:EnRouteSegmentPoint");

ParseXML::NodeAIXMEnRouteSegmentPoint::NodeAIXMEnRouteSegmentPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMEnRouteSegmentPoint::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmpointchoicenavaidsystem:
		m_uuid = el->get_uuid();
		break;

	case NodeLink::lt_aixmpointchoicefixdesignatedpoint:
		m_uuid = el->get_uuid();
		break;

	case NodeLink::lt_aixmpointchoiceairportreferencepoint:
		m_uuid = el->get_uuid();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMTerminalSegmentPoint::elementname("aixm:TerminalSegmentPoint");

ParseXML::NodeAIXMTerminalSegmentPoint::NodeAIXMTerminalSegmentPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMTerminalSegmentPoint::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmpointchoicenavaidsystem:
		m_uuid = el->get_uuid();
		break;

	case NodeLink::lt_aixmpointchoicefixdesignatedpoint:
		m_uuid = el->get_uuid();
		break;

	case NodeLink::lt_aixmpointchoiceairportreferencepoint:
		m_uuid = el->get_uuid();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMStart::elementname("aixm:start");

ParseXML::NodeAIXMStart::NodeAIXMStart(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStart::on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el)
{
	if (el)
		m_uuid = el->get_uuid();
}

const std::string ParseXML::NodeAIXMEnd::elementname("aixm:end");

ParseXML::NodeAIXMEnd::NodeAIXMEnd(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMEnd::on_subelement(const Glib::RefPtr<NodeAIXMEnRouteSegmentPoint>& el)
{
	if (el)
		m_uuid = el->get_uuid();
}

const std::string ParseXML::NodeAIXMStartPoint::elementname("aixm:startPoint");

ParseXML::NodeAIXMStartPoint::NodeAIXMStartPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStartPoint::on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el)
{
	if (el)
		m_uuid = el->get_uuid();
}

const std::string ParseXML::NodeAIXMEndPoint::elementname("aixm:endPoint");

ParseXML::NodeAIXMEndPoint::NodeAIXMEndPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMEndPoint::on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el)
{
	if (el)
		m_uuid = el->get_uuid();
}

const std::string ParseXML::NodeADRConnectingPoint::elementname("adr:connectingPoint");

ParseXML::NodeADRConnectingPoint::NodeADRConnectingPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRConnectingPoint::on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el)
{
	if (el)
		m_uuid = el->get_uuid();
}

const std::string ParseXML::NodeADRInitialApproachFix::elementname("adr:initialApproachFix");

ParseXML::NodeADRInitialApproachFix::NodeADRInitialApproachFix(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRInitialApproachFix::on_subelement(const Glib::RefPtr<NodeAIXMTerminalSegmentPoint>& el)
{
	if (el)
		m_uuid = el->get_uuid();
}

const std::string ParseXML::NodeAIXMAirspaceLayer::elementname("aixm:AirspaceLayer");

ParseXML::NodeAIXMAirspaceLayer::NodeAIXMAirspaceLayer(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirspaceLayer::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlimit:
	case NodeText::tt_aixmlowerlimit:
	case NodeText::tt_aixmupperlimitreference:
	case NodeText::tt_aixmlowerlimitreference:
		el->update(m_availability.get_altrange());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMAirspaceLayer::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmdiscretelevelseries:
		m_availability.set_levels(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMLevels::elementname("aixm:levels");

ParseXML::NodeAIXMLevels::NodeAIXMLevels(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMLevels::on_subelement(const Glib::RefPtr<NodeAIXMAirspaceLayer>& el)
{
	if (el)
		m_availability = el->get_availability();
}

const std::string ParseXML::NodeAIXMRouteAvailability::elementname("aixm:RouteAvailability");

ParseXML::NodeAIXMRouteAvailability::NodeAIXMRouteAvailability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAvailabilityBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRouteAvailability::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmstatus:
		if (el->get_text() == "CLSD")
			m_availability.set_status(RouteSegmentTimeSlice::Availability::status_closed);
		else if (el->get_text() == "OPEN")
			m_availability.set_status(RouteSegmentTimeSlice::Availability::status_open);
		else if (el->get_text() == "COND")
			m_availability.set_status(RouteSegmentTimeSlice::Availability::status_conditional);
		else
			error(get_static_element_name() + ": unknown status " + el->get_text());
		break;

	case NodeText::tt_aixmdirection:
		if (el->get_text() == "FORWARD")
			m_availability.set_forward();
		else if (el->get_text() == "BACKWARD")
			m_availability.set_backward();
		else
			error(get_static_element_name() + ": unknown direction " + el->get_text());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMRouteAvailability::on_subelement(const Glib::RefPtr<NodeAIXMLevels>& el)
{
	const RouteSegmentTimeSlice::Availability& a(el->get_availability());
	m_availability.set_altrange(a.get_altrange());
	m_availability.set_levels(a.get_levels());
}

void ParseXML::NodeAIXMRouteAvailability::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRRouteAvailabilityExtension::ptr_t elx(el->get<NodeADRRouteAvailabilityExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	m_availability.set_cdr(elx->get_cdr());
	m_availability.set_timetable(elx->get_timetable());
}

const std::string ParseXML::NodeAIXMProcedureAvailability::elementname("aixm:ProcedureAvailability");

ParseXML::NodeAIXMProcedureAvailability::NodeAIXMProcedureAvailability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAvailabilityBase(parser, tagname, childnum, properties), m_status(StandardInstrumentTimeSlice::status_invalid)
{
}

void ParseXML::NodeAIXMProcedureAvailability::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmstatus:
		if (el->get_text() == "USABLE")
			m_status = StandardInstrumentTimeSlice::status_usable;
		else if (el->get_text() == "OTHER:__ADR__ATC_DISCRETION")
			m_status = StandardInstrumentTimeSlice::status_atcdiscr;
		else
			error(get_static_element_name() + ": unknown status " + el->get_text());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMProcedureAvailability::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRProcedureAvailabilityExtension::ptr_t elx(el->get<NodeADRProcedureAvailabilityExtension>());
	if (!elx) {
		error(get_static_element_name() + ": invalid availability type " + el->get_element_name());
		return;
	}
	m_timetable = elx->get_timetable();
}

const std::string ParseXML::NodeAIXMAvailability::elementname("aixm:availability");

ParseXML::NodeAIXMAvailability::NodeAIXMAvailability(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAvailability::on_subelement(const Glib::RefPtr<NodeAIXMAvailabilityBase>& el)
{
	if (!el)
		return;
	if (m_el)
		error(get_static_element_name() + ": multiple subelements");
	m_el = el;
}

const std::string ParseXML::NodeAIXMClientRoute::elementname("aixm:clientRoute");

ParseXML::NodeAIXMClientRoute::NodeAIXMClientRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMClientRoute::on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el)
{
	if (!el)
		return;
	m_route = el->get_route();
	m_start = el->get_start();
	m_end = el->get_end();
}

const std::string ParseXML::NodeAIXMRoutePortion::elementname("aixm:RoutePortion");

ParseXML::NodeAIXMRoutePortion::NodeAIXMRoutePortion(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRoutePortion::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmreferencedroute:
		m_route = el->get_uuid();
		break;

	case NodeLink::lt_aixmstartnavaidsystem:
	case NodeLink::lt_aixmstartfixdesignatedpoint:
	case NodeLink::lt_aixmstartairportreferencepoint:
		m_start = el->get_uuid();
		break;

	case NodeLink::lt_aixmendnavaidsystem:
	case NodeLink::lt_aixmendfixdesignatedpoint:
	case NodeLink::lt_aixmendairportreferencepoint:
		m_end = el->get_uuid();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMRoutePortion::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRRoutePortionExtension::ptr_t elx(el->get<NodeADRRoutePortionExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	if (!elx->get_refproc().is_nil()) {
		if (!m_route.is_nil()) {
			std::ostringstream oss;
			oss << get_static_element_name() + ": referencedRoute " << m_route
			    << " and adr referencedProcedure " << elx->get_refproc() << " both set";
			error(oss.str());
		}
		m_route = elx->get_refproc();
	}
}

const std::string ParseXML::NodeAIXMDirectFlightClass::elementname("aixm:DirectFlightClass");

ParseXML::NodeAIXMDirectFlightClass::NodeAIXMDirectFlightClass(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_length(std::numeric_limits<double>::quiet_NaN())
{
}

void ParseXML::NodeAIXMDirectFlightClass::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmexceedlength:
		m_length = el->get_double();
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMFlightConditionCircumstance::elementname("aixm:FlightConditionCircumstance");

ParseXML::NodeAIXMFlightConditionCircumstance::NodeAIXMFlightConditionCircumstance(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_refloc(-1), m_locrel(locrel_invalid)
{
}

void ParseXML::NodeAIXMFlightConditionCircumstance::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmreferencelocation:
	{
		int b(el->get_yesno());
		if (b == -1) {
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid boolean " + el->get_text());
			break;
		}
		m_refloc = !!b;
		break;
	}

	case NodeText::tt_aixmrelationwithlocation:
		if (el->get_text() == "DEP")
			m_locrel = locrel_dep;
		else if (el->get_text() == "ARR")
			m_locrel = locrel_arr;
		else if (el->get_text() == "ACT")
			m_locrel = locrel_act;
		else if (el->get_text() == "AVBL")
			m_locrel = locrel_avbl;
		else if (el->get_text() == "XNG")
			m_locrel = locrel_xng;
		else
			error(get_static_element_name() + ": unknown relationWithLocation " + el->get_element_name());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string& ParseXML::NodeAIXMFlightConditionCircumstance::get_locrel_string(locrel_t lr)
{
	switch (lr) {
	case locrel_dep:
	{
		static const std::string r("DEP");
		return r;
	}

	case locrel_arr:
	{
		static const std::string r("ARR");
		return r;
	}

	case locrel_act:
	{
		static const std::string r("ACT");
		return r;
	}

	case locrel_avbl:
	{
		static const std::string r("AVBL");
		return r;
	}

	case locrel_xng:
	{
		static const std::string r("XNG");
		return r;
	}

	case locrel_invalid:
	default:
	{
		static const std::string r("?");
		return r;
	}
	}
}

const std::string ParseXML::NodeAIXMOperationalCondition::elementname("aixm:operationalCondition");

ParseXML::NodeAIXMOperationalCondition::NodeAIXMOperationalCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_refloc(false), m_locrel(NodeAIXMFlightConditionCircumstance::locrel_invalid)
{
}

void ParseXML::NodeAIXMOperationalCondition::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCircumstance>& el)
{
	if (!el)
		return;
	m_refloc = el->is_refloc();
	m_locrel = el->get_locrel();
}

const std::string ParseXML::NodeAIXMFlightRestrictionLevel::elementname("aixm:FlightRestrictionLevel");

ParseXML::NodeAIXMFlightRestrictionLevel::NodeAIXMFlightRestrictionLevel(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlightRestrictionLevel::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlevel:
	case NodeText::tt_aixmlowerlevel:
	case NodeText::tt_aixmupperlevelreference:
	case NodeText::tt_aixmlowerlevelreference:
		el->update(m_altrange);
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMFlightLevel::elementname("aixm:flightLevel");

ParseXML::NodeAIXMFlightLevel::NodeAIXMFlightLevel(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlightLevel::on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionLevel>& el)
{
	if (!el)
		return;
	m_altrange = el->get_altrange();
}

const std::string ParseXML::NodeADRFlightConditionCombinationExtension::elementname("adr:FlightConditionCombinationExtension");

ParseXML::NodeADRFlightConditionCombinationExtension::NodeADRFlightConditionCombinationExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRFlightConditionCombinationExtension::on_subelement(const Glib::RefPtr<NodeADRTimeTable>& el)
{
	if (!el)
		return;
	const TimeTable& tt(el->get_timetable());
	m_timetable.insert(m_timetable.end(), tt.begin(), tt.end());
	m_timetable.simplify();
}

const std::string ParseXML::NodeAIXMDirectFlightSegment::elementname("aixm:DirectFlightSegment");

ParseXML::NodeAIXMDirectFlightSegment::NodeAIXMDirectFlightSegment(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMDirectFlightSegment::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmstartnavaidsystem:
	case NodeLink::lt_aixmstartfixdesignatedpoint:
	case NodeLink::lt_aixmstartairportreferencepoint:
		m_start = el->get_uuid();
		break;

	case NodeLink::lt_aixmendnavaidsystem:
	case NodeLink::lt_aixmendfixdesignatedpoint:
	case NodeLink::lt_aixmendairportreferencepoint:
		m_end = el->get_uuid();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMElementRoutePortionElement::elementname("aixm:element_routePortionElement");

ParseXML::NodeAIXMElementRoutePortionElement::NodeAIXMElementRoutePortionElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMElementRoutePortionElement::on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el)
{
	if (!el)
		return;
	m_route = el->get_route();
	m_start = el->get_start();
	m_end = el->get_end();
}

const std::string ParseXML::NodeAIXMElementDirectFlightElement::elementname("aixm:element_directFlightElement");

ParseXML::NodeAIXMElementDirectFlightElement::NodeAIXMElementDirectFlightElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMElementDirectFlightElement::on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightSegment>& el)
{
	if (!el)
		return;
	m_start = el->get_start();
	m_end = el->get_end();
}

const std::string ParseXML::NodeAIXMFlightRoutingElement::elementname("aixm:FlightRoutingElement");

ParseXML::NodeAIXMFlightRoutingElement::NodeAIXMFlightRoutingElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_ordernumber(0), m_type(RestrictionElement::type_invalid), m_star(false)
{
}

void ParseXML::NodeAIXMFlightRoutingElement::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmordernumber:
		m_ordernumber = el->get_ulong();
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMFlightRoutingElement::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmpointelementnavaidsystem:
		if (m_type != RestrictionElement::type_invalid)
			error(get_static_element_name() + ": duplicate restriction element " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = RestrictionElement::type_point;
		break;

	case NodeLink::lt_aixmpointelementfixdesignatedpoint:
		if (m_type != RestrictionElement::type_invalid)
			error(get_static_element_name() + ": duplicate restriction element " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = RestrictionElement::type_point;
		break;

	case NodeLink::lt_aixmelementairspaceelement:
		if (m_type != RestrictionElement::type_invalid)
			error(get_static_element_name() + ": duplicate restriction element " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = RestrictionElement::type_airspace;
		break;

	case NodeLink::lt_aixmelementstandardinstrumentarrivalelement:
		if (m_type != RestrictionElement::type_invalid)
			error(get_static_element_name() + ": duplicate restriction element " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = RestrictionElement::type_sidstar;
		m_star = true;
		break;

	case NodeLink::lt_aixmelementstandardinstrumentdepartureelement:
		if (m_type != RestrictionElement::type_invalid)
			error(get_static_element_name() + ": duplicate restriction element " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = RestrictionElement::type_sidstar;
		m_star = false;
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMFlightRoutingElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightLevel>& el)
{
	if (!el)
		return;
	m_altrange = el->get_altrange();
}

void ParseXML::NodeAIXMFlightRoutingElement::on_subelement(const Glib::RefPtr<NodeAIXMElementRoutePortionElement>& el)
{
	if (!el)
		return;
	if (m_type != RestrictionElement::type_invalid)
		error(get_static_element_name() + ": duplicate restriction element " + el->get_element_name());
	m_uuid = el->get_route();
	m_start = el->get_start();
	m_end = el->get_end();
	m_type = RestrictionElement::type_route;
	if (m_uuid.is_nil())
		error(get_static_element_name() + ": RoutePortionElement child but route is nil");
}

void ParseXML::NodeAIXMFlightRoutingElement::on_subelement(const Glib::RefPtr<NodeAIXMElementDirectFlightElement>& el)
{
	if (!el)
		return;
	if (m_type != RestrictionElement::type_invalid)
		error(get_static_element_name() + ": duplicate restriction element " + el->get_element_name());
	m_uuid = UUID();
	m_start = el->get_start();
	m_end = el->get_end();
	m_type = RestrictionElement::type_route;
}

RestrictionElement::ptr_t ParseXML::NodeAIXMFlightRoutingElement::get_elem(void) const
{
	switch (m_type) {
	case RestrictionElement::type_point:
		return RestrictionElement::ptr_t(new RestrictionElementPoint(m_altrange, m_uuid));

	case RestrictionElement::type_airspace:
		return RestrictionElement::ptr_t(new RestrictionElementAirspace(m_altrange, m_uuid));

	case RestrictionElement::type_sidstar:
		return RestrictionElement::ptr_t(new RestrictionElementSidStar(m_altrange, m_uuid, m_star));

	case RestrictionElement::type_route:
		return RestrictionElement::ptr_t(new RestrictionElementRoute(m_altrange, m_start, m_end, m_uuid));

	default:
		error(get_static_element_name() + ": invalid routing element");
		return RestrictionElement::ptr_t();
	}
}

const std::string ParseXML::NodeAIXMRouteElement::elementname("aixm:routeElement");

ParseXML::NodeAIXMRouteElement::NodeAIXMRouteElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRouteElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightRoutingElement>& el)
{
	if (!el)
		return;
	m_elem = el->get_elem();
	m_ordernumber = el->get_ordernumber();
}

const std::string ParseXML::NodeAIXMFlightRestrictionRoute::elementname("aixm:FlightRestrictionRoute");

ParseXML::NodeAIXMFlightRestrictionRoute::NodeAIXMFlightRestrictionRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlightRestrictionRoute::on_subelement(const Glib::RefPtr<NodeAIXMRouteElement>& el)
{
	if (!el)
		return;
	m_elems.push_back(elem_t(el->get_elem(), el->get_ordernumber()));
	if (!m_elems.back().first)
		error(get_static_element_name() + ": null restriction element " + el->get_element_name());
}

RestrictionSequence ParseXML::NodeAIXMFlightRestrictionRoute::get_seq(void) const
{
	RestrictionSequence s;
	for (elems_t::const_iterator i(m_elems.begin()), e(m_elems.end()); i != e; ) {
		s.add(i->first);
		elems_t::const_iterator i2(i);
		++i;
		if (i == e)
			break;
		if (i2->second >= i->second)
			error(get_static_element_name() + ": restriction sequence bad order of elements");
	}
	return s;
}

const std::string ParseXML::NodeAIXMRegulatedRoute::elementname("aixm:regulatedRoute");

ParseXML::NodeAIXMRegulatedRoute::NodeAIXMRegulatedRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRegulatedRoute::on_subelement(const Glib::RefPtr<NodeAIXMFlightRestrictionRoute>& el)
{
	if (!el)
		return;
	m_res.add(el->get_seq());
}

const std::string ParseXML::NodeAIXMAircraftCharacteristic::elementname("aixm:AircraftCharacteristic");

ParseXML::NodeAIXMAircraftCharacteristic::NodeAIXMAircraftCharacteristic(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_engines(0), m_acfttype(ConditionAircraft::acfttype_invalid),
	  m_engine(ConditionAircraft::engine_invalid), m_navspec(ConditionAircraft::navspec_invalid), m_vertsep(ConditionAircraft::vertsep_invalid)
{
}

void ParseXML::NodeAIXMAircraftCharacteristic::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmtype:
		if (el->get_text() == "LANDPLANE")
			m_acfttype = ConditionAircraft::acfttype_landplane;
		else if (el->get_text() == "SEAPLANE")
			m_acfttype = ConditionAircraft::acfttype_seaplane;
		else if (el->get_text() == "AMPHIBIAN")
			m_acfttype = ConditionAircraft::acfttype_amphibian;
		else if (el->get_text() == "HELICOPTER")
			m_acfttype = ConditionAircraft::acfttype_helicopter;
		else if (el->get_text() == "GYROCOPTER")
			m_acfttype = ConditionAircraft::acfttype_gyrocopter;
		else if (el->get_text() == "TILTWING")
			m_acfttype = ConditionAircraft::acfttype_tiltwing;
		else
			error(get_static_element_name() + ": unknown type " + el->get_element_name());
		break;

	case NodeText::tt_aixmengine:
		if (el->get_text() == "PISTON")
			m_engine = ConditionAircraft::engine_piston;
		else if (el->get_text() == "TURBOPROP")
			m_engine = ConditionAircraft::engine_turboprop;
		else if (el->get_text() == "JET")
			m_engine = ConditionAircraft::engine_jet;
		else
			error(get_static_element_name() + ": unknown engine " + el->get_element_name());
		break;

	case NodeText::tt_aixmtypeaircrafticao:
		m_icaotype = el->get_text();
		break;

	case NodeText::tt_aixmnavigationspecification:
		if (el->get_text() == "RNAV_1")
			m_navspec = ConditionAircraft::navspec_rnav1;
		else
			error(get_static_element_name() + ": unknown navigationSpecification " + el->get_element_name());
		break;

	case NodeText::tt_aixmnumberengine:
		m_engines = el->get_ulong();
		break;

	case NodeText::tt_aixmverticalseparationcapability:
		if (el->get_text() == "RVSM")
			m_vertsep = ConditionAircraft::vertsep_rvsm;
		else
			error(get_static_element_name() + ": unknown verticalSeparationCapability " + el->get_element_name());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMFlightConditionAircraft::elementname("aixm:flightCondition_aircraft");

ParseXML::NodeAIXMFlightConditionAircraft::NodeAIXMFlightConditionAircraft(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_engines(0), m_acfttype(ConditionAircraft::acfttype_invalid),
	  m_engine(ConditionAircraft::engine_invalid), m_navspec(ConditionAircraft::navspec_invalid), m_vertsep(ConditionAircraft::vertsep_invalid)
{
}

void ParseXML::NodeAIXMFlightConditionAircraft::on_subelement(const Glib::RefPtr<NodeAIXMAircraftCharacteristic>& el)
{
	if (!el)
		return;
	m_icaotype = el->get_icaotype();
	m_engines = el->get_engines();
	m_acfttype = el->get_acfttype();
	m_engine = el->get_engine();
	m_navspec = el->get_navspec();
	m_vertsep = el->get_vertsep();
}

const std::string ParseXML::NodeAIXMFlightCharacteristic::elementname("aixm:FlightCharacteristic");

ParseXML::NodeAIXMFlightCharacteristic::NodeAIXMFlightCharacteristic(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_civmil(ConditionFlight::civmil_invalid), m_purpose(ConditionFlight::purpose_invalid)
{
}

void ParseXML::NodeAIXMFlightCharacteristic::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmmilitary:
		if (el->get_text() == "CIV")
			m_civmil = ConditionFlight::civmil_civ;
		else if (el->get_text() == "MIL")
			m_civmil = ConditionFlight::civmil_mil;
		else
			error(get_static_element_name() + ": unknown military " + el->get_element_name());
		break;

	case NodeText::tt_aixmpurpose:
		if (el->get_text() == "ALL")
			m_purpose = ConditionFlight::purpose_all;
		else if (el->get_text() == "SCHEDULED")
			m_purpose = ConditionFlight::purpose_scheduled;
		else if (el->get_text() == "NON_SCHEDULED")
			m_purpose = ConditionFlight::purpose_nonscheduled;
		else if (el->get_text() == "PRIVATE")
			m_purpose = ConditionFlight::purpose_private;
		else if (el->get_text() == "PARTICIPANT")
			m_purpose = ConditionFlight::purpose_participant;
		else if (el->get_text() == "AIR_TRAINING")
			m_purpose = ConditionFlight::purpose_airtraining;
		else if (el->get_text() == "AIR_WORK")
			m_purpose = ConditionFlight::purpose_airwork;
		else
			error(get_static_element_name() + ": unknown purpose " + el->get_element_name());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMFlightConditionFlight::elementname("aixm:flightCondition_flight");

ParseXML::NodeAIXMFlightConditionFlight::NodeAIXMFlightConditionFlight(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_civmil(ConditionFlight::civmil_invalid), m_purpose(ConditionFlight::purpose_invalid)
{
}

void ParseXML::NodeAIXMFlightConditionFlight::on_subelement(const Glib::RefPtr<NodeAIXMFlightCharacteristic>& el)
{
	if (!el)
		return;
	m_civmil = el->get_civmil();
	m_purpose = el->get_purpose();
}

const std::string ParseXML::NodeAIXMFlightConditionOperand::elementname("aixm:flightCondition_operand");

ParseXML::NodeAIXMFlightConditionOperand::NodeAIXMFlightConditionOperand(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlightConditionOperand::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCombination>& el)
{
	if (!el)
		return;
	if (m_cond)
		error(get_static_element_name() + ": duplicate " + el->get_element_name());
	m_cond = el->get_cond();
	if (!el->get_timetable().empty())
		error(get_static_element_name() + ": nonfinal condition combination has timetable " + el->get_element_name());
}

const std::string ParseXML::NodeAIXMFlightConditionRoutePortionCondition::elementname("aixm:flightCondition_routePortionCondition");

ParseXML::NodeAIXMFlightConditionRoutePortionCondition::NodeAIXMFlightConditionRoutePortionCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlightConditionRoutePortionCondition::on_subelement(const Glib::RefPtr<NodeAIXMRoutePortion>& el)
{
	if (!el)
		return;
	m_route = el->get_route();
	m_start = el->get_start();
	m_end = el->get_end();
}

const std::string ParseXML::NodeAIXMFlightConditionDirectFlightCondition::elementname("aixm:flightCondition_directFlightCondition");

ParseXML::NodeAIXMFlightConditionDirectFlightCondition::NodeAIXMFlightConditionDirectFlightCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_length(std::numeric_limits<double>::quiet_NaN())
{
}

void ParseXML::NodeAIXMFlightConditionDirectFlightCondition::on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightClass>& el)
{
	if (!el)
		return;
	m_length = el->get_length();
}

void ParseXML::NodeAIXMFlightConditionDirectFlightCondition::on_subelement(const Glib::RefPtr<NodeAIXMDirectFlightSegment>& el)
{
	if (!el)
		return;
	m_start = el->get_start();
	m_end = el->get_end();
}

const std::string ParseXML::NodeADRAirspaceBorderCrossingObject::elementname("adr:AirspaceBorderCrossingObject");

ParseXML::NodeADRAirspaceBorderCrossingObject::NodeADRAirspaceBorderCrossingObject(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRAirspaceBorderCrossingObject::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_adrenteredairspace:
		m_end = el->get_uuid();
		break;

	case NodeLink::lt_adrexitedairspace:
		m_start = el->get_uuid();
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeADRBorderCrossingCondition::elementname("adr:borderCrossingCondition");

ParseXML::NodeADRBorderCrossingCondition::NodeADRBorderCrossingCondition(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRBorderCrossingCondition::on_subelement(const Glib::RefPtr<NodeADRAirspaceBorderCrossingObject>& el)
{
	if (!el)
		return;
	m_start = el->get_start();
	m_end = el->get_end();
}

const std::string ParseXML::NodeADRFlightConditionElementExtension::elementname("adr:FlightConditionElementExtension");

ParseXML::NodeADRFlightConditionElementExtension::NodeADRFlightConditionElementExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRFlightConditionElementExtension::on_subelement(const Glib::RefPtr<NodeADRBorderCrossingCondition>& el)
{
	if (!el)
		return;
	m_start = el->get_start();
	m_end = el->get_end();
}

const std::string ParseXML::NodeAIXMFlightConditionElement::elementname("aixm:FlightConditionElement");

ParseXML::NodeAIXMFlightConditionElement::NodeAIXMFlightConditionElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMElementBase(parser, tagname, childnum, properties), m_length(std::numeric_limits<double>::quiet_NaN()), m_index(0), m_refloc(false),
	  m_locrel(NodeAIXMFlightConditionCircumstance::locrel_invalid), m_type(Condition::type_invalid), m_arr(false), m_engines(0),
	  m_acfttype(ConditionAircraft::acfttype_invalid), m_engine(ConditionAircraft::engine_invalid), m_navspec(ConditionAircraft::navspec_invalid),
	  m_vertsep(ConditionAircraft::vertsep_invalid), m_civmil(ConditionFlight::civmil_invalid), m_purpose(ConditionFlight::purpose_invalid)
{
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmindex:
		m_index = el->get_ulong();
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmflightconditionairspacecondition:
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = Condition::type_crossingairspace1;
		break;

	case NodeLink::lt_aixmsignificantpointconditionnavaidsystem:
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = Condition::type_crossingpoint;
		break;

	case NodeLink::lt_aixmsignificantpointconditionfixdesignatedpoint:
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = Condition::type_crossingpoint;
		break;

	case NodeLink::lt_aixmflightconditionairportheliportcondition:
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = Condition::type_crossingpoint;
		break;

	case NodeLink::lt_aixmflightconditionstandardinstrumentarrivalcondition:
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = Condition::type_crossingsidstar;
		m_arr = true;
		break;

	case NodeLink::lt_aixmflightconditionstandardinstrumentdeparturecondition:
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_uuid = el->get_uuid();
		m_type = Condition::type_crossingsidstar;
		m_arr = false;
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionOperand>& el)
{
	if (!el)
		return;
	if (m_type != Condition::type_invalid)
		error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
	m_cond = el->get_cond();
	m_type = Condition::type_and;
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionDirectFlightCondition>& el)
{
	if (!el)
		return;
	if (!std::isnan(el->get_length())) {
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_length = el->get_length();
		m_type = Condition::type_dctlimit;
	}
	if (!el->get_start().is_nil() && !el->get_end().is_nil()) {
		if (m_type != Condition::type_invalid)
			error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
		m_start = el->get_start();
		m_end = el->get_end();
		m_type = Condition::type_crossingdct;
	}
	if (m_type == Condition::type_invalid)
		error(get_static_element_name() + ": empty " + el->get_element_name());
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionAircraft>& el)
{
	if (!el)
		return;
	if (m_type != Condition::type_invalid)
		error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
	m_icaotype = el->get_icaotype();
	m_engines = el->get_engines();
	m_acfttype = el->get_acfttype();
	m_engine = el->get_engine();
	m_navspec = el->get_navspec();
	m_vertsep = el->get_vertsep();
	m_type = Condition::type_aircraft;
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionRoutePortionCondition>& el)
{
	if (!el)
		return;
	if (m_type != Condition::type_invalid)
		error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
	m_uuid = el->get_route();
	m_start = el->get_start();
	m_end = el->get_end();
	m_type = Condition::type_crossingairway;
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionFlight>& el)
{
	if (!el)
		return;
	if (m_type != Condition::type_invalid)
		error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
	m_civmil = el->get_civmil();
	m_purpose = el->get_purpose();
	m_type = Condition::type_flight;
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMOperationalCondition>& el)
{
	if (!el)
		return;
	m_refloc = el->is_refloc();
	m_locrel = el->get_locrel();
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMFlightLevel>& el)
{
	if (!el)
		return;
	m_altrange = el->get_altrange();
}

void ParseXML::NodeAIXMFlightConditionElement::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRFlightConditionElementExtension::ptr_t elx(el->get<NodeADRFlightConditionElementExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	if (m_type != Condition::type_invalid)
		error(get_static_element_name() + ": duplicate condition " + el->get_element_name());
	m_start = elx->get_start();
	m_end = elx->get_end();
	m_type = Condition::type_crossingairspace2;
}

const Condition::ptr_t& ParseXML::NodeAIXMFlightConditionElement::get_cond(void)
{
	switch (m_type) {
	case Condition::type_crossingairspace1:
		switch (m_locrel) {
		case NodeAIXMFlightConditionCircumstance::locrel_xng:
			m_cond = Condition::ptr_t(new ConditionCrossingAirspace1(get_childnum(), m_altrange, m_uuid, m_refloc));
			break;

		case NodeAIXMFlightConditionCircumstance::locrel_dep:
			m_cond = Condition::ptr_t(new ConditionDepArrAirspace(get_childnum(), m_uuid, false, m_refloc));
			break;

		case NodeAIXMFlightConditionCircumstance::locrel_arr:
			m_cond = Condition::ptr_t(new ConditionDepArrAirspace(get_childnum(), m_uuid, true, m_refloc));
			break;

		case NodeAIXMFlightConditionCircumstance::locrel_act:
			m_cond = Condition::ptr_t(new ConditionCrossingAirspaceActive(get_childnum(), m_uuid));
			break;

		default:
			m_cond.clear();
			error(get_static_element_name() + ": invalid location relation " +
			      NodeAIXMFlightConditionCircumstance::get_locrel_string(m_locrel) + " for Airspace Crossing");
			break;
		}
		return m_cond;

	case Condition::type_crossingairspace2:
		if (m_locrel != NodeAIXMFlightConditionCircumstance::locrel_xng)
			error(get_static_element_name() + ": invalid location relation " +
			      NodeAIXMFlightConditionCircumstance::get_locrel_string(m_locrel) + " for Airspace2 Crossing");
		m_cond = Condition::ptr_t(new ConditionCrossingAirspace2(get_childnum(), m_altrange, m_start, m_end, m_refloc));
		return m_cond;

	case Condition::type_crossingpoint:
		switch (m_locrel) {
		case NodeAIXMFlightConditionCircumstance::locrel_xng:
			m_cond = Condition::ptr_t(new ConditionCrossingPoint(get_childnum(), m_altrange, m_uuid, m_refloc));
			break;

		case NodeAIXMFlightConditionCircumstance::locrel_dep:
			m_cond = Condition::ptr_t(new ConditionDepArr(get_childnum(), m_uuid, false, m_refloc));
			break;

		case NodeAIXMFlightConditionCircumstance::locrel_arr:
			m_cond = Condition::ptr_t(new ConditionDepArr(get_childnum(), m_uuid, true, m_refloc));
			break;

		default:
			m_cond.clear();
			error(get_static_element_name() + ": invalid location relation " +
			      NodeAIXMFlightConditionCircumstance::get_locrel_string(m_locrel) + " for Point Crossing");
			break;
		}
		return m_cond;

	case Condition::type_crossingsidstar:
		if (m_locrel != NodeAIXMFlightConditionCircumstance::locrel_xng)
			error(get_static_element_name() + ": invalid location relation " +
			      NodeAIXMFlightConditionCircumstance::get_locrel_string(m_locrel) + " for SID/STAR Crossing");
		m_cond = Condition::ptr_t(new ConditionSidStar(get_childnum(), m_uuid, m_arr, m_refloc));
		return m_cond;

	case Condition::type_dctlimit:
		m_cond = Condition::ptr_t(new ConditionDctLimit(get_childnum(), m_length));
		return m_cond;

	case Condition::type_crossingdct:
		m_cond = Condition::ptr_t(new ConditionCrossingDct(get_childnum(), m_altrange, m_start, m_end, m_refloc));
		return m_cond;

	case Condition::type_crossingairway:
		switch (m_locrel) {
		case NodeAIXMFlightConditionCircumstance::locrel_xng:
			m_cond = Condition::ptr_t(new ConditionCrossingAirway(get_childnum(), m_altrange, m_start, m_end, m_uuid, m_refloc));
			break;

		case NodeAIXMFlightConditionCircumstance::locrel_avbl:
			m_cond = Condition::ptr_t(new ConditionCrossingAirwayAvailable(get_childnum(), m_altrange, m_start, m_end, m_uuid));
			break;

		default:
			m_cond.clear();
			error(get_static_element_name() + ": invalid location relation " +
			      NodeAIXMFlightConditionCircumstance::get_locrel_string(m_locrel) + " for Airway Crossing");
			break;
		}
		return m_cond;

	case Condition::type_aircraft:
		m_cond = Condition::ptr_t(new ConditionAircraft(get_childnum(), m_icaotype, m_engines, m_acfttype, m_engine, m_navspec, m_vertsep));
		return m_cond;

	case Condition::type_flight:
		m_cond = Condition::ptr_t(new ConditionFlight(get_childnum(), m_civmil, m_purpose));
		return m_cond;

	case Condition::type_and:
	case Condition::type_seq:
		return m_cond;

	default:
		m_cond.clear();
		error(get_static_element_name() + ": invalid type");
		return m_cond;
	}
}

const std::string ParseXML::NodeAIXMElement::elementname("aixm:element");

ParseXML::NodeAIXMElement::NodeAIXMElement(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMElement::on_subelement(const Glib::RefPtr<NodeAIXMElementBase>& el)
{
	if (!el)
		return;
	if (m_el)
		error(get_static_element_name() + ": multiple subelements");
	m_el = el;
}

const std::string ParseXML::NodeAIXMFlightConditionCombination::elementname("aixm:FlightConditionCombination");

ParseXML::NodeAIXMFlightConditionCombination::NodeAIXMFlightConditionCombination(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_operator(operator_invalid)
{
}

void ParseXML::NodeAIXMFlightConditionCombination::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmlogicaloperator:
		if (el->get_text() == "AND")
			m_operator = operator_and;
		else if (el->get_text() == "ANDNOT")
			m_operator = operator_andnot;
		else if (el->get_text() == "OR")
			m_operator = operator_or;
		else if (el->get_text() == "SEQ")
			m_operator = operator_seq;
		else
			error(get_static_element_name() + ": unknown logicalOperator " + el->get_element_name());
		break;

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMFlightConditionCombination::on_subelement(const Glib::RefPtr<NodeAIXMElement>& el)
{
	if (!el)
		return;
	NodeAIXMFlightConditionElement::ptr_t elx(el->get<NodeAIXMFlightConditionElement>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid element type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no element");
		return;
	}
	m_elements.push_back(element_t(elx->get_cond(), elx->get_index()));
	if (!m_elements.back().first) {
		error(get_static_element_name() + ": null condition " + el->get_element_name());
	} else {
		m_elements.back().first->set_childnum(el->get_childnum());
	}
}

void ParseXML::NodeAIXMFlightConditionCombination::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRFlightConditionCombinationExtension::ptr_t elx(el->get<NodeADRFlightConditionCombinationExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	m_timetable = elx->get_timetable();
}

Condition::ptr_t ParseXML::NodeAIXMFlightConditionCombination::get_cond(void) const
{
	// no element: means true
	if (m_elements.empty())
		return Condition::ptr_t(new ConditionConstant(get_childnum(), true));
	if (m_elements.size() == 1)
		return m_elements.front().first;
	bool finalinv(false), elinv(false), subseqinv(false);
	switch (m_operator) {
	case operator_and:
		finalinv = false;
		elinv = false;
		subseqinv = false;
		break;

	case operator_andnot:
		finalinv = false;
		elinv = false;
		subseqinv = true;
		break;

	case operator_or:
		finalinv = true;
		elinv = true;
		subseqinv = true;
		break;

	case operator_seq:
	{
		Glib::RefPtr<ConditionSequence> c(new ConditionSequence(get_childnum()));
		for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ) {
			c->add(i->first);
			elements_t::const_iterator i2(i);
			++i;
			if (i == e)
				break;
			if (i2->second >= i->second) {
				error(get_static_element_name() + ": sequence bad order of elements");
				return Condition::ptr_t();
			}
		}
		return c;
	}

	case operator_invalid:
	default:
		error(get_static_element_name() + ": invalid operator");
		return Condition::ptr_t();
	}
	Glib::RefPtr<ConditionAnd> c(new ConditionAnd(get_childnum(), finalinv));
	for (elements_t::const_iterator i(m_elements.begin()), e(m_elements.end()); i != e; ++i) {
		c->add(i->first, elinv);
		elinv = subseqinv;
	}
	return c;
}

const std::string ParseXML::NodeAIXMFlight::elementname("aixm:flight");

ParseXML::NodeAIXMFlight::NodeAIXMFlight(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlight::on_subelement(const Glib::RefPtr<NodeAIXMFlightConditionCombination>& el)
{
	if (!el)
		return;
	m_cond = el->get_cond();
	m_timetable = el->get_timetable();
}

const std::string ParseXML::NodeADRFlightRestrictionExtension::elementname("adr:FlightRestrictionExtension");

ParseXML::NodeADRFlightRestrictionExtension::NodeADRFlightRestrictionExtension(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMExtensionBase(parser, tagname, childnum, properties), m_enabled(false), m_procind(FlightRestrictionTimeSlice::procind_invalid)
{
}

void ParseXML::NodeADRFlightRestrictionExtension::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_adrprocessingindicator:
		if (el->get_text() == "TFR")
			m_procind = FlightRestrictionTimeSlice::procind_tfr;
		else if (el->get_text() == "RAD_DCT")
			m_procind = FlightRestrictionTimeSlice::procind_raddct;
		else if (el->get_text() == "FRA_DCT")
			m_procind = FlightRestrictionTimeSlice::procind_fradct;
		else if (el->get_text() == "FPR")
			m_procind = FlightRestrictionTimeSlice::procind_fpr;
		else if (el->get_text() == "AD_CP")
			m_procind = FlightRestrictionTimeSlice::procind_adcp;
		else if (el->get_text() == "OTHER:__ADR__AD_FLIGHT_RULE")
			m_procind = FlightRestrictionTimeSlice::procind_adfltrule;
		else if (el->get_text() == "OTHER:__ADR__FLIGHT_PROPERTY_ON_TP")
			m_procind = FlightRestrictionTimeSlice::procind_fltprop;
		else
			error(get_static_element_name() + ": unknown processingIndicator " + el->get_element_name());
		break;

	case NodeText::tt_adrenabled:
	{
		int b(el->get_yesno());
		if (b == -1) {
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid boolean " + el->get_text());
			break;
		}
		m_enabled = !!el->get_yesno();
		break;
	}

	default:
		error(get_static_element_name() + ": unknown text " + el->get_element_name());
		break;
	}
}

ParseXML::NodeAIXMAnyTimeSlice::NodeAIXMAnyTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties), m_validstart(0), m_validend(std::numeric_limits<long>::max()),
	  m_featurestart(0), m_featureend(std::numeric_limits<long>::max()), m_interpretation(interpretation_invalid)
{
}

void ParseXML::NodeAIXMAnyTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixminterpretation:
		if (el->get_text() == "BASELINE")
			m_interpretation = interpretation_baseline;
		else if (el->get_text() == "PERMDELTA")
			m_interpretation = interpretation_permdelta;
		else if (el->get_text() == "TEMPDELTA")
			m_interpretation = interpretation_tempdelta;
		else
			error("aixm TimeSlice: invalid interpretation: " + el->get_text());
		break;

	default:
		error("aixm TimeSlice: unknown text " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMAnyTimeSlice::on_subelement(const NodeGMLValidTime::ptr_t& el)
{
	if (el) {
		m_validstart = el->get_start();
		m_validend = el->get_end();
		TimeSlice& ts(get_tsbase());
		if (&ts) {
			uint64_t start(m_validstart), end(m_validend);
			if (m_validstart < 0)
				start = 0;
			else if (m_validstart == std::numeric_limits<long>::max())
				start = std::numeric_limits<uint64_t>::max();
			if (m_validend < 0)
				end = 0;
			else if (m_validend == std::numeric_limits<long>::max())
				end = std::numeric_limits<uint64_t>::max();
			ts.set_starttime(start);
			ts.set_endtime(end);
		}
	}
}

void ParseXML::NodeAIXMAnyTimeSlice::on_subelement(const NodeAIXMFeatureLifetime::ptr_t& el)
{
	if (el) {
		m_featurestart = el->get_start();
		m_featureend = el->get_end();
	}
}

const std::string ParseXML::NodeAIXMAirportHeliportTimeSlice::elementname("aixm:AirportHeliportTimeSlice");

ParseXML::NodeAIXMAirportHeliportTimeSlice::NodeAIXMAirportHeliportTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirportHeliportTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmname:
		m_timeslice.set_name(el->get_text());
		break;

	case NodeText::tt_aixmlocationindicatoricao:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixmdesignatoriata:
		m_timeslice.set_iata(el->get_text());
		break;

	case NodeText::tt_aixmcontroltype:
		if (el->get_text() == "CIVIL") {
			m_timeslice.set_civ(true);
		} else if (el->get_text() == "MIL") {
			m_timeslice.set_mil(true);
		} else if (el->get_text() == "JOINT") {
			m_timeslice.set_civ(true);
			m_timeslice.set_mil(true);
		} else {
			error(get_static_element_name() + ": unknown " + el->get_element_name() + " " + el->get_text());
		}
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMAirportHeliportTimeSlice::on_subelement(const NodeAIXMServedCity::ptr_t& el)
{
	if (!el)
		return;
	m_timeslice.set_cities(el->get_cities());
}

void ParseXML::NodeAIXMAirportHeliportTimeSlice::on_subelement(const NodeAIXMARP::ptr_t& el)
{
	if (!el)
		return;
	m_timeslice.set_coord(el->get_coord());
	m_timeslice.set_elev(el->get_elev());
}

const std::string ParseXML::NodeAIXMAirportHeliportCollocationTimeSlice::elementname("aixm:AirportHeliportCollocationTimeSlice");

ParseXML::NodeAIXMAirportHeliportCollocationTimeSlice::NodeAIXMAirportHeliportCollocationTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirportHeliportCollocationTimeSlice::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmhostairport:
		m_timeslice.set_host(el->get_uuid());
		break;

	case NodeLink::lt_aixmdependentairport:
		m_timeslice.set_dep(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMDesignatedPointTimeSlice::elementname("aixm:DesignatedPointTimeSlice");

ParseXML::NodeAIXMDesignatedPointTimeSlice::NodeAIXMDesignatedPointTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMDesignatedPointTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmname:
		m_timeslice.set_name(el->get_text());
		break;

	case NodeText::tt_aixmtype:
		if (el->get_text() == "ICAO")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_icao);
		else if (el->get_text() == "TERMINAL")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_terminal);
		else if (el->get_text() == "COORD")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_coord);
		else if (el->get_text() == "OTHER:__ADR__BOUNDARY")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_adrboundary);
		else if (el->get_text() == "OTHER:__ADR__REFERENCE")
			m_timeslice.set_type(ADR::DesignatedPointTimeSlice::type_adrreference);
		else
			error(get_static_element_name() + ": unknown " + el->get_element_name() + " " + el->get_text());
		break;

	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMDesignatedPointTimeSlice::on_subelement(const NodeAIXMLocation::ptr_t& el)
{
	if (el)
		m_timeslice.set_coord(el->get_coord());
}

const std::string ParseXML::NodeAIXMNavaidTimeSlice::elementname("aixm:NavaidTimeSlice");

ParseXML::NodeAIXMNavaidTimeSlice::NodeAIXMNavaidTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMNavaidTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmname:
		m_timeslice.set_name(el->get_text());
		break;

	case NodeText::tt_aixmtype:
		if (el->get_text() == "VOR")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_vor);
		else if (el->get_text() == "VOR_DME")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_vor_dme);
		else if (el->get_text() == "VORTAC")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_vortac);
		else if (el->get_text() == "TACAN")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_tacan);
		else if (el->get_text() == "ILS")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_ils);
		else if (el->get_text() == "ILS_DME")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_ils_dme);
		else if (el->get_text() == "LOC")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_loc);
		else if (el->get_text() == "LOC_DME")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_loc_dme);
		else if (el->get_text() == "DME")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_dme);
		else if (el->get_text() == "NDB")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_ndb);
		else if (el->get_text() == "NDB_DME")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_ndb_dme);
		else if (el->get_text() == "NDB_MKR")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_ndb_mkr);
		else if (el->get_text() == "MKR")
			m_timeslice.set_type(ADR::NavaidTimeSlice::type_mkr);
		else
			error(get_static_element_name() + ": unknown " + el->get_element_name() + " " + el->get_text());
		break;

	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMNavaidTimeSlice::on_subelement(const NodeAIXMLocation::ptr_t& el)
{
	if (el) {
		m_timeslice.set_coord(el->get_coord());
		m_timeslice.set_elev(el->get_elev());
	}
}

const std::string ParseXML::NodeAIXMAngleIndicationTimeSlice::elementname("aixm:AngleIndicationTimeSlice");

ParseXML::NodeAIXMAngleIndicationTimeSlice::NodeAIXMAngleIndicationTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAngleIndicationTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmangletype:
		if (el->get_text() != "MAG")
			error(get_static_element_name() + ": unknown " + el->get_element_name() + " " + el->get_text());
		break;

	case NodeText::tt_aixmangle:
		m_timeslice.set_angle_deg(el->get_double());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMAngleIndicationTimeSlice::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmfix:
		m_timeslice.set_fix(el->get_uuid());
		break;

	case NodeLink::lt_aixmpointchoicenavaidsystem:
		m_timeslice.set_navaid(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMDistanceIndicationTimeSlice::elementname("aixm:DistanceIndicationTimeSlice");

ParseXML::NodeAIXMDistanceIndicationTimeSlice::NodeAIXMDistanceIndicationTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMDistanceIndicationTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdistance:
		//m_dist = el->get_double();
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMDistanceIndicationTimeSlice::on_subelement(const NodeLink::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmfix:
		m_timeslice.set_fix(el->get_uuid());
		break;

	case NodeLink::lt_aixmpointchoicenavaidsystem:
		m_timeslice.set_navaid(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMAirspaceTimeSlice::elementname("aixm:AirspaceTimeSlice");

ParseXML::NodeAIXMAirspaceTimeSlice::NodeAIXMAirspaceTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirspaceTimeSlice::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmtype:
		if (el->get_text() == "ATZ")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_atz);
		else if (el->get_text() == "CBA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_cba);
		else if (el->get_text() == "CTA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_cta);
		else if (el->get_text() == "CTA_P")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_cta_p);
		else if (el->get_text() == "CTR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_ctr);
		else if (el->get_text() == "D")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_d);
		else if (el->get_text() == "D_OTHER")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_d_other);
		else if (el->get_text() == "FIR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_fir);
		else if (el->get_text() == "FIR_P")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_fir_p);
		else if (el->get_text() == "NAS")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_nas);
		else if (el->get_text() == "OCA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_oca);
		else if (el->get_text() == "OTHER:__ADR__AUAG")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_adr_auag);
		else if (el->get_text() == "OTHER:__ADR__FAB")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_adr_fab);
		else if (el->get_text() == "P")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_p);
		else if (el->get_text() == "PART")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_part);
		else if (el->get_text() == "R")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_r);
		else if (el->get_text() == "SECTOR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_sector);
		else if (el->get_text() == "SECTOR_C")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_sector_c);
		else if (el->get_text() == "TMA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_tma);
		else if (el->get_text() == "TRA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_tra);
		else if (el->get_text() == "TSA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_tsa);
		else if (el->get_text() == "UIR")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_uir);
		else if (el->get_text() == "UTA")
			m_timeslice.set_type(ADR::AirspaceTimeSlice::type_uta);
		else
			error(get_static_element_name() + ": unknown type " + el->get_text());
		break;

	case NodeText::tt_aixmlocaltype:
		if (el->get_text() == "MRA")
			m_timeslice.set_localtype(ADR::AirspaceTimeSlice::localtype_mra);
		else if (el->get_text() == "MTA")
			m_timeslice.set_localtype(ADR::AirspaceTimeSlice::localtype_mta);
		else 
			error(get_static_element_name() + ": unknown local type " + el->get_text());
		break;

	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixmdesignatoricao:
	{
		int b(el->get_yesno());
		if (b == -1) {
			error(get_static_element_name() + "/" + el->get_element_name() + ": invalid boolean " + el->get_text());
			break;
		}
		m_timeslice.set_icao(!!b);
		break;
	}

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMAirspaceTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMGeometryComponent>& el)
{
	if (!el)
		return;
	m_timeslice.get_components().push_back(el->get_comp());
}

void ParseXML::NodeAIXMAirspaceTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMActivation>& el)
{
	if (!el)
		return;
	NodeAIXMAirspaceActivation::ptr_t elx(el->get<NodeAIXMAirspaceActivation>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid activation type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no activation");
		return;
	}
	m_timeslice.set_timetable(elx->get_timetable());
}

void ParseXML::NodeAIXMAirspaceTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRAirspaceExtension::ptr_t elx(el->get<NodeADRAirspaceExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	m_timeslice.set_flexibleuse(elx->is_flexibleuse());
	m_timeslice.set_level(1, elx->is_level1());
	m_timeslice.set_level(2, elx->is_level2());
	m_timeslice.set_level(3, elx->is_level3());
}

const AirspaceTimeSlice& ParseXML::NodeAIXMAirspaceTimeSlice::get_timeslice(void) const
{
	if (m_timeslice.get_components().size() == 1 &&
	    m_timeslice.get_components()[0].get_operator() == AirspaceTimeSlice::Component::operator_invalid)
		const_cast<AirspaceTimeSlice::Component&>(m_timeslice.get_components()[0]).set_operator(AirspaceTimeSlice::Component::operator_base);
	return m_timeslice;
}

const std::string ParseXML::NodeAIXMStandardLevelTableTimeSlice::elementname("aixm:StandardLevelTableTimeSlice");

ParseXML::NodeAIXMStandardLevelTableTimeSlice::NodeAIXMStandardLevelTableTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardLevelTableTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmname:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixmstandardicao:
	{
		int t(el->get_yesno());
		if (t == -1)
			error(get_static_element_name() + ": invalid standardICAO " + el->get_text());
		else
			m_timeslice.set_standardicao(!!t);
		break;
	}

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

const std::string ParseXML::NodeAIXMStandardLevelColumnTimeSlice::elementname("aixm:StandardLevelColumnTimeSlice");

ParseXML::NodeAIXMStandardLevelColumnTimeSlice::NodeAIXMStandardLevelColumnTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardLevelColumnTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmseries:
		if (el->get_text() == "EVEN")
			m_timeslice.set_series(StandardLevelColumnTimeSlice::series_even);
		else if (el->get_text() == "ODD")
			m_timeslice.set_series(StandardLevelColumnTimeSlice::series_odd);
		else if (el->get_text() == "OTHER:__ADR__UNIDIRECTIONAL")
			m_timeslice.set_series(StandardLevelColumnTimeSlice::series_unidirectional);
		else 
			error(get_static_element_name() + ": invalid series " + el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMStandardLevelColumnTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmleveltable:
		m_timeslice.set_leveltable(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMRouteTimeSlice::elementname("aixm:RouteTimeSlice");

ParseXML::NodeAIXMRouteTimeSlice::NodeAIXMRouteTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRouteTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdesignatorprefix:
		m_desigprefix = el->get_text();
		break;

	case NodeText::tt_aixmdesignatorsecondletter:
		m_desigsecondletter = el->get_text();
		break;

	case NodeText::tt_aixmdesignatornumber:
		m_designumber = el->get_text();
		break;

	case NodeText::tt_aixmmultipleidentifier:
		m_multiid = el->get_text();
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
	m_timeslice.set_ident(m_desigprefix + m_desigsecondletter + m_designumber + m_multiid);
}

const std::string ParseXML::NodeAIXMRouteSegmentTimeSlice::elementname("aixm:RouteSegmentTimeSlice");

ParseXML::NodeAIXMRouteSegmentTimeSlice::NodeAIXMRouteSegmentTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlimit:
	case NodeText::tt_aixmlowerlimit:
	case NodeText::tt_aixmupperlimitreference:
	case NodeText::tt_aixmlowerlimitreference:
		el->update(m_timeslice.get_altrange());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmrouteformed:
		m_timeslice.set_route(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMStart>& el)
{
	if (el)
		m_timeslice.set_start(el->get_uuid());
}

void ParseXML::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMEnd>& el)
{
	if (el)
		m_timeslice.set_end(el->get_uuid());
}

void ParseXML::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el)
{
	if (!el)
		return;
	NodeAIXMRouteAvailability::ptr_t elx(el->get<NodeAIXMRouteAvailability>());
	if (!elx) {
		error(get_static_element_name() + ": invalid availability type " + el->get_element_name());
		return;
	}
	m_timeslice.get_availability().push_back(elx->get_availability());
}

void ParseXML::NodeAIXMRouteSegmentTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRRouteSegmentExtension::ptr_t elx(el->get<NodeADRRouteSegmentExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	m_timeslice.get_levels().push_back(RouteSegmentTimeSlice::Level());
	m_timeslice.get_levels().back().set_timetable(elx->get_timetable());
	m_timeslice.get_levels().back().set_altrange(elx->get_altrange());
}

const std::string ParseXML::NodeAIXMStandardInstrumentDepartureTimeSlice::elementname("aixm:StandardInstrumentDepartureTimeSlice");

ParseXML::NodeAIXMStandardInstrumentDepartureTimeSlice::NodeAIXMStandardInstrumentDepartureTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardInstrumentDepartureTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixminstruction:
		m_timeslice.set_instruction(el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMStandardInstrumentDepartureTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmairportheliport:
		m_timeslice.set_airport(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMStandardInstrumentDepartureTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el)
{
	if (!el)
		return;
	NodeAIXMProcedureAvailability::ptr_t elx(el->get<NodeAIXMProcedureAvailability>());
	if (!elx) {
		error(get_static_element_name() + ": invalid availability type " + el->get_element_name());
		return;
	}
	m_timeslice.set_timetable(elx->get_timetable());
	m_timeslice.set_status(elx->get_status());
}

void ParseXML::NodeAIXMStandardInstrumentDepartureTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRStandardInstrumentDepartureExtension::ptr_t elx(el->get<NodeADRStandardInstrumentDepartureExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	NodeADRStandardInstrumentDepartureExtension::connpt_t c(elx->get_connpt());
	for (NodeADRStandardInstrumentDepartureExtension::connpt_t::const_iterator ci(c.begin()), ce(c.end()); ci != ce; ++ci)
		m_timeslice.get_connpoints().insert(*ci);
}

const std::string ParseXML::NodeAIXMStandardInstrumentArrivalTimeSlice::elementname("aixm:StandardInstrumentArrivalTimeSlice");

ParseXML::NodeAIXMStandardInstrumentArrivalTimeSlice::NodeAIXMStandardInstrumentArrivalTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardInstrumentArrivalTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixminstruction:
		m_timeslice.set_instruction(el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMStandardInstrumentArrivalTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmairportheliport:
		m_timeslice.set_airport(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMStandardInstrumentArrivalTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMAvailability>& el)
{
	if (!el)
		return;
	NodeAIXMProcedureAvailability::ptr_t elx(el->get<NodeAIXMProcedureAvailability>());
	if (!elx) {
		error(get_static_element_name() + ": invalid availability type " + el->get_element_name());
		return;
	}
	m_timeslice.set_timetable(elx->get_timetable());
	m_timeslice.set_status(elx->get_status());
}

void ParseXML::NodeAIXMStandardInstrumentArrivalTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRStandardInstrumentArrivalExtension::ptr_t elx(el->get<NodeADRStandardInstrumentArrivalExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	NodeADRStandardInstrumentArrivalExtension::connpt_t c(elx->get_connpt());
	for (NodeADRStandardInstrumentArrivalExtension::connpt_t::const_iterator ci(c.begin()), ce(c.end()); ci != ce; ++ci)
		m_timeslice.get_connpoints().insert(*ci);
	m_timeslice.set_iaf(elx->get_iaf());
}

const std::string ParseXML::NodeAIXMDepartureLegTimeSlice::elementname("aixm:DepartureLegTimeSlice");

ParseXML::NodeAIXMDepartureLegTimeSlice::NodeAIXMDepartureLegTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMDepartureLegTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlimitaltitude:
	case NodeText::tt_aixmlowerlimitaltitude:
	case NodeText::tt_aixmupperlimitreference:
	case NodeText::tt_aixmlowerlimitreference:
		el->update(m_timeslice.get_altrange());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMDepartureLegTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmdeparture:
		m_timeslice.set_route(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMDepartureLegTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMStartPoint>& el)
{
	if (el)
		m_timeslice.set_start(el->get_uuid());
}

void ParseXML::NodeAIXMDepartureLegTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMEndPoint>& el)
{
	if (el)
		m_timeslice.set_end(el->get_uuid());
}

const std::string ParseXML::NodeAIXMArrivalLegTimeSlice::elementname("aixm:ArrivalLegTimeSlice");

ParseXML::NodeAIXMArrivalLegTimeSlice::NodeAIXMArrivalLegTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMArrivalLegTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmupperlimitaltitude:
	case NodeText::tt_aixmlowerlimitaltitude:
	case NodeText::tt_aixmupperlimitreference:
	case NodeText::tt_aixmlowerlimitreference:
		el->update(m_timeslice.get_altrange());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMArrivalLegTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmarrival:
		m_timeslice.set_route(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMArrivalLegTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMStartPoint>& el)
{
	if (el)
		m_timeslice.set_start(el->get_uuid());
}

void ParseXML::NodeAIXMArrivalLegTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMEndPoint>& el)
{
	if (el)
		m_timeslice.set_end(el->get_uuid());
}

const std::string ParseXML::NodeAIXMOrganisationAuthorityTimeSlice::elementname("aixm:OrganisationAuthorityTimeSlice");

ParseXML::NodeAIXMOrganisationAuthorityTimeSlice::NodeAIXMOrganisationAuthorityTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMOrganisationAuthorityTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmtype:
		if (el->get_text() != "STATE")
			error(get_static_element_name() + ": unknown type " + el->get_text());
		break;

	case NodeText::tt_aixmname:
		m_timeslice.set_name(el->get_text());
		break;

	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		m_timeslice.set_border(UUID::from_countryborder(el->get_text()));
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

const std::string ParseXML::NodeAIXMSpecialDateTimeSlice::elementname("aixm:SpecialDateTimeSlice");

ParseXML::NodeAIXMSpecialDateTimeSlice::NodeAIXMSpecialDateTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMSpecialDateTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmtype:
		if (el->get_text() == "HOL")
			m_timeslice.set_type(SpecialDateTimeSlice::type_hol);
		else if (el->get_text() == "BUSY_FRI")
			m_timeslice.set_type(SpecialDateTimeSlice::type_busyfri);
		else 
			error(get_static_element_name() + ": invalid type " + el->get_text());
		break;

	case NodeText::tt_aixmname:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixmdateday:
	{
		const char *cp(el->get_text().c_str());
		char *cp1;
		m_timeslice.set_day(strtoul(cp, &cp1, 10));
		if (cp1 == cp || !cp1 || *cp1 != '-') {
			m_timeslice.set_day(0);
			m_timeslice.set_month(0);
			error(get_static_element_name() + ": invalid date " + el->get_text());
			break;
		}
		cp = cp1 + 1;
		m_timeslice.set_month(strtoul(cp, &cp1, 10));
		if (cp1 == cp || !cp1 || (*cp1 && *cp1 != ' ')) {
			m_timeslice.set_day(0);
			m_timeslice.set_month(0);
			error(get_static_element_name() + ": invalid date " + el->get_text());
			break;
		}
		break;
	}

	case NodeText::tt_aixmdateyear:
		m_timeslice.set_year(el->get_ulong());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMSpecialDateTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmauthority:
		m_timeslice.set_authority(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

const std::string ParseXML::NodeAIXMUnitTimeSlice::elementname("aixm:UnitTimeSlice");

ParseXML::NodeAIXMUnitTimeSlice::NodeAIXMUnitTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMUnitTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmtype:
		if (el->get_text() != "OTHER:__ADR__AMC")
			error(get_static_element_name() + ": invalid series " + el->get_text());
		break;

	case NodeText::tt_aixmname:
		m_timeslice.set_name(el->get_text());
		break;

	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

const std::string ParseXML::NodeAIXMAirTrafficManagementServiceTimeSlice::elementname("aixm:AirTrafficManagementServiceTimeSlice");

ParseXML::NodeAIXMAirTrafficManagementServiceTimeSlice::NodeAIXMAirTrafficManagementServiceTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirTrafficManagementServiceTimeSlice::on_subelement(const Glib::RefPtr<NodeLink>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeLink::lt_aixmserviceprovider:
		m_timeslice.set_serviceprovider(el->get_uuid());
		break;

	case NodeLink::lt_aixmclientairspace:
		m_timeslice.get_clientairspaces().insert(el->get_uuid());
		break;

	default:
		error(get_static_element_name() + ": unknown link " + el->get_element_name());
		break;
	}
}

void ParseXML::NodeAIXMAirTrafficManagementServiceTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMClientRoute>& el)
{
	if (!el)
		return;
}

const std::string ParseXML::NodeAIXMFlightRestrictionTimeSlice::elementname("aixm:FlightRestrictionTimeSlice");

ParseXML::NodeAIXMFlightRestrictionTimeSlice::NodeAIXMFlightRestrictionTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMAnyTimeSlice(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlightRestrictionTimeSlice::on_subelement(const Glib::RefPtr<NodeText>& el)
{
	if (!el)
		return;
	switch (el->get_type()) {
	case NodeText::tt_aixmdesignator:
		m_timeslice.set_ident(el->get_text());
		break;

	case NodeText::tt_aixminstruction:
		m_timeslice.set_instruction(el->get_text());
		break;

	case NodeText::tt_aixmtype:
		if (el->get_text() == "ALLOWED")
			m_timeslice.set_type(FlightRestrictionTimeSlice::type_allowed);
		else if (el->get_text() == "MANDATORY")
			m_timeslice.set_type(FlightRestrictionTimeSlice::type_mandatory);
		else if (el->get_text() == "CLSD")
			m_timeslice.set_type(FlightRestrictionTimeSlice::type_closed);
		else if (el->get_text() == "FORBID")
			m_timeslice.set_type(FlightRestrictionTimeSlice::type_forbidden);
		else
			error(get_static_element_name() + ": unknown type " + el->get_element_name());
		break;

	default:
		NodeAIXMAnyTimeSlice::on_subelement(el);
		break;
	}
}

void ParseXML::NodeAIXMFlightRestrictionTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMFlight>& el)
{
	if (!el)
		return;
	m_timeslice.set_timetable(el->get_timetable());
	m_timeslice.set_condition(el->get_cond());
}

void ParseXML::NodeAIXMFlightRestrictionTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMRegulatedRoute>& el)
{
	if (!el)
		return;
	m_timeslice.get_restrictions().add(el->get_res());
}

void ParseXML::NodeAIXMFlightRestrictionTimeSlice::on_subelement(const Glib::RefPtr<NodeAIXMExtension>& el)
{
	if (!el)
		return;
	NodeADRFlightRestrictionExtension::ptr_t elx(el->get<NodeADRFlightRestrictionExtension>());
	if (!elx) {
		if (el->get_el())
			error(get_static_element_name() + ": invalid extension type " + el->get_el()->get_element_name());
		else
			error(get_static_element_name() + ": no extension");
		return;
	}
	m_timeslice.set_enabled(elx->is_enabled());
	m_timeslice.set_procind(elx->get_procind());
}

const std::string ParseXML::NodeAIXMTimeSlice::elementname("aixm:timeSlice");

ParseXML::NodeAIXMTimeSlice::NodeAIXMTimeSlice(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMTimeSlice::on_subelement(const NodeAIXMAnyTimeSlice::ptr_t& el)
{
	if (el)
		m_nodes.push_back(el);
}

ParseXML::NodeAIXMObject::NodeAIXMObject(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMObject::on_subelement(const NodeText::ptr_t& el)
{
	if (!el)
		return;
	if (el->get_type() != NodeText::tt_gmlidentifier) {
		error("Object: unknown text " + el->get_element_name());
		return;
	}
	m_uuid = el->get_uuid();
	if (m_uuid.is_nil())
		error("Object: invalid UUID");
}

namespace ADR {

template<typename T>
void ParseXML::NodeAIXMObject::fake_uuid(const T& el)
{
}

template<>
void ParseXML::NodeAIXMObject::fake_uuid(const NodeAIXMAirportHeliportCollocationTimeSlice::ptr_t& el)
{
	m_uuid = UUID(el->get_timeslice().get_host(), "AirportCollocation");
}

template<>
void ParseXML::NodeAIXMObject::fake_uuid(const NodeAIXMAirTrafficManagementServiceTimeSlice::ptr_t& el)
{
	m_uuid = UUID(el->get_timeslice().get_serviceprovider(), "AirTrafficManagementService");
}

template<>
void ParseXML::NodeAIXMObject::fake_uuid(const NodeAIXMAngleIndicationTimeSlice::ptr_t& el)
{
	std::ostringstream oss;
	oss << "AngleIndication" << el->get_timeslice().get_fix() << el->get_timeslice().get_angle_deg();
	m_uuid = UUID(el->get_timeslice().get_navaid(), oss.str());
}

template<>
void ParseXML::NodeAIXMObject::fake_uuid(const NodeAIXMDistanceIndicationTimeSlice::ptr_t& el)
{
	std::ostringstream oss;
	oss << "DistanceIndication" << el->get_timeslice().get_fix() << el->get_timeslice().get_dist_nmi();
	m_uuid = UUID(el->get_timeslice().get_navaid(), oss.str());
}

template<typename BT, typename TS>
void ParseXML::NodeAIXMObject::on_timeslice(const NodeAIXMTimeSlice::ptr_t& el)
{
	if (!el)
		return;
	for (unsigned int i = 0, n = el->size(); i < n; ++i) {
		NodeAIXMAnyTimeSlice::ptr_t ely(el->operator[](i));
		if (!ely)
			continue;
		typename TS::ptr_t elx(TS::ptr_t::cast_dynamic(ely));
		if (!elx) {
			error(get_element_name() + ": invalid TimeSlice " + ely->get_element_name());
			continue;
		}
		if (m_uuid.is_nil()) {
			fake_uuid(elx);
			if (m_uuid.is_nil())
				error(get_element_name() + ": no UUID");
		}
		typename BT::ptr_t p;
		{
			Object::const_ptr_t p1(m_parser.m_db.load_temp(m_uuid));
			if (!p1)
				p1 = m_parser.m_db.load(m_uuid);
			if (p1) {
				p = BT::ptr_t::cast_dynamic(Object::ptr_t::cast_const(p1));
				if (!p) {
					std::ostringstream oss;
					p1->print(oss << "Object ") << " not " << to_str(BT::get_static_type()) << ", discarding";
					warning(oss.str());
				}
			}
		}
		if (!p) {
			p = typename BT::ptr_t(new BT(m_uuid));
			p->set_modified();
			p->set_dirty();
		}
		p->add_timeslice(elx->get_timeslice());
		if (p->is_dirty()) {
			m_parser.m_db.save_temp(p);
			if (m_parser.m_verbose)
				p->print(std::cout);
		}
	}
}

};

const std::string ParseXML::NodeAIXMAirportHeliport::elementname("aixm:AirportHeliport");

ParseXML::NodeAIXMAirportHeliport::NodeAIXMAirportHeliport(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirportHeliport::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<Airport, NodeAIXMAirportHeliportTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMAirportHeliportCollocation::elementname("aixm:AirportHeliportCollocation");

ParseXML::NodeAIXMAirportHeliportCollocation::NodeAIXMAirportHeliportCollocation(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirportHeliportCollocation::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<AirportCollocation, NodeAIXMAirportHeliportCollocationTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMDesignatedPoint::elementname("aixm:DesignatedPoint");

ParseXML::NodeAIXMDesignatedPoint::NodeAIXMDesignatedPoint(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMDesignatedPoint::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<DesignatedPoint, NodeAIXMDesignatedPointTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMNavaid::elementname("aixm:Navaid");

ParseXML::NodeAIXMNavaid::NodeAIXMNavaid(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMNavaid::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<Navaid, NodeAIXMNavaidTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMAngleIndication::elementname("aixm:AngleIndication");

ParseXML::NodeAIXMAngleIndication::NodeAIXMAngleIndication(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAngleIndication::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<AngleIndication, NodeAIXMAngleIndicationTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMDistanceIndication::elementname("aixm:DistanceIndication");

ParseXML::NodeAIXMDistanceIndication::NodeAIXMDistanceIndication(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMDistanceIndication::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<DistanceIndication, NodeAIXMDistanceIndicationTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMAirspace::elementname("aixm:Airspace");

ParseXML::NodeAIXMAirspace::NodeAIXMAirspace(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirspace::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<Airspace, NodeAIXMAirspaceTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMStandardLevelTable::elementname("aixm:StandardLevelTable");

ParseXML::NodeAIXMStandardLevelTable::NodeAIXMStandardLevelTable(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardLevelTable::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<StandardLevelTable, NodeAIXMStandardLevelTableTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMStandardLevelColumn::elementname("aixm:StandardLevelColumn");

ParseXML::NodeAIXMStandardLevelColumn::NodeAIXMStandardLevelColumn(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardLevelColumn::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<StandardLevelColumn, NodeAIXMStandardLevelColumnTimeSlice>(el);
	if (!el)
		return;
}

const std::string ParseXML::NodeAIXMRouteSegment::elementname("aixm:RouteSegment");

ParseXML::NodeAIXMRouteSegment::NodeAIXMRouteSegment(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRouteSegment::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<RouteSegment, NodeAIXMRouteSegmentTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMRoute::elementname("aixm:Route");

ParseXML::NodeAIXMRoute::NodeAIXMRoute(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMRoute::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<Route, NodeAIXMRouteTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMStandardInstrumentDeparture::elementname("aixm:StandardInstrumentDeparture");

ParseXML::NodeAIXMStandardInstrumentDeparture::NodeAIXMStandardInstrumentDeparture(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardInstrumentDeparture::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<StandardInstrumentDeparture, NodeAIXMStandardInstrumentDepartureTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMStandardInstrumentArrival::elementname("aixm:StandardInstrumentArrival");

ParseXML::NodeAIXMStandardInstrumentArrival::NodeAIXMStandardInstrumentArrival(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMStandardInstrumentArrival::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<StandardInstrumentArrival, NodeAIXMStandardInstrumentArrivalTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMDepartureLeg::elementname("aixm:DepartureLeg");

ParseXML::NodeAIXMDepartureLeg::NodeAIXMDepartureLeg(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMDepartureLeg::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<DepartureLeg, NodeAIXMDepartureLegTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMArrivalLeg::elementname("aixm:ArrivalLeg");

ParseXML::NodeAIXMArrivalLeg::NodeAIXMArrivalLeg(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMArrivalLeg::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<ArrivalLeg, NodeAIXMArrivalLegTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMOrganisationAuthority::elementname("aixm:OrganisationAuthority");

ParseXML::NodeAIXMOrganisationAuthority::NodeAIXMOrganisationAuthority(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMOrganisationAuthority::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<OrganisationAuthority, NodeAIXMOrganisationAuthorityTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMSpecialDate::elementname("aixm:SpecialDate");

ParseXML::NodeAIXMSpecialDate::NodeAIXMSpecialDate(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMSpecialDate::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<SpecialDate, NodeAIXMSpecialDateTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMUnit::elementname("aixm:Unit");

ParseXML::NodeAIXMUnit::NodeAIXMUnit(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMUnit::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<Unit, NodeAIXMUnitTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMAirTrafficManagementService::elementname("aixm:AirTrafficManagementService");

ParseXML::NodeAIXMAirTrafficManagementService::NodeAIXMAirTrafficManagementService(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMAirTrafficManagementService::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<AirTrafficManagementService, NodeAIXMAirTrafficManagementServiceTimeSlice>(el);
}

const std::string ParseXML::NodeAIXMFlightRestriction::elementname("aixm:FlightRestriction");

ParseXML::NodeAIXMFlightRestriction::NodeAIXMFlightRestriction(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeAIXMObject(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeAIXMFlightRestriction::on_subelement(const NodeAIXMTimeSlice::ptr_t& el)
{
	on_timeslice<FlightRestriction, NodeAIXMFlightRestrictionTimeSlice>(el);
}

const std::string ParseXML::NodeADRMessageMember::elementname("adrmsg:hasMember");

ParseXML::NodeADRMessageMember::NodeADRMessageMember(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: NodeNoIgnore(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRMessageMember::on_subelement(const NodeAIXMObject::ptr_t& el)
{
	if (el)
		m_obj = el;
}

const std::string ParseXML::NodeADRMessage::elementname("adrmsg:ADRMessage");

ParseXML::NodeADRMessage::NodeADRMessage(ParseXML& parser, const std::string& tagname, unsigned int childnum, const AttributeList& properties)
	: Node(parser, tagname, childnum, properties)
{
}

void ParseXML::NodeADRMessage::on_subelement(const NodeADRMessageMember::ptr_t& el)
{
}

const bool ParseXML::tracestack;

ParseXML::ParseXML(Database& db, bool verbose)
	: m_db(db), m_errorcnt(0), m_warncnt(0), m_verbose(verbose)
{
	NodeText::check_type_order();
	NodeLink::check_type_order();
	// create factory table
	m_factory[NodeADRMessageMember::get_static_element_name()] = NodeADRMessageMember::create;
	m_factory[NodeADRWeekDayPattern::get_static_element_name()] = NodeADRWeekDayPattern::create;
	m_factory[NodeADRSpecialDayPattern::get_static_element_name()] = NodeADRSpecialDayPattern::create;
	m_factory[NodeADRTimePatternCombination::get_static_element_name()] = NodeADRTimePatternCombination::create;
	m_factory[NodeADRTimeTable1::get_static_element_name()] = NodeADRTimeTable1::create;
	m_factory[NodeADRTimeTable::get_static_element_name()] = NodeADRTimeTable::create;
	m_factory[NodeADRLevels::get_static_element_name()] = NodeADRLevels::create;
	m_factory[NodeADRPropertiesWithScheduleExtension::get_static_element_name()] = NodeADRPropertiesWithScheduleExtension::create;
	m_factory[NodeADRDefaultApplicability::get_static_element_name()] = NodeADRDefaultApplicability::create;
	m_factory[NodeADRAirspaceExtension::get_static_element_name()] = NodeADRAirspaceExtension::create;
	m_factory[NodeADRNonDefaultApplicability::get_static_element_name()] = NodeADRNonDefaultApplicability::create;
	m_factory[NodeADRRouteAvailabilityExtension::get_static_element_name()] = NodeADRRouteAvailabilityExtension::create;
	m_factory[NodeADRProcedureAvailabilityExtension::get_static_element_name()] = NodeADRProcedureAvailabilityExtension::create;
	m_factory[NodeADRRouteSegmentExtension::get_static_element_name()] = NodeADRRouteSegmentExtension::create;
	m_factory[NodeADRIntermediateSignificantPoint::get_static_element_name()] = NodeADRIntermediateSignificantPoint::create;
	m_factory[NodeADRIntermediatePoint::get_static_element_name()] = NodeADRIntermediatePoint::create;
	m_factory[NodeADRRoutePortionExtension::get_static_element_name()] = NodeADRRoutePortionExtension::create;
	m_factory[NodeADRStandardInstrumentDepartureExtension::get_static_element_name()] = NodeADRStandardInstrumentDepartureExtension::create;
	m_factory[NodeADRStandardInstrumentArrivalExtension::get_static_element_name()] = NodeADRStandardInstrumentArrivalExtension::create;
	m_factory[NodeAIXMExtension::get_static_element_name()] = NodeAIXMExtension::create;
	m_factory[NodeAIXMAirspaceActivation::get_static_element_name()] = NodeAIXMAirspaceActivation::create;
	m_factory[NodeAIXMActivation::get_static_element_name()] = NodeAIXMActivation::create;
	m_factory[NodeAIXMElevatedPoint::get_static_element_name()] = NodeAIXMElevatedPoint::create;
	m_factory[NodeAIXMPoint::get_static_element_name()] = NodeAIXMPoint::create;
	m_factory[NodeAIXMARP::get_static_element_name()] = NodeAIXMARP::create;
	m_factory[NodeAIXMLocation::get_static_element_name()] = NodeAIXMLocation::create;
	m_factory[NodeAIXMCity::get_static_element_name()] = NodeAIXMCity::create;
	m_factory[NodeAIXMServedCity::get_static_element_name()] = NodeAIXMServedCity::create;
	m_factory[NodeGMLValidTime::get_static_element_name()] = NodeGMLValidTime::create;
	m_factory[NodeAIXMFeatureLifetime::get_static_element_name()] = NodeAIXMFeatureLifetime::create;
	m_factory[NodeGMLBeginPosition::get_static_element_name()] = NodeGMLBeginPosition::create;
	m_factory[NodeGMLEndPosition::get_static_element_name()] = NodeGMLEndPosition::create;
	m_factory[NodeGMLTimePeriod::get_static_element_name()] = NodeGMLTimePeriod::create;
	m_factory[NodeGMLLinearRing::get_static_element_name()] = NodeGMLLinearRing::create;
	m_factory[NodeGMLInterior::get_static_element_name()] = NodeGMLInterior::create;
	m_factory[NodeGMLExterior::get_static_element_name()] = NodeGMLExterior::create;
	m_factory[NodeGMLPolygonPatch::get_static_element_name()] = NodeGMLPolygonPatch::create;
	m_factory[NodeGMLPatches::get_static_element_name()] = NodeGMLPatches::create;
	m_factory[NodeAIXMSurface::get_static_element_name()] = NodeAIXMSurface::create;
	m_factory[NodeAIXMHorizontalProjection::get_static_element_name()] = NodeAIXMHorizontalProjection::create;
	m_factory[NodeAIXMAirspaceVolumeDependency::get_static_element_name()] = NodeAIXMAirspaceVolumeDependency::create;
	m_factory[NodeAIXMContributorAirspace::get_static_element_name()] = NodeAIXMContributorAirspace::create;
	m_factory[NodeAIXMAirspaceVolume::get_static_element_name()] = NodeAIXMAirspaceVolume::create;
	m_factory[NodeAIXMTheAirspaceVolume::get_static_element_name()] = NodeAIXMTheAirspaceVolume::create;
	m_factory[NodeAIXMAirspaceGeometryComponent::get_static_element_name()] = NodeAIXMAirspaceGeometryComponent::create;
	m_factory[NodeAIXMGeometryComponent::get_static_element_name()] = NodeAIXMGeometryComponent::create;
	m_factory[NodeAIXMEnRouteSegmentPoint::get_static_element_name()] = NodeAIXMEnRouteSegmentPoint::create;
	m_factory[NodeAIXMTerminalSegmentPoint::get_static_element_name()] = NodeAIXMTerminalSegmentPoint::create;
	m_factory[NodeAIXMStart::get_static_element_name()] = NodeAIXMStart::create;
	m_factory[NodeAIXMEnd::get_static_element_name()] = NodeAIXMEnd::create;
	m_factory[NodeAIXMStartPoint::get_static_element_name()] = NodeAIXMStartPoint::create;
	m_factory[NodeAIXMEndPoint::get_static_element_name()] = NodeAIXMEndPoint::create;
	m_factory[NodeADRConnectingPoint::get_static_element_name()] = NodeADRConnectingPoint::create;
	m_factory[NodeADRInitialApproachFix::get_static_element_name()] = NodeADRInitialApproachFix::create;
	m_factory[NodeAIXMAirspaceLayer::get_static_element_name()] = NodeAIXMAirspaceLayer::create;
	m_factory[NodeAIXMLevels::get_static_element_name()] = NodeAIXMLevels::create;
	m_factory[NodeAIXMRouteAvailability::get_static_element_name()] = NodeAIXMRouteAvailability::create;
	m_factory[NodeAIXMProcedureAvailability::get_static_element_name()] = NodeAIXMProcedureAvailability::create;
	m_factory[NodeAIXMAvailability::get_static_element_name()] = NodeAIXMAvailability::create;
	m_factory[NodeAIXMClientRoute::get_static_element_name()] = NodeAIXMClientRoute::create;
	m_factory[NodeAIXMRoutePortion::get_static_element_name()] = NodeAIXMRoutePortion::create;
	m_factory[NodeAIXMDirectFlightClass::get_static_element_name()] = NodeAIXMDirectFlightClass::create;
	m_factory[NodeAIXMFlightConditionCircumstance::get_static_element_name()] = NodeAIXMFlightConditionCircumstance::create;
	m_factory[NodeAIXMOperationalCondition::get_static_element_name()] = NodeAIXMOperationalCondition::create;
	m_factory[NodeAIXMFlightRestrictionLevel::get_static_element_name()] = NodeAIXMFlightRestrictionLevel::create;
	m_factory[NodeAIXMFlightLevel::get_static_element_name()] = NodeAIXMFlightLevel::create;
	m_factory[NodeADRFlightConditionCombinationExtension::get_static_element_name()] = NodeADRFlightConditionCombinationExtension::create;
	m_factory[NodeAIXMDirectFlightSegment::get_static_element_name()] = NodeAIXMDirectFlightSegment::create;
	m_factory[NodeAIXMElementRoutePortionElement::get_static_element_name()] = NodeAIXMElementRoutePortionElement::create;
	m_factory[NodeAIXMElementDirectFlightElement::get_static_element_name()] = NodeAIXMElementDirectFlightElement::create;
	m_factory[NodeAIXMFlightRoutingElement::get_static_element_name()] = NodeAIXMFlightRoutingElement::create;
	m_factory[NodeAIXMRouteElement::get_static_element_name()] = NodeAIXMRouteElement::create;
	m_factory[NodeAIXMFlightRestrictionRoute::get_static_element_name()] = NodeAIXMFlightRestrictionRoute::create;
	m_factory[NodeAIXMRegulatedRoute::get_static_element_name()] = NodeAIXMRegulatedRoute::create;
	m_factory[NodeAIXMAircraftCharacteristic::get_static_element_name()] = NodeAIXMAircraftCharacteristic::create;
	m_factory[NodeAIXMFlightConditionAircraft::get_static_element_name()] = NodeAIXMFlightConditionAircraft::create;
	m_factory[NodeAIXMFlightCharacteristic::get_static_element_name()] = NodeAIXMFlightCharacteristic::create;
	m_factory[NodeAIXMFlightConditionFlight::get_static_element_name()] = NodeAIXMFlightConditionFlight::create;
	m_factory[NodeAIXMFlightConditionOperand::get_static_element_name()] = NodeAIXMFlightConditionOperand::create;
	m_factory[NodeAIXMFlightConditionRoutePortionCondition::get_static_element_name()] = NodeAIXMFlightConditionRoutePortionCondition::create;
	m_factory[NodeAIXMFlightConditionDirectFlightCondition::get_static_element_name()] = NodeAIXMFlightConditionDirectFlightCondition::create;
	m_factory[NodeADRAirspaceBorderCrossingObject::get_static_element_name()] = NodeADRAirspaceBorderCrossingObject::create;
	m_factory[NodeADRBorderCrossingCondition::get_static_element_name()] = NodeADRBorderCrossingCondition::create;
	m_factory[NodeADRFlightConditionElementExtension::get_static_element_name()] = NodeADRFlightConditionElementExtension::create;
	m_factory[NodeAIXMFlightConditionElement::get_static_element_name()] = NodeAIXMFlightConditionElement::create;
	m_factory[NodeAIXMElement::get_static_element_name()] = NodeAIXMElement::create;
	m_factory[NodeAIXMFlightConditionCombination::get_static_element_name()] = NodeAIXMFlightConditionCombination::create;
	m_factory[NodeAIXMFlight::get_static_element_name()] = NodeAIXMFlight::create;
	m_factory[NodeADRFlightRestrictionExtension::get_static_element_name()] = NodeADRFlightRestrictionExtension::create;
	m_factory[NodeAIXMAirportHeliportTimeSlice::get_static_element_name()] = NodeAIXMAirportHeliportTimeSlice::create;
	m_factory[NodeAIXMAirportHeliportCollocationTimeSlice::get_static_element_name()] = NodeAIXMAirportHeliportCollocationTimeSlice::create;
	m_factory[NodeAIXMDesignatedPointTimeSlice::get_static_element_name()] = NodeAIXMDesignatedPointTimeSlice::create;
	m_factory[NodeAIXMNavaidTimeSlice::get_static_element_name()] = NodeAIXMNavaidTimeSlice::create;
	m_factory[NodeAIXMAngleIndicationTimeSlice::get_static_element_name()] = NodeAIXMAngleIndicationTimeSlice::create;
	m_factory[NodeAIXMDistanceIndicationTimeSlice::get_static_element_name()] = NodeAIXMDistanceIndicationTimeSlice::create;
	m_factory[NodeAIXMAirspaceTimeSlice::get_static_element_name()] = NodeAIXMAirspaceTimeSlice::create;
	m_factory[NodeAIXMStandardLevelTableTimeSlice::get_static_element_name()] = NodeAIXMStandardLevelTableTimeSlice::create;
	m_factory[NodeAIXMStandardLevelColumnTimeSlice::get_static_element_name()] = NodeAIXMStandardLevelColumnTimeSlice::create;
	m_factory[NodeAIXMRouteTimeSlice::get_static_element_name()] = NodeAIXMRouteTimeSlice::create;
	m_factory[NodeAIXMRouteSegmentTimeSlice::get_static_element_name()] = NodeAIXMRouteSegmentTimeSlice::create;
	m_factory[NodeAIXMStandardInstrumentDepartureTimeSlice::get_static_element_name()] = NodeAIXMStandardInstrumentDepartureTimeSlice::create;
	m_factory[NodeAIXMStandardInstrumentArrivalTimeSlice::get_static_element_name()] = NodeAIXMStandardInstrumentArrivalTimeSlice::create;
	m_factory[NodeAIXMDepartureLegTimeSlice::get_static_element_name()] = NodeAIXMDepartureLegTimeSlice::create;
	m_factory[NodeAIXMArrivalLegTimeSlice::get_static_element_name()] = NodeAIXMArrivalLegTimeSlice::create;
	m_factory[NodeAIXMOrganisationAuthorityTimeSlice::get_static_element_name()] = NodeAIXMOrganisationAuthorityTimeSlice::create;
	m_factory[NodeAIXMSpecialDateTimeSlice::get_static_element_name()] = NodeAIXMSpecialDateTimeSlice::create;
	m_factory[NodeAIXMUnitTimeSlice::get_static_element_name()] = NodeAIXMUnitTimeSlice::create;
	m_factory[NodeAIXMAirTrafficManagementServiceTimeSlice::get_static_element_name()] = NodeAIXMAirTrafficManagementServiceTimeSlice::create;
	m_factory[NodeAIXMFlightRestrictionTimeSlice::get_static_element_name()] = NodeAIXMFlightRestrictionTimeSlice::create;
	m_factory[NodeAIXMTimeSlice::get_static_element_name()] = NodeAIXMTimeSlice::create;
	m_factory[NodeAIXMAirportHeliport::get_static_element_name()] = NodeAIXMAirportHeliport::create;
	m_factory[NodeAIXMAirportHeliportCollocation::get_static_element_name()] = NodeAIXMAirportHeliportCollocation::create;
	m_factory[NodeAIXMDesignatedPoint::get_static_element_name()] = NodeAIXMDesignatedPoint::create;
	m_factory[NodeAIXMNavaid::get_static_element_name()] = NodeAIXMNavaid::create;
	m_factory[NodeAIXMAngleIndication::get_static_element_name()] = NodeAIXMAngleIndication::create;
	m_factory[NodeAIXMDistanceIndication::get_static_element_name()] = NodeAIXMDistanceIndication::create;
	m_factory[NodeAIXMAirspace::get_static_element_name()] = NodeAIXMAirspace::create;
	m_factory[NodeAIXMStandardLevelTable::get_static_element_name()] = NodeAIXMStandardLevelTable::create;
	m_factory[NodeAIXMStandardLevelColumn::get_static_element_name()] = NodeAIXMStandardLevelColumn::create;
	m_factory[NodeAIXMRoute::get_static_element_name()] = NodeAIXMRoute::create;
	m_factory[NodeAIXMRouteSegment::get_static_element_name()] = NodeAIXMRouteSegment::create;
	m_factory[NodeAIXMStandardInstrumentArrival::get_static_element_name()] = NodeAIXMStandardInstrumentArrival::create;
	m_factory[NodeAIXMStandardInstrumentDeparture::get_static_element_name()] = NodeAIXMStandardInstrumentDeparture::create;
	m_factory[NodeAIXMDepartureLeg::get_static_element_name()] = NodeAIXMDepartureLeg::create;
	m_factory[NodeAIXMArrivalLeg::get_static_element_name()] = NodeAIXMArrivalLeg::create;
	m_factory[NodeAIXMOrganisationAuthority::get_static_element_name()] = NodeAIXMOrganisationAuthority::create;
	m_factory[NodeAIXMSpecialDate::get_static_element_name()] = NodeAIXMSpecialDate::create;
	m_factory[NodeAIXMUnit::get_static_element_name()] = NodeAIXMUnit::create;
	m_factory[NodeAIXMAirTrafficManagementService::get_static_element_name()] = NodeAIXMAirTrafficManagementService::create;
	m_factory[NodeAIXMFlightRestriction::get_static_element_name()] = NodeAIXMFlightRestriction::create;
	// shorthand namespace table
	m_namespaceshort["http://www.eurocontrol.int/cfmu/b2b/ADRMessage"] = "adrmsg";
	m_namespaceshort["http://www.aixm.aero/schema/5.1/extensions/ADR"] = "adr";
	m_namespaceshort["http://www.opengis.net/gml/3.2"] = "gml";
	m_namespaceshort["http://www.aixm.aero/schema/5.1"] = "aixm";
	m_namespaceshort["http://www.w3.org/1999/xlink"] = "xlink";
}

ParseXML::~ParseXML()
{
}

void ParseXML::error(const std::string& text)
{
	++m_errorcnt;
	std::cerr << "E: " << text << std::endl;
}

void ParseXML::error(const std::string& child, const std::string& text)
{
	++m_errorcnt;
	std::cerr << "E: " << child << ": " << text << std::endl;
}

void ParseXML::warning(const std::string& text)
{
	++m_warncnt;
	std::cerr << "W: " << text << std::endl;
}

void ParseXML::warning(const std::string& child, const std::string& text)
{
	++m_warncnt;
	std::cerr << "W: " << child << ": " << text << std::endl;
}

void ParseXML::info(const std::string& text)
{
	std::cerr << "I: " << text << std::endl;
}

void ParseXML::info(const std::string& child, const std::string& text)
{
	std::cerr << "I: " << child << ": " << text << std::endl;
}

void ParseXML::on_start_document(void)
{
	m_parsestack.clear();
}

void ParseXML::on_end_document(void)
{
	if (!m_parsestack.empty()) {
		if (true)
			std::cerr << "ADR Loader: parse stack not empty at end of document" << std::endl;
		error("parse stack not empty at end of document");
	}
}

void ParseXML::on_start_element(const Glib::ustring& name, const AttributeList& properties)
{
	std::string name1(name);
	// handle namespace
	for (AttributeList::const_iterator i(properties.begin()), e(properties.end()); i != e; ++i) {
		if (i->name.compare(0, 6, "xmlns:"))
			continue;
		std::string ns(i->name.substr(6));
		std::string val(i->value);
		std::pair<namespacemap_t::iterator,bool> ins(m_namespacemap.insert(std::make_pair(ns, val)));
		if (ins.second)
			continue;
		if (ins.first->second != val)
			warning("Redefining namespace " + ns + " to " + val);
		ins.first->second = val;
	}
	{
		std::string::size_type n(name1.find(':'));
		if (n != std::string::npos) {
			std::string ns(name1.substr(0, n));
			namespacemap_t::const_iterator i(m_namespacemap.find(ns));
			if (i == m_namespacemap.end()) {
				error("Namespace " + ns + " not found");
			} else {
				namespacemap_t::const_iterator j(m_namespaceshort.find(i->second));
				if (j == m_namespaceshort.end()) {
					error("Namespace " + i->second + " unknown");
				} else {
					name1 = j->second + name1.substr(n);
				}
			}
		}
	}
	AttributeList properties1(properties);
	for (AttributeList::iterator pi(properties1.begin()), pe(properties1.end()); pi != pe; ++pi) {
		std::string name1(pi->name);
		std::string::size_type n(name1.find(':'));
		if (n == std::string::npos)
			continue;
		std::string ns(name1.substr(0, n));
		if (ns == "xmlns")
			continue;
		namespacemap_t::const_iterator i(m_namespacemap.find(ns));
		if (i == m_namespacemap.end()) {
			error("Namespace " + ns + " not found");
			continue;
		}
		namespacemap_t::const_iterator j(m_namespaceshort.find(i->second));
		if (j == m_namespaceshort.end()) {
			error("Namespace " + i->second + " unknown");
			continue;
		}
		name1 = j->second + name1.substr(n);
		pi->name = name1;
	}
	unsigned int childnum(0);
	if (m_parsestack.empty()) {
		if (name1 == NodeADRMessage::get_static_element_name()) {
			if (tracestack)
				std::cerr << ">> " << name << " (" << NodeADRMessage::get_static_element_name() << ')' << std::endl;
			try {
				Node::ptr_t p(new NodeADRMessage(*this, name, childnum, properties1));
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create ADRMessage node: ") + e.what());
				throw;
			}
			return;
		}
		error("document must begin with " + NodeADRMessage::get_static_element_name());
	}
	childnum = m_parsestack.back()->get_childcount();
	if (m_parsestack.back()->is_ignore()) {
		if (tracestack)
			std::cerr << ">> ignore: " << name << " (" << name1 << ')' << std::endl;
		try {
			Node::ptr_t p(new NodeIgnore(*this, name, childnum, properties1, name1));
			m_parsestack.push_back(p);
		} catch (const std::runtime_error& e) {
			error(std::string("Cannot create Ignore node: ") + e.what());
			throw;
		}
		return;
	}
	// Node factory
	{
		NodeText::texttype_t tt(NodeText::find_type(name1));
		if (tt != NodeText::tt_invalid) {
			if (tracestack)
				std::cerr << ">> " << name << " (" << NodeText::get_static_element_name(tt) << ')' << std::endl;
			try {
				Node::ptr_t p(new NodeText(*this, name, childnum, properties1, tt));
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create Text ") + NodeText::get_static_element_name(tt) + " node: " + e.what());
				throw;
			}
			return;
		}
	}
	{
		NodeLink::linktype_t lt(NodeLink::find_type(name1));
		if (lt != NodeLink::lt_invalid) {
			if (tracestack)
				std::cerr << ">> " << name << " (" << NodeLink::get_static_element_name(lt) << ')' << std::endl;
			try {
				Node::ptr_t p(new NodeLink(*this, name, childnum, properties1, lt));
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create Link ") + NodeLink::get_static_element_name(lt) + " node: " + e.what());
				throw;
			}
			return;
		}
	}
	{
		factory_t::const_iterator fi(m_factory.find(name1));
		if (fi != m_factory.end()) {
			Node::ptr_t p;
			try {
				p = (fi->second)(*this, name, childnum, properties1);
				m_parsestack.push_back(p);
			} catch (const std::runtime_error& e) {
				error(std::string("Cannot create ") + name1 + " node: " + e.what());
				throw;
			}
			if (tracestack)
				std::cerr << ">> " << name << " (" << p->get_element_name() << ')' << std::endl;
			return;
		}
	}
	// finally: ignoring element
	{
		if (tracestack)
			std::cerr << ">> unknown: " << name << std::endl;
		try {
			Node::ptr_t p(new NodeIgnore(*this, name, childnum, properties1, name1));
			m_parsestack.push_back(p);
		} catch (const std::runtime_error& e) {
			error(std::string("Cannot create ADRMessage node: ") + e.what());
			throw;
		}
		return;
	}
}

void ParseXML::on_end_element(const Glib::ustring& name)
{
	if (m_parsestack.empty())
		error("parse stack underflow on end element");
	Node::ptr_t p(m_parsestack.back());
	m_parsestack.pop_back();
	if (tracestack)
		std::cerr << "<< " << p->get_element_name() << std::endl;
	if (p->get_tagname() != (std::string)name)
		error("end element name " + name + " does not match expected name " + p->get_tagname());
	if (!m_parsestack.empty()) {
		try {
			m_parsestack.back()->on_subelement(p);
		} catch (const std::runtime_error& e) {
			error(std::string("Error in on_subelement method of ") + m_parsestack.back()->get_element_name() + ": " + e.what());
			throw;
		}
		return;
	}
	{
		NodeADRMessage::ptr_t px(NodeADRMessage::ptr_t::cast_dynamic(p));
		if (!px)
			error("top node is not an " + NodeADRMessage::get_static_element_name());
	}
}

void ParseXML::on_characters(const Glib::ustring& characters)
{
	if (m_parsestack.empty())
		return;
	try {
		m_parsestack.back()->on_characters(characters);
	} catch (const std::runtime_error& e) {
		error(std::string("Error in on_characters method of ") + m_parsestack.back()->get_element_name() + ": " + e.what());
		throw;
	}
}

void ParseXML::on_comment(const Glib::ustring& text)
{
}

void ParseXML::on_warning(const Glib::ustring& text)
{
	warning("Parser: Warning: " + text);
}

void ParseXML::on_error(const Glib::ustring& text)
{
	warning("Parser: Error: " + text);
}

void ParseXML::on_fatal_error(const Glib::ustring& text)
{
	error("Parser: Fatal Error: " + text);
}
