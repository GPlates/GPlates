/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_GPMLREADEREXCEPTION_H
#define GPLATES_FILE_IO_GPMLREADEREXCEPTION_H

#include "ReadErrors.h"

#include "global/GPlatesException.h"

#include "model/XmlNode.h"


namespace GPlatesFileIO
{
	/**
	 * An exception type used when reading GPML files.
	 */
	class GpmlReaderException :
			public GPlatesGlobal::Exception
	{
	public:

		GpmlReaderException(
				const GPlatesUtils::CallStack::Trace &exception_source_,
				const GPlatesModel::XmlElementNode::non_null_ptr_type &location_,
				const ReadErrors::Description &description_,
				const char *source_location_ = "not specified") :
			GPlatesGlobal::Exception(exception_source_),
			d_location(location_),
			d_description(description_), 
			d_source_location(source_location_)
		{  }

		~GpmlReaderException() throw() { }

		const GPlatesModel::XmlElementNode::non_null_ptr_type
		location() const
		{
			return d_location;
		}

		ReadErrors::Description
		description() const
		{
			return d_description;
		}

		const char *
		source_location() const
		{
			return d_source_location;
		}

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "GpmlReaderException";
		}

	private:

		GPlatesModel::XmlElementNode::non_null_ptr_type d_location;
		ReadErrors::Description d_description;
		const char *d_source_location;
	};
}

#endif // GPLATES_FILE_IO_GPMLREADEREXCEPTION_H
