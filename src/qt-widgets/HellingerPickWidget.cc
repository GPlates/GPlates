/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015, 2016 Geological Survey of Norway
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

#include <QMessageBox>
#include <QScrollBar>

#include "HellingerDialog.h"
#include "HellingerModel.h"
#include "HellingerPickWidget.h"

namespace{
	enum PickColumns
	{
		SEGMENT_NUMBER,
		SEGMENT_TYPE,
		LAT,
		LON,
		UNCERTAINTY,

		NUM_COLUMNS
	};


	boost::optional<unsigned int>
	selected_segment_from_tree_widget(
			const QTreeWidget *tree)
	{

		if (tree->currentItem())
		{
			QString segment_string = tree->currentItem()->text(0);
			unsigned int segment = segment_string.toInt();

			return boost::optional<unsigned int>(segment);
		}
		return boost::none;
	}

	boost::optional<unsigned int>
	selected_row_from_tree_widget(
			const QTreeWidget *tree)
	{
		const QModelIndex index = tree->selectionModel()->currentIndex();

		if (index.isValid())
		{
			return boost::optional<unsigned int>(index.row());
		}

		return boost::none;
	}

	bool
	tree_item_is_pick_item(
			const QTreeWidgetItem *item)
	{
		return !item->text(SEGMENT_TYPE).isEmpty();
	}

	bool
	tree_item_is_segment_item(
			const QTreeWidgetItem *item)
	{
		return item->text(SEGMENT_TYPE).isEmpty();
	}

	/**
	 * @brief renumber_expanded_status_map
	 * On return the keys of @a map will be contiguous from 1.
	 * @param map
	 */
	void
	renumber_expanded_status_map(
			GPlatesQtWidgets::HellingerDialog::expanded_status_map_type &map)
	{
		GPlatesQtWidgets::HellingerDialog::expanded_status_map_type new_map;
		int new_index = 1;
		BOOST_FOREACH(GPlatesQtWidgets::HellingerDialog::expanded_status_map_type::value_type pair,map)
		{
			new_map.insert(std::pair<int,bool>(new_index++,pair.second));
		}
		map = new_map;
	}

	void
	set_text_colour_according_to_enabled_state(
			QTreeWidgetItem *item,
			bool enabled)
	{

		const Qt::GlobalColor text_colour = enabled? Qt::black : Qt::gray;
		static const Qt::GlobalColor background_colour = Qt::white;

		item->setBackground(SEGMENT_NUMBER,background_colour);
		item->setBackground(SEGMENT_TYPE,background_colour);
		item->setBackground(LAT,background_colour);
		item->setBackground(LON,background_colour);
		item->setBackground(UNCERTAINTY,background_colour);

		item->setForeground(SEGMENT_NUMBER,text_colour);
		item->setForeground(SEGMENT_TYPE,text_colour);
		item->setForeground(LAT,text_colour);
		item->setForeground(LON,text_colour);
		item->setForeground(UNCERTAINTY,text_colour);
	}

	void
	set_hovered_item(
			QTreeWidgetItem *item)
	{

		static const Qt::GlobalColor text_colour = Qt::black;
		static const Qt::GlobalColor background_colour = Qt::yellow;

		item->setBackground(SEGMENT_NUMBER,background_colour);
		item->setBackground(SEGMENT_TYPE,background_colour);
		item->setBackground(LAT,background_colour);
		item->setBackground(LON,background_colour);
		item->setBackground(UNCERTAINTY,background_colour);

		item->setForeground(SEGMENT_NUMBER,text_colour);
		item->setForeground(SEGMENT_TYPE,text_colour);
		item->setForeground(LAT,text_colour);
		item->setForeground(LON,text_colour);
		item->setForeground(UNCERTAINTY,text_colour);
	}

	void
	reset_hovered_item(
			QTreeWidgetItem *item,
			bool original_state)
	{
		set_text_colour_according_to_enabled_state(item,original_state);
	}

	/**
	 * @brief translate_segment_type
	 *	Convert PLATE_ONE_PICK_TYPE/DISABLED_PLATE_ONE_PICK_TYPE types to a QString form of PLATE_ONE_PICK_TYPE;
	 *  similarly for PLATE_TWO... and PLATE_THREE...
	 * @param type
	 * @return
	 */
	QString
	translate_segment_type(
			GPlatesQtWidgets::HellingerPlateIndex type)
	{
		switch(type)
		{
		case GPlatesQtWidgets::PLATE_ONE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::PLATE_ONE_PICK_TYPE);
			break;
		case GPlatesQtWidgets::PLATE_TWO_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::PLATE_TWO_PICK_TYPE);
			break;
		case GPlatesQtWidgets::PLATE_THREE_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::PLATE_THREE_PICK_TYPE);
			break;
		default:
			return QString();
		}
	}

	void
	add_pick_to_segment(
			QTreeWidget *tree,
			QTreeWidgetItem *parent_item,
			const int &segment_number,
			const GPlatesQtWidgets::HellingerPick &pick,
			GPlatesQtWidgets::HellingerPickWidget::tree_items_collection_type &tree_indices,
			bool set_as_selected)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(SEGMENT_NUMBER, QString::number(segment_number));
		item->setText(SEGMENT_TYPE, translate_segment_type(pick.d_segment_type));
		item->setText(LAT, QString::number(pick.d_lat));
		item->setText(LON, QString::number(pick.d_lon));
		item->setText(UNCERTAINTY, QString::number(pick.d_uncertainty));
		parent_item->addChild(item);

		if (!pick.d_is_enabled)
		{
			item->setForeground(SEGMENT_NUMBER,Qt::gray);
			item->setForeground(SEGMENT_TYPE,Qt::gray);
			item->setForeground(LAT,Qt::gray);
			item->setForeground(LON,Qt::gray);
			item->setForeground(UNCERTAINTY,Qt::gray);
		}
		if (pick.d_is_enabled)
		{
			tree_indices.push_back(item);
		}
		item->setSelected(set_as_selected);
		if (set_as_selected)
		{
			tree->setCurrentItem(item);
		}
	}

	void
	add_pick_to_tree(
			const int &segment_number,
			const GPlatesQtWidgets::HellingerPick &pick,
			QTreeWidget *tree,
			GPlatesQtWidgets::HellingerPickWidget::tree_items_collection_type &tree_indices,
			bool set_as_selected_pick)
	{
		QString segment_as_string = QString::number(segment_number);
		QList<QTreeWidgetItem*> items = tree->findItems(
					segment_as_string, Qt::MatchExactly, 0);
		QTreeWidgetItem *item;
		if (items.isEmpty())
		{
			item = new QTreeWidgetItem(tree);
			item->setText(0, segment_as_string);

		}
		else
		{
			item = items.at(0);
		}
		add_pick_to_segment(tree,
							item,
							segment_number,
							pick,
							tree_indices,
							set_as_selected_pick);

	}


	/**
	 * For debugging.
	 */
	void
	display_map(
			const GPlatesQtWidgets::HellingerDialog::expanded_status_map_type &map)
	{
		BOOST_FOREACH(GPlatesQtWidgets::HellingerDialog::expanded_status_map_type::value_type pair,map)
		{
			qDebug() << "key: " << pair.first << ", value: " << pair.second;
		}
	}

}

GPlatesQtWidgets::HellingerPickWidget::HellingerPickWidget(
		GPlatesQtWidgets::HellingerDialog *hellinger_dialog,
		GPlatesQtWidgets::HellingerModel *hellinger_model):
	QWidget(hellinger_dialog),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_hovered_item_original_state(true)
{
	setupUi(this);
	set_up_connections();
	initialise_widgets();
}

void
GPlatesQtWidgets::HellingerPickWidget::update_after_switching_tabs()
{

}

void
GPlatesQtWidgets::HellingerPickWidget::update_from_model(
		bool expand_tree_after_update)
{
	update_tree_from_model();
	update_buttons();

	if (expand_tree_after_update)
	{
		handle_expand_all();
	}
}

void
GPlatesQtWidgets::HellingerPickWidget::update_buttons()
{
	bool picks_loaded_ = picks_loaded();
	button_expand_all->setEnabled(picks_loaded_);
	button_collapse_all->setEnabled(picks_loaded_);
	button_renumber->setEnabled(!d_hellinger_model_ptr->segments_are_ordered());
	button_clear->setEnabled(picks_loaded_);

	button_remove_segment->setEnabled(static_cast<bool>(d_selected_segment));
	button_remove_pick->setEnabled(static_cast<bool>(d_selected_pick));

	button_edit_pick->setEnabled(d_selected_pick &&
								 !d_hellinger_dialog_ptr->adjust_pole_tool_is_active());
	button_edit_segment->setEnabled(d_selected_segment &&
									!d_hellinger_dialog_ptr->adjust_pole_tool_is_active());

	button_new_pick->setEnabled(!d_hellinger_dialog_ptr->adjust_pole_tool_is_active());
	button_new_segment->setEnabled(!d_hellinger_dialog_ptr->adjust_pole_tool_is_active());

	//Update enable/disable depending on state of selected pick, if we have
	// a selected pick.
	update_enable_disable_buttons();
}



void
GPlatesQtWidgets::HellingerPickWidget::expand_segment(
		const unsigned int segment_number)
{
	int top_level_items = tree_widget->topLevelItemCount();
	for (int i = 0; i < top_level_items ; ++i)
	{
		const unsigned int segment = tree_widget->topLevelItem(i)->text(0).toInt();

		if (segment == segment_number){
			tree_widget->topLevelItem(i)->setExpanded(true);
			expanded_status_map_type::iterator iter = d_segment_expanded_status.find(segment);
			if (iter != d_segment_expanded_status.end())
			{
				iter->second = true;
			}
			return;
		}
	}
}

void
GPlatesQtWidgets::HellingerPickWidget::set_selected_segment(
		const unsigned int segment_number)
{
	int top_level_items = tree_widget->topLevelItemCount();
	for (int i = 0; i < top_level_items ; ++i)
	{
		const unsigned int segment = tree_widget->topLevelItem(i)->text(0).toInt();
		if (segment == segment_number){
			tree_widget->setCurrentItem(tree_widget->topLevelItem(i));
			return;
		}
	}
}

void
GPlatesQtWidgets::HellingerPickWidget::set_selected_pick(
		const hellinger_model_type::const_iterator &it)
{
	d_selected_pick = it;
}

boost::optional<unsigned int>
GPlatesQtWidgets::HellingerPickWidget::segment_number_of_selected_pick()
{
	return selected_segment_from_tree_widget(tree_widget);
}

boost::optional<unsigned int>
GPlatesQtWidgets::HellingerPickWidget::selected_segment()
{
	return d_selected_segment;
}

boost::optional<unsigned int>
GPlatesQtWidgets::HellingerPickWidget::selected_row()
{
	return selected_row_from_tree_widget(tree_widget);
}

boost::optional<GPlatesQtWidgets::hellinger_model_type::const_iterator>
GPlatesQtWidgets::HellingerPickWidget::selected_pick()
{
	return d_selected_pick;
}

GPlatesQtWidgets::HellingerPickWidget::tree_items_collection_type
GPlatesQtWidgets::HellingerPickWidget::tree_items() const
{
	return d_tree_items;
}

void
GPlatesQtWidgets::HellingerPickWidget::restore()
{
	restore_expanded_status();
}

void GPlatesQtWidgets::HellingerPickWidget::handle_close()
{
	store_expanded_status();
}

void
GPlatesQtWidgets::HellingerPickWidget::initialise_widgets()
{
	QStringList labels;
	labels << QObject::tr("Segment")
		   << QObject::tr("Plate index")
		   << QObject::tr("Latitude")
		   << QObject::tr("Longitude")
		   << QObject::tr("Uncertainty (km)");
	tree_widget->setHeaderLabels(labels);

	tree_widget->header()->resizeSection(SEGMENT_NUMBER,80);
	tree_widget->header()->resizeSection(SEGMENT_TYPE,140);
	tree_widget->header()->resizeSection(LAT,90);
	tree_widget->header()->resizeSection(LON,90);
	tree_widget->header()->resizeSection(UNCERTAINTY,90);
}

void
GPlatesQtWidgets::HellingerPickWidget::set_up_connections()
{
	QObject::connect(button_expand_all, SIGNAL(clicked()), this, SLOT(handle_expand_all()));
	QObject::connect(button_collapse_all, SIGNAL(clicked()), this, SLOT(handle_collapse_all()));
	QObject::connect(button_new_pick, SIGNAL(clicked()), this, SLOT(handle_add_new_pick()));
	QObject::connect(button_edit_pick, SIGNAL(clicked()), this, SLOT(handle_edit_pick()));
	QObject::connect(button_remove_pick, SIGNAL(clicked()), this, SLOT(handle_remove_pick()));
	QObject::connect(button_remove_segment, SIGNAL(clicked()), this, SLOT(handle_remove_segment()));
	QObject::connect(button_new_segment, SIGNAL(clicked()), this, SLOT(handle_add_new_segment()));
	QObject::connect(button_edit_segment, SIGNAL(clicked()), this, SLOT(handle_edit_segment()));
	QObject::connect(button_activate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
	QObject::connect(button_deactivate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
	QObject::connect(button_renumber, SIGNAL(clicked()), this, SLOT(handle_renumber_segments()));
	QObject::connect(button_clear,SIGNAL(clicked()),this,SLOT(handle_clear()));
	QObject::connect(tree_widget,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::connect(tree_widget,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::connect(tree_widget->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
					 this, SLOT(handle_selection_changed(const QItemSelection &, const QItemSelection &)));
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_edit_pick()
{
	store_expanded_status();
	Q_EMIT edit_pick_signal();
}

void GPlatesQtWidgets::HellingerPickWidget::handle_add_new_pick()
{
	store_expanded_status();
	Q_EMIT add_new_pick_signal();
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_remove_pick()
{
	if (!d_selected_pick)
	{
		return;
	}
	QMessageBox message_box;
	message_box.setIcon(QMessageBox::Warning);
	message_box.setWindowTitle(tr("Remove pick"));
	message_box.setText(
				tr("Are you sure you want to remove the pick?"));
	message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
	message_box.setDefaultButton(QMessageBox::Ok);
	int ret = message_box.exec();

	if (ret == QMessageBox::Cancel)
	{
		return;
	}
	else
	{
		boost::optional<unsigned int> segment = selected_segment_from_tree_widget(tree_widget);
		boost::optional<unsigned int> row = selected_row_from_tree_widget(tree_widget);

		if (!(segment && row))
		{
			return;
		}

		if (d_selected_pick && *d_selected_pick == d_hellinger_model_ptr->get_pick(*segment,*row))
		{
			d_selected_pick.reset();
		}

		d_hellinger_model_ptr->remove_pick(*segment, *row);
		update_tree_from_model();
		update_buttons();
		restore_expanded_status();

		Q_EMIT tree_updated_signal();
	}
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_add_new_segment()
{
	store_expanded_status();
	Q_EMIT add_new_segment_signal();
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_edit_segment()
{
	store_expanded_status();
	Q_EMIT edit_segment_signal();
}


void
GPlatesQtWidgets::HellingerPickWidget::handle_remove_segment()
{
	if (!d_selected_segment)
	{
		return;
	}
	QMessageBox message_box;
	message_box.setIcon(QMessageBox::Warning);
	message_box.setWindowTitle(tr("Remove segment"));
	message_box.setText(
				tr("Are you sure you want to remove the segment?"));
	message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
	message_box.setDefaultButton(QMessageBox::Ok);
	int ret = message_box.exec();

	if (ret == QMessageBox::Cancel)
	{
		return;
	}
	else
	{
		store_expanded_status();
		QString segment = tree_widget->currentItem()->text(0);
		unsigned int segment_int = segment.toInt();

		if (d_selected_segment && *d_selected_segment == segment_int)
		{
			d_selected_segment.reset();
		}

		d_hellinger_model_ptr->remove_segment(segment_int);

		update_tree_from_model();
		update_buttons();
		restore_expanded_status();

		Q_EMIT tree_updated_signal();
	}
}


void
GPlatesQtWidgets::HellingerPickWidget::handle_selection_changed(
		const QItemSelection &new_selection,
		const QItemSelection &old_selection)
{
	update_selected_pick_and_segment();
	update_buttons();

	Q_EMIT tree_updated_signal();
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_pick_state_changed()
{
	boost::optional<unsigned int> segment = selected_segment_from_tree_widget(tree_widget);
	boost::optional<unsigned int> row = selected_row_from_tree_widget(tree_widget);

	if (!(segment && row))
	{
		return;
	}

	store_expanded_status();
	store_scrollbar_status();

	bool new_enabled_state = !d_hellinger_model_ptr->pick_is_enabled(*segment, *row);

	d_hellinger_model_ptr->set_pick_state(*segment,*row,new_enabled_state);

	button_activate_pick->setEnabled(!new_enabled_state);
	button_deactivate_pick->setEnabled(new_enabled_state);

	update_tree_from_model();

	restore_expanded_status();
	restore_scrollbar_status();

	if (tree_widget->currentItem())
	{
		tree_widget->scrollToItem(tree_widget->currentItem());
	}

	tree_widget->setFocus();
	Q_EMIT tree_updated_signal();
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_clear()
{
	QMessageBox message_box;
	message_box.setIcon(QMessageBox::Warning);
	message_box.setWindowTitle(tr("Clear all picks"));
	message_box.setText(
				tr("Are you sure you want to remove all the picks?"));
	message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
	message_box.setDefaultButton(QMessageBox::Ok);
	int ret = message_box.exec();

	if (ret == QMessageBox::Cancel)
	{
		return;
	}
	else
	{
		d_hellinger_model_ptr->clear_all_picks();
		update_tree_from_model();
	}
	update_selected_pick_and_segment();
	update_buttons();

	Q_EMIT tree_updated_signal();
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_renumber_segments()
{
	renumber_segments();
}

void
GPlatesQtWidgets::HellingerPickWidget::store_expanded_status()
{
	int count = tree_widget->topLevelItemCount();
	d_segment_expanded_status.clear();
	for (int i = 0 ; i < count; ++i)
	{
		d_segment_expanded_status.insert(
				std::make_pair<int,bool>(
						tree_widget->topLevelItem(i)->text(0).toInt()/*segment*/,
						tree_widget->topLevelItem(i)->isExpanded()));
	}
}

void
GPlatesQtWidgets::HellingerPickWidget::restore_expanded_status()
{
	int top_level_items = tree_widget->topLevelItemCount();
	QObject::disconnect(tree_widget,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::disconnect(tree_widget,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));
	for (int i = 0; i < top_level_items ; ++i)
	{
		int segment = tree_widget->topLevelItem(i)->text(0).toInt();
		expanded_status_map_type::const_iterator iter = d_segment_expanded_status.find(segment);
		if (iter != d_segment_expanded_status.end())
		{
			tree_widget->topLevelItem(i)->setExpanded(iter->second);
		}

	}
	QObject::connect(tree_widget,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::connect(tree_widget,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));
}

void
GPlatesQtWidgets::HellingerPickWidget::update_tree_from_model()
{
	QObject::disconnect(tree_widget->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
						this, SLOT(handle_selection_changed(const QItemSelection &, const QItemSelection &)));
	QObject::disconnect(tree_widget,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::disconnect(tree_widget,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));

	tree_widget->clear();

	d_tree_items.clear();
	hellinger_model_type::const_iterator
			iter = d_hellinger_model_ptr->begin(),
			end = d_hellinger_model_ptr->end();

	for (; iter != end ; ++iter)
	{
		bool set_as_selected_pick =(d_selected_pick && (*d_selected_pick == iter));
		add_pick_to_tree(
					iter->first,
					iter->second,
					tree_widget,
					d_tree_items,
					set_as_selected_pick);

	}

	QObject::connect(tree_widget->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
					 this, SLOT(handle_selection_changed(const QItemSelection &, const QItemSelection &)));
	QObject::connect(tree_widget,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::connect(tree_widget,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));
}

void
GPlatesQtWidgets::HellingerPickWidget::update_selected_pick_and_segment()
{
	d_selected_pick.reset();
	d_selected_segment.reset();

	if (!tree_widget->currentItem())
	{
		return;
	}

	boost::optional<unsigned int> selected_segment_number = selected_segment_from_tree_widget(tree_widget);
	boost::optional<unsigned int> selected_row_number = selected_row_from_tree_widget(tree_widget);

	if (tree_item_is_segment_item(tree_widget->currentItem()))
	{
		d_selected_segment.reset(selected_segment_number.get());
	}
	else
	{
		d_selected_pick.reset(d_hellinger_model_ptr->get_pick(selected_segment_number.get(),
															  selected_row_number.get()));
	}

}

void
GPlatesQtWidgets::HellingerPickWidget::update_enable_disable_buttons()
{

	button_activate_pick->setEnabled(false);
	button_deactivate_pick->setEnabled(false);

	if (!d_selected_pick)
	{
		return;
	}

	boost::optional<unsigned int> segment = selected_segment_from_tree_widget(tree_widget);
	boost::optional<unsigned int> row = selected_row_from_tree_widget(tree_widget);

	if (segment && row)
	{
		bool enabled = d_hellinger_model_ptr->pick_is_enabled(*segment, *row);

		button_activate_pick->setEnabled(!enabled);
		button_deactivate_pick->setEnabled(enabled);
#if 0
		if (d_hellinger_edit_point_dialog->isVisible())
		{
			d_hellinger_edit_point_dialog->set_active(true);
			d_hellinger_edit_point_dialog->update_pick_from_model(*segment,*row);
		}
#endif
	}
}

bool
GPlatesQtWidgets::HellingerPickWidget::picks_loaded()
{
	return (tree_widget->topLevelItemCount() != 0);
}

void
GPlatesQtWidgets::HellingerPickWidget::update_hovered_item(
		const unsigned int geometry_index,
		bool is_enabled)
{
	if (geometry_index > d_tree_items.size())
	{
		return;
	}

	QTreeWidgetItem *hovered_item = d_tree_items.at(geometry_index);

	if (d_hovered_item)
	{
		reset_hovered_item(*d_hovered_item, d_hovered_item_original_state);
	}

	d_hovered_item = hovered_item;

	if (d_hovered_item)
	{
		set_hovered_item(*d_hovered_item);
		d_hovered_item_original_state = is_enabled;
	}	
}

void
GPlatesQtWidgets::HellingerPickWidget::set_selected_pick_from_geometry_index(
		const unsigned int geometry_index)
{
	if (geometry_index > d_tree_items.size())
	{
		return;
	}

	clear_hovered_item();

	QTreeWidgetItem *selected_item = d_tree_items.at(geometry_index);

	if (selected_item)
	{
		tree_widget->setCurrentItem(selected_item);
		selected_item->setSelected(true);
	}
}

void
GPlatesQtWidgets::HellingerPickWidget::clear_hovered_item()
{
	if (d_hovered_item)
	{
		reset_hovered_item(*d_hovered_item,d_hovered_item_original_state);
	}
	d_hovered_item = boost::none;
}

void
GPlatesQtWidgets::HellingerPickWidget::renumber_segments()
{
	store_expanded_status();
	d_hellinger_model_ptr->renumber_segments();
	renumber_expanded_status_map(d_segment_expanded_status);
	update_tree_from_model();
	button_renumber->setEnabled(false);
	restore_expanded_status();
}

void
GPlatesQtWidgets::HellingerPickWidget::update_after_new_or_edited_pick(
		const hellinger_model_type::const_iterator &it,
		const int segment_number)
{
	set_selected_pick(it);
	update_from_model();
	restore_expanded_status();
	expand_segment(segment_number);
	update_buttons();
	if (tree_widget->currentItem())
	{
		tree_widget->scrollToItem(tree_widget->currentItem());
	}
}

void GPlatesQtWidgets::HellingerPickWidget::update_after_new_or_edited_segment(
		const int segment_number)
{
	update_from_model();
	restore_expanded_status();
	expand_segment(segment_number);
	set_selected_segment(segment_number);
	update_buttons();
	if (tree_widget->currentItem())
	{
		tree_widget->scrollToItem(tree_widget->currentItem());
	}
}

void
GPlatesQtWidgets::HellingerPickWidget::store_scrollbar_status()
{
	d_scrollbar_position = tree_widget->verticalScrollBar()->value();
	d_scrollbar_maximum = tree_widget->verticalScrollBar()->maximum();
}

void
GPlatesQtWidgets::HellingerPickWidget::restore_scrollbar_status()
{
	tree_widget->verticalScrollBar()->setMaximum(d_scrollbar_maximum);
	tree_widget->verticalScrollBar()->setValue(d_scrollbar_position);
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_expand_all()
{
	tree_widget->expandAll();
	store_expanded_status();
}

void
GPlatesQtWidgets::HellingerPickWidget::handle_collapse_all()
{
	tree_widget->collapseAll();
	store_expanded_status();
}


