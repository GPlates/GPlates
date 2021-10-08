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

#ifndef GPLATES_GUI_CANVASTOOLCHOICE_H
#define GPLATES_GUI_CANVASTOOLCHOICE_H

#include <QObject>

#include "CanvasTool.h"
#include "FeatureWeakRefSequence.h"


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
	class QueryFeaturePropertiesDialog;
}

namespace GPlatesGui
{
	/**
	 * This class contains the current choice of CanvasTool.
	 *
	 * It also provides slots to choose the CanvasTool.
	 *
	 * This serves the role of the Context class in the State Pattern in Gamma et al.
	 */
	class CanvasToolChoice:
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Construct a CanvasToolChoice instance.
		 *
		 * These parameters are needed by various CanvasTool derivations, which will be
		 * instantiated by this class.
		 */
		CanvasToolChoice(
				Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				FeatureWeakRefSequence::non_null_ptr_type external_hit_sequence_ptr,
				GPlatesQtWidgets::QueryFeaturePropertiesDialog &qfp_dialog_);

		~CanvasToolChoice()
		{  }

		CanvasTool &
		tool_choice() const
		{
			return *d_tool_choice_ptr;
		}

	public slots:
		void
		choose_reorient_globe_tool()
		{
			change_tool_if_necessary(d_reorient_globe_tool_ptr);
		}

		void
		choose_zoom_globe_tool()
		{
			change_tool_if_necessary(d_zoom_globe_tool_ptr);
		}

		void
		choose_query_feature_tool()
		{
			change_tool_if_necessary(d_query_feature_tool_ptr);
		}

	private:
		/**
		 * This is the ReorientGlobe tool which the user may choose.
		 */
		CanvasTool::non_null_ptr_type d_reorient_globe_tool_ptr;

		/**
		 * This is the ZoomGlobe tool which the user may choose.
		 */
		CanvasTool::non_null_ptr_type d_zoom_globe_tool_ptr;

		/**
		 * This is the QueryFeature tool which the user may choose.
		 */
		CanvasTool::non_null_ptr_type d_query_feature_tool_ptr;

		/**
		 * The current choice of CanvasTool.
		 */
		CanvasTool::non_null_ptr_type d_tool_choice_ptr;

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		CanvasToolChoice(
				const CanvasToolChoice &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		CanvasToolChoice &
		operator=(
				const CanvasToolChoice &);

		void
		change_tool_if_necessary(
				CanvasTool::non_null_ptr_type new_tool_choice);
	};
}

#endif  // GPLATES_GUI_CANVASTOOLCHOICE_H
