/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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
#ifndef GPLATES_UNIT_TEST_APPLICATIONSTATE_TEST_H
#define GPLATES_UNIT_TEST_APPLICATIONSTATE_TEST_H

#include <boost/test/unit_test.hpp>

#include "app-logic/ApplicationState.h"
#include "unit-test/GPlatesTestSuite.h"

//WARNING: 
//BEFORE YOU WRITE ANY UNIT TEST CODE FOR GPlatesAppLogic::ApplicationState,
//PLEASE READ THE FOLLOWING COMMENTS.

//If an ApplicationState object is initialized in GPlates Unit Test application,
//the application will crash when boost unit test framework destructs the test case.
//See the stack trace below.
//QSettings assumes there is always a valid QCoreApplication object. But for unit test application,
//the QCoreApplication has never been constructed properly.
/*
Program received signal SIGSEGV, Segmentation fault.
0x00007ffff36e9879 in QCoreApplication::applicationName() () from /usr/lib/libQtCore.so.4
(gdb) bt
#0  0x00007ffff36e9879 in QCoreApplication::applicationName() () from /usr/lib/libQtCore.so.4
#1  0x00007ffff36b1945 in QSettings::QSettings(QObject*) () from /usr/lib/libQtCore.so.4
#2  0x0000000000a7878d in ~UserPreferences (this=0x1221460, __in_chrg=<value optimised out>) at /home/mchin/gplates_src/trunk/src/app-logic/UserPreferences.cc:70
#3  0x00000000009f9666 in boost::checked_delete<GPlatesAppLogic::UserPreferences> (x=0x1221460) at /usr/include/boost/checked_delete.hpp:34
#4  0x00000000009f7cb5 in ~scoped_ptr (this=0x12226d8, __in_chrg=<value optimised out>) at /usr/include/boost/smart_ptr/scoped_ptr.hpp:80
#5  0x00000000009f4c46 in ~ApplicationState (this=0x12226a0, __in_chrg=<value optimised out>) at /home/mchin/gplates_src/trunk/src/app-logic/ApplicationState.cc:157
#6  0x00000000009ef0d6 in ~ApplicationStateTest (this=0x12226a0, __in_chrg=<value optimised out>) at /home/mchin/gplates_src/trunk/src/unit-test/ApplicationStateTest.h:36
#7  0x00000000009ef0f6 in boost::checked_delete<GPlatesUnitTest::ApplicationStateTest> (x=0x12226a0) at /usr/include/boost/checked_delete.hpp:34
#8  0x00000000009ef538 in boost::detail::sp_counted_impl_p<GPlatesUnitTest::ApplicationStateTest>::dispose (this=0x1225a30) at /usr/include/boost/smart_ptr/detail/sp_counted_impl.hpp:78
#9  0x00000000009a244c in boost::detail::sp_counted_base::release (this=0x1225a30) at /usr/include/boost/smart_ptr/detail/sp_counted_base_gcc_x86.hpp:145
#10 0x00000000009a24c5 in ~shared_count (this=0x12245c0, __in_chrg=<value optimised out>) at /usr/include/boost/smart_ptr/detail/shared_count.hpp:217
#11 0x00000000009eeb90 in ~shared_ptr (this=0x12245b8, __in_chrg=<value optimised out>) at /usr/include/boost/smart_ptr/shared_ptr.hpp:169
#12 0x00000000009eeca4 in ~user_tc_method_invoker (this=0x12245b8, __in_chrg=<value optimised out>) at /usr/include/boost/test/unit_test_suite_impl.hpp:235
#13 0x00000000009ef2f0 in ~callback0_impl_t (this=0x12245b0, __in_chrg=<value optimised out>) at /usr/include/boost/test/utils/callback.hpp:85
#14 0x00000000009ef365 in boost::checked_delete<boost::unit_test::ut_detail::callback0_impl_t<boost::unit_test::ut_detail::unused, boost::unit_test::ut_detail::user_tc_method_invoker<GPlatesUnitTest::ApplicationStateTest, GPlatesUnitTest::ApplicationStateTest> > > (x=0x12245b0) at /usr/include/boost/checked_delete.hpp:34
#15 0x00000000009ef4e0 in boost::detail::sp_counted_impl_p<boost::unit_test::ut_detail::callback0_impl_t<boost::unit_test::ut_detail::unused, boost::unit_test::ut_detail::user_tc_method_invoker<GPlatesUnitTest::ApplicationStateTest, GPlatesUnitTest::ApplicationStateTest> > >::dispose (this=0x1222fa0) at /usr/include/boost/smart_ptr/detail/sp_counted_impl.hpp:78
#16 0x0000000000cddf09 in boost::unit_test::test_case::~test_case() ()
#17 0x0000000000cb0d3b in boost::unit_test::framework_impl::~framework_impl() ()
#18 0x00007ffff18c7262 in __run_exit_handlers (status=201) at exit.c:78
#19 *__GI_exit (status=201) at exit.c:100
#20 0x00007ffff18acc54 in __libc_start_main (main=<value optimised out>, argc=<value optimised out>, ubp_av=<value optimised out>, init=<value optimised out>, fini=<value optimised out>,
    rtld_fini=<value optimised out>, stack_end=0x7fffffffe618) at libc-start.c:258
#21 0x00000000009a1a09 in _start ()
*/

namespace GPlatesUnitTest{

	class ApplicationStateTest
	{
	public:
		ApplicationStateTest()
		{ }

		void 
		test_get_model_interface();

	private:
		//GPlatesAppLogic::ApplicationState d_application_state;
	};

	
	class ApplicationStateTestSuite : 
		public GPlatesUnitTest::GPlatesTestSuite
	{
	public:
		ApplicationStateTestSuite(
				unsigned depth);

	protected:
		void 
		construct_maps();
	};
}
#endif //GPLATES_UNIT_TEST_APPLICATIONSTATE_TEST_H 

