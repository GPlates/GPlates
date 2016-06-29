/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_EXPORTOPTIONSUTILS_H
#define GPLATES_GUI_EXPORTOPTIONSUTILS_H

#include <boost/optional.hpp>
#include <QSize>
#include "qt-widgets/VelocityMethodWidget.h"

#include "app-logic/VelocityDeltaTime.h"


namespace GPlatesGui
{
	namespace ExportOptionsUtils
	{
		/**
		 * Options useful when exporting to Shapefile - either a single file or multiple files.
		 */
		struct ExportFileOptions
		{
			explicit
			ExportFileOptions(
					bool export_to_a_single_file_ = true,
					bool export_to_multiple_files_ = true,
					bool separate_output_directory_per_file_ = true) :
				export_to_a_single_file(export_to_a_single_file_),
				export_to_multiple_files(export_to_multiple_files_),
				separate_output_directory_per_file(separate_output_directory_per_file_)
			{  }


			/**
			 * Export all @a ReconstructionGeometry derived objects to a single export file.
			 */
			bool export_to_a_single_file;

			/**
			 * Export @a ReconstructionGeometry derived objects to multiple export files.
			 *
			 * By default each output file corresponds to an input file that the features
			 * (that generated the reconstruction geometries) came from.
			 */
			bool export_to_multiple_files;

			/**
			 * If 'true' then the *multiple* export files will follow the pattern...
			 *
			 *   "<export_path>/<collection_filename>/<export_template_filename>"
			 *
			 * ...otherwise they will follow the pattern...
			 *
			 *   "<export_path>/<collection_filename>_<export_template_filename>"
			 *
			 * NOTE: This option only applies if @a export_to_multiple_files is true.
			 */
			bool separate_output_directory_per_file;
		};


		/**
		 * Common image resolution options useful when exporting either screenshots or to SVG.
		 */
		struct ExportImageResolutionOptions
		{
			explicit
			ExportImageResolutionOptions(
					bool constrain_aspect_ratio_,
					boost::optional<QSize> image_size_ = boost::none) :
				image_size(image_size_),
				constrain_aspect_ratio(constrain_aspect_ratio_)
			{  }

			/**
			 * Image size - boost::none means use the current globe/map viewport dimensions.
			 */
			boost::optional<QSize> image_size;

			/**
			 * Whether to keep the ratio of width to height constant.
			 */
			bool constrain_aspect_ratio;
		};


		/**
		 * Common rotations options useful when exporting either total or stage rotations.
		 */
		struct ExportRotationOptions
		{
			//! How to write out an identity rotation.
			enum IdentityRotationFormatType
			{
				WRITE_IDENTITY_AS_INDETERMINATE,
				WRITE_IDENTITY_AS_NORTH_POLE
			};

			//! How to write out a Euler pole.
			enum EulerPoleFormatType
			{
				WRITE_EULER_POLE_AS_CARTESIAN,
				WRITE_EULER_POLE_AS_LATITUDE_LONGITUDE,
			};


			explicit
			ExportRotationOptions(
					IdentityRotationFormatType identity_rotation_format_,
					EulerPoleFormatType euler_pole_format_) :
				identity_rotation_format(identity_rotation_format_),
				euler_pole_format(euler_pole_format_)
			{  }


			IdentityRotationFormatType identity_rotation_format;
			EulerPoleFormatType euler_pole_format;
		};


		/**
		 * Rotations options useful when exporting *stage* rotations only.
		 */
		struct ExportStageRotationOptions
		{
			explicit
			ExportStageRotationOptions(
					const double &time_interval_) :
				time_interval(time_interval_)
			{  }


			//! The stage rotation time interval (in My).
			double time_interval;
		};


		/**
		 * Velocity calculation options.
		 */
		struct ExportVelocityCalculationOptions
		{
			explicit
			ExportVelocityCalculationOptions(
					GPlatesAppLogic::VelocityDeltaTime::Type delta_time_type_,
					const double &delta_time_,
					bool is_boundary_smoothing_enabled_,
					const double &boundary_smoothing_angular_half_extent_degrees_,
					bool exclude_deforming_regions_) :
				delta_time_type(delta_time_type_),
				delta_time(delta_time_),
				is_boundary_smoothing_enabled(is_boundary_smoothing_enabled_),
				boundary_smoothing_angular_half_extent_degrees(boundary_smoothing_angular_half_extent_degrees_),
				exclude_deforming_regions(exclude_deforming_regions_)
			{  }

			GPlatesAppLogic::VelocityDeltaTime::Type delta_time_type;
			double delta_time;

			bool is_boundary_smoothing_enabled;
			double boundary_smoothing_angular_half_extent_degrees;
			bool exclude_deforming_regions;
		};

		/**
		 * Net Rotation options
		 */
		struct ExportNetRotationOptions
		{
		    explicit
		    ExportNetRotationOptions(
			    const double &delta_time_,
			    const GPlatesQtWidgets::VelocityMethodWidget::VelocityMethod &velocity_method_):
			delta_time(delta_time_),
			velocity_method(velocity_method_)
		    {}

		    double delta_time;
		    GPlatesQtWidgets::VelocityMethodWidget::VelocityMethod velocity_method;
		};
	}
}

#endif // GPLATES_GUI_EXPORTOPTIONSUTILS_H
