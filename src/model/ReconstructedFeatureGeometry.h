/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H
#define GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H

#include "types.h"
#include "FeatureHandle.h"
#include "utils/non_null_intrusive_ptr.h"
#include <boost/optional.hpp>


namespace GPlatesModel
{
	// Forward declaration to avoid circularity of headers.
	class Reconstruction;

	template<class T>
	class ReconstructedFeatureGeometry
	{
	public:
		typedef T geometry_type;

	private:
		GPlatesUtils::non_null_intrusive_ptr<const geometry_type> d_geometry_ptr;

		/**
		 * This is a weak-ref to the feature, of which this RFG is a reconstruction.
		 */
		FeatureHandle::weak_ref d_feature_ref;
		
		/**
		 * This is an iterator to the (geometry-valued) property that this RFG was
		 * derived from.
		 */
		FeatureHandle::properties_iterator d_property_iterator;

		/**
		 * This is the Reconstruction instance which contains this RFG.
		 *
		 * Note that we do NOT want this to be any sort of ref-counting pointer, since the
		 * Reconstruction instance which contains this RFG, does so using a ref-counting
		 * pointer; circularity of ref-counting pointers would lead to memory leaks.
		 *
		 * Note that this pointer may be NULL.
		 *
		 * This pointer should only @em ever point to a Reconstruction instance which
		 * @em does contain this RFG inside its vector.  (This is the only way we can
		 * guarantee that the Reconstruction instance actually exists, ie that the pointer
		 * is not a dangling pointer.)
		 */
		Reconstruction *d_reconstruction_ptr;

		/**
		 * We cache the plate id here so that it can be extracted by drawing code
		 * for use with on-screen colouring.
		 */
		boost::optional<integer_plate_id_type> d_reconstruction_plate_id;

	public:
		ReconstructedFeatureGeometry(
				GPlatesUtils::non_null_intrusive_ptr<const geometry_type> geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_,
				integer_plate_id_type reconstruction_plate_id_):
			d_geometry_ptr(geometry_ptr),
			d_feature_ref(feature_handle.reference()),
			d_property_iterator(property_iterator_),
			d_reconstruction_ptr(NULL),
			d_reconstruction_plate_id(reconstruction_plate_id_)
		{  }

		ReconstructedFeatureGeometry(
				GPlatesUtils::non_null_intrusive_ptr<const geometry_type> geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_):
			d_geometry_ptr(geometry_ptr),
			d_feature_ref(feature_handle.reference()),
			d_property_iterator(property_iterator_),
			d_reconstruction_ptr(NULL)
		{  }

		/**
		 * Copy-constructor.
		 */
		ReconstructedFeatureGeometry(
				const ReconstructedFeatureGeometry &other):
			d_geometry_ptr(other.d_geometry_ptr),
			d_feature_ref(other.d_feature_ref),
			d_property_iterator(other.d_property_iterator),
			// Need to copy the reconstruction-ptr, or the reconstruction-ptr will be
			// wiped whenever copy-construction occurs.  Copy-construction occurs, for
			// example, when a vector is resized...
			d_reconstruction_ptr(other.d_reconstruction_ptr),
			d_reconstruction_plate_id(other.d_reconstruction_plate_id)
		{  }

		/**
		 * Copy-assignment operator.
		 *
		 * Note that this function does NOT actually do an exact copy:  Specifically, it
		 * does not copy the reconstruction-ptr member from 'other' to 'this' -- instead,
		 * the reconstruction-ptr is left untouched.
		 *
		 * The reasoning:  Either this RFG is contained within (the vector within) a
		 * Reconstruction instance, in which case its reconstruction-ptr should @em still
		 * point to the enclosing Reconstruction instance (We'll assume that the
		 * reconstruction-ptr has been set appropriately, using @a set_reconstruction_ptr),
		 * @em or this RFG is @em not contained within a Reconstruction instance, in which
		 * case its reconstruction-ptr should still be NULL.
		 */
		ReconstructedFeatureGeometry &
		operator=(
				const ReconstructedFeatureGeometry &other)
		{
			d_geometry_ptr = other.d_geometry_ptr;
			d_feature_ref = other.d_feature_ref;
			d_property_iterator = other.d_property_iterator;
			// Leave 'd_reconstruction_ptr' untouched.
			d_reconstruction_plate_id = other.d_reconstruction_plate_id;

			return *this;
		}

		const GPlatesUtils::non_null_intrusive_ptr<const geometry_type>
		geometry() const
		{
			return d_geometry_ptr;
		}

		const FeatureHandle::weak_ref
		feature_ref() const
		{
			return d_feature_ref;
		}

		const FeatureHandle::properties_iterator
		property() const
		{
			return d_property_iterator;
		}

		/**
		 * Return the cached plate id.
		 */
		const boost::optional<integer_plate_id_type> &
		reconstruction_plate_id() const
		{
			return d_reconstruction_plate_id;
		}

		/**
		 * Access the Reconstruction instance which contains this RFG.
		 *
		 * This could be used, for instance, to access the ReconstructionTree which was
		 * used to reconstruct this RFG.
		 *
		 * Note that this pointer may be NULL.
		 */
		const Reconstruction *
		reconstruction() const
		{
			return d_reconstruction_ptr;
		}

		/**
		 * Access the Reconstruction instance which contains this RFG.
		 *
		 * This could be used, for instance, to access the ReconstructionTree which was
		 * used to reconstruct this RFG.
		 *
		 * Note that this pointer may be NULL.
		 */
		Reconstruction *
		reconstruction()
		{
			return d_reconstruction_ptr;
		}

		/**
		 * Set the reconstruction pointer.
		 *
		 * This function is intended to be invoked @em only when the RFG is sitting in the
		 * vector inside the Reconstruction instance, since even a copy-construction will
		 * reset the value of the reconstruction pointer back to NULL.
		 *
		 * WARNING:  This function should only be invoked by the code which is actually
		 * assigning an RFG instance into (the vector inside) a Reconstruction instance.
		 */
		void
		set_reconstruction_ptr(
				Reconstruction *reconstruction_ptr)
		{
			d_reconstruction_ptr = reconstruction_ptr;
		}
	};
}

#endif  // GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H
