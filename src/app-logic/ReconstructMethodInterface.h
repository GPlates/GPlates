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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODINTERFACE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODINTERFACE_H

#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "GeometryDeformation.h"
#include "MultiPointVectorField.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructHandle.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"
#include "ReconstructMethodType.h"
#include "ReconstructParams.h"
#include "VelocityDeltaTime.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Interface for reconstructing feature geometries (derived classes handle different methods of reconstruction).
	 */
	class ReconstructMethodInterface :
			public GPlatesUtils::ReferenceCount<ReconstructMethodInterface>
	{
	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructMethodInterface.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructMethodInterface> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructMethodInterface> non_null_ptr_to_const_type;


		//! Typedef for a geometry type.
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_type;


		/**
		 * Associates a present day or resolved geometry with its geometry property iterator.
		 */
		struct Geometry
		{
			Geometry(
					GPlatesModel::FeatureHandle::iterator property_iterator_,
					geometry_type geometry_) :
				property_iterator(property_iterator_),
				geometry(geometry_)
			{  }

			GPlatesModel::FeatureHandle::iterator property_iterator;
			geometry_type geometry;
		};


		/**
		 * Extrinsic reconstruction state that features are reconstructed with - this is information
		 * that is "passed into" a reconstruct method during reconstruction (and initialisation).
		 *
		 * The intrinsic state is the properties of the features being reconstructed.
		 *
		 * Both types of state are needed to reconstruct features.
		 *
		 * For initialisation this is currently passed into the constructors of derived classes
		 * by the reconstruct method registry.
		 *
		 * @a geometry_deformation is optional - if set to boost::none then no deformation will occur.
		 *
		 * NOTE: If these parameters change then a new reconstruct method instance should be created.
		 */
		struct Context
		{
			Context(
					const ReconstructParams &reconstruct_params_,
					const ReconstructionTreeCreator &reconstruction_tree_creator_,
					boost::optional<GeometryDeformation::resolved_network_time_span_type::non_null_ptr_to_const_type>
							geometry_deformation_ = boost::none) :
				reconstruct_params(reconstruct_params_),
				reconstruction_tree_creator(reconstruction_tree_creator_)
			{
				if (geometry_deformation_)
				{
					geometry_deformation = geometry_deformation_.get();
				}
			}

			ReconstructParams reconstruct_params;
			ReconstructionTreeCreator reconstruction_tree_creator;
			boost::optional<GeometryDeformation::resolved_network_time_span_type::non_null_ptr_to_const_type>
					geometry_deformation;
		};


		virtual
		~ReconstructMethodInterface()
		{  }


		/**
		 * Returns the type of 'this' reconstruct method.
		 */
		ReconstructMethod::Type
		get_reconstruction_method_type() const
		{
			return d_reconstruction_method_type;
		}


		/**
		 * Returns the feature associated with this reconstruct method.
		 *
		 * Methods called on 'this' interface will apply to this feature.
		 */
		const GPlatesModel::FeatureHandle::weak_ref &
		get_feature_ref() const
		{
			return d_feature_weak_ref;
		}


		/**
		 * The same as @a get_resolved_feature_geometries with a reconstruction time of zero except there
		 * *must* be one geometry for *each* geometry property in the feature (associated with this
		 * reconstruct method) that is reconstructable when @a reconstruct_feature_geometries is called -
		 * but they do not have to be returned in any particular order.
		 *
		 * So this means if the geometry is *not* active at present day it is still returned.
		 * And this means it could return a different result than @a get_resolved_feature_geometries (with a time of zero).
		 * TODO: May need to revisit this.
		 */
		virtual
		void
		get_present_day_feature_geometries(
				std::vector<Geometry> &present_day_geometries) const = 0;


#if 0
		/**
		 * Returns the resolved geometries for the geometry properties of the feature associated
		 * with this reconstruct method, to the specified reconstruction time.
		 *
		 * NOTE: Unlike @a get_present_day_feature_geometries, this method can return fewer geometries
		 * than there are geometry feature properties that can be reconstructed.
		 * If a geometry property is time-dependent and is not active at @a reconstruction_time
		 * then a corresponding resolved geometry will not be returned.
		 */
		virtual
		void
		get_resolved_feature_geometries(
				std::vector<Geometry> &resolved_geometries,
				const double &reconstruction_time) const = 0;
#endif


		/**
		 * Reconstructs the feature associated with this reconstruct method to the specified
		 * reconstruction time and returns one or more reconstructed feature geometries.
		 *
		 * The reconstructed feature geometries are appended to @a reconstructed_feature_geometries.
		 *
		 * @a reconstruct_handle can be stored in any generated @a ReconstructedFeatureGeometry
		 * instances to identify it as having been generated by our caller (ie, the caller might
		 * get the next global reconstruct handle and use it to identify all RFGs that it generates
		 * through these calls to @a reconstruct_feature_geometries).
		 *
		 * Note that the reconstruction tree creator can be used to get reconstruction trees at times
		 * other than @a reconstruction_time.
		 * This is useful for reconstructing flowlines since the function might be hooked up
		 * to a reconstruction tree cache.
		 */
		virtual
		void
		reconstruct_feature_geometries(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time) = 0;


		/**
		 * Calculates velocities at the positions of the reconstructed feature geometries, of the feature
		 * associated with this reconstruct method, at the specified reconstruction time and returns
		 * one or more reconstructed feature *velocities*.
		 *
		 * The reconstructed feature velocities are appended to @a reconstructed_feature_velocities.
		 *
		 * @a reconstruct_handle can be stored in any generated @a MultiPointVectorField
		 * instances to identify it as having been generated by our caller (ie, the caller might
		 * get the next global reconstruct handle and use it to identify all velocities that it
		 * generates through these calls to @a reconstruct_feature_velocities).
		 */
		virtual
		void
		reconstruct_feature_velocities(
				std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type)
		{
			// The default implementation is sufficient for some derived classes.
			reconstruct_feature_velocities_by_plate_id(
					reconstructed_feature_velocities,
					reconstruct_handle,
					context,
					reconstruction_time,
					velocity_delta_time,
					velocity_delta_time_type);
		}


		/**
		 * Reconstructs the specified geometry from present day to the specified reconstruction time -
		 * unless @a reverse_reconstruct is true in which case the geometry is assumed to be
		 * the reconstructed geometry (at the reconstruction time) and the returned geometry will
		 * then be the present day geometry.
		 *
		 * NOTE: The feature associated with this reconstruct method is used as a source of
		 * feature properties that determine how to perform the reconstruction (for example,
		 * a reconstruction plate ID) - the feature's geometries are not reconstructed.
		 *
		 * This is mainly useful when you have a feature and are modifying its geometry at some
		 * reconstruction time (not present day). After each modification the geometry needs to be
		 * reverse reconstructed to present day before it can be attached back onto the feature
		 * because feature's typically store present day geometry in their geometry properties.
		 *
		 * Note that the reconstruction tree creator can be used to get reconstruction trees at times
		 * other than @a reconstruction_time.
		 * This is useful for reconstructing flowlines since the function might be hooked up
		 * to a reconstruction tree cache.
		 */
		virtual
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const Context &context,
				const double &reconstruction_time,
				bool reverse_reconstruct) = 0;

	protected:

		/**
		 * Constructor associates a feature with this (derived) reconstruct method instance.
		 */
		ReconstructMethodInterface(
				ReconstructMethod::Type reconstruction_method_type,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) :
			d_reconstruction_method_type(reconstruction_method_type),
			d_feature_weak_ref(feature_weak_ref)
		{  }


		/**
		 * The default method of calculating velocities that is suitable for some derived classes.
		 */
		void
		reconstruct_feature_velocities_by_plate_id(
				std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
				const ReconstructHandle::type &reconstruct_handle,
				const Context &context,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type);

	private:

		ReconstructMethod::Type d_reconstruction_method_type;
		GPlatesModel::FeatureHandle::weak_ref d_feature_weak_ref;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODINTERFACE_H
