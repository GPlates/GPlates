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

#include <vector>
#include <QDebug>
#include <QString>

#include "ModifyLoadedFeatureCollectionsFilter.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ClassifyFeatureCollection.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "model/ModelUtils.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlPlateId.h"

#include "qt-widgets/AssignReconstructionPlateIdsDialog.h"

#include "utils/UnicodeStringUtils.h"


GPlatesGui::ModifyLoadedFeatureCollectionsFilter::ModifyLoadedFeatureCollectionsFilter(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QWidget *assign_plate_ids_dialog_parent) :
	d_assign_recon_plate_ids_dialog_ptr(
			new GPlatesQtWidgets::AssignReconstructionPlateIdsDialog(
					application_state,
					view_state,
					assign_plate_ids_dialog_parent))
{
}


void
GPlatesGui::ModifyLoadedFeatureCollectionsFilter::modify_loaded_files(
		const file_seq_type &loaded_files)
{
	// Ask the user which feature collections they want to assign plate ids to.
	// It's not an error or warning if there are no plate boundaries to
	// assign plate ids with because the user is not choosing to do this -
	// it's being done every time new files are loaded.
	d_assign_recon_plate_ids_dialog_ptr->assign_plate_ids_to_newly_loaded_feature_collections(
			loaded_files,
			false/*pop_up_message_box_if_no_plate_boundaries*/);
}
