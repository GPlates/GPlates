/* $Id$ */

/**
 * \file
 * Contains the implementation of the PaletteSelectionWidget class.
 *
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

#include "GlobeAndMapWidget.h"
#include "PaletteSelectionWidget.h"
#include "gui/ColourScheme.h"
#include "presentation/ViewState.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>

namespace
{
	QLabel *
	create_label(const QString &text)
	{
		QLabel *label = new QLabel(text);
		label->setMinimumSize(150, 100);
		return label;
	}
}

GPlatesQtWidgets::PaletteSelectionWidget::PaletteSelectionWidget(
		GPlatesPresentation::ViewState &view_state,
		GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
		QWidget *parent_) :
	QScrollArea(parent_),
	d_view_state(view_state),
	d_existing_globe_and_map_widget_ptr(existing_globe_and_map_widget_ptr),
	d_widget(new QWidget(this))
{
	create_layout();

	setWidget(d_widget);
	setWidgetResizable(false);
	setFrameShape(QFrame::NoFrame);
}

void
GPlatesQtWidgets::PaletteSelectionWidget::set_colour_schemes(
		boost::shared_ptr<colour_scheme_collection_type> colour_schemes,
		int selected)
{
	d_colour_schemes = colour_schemes;
	create_layout();
}

void
GPlatesQtWidgets::PaletteSelectionWidget::create_layout()
{
	if (d_widget->layout())
	{
		// Remove all existing widgets
		QLayoutItem *item;
		QLayout *widget_layout = d_widget->layout();
		while ((item = widget_layout->takeAt(0)) != NULL)
		{
			item->widget()->hide();
			widget_layout->removeItem(item);
			delete item->widget();
		}

		// Then delete the existing layout before adding the new one
		delete d_widget->layout();
	}

	QGridLayout *grid_layout = new QGridLayout(d_widget);

	if (d_colour_schemes)
	{
		int col = 0;
		static const int MAX_COLS = 2;

		for (colour_scheme_collection_type::iterator iter = d_colour_schemes->begin();
				iter != d_colour_schemes->end();
				++iter)
		{
			/*
			grid_layout->addWidget(create_label(iter->first), 0, col);
			GlobeAndMapWidget *globe_and_map_widget_ptr = new GlobeAndMapWidget(
					d_existing_globe_and_map_widget_ptr,
					iter->second,
					d_widget);
			grid_layout->addWidget(globe_and_map_widget_ptr, 1, col);
			++col;
			*/
			QWidget *cell_widget = new QWidget();
			QVBoxLayout *cell_widget_layout = new QVBoxLayout(cell_widget);
			GlobeAndMapWidget *globe_and_map_widget = new GlobeAndMapWidget(
					d_existing_globe_and_map_widget_ptr,
					iter->second,
					cell_widget);
			globe_and_map_widget->set_mouse_wheel_enabled(false);
			cell_widget_layout->addWidget(globe_and_map_widget);
			cell_widget_layout->addWidget(new QLabel(iter->first));

			grid_layout->addWidget(cell_widget, col / MAX_COLS, col % MAX_COLS);
			++col;
		}
	}

	d_widget->setLayout(grid_layout);

	static int s = 400;
	d_widget->setMinimumSize(s, 200);
	d_widget->setMaximumSize(s, 400);
	d_widget->resize(s, s);
}

