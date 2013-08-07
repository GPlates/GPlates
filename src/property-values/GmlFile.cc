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


namespace
{
	template<class T>
	bool
	opt_cow_eq(
			const boost::optional<GPlatesUtils::CopyOnWrite<T> > &opt1,
			const boost::optional<GPlatesUtils::CopyOnWrite<T> > &opt2)
	{
		if (opt1)
		{
			if (!opt2)
			{
				return false;
			}
			return *opt1.get().get_const() == *opt2.get().get_const();
		}
		else
		{
			return !opt2;
		}
	}
}


void
GPlatesPropertyValues::GmlFile::set_range_parameters(
		const composite_value_type &range_parameters_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().range_parameters = range_parameters_;
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GmlFile::set_file_name(
		const XsString::non_null_ptr_to_const_type &file_name_,
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	MutableRevisionHandler revision_handler(this);

	Revision &revision = revision_handler.get_mutable_revision<Revision>();

	revision.file_name = file_name_;
	revision.proxied_raster_cache->set_file_name(file_name_->get_value(), read_errors);

	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GmlFile::set_file_structure(
		const XsString::non_null_ptr_to_const_type &file_structure_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().file_structure = file_structure_;
	revision_handler.handle_revision_modification();
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type>
GPlatesPropertyValues::GmlFile::get_mime_type() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.mime_type)
	{
		return boost::none;
	}

	return revision.mime_type->get();
}


void
GPlatesPropertyValues::GmlFile::set_mime_type(
		const boost::optional<XsString::non_null_ptr_to_const_type> &mime_type_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().mime_type = mime_type_;
	revision_handler.handle_revision_modification();
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type>
GPlatesPropertyValues::GmlFile::get_compression() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.compression)
	{
		return boost::none;
	}

	return revision.compression->get();
}


void
GPlatesPropertyValues::GmlFile::set_compression(
		const boost::optional<XsString::non_null_ptr_to_const_type> &compression_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().compression = compression_;
	revision_handler.handle_revision_modification();
}


std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesPropertyValues::GmlFile::get_proxied_raw_rasters() const
{
	return get_current_revision<Revision>().proxied_raster_cache->proxied_raw_rasters();
}


std::ostream &
GPlatesPropertyValues::GmlFile::print_to(
		std::ostream &os) const
{
	return os << *get_current_revision<Revision>().file_name.get_const();
}


bool
GPlatesPropertyValues::GmlFile::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return range_parameters == other_revision.range_parameters &&
			*file_name.get_const() == *other_revision.file_name.get_const() &&
			*file_structure.get_const() == *other_revision.file_structure.get_const() &&
			opt_cow_eq(mime_type, other_revision.mime_type) &&
			opt_cow_eq(compression, other_revision.compression) &&
			GPlatesModel::PropertyValue::Revision::equality(other);
}
