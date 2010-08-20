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
#include <QPalette>
#include <QApplication>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaType>
#include <QPixmap>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QUrl>

#include "ColouringDialog.h"
#include "GlobeAndMapWidget.h"
#include "ReadErrorAccumulationDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/PropertyExtractors.h"

#include "file-io/CptReader.h"

#include "gui/Colour.h"
#include "gui/ColourSchemeContainer.h"
#include "gui/ColourSchemeInfo.h"
#include "gui/CptColourPalette.h"
#include "gui/GenericColourScheme.h"
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
		combobox->insertSeparator(combobox->count());
	}

	void
	remove_separator(
			QComboBox *combobox)
	{
		if (combobox->count() == 2)
		{
			combobox->removeItem(1);
		}
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
		BOOST_FOREACH(const QUrl &url, urls)
		{
			if (url.scheme() == "file")
			{
				QString path = url.toLocalFile();

				// Only accept .cpt files.
				if (path.endsWith(".cpt"))
				{
					pathnames.push_back(path);
				}
			}
		}
		return pathnames;	// QStringList implicitly shares data and uses pimpl idiom, return by value is fine.
	}

}  // anonymous namespace


GPlatesQtWidgets::ColouringDialog::ColouringDialog(
		GPlatesPresentation::ViewState &view_state,
		const GlobeAndMapWidget &existing_globe_and_map_widget,
		ReadErrorAccumulationDialog &read_error_accumulation_dialog,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_application_state(view_state.get_application_state()),
	d_existing_globe_and_map_widget_ptr(&existing_globe_and_map_widget),
	d_read_error_accumulation_dialog_ptr(&read_error_accumulation_dialog),
	d_colour_scheme_container(view_state.get_colour_scheme_container()),
	d_view_state_colour_scheme_delegator(view_state.get_colour_scheme_delegator()),
	d_preview_colour_scheme(
			new PreviewColourScheme(
				d_view_state_colour_scheme_delegator)),
	d_globe_and_map_widget_ptr(
			new GlobeAndMapWidget(
				d_existing_globe_and_map_widget_ptr,
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

	// Create the blank icon.
	QPixmap blank_pixmap(ICON_SIZE, ICON_SIZE);
	blank_pixmap.fill(*GPlatesGui::HTMLColourNames::instance().get_colour("slategray"));
	d_blank_icon = QIcon(blank_pixmap);

	// Set up our GlobeAndMapWidget that we use for rendering.
	d_globe_and_map_widget_ptr->resize(ICON_SIZE, ICON_SIZE);
	d_globe_and_map_widget_ptr->move(1 - ICON_SIZE, 1 - ICON_SIZE); // leave 1px showing

	// Set up the list of feature collections.
	populate_feature_collections();
	feature_collections_combobox->installEventFilter( this );

	// Set up the table of colour scheme categories.
	populate_colour_scheme_categories();
	categories_table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	categories_table->horizontalHeader()->hide();
	categories_table->verticalHeader()->hide();

	// Set up the list of colour schemes.
 	colour_schemes_list->setViewMode(QListWidget::IconMode);
 	colour_schemes_list->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
 //	colour_schemes_list->setSpacing(SPACING); //Due to a qt bug, the setSpacing doesn't work well in IconMode.
 	colour_schemes_list->setMovement(QListView::Static);
 	colour_schemes_list->setWrapping(true);
 	colour_schemes_list->setResizeMode(QListView::Adjust);
 	colour_schemes_list->setUniformItemSizes(true);
 	colour_schemes_list->setWordWrap(true);
	
	// Change the background colour of the right hand side.
	QPalette right_palette = right_side_frame->palette();
	right_palette.setColor(QPalette::Active, QPalette::Window, colour_schemes_list->palette().color(QPalette::Base));
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
GPlatesQtWidgets::ColouringDialog::repopulate_feature_collections()
{
	feature_collections_combobox->clear();
	// Note that we store a weak-ref to the feature collection as the combobox item userData.
	
	// First, we add a special entry for "all feature collections", to allow the
	// user to change the colour scheme for all feature collections without a
	// special colour scheme chosen.
	feature_collections_combobox->addItem(
			"(All)",
			QVariant(GPlatesModel::FeatureCollectionHandle::const_weak_ref()));

	GPlatesAppLogic::FeatureCollectionFileState& feature_collection_file_state =
	d_application_state.get_feature_collection_file_state();


	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_file_refs =
			feature_collection_file_state.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &loaded_file_ref,
			loaded_file_refs)
	{
		const GPlatesModel::FeatureCollectionHandle* fh = 
				loaded_file_ref.get_file().get_feature_collection().handle_ptr();
		QVariant qv;
		qv.setValue(GPlatesModel::FeatureCollectionHandle::const_weak_ref(*fh));
		feature_collections_combobox->addItem(
					loaded_file_ref.get_file().get_file_info().get_display_name(false),
					qv);
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

	// Show "Add" button for Single Colour category, "Open" button for every other category.
	// Also show "Edit" button only for Single Colour category.
	if (category == GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR)
	{
		open_button->hide();
		add_button->show();
		edit_button->show();
	}
	else
	{
		open_button->show();
		add_button->hide();
		edit_button->hide();
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
	if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR)
	{
		ev->ignore();
		return;
	}
	
	// OK, user wants to drop something. Does it have .cpt files?
	if (ev->mimeData()->hasUrls())
	{
		QStringList cpts = extract_colour_palette_pathnames_from_file_urls(ev->mimeData()->urls());
		if (!cpts.isEmpty())
		{
			ev->acceptProposedAction();
		}
		else
		{
			ev->ignore();
		}
	}
	else
	{
		ev->ignore();
	}
}

void
GPlatesQtWidgets::ColouringDialog::dropEvent(
		QDropEvent *ev)
{
	// OK, user is dropping something. Does it have .cpt files?
	if (ev->mimeData()->hasUrls())
	{
		QStringList cpts = extract_colour_palette_pathnames_from_file_urls(ev->mimeData()->urls());
		if (!cpts.isEmpty())
		{
			ev->acceptProposedAction();
			open_cpt_files(cpts);
		}
		else
		{
			ev->ignore();
		}
	}
	else
	{
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
	static const QString OPEN_DIALOG_TITLE = "Open Files";
	static const QString ALL_FILES = ";;All files (*)";
	static const QString REGULAR_FILTER = "Regular CPT files (*.cpt)" + ALL_FILES;
	static const QString CATEGORICAL_FILTER = "Categorical CPT files (*.cpt)" + ALL_FILES;
	static const QString UNIFIED_FILTER = "Regular or categorical CPT files (*.cpt)" + ALL_FILES;

	const QString *filter = NULL;

	switch (d_current_colour_scheme_category)
	{
		case GPlatesGui::ColourSchemeCategory::PLATE_ID:
			filter = &UNIFIED_FILTER;
			break;

		case GPlatesGui::ColourSchemeCategory::FEATURE_AGE:
			filter = &REGULAR_FILTER;
			break;

		case GPlatesGui::ColourSchemeCategory::FEATURE_TYPE:
			filter = &CATEGORICAL_FILTER;
			break;

		default:
			return;
	}

	QStringList file_list = QFileDialog::getOpenFileNames(
			this,
			OPEN_DIALOG_TITLE,
			QString() /* directory */,
			*filter);
	open_cpt_files(file_list);
}


void
GPlatesQtWidgets::ColouringDialog::open_cpt_files(
		const QStringList &file_list)
{
	if (file_list.count() == 0)
	{
		return;
	}

	switch (d_current_colour_scheme_category)
	{
		case GPlatesGui::ColourSchemeCategory::PLATE_ID:
			open_cpt_files<GPlatesFileIO::IntegerCptReader<int> >(
					file_list,
					GPlatesAppLogic::PropertyExtractorAdapter<
						GPlatesAppLogic::PlateIdPropertyExtractor, int>());
			break;

		case GPlatesGui::ColourSchemeCategory::FEATURE_AGE:
			open_cpt_files<GPlatesFileIO::RegularCptReader>(
					file_list,
					GPlatesAppLogic::AgePropertyExtractor(d_application_state));
			break;

		case GPlatesGui::ColourSchemeCategory::FEATURE_TYPE:
			open_cpt_files<GPlatesFileIO::CategoricalCptReader<GPlatesModel::FeatureType>::Type>(
					file_list,
					GPlatesAppLogic::FeatureTypePropertyExtractor());
			break;

		default:
			break;
	}
}


void
GPlatesQtWidgets::ColouringDialog::handle_add_button_clicked(
		bool checked)
{
	add_single_colour();
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


template<class CptReaderType, typename PropertyExtractorType>
void
GPlatesQtWidgets::ColouringDialog::open_cpt_files(
		const QStringList &file_list,
		const PropertyExtractorType &property_extractor)
{
	int first_index_in_list = -1;

	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_error_accumulation_dialog_ptr->read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();

	CptReaderType reader;
	BOOST_FOREACH(QString filename, file_list)
	{
		typedef typename CptReaderType::colour_palette_type colour_palette_type;
		typename colour_palette_type::maybe_null_ptr_type cpt = reader.read_file(filename, read_errors);

		// cpt can be NULL if the file yielded no parseable lines.
		if (cpt)
		{
			GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme =
				GPlatesGui::make_colour_scheme(
						cpt.get(),
						property_extractor);

			QFileInfo file_info(filename);

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
	}

	d_read_error_accumulation_dialog_ptr->update();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors)
	{
		d_read_error_accumulation_dialog_ptr->show();
	}

	if (first_index_in_list != -1)
	{
		start_rendering_from(first_index_in_list);
		colour_schemes_list->setCurrentRow(colour_schemes_list->count() - 1);
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

			// Also enable or disable the Edit button.
			edit_button->setEnabled(!colour_scheme_info.is_built_in);
		}
	}
}


void
GPlatesQtWidgets::ColouringDialog::edit_current_colour_scheme()
{
	// If a non-built-in colour scheme is double clicked, the user can edit the colour.
	if (d_current_colour_scheme_category == GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR)
	{
		QListWidgetItem *item = colour_schemes_list->currentItem();
		if (!item)
		{
			return;
		}

		QVariant qv = item->data(Qt::UserRole);
		GPlatesGui::ColourSchemeContainer::id_type id = qv.value<GPlatesGui::ColourSchemeContainer::id_type>();
		
		const GPlatesGui::ColourSchemeInfo &colour_scheme_info = d_colour_scheme_container.get(
				GPlatesGui::ColourSchemeCategory::SINGLE_COLOUR, id);
		if (colour_scheme_info.is_built_in)
		{
			return;
		}

		GPlatesGui::SingleColourScheme *colour_scheme_ptr =
			dynamic_cast<GPlatesGui::SingleColourScheme *>(colour_scheme_info.colour_scheme_ptr.get());
		if (!colour_scheme_ptr)
		{
			return;
		}

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

		GPlatesGui::ColourSchemeCategory::Type category = colour_scheme_handle->first;
		GPlatesGui::ColourSchemeContainer::id_type id = colour_scheme_handle->second;
		load_category(category, static_cast<int>(id));
	}
	else
	{
		// No, there isn't a special colour scheme for this feature collection.
		use_global_checkbox->setCheckState(Qt::Checked);

		splitter->setEnabled(false);
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


bool 
GPlatesQtWidgets::ColouringDialog::eventFilter( 
		QObject *o, 
		QEvent *e )
{
	if( o == feature_collections_combobox &&
		e->type() == QEvent::MouseButtonPress)
	{
		repopulate_feature_collections();
	}
	return false;
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

	// Open button.
	QObject::connect(
			open_button,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_open_button_clicked(bool)));

	// Add button.
	QObject::connect(
			add_button,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_add_button_clicked(bool)));

	// Edit button.
	QObject::connect(
			edit_button,
			SIGNAL(clicked(bool)),
			this,
			SLOT(edit_current_colour_scheme()));

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
			SLOT(edit_current_colour_scheme()));

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
		const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
{
	if (!d_preview_colour_scheme)
	{
		return d_view_state_colour_scheme_delegator->get_colour(reconstruction_geometry);
	}

	// Find the feature collection from which the reconstruction_geometry was created.
	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
					&reconstruction_geometry);
	GPlatesModel::FeatureCollectionHandle *feature_collection_ptr =
			feature_ref ? feature_ref.get()->parent_ptr() : NULL;

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

