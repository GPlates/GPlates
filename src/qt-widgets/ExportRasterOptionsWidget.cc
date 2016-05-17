/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <cmath>
#include <utility> // std::pair

#include "ExportRasterOptionsWidget.h"

#include "QtWidgetUtils.h"

#include "maths/MathsUtils.h"


namespace
{
	/**
	 * Calculates the export raster dimensions from resolution and lat/lon extents.
	 */
	std::pair<unsigned int/*width*/, unsigned int/*height*/>
	get_export_raster_parameters(
			const double &top_extents,
			const double &bottom_extents,
			const double &left_extents,
			const double &right_extents,
			const double &raster_resolution_in_degrees)
	{
		// Avoid divide by zero.
		if (GPlatesMaths::are_almost_exactly_equal(raster_resolution_in_degrees, 0.0))
		{
			return std::make_pair(0, 0);
		}

		const double lat_extent = top_extents - bottom_extents;
		const double lon_extent = right_extents - left_extents;

		// Avoid zero width or height exported raster.
		if (GPlatesMaths::are_almost_exactly_equal(lat_extent, 0.0) ||
			GPlatesMaths::are_almost_exactly_equal(lon_extent, 0.0))
		{
			return std::make_pair(0, 0);
		}

		// We use absolute value in case user swapped top/bottom or left/right to flip exported raster.
		// We also round to the nearest integer.
		unsigned int width = static_cast<unsigned int>(
				std::fabs(lon_extent / raster_resolution_in_degrees) + 0.5);
		unsigned int height = static_cast<unsigned int>(
				std::fabs(lat_extent / raster_resolution_in_degrees) + 0.5);

		return std::make_pair(width, height);
	}
}


GPlatesQtWidgets::ExportRasterOptionsWidget::ExportRasterOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr &
				export_configuration) :
	ExportOptionsWidget(parent_),
	d_export_configuration(*export_configuration)
{
	setupUi(this);

	//
	// Set the state of the export options widget according to the default export configuration passed to us.
	//

	resolution_spin_box->setValue(export_configuration->resolution_in_degrees);

	top_extents_spinbox->setValue(export_configuration->lat_lon_extents.top);
	bottom_extents_spinbox->setValue(export_configuration->lat_lon_extents.bottom);
	left_extents_spinbox->setValue(export_configuration->lat_lon_extents.left);
	right_extents_spinbox->setValue(export_configuration->lat_lon_extents.right);

	// Set the min/max longitude values.
	left_extents_spinbox->setMinimum(export_configuration->lat_lon_extents.right - 360);
	left_extents_spinbox->setMaximum(export_configuration->lat_lon_extents.right + 360);
	right_extents_spinbox->setMinimum(export_configuration->lat_lon_extents.left - 360);
	left_extents_spinbox->setMaximum(export_configuration->lat_lon_extents.left + 360);

	// If raster compression is an option then initialise it, otherwise hide it.
	if (d_export_configuration.compress)
	{
		enable_compression_checkbox->setChecked(d_export_configuration.compress.get());
	}
	else
	{
		compression_group_box->hide();
	}

	update_raster_dimensions();

	make_signal_slot_connections();
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportRasterOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	d_export_configuration.set_filename_template(filename_template);

	return GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr(
			new GPlatesGui::ExportRasterAnimationStrategy::Configuration(
					d_export_configuration));
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::make_signal_slot_connections()
{
	QObject::connect(
			resolution_spin_box,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_resolution_spin_box_value_changed(double)));

	QObject::connect(
			top_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_top_extents_spin_box_value_changed(double)));
	QObject::connect(
			bottom_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_bottom_extents_spin_box_value_changed(double)));
	QObject::connect(
			left_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_left_extents_spin_box_value_changed(double)));
	QObject::connect(
			right_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_right_extents_spin_box_value_changed(double)));

	QObject::connect(
			use_global_extents_button,
			SIGNAL(clicked()),
			this,
			SLOT(react_use_global_extents_button_clicked()));

	QObject::connect(
			enable_compression_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_enable_compression_check_box_clicked()));
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::update_raster_dimensions()
{
	const std::pair<unsigned int/*width*/, unsigned int/*height*/> dimensions =
			get_export_raster_parameters(
					d_export_configuration.lat_lon_extents.top,
					d_export_configuration.lat_lon_extents.bottom,
					d_export_configuration.lat_lon_extents.left,
					d_export_configuration.lat_lon_extents.right,
					d_export_configuration.resolution_in_degrees);

	QString width_string;
	width_string.setNum(dimensions.first);
	width_line_edit->setText(width_string);

	QString height_string;
	height_string.setNum(dimensions.second);
	height_line_edit->setText(height_string);
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_resolution_spin_box_value_changed(
		double value)
{
	d_export_configuration.resolution_in_degrees = value;
	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_top_extents_spin_box_value_changed(
		double value)
{
	d_export_configuration.lat_lon_extents.top = value;
	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_bottom_extents_spin_box_value_changed(
		double value)
{
	d_export_configuration.lat_lon_extents.bottom = value;
	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_left_extents_spin_box_value_changed(
		double value)
{
	d_export_configuration.lat_lon_extents.left = value;

	// Make sure longitude extent cannot exceed 360 degrees (either direction).
	right_extents_spinbox->setMinimum(d_export_configuration.lat_lon_extents.left - 360);
	right_extents_spinbox->setMaximum(d_export_configuration.lat_lon_extents.left + 360);

	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_right_extents_spin_box_value_changed(
		double value)
{
	d_export_configuration.lat_lon_extents.right = value;

	// Make sure longitude extent cannot exceed 360 degrees (either direction).
	left_extents_spinbox->setMinimum(d_export_configuration.lat_lon_extents.right - 360);
	left_extents_spinbox->setMaximum(d_export_configuration.lat_lon_extents.right + 360);

	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_use_global_extents_button_clicked()
{
	const double left = -180;
	const double right = 180;

	// Reset the min/max longitude values.
	left_extents_spinbox->setMinimum(right - 360);
	left_extents_spinbox->setMaximum(right + 360);
	right_extents_spinbox->setMinimum(left - 360);
	left_extents_spinbox->setMaximum(left + 360);

	top_extents_spinbox->setValue(90);
	bottom_extents_spinbox->setValue(-90);
	left_extents_spinbox->setValue(left);
	right_extents_spinbox->setValue(right);
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_enable_compression_check_box_clicked()
{
	d_export_configuration.compress = enable_compression_checkbox->isChecked();
}
