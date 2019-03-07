/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015, 2016 Geological Survey of Norway
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

#ifndef GPLATES_GUI_EXPORTNETROTATIONANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTNETROTATIONANIMATIONSTRATEGY_H

#include <utility>
#include <boost/optional.hpp>

#include "ExportAnimationStrategy.h"
#include "ExportOptionsUtils.h"

#include "file-io/MultiPointVectorFieldExport.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"

#include "qt-widgets/VelocityMethodWidget.h"

#include "utils/non_null_intrusive_ptr.h"

#include "view-operations/VisibleReconstructionGeometryExport.h"


namespace GPlatesAppLogic
{
	class MultiPointVectorField;
}

namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for
	 * writing net rotations.
	 *
	 * ExportNetRotationAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportNetRotationAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportNetRotationAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportNetRotationAnimationStrategy> non_null_ptr_type;

		typedef std::vector<const GPlatesFileIO::File::Reference *> file_collection_type;


		/**
		 * Configuration options. For net-rotations these are
		 * 1) the csv export file type (comma, tab or semicolon)
		 * 2) velocity calculation options
		 */
		class Configuration :
				public ExportAnimationStrategy::ConfigurationBase
		{
		public:


			enum CsvExportType
			{
				CSV_COMMA,
				CSV_TAB,
				CSV_SEMICOLON
			};

			explicit
			Configuration(
					const QString &filename_template_,
					CsvExportType csv_export_type_,
					const ExportOptionsUtils::ExportNetRotationOptions &options):
				ConfigurationBase(filename_template_),
				d_csv_export_type(csv_export_type_),
				d_options(options)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			CsvExportType d_csv_export_type;
			ExportOptionsUtils::ExportNetRotationOptions d_options;
		};

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;
		//! Typedef for a shared pointer to @a Configuration.
		typedef boost::shared_ptr<Configuration> configuration_ptr;


		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration)
		{
			return non_null_ptr_type(
					new ExportNetRotationAnimationStrategy(
							export_animation_context,
							export_configuration));
		}


		virtual
		~ExportNetRotationAnimationStrategy()
		{  }


		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);


		/**
		 * Allows Strategy objects to do any housekeeping that might be necessary
		 * after all export iterations are completed.
		 *
		 * @param export_successful is true if all iterations were performed successfully,
		 *        false if there was any kind of interruption.
		 *
		 * Called by ExportAnimationContext.
		 */
		virtual
		void
		wrap_up(
				bool export_successful);


		void
		set_template_filename(
				const QString &);

		// Is public since used by anonymous functions in cpp file.
		typedef std::pair<GPlatesMaths::LatLonPoint, double> pole_type;

	protected:

		// Required only if using existing velocity mesh for calculations.
		typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *> vector_field_seq_type;

		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportNetRotationAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration);



	private:

		/**
		 * @brief export_iteration_using_existing_velocity_mesh - calculates net-rotations using the velocities
		 * of an existing velocity mesh layer.
		 * @param frame_index
		 * @return
		 */
		bool
		export_iteration_using_existing_velocity_mesh(
				std::size_t frame_index);

		/**
		 * @brief export_iteration - calculates net-rotations based on a hard-coded 1-degree lat-lon grid.
		 * @param frame_index
		 * @return
		 */
		bool
		export_iteration(
				std::size_t frame_index);


		typedef std::pair<double, pole_type> time_pole_pair_type;


		/**
		 * The list of currently loaded files that are active.
		 *
		 * This code is copied from "gui/ExportReconstructedGeometryAnimationStrategy.h".
		 */
		file_collection_type d_loaded_files;

		/**
		 * The active and loaded reconstruction file(s) used in the reconstruction.
		 */
		file_collection_type d_loaded_reconstruction_files;

		//! Export configuration parameters.
		const_configuration_ptr d_configuration;

		std::vector<time_pole_pair_type> d_total_poles;

		/**
		 * @brief d_referenced_files_set - Set of the referenced geometry files encountered during the whole export sequence.
		 */
		std::set<const GPlatesFileIO::File::Reference *> d_referenced_files_set;

		GPlatesModel::integer_plate_id_type d_anchor_plate_id;

	};
}


#endif //GPLATES_GUI_EXPORTNETROTATIONANIMATIONSTRATEGY_H


