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
#include <gtest/gtest.h>

#include "app-logic/ApplicationState.h"

#include "model/FeatureCollectionHandle.h"
#include "model/Model.h"
#include "model/ModelInterface.h"


// Constructing (and, just as importantly, destroying) an ApplicationState requires a
// QCoreApplication - its UserPreferences member uses QSettings - which the test main()
// provides. Historically no application object existed in the unit-test executable, so
// ApplicationState was untestable (its destructor segfaulted); this test also serves to
// verify that that constraint is gone.
TEST(ApplicationStateTest, construct_and_get_model_interface)
{
	GPlatesAppLogic::ApplicationState application_state;

	GPlatesModel::ModelInterface model = application_state.get_model_interface();

	// The model's feature store root should exist and be empty.
	ASSERT_TRUE(model->root().is_valid());
	EXPECT_EQ(model->root()->size(), 0u);

	// The model should be usable: create a feature collection in the feature store root.
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			GPlatesModel::FeatureCollectionHandle::create(model->root());
	EXPECT_TRUE(feature_collection.is_valid());
	EXPECT_EQ(model->root()->size(), 1u);

	// ~ApplicationState runs at end of scope - the historical crash site (QSettings in
	// ~UserPreferences) - so simply completing this test without crashing is part of the test.
}
