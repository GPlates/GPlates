/* $Id$ */

/**
 * \file 
 * Defines the interface for writing data in GMT xy format.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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


#include <ostream>
#include <fstream>
#include <vector>
#include <QTextStream>
#include <unicode/ustream.h>
#include <boost/foreach.hpp>

#include "GMTFormatWriter.h"
#include "GMTFormatGeometryExporter.h"
#include "PlatesLineFormatHeaderVisitor.h"
#include "ErrorOpeningFileForWritingException.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/TemplateTypeParameterType.h"
#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	//! Interface for formatting of feature header.
	class GMTHeaderFormatter
	{
	public:
		virtual
			~GMTHeaderFormatter()
		{  }

		/**
		 * Format feature into a sequence of header lines (returned as strings).
		 * @return @c true if there is enough information to print a header.
		 */
		virtual
			bool
			get_feature_header_lines(
			const GPlatesModel::FeatureHandle& feature_handle,
			std::vector<QString>& header_lines) = 0;
	};
}

namespace
{
	/**
	 * Formats a header using PLATES4 information if available.
	 * Otherwise formats a short header containing feature type and id.
	 */
	class Plates4StyleHeader :
		public GPlatesFileIO::GMTHeaderFormatter,
		private GPlatesModel::ConstFeatureVisitor
	{
	public:
		virtual
			bool
			get_feature_header_lines(
			const GPlatesModel::FeatureHandle& feature_handle,
			std::vector<QString>& header_lines);

	private:
		void
			format_header_lines(
			const GPlatesFileIO::OldPlatesHeader& old_plates_header,
			std::vector<QString>& header_lines);

		GPlatesFileIO::PlatesLineFormatHeaderVisitor d_plates_header_visitor;
	};

	/**
	 * Formats a header by printing out the feature's property values as strings.
	 */
	class VerboseHeader :
		public GPlatesFileIO::GMTHeaderFormatter,
		private GPlatesModel::ConstFeatureVisitor
	{
	public:
		VerboseHeader() :
			d_header_lines(NULL),
			d_nested_depth(0)
		  {  }

		virtual
			bool
			get_feature_header_lines(
			const GPlatesModel::FeatureHandle& feature_handle,
			std::vector<QString>& header_lines);

	private:
		typedef std::map<GPlatesModel::XmlAttributeName,
			GPlatesModel::XmlAttributeValue> AttributeMap;

		void
			visit_feature_handle(
			const GPlatesModel::FeatureHandle &feature_handle);

		void
			visit_inline_property_container(
			const GPlatesModel::InlinePropertyContainer &inline_property_container);

		void
			visit_enumeration(
			const GPlatesPropertyValues::Enumeration &enumeration);

		void
			visit_gml_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		void
			visit_gml_time_period(
			const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		void
			visit_gpml_polarity_chron_id(
			const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);

		void
			visit_gpml_constant_value(
			const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		void
			visit_gpml_feature_reference(
			const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference);

		void
			visit_gpml_feature_snapshot_reference(
			const GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference);

		void
			visit_gpml_property_delegate(
			const GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate);

		void
			visit_gpml_key_value_dictionary(
			const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

		void
			visit_gpml_piecewise_aggregation(
			const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);

		void
			visit_hot_spot_trail_mark(
			const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark);

		void
			visit_gpml_measure(
			const GPlatesPropertyValues::GpmlMeasure &gpml_measure);

		void
			visit_gpml_old_plates_header(
			const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		void
			write_gpml_time_window(
			const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);

		void
			visit_gpml_irregular_sampling(
			const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		void
			visit_gpml_plate_id(
			const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		void
			visit_gpml_revision_id(
			const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id);

		void
			write_gpml_time_sample(
			const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample);

		void
			visit_xs_string(
			const GPlatesPropertyValues::XsString &xs_string);

		void
			visit_xs_boolean(
			const GPlatesPropertyValues::XsBoolean &xs_boolean);

		void
			visit_xs_double(
			const GPlatesPropertyValues::XsDouble &xs_double);

		void
			visit_xs_integer(
			const GPlatesPropertyValues::XsInteger &xs_integer);

		void
			write_gpml_key_value_dictionary_element(
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element);

		void
			format_attributes(
			const AttributeMap &attribute_map);

		void
			startHeaderLine();

		void
			endHeaderLine();

		//! Output of @a get_feature_header_lines.
		std::vector<QString>* d_header_lines;

		//! Current header line being formatted (not used by all methods).
		QString d_current_line;

		//! Used to write to d_current_line.
		QTextStream d_line_stream;

		//! The depth of nesting of property values.
		int d_nested_depth;

		//! Keeps track of the last property that was read.
		boost::optional<GPlatesModel::PropertyName> d_last_property_seen;
	};

	/**
	 * Formats PLATES4 style header if feature has an old plates header property.
	 * Otherwise formats a verbose header.
	 */
	class PreferPlates4StyleHeader :
		public GPlatesFileIO::GMTHeaderFormatter,
		private GPlatesModel::ConstFeatureVisitor
	{
	public:
		virtual
			bool
			get_feature_header_lines(
			const GPlatesModel::FeatureHandle& feature_handle,
			std::vector<QString>& header_lines);

	private:
		virtual
			void
			visit_feature_handle(
			const GPlatesModel::FeatureHandle &feature_handle)
		{
			visit_feature_properties(feature_handle);
		}

		virtual
			void
			visit_inline_property_container(
			const GPlatesModel::InlinePropertyContainer &inline_property_container)
		{
			visit_property_values(inline_property_container);
		}

		virtual
			void
			visit_gpml_old_plates_header(
			const GPlatesPropertyValues::GpmlOldPlatesHeader &/*gpml_old_plates_header*/)
		{
			d_has_old_plates_header = true;
		}

		bool d_has_old_plates_header;
		Plates4StyleHeader d_plates4_style_header;
		VerboseHeader d_verbose_header;
	};

}


GPlatesFileIO::GMTFormatWriter::GMTFormatWriter(
	const FileInfo &file_info,
	HeaderFormat header_format)
{
	// Open the file.
	d_output_file.reset( new QFile(file_info.get_qfileinfo().filePath()) );
	if ( ! d_output_file->open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(
			file_info.get_qfileinfo().filePath());
	}

	d_output_stream.reset( new QTextStream(d_output_file.get()) );

	switch (header_format)
	{
	case PLATES4_STYLE_HEADER:
		d_feature_header.reset(new Plates4StyleHeader());
		break;

	case VERBOSE_HEADER:
		d_feature_header.reset(new VerboseHeader());
		break;

	case PREFER_PLATES4_STYLE_HEADER:
		d_feature_header.reset(new PreferPlates4StyleHeader());
		break;

	default:
		throw GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__);
	}
}


GPlatesFileIO::GMTFormatWriter::~GMTFormatWriter()
{
	// Empty destructor defined here so that 'd_feature_header' destructor
	// will be instantiated here where 'GMTHeaderFormatter' is a complete type .
}


void
GPlatesFileIO::GMTFormatWriter::write_feature(
	const GPlatesModel::FeatureHandle& feature_handle)
{
	// Clear accumulator before visiting feature.
	d_feature_accumulator.clear();

	// Collect any geometries in the current feature.
	feature_handle.accept_visitor(*this);

	std::vector<QString> header_lines;

	// Delegate formating of feature header.
	const bool valid_header = d_feature_header->get_feature_header_lines(
		feature_handle, header_lines);
	
	// If we have a valid header and at least one geometry then we can output for the current feature.
	if (valid_header && d_feature_accumulator.have_geometry())
	{
		// For each GeometryOnSphere write out a header and the geometry data.
		for(
			FeatureAccumulator::geometries_const_iterator_type geometry_iter =
				d_feature_accumulator.geometries_begin();
			geometry_iter != d_feature_accumulator.geometries_end();
			++geometry_iter)
		{
			// Write out the header.
			d_header_printer.print_header_lines(*d_output_stream, header_lines);

			// Write out the geometry.
			GMTFormatGeometryExporter geometry_exporter(*d_output_stream);
			geometry_exporter.export_geometry(*geometry_iter);
		}
	}
}


void
GPlatesFileIO::GMTFormatWriter::visit_feature_handle(
	const GPlatesModel::FeatureHandle &feature_handle)
{
	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFileIO::GMTFormatWriter::visit_inline_property_container(
	const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	visit_property_values(inline_property_container);
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_line_string(
	const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_feature_accumulator.add_geometry(gml_line_string.polyline());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_multi_point(
	const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_feature_accumulator.add_geometry(gml_multi_point.multipoint());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_orientable_curve(
	const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_point(
	const GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_feature_accumulator.add_geometry(gml_point.point());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gml_polygon(
	const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// FIXME: Handle interior rings. Requires a bit of restructuring.
	d_feature_accumulator.add_geometry(gml_polygon.exterior());
}


void
GPlatesFileIO::GMTFormatWriter::visit_gpml_constant_value(
	const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFileIO::GMTFormatWriter::HeaderPrinter::print_header_lines(
	QTextStream& output_stream,
	std::vector<QString>& header_lines)
{
	// The '>' symbol is used to terminate a list of points.
	// It's also used to start a header line.
	// If this is the first feature written to the file then
	// we don't have a '>' marker from the previous feature's list of points.
	if (d_is_first_header_in_file)
	{
		output_stream << '>';
		d_is_first_header_in_file = false;
	}

	bool first_line_in_header = true;

	// Print each line of the GMT header.
	BOOST_FOREACH(
		const QString& line,
		header_lines)
	{
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

namespace
{
	bool
	Plates4StyleHeader::get_feature_header_lines(
		 const GPlatesModel::FeatureHandle& feature_handle,
		 std::vector<QString>& header_lines)
	{
		GPlatesFileIO::OldPlatesHeader old_plates_header;

		if ( d_plates_header_visitor.get_old_plates_header(
			feature_handle,
			old_plates_header) )
		{
			format_header_lines(old_plates_header, header_lines);
			return true;
		}

		return false;
	}


	void
		Plates4StyleHeader::format_header_lines(
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


	bool
		VerboseHeader::get_feature_header_lines(
		const GPlatesModel::FeatureHandle& feature_handle,
		std::vector<QString>& header_lines)
	{
		d_header_lines = &header_lines;
		feature_handle.accept_visitor(*this);
		d_header_lines = NULL;

		return true;
	}

	void
		VerboseHeader::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
	{
		startHeaderLine();

		d_line_stream
			<< GPlatesUtils::make_qstring_from_icu_string(
				feature_handle.feature_type().get_name())
			<< " <identity>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				feature_handle.feature_id().get())
			<< " <revision>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				feature_handle.revision_id().get());

		endHeaderLine();

		// Now visit each of the properties in turn.
		visit_feature_properties(feature_handle);
	}

	void
		VerboseHeader::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
	{
		d_last_property_seen = inline_property_container.property_name();

		startHeaderLine();

		d_line_stream
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(
				inline_property_container.property_name().get_name());

		format_attributes(inline_property_container.xml_attributes());

		visit_property_values(inline_property_container);

		endHeaderLine();
	}

	void
		VerboseHeader::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
	{
		startHeaderLine();

		d_line_stream
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(enumeration.value().get());

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
	{
		startHeaderLine();

		//d_line_stream << " TimeInstant";

		//format_attributes(gml_time_instant.time_position_xml_attributes());

		const GPlatesPropertyValues::GeoTimeInstant &time_position = 
			gml_time_instant.time_position();
		if (time_position.is_real()) {
			d_line_stream << ' ' << time_position.value();
		} else if (time_position.is_distant_past()) {
			d_line_stream << " http://gplates.org/times/distantPast";
		} else if (time_position.is_distant_future()) {
			d_line_stream << " http://gplates.org/times/distantFuture";
		}

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
	{
		startHeaderLine();

		d_line_stream << " TimePeriod";

		d_line_stream << " <begin>";
		gml_time_period.begin()->accept_visitor(*this);

		d_line_stream << " <end>";
		gml_time_period.end()->accept_visitor(*this);

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_polarity_chron_id(
		const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id)
	{
		startHeaderLine();

		d_line_stream << "PolarityChronId";

		if (gpml_polarity_chron_id.get_era()) {
			d_line_stream << " <era>" << *gpml_polarity_chron_id.get_era();
		}
		if (gpml_polarity_chron_id.get_major_region()) {
			d_line_stream << " <major>" << *gpml_polarity_chron_id.get_major_region();
		}
		if (gpml_polarity_chron_id.get_minor_region()) {
			d_line_stream << " <minor>" << *gpml_polarity_chron_id.get_minor_region();
		}

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
	{
		startHeaderLine();
		gpml_constant_value.value()->accept_visitor(*this);
		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_feature_reference(
		const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference)
	{
		startHeaderLine();

		d_line_stream
			<< " FeatureReference "
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_feature_reference.feature_id().get())
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_feature_reference.value_type().get_name());

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_feature_snapshot_reference(
		const GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference)
	{
		startHeaderLine();

		d_line_stream
			<< " FeatureSnapshotReference <targetFeature>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_feature_snapshot_reference.feature_id().get())
			<< " <targetRevision>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_feature_snapshot_reference.revision_id().get())
			<< " <valueType>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_feature_snapshot_reference.value_type().get_name());

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_property_delegate(
		const GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate)
	{
		startHeaderLine();

		d_line_stream
			<< " PropertyDelegate <targetFeature>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_property_delegate.feature_id().get())
			<< " <targetProperty>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_property_delegate.target_property().get_name())
			<< " <valueType>"
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_property_delegate.value_type().get_name());

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
	{
		startHeaderLine();

		d_line_stream << " KeyValueDictionary";

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
			iter = gpml_key_value_dictionary.elements().begin(),
			end = gpml_key_value_dictionary.elements().end();
		for ( ; iter != end; ++iter)
		{
			write_gpml_key_value_dictionary_element(*iter);
		}

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
	{
		startHeaderLine();

		d_line_stream
			<< " PiecewiseAggregation"
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_piecewise_aggregation.value_type().get_name());

		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
			gpml_piecewise_aggregation.time_windows().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
			gpml_piecewise_aggregation.time_windows().end();
		for ( ; iter != end; ++iter) 
		{
			write_gpml_time_window(*iter);
		}

		endHeaderLine();
	}

	void
		VerboseHeader::visit_hot_spot_trail_mark(
		const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark)
	{
		startHeaderLine();

		d_line_stream
			<< " HotSpotTrailMark ";
		gpml_hot_spot_trail_mark.position()->accept_visitor(*this);

		if (gpml_hot_spot_trail_mark.trail_width()) {
			d_line_stream << " <trailWidth>";
			(*gpml_hot_spot_trail_mark.trail_width())->accept_visitor(*this);
		}
		if (gpml_hot_spot_trail_mark.measured_age()) {
			d_line_stream << " <measuredAge>";
			(*gpml_hot_spot_trail_mark.measured_age())->accept_visitor(*this);
		}
		if (gpml_hot_spot_trail_mark.measured_age_range()) {
			d_line_stream << " <measuredAgeRange>";
			(*gpml_hot_spot_trail_mark.measured_age_range())->accept_visitor(*this);
		}

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_measure(
		const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
	{
		startHeaderLine();

		format_attributes(gpml_measure.quantity_xml_attributes());

		d_line_stream << ' ' << gpml_measure.quantity();

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
	{
		startHeaderLine();

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

		endHeaderLine();
	}

	void
		VerboseHeader::write_gpml_time_window(
		const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
	{
		d_line_stream << " <timeDependentPropertyValue>";
		gpml_time_window.time_dependent_value()->accept_visitor(*this);

		d_line_stream << " <validTime>";
		gpml_time_window.valid_time()->accept_visitor(*this);

		d_line_stream
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_time_window.value_type().get_name());
	}

	void
		VerboseHeader::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
	{
		startHeaderLine();

		d_line_stream << " IrregularSampling";

		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter =
			gpml_irregular_sampling.time_samples().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator end =
			gpml_irregular_sampling.time_samples().end();
		for ( ; iter != end; ++iter) 
		{
			write_gpml_time_sample(*iter);
		}

		// The interpolation function is optional.
		if (gpml_irregular_sampling.interpolation_function() != NULL)
		{
			d_line_stream << " <interpolationFunction>";
			gpml_irregular_sampling.interpolation_function()->accept_visitor(*this);
		}

		d_line_stream
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_irregular_sampling.value_type().get_name());

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
	{
		startHeaderLine();

		d_line_stream
			<< ' '
			<< gpml_plate_id.value();

		endHeaderLine();
	}

	void
		VerboseHeader::visit_gpml_revision_id(
		const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id)
	{
		startHeaderLine();

		d_line_stream
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_revision_id.value().get());

		endHeaderLine();
	}

	void
		VerboseHeader::write_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		gpml_time_sample.value()->accept_visitor(*this);

		gpml_time_sample.valid_time()->accept_visitor(*this);

		// The description is optional.
		if (gpml_time_sample.description() != NULL) 
		{
			gpml_time_sample.description()->accept_visitor(*this);
		}

		d_line_stream
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(
				gpml_time_sample.value_type().get_name());
	}

	void
		VerboseHeader::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
	{
		startHeaderLine();

		d_line_stream
			<< ' '
			<< GPlatesUtils::make_qstring_from_icu_string(
				xs_string.value().get());

		endHeaderLine();
	}

	void
		VerboseHeader::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
	{
		startHeaderLine();

		d_line_stream << ' ' << xs_boolean.value();

		endHeaderLine();
	}

	void
		VerboseHeader::visit_xs_double(
		const GPlatesPropertyValues::XsDouble &xs_double)
	{
		startHeaderLine();

		d_line_stream << ' ' << xs_double.value();

		endHeaderLine();
	}

	void
		VerboseHeader::visit_xs_integer(
		const GPlatesPropertyValues::XsInteger &xs_integer)
	{
		startHeaderLine();

		d_line_stream << ' ' << xs_integer.value();

		endHeaderLine();
	}

	void
		VerboseHeader::write_gpml_key_value_dictionary_element(
		const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
	{
		d_line_stream << " (";
		element.key()->accept_visitor(*this);
		d_line_stream << ", ";
		element.value()->accept_visitor(*this);
		d_line_stream << ')';
	}

	void
		VerboseHeader::format_attributes(
		const AttributeMap &attribute_map)
	{
		if (!attribute_map.empty())
		{
			d_line_stream << " :";
		}

		BOOST_FOREACH (
			const AttributeMap::value_type& attribute,
			attribute_map)
		{
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
		VerboseHeader::startHeaderLine()
	{
		d_line_stream.setString(&d_current_line);
		++d_nested_depth;
	}

	void
		VerboseHeader::endHeaderLine()
	{
		GPlatesGlobal::Assert(d_nested_depth > 0,
			GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
		if (--d_nested_depth == 0)
		{
			d_header_lines->push_back(d_current_line);
			d_current_line.clear();
		}
	}

	bool
		PreferPlates4StyleHeader::get_feature_header_lines(
		const GPlatesModel::FeatureHandle& feature_handle, std::vector<QString>& header_lines)
	{
		// See if has old plates header.
		d_has_old_plates_header = false;
		feature_handle.accept_visitor(*this);

		return d_has_old_plates_header
			? d_plates4_style_header.get_feature_header_lines(feature_handle, header_lines)
			: d_verbose_header.get_feature_header_lines(feature_handle, header_lines);
	}
}
