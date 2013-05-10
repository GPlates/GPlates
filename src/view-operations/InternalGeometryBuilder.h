/* $Id$ */

/**
 * \file 
 * Helper class used to build geometry used to create a new feature.
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

#ifndef GPLATES_VIEWOPERATIONS_INTERNALGEOMETRYBUILDER_H
#define GPLATES_VIEWOPERATIONS_INTERNALGEOMETRYBUILDER_H

#include <vector>
#include <boost/optional.hpp>

#include "maths/GeometryOnSphere.h"
#include "maths/GeometryType.h"
#include "maths/PointOnSphere.h"

namespace GPlatesViewOperations
{
	class GeometryBuilder;

	/**
	 * This is a helper class used only by @a GeometryBuilder to help
	 * with building geometry(s).
	 */
	class InternalGeometryBuilder
	{
	public:
		/**
		* This typedef is used wherever geometry (of some unknown type) is expected.
		* It is a boost::optional because creation of geometry may fail for various reasons.
		*/
		typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;

		//! Sequence of points on sphere.
		typedef std::vector<GPlatesMaths::PointOnSphere> point_seq_type;

		//! Iterator over sequence of const points on sphere.
		typedef point_seq_type::const_iterator point_seq_const_iterator_type;

		/**
		 * Construct empty geometry.
		 *
		 * @param geometry_builder parent builder that we signal when geometry type changes.
		 * @param desired_geom_type the desired type of geometry we are trying to build.
		 */
		InternalGeometryBuilder(
				GeometryBuilder *geometry_builder,
				GPlatesMaths::GeometryType::Value desired_geom_type);

		/**
		 * Sets the type of geometry we'd like to build.
		 */
		void
		set_desired_geometry_type(
				GPlatesMaths::GeometryType::Value geom_type)
		{
			d_desired_geometry_type = geom_type;

			// This might change the actual geometry type so mark as needing update.
			d_update = true;
		}

		/**
		 * Returns actual geometry.
		 *
		 * NOTE: call @a update first to get update-to-date geometry type.
		 *
		 * This may differ from the desired geometry due to insufficient number
		 * of points for example.
		 */
		GPlatesMaths::GeometryType::Value
		get_actual_geometry_type() const
		{
			return d_actual_geometry_type;
		}

		/**
		 * Return read-only reference to internal point sequence.
		 */
		const point_seq_type &
		get_point_seq_const() const
		{
			return d_point_seq;
		}

		/**
		 * Return reference to internal point sequence for modification.
		 *
		 * Internally the state is marked as modified and needing an update.
		 */
		point_seq_type &
		get_point_seq()
		{
			// We're returning a non-const reference to our point sequence so
			// we have to assume the client will modify our internal state.
			// The const-ref version doesn't do this.
			d_update = true;

			return d_point_seq;
		}

		/**
		 * Returns a @a GeometryOnSphere representing the current geometry state.
		 *
		 * NOTE: call @a update first to get update-to-date geometry.
		 *
		 * Might return boost::none if we have no (valid) point data yet.
		 */
		geometry_opt_ptr_type
		get_geometry_on_sphere() const
		{
			return d_geometry_opt_ptr;
		}

		/**
		 * Updates internal state to reflect current point
		 * sequence and actual geometry type.
		 */
		void
		update() const;

	private:
		/**
		 * The type of geometry we are trying to build.
		 * 
		 * The kind of geometry we get might not match the user's intention.
		 * For example, if there are not enough points to make a gml:LineString
		 * but there are enough for a gml:Point.
		 */
		GPlatesMaths::GeometryType::Value d_desired_geometry_type;

		point_seq_type d_point_seq;

		/**
		* What kind of geometry did we successfully build last?
		* 
		* This may be boost::none if we have no (valid) point data yet.
		*
		* If the user were to manage to click a point, then click a point on the
		* exact opposite side of the globe, they should be congratulated with a
		* little music and fireworks show (and the geometry will stubbornly refuse
		* to update, because we can't create a PolylineOnSphere out of it).
		*/
		mutable geometry_opt_ptr_type d_geometry_opt_ptr;

		/**
		* The actual type of geometry as it currently stands.
		*/
		mutable GPlatesMaths::GeometryType::Value d_actual_geometry_type;

		//! Does @a d_geometry_opt_ptr or @a d_actual_geometry_type need updating?
		mutable bool d_update;

		/**
		 * Attempts to create a GeometryOnSphere of the specified type.
		 */
		void
		create_geometry_on_sphere(
				GPlatesMaths::GeometryType::Value) const;
	};
}

#endif // GPLATES_VIEWOPERATIONS_INTERNALGEOMETRYBUILDER_H
