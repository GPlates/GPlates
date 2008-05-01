/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "EditFeatureGeometriesWidget.h"

#include "feature-visitors/EditFeatureGeometriesWidgetPopulator.h"
#include "utils/UnicodeStringUtils.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesQtWidgets::EditFeatureGeometriesWidget::EditFeatureGeometriesWidget(
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::FeatureFocus &feature_focus,
		QWidget *parent_):
	QWidget(parent_),
	d_view_state_ptr(&view_state_),
	d_feature_focus_ptr(&feature_focus)
{
	setupUi(this);
	reset();
	
	QObject::connect(d_view_state_ptr, SIGNAL(reconstruction_time_changed(double)),
			this, SLOT(refresh_display()));
}


void
GPlatesQtWidgets::EditFeatureGeometriesWidget::reset()
{
	tree_geometry->clear();
}


void
GPlatesQtWidgets::EditFeatureGeometriesWidget::refresh_display()
{
	reset();
	
	GPlatesFeatureVisitors::EditFeatureGeometriesWidgetPopulator populator(
			d_view_state_ptr->reconstruction(), *tree_geometry);
	populator.visit_feature_handle(*d_feature_ref);
	
	double time = d_view_state_ptr->reconstruction_time();
	unsigned long root = d_view_state_ptr->reconstruction_root();
	
	lineedit_root_plateid->setText(tr("%1").arg(root));
	lineedit_reconstruction_time->setText(tr("%L1").arg(time));
	tree_geometry->resizeColumnToContents(0);
	tree_geometry->resizeColumnToContents(1);
	tree_geometry->resizeColumnToContents(2);
}


void
GPlatesQtWidgets::EditFeatureGeometriesWidget::edit_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	d_feature_ref = feature_ref;
	refresh_display();
}



