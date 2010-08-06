/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_APP_LOGIC_RESOLVEDRASTER_H
#define GPLATES_APP_LOGIC_RESOLVEDRASTER_H

#include "ReconstructionGeometry.h"

#include "model/FeatureHandle.h"
#include "model/types.h"
#include "model/WeakObserver.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRaster.h"


namespace GPlatesAppLogic
{
	/**
	 * A type of @a ReconstructionGeometry representing a raster.
	 *
	 * Used to represent a constant or time-dependent raster, but not
	 * a reconstructed raster (a raster divided into polygons where each polygon
	 * is rotated independently according to its plate id).
	 */
	class ResolvedRaster :
			public ReconstructionGeometry
			// FIXME: Add this back when raster is a property/band of a feature
			//public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		/**
		 * A convenience typedef for a shared pointer to a non-const @a ResolvedRaster.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedRaster> non_null_ptr_type;

		/**
		 * A convenience typedef for a shared pointer to a non-const @a ResolvedRaster.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedRaster> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the WeakObserver base class of this class.
		 */
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;


		/**
		 * Create a @a ResolvedRaster.
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster
				// FIXME: Add this back when raster is a property/band of a feature
// 				GPlatesModel::FeatureHandle &feature_handle,
// 				GPlatesModel::FeatureHandle::iterator property_iterator_,
		)
		{
			return non_null_ptr_type(
					new ResolvedRaster(reconstruction_tree_, georeferencing, raster),
					GPlatesUtils::NullIntrusivePointerHandler());
		}


		/**
		 * Returns the georeferencing parameters to position the raster onto the globe.
		 */
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &
		get_georeferencing() const
		{
			return d_georeferencing;
		}


		/**
		 * Returns the raster data.
		 */
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &
		get_raster() const
		{
			return d_raster;
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		// FIXME: Add this back when ResolvedRaster becomes a weak observer
		// (which is when raster is a property/band of a feature).
#if 0
		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);
#endif

	protected:
		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ResolvedRaster(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster) :
			ReconstructionGeometry(reconstruction_tree_),
			d_georeferencing(georeferencing),
			d_raster(raster)
		{  }

	private:
		/**
		 * The georeferencing parameters to position the raster onto the globe.
		 */
		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type d_georeferencing;

		/**
		 * The raster data.
		 */
		GPlatesPropertyValues::RawRaster::non_null_ptr_type d_raster;
	};
}

#endif // GPLATES_APP_LOGIC_RESOLVEDRASTER_H
