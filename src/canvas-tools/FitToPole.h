/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2013 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_FITTOPOLE_H
#define GPLATES_CANVASTOOLS_FITTOPOLE_H

#include <QObject>

#include "CanvasTool.h"

#include "view-operations/RenderedGeometryCollection.h"

#include "qt-widgets/HellingerDialog.h"

// TODO: Check if we need any of these "inherited" includes/forward-declarations.
namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesCanvasTools
{
	/**
	 * Canvas tool used for fitting points to a rotation pole.
	 */
	class FitToPole :
			public QObject,
			public CanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<FitToPole>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FitToPole> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::HellingerDialog &hellinger_dialog)
		{
			return new FitToPole(
					status_bar_callback,
					rendered_geom_collection,
					main_rendered_layer_type,
					hellinger_dialog);
		}

		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();

		// TODO: The various handle_ methods below are inherited from the SmallCircleTool which we copied.
		// Update these as appropriate for the Fitting tool.
		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		virtual
		void
		handle_move_without_drag(	
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		/**
		 * We'll use shift-left-click to continue drawing an additional circle after closing 
		 * the current circle, so that we can build up multiple concentric circles in the same
		 * operation.
		 */
		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere, 
				bool is_on_earth, 
				double proximity_inclusion_threshold);

	private Q_SLOTS:

		/**
		 *  Respond to the widget's clear signal.                                                                    
		 */
		void
		handle_clear_geometries();

	private:

		void
		paint();

		FitToPole(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::HellingerDialog &hellinger_dialog);

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		//! FitToPole layer
		// TODO: Check if we need this separate layer, or if this can be done
		// on the PoleManipulation layer. (At least while this fit-to-pole tool
		// remains a part of the PoleManipulation workflows, as opposed to going
		// into its own, say, "Statistics" or "Fitting" workflow...
		GPlatesViewOperations::RenderedGeometryLayer *d_fit_to_pole_layer_ptr;

		GPlatesQtWidgets::HellingerDialog *d_hellinger_dialog_ptr;

	};
}

#endif  // GPLATES_CANVASTOOLS_FITTOPOLE_H
