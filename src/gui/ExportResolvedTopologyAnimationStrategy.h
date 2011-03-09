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
 
#ifndef GPLATES_GUI_EXPORTRESOLVEDTOPOLOGYSTRATEGY_H
#define GPLATES_GUI_EXPORTRESOLVEDTOPOLOGYSTRATEGY_H


#include <vector>
#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "ExportAnimationStrategy.h"

#include "file-io/File.h"
#include "file-io/ResolvedTopologicalBoundaryExport.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalBoundary;
}

namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for 
	 * writing plate velocity meshes.
	 * 
	 * ExportResolvedTopologyAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportResolvedTopologyAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportResolvedTopologyAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportResolvedTopologyAnimationStrategy> non_null_ptr_type;


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
				GMT
			};

			explicit
			Configuration(
					const QString& filename_template_,
					FileFormat file_format_,
					const GPlatesFileIO::ResolvedTopologicalBoundaryExport::OutputOptions &output_options_) :
				ConfigurationBase(filename_template_),
				file_format(file_format_),
				output_options(output_options_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			FileFormat file_format;
			GPlatesFileIO::ResolvedTopologicalBoundaryExport::OutputOptions output_options;
		};

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;


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
					new ExportResolvedTopologyAnimationStrategy(
							export_animation_context, cfg));
		}


		virtual
		~ExportResolvedTopologyAnimationStrategy()
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
		ExportResolvedTopologyAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &cfg);
		
	private:
		//! Typedef for a sequence of resolved topological geometries.
		typedef std::vector<const GPlatesAppLogic::ResolvedTopologicalBoundary *> resolved_geom_seq_type;


		/**
		 * The list of currently loaded files.
		 */
		std::vector<const GPlatesFileIO::File::Reference *> d_loaded_files;

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


#endif // GPLATES_GUI_EXPORTRESOLVEDTOPOLOGYSTRATEGY_H
