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

#include <boost/optional.hpp>
#include <QMetaType>

#include "ChooseGeometryPropertyWidget.h"

#include "QtWidgetUtils.h"

#include "model/GPGIMInfo.h"


namespace
{
	typedef boost::optional<GPlatesModel::PropertyName> user_data_type;
}


Q_DECLARE_METATYPE( user_data_type )


GPlatesQtWidgets::ChooseGeometryPropertyWidget::ChooseGeometryPropertyWidget(
		SelectionWidget::DisplayWidget display_widget,
		QWidget *parent_) :
	QWidget(parent_),
	d_selection_widget(new SelectionWidget(display_widget, this))
{
	QtWidgetUtils::add_widget_to_placeholder(d_selection_widget, this);

	QObject::connect(
			d_selection_widget,
			SIGNAL(item_activated(int)),
			this,
			SLOT(handle_item_activated(int)));
}


boost::optional<GPlatesModel::PropertyName>
GPlatesQtWidgets::ChooseGeometryPropertyWidget::get_property_name() const
{
	boost::optional<user_data_type> curr = d_selection_widget->get_data<user_data_type>(
			d_selection_widget->get_current_index());
	if (curr)
	{
		return *curr;
	}
	else
	{
		return boost::none;
	}
}


void
GPlatesQtWidgets::ChooseGeometryPropertyWidget::populate(
		const GPlatesModel::FeatureType &target_feature_type)
{
	// FIXME: This list should ideally be dynamic, depending on:
	//  - the type of GeometryOnSphere we are given (e.g. gpml:position for gml:Point)
	//  - the type of feature the user has selected in the first list (since different
	//    feature types are supposed to have a different selection of valid properties)

	d_selection_widget->clear();

	typedef GPlatesModel::GPGIMInfo::geometric_property_name_map_type geometric_property_name_map_type;
	static const geometric_property_name_map_type geometry_prop_names =
			GPlatesModel::GPGIMInfo::get_geometric_property_name_map();

	// Add all property names that match our desired feature.
	typedef GPlatesModel::GPGIMInfo::feature_geometric_property_map_type feature_geometric_property_map_type;
	static const feature_geometric_property_map_type map =
		GPlatesModel::GPGIMInfo::get_feature_geometric_property_map();
	feature_geometric_property_map_type::const_iterator it = map.lower_bound(target_feature_type);
	feature_geometric_property_map_type::const_iterator end = map.upper_bound(target_feature_type);
	for ( ; it != end; ++it)
	{
		GPlatesModel::PropertyName prop = it->second;

		// Display name defaults to the QualifiedXmlName.
		QString display_name;
		geometric_property_name_map_type::const_iterator display_name_it = geometry_prop_names.find(prop);
		if (display_name_it != geometry_prop_names.end())
		{
			display_name = display_name_it->second;
		}
		else
		{
			display_name = GPlatesUtils::make_qstring_from_icu_string(prop.build_aliased_name());
		}

		// Finally, add the item for display.
		d_selection_widget->add_item<user_data_type>(display_name, prop);
	}

	if (d_selection_widget->get_count())
	{
		d_selection_widget->set_current_index(0);
	}
}


void
GPlatesQtWidgets::ChooseGeometryPropertyWidget::handle_item_activated(
		int index)
{
	emit item_activated();
}

