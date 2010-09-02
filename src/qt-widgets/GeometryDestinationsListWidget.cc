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

#include "GeometryDestinationsListWidget.h"

#include "model/GPGIMInfo.h"


GPlatesQtWidgets::GeometryDestinationsListWidget::GeometryDestinationsListWidget(
		QWidget *parent_) :
	QListWidget(parent_)
{  }


GPlatesQtWidgets::PropertyNameItem *
GPlatesQtWidgets::GeometryDestinationsListWidget::get_current_property_name_item() const
{
	return static_cast<PropertyNameItem *>(currentItem());
}


void
GPlatesQtWidgets::GeometryDestinationsListWidget::populate(
		const GPlatesModel::FeatureType &target_feature_type)
{
	typedef GPlatesModel::GPGIMInfo::geometry_prop_name_map_type geometry_prop_name_map_type;
	static const geometry_prop_name_map_type geometry_prop_names =
			GPlatesModel::GPGIMInfo::get_geometry_prop_name_map();
	typedef GPlatesModel::GPGIMInfo::geometry_prop_timedependency_map_type geometry_prop_timedependency_map_type;
	static const geometry_prop_timedependency_map_type geometry_time_dependencies =
			GPlatesModel::GPGIMInfo::get_geometry_prop_timedependency_map();
	// FIXME: This list should ideally be dynamic, depending on:
	//  - the type of GeometryOnSphere we are given (e.g. gpml:position for gml:Point)
	//  - the type of feature the user has selected in the first list (since different
	//    feature types are supposed to have a different selection of valid properties)
	clear();
	// Iterate over the feature_type_info_table, and add all property names
	// that match our desired feature.
	typedef GPlatesModel::GPGIMInfo::feature_geometric_prop_map_type feature_geometric_prop_map_type;
	static const feature_geometric_prop_map_type map =
		GPlatesModel::GPGIMInfo::get_feature_geometric_prop_map();
	feature_geometric_prop_map_type::const_iterator it = map.lower_bound(target_feature_type);
	feature_geometric_prop_map_type::const_iterator end = map.upper_bound(target_feature_type);
	for ( ; it != end; ++it) {
		GPlatesModel::PropertyName prop = it->second;
		// Display name defaults to the QualifiedXmlName.
		QString display_name = GPlatesUtils::make_qstring_from_icu_string(prop.build_aliased_name());
		geometry_prop_name_map_type::const_iterator display_name_it = geometry_prop_names.find(prop);
		if (display_name_it != geometry_prop_names.end()) {
			display_name = display_name_it->second;
		}
		// Now we have to look up the time-dependent flag somewhere, too.
		bool expects_time_dependent_wrapper = true;
		geometry_prop_timedependency_map_type::const_iterator time_dependency_it = geometry_time_dependencies.find(prop);
		if (time_dependency_it != geometry_time_dependencies.end()) {
			expects_time_dependent_wrapper = time_dependency_it->second;
		}
		// Finally, add the item to the QListWidget.
		addItem(new PropertyNameItem(prop, display_name, expects_time_dependent_wrapper));
	}
	setCurrentRow(0);
}

