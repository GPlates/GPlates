/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include "ViewFeatureGeometriesWidget.h"

#include "app-logic/ApplicationState.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"
#include "presentation/ViewState.h"
#include "utils/UnicodeStringUtils.h"


GPlatesQtWidgets::ViewFeatureGeometriesWidget::ViewFeatureGeometriesWidget(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_feature_focus_ptr(&view_state_.get_feature_focus())
{
	setupUi(this);
	reset();
	
	QObject::connect(d_application_state_ptr,
			SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(refresh_display()));
}


void
GPlatesQtWidgets::ViewFeatureGeometriesWidget::reset()
{
	tree_geometry->clear();
}


void
GPlatesQtWidgets::ViewFeatureGeometriesWidget::refresh_display()
{
	reset();
	if ( ! d_feature_ref.is_valid()) {
		// Always check your weak-refs, even if they should be valid because
		// FeaturePropertiesDialog promised it'd check them, because in this
		// one case we can also get updated directly when the reconstruction
		// time changes.
		return;
	}

	//PROFILE_BEGIN(populate, "ViewFeatureGeometriesWidgetPopulator");

	GPlatesFeatureVisitors::ViewFeatureGeometriesWidgetPopulator populator(
			d_application_state_ptr->get_current_reconstruction(), *tree_geometry);
	populator.populate(d_feature_ref, d_focused_rg);
	
	double time = d_application_state_ptr->get_current_reconstruction_time();
	unsigned long root = d_application_state_ptr->get_current_anchored_plate_id();

	//PROFILE_END(populate);
	
	//PROFILE_BLOCK("resize columns for ViewFeatureGeometriesWidget");

	lineedit_root_plateid->setText(tr("%1").arg(root));
	lineedit_reconstruction_time->setText(tr("%L1").arg(time));
	tree_geometry->resizeColumnToContents(0);
	tree_geometry->resizeColumnToContents(1);
	tree_geometry->resizeColumnToContents(2);
}


void
GPlatesQtWidgets::ViewFeatureGeometriesWidget::edit_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_rg)
{
	d_feature_ref = feature_ref;
	d_focused_rg = focused_rg;

	refresh_display();
}



