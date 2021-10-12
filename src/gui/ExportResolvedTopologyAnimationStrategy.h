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

#include "gui/ExportAnimationStrategy.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"

#include "utils/ExportTemplateFilenameSequence.h"


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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportResolvedTopologyAnimationStrategy,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportResolvedTopologyAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		static const QString DEFAULT_RESOLOVED_TOPOLOGIES_FILENAME_TEMPLATE;
		static const QString RESOLOVED_TOPOLOGIES_FILENAME_TEMPLATE_DESC;
		static const QString RESOLOVED_TOPOLOGIES_DESC;
		
		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const ExportAnimationStrategy::Configuration &cfg=
					ExportAnimationStrategy::Configuration(
							DEFAULT_RESOLOVED_TOPOLOGIES_FILENAME_TEMPLATE));


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

		virtual
		const QString&
		get_default_filename_template();

		virtual
		const QString&
		get_filename_template_desc()
		{
			return RESOLOVED_TOPOLOGIES_FILENAME_TEMPLATE_DESC;
		}


		virtual
		const QString&
				get_description()
		{
			return RESOLOVED_TOPOLOGIES_DESC;
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


	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportResolvedTopologyAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString &filename_template);
		
	private:
		//! Replaces placeholder string when exporting all platepolygons to a single file.
		static const QString s_placeholder_platepolygons;

		//! Replaces placeholder string when exporting all line geometry to a single file.
		static const QString s_placeholder_lines;

		//! Replaces placeholder string when exporting all ridge/transform geometry to a single file.
		static const QString s_placeholder_ridge_transforms;

		//! Replaces placeholder string when exporting all subduction geometry to a single file.
		static const QString s_placeholder_subductions;

		//! Replaces placeholder string when exporting all left subduction geometry to a single file.
		static const QString s_placeholder_left_subductions;

		//! Replaces placeholder string when exporting all right subduction geometry to a single file.
		static const QString s_placeholder_right_subductions;

		//! Typedef for a sequence of resolved topological geometries.
		typedef std::vector<const GPlatesAppLogic::ResolvedTopologicalBoundary *> resolved_geom_seq_type;


		/**
		 * Export to the various files.
		 */
		void
		export_files(
				const resolved_geom_seq_type &resolved_geom_seq,
				const double &recon_time,
				const QString &filebasename);

		ExportResolvedTopologyAnimationStrategy();
		ExportResolvedTopologyAnimationStrategy(
				const ExportResolvedTopologyAnimationStrategy&);
	};
}


#endif // GPLATES_GUI_EXPORTRESOLVEDTOPOLOGYSTRATEGY_H
