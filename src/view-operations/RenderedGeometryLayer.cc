/* $Id$ */

/**
 * \file 
 * Used to group a subset of @a RenderedGeometry objects.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <algorithm>
#include <vector>
#include <boost/bind.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>

#include "RenderedGeometryLayer.h"

#include "RenderedGeometryLayerVisitor.h"
#include "RenderedGeometryVisitor.h"
#include "RenderedPointOnSphere.h"
#include "RenderedRadialArrow.h"
#include "RenderedSmallCircle.h"
#include "RenderedTangentialArrow.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "utils/LatLonAreaSampling.h"
#include "utils/ReferenceCount.h"

#include "utils/Profile.h"


namespace GPlatesViewOperations
{
	/**
	 * Interface for implementation of rendered geometry layer.
	 */
	class RenderedGeometryLayerImpl :
			public GPlatesUtils::ReferenceCount<RenderedGeometryLayerImpl>
	{
	public:
		//! Convenience typedef for rendered geometry index.
		typedef RenderedGeometryLayer::rendered_geometry_index_type rendered_geometry_index_type;

		//! Convenience typedef for a rendered geometry stored in a spatial partition.
		typedef RenderedGeometryLayer::PartitionedRenderedGeometry partitioned_rendered_geometry_type;

		//! Convenience typedef for rendered geometries spatial partition.
		typedef RenderedGeometryLayer::rendered_geometries_spatial_partition_type
				rendered_geometries_spatial_partition_type;

		/**
		 * The default depth of the rendered geometries spatial partition (the quad trees in each cube face).
		 */
		static const unsigned int DEFAULT_SPATIAL_PARTITION_DEPTH = 7;


		virtual
		~RenderedGeometryLayerImpl()
		{  }

		virtual
		void
		set_ratio_zoom_dependent_bin_dimension_to_globe_radius(
				float ratio_zoom_dependent_bin_dimension_to_globe_radius) = 0;

		virtual
		void
		set_viewport_zoom_factor(
				const double &viewport_zoom_factor) = 0;

		virtual
		const double &
		get_viewport_zoom_factor() const = 0;

		virtual
		bool
		is_empty() const = 0;

		virtual
		unsigned int
		get_num_rendered_geometries() const = 0;

		virtual
		const RenderedGeometry &
		get_rendered_geometry(
				rendered_geometry_index_type) const = 0;

		virtual
		rendered_geometries_spatial_partition_type::non_null_ptr_to_const_type
		get_rendered_geometries() const = 0;

		virtual
		void
		add_rendered_geometry(
				RenderedGeometry,
				boost::optional<const GPlatesMaths::CubeQuadTreeLocation &> cube_quad_tree_location) = 0;

		virtual
		void
		clear_rendered_geometries() = 0;
	};


	namespace
	{
		/**
		 * Standard rendered layer implementation that simply appends each added
		 * rendered geometry to the end of a sequence.
		 */
		class ZoomIndependentLayerImpl :
				public RenderedGeometryLayerImpl
		{
		public:
			explicit
			ZoomIndependentLayerImpl(
					const double &viewport_zoom_factor) :
				d_rendered_geom_spatial_partition(
						rendered_geometries_spatial_partition_type::create(
								DEFAULT_SPATIAL_PARTITION_DEPTH)),
				d_current_viewport_zoom_factor(viewport_zoom_factor)
			{  }

			virtual
			void
			set_ratio_zoom_dependent_bin_dimension_to_globe_radius(
					float ratio_zoom_dependent_bin_dimension_to_globe_radius)
			{
				// Do nothing.
			}

			virtual
			void
			set_viewport_zoom_factor(
					const double &viewport_zoom_factor)
			{
				// Only store it in case client requests it - otherwise we're not interested in zoom.
				d_current_viewport_zoom_factor = viewport_zoom_factor;
			}

			virtual
			const double &
			get_viewport_zoom_factor() const
			{
				return d_current_viewport_zoom_factor;
			}

			virtual
			bool
			is_empty() const
			{
				// Rendered geoms added to both the sequence and spatial partition so just pick one.
				return d_rendered_geom_seq.empty();
			}


			virtual
			unsigned int
			get_num_rendered_geometries() const
			{
				// Rendered geoms added to both the sequence and spatial partition so just pick one.
				return d_rendered_geom_seq.size();
			}

			virtual
			const RenderedGeometry &
			get_rendered_geometry(
					rendered_geometry_index_type rendered_geom_index) const
			{
				// Rendered geoms added to both the sequence and spatial partition,
				// but only the sequence is ordered.
				return d_rendered_geom_seq[rendered_geom_index];
			}


			virtual
			rendered_geometries_spatial_partition_type::non_null_ptr_to_const_type
			get_rendered_geometries() const
			{
				return d_rendered_geom_spatial_partition;
			}


			virtual
			void
			add_rendered_geometry(
					RenderedGeometry rendered_geom,
					boost::optional<const GPlatesMaths::CubeQuadTreeLocation &> cube_quad_tree_location)
			{
				// Rendered geometries are to be rendered in the order they are added.
				const rendered_geometry_index_type render_order = d_rendered_geom_seq.size();

				// Add to the sequence of rendered geometries.
				d_rendered_geom_seq.push_back(rendered_geom);

				// Keep track of draw order when storing in a spatial partition.
				const partitioned_rendered_geometry_type partitioned_rendered_geom(
						rendered_geom,
						render_order);

				// Also add to the spatial partition.
				// If the caller has specified a destination node in the spatial partition then add to that.
				if (cube_quad_tree_location)
				{
					d_rendered_geom_spatial_partition->add(
							partitioned_rendered_geom,
							cube_quad_tree_location.get());
				}
				else // otherwise just add to the root of the spatial partition...
				{
					d_rendered_geom_spatial_partition->add_unpartitioned(partitioned_rendered_geom);
				}
			}


			virtual
			void
			clear_rendered_geometries()
			{
				d_rendered_geom_seq.clear();
				d_rendered_geom_spatial_partition->clear();
			}

		private:

			//! Typedef for sequence of @a RenderedGeometry objects.
			typedef std::vector<RenderedGeometry> rendered_geom_seq_type;

			rendered_geom_seq_type d_rendered_geom_seq;
			rendered_geometries_spatial_partition_type::non_null_ptr_type d_rendered_geom_spatial_partition;
			double d_current_viewport_zoom_factor;

		};


		/**
		 * Determines if a @a RenderedGeometry should be classified as zoom-dependent or not.
		 */
		class IsZoomDependent :
				private GPlatesViewOperations::ConstRenderedGeometryVisitor
		{
		public:
			/**
			 * Returns true and position on sphere of rendered geometry if the type
			 * of rendered geometry is zoom-dependent.
			 */
			boost::optional<GPlatesMaths::PointOnSphere>
			operator()(
					const RenderedGeometry &rendered_geom)
			{
				d_position_on_sphere = boost::none;

				// Visit the rendered geometry to determine if it's zoom-dependent.
				rendered_geom.accept_visitor(*this);

				return d_position_on_sphere;
			}


		private:
			virtual
			void
			visit_rendered_point_on_sphere(
					const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
			{
				d_position_on_sphere = rendered_point_on_sphere.get_point_on_sphere();
			}

			virtual
			void
			visit_rendered_radial_arrow(
					const RenderedRadialArrow &rendered_radial_arrow)
			{
				d_position_on_sphere = rendered_radial_arrow.get_position();
			}

			virtual
			void
			visit_rendered_tangential_arrow(
					const RenderedTangentialArrow &rendered_tangential_arrow)
			{
				d_position_on_sphere = rendered_tangential_arrow.get_start_position();
			}

		private:
			boost::optional<GPlatesMaths::PointOnSphere> d_position_on_sphere;
		};


		/**
		 * Returns true and position on sphere of rendered geometry if the type
		 * of rendered geometry is zoom-dependent.
		 */
		inline
		boost::optional<GPlatesMaths::PointOnSphere>
		is_zoom_dependent(
				const RenderedGeometry &rendered_geom)
		{
			return IsZoomDependent()(rendered_geom);
		}


		/**
		 * Sequence of @a RenderedGeometry objects that changes with zoom.
		 */
		class ZoomDependentLayerImpl :
				public RenderedGeometryLayerImpl
		{
		public:
			ZoomDependentLayerImpl(
					float ratio_zoom_dependent_bin_dimension_to_globe_radius,
					const double &viewport_zoom_factor) :
				d_current_ratio_zoom_dependent_bin_dimension_to_globe_radius(
						ratio_zoom_dependent_bin_dimension_to_globe_radius),
				d_current_viewport_zoom_factor(viewport_zoom_factor),
				d_zoom_independent_rendered_geom_spatial_partition(
						rendered_geometries_spatial_partition_type::create(
								DEFAULT_SPATIAL_PARTITION_DEPTH)),
				d_zoom_dependent_seq(
						get_zoom_dependent_sample_spacing(
							ratio_zoom_dependent_bin_dimension_to_globe_radius,
							viewport_zoom_factor))
			{  }


			virtual
			void
			set_ratio_zoom_dependent_bin_dimension_to_globe_radius(
					float ratio_zoom_dependent_bin_dimension_to_globe_radius)
			{
				// If the ratio hasn't changed then nothing to do.
				if (GPlatesMaths::are_almost_exactly_equal(
					ratio_zoom_dependent_bin_dimension_to_globe_radius,
					d_current_ratio_zoom_dependent_bin_dimension_to_globe_radius))
				{
					return;
				}

				d_current_ratio_zoom_dependent_bin_dimension_to_globe_radius =
						ratio_zoom_dependent_bin_dimension_to_globe_radius;
				reset_sample_spacing();
			}


			virtual
			void
			set_viewport_zoom_factor(
					const double &viewport_zoom_factor)
			{
				// If zoom factor hasn't changed then nothing to do.
				// We don't need exact bit-for-bit comparisons here, so we use GPlatesMaths::real_t.
				if (viewport_zoom_factor == d_current_viewport_zoom_factor)
				{
					return;
				}

				d_current_viewport_zoom_factor = viewport_zoom_factor;
				reset_sample_spacing();
			}

			virtual
			const double &
			get_viewport_zoom_factor() const
			{
				return d_current_viewport_zoom_factor.dval();
			}


			virtual
			bool
			is_empty() const
			{
				return d_zoom_independent_seq.empty() && d_zoom_dependent_seq.empty();
			}


			virtual
			unsigned int
			get_num_rendered_geometries() const
			{
				return d_zoom_independent_seq.size() + d_zoom_dependent_seq.get_num_sampled_elements();
			}


			virtual
			const RenderedGeometry &
			get_rendered_geometry(
					rendered_geometry_index_type rendered_geom_index) const
			{
				// The first range of indices index into the view-independent
				// rendered geometries.
				if (rendered_geom_index < d_zoom_independent_seq.size())
				{
					return d_zoom_independent_seq[rendered_geom_index];
				}

				// The second range of indices index into the view-dependent 
				// rendered geometries.
				const unsigned int zoom_dependent_index =
						rendered_geom_index - d_zoom_independent_seq.size();

				return d_zoom_dependent_seq.get_sampled_element(zoom_dependent_index);
			}


			virtual
			void
			add_rendered_geometry(
					RenderedGeometry rendered_geom,
					boost::optional<const GPlatesMaths::CubeQuadTreeLocation &> cube_quad_tree_location)
			{
				boost::optional<GPlatesMaths::PointOnSphere> zoom_dependent =
						is_zoom_dependent(rendered_geom);

				// Add to the appropriate sequence (zoom-dependent or zoom-independent).
				if (zoom_dependent)
				{
					// We can't make use of the zoom-dependent spatial partition since we're discarding
					// some rendered geometries (only keeping one per zoom-dependent sampling bin).

					const GPlatesMaths::PointOnSphere &point_on_sphere_location = *zoom_dependent;

					// Add to the lat/lon area sampling.
					d_zoom_dependent_seq.add_element(rendered_geom, point_on_sphere_location);
				}
				else // zoom-independent...
				{
					// Zoom-independent rendered geometries are to be rendered in the order they are added.
					// And zoom-independent geometries are rendered before zoom-dependent geometries.
					const rendered_geometry_index_type render_order = d_zoom_independent_seq.size();

					d_zoom_independent_seq.push_back(rendered_geom);

					// Keep track of draw order when storing in a spatial partition.
					const partitioned_rendered_geometry_type partitioned_rendered_geom(
							rendered_geom,
							render_order);

					// Also add to the zoom-independent spatial partition.
					// If the caller has specified a destination node in the spatial partition then add to that.
					if (cube_quad_tree_location)
					{
						d_zoom_independent_rendered_geom_spatial_partition->add(
								partitioned_rendered_geom,
								cube_quad_tree_location.get());
					}
					else // otherwise just add to the root of the spatial partition...
					{
						d_zoom_independent_rendered_geom_spatial_partition->add_unpartitioned(
								partitioned_rendered_geom);
					}
				}
			}


			virtual
			void
			clear_rendered_geometries()
			{
				d_zoom_independent_seq.clear();
				d_zoom_independent_rendered_geom_spatial_partition->clear();
				d_zoom_dependent_seq.clear_elements();
			}


			virtual
			rendered_geometries_spatial_partition_type::non_null_ptr_to_const_type
			get_rendered_geometries() const
			{
				if (d_zoom_dependent_seq.empty())
				{
					return d_zoom_independent_rendered_geom_spatial_partition;
				}

				// Copy the zoom-independent spatial partition and add the zoom-dependent
				// rendered geometries only to the root of the spatial partition.

				rendered_geometries_spatial_partition_type::non_null_ptr_type spatial_partition =
						rendered_geometries_spatial_partition_type::create(
								DEFAULT_SPATIAL_PARTITION_DEPTH);

				GPlatesMaths::CubeQuadTreePartitionUtils::merge(
						*spatial_partition,
						*d_zoom_independent_rendered_geom_spatial_partition);

				// Iterate over the zoom-dependent rendered geometries.
				const unsigned int num_zoom_dependent_geoms = d_zoom_dependent_seq.get_num_sampled_elements();
				for (unsigned int zoom_dependent_geom_index = 0;
					zoom_dependent_geom_index < num_zoom_dependent_geoms;
					++zoom_dependent_geom_index)
				{
					const RenderedGeometry &rendered_geom =
							d_zoom_dependent_seq.get_sampled_element(zoom_dependent_geom_index);

					// Zoom-dependent rendered geometries are to be rendered *after* the
					// zoom-independent ones.
					const rendered_geometry_index_type render_order =
							d_zoom_independent_seq.size() + zoom_dependent_geom_index;

					// Keep track of draw order when storing in a spatial partition.
					const partitioned_rendered_geometry_type partitioned_rendered_geom(
							rendered_geom,
							render_order);

					// Add to the root of the spatial partition (since have no spatial extent info).
					spatial_partition->add_unpartitioned(partitioned_rendered_geom);
				}

				return spatial_partition;
			}

		private:

			//! Typedef for sequence of zoom-independent @a RenderedGeometry objects.
			typedef std::vector<RenderedGeometry> zoom_independent_seq_type;

			//! Typedef for sequence of zoom-dependent @a RenderedGeometry objects.
			typedef GPlatesUtils::LatLonAreaSampling<RenderedGeometry> zoom_dependent_seq_type;


			float d_current_ratio_zoom_dependent_bin_dimension_to_globe_radius;
			GPlatesMaths::real_t d_current_viewport_zoom_factor;

			zoom_independent_seq_type d_zoom_independent_seq;
			rendered_geometries_spatial_partition_type::non_null_ptr_type
					d_zoom_independent_rendered_geom_spatial_partition;

			zoom_dependent_seq_type d_zoom_dependent_seq;


			/**
			 * Calculate a lat/lon area sample spacing from the viewport zoom factor.
			 */
			static
			double
			get_zoom_dependent_sample_spacing(
					float ratio_zoom_dependent_bin_dimension_to_globe_radius,
					const double &viewport_zoom_factor)
			{
				double sample_spacing_degrees =
						ratio_zoom_dependent_bin_dimension_to_globe_radius /
						viewport_zoom_factor *
						(180.0 / GPlatesMaths::PI);

				// Clamp sample spacing to a minimum value otherwise we can run out of memory
				// due to a large number of sample bins - this is particularly noticeable when
				// zooming in to very high levels.
				static const double MIN_SAMPLE_SPACING_DEGREES = 0.25;
				if (sample_spacing_degrees < MIN_SAMPLE_SPACING_DEGREES)
				{
					sample_spacing_degrees = MIN_SAMPLE_SPACING_DEGREES;
				}

				return sample_spacing_degrees;
			}

			void
			reset_sample_spacing()
			{
				// Modify the lat/lon area sampling with a new spacing that reflects the current
				// zoom factor and bin dimension to globe radius ratio.
				const double sample_bin_angle_spacing_degrees =
						get_zoom_dependent_sample_spacing(
								d_current_ratio_zoom_dependent_bin_dimension_to_globe_radius,
								d_current_viewport_zoom_factor.dval());

				d_zoom_dependent_seq.reset_sample_spacing(sample_bin_angle_spacing_degrees);
			}
		};


		/**
		 * Helper structure when copying between render layer impl's in render order.
		 */
		struct PartitionedLocatedRenderedGeometry
		{
			PartitionedLocatedRenderedGeometry(
					const RenderedGeometryLayer::PartitionedRenderedGeometry &parititioned_rendered_geometry_,
					const GPlatesMaths::CubeQuadTreeLocation &cube_quad_tree_location_) :
				parititioned_rendered_geometry(parititioned_rendered_geometry_),
				cube_quad_tree_location(cube_quad_tree_location_)
			{  }

			RenderedGeometryLayer::PartitionedRenderedGeometry parititioned_rendered_geometry;
			GPlatesMaths::CubeQuadTreeLocation cube_quad_tree_location;

			//! Used to sort by render order.
			struct SortRenderOrder
			{
				bool
				operator()(
						const PartitionedLocatedRenderedGeometry &lhs,
						const PartitionedLocatedRenderedGeometry &rhs) const
				{
					return lhs.parititioned_rendered_geometry.render_order <
							rhs.parititioned_rendered_geometry.render_order;
				}
			};
		};


		/**
		 * Copy the src rendered layer's rendered geometries and over to the dst layer in rendered order.
		 */
		void
		copy_rendered_geometries_in_render_order(
				RenderedGeometryLayerImpl &dst_rendered_geometry_layer_impl,
				const RenderedGeometryLayerImpl &src_rendered_geometry_layer_impl)
		{
			if (src_rendered_geometry_layer_impl.is_empty())
			{
				return;
			}

			std::vector<PartitionedLocatedRenderedGeometry> src_rendered_geoms;
			src_rendered_geoms.reserve(src_rendered_geometry_layer_impl.get_num_rendered_geometries());

			// Iterate over the src layer's rendered geometries spatial partition so that we can
			// obtain the cube quad tree location information for each rendered geometry.
			RenderedGeometryLayer::rendered_geometries_spatial_partition_type::non_null_ptr_to_const_type
					src_spatial_partition = src_rendered_geometry_layer_impl.get_rendered_geometries();
			RenderedGeometryLayer::rendered_geometries_spatial_partition_type::const_iterator
					src_rendered_geoms_iter = src_spatial_partition->get_iterator();
			for ( ; !src_rendered_geoms_iter.finished(); src_rendered_geoms_iter.next())
			{
				src_rendered_geoms.push_back(
						PartitionedLocatedRenderedGeometry(
								src_rendered_geoms_iter.get_element(),
								src_rendered_geoms_iter.get_location()));
			}

			// We need to add to the destination rendered layer in render order.
			std::sort(
					src_rendered_geoms.begin(),
					src_rendered_geoms.end(),
					PartitionedLocatedRenderedGeometry::SortRenderOrder());

			// Add to the destination rendered layer in render order.
			const unsigned int num_src_rendered_geoms = src_rendered_geoms.size();
			for (unsigned int n = 0; n < num_src_rendered_geoms; ++n)
			{
				dst_rendered_geometry_layer_impl.add_rendered_geometry(
						src_rendered_geoms[n].parititioned_rendered_geometry.rendered_geometry,
						src_rendered_geoms[n].cube_quad_tree_location);
			}
		}
	}
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryLayer(
		const double &viewport_zoom_factor,
		user_data_type user_data) :
	d_user_data(user_data),
	d_impl(new ZoomIndependentLayerImpl(viewport_zoom_factor)),
	d_is_active(false)
{
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryLayer(
		float ratio_zoom_dependent_bin_dimension_to_globe_radius,
		const double &viewport_zoom_factor,
		user_data_type user_data) :
	d_user_data(user_data),
	d_impl(new ZoomDependentLayerImpl(
			ratio_zoom_dependent_bin_dimension_to_globe_radius, viewport_zoom_factor)),
	d_is_active(false)
{
}


GPlatesViewOperations::RenderedGeometryLayer::~RenderedGeometryLayer()
{
	// boost::intrusive_ptr destructor requires complete type.
}


void
GPlatesViewOperations::RenderedGeometryLayer::set_ratio_zoom_dependent_bin_dimension_to_globe_radius(
		float ratio_zoom_dependent_bin_dimension_to_globe_radius)
{
	if (GPlatesMaths::are_almost_exactly_equal(ratio_zoom_dependent_bin_dimension_to_globe_radius, 0.0f))
	{
		// A value of zero means zoom *independent*.
		// However if we have a zoom *dependent* layer then we need to convert it.
		if (dynamic_cast<ZoomDependentLayerImpl *>(d_impl.get()))
		{
			// Create a new zoom *independent* implementation.
			rendered_geometry_layer_impl_ptr_type new_impl(
					new ZoomIndependentLayerImpl(d_impl->get_viewport_zoom_factor()));

			// Copy the old impl's rendered geometries and over to the new impl.
			copy_rendered_geometries_in_render_order(*new_impl, *d_impl);

			// Replace the old impl with the new impl.
			d_impl = new_impl;

			return;
		}
	}
	else // ratio_zoom_dependent_bin_dimension_to_globe_radius > 0 ...
	{
		// A non-zero value means zoom *dependent*.
		// However if we have a zoom *independent* layer then we need to convert it.
		if (dynamic_cast<ZoomIndependentLayerImpl *>(d_impl.get()))
		{
			// Create a new zoom *dependent* implementation.
			rendered_geometry_layer_impl_ptr_type new_impl(
					new ZoomDependentLayerImpl(
							ratio_zoom_dependent_bin_dimension_to_globe_radius,
							d_impl->get_viewport_zoom_factor()));

			// Copy the old impl's rendered geometries and over to the new impl.
			copy_rendered_geometries_in_render_order(*new_impl, *d_impl);

			// Replace the old impl with the new impl.
			d_impl = new_impl;

			return;
		}
	}

	// Otherwise the type of implementation has not changed so just delegate to it.
	d_impl->set_ratio_zoom_dependent_bin_dimension_to_globe_radius(ratio_zoom_dependent_bin_dimension_to_globe_radius);
}


void
GPlatesViewOperations::RenderedGeometryLayer::set_viewport_zoom_factor(
		const double &viewport_zoom_factor)
{
	d_impl->set_viewport_zoom_factor(viewport_zoom_factor);
}


void
GPlatesViewOperations::RenderedGeometryLayer::set_active(
		bool active)
{
	if (active != d_is_active)
	{
		d_is_active = active;

		Q_EMIT layer_was_updated(*this, d_user_data);
	}
}


bool
GPlatesViewOperations::RenderedGeometryLayer::is_empty() const
{
	return d_impl->is_empty();
}


unsigned int
GPlatesViewOperations::RenderedGeometryLayer::get_num_rendered_geometries() const
{
	return d_impl->get_num_rendered_geometries();
}


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryLayer::get_rendered_geometry(
		rendered_geometry_index_type rendered_geom_index) const
{
	return d_impl->get_rendered_geometry(rendered_geom_index);
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator
GPlatesViewOperations::RenderedGeometryLayer::rendered_geometry_begin() const
{
	return RenderedGeometryIterator(d_impl, 0);
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator
GPlatesViewOperations::RenderedGeometryLayer::rendered_geometry_end() const
{
	return RenderedGeometryIterator(d_impl, d_impl->get_num_rendered_geometries());
}


void
GPlatesViewOperations::RenderedGeometryLayer::add_rendered_geometry(
		RenderedGeometry rendered_geom,
		boost::optional<const GPlatesMaths::CubeQuadTreeLocation &> cube_quad_tree_location)
{
	d_impl->add_rendered_geometry(rendered_geom, cube_quad_tree_location);

	Q_EMIT layer_was_updated(*this, d_user_data);
}


void
GPlatesViewOperations::RenderedGeometryLayer::clear_rendered_geometries()
{
	// The empty checks were removed to ensure that the globe or map refresh
	// themselves even if no rendered geometries were created.
#if 0
	if (!d_impl->is_empty())
	{
#endif
		d_impl->clear_rendered_geometries();

		Q_EMIT layer_was_updated(*this, d_user_data);
#if 0
	}
#endif
}


GPlatesViewOperations::RenderedGeometryLayer::rendered_geometries_spatial_partition_type::non_null_ptr_to_const_type
GPlatesViewOperations::RenderedGeometryLayer::get_rendered_geometries() const
{
	return d_impl->get_rendered_geometries();
}


void
GPlatesViewOperations::RenderedGeometryLayer::accept_visitor(
		ConstRenderedGeometryLayerVisitor &visitor) const
{
	// Ask the visitor if it wants to visit us.
	// It can query our active status to decide.
	if (visitor.visit_rendered_geometry_layer(*this))
	{
		// Visit each RenderedGeometry.
		std::for_each(
			rendered_geometry_begin(),
			rendered_geometry_end(),
			boost::bind(&RenderedGeometry::accept_visitor, _1, boost::ref(visitor)));
	}
}


void
GPlatesViewOperations::RenderedGeometryLayer::accept_visitor(
		RenderedGeometryLayerVisitor &visitor)
{
	// Ask the visitor if it wants to visit us.
	// It can query our active status to decide.
	if (visitor.visit_rendered_geometry_layer(*this))
	{
		// Visit each RenderedGeometry.
		std::for_each(
			rendered_geometry_begin(),
			rendered_geometry_end(),
			boost::bind(&RenderedGeometry::accept_visitor, _1, boost::ref(visitor)));
	}
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator::RenderedGeometryIterator(
		rendered_geometry_layer_impl_ptr_type layer_impl,
		rendered_geometry_index_type rendered_geom_index) :
	d_layer_impl(layer_impl),
	d_rendered_geom_index(rendered_geom_index)
{
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator::RenderedGeometryIterator(
		const RenderedGeometryIterator &rhs) :
	d_layer_impl(rhs.d_layer_impl),
	d_rendered_geom_index(rhs.d_rendered_geom_index)
{
	// boost::intrusive_ptr copy constructor needs complete type.
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator::~RenderedGeometryIterator()
{
	// boost::intrusive_ptr destructor needs complete type.
}


const GPlatesViewOperations::RenderedGeometry &
GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator::operator*() const
{
	return d_layer_impl->get_rendered_geometry(d_rendered_geom_index);
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator &
GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryIterator::operator++()
{
	++d_rendered_geom_index;
	return *this;
}


bool
GPlatesViewOperations::operator==(
		const RenderedGeometryLayer::RenderedGeometryIterator &lhs,
		const RenderedGeometryLayer::RenderedGeometryIterator &rhs)
{
	return lhs.d_layer_impl.get() == rhs.d_layer_impl.get() &&
			lhs.d_rendered_geom_index == rhs.d_rendered_geom_index;
}
