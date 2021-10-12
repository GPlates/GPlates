/* $Id$ */

/**
 * \file Interface for activating/deactivating geometry operations.
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

#ifndef GPLATES_VIEWOPERATIONS_CLONEOPERATION_H
#define GPLATES_VIEWOPERATIONS_CLONEOPERATION_H

#include <QObject>

#include "GeometryBuilder.h"

#include "gui/ChooseCanvasTool.h"

#include "presentation/ViewState.h"

namespace GPlatesViewOperations
{
	
	class CloneOperation :
		public QObject
	{
		Q_OBJECT

	public:
		CloneOperation(
				GPlatesGui::ChooseCanvasTool* choose_canvas_tool,
				GPlatesViewOperations::GeometryBuilder* digitise_geometry_builder,
				GPlatesViewOperations::GeometryBuilder* focused_feature_geometry_builder,
				GPlatesPresentation::ViewState & view_state) :
			d_choose_canvas_tool(choose_canvas_tool),
			d_digitise_geometry_builder(digitise_geometry_builder),
			d_focused_feature_geometry_builder(focused_feature_geometry_builder),
			d_view_state(view_state)
		{ }

		virtual
		~CloneOperation()
		{ }

	public slots:
			
		void
		clone_focused_geometry();

		void 
		clone_focused_feature();

	private:
		GPlatesGui::ChooseCanvasTool* d_choose_canvas_tool;

		GPlatesViewOperations::GeometryBuilder* d_digitise_geometry_builder;

		GPlatesViewOperations::GeometryBuilder* d_focused_feature_geometry_builder;

		GPlatesPresentation::ViewState & d_view_state;
	};
}

#endif // GPLATES_VIEWOPERATIONS_CLONEOPERATION_H



