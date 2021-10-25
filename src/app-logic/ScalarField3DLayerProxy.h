/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPROXY_H
#define GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPROXY_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedScalarField3D.h"
#include "ScalarField3DLayerTask.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"

#include "maths/GeometryOnSphere.h"
#include "maths/types.h"

#include "model/FeatureHandle.h"

#include "property-values/TextContent.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesAppLogic
{
	/**
	 * A layer proxy for a 3D scalar field to be visualised using volume rendering.
	 */
	class ScalarField3DLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ScalarField3DLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<ScalarField3DLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ScalarField3DLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarField3DLayerProxy> non_null_ptr_to_const_type;

		//! Typedef for a sequence of surface polygon mask geometries (polylines, polygons).
		typedef std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> surface_polygons_mask_seq_type;

		//! Typedef for a sequence of surface polygon mask geometries (polylines, polygons).
		typedef std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> cross_sections_seq_type;


		/**
		 * Creates a @a ScalarField3DLayerProxy object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ScalarField3DLayerProxy());
		}


		/**
		 * Returns the scalar field filename for the current reconstruction time.
		 */
		const boost::optional<GPlatesPropertyValues::TextContent> &
		get_scalar_field_filename()
		{
			return get_scalar_field_filename(d_current_reconstruction_time);
		}

		/**
		 * Returns the scalar field filename at the specified reconstruction time.
		 */
		const boost::optional<GPlatesPropertyValues::TextContent> &
		get_scalar_field_filename(
				const double &reconstruction_time);


		/**
		 * Returns the resolved scalar field for the current reconstruction time.
		 *
		 * This is currently (a derivation of @a ReconstructionGeometry) that just references this layer proxy.
		 * An example client of @a ResolvedScalarField3D is @a GLVisualLayers which is
		 * responsible for *visualising* the scalar field on the screen.
		 *
		 * Returns boost::none if there is no input scalar field feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedScalarField3D::non_null_ptr_type>
		get_resolved_scalar_field_3d()
		{
			return get_resolved_scalar_field_3d(d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved scalar field for the specified time.
		 *
		 * Returns boost::none if there is no input scalar field feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedScalarField3D::non_null_ptr_type>
		get_resolved_scalar_field_3d(
				const double &reconstruction_time);


		/**
		 * Returns the cross sections (geometries) for the current reconstruction time.
		 *
		 * These geometries are used for cross-sections of the 3D scalar field.
		 *
		 * The geometries can be reconstructed feature geometries (polygons and polylines),
		 * resolved topological geometries (polygons and polylines) and resolved networks.
		 *
		 * Returns false if there are no cross section layers connected (or no geometries in connected layers).
		 */
		bool
		get_cross_sections(
				cross_sections_seq_type &cross_sections)
		{
			return get_cross_sections(cross_sections, d_current_reconstruction_time);
		}

		/**
		 * Returns the cross sections (geometries) for the specified time.
		 *
		 * Returns false if there are no cross section layers connected (or no geometries in connected layers).
		 */
		bool
		get_cross_sections(
				cross_sections_seq_type &cross_sections,
				const double &reconstruction_time);


		/**
		 * Returns the surface polygons mask (geometries) for the current reconstruction time.
		 *
		 * These geometries are used as surface fill masks of the 3D scalar field to limit the
		 * region in which scalar field is rendered.
		 *
		 * The geometries can be reconstructed feature geometries, resolved topological
		 * geometries and resolved networks.
		 *
		 * Returns false if there are no surface polygons mask layers connected
		 * (or no geometries in connected layers).
		 */
		bool
		get_surface_polygons_mask(
				surface_polygons_mask_seq_type &surface_polygons_mask)
		{
			return get_surface_polygons_mask(surface_polygons_mask, d_current_reconstruction_time);
		}

		/**
		 * Returns the surface polygons mask (geometries) for the specified time.
		 *
		 * Returns false if there are no surface polygons mask layers connected
		 * (or no geometries in connected layers).
		 */
		bool
		get_surface_polygons_mask(
				surface_polygons_mask_seq_type &surface_polygons_mask,
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if this scalar field layer proxy has changed.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();


		/**
		 * Returns the subject token that clients can use to determine if the scalar field itself
		 * has changed for the current reconstruction time.
		 *
		 * This is useful for time-dependent scalar fields.
		 */
		const GPlatesUtils::SubjectToken &
		get_scalar_field_subject_token()
		{
			return get_scalar_field_subject_token(d_current_reconstruction_time);
		}

		/**
		 * Returns the subject token that clients can use to determine if the scalar field itself
		 * has changed for the specified reconstruction time.
		 *
		 * This is useful for time-dependent scalar fields.
		 */
		const GPlatesUtils::SubjectToken &
		get_scalar_field_subject_token(
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if the scalar field feature has changed.
		 *
		 * This is useful for determining if only the scalar field feature has changed.
		 */
		const GPlatesUtils::SubjectToken &
		get_scalar_field_feature_subject_token();


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
		 * Specify the scalar field feature.
		 */
		void
		set_current_scalar_field_feature(
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> scalar_field_feature,
				const ScalarField3DLayerTask::Params &scalar_field_params);

		/**
		 * The scalar field feature has been modified.
		 */
		void
		modified_scalar_field_feature(
				const ScalarField3DLayerTask::Params &scalar_field_params);


		//! Add a 'cross section' reconstructed static geometries layer proxy.
		void
		add_cross_section_reconstructed_geometries_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometries_layer_proxy);

		//! Remove a 'cross section' reconstructed static geometries layer proxy.
		void
		remove_cross_section_reconstructed_geometries_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometries_layer_proxy);

		//! Add a 'cross section' topological boundary resolver layer proxy.
		void
		add_cross_section_topological_boundary_resolver_layer_proxy(
				const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

		//! Remove a 'cross section' topological boundary resolver layer proxy.
		void
		remove_cross_section_topological_boundary_resolver_layer_proxy(
				const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

		//! Add a 'cross section' topological network resolver layer proxy.
		void
		add_cross_section_topological_network_resolver_layer_proxy(
				const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy);

		//! Remove a 'cross section' topological network resolver layer proxy.
		void
		remove_cross_section_topological_network_resolver_layer_proxy(
				const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy);


		//! Add a 'surface polygons mask' reconstructed static geometries layer proxy.
		void
		add_surface_polygons_mask_reconstructed_geometries_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometries_layer_proxy);

		//! Remove a 'surface polygons mask' reconstructed static geometries layer proxy.
		void
		remove_surface_polygons_mask_reconstructed_geometries_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometries_layer_proxy);

		//! Add a 'surface polygons mask' topological boundary resolver layer proxy.
		void
		add_surface_polygons_mask_topological_boundary_resolver_layer_proxy(
				const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

		//! Remove a 'surface polygons mask' topological boundary resolver layer proxy.
		void
		remove_surface_polygons_mask_topological_boundary_resolver_layer_proxy(
				const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

		//! Add a 'surface polygons mask' topological network resolver layer proxy.
		void
		add_surface_polygons_mask_topological_network_resolver_layer_proxy(
				const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy);

		//! Remove a 'surface polygons mask' topological network resolver layer proxy.
		void
		remove_surface_polygons_mask_topological_network_resolver_layer_proxy(
				const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy);

	private:
		/**
		 * Potentially time-varying feature properties for the currently resolved scalar field
		 * (ie, at the cached reconstruction time).
		 */
		struct ResolvedScalarFieldFeatureProperties
		{
			void
			invalidate()
			{
				cached_scalar_field_filename = boost::none;
			}

			//! The scalar field filename.
			boost::optional<GPlatesPropertyValues::TextContent> cached_scalar_field_filename;

			//! The reconstruction time.
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};


		/**
		 * The cached cross sections (from other layers).
		 */
		struct CrossSections
		{
			void
			invalidate()
			{
				cached_geometries = boost::none;
				cached_reconstruction_time = boost::none;
			}

			/**
			 * The cached geometries.
			 */
			boost::optional<cross_sections_seq_type> cached_geometries;

			/**
			 * The reconstruction time of the cached geometries.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};


		/**
		 * The cached surface polygons mask (from other layers).
		 */
		struct SurfacePolygonsMask
		{
			void
			invalidate()
			{
				cached_geometries = boost::none;
				cached_reconstruction_time = boost::none;
			}

			/**
			 * The cached geometries.
			 */
			boost::optional<surface_polygons_mask_seq_type> cached_geometries;

			/**
			 * The reconstruction time of the cached geometries.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};


		/**
		 * The scalar field input feature.
		 */
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_current_scalar_field_feature;

		/**
		 * Used to get cross section geometries from reconstructed feature geometries.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy>
				d_current_cross_section_reconstructed_geometry_layer_proxies;

		/**
		 * Used to get surface polygon mask geometries from reconstructed feature geometries.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy>
				d_current_surface_polygons_mask_reconstructed_geometry_layer_proxies;

		/**
		 * Used to get cross section geometries from resolved topological boundaries.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyGeometryResolverLayerProxy>
				d_current_cross_section_topological_boundary_resolver_layer_proxies;

		/**
		 * Used to get surface polygon mask geometries from resolved topological boundaries.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyGeometryResolverLayerProxy>
				d_current_surface_polygons_mask_topological_boundary_resolver_layer_proxies;

		/**
		 * Used to get cross section geometries from resolved topological networks.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyNetworkResolverLayerProxy>
				d_current_cross_section_topological_network_resolver_layer_proxies;

		/**
		 * Used to get surface polygon mask geometries from resolved topological networks.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyNetworkResolverLayerProxy>
				d_current_surface_polygons_mask_topological_network_resolver_layer_proxies;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		//! Time-varying (potentially) scalar field feature properties.
		ResolvedScalarFieldFeatureProperties d_cached_resolved_scalar_field_feature_properties;

		/**
		 * The cached cross sections (from other layers).
		 */
		CrossSections d_cached_cross_sections;

		/**
		 * The cached surface polygons mask (from other layers).
		 */
		SurfacePolygonsMask d_cached_surface_polygons_mask;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The subject token that clients can use to determine if the scalar field itself has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_scalar_field_subject_token;

		/**
		 * The subject token that clients can use to determine if the scalar field feature has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_scalar_field_feature_subject_token;


		ScalarField3DLayerProxy() :
			d_current_reconstruction_time(0)
		{  }


		void
		invalidate_scalar_field_feature();

		void
		invalidate_scalar_field();

		void
		invalidate();


		/**
		 * Attempts to resolve a scalar field.
		 *
		 * Can fail if not enough information is available to resolve the scalar field.
		 * Such as no scalar field feature or scalar field feature does not have the required property values.
		 * In this case the returned value will be false.
		 */
		bool
		resolve_scalar_field_feature(
				const double &reconstruction_time);


		//! Sets some scalar field parameters.
		void
		set_scalar_field_params(
				const ScalarField3DLayerTask::Params &raster_params);


		/**
		 * Checks if the specified cross section input layer proxy has changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		template <class InputLayerProxyWrapperType>
		void
		check_cross_section_input_layer_proxy(
				InputLayerProxyWrapperType &input_layer_proxy_wrapper);

		/**
		 * Checks if the specified surface polygons mask input layer proxy has changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		template <class InputLayerProxyWrapperType>
		void
		check_surface_polygons_mask_input_layer_proxy(
				InputLayerProxyWrapperType &input_layer_proxy_wrapper);


		/**
		 * Checks if any input layer proxies have changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		void
		check_input_layer_proxies();

		void
		check_cross_section_input_layer_proxies();

		void
		check_surface_polygons_mask_input_layer_proxies();
	};
}

#endif // GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPROXY_H
