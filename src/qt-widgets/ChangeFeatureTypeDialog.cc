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

#include <algorithm>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextDocument>
#include <QLabel>
#include <QPalette>

#include "ChangeFeatureTypeDialog.h"

#include "ChangeGeometryPropertyWidget.h"
#include "ChooseFeatureTypeWidget.h"
#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"

#include "file-io/FeaturePropertiesMap.h"

#include "gui/FeatureFocus.h"

#include "model/GPGIMInfo.h"

#include "utils/UnicodeStringUtils.h"


GPlatesQtWidgets::ChangeFeatureTypeDialog::ChangeFeatureTypeDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesGui::FeatureFocus &feature_focus,
		QWidget *parent_) :
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_application_state(application_state),
	d_feature_focus(feature_focus),
	d_new_feature_type_widget(
			new ChooseFeatureTypeWidget(
				SelectionWidget::Q_COMBO_BOX,
				this)),
	d_widget_container(new QWidget(this)),
	d_widget_container_layout(new QVBoxLayout(d_widget_container)),
	d_invalid_properties_widget(new InvalidPropertiesWidget(this)),
	d_num_active_widgets(0)
{
	setupUi(this);

	// Set up the widget for choosing the new feature type.
	QtWidgetUtils::add_widget_to_placeholder(
			d_new_feature_type_widget, new_feature_type_placeholder_widget);
	QObject::connect(
			d_new_feature_type_widget,
			SIGNAL(current_index_changed(boost::optional<GPlatesModel::FeatureType>)),
			this,
			SLOT(handle_feature_type_changed(boost::optional<GPlatesModel::FeatureType>)));

	// Inside the scroll area, there is (from top to bottom):
	//  - A widget holding all the ChangeGeometryPropertyWidgets; this widget has
	//    the layout d_widget_container_layout; and
	//  - A widget showing the invalid non-geometric properties.
	QWidget *scrollarea_widget = new QWidget(this);
	main_scrollarea->setWidget(scrollarea_widget);

	QLabel *invalid_geometric_properties_label = new QLabel(
			tr("Please reassign the following geometric properties that are invalid for the new feature type:"),
			this);
	invalid_geometric_properties_label->setWordWrap(true);
	d_widget_container_layout->addWidget(invalid_geometric_properties_label);

	QVBoxLayout *scrollarea_widget_layout = new QVBoxLayout(scrollarea_widget);
	scrollarea_widget_layout->setContentsMargins(0, 0, 0, 0);
	scrollarea_widget_layout->addWidget(d_widget_container);
	scrollarea_widget_layout->addWidget(d_invalid_properties_widget);
	scrollarea_widget_layout->addStretch();

	// ButtonBox signals.
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(change_feature_type()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));
}


void
GPlatesQtWidgets::ChangeFeatureTypeDialog::populate(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	d_feature_ref = feature_ref;

	if (feature_ref)
	{
		const GPlatesModel::FeatureType &feature_type = feature_ref->feature_type();
		d_new_feature_type_widget->populate(
				GPlatesModel::GPGIMInfo::is_topological(feature_type));
		d_new_feature_type_widget->set_feature_type(feature_type);
	}
}


void
GPlatesQtWidgets::ChangeFeatureTypeDialog::handle_feature_type_changed(
		boost::optional<GPlatesModel::FeatureType> feature_type_opt)
{
	QPushButton *ok_button = main_buttonbox->button(QDialogButtonBox::Ok);

	if (feature_type_opt && d_feature_ref.is_valid())
	{
		const GPlatesModel::FeatureType &new_feature_type = *feature_type_opt;

		// Only enable the OK button if the new feature type differs from the
		// feature's existing feature type.
		const GPlatesModel::FeatureType &existing_feature_type = d_feature_ref->feature_type();
		ok_button->setEnabled(new_feature_type != existing_feature_type);

		//
		// We iterate over the properties of the feature and:
		//  - If a property is a valid geometric property of the old feature type but
		//    not valid for the new feature type, show a ChangeGeometryPropertyWidget
		//    to the user, where they can change the property name.
		//  - Otherwise, if a non-geometric property is not valid for the new feature
		//    type, collect these and display in a list at the end of the dialog.
		//

		QStringList invalid_non_geometric_properties;

		// The next widget from the d_change_geometry_property_widget_pool to be used.
		std::vector<ChangeGeometryPropertyWidget *>::iterator next_widget =
			d_change_geometry_property_widget_pool.begin();

		for (GPlatesModel::FeatureHandle::iterator iter = d_feature_ref->begin();
				iter != d_feature_ref->end(); ++iter)
		{
			const GPlatesModel::TopLevelProperty &curr_top_level_property = **iter;
			const GPlatesModel::PropertyName &curr_property_name =
				curr_top_level_property.property_name();

			// Is it a geometric property for existing_feature_type but not a geometric
			// property for new_feature_type?
			if (GPlatesModel::GPGIMInfo::is_valid_geometric_property(
						existing_feature_type, curr_property_name))
			{
				if (!GPlatesModel::GPGIMInfo::is_valid_geometric_property(
							new_feature_type, curr_property_name))
				{
					if (next_widget == d_change_geometry_property_widget_pool.end())
					{
						// Pool doesn't have enough widgets; let's create a new one.
						ChangeGeometryPropertyWidget *new_widget =
							new ChangeGeometryPropertyWidget(d_feature_focus, this);
						d_change_geometry_property_widget_pool.push_back(new_widget);
						d_widget_container_layout->addWidget(new_widget);
						next_widget = d_change_geometry_property_widget_pool.end() - 1;
					}

					ChangeGeometryPropertyWidget &curr_widget = **next_widget;
					curr_widget.populate(d_feature_ref, iter, new_feature_type);
					curr_widget.setVisible(true);

					++next_widget;
				}
			}
			else // Non-geometric property for existing feature type.
			{
				const GPlatesFileIO::FeaturePropertiesMap &feature_properties_map =
					GPlatesFileIO::FeaturePropertiesMap::instance();

				if (!feature_properties_map.is_valid_property(
							new_feature_type, curr_property_name))
				{
					invalid_non_geometric_properties.push_back(
							GPlatesUtils::make_qstring_from_icu_string(
								curr_property_name.build_aliased_name()));
				}
			}
		}

		// Hide all widgets in the pool that weren't used this time.
		d_num_active_widgets = (next_widget - d_change_geometry_property_widget_pool.begin());
		if (next_widget == d_change_geometry_property_widget_pool.begin())
		{
			d_widget_container->hide();
		}
		else
		{
			d_widget_container->show();
			for ( ; next_widget != d_change_geometry_property_widget_pool.end(); ++next_widget)
			{
				(*next_widget)->setVisible(false);
			}
		}

		// Display the invalid non-geometric properties that we found.
		if (invalid_non_geometric_properties.size())
		{
			d_invalid_properties_widget->populate(invalid_non_geometric_properties);
		}
		d_invalid_properties_widget->setVisible(invalid_non_geometric_properties.size());
	}
	else
	{
		ok_button->setEnabled(false);
	}
}


void
GPlatesQtWidgets::ChangeFeatureTypeDialog::change_feature_type()
{
	if (d_feature_ref.is_valid())
	{
		// Change the feature type in the model.
		boost::optional<GPlatesModel::FeatureType> feature_type_opt =
			d_new_feature_type_widget->get_feature_type();
		if (!feature_type_opt)
		{
			return;
		}
		d_feature_ref->set_feature_type(*feature_type_opt);

		// Ask each of the subwidgets to change the property they were given.
		typedef std::vector<ChangeGeometryPropertyWidget *>::iterator widget_iterator_type;
		widget_iterator_type end = d_change_geometry_property_widget_pool.begin() + d_num_active_widgets;
		GPlatesModel::FeatureHandle::iterator new_focused_geometry_property;
		for (widget_iterator_type iter = d_change_geometry_property_widget_pool.begin();
				iter != end; ++iter)
		{
			(*iter)->process(new_focused_geometry_property);
		}

		// Perform a reconstruction before changing the focused geometry, so
		// that FeatureFocus can pick up the new reconstruction geometry.
		d_application_state.reconstruct();
		if (new_focused_geometry_property.is_still_valid())
		{
			d_feature_focus.set_focus(d_feature_ref, new_focused_geometry_property);
		}
	}

	accept();
}


GPlatesQtWidgets::ChangeFeatureTypeDialog::InvalidPropertiesWidget::InvalidPropertiesWidget(
		QWidget *parent_) :
	QWidget(parent_),
	d_invalid_properties_textedit(new QTextEdit(this))
{
	QVBoxLayout *this_layout = new QVBoxLayout(this);

	static const char *EXPLANATORY_TEXT = QT_TR_NOOP(
			"Please manually review the following non-geometric properties that are invalid "
			"for the new feature type:");
	QLabel *explanatory_label = new QLabel(this);
	explanatory_label->setText(tr(EXPLANATORY_TEXT));
	explanatory_label->setWordWrap(true);

	this_layout->addWidget(explanatory_label);
	this_layout->addWidget(d_invalid_properties_textedit);

	d_invalid_properties_textedit->setReadOnly(true);
	d_invalid_properties_textedit->setFrameStyle(QFrame::NoFrame);
	QPalette textedit_palette = d_invalid_properties_textedit->palette();
	textedit_palette.setColor(QPalette::Base, textedit_palette.color(QPalette::Window));
	d_invalid_properties_textedit->setPalette(textedit_palette);
	d_invalid_properties_textedit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}


void
GPlatesQtWidgets::ChangeFeatureTypeDialog::InvalidPropertiesWidget::populate(
		const QStringList &invalid_properties)
{
	QString joined_properties = invalid_properties.join("\n");

	// Deallocation of 'doc' is the responsibility of 'd_invalid_properties_textedit'.
	QTextDocument *doc = new QTextDocument(joined_properties, d_invalid_properties_textedit);
	d_invalid_properties_textedit->setDocument(doc);
	doc->adjustSize();
	
	// Resize the text edit so that the user doesn't need to scroll it.
	if (invalid_properties.count())
	{
		d_invalid_properties_textedit->show();
		d_invalid_properties_textedit->setMinimumHeight(static_cast<int>(doc->size().height()));
	}
	else
	{
		d_invalid_properties_textedit->hide();
	}
}

