/* $Id$ */

/**
* \file
* $Revision$
* $Date$
*
* Copyright (C) 2009, 2010 The University of Sydney, Australia
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
#include <gtest/gtest.h>

#include <QApplication>
#include <QtGlobal>

#include "maths/MathsUtils.h"


//
// GoogleTest entry point for the GPlates unit tests.
//
// To list the unit tests you can run 'gplates-unit-test --gtest_list_tests'.
// To run specific unit tests you can run 'gplates-unit-test --gtest_filter=RealTest.*', for example.
// CTest registers each test case individually (see 'gtest_discover_tests()' in 'src/CMakeLists.txt'),
// so 'ctest -R RealTest' achieves the same from the build directory.
//
int main(int argc, char* argv[])
{
	// Let GoogleTest consume its '--gtest_*' arguments before Qt sees argv.
	::testing::InitGoogleTest(&argc, argv);

	// Test discovery: 'gtest_discover_tests()' runs this executable with '--gtest_list_tests'
	// at build time, possibly on a headless machine. Listing the tests requires no Qt
	// application, resources or display, so return before any of that is set up.
	if (::testing::GTEST_FLAG(list_tests))
	{
		return RUN_ALL_TESTS();  // Prints the test list and exits (runs nothing).
	}

	// No test needs a real windowing system; default to Qt's 'offscreen' platform so the tests
	// run identically on headless CI machines and developer machines.
	// An explicitly set QT_QPA_PLATFORM environment variable still wins.
	if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
	{
		qputenv("QT_QPA_PLATFORM", "offscreen");
	}

	// A QApplication (rather than QCoreApplication) so that tests can exercise QSettings-dependent
	// code such as GPlatesAppLogic::ApplicationState/UserPreferences (historically untestable
	// because no application object existed) and, in future, widget-level code.
	// This costs nothing under the offscreen platform.
	QApplication application(argc, argv);

	// A distinct application name so that QSettings used by tests never touch a developer's
	// real GPlates preferences.
	QCoreApplication::setOrganizationName("GPlates");
	QCoreApplication::setApplicationName("gplates-unit-test");

	// Initialise Qt resources that exist in the static 'qt-resources' library.
	Q_INIT_RESOURCE(opengl);
	Q_INIT_RESOURCE(python);
	Q_INIT_RESOURCE(gpgim);
	Q_INIT_RESOURCE(qt_widgets);

	// Sanity check: Proceed only if we have access to infinity and NaN.
	// This should pass on all systems that we support.
	GPlatesMaths::assert_has_infinity_and_nan();

	return RUN_ALL_TESTS();
}
