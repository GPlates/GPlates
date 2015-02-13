/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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
#include <boost/none.hpp>
#include <vector>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QString>
#include <QLocale>

#include "EditGeometryWidget.h"
#include "EditTableActionWidget.h"
#include "EditTableWidget.h"
#include "InvalidPropertyValueException.h"
#include "UninitialisedEditWidgetException.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/GeometryUtils.h"

#include "feature-visitors/GeometrySetter.h"

#include "maths/GeometryOnSphere.h"
#include "maths/InvalidLatLonException.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "presentation/ViewState.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"

#include "utils/GeometryCreationUtils.h"


namespace
{
	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;

	// FIXME: If the DigitisationWidget is any indication, ideally, we won't have to
	// deal with specific GeometryOnSphere derivations at all - we'd just handle it
	// with GeometryCreationUtils and maybe some visitors (for getting things into a
	// property-value, a'la CreateFeatureDialog.)
	typedef GPlatesMaths::PolylineOnSphere polyline_type;
	typedef GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr_type;

	enum LatLonColumnLayout
	{
		COLUMN_LAT, COLUMN_LON, COLUMN_ACTION
	};
	
	
	/**
	 * Fetches the appropriate action widget given a row number.
	 * May return NULL.
	 */
	GPlatesQtWidgets::EditTableActionWidget *
	get_action_widget_for_row(
			QTableWidget &table,
			int row)
	{
		if (row < 0 || row >= table.rowCount()) {
			return NULL;
		}
		if (table.cellWidget(row, COLUMN_ACTION) == NULL) {
			return NULL;
		}
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
				static_cast<GPlatesQtWidgets::EditTableActionWidget *>(
						table.cellWidget(row, COLUMN_ACTION));
		return action_widget;
	}
	
	
	/**
	 * Uses rowCount() and setRowCount() to ensure the table has at least
	 * @a rows rows available. If the table has more rows currently allocated,
	 * this function does not shrink the table.
	 *
	 * The number of rows in the table after the operation is returned.
	 */
	int
	ensure_table_size(
			QTableWidget &table,
			int rows)
	{
		if (table.rowCount() < rows) {
			table.setRowCount(rows);
		}
		return table.rowCount();
	}
	
	
	/**
	 * Allocates QTableWidgetItems and populates a QTableWidget from a
	 * lat,lon pair.
	 *
	 * No checking is done to see if the table is the correct size!
	 * The caller is responsible for adding rows to the table appropriately.
	 */
	void
	populate_table_row_from_lat_lon(
			GPlatesQtWidgets::EditGeometryWidget &geometry_widget,
			QTableWidget &table,
			int row,
			double lat,
			double lon)
	{
		static QLocale locale;

		// Add the lat and lon cells.
		table.setItem(row, COLUMN_LAT, new QTableWidgetItem(locale.toString(lat)));
		table.setItem(row, COLUMN_LON, new QTableWidgetItem(locale.toString(lon)));
		// Add the "Action" cell - we need to set this as uneditable.
		QTableWidgetItem *action_item = new QTableWidgetItem();
		action_item->setFlags(0);
		table.setItem(row, COLUMN_ACTION, action_item);


		// Don't add the action widget to each line of the table. We're going to investigate doing it
		// on an as-needed basis, i.e. create and add the action widget only to the currently highlighted
		// row. 
#if 0
		// Creating the action_widget is not a memory leak - Qt will take ownership of
		// the action_widget memory, and clean it up when the table row is deleted.
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
				new GPlatesQtWidgets::EditTableActionWidget(geometry_widget, &geometry_widget);
		table.setCellWidget(row, COLUMN_ACTION, action_widget);
#endif
	}


	/**
	 * Allocates QTableWidgetItems and populates a QTableWidget from a
	 * GPlatesMaths::PointOnSphere.
	 *
	 * No checking is done to see if the table is the correct size!
	 * The caller is responsible for adding rows to the table appropriately.
	 */
	void
	populate_table_row_with_blank_point(
			GPlatesQtWidgets::EditGeometryWidget *geometry_widget,
			QTableWidget &table,
			int row)
	{
		// Add the lat and lon cells.
		table.setItem(row, COLUMN_LAT, new QTableWidgetItem());
		table.setItem(row, COLUMN_LON, new QTableWidgetItem());
		// Add the "Action" cell - we need to set this as uneditable.
		QTableWidgetItem *action_item = new QTableWidgetItem();
		action_item->setFlags(0);
		table.setItem(row, COLUMN_ACTION, action_item);
		// Creating the action_widget is not a memory leak - Qt will take ownership of
		// the action_widget memory, and clean it up when the table row is deleted.
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
				new GPlatesQtWidgets::EditTableActionWidget(
							geometry_widget, geometry_widget);
		table.setCellWidget(row, COLUMN_ACTION, action_widget);
	}


	/**
	 * Allocates QTableWidgetItems and populates a QTableWidget from a
	 * GPlatesMaths::PolylineOnSphere.
	 *
	 * The table will be modified to ensure there are enough rows available,
	 * and then new QTableWidgetItems will be set for each point in the
	 * polyline, starting with row @a offset and up to row @a offset +
	 * the number of points in the polyline.
	 */
	void
	populate_table_rows_from_polyline(
			GPlatesQtWidgets::EditGeometryWidget &geometry_widget,
			QTableWidget &table,
			int offset,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline)
	{
		ensure_table_size(table, offset + static_cast<int>(polyline->number_of_segments() + 1));
		
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator it = polyline->vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline->vertex_end();
		for (int row = offset; it != end; ++it, ++row) {
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*it);
			populate_table_row_from_lat_lon(geometry_widget, table, row,
					llp.latitude(), llp.longitude());
		}
	}


	/**
	 * Allocates QTableWidgetItems and populates a QTableWidget from a
	 * GPlatesMaths::MultiPointOnSphere.
	 *
	 * The table will be modified to ensure there are enough rows available,
	 * and then new QTableWidgetItems will be set for each point in the
	 * multipoint, starting with row @a offset and up to row @a offset +
	 * the number of points in the multipoint.
	 */
	void
	populate_table_rows_from_multi_point(
			GPlatesQtWidgets::EditGeometryWidget &geometry_widget,
			QTableWidget &table,
			int offset,
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multipoint)
	{
		ensure_table_size(table, offset + static_cast<int>(multipoint->number_of_points()));
		
		GPlatesMaths::MultiPointOnSphere::const_iterator it = multipoint->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator end = multipoint->end();
		for (int row = offset; it != end; ++it, ++row) {
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*it);
			populate_table_row_from_lat_lon(geometry_widget, table, row,
					llp.latitude(), llp.longitude());
		}
	}


	/**
	 * Allocates QTableWidgetItems and populates a QTableWidget from a
	 * GPlatesMaths::PointOnSphere.
	 *
	 * The table will be modified to ensure there are enough rows available,
	 * and then a new QTableWidgetItem will be set for the point.
	 */
	void
	populate_table_rows_from_point(
			GPlatesQtWidgets::EditGeometryWidget &geometry_widget,
			QTableWidget &table,
			int offset,
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point)
	{
		ensure_table_size(table, offset + 1);
		
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*point);
		populate_table_row_from_lat_lon(geometry_widget, table, offset,
				llp.latitude(), llp.longitude());
	}


	/**
	 * Allocates QTableWidgetItems and populates a QTableWidget from a
	 * GPlatesMaths::PolygonOnSphere.
	 *
	 * The table will be modified to ensure there are enough rows available,
	 * and then new QTableWidgetItems will be set for each point in the
	 * polygon, starting with row @a offset and up to row @a offset +
	 * the number of points in the polygon.
	 */
	void
	populate_table_rows_from_polygon(
			GPlatesQtWidgets::EditGeometryWidget &geometry_widget,
			QTableWidget &table,
			int offset,
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon)
	{
		ensure_table_size(table, offset + static_cast<int>(polygon->number_of_vertices()));
		
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator it = polygon->vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = polygon->vertex_end();
		for (int row = offset; it != end; ++it, ++row) {
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*it);
			populate_table_row_from_lat_lon(geometry_widget, table, row,
					llp.latitude(), llp.longitude());
		}
	}
	
	
	/**
	 * Enumeration of possible problems that may be encountered when converting
	 * table contents to GPlates geometry.
	 */
	enum TableRowValidity
	{
		VALID,
		UNPARSEABLE_LAT, UNPARSEABLE_LON,
		INVALID_TABLE_ITEM_LAT, INVALID_TABLE_ITEM_LON,
		INVALID_LAT_LON_POINT
	};
	
	/**
	 * Struct to pair a problem with the table row it was encountered on,
	 * for highlighting purposes.
	 */
	struct InvalidTableRow
	{
		int row;
		TableRowValidity reason;
	};
	
	/**
	 * Struct to be passed around when constructing polylines to accumulate
	 * all problems encountered when converting the QTableWidget to geometry.
	 */
	struct PolylineConstructionProblems
	{
		GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity polyline_validity;
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity validity;
		std::vector<InvalidTableRow> invalid_rows;
	};

	/**
	 * Goes through the points in the table and attempts to build a vector of
	 * PointOnSphere out of them.
	 * 
	 * Invalid points in the table will be skipped over, and added to the
	 * vector @a invalid_rows, which must be created and passed in to this function.
	 * Any errors in converting table cells to LatLonPoints will be appended to
	 * this vector.
	 */
	std::vector<GPlatesMaths::PointOnSphere>
	build_points_from_table_rows(
			QTableWidget &table,
			int start_row,
			int length,
			std::vector<InvalidTableRow> &invalid_rows)
	{
		static QLocale locale;

		std::vector<GPlatesMaths::PointOnSphere> points;
		points.reserve(length);
		
		// Build a vector of points that we can pass to PolylineOnSphere's validity test.
		for (int i = start_row; i < start_row + length; ++i) {
			double lat = 0.0;
			double lon = 0.0;
			
			// (Attempt to) parse lat,lon from table cells.
			bool lat_ok = false;
			QTableWidgetItem *lat_item = table.item(i, COLUMN_LAT);
			if (lat_item != NULL) {
				lat = locale.toDouble(lat_item->text(), &lat_ok);
				if ( ! lat_ok) {
					InvalidTableRow invalid = {i, UNPARSEABLE_LAT};
					invalid_rows.push_back(invalid);
				}
			} else {
				InvalidTableRow invalid = {i, INVALID_TABLE_ITEM_LAT};
				invalid_rows.push_back(invalid);
			}
			bool lon_ok = false;
			QTableWidgetItem *lon_item = table.item(i, COLUMN_LON);
			if (lon_item != NULL) {
				lon = locale.toDouble(lon_item->text(), &lon_ok);
				if ( ! lon_ok) {
					InvalidTableRow invalid = {i, UNPARSEABLE_LON};
					invalid_rows.push_back(invalid);
				}
			} else {
				InvalidTableRow invalid = {i, INVALID_TABLE_ITEM_LON};
				invalid_rows.push_back(invalid);
			}
			
			// (Attempt to) create a LatLonPoint for the coordinates.
			if (lat_ok && lon_ok) {
				// At this point we have a valid lat,lon - valid as far as doubles are concerned.
				try {
					points.push_back(GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(lat,lon)));
				} catch (GPlatesMaths::InvalidLatLonException &) {
					InvalidTableRow invalid = {i, INVALID_LAT_LON_POINT};
					invalid_rows.push_back(invalid);
				}
			} else {
				// Something went wrong. Skip over this row and pretend it doesn't exist.
				// This may be the case when e.g. the user inserts a new blank row.
			}
		}

		return points;
	}
		
	/** 
	 * Highlights any problematic table cells.
	 */
	void
	highlight_invalid_table_cells(
			QTableWidget &table,
			const std::vector<InvalidTableRow> &invalid_rows)
	{
		static QBrush default_foreground = QTableWidgetItem().foreground();
		static QBrush erroneous_foreground = QBrush(Qt::red);
		
		// First, clear any previous highlights.
		for (int i = 0; i < table.rowCount(); ++i) {
			QTableWidgetItem *lat_item = table.item(i, COLUMN_LAT);
			QTableWidgetItem *lon_item = table.item(i, COLUMN_LON);
			if (lat_item != NULL) {
				lat_item->setForeground(default_foreground);
			}
			if (lon_item != NULL) {
				lon_item->setForeground(default_foreground);
			}
		}
		
      // Second, highlight the bad rows.
		std::vector<InvalidTableRow>::const_iterator it = invalid_rows.begin();
		std::vector<InvalidTableRow>::const_iterator end = invalid_rows.end();
		for (; it != end; ++it) {
			QTableWidgetItem *lat_item = NULL;
			QTableWidgetItem *lon_item = NULL;
			if (it->row >= 0 && it->row < table.rowCount()) {
				lat_item = table.item(it->row, COLUMN_LAT);
				lon_item = table.item(it->row, COLUMN_LON);
			}
			
			switch (it->reason)
			{
			default:
			case VALID:
					break;

			case UNPARSEABLE_LAT:
			case INVALID_TABLE_ITEM_LAT:
					if (lat_item != NULL) {
						lat_item->setForeground(erroneous_foreground);
					}
					break;

			case UNPARSEABLE_LON:
			case INVALID_TABLE_ITEM_LON:
					if (lon_item != NULL) {
						lon_item->setForeground(erroneous_foreground);
					}
					break;

			case INVALID_LAT_LON_POINT:
					if (lat_item != NULL) {
						lat_item->setForeground(erroneous_foreground);
					}
					if (lon_item != NULL) {
						lon_item->setForeground(erroneous_foreground);
					}
					break;
			}
		}
	}
	
	/**
	 * Highlights table cells and updates labels to provide feedback to the
	 * user about GeometryOnSphere validity.
	 */
	void
	display_validity_problems(
			QTableWidget &table,
			QLabel &label_error_feedback,
			const PolylineConstructionProblems &problems)
	{
		static QString label_valid_style = "color: rgb(0, 192, 0)";
		static QString label_invalid_style = "color: rgb(192, 0, 0)";

		// Highlight the individual cells that are causing problems.
		highlight_invalid_table_cells(table, problems.invalid_rows);
		
		// Provide an informative message about this particular problem (constructing a polyline).
		switch (problems.validity)
		{
		case GPlatesUtils::GeometryConstruction::VALID:
				// The polyline can be constructed. However, we might still want to warn
				// the user that we have skipped points in order to construct the polyline.
				label_error_feedback.setText(QObject::tr("Valid geometry."));
				label_error_feedback.setVisible(true);
				label_error_feedback.setStyleSheet(label_valid_style);
				break;
		
		case GPlatesUtils::GeometryConstruction::INVALID_INSUFFICIENT_POINTS:
				// Not enough points to make even a single (valid) line segment.
				label_error_feedback.setText(QObject::tr("Invalid geometry: insufficient distinct points."));
				label_error_feedback.setVisible(true);
				label_error_feedback.setStyleSheet(label_invalid_style);
				break;

		case GPlatesUtils::GeometryConstruction::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
				// Segments of a polyline cannot be defined between two points which are antipodal.
				label_error_feedback.setText(QObject::tr("Invalid line segment: consecutive points are antipodal."));
				label_error_feedback.setVisible(true);
				label_error_feedback.setStyleSheet(label_invalid_style);
				break;

		default:
				// Incompatible points encountered! For no defined reason!
				label_error_feedback.setText(QObject::tr("Invalid geometry: <No reason available>."));
				label_error_feedback.setVisible(true);
				label_error_feedback.setStyleSheet(label_invalid_style);
				break;
		}
	}
	
	
	/**
	 * @a validity is a reference to a GeometryConstructionValidity that
	 * should be created by the caller and will be set by this function.
	 */
	geometry_opt_ptr_type
	create_geometry_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity,
			GPlatesMaths::GeometryType::Value geometry_type)
	{
		switch(geometry_type)
		{
		case GPlatesMaths::GeometryType::POLYLINE:
			return geometry_opt_ptr_type(GPlatesUtils::create_polyline_on_sphere(points, validity));

		case GPlatesMaths::GeometryType::MULTIPOINT:
			return geometry_opt_ptr_type(GPlatesUtils::create_multipoint_on_sphere(points, validity));

		case GPlatesMaths::GeometryType::POINT:
			return geometry_opt_ptr_type(GPlatesUtils::create_point_on_sphere(points, validity));

		case GPlatesMaths::GeometryType::POLYGON:
			return geometry_opt_ptr_type(GPlatesUtils::create_polygon_on_sphere(points, validity));

		default:
			return boost::none;
		}
	}

	/**
	 * Goes through the points in the table and tests if they make a valid
	 * PolylineOnSphere. Updates the table cells' foreground colours
	 * appropriately, and will adjust the text and visibility of the provided
	 * QLabel to provide feedback to the user.
	 *
	 * FIXME: This will probably get deprecated fast once EditGeometryWidget is
	 * properly using utils/GeometryConstructionUtils.h
	 */
	bool
	test_polyline_on_sphere_validity(
			std::vector<GPlatesMaths::PointOnSphere> &points,
			PolylineConstructionProblems &problems)
	{
		// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
		std::pair<
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
				invalid_points;
		// FIXME: It would be nice if we could look at those iterators, calculate the appropriate
		// table rows (remember, we may have skipped rows!), and highlight the bad ones.
		
		// Evaluate construction parameter validity.
		// FIXME: Switch to GPlatesUtils::GeometryConstruction::GeometryConstructionValidity.
		problems.polyline_validity = polyline_type::evaluate_construction_parameter_validity(
				points, invalid_points);

		// FIXME: how strict do we want to be when we say "valid"? Remember, we may have
		// skipped over some points.
		return (problems.invalid_rows.empty() && problems.polyline_validity == polyline_type::VALID);
	}

	/**
	 * Work around a graphical glitch, where the EditTableActionWidgets around the
	 * recently scrolled-to row appear to be misaligned.
	 * 
	 * This graphical glitch appears most prominently when appending a point to the
	 * table, but can also appear due to auto-scrolling when inserting a new row above
	 * or below via action widget buttons, and most likely this can also happen
	 * during row deletion, so, better safe than sorry.
	 */
	void
	work_around_table_graphical_glitch(
			GPlatesQtWidgets::EditGeometryWidget &edit_geometry_widget,
			QTableWidget &table)
	{
		GPlatesQtWidgets::EditTableWidget *table_widget_ptr = &edit_geometry_widget;
		static const GPlatesQtWidgets::EditTableActionWidget dummy(table_widget_ptr, NULL);
		table.horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width() + 1);
		table.horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width());
	}
}


GPlatesQtWidgets::EditGeometryWidget::EditGeometryWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_),
	d_geometry_type(GPlatesMaths::GeometryType::NONE)
{
	setupUi(this);
	// Set column widths and resizabilty.
	EditTableActionWidget dummy(this, NULL);
	table_points->horizontalHeader()->setResizeMode(COLUMN_LAT, QHeaderView::Stretch);
	table_points->horizontalHeader()->setResizeMode(COLUMN_LON, QHeaderView::Stretch);
	table_points->horizontalHeader()->setResizeMode(COLUMN_ACTION, QHeaderView::Fixed);
	table_points->horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width());
	table_points->horizontalHeader()->setMovable(true);
	// Set up a minimum row height as well, for the action widgets' sake.
	table_points->verticalHeader()->setDefaultSectionSize(dummy.height());
	
	// Clear spinboxes and things.
	reset_widget_to_default_values();
	
	// FIXME: Find the right signal to look for. This one (cellActivated) kinda works,
	// but what happens is, user changes value, user hits enter, value goes in cell,
	// user hits enter again, cellActivated(). We need something better - but cellChanged()
	// fires when we're populating the table...
	QObject::connect(table_points, SIGNAL(cellActivated(int, int)),
			this, SLOT(handle_cell_changed(int, int)));
	
	// Signals for managing data entry focus for the "Append Point" widgets.
	QObject::connect(button_append_point, SIGNAL(clicked()),
			this, SLOT(append_point_clicked()));
	
	QObject::connect(table_points, SIGNAL(currentCellChanged(int,int,int,int)),
						this, SLOT(handle_current_cell_changed(int,int,int,int)));
		

	setFocusProxy(table_points);

}


void
GPlatesQtWidgets::EditGeometryWidget::reset_widget_to_default_values()
{
	d_property_value_ptr = NULL;
	// Reset table.
	table_points->clearContents();
	table_points->setRowCount(0);
	
	// Reset error feedback.
	test_geometry_validity();

	// Reset widgets.
	d_geometry_type = GPlatesMaths::GeometryType::POLYLINE;
	spinbox_lat->setValue(0.0);
	spinbox_lon->setValue(0.0);
	
	set_clean();
}


void
GPlatesQtWidgets::EditGeometryWidget::configure_for_property_value_type(
		const GPlatesPropertyValues::StructuralType &property_value_type)
{
	static const GPlatesPropertyValues::StructuralType LINE_STRING_PROPERTY_TYPE =
			GPlatesPropertyValues::StructuralType::create_gml("LineString");
	static const GPlatesPropertyValues::StructuralType MULTI_POINT_PROPERTY_TYPE =
			GPlatesPropertyValues::StructuralType::create_gml("MultiPoint");
	static const GPlatesPropertyValues::StructuralType POINT_PROPERTY_TYPE =
			GPlatesPropertyValues::StructuralType::create_gml("Point");
	static const GPlatesPropertyValues::StructuralType POLYGON_PROPERTY_TYPE =
			GPlatesPropertyValues::StructuralType::create_gml("Polygon");

	if (property_value_type == LINE_STRING_PROPERTY_TYPE)
	{
		d_geometry_type = GPlatesMaths::GeometryType::POLYLINE;
	}
	else if (property_value_type == MULTI_POINT_PROPERTY_TYPE)
	{
		d_geometry_type = GPlatesMaths::GeometryType::MULTIPOINT;
	}
	else if (property_value_type == POINT_PROPERTY_TYPE)
	{
		d_geometry_type = GPlatesMaths::GeometryType::POINT;
	}
	else if (property_value_type == POLYGON_PROPERTY_TYPE)
	{
		d_geometry_type = GPlatesMaths::GeometryType::POLYGON;
	}
	else
	{
		d_geometry_type = GPlatesMaths::GeometryType::NONE;
		throw PropertyValueNotSupportedException(GPLATES_EXCEPTION_SOURCE);
	}
}


void
GPlatesQtWidgets::EditGeometryWidget::update_widget_from_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_property_value_ptr = &gml_line_string;
	// Reset table, then fill with points.
	table_points->clearContents();
	table_points->setRowCount(0);
	populate_table_rows_from_polyline(*this, *table_points, 0, gml_line_string.polyline());

	d_geometry_type = GPlatesMaths::GeometryType::POLYLINE;

	// Reset error feedback.
	test_geometry_validity();

	set_clean();

	table_points->setCurrentCell(0,0);
}


void
GPlatesQtWidgets::EditGeometryWidget::update_widget_from_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_property_value_ptr = &gml_multi_point;
	// Reset table, then fill with points.
	table_points->clearContents();
	table_points->setRowCount(0);
	populate_table_rows_from_multi_point(*this, *table_points, 0, gml_multi_point.multipoint());
	
	d_geometry_type = GPlatesMaths::GeometryType::MULTIPOINT;

	// Reset error feedback.
	test_geometry_validity();

	set_clean();

	table_points->setCurrentCell(0,0);
}


void
GPlatesQtWidgets::EditGeometryWidget::update_widget_from_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_property_value_ptr = &gml_point;
	// Reset table, then fill with points.
	table_points->clearContents();
	table_points->setRowCount(0);
	populate_table_rows_from_point(*this, *table_points, 0, gml_point.point());
	
	d_geometry_type = GPlatesMaths::GeometryType::POINT;

	// Reset error feedback.
	test_geometry_validity();

	set_clean();

	table_points->setCurrentCell(0,0);
}


void
GPlatesQtWidgets::EditGeometryWidget::update_widget_from_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	d_property_value_ptr = &gml_polygon;
	// Reset table, then fill with points.
	table_points->clearContents();
	table_points->setRowCount(0);
	populate_table_rows_from_polygon(*this, *table_points, 0, gml_polygon.exterior());
	
	d_geometry_type = GPlatesMaths::GeometryType::POLYGON;

	// Reset error feedback.
	test_geometry_validity();

	set_clean();

	table_points->setCurrentCell(0,0);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditGeometryWidget::create_property_value_from_widget() const
{
	// For now, assume we're trying to make a GmlLineString with a single PolylineOnSphere.
	int line_start = 0;
	int line_length = table_points->rowCount();

	PolylineConstructionProblems problems;
	// Build a list of points based on the valid entries in the table.
	std::vector<GPlatesMaths::PointOnSphere> points = build_points_from_table_rows(
			*table_points, line_start, line_length, problems.invalid_rows);

	// FIXME: needs a better hint than the combobox index.
	geometry_opt_ptr_type geometry_opt_ptr =
			create_geometry_on_sphere(points, problems.validity, d_geometry_type);
	if (geometry_opt_ptr) {
		// Create a property value using the present-day GeometryOnSphere.
		const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_value_opt =
				GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
						geometry_opt_ptr.get());

		if (geometry_value_opt) {
			// Give them the PropertyValue they desire so much.
			return *geometry_value_opt;
		} else {
			// Might happen, if EditGeometryWidget and the GeometricPropertyValueConstructor
			// disagree on what is implemented and what is not.
			throw InvalidPropertyValueException(GPLATES_EXCEPTION_SOURCE,
					tr("There was an error converting the digitised geometry to a usable property value."));
		}
	} else {
		// FIXME: Wording.
		throw InvalidPropertyValueException(GPLATES_EXCEPTION_SOURCE,
				tr("There was an error creating the geometry. Check there are sufficient points in the table."));
	}
}


bool
GPlatesQtWidgets::EditGeometryWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!
	// FIXME: You know what? This should probably be a boost::optional of non_null_intrusive_ptr.
	// It's late and I'm tired and there's no time for big API changes right now though.
	if (d_property_value_ptr.get() != NULL) {
		if (is_dirty()) {
			set_geometry_for_property_value();
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}



void
GPlatesQtWidgets::EditGeometryWidget::handle_insert_row_above(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_point_into_table(row);
	}
}

void
GPlatesQtWidgets::EditGeometryWidget::handle_insert_row_below(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_point_into_table(row + 1);
	}
}

void
GPlatesQtWidgets::EditGeometryWidget::handle_delete_row(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		delete_point_from_table(row);
	}
}


void
GPlatesQtWidgets::EditGeometryWidget::handle_cell_changed(
		int row,
		int column)
{
	// The action widget for that row should store all the info we need about which
	// particular part of the geometric PropertyValue we need to change.
	
	if (test_geometry_validity()) {
		set_dirty();
		Q_EMIT commit_me();
	}
}


int
GPlatesQtWidgets::EditGeometryWidget::get_row_for_action_widget(
		const EditTableActionWidget *action_widget)
{
	for (int i = 0; i < table_points->rowCount(); ++i)
	{
		if (table_points->cellWidget(i, COLUMN_ACTION) == action_widget) {
			return i;
		}
	}
	return -1;
}


void
GPlatesQtWidgets::EditGeometryWidget::append_point_to_table(
		double lat,
		double lon)
{
	// Append a new point at the end of the table.
	// Note: When we are able to edit multi-geometries and GmlPolygon's interior and exterior rings,
	// we may want to include a 'append break' button or modify this function to be smart about
	// where it is appending the point.
	int row = table_points->rowCount();
	table_points->insertRow(row);
	populate_table_row_from_lat_lon(*this, *table_points, row, lat, lon);
	
	// Scroll to show the user the point they just added.
	QTableWidgetItem *table_item_to_scroll_to = table_points->item(row, 0);
	if (table_item_to_scroll_to != NULL) {
		table_points->scrollToItem(table_item_to_scroll_to);
	}
	// Work around a graphical glitch, where the EditTableActionWidgets above the
	// recently scrolled-to row appear to be misaligned.
	work_around_table_graphical_glitch(*this, *table_points);

	// Check if what we have now is (still) a valid polyline.
	if (test_geometry_validity()) {
		set_dirty();
		Q_EMIT commit_me();
	}

	// Set the current cell to be a cell from the new row, so that an action widget is added to it. 
	table_points->setCurrentCell(row,COLUMN_ACTION);

}


void
GPlatesQtWidgets::EditGeometryWidget::insert_blank_point_into_table(
		int row)
{
	// Insert a new blank row.
	// Note: When we are able to edit multi-geometries and GmlPolygon's interior and exterior rings,
	// we may want to include a 'append break' button or modify this function to be smart about
	// where it is inserting the point.
	table_points->insertRow(row);
	populate_table_row_with_blank_point(this, *table_points, row);
	
	// Work around a graphical glitch, where the EditTableActionWidgets above the
	// recently scrolled-to row appear to be misaligned. And yes, the table widget
	// may auto-scroll if we (for instance) insert a row at the end.
	work_around_table_graphical_glitch(*this, *table_points);
	
	// Open up an editor for the first coordinate field.
	QTableWidgetItem *coord_item = table_points->item(row, COLUMN_LAT);
	if (coord_item != NULL) {
		table_points->setCurrentItem(coord_item);
		table_points->editItem(coord_item);
	}

	// Check if what we have now is (still) a valid polyline.
	if (test_geometry_validity()) {
		set_dirty();
		Q_EMIT commit_me();
	}
}


void
GPlatesQtWidgets::EditGeometryWidget::delete_point_from_table(
		int row)
{

	// Before we delete the row, delete the action widget. removeRow() messes with the previous/current
	// row indices, and then calls handle_current_cell_changed, which cannot delete the old action widget, 
	// the upshot being that we end up with a surplus action widget which we can't get rid of. 
	table_points->removeCellWidget(row,COLUMN_ACTION);
	// Delete the given row.
	table_points->removeRow(row);

	// Work around a potential graphical glitch involving scrolling, as per the
	// append and insert point functions.
	work_around_table_graphical_glitch(*this, *table_points);
	
	// Check if what we have now is (still) a valid polyline.
	if (test_geometry_validity()) {
		set_dirty();
		Q_EMIT commit_me();
	}
}


bool
GPlatesQtWidgets::EditGeometryWidget::test_geometry_validity()
{
	// For now, assume we're trying to make a GmlLineString with a single PolylineOnSphere.
	int line_start = 0;
	int line_length = table_points->rowCount();

	PolylineConstructionProblems problems;
	// Build a list of points based on the valid entries in the table.
	std::vector<GPlatesMaths::PointOnSphere> points = build_points_from_table_rows(
			*table_points, line_start, line_length, problems.invalid_rows);

	bool ok = true;
	// Instead of the obsolete test_polyline_on_sphere_validity, just attempt
	// to make a GeometryOnSphere using the utility code.
	geometry_opt_ptr_type geometry_opt_ptr =
			create_geometry_on_sphere(points, problems.validity, d_geometry_type);
	if ( ! geometry_opt_ptr) {
		ok = false;
	}
	
	// Highlight any problems, and update the label appropriately.
	display_validity_problems(*table_points, *label_error_feedback, problems);
	
	return ok;
}


bool
GPlatesQtWidgets::EditGeometryWidget::set_geometry_for_property_value()
{
	// If the EditWidgetGroupBox wants a GmlLineString (etc) updated,
	// this is where we come to do it.

	// For now, assume we're trying to make a GmlLineString with a single PolylineOnSphere.
	int line_start = 0;
	int line_length = table_points->rowCount();

	PolylineConstructionProblems problems;
	// Build a list of points based on the valid entries in the table.
	std::vector<GPlatesMaths::PointOnSphere> points = build_points_from_table_rows(
			*table_points, line_start, line_length, problems.invalid_rows);

	// FIXME: You know what? This should probably be a boost::optional of non_null_intrusive_ptr.
	// It's late and I'm tired and there's no time for big API changes right now though.
	if (d_property_value_ptr.get() != NULL) {
		// FIXME: Pass some kind of BETTER hint to create_geometry_on_sphere.
		geometry_opt_ptr_type geometry_opt_ptr =
				create_geometry_on_sphere(points, problems.validity, d_geometry_type);
		if (geometry_opt_ptr) {
			GPlatesFeatureVisitors::GeometrySetter geometry_setter(*geometry_opt_ptr);
			geometry_setter.set_geometry(d_property_value_ptr.get());
			return true;
		}
	}
	return false;
}

void
GPlatesQtWidgets::EditGeometryWidget::handle_current_cell_changed(
		int current_row, int current_column, int previous_row, int previous_column)
{

	if ((current_row != previous_row) && current_row >=0)
	{
		if (table_points->cellWidget(previous_row,COLUMN_ACTION))
		{
			table_points->removeCellWidget(previous_row,COLUMN_ACTION);
		}
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
				new GPlatesQtWidgets::EditTableActionWidget(this, this);
		table_points->setCellWidget(current_row, COLUMN_ACTION, action_widget);
	}

}
