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

#include <QObject>

#include "EditAffineTransformGeoreferencingWidget.h"

#include "maths/MathsUtils.h"

#include "presentation/ViewState.h"


namespace
{
	const QString HELP_GRID_LINE_REGISTRATION_DIALOG_TITLE =
			QObject::tr("Time to start topology reconstruction from");
	const QString HELP_GRID_LINE_REGISTRATION_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Grid line registration involves placing the pixel <b>centres</b> of border pixels at the "
			"georeferencing extents. The default is pixel registration which places the pixel <b>area</b> "
			"boundaries of border pixels at the georeferencing extents, and hence the centres of "
			"border pixels are essentially moved inside the georeferencing extents by half a pixel.</p>"
			"<p>This applies to both the simpler lat-lon extents and the more general affine transform:</p>"
			"<ul>"
			"<li><b>Lat-lon extents</b>: The top latitude and left longitude refer to the top-left pixel "
			"<i>centre</i> for grid line registration and top-left <i>corner</i> of top-left pixel for pixel registration. "
			"Additionally the bottom latitude and right longitude refer to the bottom-right pixel <i>centre</i> for "
			"grid line registration and bottom-right <i>corner</i> of bottom-right pixel for pixel registration. "
			"Also note that the top latitude can be less than the bottom latitude (raster is flipped vertically), "
			"and the right longitude can be less than the left longitude (raster is flipped horizontally).</li>"
			"<li><b>Affine transform</b>: The top-left x and y coordinates refer to the top-left pixel "
			"<i>centre</i> for grid line registration and top-left <i>corner</i> of top-left pixel for pixel registration. "
			"The remaining affine parameters determine the bottom-right pixel <i>centre</i> for grid line registration and "
			"bottom-right <i>corner</i> of bottom-right pixel for pixel registration. For example, if the raster width and height "
			"are W and H pixels then the latitude of the bottom-right pixel <i>centre</i> for grid line registration is "
			"<i>top_left_y_coordinate + (W - 1) * y_component_of_pixel_width + (H - 1) * y_component_of_pixel_height</i>, and "
			"the bottom-right <i>corner</i> of bottom-right pixel for pixel registration is "
			"<i>top_left_y_coordinate + W * y_component_of_pixel_width + H * y_component_of_pixel_height</i>, "
			"noting that pixel registration has one extra pixel increment compared to grid line registration due to "
			"extents covering pixel areas instead of pixel centres.</li>"
			"</ul>"
			"<p>Note that it helps to think of top/bottom/left/right as references to the raster image <i>before</i> it is "
			"georeferenced onto the globe since georeferencing can scale and invert the image, and skew in the case of an "
			"affine transform.</p>"
			"</body></html>\n");

	bool
	any_changed(
			QDoubleSpinBox **spinboxes,
			double *last_known_values,
			unsigned int length)
	{
		for (unsigned int i = 0; i != length; ++i)
		{
			if (!GPlatesMaths::are_almost_exactly_equal((*spinboxes)->value(), *last_known_values))
			{
				return true;
			}
			++spinboxes;
			++last_known_values;
		}
		return false;
	}
}


GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::EditAffineTransformGeoreferencingWidget(
		GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing,
		QWidget *parent_) :
	QWidget(parent_),
	d_georeferencing(georeferencing),
	d_raster_width(0),
	d_raster_height(0),
	d_help_grid_line_registration_dialog(
			new InformationDialog(
					HELP_GRID_LINE_REGISTRATION_DIALOG_TEXT,
					HELP_GRID_LINE_REGISTRATION_DIALOG_TITLE,
					this))
{
	setupUi(this);

	// Store pointers to the spinboxes in arrays, for ease of access.
	d_extents_spinboxes[0] = extents_spinbox_0;
	d_extents_spinboxes[1] = extents_spinbox_1;
	d_extents_spinboxes[2] = extents_spinbox_2;
	d_extents_spinboxes[3] = extents_spinbox_3;

	d_affine_transform_spinboxes[0] = affine_transform_spinbox_0;
	d_affine_transform_spinboxes[1] = affine_transform_spinbox_1;
	d_affine_transform_spinboxes[2] = affine_transform_spinbox_2;
	d_affine_transform_spinboxes[3] = affine_transform_spinbox_3;
	d_affine_transform_spinboxes[4] = affine_transform_spinbox_4;
	d_affine_transform_spinboxes[5] = affine_transform_spinbox_5;

	// Store initial values.
	for (unsigned int i = 0; i != lat_lon_extents_type::NUM_COMPONENTS; ++i)
	{
		d_last_known_extents_values[i] = d_extents_spinboxes[i]->value();
	}
	for (unsigned int i = 0; i != affine_transform_type::NUM_COMPONENTS; ++i)
	{
		d_last_known_affine_transform_values[i] = d_affine_transform_spinboxes[i]->value();
	}

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::make_signal_slot_connections()
{
	QObject::connect(
			push_button_help_grid_line_registration, SIGNAL(clicked()),
			d_help_grid_line_registration_dialog, SLOT(show()));

	QObject::connect(
			advanced_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_advanced_checkbox_state_changed(int)));

	// Extents spinboxes.
	for (unsigned int i = 0; i != lat_lon_extents_type::NUM_COMPONENTS; ++i)
	{
		QObject::connect(
				d_extents_spinboxes[i],
				SIGNAL(editingFinished()),
				this,
				SLOT(update_extents_if_necessary()));
	}

	// Affine transform spinboxes.
	for (unsigned int i = 0; i != affine_transform_type::NUM_COMPONENTS; ++i)
	{
		QObject::connect(
				d_affine_transform_spinboxes[i],
				SIGNAL(editingFinished()),
				this,
				SLOT(update_affine_transform_if_necessary()));
	}

	// Buttons.
	QObject::connect(
			use_global_extents_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_global_extents_button_clicked()));
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::handle_advanced_checkbox_state_changed(
		int state)
{
	switch (state)
	{
		case Qt::Unchecked:
			populate_lat_lon_extents_spinboxes(
					d_georeferencing->get_lat_lon_extents(d_raster_width, d_raster_height));
			main_stackedwidget->setCurrentIndex(0);
			break;

		case Qt::Checked:
			populate_affine_transform_spinboxes(
					d_georeferencing->get_parameters());
			main_stackedwidget->setCurrentIndex(1);
			break;
	}
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::update_extents_if_necessary()
{
	using namespace GPlatesPropertyValues;

	if (any_changed(
			d_extents_spinboxes,
			d_last_known_extents_values,
			lat_lon_extents_type::NUM_COMPONENTS))
	{
		// Change the underlying data source.
		lat_lon_extents_type new_extents;
		for (unsigned int i = 0; i != lat_lon_extents_type::NUM_COMPONENTS; ++i)
		{
			new_extents.components[i] = d_extents_spinboxes[i]->value();
		}
		d_georeferencing->set_lat_lon_extents(
				new_extents,
				d_raster_width,
				d_raster_height);

		Q_EMIT georeferencing_changed();
	}
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::update_affine_transform_if_necessary()
{
	if (any_changed(
			d_affine_transform_spinboxes,
			d_last_known_affine_transform_values,
			affine_transform_type::NUM_COMPONENTS))
	{
		// Change the underlying data source.
		affine_transform_type new_parameters;
		for (unsigned int i = 0; i != affine_transform_type::NUM_COMPONENTS; ++i)
		{
			new_parameters.components[i] = d_affine_transform_spinboxes[i]->value();
		}
		d_georeferencing->set_parameters(new_parameters);

		Q_EMIT georeferencing_changed();

		// Read it back into the spinboxes (there's no guarantee that what we put in
		// is what we get back out).
		populate_affine_transform_spinboxes(d_georeferencing->get_parameters());
	}
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::handle_use_global_extents_button_clicked()
{
	d_georeferencing->reset_to_global_extents(d_raster_width, d_raster_height);
	Q_EMIT georeferencing_changed();
	refresh();
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::refresh()
{
	if (main_stackedwidget->currentIndex() == 0 /* lat-lon extents page */)
	{
		populate_lat_lon_extents_spinboxes(
				d_georeferencing->get_lat_lon_extents(d_raster_width, d_raster_height));
	}
	else
	{
		populate_affine_transform_spinboxes(
				d_georeferencing->get_parameters());
	}
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::populate_lat_lon_extents_spinboxes(
		const boost::optional<lat_lon_extents_type> &extents)
{
	if (extents)
	{
		for (unsigned int i = 0; i != lat_lon_extents_type::NUM_COMPONENTS; ++i)
		{
			d_extents_spinboxes[i]->setValue(extents->components[i]);
			d_last_known_extents_values[i] = extents->components[i];
		}
		extents_widget->show();
		cannot_convert_to_extents_label->hide();
	}
	else
	{
		extents_widget->hide();
		cannot_convert_to_extents_label->show();
	}
}


void
GPlatesQtWidgets::EditAffineTransformGeoreferencingWidget::populate_affine_transform_spinboxes(
		const affine_transform_type &parameters)
{
	for (unsigned int i = 0; i != affine_transform_type::NUM_COMPONENTS; ++i)
	{
		d_affine_transform_spinboxes[i]->setValue(parameters.components[i]);
		d_last_known_affine_transform_values[i] = parameters.components[i];
	}
}

