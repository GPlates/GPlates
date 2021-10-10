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

#ifndef GPLATES_GUI_TEMPORARYGEOMETRYFOCUS_H
#define GPLATES_GUI_TEMPORARYGEOMETRYFOCUS_H

#include <QObject>
#include "model/TemporaryGeometry.h"


namespace GPlatesGui
{
	/**
	 * This class is used to store the notion of which temporary geometry currently has the
	 * focus.
	 *
	 * Anything interested in displaying the currently-focused temporary geometry can listen to
	 * events emitted from here.
	 */
	class TemporaryGeometryFocus:
			public QObject
	{
		Q_OBJECT
	public:
		
		TemporaryGeometryFocus()
		{  }

		virtual
		~TemporaryGeometryFocus()
		{  }

		/**
		 * Accessor for the currently-focused temporary geometry (if there is one).
		 */
		GPlatesModel::TemporaryGeometry::maybe_null_ptr_type
		focused_temporary_geometry()
		{
			return d_focused_geometry;
		}

	public slots:

		/**
		 * Change which temporary geometry is currently focused.
		 *
		 * Will emit focus_changed() to anyone who cares, provided that @a new_geometry
		 * is actually different to the previous temporary geometry.
		 */
		void
		set_focus(
				GPlatesModel::TemporaryGeometry::non_null_ptr_type new_geometry);

		/**
		 * Clear the focus.
		 *
		 * Future calls to focused_temporary_geometry() will return a NULL pointer.
		 * Will emit focus_changed() to anyone who cares.
		 */
		void
		unset_focus();

		/**
		 * Call this method when you have modified the properties of the currently-focused
		 * temporary geometry.
		 *
		 * TemporaryGeometryFocus will emit signals to notify anyone who needs to track
		 * modifications to the currently-focused temporary geometry.
		 */
		void
		announce_modfication_of_focused_geometry();

	signals:

		/**
		 * Emitted when a new temporary geometry has been clicked on, or the current focus
		 * has been cleared.
		 */
		void
		focus_changed(
				GPlatesModel::TemporaryGeometry::maybe_null_ptr_type);

		/**
		 * Emitted when the currently-focused temporary geometry has been modified.
		 */
		void
		focused_geometry_modified(
				GPlatesModel::TemporaryGeometry::maybe_null_ptr_type);	

	private:

		/**
		 * The currently-focused temporary geometry.
		 *
		 * Note that there might not be any currently-focused temporary geometry, in which
		 * case this would be a null pointer.
		 */
		GPlatesModel::TemporaryGeometry::maybe_null_ptr_type d_focused_geometry;

	};
}

#endif // GPLATES_GUI_TEMPORARYGEOMETRYFOCUS_H
