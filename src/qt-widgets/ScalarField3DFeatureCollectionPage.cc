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

#include "ScalarField3DFeatureCollectionPage.h"


GPlatesQtWidgets::ScalarField3DFeatureCollectionPage::ScalarField3DFeatureCollectionPage(
		bool &save_after_finish,
		QWidget *parent_) :
	QWizardPage(parent_),
	d_save_after_finish(save_after_finish)
{
	setupUi(this);

	setTitle("Feature Collection");
	setSubTitle("Create a new feature collection to contain the scalar field.");

	feature_collections_listwidget->setCurrentRow(0);

	QObject::connect(
			save_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_save_checkbox_state_changed(int)));
}


bool
GPlatesQtWidgets::ScalarField3DFeatureCollectionPage::isComplete() const
{
	return true;
}


void
GPlatesQtWidgets::ScalarField3DFeatureCollectionPage::handle_save_checkbox_state_changed(
		int state)
{
	d_save_after_finish = (state == Qt::Checked);
}

