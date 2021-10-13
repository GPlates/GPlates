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

		//! Typedef for a spatial partition of rendered geometries.
		typedef GPlatesMaths::CubeQuadTreePartition<RenderedGeometry> rendered_geometries_spatial_partition_type;


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
		 * Sets the viewport zoom factor.
		 * Note: this does nothing unless 'this' @a RenderedGeometryLayer
		 * was created using the zoom-dependent version of constructor.
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
		 * NOTE: Using @a add_rendered_geometries will break this guarantee.
		 *
		 * NOTE: Once/if @a add_rendered_geometries is called after @a clear_rendered_geometries
		 * then calls to this method, interleaved with adding more rendered geometries, will
		 * cause this method to be slow.
		 */
		RenderedGeometry
		get_rendered_geometry(
				rendered_geometry_index_type rendered_geom_index) const;


		/**
		 * Returns the rendered geometries in a spatial partition - however
		 * @a add_rendered_geometries must have been called at least once.
		 *
		 * This is an alternative way to traverse the rendered geometries -
		 * an alternative to @a get_rendered_geometry or iterating over the range
		 * @a rendered_geometry_begin to @a rendered_geometry_end.
		 *
		 * Note that boost::none is returned if the implementation behind this
		 * rendered geometry layer is zoom-dependent or if @a add_rendered_geometries
		 * was never called after @a clear_rendered_geometries.
		 */
		boost::optional<const rendered_geometries_spatial_partition_type &>
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
		 * The order added will be the order returned by @a get_rendered_geometry, or by
		 * the iteration range @a rendered_geometry_begin and @a rendered_geometry_end,
		 * *only* if @a clear_rendered_geometries is called after a call to
		 * @a add_rendered_geometries and before this call.
		 * In other words mixing @a add_rendered_geometry and @a add_rendered_geometries
		 * in the same group of rendered geometries will cause undefined iteration order.
		 */
		void
		add_rendered_geometry(
				RenderedGeometry);


		/**
		 * Adds rendered geometries and passes ownership of the spatial partition into the layer.
		 *
		 * If the implementation behind this layer is zoom-independent then the spatial partition
		 * is used as is, otherwise the spatial partition is simply traversed to get the
		 * rendered geometries and then discarded.
		 *
		 * NOTE: Calling this will not guarantee that the order of rendered geometries added
		 * will be the same as the order returned by @a get_rendered_geometry.
		 *
		 * NOTE: Caller should ideally give up ownership of the spatial partition or at least
		 * not modify it after this call.
		 */
		void
		add_rendered_geometries(
				const GPlatesGlobal::PointerTraits<rendered_geometries_spatial_partition_type>::non_null_ptr_type &
						rendered_geometries_spatial_partition);


		void
		clear_rendered_geometries();


		void
		accept_visitor(
				ConstRenderedGeometryLayerVisitor &) const;


		void
		accept_visitor(
				RenderedGeometryLayerVisitor &);

	signals:
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
