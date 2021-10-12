/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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
#include "WeakObserver.h"
#include "types.h"
#include "FeatureHandle.h"
#include "property-values/GeoTimeInstant.h"


namespace GPlatesModel
{
	class ReconstructedFeatureGeometry:
			public ReconstructionGeometry,
			public WeakObserver<FeatureHandle>
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
		 * A convenience typedef for the WeakObserver base class of this class.
		 */
		typedef WeakObserver<FeatureHandle> WeakObserverType;

		/**
		 * Create a ReconstructedFeatureGeometry instance with an optional reconstruction
		 * plate ID and an optional time of formation.
		 */
		static
		const non_null_ptr_type
		create(
				geometry_ptr_type geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_,
				boost::optional<integer_plate_id_type> reconstruction_plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_)
		{
			non_null_ptr_type ptr(
					new ReconstructedFeatureGeometry(geometry_ptr, feature_handle,
							property_iterator_, reconstruction_plate_id_,
							time_of_formation_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Create a ReconstructedFeatureGeometry instance @em without a reconstruction
		 * plate ID or a feature formation time.
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
		 * Return whether this RFG references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const FeatureHandle &that_feature_handle) const
		{
			return (feature_handle_ptr() == &that_feature_handle);
		}

		/**
		 * Return the pointer to the FeatureHandle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a
		 * FeatureHandle; non-NULL otherwise.
		 *
		 * This function will not throw.
		 */
		FeatureHandle *
		feature_handle_ptr() const
		{
			return WeakObserverType::publisher_ptr();
		}

		/**
		 * Return whether this pointer is valid to be dereferenced (to obtain a
		 * FeatureHandle).
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (feature_handle_ptr() != NULL);
		}

		/**
		 * Return a weak-ref to the feature whose reconstructed geometry this RFG contains,
		 * or an invalid weak-ref, if this pointer is not valid to be dereferenced.
		 */
		const FeatureHandle::weak_ref
		get_feature_ref() const;

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
		 * Return the cached time of formation of the feature.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		time_of_formation() const
		{
			return d_time_of_formation;
		}

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				WeakObserverVisitor<FeatureHandle> &visitor);

	private:

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
		 * The reconstruction plate ID is used when colouring feature geometries by plate
		 * ID.  It's also of interest to a user who has clicked on the feature geometry.
		 */
		boost::optional<integer_plate_id_type> d_reconstruction_plate_id;

		/**
		 * The cached time of formation of the feature, if it exists.
		 *
		 * This is cached so that it can be used to calculate the age of the feature at any
		 * particular reconstruction time.  The age of the feature is used when colouring
		 * feature geometries by age.
		 */
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_formation;

		/**
		 * Instantiate a reconstructed feature geometry with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedFeatureGeometry(
				geometry_ptr_type geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_,
				boost::optional<integer_plate_id_type> reconstruction_plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_):
			ReconstructionGeometry(geometry_ptr),
			WeakObserverType(feature_handle),
			d_property_iterator(property_iterator_),
			d_reconstruction_plate_id(reconstruction_plate_id_),
			d_time_of_formation(time_of_formation_)
		{  }

		/**
		 * Instantiate a reconstructed feature geometry @em without a reconstruction plate
		 * ID or a cached feature formation time.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedFeatureGeometry(
				geometry_ptr_type geometry_ptr,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_):
			ReconstructionGeometry(geometry_ptr),
			WeakObserverType(feature_handle),
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
}

#endif  // GPLATES_MODEL_RECONSTRUCTEDFEATUREGEOMETRY_H
