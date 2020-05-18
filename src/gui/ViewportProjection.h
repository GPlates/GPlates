/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_VIEWPORTPROJECTION_H
#define GPLATES_GUI_VIEWPORTPROJECTION_H

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QObject>

#include "GlobeProjectionType.h"
#include "MapProjection.h"

namespace GPlatesGui
{
	/**
	 * A central place to set view projection and listens for changes.
	 */
	class ViewportProjection :
			public QObject
	{

		Q_OBJECT
		
	public:

		//! Typedef for a globe or map projection.
		typedef boost::variant<GlobeProjection::Type, MapProjection::Type> projection_type;


		explicit
		ViewportProjection(
				GlobeProjection::Type globe_projection_type) :
			d_projection_type(globe_projection_type),
			d_map_central_meridian(0.)
		{  }

		explicit
		ViewportProjection(
				MapProjection::Type map_projection_type) :
			d_projection_type(map_projection_type),
			d_map_central_meridian(0.)
		{  }


		//! Set projection type and notify any listeners.
		void
		set_projection_type(
				projection_type projection_type_)
		{
			Q_EMIT projection_type_about_to_change(*this);

			d_projection_type = projection_type_;

			Q_EMIT projection_type_changed(*this);
		}

		//! Set globe projection type and notify any listeners.
		void
		set_globe_projection_type(
				GlobeProjection::Type globe_projection_type)
		{
			Q_EMIT projection_type_about_to_change(*this);

			d_projection_type = globe_projection_type;

			Q_EMIT projection_type_changed(*this);
		}

		//! Set map projection type and notify any listeners.
		void
		set_map_projection_type(
				MapProjection::Type map_projection_type)
		{
			Q_EMIT projection_type_about_to_change(*this);

			d_projection_type = map_projection_type;

			Q_EMIT projection_type_changed(*this);
		}

		//! Set central meridian and notify any listeners.
		void
		set_map_central_meridian(
				const double &map_central_meridian)
		{
			Q_EMIT central_meridian_about_to_change(*this);

			d_map_central_meridian = map_central_meridian;

			Q_EMIT central_meridian_changed(*this);
		}


		projection_type
		get_projection_type() const
		{
			return d_projection_type;
		}

		//! Returns globe projection type (or none if a map projection type).
		boost::optional<GlobeProjection::Type>
		get_globe_projection_type() const
		{
			if (const GlobeProjection::Type *globe_projection_type =
				boost::get<GlobeProjection::Type>(&d_projection_type))
			{
				return *globe_projection_type;
			}

			return boost::none;
		}

		//! Returns map projection type (or none if a globe projection type).
		boost::optional<MapProjection::Type>
		get_map_projection_type() const
		{
			if (const MapProjection::Type *map_projection_type =
				boost::get<MapProjection::Type>(&d_projection_type))
			{
				return *map_projection_type;
			}

			return boost::none;
		}


		const double &
		get_map_central_meridian() const
		{
			return d_map_central_meridian;
		}


	Q_SIGNALS:
		void
		projection_type_about_to_change(
				const GPlatesGui::ViewportProjection &viewport_projection);

		void
		projection_type_changed(
				const GPlatesGui::ViewportProjection &viewport_projection);

		void
		central_meridian_changed(
				const GPlatesGui::ViewportProjection &viewport_projection);

		void
		central_meridian_about_to_change(
				const GPlatesGui::ViewportProjection &viewport_projection);

	private:
		projection_type d_projection_type;
		double d_map_central_meridian;
	};
}

#endif // GPLATES_GUI_VIEWPORTPROJECTION_H
