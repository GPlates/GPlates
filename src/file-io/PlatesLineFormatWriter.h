/* $Id$ */

/**
 * \file 
 * Defines the interface for writing data in Plates4 data format.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_FILEIO_PLATESLINEFORMATWRITER_H
#define GPLATES_FILEIO_PLATESLINEFORMATWRITER_H

#include <iosfwd>
#include <boost/optional.hpp>
#include "FileInfo.h"
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PointOnSphere.h"

namespace GPlatesFileIO
{
	class PlatesLineFormatWriter:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		/**
		 * @pre file_info.is_writable() is true.
		 */
		explicit
		PlatesLineFormatWriter(
				const FileInfo &file_info);

		virtual
		~PlatesLineFormatWriter()
		{
			delete d_output;
		}

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				const GPlatesModel::InlinePropertyContainer &inline_property_container);

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_gpml_time_sample(
				const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample);

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);

	private:

		struct PlatesLineFormatAccumulator {

			struct OldPlatesHeader {
				unsigned region_number;
				unsigned reference_number;
				unsigned string_number;
				UnicodeString geographic_description;
				GPlatesModel::integer_plate_id_type plate_id_number;
				double age_of_appearance;
				double age_of_disappearance;
				UnicodeString data_type_code;
				unsigned data_type_code_number;
				UnicodeString data_type_code_number_additional;
				GPlatesModel::integer_plate_id_type conjugate_plate_id_number;
				unsigned colour_code;
				unsigned number_of_points;

				OldPlatesHeader(
						unsigned int region_number_,
						unsigned int reference_number_,
						unsigned int string_number_,
						const UnicodeString &geographic_description_,
						GPlatesModel::integer_plate_id_type plate_id_number_,
						double age_of_appearance_,
						double age_of_disappearance_,
						const UnicodeString &data_type_code_,
						unsigned int data_type_code_number_,
						const UnicodeString &data_type_code_number_additional_,
						GPlatesModel::integer_plate_id_type conjugate_plate_id_number_,
						unsigned int colour_code_,
						unsigned int number_of_points_):
					region_number(region_number_),
					reference_number(reference_number_),
					string_number(string_number_),
					geographic_description(geographic_description_),
					plate_id_number(plate_id_number_),
					age_of_appearance(age_of_appearance_),
					age_of_disappearance(age_of_disappearance_),
					data_type_code(data_type_code_),
					data_type_code_number(data_type_code_number_),
					data_type_code_number_additional(data_type_code_number_additional_),
					conjugate_plate_id_number(conjugate_plate_id_number_),
					colour_code(colour_code_),
					number_of_points(number_of_points_)
				{ }

				/**
				 * Default constructor for an OldPlatesHeader.
				 *
				 * The hard-coded default values used here were chosen for their
				 * ability to alert the user to their invalidity (e.g., there is
				 * no '999' plate id, nor 'XX' data type in the PLATES4 format).
				 */
				OldPlatesHeader() : 
					region_number(0),
					reference_number(0),
					string_number(0),
					geographic_description("This header contains only default values."),
					plate_id_number(999),
					age_of_appearance(999.0),
					age_of_disappearance(-999.0),
					data_type_code("XX"),
					data_type_code_number(0),
					data_type_code_number_additional(""),
					conjugate_plate_id_number(999),
					colour_code(1),
					number_of_points(1)
				{ }
			};

			boost::optional<OldPlatesHeader> old_plates_header;

			boost::optional<UnicodeString> feature_type;
			boost::optional<UnicodeString> feature_id;

			boost::optional<GPlatesModel::integer_plate_id_type> plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> conj_plate_id;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> age_of_appearance;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> age_of_disappearance;
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> 
					polyline;
			boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
					point;

			boost::optional<GPlatesModel::PropertyName> current_propname;

			/**
			 * Test whether the accumulator has acquired enough information to
			 * be able to print a meaningful entry in a PLATES4 file.
			 */
			bool 
			have_sufficient_info_for_output() const;

		};
	   	
		PlatesLineFormatAccumulator d_accum;
		std::ostream *d_output;

		void
		print_header_lines(
				std::ostream *os, 
				const PlatesLineFormatAccumulator::OldPlatesHeader &old_plates_header);
	};
}

#endif  // GPLATES_FILEIO_PLATESLINEFORMATWRITER_H
