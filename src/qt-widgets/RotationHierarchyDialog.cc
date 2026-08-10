/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#include <cmath>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>

#include "RotationHierarchyDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"

#include "maths/FiniteRotation.h"
#include "maths/MathsUtils.h"
#include "maths/UnitQuaternion3D.h"

#include "model/FeatureCollectionHandle.h"

#include "presentation/ViewState.h"

#include "utils/Earth.h"


namespace
{
	enum ColumnName
	{
		COLUMN_PLATE = 0,
		COLUMN_PARENT,
		COLUMN_RATE_REL_PARENT,
		COLUMN_SPEED_REL_PARENT,
		COLUMN_RATE_REL_ANCHOR,
		COLUMN_SPEED_REL_ANCHOR,
		COLUMN_POLE_SPAN,
		COLUMN_NOTES,

		NUM_COLUMNS
	};


	/**
	 * The rotation of @a moving_plate relative to @a fixed_plate, or none if either plate is
	 * absent from @a tree.
	 */
	boost::optional<GPlatesMaths::FiniteRotation>
	relative_rotation(
			const GPlatesAppLogic::ReconstructionTree &tree,
			GPlatesModel::integer_plate_id_type moving_plate,
			GPlatesModel::integer_plate_id_type fixed_plate)
	{
		const boost::optional<GPlatesMaths::FiniteRotation> moving_absolute =
				tree.get_composed_absolute_rotation_or_none(moving_plate);
		const boost::optional<GPlatesMaths::FiniteRotation> fixed_absolute =
				tree.get_composed_absolute_rotation_or_none(fixed_plate);
		if (!moving_absolute || !fixed_absolute)
		{
			return boost::none;
		}

		return GPlatesMaths::compose(
				GPlatesMaths::get_reverse(*fixed_absolute), *moving_absolute);
	}


	/**
	 * The magnitude of the stage rotation taking @a moving_plate from its position in
	 * @a younger_tree to its position in @a older_tree, expressed as degrees per My.
	 *
	 * Returns none if the plate is missing from either tree. Returns zero for a stationary
	 * plate; the identity rotation has no determinate axis, so it is handled separately.
	 */
	boost::optional<double>
	stage_rotation_rate(
			const GPlatesAppLogic::ReconstructionTree &younger_tree,
			const GPlatesAppLogic::ReconstructionTree &older_tree,
			GPlatesModel::integer_plate_id_type moving_plate,
			GPlatesModel::integer_plate_id_type fixed_plate,
			double interval)
	{
		if (interval <= 0.0)
		{
			return boost::none;
		}

		const boost::optional<GPlatesMaths::FiniteRotation> younger =
				relative_rotation(younger_tree, moving_plate, fixed_plate);
		const boost::optional<GPlatesMaths::FiniteRotation> older =
				relative_rotation(older_tree, moving_plate, fixed_plate);
		if (!younger || !older)
		{
			return boost::none;
		}

		// The stage rotation carrying the older position onto the younger one.
		const GPlatesMaths::FiniteRotation stage =
				GPlatesMaths::compose(*younger, GPlatesMaths::get_reverse(*older));

		const GPlatesMaths::UnitQuaternion3D &quat = stage.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(quat))
		{
			// A stationary plate. Asking for the rotation parameters would throw, since the
			// axis of an identity rotation is indeterminate.
			return 0.0;
		}

		const GPlatesMaths::UnitQuaternion3D::RotationParams params =
				quat.get_rotation_params(stage.axis_hint());

		return std::fabs(GPlatesMaths::convert_rad_to_deg(params.angle.dval())) / interval;
	}


	/**
	 * The fastest surface speed on a plate turning at @a degrees_per_my, in cm/yr.
	 *
	 * This is the speed at 90 degrees from the stage pole, ie, the maximum anywhere on the
	 * plate. It is quoted rather than a speed at some particular point because the rotation
	 * hierarchy has no geometry attached to it.
	 */
	double
	max_speed_cm_per_yr(
			double degrees_per_my)
	{
		// radians/My * km = km/My, and 1 km/My is 0.1 cm/yr.
		return GPlatesMaths::convert_deg_to_rad(degrees_per_my) *
				GPlatesUtils::Earth::MEAN_RADIUS_KMS * 0.1;
	}


	QString
	format_optional_rate(
			const boost::optional<double> &rate)
	{
		if (!rate)
		{
			return QObject::tr("-");
		}
		return QString::number(*rate, 'f', 3);
	}


	QString
	format_optional_speed(
			const boost::optional<double> &rate)
	{
		if (!rate)
		{
			return QObject::tr("-");
		}
		return QString::number(max_speed_cm_per_yr(*rate), 'f', 2);
	}
}


GPlatesQtWidgets::RotationHierarchyDialog::RotationHierarchyDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	GPlatesDialog(parent_, Qt::Window),
	d_application_state(view_state.get_application_state()),
	d_tree_widget(new QTreeWidget(this)),
	d_velocity_interval_spinbox(new QDoubleSpinBox(this)),
	d_show_all_plates_checkbox(new QCheckBox(tr("Show plates &outside their rotation sequence"), this)),
	d_summary_label(new QLabel(this)),
	d_velocity_interval(1.0),
	d_show_all_plates(true)
{
	setWindowTitle(tr("Rotation Hierarchy"));
	resize(900, 600);

	QStringList headers;
	headers << tr("Plate") << tr("Parent")
			<< tr("deg/My (parent)") << tr("cm/yr (parent)")
			<< tr("deg/My (anchor)") << tr("cm/yr (anchor)")
			<< tr("Rotation sequence (Ma)") << tr("Notes");
	d_tree_widget->setColumnCount(NUM_COLUMNS);
	d_tree_widget->setHeaderLabels(headers);
	d_tree_widget->setAlternatingRowColors(true);
	d_tree_widget->setRootIsDecorated(true);
	d_tree_widget->setUniformRowHeights(true);

	d_velocity_interval_spinbox->setRange(0.1, 100.0);
	d_velocity_interval_spinbox->setSingleStep(1.0);
	d_velocity_interval_spinbox->setDecimals(1);
	d_velocity_interval_spinbox->setValue(d_velocity_interval);
	d_velocity_interval_spinbox->setToolTip(
			tr("The interval, in My, over which the stage rotations are measured."));

	d_show_all_plates_checkbox->setChecked(d_show_all_plates);
	d_show_all_plates_checkbox->setToolTip(
			tr("Include plates whose rotation sequence does not span the current "
				"reconstruction time. Editing the pole of such a plate has no effect."));

	QHBoxLayout *controls_layout = new QHBoxLayout();
	controls_layout->addWidget(new QLabel(tr("&Velocity interval (My):"), this));
	controls_layout->addWidget(d_velocity_interval_spinbox);
	controls_layout->addSpacing(12);
	controls_layout->addWidget(d_show_all_plates_checkbox);
	controls_layout->addStretch();

	QVBoxLayout *main_layout = new QVBoxLayout(this);
	main_layout->addLayout(controls_layout);
	main_layout->addWidget(d_tree_widget);
	main_layout->addWidget(d_summary_label);

	QObject::connect(
			d_velocity_interval_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_velocity_interval_changed(double)));
	QObject::connect(
			d_show_all_plates_checkbox, SIGNAL(toggled(bool)),
			this, SLOT(handle_show_all_plates_toggled(bool)));

	// Track the reconstruction so the tree, the rates and the rewiring notes stay in step
	// with the time shown on the globe.
	QObject::connect(
			&d_application_state, SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this, SLOT(handle_reconstruction()));
}


void
GPlatesQtWidgets::RotationHierarchyDialog::handle_reconstruction()
{
	if (isVisible())
	{
		update();
	}
}


void
GPlatesQtWidgets::RotationHierarchyDialog::handle_velocity_interval_changed(
		double interval)
{
	d_velocity_interval = interval;
	update();
}


void
GPlatesQtWidgets::RotationHierarchyDialog::handle_show_all_plates_toggled(
		bool show_all)
{
	d_show_all_plates = show_all;
	update();
}


GPlatesQtWidgets::RotationHierarchyDialog::time_span_map_type
GPlatesQtWidgets::RotationHierarchyDialog::collect_time_spans() const
{
	time_span_map_type time_spans;

	GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder plate_id_finder;
	// Disabled samples are not used to reconstruct, so they must not widen the span either.
	GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder time_period_finder(true);

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			d_application_state.get_feature_collection_file_state().get_loaded_files();

	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			loaded_files)
	{
		const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				file_ref.get_file().get_feature_collection();
		if (!feature_collection.is_valid())
		{
			continue;
		}

		for (GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection->begin();
				feature_iter != feature_collection->end(); ++feature_iter)
		{
			plate_id_finder.reset();
			time_period_finder.reset();

			plate_id_finder.visit_feature((*feature_iter)->reference());
			time_period_finder.visit_feature((*feature_iter)->reference());

			if (!plate_id_finder.moving_ref_frame_plate_id() ||
					!time_period_finder.begin_time() ||
					!time_period_finder.end_time())
			{
				// Not a total reconstruction sequence, or one with no enabled samples.
				continue;
			}

			const GPlatesPropertyValues::GeoTimeInstant &begin = *time_period_finder.begin_time();
			const GPlatesPropertyValues::GeoTimeInstant &end = *time_period_finder.end_time();
			if (!begin.is_real() || !end.is_real())
			{
				continue;
			}

			const GPlatesModel::integer_plate_id_type moving_plate =
					*plate_id_finder.moving_ref_frame_plate_id();

			// 'end' is the youngest time, 'begin' the oldest. A plate can be driven by more
			// than one sequence, so accumulate the union of their spans.
			const time_span_type span(end.value(), begin.value());

			const time_span_map_type::iterator existing = time_spans.find(moving_plate);
			if (existing == time_spans.end())
			{
				time_spans.insert(std::make_pair(moving_plate, span));
			}
			else
			{
				existing->second.first = (std::min)(existing->second.first, span.first);
				existing->second.second = (std::max)(existing->second.second, span.second);
			}
		}
	}

	return time_spans;
}


void
GPlatesQtWidgets::RotationHierarchyDialog::add_edges(
		QTreeWidgetItem *parent_item,
		const GPlatesAppLogic::ReconstructionTree::edge_list_type &edges,
		const GPlatesAppLogic::ReconstructionTree &tree,
		const GPlatesAppLogic::ReconstructionTree &younger_tree,
		const GPlatesAppLogic::ReconstructionTree &older_tree,
		const time_span_map_type &time_spans,
		double reconstruction_time)
{
	for (GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator edge_iter = edges.begin();
			edge_iter != edges.end(); ++edge_iter)
	{
		const GPlatesAppLogic::ReconstructionTree::Edge &edge = *edge_iter;

		const GPlatesModel::integer_plate_id_type moving_plate = edge.get_moving_plate();
		const GPlatesModel::integer_plate_id_type fixed_plate = edge.get_fixed_plate();

		QStringList notes;

		// Does this plate's rotation sequence actually span the current time? If not, any
		// pole edit made now is silently discarded, so it is worth saying so plainly.
		bool spans_current_time = true;
		QString span_text = tr("(none)");
		const time_span_map_type::const_iterator span_iter = time_spans.find(moving_plate);
		if (span_iter != time_spans.end())
		{
			const double youngest = span_iter->second.first;
			const double oldest = span_iter->second.second;
			span_text = QString("%1 - %2")
					.arg(youngest, 0, 'f', 1)
					.arg(oldest, 0, 'f', 1);

			if (reconstruction_time < youngest || reconstruction_time > oldest)
			{
				spans_current_time = false;
				notes << tr("outside rotation sequence - pole edits will not stick");
			}
		}
		else
		{
			spans_current_time = false;
			notes << tr("no rotation sequence found");
		}

		if (!d_show_all_plates && !spans_current_time)
		{
			continue;
		}

		// Has the circuit been rewired at this instant? Compare the plate's parent slightly
		// either side of the current time.
		const boost::optional<const GPlatesAppLogic::ReconstructionTree::Edge &> younger_edge =
				younger_tree.get_edge(moving_plate);
		const boost::optional<const GPlatesAppLogic::ReconstructionTree::Edge &> older_edge =
				older_tree.get_edge(moving_plate);
		if (younger_edge && older_edge &&
				younger_edge->get_fixed_plate() != older_edge->get_fixed_plate())
		{
			notes << tr("parent changes here: %1 -> %2")
					.arg(older_edge->get_fixed_plate())
					.arg(younger_edge->get_fixed_plate());
		}

		const boost::optional<double> rate_rel_parent = stage_rotation_rate(
				younger_tree, older_tree, moving_plate, fixed_plate, d_velocity_interval);
		const boost::optional<double> rate_rel_anchor = stage_rotation_rate(
				younger_tree, older_tree, moving_plate, tree.get_anchor_plate_id(),
				d_velocity_interval);

		QTreeWidgetItem *item = new QTreeWidgetItem(parent_item);
		item->setText(COLUMN_PLATE, QString::number(moving_plate));
		item->setText(COLUMN_PARENT, QString::number(fixed_plate));
		item->setText(COLUMN_RATE_REL_PARENT, format_optional_rate(rate_rel_parent));
		item->setText(COLUMN_SPEED_REL_PARENT, format_optional_speed(rate_rel_parent));
		item->setText(COLUMN_RATE_REL_ANCHOR, format_optional_rate(rate_rel_anchor));
		item->setText(COLUMN_SPEED_REL_ANCHOR, format_optional_speed(rate_rel_anchor));
		item->setText(COLUMN_POLE_SPAN, span_text);
		item->setText(COLUMN_NOTES, notes.join(tr("; ")));

		for (int column = COLUMN_RATE_REL_PARENT; column <= COLUMN_SPEED_REL_ANCHOR; ++column)
		{
			item->setTextAlignment(column, Qt::AlignRight | Qt::AlignVCenter);
		}

		add_edges(
				item, edge.get_child_edges(), tree, younger_tree, older_tree,
				time_spans, reconstruction_time);
	}
}


void
GPlatesQtWidgets::RotationHierarchyDialog::update()
{
	d_tree_widget->clear();

	const GPlatesAppLogic::Reconstruction &reconstruction =
			d_application_state.get_current_reconstruction();
	const double reconstruction_time = reconstruction.get_reconstruction_time();

	const GPlatesAppLogic::ReconstructionTreeCreator tree_creator =
			reconstruction.get_default_reconstruction_layer_output()->get_reconstruction_tree_creator();

	// Straddle the current time so that both the stage rotations and the parent comparison
	// describe what is happening *at* this instant rather than side-effects of a one-sided
	// interval. Times are clamped at present day, which is as young as a reconstruction goes.
	const double half_interval = 0.5 * d_velocity_interval;
	const double younger_time = (std::max)(0.0, reconstruction_time - half_interval);
	const double older_time = reconstruction_time + half_interval;

	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree =
			tree_creator.get_reconstruction_tree(reconstruction_time);
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type younger_tree =
			tree_creator.get_reconstruction_tree(younger_time);
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type older_tree =
			tree_creator.get_reconstruction_tree(older_time);

	const time_span_map_type time_spans = collect_time_spans();

	QTreeWidgetItem *anchor_item = new QTreeWidgetItem(d_tree_widget);
	anchor_item->setText(COLUMN_PLATE, QString::number(tree->get_anchor_plate_id()));
	anchor_item->setText(COLUMN_PARENT, tr("(anchor)"));
	anchor_item->setText(COLUMN_POLE_SPAN, tr("-"));

	add_edges(
			anchor_item, tree->get_anchor_plate_edges(),
			*tree, *younger_tree, *older_tree, time_spans, reconstruction_time);

	anchor_item->setExpanded(true);
	d_tree_widget->expandAll();
	for (int column = 0; column < NUM_COLUMNS; ++column)
	{
		d_tree_widget->resizeColumnToContents(column);
	}

	d_summary_label->setText(
			tr("%1 plates in the circuit at %2 Ma, anchored to plate %3. "
				"Rates are measured over %4 My straddling the current time.")
					.arg(tree->get_all_edges().size())
					.arg(reconstruction_time, 0, 'f', 1)
					.arg(tree->get_anchor_plate_id())
					.arg(d_velocity_interval, 0, 'f', 1));
}
