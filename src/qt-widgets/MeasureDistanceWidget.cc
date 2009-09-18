/* $Id$ */

/**
 * @file 
 * Contains implementation of MeasureDistanceWidget
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <QString>

#include "MeasureDistanceWidget.h"
#include "maths/LatLonPointConversions.h"
#include "maths/types.h"
#include "canvas-tools/MeasureDistanceState.h"

namespace
{
	void
	set_lineedit_text(
			QLineEdit &control,
			double value,
			int precision)
	{
		control.setText(QString("%1").arg(value, 0, 'f', precision));
	}

	void
	display_point_on_sphere(
			QLineEdit &lat_control,
			QLineEdit &lon_control,
			const GPlatesMaths::PointOnSphere &point_on_sphere,
			int precision)
	{
		GPlatesMaths::LatLonPoint lat_lon = make_lat_lon_point(point_on_sphere);
		set_lineedit_text(lat_control, lat_lon.latitude(), precision);
		set_lineedit_text(lon_control, lat_lon.longitude(), precision);
	}

	void
	clear_and_disable(
			QLineEdit *control)
	{
		control->setEnabled(false);
		control->setText(QString());
	}
}

const unsigned int
GPlatesQtWidgets::MeasureDistanceWidget::PRECISION = 4;

GPlatesQtWidgets::MeasureDistanceWidget::MeasureDistanceWidget(
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		QWidget *parent_):
	QWidget(parent_),
	d_measure_distance_state_ptr(&measure_distance_state)
{
	setupUi(this);

	make_signal_slot_connections();

	// hide the Feature Present group box (by default)
	groupbox_feature_present->hide();

	// pick up radius of earth from MeasureDistanceState
	lineedit_radius->setText(
			QString("%1").arg(measure_distance_state.get_radius().dval()));

	// remember the original palette for radius box (should be the same for all QLineEdit)
	d_lineedit_original_palette = lineedit_radius->palette();
}

void
GPlatesQtWidgets::MeasureDistanceWidget::make_signal_slot_connections()
{
	// handle Qt events
	QObject::connect(
			lineedit_radius,
			SIGNAL(textEdited(const QString &)),
			this,
			SLOT(lineedit_radius_text_edited(const QString &)));

	// connect to MeasureDistanceState
	QObject::connect(
			d_measure_distance_state_ptr,
			SIGNAL(quick_measure_updated(
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<double>)),
			this,
			SLOT(update_quick_measure(
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<double>)));
	QObject::connect(
			d_measure_distance_state_ptr,
			SIGNAL(feature_measure_updated(
					double,
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<double>)),
			this,
			SLOT(update_feature_measure(
					double,
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<GPlatesMaths::PointOnSphere>,
					boost::optional<double>)));
	QObject::connect(
			d_measure_distance_state_ptr,
			SIGNAL(feature_measure_updated()),
			this,
			SLOT(update_feature_measure()));
	QObject::connect(
			d_measure_distance_state_ptr,
			SIGNAL(quick_measure_highlight_changed(bool)),
			this,
			SLOT(change_quick_measure_highlight(bool)));
	QObject::connect(
			d_measure_distance_state_ptr,
			SIGNAL(feature_measure_highlight_changed(bool)),
			this,
			SLOT(change_feature_measure_highlight(bool)));
}

void
GPlatesQtWidgets::MeasureDistanceWidget::update_quick_measure(
		boost::optional<GPlatesMaths::PointOnSphere> start,
		boost::optional<GPlatesMaths::PointOnSphere> end,
		boost::optional<double> distance)
{
	if (start)
	{
		display_point_on_sphere(
				*lineedit_quick_start_lat,
				*lineedit_quick_start_lon,
				*start,
				PRECISION);
		lineedit_quick_start_lat->setEnabled(true);
		lineedit_quick_start_lon->setEnabled(true);
	}
	else
	{
		clear_and_disable(lineedit_quick_start_lat);
		clear_and_disable(lineedit_quick_start_lon);
	}

	if (end)
	{
		display_point_on_sphere(
				*lineedit_quick_end_lat,
				*lineedit_quick_end_lon,
				*end,
				PRECISION);
		lineedit_quick_end_lat->setEnabled(true);
		lineedit_quick_end_lon->setEnabled(true);
	}
	else
	{
		clear_and_disable(lineedit_quick_end_lat);
		clear_and_disable(lineedit_quick_end_lon);
	}

	if (distance)
	{
		set_lineedit_text(
				*lineedit_quick_distance,
				*distance,
				PRECISION);
		lineedit_quick_distance->setEnabled(true);
	}
	else
	{
		clear_and_disable(lineedit_quick_distance);
	}
}

void
GPlatesQtWidgets::MeasureDistanceWidget::update_feature_measure(
		double total_distance,
		boost::optional<GPlatesMaths::PointOnSphere> segment_start,
		boost::optional<GPlatesMaths::PointOnSphere> segment_end,
		boost::optional<double> segment_distance)
{
	// switch which groupbox is shown
	groupbox_feature_none->setVisible(false);
	groupbox_feature_present->setVisible(true);

	set_lineedit_text(
			*lineedit_feature_total,
			total_distance,
			PRECISION);
	// note: lineedit_feature_total is not disabled ever since
	// total_distance is always supplied

	if (segment_start)
	{
		display_point_on_sphere(
				*lineedit_feature_start_lat,
				*lineedit_feature_start_lon,
				*segment_start,
				PRECISION);
		lineedit_feature_start_lat->setEnabled(true);
		lineedit_feature_start_lon->setEnabled(true);
	}
	else
	{
		clear_and_disable(lineedit_feature_start_lat);
		clear_and_disable(lineedit_feature_start_lon);
	}

	if (segment_end)
	{
		display_point_on_sphere(
				*lineedit_feature_end_lat,
				*lineedit_feature_end_lon,
				*segment_end,
				PRECISION);
		lineedit_feature_end_lat->setEnabled(true);
		lineedit_feature_end_lon->setEnabled(true);
	}
	else
	{
		clear_and_disable(lineedit_feature_end_lat);
		clear_and_disable(lineedit_feature_end_lon);
	}

	if (segment_distance)
	{
		set_lineedit_text(
				*lineedit_feature_distance,
				*segment_distance,
				PRECISION);
		lineedit_feature_distance->setEnabled(true);
	}
	else
	{
		clear_and_disable(lineedit_feature_distance);
	}
}

void
GPlatesQtWidgets::MeasureDistanceWidget::update_feature_measure()
{
	// switch which groupbox is shown
	groupbox_feature_none->setVisible(true);
	groupbox_feature_present->setVisible(false);
}

void
GPlatesQtWidgets::MeasureDistanceWidget::lineedit_radius_text_edited(
		const QString &text)
{
	bool ok;
	double radius = text.toDouble(&ok);
	if (ok)
	{
		d_measure_distance_state_ptr->set_radius(
				GPlatesMaths::real_t(radius));
		restore_background_colour(lineedit_radius);
	}
	else
	{
		change_background_colour(lineedit_radius, QColor(255, 0, 0)); // red
	}
}

void
GPlatesQtWidgets::MeasureDistanceWidget::change_quick_measure_highlight(
		bool is_highlighted)
{
	if (is_highlighted)
	{
		change_background_colour(lineedit_quick_distance, QColor(255, 255, 0)); // yellow
	}
	else
	{
		restore_background_colour(lineedit_quick_distance);
	}
}

void
GPlatesQtWidgets::MeasureDistanceWidget::change_feature_measure_highlight(
		bool is_highlighted)
{
	if (is_highlighted)
	{
		change_background_colour(lineedit_feature_distance, QColor(255, 255, 0)); // yellow
	}
	else
	{
		restore_background_colour(lineedit_feature_distance);
	}
}

void
GPlatesQtWidgets::MeasureDistanceWidget::change_background_colour(
		QLineEdit *lineedit,
		const QColor &colour)
{
		QPalette colour_palette = d_lineedit_original_palette;
		colour_palette.setColor(QPalette::Base, colour);
		lineedit->setPalette(colour_palette);
}

void
GPlatesQtWidgets::MeasureDistanceWidget::restore_background_colour(
		QLineEdit *lineedit)
{
	lineedit->setPalette(d_lineedit_original_palette);
}

