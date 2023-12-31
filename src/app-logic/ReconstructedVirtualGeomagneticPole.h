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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTEDVIRTUALGEOMAGNETICPOLE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTEDVIRTUALGEOMAGNETICPOLE_H

#include "ReconstructedFeatureGeometry.h"

namespace GPlatesAppLogic
{

	struct ReconstructedVirtualGeomagneticPoleParams
	{
	
			boost::optional<GPlatesMaths::PointOnSphere> d_site_point;
			boost::optional<GPlatesModel::FeatureHandle::iterator> d_site_iterator;
			boost::optional<GPlatesMaths::PointOnSphere> d_vgp_point;
			boost::optional<GPlatesModel::FeatureHandle::iterator> d_vgp_iterator;
			boost::optional<double> d_a95;
			boost::optional<double> d_dm;
			boost::optional<double> d_dp;
			boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_begin_time;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_end_time;
			boost::optional<double> d_age;
	};
	/**
	 * A reconstructed virtual geomagnetic pole minus the sample site geometry
	 * (which is a @a ReconstructedFeatureGeometry).
	 *
	 * This inherits from @a ReconstructedFeatureGeometry because it is a reconstructed
	 * feature geometry and this allows code to search for ReconstructedFeatureGeometry's
	 * and have reconstructed virtual geomagnetic pole geometries automatically included
	 * in that search.
	 */
	class ReconstructedVirtualGeomagneticPole :
			public ReconstructedFeatureGeometry
	{
	public:
		/**
		 * A convenience typedef for a non-null shared pointer to a non-const @a ReconstructedVirtualGeomagneticPole.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedVirtualGeomagneticPole>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a non-null shared pointer to a const @a ReconstructedVirtualGeomagneticPole.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedVirtualGeomagneticPole>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ReconstructedVirtualGeomagneticPole>.
		 */
		typedef boost::intrusive_ptr<ReconstructedVirtualGeomagneticPole> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const ReconstructedVirtualGeomagneticPole>.
		 */
		typedef boost::intrusive_ptr<const ReconstructedVirtualGeomagneticPole> maybe_null_ptr_to_const_type;


		/**
		 * Create a ReconstructedVirtualGeomagneticPole instance with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 * For instance, a ReconstructedVirtualGeomagneticPole might be created without a
		 * reconstruction plate ID if no reconstruction plate ID is found amongst the
		 * properties of the feature being reconstructed, but the client code still wants
		 * to "reconstruct" the geometries of the feature using the identity rotation.
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructedVirtualGeomagneticPoleParams &params,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const geometry_ptr_type &geometry_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new ReconstructedVirtualGeomagneticPole(
							params,
							reconstruction_tree,
							reconstruction_tree_creator,
							geometry_ptr,
							feature_handle,
							property_iterator_,
							reconstruction_plate_id_,
							time_of_formation_,
							reconstruct_handle_));
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

		inline 
		const ReconstructedVirtualGeomagneticPoleParams &
		vgp_params() const
		{
			return d_VGP_params;
		}

	private:
		/**
		 * Instantiate a reconstructed virtual geomagnetic pole with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructedVirtualGeomagneticPole(
				const ReconstructedVirtualGeomagneticPoleParams &params,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const geometry_ptr_type &geometry_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_,
				boost::optional<ReconstructHandle::type> reconstruct_handle_):
			ReconstructedFeatureGeometry(
					reconstruction_tree_,
					reconstruction_tree_creator,
					feature_handle,
					property_iterator_,
					geometry_ptr,
					ReconstructMethod::VIRTUAL_GEOMAGNETIC_POLE,
					reconstruction_plate_id_,
					time_of_formation_,
					reconstruct_handle_),
			d_VGP_params(params)
		{  }

		ReconstructedVirtualGeomagneticPoleParams d_VGP_params;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTEDVIRTUALGEOMAGNETICPOLE_H
