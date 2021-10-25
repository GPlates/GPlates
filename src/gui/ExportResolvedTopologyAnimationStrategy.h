/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_EXPORTRESOLVEDTOPOLOGYANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTRESOLVEDTOPOLOGYANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "ExportAnimationStrategy.h"
#include "ExportOptionsUtils.h"

#include "utils/ReferenceCount.h"

#include "view-operations/VisibleReconstructionGeometryExport.h"


namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for  exporting
	 * resolved topologies in a general manner (as opposed to the CitcomS-specific manner).
	 * 
	 * This class serves as the concrete Strategy role as
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
		 * Configuration options.
		 */
		class Configuration :
				public ExportAnimationStrategy::ConfigurationBase
		{
		public:
			enum FileFormat
			{
				SHAPEFILE,
				OGRGMT,
				GMT
			};

			explicit
			Configuration(
					const QString& filename_template_,
					FileFormat file_format_,
					const ExportOptionsUtils::ExportFileOptions &file_options_,
					bool export_topological_lines_ = true,
					bool export_topological_polygons_ = true,
					bool wrap_to_dateline_ = true) :
				ConfigurationBase(filename_template_),
				file_format(file_format_),
				file_options(file_options_),
				export_topological_lines(export_topological_lines_),
				export_topological_polygons(export_topological_polygons_),
				wrap_to_dateline(wrap_to_dateline_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			FileFormat file_format;
			ExportOptionsUtils::ExportFileOptions file_options;
			bool export_topological_lines;
			bool export_topological_polygons;
			bool wrap_to_dateline;
		};

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;


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
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		ExportResolvedTopologyAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &cfg);
		
	private:
		/**
		 * The list of currently loaded files.
		 */
		GPlatesViewOperations::VisibleReconstructionGeometryExport::files_collection_type
				d_loaded_files;

		/**
		 * The active and loaded reconstruction file(s) used in the reconstruction.
		 */
		GPlatesViewOperations::VisibleReconstructionGeometryExport::files_collection_type d_loaded_reconstruction_files;

		//! Export configuration parameters.
		const_configuration_ptr d_configuration;
	};
}

#endif // GPLATES_GUI_EXPORTRESOLVEDTOPOLOGYANIMATIONSTRATEGY_H
