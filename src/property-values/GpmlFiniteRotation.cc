/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2010 The University of Sydney, Australia
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
#include <boost/none.hpp>

#include "GmlPoint.h"
#include "GpmlFiniteRotation.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"

#include "model/BubbleUpRevisionHandler.h"


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create(
		const std::pair<double, double> &gpml_euler_pole,
		const double &gml_angle_in_degrees,
		boost::optional<const GPlatesModel::MetadataContainer &> metadata_)
{
	const double &lon = gpml_euler_pole.first;
	const double &lat = gpml_euler_pole.second;

	// FIXME:  Check the validity of the lat/lon coords using functions in LatLonPoint.
	GPlatesMaths::LatLonPoint llp(lat, lon);
	GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
	GPlatesMaths::FiniteRotation fr =
			GPlatesMaths::FiniteRotation::create(
					p,
					GPlatesMaths::convert_deg_to_rad(gml_angle_in_degrees));

	return create(fr, metadata_);
}


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create(
		const GmlPoint::non_null_ptr_to_const_type &gpml_euler_pole,
		const GpmlMeasure::non_null_ptr_to_const_type &gml_angle_in_degrees,
		boost::optional<const GPlatesModel::MetadataContainer &> metadata_)
{
	GPlatesMaths::FiniteRotation fr =
			GPlatesMaths::FiniteRotation::create(
					*gpml_euler_pole->get_point(),
					GPlatesMaths::convert_deg_to_rad(gml_angle_in_degrees->get_quantity()));

	return create(fr, metadata_);
}


const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
GPlatesPropertyValues::GpmlFiniteRotation::create_zero_rotation(
		boost::optional<const GPlatesModel::MetadataContainer &> metadata_)
{
	using namespace ::GPlatesMaths;

	FiniteRotation fr = FiniteRotation::create_identity_rotation();

	return create(fr, metadata_);
}


bool
GPlatesPropertyValues::GpmlFiniteRotation::is_zero_rotation() const
{
	return ::GPlatesMaths::represents_identity_rotation(get_finite_rotation().unit_quat());
}


void
GPlatesPropertyValues::GpmlFiniteRotation::set_finite_rotation(
		const GPlatesMaths::FiniteRotation &fr)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().finite_rotation = fr;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlFiniteRotation::set_metadata(
		const GPlatesModel::MetadataContainer &metadata)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().metadata = metadata;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlFiniteRotation::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	os << revision.finite_rotation;

	os << ", [ ";

	BOOST_FOREACH(const GPlatesModel::MetadataContainer::value_type &metadata_entry, revision.metadata)
	{
		os << '(' << metadata_entry->get_name().toStdString() << ": " << metadata_entry->get_content().toStdString() << "), ";
	}

	return os << " ]";
}


GPlatesPropertyValues::GpmlFiniteRotation::Revision::Revision(
		const GPlatesMaths::FiniteRotation &finite_rotation_,
		boost::optional<const GPlatesModel::MetadataContainer &> metadata_) :
	finite_rotation(finite_rotation_)
{
	if (metadata_)
	{
		// Clone each metadata entry - so that we have our own copy that's different from our client.
		BOOST_FOREACH(const GPlatesModel::MetadataContainer::value_type &metadata_entry, metadata_.get())
		{
			metadata.push_back(metadata_entry->clone());
		}
	}
}


GPlatesPropertyValues::GpmlFiniteRotation::Revision::Revision(
		const Revision &other,
		boost::optional<GPlatesModel::RevisionContext &> context_) :
	PropertyValue::Revision(context_),
	finite_rotation(other.finite_rotation)
{
	// Clone each metadata entry.
	BOOST_FOREACH(const GPlatesModel::MetadataContainer::value_type &metadata_entry, other.metadata)
	{
		metadata.push_back(metadata_entry->clone());
	}
}


bool
GPlatesPropertyValues::GpmlFiniteRotation::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (finite_rotation != other_revision.finite_rotation)
	{
		return false;
	}

	if (metadata.size() != other_revision.metadata.size())
	{
		return false;
	}

	// Copy of other metadata so can remove equality matches.
	GPlatesModel::MetadataContainer other_metadata(other_revision.metadata);

	// FIXME: Change from O(N^2) search to something faster.
	// Perhaps not really needed though, since metadata container sizes should be quite small.
	GPlatesModel::MetadataContainer::const_iterator metadata_iter = metadata.begin();
	GPlatesModel::MetadataContainer::const_iterator metadata_end = metadata.end();
	for ( ; metadata_iter != metadata_end; ++metadata_iter)
	{
		const GPlatesModel::Metadata &metadata_entry = **metadata_iter;

		GPlatesModel::MetadataContainer::iterator other_metadata_iter = other_metadata.begin();
		GPlatesModel::MetadataContainer::iterator other_metadata_end = other_metadata.end();
		for ( ; other_metadata_iter != other_metadata_end; ++other_metadata_iter)
		{
			const GPlatesModel::Metadata &other_metadata_entry = **other_metadata_iter;

			if (metadata_entry == other_metadata_entry)
			{
				// Remove other metadata entry so we don't compare against it twice.
				other_metadata.erase(other_metadata_iter);
				break;
			}
		}

		if (other_metadata_iter == other_metadata_end)
		{
			// Didn't find a match for the current metadata entry.
			return false;
		}
	}

	return PropertyValue::Revision::equality(other);
}
