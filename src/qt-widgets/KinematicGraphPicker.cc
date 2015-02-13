/**
 * \file
 * $Revision: 8188 $
 * $Date: 2010-04-23 12:25:09 +0200 (Fri, 23 Apr 2010) $
 *
 * Copyright (C) 2014 Geological Survey of Norway
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

#include "qwt_picker_machine.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_curve.h"
#include "qwt_series_data.h"

#include <algorithm>
#include <functional>

#include "KinematicGraphPicker.h"

namespace
{

	class
	SmallestTimeCoordinateYoungerThan:
			public std::unary_function<QPointF,bool>
	{

	public:
		explicit
		SmallestTimeCoordinateYoungerThan(
				double value):
			d_x(value)
		{ }

		bool
		operator()(
				const QPointF &point) const
		{
			return (point.x() < d_x);
		}

	private:
		double d_x;
	};

	/**
	 * @brief get_interpolated_y_value - returns y value based on linear interpolation
	 * between the x-values to the left and right of the x value contained in @a point.
	 * @param point - x and y coordinates of the cursor position at which we want the
	 * interpolated y value. Only the x value is used here.
	 * @param data
	 * @return
	 *
	 * Assumes that @a data is sorted by x from lowest to highest, and that
	 * point.x() lies inside the range of x values of @a data
	 */
	boost::optional<double>
	get_interpolated_y_value(
			const QPointF &point,
			const QwtPointSeriesData *data)
	{

		if (data->samples().isEmpty())
		{
			return boost::none;
		}

		int x = point.toPoint().x();

		SmallestTimeCoordinateYoungerThan functor(x);

		QVector<QPointF>::const_iterator upper,lower;
		upper = std::find_if(data->samples().begin(),data->samples().end(),functor);

		if (upper == data->samples().end() || upper == data->samples().begin())
		{
			return boost::none;
		}


		lower = upper;
		--lower;


		double x_lower = lower->x();
		double x_upper = upper->x();

		double y_lower = lower->y();
		double y_upper = upper->y();

		if (GPlatesMaths::are_almost_exactly_equal(x_lower,x_upper))
		{
			return boost::none;
		}

		// Interpolate
		double y = y_lower + (x-x_lower)*(y_upper-y_lower)/(x_upper-x_lower);

		return y;
	}
}

GPlatesQtWidgets::KinematicGraphPicker::KinematicGraphPicker(
		const QwtPointSeriesData *point_series_data,
		const QwtPlotCurve *plot_curve,
		QwtPlot::Axis axis1,
		QwtPlot::Axis axis2,
		RubberBand rubber_band,
		DisplayMode display_mode,
		QwtPlotCanvas *canvas_) :
	QwtPlotPicker(axis1, axis2, rubber_band, display_mode, canvas_),
	d_data_ptr(point_series_data),
	d_plot_curve_ptr(plot_curve),
	d_type(GPlatesQtWidgets::KinematicGraphsDialog::LATITUDE_GRAPH_TYPE)
{
    setStateMachine(new QwtPickerDragPointMachine());
}

QwtText
GPlatesQtWidgets::KinematicGraphPicker::trackerTextF(
		const QPointF &point) const
{
	int x = point.toPoint().x();



	if (d_data_ptr->samples().empty())
	{
		return QwtText(QString("%1 Ma").arg(x));
	}

	boost::optional<double> y = get_interpolated_y_value(point,d_data_ptr);

	if (!y)
	{
		return QwtText(QString("%1 Ma").arg(x));
	}

	QString text = QString("%1 Ma, %2 ").arg(x).arg(*y);
	QString unit_text;

	switch(d_type)
	{

	case GPlatesQtWidgets::KinematicGraphsDialog::LATITUDE_GRAPH_TYPE:
	case GPlatesQtWidgets::KinematicGraphsDialog::LONGITUDE_GRAPH_TYPE:
	case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_AZIMUTH_GRAPH_TYPE:
		unit_text = QObject::tr("\260");
		break;
	case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_MAG_GRAPH_TYPE:
	case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_COLAT_GRAPH_TYPE:
	case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_LON_GRAPH_TYPE:
		unit_text = QObject::tr(" cm/year");
		break;
	case GPlatesQtWidgets::KinematicGraphsDialog::ANGULAR_VELOCITY_GRAPH_TYPE:
	case GPlatesQtWidgets::KinematicGraphsDialog::ROTATION_RATE_GRAPH_TYPE:
		unit_text = QObject::tr(" \260/Ma");
		break;
	default:
        break;
	}
	text += unit_text;

	return QwtText(text);
}


