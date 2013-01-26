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

#include <cmath>
#include <QProgressBar>

#include "GenerateVelocityDomainTerra.h"

#include "maths/MathsUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


namespace
{
	/**
	 * Subdivides the two specified vectors and returns the midpoint on the sphere.
	 */
	GPlatesMaths::UnitVector3D
	midpoint(
			const GPlatesMaths::UnitVector3D &v1,
			const GPlatesMaths::UnitVector3D &v2)
	{
		return (GPlatesMaths::Vector3D(v1) + GPlatesMaths::Vector3D(v2)).get_normalisation();
	}
}


unsigned int
GPlatesAppLogic::GenerateVelocityDomainTerra::calculate_num_processors(
		unsigned int mt,
		unsigned int nt,
		unsigned int nd)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			mt >= nt &&
				mt > 0 && GPlatesUtils::Base2::is_power_of_two(mt) &&
				nt > 0 && GPlatesUtils::Base2::is_power_of_two(nt) &&
				(nd == 5 || nd == 10),
			GPLATES_ASSERTION_SOURCE);

	const unsigned int ldiv = mt / nt;
	const unsigned int sp = ldiv * ldiv;

	return 10 * sp / nd;
}


GPlatesAppLogic::GenerateVelocityDomainTerra::Grid::Grid(
		unsigned int mt,
		unsigned int nt,
		unsigned int nd) :
	d_mt(mt),
	d_nt(nt),
	d_nd(nd),
	d_num_processors(calculate_num_processors(mt, nt, nd))
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_mt >= d_nt &&
				d_mt > 0 && GPlatesUtils::Base2::is_power_of_two(d_mt) &&
				d_nt > 0 && GPlatesUtils::Base2::is_power_of_two(d_nt) &&
				(d_nd == 5 || d_nd == 10),
			GPLATES_ASSERTION_SOURCE);

	// Allocate storage for each diamond.
	for (unsigned int n = 0; n < 10; ++n)
	{
		d_diamond[n].allocate(d_mt);
	}

	//
	// Most of this code was ported over directly from the Terra fortran code by Leonardo Quevedo.
	//

	const double fifthpi = 0.2 * GPlatesMaths::PI;
	const double w = 2.0 * std::acos(1.0 / (2.0 * std::sin(fifthpi)));
	const double cosw = std::cos(w);
	const double sinw = std::sin(w);

	const unsigned int lvt = GPlatesUtils::Base2::log2_power_of_two(d_mt);

	for (unsigned int id = 0; id < 10; ++id)
	{
		double sgn = (id > 4) ? -1.0 : 1.0;
		double phi;
#if 1 // This is similar to the code that Terra uses...
		phi = ( 2 * std::fmod(id + 1.0, 5.0) - 3 + id / 5 ) * fifthpi;
#else
		if (id < 5)
		{
			phi = 2.0 * (double(id) - 0.5) * fifthpi;
		}
		else
		{
			phi = 2.0 * (double(id) - 5.0) * fifthpi;
		}
#endif

		d_diamond[id].get(0, 0) = GPlatesMaths::UnitVector3D(
				0.0,
				0.0,
				sgn);
		d_diamond[id].get(d_mt, 0) = GPlatesMaths::UnitVector3D(
				sinw * std::cos(phi),
				sinw * std::sin(phi),
				cosw * sgn);
		d_diamond[id].get(0, d_mt) = GPlatesMaths::UnitVector3D(
				sinw * std::cos(phi + fifthpi + fifthpi),
				sinw * std::sin(phi + fifthpi + fifthpi),
				cosw * sgn);
		d_diamond[id].get(d_mt, d_mt) = GPlatesMaths::UnitVector3D(
				sinw * std::cos(phi + fifthpi),
				sinw * std::sin(phi + fifthpi),
				-cosw * sgn);

		for (unsigned int level = 0; level < lvt; ++level)
		{
			unsigned int m = 1 << level;
			unsigned int l = d_mt / m;
			unsigned int l2 = l / 2;

			for (unsigned int j1 = 1; j1 < m + 2; ++j1)
			{
				for (unsigned int j2 = 1; j2 < m + 1; ++j2)
				{
					// Rows of diamond
					unsigned int i1 = (j1 - 1) * l;
					unsigned int i2 = (j2 - 1) * l + l2;
					// Find Midpoint on small circle
					d_diamond[id].get(i1, i2) =
							midpoint(d_diamond[id].get(i1, i2 - l2), d_diamond[id].get(i1, i2 + l2));
				}
			}

			for (unsigned int j1 = 1; j1 < m + 2; ++j1)
			{
				for (unsigned int j2 = 1; j2 < m + 1; ++j2)
				{
					// Columns of diamond
					unsigned int i1 = (j2 - 1) * l + l2;
					unsigned int i2 = (j1 - 1) * l;
					// Find Midpoint on small circle
					d_diamond[id].get(i1, i2) =
							midpoint(d_diamond[id].get(i1 - l2, i2), d_diamond[id].get(i1 + l2, i2));
				}
			}

			for (unsigned int j1 = 1; j1 < m + 1; ++j1)
			{
				for (unsigned int j2 = 1; j2 < m + 1; ++j2)
				{
					// Diagonals of diamond
					unsigned int i1 = (j1 - 1) * l + l2;
					unsigned int i2 = (j2 - 1) * l + l2;
					// Find Midpoint on small circle
					d_diamond[id].get(i1, i2) =
							midpoint(d_diamond[id].get(i1 - l2, i2 + l2), d_diamond[id].get(i1 + l2, i2 - l2));
				}
			}
		}
	}
}


GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GenerateVelocityDomainTerra::Grid::get_processor_sub_domain(
		unsigned int processor_number) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			processor_number < d_num_processors,
			GPLATES_ASSERTION_SOURCE);

	std::vector<GPlatesMaths::PointOnSphere> sub_domain;

	//
	// Most of this code was ported over directly from the Terra fortran code by Leonardo Quevedo.
	//

	unsigned int ldiv = d_mt / d_nt;
	unsigned int i1beg = (processor_number % ldiv) * d_nt;
	unsigned int i2beg;
	unsigned int idbeg;
	unsigned int idend;

	if (d_nd == 5) // Split in two sets of diamonds
	{
		if (processor_number < (d_num_processors / 2))
		{
			idbeg = 0;
			idend = 5;
		}
		else
		{
			idbeg = 5;
			idend = 10;
		}
		i2beg = ((processor_number % (d_num_processors / 2)) / ldiv) * d_nt;
	}
	else
	{
		idbeg = 0;
		idend = 10;
		i2beg = (processor_number / ldiv) * d_nt;
	}

	unsigned int i1end = i1beg + d_nt + 1;
	unsigned int i2end = i2beg + d_nt + 1;

	for (unsigned int id = idbeg; id < idend; ++id)
	{
		for (unsigned int i2 = i2beg; i2 < i2end; ++i2)
		{
			for (unsigned int i1 = i1beg; i1 < i1end; ++i1)
			{
				sub_domain.push_back(GPlatesMaths::PointOnSphere(d_diamond[id].get(i1, i2)));
			}
		}
	}

	return GPlatesMaths::MultiPointOnSphere::create_on_heap(sub_domain);
}
