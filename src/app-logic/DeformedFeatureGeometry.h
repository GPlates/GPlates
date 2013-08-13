/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_DEFORMEDFEATUREGEOMETRY_H
#define GPLATES_APP_LOGIC_DEFORMEDFEATUREGEOMETRY_H

#include <vector>

#include "GeometryDeformation.h"
#include "ReconstructedFeatureGeometry.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"


namespace GPlatesAppLogic
{
	/**
	 * A feature geometry that has been deformed.
	 *
	 * It could actually have been rigidly reconstructed if the geometry did not intersect
	 * any deforming regions, but the main difference with ReconstructedFeatureGeometry is
	 * DeformedFeatureGeometry went through the deformation pipeline.
	 *
	 * Represents a feature geometry that has been deformed *and* contains extra per-point
	 * deformation information.
	 */
	class DeformedFeatureGeometry :
			public ReconstructedFeatureGeometry
	{
	public:
		//! A convenience typedef for a non-null shared pointer to a non-const @a DeformedFeatureGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<DeformedFeatureGeometry> non_null_ptr_type;

		//! A convenience typedef for a non-null shared pointer to a const @a DeformedFeatureGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<const DeformedFeatureGeometry> non_null_ptr_to_const_type;


		//! Typedef for a sequence of per-geometry-point deformation information.
		typedef std::vector<GeometryDeformation::DeformationInfo> point_deformation_info_seq_type;


		/**
		 * Create a DeformedFeatureGeometry instance.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				const geometry_ptr_type &deformed_geometry,
				const point_deformation_info_seq_type &point_deformation_information_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new DeformedFeatureGeometry(
							reconstruction_tree,
							reconstruction_tree_creator,
							feature_handle,
							property_iterator,
							deformed_geometry,
							point_deformation_information_,
							reconstruction_plate_id_,
							time_of_formation_,
							reconstruct_handle_));
		}


		/**
		 * Returns the per-geometry-point deformation information.
		 */
		const point_deformation_info_seq_type &
		get_point_deformation_information() const
		{
			return d_point_deformation_information;
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

	private:

		/**
		 * Per-geometry-point deformation information.
		 */
		point_deformation_info_seq_type d_point_deformation_information;


		/**
		 * Instantiate a deformed feature geometry.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		DeformedFeatureGeometry(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				const geometry_ptr_type &deformed_geometry,
				const point_deformation_info_seq_type &point_deformation_information_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_,
				boost::optional<ReconstructHandle::type> reconstruct_handle_) :
			ReconstructedFeatureGeometry(
					reconstruction_tree_,
					reconstruction_tree_creator,
					feature_handle,
					property_iterator,
					deformed_geometry,
					ReconstructMethod::BY_PLATE_ID,
					reconstruction_plate_id_,
					time_of_formation_,
					reconstruct_handle_),
			d_point_deformation_information(point_deformation_information_)
		{  }

	};
}

#endif // GPLATES_APP_LOGIC_DEFORMEDFEATUREGEOMETRY_H
