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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTCONTEXT_H
#define GPLATES_APP_LOGIC_RECONSTRUCTCONTEXT_H

#include <map>
#include <vector>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructHandle.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"
#include "ReconstructParams.h"
#include "ReconstructMethodInterface.h"
#include "ReconstructMethodType.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	class ReconstructMethodRegistry;

	/**
	 * Used to reconstruct regular features into @a ReconstructedFeatureGeometry objects
	 * at various reconstruction times.
	 *
	 * This class keeps a mapping of features to reconstruct methods internally.
	 * This is because:
	 * - Avoids unnecessarily detecting the reconstruct method at each reconstruction time,
	 * - Makes it easier to extract a mapping of present-day geometries knowing that the
	 *   features (and hence present day geometries) have not changed. This is useful when
	 *   things like static polygon raster reconstruction that map a present day polygon geometry
	 *   to an OpenGL polygon mesh that persists as long as the feature remains unchanged.
	 *
	 * NOTE: This only reconstructs features which can be reconstructed
	 * as @a ReconstructedFeatureGeometry objects (eg, does not handle topological features).
	 */
	class ReconstructContext
	{
	public:
		//! Typedef for a geometry type.
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_type;

		/**
		 * Typedef for a handle to a geometry feature property.
		 *
		 * It's referred to as 'resolved' instead of present day because it could be a
		 * time-dependent property. In either case it is *not* the reconstructed geometry.
		 *
		 * This property handle, along with methods @a get_present_day and @a reconstruct,
		 * can be used by clients to efficiently map any reconstructed feature geometry, across
		 * all features, to its present day, or even resolved, geometry. This is useful when
		 * the client needs to associate an object with each present day geometry such as
		 * an OpenGL polygon mesh - the geometry property handle can then be used to quickly
		 * locate the OpenGL polygon mesh.
		 */
		typedef unsigned int geometry_property_handle_type;


		/**
		 * Used to associate a reconstructed feature geometry with its resolved geometry (ie, *unreconstructed*).
		 */
		class Reconstruction
		{
		public:
			Reconstruction(
					const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_feature_geometry,
					geometry_property_handle_type geometry_property_handle) :
				d_reconstructed_feature_geometry(reconstructed_feature_geometry),
				d_geometry_property_handle(geometry_property_handle)
			{  }

			//! Returns the reconstructed feature geometry.
			const ReconstructedFeatureGeometry::non_null_ptr_type &
			get_reconstructed_feature_geometry() const
			{
				return d_reconstructed_feature_geometry;
			}

			//! Returns the geometry property handle.
			geometry_property_handle_type
			get_geometry_property_handle() const
			{
				return d_geometry_property_handle;
			}

		private:
			ReconstructedFeatureGeometry::non_null_ptr_type d_reconstructed_feature_geometry;
			geometry_property_handle_type d_geometry_property_handle;
		};


		/**
		 * Used to associate a feature with its reconstructed feature geometry(s).
		 */
		class ReconstructedFeature
		{
		public:
			//! Typedef for a sequence of @a Reconstruction objects.
			typedef std::vector<Reconstruction> reconstruction_seq_type;


			explicit
			ReconstructedFeature(
					const GPlatesModel::FeatureHandle::weak_ref &feature) :
				d_feature(feature)
			{  }

			/**
			 * Returns the feature.
			 */
			const GPlatesModel::FeatureHandle::weak_ref &
			get_feature() const
			{
				return d_feature;
			}

			/**
			 * Returns the reconstructed feature geometries of 'this' feature.
			 *
			 * NOTE: The returned sequence can be empty if the feature is inactive at the
			 * reconstruction time for example.
			 */
			const reconstruction_seq_type &
			get_reconstructions() const
			{
				return d_reconstructions;
			}

		private:
			GPlatesModel::FeatureHandle::weak_ref d_feature;

			reconstruction_seq_type d_reconstructions;

			friend class ReconstructContext;
		};


		/**
		 * Constructor.
		 *
		 * Effectively the same as @a reassign_reconstruct_methods_to_features.
		 * See @a reassign_reconstruct_methods_to_features for details.
		 */
		explicit
		ReconstructContext(
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_feature_collections =
								std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>());


		/**
		 * Removes any features added in a previous call to @a reassign_reconstruct_methods_to_features,
		 * or constructor, and for each feature in each feature collection determines
		 * which reconstruct method to use.
		 *
		 * Calls to @a reconstruct will then use that mapping of features to reconstruct methods
		 * when carrying out reconstructions.
		 *
		 * Note: If the features change you should @a reassign_reconstruct_methods_to_features again.
		 * This is because each feature might now require a different reconstruct method.
		 */
		void
		reassign_reconstruct_methods_to_features(
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_feature_collections);


		/**
		 * The same as @a get_resolved_geometries with a reconstruction time of zero
		 * except the returned sequence contains geometries instead of optional geometries -
		 * this is because the value of the geometry property (at time zero) is obtained
		 * regardless of whether its active at present day or not - in the majority of cases
		 * it will be active at present day.
		 *
		 * TODO: May need to revisit the notion of disregarding active state at present day.
		 *
		 * The returned sequence can be indexed using @a geometry_property_handle_type.
		 *
		 * The returned reference is valid until @a reassign_reconstruct_methods_to_features is called.
		 */
		const std::vector<geometry_type> &
		get_present_day_geometries();


#if 0
		/**
		 * Returns the resolved geometries for the geometry properties of the features specified
		 * in the constructor, or the most recent call to @a reassign_reconstruct_methods_to_features,
		 * at the specified reconstruction time.
		 *
		 * The returned sequence can be indexed using @a geometry_property_handle_type.
		 *
		 * If a geometry property is time-dependent and is not active at @a reconstruction_time
		 * then the corresponding resolved geometry will be boost::none.
		 */
		std::vector<boost::optional<geometry_type> >
		get_resolved_geometries(
				const double &reconstruction_time);
#endif


		/**
		 * Reconstructs the features specified in the constructor, or the most recent call
		 * to @a reassign_reconstruct_methods_to_features, to the specified reconstruction time.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructParams &reconstruct_params,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);


		/**
		 * Reconstructs the features specified in the constructor, or the most recent call
		 * to @a reassign_reconstruct_methods_to_features, to the specified reconstruction time.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 *
		 * This differs from the other overloads of @a reconstruct in that this method also
		 * associates each reconstructed feature geometry with the feature geometry property
		 * it was reconstructed from.
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<Reconstruction> &reconstructed_feature_geometries,
				const ReconstructParams &reconstruct_params,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);


		/**
		 * Reconstructs the features specified in the constructor, or the most recent call
		 * to @a reassign_reconstruct_methods_to_features, to the specified reconstruction time.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 *
		 * This differs from the other overloads of @a reconstruct in that this method returns
		 * reconstructions grouped by *feature*.
		 * Note that even if a feature is not active (or generates no reconstructions for some reason)
		 * it will still be returned (it just won't have any reconstructions in it) - this is useful
		 * for co-registration, for example, which correlates by feature over several frames (times).
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructedFeature> &reconstructed_features,
				const ReconstructParams &reconstruct_params,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);

	private:
		/**
		 * Groups a feature with its geometry properties.
		 */
		struct ReconstructMethodFeature
		{
			struct GeometryPropertyToHandle
			{
				GPlatesModel::FeatureHandle::iterator property_iterator;
				geometry_property_handle_type geometry_property_handle;
			};

			//! Typedef for a sequence mapping geometry properties to geometry property handles.
			typedef std::vector<GeometryPropertyToHandle> geometry_property_to_handle_seq_type;


			explicit
			ReconstructMethodFeature(
					const GPlatesModel::FeatureHandle::weak_ref &feature_) :
				feature(feature_)
			{  }


			GPlatesModel::FeatureHandle::weak_ref feature;

			//! Each reconstructable geometry property in the feature maps to a geometry property handle.
			geometry_property_to_handle_seq_type geometry_property_to_handle_seq;
		};

		/**
		 * Groups features with their reconstruct method.
		 */
		struct ReconstructMethodFeatures
		{
			explicit
			ReconstructMethodFeatures(
					const ReconstructMethodInterface::non_null_ptr_type &reconstruct_method_) :
				reconstruct_method(reconstruct_method_)
			{  }

			ReconstructMethodInterface::non_null_ptr_type reconstruct_method;
			std::vector<ReconstructMethodFeature> features;
		};

		//! Typedef or a sequence of reconstruct methods and their associated features.
		typedef std::vector<ReconstructMethodFeatures> reconstruct_method_features_seq_type;


		//! Grouping of features with their reconstruct method.
		reconstruct_method_features_seq_type d_reconstruct_method_features_seq;

		/**
		 * The present day geometries of all reconstructable geometry properties of all features.
		 */
		boost::optional<std::vector<geometry_type> > d_cached_present_day_geometries;


		/**
		 * Converts the reconstructed feature geometries, of the specified feature, to reconstructions.
		 */
		void
		get_feature_reconstructions(
				std::vector<Reconstruction> &reconstructions,
				const ReconstructMethodFeature &reconstruct_method_feature,
				const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries);

		/**
		 * Returns true if the geometry property handles have been assigned and are up-to-date
		 * with the current set of features.
		 */
		bool
		have_assigned_geometry_property_handles() const
		{
			return d_cached_present_day_geometries;
		}

		/**
		 * Iterates over the assigned features and assigns geometry property handles.
		 */
		void
		assign_geometry_property_handles();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTCONTEXT_H
