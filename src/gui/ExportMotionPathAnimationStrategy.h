/* $Id: ExportFlowlinesAnimationStrategy.h 8274 2010-05-03 20:02:16Z rwatson $ */

/**
 * \file 
 * $Revision: 8274 $
 * $Date: 2010-05-03 22:02:16 +0200 (ma, 03 mai 2010) $ 
 * 
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
 
#ifndef GPLATES_GUI_EXPORTMOTIONPATHANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTMOTIONPATHANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QString>

#include "file-io/File.h"
#include "gui/ExportAnimationStrategy.h"
#include "utils/ExportTemplateFilenameSequence.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"



namespace GPlatesFileIO
{
	class File;
}

namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for 
	 * writing plate velocity meshes.
	 * 
	 * ExportMotionPathsAnimationStrategy serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportMotionPathAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportMotionPathsAnimationStrategy,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportMotionPathAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;
				
		/**
		 * For storing files referenced in the current reconstruction.                                                                      
		 */		
		typedef std::vector<const GPlatesFileIO::File::Reference *> files_collection_type;				

		static const QString 
			DEFAULT_MOTION_PATHS_GMT_FILENAME_TEMPLATE;
		static const QString 
			DEFAULT_MOTION_PATHS_SHP_FILENAME_TEMPLATE;
		static const QString 
			MOTION_PATHS_FILENAME_TEMPLATE_DESC;
		static const QString
			MOTION_PATHS_DESC;

		enum FileFormat
		{
			GMT,
			SHAPEFILE
		};


		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				FileFormat format = GMT,
				const ExportAnimationStrategy::Configuration& cfg=
					ExportAnimationStrategy::Configuration(
						DEFAULT_MOTION_PATHS_GMT_FILENAME_TEMPLATE));


		virtual
		~ExportMotionPathAnimationStrategy()
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
			return MOTION_PATHS_DESC;
		}


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
		ExportMotionPathAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString &filename_template);
		
	private:
		ExportMotionPathAnimationStrategy();
		
		/**
		 * The reconstruction file(s) used to create this reconstruction.                                                                     
		 */
		files_collection_type d_loaded_files;		

		FileFormat d_file_format;
	};
}


#endif //GPLATES_GUI_EXPORTMOTIONPATHANIMATIONSTRATEGY_H


