/* $Id: ColouringDialog.cc 10521 2010-12-11 06:33:37Z elau $ */

/**
 * \file 
 * $Revision: 10521 $
 * $Date: 2010-12-11 17:33:37 +1100 (Sat, 11 Dec 2010) $ 
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
#include "DrawStyleDialog.h"
#include "gui/DrawStyleManager.h"
#include "gui/HTMLColourNames.h"
#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"

GPlatesQtWidgets::DrawStyleDialog::DrawStyleDialog(
		GPlatesPresentation::ViewState &view_state,
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
		QWidget* parent_) :
	d_visual_layer(visual_layer)
{
	setupUi(this);
	
	categories_table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	categories_table->horizontalHeader()->hide();
	categories_table->verticalHeader()->hide();

	// Set up the list of colour schemes.
	style_list->setViewMode(QListWidget::IconMode);
	style_list->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	//	colour_schemes_list->setSpacing(SPACING); //Due to a qt bug, the setSpacing doesn't work well in IconMode.
	style_list->setMovement(QListView::Static);
	style_list->setWrapping(true);
	style_list->setResizeMode(QListView::Adjust);
	style_list->setUniformItemSizes(true);
	style_list->setWordWrap(true);

	QPixmap blank_pixmap(ICON_SIZE, ICON_SIZE);
	blank_pixmap.fill(*GPlatesGui::HTMLColourNames::instance().get_colour("slategray"));
	d_blank_icon = QIcon(blank_pixmap);
	d_style_mgr = GPlatesGui::DrawStyleManager::instance();
	init_dlg();
	make_signal_slot_connections();
	open_button->hide();
	add_button->hide();
	edit_button->hide();
}

void
GPlatesQtWidgets::DrawStyleDialog::init_catagory_table()
{
	categories_table->clear();
	GPlatesGui::DrawStyleManager::CatagoryContainer catas = GPlatesGui::DrawStyleManager::instance()->all_catagories();
	std::size_t n_size = catas.size();
	categories_table->setRowCount(n_size);
	for(std::size_t row=0; row < n_size; row++)
	{
		QTableWidgetItem *item = new QTableWidgetItem(catas[row]->name());
		item->setToolTip(catas[row]->desc());
		QVariant qv;
		qv.setValue(static_cast<void*>(catas[row])); // Store the colour scheme ID in the item data.
		item->setData(Qt::UserRole, qv);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		categories_table->setItem(row, 0, item);
	}
}

void
GPlatesQtWidgets::DrawStyleDialog::make_signal_slot_connections()
{
	// Close button.
	QObject::connect(
			close_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_close_button_clicked()));

	// Categories table.
	QObject::connect(
			categories_table,
			SIGNAL(currentCellChanged(int, int, int, int)),
			this,
			SLOT(handle_categories_table_cell_changed(int, int, int, int)));
}

void
GPlatesQtWidgets::DrawStyleDialog::handle_close_button_clicked()
{
	set_style();
	hide();
}


void
GPlatesQtWidgets::DrawStyleDialog::set_style()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
			locked_visual_layer->get_visual_layer_params().get());
		
		QListWidgetItem* item = style_list->currentItem();
		if(item)
		{
			QVariant qv = item->data(Qt::UserRole);
			GPlatesGui::StyleAdapter* sa = static_cast<GPlatesGui::StyleAdapter*>(qv.value<void*>());

			if (params)
			{
				params->set_style_adaper(sa);
			}
		}
	}
	GPlatesGui::DrawStyleManager::instance()->emit_style_changed();
}

void
GPlatesQtWidgets::DrawStyleDialog::init_dlg()
{
	init_catagory_table();
}

void
GPlatesQtWidgets::DrawStyleDialog::handle_categories_table_cell_changed(
		int current_row,
		int current_column,
		int previous_row,
		int previous_column)
{
	if(current_row < 0)
	{
		qWarning() << "The index of current row is negative number. Do nothing and return.";
		return;
	}
	QTableWidgetItem* item = categories_table->currentItem();
	QVariant qv = item->data(Qt::UserRole);
	GPlatesGui::StyleCatagory* cata = static_cast<GPlatesGui::StyleCatagory*>(qv.value<void*>());
	load_category(cata);
}

void
GPlatesQtWidgets::DrawStyleDialog::load_category(const GPlatesGui::StyleCatagory* cata)
{
	using namespace GPlatesGui;
	
	GPlatesGui::DrawStyleManager::StyleContainer styles = GPlatesGui::DrawStyleManager::instance()->get_styles(*cata);
	style_list->clear();
	
	BOOST_FOREACH(StyleAdapter* sa, styles)
	{
		QListWidgetItem *item = new QListWidgetItem(
				d_blank_icon,
				sa->name(),
				style_list);

		QVariant qv;
		qv.setValue(static_cast<void*>(sa)); // Store the colour scheme ID in the item data.
		item->setData(Qt::UserRole, qv);
		style_list->addItem(item);
	}

	// Show "Add" button for Single Colour category, "Open" button for every other category.
	// Also show "Edit" button only for Single Colour category.
	if (cata->name() == GPlatesGui::COLOUR_SINGLE)
	{
		open_button->hide();
		add_button->show();
		edit_button->show();
	}
	else if(cata->name() == GPlatesGui::COLOUR_PLATE_ID || 
			cata->name() == GPlatesGui::COLOUR_FEATURE_AGE || 
			cata->name() == GPlatesGui::COLOUR_FEATURE_TYPE)
	{
		open_button->show();
		add_button->hide();
		edit_button->hide();
	}
#if 0
	// Set the rendering chain in motion.
	if (d_show_thumbnails)
	{
		start_rendering_from(0);
	}
#endif
}






