/* $Id: GenericMapper.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file validate filename template.
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
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

#ifndef GPLATES_UTILS_SPATIALREFERENCESYSTEM_H
#define GPLATES_UTILS_SPATIALREFERENCESYSTEM_H

#include <vector>
#include <QString>

namespace GPlatesUtils
{
	/*
	* This file contains utilities of handling Spatial Reference System(srs).
	*/

	namespace SpatialReferenceSystem
	{
		/*
		* Class to represent dimension.
		* Define class instead of enumeration, because of type-safe.
		*/
		class Dimension
		{
		public:
			static 
			inline
			Dimension
			threeD()
			{
				return Dimension(3);
			}

			static 
			inline
			Dimension
			twoD()
			{
				return Dimension(2);
			}

			bool
			operator==(
					const Dimension& right) const
			{
				return d_value == right.d_value;
			}

		private:
			explicit
			Dimension(
					unsigned char value)
				:d_value(value)
			{ }

			unsigned char d_value;
		};

		/*
		* Class represent Coordinate Reference System. 
		*  
		*/
		class CoordinateReferenceSystem
		{
		public:
			static
			inline
			CoordinateReferenceSystem
			espg_4326()
			{
				return CoordinateReferenceSystem(ESPG_4326, Dimension::twoD());
			}

			static
			inline
			CoordinateReferenceSystem
			create_by_name(
					const QString& name,
					Dimension d = Dimension::twoD())
			{
				return CoordinateReferenceSystem(ESPG_4326, d);
			}

			bool
			inline
			is_3D() const
			{
				return d_dimension == Dimension::threeD();
			}

			const QString
			inline
			name() const
			{
				return d_name;
			}

		private:
			enum CRS
			{
				ESPG_4326,
				Invalid
			};
			
			CoordinateReferenceSystem();
			CoordinateReferenceSystem(
					CRS crs,
					Dimension dimension):
				d_crs(crs),
				d_dimension(dimension)
			{ }
			
			CRS d_crs;
			Dimension d_dimension;
			QString d_name;
			QString d_desc;
		};


		/*
		* Coordinates class.
		*/
		class Coordinates
		{
		public:
			Coordinates(
					std::vector<double>& coordinates,
					CoordinateReferenceSystem crs):
				d_crs(crs),
				d_coor_array(coordinates)
			{
				//TODO:
				//validate coordinates, throw exception.
			}

			inline
			double
			x() const
			{
				return d_coor_array[0];
			}

			inline
			double
			y() const
			{
				return d_coor_array[1];
			}

			inline
			double
			z() const
			{
				if(d_crs.is_3D())
				{
					return d_coor_array[2];
				}
				else
				{
					//TODO:
					//throw exception
					return 0xBAADBEEF;
				}
			}

		private:
			Coordinates();
			CoordinateReferenceSystem d_crs;
			std::vector<double> d_coor_array;
		};

		/*
		* Transform points from one system to the other.
		* Result will replaced input points.
		*/
		void
		transform(
				const CoordinateReferenceSystem& from,
				const CoordinateReferenceSystem& to,
				std::vector<Coordinates>& points);
	}
}
#endif


