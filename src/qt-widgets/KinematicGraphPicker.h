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

#ifndef KINEMATICGRAPHPICKER_H
#define KINEMATICGRAPHPICKER_H

#include "qwt_plot_canvas.h"
#include "qwt_plot_picker.h"
#include "qwt_series_data.h"
#include "qwt_text.h"

#include "KinematicGraphsDialog.h"

class QwtPlotCurve;

namespace GPlatesQtWidgets
{

/**
 * @brief The KinematicGraphPicker class - used to extract and display
 * information from the kinematic graph.
 */
class KinematicGraphPicker : public QwtPlotPicker
{

public:
	KinematicGraphPicker(
			const QwtPointSeriesData *point_series_data,
			const QwtPlotCurve *plot_curve,
			QwtPlot::Axis axis1,
			QwtPlot::Axis axis2,
			QwtPicker::RubberBand rubber_band,
			QwtPicker::DisplayMode display_mode,
			QwtPlotCanvas *canvas);

	QwtText
	trackerTextF(const QPointF &) const;

	void
	set_graph_type(
			const KinematicGraphsDialog::KinematicGraphType &type)
	{
		d_type = type;
	}

Q_SIGNALS:

public Q_SLOTS:

private:

	const QwtPointSeriesData *d_data_ptr;

	const QwtPlotCurve *d_plot_curve_ptr;

	KinematicGraphsDialog::KinematicGraphType d_type;

};

}
#endif // KINEMATICGRAPHPICKER_H
