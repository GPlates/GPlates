/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "ExportImageResolutionOptionsWidget.h"

#include "GlobeAndMapWidget.h"
#include "ReconstructionViewWidget.h"
#include "SceneView.h"
#include "ViewportWindow.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesQtWidgets::ExportImageResolutionOptionsWidget::ExportImageResolutionOptionsWidget(
		QWidget *parent_,
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const GPlatesGui::ExportOptionsUtils::ExportImageResolutionOptions &default_export_image_resolution_options) :
	QWidget(parent_),
	d_export_animation_context(export_animation_context),
	d_export_image_resolution_options(default_export_image_resolution_options)
{
	setupUi(this);

	// Make signal/slot connections *before* we set values on the GUI controls.
	make_signal_slot_connections();

	//
	// Set the state of the export options widget according to the default export configuration passed to us.
	//

	// If the image dimensions have not been specified then use the current globe/map canvas dimensions.
	if (!d_export_image_resolution_options.image_size)
	{
		d_export_image_resolution_options.image_size =
				export_animation_context.viewport_window().reconstruction_view_widget()
						.globe_and_map_widget().get_active_view().get_viewport_size();
	}

	constrain_aspect_ratio_check_box->setChecked(d_export_image_resolution_options.constrain_aspect_ratio);

	width_spin_box->setValue(d_export_image_resolution_options.image_size->width());
	height_spin_box->setValue(d_export_image_resolution_options.image_size->height());
}


void
GPlatesQtWidgets::ExportImageResolutionOptionsWidget::make_signal_slot_connections()
{
	QObject::connect(
			width_spin_box,
			SIGNAL(valueChanged(int)),
			this,
			SLOT(react_width_spin_box_value_changed(int)));
	QObject::connect(
			height_spin_box,
			SIGNAL(valueChanged(int)),
			this,
			SLOT(react_height_spin_box_value_changed(int)));
	QObject::connect(
			constrain_aspect_ratio_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_constrain_aspect_ratio_check_box_clicked()));
	QObject::connect(
			use_main_window_dimensions_push_button, SIGNAL(clicked()),
			this, SLOT(handle_use_main_window_dimensions_push_button_clicked()));
}


void
GPlatesQtWidgets::ExportImageResolutionOptionsWidget::react_width_spin_box_value_changed(
		int width_value)
{
	// We should have ensured the image size is not none in the constructor.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_export_image_resolution_options.image_size,
			GPLATES_ASSERTION_SOURCE);

	// Change the height also if the aspect ratio is constrained.
	if (d_constrained_aspect_ratio)
	{
		// Scale the height according to the current aspect ratio.
		const int height_value = static_cast<int>(width_value / d_constrained_aspect_ratio.get() + 0.5);

		height_spin_box->setValue(height_value);
	}

	// Change the width (over-writing the old value) now that we've (potentially) constrained the height.
	d_export_image_resolution_options.image_size->setWidth(width_value);
}


void
GPlatesQtWidgets::ExportImageResolutionOptionsWidget::react_height_spin_box_value_changed(
		int value)
{
	// We should have ensured the image size is not none in the constructor.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_export_image_resolution_options.image_size,
			GPLATES_ASSERTION_SOURCE);

	d_export_image_resolution_options.image_size->setHeight(value);
}


void
GPlatesQtWidgets::ExportImageResolutionOptionsWidget::react_constrain_aspect_ratio_check_box_clicked()
{
	d_export_image_resolution_options.constrain_aspect_ratio = constrain_aspect_ratio_check_box->isChecked();

	if (d_export_image_resolution_options.constrain_aspect_ratio)
	{
		// Record the current aspect ratio.
		d_constrained_aspect_ratio =
				double(d_export_image_resolution_options.image_size->width()) /
						d_export_image_resolution_options.image_size->height();
	}
	else
	{
		d_constrained_aspect_ratio = boost::none;
	}

	// Disable the height spin box if the aspect ratio is being constrained.
	height_spin_box->setDisabled(d_export_image_resolution_options.constrain_aspect_ratio);
}


void
GPlatesQtWidgets::ExportImageResolutionOptionsWidget::handle_use_main_window_dimensions_push_button_clicked()
{
	QSize main_window_size =
			d_export_animation_context.viewport_window().reconstruction_view_widget()
					.globe_and_map_widget().get_active_view().get_viewport_size();

	// Constrain the aspect ratio if necessary.
	if (d_constrained_aspect_ratio)
	{
		// Here we actually change the aspect ratio to be that of the main window.
		d_constrained_aspect_ratio = double(main_window_size.width()) / main_window_size.height();
	}

	width_spin_box->setValue(main_window_size.width());
	height_spin_box->setValue(main_window_size.height());
}
