/* $Id$ */

/**
 * \file 
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

#include "ApplicationState.h"

#include "FeatureCollectionFileState.h"
#include "FeatureCollectionFileIO.h"
#include "ReconstructionActivationStrategy.h"


GPlatesAppLogic::ApplicationState::ApplicationState() :
	d_reconstruction_activation_strategy(
			new GPlatesAppLogic::ReconstructionActivationStrategy()),
	d_feature_collection_file_state(
			new GPlatesAppLogic::FeatureCollectionFileState()),
	d_feature_collection_file_io(
			new GPlatesAppLogic::FeatureCollectionFileIO(
					d_model, *d_feature_collection_file_state))
{
	// We have a strategy for activating reconstruction files.
	d_feature_collection_file_state->set_reconstruction_activation_strategy(
			d_reconstruction_activation_strategy.get());
}


GPlatesAppLogic::ApplicationState::~ApplicationState()
{
	// boost::scoped_ptr destructor needs complete type
}


GPlatesAppLogic::FeatureCollectionFileState &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_state()
{
	return *d_feature_collection_file_state;
}


GPlatesAppLogic::FeatureCollectionFileIO &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_io()
{
	return *d_feature_collection_file_io;
}
