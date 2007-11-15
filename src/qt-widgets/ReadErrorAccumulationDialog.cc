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

	// Map of Filename -> Error collection for reporting all errors of a particular type for each file.
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
	 * This table is sourced from http://trac.gplates.org/wiki/ReadErrorMessages .
	 */
	static ReadErrorDescription description_table[] = {
		// Errors from PLATES Line Format files:
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
		
		// Errors from PLATES Rotation Format files:
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
		
		// Generic file-related errors:
		{ GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
				QT_TR_NOOP("Error opening the file for reading.") },
	};
	
	/**
	 * This table is sourced from http://trac.gplates.org/wiki/ReadErrorMessages .
	 */
	static ReadErrorResult result_table[] = {
		// Errors from PLATES Line Format files:
		{ GPlatesFileIO::ReadErrors::UnclassifiedFeatureCreated,
				QT_TR_NOOP("Because the 'Data Type Code' was not known, Unclassified Features will be created.") },
		{ GPlatesFileIO::ReadErrors::FeatureDiscarded,
				QT_TR_NOOP("The feature was discarded due to errors encountered when parsing.") },

		// Errors from PLATES Rotation Format files:
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

		// Generic file-related errors:
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
			// Add the error to the map based on filename.
			std::ostringstream source;
			it->d_data_source->write_full_name(source);
			
			file_errors[source.str()].push_back(*it);
		}
	}
	
}


GPlatesQtWidgets::ReadErrorAccumulationDialog::ReadErrorAccumulationDialog(
		QWidget *parent_):
	QDialog(parent_),
	d_information_dialog(s_information_dialog_text, s_information_dialog_title, this)
{
	setupUi(this);
	clear();
	
	QObject::connect(button_help, SIGNAL(clicked()),
			this, SLOT(pop_up_help_dialog()));

	QObject::connect(button_expand_all, SIGNAL(clicked()),
			this, SLOT(expandAll()));
	QObject::connect(button_collapse_all, SIGNAL(clicked()),
			this, SLOT(collapseAll()));
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::clear()
{
	label_problem->setText(tr("There are no warnings or errors for the currently-loaded files."));
	treeWidget_errors->clear();
	// Add the "Failures to Begin (n)" item.
	d_tree_item_failures_to_begin_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_failures_to_begin_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_failures_to_begin_ptr);
	// Add the "Terminating Errors (n)" item.
	d_tree_item_terminating_errors_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_terminating_errors_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_terminating_errors_ptr);
	// Add the "Recoverable Errors (n)" item.
	d_tree_item_recoverable_errors_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_recoverable_errors_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_recoverable_errors_ptr);
	// Add the "Warnings (n)" item.
	d_tree_item_warnings_ptr = new QTreeWidgetItem(treeWidget_errors);
	d_tree_item_warnings_ptr->setHidden(true);
	treeWidget_errors->addTopLevelItem(d_tree_item_warnings_ptr);
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::update()
{
	// Populate the Failures to Begin tree.
	populate_top_level_tree_widget(d_tree_item_failures_to_begin_ptr, tr("Failure to Begin (%1)"),
			d_read_errors.d_failures_to_begin, QIcon(":/gnome_dialog_error_16.png"));

	// Populate the Terminating Errors tree.
	populate_top_level_tree_widget(d_tree_item_terminating_errors_ptr, tr("Terminating Errors (%1)"),
			d_read_errors.d_terminating_errors, QIcon(":/gnome_dialog_error_16.png"));

	// Populate the Recoverable Errors tree.
	populate_top_level_tree_widget(d_tree_item_recoverable_errors_ptr, tr("Recoverable Errors (%1)"),
			d_read_errors.d_recoverable_errors, QIcon(":/gnome_dialog_error_16.png"));

	// Populate the Warnings tree.
	populate_top_level_tree_widget(d_tree_item_warnings_ptr, tr("Warnings (%1)"),
			d_read_errors.d_warnings, QIcon(":/gnome_dialog_warning_16.png"));

	// Update labels.
	QString summary_str = build_summary_string();
	label_problem->setText(summary_str);
}


const QString GPlatesQtWidgets::ReadErrorAccumulationDialog::s_information_dialog_text = QObject::tr(
		"<html><body>\n"
		"Read errors fall into four categories: <ul> <li>failures to begin</li> "
		"<li>terminating errors</li> <li>recoverable errors</li> <li>warnings</li> </ul>\n"
		"\n"
		"<h3>Failure To Begin:</h3>\n"
		"<ul>\n"
		"<li> A failure to begin has occurred when GPlates is not even able to start reading "
		"data from the data source. </li>\n"
		"<li> Examples of failures to begin might include: the file cannot be located on disk "
		"or opened for reading; the database cannot be accessed; no network connection "
		"could be established. </li>\n"
		"<li> In the event of a failure to begin, GPlates will not be able to load any data "
		"from the data source. </li>\n"
		"</ul>\n"
		"<h3>Terminating Error:</h3>\n"
		"<ul>\n"
		"<li> A terminating error halts the reading of data in such a way that GPlates is "
		"unable to read any more data from the data source. </li>\n"
		"<li> Examples of terminating errors might include: a file-system error; a broken "
		"network connection. </li>\n"
		"<li> When a terminating error occurs, GPlates will retain the data it has already "
		"read, but will not be able to read any more data from the data source. </li>\n"
		"</ul>\n"
		"<h3>Recoverable Error:</h3>\n"
		"<ul>\n"
		"<li> A recoverable error is an error from which GPlates is able to recover (generally "
		"an error in the data), although some amount of data had to be discarded because "
		"it was invalid or malformed. </li>\n"
		"<li> Examples of recoverable errors might include: corrupted data; the wrong type of "
		"data encountered in an attribute field (for example, text where an integer was "
		"expected). </li>\n"
		"<li> When a terminating error occurs, GPlates will retain the data it has already "
		"successfully read; discard the invalid or malformed data (which will result in "
		"some data loss); and continue reading from the data source. GPlates will discard "
		"the smallest possible amount of data, and will inform you exactly what was discarded. "
		"</li>\n"
		"</ul>\n"
		"<h3>Warning:</h3>\n"
		"<ul>\n"
		"<li> A warning is a notification of a problem which has not caused data loss, but "
		"which has resulted in GPlates handling the situation in an arbitrary or "
		"questionable manner. </li>\n"
		"<li> Examples of warnings might include: warnings about ambiguous data (in which the "
		"developers have had to choose one particular way of handling the data); warnings "
		"about deprecated syntax or a deprecated file format (which may be permanently "
		"disabled in the future).</li>\n"
		"<li> A warning will not have resulted in any data loss, but the user may wish to "
		"investigate the warning, in order to verify that GPlates handled the ambiguous "
		"data in an appropriate fashion; to upgrade the file format of the data; or to be "
		"aware that other programs may handle the ambiguous data differently. </li>\n"
		"</ul>\n"
		"<i>Please be aware that all software needs to respond to situations such as these; "
		"GPlates is simply informing you when these situations occur! The integrity of your "
		"data is important to us!</i>\n"
		"</body></html>\n");

const QString GPlatesQtWidgets::ReadErrorAccumulationDialog::s_information_dialog_title = QObject::tr(
		"Read error types");

void
GPlatesQtWidgets::ReadErrorAccumulationDialog::populate_top_level_tree_widget(
		QTreeWidgetItem *tree_item_ptr,
		QString tree_item_text,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	// Un-hide the top-level item if it has content to add, and update the text.
	if ( ! errors.empty()) {
		tree_item_ptr->setText(0, tree_item_text.arg(errors.size()));
		tree_item_ptr->setHidden(false);
		tree_item_ptr->setExpanded(true);
	}
	
	// Build map of Filename -> Error collection.
	file_errors_map_type file_errors;
	group_read_errors_by_file(file_errors, errors);
	
	// Iterate over map to add file errors of this type grouped by file.
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

	// Add the "filename.dat (format)" item.
	QTreeWidgetItem *file_item = new QTreeWidgetItem(tree_item_ptr);
	std::ostringstream file_str;
	first_error.write_short_name(file_str);
	file_item->setText(0, QString::fromAscii(file_str.str().c_str()));
 	file_item->setIcon(0, file_icon);
	file_item->setExpanded(true);
	
	// Add the full path subitem.
	QTreeWidgetItem *path_item = new QTreeWidgetItem(file_item);
	std::ostringstream path_str;
	first_error.d_data_source->write_full_name(path_str);
	path_item->setText(0, QString::fromAscii(path_str.str().c_str()));
 	path_item->setIcon(0, path_icon);
	
	// Add all error occurrences for this file, for this error type.
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
	location_str << " [" << error.d_description << "; " <<
			error.d_result << "]";
	location_item->setText(0, QString::fromAscii(location_str.str().c_str()));
	location_item->setIcon(0, occurrence_icon);
	location_item->setExpanded(false);

	// Create leaf node with description as child of location.
	QTreeWidgetItem *description_item = new QTreeWidgetItem(location_item);
	description_item->setText(0, QString("[%1] %2")
			.arg(error.d_description)
			.arg(get_description_as_string(error.d_description)));
	description_item->setIcon(0, description_icon);

	// Create leaf node with result as child of location.
	QTreeWidgetItem *result_item = new QTreeWidgetItem(location_item);
	result_item->setText(0, QString("[%1] %2")
			.arg(error.d_result)
			.arg(get_result_as_string(error.d_result)));
	result_item->setIcon(0, result_icon);
}


const QString
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_summary_string()
{
	size_t num_failures = d_read_errors.d_failures_to_begin.size() +
			d_read_errors.d_terminating_errors.size();
	size_t num_recoverable_errors = d_read_errors.d_recoverable_errors.size();
	size_t num_warnings = d_read_errors.d_warnings.size();
	
	/*
	 * Firstly, work out what errors need to be summarised.
	 * The prefix of the sentence is affected by the quantity of the first error listed.
	 */
	QString prefix(tr("There were"));
	QString errors("");
	// Build sentence fragment for failures.
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
	// Build sentence fragment for errors.
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
	// Build sentence fragment for warnings.
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
	// Build sentence fragment for no problems.
	if (errors.isEmpty()) {
		errors.append(tr(" no problems"));
	}
	
	// Finally, build the whole summary string.
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



