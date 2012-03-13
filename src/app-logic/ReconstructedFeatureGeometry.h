/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDFEATUREGEOMETRY_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDFEATUREGEOMETRY_H

#include <boost/optional.hpp>

#include "ReconstructHandle.h"
#include "ReconstructionGeometry.h"
#include "ReconstructMethodFiniteRotation.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/WeakObserver.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry:
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		//! Typedef for a non-null shared pointer to a non-const @a ReconstructedFeatureGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedFeatureGeometry> non_null_ptr_type;

		//! Typedef for a non-null shared pointer to a const @a ReconstructedFeatureGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedFeatureGeometry> non_null_ptr_to_const_type;

		//! Typedef for boost::intrusive_ptr<ReconstructedFeatureGeometry>.
		typedef boost::intrusive_ptr<ReconstructedFeatureGeometry> maybe_null_ptr_type;

		//! Typedef for boost::intrusive_ptr<const ReconstructedFeatureGeometry>.
		typedef boost::intrusive_ptr<const ReconstructedFeatureGeometry> maybe_null_ptr_to_const_type;

		//! Typedef for the WeakObserver base class of this class.
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;

		//! Typedef for a non-null shared pointer to a non-const @a GeometryOnSphere.
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr_type;


		/**
		 * Used to obtain a resolved geometry and its finite rotation transform (reconstruction).
		 *
		 * The finite rotation is currently used when visualising @a ReconstructedFeatureGeometry
		 * objects in the globe view - the finite rotation transform is performed by the graphics hardware.
		 */
		class FiniteRotationReconstruction
		{
		public:
			FiniteRotationReconstruction(
					const geometry_ptr_type &resolved_geometry,
					const ReconstructMethodFiniteRotation::non_null_ptr_to_const_type &reconstruct_method_finite_rotation) :
				d_resolved_geometry(resolved_geometry),
				d_reconstruct_method_finite_rotation(reconstruct_method_finite_rotation)
			{  }

			/**
			 * The resolved feature geometry (this is *unreconstructed* geometry).
			 *
			 * This represents the feature's geometry *before* it is reconstructed.
			 * It is 'resolved' meaning it represents the geometry at the reconstruction time
			 * if the feature geometry property is a time-dependent property.
			 * If the geometry property is constant then it effectively represents present-day geometry.
			 */
			const geometry_ptr_type &
			get_resolved_geometry() const
			{
				return d_resolved_geometry;
			}

			/**
			 * The reconstruct method finite rotation to reconstruct the resolved geometry.
			 *
			 * The returned object can be tested to see if it's a finite rotation transform.
			 */
			const ReconstructMethodFiniteRotation::non_null_ptr_to_const_type &
			get_reconstruct_method_finite_rotation() const
			{
				return d_reconstruct_method_finite_rotation;
			}

			/**
			 * Transforms (reconstructs) the resolved geometry into the reconstructed geometry.
			 */
			geometry_ptr_type
			get_reconstructed_geometry() const
			{
				return d_reconstruct_method_finite_rotation->transform(d_resolved_geometry);
			}

		private:
			geometry_ptr_type d_resolved_geometry;
			ReconstructMethodFiniteRotation::non_null_ptr_to_const_type d_reconstruct_method_finite_rotation;
		};


		/**
		 * Create a ReconstructedFeatureGeometry instance from a *reconstructed* geometry
		 * with an optional reconstruction plate ID and an optional time of formation.
		 *
		 * For instance, a ReconstructedFeatureGeometry might be created without a
		 * reconstruction plate ID if no reconstruction plate ID is found amongst the
		 * properties of the feature being reconstructed, but the client code still wants
		 * to "reconstruct" the geometries of the feature using the identity rotation.
		 * Or some other method (not by plate ID) has been used to "reconstruct".
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				GPlatesModel::FeatureHandle &feature_handle_,
				const GPlatesModel::FeatureHandle::iterator &property_iterator_,
				const geometry_ptr_type &reconstructed_geometry_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new ReconstructedFeatureGeometry(
							reconstruction_tree_,
							feature_handle_,
							property_iterator_,
							reconstructed_geometry_,
							reconstruction_plate_id_,
							time_of_formation_,
							reconstruct_handle_));
		}


		/**
		 * Create a ReconstructedFeatureGeometry instance from an *unreconstructed* geometry and
		 * a reconstruction transform (specific to the particular reconstruct method).
		 * And an optional time of formation.
		 *
		 * This is useful for delaying reconstruction until it's actually need - which is if
		 * the @a reconstructed_geometry method is called. It is also useful for transforming
		 * the geometry on the graphics hardware (where it's much faster) if the reconstruction
		 * transform is a finite rotation - note that only applies to the globe view (not map views).
		 * In the case of transforming on the graphics hardware only the resolved geometry and
		 * the finite rotation are required (the final reconstructed geometry is not required).
		 *
		 * @a resolved_geometry_ the unreconstructed but time-resolved if geometry is
		 * time-dependent. This is the geometry of the feature property at the reconstruction time
		 * which is effectively the present-day geometry (if the geometry property is constant
		 * with time) or a lookup of the geometry in a time-dependent geometry property (if the
		 * geometry is stored in a time-dependent property).
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				GPlatesModel::FeatureHandle &feature_handle_,
				const GPlatesModel::FeatureHandle::iterator &property_iterator_,
				// NOTE: This is the *unreconstructed* geometry...
				const geometry_ptr_type &resolved_geometry_,
				const ReconstructMethodFiniteRotation::non_null_ptr_to_const_type &reconstruct_method_transform_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new ReconstructedFeatureGeometry(
							reconstruction_tree_,
							feature_handle_,
							property_iterator_,
							resolved_geometry_,
							reconstruct_method_transform_,
							reconstruction_plate_id_,
							time_of_formation_,
							reconstruct_handle_));
		}


		virtual
		~ReconstructedFeatureGeometry()
		{  }

		/**
		 * Get a non-null pointer to a const ReconstructedFeatureGeometry which points to this
		 * instance.
		 *
		 * Since the ReconstructedFeatureGeometry constructors are private, it should never
		 * be the case that a ReconstructedFeatureGeometry instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to a ReconstructedFeatureGeometry which points to this
		 * instance.
		 *
		 * Since the ReconstructedFeatureGeometry constructors are private, it should never
		 * be the case that a ReconstructedFeatureGeometry instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Return whether this RFG references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const GPlatesModel::FeatureHandle &that_feature_handle) const
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
		GPlatesModel::FeatureHandle *
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
		const GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() const;

		/**
		 * Access the feature property which contained the reconstructed geometry.
		 */
		const GPlatesModel::FeatureHandle::iterator
		property() const
		{
			return d_property_iterator;
		}


		/**
		 * Returns the reconstructed geometry - WARNING - only call if necessary - see comment below.
		 *
		 * NOTE: Do not call this method unless you need the reconstructed geometry for
		 * some calculation or inspection - this is because visualisation of reconstructed
		 * geometries attempts to bypass transforming (reconstructing) geometries on the CPU
		 * and instead perform them on the GPU (graphics hardware) where the transformation
		 * is *much* faster (in the globe view anyway). So any unnecessary calls to this method
		 * will effectively cancel that gain. This is especially the case for large geometries
		 * containing hundreds or thousands of vertices.
		 * Of course, there will be situations where calling this method is necessary such
		 * as reconstructing topological sections so that dynamic plate polygons can be resolved, or
		 * displaying the geometry in a GUI table, or projecting geometry into a map view.
		 */
		const geometry_ptr_type &
		reconstructed_geometry() const;


		/**
		 * Returns the information to perform the reconstruction as a finite rotation if applicable.
		 *
		 * It is currently used when visualising @a ReconstructedFeatureGeometry objects in
		 * the globe view - the finite rotation transform is performed by the graphics hardware.
		 *
		 * Returns false if 'this' @a ReconstructedFeatureGeometry was created directly
		 * with a reconstructed geometry (instead of a resolved geometry and a finite rotation).
		 *
		 * Returns boost::none if the reconstruction transform is not a finite rotation or if this
		 * @a ReconstructedFeatureGeometry was not created with one (even though it could have been).
		 * In this case you'll need to call @a reconstructed_geometry.
		 */
		const boost::optional<FiniteRotationReconstruction> &
		finite_rotation_reconstruction() const
		{
			return d_finite_rotation_reconstruction;
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
		const boost::optional<GPlatesModel::integer_plate_id_type> &
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
		 * Returns the optional reconstruct handle that this RFG was created with.
		 *
		 * The main reason this was added was to enable identification of an RFG among a list
		 * of RFG observers of a feature - this is useful when searching for an RFG that was
		 * generated in a specific scenario (reconstruct handle) such a topological section
		 * geometries that are found via the topological section feature.
		 */
		const boost::optional<ReconstructHandle::type> &
		get_reconstruct_handle() const
		{
			return d_reconstruct_handle;
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

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);

	protected:
		ReconstructedFeatureGeometry(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				GPlatesModel::FeatureHandle &feature_handle_,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				const geometry_ptr_type &reconstructed_geometry_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none);

		ReconstructedFeatureGeometry(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				GPlatesModel::FeatureHandle &feature_handle_,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				// NOTE: This is the *unreconstructed* geometry...
				const geometry_ptr_type &resolved_geometry_,
				const ReconstructMethodFiniteRotation::non_null_ptr_to_const_type &reconstruct_method_transform_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none);

	private:
		/**
		 * This is an iterator to the (geometry-valued) property from which this RFG was
		 * derived.
		 */
		GPlatesModel::FeatureHandle::iterator d_property_iterator;

		/**
		 * The reconstructed feature geometry.
		 *
		 * If the reconstructed geometry can be generated by transforming
		 * resolved geometry using a finite transform then we don't need to
		 * transform it until a client requests it.
		 *
		 * This also makes rendering a lot faster because the graphics card (in globe view anyway)
		 * can do the transformation saving us having to do it on the CPU.
		 */
		mutable boost::optional<geometry_ptr_type> d_reconstructed_geometry;

		/**
		 * The optional finite rotation transform (and resolved geometry).
		 *
		 * Is boost::none if reconstruction cannot be represented as a finite rotation or
		 * if this @a ReconstructedFeatureGeometry was not created with one.
		 */
		boost::optional<FiniteRotationReconstruction> d_finite_rotation_reconstruction;

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
		boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;

		/**
		 * The cached time of formation of the feature, if it exists.
		 *
		 * This is cached so that it can be used to calculate the age of the feature at any
		 * particular reconstruction time.  The age of the feature is used when colouring
		 * feature geometries by age.
		 */
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_formation;

		/**
		 * An optional reconstruct handle that can be used by clients to identify where this RFG came from.
		 */
		boost::optional<ReconstructHandle::type> d_reconstruct_handle;
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTEDFEATUREGEOMETRY_H
