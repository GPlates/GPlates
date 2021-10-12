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
    profiled code is at least a few microseconds. The second requirement is
	because the overhead of profiling is on the order of a microsecond because
	API calls like the WIN32 QueryPerformanceCounter() take about 0.6 microseconds
	(in fact about 90% of the profiling overhead is due to QueryPerformanceCounter
	call alone) giving an inaccuracy in profiling time of around the same amount.
	And because the profile overhead is on the order of a microsecond - a profiled
	code segment that consumes a large proportion of the program running time will
	slow down program execution if it is also on the order of a microsecond.
	For example a tight loop that consumes 100% of program running time will slow
	down the total program running time by a factor of two if each loop iteration is
	2 x 0.6 microseconds = 1.2 microseconds long (note: the "2 x" here is because
	each profiled segment requires two calls to QueryPerformanceCounter). The program
	will now require 1.2 microseconds for each loop iteration code segment plus
	1.2 microseconds for the begin/end profile calls around each loop iteration
	code segment thus slowing down the total program running time by a factor of two.

    An example profile usage..

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
	  PROFILE_CODE(foo, foo());
	}

    bar()
    {
      ... // some non-profiled code

      // Start of block A
      ...  // This part of block is not profiled in "bar_blockA"

      PROFILE_BEGIN(bar_blockA, "bar_blockA"); // profiling of "bar_blockA" begins here
      foo();
      // profiling of "bar_blockA" stops here
	  PROFILE_END(bar_blockA);

      ... // some more non-profiled code

      // Start of block B
      PROFILE_BEGIN(bar_blockB, "bar_blockB"); // profiling of "bar_blockB" begins here
      foo();
      // profiling of "bar_blockB" stops here
	  PROFILE_END(bar_blockB);
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

/**
 * Define PROFILE_EXCLUDE_NEW_DELETE if you want to exclude the time spent in
 * operator new, new [], delete, and delete [].
 * NOTE: On windows you will need to set the '/FORCE:MULTIPLE' linker option
 * to avoid linker errors due to multiple operator new and delete symbols.
 */

#if defined(PROFILE_GPLATES)

/**
 * Starts profiling until the matching PROFILE_END is reached or
 * an exception is thrown or the function we're in returns early.
 *
 * @a name is a string of type "const char *".
 * All profiled sections of code with the same @a name have their time counts added.
 * The profile report will have an entry for each distinct @a name.
 *
 * Use @a profile_tag to match each PROFILE_BEGIN call with a PROFILE_END call.
 * @a profile_tag is an identifier and must use C++ naming rules.
 * @a profile_tag must be unique within the current scope (in other words
 * each PROFILE_BEGIN() call in the same scope/block must use a different
 * @a profile_tag). It does not need to be unique with respect identifiers
 * outside profiling (eg, local function variables near PROFILE_BEGIN()) since
 * @a profile_tag is prefixed before being used as an identifier.
 *
 * If an exception is thrown before the matching PROFILE_END() or the function
 * returns before the matching PROFILE_END() then PROFILE_END() is effectively
 * called for you when the exception is thrown or the function returns early.
 */
#define PROFILE_BEGIN(profile_tag, name) \
	/* Cache lookup of 'name' into static variable as it will never change. */ \
	static void *PROFILE_ANONYMOUS_VARIABLE(gplates_profile_cache) = \
			GPlatesUtils::profile_get_cache(name); \
	/* Starting profiling for profile "name". */ \
	GPlatesUtils::profile_begin(PROFILE_ANONYMOUS_VARIABLE(gplates_profile_cache)); \
	/* Make sure PROFILE_END() is called if it is not reached - */ \
	/* this can happen if an exception is thrown or function 'return's early. */ \
	GPlatesUtils::ProfileBlockEnd PROFILE_SCOPE_VARIABLE(profile_tag);

/**
 * Stops profiling the matching PROFILE_BEGIN call.
 *
 * Use @a profile_tag to match each PROFILE_BEGIN call with a PROFILE_END call.
 * @a profile_tag is an identifier and must use C++ naming rules.
 *
 * The total time spent between PROFILE_BEGIN and PROFILE_END is accumulated into
 * the profile node identified by the matching PROFILE_BEGIN with same @a profile_tag.
 */
#define PROFILE_END(profile_tag) \
	/* If PROFILE_END() has been reached then we can dismiss exception-safe/early-return. */ \
	PROFILE_SCOPE_VARIABLE(profile_tag).dismiss(); \
	/* Stop profiling that begin with matching PROFILE_BEGIN() call. */ \
	GPlatesUtils::profile_end();

/**
 * Starts profiling until the end of the current scope in which this PROFILE_BLOCK
 * call was made. The end of the scope is the same as when the destructor of a
 * local object would be called if it was created at the call site of PROFILE_BLOCK.
 * There is no need to call PROFILE_END as PROFILE_BLOCK calls both PROFILE_BEGIN
 * and PROFILE_END.
 */
#define PROFILE_BLOCK(name) \
	/* PROFILE_BEGIN() will call PROFILE_END() at end of scope if PROFILE_END() */ \
	/* is not called explicitly by user (which is what's happening here). */ \
	PROFILE_BEGIN(__LINE__/*Just need something unique*/, name);

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
 *
 * @a profile_tag is only used internally to match PROFILE_BEGIN and PROFILE_END calls.
 * @a profile_tag is an identifier and must use C++ naming rules.
 * @a profile_tag must be unique within the current scope (in other words
 * each PROFILE_CODE() call in the same scope/block must use a different
 * @a profile_tag). It does not need to be unique with respect identifiers
 * outside profiling (eg, local function variables near PROFILE_BEGIN()) since
 * @a profile_tag is prefixed before being used as an identifier.
 *
 * There is no need to call PROFILE_END as PROFILE_CODE calls both PROFILE_BEGIN
 * and PROFILE_END.
 *
 * For example...
 *
 *   PROFILE_CODE(foo_or_bar, const bool test = foo() || bar(););
 *
 * ...is equivalent to...
 *
 *   PROFILE_BEGIN(foo_or_bar, "const bool test = foo() || bar();");
 *   const bool test = foo() || bar();
 *   PROFILE_END(foo_or_bar);
 *
 */
#define PROFILE_CODE(profile_tag, code) \
	/* The PROFILE_CONCATENATE() is just to give the 'profile_tag' its own */ \
	/* namespace effectively - to avoid name collision with PROFILE_BEGIN() */ \
	PROFILE_BEGIN(PROFILE_CONCATENATE(code_, profile_tag), #code); \
	{ \
		code; \
	} \
	PROFILE_END(PROFILE_CONCATENATE(code_, profile_tag));

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

#define PROFILE_BEGIN(profile_tag, name)
#define PROFILE_END(profile_tag)
#define PROFILE_BLOCK(name)
#define PROFILE_FUNC()
#define PROFILE_CODE(profile_tag, code) code
#define PROFILE_REPORT_TO_OSTREAM(output_stream)
#define PROFILE_REPORT_TO_FILE(filename)

#endif // if defined(PROFILE_GPLATES) ... else ...


#define PROFILE_CONCATENATE_DIRECT(s1, s2)   s1##s2
#define PROFILE_CONCATENATE(s1, s2)          PROFILE_CONCATENATE_DIRECT(s1, s2)
#define PROFILE_SCOPE_VARIABLE(name)         PROFILE_CONCATENATE(gplates_profile_scope_, name)
#define PROFILE_ANONYMOUS_VARIABLE(name)     PROFILE_CONCATENATE(name, __LINE__)

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
		ProfileBlockEnd() :
			d_dismiss(false)
		{ }

		void
		dismiss()
		{
			d_dismiss = true;
		}

		~ProfileBlockEnd()
		{
			if (!d_dismiss)
			{
				// Since this is a destructor we cannot let any exceptions escape.
				// If one is thrown we just have to lump it and continue on.
				try
				{
					profile_end();
				}
				catch (...)
				{
				}
			}
		}

	private:
		bool d_dismiss;
	};
}

#endif // GPLATES_UTILS_PROFILE_H
