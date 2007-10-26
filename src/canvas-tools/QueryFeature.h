/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_QUERYFEATURE_H
#define GPLATES_CANVASTOOLS_QUERYFEATURE_H

#include <QObject>

#include "gui/CanvasTool.h"
#include "gui/FeatureWeakRefSequence.h"


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
	class QueryFeaturePropertiesDialog;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to query features by clicking on them.
	 */
	class QueryFeature:
			// It seems that QObject must be the first base specified here...
			public QObject, public GPlatesGui::CanvasTool
	{
		Q_OBJECT

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<QueryFeature>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<QueryFeature> non_null_ptr_type;

		virtual
		~QueryFeature()
		{  }

		/**
		 * Create a QueryFeature instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureWeakRefSequence::non_null_ptr_type external_hit_sequence_ptr,
				GPlatesQtWidgets::QueryFeaturePropertiesDialog &qfp_dialog_)
		{
			QueryFeature::non_null_ptr_type ptr(*(new QueryFeature(globe, globe_canvas_,
					view_state_, external_hit_sequence_ptr, qfp_dialog_)));
			return ptr;
		}

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

	signals:
		void
		sorted_hits_updated();

		void
		no_hits_found();

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		QueryFeature(
				GPlatesGui::Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureWeakRefSequence::non_null_ptr_type external_hit_sequence_ptr,
				GPlatesQtWidgets::QueryFeaturePropertiesDialog &qfp_dialog_):
			CanvasTool(globe, globe_canvas_),
			d_view_state_ptr(&view_state_),
			d_external_hit_sequence_ptr(external_hit_sequence_ptr),
			d_qfp_dialog_ptr(&qfp_dialog_)
		{  }

		const GPlatesQtWidgets::ViewportWindow &
		view_state() const
		{
			return *d_view_state_ptr;
		}

		GPlatesGui::FeatureWeakRefSequence &
		external_hit_sequence() const
		{
			return *d_external_hit_sequence_ptr;
		}

		GPlatesQtWidgets::QueryFeaturePropertiesDialog &
		qfp_dialog() const
		{
			return *d_qfp_dialog_ptr;
		}

	private:
		/**
		 * This is the view state which is used to obtain the reconstruction root.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This is the external sequence of hits which will be updated in the event that
		 * the test point hits one or more geometries.
		 */
		GPlatesGui::FeatureWeakRefSequence::non_null_ptr_type d_external_hit_sequence_ptr;

		/**
		 * This is the dialog box which we will be populating in response to a feature
		 * query.
		 */
		GPlatesQtWidgets::QueryFeaturePropertiesDialog *d_qfp_dialog_ptr;

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		QueryFeature(
				const QueryFeature &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		QueryFeature &
		operator=(
				const QueryFeature &);
	};
}

#endif  // GPLATES_CANVASTOOLS_QUERYFEATURE_H
