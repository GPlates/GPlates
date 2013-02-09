/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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

#include "utils/AnimationSequenceUtils.h"
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


		void
		clear_export_animation_strategies()
		{
			d_exporter_multimap.clear();
		}


		const GPlatesGui::AnimationController &
		animation_controller() const;
		
		/**
		 * The SequenceInfo configured by the export dialog may be different from the
		 * global one configured in the AnimationController, due to export dialogs
		 * being smushed together. ExportAnimationStrategy should use *this* accessor
		 * to determine the appropriate range for a filename sequence.
		 */
		GPlatesUtils::AnimationSequence::SequenceInfo
		get_sequence()
		{
			return d_sequence_info;
		}

		/**
		 * The SequenceInfo configured by the export dialog may be different from the
		 * global one configured in the AnimationController, due to export dialogs
		 * being smushed together. ExportAnimationDialog should use *this* accessor
		 * to set the appropriate range before any calls to add_export_animation_strategy
		 * to ensure that the filename templates within those strategies are using
		 * the appropriate sequence.
		 */
		void
		set_sequence(
				const GPlatesUtils::AnimationSequence::SequenceInfo &seq)
		{
			d_sequence_info = seq;
		}
		                                

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
		 * Before calling this final export step, you are expected to have
		 * configured the context by calling set_sequence() and
		 * add_export_animation_strategy()
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
		/**
		 * Typedef to multimap export ID to an @a ExportAnimationStrategy.
		 *
		 * Note that we allow multiple entries matching the same export ID - this is because
		 * the user may want multiple exports with the same export type and format but with
		 * different export options.
		 */
		typedef std::multimap<
				ExportAnimationType::ExportID, 
				ExportAnimationStrategy::non_null_ptr_type> 
			exporter_multimap_type;


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
		 * And this is just the currently-set-up animation sequence, which
		 * now may differ from the global animation in the AnimationController
		 * because the Export Snapshot/Sequence dialogs were smushed together.
		 */
		GPlatesUtils::AnimationSequence::SequenceInfo d_sequence_info;
		
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
		 * A multimap of export ID to exporters.
		 *
		 * Note that we allow multiple entries matching the same export ID - this is because
		 * the user may want multiple exports with the same export type and format but with
		 * different export options.
		 */
		exporter_multimap_type d_exporter_multimap;
	};
}

#endif // GPLATES_GUI_EXPORTANIMATIONCONTEXT
