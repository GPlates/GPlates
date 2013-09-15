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

#include "GmlFile.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


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


const GPlatesPropertyValues::GmlFile::non_null_ptr_type
GPlatesPropertyValues::GmlFile::create(
		const composite_value_type &range_parameters_,
		const XsString::non_null_ptr_type &file_name_,
		const XsString::non_null_ptr_type &file_structure_,
		const boost::optional<XsString::non_null_ptr_type> &mime_type_,
		const boost::optional<XsString::non_null_ptr_type> &compression_,
		GPlatesFileIO::ReadErrorAccumulation *read_errors_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(
			new GmlFile(
					transaction,
					range_parameters_, file_name_, file_structure_, mime_type_, compression_, read_errors_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GmlFile::set_range_parameters(
		const composite_value_type &range_parameters_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().range_parameters = range_parameters_;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlFile::set_file_name(
		XsString::non_null_ptr_type file_name_,
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_revision<Revision>();

	revision.file_name.change(revision_handler.get_model_transaction(), file_name_);

	revision_handler.commit();

	// Update the proxied raster cache using the new filename.
	// NOTE: We do this *after* the commit otherwise we would be looking at an old version filename.
	revision.update_proxied_raster_cache(read_errors);
}


void
GPlatesPropertyValues::GmlFile::set_file_structure(
		XsString::non_null_ptr_type file_structure_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().file_structure.change(
			revision_handler.get_model_transaction(), file_structure_);
	revision_handler.commit();
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type>
GPlatesPropertyValues::GmlFile::get_mime_type() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.mime_type)
	{
		return boost::none;
	}

	return GPlatesUtils::static_pointer_cast<const XsString>(
			revision.mime_type->get_revisionable());
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type>
GPlatesPropertyValues::GmlFile::get_mime_type()
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.mime_type)
	{
		return boost::none;
	}

	return revision.mime_type->get_revisionable();
}


void
GPlatesPropertyValues::GmlFile::set_mime_type(
		boost::optional<XsString::non_null_ptr_type> mime_type_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	if (revision.mime_type)
	{
		if (mime_type_)
		{
			revision.mime_type->change(revision_handler.get_model_transaction(), mime_type_.get());
		}
		else
		{
			revision.mime_type->detach(revision_handler.get_model_transaction());
			revision.mime_type = boost::none;
		}
	}
	else if (mime_type_)
	{
		revision.mime_type = GPlatesModel::RevisionedReference<XsString>::attach(
				revision_handler.get_model_transaction(), *this, mime_type_.get());
	}
	// ...else nothing to do.

	revision_handler.commit();
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type>
GPlatesPropertyValues::GmlFile::get_compression() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.compression)
	{
		return boost::none;
	}

	return GPlatesUtils::static_pointer_cast<const XsString>(
			revision.compression->get_revisionable());
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type>
GPlatesPropertyValues::GmlFile::get_compression()
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.compression)
	{
		return boost::none;
	}

	return revision.compression->get_revisionable();
}


void
GPlatesPropertyValues::GmlFile::set_compression(
		boost::optional<XsString::non_null_ptr_type> compression_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	if (revision.compression)
	{
		if (compression_)
		{
			revision.compression->change(revision_handler.get_model_transaction(), compression_.get());
		}
		else
		{
			revision.compression->detach(revision_handler.get_model_transaction());
			revision.compression = boost::none;
		}
	}
	else if (compression_)
	{
		revision.compression = GPlatesModel::RevisionedReference<XsString>::attach(
				revision_handler.get_model_transaction(), *this, compression_.get());
	}
	// ...else nothing to do.

	revision_handler.commit();
}


std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesPropertyValues::GmlFile::get_proxied_raw_rasters() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.proxied_raster_cache)
	{
		// We can't actually report the read errors to the user.
		GPlatesFileIO::ReadErrorAccumulation read_errors;
		revision.update_proxied_raster_cache(&read_errors);
	}

	return revision.proxied_raster_cache.get()->proxied_raw_rasters();
}


std::ostream &
GPlatesPropertyValues::GmlFile::print_to(
		std::ostream &os) const
{
	return os << *get_current_revision<Revision>().file_name.get_revisionable();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GmlFile::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	if (child_revisionable == revision.file_name.get_revisionable())
	{
		// Invalidate the proxied raster cache because that's calculated using the filename.
		// We can't actually re-calculate the cache because we don't know the new filename yet
		// because the child property value hasn't modified it just yet.
		revision.proxied_raster_cache = boost::none;

		return revision.file_name.clone_revision(transaction);
	}
	if (child_revisionable == revision.file_structure.get_revisionable())
	{
		return revision.file_structure.clone_revision(transaction);
	}
	if (revision.mime_type &&
		child_revisionable == revision.mime_type->get_revisionable())
	{
		return revision.mime_type->clone_revision(transaction);
	}
	if (revision.compression &&
		child_revisionable == revision.compression->get_revisionable())
	{
		return revision.compression->clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesPropertyValues::GmlFile::Revision::Revision(
		GPlatesModel::ModelTransaction &transaction_,
		RevisionContext &child_context_,
		const composite_value_type &range_parameters_,
		const XsString::non_null_ptr_type &file_name_,
		const XsString::non_null_ptr_type &file_structure_,
		const boost::optional<XsString::non_null_ptr_type> &mime_type_,
		const boost::optional<XsString::non_null_ptr_type> &compression_,
		GPlatesFileIO::ReadErrorAccumulation *read_errors_) :
	range_parameters(range_parameters_),
	file_name(
			GPlatesModel::RevisionedReference<XsString>::attach(
					transaction_, child_context_, file_name_)),
	file_structure(
			GPlatesModel::RevisionedReference<XsString>::attach(
					transaction_, child_context_, file_structure_)),
	proxied_raster_cache(ProxiedRasterCache::create(file_name_->get_value(), read_errors_))
{
	if (mime_type_)
	{
		mime_type = GPlatesModel::RevisionedReference<XsString>::attach(
				transaction_, child_context_, mime_type_.get());
	}

	if (compression_)
	{
		compression = GPlatesModel::RevisionedReference<XsString>::attach(
				transaction_, child_context_, compression_.get());
	}
}


GPlatesPropertyValues::GmlFile::Revision::Revision(
		const Revision &other_,
		boost::optional<RevisionContext &> context_,
		RevisionContext &child_context_) :
	PropertyValue::Revision(context_),
	range_parameters(other_.range_parameters),
	file_name(other_.file_name),
	file_structure(other_.file_structure),
	mime_type(other_.mime_type),
	compression(other_.compression)
{
	// Clone data members that were not deep copied.
	file_name.clone(child_context_);
	file_structure.clone(child_context_);

	if (mime_type)
	{
		mime_type->clone(child_context_);
	}

	if (compression)
	{
		compression->clone(child_context_);
	}
}


GPlatesPropertyValues::GmlFile::Revision::Revision(
		const Revision &other_,
		boost::optional<RevisionContext &> context_) :
	PropertyValue::Revision(context_),
	range_parameters(other_.range_parameters),
	file_name(other_.file_name),
	file_structure(other_.file_structure),
	mime_type(other_.mime_type),
	compression(other_.compression)
{
}


bool
GPlatesPropertyValues::GmlFile::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return range_parameters == other_revision.range_parameters &&
			*file_name.get_revisionable() == *other_revision.file_name.get_revisionable() &&
			*file_structure.get_revisionable() == *other_revision.file_structure.get_revisionable() &&
			opt_eq(mime_type, other_revision.mime_type) &&
			opt_eq(compression, other_revision.compression) &&
			GPlatesModel::Revision::equality(other);
}


void
GPlatesPropertyValues::GmlFile::Revision::update_proxied_raster_cache(
		GPlatesFileIO::ReadErrorAccumulation *read_errors) const
{
	// Update the proxied raster cache.
	if (proxied_raster_cache)
	{
		proxied_raster_cache.get()->set_file_name(file_name.get_revisionable()->get_value(), read_errors);
	}
	else
	{
		proxied_raster_cache =
				ProxiedRasterCache::create(file_name.get_revisionable()->get_value(), read_errors);
	}
}
