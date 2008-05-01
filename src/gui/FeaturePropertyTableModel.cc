/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
#include <vector>
#include "FeaturePropertyTableModel.h"

#include "model/types.h"
#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "model/ModelUtils.h"
#include "model/DummyTransactionHandle.h"
#include "utils/UnicodeStringUtils.h"
#include "feature-visitors/ToQvariantConverter.h"
#include "feature-visitors/FromQvariantConverter.h"
#include "feature-visitors/PropertyValueSetter.h"


namespace
{
	QString
	property_value_to_qstring(
			const GPlatesModel::PropertyValue &property_value)
	{
		GPlatesFeatureVisitors::ToQvariantConverter toqv_converter;
		property_value.accept_visitor(toqv_converter);
		if (toqv_converter.found_qvariants_begin() != toqv_converter.found_qvariants_end()) {
			return (*toqv_converter.found_qvariants_begin()).toString();
		} else {
			return QString("<Unable to convert PropertyValue>");
		}
	}

	QString
	property_container_to_qstring(
			const GPlatesModel::PropertyContainer &property_container)
	{
		GPlatesFeatureVisitors::ToQvariantConverter toqv_converter;
		property_container.accept_visitor(toqv_converter);
		if (toqv_converter.found_qvariants_begin() != toqv_converter.found_qvariants_end()) {
			QString str = "[";
			GPlatesFeatureVisitors::ToQvariantConverter::qvariant_container_const_iterator it =
					toqv_converter.found_qvariants_begin();
			GPlatesFeatureVisitors::ToQvariantConverter::qvariant_container_const_iterator end =
					toqv_converter.found_qvariants_end();
			for ( ; it != end; ++it) {
				str.append(" '");
				str.append((*it).toString());
				str.append("'");
			}
			str.append(" ]");
			return str;
		} else {
			return QString("[ Empty PropertyContainer ]");
		}
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
	
	GPlatesModel::FeatureHandle::properties_iterator it = get_property_iterator_for_row(idx.row());
	if (*it == NULL) {
		// Always check your property iterators.
		return false;
	}
	
	// Convert the supplied QVariant to a PropertyValue.
	GPlatesFeatureVisitors::FromQvariantConverter fromqv_converter(value);
	(*it)->accept_visitor(fromqv_converter);
	
	if (fromqv_converter.get_property_value()) {
		// Successful conversion. Now, assign the new value.
		GPlatesModel::PropertyValue::non_null_ptr_type new_value = *(fromqv_converter.get_property_value());
		
		bool success = assign_new_property_value(it, new_value);
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
	d_feature_ref = feature_ref;
	
	layoutAboutToBeChanged();
	d_property_info_cache.clear();
	layoutChanged();
	
	// Always check validity of weak_refs!
	if ( ! d_feature_ref.is_valid()) {
		return;
	}

	layoutAboutToBeChanged();
	GPlatesModel::FeatureHandle::properties_iterator it = d_feature_ref->properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = d_feature_ref->properties_end();
	for (; it != end; ++it)
	{
		if (*it != NULL) {
			const GPlatesModel::PropertyName property_name = (*it)->property_name();
			// To work out if property is editable inline, we do a dry-run of the FromQvariantConverter.
			QVariant dummy;
			GPlatesFeatureVisitors::FromQvariantConverter qvariant_converter(dummy);
			(*it)->accept_visitor(qvariant_converter);
			bool can_convert_inline = qvariant_converter.get_property_value();
	
			FeaturePropertyTableInfo info = { property_name, it, can_convert_inline };
			d_property_info_cache.push_back(info);
		}
	}
	layoutChanged();
}



const GPlatesModel::PropertyName
GPlatesGui::FeaturePropertyTableModel::get_property_name(
		int row) const
{
	const GPlatesModel::PropertyName property_name = d_property_info_cache.at(row).property_name;
	return property_name;
}


GPlatesModel::FeatureHandle::properties_iterator
GPlatesGui::FeaturePropertyTableModel::get_property_iterator_for_row(
		int row) const
{
	GPlatesModel::FeatureHandle::properties_iterator it = d_property_info_cache.at(row).property_iterator;
	return it;
}


int
GPlatesGui::FeaturePropertyTableModel::get_row_for_property_iterator(
		GPlatesModel::FeatureHandle::properties_iterator property_iterator) const
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
	return d_property_info_cache.at(row).editable_inline;
}


bool
GPlatesGui::FeaturePropertyTableModel::assign_new_property_value(
		GPlatesModel::FeatureHandle::properties_iterator property_iterator,
		GPlatesModel::PropertyValue::non_null_ptr_type property_value)
{
	if (*property_iterator == NULL) {
		// Always check your property iterators.
		return false;
	}
	// FIXME: UNDO
	// FIXME: the assignment will need to be integrated into a QUndoCommand derivative.
	GPlatesFeatureVisitors::PropertyValueSetter setter(property_iterator, property_value);
	setter.visit_feature_handle(*d_feature_ref);
	
	// We have just changed the model. Let QTableView know.
	int row = get_row_for_property_iterator(property_iterator);
	if (row != -1) {
		QModelIndex idx_begin = index(row, 0);
		QModelIndex idx_end = index(row, 1);
		emit dataChanged(idx_begin, idx_end);
	}
	
	// We have just changed the model. Tell anyone who cares to know.
	emit feature_modified(d_feature_ref);
	d_feature_focus->notify_of_focused_feature_modification();
	return setter.old_property_value();
}


void
GPlatesGui::FeaturePropertyTableModel::delete_property(
		GPlatesModel::FeatureHandle::properties_iterator property_iterator)
{
	// FIXME: UNDO
	// Delete the property container for the given iterator.
	GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
	d_feature_ref->remove_property_container(property_iterator, transaction);
	transaction.commit();
	
	// Update our internal cache of the table rows, and let QTableView know.
	layoutAboutToBeChanged();
	property_info_container_iterator it = d_property_info_cache.begin();
	property_info_container_iterator end = d_property_info_cache.end();
	for ( ; it != end; ++it) {
		if (it->property_iterator == property_iterator) {
			d_property_info_cache.erase(it);
			break;
		}
	}
	layoutChanged();

	// We have just changed the model. Tell anyone who cares to know.
	emit feature_modified(d_feature_ref);
	d_feature_focus->notify_of_focused_feature_modification();
}


GPlatesModel::FeatureHandle::properties_iterator
GPlatesGui::FeaturePropertyTableModel::append_property_value_to_feature(
		GPlatesModel::PropertyValue::non_null_ptr_type property_value,
		const GPlatesModel::PropertyName &property_name)
{
	// FIXME: UNDO
	const GPlatesModel::InlinePropertyContainer::non_null_ptr_type property_container =
			GPlatesModel::ModelUtils::append_property_value_to_feature(
					property_value,
					property_name,
					d_feature_ref);

	// Search for the iterator which corresponds to this new property container.
	layoutAboutToBeChanged();
	GPlatesModel::FeatureHandle::properties_iterator it = d_feature_ref->properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = d_feature_ref->properties_end();
	for (; it != end; ++it)
	{
		if (*it != NULL && *it == property_container) {
			// Found the iterator. Add the info we need to our cache.
			const GPlatesModel::PropertyName new_property_name = (*it)->property_name();
			// To work out if property is editable inline, we do a dry-run of the FromQvariantConverter.
			QVariant dummy;
			GPlatesFeatureVisitors::FromQvariantConverter qvariant_converter(dummy);
			(*it)->accept_visitor(qvariant_converter);
			bool can_convert_inline = qvariant_converter.get_property_value();
	
			FeaturePropertyTableInfo info = { new_property_name, it, can_convert_inline };
			d_property_info_cache.push_back(info);
		}
	}
	layoutChanged();

	// We have just changed the model. Tell anyone who cares to know.
	emit feature_modified(d_feature_ref);
	d_feature_focus->notify_of_focused_feature_modification();

	// Note: Might theoretically be returning properties_end() in some bizzarro world where 
	// we didn't append the property correctly. But ModelUtils::append_property_value_to_feature()
	// can't possibly fail, can it?
	return it;
}


QVariant
GPlatesGui::FeaturePropertyTableModel::get_property_name_as_qvariant(
		int row) const
{
	const GPlatesModel::PropertyName &property_name = get_property_name(row);
	return GPlatesUtils::make_qstring_from_icu_string(property_name.build_aliased_name());
}


QVariant
GPlatesGui::FeaturePropertyTableModel::get_property_value_as_qvariant(
		int row,
		int role) const
{
	GPlatesModel::FeatureHandle::properties_iterator it = get_property_iterator_for_row(row);
	if (*it == NULL) {
		// Always check your property iterators.
		return QVariant();
	}

	// For now, just test the actual feature - no modified cache yet.
	GPlatesFeatureVisitors::ToQvariantConverter qvariant_converter;
	qvariant_converter.set_desired_role(role);
	(*it)->accept_visitor(qvariant_converter);
	
	if (qvariant_converter.found_qvariants_begin() != qvariant_converter.found_qvariants_end()) {
		// We were able to make a QVariant out of this property value.
		// FIXME: Note that we are only returning the first result, and not checking for multiple
		// matching property names.
		return *qvariant_converter.found_qvariants_begin();
	} else {
		// The property exists, but we were unable to render it in a single cell.
		return QVariant();
	}
}





