/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDSCALARCOVERAGE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDSCALARCOVERAGE_H

#include <vector>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionGeometry.h"

#include "model/FeatureHandle.h"
#include "model/WeakObserver.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesAppLogic
{
	/**
	 * A coverage of scalar values associated with points in a domain geometry.
	 *
	 * The domains are regular geometries (points/multipoints/polylines/polygons) whose positions
	 * might have been deformed. The range is a mapping of each domain point to a scalar value and
	 * the scalar values may have evolved/changed over time (according to deformation strain).
	 *
	 * NOTE: This is not a ReconstructedFeatureGeometry (ie, instead inherits from ReconstructionGeometry)
	 * because the reconstructed domain geometry is already a ReconstructedFeatureGeometry.
	 * This reconstruction geometry is really just for the scalar values associated with the domain.
	 * This avoids things like exporting the domain geometries twice (because export collects all RFGs).
	 */
	class ReconstructedScalarCoverage :
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		//! A convenience typedef for a non-null shared pointer to a non-const @a ReconstructedScalarCoverage.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedScalarCoverage> non_null_ptr_type;

		//! A convenience typedef for a non-null shared pointer to a const @a ReconstructedScalarCoverage.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedScalarCoverage> non_null_ptr_to_const_type;

		//! Typedef for the WeakObserver base class of this class.
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;


		//! Typedef for a sequence of per-geometry-point scalar values.
		typedef std::vector<double> point_scalar_value_seq_type;


		/**
		 * Create a ReconstructedScalarCoverage instance.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_domain_geometry,
				GPlatesModel::FeatureHandle::iterator range_property_iterator,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const point_scalar_value_seq_type &reconstructed_point_scalar_values,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new ReconstructedScalarCoverage(
							reconstructed_domain_geometry,
							range_property_iterator,
							scalar_type,
							reconstructed_point_scalar_values,
							reconstruct_handle_));
		}


		/**
		 * Returns the reconstructed domain geometry.
		 *
		 * Note: This could be a DeformedFeatureGeometry (derived from ReconstructedFeatureGeometry)
		 * which also contains deformation strain information.
		 *
		 * Note: The reconstructed/deformed geometry is also in the base ReconstructedFeatureGeometry
		 * of this class (along with feature, plate id, etc).
		 */
		ReconstructedFeatureGeometry::non_null_ptr_type
		get_reconstructed_domain_geometry() const
		{
			return d_reconstructed_domain_geometry;
		}


		/**
		 * Access the feature property which contained the reconstructed domain geometry.
		 */
		const GPlatesModel::FeatureHandle::iterator
		get_domain_property() const
		{
			return d_reconstructed_domain_geometry->property();
		}

		/**
		 * Returns the reconstructed domain geometry.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_reconstructed_geometry() const
		{
			return d_reconstructed_domain_geometry->reconstructed_geometry();
		}


		/**
		 * Access the feature property from which the scalar values were reconstructed.
		 */
		const GPlatesModel::FeatureHandle::iterator
		get_range_property() const
		{
			return d_range_property_iterator;
		}

		/**
		 * Returns the type of the scalar values.
		 *
		 * The range property contains one or more scalar sequences.
		 * Each scalar sequence is identified by a scalar type.
		 */
		const GPlatesPropertyValues::ValueObjectType &
		get_scalar_type() const
		{
			return d_scalar_type;
		}

		/**
		 * Returns the per-geometry-point scalar values.
		 *
		 * Note: Each scalar maps to a point in @a get_reconstructed_geometry.
		 */
		const point_scalar_value_seq_type &
		get_reconstructed_point_scalar_values() const
		{
			return d_reconstructed_point_scalar_values;
		}


		/**
		 * Get a non-null pointer to const.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to non-const.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Return whether this RG references @a that_feature_handle.
		 */
		bool
		references(
				const GPlatesModel::FeatureHandle &that_feature_handle) const
		{
			return d_reconstructed_domain_geometry->references(that_feature_handle);
		}

		/**
		 * Return the pointer to the FeatureHandle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a
		 * FeatureHandle; non-NULL otherwise.
		 */
		GPlatesModel::FeatureHandle *
		feature_handle_ptr() const
		{
			return d_reconstructed_domain_geometry->feature_handle_ptr();
		}

		/**
		 * Return whether this pointer is valid to be dereferenced (to obtain a FeatureHandle).
		 */
		bool
		is_valid() const
		{
			return d_reconstructed_domain_geometry->is_valid();
		}

		/**
		 * Return a weak-ref to the *domain* feature used for the domain of the vector field.
		 */
		const GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() const
		{
			return d_reconstructed_domain_geometry->get_feature_ref();
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

	private:

		/**
		 * The reconstructed domain.
		 */
		ReconstructedFeatureGeometry::non_null_ptr_type d_reconstructed_domain_geometry;

		/**
		 * The range property that the scalar values came from.
		 */
		GPlatesModel::FeatureHandle::iterator d_range_property_iterator;

		/**
		 * The type of the scalar values.
		 */
		GPlatesPropertyValues::ValueObjectType d_scalar_type;

		/**
		 * Per-geometry-point scalar values.
		 */
		point_scalar_value_seq_type d_reconstructed_point_scalar_values;


		/**
		 * Instantiate a reconstructed scalar coverage.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedScalarCoverage(
				const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_domain_geometry,
				GPlatesModel::FeatureHandle::iterator range_property_iterator,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const point_scalar_value_seq_type &reconstructed_point_scalar_values,
				boost::optional<ReconstructHandle::type> reconstruct_handle_) :
			ReconstructionGeometry(
					reconstructed_domain_geometry->get_reconstruction_time(),
					reconstruct_handle_),
			WeakObserverType(*reconstructed_domain_geometry->feature_handle_ptr()),
			d_reconstructed_domain_geometry(reconstructed_domain_geometry),
			d_range_property_iterator(range_property_iterator),
			d_scalar_type(scalar_type),
			d_reconstructed_point_scalar_values(reconstructed_point_scalar_values)
		{  }

	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTEDSCALARCOVERAGE_H
