/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for drawing text.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDSTRING_H
#define GPLATES_VIEWOPERATIONS_RENDEREDSTRING_H

#include <QString>
#include <QFont>

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "gui/ColourProxy.h"

namespace GPlatesViewOperations
{
	class RenderedString :
		public RenderedGeometryImpl
	{
	public:
		RenderedString(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
				const QString &string,
				const GPlatesGui::ColourProxy &colour,
				const GPlatesGui::ColourProxy &shadow_colour,
				int x_offset = 0,
				int y_offset = 0,
				const QFont &font = QFont()) :
		d_point_on_sphere(point_on_sphere),
		d_string(string),
		d_colour(colour),
		d_shadow_colour(shadow_colour),
		d_x_offset(x_offset),
		d_y_offset(y_offset),
		d_font(font)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_string(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return NULL; // we don't want strings to be clickable
		}
		
		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
			const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return NULL; // we don't want strings to be clickable
		}		

		const GPlatesMaths::PointOnSphere &
		get_point_on_sphere() const
		{
			return *d_point_on_sphere;
		}

		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		get_point_on_sphere_ptr() const
		{
			return d_point_on_sphere;
		}

		const QString &
		get_string() const
		{
			return d_string;
		}

		const GPlatesGui::ColourProxy &
		get_colour() const
		{
			return d_colour;
		}

		const GPlatesGui::ColourProxy &
		get_shadow_colour() const
		{
			return d_shadow_colour;
		}

		const QFont &
		get_font() const
		{
			return d_font;
		}

		int
		get_x_offset() const
		{
			return d_x_offset;
		}

		int
		get_y_offset() const
		{
			return d_y_offset;
		}

	private:

		//! Location of text
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type d_point_on_sphere;

		//! Text to display
		QString d_string;

		//! Colour of text
		GPlatesGui::ColourProxy d_colour;

		//! Colour of shadow; set ColourProxy to boost::none if you do not want shadows
		GPlatesGui::ColourProxy d_shadow_colour;

		//! Shifts the text d_x_offset pixels to the right of where it would otherwise be
		int d_x_offset;

		//! Shifts the text d_y_offset pixels above of where it would otherwise be
		int d_y_offset;

		//! Font in which to display the text
		QFont d_font;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDSTRING_H
