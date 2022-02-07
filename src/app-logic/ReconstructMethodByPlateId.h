/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODBYPLATEID_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODBYPLATEID_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructMethodInterface.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	/**
	 * Reconstructs a feature using its present day geometry and its plate Id.
	 */
	class ReconstructMethodByPlateId :
			public ReconstructMethodInterface
	{
	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructMethodByPlateId.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructMethodByPlateId> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructMethodByPlateId> non_null_ptr_to_const_type;


		/**
		 * Returns true if can reconstruct the specified feature.
		 *
		 * It only needs to have a non-topological geometry to pass this test.
		 */
		static
		bool
		can_reconstruct_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref);


		/**
		 * Creates a @a ReconstructMethodByPlateId object associated with the specified feature.
		 */
		static
		ReconstructMethodByPlateId::non_null_ptr_type
		create(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const Context &context)
		{
			return non_null_ptr_type(new ReconstructMethodByPlateId(feature_ref, context));
		}


		/**
		 * Returns the present day geometries of the feature associated with this reconstruct method.
		 */
		virtual
		void
		get_present_day_feature_geometries(
				std::vector<Geometry> &present_day_geometries) const;


		/**
		 * Reconstructs the feature associated with this reconstruct method to the specified
		 * reconstruction time and returns one or more reconstructed feature geometries.
		 *
		 * NOTE: This will still generate a reconstructed feature geometry if the
		 * feature has no plate ID (ie, even if @a can_reconstruct_feature returns false).
		 * In this case the identity rotation is used.
		 */
		virtual
		void
		reconstruct_feature_geometries(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time);


		/**
		 * Calculates velocities at the positions of the reconstructed feature geometries, of the feature
		 * associated with this reconstruct method, at the specified reconstruction time and returns
		 * one or more reconstructed feature *velocities*.
		 */
		virtual
		void
		reconstruct_feature_velocities(
				std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time);


		/**
		 * Reconstructs the specified geometry from present day to the specified reconstruction time -
		 * unless @a reverse_reconstruct is true in which case the geometry is assumed to be
		 * the reconstructed geometry (at the reconstruction time) and the returned geometry will
		 * then be the present day geometry.
		 *
		 * NOTE: The feature associated with this reconstruct method is used as a source of
		 * feature properties that determine how to perform the reconstruction (for example,
		 * a reconstruction plate ID) - the feature's geometries are not reconstructed.
		 */
		virtual
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const Context &context,
				const double &reconstruction_time,
				bool reverse_reconstruct);

	private:

		/**
		 * Associate a deformed geometry's time span with its feature property.
		 */
		struct DeformedGeometryPropertyTimeSpan
		{
			DeformedGeometryPropertyTimeSpan(
					const GPlatesModel::FeatureHandle::iterator &property_iterator_,
					const GeometryDeformation::GeometryTimeSpan::non_null_ptr_type &geometry_time_span_) :
				property_iterator(property_iterator_),
				geometry_time_span(geometry_time_span_)
			{  }

			GPlatesModel::FeatureHandle::iterator property_iterator;
			GeometryDeformation::GeometryTimeSpan::non_null_ptr_type geometry_time_span;
		};

		//! Typedef for a sequence of deformed geometries.
		typedef std::vector<DeformedGeometryPropertyTimeSpan> deformed_geometry_time_span_sequence_type;


		/**
		 * The deformed geometry look up tables, or boost::none if not using deformation.
		 *
		 * There's one entry for each feature geometry property.
		 */
		boost::optional<deformed_geometry_time_span_sequence_type> d_deformed_geometry_property_time_spans;


		explicit
		ReconstructMethodByPlateId(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const Context &context);

		void
		initialise_deformation(
				const Context &context);
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODBYPLATEID_H
