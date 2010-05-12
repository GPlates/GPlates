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

#include <boost/foreach.hpp>
#include <QFileInfo>
#include <QHeaderView>
#include <QApplication>
#include <QDesktopWidget>
#include <QMetaType>
#include <QPixmap>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QUrl>
#include <QtGlobal>

#include "ColouringDialog.h"
#include "GlobeAndMapWidget.h"

#include "app-logic/ApplicationState.h"

#include "file-io/RegularCptReader.h"

#include "gui/Colour.h"
#include "gui/ColourSchemeContainer.h"
#include "gui/ColourSchemeInfo.h"
#include "gui/ColourSchemeFactory.h"
#include "gui/GenericContinuousColourPalette.h"
#include "gui/HTMLColourNames.h"
#include "gui/SingleColourScheme.h"

#include "model/FeatureHandle.h"
#include "model/WeakReference.h"
#include "model/WeakReferenceCallback.h"

#include "presentation/ViewState.h"

#include "utils/UnicodeStringUtils.h"

#include <QDebug>


// This is to allow FeatureCollectionHandle::const_weak_ref to be stored in the
// userData of a QComboBox item, which is of type QVariant.
// The macro is defined in <QMetaType>.
Q_DECLARE_METATYPE(GPlatesModel::WeakReference<const GPlatesModel::FeatureCollectionHandle>)


namespace
{
	void
	insert_separator(
			QComboBox *combobox)
	{
#if QT_VERSION >= 0x040400
		combobox->insertSeparator(combobox->count());
#endif
	}

	void
	remove_separator(
			QComboBox *combobox)
	{
#if QT_VERSION >= 0x040400
		if (combobox->count() == 2)
		{
			combobox->removeItem(1);
		}
#endif
	}

	/**
	 * Automagically removes a feature collection from the combobox when it gets deactivated.
	 */
	class FeatureCollectionRemover :
			public GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureCollectionHandle>
	{
	public:

		FeatureCollectionRemover(
				QComboBox *combobox) :
			d_combobox(combobox)
		{
		}

		void
		publisher_deactivated(
				const deactivated_event_type &event)
		{
			GPlatesModel::FeatureCollectionHandle::const_weak_ref deactivated = event.reference();

			for (int i = 0; i < d_combobox->count(); ++i)
			{
				QVariant qv = d_combobox->itemData(i);
				if (!qv.isNull())
				{
					GPlatesModel::FeatureCollectionHandle::const_weak_ref curr =
						qv.value<GPlatesModel::FeatureCollectionHandle::const_weak_ref>();
					if (curr == deactivated)
					{
						// Found: remove from combobox.
						if (d_combobox->currentIndex() == i)
						{
							d_combobox->setCurrentIndex(0);
						}
						d_combobox->removeItem(i);

						// If we just removed the last feature collection, also remove the separator.
						remove_separator(d_combobox);
						return;
					}
				}
			}
		}

	private:

		QComboBox *d_combobox;
	};


	void
	add_feature_collection_to_combobox(
			GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection,
			QComboBox *combobox)
	{
		// FIXME: Maybe show a feature collection only if it contains reconstructable features.

		// Attach a callback so that the feature collection removes itself when deactivated.
		feature_collection.attach_callback(
				new FeatureCollectionRemover(combobox));

		// Add a separator to the combobox first if this is the first feature collection to be added.
		if (combobox->count() == 1) /* i.e. just the all feature collections line */
		{
			insert_separator(combobox);
		}
		
		// Now, add it to the feature collection combobox.
		const boost::optional<UnicodeString> &filename_opt = feature_collection->filename();
		QString filename = filename_opt ?
			QFileInfo(GPlatesUtils::make_qstring_from_icu_string(*filename_opt)).fileName() :
			"New Feature Collection";
		QVariant qv;
		qv.setValue(feature_collection);
		combobox->addItem(filename, qv);
	}


	class AddFeatureCollectionCallback :
			public GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureStoreRootHandle>
	{
	public:

		AddFeatureCollectionCallback(
				QComboBox *combobox) :
			d_combobox(combobox)
		{
		}

		void
		publisher_added(
				const added_event_type &event)
		{
			typedef std::vector<GPlatesModel::FeatureStoreRootHandle::const_iterator> new_children_collection_type;
			const new_children_collection_type &new_children_collection = event.new_children();

			for (new_children_collection_type::const_iterator iter = new_children_collection.begin();
					iter != new_children_collection.end(); ++iter)
			{
				const GPlatesModel::FeatureStoreRootHandle::const_iterator &new_child_iter = *iter;
				if (new_child_iter.is_still_valid())
				{
					add_feature_collection_to_combobox(
							(*new_child_iter)->reference(),
							d_combobox);
				}
			}
		}

	private:

		QComboBox *d_combobox;
	};


	/**
	 * Transforms a list of file:// urls into a list of pathnames in string form.
	 * Ignores any non-file url, and ignores any non-colour-palette file extension.
	 * Used for drag-and-drop support.
	 */
	QStringList
	extract_colour_palette_pathnames_from_file_urls(
			const QList<QUrl> &urls)
	{
		QStringList pathnames;
		Q_FOREACH(const QUrl &url, urls) {
			if (url.scheme() == "file") {
				QString path = url.toLocalFile();
				// Only accept .cpt files.
				if (path.endsWith(".cpt")) {
					pathnames.push_back(path);
				}
			}
		}
		return pathnames;	// QStringList implicitly shares data and uses pimpl idiom, return by value is fine.
	}

}// anon namespace


GPlatesQtWidgets::ColouringDialog::ColouringDialog(
		GPlatesPresentation::ViewState &view_state,
		GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_application_state(view_state.get_application_state()),
	d_existing_globe_and_map_widget_ptr(existing_globe_and_map_widget_ptr),
	d_colour_scheme_container(view_state.get_colour_scheme_container()),
	d_view_state_colour_scheme_delegator(view_state.get_colour_scheme_delegator()),
	d_preview_colour_scheme(
			new PreviewColourScheme(
				d_view_state_colour_scheme_delegator)),
	d_globe_and_map_widget_ptr(
			new GlobeAndMapWidget(
				existing_globe_and_map_widget_ptr,
				d_preview_colour_scheme,
				this)),
	d_feature_store_root(
			view_state.get_application_state().get_model_interface()->root()),
	d_show_thumbnails(true),
	d_suppress_next_repaint(false),
	d_last_single_colour(Qt::white)
{
	setupUi(this);
	reposition();

	d_categories_table_original_palette = categories_table->palette();

	// Create the blank icon.
	QPixmap blank_pixmap(ICON_SIZE, ICON_SIZE);
	blank_pixmap.fill(*GPlatesGui::HTMLColourNames::instance().get_colour("slategray"));
	d_blank_icon = QIcon(blank_pixmap);

	// Set up our GlobeAndMapWidget that we use for rendering.
	d_globe_and_map_widget_ptr->resize(ICON_SIZE, ICON_SIZE);
	d_globe_and_map_widget_ptr->move(1 - ICON_SIZE, 1 - ICON_SIZE); // leave 1px showing

	// Set up the list of feature collections.
	populate_feature_collections();

	// Set up the table of colour scheme categories.
	populate_colour_scheme_categories();
	categories_table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	categories_table->horizontalHeader()->hide();
	categories_table->verticalHeader()->hide();
	set_categories_table_active_palette();

	// Set up the list of colour schemes.
	colour_schemes_list->setViewMode(QListWidget::IconMode);
	colour_schemes_list->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	colour_schemes_list->setSpacing(SPACING);
	colour_schemes_list->setMovement(QListView::Static);
	colour_schemes_list->setWrapping(true);
	colour_schemes_list->setResizeMode(QListView::Adjust);
	colour_schemes_list->setUniformItemSizes(true);
	colour_schemes_list->setWordWrap(true);
	
	// Change the background colour of the right hand side.
	QPalette right_palette = right_side_frame->palette();
	right_palette.setColor(QPalette::Window, colour_schemes_list->palette().color(QPalette::Base));
	right_side_frame->setPalette(right_palette);

	// Get current colour scheme selection from ViewState's colour scheme delegator.
	std::pair<GPlatesGui::ColourSchemeCategory::Type, GPlatesGui::ColourSchemeContainer::id_type> curr_colour_scheme =
		*d_view_state_colour_scheme_delegator->get_colour_scheme();
	load_category(curr_colour_scheme.first);

	// Listen in to notifications from the feature store root to find out new FCs.
	d_feature_store_root.attach_callback(
			new AddFeatureCollectionCallback(feature_collections_combobox));
	
	// Move the splitter as far left as possible without collapsing the left side
	QList<int> sizes;
	sizes.append(1);
	sizes.append(this->width());
	splitter->setSizes(sizes);
	
	make_signal_slot_connections();

	categories_table->setFocus();
}


void
GPlatesQtWidgets::ColouringDialog::set_categories_table_active_palette()
{
	QPalette categories_table_palette = categories_table->palette();
	categories_table_palette.setColor(
			QPalette::Disabled /* cells are not editable, so by default they get painted as disabled, which looks ugly */,
			QPalette::Text,
			categories_table_palette.color(QPalette::Active, QPalette::Text));
	categories_table->setPalette(categories_table_palette);
}


void
GPlatesQtWidgets::ColouringDialog::set_categories_table_inactive_palette()
{
	categories_table->setPalette(d_categories_table_original_palette);
}


void
GPlatesQtWidgets::ColouringDialog::reposition()
{
	// Reposition to halfway down the right side of the parent window.
	QWidget *par = parentWidget();
	if (par)
	{
		int new_x = par->pos().x() + par->frameGeometry().width();
		int new_y = par->pos().y() + (par->frameGeometry().height() - frameGeometry().height()) / 2;

		// Ensure the dialog is not off-screen.
		QDesktopWidget *desktop = QApplication::desktop();
		QRect screen_geometry = desktop->screenGeometry(par);
		if (new_x + frameGeometry().width() > screen_geometry.right())
		{
			new_x = screen_geometry.right() - frameGeometry().width();
		}
		if (new_y + frameGeometry().height() > screen_geometry.bottom())
		{
			new_y = screen_geometry.bottom() - frameGeometry().height();
		}

		move(new_x, new_y);
	}
}


void
GPlatesQtWidgets::ColouringDialog::populate_colour_scheme_categories()
{
	using namespace GPlatesGui::ColourSchemeCategory;

	categories_table->setRowCount(NUM_CATEGORIES);
	int row = 0;
	BOOST_FOREACH(Type category, std::make_pair(begin(), end()))
	{
		QTableWidgetItem *item = new QTableWidgetItem(get_description(category));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		categories_table->setItem(row, 0, item);
		++row;
	}
}


void
GPlatesQtWidgets::ColouringDialog::populate_feature_collections()
{
	// Note that we store a weak-ref to the feature collection as the combobox item userData.
	
	// First, we add a special entry for "all feature collections", to allow the
	// user to change the colour scheme for all feature collections without a
	// special colour scheme chosen.
	feature_collections_combobox->addItem(
			"(All)",
			QVariant(GPlatesModel::FeatureCollectionHandle::const_weak_ref()));

	// Get the present feature collections from the feature store root.
	typedef GPlatesModel::FeatureStoreRootHandle::const_iterator iterator_type;
	for (iterator_type iter = d_feature_store_root->begin(); iter != d_feature_store_root->end(); ++iter)
	{
		add_feature_collection_to_combobox(
				(*iter)->reference(),
				feature_collections_combobox);
	}
}


void
GPlatesQtWidgets::ColouringDialog::load_category(
		GPlatesGui::ColourSchemeCategory::Type category,
		int id_to_select)
{
	// Clear the list before populating it.
	colour_schemes_list->clear();

	// Ensure that the categories_table is in sync with the given category but
	// let's unhook signals before we do anything.
	QObject::disconnect(
			categories_table,
			SIGNAL(currentCellChanged(int, int, int, int)),
			this,
			SLOT(handle_categories_table_cell_changed(int, int, int, int)));
	categories_table->setCurrentCell(category, 0);
	QObject::connect(
			categories_table,
			SIGNAL(currentCellChanged(int, int, int, int)),
			this,
			SLOT(handle_categories_table_cell_changed(int, int, int, int)));

	// Remember the category.
	d_current_colour_scheme_category = category;

	typedef GPlatesGui::ColourSchemeContainer::iterator::value_type value_type;
	BOOST_FOREACH(
			const value_type &colour_scheme,
			std::make_pair(
					d_colour_scheme_container.begin(category),
					d_colour_scheme_container.end(category)))
	{
		GPlatesGui::ColourSchemeContainer::id_type id = colour_scheme.first;
		const GPlatesGui::ColourSchemeInfo &colour_scheme_info = colour_scheme.second;

		insert_list_widget_item(colour_scheme_info, id);

		if (static_cast<int>(id) == id_to_select)
		{
			colour_schemes_list->setCurrentRow(colour_schemes_list->count() - 1);
		}
	}
	
	// If the user selects a new category, automatically select the first colour
	// scheme in that category if there is one.
	if (id_to_select == -1 && colour_schemes_list->count())
	{
		colour_schemes_list->setCurrentRow(0);
	}

	// Change the "Open" button to "Add" for Single Colour category.
	if (category == GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR)
	{
		open_button->setText("Add...");
	}
	else
	{
		open_button->setText("Open...");
	}
	
	// FIXME: For now, hide "Open" and "Remove" for Plate ID and Feature Type
	// because we can't read categorical CPT files yet.
	if (category == GPlatesGui::ColourSchemeCategory::PLATE_ID ||
			category == GPlatesGui::ColourSchemeCategory::FEATURE_TYPE)
	{
		open_button->hide();
		remove_button->hide();
	}
	else
	{
		open_button->show();
		remove_button->show();
	}

	// Set the rendering chain in motion.
	if (d_show_thumbnails)
	{
		start_rendering_from(0);
	}
}


void
GPlatesQtWidgets::ColouringDialog::insert_list_widget_item(
		const GPlatesGui::ColourSchemeInfo &colour_scheme_info,
		GPlatesGui::ColourSchemeContainer::id_type id)
{
	QListWidgetItem *item = new QListWidgetItem(
			d_blank_icon,
			colour_scheme_info.short_description,
			colour_schemes_list);
	item->setToolTip(colour_scheme_info.long_description);
	QVariant qv;
	qv.setValue(id); // Store the colour scheme ID in the item data.
	item->setData(Qt::UserRole, qv);
	colour_schemes_list->addItem(item);
}


void
GPlatesQtWidgets::ColouringDialog::dragEnterEvent(
		QDragEnterEvent *ev)
{
	// We don't support any dropping of files for the Single Colour mode.
	if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR) {
		ev->ignore();
		return;
	}
	
	// OK, user wants to drop something. Does it have .cpt files?
	if (ev->mimeData()->hasUrls()) {
		QStringList cpts = extract_colour_palette_pathnames_from_file_urls(ev->mimeData()->urls());
		if ( ! cpts.isEmpty()) {
			ev->acceptProposedAction();
		} else {
			ev->ignore();
		}
	} else {
		ev->ignore();
	}
}

void
GPlatesQtWidgets::ColouringDialog::dropEvent(
		QDropEvent *ev)
{
	// OK, user is dropping something. Does it have .cpt files?
	if (ev->mimeData()->hasUrls()) {
		QStringList cpts = extract_colour_palette_pathnames_from_file_urls(ev->mimeData()->urls());
		if ( ! cpts.isEmpty()) {
			ev->acceptProposedAction();
			open_files(cpts);
		} else {
			ev->ignore();
		}
	} else {
		ev->ignore();
	}
}


void
GPlatesQtWidgets::ColouringDialog::start_rendering_from(
		int list_index)
{
	if (d_show_thumbnails)
	{
		if (list_index < colour_schemes_list->count())
		{
			// Load the first colour scheme.
			load_colour_scheme_from(colour_schemes_list->item(list_index));
			d_next_icon_to_render = list_index;

			// Show the GlobeAndMapWidget and refresh it.
			d_globe_and_map_widget_ptr->show();
			d_globe_and_map_widget_ptr->update_canvas();
		}
	}
	else
	{
		// Set all icons to d_blank_icon.
		for (int i = 0; i < colour_schemes_list->count(); ++i)
		{
			colour_schemes_list->item(i)->setIcon(d_blank_icon);
		}
	}
}


void
GPlatesQtWidgets::ColouringDialog::load_colour_scheme_from(
		QListWidgetItem *item)
{
	QVariant qv = item->data(Qt::UserRole);
	GPlatesGui::ColourSchemeContainer::id_type id = qv.value<GPlatesGui::ColourSchemeContainer::id_type>();
	d_preview_colour_scheme->set_preview_colour_scheme(
			d_colour_scheme_container.get(d_current_colour_scheme_category, id).colour_scheme_ptr,
			d_current_feature_collection);
}


void
GPlatesQtWidgets::ColouringDialog::handle_close_button_clicked(
		bool checked)
{
	hide();
}


void
GPlatesQtWidgets::ColouringDialog::handle_open_button_clicked(
		bool checked)
{
	if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR)
	{
		add_single_colour();
	}
	else
	{
		open_file();
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_remove_button_clicked(
		bool checked)
{
	// Change colour scheme to first in category.
	int row_to_remove = colour_schemes_list->currentRow();
	colour_schemes_list->setCurrentRow(0);
	QListWidgetItem *current_item = colour_schemes_list->takeItem(row_to_remove);

	// Remove item from list.
	QVariant qv = current_item->data(Qt::UserRole);
	GPlatesGui::ColourSchemeContainer::id_type id = qv.value<GPlatesGui::ColourSchemeContainer::id_type>();
	delete current_item; // takeItem hands over ownership of the pointer.

	// Remove from container.
	d_colour_scheme_container.remove(d_current_colour_scheme_category, id);
}


void
GPlatesQtWidgets::ColouringDialog::open_file()
{
	if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::PLATE_ID ||
			d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::FEATURE_TYPE)
	{
		QStringList file_list = QFileDialog::getOpenFileNames(
				this,
				"Open Files",
				QString(),
				"Categorical CPT file (*.cpt)");
		open_files(file_list);
	}
	else if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::FEATURE_AGE)
	{
		QStringList file_list = QFileDialog::getOpenFileNames(
				this,
				"Open Files",
				QString(),
				"Regular CPT file (*.cpt)");
		open_files(file_list);
	}
}


void
GPlatesQtWidgets::ColouringDialog::open_files(
		const QStringList &file_list)
{
	// NOTE: Yeah, this duplicates the logic of which Open dialog to show. Maybe it could be
	// put in an anon namespace function or something.
	if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::PLATE_ID ||
			d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::FEATURE_TYPE)
	{
		open_categorical_cpt_file(file_list);
	}
	else if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::FEATURE_AGE)
	{
		open_regular_cpt_file(file_list);
	}
}


void
GPlatesQtWidgets::ColouringDialog::open_regular_cpt_file(
		const QStringList &file_list)
{
	if (file_list.count() == 0)
	{
		return;
	}

	int first_index_in_list = -1;

	GPlatesFileIO::RegularCptReader reader;
	BOOST_FOREACH(QString file, file_list)
	{
		try
		{
			GPlatesGui::GenericContinuousColourPalette *cpt = reader.read_file(file);
			GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme =
				GPlatesGui::ColourSchemeFactory::create_custom_age_colour_scheme(
						d_application_state, cpt);

			QFileInfo file_info(file);

			GPlatesGui::ColourSchemeInfo cpt_info(
					colour_scheme,
					file_info.fileName(),
					file_info.absoluteFilePath(),
					false /* not built-in */);
			GPlatesGui::ColourSchemeContainer::id_type id = d_colour_scheme_container.add(
					d_current_colour_scheme_category, cpt_info);

			insert_list_widget_item(cpt_info, id);
			if (first_index_in_list == -1)
			{
				first_index_in_list = colour_schemes_list->count() - 1;
			}
		}
		catch (const GPlatesFileIO::ErrorReadingCptFileException &err)
		{
			QMessageBox::critical(this, "Error", err.message);
		}
	}

	if (first_index_in_list != -1)
	{
		start_rendering_from(first_index_in_list);
		colour_schemes_list->setCurrentRow(colour_schemes_list->count() - 1);
	}
}


void
GPlatesQtWidgets::ColouringDialog::open_categorical_cpt_file(
		const QStringList &file_list)
{
	if (file_list.count() == 0)
	{
		return;
	}
}


void
GPlatesQtWidgets::ColouringDialog::add_single_colour()
{
	QColor selected_colour = QColorDialog::getColor(d_last_single_colour, this);
	if (selected_colour.isValid())
	{
		d_last_single_colour = selected_colour;
		GPlatesGui::ColourSchemeContainer::id_type id = d_colour_scheme_container.add_single_colour_scheme(
				selected_colour,
				selected_colour.name(),
				false /* not built-in */);

		// Add an item in the list and render its icon.
		const GPlatesGui::ColourSchemeInfo &new_colour_scheme = d_colour_scheme_container.get(
				GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR, id);
		insert_list_widget_item(new_colour_scheme, id);
		start_rendering_from(colour_schemes_list->count() - 1);
		colour_schemes_list->setCurrentRow(colour_schemes_list->count() - 1);
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_main_repaint(
		bool mouse_down)
{
	if (!mouse_down)
	{
		if (d_suppress_next_repaint)
		{
			d_suppress_next_repaint = false;
			return;
		}

		if (d_next_icon_to_render == -1)
		{
			start_rendering_from(0);
		}
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_repaint(
		bool mouse_down)
{
	colour_schemes_list->item(d_next_icon_to_render)->setIcon(
			QIcon(
				QPixmap::fromImage(
					d_globe_and_map_widget_ptr->grab_frame_buffer())));
	++d_next_icon_to_render;

	if (d_next_icon_to_render < colour_schemes_list->count())
	{
		// Load the next colour scheme.
		load_colour_scheme_from(colour_schemes_list->item(d_next_icon_to_render));

		// Refresh.
		d_globe_and_map_widget_ptr->update_canvas();
	}
	else
	{
		// We're done with rendering; hide for now.
		d_globe_and_map_widget_ptr->hide();
		d_next_icon_to_render = -1;
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_categories_table_cell_changed(
		int current_row,
		int current_column,
		int previous_row,
		int previous_column)
{
	if (current_row != previous_row)
	{
		load_category(static_cast<GPlatesGui::ColourSchemeCategory::Type>(current_row));
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_colour_schemes_list_selection_changed()
{
	boost::optional<GPlatesGui::ColourSchemeDelegator::colour_scheme_handle> current_active_colour_scheme_opt =
		d_view_state_colour_scheme_delegator->get_colour_scheme(
				d_current_feature_collection);
	if (!current_active_colour_scheme_opt)
	{
		return;
	}

	GPlatesGui::ColourSchemeDelegator::colour_scheme_handle current_active_colour_scheme =
		*current_active_colour_scheme_opt;
	if (colour_schemes_list->count())
	{
		QListWidgetItem *current_item = colour_schemes_list->currentItem();
		if (current_item)
		{
			QVariant qv = current_item->data(Qt::UserRole);
			GPlatesGui::ColourSchemeContainer::id_type id =
				qv.value<GPlatesGui::ColourSchemeContainer::id_type>();
			if (id == current_active_colour_scheme.second)
			{
				// All of this is a horrible round-about way of making sure that the
				// current selection in the list widget can't get deselected without
				// another selection being made - this is possible on Linux if the user
				// clicks on the white space outside of any icon.
				colour_schemes_list->setCurrentItem(current_item);
			}
			else
			{
				// Selection's changed, so we better tell the colour scheme delegator.
				d_view_state_colour_scheme_delegator->set_colour_scheme(
						d_current_colour_scheme_category,
						id,
						d_current_feature_collection);

				// There is no need to repaint the previews when we actually go and change
				// the colour scheme (by definition of a preview).
				d_suppress_next_repaint = true;
			}

			// Enable or disable the Remove button depending on whether the colour scheme
			// is built in or not.
			const GPlatesGui::ColourSchemeInfo &colour_scheme_info =
				d_colour_scheme_container.get(d_current_colour_scheme_category, id);
			remove_button->setEnabled(!colour_scheme_info.is_built_in);
		}
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_colour_schemes_list_item_double_clicked(
		QListWidgetItem *item)
{
	// If a non-built-in colour scheme is double clicked, the user can edit the colour.
	if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR)
	{
		QVariant qv = item->data(Qt::UserRole);
		GPlatesGui::ColourSchemeContainer::id_type id = qv.value<GPlatesGui::ColourSchemeContainer::id_type>();
		
		const GPlatesGui::ColourSchemeInfo &colour_scheme_info = d_colour_scheme_container.get(
				GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR, id);
		if (!colour_scheme_info.is_built_in)
		{
			GPlatesGui::SingleColourScheme *colour_scheme_ptr =
				dynamic_cast<GPlatesGui::SingleColourScheme *>(colour_scheme_info.colour_scheme_ptr.get());
			if (colour_scheme_ptr)
			{
				boost::optional<GPlatesGui::Colour> original_colour = colour_scheme_ptr->get_colour();
				QColor selected_colour = QColorDialog::getColor(
						original_colour ? *original_colour : GPlatesGui::Colour::get_white(),
						this);
				if (selected_colour.isValid())
				{
					d_colour_scheme_container.edit_single_colour_scheme(
							id,
							selected_colour,
							selected_colour.name());

					// colour_scheme_info should now be modified.
					item->setText(colour_scheme_info.short_description);
					item->setToolTip(colour_scheme_info.long_description);
				}
			}
		}
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_show_thumbnails_changed(
		int state)
{
	bool new_state = (state == Qt::Checked);
	if (new_state != d_show_thumbnails)
	{
		d_show_thumbnails = new_state;
		if (colour_schemes_list->count())
		{
			start_rendering_from(0);
		}
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_feature_collections_combobox_index_changed(
		int index)
{
	QVariant qv = feature_collections_combobox->itemData(index);
	d_current_feature_collection = qv.value<GPlatesModel::FeatureCollectionHandle::const_weak_ref>();

	use_global_checkbox->setEnabled(d_current_feature_collection);

	// See whether the feature collection chosen has a special colour scheme set.
	// Note that the following logic works even if the user selected 'All'.
	boost::optional<GPlatesGui::ColourSchemeDelegator::colour_scheme_handle> colour_scheme_handle =
		d_view_state_colour_scheme_delegator->get_colour_scheme(d_current_feature_collection);
	if (colour_scheme_handle)
	{
		// Yes, there is a special colour scheme for this feature collection.
		use_global_checkbox->setCheckState(Qt::Unchecked);

		splitter->setEnabled(true);
		set_categories_table_active_palette();

		GPlatesGui::ColourSchemeCategory::Type category = colour_scheme_handle->first;
		GPlatesGui::ColourSchemeContainer::id_type id = colour_scheme_handle->second;
		load_category(category, static_cast<int>(id));
	}
	else
	{
		// No, there isn't a special colour scheme for this feature collection.
		use_global_checkbox->setCheckState(Qt::Checked);

		splitter->setEnabled(false);
		set_categories_table_inactive_palette();
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_use_global_changed(
		int state)
{
	if (state == Qt::Checked)
	{
		// Unset the special colour scheme for this feature collection.
		d_view_state_colour_scheme_delegator->unset_colour_scheme(d_current_feature_collection);
	}
	else
	{
		// Give the feature collection a special colour scheme, use plate id as default.
		categories_table->setCurrentCell(0, 0);
		colour_schemes_list->setCurrentRow(0);
		d_view_state_colour_scheme_delegator->set_colour_scheme(
				GPlatesGui::ColourSchemeCategory::PLATE_ID, 0,
				d_current_feature_collection);
	}

	// Force a refresh of the dialog's contents.
	handle_feature_collections_combobox_index_changed(feature_collections_combobox->currentIndex());
}


void
GPlatesQtWidgets::ColouringDialog::make_signal_slot_connections()
{
	// Close button.
	QObject::connect(
			close_button,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_close_button_clicked(bool)));

	// Open/Add button.
	QObject::connect(
			open_button,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_open_button_clicked(bool)));

	// Remove button.
	QObject::connect(
			remove_button,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_remove_button_clicked(bool)));

	// Refreshing the previews.
	QObject::connect(
			d_existing_globe_and_map_widget_ptr,
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_main_repaint(bool)));
	QObject::connect(
			d_globe_and_map_widget_ptr,
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_repaint(bool)));

	// Categories table.
	QObject::connect(
			categories_table,
			SIGNAL(currentCellChanged(int, int, int, int)),
			this,
			SLOT(handle_categories_table_cell_changed(int, int, int, int)));

	// Colour schemes list.
	QObject::connect(
			colour_schemes_list,
			SIGNAL(itemSelectionChanged()),
			this,
			SLOT(handle_colour_schemes_list_selection_changed()));
	QObject::connect(
			colour_schemes_list,
			SIGNAL(itemDoubleClicked(QListWidgetItem *)),
			this,
			SLOT(handle_colour_schemes_list_item_double_clicked(QListWidgetItem *)));

	// Show thumbnails checkbox.
	QObject::connect(
			show_thumbnails_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_show_thumbnails_changed(int)));

	// Feature collection combobox.
	QObject::connect(
			feature_collections_combobox,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(handle_feature_collections_combobox_index_changed(int)));

	// Use global checkbox.
	QObject::connect(
			use_global_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_use_global_changed(int)));
}


GPlatesQtWidgets::ColouringDialog::PreviewColourScheme::PreviewColourScheme(
		GPlatesGui::ColourSchemeDelegator::non_null_ptr_type view_state_colour_scheme_delegator) :
	d_view_state_colour_scheme_delegator(view_state_colour_scheme_delegator),
	d_altered_feature_collection()
{
}


void
GPlatesQtWidgets::ColouringDialog::PreviewColourScheme::set_preview_colour_scheme(
		GPlatesGui::ColourScheme::non_null_ptr_type preview_colour_scheme,
		GPlatesModel::FeatureCollectionHandle::const_weak_ref altered_feature_collection)
{
	d_altered_feature_collection = altered_feature_collection;
	d_preview_colour_scheme = preview_colour_scheme;
}


boost::optional<GPlatesGui::Colour>
GPlatesQtWidgets::ColouringDialog::PreviewColourScheme::get_colour(
		const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
{
	if (!d_preview_colour_scheme)
	{
		return d_view_state_colour_scheme_delegator->get_colour(reconstruction_geometry);
	}

	// Find the feature collection from which the reconstruction_geometry was created.
	GPlatesModel::FeatureHandle::weak_ref feature_ref;
	GPlatesModel::FeatureCollectionHandle *feature_collection_ptr =
		GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
			&reconstruction_geometry, feature_ref) ?
		feature_ref->parent_ptr() :
		NULL;

	if (d_altered_feature_collection)
	{
		// We're previewing a colour scheme for a particular feature collection.
		if (d_altered_feature_collection.handle_ptr() == feature_collection_ptr)
		{
			return (*d_preview_colour_scheme)->get_colour(reconstruction_geometry);
		}
		else
		{
			return d_view_state_colour_scheme_delegator->get_colour(reconstruction_geometry);
		}
	}
	else
	{
		// We're previewing the global colour scheme.
		if (feature_collection_ptr &&
				d_view_state_colour_scheme_delegator->get_colour_scheme(
					feature_collection_ptr->reference()))
		{
			// ViewState's ColourSchemeDelegator has a special colour scheme set for
			// this feature collection, so use it.
			return d_view_state_colour_scheme_delegator->get_colour(reconstruction_geometry);
		}
		else
		{
			// ViewState's ColourSchemeDelegator would colour this using the global
			// colour scheme, so let's colour it with our preview of the global colour
			// scheme.
			return (*d_preview_colour_scheme)->get_colour(reconstruction_geometry);
		}
	}
}
