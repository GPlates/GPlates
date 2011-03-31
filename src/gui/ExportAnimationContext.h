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
 
#ifndef GPLATES_GUI_EXPORTANIMATIONCONTEXT_H
#define GPLATES_GUI_EXPORTANIMATIONCONTEXT_H

#include <map>

#include <QDir>
#include <QString>

#include <boost/function.hpp>

#include "ExportAnimationStrategy.h"
#include "ExportAnimationType.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;		// ViewState.
	class ExportAnimationDialog;	// Pointer back to dialog for status updates via this Context.
}

namespace GPlatesGui
{
	class AnimationController;

	/**
	 * ExportAnimationContext manages the iteration steps and progress bar updates while
	 * we are exporting an animation via the ExportAnimationDialog.
	 * 
	 * It serves as the Context role in the Strategy pattern, in Gamma et al p315.
	 * It maintains a list of ExportAnimationStrategy derivations which perform the
	 * work of exporting one frame of animation.
	 *
	 * These strategies keep a back-reference to this ExportAnimationContext so that
	 * they can access particular members that are useful to them, such as ViewState.
	 * This practice is described in Gamma et al p319, "Implementation", bullet 1.
	 */
	class ExportAnimationContext:
			public GPlatesUtils::ReferenceCount<ExportAnimationContext>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportAnimationContext>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportAnimationContext> non_null_ptr_type;


		explicit
		ExportAnimationContext(
				GPlatesQtWidgets::ExportAnimationDialog &export_animation_dialog_,
				GPlatesGui::AnimationController &animation_controller_,
				GPlatesPresentation::ViewState &view_state_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_);

		virtual
		~ExportAnimationContext()
		{  }
	
		const double &
		view_time() const;
	
		GPlatesQtWidgets::ExportAnimationDialog *
		get_export_dialog()
		{
			return d_export_animation_dialog_ptr;
		}

		bool
		is_running() const
		{
			return d_export_running;
		}

		const QDir &
		target_dir() const
		{
			return d_target_dir;
		}

		void
		set_target_dir(
				const QDir &dir)
		{
			d_target_dir = dir;
		}
		
		void
		add_export_animation_strategy(
				ExportAnimationType::ExportID,
				const ExportAnimationStrategy::const_configuration_base_ptr &cfg);


		const GPlatesGui::AnimationController &
		animation_controller() const;

		GPlatesPresentation::ViewState &
		view_state();

		GPlatesQtWidgets::ViewportWindow &
		viewport_window();

		/**
		 * Used by ExportAnimationDialog in response to user.
		 * Could be better, notifying the dialog via slot or
		 * regular old method call.
		 */
		void
		abort()
		{
			d_abort_now = true;
		}

		/**
		 * Prepares filename template, calls suitable functions for
		 * each export iteration, updates progress bar.
		 *
		 * Returns true on success, false if interrupted.
		 *
		 * Note: passing in reference to ExportAnimationContext because
		 * one day we'll want to move these export_ methods out of this
		 * dialog, but we still need to update the progress bar.
		 */
		bool
		do_export();

		void
		update_status_message(
				const QString &message);

// The following enumeration seems to be obsolete. 
#if 0
		enum EXPORT_ITEMS
		{
			RECONSTRUCTED_GEOMETRIES_GMT,
			RECONSTRUCTED_GEOMETRIES_SHAPEFILE,
			PROJECTED_GEOMETRIES_SVG,
			MESH_VELOCITIES_GPML,
			RESOLVED_TOPOLOGIES_GMT,
			INVALID=999
		};
#endif

	private:
		//! Typedef to map export ID to an @a ExportAnimationStrategy.
		typedef std::map<
				ExportAnimationType::ExportID, 
				ExportAnimationStrategy::non_null_ptr_type> 
			exporter_map_type;


		void
		cleanup_exporters_map()
		{
			d_exporter_map.clear();
		}

		/**
		 * Pointer back to the ExportAnimationDialog, so that we can update
		 * the progress bar and status message during export.
		 */
		GPlatesQtWidgets::ExportAnimationDialog *d_export_animation_dialog_ptr;

		/**
		 * This is the animation controller, which holds the state of any
		 * animation set up in the application.
		 * This allows us to control the same animation from
		 * ExportAnimationContext, AnimateDialog and AnimateControlWidget.
		 */
		GPlatesGui::AnimationController *d_animation_controller_ptr;
				
		/**
		 * View State pointer, which needs to be accessible to the various
		 * strategies so that they can get access to things like the current
		 * anchored plate ID and the Reconstruction.
		 */
		GPlatesPresentation::ViewState *d_view_state;

		/**
		 * Temporary access point for some view state.
		 * FIXME: remove this after everything non widget based has been moved
		 * from ViewportWindow to ViewState.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window;

		/**
		 * Flag that gets set when the user requests, nay demands, that we
		 * stop what we are doing. As soon as the next Strategy has finished
		 * doing the current frame, we'll abort.
		 */
		bool d_abort_now;

		/**
		 * Flag set while we are in the do_export() loop.
		 * This is used by the @a is_running() accessor, which the dialog wants
		 * so it knows if it should call @a abort() when the user callously
		 * closes the dialog mid-export.
		 */
		bool d_export_running;
		
		/**
		 * The target output directory where all the files get written to.
		 */
		QDir d_target_dir;

		/**
		 * A map of export ID to exporters.
		 */
		exporter_map_type d_exporter_map;
	};
}

#endif // GPLATES_GUI_EXPORTANIMATIONCONTEXT
