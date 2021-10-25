/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_EXPORTDEFORMATIONANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTDEFORMATIONANIMATIONSTRATEGY_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <QString>

#include "ExportAnimationStrategy.h"
#include "ExportOptionsUtils.h"

#include "file-io/DeformationExport.h"

#include "model/FeatureCollectionHandle.h"

#include "utils/non_null_intrusive_ptr.h"

#include "view-operations/VisibleReconstructionGeometryExport.h"


namespace GPlatesGui
{
	// Forward declaration.
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for writing deformation.
	 * 
	 * ExportDeformationAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportDeformationAnimationStrategy :
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportDeformationAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportDeformationAnimationStrategy> non_null_ptr_type;


		/**
		 * Configuration options.
		 */
		class Configuration :
				public ExportAnimationStrategy::ConfigurationBase
		{
		public:

			enum FileFormat
			{
				GPML,
				GMT
			};


			explicit
			Configuration(
					const QString &filename_template_,
					FileFormat file_format_,
					const ExportOptionsUtils::ExportFileOptions &file_options_,
					bool include_principal_strain_,
					// Only applied when 'include_principal_strain' is true...
					const GPlatesFileIO::DeformationExport::PrincipalStrainOptions &principal_strain_options_,
					bool include_dilatation_strain_,
					bool include_dilatation_strain_rate_,
					bool include_second_invariant_strain_rate_) :
				ConfigurationBase(filename_template_),
				file_format(file_format_),
				file_options(file_options_),
				include_principal_strain(include_principal_strain_),
				principal_strain_options(principal_strain_options_),
				include_dilatation_strain(include_dilatation_strain_),
				include_dilatation_strain_rate(include_dilatation_strain_rate_),
				include_second_invariant_strain_rate(include_second_invariant_strain_rate_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			FileFormat file_format;
			ExportOptionsUtils::ExportFileOptions file_options;
			bool include_principal_strain;
			GPlatesFileIO::DeformationExport::PrincipalStrainOptions principal_strain_options;
			bool include_dilatation_strain;
			bool include_dilatation_strain_rate;
			bool include_second_invariant_strain_rate;
		};

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;
		//! Typedef for a shared pointer to @a Configuration.
		typedef boost::shared_ptr<Configuration> configuration_ptr;


		/**
		 * GPML format configuration options.
		 */
		class GpmlConfiguration :
				public Configuration
		{
		public:

			explicit
			GpmlConfiguration(
					const QString &filename_template_,
					const ExportOptionsUtils::ExportFileOptions &file_options_,
					bool include_principal_strain_,
					// Only applied when 'include_principal_strain' is true...
					const GPlatesFileIO::DeformationExport::PrincipalStrainOptions &principal_strain_options_,
					bool include_dilatation_strain_,
					bool include_dilatation_strain_rate_,
					bool include_second_invariant_strain_rate_) :
				Configuration(
						filename_template_,
						GPML,
						file_options_,
						include_principal_strain_,
						principal_strain_options_,
						include_dilatation_strain_,
						include_dilatation_strain_rate_,
						include_second_invariant_strain_rate_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new GpmlConfiguration(*this));
			}
		};


		/**
		 * GMT format configuration options.
		 */
		class GMTConfiguration :
				public Configuration
		{
		public:

			enum DomainPointFormatType
			{
				LON_LAT,
				LAT_LON
			};

			explicit
			GMTConfiguration(
					const QString &filename_template_,
					const ExportOptionsUtils::ExportFileOptions &file_options_,
					DomainPointFormatType domain_point_format_,
					bool include_principal_strain_,
					// Only applied when 'include_principal_strain' is true...
					const GPlatesFileIO::DeformationExport::PrincipalStrainOptions &principal_strain_options_,
					bool include_dilatation_strain_,
					bool include_dilatation_strain_rate_,
					bool include_second_invariant_strain_rate_) :
				Configuration(
						filename_template_,
						GMT,
						file_options_,
						include_principal_strain_,
						principal_strain_options_,
						include_dilatation_strain_,
						include_dilatation_strain_rate_,
						include_second_invariant_strain_rate_),
				domain_point_format(domain_point_format_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new GMTConfiguration(*this));
			}

			DomainPointFormatType domain_point_format;
		};


		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration)
		{
			return non_null_ptr_type(
					new ExportDeformationAnimationStrategy(
							export_animation_context,
							export_configuration));
		}


		virtual
		~ExportDeformationAnimationStrategy()
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

	protected:

		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportDeformationAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration);
		
	private:
		/**
		 * The list of currently loaded files that are active.
		 *
		 * This code is copied from "gui/ExportReconstructedGeometryAnimationStrategy.h".
		 */
		GPlatesViewOperations::VisibleReconstructionGeometryExport::files_collection_type d_loaded_files;

		//! Export configuration parameters.
		const_configuration_ptr d_configuration;
	};
}

#endif // GPLATES_GUI_EXPORTDEFORMATIONANIMATIONSTRATEGY_H
