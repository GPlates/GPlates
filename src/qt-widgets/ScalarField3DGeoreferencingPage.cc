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

#include <QDebug>

#include "ScalarField3DGeoreferencingPage.h"

#include "EditAffineTransformGeoreferencingWidget.h"
#include "ImportScalarField3DDialog.h"
#include "QtWidgetUtils.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


GPlatesQtWidgets::ScalarField3DGeoreferencingPage::ScalarField3DGeoreferencingPage(
		GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing,
		unsigned int &raster_width,
		unsigned int &raster_height,
		ScalarField3DDepthLayersSequence &depth_layers_sequence,
		QWidget *parent_) :
	QWizardPage(parent_),
	d_georeferencing(georeferencing),
	d_georeferencing_widget(
			new EditAffineTransformGeoreferencingWidget(
				georeferencing,
				this)),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_depth_layers_sequence(depth_layers_sequence),
	d_last_seen_raster_width(0),
	d_last_seen_raster_height(0)
{
	setupUi(this);

	setTitle("Georeferencing");
	setSubTitle("Specify the extent of the (depth layered) rasters using lat-lon bounds or an affine transformation.");

	QtWidgetUtils::add_widget_to_placeholder(d_georeferencing_widget, georeferencing_placeholder_widget);
}


void
GPlatesQtWidgets::ScalarField3DGeoreferencingPage::initializePage()
{
	if (d_raster_width != d_last_seen_raster_width ||
			d_raster_height != d_last_seen_raster_height)
	{
		d_last_seen_raster_width = d_raster_width;
		d_last_seen_raster_height = d_raster_height;

		d_georeferencing_widget->reset(
				d_last_seen_raster_width,
				d_last_seen_raster_height);
	}
}

