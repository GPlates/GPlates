/* $Id$ */

/**
 * \file Various GMT format feature headers.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "GMTFormatHeader.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "utils/StringFormattingUtils.h"


bool
GPlatesFileIO::GMTFormatPlates4StyleHeader::get_feature_header_lines(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		std::vector<QString>& header_lines)
{
	GPlatesFileIO::OldPlatesHeader old_plates_header;

	if ( d_plates_header_visitor.get_old_plates_header(
		feature,
		old_plates_header) )
	{
		format_header_lines(old_plates_header, header_lines);
		return true;
	}

	return false;
}


void
GPlatesFileIO::GMTFormatPlates4StyleHeader::format_header_lines(
		const GPlatesFileIO::OldPlatesHeader& old_plates_header,
		std::vector<QString>& header_lines)
{
	header_lines.resize(2);

	QTextStream header_line_stream1(&header_lines[0]);
	QTextStream header_line_stream2(&header_lines[1]);

	using namespace GPlatesUtils;

	// First line of the PLATES4-style GMT header.
	header_line_stream1
		<< formatted_int_to_string(old_plates_header.region_number, 2).c_str()
		<< formatted_int_to_string(old_plates_header.reference_number, 2).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.string_number, 4).c_str()
		<< " "
		<< GPlatesUtils::make_qstring_from_icu_string(old_plates_header.geographic_description);

	// Second line of the PLATES4-style GMT header.
	header_line_stream2
		<< " "
		<< formatted_int_to_string(old_plates_header.plate_id_number, 3).c_str()
		<< " "
		<< formatted_double_to_string(old_plates_header.age_of_appearance, 6, 1).c_str()
		<< " "
		<< formatted_double_to_string(old_plates_header.age_of_disappearance, 6, 1).c_str()
		<< " "
		<< GPlatesUtils::make_qstring_from_icu_string(old_plates_header.data_type_code)
		<< formatted_int_to_string(old_plates_header.data_type_code_number, 4).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.conjugate_plate_id_number, 3).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.colour_code, 3).c_str()
		<< " "
		<< formatted_int_to_string(old_plates_header.number_of_points, 5).c_str();
}


GPlatesFileIO::GMTFormatVerboseHeader::GMTFormatVerboseHeader() :
	d_header_lines(NULL),
	d_nested_depth(0)
{
}

bool
GPlatesFileIO::GMTFormatVerboseHeader::get_feature_header_lines(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		std::vector<QString>& header_lines)
{
	d_header_lines = &header_lines;
	visit_feature(feature);
	d_header_lines = NULL;

	return true;
}

bool
GPlatesFileIO::GMTFormatVerboseHeader::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	start_header_line();

	d_line_stream
		<< GPlatesUtils::make_qstring_from_icu_string(
			feature_handle.handle_data().feature_type().get_name());

	d_line_stream
		<< " <identity>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			feature_handle.handle_data().feature_id().get())
		<< "</identity>";
	
	d_line_stream
		<< " <revision>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			feature_handle.revision_id().get())
		<< "</revision>";

	end_header_line();

	return true;
}

bool
GPlatesFileIO::GMTFormatVerboseHeader::initialise_pre_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	d_property_accumulator.clear();

	start_header_line();

	d_line_stream
		<< ' '
		<< GPlatesUtils::make_qstring_from_icu_string(
			top_level_property_inline.property_name().get_name());

	format_attributes(top_level_property_inline.xml_attributes());

	return true;
}

void
GPlatesFileIO::GMTFormatVerboseHeader::finalise_post_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	// If the current property is the reconstruction plate id then simplify the printing
	// of it so it's not too hard to parse with awk/sed.
	if (d_property_accumulator.is_reconstruction_plate_id_property(
			top_level_property_inline.property_name()))
	{
		// Clear what we've written so far for the current property.
		clear_header_line();

		// Write it out again but simpler.
		d_line_stream << " reconstructionPlateId ";
		if (d_property_accumulator.get_plate_id())
		{
			d_line_stream << *d_property_accumulator.get_plate_id();
		}
	}

	// Only output the header line if the current property is not geometry.
	// Because the geometry is not part of the header - it gets written out separately.
	const bool output_header_line = !d_property_accumulator.get_is_geometry_property();
	end_header_line(output_header_line);
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
	start_header_line();

	d_line_stream << GPlatesUtils::make_qstring_from_icu_string(enumeration.value().get());

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &)
{
	d_property_accumulator.set_is_geometry_property();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &)
{
	d_property_accumulator.set_is_geometry_property();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &)
{
	d_property_accumulator.set_is_geometry_property();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &)
{
	d_property_accumulator.set_is_geometry_property();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	start_header_line();

	d_line_stream << " TimeInstant";

	//format_attributes(gml_time_instant.time_position_xml_attributes());

	d_line_stream << " <timePosition>";

	const GPlatesPropertyValues::GeoTimeInstant &time_position = 
		gml_time_instant.time_position();
	if (time_position.is_real()) {
		d_line_stream << time_position.value();
	} else if (time_position.is_distant_past()) {
		d_line_stream << "http://gplates.org/times/distantPast";
	} else if (time_position.is_distant_future()) {
		d_line_stream << "http://gplates.org/times/distantFuture";
	}

	d_line_stream << "</timePosition>";

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	start_header_line();

	d_line_stream << " TimePeriod";

	d_line_stream << " <begin>";
	gml_time_period.begin()->accept_visitor(*this);
	d_line_stream << "</begin>";

	d_line_stream << " <end>";
	gml_time_period.end()->accept_visitor(*this);
	d_line_stream << "</end>";

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_polarity_chron_id(
		const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id)
{
	start_header_line();

	d_line_stream << " PolarityChronId";

	if (gpml_polarity_chron_id.get_era()) {
		d_line_stream << " <era>" << *gpml_polarity_chron_id.get_era() << "</era>";
	}
	if (gpml_polarity_chron_id.get_major_region()) {
		d_line_stream << " <major>" << *gpml_polarity_chron_id.get_major_region() << "</major>";
	}
	if (gpml_polarity_chron_id.get_minor_region()) {
		d_line_stream << " <minor>" << *gpml_polarity_chron_id.get_minor_region() << "</minor>";
	}

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	start_header_line();

	d_line_stream << " ConstantValue";

	d_line_stream << " <value>";
	gpml_constant_value.value()->accept_visitor(*this);
	d_line_stream << "</value>";
	
	d_line_stream
		<< " <valueType>"
		<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_constant_value.value_type().get_name())
		<< "</valueType>";

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_feature_reference(
		const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference)
{
	start_header_line();

	d_line_stream << " FeatureReference";

	d_line_stream
		<< " <targetFeature>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_feature_reference.feature_id().get())
		<< "</targetFeature>";
	
	d_line_stream
		<< " <valueType>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_feature_reference.value_type().get_name())
		<< "</valueType>";

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_feature_snapshot_reference(
		const GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference)
{
	start_header_line();

	d_line_stream << " FeatureSnapshotReference";

	d_line_stream
		<< " <targetFeature>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_feature_snapshot_reference.feature_id().get())
		<< "</targetFeature>";
	
	d_line_stream
		<< " <targetRevision>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_feature_snapshot_reference.revision_id().get())
		<< "</targetRevision>";
	
	d_line_stream
		<< " <valueType>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_feature_snapshot_reference.value_type().get_name())
		<< "</valueType>";

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_property_delegate(
		const GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate)
{
	start_header_line();

	d_line_stream << " PropertyDelegate";

	d_line_stream
		<< " <targetFeature>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_property_delegate.feature_id().get())
		<< "</targetFeature>";
	
	d_line_stream
		<< " <targetProperty>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_property_delegate.target_property().get_name())
		<< "</targetProperty>";
	
	d_line_stream
		<< " <valueType>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_property_delegate.value_type().get_name())
		<< "</valueType>";

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	start_header_line();

	d_line_stream << " KeyValueDictionary";

	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
		iter = gpml_key_value_dictionary.elements().begin(),
		end = gpml_key_value_dictionary.elements().end();
	for ( ; iter != end; ++iter)
	{
		write_gpml_key_value_dictionary_element(*iter);
	}

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	start_header_line();

	d_line_stream << " PiecewiseAggregation";

	d_line_stream
		<< " <valueType>"
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_piecewise_aggregation.value_type().get_name())
		<< "</valueType>";

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
		gpml_piecewise_aggregation.time_windows().begin();
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
		gpml_piecewise_aggregation.time_windows().end();
	for ( ; iter != end; ++iter) 
	{
		d_line_stream << " <timeWindow>";
		write_gpml_time_window(*iter);
		d_line_stream << "</timeWindow>";
	}

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_hot_spot_trail_mark(
		const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark)
{
	start_header_line();

	d_line_stream << " HotSpotTrailMark";

	d_line_stream << " <position>";
	gpml_hot_spot_trail_mark.position()->accept_visitor(*this);
	d_line_stream << "</position>";

	if (gpml_hot_spot_trail_mark.trail_width()) {
		d_line_stream << " <trailWidth>";
		(*gpml_hot_spot_trail_mark.trail_width())->accept_visitor(*this);
		d_line_stream << "</trailWidth>";
	}
	if (gpml_hot_spot_trail_mark.measured_age()) {
		d_line_stream << " <measuredAge>";
		(*gpml_hot_spot_trail_mark.measured_age())->accept_visitor(*this);
		d_line_stream << "</measuredAge>";
	}
	if (gpml_hot_spot_trail_mark.measured_age_range()) {
		d_line_stream << " <measuredAgeRange>";
		(*gpml_hot_spot_trail_mark.measured_age_range())->accept_visitor(*this);
		d_line_stream << "</measuredAgeRange>";
	}

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_measure(
		const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	start_header_line();

	format_attributes(gpml_measure.quantity_xml_attributes());

	d_line_stream << ' ' << gpml_measure.quantity();

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	start_header_line();

	d_line_stream << " OldPlatesHeader";

	d_line_stream
		<< ' '
		<< gpml_old_plates_header.region_number()
		<< gpml_old_plates_header.reference_number()
		<< "  "
		<< gpml_old_plates_header.string_number()
		<< ' '
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_old_plates_header.geographic_description())
		<< ' '
		<< gpml_old_plates_header.plate_id_number()
		<< "   "
		<< gpml_old_plates_header.age_of_appearance()
		<< ' '
		<< gpml_old_plates_header.age_of_disappearance()
		<< ' '
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_old_plates_header.data_type_code())
		<< "   "
		<< gpml_old_plates_header.data_type_code_number()
		<< ' '
		<< gpml_old_plates_header.conjugate_plate_id_number()
		<< "   "
		<< gpml_old_plates_header.colour_code();

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::write_gpml_time_window(
		const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	d_line_stream << " TimeWindow";

	d_line_stream << " <timeDependentPropertyValue>";
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	d_line_stream << "</timeDependentPropertyValue>";
		
	d_line_stream << " <validTime>";
	gpml_time_window.valid_time()->accept_visitor(*this);
	d_line_stream << "</validTime>";

	d_line_stream << " <valueType>";
	d_line_stream << GPlatesUtils::make_qstring_from_icu_string(
			gpml_time_window.value_type().get_name());
	d_line_stream << "</valueType>";
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	start_header_line();

	d_line_stream << " IrregularSampling";

	std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter =
		gpml_irregular_sampling.time_samples().begin();
	std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator end =
		gpml_irregular_sampling.time_samples().end();
	for ( ; iter != end; ++iter) 
	{
		d_line_stream << " <timeSample>";
		write_gpml_time_sample(*iter);
		d_line_stream << "</timeSample>";
	}

	// The interpolation function is optional.
	if (gpml_irregular_sampling.interpolation_function() != NULL)
	{
		d_line_stream << " <interpolationFunction>";
		gpml_irregular_sampling.interpolation_function()->accept_visitor(*this);
		d_line_stream << "</interpolationFunction>";
	}

	d_line_stream << " <valueType>";
	d_line_stream << GPlatesUtils::make_qstring_from_icu_string(
			gpml_irregular_sampling.value_type().get_name());
	d_line_stream << "</valueType>";

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	start_header_line();

	d_line_stream
		<< ' '
		<< gpml_plate_id.value();

	end_header_line();

	// Also store the plate id in case we decide to rewrite in a simpler format.
	d_property_accumulator.set_plate_id(gpml_plate_id.value());
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_gpml_revision_id(
		const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id)
{
	start_header_line();

	d_line_stream
		<< ' '
		<< GPlatesUtils::make_qstring_from_icu_string(
			gpml_revision_id.value().get());

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::write_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
{
	d_line_stream << " TimeSample";

	d_line_stream << " <value>";
	gpml_time_sample.value()->accept_visitor(*this);
	d_line_stream << "</value>";

	d_line_stream << " <validTime>";
	gpml_time_sample.valid_time()->accept_visitor(*this);
	d_line_stream << "</validTime>";

	// The description is optional.
	if (gpml_time_sample.description() != NULL) 
	{
		d_line_stream << " <description>";
		gpml_time_sample.description()->accept_visitor(*this);
		d_line_stream << "</description>";
	}

	d_line_stream << " <valueType>";
	d_line_stream << GPlatesUtils::make_qstring_from_icu_string(
			gpml_time_sample.value_type().get_name());
	d_line_stream << "</valueType>";
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	start_header_line();

	d_line_stream
		<< ' '
		<< GPlatesUtils::make_qstring_from_icu_string(
			xs_string.value().get());

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	start_header_line();

	d_line_stream << ' ' << xs_boolean.value();

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_xs_double(
		const GPlatesPropertyValues::XsDouble &xs_double)
{
	start_header_line();

	d_line_stream << ' ' << xs_double.value();

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::visit_xs_integer(
		const GPlatesPropertyValues::XsInteger &xs_integer)
{
	start_header_line();

	d_line_stream << ' ' << xs_integer.value();

	end_header_line();
}

void
GPlatesFileIO::GMTFormatVerboseHeader::write_gpml_key_value_dictionary_element(
		const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
{
	d_line_stream << " (";
	element.key()->accept_visitor(*this);
	d_line_stream << ", ";
	element.value()->accept_visitor(*this);
	d_line_stream << ')';
}

void
GPlatesFileIO::GMTFormatVerboseHeader::format_attributes(
		const AttributeMap &attribute_map)
{
	if (!attribute_map.empty())
	{
		d_line_stream << ':';
	}

	for (AttributeMap::const_iterator attribute_map_iter = attribute_map.begin();
		attribute_map_iter != attribute_map.end();
		++attribute_map_iter)
	{
		const AttributeMap::value_type &attribute = *attribute_map_iter;

		d_line_stream
			<< " ("
			<< GPlatesUtils::make_qstring_from_icu_string(
				attribute.first.get_name())
			<< ", "
			<< GPlatesUtils::make_qstring_from_icu_string(
				attribute.second.get())
			<< ")";
	}
}

void
GPlatesFileIO::GMTFormatVerboseHeader::start_header_line()
{
	d_line_stream.setString(&d_current_line);
	++d_nested_depth;
}

void
GPlatesFileIO::GMTFormatVerboseHeader::end_header_line(
		bool output)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_nested_depth > 0,
			GPLATES_ASSERTION_SOURCE);
	if (--d_nested_depth == 0)
	{
		if (output)
		{
			d_header_lines->push_back(d_current_line);
		}
		d_current_line.clear();
	}
}

void
GPlatesFileIO::GMTFormatVerboseHeader::clear_header_line()
{
	d_current_line.clear();
}


bool
GPlatesFileIO::GMTFormatPreferPlates4StyleHeader::get_feature_header_lines(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		std::vector<QString>& header_lines)
{
	// See if has old plates header.
	d_has_old_plates_header = false;
	visit_feature(feature);

	return d_has_old_plates_header
		? d_plates4_style_header.get_feature_header_lines(feature, header_lines)
		: d_verbose_header.get_feature_header_lines(feature, header_lines);
}


void
GPlatesFileIO::GMTHeaderPrinter::print_global_header_lines(
		QTextStream& output_stream,
		std::vector<QString>& header_lines)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_is_first_feature_header_in_file,
			GPLATES_ASSERTION_SOURCE);

	// Print each line of the GMT header preceded by the '>' character.
	std::vector<QString>::const_iterator header_line_iter;
	for (header_line_iter = header_lines.begin();
		header_line_iter != header_lines.end();
		++header_line_iter)
	{
		const QString &line = *header_line_iter;
		output_stream << '>' << line << endl;
	}
}


void
GPlatesFileIO::GMTHeaderPrinter::print_feature_header_lines(
		QTextStream& output_stream,
		std::vector<QString>& header_lines)
{
	// The '>' symbol is used to terminate a list of points.
	// It's also used to start a header line.
	// If this is the first feature written to the file then
	// we don't have a '>' marker from the previous feature's list of points.
	if (d_is_first_feature_header_in_file)
	{
		output_stream << '>';
		d_is_first_feature_header_in_file = false;
	}

	if (header_lines.empty())
	{
		// There are no header lines to output so just output a newline and return.
		output_stream << endl;
		return;
	}

	bool first_line_in_header = true;

	// Print each line of the GMT header.
	std::vector<QString>::const_iterator header_line_iter;
	for (header_line_iter = header_lines.begin();
		header_line_iter != header_lines.end();
		++header_line_iter)
	{
		const QString &line = *header_line_iter;

		if (first_line_in_header)
		{
			// First line in header uses '>' marker written by previous geometry.
			output_stream << line << endl;
			first_line_in_header = false;
		}
		else
		{
			// 2nd, 3rd, etc lines in header write their own '>' marker.
			output_stream << '>' << line << endl;
		}
	}
}
