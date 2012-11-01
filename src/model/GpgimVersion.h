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

#ifndef GPLATES_MODEL_GPGIMVERSION_H
#define GPLATES_MODEL_GPGIMVERSION_H

#include <iosfwd>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "utils/QtStreamable.h"


namespace GPlatesModel
{
	/**
	 * The GPlates Geological Information Model (GPGIM) version number.
	 *
	 * In string format the version number looks like "<MAJOR>.<MINOR>.<REVISION>".
	 *
	 * For GPML files this version is stored in the 'gpml:version' attribute of the
	 * feature collection XML element.
	 */
	class GpgimVersion :
			public boost::less_than_comparable<GpgimVersion>,
			public boost::equality_comparable<GpgimVersion>,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<GpgimVersion>
	{
	public:

		//
		// The default version for "1.6" is "1.6.0317".
		//
		// That is the version GPlates has effectively been using since 2009 (until now - Sept 2012).
		// Although the GPML only stored "1.6" during that time.
		//
		static const unsigned int DEFAULT_ONE_POINT_SIX_REVISION = 317;


		/**
		 * Creates a GpgimVersion from a "<MAJOR>.<MINOR>.<REVISION>" version string,
		 * or boost::none if the version string cannot be parsed.
		 *
		 * NOTE: The major, minor and revision numbers in the string can optionally have zeros in front.
		 * For example, the <REVISION> part can be "0317" or "317".
		 * when this versioning was started.
		 *
		 * Note that the revision is (only) optional for "1.6" since that is when this versioning started.
		 * In this case the revision number will be set to 317 since that was the revision number
		 * when this versioning was started.
		 */
		static
		boost::optional<GpgimVersion>
		create(
				const QString &version);


		/**
		 * Constructs a @a GpgimVersion from version numbers.
		 *
		 * Throws exception if the version numbers do not match a valid version.
		 */
		GpgimVersion(
				unsigned int major_,
				unsigned int minor_,
				unsigned int revision_);


		/**
		 * Returns the major version number in "<MAJOR>.<MINOR>.<REVISION>".
		 */
		unsigned int
		major() const
		{
			return d_major;
		}


		/**
		 * Returns the minor version number in "<MAJOR>.<MINOR>.<REVISION>".
		 */
		unsigned int
		minor() const
		{
			return d_minor;
		}


		/**
		 * Returns the revision number in "<MAJOR>.<MINOR>.<REVISION>".
		 *
		 * NOTE: If only "1.6" was passed to @a create (ie, no revision number) then this will
		 * return the default revision for "1.6" which is 317.
		 */
		unsigned int
		revision() const
		{
			return d_revision;
		}


		/**
		 * Returns the version string as "<MAJOR>.<MINOR>.<REVISION>".
		 *
		 * Currently this follows the convention used on the GPGIM website:
		 *    http://www.earthbyte.org/Resources/GPGIM/feed_public.xml
		 * Which starts at at '0200' for version 1.5 and '0300' for version "1.6".
		 *
		 * For example, "1.6.0317" is returned for MAJOR=1, MINOR=6, REVISION=317
		 * - note the revision number (in the string) is "0317" instead of "317".
		 * However the version string passed to @a create can be either "1.6.0317" or "1.6.317".
		 */
		QString
		version_string() const;


		/**
		 * Equality comparison operator.
		 *
		 * The inequality comparison operator is provided by boost::equality_comparable.
		 */
		bool
		operator==(
				const GpgimVersion &rhs) const
		{
			return d_major == rhs.d_major &&
				d_minor == rhs.d_minor &&
				d_revision == rhs.d_revision;
		}


		/**
		 * Less than comparison operator.
		 *
		 * The other comparison operators are provided by boost::less_than_comparable.
		 */
		bool
		operator<(
				const GpgimVersion &rhs) const;

	private:

		unsigned int d_major;
		unsigned int d_minor;
		unsigned int d_revision;

	};


	std::ostream &
	operator<<(
			std::ostream &o,
			const GpgimVersion &gpml_version);
}

#endif // GPLATES_MODEL_GPGIMVERSION_H
