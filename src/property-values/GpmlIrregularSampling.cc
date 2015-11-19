/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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
#include <boost/foreach.hpp>
#include <iostream>
#include <typeinfo>

#include "GpmlIrregularSampling.h"

namespace
{
	bool
	maybe_null_ptr_eq(
			const GPlatesPropertyValues::GpmlInterpolationFunction::maybe_null_ptr_type &p1,
			const GPlatesPropertyValues::GpmlInterpolationFunction::maybe_null_ptr_type &p2)
	{
		if (p1)
		{
			if (!p2)
			{
				return false;
			}
			return *p1 == *p2;
		}
		else
		{
			return !p2;
		}
	}
}


const GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
GPlatesPropertyValues::GpmlIrregularSampling::deep_clone() const
{
	GpmlIrregularSampling::non_null_ptr_type dup = clone();

	// Now we need to clear the time sample vector in the duplicate, before we push-back the
	// cloned time samples.
	dup->d_time_samples.clear();
	std::vector<GpmlTimeSample>::const_iterator iter, end = d_time_samples.end();
	for (iter = d_time_samples.begin(); iter != end; ++iter) {
		dup->d_time_samples.push_back((*iter).deep_clone());
	}

	if (d_interpolation_function) {
		GpmlInterpolationFunction::non_null_ptr_type cloned_interpolation_function =
				d_interpolation_function->deep_clone_as_interp_func();
		dup->d_interpolation_function =
				GpmlInterpolationFunction::maybe_null_ptr_type(
						cloned_interpolation_function.get());
	}

	return dup;
}


std::ostream &
GPlatesPropertyValues::GpmlIrregularSampling::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GpmlIrregularSampling }";
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GpmlIrregularSampling &other_casted =
			dynamic_cast<const GpmlIrregularSampling &>(other);
		return d_time_samples == other_casted.d_time_samples &&
			maybe_null_ptr_eq(d_interpolation_function, other_casted.d_interpolation_function);
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::is_disabled() const
{
		return contain_disabled_sequence_flag();
}


void
GPlatesPropertyValues::GpmlIrregularSampling::set_disabled(
		bool flag)
{
	if(d_time_samples.empty())
	{
		qWarning() << "No time sample found in this GpmlIrregularSampling.";
		return;
	}
	using namespace GPlatesModel;
	//first, remove all DISABLED_SEQUENCE_FLAG
	BOOST_FOREACH(GpmlTimeSample &sample, d_time_samples)
	{
		GpmlTotalReconstructionPole *trs_pole = 
			dynamic_cast<GpmlTotalReconstructionPole *>(sample.value().get());
		if(trs_pole)
		{
			MetadataContainer 
				&meta_data = trs_pole->metadata(), new_meta_data;
			BOOST_FOREACH(Metadata::shared_ptr_type m, meta_data)
			{
				if(m->get_name() != Metadata::DISABLED_SEQUENCE_FLAG)
				{
					new_meta_data.push_back(m);
				}
			}
			trs_pole->metadata() = new_meta_data;
		}
	}
	//then add new DISABLED_SEQUENCE_FLAG
	GpmlTotalReconstructionPole *first_pole = 
		dynamic_cast<GpmlTotalReconstructionPole *>(d_time_samples[0].value().get());
	if(flag && first_pole)
	{
		first_pole->metadata().insert(
				first_pole->metadata().begin(), 
				Metadata::shared_ptr_type(
						new Metadata(
								Metadata::DISABLED_SEQUENCE_FLAG, 
								"true")));
	}
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::contain_disabled_sequence_flag() const 
{
	using namespace GPlatesModel;
	BOOST_FOREACH(const GpmlTimeSample &sample, d_time_samples)
	{
		const GpmlTotalReconstructionPole *trs_pole = 
			dynamic_cast<const GpmlTotalReconstructionPole *>(sample.value().get());
		if(trs_pole)
		{
			const MetadataContainer &meta_data = trs_pole->metadata();
			BOOST_FOREACH(const Metadata::shared_ptr_type m, meta_data)
			{
				if((m->get_name() == Metadata::DISABLED_SEQUENCE_FLAG) && 
					!m->get_content().compare("true",Qt::CaseInsensitive))
				{
					return true;
				}
			}
		}
	}
	return false;
}
