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

#include "FeatureTableModel.h"

#include <QApplication>
#include <QHeaderView>
#include <QString>
#include <QLocale>
#include <QFont>
#include <QFontMetrics>
#include <QDebug>

#include "model/types.h"
#include "model/FeatureHandle.h"
#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "feature-visitors/XsStringFinder.h"
#include "feature-visitors/GeometryFinder.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GeoTimeInstant.h"
#include "utils/UnicodeStringUtils.h"
#include "maths/LatLonPointConversions.h"
#include "maths/PointOnSphere.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace
{
	typedef QVariant (*table_cell_accessor_type)(
			GPlatesModel::FeatureHandle &feature);
	
	struct ColumnHeadingInfo
	{
		const char *label;
		const char *tooltip;
		int width;
		QHeaderView::ResizeMode resize_mode;
		table_cell_accessor_type accessor;
		QFlags<Qt::AlignmentFlag> alignment;
	};


	// Accessor functions for table cells:
	
	QVariant
	null_table_accessor(
			GPlatesModel::FeatureHandle &feature)
	{
		return QVariant();
	}
	
	QVariant
	get_feature_type(
			GPlatesModel::FeatureHandle &feature)
	{
		return QVariant(GPlatesUtils::make_qstring_from_icu_string(
					feature.feature_type().build_aliased_name()));
	}
	
	QVariant
	get_plate_id(
			GPlatesModel::FeatureHandle &feature)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
		plate_id_finder.visit_feature_handle(feature);
		if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) {
			// The feature has a reconstruction plate ID.
			GPlatesModel::integer_plate_id_type recon_plate_id =
					*plate_id_finder.found_plate_ids_begin();
			return QVariant(static_cast<quint32>(recon_plate_id));
		} else {
			return QVariant();
		}
	}
	
	QString
	format_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
	{
		QLocale locale;
		if (time_instant.time_position().is_real()) {
			return locale.toString(time_instant.time_position().value());
		} else if (time_instant.time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.time_position().is_distant_future()) {
			return QObject::tr("future");
		} else {
			return QObject::tr("<invalid>");
		}
	}

	QString
	format_time_period(
			const GPlatesPropertyValues::GmlTimePeriod &time_period)
	{
		return QObject::tr("%1 - %2")
				.arg(format_time_instant(*(time_period.begin())))
				.arg(format_time_instant(*(time_period.end())));
	}
			
	QVariant
	get_time_begin(
			GPlatesModel::FeatureHandle &feature)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
			GPlatesModel::PropertyName::create_gml("validTime");
		GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
		time_period_finder.visit_feature_handle(feature);
		if (time_period_finder.found_time_periods_begin() != time_period_finder.found_time_periods_end()) {
			// The feature has a gpml:validTime property.
			// FIXME: This could be from a gpml:TimeVariantFeature, OR a gpml:InstantaneousFeature,
			// in the latter case it has a slightly different meaning and we should be displaying the
			// gpml:reconstructedTime property instead.
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
					*time_period_finder.found_time_periods_begin();
			return format_time_instant(*(time_period->begin()));
		} else {
			return QVariant();
		}
	}

	QVariant
	get_time_end(
			GPlatesModel::FeatureHandle &feature)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
			GPlatesModel::PropertyName::create_gml("validTime");
		GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
		time_period_finder.visit_feature_handle(feature);
		if (time_period_finder.found_time_periods_begin() != time_period_finder.found_time_periods_end()) {
			// The feature has a gpml:validTime property.
			// FIXME: This could be from a gpml:TimeVariantFeature, OR a gpml:InstantaneousFeature,
			// in the latter case it has a slightly different meaning and we should be displaying the
			// gpml:reconstructedTime property instead.
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
					*time_period_finder.found_time_periods_begin();
			return format_time_instant(*(time_period->end()));
		} else {
			return QVariant();
		}
	}
	
	QVariant
	get_name(
			GPlatesModel::FeatureHandle &feature)
	{
		// FIXME: Need to adapt according to user's current codeSpace setting.
		static const GPlatesModel::PropertyName name_property_name = 
			GPlatesModel::PropertyName::create_gml("name");
		GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
		string_finder.visit_feature_handle(feature);
		if (string_finder.found_strings_begin() != string_finder.found_strings_end()) {
			// The feature has one or more name properties. Use the first one for now.
			GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name = 
					*string_finder.found_strings_begin();
			return GPlatesUtils::make_qstring(name->value());
		} else {
			return QVariant();
		}
	}

	QVariant
	get_description(
			GPlatesModel::FeatureHandle &feature)
	{
		static const GPlatesModel::PropertyName description_property_name =
			GPlatesModel::PropertyName::create_gml("description");
		GPlatesFeatureVisitors::XsStringFinder string_finder(description_property_name);
		string_finder.visit_feature_handle(feature);
		if (string_finder.found_strings_begin() != string_finder.found_strings_end()) {
			// The feature has a description property
			GPlatesPropertyValues::XsString::non_null_ptr_to_const_type description = 
					*string_finder.found_strings_begin();
			return GPlatesUtils::make_qstring(description->value());
		} else {
			return QVariant();
		}
	}
	
	QString
	format_geometry_point(
			const GPlatesMaths::PointOnSphere &point)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
		QLocale locale;
		QString point_str = QObject::tr("(%1 ; %2)")
				.arg(locale.toString(llp.latitude()))
				.arg(locale.toString(llp.longitude()));
		return point_str;
	}

	QString
	format_geometry_polyline(
			const GPlatesMaths::PolylineOnSphere &polyline)
	{
		QString begin_str = format_geometry_point(*polyline.vertex_begin());
		QString end_str = format_geometry_point(*(--polyline.vertex_end()));
		QString middle_str;
		if (polyline.number_of_vertices() == 3) {
			middle_str = QObject::tr("... 1 vertex ... ");
		} else if (polyline.number_of_vertices() > 3) {
			middle_str = QObject::tr("... %1 vertices ... ").arg(polyline.number_of_vertices() - 2);
		}
		return QObject::tr("%1 %3%2")
				.arg(begin_str)
				.arg(end_str)
				.arg(middle_str);
	}
	
	QVariant
	get_geometry(
			GPlatesModel::FeatureHandle &feature)
	{
		GPlatesFeatureVisitors::GeometryFinder geometry_finder;
		geometry_finder.visit_feature_handle(feature);
		if (geometry_finder.has_found_geometry()) {
			// The feature has some geometry of some kind.
			if (geometry_finder.found_points_begin() != geometry_finder.found_points_end()) {
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_ptr =
						(*geometry_finder.found_points_begin())->point();
				return format_geometry_point(*point_ptr);
				
			} else if (geometry_finder.found_line_strings_begin() != geometry_finder.found_line_strings_end()) {
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
						(*geometry_finder.found_line_strings_begin())->polyline();
				return format_geometry_polyline(*polyline_ptr);
			
			} else {
				return QVariant();
			}
		} else {
			return QVariant();
		}
	}

	// The dispatch table for the above functions:
	
	static const ColumnHeadingInfo column_heading_info_table[] = {
		{ QT_TR_NOOP("Feature Type"), QT_TR_NOOP(""),
				140, QHeaderView::ResizeToContents,
				get_feature_type, Qt::AlignLeft | Qt::AlignVCenter },
				
		{ QT_TR_NOOP("Plate ID"), QT_TR_NOOP("The Plate ID used to reconstruct the feature"),
				60, QHeaderView::Fixed,
				get_plate_id, Qt::AlignCenter },
				
		{ QT_TR_NOOP("Begin"), QT_TR_NOOP("Time of Appearance (Ma)"),
				60, QHeaderView::Fixed,
				get_time_begin, Qt::AlignCenter },
				
		{ QT_TR_NOOP("End"), QT_TR_NOOP("Time of Disappearance (Ma)"),
				60, QHeaderView::Fixed,
				get_time_end, Qt::AlignCenter }, 
				
		{ QT_TR_NOOP("Name"), QT_TR_NOOP(""),
				96, QHeaderView::Stretch,
				get_name, Qt::AlignLeft | Qt::AlignVCenter },
				
		{ QT_TR_NOOP("Description"), QT_TR_NOOP(""),
				240, QHeaderView::ResizeToContents,
				get_description, Qt::AlignLeft | Qt::AlignVCenter },
				
		{ QT_TR_NOOP("Geometry (lat ; lon)"), QT_TR_NOOP("Summary of the first geometry associated with the feature"),
				134, QHeaderView::ResizeToContents,
				get_geometry, Qt::AlignCenter },
	};

	const QString
	get_column_heading(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return QObject::tr("");
		}
		return QObject::tr(column_heading_info_table[column].label);
	}

	const QString
	get_column_tooltip(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return QObject::tr("");
		}
		return QObject::tr(column_heading_info_table[column].tooltip);
	}

	int
	get_column_width(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return 0;
		}
		return column_heading_info_table[column].width;
	}

	table_cell_accessor_type
	get_column_accessor(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return null_table_accessor;
		}
		return column_heading_info_table[column].accessor;
	}

	QFlags<Qt::AlignmentFlag>
	get_column_alignment(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return Qt::AlignLeft | Qt::AlignVCenter;
		}
		return column_heading_info_table[column].alignment;
	}
}



GPlatesGui::FeatureTableModel::FeatureTableModel(
		FeatureFocus &feature_focus,
		QObject *parent_):
	QAbstractTableModel(parent_),
	d_feature_focus(feature_focus),
	d_sequence_ptr(FeatureWeakRefSequence::create())
{
	QObject::connect(&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref)),
			this,
			SLOT(handle_feature_modified(GPlatesModel::FeatureHandle::weak_ref)));
}



int
GPlatesGui::FeatureTableModel::rowCount(
		const QModelIndex &parent_) const
{
	return static_cast<int>(d_sequence_ptr->size());
}

int
GPlatesGui::FeatureTableModel::columnCount(
		const QModelIndex &parent_) const
{
	return NUM_ELEMS(column_heading_info_table);
}

Qt::ItemFlags
GPlatesGui::FeatureTableModel::flags(
		const QModelIndex &idx) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
GPlatesGui::FeatureTableModel::headerData(
		int section,
		Qt::Orientation orientation,
		int role) const
{
	// The new way we are attempting to return an appropriate vertical
	// and horizontal size in the Qt::SizeHintRole for the header!
	QFontMetrics fm = QApplication::fontMetrics();
	
#if 1
	qDebug() << "\nFONT METRICS DEBUGGING:";
	qDebug() << "QApplication::style() == " << QApplication::style()->metaObject()->className();
	qDebug() << "QApplication::font().toString() == " << QApplication::font().toString();
	qDebug() << "QLocale().name() == " << QLocale().name();
	qDebug() << "fm.ascent() == " << fm.ascent();
	qDebug() << "fm.descent() == " << fm.descent();
	qDebug() << "fm.boundingRect(Q) == " << fm.boundingRect('Q');
	qDebug() << "fm.boundingRect(y) == " << fm.boundingRect('y');
	qDebug() << "fm.boundingRect(QylLj!|[]`~_) == " << fm.boundingRect("QylLj!|[]`~_");
	qDebug() << "fm.height() == " << fm.height();
	qDebug() << "fm.lineSpacing() == " << fm.lineSpacing();
	qDebug() << "fm.leading() == " << fm.leading();
#endif
	
	// We are only interested in modifying the horizontal header.
	if (orientation == Qt::Horizontal) {
		// No need to supply tooltip data, etc. for headers. We are only interested in a few roles.

		if (role == Qt::DisplayRole) {
			return get_column_heading(section);

		} else if (role == Qt::ToolTipRole) {
			return get_column_tooltip(section);
			
		} else if (role == Qt::SizeHintRole) {
			// Annoyingly, the metrics alone do not appear to be sufficient to supply the height
			// of the header, so a few pixels are added.
			// Furthermore, for some reason, the height is now being set correctly but the width
			// does not updated until a call to QTableView::resizeColumnsToContents();
			return QSize(get_column_width(section), fm.height()+4);
			
		} else {
			return QVariant();
		}
		
	} else {
		// Vertical header; ignore.
		return QVariant();
	}
}


QVariant
GPlatesGui::FeatureTableModel::data(
		const QModelIndex &idx,
		int role) const
{
	if (!idx.isValid()) {
		return QVariant();
	}
	if (idx.row() < 0 || idx.row() >= rowCount()) {
		return QVariant();
	}
	
	if (role == Qt::DisplayRole) {
		// Obtain FeatureHandle and CHECK VALIDITY!
		GPlatesModel::FeatureHandle::weak_ref feature_ref = d_sequence_ptr->at(idx.row());
		if ( ! feature_ref.is_valid()) {
			return QVariant();
		}
		
		// Cell contents is returned via call to column-specific dispatch function.
		table_cell_accessor_type accessor = get_column_accessor(idx.column());
		return accessor(*feature_ref);
	} else if (role == Qt::TextAlignmentRole) {
		return QVariant(get_column_alignment(idx.column()));
	}
	return QVariant();
}


void
GPlatesGui::FeatureTableModel::handle_selection_change(
		const QItemSelection &selected,
		const QItemSelection &deselected)
{
	if (selected.indexes().isEmpty()) {
		return;
	}
	// We assume that the view has been constrained to allow only single-row selections,
	// so only concern ourselves with the first index in the list.
	QModelIndex idx = selected.indexes().first();
	if ( ! idx.isValid()) {
		return;
	}
	// Obtain FeatureHandle and CHECK VALIDITY!
	GPlatesModel::FeatureHandle::weak_ref feature_ref = d_sequence_ptr->at(idx.row());
	if ( ! feature_ref.is_valid()) {
		return;
	}
	
	// When the user clicks a line of the table, we change the currently focused feature.
	d_feature_focus.set_focused_feature(feature_ref);
}


void
GPlatesGui::FeatureTableModel::handle_feature_modified(
		GPlatesModel::FeatureHandle::weak_ref modified_feature_ref)
{
	// First, figure out which row of the table (if any) contains the modified feature weakref.
	FeatureWeakRefSequence::const_iterator it = d_sequence_ptr->begin();
	FeatureWeakRefSequence::const_iterator end = d_sequence_ptr->end();
	int row = 0;
	for ( ; it != end; ++it, ++row) {
		if (*it == modified_feature_ref) {
			QModelIndex idx_begin = index(row, 0);
			QModelIndex idx_end = index(row, NUM_ELEMS(column_heading_info_table) - 1);
			emit dataChanged(idx_begin, idx_end);
		}
	}
}


void
GPlatesGui::FeatureTableModel::set_default_resize_modes(
		QHeaderView &header)
{
	for (int column = 0; column < static_cast<int>(NUM_ELEMS(column_heading_info_table)); ++column) {
		header.setResizeMode(column, column_heading_info_table[column].resize_mode);
	}
}

