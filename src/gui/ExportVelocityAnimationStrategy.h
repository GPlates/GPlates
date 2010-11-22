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

#include "model/FeatureCollectionHandle.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"
#include "utils/ExportTemplateFilenameSequence.h"

#include "view-operations/VisibleReconstructionGeometryExport.h"

#include "gui/ExportAnimationStrategy.h"

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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportVelocityAnimationStrategy,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportVelocityAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		static const QString 
			DEFAULT_MESH_VELOCITIES_FILENAME_TEMPLATE;
		static const QString 
			MESH_VELOCITIES_FILENAME_TEMPLATE_DESC;
		static const QString
			MESH_VELOCITIES_DESC;

		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const ExportAnimationStrategy::Configuration& cfg=
					ExportAnimationStrategy::Configuration(
							DEFAULT_MESH_VELOCITIES_FILENAME_TEMPLATE));


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
			return MESH_VELOCITIES_DESC;
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
		typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *> vector_field_seq_type;

		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportVelocityAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString filename_template);

		void
		export_velocity_fields_to_file(
				const vector_field_seq_type &velocity_fields,
				QString filename);

		
		QString
		get_file_name_from_feature_collection_handle(
				GPlatesModel::FeatureCollectionHandle::weak_ref);
		
	private:
		/**
		 * The list of currently loaded files that are active.
		 *
		 * This code is copied from "gui/ExportReconstructedGeometryAnimationStrategy.h".
		 */
		GPlatesViewOperations::VisibleReconstructionGeometryExport::files_collection_type d_active_files;

		ExportVelocityAnimationStrategy();
	};
}


#endif //GPLATES_GUI_EXPORTVELOCITYANIMATIONSTRATEGY_H


