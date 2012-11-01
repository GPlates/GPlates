/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include <QHeaderView>
#include <QString>
#include <QDebug>
#include <vector>
#include "FeaturePropertyTableModel.h"

#include "model/types.h"
#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/ModelUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "feature-visitors/ToQvariantConverter.h"
#include "feature-visitors/FromQvariantConverter.h"


namespace
{
	/**
	 * Returns a simple representation of the first value of a TopLevelProperty.
	 * Note that returning a QVariant allows for the model/view architecture to supply
	 * QSpinboxes etc as appropriate (for an editable model)
	 */
	QVariant
	top_level_property_to_simple_qvariant(
			const GPlatesModel::TopLevelProperty &top_level_property,
			int role)
	{
		// For now, just test the actual feature - no modified cache yet.
		GPlatesFeatureVisitors::ToQvariantConverter qvariant_converter;
		qvariant_converter.set_desired_role(role);
		top_level_property.accept_visitor(qvariant_converter);
		
		if (qvariant_converter.found_values_begin() != qvariant_converter.found_values_end()) {
			// We were able to make a QVariant out of this property value.
			// FIXME: Note that we are only returning the first result, and not checking for multiple
			// matching property names.
			return *qvariant_converter.found_values_begin();
		} else {
			// The property exists, but we were unable to render it in a single cell.
			return QVariant();
		}
	}
	

	/**
	 * Returns a more verbose representation of a TopLevelProperty.
	 * Useful for debugging.
	 */
	QVariant
	top_level_property_to_verbose_qstring(
			const GPlatesModel::TopLevelProperty &top_level_property,
			int role)
	{
		if (role != Qt::DisplayRole) {
			return top_level_property_to_simple_qvariant(top_level_property, role);
		}
		
		GPlatesFeatureVisitors::ToQvariantConverter toqv_converter;
		top_level_property.accept_visitor(toqv_converter);
		QString str;
		if (toqv_converter.found_time_dependencies_begin() != toqv_converter.found_time_dependencies_end()) {
			GPlatesFeatureVisitors::ToQvariantConverter::qvariant_container_const_iterator it =
					toqv_converter.found_time_dependencies_begin();
			GPlatesFeatureVisitors::ToQvariantConverter::qvariant_container_const_iterator end =
					toqv_converter.found_time_dependencies_end();
			for ( ; it != end; ++it) {
				str.append((*it).toString());
				str.append(" ");
			}
			str.append(": ");
		}
		if (toqv_converter.found_values_begin() != toqv_converter.found_values_end()) {
			str.append("[");
			GPlatesFeatureVisitors::ToQvariantConverter::qvariant_container_const_iterator it =
					toqv_converter.found_values_begin();
			GPlatesFeatureVisitors::ToQvariantConverter::qvariant_container_const_iterator end =
					toqv_converter.found_values_end();
			for ( ; it != end; ++it) {
				str.append(" '");
				str.append((*it).toString());
				str.append("'");
			}
			str.append(" ]");
			return str;
		} else {
			return QString("[ Empty TopLevelProperty or unable to convert ]");
		}
	}

	/**
	 * This function is necessary to calculate the number of properties that
	 * are about to be added to the model, to work around a regression
	 * that affects QTableView in Qt version 4.3.0 (Trolltech Bug #169255).
	 */
	int
	calculate_number_of_properties(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		int count = 0;
		// Need to test weak-ref *before* we dereference it to get properties iterators.
		if ( ! feature_ref.is_valid()) {
			// Nothing can be done.
			return 0;
		}
		GPlatesModel::FeatureHandle::iterator it = feature_ref->begin();
		GPlatesModel::FeatureHandle::iterator end = feature_ref->end();
		for (; it != end; ++it)
		{
			++count;
		}
		return count;
	}
}


GPlatesGui::FeaturePropertyTableModel::FeaturePropertyTableModel(
		GPlatesGui::FeatureFocus &feature_focus,
		QObject *parent_):
	QAbstractTableModel(parent_),
	d_feature_focus(&feature_focus)
{  }


int
GPlatesGui::FeaturePropertyTableModel::rowCount(
		const QModelIndex &parent_) const
{
	return static_cast<int>(d_property_info_cache.size());
}


Qt::ItemFlags
GPlatesGui::FeaturePropertyTableModel::flags(
		const QModelIndex &idx) const
{
	if (!idx.isValid()) {
		return 0;
	}
	if (idx.row() < 0 || idx.row() >= rowCount()) {
		return 0;
	}

	if (idx.column() == 0) {
		// Property Name column is not editable.
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	} else {
		// Only return 'editable' if we know we can do so.
		// This is figured out in set_feature_reference().
		if (is_property_editable_inline(idx.row())) {
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
		} else {
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		}
	}
}


QVariant
GPlatesGui::FeaturePropertyTableModel::headerData(
		int section,
		Qt::Orientation orientation,
		int role) const
{
	if (orientation == Qt::Horizontal) {
		// No need to supply tooltip data, etc. for headers. We are only interested in a few roles.

		if (role == Qt::DisplayRole) {
			if (section == 0) {
				return QString(tr("Property"));
			} else {
				return QString(tr("Value"));
			}
		} else {
			return QVariant();
		}
		
	} else {
		// Vertical header; we can supply the property name here, too, though it can be switched
		// off at the view end.
		if (role == Qt::DisplayRole) {
			if (section < 0 || section >= rowCount()) {
				return QVariant();
			}
			return get_property_name_as_qvariant(section);
		} else {
			return QVariant();
		}
	}
}


QVariant
GPlatesGui::FeaturePropertyTableModel::data(
		const QModelIndex &idx,
		int role) const
{
	if (!idx.isValid()) {
		return QVariant();
	}
	if (idx.row() < 0 || idx.row() >= rowCount()) {
		return QVariant();
	}
	
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		if (idx.column() == 0) {
			// Property Name
			return get_property_name_as_qvariant(idx.row());
		} else {
			// Property Value
			return get_property_value_as_qvariant(idx.row(), role);
		}
	}
	return QVariant();
}


bool
GPlatesGui::FeaturePropertyTableModel::setData(
		const QModelIndex &idx,
		const QVariant &value,
		int role)
{
	if ( ! idx.isValid() || role != Qt::EditRole || idx.column() == 0) {
		return false;
	}
	
	GPlatesModel::FeatureHandle::iterator it = get_property_iterator_for_row(idx.row());
	if ( ! it.is_still_valid()) {
		// Always check your property iterators.
		return false;
	}
	
	// Convert the supplied QVariant to a PropertyValue.
	GPlatesFeatureVisitors::FromQvariantConverter fromqv_converter(value);
	GPlatesModel::TopLevelProperty::non_null_ptr_type top_level_prop_clone = (*it)->deep_clone();
	top_level_prop_clone->accept_visitor(fromqv_converter);
	*it = top_level_prop_clone;
	
	if (fromqv_converter.get_property_value()) {
		// Successful conversion. Now, assign the new value.
		GPlatesModel::PropertyValue::non_null_ptr_type new_value = *(fromqv_converter.get_property_value());

#if 0		
		// We are unable to actually do this right now, because assign_new_property_value
		// isn't smart enough to handle nested PropertyValues, i.e. anything we've wrapped
		// inside a ConstantValue. For now, the whole tablemodel is non-editable.
		bool success = assign_new_property_value(it, new_value);
#else
		bool success = false;
#endif
		if (success) {
			// Presence of the old property value implies a new one was set.
			// Tell the QTableView about it.
			emit dataChanged(idx, idx);
			return true;
		} else {
			// A failure to set the new property value.
			return false;
		}
	} else {
		// A failure to find the given property, or more likely, an incompatible property
		// value type.
		return false;
	}
}


void
GPlatesGui::FeaturePropertyTableModel::set_feature_reference(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// If we are given an invalid feature reference,
	// or the new feature reference is different to the previous one,
	// then we definitely want a clean slate before we refresh_data(), for consistency.
	if ( ! feature_ref.is_valid() || d_feature_ref != feature_ref) {
		clear_table();
	}
	d_feature_ref = feature_ref;
	refresh_data();
}

void
GPlatesGui::FeaturePropertyTableModel::clear_table()
{
	layoutAboutToBeChanged();
	// We also need to call beginRemoveRows() because of a QTableView regression in Qt 4.3.0.
	int rows_to_be_removed = static_cast<int>(d_property_info_cache.size());
	if (rows_to_be_removed > 0) {
		beginRemoveRows(QModelIndex(), 0, rows_to_be_removed - 1);
	}
	d_property_info_cache.clear();
	if (rows_to_be_removed > 0) {
		endRemoveRows();
	}
	layoutChanged();
}

void
GPlatesGui::FeaturePropertyTableModel::refresh_data()
{
	// Always check validity of weak_refs!
	if ( ! d_feature_ref.is_valid()) {
		return;
	}
	
	// Go through every cached property iterator we have, and see if it needs to be removed.
	property_info_container_iterator remove_it = d_property_info_cache.begin();
	property_info_container_iterator remove_end = d_property_info_cache.end();
	for ( ; remove_it != remove_end; /* iterator (sometimes) incremented at end of loop */ ) {
		bool element_was_erased = false;

		// Check the properties_iterator and the intrusive_ptr it refers to.
		if ( ! remove_it->property_iterator.is_still_valid()) {
			int row = get_row_for_property_iterator(remove_it->property_iterator);

			// Found an invalid property iterator.  Remove it from the table.
			beginRemoveRows(QModelIndex(), row, row);

			// Need to update the iterator after its element was erased, which
			// invalidates the iterator (right?).
			remove_it = d_property_info_cache.erase(remove_it);

			// Need to update the "end" iterator, since an element has been erased from
			// a contiguous-storage container (a std::vector), which invalidates any
			// iterators which point to the following elements (Josuttis 99, section
			// 6.2.2 "Vector Operations").
			remove_end = d_property_info_cache.end();

			endRemoveRows();
			element_was_erased = true;
		} /*else if (*remove_it->property_iterator == NULL) {
			// FIXME:  The above if-test NULL-check is not necessary.
			// The properties_iterator 'is_still_valid' test already checks for NULL.

			int row = get_row_for_property_iterator(remove_it->property_iterator);

			// Found a NULL intrusive_ptr.  Remove it from the table.
			beginRemoveRows(QModelIndex(), row, row);

			// Need to update the iterator after its element was erased, which
			// invalidates the iterator (right?).
			remove_it = d_property_info_cache.erase(remove_it);

			// Need to update the "end" iterator, since an element has been erased from
			// a contiguous-storage container (a std::vector), which invalidates any
			// iterators which point to the following elements (Josuttis 99, section
			// 6.2.2 "Vector Operations").
			remove_end = d_property_info_cache.end();

			endRemoveRows();
			element_was_erased = true;
		}*/

		// Only increment the iterator if it hasn't already been advanced as a result of
		// erasing an element.
		if ( ! element_was_erased) {
			++remove_it;
		}
	}

	// Go through every actual property iterator in the feature, and see if it needs
	// to be added to the table.
	GPlatesModel::FeatureHandle::iterator add_it = d_feature_ref->begin();
	GPlatesModel::FeatureHandle::iterator add_end = d_feature_ref->end();
	for (; add_it != add_end; ++add_it)
	{
		// Check the properties_iterator and the intrusive_ptr it refers to.
		// if (add_it.is_valid()) {
			int test_row = get_row_for_property_iterator(add_it);
			if (test_row == -1) {
				// We've found something not in the cache.
				
				int row_to_be_added = static_cast<int>(d_property_info_cache.size());
				beginInsertRows(QModelIndex(), row_to_be_added, row_to_be_added);
				
				const GPlatesModel::PropertyName property_name = (*add_it)->property_name();
				// To work out if property is editable inline, we do a dry-run of the FromQvariantConverter.
				QVariant dummy;
				GPlatesFeatureVisitors::FromQvariantConverter qvariant_converter(dummy);
				GPlatesModel::TopLevelProperty::non_null_ptr_type top_level_prop_clone = (*add_it)->deep_clone();
				top_level_prop_clone->accept_visitor(qvariant_converter);
				*add_it = top_level_prop_clone;
				bool can_convert_inline = qvariant_converter.get_property_value();
		
				FeaturePropertyTableInfo info = { property_name, add_it, can_convert_inline };
				d_property_info_cache.push_back(info);
				
				endInsertRows();
			}
		// }
	}

	// Update every single data cell because we just don't know what's changed and what hasn't.
	emit dataChanged(index(0, 1), index(static_cast<int>(d_property_info_cache.size()) - 1, 1));
}



const GPlatesModel::PropertyName
GPlatesGui::FeaturePropertyTableModel::get_property_name(
		int row) const
{
	const GPlatesModel::PropertyName property_name = d_property_info_cache.at(row).property_name;
	return property_name;
}


GPlatesModel::FeatureHandle::iterator
GPlatesGui::FeaturePropertyTableModel::get_property_iterator_for_row(
		int row) const
{
	GPlatesModel::FeatureHandle::iterator it = d_property_info_cache.at(row).property_iterator;
	return it;
}


int
GPlatesGui::FeaturePropertyTableModel::get_row_for_property_iterator(
		GPlatesModel::FeatureHandle::iterator property_iterator) const
{
	int row = 0;
	// Figure out which row of the table (if any) contains the property iterator.
	property_info_container_const_iterator it = d_property_info_cache.begin();
	property_info_container_const_iterator end = d_property_info_cache.end();
	for ( ; it != end; ++it, ++row)
	{
		if (it->property_iterator == property_iterator) {
			return row;
		}
	}
	return -1;
}


bool
GPlatesGui::FeaturePropertyTableModel::is_property_editable_inline(
		int row) const
{
#if 1
	// Because of Bug #77, we are now editing PropertyValues in-place via
	// the edit widgets, rather than creating new PropertyValues each time.
	// This was non-trivial to add to the various Edit*Widget classes,
	// but is much more difficult to add to FromQvariantConverter. As
	// a result, we are disabling edits of property values via table cells
	// until a better solution can be worked on.
	return false;
#else
	return d_property_info_cache.at(row).editable_inline;
#endif
}




QVariant
GPlatesGui::FeaturePropertyTableModel::get_property_name_as_qvariant(
		int row) const
{
	const GPlatesModel::PropertyName &property_name = get_property_name(row);
	return convert_qualified_xml_name_to_qstring(property_name);
}


QVariant
GPlatesGui::FeaturePropertyTableModel::get_property_value_as_qvariant(
		int row,
		int role) const
{
	GPlatesModel::FeatureHandle::iterator it = get_property_iterator_for_row(row);
	
	if ( ! it.is_still_valid()) {
		// Always check your property iterators.
		return QVariant("< NULL >");
	}

	return top_level_property_to_simple_qvariant(**it, role);
}





