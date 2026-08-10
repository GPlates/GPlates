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

#ifndef GPLATES_QTWIDGETS_ROTATIONHIERARCHYDIALOG_H
#define GPLATES_QTWIDGETS_ROTATIONHIERARCHYDIALOG_H

#include <map>
#include <utility>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "GPlatesDialog.h"

#include "app-logic/ReconstructionTree.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * Displays the plate circuit described by the loaded rotation files as a tree, rooted at
	 * the anchored plate.
	 *
	 * The dialog answers three questions that are otherwise awkward to get at:
	 *
	 *  - what is attached to what, at the current reconstruction time;
	 *  - how fast each plate is moving, both relative to its parent and relative to the
	 *    anchor;
	 *  - which plates change parent at the current time, ie, where the circuit is rewired.
	 *
	 * It also reports the time span actually covered by each plate's rotation sequence, and
	 * flags plates whose sequence does not span the current reconstruction time. Editing a
	 * plate's pole outside that span is silently discarded by
	 * TotalReconstructionSequenceRotationInserter, so being able to see the span makes an
	 * otherwise invisible failure obvious.
	 */
	class RotationHierarchyDialog:
			public GPlatesDialog
	{
		Q_OBJECT

	public:

		explicit
		RotationHierarchyDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		virtual
		~RotationHierarchyDialog()
		{  }

		/**
		 * Rebuild the tree from the current reconstruction.
		 */
		void
		update();

	private Q_SLOTS:

		void
		handle_reconstruction();

		void
		handle_velocity_interval_changed(
				double interval);

		void
		handle_show_all_plates_toggled(
				bool show_all);

	private:

		/**
		 * The time span covered by a plate's rotation sequence, in Ma.
		 *
		 * 'first' is the youngest (smallest) time, 'second' the oldest (largest).
		 */
		typedef std::pair<double, double> time_span_type;

		typedef std::map<GPlatesModel::integer_plate_id_type, time_span_type> time_span_map_type;

		/**
		 * Populate the tree beneath @a parent_item with the children of @a edge.
		 */
		void
		add_edges(
				QTreeWidgetItem *parent_item,
				const GPlatesAppLogic::ReconstructionTree::edge_list_type &edges,
				const GPlatesAppLogic::ReconstructionTree &tree,
				const GPlatesAppLogic::ReconstructionTree &younger_tree,
				const GPlatesAppLogic::ReconstructionTree &older_tree,
				const time_span_map_type &time_spans,
				double reconstruction_time);

		/**
		 * Collect the time span of every total reconstruction sequence in the loaded files.
		 */
		time_span_map_type
		collect_time_spans() const;

		GPlatesAppLogic::ApplicationState &d_application_state;

		QTreeWidget *d_tree_widget;
		QDoubleSpinBox *d_velocity_interval_spinbox;
		QCheckBox *d_show_all_plates_checkbox;
		QLabel *d_summary_label;

		/**
		 * The interval, in My, over which stage rotations are measured.
		 */
		double d_velocity_interval;

		/**
		 * Whether to list plates whose rotation sequence does not span the current time.
		 */
		bool d_show_all_plates;
	};
}

#endif // GPLATES_QTWIDGETS_ROTATIONHIERARCHYDIALOG_H
