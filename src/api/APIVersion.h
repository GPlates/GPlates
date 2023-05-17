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

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"
#include "scribe/TranscribeEnumProtocol.h"

#include "utils/QtStreamable.h"


namespace GPlatesApi
{
	/**
	 * The GPlates Python API (pyGPlates) version number.
	 *
	 * This is formatted in the PEP440 versioning scheme (https://www.python.org/dev/peps/pep-0440/).
	 *
	 * NOTE: To update the version you'll need to edit 'cmake/modules/Version.cmake'
	 *       (which will then require running cmake again).
	 */
	class Version :
			public boost::less_than_comparable<Version>,
			public boost::equality_comparable<Version>,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<Version>
	{
	public:

		/**
		 * Gets the current version (of this imported pyGPlates build).
		 */
		static
		Version
		get_imported_version();


		/**
		 * Creates a Version using the specified major, minor, patch numbers and
		 * optional pre-release PEP440 suffix "(.dev|a|b|rc)N".
		 *
		 * Note: The pre-release suffix (if specified) should use the PEP440 format restricted to just
		 *       ".devN", "aN", "bN" and "rcN" (where N is a non-negative integer).
		 */
		Version(
				unsigned int major,
				unsigned int minor,
				unsigned int patch = 0,
				boost::optional<QString> prerelease_suffix_string = boost::none);


		/**
		 * Create using the specified PEP440 version string "N.N[.N][(.dev|a|b|rc)N]".
		 *
		 * Note: The pre-release suffix (if included) should use the PEP440 format restricted to just
		 *       ".devN", "aN", "bN" and "rcN" (where N is a non-negative integer).
		 */
		explicit
		Version(
				QString version_string);


		unsigned int
		get_major() const
		{
			return d_major;
		}

		unsigned int
		get_minor() const
		{
			return d_minor;
		}

		unsigned int
		get_patch() const
		{
			return d_patch;
		}

		/**
		 * Return the optional pre-release PEP440 suffix "(.dev|a|b|rc)N".
		 */
		boost::optional<QString>
		get_prerelease_suffix_string() const;


		/**
		 * Return the PEP440 version string "N.N.N[(.dev|a|b|rc)N]".
		 */
		QString
		get_version_string() const;


		/**
		 * Equality comparison operator.
		 *
		 * The inequality comparison operator is provided by boost::equality_comparable.
		 */
		bool
		operator==(
				const Version &rhs) const;


		/**
		 * Less than comparison operator.
		 *
		 * The other comparison operators are provided by boost::less_than_comparable.
		 */
		bool
		operator<(
				const Version &rhs) const;

	private:

		struct PrereleaseSuffix
		{
			// NOTE: These enum values are ordered by version precedence.
			enum Type
			{
				DEVELOPMENT,
				ALPHA,
				BETA,
				RELEASE_CANDIDATE
			};

			Type type;
			unsigned int number;

		private: // Transcribe...

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			// Using friend function injection for access to enum of private nested class.
			friend
			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					Type &type,
					bool transcribed_construct_data)
			{
				// WARNING: Changing the string ids will break backward/forward compatibility.
				//          So don't change the string ids even if the enum name changes.
				static const GPlatesScribe::EnumValue enum_values[] =
				{
					GPlatesScribe::EnumValue("DEVELOPMENT", DEVELOPMENT),
					GPlatesScribe::EnumValue("ALPHA", ALPHA),
					GPlatesScribe::EnumValue("BETA", BETA),
					GPlatesScribe::EnumValue("RELEASE_CANDIDATE", RELEASE_CANDIDATE)
				};

				return GPlatesScribe::transcribe_enum_protocol(
						TRANSCRIBE_SOURCE,
						scribe,
						type,
						enum_values,
						enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
			}
		};

		unsigned int d_major;
		unsigned int d_minor;
		unsigned int d_patch;
		// Optional pre-release suffix...
		boost::optional<PrereleaseSuffix> d_prerelease_suffix;


		static
		boost::optional<PrereleaseSuffix>
		extract_prerelease_suffix(
				QString prerelease_suffix_string);

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<Version> &version);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/**
	 * Prints @a version in PEP440 format.
	 */
	std::ostream &
	operator<<(
			std::ostream &os,
			const Version &version);
}

#endif // GPLATES_API_APIVERSION_H
