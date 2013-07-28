/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_PLATESROTATIONFORMATWRITER_H
#define GPLATES_FILEIO_PLATESROTATIONFORMATWRITER_H

#include <iosfwd>
#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>
#include <list>
#include <fstream>
#include "ErrorOpeningFileForWritingException.h"
#include "FileInfo.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlTotalReconstructionPole.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "maths/FiniteRotation.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesPropertyValues
{
	class GpmlTimeSample;
	class GpmlTimeWindow;
}

namespace GPlatesFileIO
{
	class PlatesRotationFormatWriter:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		explicit
		PlatesRotationFormatWriter(
				const FileInfo &file_info,
				const GPlatesModel::Gpgim &gpgim);

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

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
		visit_gpml_total_reconstruction_pole(
				const GPlatesPropertyValues::GpmlTotalReconstructionPole &trp);

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
		visit_gpml_metadata(
				const GPlatesPropertyValues::GpmlMetadata &gpml_metadata){}		

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);

	protected:

		void
		write_gpml_time_sample(
				const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample);

		struct PlatesRotationFormatAccumulator
		{
			struct ReconstructionPoleData
			{
				boost::optional<GPlatesMaths::FiniteRotation> finite_rotation;
				boost::optional<GPlatesUtils::UnicodeString> comment;
				boost::optional<double> time;
				boost::optional<bool> is_disabled;
				std::vector<GPlatesModel::Metadata::shared_ptr_to_const_type > metadata;

				/**
				 * Test whether the rotation pole data has acquired enough information to
				 * be able to print a meaningful entry in a PLATES4 file.
				 */
				bool 
				have_sufficient_info_for_output() const
				{
					return finite_rotation && time;
				}
			};

			std::list< ReconstructionPoleData > reconstruction_poles;
			boost::optional<GPlatesModel::integer_plate_id_type> moving_plate_id;
			boost::optional<GPlatesModel::integer_plate_id_type> fixed_plate_id;

			/**
			 * Return a reference to the reconstruction pole that is currently
			 * under construction.
			 */
			ReconstructionPoleData &
			current_pole()
			{
				return reconstruction_poles.back();
			}


			/**
			 * Test whether the accumulator has acquired enough information to
			 * be able to print meaningful entries in a PLATES4 file.
			 */
			bool 
			have_sufficient_info_for_output() const;


			/**
			 * Print lines to the rotation file using the accumulated data.
			 * Should only be called if have_sufficient_info_for_output() returned true.
			 */
			void
			print_rotation_lines(std::ostream *os);
		};
	   	
		PlatesRotationFormatAccumulator d_accum;
		boost::scoped_ptr<std::ostream> d_output;
	};
}

#endif  // GPLATES_FILEIO_PLATESROTATIONFORMATWRITER_H



