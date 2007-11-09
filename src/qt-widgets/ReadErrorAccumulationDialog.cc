/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <map>
#include <string>
#include <sstream>
#include "ReadErrorAccumulationDialog.h"

#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

namespace
{
	// Maps used for error text lookup.
	typedef std::map<GPlatesFileIO::ReadErrors::Description, QString> description_map_type;
	typedef description_map_type::const_iterator description_map_const_iterator;
	typedef std::map<GPlatesFileIO::ReadErrors::Result, QString> result_map_type;
	typedef result_map_type::const_iterator result_map_const_iterator;

	// Map of Filename -> Error collection for reporting all errors (of a particular type) for each file.
	typedef std::map<std::string, GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type> file_errors_map_type;
	typedef file_errors_map_type::const_iterator file_errors_map_const_iterator;

	struct ReadErrorDescription
	{
		GPlatesFileIO::ReadErrors::Description code;
		const char *text;
	};
	
	struct ReadErrorResult
	{
		GPlatesFileIO::ReadErrors::Result code;
		const char *text;
	};
	
	/**
	 * This table is sourced from http://trac.gplates.org/wiki/ReadErrorMessages
	 */
	static ReadErrorDescription description_table[] = {
		// Errors from PLATES Line Format files
		{ GPlatesFileIO::ReadErrors::InvalidPlatesRegionNumber,
				QT_TR_NOOP("Invalid 'Region Number' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesReferenceNumber,
				QT_TR_NOOP("Invalid 'Reference Number' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesStringNumber,
				QT_TR_NOOP("Invalid 'String Number' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesGeographicDescription,
				QT_TR_NOOP("Invalid 'Geographic Description' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesPlateIdNumber,
				QT_TR_NOOP("Invalid 'Plate Id Number' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfAppearance,
				QT_TR_NOOP("Invalid 'Age Of Appearance' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfDisappearance,
				QT_TR_NOOP("Invalid 'Age Of Disappearance' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCode,
				QT_TR_NOOP("Invalid 'Data Type Code' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumber,
				QT_TR_NOOP("Invalid 'Data Type Code Number' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumberAdditional,
				QT_TR_NOOP("Invalid 'Data Type Code Number Additional' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesConjugatePlateIdNumber,
				QT_TR_NOOP("Invalid 'Conjugate Plate Id Number' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesColourCode,
				QT_TR_NOOP("Invalid 'Colour Code' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesNumberOfPoints,
				QT_TR_NOOP("Invalid 'Number Of Points' encountered when parsing PLATES line format header.") },
		{ GPlatesFileIO::ReadErrors::UnknownPlatesDataTypeCode,
				QT_TR_NOOP("The 'Data Type Code' in the PLATES header is not recognized by GPlates' PLATES Line Format Reader.") },
		{ GPlatesFileIO::ReadErrors::MissingPlatesPolylinePoint,
				QT_TR_NOOP("A poly line point in a PLATES line format file was expected, but not found.") },
		{ GPlatesFileIO::ReadErrors::MissingPlatesHeaderSecondLine,
				QT_TR_NOOP("The second line of the PLATES header was not found when parsing a PLATES line format file.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesPolylinePoint,
				QT_TR_NOOP("A polyline in a PLATES line format file was invalid (was not of '<latitude> <longtitude> <plotter code>' form).") },
		{ GPlatesFileIO::ReadErrors::BadPlatesPolylinePlotterCode,
				QT_TR_NOOP("The plotter code in a PLATES poly line point was invalid (not pen-up or pen-down).") },
		{ GPlatesFileIO::ReadErrors::BadPlatesPolylineLatitude,
				QT_TR_NOOP("The latitude in a PLATES poly line point was invalid (not between -90 and 90).") },
		{ GPlatesFileIO::ReadErrors::BadPlatesPolylineLongitude,
				QT_TR_NOOP("The longtitude in a PLATES poly line point was invalid (not between -360 and 360).") },
		
		// Errors from PLATES Rotation Format files
		{ GPlatesFileIO::ReadErrors::CommentMovingPlateIdAfterNonCommentSequence,
				QT_TR_NOOP("A commented-out pole was found after a non-commented-out sequence.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingFixedPlateId,
				QT_TR_NOOP("Error reading the fixed plate ID.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingGeoTime,
				QT_TR_NOOP("Error reading the geological time.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingMovingPlateId,
				QT_TR_NOOP("Error reading the moving plate ID.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingPoleLatitude,
				QT_TR_NOOP("Error reading the pole latitude coordinate.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingPoleLongitude,
				QT_TR_NOOP("Error reading the pole longitude coordinate.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingRotationAngle,
				QT_TR_NOOP("Error reading the rotation angle.") },
		{ GPlatesFileIO::ReadErrors::MovingPlateIdEqualsFixedPlateId,
				QT_TR_NOOP("The moving plate ID is identical to the fixed plate ID.") },
		{ GPlatesFileIO::ReadErrors::NoCommentFound,
				QT_TR_NOOP("No comment was found.") },
		{ GPlatesFileIO::ReadErrors::NoExclMarkToStartComment,
				QT_TR_NOOP("No exclamation mark was found before the start of the comment.") },
		{ GPlatesFileIO::ReadErrors::SamePlateIdsButDuplicateGeoTime,
				QT_TR_NOOP("Consecutive poles had the same plate IDs and identical geo-times.") },
		{ GPlatesFileIO::ReadErrors::SamePlateIdsButEarlierGeoTime,
				QT_TR_NOOP("Consecutive poles had the same plate IDs and overlapping geo-times.") },
		
		// Generic file-related errors
		{ GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
				QT_TR_NOOP("Error opening the file for reading.") },
	};
	
	/**
	 * This table is sourced from http://trac.gplates.org/wiki/ReadErrorMessages
	 */
	static ReadErrorResult result_table[] = {
		// Errors from PLATES Line Format files
		{ GPlatesFileIO::ReadErrors::UnclassifiedFeatureCreated,
				QT_TR_NOOP("Because the 'Data Type Code' was not known, Unclassified Features will be created.") },
		{ GPlatesFileIO::ReadErrors::FeatureDiscarded,
				QT_TR_NOOP("The feature was discarded due to errors encountered when parsing.") },

		// Errors from PLATES Rotation Format files
		{ GPlatesFileIO::ReadErrors::EmptyCommentCreated,
				QT_TR_NOOP("An empty comment was created.") },
		{ GPlatesFileIO::ReadErrors::ExclMarkInsertedAtCommentStart,
				QT_TR_NOOP("An exclamation mark was inserted to start the comment.") },
		{ GPlatesFileIO::ReadErrors::MovingPlateIdChangedToMatchEarlierSequence,
				QT_TR_NOOP("The moving plate ID of the pole was changed to match the earlier sequence.") },
		{ GPlatesFileIO::ReadErrors::NewOverlappingSequenceBegun,
				QT_TR_NOOP("A new sequence was begun which overlaps.") },
		{ GPlatesFileIO::ReadErrors::PoleDiscarded,
				QT_TR_NOOP("The pole was discarded.") },

		// Generic file-related errors
		{ GPlatesFileIO::ReadErrors::FileNotLoaded,
				QT_TR_NOOP("The file was not loaded.") },
	};

	
	/**
	 * Converts the statically defined description table to a STL map.
	 */
	const description_map_type &
	build_description_map()
	{
		static description_map_type map;
		
		ReadErrorDescription *begin = description_table;
		ReadErrorDescription *end = begin + NUM_ELEMS(description_table);
		for (; begin != end; ++begin) {
			map[begin->code] = QObject::tr(begin->text);
		}
		
		return map;
	}
	
	/**
	 * Converts the statically defined result table to a STL map.
	 */
	const result_map_type &
	build_result_map()
	{
		static result_map_type map;
		
		ReadErrorResult *begin = result_table;
		ReadErrorResult *end = begin + NUM_ELEMS(result_table);
		for (; begin != end; ++begin) {
			map[begin->code] = QObject::tr(begin->text);
		}
		
		return map;
	}

	/**
	 * Takes a read_error_collection_type and creates a map of read_error_collection_types,
	 * grouped by filename.
	 */
	void
	group_read_errors_by_file(
			file_errors_map_type &file_errors,
			const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors)
	{
		GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
		GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
		for (; it != end; ++it) {
			// Add to map based on filename
			std::ostringstream source;
			it->d_data_source->write_full_name(source);
			
			file_errors[source.str()].push_back(*it);
		}
	}
	
}


GPlatesQtWidgets::ReadErrorAccumulationDialog::ReadErrorAccumulationDialog(
		QWidget *parent_):
	QDialog(parent_)
{
	setupUi(this);
	clear();
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::clear()
{
	// Failures to Begin (n)
	d_tree_item_failures_to_begin_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_failures_to_begin_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_failures_to_begin_ptr);
	// Terminating Errors (n)
	d_tree_item_terminating_errors_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_terminating_errors_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_terminating_errors_ptr);
	// Recoverable Errors (n)
	d_tree_item_recoverable_errors_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_recoverable_errors_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_recoverable_errors_ptr);
	// Warnings (n)
	d_tree_item_warnings_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_warnings_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_warnings_ptr);
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::update()
{
	// Populate Failures to Begin tree
	populate_top_level_tree_widget(d_tree_item_failures_to_begin_ptr, tr("Failure to Begin (%1)"),
			d_read_errors.d_failures_to_begin, QIcon(":/gnome_dialog_error_16.png"));

	// Populate Terminating Errors tree
	populate_top_level_tree_widget(d_tree_item_terminating_errors_ptr, tr("Terminating Errors (%1)"),
			d_read_errors.d_terminating_errors, QIcon(":/gnome_dialog_error_16.png"));

	// Populate Recoverable Errors tree
	populate_top_level_tree_widget(d_tree_item_recoverable_errors_ptr, tr("Recoverable Errors (%1)"),
			d_read_errors.d_recoverable_errors, QIcon(":/gnome_dialog_error_16.png"));

	// Populate Warnings tree
	populate_top_level_tree_widget(d_tree_item_warnings_ptr, tr("Warnings (%1)"),
			d_read_errors.d_warnings, QIcon(":/gnome_dialog_warning_16.png"));

	// Update labels
	QString summary_str = build_summary_string();
	label_problem->setText(summary_str);
}

void
GPlatesQtWidgets::ReadErrorAccumulationDialog::populate_top_level_tree_widget(
		QTreeWidgetItem *tree_item_ptr,
		QString tree_item_text,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	// Un-hide the top-level item if it has content to add, and update the text
	if ( ! errors.empty()) {
		tree_item_ptr->setText(0, tree_item_text.arg(errors.size()));
		tree_item_ptr->setHidden(false);
		tree_item_ptr->setExpanded(true);
	}
	
	// Build map of Filename -> Error collection
	file_errors_map_type file_errors;
	group_read_errors_by_file(file_errors, errors);
	
	// Iterate over map to add file errors of this type grouped by file
	file_errors_map_const_iterator it = file_errors.begin();
	file_errors_map_const_iterator end = file_errors.end();
	for (; it != end; ++it) {
		create_file_tree_widget(tree_item_ptr, it->second, occurrence_icon);
	}
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_file_tree_widget(
		QTreeWidgetItem *tree_item_ptr,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	if (errors.empty()) {
		return;
	}
	// We must refer to the first entry to get the path info we need.
	const GPlatesFileIO::ReadErrorOccurrence &first_error = errors[0];
	
	static const QIcon file_icon(":/gnome_text_file_16.png");
	static const QIcon path_icon(":/gnome_folder_16.png");

	// Add filename.dat (format)
	QTreeWidgetItem *file_item = new QTreeWidgetItem(tree_item_ptr);
	std::ostringstream file_str;
	first_error.write_short_name(file_str);
	file_item->setText(0, QString::fromAscii(file_str.str().c_str()));
 	file_item->setIcon(0, file_icon);
	file_item->setExpanded(true);
	
	// Add full path to file subitem
	QTreeWidgetItem *path_item = new QTreeWidgetItem(file_item);
	std::ostringstream path_str;
	first_error.d_data_source->write_full_name(path_str);
	path_item->setText(0, QString::fromAscii(path_str.str().c_str()));
 	path_item->setIcon(0, path_icon);
	
	// Add error occurrences for this file for this error type:
	GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
	GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
	for (; it != end; ++it) {
		create_error_tree(file_item, *it, occurrence_icon);
	}
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_error_tree(
		QTreeWidgetItem *parent_item_ptr,
		const GPlatesFileIO::ReadErrorOccurrence &error,
		const QIcon &occurrence_icon)
{
	static const QIcon info_icon(":/gnome_folder_16.png");
	static const QIcon description_icon(":/gnome_help_agent_16.png");
	static const QIcon result_icon(":/gnome_gtk_edit_16.png");

	// Create node with error location.
	QTreeWidgetItem *location_item = new QTreeWidgetItem(parent_item_ptr);
	std::ostringstream location_str;
	location_str << "Line ";
	error.d_location->write(location_str);
	location_item->setText(0, QString::fromAscii(location_str.str().c_str()));
	location_item->setIcon(0, occurrence_icon);
	location_item->setExpanded(false);

	// Create node with description as child of location
	QTreeWidgetItem *description_item = new QTreeWidgetItem(location_item);
	description_item->setText(0, get_description_as_string(error.d_description));
	description_item->setIcon(0, description_icon);

	// Create node with result as child of location
	QTreeWidgetItem *result_item = new QTreeWidgetItem(location_item);
	result_item->setText(0, get_result_as_string(error.d_result));
	result_item->setIcon(0, result_icon);
}


const QString
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_summary_string()
{
	size_t num_failures = d_read_errors.d_failures_to_begin.size() +
			d_read_errors.d_terminating_errors.size();
	size_t num_recoverable_errors = d_read_errors.d_recoverable_errors.size();
	size_t num_warnings = d_read_errors.d_warnings.size();
	
	// Work out errors first - prefix can be affected by quantity of first error listed.
	QString prefix(tr("There were"));
	QString errors("");
	// failures?
	if (num_failures > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_failures <= 1) {
				prefix = tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_failures > 1) {
			errors.append(tr("%1 failures").arg(num_failures));
		} else {
			errors.append(tr("%1 failure").arg(num_failures));
		}
	}
	// errors?
	if (num_recoverable_errors > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_recoverable_errors <= 1) {
				prefix = tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_recoverable_errors > 1) {
			errors.append(tr("%1 errors").arg(num_recoverable_errors));
		} else {
			errors.append(tr("%1 error").arg(num_recoverable_errors));
		}
	}
	// warnings?
	if (num_warnings > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_warnings <= 1) {
				prefix = tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_warnings > 1) {
			errors.append(tr("%1 warnings").arg(num_warnings));
		} else {
			errors.append(tr("%1 warning").arg(num_warnings));
		}
	}
	// nothing wrong?
	if (errors.isEmpty()) {
		errors.append(tr(" no problems"));
	}
	
	// Build final summary string
	QString summary("");
	summary.append(prefix);
	summary.append(errors);
	summary.append(".");
	
	return summary;
}


const QString &
GPlatesQtWidgets::ReadErrorAccumulationDialog::get_description_as_string(
		GPlatesFileIO::ReadErrors::Description code)
{
	static const QString description_not_found = QObject::tr("(Text not found for error description code.)");
	static const description_map_type &map = build_description_map();
	
	description_map_const_iterator r = map.find(code);
	if (r != map.end()) {
		return r->second;
	} else {
		return description_not_found;
	}
}


const QString &
GPlatesQtWidgets::ReadErrorAccumulationDialog::get_result_as_string(
		GPlatesFileIO::ReadErrors::Result code)
{
	static const QString result_not_found = QObject::tr("(Text not found for error result code.)");
	static const result_map_type &map = build_result_map();
	
	result_map_const_iterator r = map.find(code);
	if (r != map.end()) {
		return r->second;
	} else {
		return result_not_found;
	}
}



