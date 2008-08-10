/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_RECONSTRUCTIONPOLEWIDGET_H
#define GPLATES_QTWIDGETS_RECONSTRUCTIONPOLEWIDGET_H

#include <QWidget>
#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>

#include "ReconstructionPoleWidgetUi.h"
#include "gui/SimpleGlobeOrientation.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
	

	class ReconstructionPoleWidget:
			public QWidget, 
			protected Ui_ReconstructionPoleWidget
	{
		Q_OBJECT
	public:
		explicit
		ReconstructionPoleWidget(
				GPlatesQtWidgets::ViewportWindow &view_state,
				QWidget *parent_ = NULL);

	public slots:

		void
		start_new_drag(
				const GPlatesMaths::PointOnSphere &current_position);

		void
		update_drag_position(
				const GPlatesMaths::PointOnSphere &current_position);

		void
		finalise();

		void
		handle_reset_adjustment();

		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry);

	private:

		GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This accumulates the rotation for us.
		 *
		 * Ignore the fact that it looks like it's a @em globe orientation.  That's just
		 * your eyes playing tricks on you.
		 *
		 * Since SimpleGlobeOrientation is non-copy-constructable and non-copy-assignable,
		 * we have to use a boost:scoped_ptr here instead of a boost::optional.
		 */
		boost::scoped_ptr<GPlatesGui::SimpleGlobeOrientation> d_accum_orientation;

		/**
		 * The reconstruction plate ID from the reconstructed feature geometry (RFG).
		 *
		 * Note that this could be 'boost::none' -- it's possible for an RFG to be created
		 * without a reconstruction plate ID.  (See the comments in class
		 * GPlatesModel::ReconstructedFeatureGeometry for details.)
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;

		/**
		 * The (initial) geometry of the currently-focused RFG (if there is one).
		 *
		 * As the user drags the geometry around to modify the total reconstruction pole,
		 * this geometry will be rotated to new positions on the globe by the accumulated
		 * rotation.
		 *
		 * Presumably, this will be non-NULL when the 'update_drag_position' slot is
		 * triggered, since if there were no geometry, what would the user be dragging?
		 */
		GPlatesMaths::GeometryOnSphere::maybe_null_ptr_to_const_type d_initial_geometry;
	};
}

#endif  // GPLATES_QTWIDGETS_RECONSTRUCTIONPOLEWIDGET_H
