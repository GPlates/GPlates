/* $Id$ */

/**
 * \file 
 * Pimpl interface class for all rendered geometry.
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

#ifndef GPLATES_GUI_RENDEREDGEOMETRY_H
#define GPLATES_GUI_RENDEREDGEOMETRY_H

#include <boost/shared_ptr.hpp>

#include "maths/ProximityHitDetail.h"


namespace GPlatesMaths
{
	class ProximityCriteria;
}

namespace GPlatesViewOperations
{
	class ConstRenderedGeometryVisitor;
	class RenderedGeometryImpl;

	/**
	 * This class describes a geometry which has been rendered for display.
	 */
	class RenderedGeometry
	{
	public:
		/**
		 * Typedef for pointer to a @a RenderedGeometry implementation.
		 */
		typedef boost::shared_ptr<RenderedGeometryImpl> impl_ptr_type;

		/**
		 * Creates a @a RenderedGeometry object that has no implementation.
		 */
		RenderedGeometry()
		{  }

		/**
		 * Creates a @a RenderedGeometry with specified implementation.
		 *
		 * Typically only used by @a RenderedGeometryFactory.
		 * Once the implementation is attached here it can only be accessed
		 * with the @a accept_visitor() method.
		 */
		RenderedGeometry(
				impl_ptr_type);

		/**
		 * Visit the rendered geometry implementation type.
		 *
		 * If have no implementation to visit (see default constructor) then nothing happens.
		 *
		 * The only way to access the RenderedGeometry implementation
		 * is to visit it and since all visitor methods reference const
		 * implementation objects there is no way to modify a @a RenderedGeometry
		 * implementation object once it is created making it effectively immutable
		 * (unless you subversively keep your own reference to the implementation).
		 */
		void
		accept_visitor(
				ConstRenderedGeometryVisitor&) const;

		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const;

	private:
		/**
		 * Pimpl idiom: pointer to implementation interface.
		 */
		impl_ptr_type d_impl;
	};
}

#endif // GPLATES_GUI_RENDEREDGEOMETRY_H
