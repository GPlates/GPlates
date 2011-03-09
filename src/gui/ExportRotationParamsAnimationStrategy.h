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
 
#ifndef GPLATES_GUI_EXPORTROTATIONPARAMSSTRATEGY_H
#define GPLATES_GUI_EXPORTROTATIONPARAMSSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "ExportAnimationStrategy.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 */
	class ExportRotationParamsAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:

		enum File_Format
		{
			COMMA,
			SEMICOLON,
			TAB,
			INVALID
		};
 		
		static const QString DEFAULT_ROTATION_PARAMS_COMMA_FILENAME_TEMPLATE;
		static const QString DEFAULT_ROTATION_PARAMS_SEMI_FILENAME_TEMPLATE;
		static const QString DEFAULT_ROTATION_PARAMS_TAB_FILENAME_TEMPLATE;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportRotationParamsAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportRotationParamsAnimationStrategy> non_null_ptr_type;
		
		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				File_Format,
				const ExportAnimationStrategy::const_configuration_base_ptr &cfg);

		virtual
		~ExportRotationParamsAnimationStrategy()
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
		ExportRotationParamsAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString &filename_template);
		
	private:
		ExportRotationParamsAnimationStrategy();
		File_Format d_format;
		char d_delimiter;
	};
}


#endif //GPLATES_GUI_EXPORTROTATIONPARAMSSTRATEGY_H


