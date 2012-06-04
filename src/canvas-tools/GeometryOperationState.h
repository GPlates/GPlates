/* $Id$ */

/**
 * \file Notifies listeners when sole active @a GeometryOperation changes.
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

#ifndef GPLATES_CANVASTOOLS_GEOMETRYOPERATIONSTATE_H
#define GPLATES_CANVASTOOLS_GEOMETRYOPERATIONSTATE_H

#include <QObject>


namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class GeometryOperation;
}

namespace GPlatesCanvasTools
{
	/**
	 * Keeps track of which @a GeometryOperation is currently active and
	 * which @a GeometryBuilder contains the geometry.
	 *
	 * This is used to let @a ModifyGeometryWidget and @a DigitisationWidget, in the task panel,
	 * deal with several canvas tools that can modify either digitised or focused feature geometry.
	 *
	 * Only one geometry operation is active at any time.
	 */
	class GeometryOperationState :
			public QObject
	{
		Q_OBJECT

	public:
		GeometryOperationState() :
			d_active_geometry_operation(NULL),
			d_active_geometry_builder(NULL)
		{  }


		/**
		 * The newly activated @a GeometryOperation calls this to indicate it's active.
		 * If the active @a GeometryOperation has switched then notify listeners
		 * of our @a switched_geometry_operation signal.
		 */
		void
		set_active_geometry_operation(
				GPlatesViewOperations::GeometryOperation *geometry_operation)
		{
			// If we're activating a new geometry operation then emit a signal.
			if (geometry_operation != d_active_geometry_operation)
			{
				d_active_geometry_operation = geometry_operation;

				emit switched_geometry_operation(geometry_operation);
			}
		}


		/**
		 * Since only one @a GeometryOperation is active at any time this
		 * method let's listeners know that there's currently no active @a GeometryOperation.
		 * This method should be called by a @a GeometryOperation derived class.
		 */
		void
		set_no_active_geometry_operation()
		{
			// If we're activating a new geometry operation then emit a signal.
			if (NULL != d_active_geometry_operation)
			{
				d_active_geometry_operation = NULL;

				emit switched_geometry_operation(NULL);
			}
		}


		/**
		 * The newly activated @a GeometryBuilder calls this to indicate it's active.
		 * If the active @a GeometryBuilder has switched then notify listeners
		 * of our @a switched_geometry_builder signal.
		 */
		void
		set_active_geometry_builder(
				GPlatesViewOperations::GeometryBuilder *geometry_builder)
		{
			// If we're activating a new geometry builder then emit a signal.
			if (geometry_builder != d_active_geometry_builder)
			{
				d_active_geometry_builder = geometry_builder;

				emit switched_geometry_builder(geometry_builder);
			}
		}


		/**
		 * Since only one @a GeometryBuilder is active at any time this
		 * method let's listeners know that there's currently no active @a GeometryBuilder.
		 * This method should be called by a @a GeometryBuilder derived class.
		 */
		void
		set_no_active_geometry_builder()
		{
			// If we're activating a new geometry builder then emit a signal.
			if (NULL != d_active_geometry_builder)
			{
				d_active_geometry_builder = NULL;

				emit switched_geometry_builder(NULL);
			}
		}


	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * The geometry operation emitting signals has changed.
		 * Only one geometry operation is active as any time.
		 * @a geometry_operation is NULL if no @a GeometryOperation is currently activated.
		 */
		void
		switched_geometry_operation(
				GPlatesViewOperations::GeometryOperation *geometry_operation);

		/**
		 * The geometry builder emitting signals has changed.
		 * Only one geometry builder is active as any time.
		 * @a geometry_builder is NULL if no @a GeometryBuilder is currently activated.
		 */
		void
		switched_geometry_builder(
				GPlatesViewOperations::GeometryBuilder *geometry_builder);


	private:
		GPlatesViewOperations::GeometryOperation *d_active_geometry_operation;
		GPlatesViewOperations::GeometryBuilder *d_active_geometry_builder;
	};
}

#endif // GPLATES_CANVASTOOLS_GEOMETRYOPERATIONSTATE_H
