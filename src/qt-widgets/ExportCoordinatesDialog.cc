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

#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QByteArray>
#include <QMimeData>
#include <QClipboard>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <memory>

#include "ExportCoordinatesDialog.h"

#include "InformationDialog.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "file-io/PlatesLineFormatGeometryExporter.h"
#include "file-io/GMTFormatGeometryExporter.h"


namespace
{
	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;
}


const QString GPlatesQtWidgets::ExportCoordinatesDialog::s_terminating_point_information_text =
		QObject::tr("<html><body>\n"
		"<h3>Including an additional terminating point for polygons</h3>"
		"<p>GPlates stores polygons using the minimum number of vertices necessary to specify "
		"a closed polygon.</p>\n"
		"<p>However, some software may expect the final point of the polygon to be identical "
		"to the first point, in order to create a closed circuit. If this box is checked, "
		"the exported data will include an additional terminating point identical to the first.</p>\n"
		"</body></html>");


GPlatesQtWidgets::ExportCoordinatesDialog::ExportCoordinatesDialog(
		QWidget *parent_):
	QDialog(parent_),
	d_geometry_opt_ptr(boost::none),
	d_export_file_dialog(new QFileDialog(this, tr("Select a filename for exporting"), QString(),
			"All files (*)")),
	d_terminating_point_information_dialog(new InformationDialog(s_terminating_point_information_text,
			tr("Polygon point conventions"), this))
{
	setupUi(this);
	
	// Disable some things we're not going to implement just yet.
	combobox_format->removeItem(CSV);
	combobox_format->removeItem(WKT);
	
	// What happens when the user selects a format?
	QObject::connect(combobox_format, SIGNAL(currentIndexChanged(int)),
			this, SLOT(handle_format_selection(int)));
	
	// The "Terminating Point" option for polygons.
	QObject::connect(button_explain_terminating_point, SIGNAL(clicked()),
			d_terminating_point_information_dialog, SLOT(show()));
	
	// Radio buttons control destination selection widgets.
	QObject::connect(radiobutton_to_file, SIGNAL(clicked()),
			this, SLOT(select_to_file_stack()));
	QObject::connect(radiobutton_to_clipboard, SIGNAL(clicked()),
			this, SLOT(select_to_clipboard_stack()));

	// Set up the export file selection dialog.
	d_export_file_dialog->setAcceptMode(QFileDialog::AcceptSave);
	QObject::connect(button_select_file, SIGNAL(clicked()),
			this, SLOT(pop_up_file_browser()));
	
	// Default 'OK' button should read 'Export'.
	QPushButton *button_export = buttonbox_export->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
	button_export->setDefault(true);
	QObject::connect(buttonbox_export, SIGNAL(accepted()),
			this, SLOT(handle_export()));
	
	// Select the default output format, to ensure that any currently-displayed
	// widgets will be initialised to the appropriate defaults.
	combobox_format->setCurrentIndex(PLATES4);
	handle_format_selection(PLATES4);
}


bool
GPlatesQtWidgets::ExportCoordinatesDialog::set_geometry_and_display(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_)
{
	// The geometry is passed in as a GeometryOnSphere::non_null_ptr_to_const_type
	// because we want to enforce that this dialog should be given valid geometry
	// if you want it to display itself. However, we must set our d_geometry_ptr member
	// as a boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, because
	// it cannot be initialised with any meaningful value at this dialog's creation time.
	d_geometry_opt_ptr = geometry_;
	
	if ( ! d_geometry_opt_ptr) {
		// We shouldn't let the dialog open without a valid geometry.
		return false;
	}
	
	// Show the dialog modally.
	return exec();
}


void
GPlatesQtWidgets::ExportCoordinatesDialog::handle_format_selection(
		int idx)
{
	OutputFormat format = OutputFormat(idx);
	switch (format)
	{
	case PLATES4:
		// Set some default options to match what the format prescribes.
		combobox_coordinate_order->setCurrentIndex(LAT_LON);
		checkbox_polygon_terminating_point->setChecked(true);
		break;

	case GMT:
		// Set some default options to match what the format prescribes.
		// Default order for GMT is (lon,lat) which is opposite of PLATES4.
		combobox_coordinate_order->setCurrentIndex(LON_LAT);
		checkbox_polygon_terminating_point->setChecked(true);
		break;

	case WKT:
		break;
	case CSV:
		break;
	default:
		// Can't possibly happen, because we set up each possible combobox state, right?
		return;
	}
}


void
GPlatesQtWidgets::ExportCoordinatesDialog::pop_up_file_browser()
{
	if (d_export_file_dialog->exec()) {
		lineedit_filename->setText(d_export_file_dialog->selectedFiles().front());
	}
}



void
GPlatesQtWidgets::ExportCoordinatesDialog::handle_export()
{
	// Sanity check.
	if ( ! d_geometry_opt_ptr) {
		QMessageBox::critical(this, tr("Invalid geometry for export"),
				tr("How the hell did you open this dialog box without a valid geometry?"));
		return;
	}
	
	// What output has the user requested?
	OutputFormat format = OutputFormat(combobox_format->currentIndex());
		
	if (radiobutton_to_file->isChecked()) {
		// Get filename.
		QString filename = lineedit_filename->text();
		if (filename.isEmpty()) {
			QMessageBox::warning(this, tr("No filename specified"),
					tr("Please specify a filename to export to."));
			lineedit_filename->setFocus();
			return;
		}
		
		// Open the file.
		QFile file(filename);
		if ( ! file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::critical(this, tr("Error writing to file"),
					tr("Error: The file could not be written."));
			lineedit_filename->setFocus();
			return;
		}
		
		QTextStream text_stream(&file);

		// Create geometry exporter and export geometry.
		export_geometry(format, text_stream);

	} else {
		// Create a byte array for the clipboard data.
		QByteArray array;
		
		QTextStream text_stream(&array);

		// Create geometry exporter and export geometry.
		export_geometry(format, text_stream);
		
		// Create mime data and assign to the clipboard.
		// FIXME: Use text/csv for CSV, and I don't know what for the others.
		QMimeData *mime = new QMimeData;
		mime->setData("text/plain", array);
		QApplication::clipboard()->setMimeData(mime, QClipboard::Clipboard);
	}
	
	// If we reach here, everything has been exported successfully and we can close the dialog.
	accept();
}

void
GPlatesQtWidgets::ExportCoordinatesDialog::export_geometry(
	OutputFormat format, QTextStream& text_stream)
{
	std::auto_ptr<GPlatesFileIO::GeometryExporter> geometry_exporter;

	// FIXME: It would be awesome if we could arrange this switch -before- we
	// open the file, to avoid clobbering the user's file needlessly.

	// Create the geometry exporter based on 'format'.
	switch (format)
	{
	case PLATES4:
		geometry_exporter.reset(
			new GPlatesFileIO::PlatesLineFormatGeometryExporter(
			text_stream,
			(combobox_coordinate_order->currentIndex() != LAT_LON),
			checkbox_polygon_terminating_point->isChecked()));
		break;

	case GMT:
		geometry_exporter.reset(
			new GPlatesFileIO::GMTFormatGeometryExporter(
			text_stream,
			// Default coordinate order for GMT is (lon, lat).
			(combobox_coordinate_order->currentIndex() != LON_LAT),
			checkbox_polygon_terminating_point->isChecked()));
		break;

	default:
		QMessageBox::critical(this, tr("Unsupported output format"),
			tr("Sorry, writing in the selected format is currently not supported."));
		return;
	}

	// Make sure we created a geometry exporter.
	GPlatesGlobal::Assert(geometry_exporter.get() != NULL,
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Export the geometry.
	geometry_exporter->export_geometry(*d_geometry_opt_ptr);
}
