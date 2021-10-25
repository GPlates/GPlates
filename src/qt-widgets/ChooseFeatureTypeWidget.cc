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
#include <QListWidget>
#include <QListWidgetItem>

#include "ChooseFeatureTypeWidget.h"

#include "QtWidgetUtils.h"

#include "model/Gpgim.h"

#include "utils/UnicodeStringUtils.h"

namespace
{
	class DefaultConstructibleFeatureType
	{
	public:

		DefaultConstructibleFeatureType()
		{  }

		DefaultConstructibleFeatureType(
				const GPlatesModel::FeatureType &feature_type) :
			d_feature_type(feature_type)
		{  }

		operator GPlatesModel::FeatureType() const
		{
			return *d_feature_type;
		}

		bool
		operator==(
				const DefaultConstructibleFeatureType &other)
		{
			return d_feature_type == other.d_feature_type;
		}

	private:

		boost::optional<GPlatesModel::FeatureType> d_feature_type;
	};
}


Q_DECLARE_METATYPE( DefaultConstructibleFeatureType )


GPlatesQtWidgets::ChooseFeatureTypeWidget::ChooseFeatureTypeWidget(
		const GPlatesModel::Gpgim &gpgim,
		SelectionWidget::DisplayWidget display_widget,
		QWidget *parent_) :
	QWidget(parent_),
	d_gpgim(gpgim),
	d_selection_widget(new SelectionWidget(display_widget, this))
{
	QtWidgetUtils::add_widget_to_placeholder(d_selection_widget, this);

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::populate(
		boost::optional<GPlatesPropertyValues::StructuralType> property_type)
{
	// Remember the current selection so we can re-select if it exists after re-populating.
	boost::optional<GPlatesModel::FeatureType> previously_selected_feature_type = get_feature_type();
	boost::optional<GPlatesModel::FeatureType> selected_feature_type;

	d_selection_widget->clear();

	const GPlatesModel::Gpgim::feature_type_seq_type &all_feature_types =
			d_gpgim.get_concrete_feature_types();

	// Iterate over all the feature types.
	BOOST_FOREACH(const GPlatesModel::FeatureType &feature_type, all_feature_types)
	{
		// Filter out feature types that don't have properties matching the target property type (if specified).
		if (property_type &&
			// Do any of the current feature's properties match the target property type ?
			!d_gpgim.get_feature_properties(feature_type, property_type.get()))
		{
			continue;
		}

		d_selection_widget->add_item<DefaultConstructibleFeatureType>(
				convert_qualified_xml_name_to_qstring(feature_type),
				feature_type);

		// Set the newly selected feature type if it matches previous selection (if there was any) and
		// the previous selection exists in the new list (ie, if we get here).
		if (!selected_feature_type &&
			previously_selected_feature_type &&
			previously_selected_feature_type.get() == feature_type)
		{
			selected_feature_type = previously_selected_feature_type.get();
		}
	}

	if (d_selection_widget->get_count())
	{
		if (selected_feature_type)
		{
			set_feature_type(selected_feature_type.get());
		}
		else
		{
			d_selection_widget->set_current_index(0);
		}
	}
}


boost::optional<GPlatesModel::FeatureType>
GPlatesQtWidgets::ChooseFeatureTypeWidget::get_feature_type() const
{
	return boost::optional<GPlatesModel::FeatureType>(
			d_selection_widget->get_data<DefaultConstructibleFeatureType>(
				d_selection_widget->get_current_index()));
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::set_feature_type(
		const GPlatesModel::FeatureType &feature_type)
{
	int index = d_selection_widget->find_data<DefaultConstructibleFeatureType>(feature_type);
	d_selection_widget->set_current_index(index);
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::focusInEvent(
		QFocusEvent *ev)
{
	d_selection_widget->setFocus();
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::handle_item_activated(
		int index)
{
	Q_EMIT item_activated();
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::handle_current_index_changed(
		int index)
{
	Q_EMIT current_index_changed(
			boost::optional<GPlatesModel::FeatureType>(
				d_selection_widget->get_data<DefaultConstructibleFeatureType>(index)));
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::make_signal_slot_connections()
{
	QObject::connect(
			d_selection_widget,
			SIGNAL(item_activated(int)),
			this,
			SLOT(handle_item_activated(int)));
	QObject::connect(
			d_selection_widget,
			SIGNAL(current_index_changed(int)),
			this,
			SLOT(handle_current_index_changed(int)));
}

