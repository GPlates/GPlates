/* $Id$ */

/**
 * \file Interface for activating/deactivating geometry operations.
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

#ifndef GPLATES_VIEWOPERATIONS_GEOMETRYOPERATION_H
#define GPLATES_VIEWOPERATIONS_GEOMETRYOPERATION_H

#include <QObject>

#include "GeometryBuilder.h"


namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesViewOperations
{
	/**
	 * Interface for activating/deactivating a geometry operation.
	 */
	class GeometryOperation :
		public QObject
	{
		Q_OBJECT

	public:
		virtual
		~GeometryOperation()
		{  }

		/**
		 * Activate this operation.
		 */
		virtual
		void
		activate() = 0;

		/**
		 * Deactivate this operation.
		 */
		virtual
		void
		deactivate() = 0;


	Q_SIGNALS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * The point at index @a point_index was in the geometry at
		 * index @a geometry_index in the geometry builder @a geometry_builder
		 * was highlighted by this geometry operation.
		 */
		void
		highlight_point_in_geometry(
				GPlatesViewOperations::GeometryBuilder *geometry_builder,
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesGui::Colour &highlight_colour);

		/**
		 * The point at index @a point_index was in the geometry at
		 * index @a geometry_index in the geometry builder @a geometry_builder
		 * was unhighlighted by this geometry operation.
		 */
		void
		unhighlight_point_in_geometry(
				GPlatesViewOperations::GeometryBuilder *geometry_builder,
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index);

	protected:
		//! Constructor.
		GeometryOperation() : d_point_is_highlighted(false) {  }


		/**
		 * If point is not currently highlighted then emit a highlight signal to listeners.
		 */
		void
		emit_highlight_point_signal(
				GPlatesViewOperations::GeometryBuilder *geometry_builder,
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesGui::Colour &highlight_colour);

		/**
		 * If point is currently highlighted then emit a unhighlight signal to listeners.
		 */
		void
		emit_unhighlight_signal(
				GPlatesViewOperations::GeometryBuilder *geometry_builder);


	private:
		//! Is a point currently highlighted in this @a GeometryOperation.
		bool d_point_is_highlighted;

		//
		// Parameters used in last highlight point signal.
		//

		GPlatesViewOperations::GeometryBuilder::GeometryIndex d_highlight_geometry_index;
		GPlatesViewOperations::GeometryBuilder::PointIndex d_highlight_point_index;
	};
}

#endif // GPLATES_VIEWOPERATIONS_GEOMETRYOPERATION_H
