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

#include "maths/LatLonPoint.h"
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

		virtual
		~RenderedGeometryLayerImpl()
		{  }

		virtual
		void
		set_viewport_zoom_factor(
				const double &viewport_zoom_factor) = 0;

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
		void
		add_rendered_geometry(
				RenderedGeometry) = 0;

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
			virtual
			void
			set_viewport_zoom_factor(
					const double &/*viewport_zoom_factor*/)
			{
				// Do nothing - we're not interested in zoom.
			}

			virtual
			bool
			is_empty() const
			{
				return d_rendered_geom_seq.empty();
			}


			virtual
			unsigned int
			get_num_rendered_geometries() const
			{
				return d_rendered_geom_seq.size();
			}

			virtual
			const RenderedGeometry &
			get_rendered_geometry(
					rendered_geometry_index_type rendered_geom_index) const
			{
#if 0
 				GPlatesGlobal::Assert(rendered_geom_index < d_rendered_geom_seq.size(),
 						GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));
#endif

				return d_rendered_geom_seq[rendered_geom_index];
			}


			virtual
			void
			add_rendered_geometry(
					RenderedGeometry rendered_geom)
			{
				d_rendered_geom_seq.push_back(rendered_geom);
			}


			virtual
			void
			clear_rendered_geometries()
			{
				d_rendered_geom_seq.clear();
			}

		private:
			//! Typedef for sequence of @a RenderedGeometry objects.
			typedef std::vector<RenderedGeometry> rendered_geom_seq_type;

			rendered_geom_seq_type d_rendered_geom_seq;
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
				d_ratio_zoom_dependent_bin_dimension_to_globe_radius(
						ratio_zoom_dependent_bin_dimension_to_globe_radius),
				d_current_viewport_zoom_factor(viewport_zoom_factor),
				d_zoom_dependent_seq(
						get_zoom_dependent_sample_spacing(
							ratio_zoom_dependent_bin_dimension_to_globe_radius,
							viewport_zoom_factor))
			{  }


			void
			set_viewport_zoom_factor(
					const double &viewport_zoom_factor)
			{
				// If zoom factor hasn't changed then nothing to do.
				// We dont' need exact bit-for-bit comparisons here so use GPlatesMaths::real_t.
				if (viewport_zoom_factor == d_current_viewport_zoom_factor)
				{
					return;
				}

				d_current_viewport_zoom_factor = viewport_zoom_factor;

				// Modify the lat/lon area sampling with a new spacing
				// that reflects the new zoom factor.
				const double sample_bin_angle_spacing_degrees =
						get_zoom_dependent_sample_spacing(
								d_ratio_zoom_dependent_bin_dimension_to_globe_radius,
								d_current_viewport_zoom_factor.dval());
				d_zoom_dependent_seq.reset_sample_spacing(sample_bin_angle_spacing_degrees);
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


		private:
			//! Typedef for sequence of zoom-independent @a RenderedGeometry objects.
			typedef std::vector<RenderedGeometry> zoom_independent_seq_type;

			//! Typedef for sequence of zoom-dependent @a RenderedGeometry objects.
			typedef GPlatesUtils::LatLonAreaSampling<RenderedGeometry> zoom_dependent_seq_type;


			const float d_ratio_zoom_dependent_bin_dimension_to_globe_radius;
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
		};
	}
}


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryLayer(
		user_data_type user_data) :
d_user_data(user_data),
d_impl(new ZoomIndependentLayerImpl()),
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

		emit layer_was_updated(*this, d_user_data);
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

	emit layer_was_updated(*this, d_user_data);
}


void
GPlatesViewOperations::RenderedGeometryLayer::clear_rendered_geometries()
{
	if (!d_impl->is_empty())
	{
		d_impl->clear_rendered_geometries();

		emit layer_was_updated(*this, d_user_data);
	}
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
