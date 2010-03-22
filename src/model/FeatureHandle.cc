/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class FeatureHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "FeatureHandle.h"

GPlatesModel::FeatureHandle::~FeatureHandle()
{
}


const GPlatesModel::FeatureHandle::children_const_iterator
GPlatesModel::FeatureHandle::get_const_iterator(
		const children_iterator &iter)
{
	if (iter.collection_handle_ptr() == NULL)
	{
		return children_const_iterator();
	}
	return children_const_iterator::create_for_index(
			*(iter.collection_handle_ptr()), iter.index());
}


const GPlatesModel::FeatureHandle::const_weak_ref
GPlatesModel::FeatureHandle::get_const_weak_ref(
		const weak_ref &ref)
{
	if (ref.handle_ptr() == NULL)
	{
		return const_weak_ref();
	}
	return const_weak_ref(*(ref.handle_ptr()));
}


const GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesModel::FeatureHandle::create(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_)
{
	non_null_ptr_type ptr(
			new FeatureHandle(feature_type_, feature_id_),
			GPlatesUtils::NullIntrusivePointerHandler());
	return ptr;
}


const GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesModel::FeatureHandle::create(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_,
		const RevisionId &revision_id_)
{
	non_null_ptr_type ptr(
			new FeatureHandle(feature_type_, feature_id_, revision_id_),
			GPlatesUtils::NullIntrusivePointerHandler());
	return ptr;
}


const GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesModel::FeatureHandle::clone() const
{
	non_null_ptr_type dup(
			new FeatureHandle(*this),
			GPlatesUtils::NullIntrusivePointerHandler());
	return dup;
}


const GPlatesModel::FeatureHandle::const_weak_ref
GPlatesModel::FeatureHandle::reference() const
{
	const_weak_ref ref(*this);
	return ref;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::FeatureHandle::reference()
{
	weak_ref ref(*this);
	return ref;
}


const GPlatesModel::RevisionId &
GPlatesModel::FeatureHandle::revision_id() const
{
	return current_revision()->revision_id();
}


const GPlatesModel::FeatureHandle::children_const_iterator
GPlatesModel::FeatureHandle::children_begin() const
{
	return children_const_iterator::create_begin(*this);
}


const GPlatesModel::FeatureHandle::children_iterator
GPlatesModel::FeatureHandle::children_begin()
{
	return children_iterator::create_begin(*this);
}


const GPlatesModel::FeatureHandle::children_const_iterator
GPlatesModel::FeatureHandle::children_end() const
{
	return children_const_iterator::create_end(*this);
}


const GPlatesModel::FeatureHandle::children_iterator
GPlatesModel::FeatureHandle::children_end()
{
	return children_iterator::create_end(*this);
}


const GPlatesModel::FeatureHandle::children_iterator
GPlatesModel::FeatureHandle::append_child(
		TopLevelProperty::non_null_ptr_type new_top_level_property,
		DummyTransactionHandle &transaction)
{
	container_size_type new_index =
			current_revision()->append_child(new_top_level_property, transaction);
	return children_iterator::create_for_index(*this, new_index);
}


void
GPlatesModel::FeatureHandle::remove_child(
		children_const_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_child(iter.index(), transaction);
}


void
GPlatesModel::FeatureHandle::remove_child(
		children_iterator iter,
		DummyTransactionHandle &transaction)
{
	current_revision()->remove_child(iter.index(), transaction);
}


const GPlatesModel::FeatureRevision::non_null_ptr_to_const_type
GPlatesModel::FeatureHandle::current_revision() const
{
	return d_current_revision;
}


const GPlatesModel::FeatureRevision::non_null_ptr_type
GPlatesModel::FeatureHandle::current_revision()
{
	return d_current_revision;
}


void
GPlatesModel::FeatureHandle::set_current_revision(
		FeatureRevision::non_null_ptr_type rev)
{
	d_current_revision = rev;
}


void
GPlatesModel::FeatureHandle::set_parent_ptr(
		FeatureCollectionHandle *new_ptr)
{
	d_feature_collection_handle_ptr = new_ptr;
}


GPlatesModel::FeatureHandle::FeatureHandle(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_):
	d_current_revision(FeatureRevision::create()),
	d_handle_data(feature_type_, feature_id_),
	d_feature_collection_handle_ptr(NULL)
{
	d_handle_data.d_feature_id.set_back_ref_target(*this);
}


GPlatesModel::FeatureHandle::FeatureHandle(
		const FeatureType &feature_type_,
		const FeatureId &feature_id_,
		const RevisionId &revision_id_):
	d_current_revision(FeatureRevision::create(revision_id_)),
	d_handle_data(feature_type_, feature_id_),
	d_feature_collection_handle_ptr(NULL)
{
	d_handle_data.d_feature_id.set_back_ref_target(*this);
}


GPlatesModel::FeatureHandle::FeatureHandle(
		const FeatureHandle &other):
	WeakObserverPublisher<FeatureHandle>(),
	GPlatesUtils::ReferenceCount<FeatureHandle>(),
	d_current_revision(other.d_current_revision),
	d_handle_data(other.d_handle_data.d_feature_type, FeatureId()),
	d_feature_collection_handle_ptr(NULL)
{
	d_handle_data.d_feature_id.set_back_ref_target(*this);
}
