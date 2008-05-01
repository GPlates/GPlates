/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "GpmlOnePointSixOutputVisitor.h"

#include <vector>
#include <utility>

#include <boost/bind.hpp>
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/XmlNamespaces.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/TemplateTypeParameterType.h"
#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"


namespace
{
	typedef std::pair<
		GPlatesModel::XmlAttributeName, 
		GPlatesModel::XmlAttributeValue> 
			XmlAttribute;

	void
	writeTemplateTypeParameterType(
			GPlatesFileIO::XmlWriter &writer,
			const GPlatesPropertyValues::TemplateTypeParameterType &value_type)
	{
		boost::optional<const UnicodeString &> alias = 
			value_type.get_namespace_alias();

		UnicodeString prefix;

		if (alias) 
		{
			// XXX:  This namespace declaration is a hack to get around the
			// fact that we can't access the current namespace declarations
			// from QXmlStreamWriter.  It ensures that the namespace of the 
			// TemplateTypeParameterType about to be written has been
			// declared.
			writer.writeNamespace(
					GPlatesUtils::make_qstring_from_icu_string(value_type.get_namespace()),
					GPlatesUtils::make_qstring_from_icu_string(*alias));

			prefix = *alias;
		}
		else 
		{
			prefix = writer.getAliasForNamespace(value_type.get_namespace_iterator());
		}

		writer.writeText(prefix + ":" + value_type.get_name());
	} 
}


GPlatesFileIO::GpmlOnePointSixOutputVisitor::GpmlOnePointSixOutputVisitor(
		QIODevice *target):
	d_output(target)
{
	d_output.writeStartDocument();

	d_output.writeNamespace(
			GPlatesUtils::XmlNamespaces::GPML_NAMESPACE, 
			GPlatesUtils::XmlNamespaces::GPML_STANDARD_ALIAS);
	d_output.writeNamespace(
			GPlatesUtils::XmlNamespaces::GML_NAMESPACE, 
			GPlatesUtils::XmlNamespaces::GML_STANDARD_ALIAS);
	d_output.writeNamespace(
			GPlatesUtils::XmlNamespaces::XSI_NAMESPACE, 
			GPlatesUtils::XmlNamespaces::XSI_STANDARD_ALIAS);

	d_output.writeStartGpmlElement("FeatureCollection");

	d_output.writeGpmlAttribute("version", "1.6");
	d_output.writeAttribute(
			GPlatesUtils::XmlNamespaces::XSI_NAMESPACE,
			"schemaLocation",
			"http://www.gplates.org/gplates ../xsd/gpml.xsd "\
			"http://www.opengis.net/gml ../../../gml/current/base");

}


GPlatesFileIO::GpmlOnePointSixOutputVisitor::~GpmlOnePointSixOutputVisitor()
{
	d_output.writeEndElement(); // </gpml:FeatureCollection>
	d_output.writeEndDocument();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Every feature must be wrapped in a "gml:featureMember" element.
	d_output.writeStartGmlElement("featureMember");
		bool pop = d_output.writeStartElement(feature_handle.feature_type());

			d_output.writeStartGpmlElement("identity");
				d_output.writeText(feature_handle.feature_id().get());
			d_output.writeEndElement();

			d_output.writeStartGpmlElement("revision");
				d_output.writeText(feature_handle.revision_id().get());
			d_output.writeEndElement();

			// Now visit each of the properties in turn.
			visit_feature_properties(feature_handle);

		d_output.writeEndElement(pop); // </gpml:SomeFeature>
	d_output.writeEndElement(); // </gml:featureMember>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	d_last_property_seen = inline_property_container.property_name();

	bool pop = d_output.writeStartElement(inline_property_container.property_name());
		d_output.writeAttributes(
			inline_property_container.xml_attributes().begin(),
			inline_property_container.xml_attributes().end());

		visit_property_values(inline_property_container);
	d_output.writeEndElement(pop);
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
	d_output.writeText(enumeration.value().get());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_output.writeStartGmlElement("LineString");

	static std::vector<std::pair<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> > 
		pos_list_xml_attrs;

	if (pos_list_xml_attrs.empty()) 
	{
		GPlatesModel::XmlAttributeName attr_name =
			GPlatesModel::XmlAttributeName::create_gml("dimension");
		GPlatesModel::XmlAttributeValue attr_value("2");
		pos_list_xml_attrs.push_back(std::make_pair(attr_name, attr_value));
	}
	d_output.writeStartGmlElement("posList");
	d_output.writeAttributes(
			pos_list_xml_attrs.begin(),
			pos_list_xml_attrs.end());

	// It would be slightly "nicer" (ie, avoiding the allocation of a temporary buffer) if we
	// were to create an iterator which performed the following transformation for us
	// automatically, but (i) that's probably not the most efficient use of our time right now;
	// (ii) it's file I/O, it's slow anyway; and (iii) we can cut it down to a single memory
	// allocation if we reserve the size of the vector in advance.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
			gml_line_string.polyline();
	std::vector<double> pos_list;
	// Reserve enough space for the coordinates, to avoid the need to reallocate.
	//
	// number of coords = 
	//   (one for each segment start-point, plus one for the final end-point
	//   (all other end-points are the start-point of the next segment, so are not counted)),
	//   times two, since each point is a (lat, lon) duple.
	pos_list.reserve((polyline_ptr->number_of_segments() + 1) * 2);

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline_ptr->vertex_begin();
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_ptr->vertex_end();
	for ( ; iter != end; ++iter) 
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);

		pos_list.push_back(llp.longitude());
		pos_list.push_back(llp.latitude());
	}
	d_output.writeNumericalSequence(pos_list.begin(), pos_list.end());

	// Don't forget to clear the vector when we're done with it!
	pos_list.clear();

	d_output.writeEndElement(); // </gml:posList>
	d_output.writeEndElement(); // </gml:LineString>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve) 
{
	d_output.writeStartGmlElement("OrientableCurve");
		d_output.writeAttributes(
				gml_orientable_curve.xml_attributes().begin(),
				gml_orientable_curve.xml_attributes().end());

		d_output.writeStartGmlElement("baseCurve");
			gml_orientable_curve.base_curve()->accept_visitor(*this);
		d_output.writeEndElement();  // </gml:base_curve>

	d_output.writeEndElement();  // </gml:OrientableCurve>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point) 
{
	d_output.writeStartGmlElement("Point");
		d_output.writeStartGmlElement("pos");

			const GPlatesMaths::PointOnSphere &pos = *gml_point.point();
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos);
			d_output.writeDecimalPair(llp.longitude(), llp.latitude());

		d_output.writeEndElement();  // </gml:pos>
	d_output.writeEndElement();  // </gml:Point>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant) 
{
	d_output.writeStartGmlElement("TimeInstant");
	d_output.writeStartGmlElement("timePosition");
		d_output.writeAttributes(
				gml_time_instant.time_position_xml_attributes().begin(),
				gml_time_instant.time_position_xml_attributes().end());

		const GPlatesPropertyValues::GeoTimeInstant &time_position = 
			gml_time_instant.time_position();
		if (time_position.is_real()) {
			d_output.writeDecimal(time_position.value());
		} else if (time_position.is_distant_past()) {
			d_output.writeText(QString("http://gplates.org/times/distantPast"));
		} else if (time_position.is_distant_future()) {
			d_output.writeText(QString("http://gplates.org/times/distantFuture"));
		}

	d_output.writeEndElement();  // </gml:timePosition>
	d_output.writeEndElement();  // </gml:TimeInstant>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period) 
{
	d_output.writeStartGmlElement("TimePeriod");
		d_output.writeStartGmlElement("begin");
			gml_time_period.begin()->accept_visitor(*this);
		d_output.writeEndElement();

		d_output.writeStartGmlElement("end");
			gml_time_period.end()->accept_visitor(*this);
		d_output.writeEndElement();
	d_output.writeEndElement(); // </gml:TimePeriod>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_polarity_chron_id(
		const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id) 
{
	d_output.writeStartGpmlElement("PolarityChronId");
	if (gpml_polarity_chron_id.get_era()) {
		d_output.writeStartGpmlElement("era");
		d_output.writeText(*gpml_polarity_chron_id.get_era());
		d_output.writeEndElement();
	}
	if (gpml_polarity_chron_id.get_major_region()) {
		d_output.writeStartGpmlElement("major");
		d_output.writeInteger(*gpml_polarity_chron_id.get_major_region());
		d_output.writeEndElement();
	}
	if (gpml_polarity_chron_id.get_minor_region()) {
		d_output.writeStartGpmlElement("minor");
		d_output.writeText(*gpml_polarity_chron_id.get_minor_region());
		d_output.writeEndElement();
	}
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	d_output.writeStartGpmlElement("ConstantValue");
		d_output.writeStartGpmlElement("value");
			gpml_constant_value.value()->accept_visitor(*this);
		d_output.writeEndElement();


		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_constant_value.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_feature_reference(
		const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference)
{
	d_output.writeStartGpmlElement("FeatureReference");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_feature_reference.feature_id().get());
		d_output.writeEndElement();


		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_feature_reference.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation) 
{
	if (gpml_finite_rotation.is_zero_rotation()) {
		d_output.writeEmptyGpmlElement("ZeroFiniteRotation");
	} else {
		d_output.writeStartGpmlElement("AxisAngleFiniteRotation");

			d_output.writeStartGpmlElement("eulerPole");
				GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
						GPlatesPropertyValues::calculate_euler_pole(gpml_finite_rotation);
				visit_gml_point(*gml_point);
			d_output.writeEndElement();

			d_output.writeStartGmlElement("angle");
				GPlatesMaths::real_t angle_in_radians =
						GPlatesPropertyValues::calculate_angle(gpml_finite_rotation);
				double angle_in_degrees =
						GPlatesMaths::degreesToRadians(angle_in_radians).dval();
				d_output.writeDecimal(angle_in_degrees);
			d_output.writeEndElement();

		d_output.writeEndElement();  // </gpml:AxisAngleFiniteRotation>
	}
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_finite_rotation_slerp(
		const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{
	d_output.writeStartGpmlElement("FiniteRotationSlerp");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_finite_rotation_slerp.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	d_output.writeStartGpmlElement("PiecewiseAggregation");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_piecewise_aggregation.value_type());
		d_output.writeEndElement();

		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
				gpml_piecewise_aggregation.time_windows().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
				gpml_piecewise_aggregation.time_windows().end();
		for ( ; iter != end; ++iter) 
		{
			d_output.writeStartGpmlElement("timeWindow");
				iter->accept_visitor(*this);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();  // </gpml:IrregularSampling>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_hot_spot_trail_mark(
		const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark)
{
	d_output.writeStartGpmlElement("HotSpotTrailMark");
		d_output.writeStartGpmlElement("position");
			gpml_hot_spot_trail_mark.position()->accept_visitor(*this);
		d_output.writeEndElement();

		if (gpml_hot_spot_trail_mark.trail_width()) {
			d_output.writeStartGpmlElement("trailWidth");
				(*gpml_hot_spot_trail_mark.trail_width())->accept_visitor(*this);
			d_output.writeEndElement();
		}
		if (gpml_hot_spot_trail_mark.measured_age()) {
			d_output.writeStartGpmlElement("measuredAge");
				(*gpml_hot_spot_trail_mark.measured_age())->accept_visitor(*this);
			d_output.writeEndElement();
		}
		if (gpml_hot_spot_trail_mark.measured_age_range()) {
			d_output.writeStartGpmlElement("measuredAgeRange");
				(*gpml_hot_spot_trail_mark.measured_age_range())->accept_visitor(*this);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();  // </gpml:HotSpotTrailMark>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_measure(
		const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	d_output.writeAttributes(
			gpml_measure.quantity_xml_attributes().begin(),
			gpml_measure.quantity_xml_attributes().end());
	d_output.writeDecimal(gpml_measure.quantity());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_time_window(
		const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	d_output.writeStartGpmlElement("TimeWindow");
		d_output.writeStartGpmlElement("timeDependentPropertyValue");
			gpml_time_window.time_dependent_value()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("validTime");
			gpml_time_window.valid_time()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_time_window.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement(); // </gpml:TimeWindow>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	d_output.writeStartGpmlElement("IrregularSampling");
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter =
				gpml_irregular_sampling.time_samples().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator end =
				gpml_irregular_sampling.time_samples().end();
		for ( ; iter != end; ++iter) 
		{
			d_output.writeStartGpmlElement("timeSample");
				iter->accept_visitor(*this);
			d_output.writeEndElement();
		}

		// The interpolation function is optional.
		if (gpml_irregular_sampling.interpolation_function() != NULL)
		{
			d_output.writeStartGpmlElement("interpolationFunction");
				gpml_irregular_sampling.interpolation_function()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_irregular_sampling.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();  // </gpml:IrregularSampling>
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_output.writeInteger(gpml_plate_id.value());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_revision_id(
		const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id)
{
	d_output.writeText(gpml_revision_id.value().get());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample) 
{
	d_output.writeStartGpmlElement("TimeSample");
		d_output.writeStartGpmlElement("value");
			gpml_time_sample.value()->accept_visitor(*this);
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("validTime");
			gpml_time_sample.valid_time()->accept_visitor(*this);
		d_output.writeEndElement();

		// The description is optional.
		if (gpml_time_sample.description() != NULL) 
		{
			d_output.writeStartGmlElement("description");
				gpml_time_sample.description()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_time_sample.value_type());
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:TimeSample>
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	d_output.writeStartGpmlElement("OldPlatesHeader");

		d_output.writeStartGpmlElement("regionNumber");
 		d_output.writeInteger(gpml_old_plates_header.region_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referenceNumber");
 		d_output.writeInteger(gpml_old_plates_header.reference_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("stringNumber");
 		d_output.writeInteger(gpml_old_plates_header.string_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("geographicDescription");
 		d_output.writeText(gpml_old_plates_header.geographic_description());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("plateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfAppearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_appearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfDisappearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_disappearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCode");
 		d_output.writeText(gpml_old_plates_header.data_type_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumber");
 		d_output.writeInteger(gpml_old_plates_header.data_type_code_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumberAdditional");
 		d_output.writeText(gpml_old_plates_header.data_type_code_number_additional());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("conjugatePlateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.conjugate_plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("colourCode");
 		d_output.writeInteger(gpml_old_plates_header.colour_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("numberOfPoints");
 		d_output.writeInteger(gpml_old_plates_header.number_of_points());
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:OldPlatesHeader>
}

void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string) {
	d_output.writeText(xs_string.value().get());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_uninterpreted_property_value(
		const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val)
{
	// XXX: Uncomment to indicate which property values weren't interpreted.
	//d_output.get_writer().writeEmptyElement("Uninterpreted");
	const GPlatesModel::XmlElementNode::non_null_ptr_type elem = 
		uninterpreted_prop_val.value();

	for_each(elem->children_begin(), elem->children_end(),
			boost::bind(&GPlatesModel::XmlNode::write_to, _1, 
				boost::ref(d_output.get_writer())));
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	d_output.writeBoolean(xs_boolean.value());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_double(
		const GPlatesPropertyValues::XsDouble &xs_double)
{
	d_output.writeDecimal(xs_double.value());
}


void
GPlatesFileIO::GpmlOnePointSixOutputVisitor::visit_xs_integer(
		const GPlatesPropertyValues::XsInteger &xs_integer)
{
	d_output.writeInteger(xs_integer.value());
}
