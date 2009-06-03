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
 
#ifndef GPLATES_GUI_EXPORTANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

#include "utils/ExportTemplateFilenameSequence.h"


namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Base class for the different Export Animation strategies.
	 * ExportAnimationStrategy serves as the (abstract) Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportAnimationStrategy:
			public GPlatesUtils::ReferenceCount<ExportAnimationStrategy>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportAnimationStrategy,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;



		virtual
		~ExportAnimationStrategy()
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
				std::size_t frame_index) = 0;


		/**
		 * Allows Strategy objects to do any housekeeping that might be necessary
		 * after all export iterations are completed.
		 *
		 * Of course, there's also the destructor, which should free up any resources
		 * we acquired in the constructor; this method is intended for any "last step"
		 * iteration operations that might need to occur. Perhaps all iterations end
		 * up in the same file and we should close that file (if all steps completed
		 * successfully). This is the place to do those final steps.
		 *
		 * @param export_successful is true if all iterations were performed successfully,
		 *        false if there was any kind of interruption.
		 *
		 * Called by ExportAnimationContext.
		 */
		virtual
		void
		wrap_up(
				bool export_successful)
		{  }


	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context);
		
		/**
		 * Pointer back to the Context, as an easy way to get at all kinds of state.
		 * See Gamma et al, Strategy pattern, p319 point 1.
		 */
		GPlatesGui::ExportAnimationContext *d_export_animation_context_ptr;
		
		/**
		 * The filename sequence to use when exporting.
		 */
		boost::optional<GPlatesUtils::ExportTemplateFilenameSequence> d_filename_sequence_opt;
		boost::optional<GPlatesUtils::ExportTemplateFilenameSequence::const_iterator> d_filename_iterator_opt;
		
	};
}


#endif //GPLATES_GUI_EXPORTANIMATIONSTRATEGY_H


