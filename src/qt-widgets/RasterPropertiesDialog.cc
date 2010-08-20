/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
 * (as "SetRasterSurfaceExtentDialog.cc")
 *
 * Copyright (C) 2010 The University of Sydney
 * (as "RasterPropertiesDialog.cc")
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

#include <QRectF>
#include <QFileDialog>
#include <QHeaderView>

#include "EditAffineTransformGeoreferencingWidget.h"
#include "FriendlyLineEdit.h"
#include "InformationDialog.h"
#include "QtWidgetUtils.h"
#include "RasterPropertiesDialog.h"

#include "presentation/ViewState.h"

#include "property-values/RasterType.h"
#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"


namespace
{
	const QString PROPERTIES_HELP_DIALOG_TITLE = QObject::tr(
			"Raster Properties");

	
	const QString PROPERTIES_HELP_DIALOG_TEXT = QObject::tr(
			"<html><body>"
			"<p>The filename of the currently loaded raster and its format are displayed in the basic properties box. "
			"Where applicable, the raster's no-data value is displayed. "
			"Statistics, where available, are shown for non-RGB rasters.</p>"
			"</body></html>");


	const QString EXTENT_HELP_DIALOG_TITLE = QObject::tr(
			"Setting the Raster Extent");


	const QString EXTENT_HELP_DIALOG_TEXT = QObject::tr(
			"<html><body>"
			"<p>Raster images are displayed on the globe transformed from pixel coordinates into geographic coordinates using an affine transform.</p>"
			"<p>Where it is possible to express this affine transform as a lat-lon bounding box:"
			"<ul>"
			"<li>The latitude values must be in the range [-90, 90], and the upper latitude must be greater than the lower latitude.</li>"
			"<li>The longitude values must be in the range [-180, 180], and the right bound is taken to the to the right of the left bound, even if it crosses the International Date Line.</li>"
			"</ul></p>"
			"<p>In the general case, the affine transform is specified using six parameters, which can be displayed by clicking \"Show affine transform parameters (advanced)\".</p>"
			"</body></html>");

	
	const QString COLOUR_MAP_HELP_DIALOG_TITLE = QObject::tr(
			"Changing the Colour Map");


	const QString COLOUR_MAP_HELP_DIALOG_TEXT = QObject::tr(
			"<html><body>"
			"<p>There are two types of rasters:"
			"<ul>"
			"<li>A raster that is a grid of numerical values, each of which represents "
			"a physical quantity with no intrinsic mapping to a colour, and</li>"
			"<li>A raster in RGB format that is a grid of coloured pixels.</li>"
			"</ul></p>"
			"<p>In order to display the first kind of raster, there must be some mechanism "
			"by which the numerical values are mapped to colours. By default, GPlates "
			"will supply a colour map that maps values within two standard deviations "
			"of the mean to a spectrum of colours. Values more than two standard deviations "
			"from the mean are clamped to the nearest value in that range for colouring purposes.</p>"
			"<p>For rasters that have integral values, it is possible to "
			"supply either a regular (continuous) or categorical CPT file to map the "
			"values to colours. For rasters that are real-valued, it is possible to "
			"supply a regular CPT file to map the values to colours.</p>"
			"<p>It is not possible to supply a colour map for RGB rasters.</p>"
			"</body></html>");
}


GPlatesQtWidgets::RasterPropertiesDialog::RasterPropertiesDialog(
		GPlatesPresentation::ViewState *view_state,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_view_state(view_state),
#if 0
	d_georeferencing_widget(
			new EditAffineTransformGeoreferencingWidget(view_state, this)),
#endif
	d_georeferencing_widget(NULL),
	d_colour_map_lineedit(
			new FriendlyLineEdit(
				QString(),
				QString("Default Colour Map"),
				this)),
	d_help_dialog(NULL)
{
	setupUi(this);
	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_map_lineedit,
			colour_map_placeholder_widget);
#if 0
	QtWidgetUtils::add_widget_to_placeholder(
			d_georeferencing_widget,
			georeferencing_placeholder_widget);
#endif
	invalid_cpt_file_label->hide();
	properties_treewidget->header()->setResizeMode(QHeaderView::ResizeToContents);
	main_buttonbox->setFocus();

	enable_all_groupboxes(false);

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::RasterPropertiesDialog::make_signal_slot_connections()
{
	// Colour Map.
	QObject::connect(
			d_colour_map_lineedit,
			SIGNAL(editingFinished()),
			this,
			SLOT(handle_colour_map_lineedit_editing_finished()));
	QObject::connect(
			use_default_colour_map_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_default_colour_map_button_clicked()));
	QObject::connect(
			open_cpt_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_open_cpt_button_clicked()));

	// Help buttons.
	QObject::connect(
			extent_help_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_extent_help_button_clicked()));
	QObject::connect(
			properties_help_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_properties_help_button_clicked()));
	QObject::connect(
			colour_map_help_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_colour_map_help_button_clicked()));

	// Main buttonbox.
	QObject::connect(
			main_buttonbox,
			SIGNAL(clicked(QAbstractButton *)),
			this,
			SLOT(handle_main_buttonbox_clicked(QAbstractButton *)));
}


void
GPlatesQtWidgets::RasterPropertiesDialog::enable_all_groupboxes(
		bool enabled)
{
	properties_groupbox->setEnabled(enabled);
	colour_map_groupbox->setEnabled(enabled);
	georeferencing_groupbox->setEnabled(enabled);
}


void
GPlatesQtWidgets::RasterPropertiesDialog::set_raster_colour_map_filename(
		const QString &filename)
{
	d_view_state->set_raster_colour_map_filename(filename);
	d_view_state->update_texture_from_raw_raster();

	// Let the user know if the CPT file is invalid.
	invalid_cpt_file_label->setVisible(d_view_state->is_raster_colour_map_invalid());
}


void
GPlatesQtWidgets::RasterPropertiesDialog::handle_colour_map_lineedit_editing_finished()
{
	set_raster_colour_map_filename(d_colour_map_lineedit->text());
}


void
GPlatesQtWidgets::RasterPropertiesDialog::handle_use_default_colour_map_button_clicked()
{
	d_colour_map_lineedit->setText(QString());
	set_raster_colour_map_filename(QString());
}


void
GPlatesQtWidgets::RasterPropertiesDialog::handle_open_cpt_button_clicked()
{
	// FIXME: Work out whether it should be a regular or categorical CPT file.
	QString filename = QFileDialog::getOpenFileName(
			this,
			"Open File",
			QString(),
			"CPT file (*.cpt);;All files (*)");
	if (!filename.isEmpty())
	{
		d_colour_map_lineedit->setText(filename);
		set_raster_colour_map_filename(filename);
	}
}


void
GPlatesQtWidgets::RasterPropertiesDialog::handle_extent_help_button_clicked()
{
	show_help_dialog(EXTENT);
}


void
GPlatesQtWidgets::RasterPropertiesDialog::handle_properties_help_button_clicked()
{
	show_help_dialog(PROPERTIES);
}


void
GPlatesQtWidgets::RasterPropertiesDialog::handle_colour_map_help_button_clicked()
{
	show_help_dialog(COLOUR_MAP);
}


void
GPlatesQtWidgets::RasterPropertiesDialog::show_help_dialog(
		HelpContext context)
{
	// Create the dialog if it hasn't already been created.
	if (!d_help_dialog)
	{
		d_help_dialog = new InformationDialog(QString(), QString(), this);
		d_help_dialog->setModal(true);
	}

	// Set the text based on the context.
	switch (context)
	{
		case PROPERTIES:
			d_help_dialog->set_text(PROPERTIES_HELP_DIALOG_TEXT);
			d_help_dialog->set_title(PROPERTIES_HELP_DIALOG_TITLE);
			break;

		case EXTENT:
			d_help_dialog->set_text(EXTENT_HELP_DIALOG_TEXT);
			d_help_dialog->set_title(EXTENT_HELP_DIALOG_TITLE);
			break;

		case COLOUR_MAP:
			d_help_dialog->set_text(COLOUR_MAP_HELP_DIALOG_TEXT);
			d_help_dialog->set_title(COLOUR_MAP_HELP_DIALOG_TITLE);
			break;

		default:
			return;
	}

	d_help_dialog->show();
}


void
GPlatesQtWidgets::RasterPropertiesDialog::handle_main_buttonbox_clicked(
		QAbstractButton *button)
{
	switch (main_buttonbox->standardButton(button))
	{
		case QDialogButtonBox::Close:
			hide();
			break;

		default:
			break;
	}
}


namespace
{
	using namespace GPlatesPropertyValues;

	const QString &
	get_raster_type_name(
			GPlatesPropertyValues::RasterType::Type type)
	{
		static const QString UNINITIALISED_STRING = "Uninitialised";
		static const QString INT8_STRING = "8-bit signed integer";
		static const QString UINT8_STRING = "8-bit unsigned integer";
		static const QString INT16_STRING = "16-bit signed integer";
		static const QString UINT16_STRING = "16-bit unsigned integer";
		static const QString INT32_STRING = "32-bit signed integer";
		static const QString UINT32_STRING = "32-bit unsigned integer";
		static const QString FLOAT_STRING = "32-bit float";
		static const QString DOUBLE_STRING = "64-bit float";
		static const QString RGBA8_STRING = "RGBA8";
		static const QString UNKNOWN_STRING = "Unknown";

		switch (type)
		{
			case GPlatesPropertyValues::RasterType::UNINITIALISED:
				return UNINITIALISED_STRING;

			case GPlatesPropertyValues::RasterType::INT8:
				return INT8_STRING;

			case GPlatesPropertyValues::RasterType::UINT8:
				return UINT8_STRING;

			case GPlatesPropertyValues::RasterType::INT16:
				return INT16_STRING;

			case GPlatesPropertyValues::RasterType::UINT16:
				return UINT16_STRING;

			case GPlatesPropertyValues::RasterType::INT32:
				return INT32_STRING;

			case GPlatesPropertyValues::RasterType::UINT32:
				return UINT32_STRING;

			case GPlatesPropertyValues::RasterType::FLOAT:
				return FLOAT_STRING;

			case GPlatesPropertyValues::RasterType::DOUBLE:
				return DOUBLE_STRING;

			case GPlatesPropertyValues::RasterType::RGBA8:
				return RGBA8_STRING;

			case GPlatesPropertyValues::RasterType::UNKNOWN:
				return UNKNOWN_STRING;
		}

		// We don't use "default" in the switch above, so that if a new raster type is
		// added, the compiler will flag the missing case as an error.
		static const QString DEFAULT_STRING = "";
		return DEFAULT_STRING;
	}


	QTreeWidgetItem *
	create_treewidget_item(
			const QString &property,
			const QString &value,
			bool set_tool_tip = false)
	{
		QStringList list;
		list << property << value;
		QTreeWidgetItem *item = new QTreeWidgetItem(list);
		if (set_tool_tip)
		{
			item->setToolTip(1, value);
		}
		return item;
	}


	QTreeWidgetItem *
	create_numeric_treewidget_item(
			const QString &property,
			const boost::optional<double> &value,
			const QString &none_string = "(unknown)")
	{
		QString value_as_string = value ?
			QString::number(*value) : none_string;

		// Change "nan" to "NaN" because it's prettier.
		if (value_as_string == "nan")
		{
			value_as_string = "NaN";
		}

		return create_treewidget_item(property, value_as_string);
	}
}


void
GPlatesQtWidgets::RasterPropertiesDialog::populate_from_data()
{
	enable_all_groupboxes(true);

	GPlatesPropertyValues::RawRaster::non_null_ptr_type raw_raster =
		d_view_state->get_raw_raster();

	// Clear the properties table widget.
	properties_treewidget->clear();

	// Display the filename.
	properties_treewidget->addTopLevelItem(
			create_treewidget_item(
				"Filename",
				d_view_state->get_raster_filename(),
				true));

	// Display the raster's format.
	GPlatesPropertyValues::RasterType::Type raster_type =
		GPlatesPropertyValues::RawRasterUtils::get_raster_type(*raw_raster);
	properties_treewidget->addTopLevelItem(
			create_treewidget_item(
				"Format",
				get_raster_type_name(raster_type)));

	// Display the no-data value.
	properties_treewidget->addTopLevelItem(
			create_numeric_treewidget_item(
				"No-Data Value",
				GPlatesPropertyValues::RawRasterUtils::get_no_data_value(*raw_raster),
				"N/A"));

	// Display statistics, if any.
	GPlatesPropertyValues::RasterStatistics *statistics =
		GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*raw_raster);
	if (statistics)
	{
		QTreeWidgetItem *statistics_item = new QTreeWidgetItem(QStringList("Statistics"));
		statistics_item->addChild(
				create_numeric_treewidget_item(
					"Minimum",
					statistics->minimum));
		statistics_item->addChild(
				create_numeric_treewidget_item(
					"Maximum",
					statistics->maximum));
		statistics_item->addChild(
				create_numeric_treewidget_item(
					"Mean",
					statistics->mean));
		statistics_item->addChild(
				create_numeric_treewidget_item(
					"Standard Deviation",
					statistics->standard_deviation));

		properties_treewidget->addTopLevelItem(statistics_item);
	}

#if 0
	// Populate the georeferencing groupbox.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_size =
		GPlatesPropertyValues::RawRasterUtils::get_raster_size(*raw_raster);
	if (raster_size)
	{
		d_georeferencing_widget->setEnabled(true);
		d_georeferencing_widget->populate_from_data(
				d_view_state->get_georeferencing(),
				raster_size->first,
				raster_size->second);
	}
	else
	{
		d_georeferencing_widget->setEnabled(false);
	}
#endif

	// Populate the colour map groupbox.
	d_colour_map_lineedit->setText(d_view_state->get_raster_colour_map_filename());
	if (GPlatesPropertyValues::RawRasterUtils::try_rgba8_raster_cast(*raw_raster))
	{
		colour_map_groupbox->setEnabled(false);
	}
	else
	{
		colour_map_groupbox->setEnabled(true);
	}
}

