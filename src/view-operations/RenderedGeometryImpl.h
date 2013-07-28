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
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


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
	class RenderedGeometryImpl :
			public GPlatesUtils::ReferenceCount<RenderedGeometryImpl>
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a RenderedGeometryImpl.
		typedef GPlatesUtils::non_null_intrusive_ptr<RenderedGeometryImpl> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a RenderedGeometryImpl.
		typedef GPlatesUtils::non_null_intrusive_ptr<const RenderedGeometryImpl> non_null_ptr_to_const_type;

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
				
		/**
		 * Default implementation returns no hit since this method probably only
		 * makes sense for rendered geometries that wrap @a GeometryOnSphere types
		 * where interest in vertex proximity occurs.
		 */
		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return GPlatesMaths::ProximityHitDetail::null;
		}		
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYIMPL_H
