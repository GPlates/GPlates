/* $Id$ */

/**
 * \file 
 * Collects as much PLATES4 header information from features as possible.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_PLATESLINEFORMATHEADERVISITOR_H
#define GPLATES_FILEIO_PLATESLINEFORMATHEADERVISITOR_H

#include <boost/optional.hpp>
#include <unicode/unistr.h>

#include "model/FeatureVisitor.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlOldPlatesHeader.h"


namespace GPlatesFileIO
{
	struct OldPlatesHeader
	{
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
			unsigned int number_of_points_);

		/**
		* Default constructor for an OldPlatesHeader.
		*
		* The hard-coded default values used here were chosen for their
		* ability to alert the user to their invalidity (e.g., there is
		* no '999' plate id, nor 'XX' data type in the PLATES4 format).
		*/
		OldPlatesHeader() : 
			region_number(99),
			reference_number(99),
			string_number(9999),
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


		/**
		 * Creates a GpmlOldPlatesHeader property value from 'this'.
		 */
		GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
		create_gpml_old_plates_header();
	};

	/**
	 * Collects PLATES4 header information.
	 * If feature doesn't have an old plates header then as much
	 * information is gathered as possible to fill it in.
	 */
	class PlatesLineFormatHeaderVisitor :
		private GPlatesModel::ConstFeatureVisitor
	{
	public:
		PlatesLineFormatHeaderVisitor();

		/**
		* Visits @a feature_handle and collects old plates header information.
		*
		* @param feature_handle feature to visit.
		* @param old_plates_header old plates header information returned in this.
		*/
		virtual
			bool
			get_old_plates_header(
					const GPlatesModel::FeatureHandle::const_weak_ref &feature,
					OldPlatesHeader& old_plates_header,
					bool append_feature_id_to_geographic_description = true);

	private:
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
			visit_gpml_old_plates_header(
			const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
			void
			visit_gpml_plate_id(
			const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
			void
			visit_xs_string(
			const GPlatesPropertyValues::XsString &xs_string);


		struct PlatesHeaderAccumulator
		{
			boost::optional<OldPlatesHeader> old_plates_header;

			boost::optional<GPlatesModel::integer_plate_id_type> plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> conj_plate_id;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> age_of_appearance;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> age_of_disappearance;
			boost::optional<UnicodeString> geographic_description;
		};


		PlatesHeaderAccumulator d_accum;
	};
}

#endif // GPLATES_FILEIO_PLATESLINEFORMATHEADERVISITOR_H
