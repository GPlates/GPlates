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

#include "GeoreferencingPage.h"

#include "EditAffineTransformGeoreferencingWidget.h"
#include "ImportRasterDialog.h"
#include "QtWidgetUtils.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


GPlatesQtWidgets::GeoreferencingPage::GeoreferencingPage(
		GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing,
		TimeDependentRasterSequence &raster_sequence,
		QWidget *parent_) :
	QWizardPage(parent_),
	d_georeferencing(georeferencing),
	d_georeferencing_widget(
			new EditAffineTransformGeoreferencingWidget(
				georeferencing,
				this)),
	d_raster_sequence(raster_sequence),
	d_last_seen_raster_width(0),
	d_last_seen_raster_height(0)
{
	setupUi(this);

	setTitle("Georeferencing");
	setSubTitle("Specify the location of the raster using lat-lon bounds or an affine transformation.");

	QtWidgetUtils::add_widget_to_placeholder(d_georeferencing_widget, georeferencing_placeholder_widget);
}


void
GPlatesQtWidgets::GeoreferencingPage::initializePage()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_raster_sequence.empty(), GPLATES_ASSERTION_SOURCE);

	// By the time we get to the georeferencing page, all of the rasters in the
	// sequence should have the same size, so let's just look at the first one.
	const TimeDependentRasterSequence::sequence_type &sequence = d_raster_sequence.get_sequence();
	if (sequence[0].width != d_last_seen_raster_width ||
			sequence[0].height != d_last_seen_raster_height)
	{
		d_last_seen_raster_width = sequence[0].width;
		d_last_seen_raster_height = sequence[0].height;

		d_georeferencing->reset_to_global_extents(
				d_last_seen_raster_width,
				d_last_seen_raster_height);
		d_georeferencing_widget->set_raster_size(
				d_last_seen_raster_width,
				d_last_seen_raster_height);
	}

	d_georeferencing_widget->refresh();
}

