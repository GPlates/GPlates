/**
 * \file 
 * $Revision: 9024 $
 * $Date: 2010-07-30 12:47:35 +0200 (fr, 30 jul 2010) $
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDSMALLCIRCLE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDSMALLCIRCLE_H

#include "ReconstructedFeatureGeometry.h"
#include "maths/PointOnSphere.h"

namespace GPlatesAppLogic
{
	/**
	 * A reconstructed small circle.
	 *
	 */
	class ReconstructedSmallCircle :
			public ReconstructedFeatureGeometry
	{
	public:
		/**
		 * A convenience typedef for a non-null shared pointer to a non-const @a ReconstructedSmallCircle.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedSmallCircle>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a non-null shared pointer to a const @a ReconstructedSmallCircle.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedSmallCircle>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructedSmallCircle>.
		 */
		typedef boost::intrusive_ptr<ReconstructedSmallCircle> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const ReconstructedSmallCircle>.
		 */
		typedef boost::intrusive_ptr<const ReconstructedSmallCircle> maybe_null_ptr_to_const_type;

		/** 
		 * A convenience typedef for a PointOnSphere::non_null_ptr_to_const type. 
		 */
		typedef GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type small_circle_centre_type;

		/**
		 * Create a ReconstructedSmallCircle instance with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const small_circle_centre_type &centre_ptr,
				const double &radius,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none)
		{
			return non_null_ptr_type(
					new ReconstructedSmallCircle(
							reconstruction_tree,
							centre_ptr,
							radius,
							feature_handle,
							property_iterator,
							reconstruction_plate_id_),
					GPlatesUtils::NullIntrusivePointerHandler());
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


		small_circle_centre_type 
		centre() const
		{
			return d_centre;
		}

		double
		radius() const
		{
			return d_radius;
		}

        boost::optional<GPlatesModel::integer_plate_id_type>
        plate_id () const
        {
                return d_reconstruction_plate_id;
        }

	private:
		/**
		 * Instantiate a reconstructed small circle.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedSmallCircle(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const small_circle_centre_type &centre,
				const double &radius,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_):
			ReconstructedFeatureGeometry(
				reconstruction_tree_,
				feature_handle,
				property_iterator,
				centre,
				reconstruction_plate_id_,
				boost::none),
				d_centre(centre),
                d_radius(radius),
                d_reconstruction_plate_id(reconstruction_plate_id_)
		{  }

		small_circle_centre_type d_centre;
		double d_radius;

        boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;


	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTEDSMALLCIRCLE_H
