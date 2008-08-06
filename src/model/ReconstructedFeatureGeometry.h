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

#include <boost/optional.hpp>
#include "ReconstructionGeometry.h"
#include "types.h"
#include "FeatureHandle.h"
#include "property-values/GeoTimeInstant.h"


namespace GPlatesModel
{
	class ReconstructedFeatureGeometry:
			public ReconstructionGeometry
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<ReconstructedFeatureGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedFeatureGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const ReconstructedFeatureGeometry,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedFeatureGeometry,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructedFeatureGeometry>.
		 */
		typedef boost::intrusive_ptr<ReconstructedFeatureGeometry> maybe_null_ptr_type;

		/**
		 * Create a ReconstructedFeatureGeometry instance with a reconstruction plate ID.
		 */
		static
		const non_null_ptr_type
		create(
				geometry_ptr_type geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_,
				integer_plate_id_type reconstruction_plate_id_)
		{
			non_null_ptr_type ptr(
					new ReconstructedFeatureGeometry(geometry_ptr, feature_handle,
							property_iterator_, reconstruction_plate_id_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Create a ReconstructedFeatureGeometry instance @em without a reconstruction
		 * plate ID.
		 *
		 * For instance, a ReconstructedFeatureGeometry might be created without a
		 * reconstruction plate ID if no reconstruction plate ID is found amongst the
		 * properties of the feature being reconstructed, but the client code still wants
		 * to "reconstruct" the geometries of the feature using the identity rotation.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				geometry_ptr_type geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_)
		{
			non_null_ptr_type ptr(
					new ReconstructedFeatureGeometry(geometry_ptr, feature_handle,
							property_iterator_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		~ReconstructedFeatureGeometry()
		{  }

		/**
		 * Get a non-null pointer to a ReconstructedFeatureGeometry which points to this
		 * instance.
		 *
		 * Since the ReconstructedFeatureGeometry constructors are private, it should never
		 * be the case that a ReconstructedFeatureGeometry instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer();

		/**
		 * Access (a reference to) the feature whose reconstructed geometry this RFG
		 * contains.
		 */
		const FeatureHandle::weak_ref
		feature_ref() const
		{
			return d_feature_ref;
		}

		/**
		 * Access the feature property which contained the reconstructed geometry.
		 */
		const FeatureHandle::properties_iterator
		property() const
		{
			return d_property_iterator;
		}

		/**
		 * Access the cached reconstruction plate ID, if it exists.
		 *
		 * Note that it's possible for a ReconstructedFeatureGeometry to be created without
		 * a reconstruction plate ID -- for example, if no reconstruction plate ID is found
		 * amongst the properties of the feature being reconstructed, but the client code
		 * still wants to "reconstruct" the geometries of the feature using the identity
		 * rotation.
		 */
		const boost::optional<integer_plate_id_type> &
		reconstruction_plate_id() const
		{
			return d_reconstruction_plate_id;
		}

		/**
		 * Return the cached feature time.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		reconstruction_feature_time() const
		{
			return d_reconstruction_feature_time;
		}

		/**
		 * Cache feature time
		 */
		void 
		set_reconstruction_feature_time(const GPlatesPropertyValues::GeoTimeInstant &feature_time) const
		{
			d_reconstruction_feature_time = feature_time;				
		} 


		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

	private:

		/**
		 * This is a weak-ref to the feature, of which this RFG is a reconstruction.
		 */
		FeatureHandle::weak_ref d_feature_ref;
		
		/**
		 * This is an iterator to the (geometry-valued) property from which this RFG was
		 * derived.
		 */
		FeatureHandle::properties_iterator d_property_iterator;

		/**
		 * The cached reconstruction plate ID, if it exists.
		 *
		 * Note that it's possible for a ReconstructedFeatureGeometry to be created without
		 * a reconstruction plate ID -- for example, if no reconstruction plate ID is found
		 * amongst the properties of the feature being reconstructed, but the client code
		 * still wants to "reconstruct" the geometries of the feature using the identity
		 * rotation.
		 *
		 * We cache the plate ID here so that it can be extracted by drawing code for use
		 * with on-screen colouring, and accessed quickly when RFGs are being queried.
		 */
		boost::optional<integer_plate_id_type> d_reconstruction_plate_id;

		/**
		 * We cache the feature time so that it can be extracted by drawing code
		 * for use with on-screen colouring.
		 */
		mutable boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_reconstruction_feature_time;

		/**
		 * Instantiate a reconstructed feature geometry with a reconstruction plate ID.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedFeatureGeometry(
				geometry_ptr_type geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_,
				integer_plate_id_type reconstruction_plate_id_):
			ReconstructionGeometry(geometry_ptr),
			d_feature_ref(feature_handle.reference()),
			d_property_iterator(property_iterator_),
			d_reconstruction_plate_id(reconstruction_plate_id_)
		{  }

		/**
		 * Instantiate a reconstructed feature geometry @em without a reconstruction plate
		 * ID.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedFeatureGeometry(
				geometry_ptr_type geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_):
			ReconstructionGeometry(geometry_ptr),
			d_feature_ref(feature_handle.reference()),
			d_property_iterator(property_iterator_)
		{  }

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		ReconstructedFeatureGeometry(
				const ReconstructedFeatureGeometry &other);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive-pointer to another.
		ReconstructedFeatureGeometry &
		operator=(
				const ReconstructedFeatureGeometry &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const ReconstructedFeatureGeometry *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const ReconstructedFeatureGeometry *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H
