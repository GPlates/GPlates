/* $Id$ */

/**
 * \file 
 * Used to group a subset of @a RenderedGeometry objects.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYER_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYER_H

#include <iterator>
#include <memory>
#include <boost/any.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <QObject>

#include "RenderedGeometry.h"

#include "maths/CubeQuadTreeLocation.h"
#include "maths/MathsFwd.h"


namespace GPlatesViewOperations
{
	class ConstRenderedGeometryLayerVisitor;
	class RenderedGeometryIteratorImpl;
	class RenderedGeometryLayerImpl;
	class RenderedGeometryLayerVisitor;

	class RenderedGeometryLayer :
		public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Typedef for arbitrary user-supplied data that will be returned when
		 * @a layer_was_updated signal is emitted.
		 */
		typedef boost::any user_data_type;

		/**
		 * Typedef for a @a RenderedGeometry index.
		 */
		typedef unsigned int rendered_geometry_index_type;

		//! Typedef for pointer to rendered geometry layer implementation.
		typedef boost::intrusive_ptr<RenderedGeometryLayerImpl> rendered_geometry_layer_impl_ptr_type;


		/**
		 * Rendered geometries stored in a spatial partition are sorted spatially rather than by
		 * render (draw) order - so this structure associated each rendered geometry with its render order.
		 */
		struct PartitionedRenderedGeometry
		{
			PartitionedRenderedGeometry(
					const RenderedGeometry &rendered_geometry_,
					rendered_geometry_index_type render_order_) :
				rendered_geometry(rendered_geometry_),
				render_order(render_order_)
			{  }

			RenderedGeometry rendered_geometry;
			rendered_geometry_index_type render_order;
		};

		//! Typedef for a spatial partition of rendered geometries.
		typedef GPlatesMaths::CubeQuadTreePartition<PartitionedRenderedGeometry>
				rendered_geometries_spatial_partition_type;


		/**
		 * Iterator over rendered geometries.
		 */
		class RenderedGeometryIterator :
				public std::iterator<std::forward_iterator_tag, RenderedGeometry>,
				public boost::forward_iteratable<RenderedGeometryIterator, RenderedGeometry *>
		{
		public:
			RenderedGeometryIterator(
					rendered_geometry_layer_impl_ptr_type layer_impl,
					rendered_geometry_index_type rendered_geom_index);

			RenderedGeometryIterator(
					const RenderedGeometryIterator &rhs);

			~RenderedGeometryIterator();

			/**
			 * Dereference operator.
			 * 'operator->()' provided by base class boost::forward_iteratable.
			 */
			const RenderedGeometry &
			operator*() const;

			/**
			 * Pre-increment operator.
			 * Post-increment operator provided by base class boost::forward_iteratable.
			 */
			RenderedGeometryIterator &
			operator++();

			/**
			 * Equality comparison operator.
			 * Inequality operator provided by base class boost::forward_iteratable.
			 */
			friend
			bool
			operator==(
					const RenderedGeometryIterator &lhs,
					const RenderedGeometryIterator &rhs);

		private:
			rendered_geometry_layer_impl_ptr_type d_layer_impl;
			rendered_geometry_index_type d_rendered_geom_index;
		};

		//! Typedef for iterator over rendered geometries.
		typedef RenderedGeometryIterator iterator;


		/**
		 * Construct a regular rendered geometry layer where each rendered geometry
		 * added gets pushed onto end of a list of rendered geometries.
		 *
		 * @param user_data arbitrary user-supplied data that will be returned
		 *        when @a layer_was_updated signal is emitted - currently this should
		 *        only be used by @a RenderedGeometryCollection.
		 */
		RenderedGeometryLayer(
				const double &viewport_zoom_factor,
				user_data_type user_data);


		/**
		 * Construct a zoom-dependent rendered geometry layer where the globe is divided
		 * into roughly equal area latitude/longitude bins that the rendered geometries
		 * are added to.
		 * When querying the rendered geometries, only one rendered geometry
		 * per bin is returned (the closest to the centre of the bin) and the
		 * order returned by query is not the same as the order of addition.
		 * Although this only applies to certain types of rendered geometry such as
		 * rendered point on sphere and rendered direction arrow which have single
		 * point geometry - the other geometries are treated in a zoom-independent
		 * manner.
		 *
		 * @param ratio_zoom_dependent_bin_dimension_to_globe_radius the size of the
		 *        zoom-dependent bin size relative to the globe radius when the globe fills
		 *        the viewport window (this is a view-dependent scalar).
		 * @param viewport_zoom_factor the current viewport zoom factor.
		 * @param user_data arbitrary user-supplied data that will be returned
		 *        when @a layer_was_updated signal is emitted - currently this should
		 *        only be used by @a RenderedGeometryCollection.
		 */
		RenderedGeometryLayer(
				float ratio_zoom_dependent_bin_dimension_to_globe_radius,
				const double &viewport_zoom_factor,
				user_data_type user_data);


		~RenderedGeometryLayer();


		/**
		 * If set to a non-zero value then constructs a zoom-dependent rendered geometry layer
		 * where the globe is divided into roughly equal area latitude/longitude bins that the
		 * rendered geometries are added to (at most one geometry is rendered per bin).
		 *
		 * If set to zero then there is no limit to the number of geometries rendered per bin region.
		 *
		 * Note that changing from a zero to non-zero value (or vice versa) will change the
		 * internal rendered layer implementation.
		 *
		 * If the specified value equals the internal value then nothing changes.
		 */
		void
		set_ratio_zoom_dependent_bin_dimension_to_globe_radius(
				float ratio_zoom_dependent_bin_dimension_to_globe_radius = 0);


		/**
		 * Sets the viewport zoom factor.
		 *
		 * Note: this does nothing unless 'this' @a RenderedGeometryLayer
		 * was created using the zoom-dependent version of constructor, or
		 * @a set_ratio_zoom_dependent_bin_dimension_to_globe_radius was called with a non-zero value.
		 */
		void
		set_viewport_zoom_factor(
				const double &viewport_zoom_factor);


		void
		set_active(
				bool active = true);


		bool
		is_active() const
		{
			return d_is_active;
		}


		bool
		is_empty() const;


		unsigned int
		get_num_rendered_geometries() const;


		/**
		 * Returns the 'rendered_geom_index'th rendered geometry added via
		 * @a add_rendered_geometry.
		 *
		 * The only exception to the ordering is when this layer is acting as a zoom-dependent
		 * rendered geometry layer *and* there's a mixture of zoom-dependent rendered geometries
		 * (currently points and arrows) and zoom-independent rendered geometries (currently
		 * multipoints, polylines and polygons). In this case the zoom-independent rendered geometries
		 * are ordered before the zoom-independent rendered geometries. And while the zoom-independent
		 * rendered geometries retain their ordering relative to each other, the zoom-dependent
		 * rendered geometries do not (since some rendered geometries can be rejected when they are
		 * added due to the fact that there can only be one per surface area bin).
		 */
		RenderedGeometry
		get_rendered_geometry(
				rendered_geometry_index_type rendered_geom_index) const;


		/**
		 * Returns the rendered geometries in a spatial partition.
		 *
		 * This is an alternative way to traverse the rendered geometries -
		 * an alternative to @a get_rendered_geometry or iterating over the range
		 * @a rendered_geometry_begin to @a rendered_geometry_end.
		 *
		 * NOTE: The returned spatial partitioned could be modified if rendered geometries are
		 * subsequently cleared or added.
		 *
		 * Note that if this layer is acting as a zoom-dependent rendered layer then some rendered
		 * geometries will exist in the root of the partition (ie, less efficient partitioning).
		 */
		GPlatesUtils::non_null_intrusive_ptr<const rendered_geometries_spatial_partition_type>
		get_rendered_geometries() const;


		/**
		 * Begin iterator for sequence of @a RenderedGeometry objects.
		 *
		 * The order of iteration is the same as the order of @a add_rendered_geometry calls.
		 *
		 * NOTE: Using @a add_rendered_geometries will break this guarantee.
		 */
		iterator
		rendered_geometry_begin() const;


		/**
		 * End iterator for sequence of @a RenderedGeometry objects.
		 *
		 * The order of iteration is the same as the order of @a add_rendered_geometry calls.
		 *
		 * NOTE: Using @a add_rendered_geometries will break this guarantee.
		 */
		iterator
		rendered_geometry_end() const;


		/**
		 * Adds a rendered geometry to the list.
		 *
		 * The order added will be the same order returned by @a get_rendered_geometry, or by
		 * the iteration range @a rendered_geometry_begin and @a rendered_geometry_end.
		 *
		 * Specify @a cube_quad_tree_location if you the rendered geometry has been partitioned
		 * into a cube quad tree spatial partition.
		 */
		void
		add_rendered_geometry(
				RenderedGeometry,
				boost::optional<const GPlatesMaths::CubeQuadTreeLocation &> cube_quad_tree_location = boost::none);


		void
		clear_rendered_geometries();


		void
		accept_visitor(
				ConstRenderedGeometryLayerVisitor &) const;


		void
		accept_visitor(
				RenderedGeometryLayerVisitor &);

	Q_SIGNALS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Signal is emitted whenever this rendered geometry layer has been
		 * updated. Currently only @a RenderedGeometryCollection needs to listen to this.
		 *
		 * This includes:
		 * - any @a RenderedGeometry objects added.
		 * - clearing of @a RenderedGeometry objects.
		 * - changing active status.
		 */
		void
		layer_was_updated(
				GPlatesViewOperations::RenderedGeometryLayer &,
				GPlatesViewOperations::RenderedGeometryLayer::user_data_type user_data);



	private:
		user_data_type d_user_data;
		rendered_geometry_layer_impl_ptr_type d_impl;
		bool d_is_active;
	};


	bool
	operator==(
			const RenderedGeometryLayer::RenderedGeometryIterator &lhs,
			const RenderedGeometryLayer::RenderedGeometryIterator &rhs);
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYER_H
