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

#include <list>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QFile>
#include <QTextStream>

#include "ErrorOpeningFileForWritingException.h"
#include "FileInfo.h"

#include "maths/FiniteRotation.h"

#include "model/FeatureVisitor.h"
#include "model/Metadata.h"
#include "model/PropertyName.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlTimeSample.h"


namespace GPlatesPropertyValues
{
	class GpmlTimeWindow;
}

namespace GPlatesFileIO
{
	class PlatesRotationFormatWriter:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		/**
		 * If @a grot_format is false then the GROT-style metadata (http://www.gplates.org/grot/index.html)
		 * is prefixed by "999 0.0 0.0 0.0 0.0 999 !" on lines that don't already contain a rotation pole.
		 * This is because the old-style PLATES4 rotation file format requires every line to contain
		 * a rotation pole (the '999' indicating a commented out pole).
		 */
		explicit
		PlatesRotationFormatWriter(
				const FileInfo &file_info,
				bool grot_format = false);

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
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);

	protected:

		void
		write_gpml_time_sample(
				const GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_to_const_type &gpml_time_sample);

		struct PlatesRotationFormatAccumulator
		{
			struct ReconstructionPoleData
			{
				boost::optional<GPlatesMaths::FiniteRotation> finite_rotation;
				boost::optional<GPlatesUtils::UnicodeString> comment;
				boost::optional<double> time;
				boost::optional<bool> is_disabled;
				boost::optional< std::vector<GPlatesModel::Metadata::shared_ptr_to_const_type> > metadata;

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
			 *
			 * Should only be called if have_sufficient_info_for_output() returned true.
			 */
			void
			print_rotations(
					QTextStream &os,
					bool grot_format);


			/**
			 * Print a rotation.
			 *
			 * Should only be called if...
			 *   PlatesRotationFormatAccumulator::ReconstructionPoleData::have_sufficient_info_for_output()
			 * ...returned true.
			 */
			void
			print_rotation(
					QTextStream &os,
					const ReconstructionPoleData &reconstruction_pole_data,
					bool grot_format);
		};
	   	
		/**
		 * Whether the output file is GROT ('.grot') format, or PLATES4('.rot') format.
		 */
		bool d_grot_format;

		PlatesRotationFormatAccumulator d_accum;
		boost::scoped_ptr<QFile> d_output_file;
		boost::scoped_ptr<QTextStream> d_output_stream;
	};
}

#endif  // GPLATES_FILEIO_PLATESROTATIONFORMATWRITER_H



