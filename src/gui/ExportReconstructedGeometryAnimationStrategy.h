/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_GUI_EXPORTRECONSTRUCTEDGEOMETRYANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTRECONSTRUCTEDGEOMETRYANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

#include "utils/ExportTemplateFilenameSequence.h"

#include "view-operations/VisibleReconstructionGeometryExport.h"

#include "gui/ExportAnimationStrategy.h"


namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for 
	 * writing reconstructed feature geometries at each timestep.
	 * 
	 * ExportReconstructedGeometryAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportReconstructedGeometryAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportReconstructedGeometryAnimationStrategy,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportReconstructedGeometryAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;
		static const QString 
				DEFAULT_RECONSTRUCTED_GEOMETRIES_GMT_FILENAME_TEMPLATE;
		static const QString 
				DEFAULT_RECONSTRUCTED_GEOMETRIES_SHP_FILENAME_TEMPLATE;
		static const QString
				RECONSTRUCTED_GEOMETRIES_FILENAME_TEMPLATE_DESC;
		static const QString 
				RECONSTRUCTED_GEOMETRIES_DESC;
			
		enum FileFormat
		{
			SHAPEFILE,
			GMT
		};


		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				FileFormat format=GMT,
				const ExportAnimationStrategy::Configuration& cfg=
					ExportAnimationStrategy::Configuration(
							DEFAULT_RECONSTRUCTED_GEOMETRIES_GMT_FILENAME_TEMPLATE));


		virtual
		~ExportReconstructedGeometryAnimationStrategy()
		{  }


		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);

		virtual
		const QString&
				get_default_filename_template();

		virtual
		const QString&
		get_filename_template_desc()
		{
			return RECONSTRUCTED_GEOMETRIES_FILENAME_TEMPLATE_DESC;
		}

		virtual
		const QString&
		get_description()
		{
			return RECONSTRUCTED_GEOMETRIES_DESC;
		}

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportReconstructedGeometryAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString &filename_template);
		
	private:
		/**
		 * The list of currently loaded files that are active.
		 */
		GPlatesViewOperations::VisibleReconstructionGeometryExport::files_collection_type
				d_active_files;
		FileFormat d_file_format;

		ExportReconstructedGeometryAnimationStrategy();
	};
}


#endif //GPLATES_GUI_EXPORTRECONSTRUCTEDGEOMETRYANIMATIONSTRATEGY_H


