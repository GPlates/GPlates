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

#include <QDir>
#include <QString>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

#include "gui/ExportAnimationStrategy.h"


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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportAnimationContext,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportAnimationContext,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;


		explicit
		ExportAnimationContext(
				GPlatesQtWidgets::ExportAnimationDialog &export_animation_dialog_,
				GPlatesGui::AnimationController &animation_controller_,
				GPlatesQtWidgets::ViewportWindow &view_state_);

		virtual
		~ExportAnimationContext()
		{  }

		const double &
		view_time() const;


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
		
		bool
		svg_exporter_enabled()
		{
			return d_svg_exporter_enabled;
		}
		
		void
		set_svg_exporter_enabled(
				bool enable)
		{
			d_svg_exporter_enabled = enable;
		}
		
		const QString
		svg_exporter_filename_template()
		{
			return "snapshot_%u_%0.2f.svg";
		}

		bool
		velocity_exporter_enabled()
		{
			return d_velocity_exporter_enabled;
		}

		void
		set_velocity_exporter_enabled(
				bool enable)
		{
			d_velocity_exporter_enabled = enable;
		}
		
		const QString
		velocity_exporter_filename_template()
		{
			return "velocity_%u_%0.2f.gpml";
		}

		bool
		gmt_exporter_enabled()
		{
			return d_gmt_exporter_enabled;
		}
		
		void
		set_gmt_exporter_enabled(
				bool enable)
		{
			d_gmt_exporter_enabled = enable;
		}
		
		const QString
		gmt_exporter_filename_template()
		{
			return "reconstructed_%u_%0.2f.xy";
		}

		bool
		shp_exporter_enabled()
		{
			return d_shp_exporter_enabled;
		}
		
		void
		set_shp_exporter_enabled(
				bool enable)
		{
			d_shp_exporter_enabled = enable;
		}
		
		const QString
		shp_exporter_filename_template()
		{
			return "reconstructed_%u_%0.2f.shp";
		}



		const GPlatesGui::AnimationController &
		animation_controller() const;

		GPlatesQtWidgets::ViewportWindow &
		view_state();


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
				QString message);

	private:
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
		GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

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
		 * Whether the strategy for writing SVG files should be enabled during export.
		 * Note we may later put this flag plus filename template etc. into a
		 * strategy-specific 'configuration' object. But time is of the essence.
		 */
		bool d_svg_exporter_enabled;

		/**
		 * Whether the strategy for writing velocity files should be enabled during export.
		 * Note we may later put this flag plus filename template etc. into a
		 * strategy-specific 'configuration' object. But time is of the essence.
		 */
		bool d_velocity_exporter_enabled;

		/**
		 * Whether the strategy for writing GMT files should be enabled during export.
		 * Note we may later put this flag plus filename template etc. into a
		 * strategy-specific 'configuration' object. But time is of the essence.
		 */
		bool d_gmt_exporter_enabled;

		/**
		 * Whether the strategy for writing Shapefile files should be enabled during export.
		 * Note we may later put this flag plus filename template etc. into a
		 * strategy-specific 'configuration' object. But time is of the essence.
		 */
		bool d_shp_exporter_enabled;
	};
}

#endif
// GPLATES_GUI_EXPORTANIMATIONCONTEXT


