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
 
#ifndef GPLATES_GUI_EXPORTRECONSTRUCTEDGEOMETRYANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTRECONSTRUCTEDGEOMETRYANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

#include "utils/ExportTemplateFilenameSequence.h"

#include "view-operations/VisibleReconstructedFeatureGeometryExport.h"

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


		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context);


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

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportReconstructedGeometryAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context);
		
	private:
		
		/**
		 * The list of visible, reconstructable FeatureCollections to take
		 * geometry from.
		 */
		GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::files_collection_type
				d_active_reconstructable_files;
	};
}


#endif //GPLATES_GUI_EXPORTRECONSTRUCTEDGEOMETRYANIMATIONSTRATEGY_H


