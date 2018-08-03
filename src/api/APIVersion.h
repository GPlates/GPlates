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

#ifndef GPLATES_API_APIVERSION_H
#define GPLATES_API_APIVERSION_H

#include <iosfwd>
#include <string>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "global/python.h"

#include "utils/QtStreamable.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * The GPlates Python API (pyGPlates) version number.
	 *
	 * In string format the version number looks like "<MAJOR>.<MINOR>.<PATCH>.<REVISION>" where
	 * <MAJOR>, <MINOR> and <PATCH> are the GPlates version and <REVISION> is the pyGPlates
	 * revision number that is incremented for each pyGPlates API change (and never reset to zero).
	 *
	 * However, comparison of Version objects only uses the <REVISION> number since this alone
	 * identifies API changes.
	 *
	 * NOTE: To increment the revision number (when an API change is made) you will need to
	 * increment the 'PYGPLATES_REVISION' variable in 'cmake/ConfigDefault.cmake' (which will
	 * then require running cmake again).
	 */
	class Version :
			public boost::less_than_comparable<Version>,
			public boost::equality_comparable<Version>,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<Version>
	{
	public:

		static
		unsigned int
		get_imported_major();

		static
		unsigned int
		get_imported_minor();

		static
		unsigned int
		get_imported_patch();

		static
		unsigned int
		get_imported_revision();


		/**
		 * Gets the current version (of this imported pyGPlates build).
		 *
		 * Using 'operator <<' on the returned instance will print "<REVISION> (GPlates <MAJOR>.<MINOR>.<PATCH>)".
		 */
		static
		Version
		get_imported_version()
		{
			return Version();
		}


		/**
		 * Creates a Version using the specified <REVISION> number.
		 *
		 * Using 'operator <<' on the returned instance will print "<REVISION>".
		 * Note that this doesn't print the GPlates version like @a get_imported_version does
		 * because we don't know which GPlates version is associated with @a revision.
		 */
		explicit
		Version(
				unsigned int revision) :
			d_revision(revision)
		{  }


		/**
		 * Returns the revision number.
		 */
		unsigned int
		get_revision() const
		{
			return d_revision ? d_revision.get() : get_imported_revision();
		}


		/**
		 * Equality comparison operator.
		 *
		 * NOTE: This only compares using the <REVISION> number.
		 *
		 * The inequality comparison operator is provided by boost::equality_comparable.
		 */
		bool
		operator==(
				const Version &rhs) const
		{
			return get_revision() == rhs.get_revision();
		}


		/**
		 * Less than comparison operator.
		 *
		 * NOTE: This only compares using the <REVISION> number.
		 *
		 * The other comparison operators are provided by boost::less_than_comparable.
		 */
		bool
		operator<(
				const Version &rhs) const
		{
			return get_revision() < rhs.get_revision();
		}

	private:


		//! Constructor used by @a get_imported_revision.
		Version()
		{  }


		//! Revision number - if none then use @a get_imported_revision.
		boost::optional<unsigned int> d_revision;


		// Give access to 'd_revision'.
		friend std::ostream & operator<<(std::ostream &, const Version &);
	};


	/**
	 * Prints "<REVISION> (GPlates <MAJOR>.<MINOR>.<PATCH>)" if @a version was created with
	 * @a get_imported_version, otherwise just prints "<REVISION>".
	 */
	std::ostream &
	operator<<(
			std::ostream &os,
			const Version &version);
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_APIVERSION_H
