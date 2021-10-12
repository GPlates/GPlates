/* $Id$ */

/**
 * \file 
 * Implementation interface for rendered geometries.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYIMPL_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYIMPL_H

#include "maths/ProximityHitDetail.h"


namespace GPlatesMaths
{
	class ProximityCriteria;
}

namespace GPlatesViewOperations
{
	class ConstRenderedGeometryVisitor;

	/**
	 * The interface for the implementation of @a RenderedGeometry.
	 */
	class RenderedGeometryImpl
	{
	public:
		virtual
		~RenderedGeometryImpl()
		{ }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor&) = 0;

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const = 0;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYIMPL_H
