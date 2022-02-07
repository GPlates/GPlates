/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTY_VALUES_COORDINATETRANSFORMATION_H
#define GPLATES_PROPERTY_VALUES_COORDINATETRANSFORMATION_H

#include <memory>
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

#include "SpatialReferenceSystem.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


// Forward declarations.
class OGRCoordinateTransformation;


namespace GPlatesPropertyValues
{
	/**
	 * Transforms coordinates from one spatial reference system to another.
	 *
	 * This class wraps OGRCoordinateTransformation.
	 */
	class CoordinateTransformation :
			public GPlatesUtils::ReferenceCount<CoordinateTransformation>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<CoordinateTransformation> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const CoordinateTransformation> non_null_ptr_to_const_type;


		/**
		 * A coordinate of (x,y) and optional z (where z is the height above geoid).
		 */
		struct Coord
		{
			Coord(
					const double &x_,
					const double &y_,
					boost::optional<double> z_ = boost::none) :
				x(x_),
				y(y_),
				z(z_)
			{  }

			double x;
			double y;
			boost::optional<double> z;
		};


		/**
		 * Creates a coordinate transformation that does nothing (identity transform).
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new CoordinateTransformation());
		}

		/**
		 * Creates a coordinate transformation from @a source_spatial_reference_system to
		 * @a target_spatial_reference_system.
		 *
		 * @a target_spatial_reference_system defaults to the standard "WGS84" coordinate system.
		 *
		 * Note: Copies are made of both spatial reference systems internally.
		 *
		 * Returns none if there is no supported transformation from the source to target
		 * spatial reference systems.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				const SpatialReferenceSystem::non_null_ptr_to_const_type &source_spatial_reference_system,
				const SpatialReferenceSystem::non_null_ptr_to_const_type &target_spatial_reference_system =
						SpatialReferenceSystem::get_WGS84());


		~CoordinateTransformation();


		/**
		 * Returns the source spatial reference system.
		 *
		 * If default @a create used, then returns SpatialReferenceSystem::get_WGS84().
		 */
		SpatialReferenceSystem::non_null_ptr_to_const_type
		get_source_spatial_reference_system() const
		{
			return d_source_srs;
		}

		/**
		 * Returns the target spatial reference system.
		 *
		 * If default @a create used, then returns SpatialReferenceSystem::get_WGS84().
		 */
		SpatialReferenceSystem::non_null_ptr_to_const_type
		get_target_spatial_reference_system() const
		{
			return d_target_srs;
		}


		/**
		 * Transform an (x,y[,z]) coordinate from the source to the target spatial reference system.
		 *
		 * If using identity transform (created with default @a create method) then x and y
		 * (and optional z) are returned unchanged.
		 *
		 * Returns none if transformation failed (see 'OGRCoordinateTransformation::Transform').
		 */
		boost::optional<Coord>
		transform(
				const Coord &coord) const;

		/**
		 * Same as @a transform but converts coordinates in place.
		 *
		 * Returns false if transformation failed (see 'OGRCoordinateTransformation::Transform').
		 */
		bool
		transform_in_place(
				Coord &coord) const;

		/**
		 * Same as @a transform but converts coordinates in place.
		 *
		 * Returns false if transformation failed (see 'OGRCoordinateTransformation::Transform').
		 */
		bool
		transform_in_place(
				double *x,
				double *y,
				double *z = NULL) const;


		/**
		 * Transform a sequence of (x,y[,z]) coordinates from source to target spatial reference system.
		 *
		 * If using identity transform (created with default @a create method) then the sequence
		 * values will remain unchanged.
		 *
		 * Returns false if transformation failed for any point in the sequence
		 * (see 'OGRCoordinateTransformation::Transform') in which case @a transform_output
		 * will remain unmodified.
		 */
		bool
		transform(
				const std::vector<Coord> &transform_input,
				std::vector<Coord> &transform_output) const;


		/**
		 * Same as @a transform but converts coordinates in place.
		 *
		 * Returns false if transformation failed for any point in the sequence
		 * (see 'OGRCoordinateTransformation::Transform').
		 */
		bool
		transform_in_place(
				std::vector<Coord> &coords) const;


		/**
		 * Same as @a transform but converts coordinates in place.
		 *
		 * @a x and @a y (and optionally z) are expected to be arrays containing @a count elements
		 * each to be transformed in place.
		 *
		 * Returns false if transformation failed for any point in the sequence
		 * (see 'OGRCoordinateTransformation::Transform').
		 */
		bool
		transform_in_place(
				unsigned int count,
				double *x,
				double *y,
				double *z = NULL) const;

	private:

		SpatialReferenceSystem::non_null_ptr_to_const_type d_source_srs;
		SpatialReferenceSystem::non_null_ptr_to_const_type d_target_srs;

		/**
		 * No coordinate transformation (NULL) means identity transform.
		 */
		boost::scoped_ptr<OGRCoordinateTransformation> d_ogr_coordinate_transformation;
		

		CoordinateTransformation();

		CoordinateTransformation(
				const SpatialReferenceSystem::non_null_ptr_to_const_type &source_srs,
				const SpatialReferenceSystem::non_null_ptr_to_const_type &target_srs,
				std::auto_ptr<OGRCoordinateTransformation> ogr_coordinate_transformation);

	};
}

#endif // GPLATES_PROPERTY_VALUES_COORDINATETRANSFORMATION_H
