/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_EXPORTROTATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTROTATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

#include "utils/ExportTemplateFilenameSequence.h"

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
	class ExportRotationAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:

 		enum RotationType
 		{
			RELATIVE_COMMA,
			RELATIVE_SEMI,
			RELATIVE_TAB,
			EQUIVALENT_COMMA,
			EQUIVALENT_SEMI,
			EQUIVALENT_TAB,
			INVALID_TYPE=999
		};

		static const QString DEFAULT_RELATIVE_COMMA_FILENAME_TEMPLATE;
		static const QString DEFAULT_RELATIVE_SEMI_FILENAME_TEMPLATE;
		static const QString DEFAULT_RELATIVE_TAB_FILENAME_TEMPLATE;
		static const QString DEFAULT_EQUIVALENT_COMMA_FILENAME_TEMPLATE;
		static const QString DEFAULT_EQUIVALENT_SEMI_FILENAME_TEMPLATE;
		static const QString DEFAULT_EQUIVALENT_TAB_FILENAME_TEMPLATE;
		static const QString ROTATION_FILENAME_TEMPLATE_DESC;
		static const QString RELATIVE_ROTATION_DESC;
		static const QString EQUIVALENT_ROTATION_DESC;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportReconstructedGeometryAnimationStrategy,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportRotationAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;
		
		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				RotationType type=EQUIVALENT_COMMA,
				const ExportAnimationStrategy::Configuration &cfg=
					ExportAnimationStrategy::Configuration(
							DEFAULT_EQUIVALENT_COMMA_FILENAME_TEMPLATE));

		virtual
		~ExportRotationAnimationStrategy()
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
		get_filename_template_desc();

		virtual
		const QString&
				get_description()
		{
			return RELATIVE_ROTATION_DESC;
		}


	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportRotationAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString &filename_template);
		
	private:
		ExportRotationAnimationStrategy();
		RotationType d_type;
		char d_delimiter;
		
	};
}


#endif //GPLATES_GUI_EXPORTROTATIONSTRATEGY_H


