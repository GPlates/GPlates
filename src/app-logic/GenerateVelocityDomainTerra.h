/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#ifndef GENERATE_VELOCITY_DOMAIN_CITCOMS_H
#define GENERATE_VELOCITY_DOMAIN_CITCOMS_H

#include <vector>
#include <boost/noncopyable.hpp>

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/UnitVector3D.h"


namespace GPlatesAppLogic
{
	namespace GenerateVelocityDomainTerra
	{
		/**
		 * Calculates the number of processors given the Terra parameters 'mt', 'nt' and 'nd'.
		 *
		 * @a mt, @a nt and @a nd are Terra parameters (by the same name).
		 *
		 * @throws PreconditionViolationError if:
		 *   @a nd is not 5 or 10,
		 *   @a mt and @a nt are not each a power-of-two,
		 *   @a mt is less than @a nt.
		 */
		unsigned int
		calculate_num_processors(
				unsigned int mt,
				unsigned int nt,
				unsigned int nd);


		/**
		 * An entire Terra grid of point locations (stored in memory) at which to calculate velocity.
		 */
		class Grid :
				private boost::noncopyable
		{
		public:

			/**
			 * Generates the positions at which to calculate velocities for Terra.
			 *
			 * @a mt, @a nt and @a nd are Terra parameters (by the same name).
			 *
			 * @throws PreconditionViolationError if:
			 *   @a nd is not 5 or 10,
			 *   @a mt and @a nt are not each a power-of-two,
			 *   @a mt is less than @a nt.
			 */
			Grid(
					unsigned int mt,
					unsigned int nt,
					unsigned int nd);


			/**
			 * Returns the number of Terra processors (determined by the constructor parameters).
			 */
			unsigned int
			get_num_processors() const
			{
				return d_num_processors;
			}


			/**
			 * Retrieve the sub-domain for the specified Terra local processor number.
			 *
			 * @a processor_number is the local processor number (also defined by Terra).
			 *
			 * @throws PreconditionViolationError if @a process_number is greater or equal to @a get_num_processors.
			 */
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
			get_processor_sub_domain(
					unsigned int processor_number) const;

		private:

			/**
			 * A single icosahedral diamond.
			 */
			class Diamond
			{
			public:

				Diamond() :
					d_mt(0)
				{  }

				void
				allocate(
						unsigned int mt)
				{
					d_mt = mt;
					const unsigned int array_size = (mt + 1) * (mt + 1);
					d_array.resize(array_size, GPlatesMaths::UnitVector3D::zBasis());
				}

				const GPlatesMaths::UnitVector3D &
				get(
						unsigned int column,
						unsigned int row) const
				{
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							column <= d_mt && row <= d_mt,
							GPLATES_ASSERTION_SOURCE);

					return d_array[column + row * (d_mt + 1)];
				}

				GPlatesMaths::UnitVector3D &
				get(
						unsigned int column,
						unsigned int row)
				{
					return const_cast<GPlatesMaths::UnitVector3D &>(
							static_cast<const Diamond *>(this)->get(column, row));
				}

			private:

				unsigned int d_mt;
				std::vector<GPlatesMaths::UnitVector3D> d_array;

			};

			unsigned int d_mt;
			unsigned int d_nt;
			unsigned int d_nd;
			unsigned int d_num_processors;

			/**
			 * The multi-dimensional array containing the entire grid of points.
			 */
			Diamond d_diamond[10/*icosahedral diamonds*/];
		};
	}
}

#endif

