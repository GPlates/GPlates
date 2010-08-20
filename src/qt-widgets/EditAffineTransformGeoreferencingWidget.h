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
 
#ifndef GPLATES_QTWIDGETS_EDITAFFINETRANSFORMGEOREFERENCINGWIDGET_H
#define GPLATES_QTWIDGETS_EDITAFFINETRANSFORMGEOREFERENCINGWIDGET_H

#include <boost/optional.hpp>
#include <QWidget>
#include <QDoubleSpinBox>

#include "EditAffineTransformGeoreferencingWidgetUi.h"

#include "property-values/Georeferencing.h"


namespace GPlatesQtWidgets
{
	class EditAffineTransformGeoreferencingWidget:
			public QWidget,
			protected Ui_EditAffineTransformGeoreferencingWidget
	{
		Q_OBJECT
		
	public:

		explicit
		EditAffineTransformGeoreferencingWidget(
				GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing,
				QWidget *parent_ = NULL);

		/**
		 * Repopulates the spinboxes depending on which page is currently visible.
		 */
		void
		refresh();

		void
		set_raster_size(
				unsigned int raster_width,
				unsigned int raster_height)
		{
			d_raster_width = raster_width;
			d_raster_height = raster_height;
		}

	signals:

		void
		warning_visible_changed(
				bool visible);

		void
		georeferencing_changed();

	private slots:

		void
		handle_advanced_checkbox_state_changed(
				int state);

		void
		update_extents_if_necessary();

		void
		update_affine_transform_if_necessary();

		void
		handle_use_global_extents_button_clicked();

	private:

		typedef GPlatesPropertyValues::Georeferencing::lat_lon_extents_type lat_lon_extents_type;
		typedef GPlatesPropertyValues::Georeferencing::parameters_type affine_transform_type;

		void
		make_signal_slot_connections();

		void
		populate_lat_lon_extents_spinboxes(
				const boost::optional<lat_lon_extents_type> &extents);

		void
		populate_affine_transform_spinboxes(
				const affine_transform_type &parameters);

		QDoubleSpinBox *d_extents_spinboxes[lat_lon_extents_type::NUM_COMPONENTS];
		QDoubleSpinBox *d_affine_transform_spinboxes[affine_transform_type::NUM_COMPONENTS];

		double d_last_known_extents_values[lat_lon_extents_type::NUM_COMPONENTS];
		double d_last_known_affine_transform_values[affine_transform_type::NUM_COMPONENTS];

		GPlatesPropertyValues::Georeferencing::non_null_ptr_type &d_georeferencing;
		unsigned int d_raster_width, d_raster_height;
	};
}

#endif  // GPLATES_QTWIDGETS_EDITAFFINETRANSFORMGEOREFERENCINGWIDGET_H

