/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "ColourScaleDialog.h"

#include "ColourScaleWidget.h"
#include "QtWidgetUtils.h"


GPlatesQtWidgets::ColourScaleDialog::ColourScaleDialog(
		QWidget *parent_) :
	QDialog(parent_, Qt::Tool),
	d_colour_scale_widget(new ColourScaleWidget(this))
{
	setupUi(this);

	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_scale_widget,
			colour_scale_widget_placeholder);
}


void
GPlatesQtWidgets::ColourScaleDialog::populate(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
}

