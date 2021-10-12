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

#ifndef GPLATES_FILEIO_GMTFORMATHEADER_H
#define GPLATES_FILEIO_GMTFORMATHEADER_H

#include <vector>
#include <QString>
#include <QTextStream>

#include "PlatesLineFormatHeaderVisitor.h"

#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"

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


namespace GPlatesModel
{
	class FeatureHandle;
}

namespace GPlatesFileIO
{
	/**
	 * Interface for formatting of a GMT feature header.
	 */
	class GMTFormatHeader
	{
	public:
		virtual
			~GMTFormatHeader()
		{  }

		/**
		 * Format feature into a sequence of header lines (returned as strings).
		 * @return @c true if there is enough information to print a header.
		 */
		virtual
		bool
		get_feature_header_lines(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature,
				std::vector<QString>& header_lines) = 0;
	};


	/**
	 * Prints lines of header and keeps track of writing starting '>' character.
	 * NOTE: Use one instance of @a GMTHeaderPrinter per file written.
	 */
	class GMTHeaderPrinter
	{
	public:
		GMTHeaderPrinter()
			: d_is_first_feature_header_in_file(true)
		{ }

		//! Prints the header lines at the top of the file.
		void
		print_global_header_lines(
				QTextStream& output_stream,
				std::vector<QString>& header_lines);

		//! Prints the header lines at beginning of a feature.
		void
		print_feature_header_lines(
				QTextStream& output_stream,
				std::vector<QString>& header_lines);

	private:
		//! Is the next feature to be written the first one ?
		bool d_is_first_feature_header_in_file;
	};


	/**
	 * Formats a header using PLATES4 information if available.
	 * Otherwise formats a short header containing feature type and id.
	 */
	class GMTFormatPlates4StyleHeader :
			public GPlatesFileIO::GMTFormatHeader,
			private GPlatesModel::ConstFeatureVisitor
	{
	public:
		virtual
		bool
		get_feature_header_lines(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature,
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
	class GMTFormatVerboseHeader :
			public GPlatesFileIO::GMTFormatHeader,
			private GPlatesModel::ConstFeatureVisitor
	{
	public:
		GMTFormatVerboseHeader();

		virtual
		bool
		get_feature_header_lines(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature,
				std::vector<QString>& header_lines);

	private:
		typedef std::map<GPlatesModel::XmlAttributeName,
				GPlatesModel::XmlAttributeValue> AttributeMap;

		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

		bool
		initialise_pre_property_values(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		void
		finalise_post_property_values(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration);

		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon);

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
		start_header_line();

		void
		end_header_line(
				bool output = true);

		void
		clear_header_line();


		//! Accumulates information when visiting a property.
		class PropertyAccumulator
		{
		public:
			PropertyAccumulator()
			{
				clear();
			}

			//! Clear accumulation when starting on a new property.
			void clear()
			{
				d_is_geometry_property = false;
				d_plate_id = boost::none;
			}

			bool
			get_is_geometry_property() const
			{
				return d_is_geometry_property;
			}

			void
			set_is_geometry_property()
			{
				d_is_geometry_property = true;
			}

			void
			set_plate_id(
					const GPlatesModel::integer_plate_id_type &plate_id)
			{
				d_plate_id = plate_id;
			}

			boost::optional<GPlatesModel::integer_plate_id_type>
			get_plate_id() const
			{
				return d_plate_id;
			}

			bool
			is_reconstruction_plate_id_property(
					const GPlatesModel::PropertyName &property_name) const
			{
				static const GPlatesModel::PropertyName reconstruction_plate_id_property_name =
						GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

				return property_name == reconstruction_plate_id_property_name;
			}

		private:
			//! Is current property a geometry ?
			bool d_is_geometry_property;

			//! The current plate id.
			boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;
		};


		//! Output of @a get_feature_header_lines.
		std::vector<QString>* d_header_lines;

		//! Current header line being formatted (not used by all methods).
		QString d_current_line;

		//! Used to write to d_current_line.
		QTextStream d_line_stream;

		//! The depth of nesting of property values.
		int d_nested_depth;

		//! Accumulates information about the current property.
		PropertyAccumulator d_property_accumulator;
	};


	/**
	 * Formats PLATES4 style header if feature has an old plates header property.
	 * Otherwise formats a verbose header.
	 */
	class GMTFormatPreferPlates4StyleHeader :
			public GPlatesFileIO::GMTFormatHeader,
			private GPlatesModel::ConstFeatureVisitor
	{
	public:
		virtual
		bool
		get_feature_header_lines(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature,
				std::vector<QString>& header_lines);

	private:
		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &/*gpml_old_plates_header*/)
		{
			d_has_old_plates_header = true;
		}

		bool d_has_old_plates_header;
		GMTFormatPlates4StyleHeader d_plates4_style_header;
		GMTFormatVerboseHeader d_verbose_header;
	};

}

#endif // GPLATES_FILEIO_GMTFORMATHEADER_H
