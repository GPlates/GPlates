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

#include "ColouringDialog.h"
#include "PaletteSelectionWidget.h"
#include "gui/ColourSchemeFactory.h"
#include "presentation/ViewState.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QColor>

GPlatesQtWidgets::ColouringDialog::ColouringDialog(
		GPlatesPresentation::ViewState &view_state,
		GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_view_state(view_state),
	d_palette_selection_widget_ptr(
			new PaletteSelectionWidget(
				view_state,
				existing_globe_and_map_widget_ptr))
{
	setupUi(this);
	make_signal_slot_connections();
	
	// Add the PaletteSelectionWidget with the panel down below
	QHBoxLayout *palette_layout = new QHBoxLayout(widget_Palette);
	palette_layout->addWidget(d_palette_selection_widget_ptr);
	palette_layout->setContentsMargins(0, 0, 0, 0);
	palette_layout->setSpacing(0);
	d_palette_selection_widget_ptr->setParent(widget_Palette);
	
	// Populate the categories (dummy list for now)
	listwidget_Categories->addItem("Plate ID");
	listwidget_Categories->addItem("Single Colour");
	listwidget_Categories->addItem("Feature Type");
	listwidget_Categories->addItem("Feature Age");
	
	// Move the splitter as far left as possible without collapsing the left side
	QList<int> sizes;
	sizes.append(1);
	sizes.append(this->width());
	splitter_Main->setSizes(sizes);
	
	/*
	QHBoxLayout *scroll_layout = new QHBoxLayout(widget_ScrollArea);
	scroll_layout->addWidget(d_globe_canvas_ptr.get());
	scroll_layout->addWidget(new QLabel("Hello World"));
	d_globe_canvas_ptr->setParent(widget_ScrollArea);
	widget_ScrollArea->setLayout(scroll_layout);
	d_globe_canvas_ptr->setMinimumSize(500, 500);
	*/

	// Populate the feature collection list (dummy list for now)
	combobox_Feature_Collection->addItem("All feature collections");
	checkbox_Use_Global->setEnabled(false);
}

void
GPlatesQtWidgets::ColouringDialog::close_button_pressed()
{
	boost::shared_ptr<PaletteSelectionWidget::colour_scheme_collection_type> colour_schemes =
		boost::shared_ptr<PaletteSelectionWidget::colour_scheme_collection_type>(
				new PaletteSelectionWidget::colour_scheme_collection_type());
	colour_schemes->push_back(std::make_pair("Default",
				GPlatesGui::ColourSchemeFactory::create_default_plate_id_colour_scheme()));
	colour_schemes->push_back(std::make_pair("By Region",
				GPlatesGui::ColourSchemeFactory::create_regional_plate_id_colour_scheme()));
	colour_schemes->push_back(std::make_pair("red.cpt (CPT file)",
				GPlatesGui::ColourSchemeFactory::create_single_colour_scheme(QColor(255, 0, 0))));
	d_palette_selection_widget_ptr->set_colour_schemes(colour_schemes, 0);
}

void
GPlatesQtWidgets::ColouringDialog::make_signal_slot_connections()
{
	QObject::connect(button_Close, SIGNAL(pressed()), this, SLOT(close_button_pressed()));
}

