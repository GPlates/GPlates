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
	const QString HELP_GRID_LINE_REGISTRATION_DIALOG_TITLE =
			QObject::tr("Grid line registration");
	const QString HELP_GRID_LINE_REGISTRATION_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Grid line registration involves placing the pixel <b>centres</b> of border pixels on the "
			"boundary of the exported region. The default is pixel registration which places the pixel <b>area</b> "
			"boxes of border pixels within the boundary, and hence the centres of border pixels are "
			"inside the exported region by half a pixel.</p>"
			"<p>The top latitude and left longitude refer to the top-left pixel <i>centre</i> for <i>grid line</i> "
			"registration and top-left <i>corner</i> of top-left pixel for <i>pixel</i> registration. "
			"Additionally the bottom latitude and right longitude refer to the bottom-right pixel <i>centre</i> for "
			"<i>grid line</i> registration and bottom-right <i>corner</i> of bottom-right pixel for <i>pixel</i> registration.</p>"
			"<p>Also note that the top latitude can be less than the bottom latitude (raster is flipped vertically), "
			"and the right longitude can be less than the left longitude (raster is flipped horizontally).</p>"
			"<p>This lat-lon georeferencing information is also saved to those formats supporting it. Note that some software "
			"reports the lat-lon extents of the exported raster in <i>pixel</i> registration (such as GDAL) while other software "
			"reports it in <i>grid line</i> registration (such as GMT). For example, a 1-degree global raster exported by GPlates "
			"with grid line registration is reported by GDAL as having pixel-registered lat-lon extents [-90.5, 90.5] and "
			"[-180.5, 180.5], and reported by GMT as having grid-line-registered lat-lon extents [-90, 90] and [-180, 180]. "
			"Both are correct since both place border pixel <i>centres</i> along the lat-lon extents [-90, 90] and [-180, 180].</p>"
			"</body></html>\n");

	/**
	 * Calculates the export raster dimensions from resolution and lat/lon extents.
	 */
	std::pair<unsigned int/*raster_width*/, unsigned int/*raster_height*/>
	get_export_raster_parameters(
			const double &top_extents,
			const double &bottom_extents,
			const double &left_extents,
			const double &right_extents,
			const double &raster_resolution_in_degrees,
			bool use_grid_line_registration)
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
		unsigned int raster_width = static_cast<unsigned int>(
				std::fabs(lon_extent / raster_resolution_in_degrees) + 0.5);
		unsigned int raster_height = static_cast<unsigned int>(
				std::fabs(lat_extent / raster_resolution_in_degrees) + 0.5);

		// Grid registration uses an extra row and column of pixels (data points) since data points are
		// *on* the grid lines instead of at the centre of grid cells (area between grid lines).
		// For example...
		//
		//   +-+-+    -----
		//   | | |    |+|+|
		//   +-+-+    -----
		//   | | |    |+|+|
		//   +-+-+    -----
		//
		// ...the '+' symbols are data points.
		// The left is grid line registration with 2x2 data points.
		// The right is pixel registration with 3x3 data points.
		//
		// However note that the grid resolution (spacing between data points) remains the same.
		//
		if (use_grid_line_registration)
		{
			raster_width += 1;
			raster_height += 1;
		}

		return std::make_pair(raster_width, raster_height);
	}
}


GPlatesQtWidgets::ExportRasterOptionsWidget::ExportRasterOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr &
				export_configuration) :
	ExportOptionsWidget(parent_),
	d_export_configuration(*export_configuration),
	d_help_grid_line_registration_dialog(
			new InformationDialog(
					HELP_GRID_LINE_REGISTRATION_DIALOG_TEXT,
					HELP_GRID_LINE_REGISTRATION_DIALOG_TITLE,
					// Seems needs to be parent instead of 'this' otherwise information dialog not centred properly..
					parent_))
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
	left_extents_spinbox->setMinimum(-360);
	left_extents_spinbox->setMaximum(360);
	right_extents_spinbox->setMinimum(-360);
	left_extents_spinbox->setMaximum(360);

	// Set grid line registration checkbox.
	grid_line_registration_checkbox->setChecked(export_configuration->use_grid_line_registration);

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
			push_button_help_grid_line_registration, SIGNAL(clicked()),
			d_help_grid_line_registration_dialog, SLOT(show()));

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
			grid_line_registration_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(handle_grid_line_registration_checkbox_state_changed(int)));

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
					d_export_configuration.resolution_in_degrees,
					d_export_configuration.use_grid_line_registration);

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
	if (d_export_configuration.lat_lon_extents.right > d_export_configuration.lat_lon_extents.left + 360)
	{
		QObject::disconnect(
				right_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_right_extents_spin_box_value_changed(double)));

		d_export_configuration.lat_lon_extents.right = d_export_configuration.lat_lon_extents.left + 360;
		right_extents_spinbox->setValue(d_export_configuration.lat_lon_extents.right);

		QObject::connect(
				right_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_right_extents_spin_box_value_changed(double)));
	}
	else if (d_export_configuration.lat_lon_extents.right < d_export_configuration.lat_lon_extents.left - 360)
	{
		QObject::disconnect(
				right_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_right_extents_spin_box_value_changed(double)));

		d_export_configuration.lat_lon_extents.right = d_export_configuration.lat_lon_extents.left - 360;
		right_extents_spinbox->setValue(d_export_configuration.lat_lon_extents.right);

		QObject::connect(
				right_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_right_extents_spin_box_value_changed(double)));
	}

	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_right_extents_spin_box_value_changed(
		double value)
{
	d_export_configuration.lat_lon_extents.right = value;

	// Make sure longitude extent cannot exceed 360 degrees (either direction).
	if (d_export_configuration.lat_lon_extents.left > d_export_configuration.lat_lon_extents.right + 360)
	{
		QObject::disconnect(
				left_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_left_extents_spin_box_value_changed(double)));

		d_export_configuration.lat_lon_extents.left = d_export_configuration.lat_lon_extents.right + 360;
		left_extents_spinbox->setValue(d_export_configuration.lat_lon_extents.left);

		QObject::connect(
				left_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_left_extents_spin_box_value_changed(double)));
	}
	else if (d_export_configuration.lat_lon_extents.left < d_export_configuration.lat_lon_extents.right - 360)
	{
		QObject::disconnect(
				left_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_left_extents_spin_box_value_changed(double)));

		d_export_configuration.lat_lon_extents.left = d_export_configuration.lat_lon_extents.right - 360;
		left_extents_spinbox->setValue(d_export_configuration.lat_lon_extents.left);

		QObject::connect(
				left_extents_spinbox, SIGNAL(valueChanged(double)),
				this, SLOT(react_left_extents_spin_box_value_changed(double)));
	}

	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::handle_grid_line_registration_checkbox_state_changed(
		int /*state*/)
{
	d_export_configuration.use_grid_line_registration = grid_line_registration_checkbox->isChecked();
	update_raster_dimensions();
}


void
GPlatesQtWidgets::ExportRasterOptionsWidget::react_use_global_extents_button_clicked()
{
	const double left = -180;
	const double right = 180;

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
