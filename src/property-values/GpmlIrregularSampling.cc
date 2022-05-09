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

#include <iostream>
#include <boost/foreach.hpp>

#include "GpmlIrregularSampling.h"

#include "GpmlFiniteRotation.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/Metadata.h"
#include "model/NotificationGuard.h"


namespace
{
	template<class T>
	bool
	opt_eq(
			const boost::optional<GPlatesModel::RevisionedReference<T> > &opt1,
			const boost::optional<GPlatesModel::RevisionedReference<T> > &opt2)
	{
		if (opt1)
		{
			if (!opt2)
			{
				return false;
			}
			return *opt1.get().get_revisionable() == *opt2.get().get_revisionable();
		}
		else
		{
			return !opt2;
		}
	}
}


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlIrregularSampling::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("IrregularSampling");


const boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlIrregularSampling::interpolation_function() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.interpolation_function)
	{
		return boost::none;
	}

	return GPlatesUtils::static_pointer_cast<const GpmlInterpolationFunction>(
			revision.interpolation_function->get_revisionable());
}


const boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
GPlatesPropertyValues::GpmlIrregularSampling::interpolation_function()
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.interpolation_function)
	{
		return boost::none;
	}

	return revision.interpolation_function->get_revisionable();
}


void
GPlatesPropertyValues::GpmlIrregularSampling::set_interpolation_function(
		boost::optional<GpmlInterpolationFunction::non_null_ptr_type> interpolation_function_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	if (revision.interpolation_function)
	{
		if (interpolation_function_)
		{
			revision.interpolation_function->change(revision_handler.get_model_transaction(), interpolation_function_.get());
		}
		else
		{
			revision.interpolation_function->detach(revision_handler.get_model_transaction());
			revision.interpolation_function = boost::none;
		}
	}
	else if (interpolation_function_)
	{
		revision.interpolation_function = GPlatesModel::RevisionedReference<GpmlInterpolationFunction>::attach(
				revision_handler.get_model_transaction(), *this, interpolation_function_.get());
	}
	// ...else nothing to do.

	revision_handler.commit();
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
	using namespace GPlatesModel;

	RevisionedVector<GpmlTimeSample> &samples = time_samples();

	if (samples.empty())
	{
		qWarning() << "No time sample found in this GpmlIrregularSampling.";
		return;
	}

	// Merge model events across this scope to avoid excessive number of model callbacks
	// when modifying the total reconstruction pole property values.
	NotificationGuard model_notification_guard(get_model());

	//first, remove all DISABLED_SEQUENCE_FLAG
	BOOST_FOREACH(GpmlTimeSample::non_null_ptr_type sample, samples)
	{
		GpmlFiniteRotation *trs_pole = dynamic_cast<GpmlFiniteRotation *>(sample->value().get());
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
	GpmlFiniteRotation *first_pole = 
			dynamic_cast<GpmlFiniteRotation *>(samples[0]->value().get());
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
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::contains_disabled_sequence_flag() const 
{
	using namespace GPlatesModel;

	const RevisionedVector<GpmlTimeSample> &samples = time_samples();

	BOOST_FOREACH(GpmlTimeSample::non_null_ptr_to_const_type sample, samples)
	{
		const GpmlFiniteRotation *trs_pole = 
			dynamic_cast<const GpmlFiniteRotation *>(sample->value().get());
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
	const GPlatesModel::RevisionedVector<GpmlTimeSample> &samples = time_samples();

	os << "[ ";

	GPlatesModel::RevisionedVector<GpmlTimeSample>::const_iterator samples_iter =
			samples.begin();
	GPlatesModel::RevisionedVector<GpmlTimeSample>::const_iterator samples_end =
			samples.end();
	for ( ; samples_iter != samples_end; ++samples_iter)
	{
		os << **samples_iter;
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlIrregularSampling::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.time_samples.get_revisionable())
	{
		return revision.time_samples.clone_revision(transaction);
	}
	if (revision.interpolation_function &&
		child_revisionable == revision.interpolation_function->get_revisionable())
	{
		return revision.interpolation_function->clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


bool
GPlatesPropertyValues::GpmlIrregularSampling::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return *time_samples.get_revisionable() == *other_revision.time_samples.get_revisionable() &&
			opt_eq(interpolation_function, other_revision.interpolation_function) &&
			PropertyValue::Revision::equality(other);
}
