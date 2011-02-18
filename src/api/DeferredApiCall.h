/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_API_DEFERREDAPICALL_H
#define GPLATES_API_DEFERREDAPICALL_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/at.hpp>


namespace GPlatesApi
{
	namespace DeferredApiCall
	{
		/**
		 * A tag for use as a template parameter to @a ArgReferenceWrappings that
		 * indicates that the corresponding function parameter should not be given a
		 * reference wrapper when bound with the function for later execution.
		 */
		struct no_wrap {  };

		/**
		 * A tag for use as a template parameter to @a ArgReferenceWrappings that
		 * indicates that the corresponding function parameter should be given a
		 * non-const reference wrapper when bound with the function for later execution.
		 */
		struct ref {  };

		/**
		 * A tag for use as a template parameter to @a ArgReferenceWrappings that
		 * indicates that the corresponding function parameter should be given a
		 * const reference wrapper when bound with the function for later execution.
		 */
		struct cref {  };

		/**
		 * For use with the macro @a GPLATES_DEFERRED_API_CALL as explained below.
		 * The template parameters should be one of @a no_wrap, @a ref or @a cref.
		 */
		template<
			typename A0 = no_wrap,
			typename A1 = no_wrap,
			typename A2 = no_wrap,
			typename A3 = no_wrap,
			typename A4 = no_wrap,
			typename A5 = no_wrap,
			typename A6 = no_wrap,
			typename A7 = no_wrap,
			typename A8 = no_wrap,
			typename A9 = no_wrap
		>
		struct ArgReferenceWrappings
		{
		private:

			typedef boost::mpl::vector< A0, A1, A2, A3, A4, A5, A6, A7, A8, A9 > wrappings;

		public:

			template<int I>
			struct get
			{
				typedef typename boost::mpl::at< wrappings, boost::mpl::int_<I> >::type type;
			};
		};
	}
}


// This is intentionally down here, not at the top of the file. I've used a
// separate header file so hide all the gory details so that what is the public
// interface is clear. There is code in "DeferredApiCallImpl.h" that requires
// complete type for the no_wrap, ref and cref tags.
#include "DeferredApiCallImpl.h"


/**
 * Returns a pointer to a function that, if called from a thread other than the
 * main GUI thread, will call the function pointed to by @a F on the main GUI thread,
 * by posting a @a GPlatesUtils::DeferredCallEvent to the @a qApp singleton.
 *
 * The typical use of this macro is where @a F is a pointer to a function that,
 * directly or indirectly, calls methods on a @a QWidget, and you wish to expose
 * @a F in the Python API. @a QWidget and all its subclases are not re-entrant,
 * and must only be used on the main GUI thread: see
 * http://doc.troll.no/master-snapshot/threads-qobject.html
 *
 * The parameter @a A is an instance of @a ArgReferenceWrappings with up to ten
 * optional parameters filled in with @a DeferredApiCall::no_wrap,
 * @a DeferredApiCall::ref or @a DeferredApiCall::cref. The i-th template
 * parameter indicates whether the i-th parameter of the function should be
 * wrapped with a @a boost::reference_wrapper when binding it with the function
 * for later execution. It is permissible to provide fewer template arguments to
 * @a ArgReferenceWrappings than there are parameters to the function being
 * wrapped; in this case, the missing parameters default to @a no_wrap.
 *
 * For example, @a status_message is a member function of ViewportWindow that
 * interacts with @a QWidget objects. It could be exposed as follows:
 *
 *		class_<GPlatesQtWidgets::ViewportWindow, ... >( ... )
 *			.def("set_status_message",
 *					GPLATES_DEFERRED_API_CALL(&GPlatesQtWidgets::ViewportWindow::status_message, ArgReferenceWrappings<ref>()))
 *
 * The first argument is the hidden 'this' pointer. Because ViewportWindow is
 * noncopyable, it would be a Bad Thing if a copy were made to be bound with the
 * function. The above effectively causes the following binding to be made:
 *
 *		boost::bind(
 *				&GPlatesQtWidgets::ViewportWindow::status_message,
 *				boost::ref(a0),
 *				a1)
 *
 * where @a a0 and @a a1 are a ViewportWindow instance and a string respectively.
 *
 * If you wish to specify the reference wrapping for more than 1 parameter, you
 * will need to use an additional pair of parentheses because any commas in the
 * template parameter list would be parsed as belonging to the macro call
 * otherwise. For example:
 *
 *		GPLATES_DEFERRED_API_CALL(&GPlatesQtWidgets::ViewportWindow::status_message, (ArgReferenceWrappings<ref, no_wrap>())))
 *
 * Without the extra parentheses, the compiler will complain that you've given
 * three arguments to the macro, which takes two arguments.
 *
 * @a GPLATES_DEFERRED_API_CALL may also be used with free functions, as follows:
 *
 *		void status_message(GPlatesQtWidgets::ViewportWindow &, const boost::python::object &);
 *		class_<GPlatesQtWidgets::ViewportWindow, ... >( ... )
 *			.def("set_status_message",
 *					GPLATES_DEFERRED_API_CALL(&status_message, ArgReferenceWrappings<ref>()))
 *
 * Note that @a F must have ten or fewer parameters, including the hidden 'this' parameter.
 *
 * Boost.Python seems to have a problem exposing functions that return a reference
 * to a built-in type, but hopefully such functions should be few.
 */
#define GPLATES_DEFERRED_API_CALL(F, A) GPlatesApi::DeferredApiCallImpl::make_wrapper((F)).wrap<F>((A))

#endif  // GPLATES_API_DEFERREDAPICALL_H
