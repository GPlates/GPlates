/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#include <stdexcept>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QTableWidget>
#include <QTextStream>
#include <QtGlobal>

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "CsvExport.h"

namespace {

	/**
	 * Opens @a file for writing and attaches @a os to it as a UTF-8 text stream.
	 *
	 * The file is opened through QFile rather than std::ofstream so that paths containing
	 * non-ASCII characters work. QString::toStdString() encodes as UTF-8, but the narrow
	 * std::ofstream constructor interprets its path using the process' ANSI code page on
	 * Windows, so any path outside that code page failed to open.
	 */
	void
	open_csv_file(
			QFile &file,
			QTextStream &os)
	{
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			throw GPlatesFileIO::ErrorOpeningFileForWritingException(
					GPLATES_EXCEPTION_SOURCE,
					file.fileName());
		}

		os.setDevice(&file);

		// The replaced code wrote UTF-8 bytes (via QString::toStdString()), so ask for UTF-8
		// explicitly rather than inheriting the locale codec under Qt5.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		os.setEncoding(QStringConverter::Utf8);
#else
		os.setCodec("UTF-8");
#endif
	}

	/**
	 * Flushes @a os and throws if anything went wrong while writing.
	 *
	 * The replaced std::ofstream had exceptions enabled for badbit/failbit; QTextStream
	 * reports failures through its device instead, so they are checked explicitly.
	 */
	void
	close_csv_file(
			QFile &file,
			QTextStream &os)
	{
		// Flush the stream into the file and the file to the OS: an error such as a full disk
		// can surface at either point, and QFile::close() retains the error status.
		os.flush();
		file.close();

		if (file.error() != QFile::NoError)
		{
			throw std::runtime_error(file.errorString().toStdString());
		}
	}

	/**
	 * Attempts to apply quoting/escaping rules to a single CSV field
	 * as correctly as possible.
	 *
	 * http://en.wikipedia.org/wiki/Comma-separated_values#Specification
	 */
	QString
	csv_quote_if_necessary(
			QString str,
			const GPlatesGui::CsvExport::ExportOptions &options)
	{
		static const QChar quote_char('"');
		static const QString escaped_quote("\"\"");	// ""
		
		// Determine if we need quotes at all; some CSV implementations may find it more
		// convenient if we don't quote (for example) numbers which do not need quoting.
		bool needs_quoting = str.contains(quote_char) || str.contains(options.delimiter) ||
				str.contains('\n') || str.startsWith(' ') || str.endsWith(' ');
		
		// If we're putting quotes around the string, we'll need to escape any quote marks
		// which may be embedded in the string. Sadly, for CSV, this is not done with
		// backslashes, but by doubling the quote character.
		str.replace(quote_char, escaped_quote);
		
		// Finally, return the field (wrapped in quotes if necessary)
		if (needs_quoting) {
			return str.prepend(quote_char).append(quote_char);
		} else {
			return str;
		}
	}

	void
	export_table_view_header(
			const QTableView &table_view,
			QTextStream &os,
			const GPlatesGui::CsvExport::ExportOptions &options)
	{
		int num_columns = table_view.model()->columnCount();
		QString header_item_as_string;
		for (int column = 0; column < num_columns ; ++column)
		{
			QVariant header = table_view.model()->headerData(column,Qt::Horizontal);

			header_item_as_string = csv_quote_if_necessary(header.toString(), options);
			os << header_item_as_string;

			// Separate fields with a delimiter.
			if (column < (num_columns - 1)) {
				os << options.delimiter;
			}
		}
		// The file is opened with QIODevice::Text, so this becomes the platform line ending
		// just as the replaced std::ofstream's text mode did.
		os << "\n";

	}

} // anonymous namespace


namespace GPlatesGui
{
	void
	GPlatesGui::CsvExport::export_table_widget(
			const QString &filename,
			const GPlatesGui::CsvExport::ExportOptions &options,
			const QTableWidget &table)
	{
		QFileInfo file_info(filename);
		QFile file(filename);
		QTextStream os;
		try{

			open_csv_file(file, os);

			int num_columns = table.columnCount();
			int num_rows = table.rowCount();

			int column;
			int row;
			QTableWidgetItem *item;
			QString item_as_str;

			for(row = 0 ; row < num_rows; row++)
			{
				for (column = 0 ; column < num_columns; column++)
				{
					// Beware:  QTableWidget::item returns a NULL pointer
					// if no item has been set at the (row, column) position.
					item = table.item(row, column);
					if (item) {
						item_as_str = csv_quote_if_necessary(item->text(), options);
						os << item_as_str;
					}

					// Separate fields with a delimiter.
					if (column < (num_columns - 1)) {
						os << options.delimiter;
					}

				}

				os << "\n";

			}

			close_csv_file(file, os);
		}
		catch (GPlatesFileIO::ErrorOpeningFileForWritingException &)
		{
			QString message = QObject::tr("Error opening file '%1' for writing")
					.arg(file_info.filePath());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
								  QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (std::exception &exc)
		{
			QString message = QObject::tr("Error writing to file '%1': %2")
					.arg(file_info.filePath()).arg(exc.what());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
								  QMessageBox::Ok, QMessageBox::Ok);
		}
		catch(...)
		{
			QString message = QObject::tr("An error occurred while writing to file '%1'")
					.arg(file_info.filePath());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
								  QMessageBox::Ok, QMessageBox::Ok);
		}

		return;

	}

	void
	CsvExport::export_table_view(
			const QString &filename,
			const CsvExport::ExportOptions &options,
			const QTableView &table)
	{
		QFileInfo file_info(filename);
		QFile file(filename);
		QTextStream os;
		try{

			open_csv_file(file, os);

			QAbstractItemModel *model = table.model();

			if (model)
			{
				int num_columns = model->columnCount();
				int num_rows = model->rowCount();

				QString item_as_str;

				export_table_view_header(table,os,options);

				for(int row = 0 ; row < num_rows; ++row)
				{
					for (int column = 0 ; column < num_columns; ++column)
					{
						// Beware:  QTableWidget::item returns a NULL pointer
						// if no item has been set at the (row, column) position.
						QVariant data = model->index(row,column).data();
						item_as_str = csv_quote_if_necessary(data.toString(), options);
						os << item_as_str;

						// Separate fields with a delimiter.
						if (column < (num_columns - 1)) {
							os << options.delimiter;
						}

					}

					os << "\n";

				}
			}

			close_csv_file(file, os);
		}
		catch (GPlatesFileIO::ErrorOpeningFileForWritingException &)
		{
			QString message = QObject::tr("Error opening file '%1' for writing")
					.arg(file_info.filePath());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
								  QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (std::exception &exc)
		{
			QString message = QObject::tr("Error writing to file '%1': %2")
					.arg(file_info.filePath()).arg(exc.what());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
								  QMessageBox::Ok, QMessageBox::Ok);
		}
		catch(...)
		{
			QString message = QObject::tr("An error occurred while writing to file '%1'")
					.arg(file_info.filePath());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
								  QMessageBox::Ok, QMessageBox::Ok);
		}

		return;

	}
	

	void
	CsvExport::export_line(
			QTextStream &os,
			const CsvExport::ExportOptions &options,
			const LineDataType &line_data)
	{

		std::vector<QString>::const_iterator it;

		for(it=line_data.begin();it!=line_data.end();it++)
		{
			QString str = csv_quote_if_necessary(*it, options);
			os << str;

			if (it!=(line_data.end() - 1))
			{
				os << options.delimiter;
			}

		}
		os << "\n";
		return;
	}

	void
	CsvExport::export_data(
			const QString &filename,
			const CsvExport::ExportOptions &options,
			const std::vector<CsvExport::LineDataType> &data)
	{
		QFileInfo file_info(filename);
		QFile file(filename);
		QTextStream os;
		try{
			open_csv_file(file, os);
			std::vector<CsvExport::LineDataType>::const_iterator it;
			for(it=data.begin();it!=data.end();it++)
			{
				export_line(os,options,*it);
			}

			close_csv_file(file, os);
		}
		catch (GPlatesFileIO::ErrorOpeningFileForWritingException &)
		{
			QString message = QObject::tr("Error opening file '%1' for writing")
					.arg(file_info.filePath());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
					QMessageBox::Ok, QMessageBox::Ok);
		}
		catch (std::exception &exc)
		{
			QString message = QObject::tr("Error writing to file '%1': %2")
					.arg(file_info.filePath()).arg(exc.what());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
					QMessageBox::Ok, QMessageBox::Ok);
		}
		catch(...)
		{
			QString message = QObject::tr("An error occurred while writing to file '%1'")
				.arg(file_info.filePath());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		}
		return;
	}


} // namespace GPlatesGui
