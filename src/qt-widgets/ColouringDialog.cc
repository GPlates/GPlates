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
#include "GlobeAndMapWidget.h"

#include "app-logic/ApplicationState.h"

#include "gui/Colour.h"
#include "gui/ColourSchemeFactory.h"

#include "model/WeakReferenceCallback.h"

#include "presentation/ViewState.h"

#include "utils/UnicodeStringUtils.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QColor>
#include <QDebug>
#include <QMetaType>


// This is to allow FeatureCollectionHandle::const_weak_ref to be stored in the
// userData of a QComboBox item, which is of type QVariant.
// The macro is defined in <QMetaType>.
Q_DECLARE_METATYPE(GPlatesModel::WeakReference<const GPlatesModel::FeatureCollectionHandle>)


namespace
{
	void
	add_single_colour_scheme(
			std::vector<GPlatesQtWidgets::ColourSchemeInfo> &colour_schemes,
			const GPlatesGui::Colour &colour,
			const QString &colour_name)
	{
		QString capitalised_colour_name = colour_name;
		capitalised_colour_name[0] = capitalised_colour_name[0].toUpper();

		colour_schemes.push_back(
				GPlatesQtWidgets::ColourSchemeInfo(
					GPlatesGui::ColourSchemeFactory::create_single_colour_scheme(colour),
					capitalised_colour_name,
					"Colours geometries in " + colour_name,
					true));
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
						d_combobox->removeItem(i);
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
		
		// Now, add it to the feature collection combobox.
		const boost::optional<UnicodeString> &filename_opt = feature_collection->filename();
		QString filename = filename_opt ?
			GPlatesUtils::make_qstring_from_icu_string(*filename_opt) :
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
}


GPlatesQtWidgets::ColouringDialog::ColouringDialog(
		GPlatesPresentation::ViewState &view_state,
		GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_view_state(view_state),
	d_palette_selection_widget_ptr(
			new PaletteSelectionWidget(
				view_state,
				existing_globe_and_map_widget_ptr)),
	d_feature_store_root(
			view_state.get_application_state().get_model_interface()->root()),
	d_label(NULL)
{
	setupUi(this);

	make_signal_slot_connections();

	create_colour_schemes();

	populate_feature_collections();

	// Listen in to notifications from the feature store root to find out new FCs.
	d_feature_store_root.attach_callback(
			new AddFeatureCollectionCallback(combobox_Feature_Collection));
	
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

	// For testing
	d_globe = new GlobeAndMapWidget(
		existing_globe_and_map_widget_ptr,
		GPlatesGui::ColourSchemeFactory::create_single_colour_scheme(QColor(255, 0, 0)),
		this);
	d_globe->resize(250, 250);
	d_globe->get_globe_canvas().update_canvas();
	d_globe->move(-249, 0);
	d_label = new QLabel(this);
	d_label->resize(250, 250);
	d_label->move(300, 300);

	QObject::connect(
			d_globe,
			SIGNAL(repainted(bool)),
			this,
			SLOT(handle_globe_repainted(bool)));
}


void
GPlatesQtWidgets::ColouringDialog::populate_feature_collections()
{
	// Note that we store a weak-ref to the feature collection as the combobox item userData.
	
	// First, we add a special entry for "all feature collections", to allow the
	// user to change the colour scheme for all feature collections without a
	// special colour scheme chosen.
	combobox_Feature_Collection->addItem(
			"(All feature collections)",
			QVariant(GPlatesModel::FeatureCollectionHandle::const_weak_ref()));

	// Get the present feature collections from the feature store root.
	typedef GPlatesModel::FeatureStoreRootHandle::const_iterator iterator_type;
	for (iterator_type iter = d_feature_store_root->begin(); iter != d_feature_store_root->end(); ++iter)
	{
		add_feature_collection_to_combobox(
				(*iter)->reference(),
				combobox_Feature_Collection);
	}
}


void
GPlatesQtWidgets::ColouringDialog::close_button_pressed()
{
	/*
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
	*/

	d_globe->get_globe_canvas().update_canvas();
}


void
GPlatesQtWidgets::ColouringDialog::handle_globe_repainted(bool)
{
	d_label->setPixmap(QPixmap::fromImage(d_globe->grab_frame_buffer()));
}


void
GPlatesQtWidgets::ColouringDialog::make_signal_slot_connections()
{
	QObject::connect(
			button_Close,
			SIGNAL(pressed()),
			this,
			SLOT(close_button_pressed()));
}


void
GPlatesQtWidgets::ColouringDialog::create_colour_schemes()
{
	// Plate ID colouring schemes:
	std::vector<ColourSchemeInfo> plate_id_schemes = d_colour_schemes[QString("Plate ID")];
	plate_id_schemes.push_back(
			ColourSchemeInfo(
				GPlatesGui::ColourSchemeFactory::create_default_plate_id_colour_scheme(),
				"Default",
				"Colours geometries in a manner that visually distinguishes nearby plates",
				true));
	plate_id_schemes.push_back(
			ColourSchemeInfo(
				GPlatesGui::ColourSchemeFactory::create_regional_plate_id_colour_scheme(),
				"Group by Region",
				"Colours geometries such that plates with the same leading digit are coloured similarly",
				true));

	// Single Colour colouring schemes:
	std::vector<ColourSchemeInfo> single_colour_schemes = d_colour_schemes[QString("Single Colour")];
	add_single_colour_scheme(
			single_colour_schemes,
			GPlatesGui::Colour::get_red(),
			"red");
	add_single_colour_scheme(
			single_colour_schemes,
			GPlatesGui::Colour::get_green(),
			"green");
	add_single_colour_scheme(
			single_colour_schemes,
			GPlatesGui::Colour::get_blue(),
			"blue");
	add_single_colour_scheme(
			single_colour_schemes,
			GPlatesGui::Colour::get_yellow(),
			"yellow");
	add_single_colour_scheme(
			single_colour_schemes,
			GPlatesGui::Colour::get_white(),
			"white");

	// Feature Type colour schemes:
	std::vector<ColourSchemeInfo> feature_type_schemes = d_colour_schemes[QString("Feature Type")];
	feature_type_schemes.push_back(
			ColourSchemeInfo(
				GPlatesGui::ColourSchemeFactory::create_default_feature_colour_scheme(),
				"Default",
				"Colours geometries by feature type",
				true));

	// Feature Age colour schemes:
	std::vector<ColourSchemeInfo> feature_age_schemes = d_colour_schemes[QString("Feature Age")];
	feature_age_schemes.push_back(
			ColourSchemeInfo(
				GPlatesGui::ColourSchemeFactory::create_default_age_colour_scheme(d_view_state.get_reconstruct()),
				"Default",
				"Colours geometries by their age based on the current reconstruction time",
				true));
}

