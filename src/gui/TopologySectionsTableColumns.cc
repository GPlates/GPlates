/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVariant>

#include "TopologySectionsTableColumns.h"

#include "app-logic/TopologyInternalUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyName.h"
#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"


// This is only a temporary define to allow Mark Turner to see what I implemented
// in terms of allowing the user to edit the begin/end times of topological sections.
// But we are not using this anymore and going with a new approach that allows the user
// to edit the begin/end times of the TopologicalPolygon property which is a time-dependent
// property (where each time instance contains a separate list of topological sections).
// The EditTimeWidget can be reused for this.
//
//#define ALLOW_EDIT_SECTION_BEGIN_END_TIMES

namespace
{
	/**
	 * Returns time of appearance using topological section first and if it's not set then
	 * returns time of appearance from feature referenced by topological section and if that's
	 * not set then returns distant past.
	 */
	GPlatesPropertyValues::GeoTimeInstant
	get_time_of_appearance(
			const GPlatesGui::TopologySectionsContainer::TableRow &table_row)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		if (table_row.get_begin_time())
		{
			return table_row.get_begin_time().get();
		}

		// There was no begin time set on the topological section so use the begin time
		// of the referenced feature instead.
		if (table_row.get_feature_ref().is_valid())
		{
			const GPlatesPropertyValues::GmlTimePeriod *time_period = NULL;
			if (GPlatesFeatureVisitors::get_property_value(
					table_row.get_feature_ref(), valid_time_property_name, time_period))
			{
				return time_period->begin()->time_position();
			}
		}

		// There was no begin time on the referenced feature (which shouldn't happen) so
		// just return distant past.
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
	}


	/**
	 * Returns time of disappearance using topological section first and if it's not set then
	 * returns time of disappearance from feature referenced by topological section and if that's
	 * not set then returns distant future.
	 */
	GPlatesPropertyValues::GeoTimeInstant
	get_time_of_disappearance(
			const GPlatesGui::TopologySectionsContainer::TableRow &table_row)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		if (table_row.get_end_time())
		{
			return table_row.get_end_time().get();
		}

		// There was no end time set on the topological section so use the end time
		// of the referenced feature instead.
		if (table_row.get_feature_ref().is_valid())
		{
			const GPlatesPropertyValues::GmlTimePeriod *time_period = NULL;
			if (GPlatesFeatureVisitors::get_property_value(
					table_row.get_feature_ref(), valid_time_property_name, time_period))
			{
				return time_period->end()->time_position();
			}
		}

		// There was no end time on the referenced feature (which shouldn't happen) so
		// just return distant future.
		return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	}

	//! Returns true if the user should be able to edit the topological sections's time period.
	bool
	should_edit_time_period(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data)
	{
		// The begin/end time of the current topological section is editable if
		// the begin/end time in the table row exists.
		return row_data.get_begin_time() && row_data.get_end_time();
	}


	// Table accessor functions:
	// These functions take the raw data and modify a QTableWidgetItem
	// to display the data appropriately.
	
	void
	null_data_accessor(
			const GPlatesGui::TopologySectionsContainer::TableRow &,
			QTableWidgetItem &)
	{  }


	/**
	 * Displays a GeomTimeInstant in a QTableWidgetItem.
	 */
	void
	display_time(
			const GPlatesPropertyValues::GeoTimeInstant &geo_time,
			QTableWidgetItem &cell)
	{
		if (geo_time.is_real())
		{
			cell.setData(Qt::DisplayRole, QVariant(static_cast<double>(geo_time.value())));
		}
		else if (geo_time.is_distant_past())
		{
			cell.setData(Qt::DisplayRole, QVariant("Distant Past"));
		}
		else if (geo_time.is_distant_future())
		{
			cell.setData(Qt::DisplayRole, QVariant("Distant Future"));
		}
	}

	void
	get_data_time_edit_flag(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		cell.setCheckState(
				should_edit_time_period(row_data)
				? Qt::Checked
				: Qt::Unchecked);
	}


	void
	get_data_time_of_appearance(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		display_time(get_time_of_appearance(row_data), cell);
	}


	void
	get_data_time_of_disappearance(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		display_time(get_time_of_disappearance(row_data), cell);
	}


	void
	get_data_feature_type(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		if (row_data.get_feature_ref().is_valid()) {
			cell.setData(Qt::DisplayRole, QVariant(GPlatesUtils::make_qstring_from_icu_string(
					row_data.get_feature_ref()->feature_type().build_aliased_name() )));
		}
	}
			

	void
	get_data_reconstruction_plate_id(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		
		if (row_data.get_feature_ref().is_valid()) {
			// Declare a pointer to a const GpmlPlateId. This is used as a return value;
			// It is passed by reference into the get_property_value() call below,
			// which sets it.
			const GPlatesPropertyValues::GpmlPlateId *property_return_value = NULL;
			
			// Attempt to find the property name and value we are interested in.
			if (GPlatesFeatureVisitors::get_property_value(
					row_data.get_feature_ref(),
					plate_id_property_name,
					property_return_value)) {
				// Convert it to something Qt can display.
				const GPlatesModel::integer_plate_id_type &plate_id = property_return_value->value();
				cell.setData(Qt::DisplayRole, QVariant(static_cast<quint32>(plate_id)));
			} else {
				// Feature resolves, but no reconstructionPlateId.
				cell.setData(Qt::DisplayRole, QObject::tr("<none>"));
			}
		}
	}


	void
	get_data_feature_name(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		static const GPlatesModel::PropertyName gml_name_property_name =
				GPlatesModel::PropertyName::create_gml("name");
		
		if (row_data.get_feature_ref().is_valid()) {
			// FIXME: As in other situations involving gml:name, we -do- want to
			// address the gml:codeSpace issue someday.
			
			// Declare a pointer to a const XsString. This is used as a return value;
			// It is passed by reference into the get_property_value() call below,
			// which sets it.
			const GPlatesPropertyValues::XsString *property_return_value = NULL;
			
			// Attempt to find the property name and value we are interested in.
			if (GPlatesFeatureVisitors::get_property_value(
					row_data.get_feature_ref(),
					gml_name_property_name,
					property_return_value)) {
				// Convert it to something Qt can display.
				QString name_qstr = GPlatesUtils::make_qstring(property_return_value->value());
				cell.setData(Qt::DisplayRole, name_qstr);
			} else {
				// Feature resolves, but no name property.
				cell.setData(Qt::DisplayRole, QString(""));
			}
		}
	}

	// Table mutator functions:
	// These functions take a QTableWidgetItem with user-entered values
	// and update the raw data appropriately.

	void
	null_data_mutator(
			GPlatesGui::TopologySectionsContainer::TableRow &,
			const QTableWidgetItem &)
	{  }

	void
	set_data_time_edit_flag(
			GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			const QTableWidgetItem &cell)
	{
		// Clear the topological section begin/end times because
		// we are about to query the begin/end times from 'row_data' and
		// that query first checks the topological section begin/end times but
		// we'll want the begin/end times of the referenced feature instead.
		row_data.set_begin_time(boost::none);
		row_data.set_end_time(boost::none);

		if (cell.checkState() == Qt::Checked)
		{
			const GPlatesPropertyValues::GeoTimeInstant begin_time = get_time_of_appearance(row_data);
			row_data.set_begin_time(begin_time);

			const GPlatesPropertyValues::GeoTimeInstant end_time = get_time_of_disappearance(row_data);
			row_data.set_end_time(end_time);
		}
		
		// Note: the update_data_from_table() method will push this table row into
		// the d_container_ptr vector, which will ultimately emit signals to notify
		// others about the updated data.
	}


	// Cell widget query functions:
	// These functions query whether a cell widget should be created to allow the user
	// to edit the raw data or whether a regular QTableWidgetItem should be created.

	bool
	null_install_edit_cell_widget_query(
				const GPlatesGui::TopologySectionsContainer::TableRow &/*row_data*/)
	{
		return false;
	}

	bool
	install_edit_time_period_widget_query(
				const GPlatesGui::TopologySectionsContainer::TableRow &row_data)
	{
		return should_edit_time_period(row_data);
	}


	// Cell widget creation functions:
	// These functions create a cell widget that allows the user to edit
	// the raw data.

	QWidget *
	null_edit_cell_widget_creator(
			QTableWidget *,
			GPlatesGui::TopologySectionsContainer &,
			GPlatesGui::TopologySectionsContainer::size_type)
	{
		return NULL;
	}

	QWidget *
	edit_begin_time_cell_widget_creator(
			QTableWidget *table_widget,
			GPlatesGui::TopologySectionsContainer &sections_container,
			GPlatesGui::TopologySectionsContainer::size_type sections_container_index)
	{
		return new GPlatesGui::TopologySectionsTableColumns::EditTimeWidget(
				GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::BEGIN_TIME,
				sections_container,
				sections_container_index,
				table_widget);
	}

	QWidget *
	edit_end_time_cell_widget_creator(
			QTableWidget *table_widget,
			GPlatesGui::TopologySectionsContainer &sections_container,
			GPlatesGui::TopologySectionsContainer::size_type sections_container_index)
	{
		return new GPlatesGui::TopologySectionsTableColumns::EditTimeWidget(
				GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::END_TIME,
				sections_container,
				sections_container_index,
				table_widget);
	}


	/**
	 * The column header information table. This gets used
	 * to set up the QTableWidget just the way we like it.
	 */
	const GPlatesGui::TopologySectionsTableColumns::ColumnHeadingInfo COLUMN_HEADING_INFO_TABLE[] = {
		{
			QT_TR_NOOP("Actions"),
			QT_TR_NOOP("Buttons in this column allow you to remove sections and change where new sections will be added."),
			104,
			QHeaderView::Fixed,	// FIXME: make this dynamic based on what the buttons need.
			Qt::AlignCenter,
			0,
			null_data_accessor,
			null_data_mutator,
			null_install_edit_cell_widget_query,
			null_edit_cell_widget_creator },

#ifdef ALLOW_EDIT_SECTION_BEGIN_END_TIMES
		//
		// NOTE: It appears that the first column after the actions column *must* have a resize mode of
		// "QHeaderView::Fixed" otherwise the column will resize to the width of the description message
		// used for the insertion arrow (which spans all columns except the actions column).
		//
		{
			QT_TR_NOOP("Restrict time"),
			QT_TR_NOOP("Controls whether the feature's time period can be refined."),
			80,
			QHeaderView::Fixed,
			Qt::AlignCenter,
			Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable,
			get_data_time_edit_flag,
			set_data_time_edit_flag,
			null_install_edit_cell_widget_query,
			null_edit_cell_widget_creator },

		// Note the resize mode is 'fixed' since we create our own cell widget whose
		// size seems to get ignored by the QTable.
		{
			QT_TR_NOOP("Appearance"),
			QT_TR_NOOP("The time this topological section first appears"),
			100, // Big enough to accommodate the installed cell widget.
			QHeaderView::Fixed,
			Qt::AlignVCenter,
			Qt::ItemIsEnabled | Qt::ItemIsSelectable,
			get_data_time_of_appearance,
			null_data_mutator,
			install_edit_time_period_widget_query,
			edit_begin_time_cell_widget_creator },

		// Note the resize mode is 'fixed' since we create our own cell widget whose
		// size seems to get ignored by the QTable.
		{
			QT_TR_NOOP("Disappearance"),
			QT_TR_NOOP("The time this topological section disappears"),
			100, // Big enough to accommodate the installed cell widget.
			QHeaderView::Fixed,
			Qt::AlignVCenter,
			Qt::ItemIsEnabled | Qt::ItemIsSelectable,
			get_data_time_of_disappearance,
			null_data_mutator,
			install_edit_time_period_widget_query,
			edit_end_time_cell_widget_creator },
#endif // ALLOW_EDIT_SECTION_BEGIN_END_TIMES

		{
			QT_TR_NOOP("Feature type"),
			QT_TR_NOOP("The type of this feature"),
			140,
			QHeaderView::ResizeToContents,
			Qt::AlignLeft | Qt::AlignVCenter,
			Qt::ItemIsEnabled | Qt::ItemIsSelectable,
			get_data_feature_type,
			null_data_mutator,
			null_install_edit_cell_widget_query,
			null_edit_cell_widget_creator },

		{
			QT_TR_NOOP("Plate ID"),
			QT_TR_NOOP("The plate ID used to reconstruct this feature"),
			60,
			QHeaderView::ResizeToContents,
			Qt::AlignCenter,
			Qt::ItemIsEnabled | Qt::ItemIsSelectable,
			get_data_reconstruction_plate_id,
			null_data_mutator,
			null_install_edit_cell_widget_query,
			null_edit_cell_widget_creator },

		{
			QT_TR_NOOP("Name"),
			QT_TR_NOOP("A convenient label for this feature"),
			140,
			QHeaderView::ResizeToContents,
			Qt::AlignLeft | Qt::AlignVCenter,
			Qt::ItemIsEnabled | Qt::ItemIsSelectable,
			get_data_feature_name, null_data_mutator,
			null_install_edit_cell_widget_query,
			null_edit_cell_widget_creator }
	};
}


std::vector<GPlatesGui::TopologySectionsTableColumns::ColumnHeadingInfo>
GPlatesGui::TopologySectionsTableColumns::get_column_heading_infos()
{
	return std::vector<ColumnHeadingInfo>(
			COLUMN_HEADING_INFO_TABLE,
			COLUMN_HEADING_INFO_TABLE +
					sizeof(COLUMN_HEADING_INFO_TABLE) / sizeof(COLUMN_HEADING_INFO_TABLE[0]));
}


GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::EditTimeWidget(
		Time begin_or_end_time,
		GPlatesGui::TopologySectionsContainer &sections_container,
		GPlatesGui::TopologySectionsContainer::size_type sections_container_index,
		QWidget *parent_):
	QWidget(parent_),
	d_begin_or_end_time(begin_or_end_time),
	d_sections_container(sections_container),
	d_sections_container_index(sections_container_index),
	d_table_row(sections_container.at(sections_container_index)),
	d_time_spinbox(NULL),
	d_distant_time_checkbox(NULL)
{
	// We are not using a Qt Designer .ui file, and do not need to call setupUi(this).
	// We will roll our layout by hand.
	// As we passed 'this' into the QHBoxLayout's constructor, we do not need to call
	// QWidget::setLayout().
	QVBoxLayout *widget_layout = new QVBoxLayout(this);
	widget_layout->setSpacing(1);
	widget_layout->setContentsMargins(0, 0, 0, 0);

	d_distant_time_checkbox = new QCheckBox(this);
	widget_layout->addWidget(d_distant_time_checkbox);
	d_distant_time_checkbox->setTristate(false);
	d_distant_time_checkbox->setText(
			(d_begin_or_end_time == BEGIN_TIME)
			? "Distant Past"
			: "Distant Future");

	QHBoxLayout *spinbox_layout = new QHBoxLayout();
	widget_layout->addLayout(spinbox_layout);
	spinbox_layout->setSpacing(1);
	spinbox_layout->setContentsMargins(0, 0, 0, 0);

	d_time_spinbox = new QDoubleSpinBox(this);
	spinbox_layout->addWidget(d_time_spinbox);

	QLabel *ma_label = new QLabel(this);
	spinbox_layout->addWidget(ma_label);
	ma_label->setText("Ma");
	ma_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	d_time_spinbox->setRange(0, 9999);
	d_time_spinbox->setSingleStep(1);
	d_time_spinbox->setDecimals(2);

	// Set the focus policies.
	//d_time_spinbox->setFocusPolicy(Qt::NoFocus);
	//setFocusProxy(d_time_spinbox);
	//d_distant_time_checkbox->setFocusPolicy(Qt::NoFocus);
	//setFocusPolicy(Qt::NoFocus/*Qt::ClickFocus*/);

	const GPlatesPropertyValues::GeoTimeInstant geo_time = get_time_from_topology_section();
	if (geo_time.is_real())
	{
		d_time_spinbox->setValue(geo_time.value());
		d_distant_time_checkbox->setCheckState(Qt::Unchecked);
	}
	else if (geo_time.is_distant_past())
	{
		d_time_spinbox->setDisabled(true);

		// If this widget represents the time of disappearance then
		// leave the "Distant Past" checkbox unchecked.
		d_distant_time_checkbox->setCheckState(
				(d_begin_or_end_time == BEGIN_TIME)
				? Qt::Checked
				: Qt::Unchecked);
	}
	else if (geo_time.is_distant_future())
	{
		d_time_spinbox->setDisabled(true);

		// If this widget represents the time of appearance then
		// leave the "Distant Future" checkbox unchecked.
		d_distant_time_checkbox->setCheckState(
				(d_begin_or_end_time == END_TIME)
				? Qt::Checked
				: Qt::Unchecked);
	}

#if 0
    d_time_spinbox->installEventFilter(this);
    d_distant_time_checkbox->installEventFilter(this);
    installEventFilter(this);
#endif

	QObject::connect(
			d_time_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(set_spinbox_time_in_topology_section(double)));

	QObject::connect(
			d_distant_time_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(set_distant_time_checkstate(int)));
}


#if 0
void
GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::focusInEvent(
		QFocusEvent *focus_event)
{
	std::cout << "focusInEvent" << std::endl;

	QWidget::focusInEvent(focus_event);
}


GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::~EditTimeWidget()
{
	std::cout << "~EditTimeWidget" << std::endl;
}


void
GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::focusOutEvent(
		QFocusEvent *focus_event)
{
	std::cout << "focusOutEvent" << std::endl;

	d_sections_container.update_at(d_sections_container_index, d_table_row);

	QWidget::focusOutEvent(focus_event);
}


bool
GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::eventFilter(
		QObject *obj,
		QEvent *event)
{
	if (event->type() == QEvent::FocusOut)
	{
		std::cout << "eventFilter: FocusOut" << std::endl;

		if (obj == this)
		{
			std::cout << "...EditTimeWidget" << std::endl;
		}

		if (obj == d_time_spinbox)
		{
			std::cout << "...d_time_spinbox" << std::endl;
		}

		if (obj == d_distant_time_checkbox)
		{
			std::cout << "...d_distant_time_checkbox" << std::endl;
		}
	}

	if (event->type() == QEvent::FocusIn)
	{
		std::cout << "eventFilter: FocusIn" << std::endl;

		if (obj == this)
		{
			std::cout << "...EditTimeWidget" << std::endl;
		}

		if (obj == d_time_spinbox)
		{
			std::cout << "...d_time_spinbox" << std::endl;
		}

		if (obj == d_distant_time_checkbox)
		{
			std::cout << "...d_distant_time_checkbox" << std::endl;
		}
	}

	// Standard event processing.
	return QObject::eventFilter(obj, event);
}
#endif


GPlatesPropertyValues::GeoTimeInstant
GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::get_time_from_topology_section() const
{
	return (d_begin_or_end_time == BEGIN_TIME)
			? get_time_of_appearance(d_table_row)
			: get_time_of_disappearance(d_table_row);
}


void
GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::set_spinbox_time_in_topology_section(
		double time)
{
	//std::cout << "set_spinbox_time_in_topology_section" << std::endl;

	const GPlatesPropertyValues::GeoTimeInstant geo_time(time);

	if (d_begin_or_end_time == BEGIN_TIME)
	{
		d_table_row.set_begin_time(geo_time);
	}
	else
	{
		d_table_row.set_end_time(geo_time);
	}
}


void
GPlatesGui::TopologySectionsTableColumns::EditTimeWidget::set_distant_time_checkstate(
		int checkbox_state)
{
	//std::cout << "set_distant_time_checkstate" << std::endl;

	GPlatesPropertyValues::GeoTimeInstant geo_time(0);

	if (checkbox_state == Qt::Checked)
	{
		d_time_spinbox->setDisabled(true);

		geo_time = (d_begin_or_end_time == BEGIN_TIME)
				? GPlatesPropertyValues::GeoTimeInstant::create_distant_past()
				: GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	}
	else
	{
		d_time_spinbox->setDisabled(false);
		d_time_spinbox->setValue(geo_time.value());
	}

	if (d_begin_or_end_time == BEGIN_TIME)
	{
		d_table_row.set_begin_time(geo_time);
	}
	else
	{
		d_table_row.set_end_time(geo_time);
	}
}
