/* $Id$ */

/**
 * @file 
 * Contains the definition of the MapTransform class.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_MAPTRANSFORM_H
#define GPLATES_GUI_MAPTRANSFORM_H

#include <QObject>
#include <QPointF>

namespace GPlatesGui
{
	/**
	 * This class centralises map transformations (e.g. translations, rotations)
	 * so that all MapView instances can be notified of transformations.
	 */
	class MapTransform :
		public QObject
	{

		Q_OBJECT

	public:

		MapTransform();

		void
		translate_maps(
				qreal dx, qreal dy);

		void
		rotate_maps(
				double angle);

		void
		reset_rotation();

		qreal
		get_total_translation_x() const;

		qreal
		get_total_translation_y() const;

		double
		get_total_rotation() const;

		void
		set_centre_of_viewport(
				const QPointF &centre_of_viewport);

		const QPointF &
		get_centre_of_viewport() const;

	signals:

		void
		translate(
				qreal dx, qreal dy);

		void
		rotate(
				double angle);

	private:

		//! Sum of all translations in the x direction.
		qreal d_total_translation_x;

		//! Sum of all translations in the y direction.
		qreal d_total_translation_y;

		//! Sum of all rotation angles.
		double d_total_rotation;

		//! Coordinates (in scene coords) of the centre of the visible viewport.
		QPointF d_centre_of_viewport;

	};
}

#endif  /* GPLATES_GUI_MAPTRANSFORM_H */
