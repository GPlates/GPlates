/* $Id$ */


/**
 * \file 
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
 
#ifndef GPLATES_GUI_EXPORTCITCOMSRESOLVEDTOPOLOGYSTRATEGY_H
#define GPLATES_GUI_EXPORTCITCOMSRESOLVEDTOPOLOGYSTRATEGY_H


#include <vector>
#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "ExportAnimationStrategy.h"

#include "file-io/File.h"
#include "file-io/CitcomsResolvedTopologicalBoundaryExport.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalGeometry;
}

namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for 
	 * exporting resolved topologies in a CitcomS-specific manner.
	 * 
	 * ExportCitcomsResolvedTopologyAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportCitcomsResolvedTopologyAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportCitcomsResolvedTopologyAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportCitcomsResolvedTopologyAnimationStrategy> non_null_ptr_type;


		class Configuration;

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;
		//! Typedef for a shared pointer to @a Configuration.
		typedef boost::shared_ptr<Configuration> configuration_ptr;

		/**
		 * Configuration options..
		 */
		class Configuration :
				public ExportAnimationStrategy::ConfigurationBase
		{
		public:
			enum FileFormat
			{
				SHAPEFILE,
				GMT,
				OGRGMT
			};

			explicit
			Configuration(
					const QString& filename_template_,
					FileFormat file_format_,
					const GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::OutputOptions &output_options_) :
				ConfigurationBase(filename_template_),
				file_format(file_format_),
				output_options(output_options_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_ptr(new Configuration(*this));
			}

			FileFormat file_format;
			GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::OutputOptions output_options;
		};


		/**
		 * Creates an export animation strategy.
		 */
		static
		const non_null_ptr_type
		create(
				ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &cfg)
		{
			return non_null_ptr_type(
					new ExportCitcomsResolvedTopologyAnimationStrategy(
							export_animation_context, cfg));
		}


		virtual
		~ExportCitcomsResolvedTopologyAnimationStrategy()
		{  }
		

		/**
		 * Sets the internal ExportTemplateFilenameSequence.
		 */
		virtual
		void
		set_template_filename(
				const QString &filename);


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


	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		ExportCitcomsResolvedTopologyAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &cfg);
		
	private:
		//! Typedef for a sequence of resolved topological geometries.
		typedef std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_geom_seq_type;


		/**
		 * The list of currently loaded files.
		 */
		std::vector<const GPlatesFileIO::File::Reference *> d_loaded_files;

		/**
		 * The active and loaded reconstruction file(s) used in the reconstruction.
		 */
		std::vector<const GPlatesFileIO::File::Reference *> d_loaded_reconstruction_files;

		//! Export configuration parameters.
		const_configuration_ptr d_configuration;


		/**
		 * Export to the various files.
		 */
		void
		export_files(
				const resolved_geom_seq_type &resolved_geom_seq,
				const double &recon_time,
				const QString &filebasename);
	};
}


#endif // GPLATES_GUI_EXPORTCITCOMSRESOLVEDTOPOLOGYSTRATEGY_H
