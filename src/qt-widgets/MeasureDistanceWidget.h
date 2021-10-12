/* $Id$ */

/**
 * @file
 * Contains the definition of the MeasureDistanceWidget class.
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
 
#ifndef GPLATES_QTWIDGETS_MEASUREDISTANCEWIDGET_H
#define GPLATES_QTWIDGETS_MEASUREDISTANCEWIDGET_H

#include <QWidget>
#include <QPalette>
#include <boost/optional.hpp>

#include "MeasureDistanceWidgetUi.h"
#include "maths/PointOnSphere.h"

namespace GPlatesCanvasTools
{
	class MeasureDistanceState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;

	/**
	 * TaskPanel widget that displays information for distance measuring canvas tool
	 */
	class MeasureDistanceWidget:
			public QWidget, 
			protected Ui_MeasureDistanceWidget
	{
		Q_OBJECT
		
	public:

		/**
		 * Contructor
		 */
		explicit
		MeasureDistanceWidget(
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				QWidget *parent_ = NULL);
		
	private slots:

		/**
		 * Update the Quick Measure part of the widget
		 */
		void
		update_quick_measure(
				boost::optional<GPlatesMaths::PointOnSphere> start,
				boost::optional<GPlatesMaths::PointOnSphere> end,
				boost::optional<double> distance);
		
		/**
		 * Update the Feature Measure part of the widget
		 * (when there is a feature to show)
		 */
		void
		update_feature_measure(
				double total_distance,
				boost::optional<GPlatesMaths::PointOnSphere> segment_start,
				boost::optional<GPlatesMaths::PointOnSphere> segment_end,
				boost::optional<double> segment_distance);

		/**
		 * Update the Feature Measure part of the widget
		 * (when there is NO feature to show)
		 */
		void
		update_feature_measure();

		/**
		 * Handles textEdited signal for lineedit_radius
		 * (we only want to pick up changes by the user, not changes
		 * made programmatically)
		 */
		void
		lineedit_radius_text_edited(
				const QString &text);

		/**
		 * Toggles background highlight of Quick Measure distance field
		 */
		void
		change_quick_measure_highlight(
				bool is_highlighted);

		/**
		 * Toggles background highlight of Feature Measure segment distance field
		 */
		void
		change_feature_measure_highlight(
				bool is_highlighted);
	
	private:

		/**
		 * A pointer to the state of the measuring distance tool
		 */
		GPlatesCanvasTools::MeasureDistanceState *d_measure_distance_state_ptr;

		/**
		 * Stores the original palette of the QLineEdit controls so that we
		 * can change their background colour back again
		 */
		QPalette
		d_lineedit_original_palette;
		
		/**
		 * Sets up Qt signal/slots
		 */
		void
		make_signal_slot_connections();

		/**
		 * Changes the background colour of a QLineEdit to a particular colour
		 */
		void
		change_background_colour(
				QLineEdit *lineedit,
				const QColor &colour);

		/**
		 * Restores the background colour of a QLineEdit
		 */
		void
		restore_background_colour(
				QLineEdit *lineedit);
		
		/**
		 * The number of decimal places used in the part above history table
		 */
		static
		const unsigned int
		PRECISION;

	};
}

#endif  // GPLATES_QTWIDGETS_MEASUREDISTANCEWIDGET_H

