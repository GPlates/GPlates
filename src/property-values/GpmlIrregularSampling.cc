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

#include "GpmlIrregularSampling.h"

#include "GpmlTotalReconstructionPole.h"

#include "model/Metadata.h"
#include "model/NotificationGuard.h"


std::vector<GPlatesPropertyValues::GpmlTimeSample>
GPlatesPropertyValues::GpmlIrregularSampling::get_time_samples() const
{
	// To keep our revision state immutable we clone the time samples so that the client
	// is unable modify them indirectly. If we didn't clone the time samples then the client
	// could copy the returned vector of time samples and then get access to pointers to 'non-const'
	// property values (from a GpmlTimeSample) and then modify our internal immutable revision state.
	// And returning 'const' GpmlTimeSamples doesn't help us here - so cloning is the only solution.
	std::vector<GpmlTimeSample> time_samples;

	const Revision &revision = get_current_revision<Revision>();

	// The copy constructor of GpmlTimeSample does *not* clone its members,
	// instead it has a 'clone()' member function for that...
	BOOST_FOREACH(const GpmlTimeSample &time_sample, revision.time_samples)
	{
		time_samples.push_back(time_sample.clone());
	}

	return time_samples;
}


void
GPlatesPropertyValues::GpmlIrregularSampling::set_time_samples(
		const std::vector<GpmlTimeSample> &time_samples)
{
	MutableRevisionHandler revision_handler(this);
	// To keep our revision state immutable we clone the time samples so that the client
	// can no longer modify them indirectly...
	revision_handler.get_mutable_revision<Revision>().set_cloned_time_samples(time_samples);
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GpmlIrregularSampling::set_interpolation_function(
		GpmlInterpolationFunction::maybe_null_ptr_to_const_type interpolation_function)
{
	MutableRevisionHandler revision_handler(this);
	// To keep our revision state immutable we clone the interpolation function so that the client
	// can no longer modify it indirectly...
	revision_handler.get_mutable_revision<Revision>().set_cloned_interpolation_function(interpolation_function);
	revision_handler.handle_revision_modification();
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::is_disabled() const
{
	return contains_disabled_sequence_flag();
}


void
GPlatesPropertyValues::GpmlIrregularSampling::set_disabled(
		bool flag)
{
	if (get_current_revision<Revision>().time_samples.empty())
	{
		qWarning() << "No time sample found in this GpmlIrregularSampling.";
		return;
	}

	// Merge model events across this scope to avoid excessive number of model callbacks
	// when modifying the total reconstruction pole property values.
	GPlatesModel::NotificationGuard model_notification_guard(get_model());

	MutableRevisionHandler revision_handler(this);
	Revision &mutable_revision = revision_handler.get_mutable_revision<Revision>();

	using namespace GPlatesModel;
	//first, remove all DISABLED_SEQUENCE_FLAG
	BOOST_FOREACH(GpmlTimeSample &sample, mutable_revision.time_samples)
	{
		GpmlTotalReconstructionPole *trs_pole = 
			dynamic_cast<GpmlTotalReconstructionPole *>(sample.get_value().get());
		if(trs_pole)
		{
			const MetadataContainer &meta_data = trs_pole->get_metadata();
			MetadataContainer new_meta_data;
			BOOST_FOREACH(Metadata::shared_ptr_type m, meta_data)
			{
				if(m->get_name() != Metadata::DISABLED_SEQUENCE_FLAG)
				{
					new_meta_data.push_back(m);
				}
			}
			trs_pole->set_metadata(new_meta_data);
		}
	}

	//then add new DISABLED_SEQUENCE_FLAG
	GpmlTotalReconstructionPole *first_pole = 
		dynamic_cast<GpmlTotalReconstructionPole *>(mutable_revision.time_samples[0].get_value().get());
	if(flag && first_pole)
	{
		MetadataContainer first_pole_meta_data = first_pole->get_metadata();

		first_pole_meta_data.insert(
				first_pole_meta_data.begin(), 
				Metadata::shared_ptr_type(
						new Metadata(
								Metadata::DISABLED_SEQUENCE_FLAG, 
								"true")));

		first_pole->set_metadata(first_pole_meta_data);
	}

	revision_handler.handle_revision_modification();
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::contains_disabled_sequence_flag() const 
{
	using namespace GPlatesModel;

	const std::vector<GpmlTimeSample> &time_samples = get_current_revision<Revision>().time_samples;

	BOOST_FOREACH(const GpmlTimeSample &sample, time_samples)
	{
		const GpmlTotalReconstructionPole *trs_pole = 
			dynamic_cast<const GpmlTotalReconstructionPole *>(sample.get_value().get());
		if(trs_pole)
		{
			const MetadataContainer &meta_data = trs_pole->get_metadata();
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


std::ostream &
GPlatesPropertyValues::GpmlIrregularSampling::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GpmlIrregularSampling }";
}


GPlatesPropertyValues::GpmlIrregularSampling::Revision::Revision(
		const Revision &other)
{
	// The copy constructor of GpmlTimeSample does *not* clone its members,
	// instead it has a 'clone()' member function for that...
	BOOST_FOREACH(const GpmlTimeSample &other_time_sample, other.time_samples)
	{
		time_samples.push_back(other_time_sample.clone());
	}

	if (other.interpolation_function)
	{
		interpolation_function = other.interpolation_function->clone().get();
	}
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return time_samples == other_revision.time_samples &&
			boost::equal_pointees(interpolation_function, other_revision.interpolation_function) &&
			GPlatesModel::PropertyValue::Revision::equality(other);
}
