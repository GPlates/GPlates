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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTLAYERPROXY_H
#define GPLATES_APP_LOGIC_RECONSTRUCTLAYERPROXY_H

#include <vector>
#include <boost/optional.hpp>

#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructContext.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionLayerProxy.h"

#include "maths/CubeQuadTreePartition.h"
#include "maths/GeometryOnSphere.h"
#include "maths/PolygonMesh.h"
#include "maths/types.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	class ReconstructParams;
	class ReconstructMethodRegistry;

	/**
	 * A layer proxy for reconstructing regular (non-topological) features containing vector geometry.
	 *
	 * The types of features reconstructed here include:
	 *  - Features with regular geometry reconstructed by plate ID.
	 *  - Features with regular geometry reconstructed by half-stage rotation (left/right plate ID).
	 *  - Flowline/MotionPath features.
	 *  - VirtualGeomagneticPole features.
	 *
	 * However all reconstructed geometries are returned as ReconstructedFeatureGeometry objects.
	 * So reconstructions like flowlines are, in fact, derivations of ReconstructedFeatureGeometry.
	 */
	class ReconstructLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ReconstructLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ReconstructLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructLayerProxy> non_null_ptr_to_const_type;

		/**
		 * Typedef for a spatial partition of reconstructed feature geometries.
		 */
		typedef GPlatesMaths::CubeQuadTreePartition<ReconstructedFeatureGeometry::non_null_ptr_type>
				reconstructed_feature_geometries_spatial_partition_type;

		/**
		 * Typedef for a spatial partition of geometries.
		 */
		typedef GPlatesMaths::CubeQuadTreePartition<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
				geometries_spatial_partition_type;

		/**
		 * Typedef for a spatial partition of reconstructed feature geometries that
		 * reference present day geometries.
		 */
		typedef GPlatesMaths::CubeQuadTreePartition<ReconstructContext::Reconstruction>
				reconstructions_spatial_partition_type;


		/**
		 * The default depth of the spatial partition (the quad trees in each cube face).
		 */
		static const unsigned int DEFAULT_SPATIAL_PARTITION_DEPTH = 7;


		/**
		 * Creates a @a ReconstructLayerProxy object.
		 */
		static
		non_null_ptr_type
		create(
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const ReconstructParams &reconstruct_params = ReconstructParams())
		{
			return non_null_ptr_type(new ReconstructLayerProxy(reconstruct_method_registry, reconstruct_params));
		}


		/**
		 * Returns the reconstructed feature geometries, for the current reconstruction time,
		 * by appending them to @a reconstructed_feature_geometries.
		 */
		void
		get_reconstructed_feature_geometries(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries)
		{
			get_reconstructed_feature_geometries(reconstructed_feature_geometries, d_current_reconstruction_time);
		}


		/**
		 * Returns the reconstructed feature geometries, at the specified time, by appending
		 * them to @a reconstructed_feature_geometries.
		 */
		void
		get_reconstructed_feature_geometries(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const double &reconstruction_time);


		/**
		 * Returns the reconstructed feature geometries, for the current reconstruction time,
		 * by appending them to @a reconstructed_feature_geometries.
		 *
		 * Note that ReconstructContext::Reconstruction::get_geometry_property_handle can index
		 * into the sequences returned by @a get_present_day_geometries and
		 * @a get_present_day_geometries_spatial_partition_locations.
		 *
		 * This geometry property handle can be used to index into the sequence of present day
		 * geometries returned by @a get_present_day_geometries.
		 */
		void
		get_reconstructed_feature_geometries(
				std::vector<ReconstructContext::Reconstruction> &reconstructed_feature_geometries)
		{
			get_reconstructed_feature_geometries(reconstructed_feature_geometries, d_current_reconstruction_time);
		}


		/**
		 * Returns the reconstructed feature geometries, at the specified time, by appending
		 * them to @a reconstructed_feature_geometries.
		 *
		 * Note that ReconstructContext::Reconstruction::get_geometry_property_handle can index
		 * into the sequences returned by @a get_present_day_geometries and
		 * @a get_present_day_geometries_spatial_partition_locations.
		 *
		 * This geometry property handle can be used to index into the sequence of present day
		 * geometries returned by @a get_present_day_geometries.
		 */
		void
		get_reconstructed_feature_geometries(
				std::vector<ReconstructContext::Reconstruction> &reconstructed_feature_geometries,
				const double &reconstruction_time);


		/**
		 * Returns the reconstructed feature geometries, at the current reconstruction time, in a spatial partition.
		 *
		 * The maximum depth of the quad trees in each cube face of the spatial partition
		 * is @a DEFAULT_SPATIAL_PARTITION_DEPTH.
		 */
		reconstructed_feature_geometries_spatial_partition_type::non_null_ptr_to_const_type
		get_reconstructed_feature_geometries_spatial_partition()
		{
			return get_reconstructed_feature_geometries_spatial_partition(d_current_reconstruction_time);
		}


		/**
		 * Returns the reconstructed feature geometries, at the specified time, in a spatial partition.
		 *
		 * The maximum depth of the quad trees in each cube face of the spatial partition
		 * is @a DEFAULT_SPATIAL_PARTITION_DEPTH.
		 */
		reconstructed_feature_geometries_spatial_partition_type::non_null_ptr_to_const_type
		get_reconstructed_feature_geometries_spatial_partition(
				const double &reconstruction_time);


		/**
		 * Returns the reconstructed feature geometries, at the current reconstruction time, in a spatial partition.
		 *
		 * Note that ReconstructContext::Reconstruction::get_geometry_property_handle can index
		 * into the sequences returned by @a get_present_day_geometries and
		 * @a get_present_day_geometries_spatial_partition_locations.
		 *
		 * The maximum depth of the quad trees in each cube face of the spatial partition
		 * is @a DEFAULT_SPATIAL_PARTITION_DEPTH.
		 */
		reconstructions_spatial_partition_type::non_null_ptr_to_const_type
		get_reconstructions_spatial_partition()
		{
			return get_reconstructions_spatial_partition(d_current_reconstruction_time);
		}


		/**
		 * Returns the reconstructed feature geometries, at the specified time, in a spatial partition.
		 *
		 * Note that ReconstructContext::Reconstruction::get_geometry_property_handle can index
		 * into the sequences returned by @a get_present_day_geometries and
		 * @a get_present_day_geometries_spatial_partition_locations.
		 *
		 * The maximum depth of the quad trees in each cube face of the spatial partition
		 * is @a DEFAULT_SPATIAL_PARTITION_DEPTH.
		 */
		reconstructions_spatial_partition_type::non_null_ptr_to_const_type
		get_reconstructions_spatial_partition(
				const double &reconstruction_time);


		/**
		 * Returns the present day geometries of the current set of reconstructable feature
		 * collections input to this layer proxy.
		 *
		 * The returned sequence can be indexed by ReconstructContext::Reconstruction::get_geometry_property_handle.
		 *
		 * Use @a get_reconstructable_feature_collections_subject_token to determine
		 * when these present day geometries have been updated.
		 */
		const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &
		get_present_day_geometries();


		/**
		 * Returns the present day geometries of the current set of reconstructable feature
		 * collections input to this layer proxy.
		 *
		 * The returned sequence can be indexed by ReconstructContext::Reconstruction::get_geometry_property_handle.
		 *
		 * NOTE: The polygon meshes can be formed by polylines and multipoints (in addition to polygons).
		 * For polylines and multipoints the vertices are treated as if they were vertices of a polygon.
		 *
		 * When a polygon mesh cannot be generated for a particular present day geometry the
		 * corresponding entry in the returned sequence will be boost::none.
		 * Current reasons for this include:
		 * - polygon has too few vertices (includes case where geometry is a @a PointOnSphere),
		 * - polygon is self-intersecting,
		 * - polygon is too big and does not project onto a 2D plane.
		 * FIXME: Remove the constraints of the last two cases.
		 *
		 * Use @a get_reconstructable_feature_collections_subject_token to determine
		 * when these present day polygon meshes have been updated.
		 */
		const std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> > &
		get_present_day_polygon_meshes();


		/**
		 * Returns the present day geometries in a spatial partition.
		 *
		 * These present day geometries can be referenced from those overloads of
		 * @a get_reconstructed_feature_geometries_spatial_partition that contain references
		 * to a present day geometries spatial partition.
		 *
		 * Use @a get_reconstructable_feature_collections_subject_token to determine
		 * when these present day geometries have been updated.
		 *
		 * The maximum depth of the quad trees in each cube face of the spatial partition
		 * is @a DEFAULT_SPATIAL_PARTITION_DEPTH.
		 */
		geometries_spatial_partition_type::non_null_ptr_to_const_type
		get_present_day_geometries_spatial_partition();


		/**
		 * Returns the present day geometries in a spatial partition.
		 *
		 * These present day geometries can be referenced from those overloads of
		 * @a get_reconstructed_feature_geometries_spatial_partition that contain references
		 * to a present day geometries spatial partition.
		 *
		 * Use @a get_reconstructable_feature_collections_subject_token to determine
		 * when these present day geometries have been updated.
		 *
		 * The maximum depth of the quad trees in each cube face of the spatial partition
		 * is @a DEFAULT_SPATIAL_PARTITION_DEPTH.
		 */
		const std::vector<GPlatesMaths::CubeQuadTreeLocation> &
		get_present_day_geometries_spatial_partition_locations();


		/**
		 * Returns the subject token that clients can use to determine if the reconstructed
		 * feature geometries have changed since they were last retrieved.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();


		/**
		 * Returns the subject token that clients can use to determine if the reconstructable
		 * feature collections have changed.
		 *
		 * This can be used to determine if @a get_present_day_geometries needs to be called
		 * again to remain up-to-date with the change.
		 */
		const GPlatesUtils::SubjectToken &
		get_reconstructable_feature_collections_subject_token();


		/**
		 * Accept a ConstLayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerProxyVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

		/**
		 * Accept a LayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				LayerProxyVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		//
		// Used by LayerTask...
		//

		/**
		 * Sets the current reconstruction time as set by the layer system.
		 */
		void
		set_current_reconstruction_time(
				const double &reconstruction_time);

		/**
		 * Set the reconstruction layer proxy used to rotate the feature geometries.
		 */
		void
		set_current_reconstruction_layer_proxy(
				const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy);

		/**
		 * Add to the list of feature collections that will be reconstructed.
		 */
		void
		add_reconstructable_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Remove from the list of feature collections that will be reconstructed.
		 */
		void
		remove_reconstructable_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * A reconstructable feature collection was modified.
		 */
		void
		modified_reconstructable_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Sets the parameters used for reconstructing.
		 */
		void
		set_current_reconstruct_params(
				const ReconstructParams &reconstruct_params);

	private:
		/**
		 * Used to associate features with reconstruct methods.
		 */
		const ReconstructMethodRegistry &d_reconstruct_method_registry;

		/**
		 * Used to reconstruct features into ReconstructedFeatureGeometry objects.
		 *
		 * We only need to reset the reconstruct context when the features are modified.
		 */
		ReconstructContext d_reconstruct_context;

		/**
		 * The input feature collections to reconstruct.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_reconstructable_feature_collections;

		/**
		 * Used to get reconstruction trees at desired reconstruction times.
		 */
		LayerProxyUtils::InputLayerProxy<ReconstructionLayerProxy> d_current_reconstruction_layer_proxy;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * The cached reconstructed feature geometries.
		 */
		boost::optional< std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> >
				d_cached_reconstructed_feature_geometries;

		/**
		 * The cached reconstructed feature geometries in the form of
		 * ReconstructContext::Reconstruction objects.
		 */
		boost::optional< std::vector<ReconstructContext::Reconstruction> > d_cached_reconstructions;

		/**
		 * The cached reconstructed feature geometries spatial partition.
		 */
		boost::optional<reconstructed_feature_geometries_spatial_partition_type::non_null_ptr_type>
				d_cached_reconstructed_feature_geometries_spatial_partition;

		/**
		 * The cached reconstructions spatial partition.
		 */
		boost::optional<reconstructions_spatial_partition_type::non_null_ptr_type>
				d_cached_reconstructions_spatial_partition;

		/**
		 * The cached present day geometries of the reconstructable features.
		 */
		boost::optional<const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &>
				d_cached_present_day_geometries;

		/**
		 * The cached present day polygon meshes of the reconstructable features (where applicable).
		 */
		boost::optional<std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> > >
				d_cached_present_day_polygon_meshes;

		/**
		 * The cached present day geometries spatial partition.
		 */
		boost::optional<geometries_spatial_partition_type::non_null_ptr_type>
				d_cached_present_day_geometries_spatial_partition;

		/**
		 * The cached locations of the present day geometries in the spatial partition.
		 */
		boost::optional<std::vector<GPlatesMaths::CubeQuadTreeLocation> >
				d_cached_present_day_geometries_spatial_partition_locations;

		/**
		 * Cached reconstruction time.
		 */
		boost::optional<GPlatesMaths::real_t> d_cached_reconstruction_time;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The subject token that clients can use to determine if the reconstructable
		 * feature collections have changed.
		 */
		mutable GPlatesUtils::SubjectToken d_reconstructable_feature_collections_subject_token;


		explicit
		ReconstructLayerProxy(
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const ReconstructParams &reconstruct_params) :
			d_reconstruct_method_registry(reconstruct_method_registry),
			d_reconstruct_context(reconstruct_method_registry, reconstruct_params),
			// Start off with a reconstruction layer proxy that does identity rotations.
			d_current_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
			d_current_reconstruction_time(0)
		{  }


	// This method is public so that @a ReconstructLayerTask can flush any RFGs when
	// it is deactivated - this is done so that topologies will no longer find the RFGs
	// when they lookup observers of topological section features.
	// This issue exists because the topology layers do not restrict topological sections
	// to their input channels (and hence have no input channels for topological sections).
	public:
		/**
		 * Resets any cached variables forcing them to be recalculated next time they're accessed.
		 */
		void
		reset_reconstructed_feature_geometry_caches();
	private:


		/**
		 * Resets any cached variables forcing them to be recalculated next time they're accessed.
		 */
		void
		reset_reconstructable_feature_collection_caches();


		/**
		 * Checks if the specified input layer proxy has changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		template <class InputLayerProxyWrapperType>
		void
		check_input_layer_proxy(
				InputLayerProxyWrapperType &input_layer_proxy_wrapper);


		/**
		 * Checks if any input layer proxies have changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		void
		check_input_layer_proxies();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTLAYERPROXY_H
