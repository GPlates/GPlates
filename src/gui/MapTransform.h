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
	// Forward declaration.
	class ViewportZoom;

	/**
	 * This class encapsulates the current state of the map view in terms of the
	 * centre of the viewport and the angle of rotation. For convenience, this
	 * class also forwards the current zoom from ViewportZoom.
	 */
	class MapTransform :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * Typedef for a point in 2D space.
		 */
		typedef QPointF point_type;

		/**
		 * Constructs a MapTransform that wraps around the given @a viewport_zoom.
		 */
		explicit
		MapTransform(
				ViewportZoom &viewport_zoom);

		/**
		 * Returns the centre of the map viewport in scene coordinates.
		 * Scene coordinates are what you have after projection from lat-lon, but
		 * before conversion into window coordinates.
		 */
		const point_type &
		get_centre_of_viewport() const
		{
			return d_centre_of_viewport;
		}

		/**
		 * Sets the centre of the map viewport in scene coordinates.
		 * Scene coordinates are what you have after projection from lat-lon, but
		 * before conversion into window coordinates.
		 *
		 * If the new centre of viewport is outside of @a MIN_CENTRE_OF_VIEWPORT_X,
		 * @a MAX_CENTRE_OF_VIEWPORT_X, @a MIN_CENTRE_OF_VIEWPORT_Y, and
		 * @a MAX_CENTRE_OF_VIEWPORT_Y, it is not set.
		 */
		void
		set_centre_of_viewport(
				const point_type &centre_of_viewport);

		/**
		 * Translates the centre of viewport by @a dx and @a dy, which are expressed
		 * in scene coordinates. Note that the translation is irrespective of the
		 * current angle of rotation.
		 *
		 * If the new centre of viewport is outside of @a MIN_CENTRE_OF_VIEWPORT_X,
		 * @a MAX_CENTRE_OF_VIEWPORT_X, @a MIN_CENTRE_OF_VIEWPORT_Y, and
		 * @a MAX_CENTRE_OF_VIEWPORT_Y, it is not set.
		 */
		void
		translate(
				double dx,
				double dy);

		/**
		 * Returns the angle of rotation of the map viewport in degrees.
		 */
		double
		get_rotation() const
		{
			return d_rotation;
		}

		/**
		 * Sets the angle of rotation of the map viewport in degrees.
		 */
		void
		set_rotation(
				double rotation);

		/**
		 * Rotates the viewport by @a angle in degrees.
		 */
		void
		rotate(
				double angle);

		/**
		 * Returns the current zoom factor.
		 */
		double
		get_zoom_factor() const;

		/**
		 * The smallest value in the x dimension permitted for the centre of viewport,
		 * in scene coordinates.
		 */
		static const double MIN_CENTRE_OF_VIEWPORT_X;

		/**
		 * The largest value in the x dimension permitted for the centre of viewport,
		 * in scene coordinates.
		 */
		static const double MAX_CENTRE_OF_VIEWPORT_X;

		/**
		 * The smallest value in the y dimension permitted for the centre of viewport,
		 * in scene coordinates.
		 */
		static const double MIN_CENTRE_OF_VIEWPORT_Y;

		/**
		 * The largest value in the y dimension permitted for the centre of viewport,
		 * in scene coordinates.
		 */
		static const double MAX_CENTRE_OF_VIEWPORT_Y;

	signals:

		/**
		 * Emitted when the centre of viewport, the rotation or the zoom factor is changed.
		 */
		void
		transform_changed(
				const GPlatesGui::MapTransform &map_transform);

	private slots:

		void
		handle_zoom_changed();
	
	private:

		ViewportZoom &d_viewport_zoom;
		point_type d_centre_of_viewport;
		double d_rotation;
	};
}

#endif  // GPLATES_GUI_MAPTRANSFORM_H
