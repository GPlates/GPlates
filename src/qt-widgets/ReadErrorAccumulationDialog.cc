/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2010 The University of Sydney, Australia
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
#include <QDir>

#include "ReadErrorAccumulationDialog.h"

#include "file-io/ReadErrorMessages.h"
#include "file-io/ReadErrorUtils.h"


GPlatesQtWidgets::ReadErrorAccumulationDialog::ReadErrorAccumulationDialog(
		QWidget *parent_):
	GPlatesDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_information_dialog(s_information_dialog_text, s_information_dialog_title, this)
{
	setupUi(this);
	clear();
	
	QObject::connect(
			button_help,
			SIGNAL(clicked()),
			this,
			SLOT(pop_up_help_dialog()));

	QObject::connect(
			button_expand_all,
			SIGNAL(clicked()),
			this,
			SLOT(expandAll()));
	QObject::connect(
			button_collapse_all,
			SIGNAL(clicked()),
			this,
			SLOT(collapseAll()));

	// Set up the button box.
	QObject::connect(
			main_buttonbox,
			SIGNAL(clicked(QAbstractButton *)),
			this,
			SLOT(handle_buttonbox_clicked(QAbstractButton *)));
	main_buttonbox->button(QDialogButtonBox::Reset)->setText("Clea&r All");
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::handle_buttonbox_clicked(
		QAbstractButton *button)
{
	QDialogButtonBox::StandardButton button_enum = main_buttonbox->standardButton(button);
	if (button_enum == QDialogButtonBox::Reset)
	{
		clear_errors();
	}
	else if (button_enum == QDialogButtonBox::Close)
	{
		accept();
	}
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::clear()
{
	label_problem->setText(tr("There are no warnings or errors for the currently-loaded files."));
	
	// Clear errors from the "By Error" tab.
	tree_widget_errors_by_type->clear();
	// Add the "Failures to Begin (n)" item.
	d_tree_type_failures_to_begin_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_failures_to_begin_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_failures_to_begin_ptr);
	// Add the "Terminating Errors (n)" item.
	d_tree_type_terminating_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_terminating_errors_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_terminating_errors_ptr);
	// Add the "Recoverable Errors (n)" item.
	d_tree_type_recoverable_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_recoverable_errors_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_recoverable_errors_ptr);
	// Add the "Warnings (n)" item.
	d_tree_type_warnings_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_warnings_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_warnings_ptr);

	// Clear errors from the "By Line" tab.
	tree_widget_errors_by_line->clear();
	// Add the "Failures to Begin (n)" item.
	d_tree_line_failures_to_begin_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_failures_to_begin_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_failures_to_begin_ptr);
	// Add the "Terminating Errors (n)" item.
	d_tree_line_terminating_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_terminating_errors_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_terminating_errors_ptr);
	// Add the "Recoverable Errors (n)" item.
	d_tree_line_recoverable_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_recoverable_errors_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_recoverable_errors_ptr);
	// Add the "Warnings (n)" item.
	d_tree_line_warnings_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_warnings_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_warnings_ptr);
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::update()
{
	static const QIcon icon_error(":/gnome_dialog_error_16.png");
	static const QIcon icon_warning(":/gnome_dialog_warning_16.png");
	
	// Disabling screen updates to work around Qt slowness when >1000 warnings.
	// http://doc.trolltech.com/4.3/qwidget.html#updatesEnabled-prop
	// Not as huge a speedup as I hoped, but every little bit helps.
	setUpdatesEnabled(false);
	
	// Populate the Failures to Begin tree by type.
	populate_top_level_tree_by_type(d_tree_type_failures_to_begin_ptr, tr("Failure to Begin (%1)"),
			d_read_errors.d_failures_to_begin, icon_error);

	// Populate the Terminating Errors tree by type.
	populate_top_level_tree_by_type(d_tree_type_terminating_errors_ptr, tr("Terminating Errors (%1)"),
			d_read_errors.d_terminating_errors, icon_error);

	// Populate the Recoverable Errors tree by type.
	populate_top_level_tree_by_type(d_tree_type_recoverable_errors_ptr, tr("Recoverable Errors (%1)"),
			d_read_errors.d_recoverable_errors, icon_error);

	// Populate the Warnings tree by type.
	populate_top_level_tree_by_type(d_tree_type_warnings_ptr, tr("Warnings (%1)"),
			d_read_errors.d_warnings, icon_warning);

	// Populate the Failures to Begin tree by line.
	populate_top_level_tree_by_line(d_tree_line_failures_to_begin_ptr, tr("Failure to Begin (%1)"),
			d_read_errors.d_failures_to_begin, icon_error);

	// Populate the Terminating Errors tree by line.
	populate_top_level_tree_by_line(d_tree_line_terminating_errors_ptr, tr("Terminating Errors (%1)"),
			d_read_errors.d_terminating_errors, icon_error);

	// Populate the Recoverable Errors tree by line.
	populate_top_level_tree_by_line(d_tree_line_recoverable_errors_ptr, tr("Recoverable Errors (%1)"),
			d_read_errors.d_recoverable_errors, icon_error);

	// Populate the Warnings tree by line.
	populate_top_level_tree_by_line(d_tree_line_warnings_ptr, tr("Warnings (%1)"),
			d_read_errors.d_warnings, icon_warning);

	// Update labels.
	QString summary_str = GPlatesFileIO::ReadErrorUtils::build_summary_string(d_read_errors);
	label_problem->setText(summary_str);

	// Re-enable screen updates after all items have been added.
	// Re-enabling implicitly calls update() on the widget.
	setUpdatesEnabled(true);
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
		"<li> A recoverable error is an error (generally an error in the data) from which "
		"GPlates is able to recover, although some amount of data had to be discarded "
		"because it was invalid or malformed in such a way that GPlates was unable to repair "
		"it. </li>\n"
		"<li> Examples of recoverable errors might include: when the wrong type of data "
		"encountered in a fixed-width attribute field (for instance, text encountered where "
		"an integer was expected). </li>\n"
		"<li> When a recoverable error occurs, GPlates will retain the data it has already "
		"successfully read; discard the invalid or malformed data (which will result in "
		"some data loss); and continue reading from the data source. GPlates will discard "
		"the smallest possible amount of data, and will inform you exactly what was discarded. "
		"</li>\n"
		"</ul>\n"
		"<h3>Warning:</h3>\n"
		"<ul>\n"
		"<li> A warning is a notification of a problem (generally a problem in the data) "
		"which required GPlates to modify the data in order to rectify the situation. "
		"<li> Examples of problems which might result in warnings include: data which are "
		"being imported into GPlates, which do not possess <i>quite</i> enough information "
		"for the needs of GPlates (such as total reconstruction poles in PLATES4 "
		"rotation-format files which have been commented-out by changing their moving plate "
		"ID to 999); an attribute field whose value is obviously incorrect, but which is easy "
		"for GPlates to repair (for instance, when the 'Number Of Points' field in a PLATES4 "
		"line-format polyline header does not match the actual number of points in the "
		"polyline). </li>\n"
		"<li> A warning will not have resulted in any data loss, but you may wish to "
		"investigate the problem, in order to verify that GPlates has 'corrected' the "
		"incorrect data in the way you would expect; and to be aware of incorrect data which "
		"other programs may handle differently. </li>\n"
		"</ul>\n"
		"<i>Please be aware that all software needs to respond to situations such as these; "
		"GPlates is simply informing you when these situations occur!<i>\n"
		"</body></html>\n");

const QString GPlatesQtWidgets::ReadErrorAccumulationDialog::s_information_dialog_title = QObject::tr(
		"Read error types");

void
GPlatesQtWidgets::ReadErrorAccumulationDialog::populate_top_level_tree_by_type(
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
	GPlatesFileIO::ReadErrorUtils::errors_by_file_map_type errors_by_file;
	GPlatesFileIO::ReadErrorUtils::group_read_errors_by_file(errors_by_file, errors);
	
	// Iterate over map to add file errors of this type grouped by file.
	GPlatesFileIO::ReadErrorUtils::errors_by_file_map_const_iterator it = errors_by_file.begin();
	GPlatesFileIO::ReadErrorUtils::errors_by_file_map_const_iterator end = errors_by_file.end();
	for (; it != end; ++it) {
		build_file_tree_by_type(tree_item_ptr, it->second, occurrence_icon);
	}
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::populate_top_level_tree_by_line(
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
	GPlatesFileIO::ReadErrorUtils::errors_by_file_map_type errors_by_file;
	GPlatesFileIO::ReadErrorUtils::group_read_errors_by_file(errors_by_file, errors);
	
	// Iterate over map to add file errors of this type grouped by file.
	GPlatesFileIO::ReadErrorUtils::errors_by_file_map_const_iterator it = errors_by_file.begin();
	GPlatesFileIO::ReadErrorUtils::errors_by_file_map_const_iterator end = errors_by_file.end();
	for (; it != end; ++it) {
		build_file_tree_by_line(tree_item_ptr, it->second, occurrence_icon);
	}
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_file_tree_by_type(
		QTreeWidgetItem *parent_item_ptr,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	if (errors.empty()) {
		return;
	}
	// We must refer to the first entry to get the path info we need.
	const GPlatesFileIO::ReadErrorOccurrence &first_error = errors[0];
	
	QTreeWidgetItem *file_info_item = create_occurrence_file_info_item(first_error);
	
	file_info_item->addChild(create_occurrence_file_path_item(first_error));

	// Build map of Description (enum) -> Error collection.
	GPlatesFileIO::ReadErrorUtils::errors_by_type_map_type errors_by_type;
	GPlatesFileIO::ReadErrorUtils::group_read_errors_by_type(errors_by_type, errors);
	
	// Iterate over map to add file errors of this type grouped by description.
	GPlatesFileIO::ReadErrorUtils::errors_by_type_map_const_iterator it = errors_by_type.begin();
	GPlatesFileIO::ReadErrorUtils::errors_by_type_map_const_iterator end = errors_by_type.end();
	for (; it != end; ++it) {
		QTreeWidgetItem *summary_item = create_occurrence_type_summary_item(
				it->second[0], occurrence_icon, it->second.size());
		
		static const QIcon file_line_icon(":/gnome_edit_find_16.png");
		build_occurrence_line_list(summary_item, it->second, file_line_icon, false);
		
		file_info_item->addChild(summary_item);
		summary_item->setExpanded(false);
	}
	
	parent_item_ptr->addChild(file_info_item);
	file_info_item->setExpanded(true);	// setExpanded won't have effect until after addChild!
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_file_tree_by_line(
		QTreeWidgetItem *parent_item_ptr,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	if (errors.empty()) {
		return;
	}
	// We must refer to the first entry to get the path info we need.
	const GPlatesFileIO::ReadErrorOccurrence &first_error = errors[0];
	
	QTreeWidgetItem *file_info_item = create_occurrence_file_info_item(first_error);
	
	file_info_item->addChild(create_occurrence_file_path_item(first_error));

	build_occurrence_line_list(file_info_item, errors, occurrence_icon, true);
	
	parent_item_ptr->addChild(file_info_item);
	file_info_item->setExpanded(true);	// setExpanded won't have effect until after addChild!
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_occurrence_line_list(
		QTreeWidgetItem *parent_item_ptr,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon,
		bool show_short_description)
{
	// Add all error occurrences for this file, for this error type.
	GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
	GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
	for (; it != end; ++it) {
		// For each occurrence, add a Line node with Description and Result nodes as children.
		QTreeWidgetItem *location_item = create_occurrence_line_item(
				*it, occurrence_icon, show_short_description);
		
		location_item->addChild(create_occurrence_description_item(*it));
		location_item->addChild(create_occurrence_result_item(*it));

		parent_item_ptr->addChild(location_item);
		location_item->setExpanded(false);
	}
}



QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_type_summary_item(
		const GPlatesFileIO::ReadErrorOccurrence &error,
		const QIcon &occurrence_icon,
		size_t quantity)
{
	// Create node with a summary of the error description and how many there are.
	QTreeWidgetItem *summary_item = new QTreeWidgetItem();
	summary_item->setText(0, QString("[%1] %2 (%3)")
			.arg(error.d_description)
			.arg(GPlatesFileIO::ReadErrorMessages::get_short_description_as_string(error.d_description))
			.arg(quantity) );
	summary_item->setIcon(0, occurrence_icon);
	
	return summary_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_file_info_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon file_icon(":/gnome_text_file_16.png");

	// Add the "filename.dat (format)" item.
	QTreeWidgetItem *file_item = new QTreeWidgetItem();
	std::ostringstream file_str;
	error.write_short_name(file_str);
	file_item->setText(0, QString::fromAscii(file_str.str().c_str()));
 	file_item->setIcon(0, file_icon);
	
	return file_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_file_path_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon path_icon(":/gnome_folder_16.png");

	// Add the full path item.
	QTreeWidgetItem *path_item = new QTreeWidgetItem();
	std::ostringstream path_str;
	error.d_data_source->write_full_name(path_str);
	path_item->setText(0, QDir::toNativeSeparators(QString::fromAscii(path_str.str().c_str())));
 	path_item->setIcon(0, path_icon);
 	
 	return path_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_line_item(
		const GPlatesFileIO::ReadErrorOccurrence &error,
		const QIcon &occurrence_icon,
		bool show_short_description)
{
	// Create node with a single line error occurrence, with a summary of the error description.
	QTreeWidgetItem *location_item = new QTreeWidgetItem();
	std::ostringstream location_str;
	error.d_location->write(location_str);
	if (show_short_description) {
		location_item->setText(0, QString("Line %1 [%2; %3] %4")
				.arg(QString::fromAscii(location_str.str().c_str()))
				.arg(error.d_description)
				.arg(error.d_result)
				.arg(GPlatesFileIO::ReadErrorMessages::get_short_description_as_string(error.d_description)) );
	} else {
		location_item->setText(0, QString("Line %1 [%2; %3]")
				.arg(QString::fromAscii(location_str.str().c_str()))
				.arg(error.d_description)
				.arg(error.d_result) );
	}
	location_item->setIcon(0, occurrence_icon);
	
	return location_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_description_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon description_icon(":/gnome_help_agent_16.png");

	// Create leaf node with full description.
	QTreeWidgetItem *description_item = new QTreeWidgetItem();
	description_item->setText(0, QString("[%1] %2")
			.arg(error.d_description)
			.arg(GPlatesFileIO::ReadErrorMessages::get_full_description_as_string(error.d_description)) );
	description_item->setIcon(0, description_icon);
	
	return description_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_result_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon result_icon(":/gnome_gtk_edit_16.png");

	// Create leaf node with result text.
	QTreeWidgetItem *result_item = new QTreeWidgetItem();
	result_item->setText(0, QString("[%1] %2")
			.arg(error.d_result)
			.arg(GPlatesFileIO::ReadErrorMessages::get_result_as_string(error.d_result)) );
	result_item->setIcon(0, result_icon);
	
	return result_item;
}
