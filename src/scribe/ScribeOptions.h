/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBEOPTIONS_H
#define GPLATES_SCRIBE_SCRIBEOPTIONS_H

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>


namespace GPlatesScribe
{
	// Forward declarations...
	class Options;


	/**
	 * The comma operator is used to combine options (and optionally a version) into a single aggregate option.
	 *
	 * It is used when transcribing as in the following...
	 *
	 *    scribe.transcribe(
	 *            TRANSCRIBE_SOURCE,
	 *            my_object,
	 *            "my_object",
	 *            (GPlatesScribe::Version(1), GPlatesScribe::EXCLUSIVE_OWNER, GPlatesScribe::DONT_TRACK));
	 *
	 * ...where the last argument of Scribe::transcribe() is...
	 *
	 *    (EXCLUSIVE_OWNER, DONT_TRACK, Version(1))
	 *
	 * ...and where the 'GPlatesScribe' qualifier has been removed for easier reading.
	 *
	 * Other examples include...
	 *
	 *    (Version(1), SHARED_OWNER, DONT_TRACK)
	 *
	 *    (EXCLUSIVE_OWNER, DONT_TRACK)             // Version defaults to zero.
	 *
	 *    Version(1)
	 *
	 *    DONT_TRACK                                // Version defaults to zero.
	 *
	 *    SHARED_OWNER                              // Version defaults to zero.
	 *
	 *    (EXCLUSIVE_OWNER, Version(1), DONT_TRACK)
	 *
	 * ...where some of those examples don't need to use the comma operator (because only a single option).
	 *
	 */
	Options
	operator,(
			const Options &lhs,
			const Options &rhs);


	/**
	 * An option flag for transcribe options.
	 *
	 * Flags are wrapped in a class to distinguish from integer version number.
	 */
	class OptionFlag
	{
	public:

		explicit
		OptionFlag(
				unsigned int bit) :
			d_flag(1 << bit)
		{  }

		unsigned int
		get() const
		{
			return d_flag;
		}

	private:

		unsigned int d_flag;
	};


	// All objects are tracked by default - use this option to request *no* tracking on an object...
	const OptionFlag DONT_TRACK(0);

	// A pointer can optionally specify that it exclusively owns the pointed-to object
	// (only applies to pointers)...
	const OptionFlag EXCLUSIVE_OWNER(1);

	// A pointer can optionally specify that it shares ownership of the pointed-to object with other pointers
	// (only applies to pointers)...
	const OptionFlag SHARED_OWNER(2);


	/**
	 * Version number for transcribe options.
	 *
	 * Number is wrapped in a class to distinguish from integer option flags.
	 */
	class Version
	{
	public:

		//! Defaults to version zero.
		explicit
		Version(
				unsigned int version = 0) :
			d_version(version)
		{  }

		unsigned int
		get() const
		{
			return d_version;
		}

	private:

		unsigned int d_version;
	};


	/**
	 * Transcribe options combined into a version number and bit flag options for ease-of-use.
	 */
	class Options
	{
	public:

		//! Default options (and version defaults to zero).
		Options() :
			d_options(0)
		{  }

		//! Implicitly convertible from @a OpionFlag.
		Options(
				OptionFlag flag);

		//! Implicitly convertible from @a Version.
		Options(
				Version version);


		unsigned int
		get_flags() const
		{
			return d_options & 0xffff; // Mask out the version.
		}

		unsigned int
		get_version() const
		{
			return d_options >> 16; // Shift out the option flags.
		}


		/**
		 * Convenience method to remove a flag (if it exists).
		 *
		 * To add a flag use the comma operator.
		 * For example use '(options, DONT_TRACK)' to add DONT_TRACK to options.
		 */
		void
		remove_flag(
				OptionFlag flag)
		{
			d_options &= ~boost::uint32_t(flag.get());
		}

	private:

		//! Combine version number (high 16 bits) and flags (low 16 bits) into a single 32-bit integer.
		boost::uint32_t d_options;

		friend Options operator,(const Options &, const Options &);
	};
}

//
// Implementation.
//

namespace GPlatesScribe
{
	/**
	 * Overload of comma operator to avoid case where first parameter is ignored because it is not
	 * convertible to a @a Options - when this happens only the second parameter is used (though
	 * some compilers will at least warn of an unused parameter).
	 *
	 * For example, the following will generate a compile-time error due to this overloaded operator:
	 *
	 *   (1, DONT_TRACK)
	 *
	 * ...which can be fixed by using the following instead...
	 *
	 *   (Version(1), DONT_TRACK)
	 *
	 * Note that the opposite order, for example '(DONT_TRACK, 1)', is already caught by the
	 * compiler since the last parameter is not convertible to @a Options.
	 */
	template <typename NonOptionType>
	Options
	operator,(
			const NonOptionType &lhs,
			const Options &rhs)
	{
		// Make sure compiler does not instantiate this function.
		//
		// An example that triggers this compile-time error is:
		//
		//   (1, DONT_TRACK)
		//
		// ...which can be fixed by using the following instead...
		//
		//   (Version(1), DONT_TRACK)
		//
		// Evaluates to false - but is dependent on template parameter - compiler workaround...
		BOOST_STATIC_ASSERT(sizeof(NonOptionType) == 0);

		return Options();
	}

	/**
	 * Prevent the above template overloaded comma operator from giving a compile-time error
	 * when 'lhs' is 'OptionFlag'.
	 */
	inline
	Options
	operator,(
			const OptionFlag &lhs,
			const Options &rhs)
	{
		return Options(lhs), rhs;
	}

	/**
	 * Prevent the above template overloaded comma operator from giving a compile-time error
	 * when 'lhs' is 'Version'.
	 */
	inline
	Options
	operator,(
			const Version &lhs,
			const Options &rhs)
	{
		return Options(lhs), rhs;
	}
}

#endif // GPLATES_SCRIBE_SCRIBEOPTIONS_H
