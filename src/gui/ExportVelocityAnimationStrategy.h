/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_EXPORTVELOCITYANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTVELOCITYANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <QString>

#include "ExportAnimationStrategy.h"
#include "ExportOptionsUtils.h"

#include "file-io/MultiPointVectorFieldExport.h"

#include "model/FeatureCollectionHandle.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

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
	 * writing plate velocity meshes.
	 * 
	 * ExportVelocityAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportVelocityAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportVelocityAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportVelocityAnimationStrategy> non_null_ptr_type;


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
				GMT,
				TERRA_TEXT,
				CITCOMS_GLOBAL
			};


			explicit
			Configuration(
					const QString &filename_template_,
					FileFormat file_format_) :
				ConfigurationBase(filename_template_),
				file_format(file_format_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			FileFormat file_format;
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
					const ExportOptionsUtils::ExportFileOptions &file_options_) :
				Configuration(filename_template_, GPML),
				file_options(file_options_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new GpmlConfiguration(*this));
			}

			ExportOptionsUtils::ExportFileOptions file_options;
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
					GPlatesFileIO::MultiPointVectorFieldExport::VelocityVectorFormatType velocity_vector_format_,
					double velocity_scale_,
					unsigned int velocity_stride_,
					DomainPointFormatType domain_point_format_,
					bool include_plate_id_,
					bool include_domain_point_,
					bool include_domain_meta_data_) :
				Configuration(filename_template_, GMT),
				file_options(file_options_),
				velocity_vector_format(velocity_vector_format_),
				velocity_scale(velocity_scale_),
				velocity_stride(velocity_stride_),
				domain_point_format(domain_point_format_),
				include_plate_id(include_plate_id_),
				include_domain_point(include_domain_point_),
				include_domain_meta_data(include_domain_meta_data_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new GMTConfiguration(*this));
			}

			ExportOptionsUtils::ExportFileOptions file_options;
			GPlatesFileIO::MultiPointVectorFieldExport::VelocityVectorFormatType velocity_vector_format;
			double velocity_scale;
			unsigned int velocity_stride;
			DomainPointFormatType domain_point_format;
			bool include_plate_id;
			bool include_domain_point;
			bool include_domain_meta_data;
		};


		/**
		 * Terra text format configuration options.
		 */
		class TerraTextConfiguration :
				public Configuration
		{
		public:

			//
			// Used to pattern match parameters contained in Terra velocity domain file names.
			//
			static const QString MT_PLACE_HOLDER; // Terra 'mt' parameter.
			static const QString NT_PLACE_HOLDER; // Terra 'nt' parameter.
			static const QString ND_PLACE_HOLDER; // Terra 'nd' parameter.
			static const QString PROCESSOR_PLACE_HOLDER; // Terra local processor number.


			explicit
			TerraTextConfiguration(
					const QString &filename_template_,
					const QString &terra_grid_filename_template_) :
				Configuration(filename_template_, TERRA_TEXT),
				terra_grid_filename_template(terra_grid_filename_template_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new TerraTextConfiguration(*this));
			}

			QString terra_grid_filename_template;
		};


		/**
		 * CitcomS global format configuration options.
		 */
		class CitcomsGlobalConfiguration :
				public Configuration
		{
		public:

			//
			// Used to pattern match parameters contained in CitcomS velocity domain file names.
			//
			static const QString DENSITY_PLACE_HOLDER; // CitcomS diamond density/resolution.
			static const QString CAP_NUM_PLACE_HOLDER; // CitcomS diamond cap number.


			explicit
			CitcomsGlobalConfiguration(
					const QString &filename_template_,
					const QString &citcoms_grid_filename_template_) :
				Configuration(filename_template_, CITCOMS_GLOBAL),
				citcoms_grid_filename_template(citcoms_grid_filename_template_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new CitcomsGlobalConfiguration(*this));
			}

			QString citcoms_grid_filename_template;
		};


		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration)
		{
			return non_null_ptr_type(
					new ExportVelocityAnimationStrategy(
							export_animation_context,
							export_configuration));
		}


		virtual
		~ExportVelocityAnimationStrategy()
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
		typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *> vector_field_seq_type;

		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportVelocityAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration);

		void
		export_velocity_fields_to_file(
				const vector_field_seq_type &velocity_fields,
				QString filename);
		
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


#endif //GPLATES_GUI_EXPORTVELOCITYANIMATIONSTRATEGY_H


