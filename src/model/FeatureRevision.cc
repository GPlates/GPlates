/* $Id$ */

/**
 * \file 
 * Contains the implementation of the FeatureRevision class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "FeatureRevision.h"


const GPlatesModel::FeatureRevision::non_null_ptr_type
GPlatesModel::FeatureRevision::create(
		const RevisionId &revision_id_)
{
	return non_null_ptr_type(
			new FeatureRevision(
				revision_id_));
}


const GPlatesModel::FeatureRevision::non_null_ptr_type
GPlatesModel::FeatureRevision::clone() const
{
	return non_null_ptr_type(
			new FeatureRevision(
				*this));
}


const GPlatesModel::FeatureRevision::non_null_ptr_type
GPlatesModel::FeatureRevision::clone(
		const property_predicate_type &clone_properties_predicate) const
{
	return non_null_ptr_type(
			new FeatureRevision(
				*this,
				clone_properties_predicate));
}


const GPlatesModel::RevisionId &
GPlatesModel::FeatureRevision::revision_id() const
{
	// Regenerate if necessary.
	if (d_revision_id_dirty)
	{
		d_revision_id = RevisionId();
		d_revision_id_dirty = false;
	}

	return d_revision_id;
}


GPlatesModel::container_size_type
GPlatesModel::FeatureRevision::add(
		TopLevelProperty::non_null_ptr_type new_child)
{
	container_size_type result = BasicRevision<FeatureHandle>::add(new_child);
	d_revision_id_dirty = true;
	return result;
}


bool
GPlatesModel::FeatureRevision::remove(
		container_size_type index)
{
	bool result = BasicRevision<FeatureHandle>::remove(index);
	if (result)
	{
		d_revision_id_dirty = true;
	}
	return result;
}


void
GPlatesModel::FeatureRevision::set(
		container_size_type index,
		TopLevelProperty::non_null_ptr_type new_child)
{
	BasicRevision<FeatureHandle>::set(index, new_child);
	d_revision_id_dirty = true;
}


GPlatesModel::FeatureRevision::FeatureRevision(
		const RevisionId &revision_id_) :
	d_revision_id(revision_id_),
	d_revision_id_dirty(false)
{
}


GPlatesModel::FeatureRevision::FeatureRevision(
		const this_type &other) :
	BasicRevision<FeatureHandle>(other),
	GPlatesUtils::ReferenceCount<FeatureRevision>(),
	d_revision_id(other.d_revision_id),
	d_revision_id_dirty(other.d_revision_id_dirty)
{
}


GPlatesModel::FeatureRevision::FeatureRevision(
		const this_type &other,
		const property_predicate_type &clone_properties_predicate) :
	BasicRevision<FeatureHandle>(
			other,
			clone_properties_predicate),
	GPlatesUtils::ReferenceCount<FeatureRevision>(),
	d_revision_id(other.d_revision_id),
	d_revision_id_dirty(other.d_revision_id_dirty)
{
}

