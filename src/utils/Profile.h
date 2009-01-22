/* $Id$ */

/**
 * \file 
 * Source code profiling.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_PROFILE_H
#define GPLATES_UTILS_PROFILE_H

#include <iosfwd>
#include <string>


/**
    This file contains interface macros for profiling
	(recording the time spent in) various sections of source code.

    To profile a section of source code make a call to PROFILE_BEGIN/PROFILE_END
	or call PROFILE_BLOCK or PROFILE_FUNC or PROFILE_CODE.
	To get the profiling results written to a text file or output stream call
	PROFILE_REPORT_TO_FILE or PROFILE_REPORT_TO_OSTREAM.
	Each time the path of execution enters a profiled section of source code
	the time recorded in this section is added to the name associated with
	the profile. It is possible, and desirable, to profile at various
    depths in the function call stack. If a function is called during
    a profiled section of code named "profileA" and part of that function
    also has a profiled section of code named "profileB" then the time spent
	in the part of the child function that is not profiled (ie, not in "profileB")
	is considered part of the parent profile (in this case "profileA"). In
    other words, time spent in nested profiles is subtracted from
    parent profiles. The time is only accurate if no other
    processes are consuming CPU resources (otherwise a process context
    switch that occurs in the middle of a section of profiled source
    code would cause the time spent in the switched process to be
    included in the time recorded for the profiled section of source
    code). This profiling can be very accurate as long no other
    cpu-intensive processes are running on the same machine AND the section of
    profiled code is at least a few hundred machine cycles. The second
	requirement is because there is a small inaccuracy in profiled times due
	to calling and returning from calls to the profiling code itself. This
	inaccuracy is designed to be as small as possible by not counting the time
	spent inside the profiling code itself. As a result the inaccuracy is
	only on the order of tens of machine cycles.
	If you have a loop that is called very often but the code inside the loop
	is small but yet consumes a significant amount of total program execution time
	then inserting a profile call inside the loop iteration can significantly
	slow down program running time since the time spent inside the profile call
	itself (not the code being profiled) will be comparable to the time spent
	in the profiled code. Note that the time spent inside the profile call itself
	is not counted though (other than the inaccuracy mentioned above).

    An example..

    #include "utils/Profile.h"

    int main()
    {
	  func();
      bar();
      foo();

      PROFILE_REPORT_TO_FILE("profile.txt");
      
      return 0;
    }

	func()
	{
	  PROFILE_FUNC();
	  ... // some code profiled under "func"

	  // The call to foo() gets profiled under "foo()".
	  PROFILE_CODE(foo());
	}

    bar()
    {
      ... // some non-profiled code

      // Start of block A
      ...  // This part of block is not profiled in "bar_blockA"

      PROFILE_BEGIN("bar_blockA"); // profiling of "bar_blockA" begins here
      foo();
      // profiling of "bar_blockA" stops here
	  PROFILE_END();

      ... // some more non-profiled code

      // Start of block B
      PROFILE_BEGIN("bar_blockB"); // profiling of "bar_blockB" begins here
      foo();
      // profiling of "bar_blockB" stops here
	  PROFILE_END();
    }

    foo()
    {
      ... // some source code that gets profiled into "bar_blockA" or
          // "bar_blockB" if 'foo()' called from 'bar()' or "foo()" if 'foo()'
		  // called from 'func()' or doesn't get profiled
          // at all if 'foo()' called from 'main()'.

      PROFILE_BLOCK("foo"); // profiling of 'foo' begins here
      ... // Some source code that is profiled in "foo". If parent profile is
          // "bar_blockA" or "bar_blockB" or "foo()" then time spent in this section is
          // subtracted from those profiles.
          // profiling of 'foo' stops here as scope in which PROFILE_BLOCK was called ends.
    }
*/

#if defined(PROFILE_GPLATES)

/**
 * Starts profiling until a matching PROFILE_END is reached.
 * All profiled sections of code with the same @a name have their time counts added.
 * The profile report will have an entry for each distinct @a name.
 * You need to match each PROFILE_BEGIN call with a matching PROFILE_END call.
 */
#define PROFILE_BEGIN(name) \
	static void *PROFILE_ANONYMOUS_VARIABLE(gplates_profile_cache) = \
			GPlatesUtils::profile_get_cache(name); \
	GPlatesUtils::profile_begin(PROFILE_ANONYMOUS_VARIABLE(gplates_profile_cache));

/**
 * Stops profiling the most recent call to PROFILE_BEGIN.
 * The total time spent between PROFILE_BEGIN and PROFILE_END is accumulated into
 * the profile node identified by @a name.
 * The profile report will have an entry for each distinct @a name.
 * You need to match each PROFILE_END call with a matching PROFILE_BEGIN call.
 */
#define PROFILE_END() \
	GPlatesUtils::profile_end();

/**
 * Starts profiling until the end of the current scope in which this PROFILE_BLOCK
 * call was made. The end of the scope is the same as when the destructor of a
 * local object would be called if it was created at the call site of PROFILE_BLOCK.
 * There is no need to call PROFILE_END as PROFILE_BLOCK calls both PROFILE_BEGIN
 * and PROFILE_END.
 */
#define PROFILE_BLOCK(name) \
	PROFILE_BEGIN(name); \
	GPlatesUtils::ProfileBlockEnd PROFILE_UNUSED PROFILE_ANONYMOUS_VARIABLE(gplates_profile_block);

/**
 * Same as @a PROFILE_BLOCK except the name of the profile is the function
 * that PROFILE_BLOCK is called from.
 * There is no need to call PROFILE_END as PROFILE_FUNC calls both PROFILE_BEGIN
 * and PROFILE_END.
 */
#define PROFILE_FUNC() \
	PROFILE_BLOCK(__FUNCTION__);

/**
 * Starts profiling just before the source code expression @a code and stops
 * profiling just after.
 * There is no need to call PROFILE_END as PROFILE_CODE calls both PROFILE_BEGIN
 * and PROFILE_END.
 *
 * For example...
 *
 *   PROFILE_CODE(const bool test = foo() || bar(););
 *
 * ...is equivalent to...
 *
 *   PROFILE_BEGIN("const bool test = foo() || bar();");
 *   const bool test = foo() || bar();
 *   PROFILE_END();
 *
 */
#define PROFILE_CODE(code) \
	PROFILE_BEGIN(#code); \
	{ \
		code; \
	} \
	PROFILE_END();

/**
 * Writes the profiling data as text to the output stream @a output_stream where
 * @a output_stream is a @a std::ostream &.
 */
#define PROFILE_REPORT_TO_OSTREAM(output_stream) \
	GPlatesUtils::profile_report_to_ostream(output_stream);

/**
 * Writes the profiling data as text to the file @a filename where
 * @a filename is a @a std::string.
 */
#define PROFILE_REPORT_TO_FILE(filename) \
	GPlatesUtils::profile_report_to_file(filename);

#else // if defined(PROFILE_GPLATES) ...

#define PROFILE_BEGIN(name)
#define PROFILE_END()
#define PROFILE_BLOCK(name)
#define PROFILE_FUNC()
#define PROFILE_CODE(code) code
#define PROFILE_REPORT_TO_OSTREAM(output_stream)
#define PROFILE_REPORT_TO_FILE(filename)

#endif // if defined(PROFILE_GPLATES) ... else ...


#define PROFILE_CONCATENATE_DIRECT(s1, s2) s1##s2
#define PROFILE_CONCATENATE(s1, s2)        PROFILE_CONCATENATE_DIRECT(s1, s2)
#define PROFILE_ANONYMOUS_VARIABLE(name)   PROFILE_CONCATENATE(name, __LINE__)

#if defined (__GNUG__)
#define PROFILE_UNUSED __attribute__ ((unused))
#else
#define PROFILE_UNUSED
#endif


namespace GPlatesUtils
{
	void *
	profile_get_cache(
			const char *name);

	void
	profile_begin(
			void *profile_cache);

	void
	profile_end();

	void
	profile_report_to_ostream(
			std::ostream &);

	void
	profile_report_to_file(
			const std::string &filename);

	/**
	 * Calls @a profile_end when lifetime of object ends.
	 */
	class ProfileBlockEnd
	{
	public:
		~ProfileBlockEnd()
		{
			profile_end();
		}
	};
}

#endif // GPLATES_UTILS_PROFILE_H
