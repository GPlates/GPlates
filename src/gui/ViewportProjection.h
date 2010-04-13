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

#include <QObject>

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

		explicit
		ViewportProjection(
				GPlatesGui::ProjectionType projection_type) :
			d_projection_type(projection_type),
			d_central_meridian(0.)
		{  }


		//! Set projection type and notify any listeners.
		void
		set_projection_type(
				GPlatesGui::ProjectionType projection_type)
		{
			d_projection_type = projection_type;

			emit projection_type_changed(*this);
		}


		//! Set central meridian and notify any listeners.
		void
		set_central_meridian(
				const double &central_meridian)
		{
			d_central_meridian = central_meridian;

			emit central_meridian_changed(*this);
		}


		GPlatesGui::ProjectionType
		get_projection_type() const
		{
			return d_projection_type;
		}


		const double &
		get_central_meridian() const
		{
			return d_central_meridian;
		}


	signals:
		void
		projection_type_changed(
				const GPlatesGui::ViewportProjection &viewport_projection);

		void
		central_meridian_changed(
				const GPlatesGui::ViewportProjection &viewport_projection);

	private:
		GPlatesGui::ProjectionType d_projection_type;
		double d_central_meridian;
	};
}

#endif // GPLATES_GUI_VIEWPORTPROJECTION_H
