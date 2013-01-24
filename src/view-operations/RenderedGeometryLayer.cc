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

#include <vector>
#include <boost/bind.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>

#include "RenderedGeometryLayer.h"

#include "RenderedDirectionArrow.h"
#include "RenderedGeometryLayerVisitor.h"
#include "RenderedGeometryVisitor.h"
#include "RenderedPointOnSphere.h"
#include "RenderedSmallCircle.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "utils/LatLonAreaSampling.h"
#include "utils/ReferenceCount.h"

//#include "utils/Profile.h"


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

		//! Convenience typedef for rendered geometries spatial partition.
		typedef RenderedGeometryLayer::rendered_geometries_spatial_partition_type
				rendered_geometries_spatial_partition_type;

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
		boost::optional<const rendered_geometries_spatial_partition_type &>
		get_rendered_geometries() const = 0;

		virtual
		void
		add_rendered_geometry(
				RenderedGeometry) = 0;

		virtual
		void
		add_rendered_geometries(
				const rendered_geometries_spatial_partition_type::non_null_ptr_type &rendered_geometries_spatial_partition) = 0;

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
				d_is_rendered_geom_seq_valid(true),
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
				if (d_is_rendered_geom_seq_valid)
				{
					// All rendered geoms are in the sequence.
					return d_rendered_geom_seq.empty();
				}

				// All rendered geoms are in the spatial partition.
				return d_spatial_partition.get()->empty();
			}


			virtual
			unsigned int
			get_num_rendered_geometries() const
			{
				if (d_is_rendered_geom_seq_valid)
				{
					// All rendered geoms are in the sequence.
					return d_rendered_geom_seq.size();
				}

				// All rendered geoms are in the spatial partition.
				return d_spatial_partition.get()->size();
			}

			virtual
			const RenderedGeometry &
			get_rendered_geometry(
					rendered_geometry_index_type rendered_geom_index) const
			{
				if (d_is_rendered_geom_seq_valid)
				{
					// All rendered geoms are in the sequence.
					return d_rendered_geom_seq[rendered_geom_index];
				}

				// Copy from spatial partition to our internal sequence.
				copy_spatial_partition_to_rendered_geometries_sequence();

				// All rendered geoms are now in the sequence.
				return d_rendered_geom_seq[rendered_geom_index];
			}


			virtual
			boost::optional<const rendered_geometries_spatial_partition_type &>
			get_rendered_geometries() const
			{
				if (!d_spatial_partition)
				{
					return boost::none;
				}

				return *d_spatial_partition.get();
			}


			virtual
			void
			add_rendered_geometry(
					RenderedGeometry rendered_geom)
			{
				if (d_spatial_partition)
				{
					// We don't have any spatial extents for the rendered geometry so just add it
					// to the root of the entire spatial partition.
					d_spatial_partition.get()->add_unpartitioned(rendered_geom);

					// Now that we're adding to the spatial partition we're invalidating
					// the render geometries sequence.
					d_is_rendered_geom_seq_valid = false;

					return;
				}

				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_is_rendered_geom_seq_valid,
						GPLATES_ASSERTION_SOURCE);

				d_rendered_geom_seq.push_back(rendered_geom);
			}


			virtual
			void
			add_rendered_geometries(
					const rendered_geometries_spatial_partition_type::non_null_ptr_type &spatial_partition)
			{
				// If first time we've added a spatial partition then add all the
				// rendered geometries in the sequence to the new spatial partition.
				if (!d_spatial_partition)
				{
					// Iterate over the rendered geometries in our sequence.
					rendered_geom_seq_type::const_iterator rendered_geoms_iter =
							d_rendered_geom_seq.begin();
					const rendered_geom_seq_type::const_iterator rendered_geoms_end =
							d_rendered_geom_seq.end();
					for ( ; rendered_geoms_iter != rendered_geoms_end; ++rendered_geoms_iter)
					{
						const RenderedGeometry &rendered_geometry = *rendered_geoms_iter;

						// Add to the root of the spatial partition (since have no spatial extent info).
						spatial_partition->add_unpartitioned(rendered_geometry);
					}

					// Set our partition to the new one - we are now the owner.
					d_spatial_partition = spatial_partition;

					// Delay iterating over the spatial partition and adding its rendered geometries
					// to our std::vector - clients may never need it in which case it would've
					// been wasted effort. This is fairly common when a layer is rendered by visiting
					// its spatial tree instead of retrieving the rendered geometries sequentially.
					d_is_rendered_geom_seq_valid = false;
				}
				else // Not the first time we've added a spatial partition since the last clear...
				{
					// Merge the new spatial partition into the current one.
					GPlatesMaths::CubeQuadTreePartitionUtils::merge(
							*d_spatial_partition.get(),
							*spatial_partition);

					d_is_rendered_geom_seq_valid = false;
				}
			}


			virtual
			void
			clear_rendered_geometries()
			{
				d_rendered_geom_seq.clear();
				d_spatial_partition = boost::none;
				d_is_rendered_geom_seq_valid = true;
			}

		private:
			//! Typedef for sequence of @a RenderedGeometry objects.
			typedef std::vector<RenderedGeometry> rendered_geom_seq_type;

			mutable rendered_geom_seq_type d_rendered_geom_seq;
			boost::optional<rendered_geometries_spatial_partition_type::non_null_ptr_type> d_spatial_partition;
			mutable bool d_is_rendered_geom_seq_valid;
			double d_current_viewport_zoom_factor;


			void
			copy_spatial_partition_to_rendered_geometries_sequence() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						!d_is_rendered_geom_seq_valid && d_spatial_partition,
						GPLATES_ASSERTION_SOURCE);

				// Iterate over the cube quad tree and add the rendered geometries to our
				// internal std::vector sequence.
				d_rendered_geom_seq.clear();
				d_rendered_geom_seq.reserve(d_spatial_partition.get()->size());
				rendered_geometries_spatial_partition_type::const_iterator rendered_geoms_iter =
						d_spatial_partition.get()->get_iterator();
				for ( ; !rendered_geoms_iter.finished(); rendered_geoms_iter.next())
				{
					d_rendered_geom_seq.push_back(rendered_geoms_iter.get_element());
				}

				d_is_rendered_geom_seq_valid = true;
			}
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
			visit_rendered_direction_arrow(
					const RenderedDirectionArrow &rendered_direction_arrow)
			{
				d_position_on_sphere = rendered_direction_arrow.get_start_position();
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
				return d_zoom_independent_seq.empty() &&
						(d_zoom_dependent_seq.get_num_sampled_elements() == 0);
			}


			virtual
			unsigned int
			get_num_rendered_geometries() const
			{
				return d_zoom_independent_seq.size() +
						d_zoom_dependent_seq.get_num_sampled_elements();
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
					RenderedGeometry rendered_geom)
			{
				boost::optional<GPlatesMaths::PointOnSphere> zoom_dependent =
						is_zoom_dependent(rendered_geom);

				// Add to the appropriate sequence (zoom-dependent or zoom-independent).
				if (zoom_dependent)
				{
					const GPlatesMaths::PointOnSphere &point_on_sphere_location = *zoom_dependent;

					// Add to the lat/lon area sampling.
					d_zoom_dependent_seq.add_element(
							rendered_geom, point_on_sphere_location);
				}
				else
				{
					d_zoom_independent_seq.push_back(rendered_geom);
				}
			}


			virtual
			void
			clear_rendered_geometries()
			{
				d_zoom_independent_seq.clear();
				d_zoom_dependent_seq.clear_elements();
			}


			virtual
			boost::optional<const rendered_geometries_spatial_partition_type &>
			get_rendered_geometries() const
			{
				return boost::none;
			}


			virtual
			void
			add_rendered_geometries(
					const rendered_geometries_spatial_partition_type::non_null_ptr_type &rendered_geometries_spatial_partition)
			{
				// We can't make use of the spatial partition since we're discarding some
				// rendered geometries (only keeping one per zoom-dependent sampling bin).
				// So we iterate over the rendered geometries in the spatial partition and
				// add using our 'add_rendered_geometry' interface.

				rendered_geometries_spatial_partition_type::const_iterator rendered_geoms_iter =
						rendered_geometries_spatial_partition->get_iterator();
				for ( ; !rendered_geoms_iter.finished(); rendered_geoms_iter.next())
				{
					add_rendered_geometry(rendered_geoms_iter.get_element());
				}
			}


		private:
			//! Typedef for sequence of zoom-independent @a RenderedGeometry objects.
			typedef std::vector<RenderedGeometry> zoom_independent_seq_type;

			//! Typedef for sequence of zoom-dependent @a RenderedGeometry objects.
			typedef GPlatesUtils::LatLonAreaSampling<RenderedGeometry> zoom_dependent_seq_type;


			float d_current_ratio_zoom_dependent_bin_dimension_to_globe_radius;
			GPlatesMaths::real_t d_current_viewport_zoom_factor;

			zoom_independent_seq_type d_zoom_independent_seq;
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
				const double sample_spacing_degrees =
						ratio_zoom_dependent_bin_dimension_to_globe_radius /
						viewport_zoom_factor *
						(180.0 / GPlatesMaths::PI);

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

			// Iterate over the old impl's rendered geometries and add them to the new impl.
			const unsigned int num_rendered_geoms = d_impl->get_num_rendered_geometries();
			for (rendered_geometry_index_type rendered_geom_index = 0;
				rendered_geom_index < num_rendered_geoms;
				++rendered_geom_index)
			{
				const RenderedGeometry &rendered_geom = d_impl->get_rendered_geometry(rendered_geom_index);
				new_impl->add_rendered_geometry(rendered_geom);
			}

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

			// Iterate over the old impl's rendered geometries and add them to the new impl.
			// NOTE: This can remove the benefits of the spatial partition inside the
			// zoom *independent* spatial partition but not much can be done about that.
			const unsigned int num_rendered_geoms = d_impl->get_num_rendered_geometries();
			for (rendered_geometry_index_type rendered_geom_index = 0;
				rendered_geom_index < num_rendered_geoms;
				++rendered_geom_index)
			{
				const RenderedGeometry &rendered_geom = d_impl->get_rendered_geometry(rendered_geom_index);
				new_impl->add_rendered_geometry(rendered_geom);
			}

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
		RenderedGeometry rendered_geom)
{
	d_impl->add_rendered_geometry(rendered_geom);

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


boost::optional<const GPlatesViewOperations::RenderedGeometryLayer::rendered_geometries_spatial_partition_type &>
GPlatesViewOperations::RenderedGeometryLayer::get_rendered_geometries() const
{
	return d_impl->get_rendered_geometries();
}


void
GPlatesViewOperations::RenderedGeometryLayer::add_rendered_geometries(
		const rendered_geometries_spatial_partition_type::non_null_ptr_type &rendered_geometries_spatial_partition)
{
	d_impl->add_rendered_geometries(rendered_geometries_spatial_partition);
}


void
GPlatesViewOperations::RenderedGeometryLayer::accept_visitor(
		ConstRenderedGeometryLayerVisitor &visitor) const
{
	// Visit each RenderedGeometry.
	std::for_each(
		rendered_geometry_begin(),
		rendered_geometry_end(),
		boost::bind(&RenderedGeometry::accept_visitor, _1, boost::ref(visitor)));
}


void
GPlatesViewOperations::RenderedGeometryLayer::accept_visitor(
		RenderedGeometryLayerVisitor &visitor)
{
	// Visit each RenderedGeometry.
	std::for_each(
		rendered_geometry_begin(),
		rendered_geometry_end(),
		boost::bind(&RenderedGeometry::accept_visitor, _1, boost::ref(visitor)));
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
