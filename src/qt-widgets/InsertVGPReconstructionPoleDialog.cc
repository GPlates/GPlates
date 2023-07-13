/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionLayerProxy.h"
#include "app-logic/ReconstructionTree.h"

#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"

#include "presentation/ViewState.h"

#include "PoleSequenceTableWidget.h"
#include "ReconstructionPoleWidget.h"
 
#include "InsertVGPReconstructionPoleDialog.h"
 
 
namespace
{
	// Adapted from ModifyReconstructionPoleWidget class.
	void
	examine_trs(
		std::vector<GPlatesQtWidgets::PoleSequenceTableWidget::PoleSequenceInfo> &
		sequence_choices,
		GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder &trs_plate_id_finder,
		GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder &trs_time_period_finder,
		GPlatesModel::integer_plate_id_type plate_id_of_interest,
		const double &reconstruction_time,
		GPlatesModel::FeatureCollectionHandle::iterator &current_feature)
	{
		using namespace GPlatesQtWidgets;

		trs_plate_id_finder.reset();
		trs_plate_id_finder.visit_feature(current_feature);

		// A valid TRS should have a fixed reference frame and a moving reference frame. 
		// Let's verify that this is a valid TRS.
		if ( ! (trs_plate_id_finder.fixed_ref_frame_plate_id() &&
			trs_plate_id_finder.moving_ref_frame_plate_id())) {
				// This feature was missing one (or both) of the plate IDs which a TRS is
				// supposed to have.  Skip this feature.
				return;
		}
		// Else, we know it found both of the required plate IDs.

		if (*trs_plate_id_finder.fixed_ref_frame_plate_id() ==
			*trs_plate_id_finder.moving_ref_frame_plate_id()) {
				// The fixed ref-frame plate ID equals the moving ref-frame plate ID? 
				// Something strange is going on here.  Skip this feature.
				return;
		}

		// Dietmar has said that he doesn't want the table to include pole sequences for
		// which the plate ID of interest is the fixed ref-frame.  (2008-09-18)
#if 0
		if (*trs_plate_id_finder.fixed_ref_frame_plate_id() == plate_id_of_interest) {
			trs_time_period_finder.reset();
			trs_time_period_finder.visit_feature(current_feature);
			if ( ! (trs_time_period_finder.begin_time() && trs_time_period_finder.end_time())) {
				// No time samples were found.  Skip this feature.
				return;
			}

			// For now, let's _not_ include sequences which don't span this
			// reconstruction time.
			GPlatesPropertyValues::GeoTimeInstant current_time(reconstruction_time);
			if (trs_time_period_finder.begin_time()->is_strictly_later_than(current_time) ||
				trs_time_period_finder.end_time()->is_strictly_earlier_than(current_time)) {
					return;
			}

			sequence_choices.push_back(
				ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo(
				current_feature->reference(),
				*trs_plate_id_finder.fixed_ref_frame_plate_id(),
				*trs_plate_id_finder.moving_ref_frame_plate_id(),
				trs_time_period_finder.begin_time()->value(),
				trs_time_period_finder.end_time()->value(),
				true));
		}
#endif
		if (*trs_plate_id_finder.moving_ref_frame_plate_id() == plate_id_of_interest) {
			trs_time_period_finder.reset();
			trs_time_period_finder.visit_feature(current_feature);
			if ( ! (trs_time_period_finder.begin_time() && trs_time_period_finder.end_time())) {
				// No time samples were found.  Skip this feature.
				return;
			}

			// For now, let's _not_ include sequences which don't span this
			// reconstruction time.
			GPlatesPropertyValues::GeoTimeInstant current_time(reconstruction_time);
			if (trs_time_period_finder.begin_time()->is_strictly_later_than(current_time) ||
				trs_time_period_finder.end_time()->is_strictly_earlier_than(current_time)) {
					return;
			}

			sequence_choices.push_back(
				PoleSequenceTableWidget::PoleSequenceInfo(
				(*current_feature)->reference(),
				*trs_plate_id_finder.fixed_ref_frame_plate_id(),
				*trs_plate_id_finder.moving_ref_frame_plate_id(),
				trs_time_period_finder.begin_time()->value(),
				trs_time_period_finder.end_time()->value(),
				false));
		}
	}

	// Adapted from ReconstructionPoleWidget.cc... 
	
	/**
	 * This finds all the TRSes (total reconstruction sequences) in the supplied reconstruction
	 * whose fixed or moving ref-frame plate ID matches our plate ID of interest.
	 *
	 * The two vectors @a trses_with_plate_id_as_fixed and @a trses_with_plate_id_as_moving
	 * will be populated with the matches.
	 */
	void
	find_trses(
			std::vector<GPlatesQtWidgets::PoleSequenceTableWidget::PoleSequenceInfo> &
					sequence_choices,
			GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder &trs_plate_id_finder,
			GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder &trs_time_period_finder,
			GPlatesModel::integer_plate_id_type plate_id_of_interest,
			const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
			const GPlatesAppLogic::Reconstruction &reconstruction)
	{
		using namespace GPlatesModel;

		// Find the reconstruction feature collections used to create the reconstruction tree.
		// They could come from any of the reconstruction layer outputs (likely only one layer but could be more).
		boost::optional<const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &> reconstruction_feature_collections;
		std::vector<GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type> reconstruction_layer_outputs;
		if (reconstruction.get_active_layer_outputs<GPlatesAppLogic::ReconstructionLayerProxy>(reconstruction_layer_outputs))
		{
			// Iterate over the reconstruction layers.
			for (unsigned int reconstruction_layer_index = 0;
				reconstruction_layer_index < reconstruction_layer_outputs.size();
				++reconstruction_layer_index)
			{
				const GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type &
						reconstruction_layer_output = reconstruction_layer_outputs[reconstruction_layer_index];

				// See if both reconstruction trees were created from the same reconstruction graph (with same time and anchor plate).
				//
				// Note: We no longer compare reconstruction tree pointers because reconstruction tree creators typically have an internal
				//       cache of reconstruction trees, and so it's possible that the cache gets invalidated because some other client requests
				//       reconstruction trees at more reconstruction times than fits in the cache thus causing a new reconstruction tree to be
				//       created that is equivalent to an original reconstruction tree but does not compare equal (because it's a new instance).
				//       So instead it's more robust to see if both reconstruction trees were generated from the same reconstruction graph.
				if (reconstruction_layer_output->get_reconstruction_tree()->created_from_same_graph_with_same_parameters(*reconstruction_tree))
				{
					reconstruction_feature_collections = reconstruction_layer_output->get_current_reconstruction_feature_collections();
					break;
				}
			}
		}
		if (!reconstruction_feature_collections)
		{
			return;
		}

		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_iter =
				reconstruction_feature_collections->begin();
		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_end =
				reconstruction_feature_collections->end();
		for ( ; collections_iter != collections_end; ++collections_iter) {
			const FeatureCollectionHandle::weak_ref &current_collection = *collections_iter;
			if ( ! current_collection.is_valid()) {
				// FIXME:  Should we do anything about this? Or is this acceptable?
				// (If the collection is not valid, then presumably it has been
				// unloaded.  In which case, why hasn't the reconstruction been
				// recalculated?)
				continue;
			}

			FeatureCollectionHandle::iterator features_iter = current_collection->begin();
			FeatureCollectionHandle::iterator features_end = current_collection->end();
			for ( ; features_iter != features_end; ++features_iter) {
				examine_trs(
						sequence_choices,
						trs_plate_id_finder,
						trs_time_period_finder,
						plate_id_of_interest,
						reconstruction.get_reconstruction_time(),
						features_iter);
			}
		}
	}
}
 
 
 
GPlatesQtWidgets::InsertVGPReconstructionPoleDialog::InsertVGPReconstructionPoleDialog(
	GPlatesPresentation::ViewState &view_state_,
	QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_pole_sequence_table_widget_ptr(new PoleSequenceTableWidget()),
	d_reconstruction_pole_widget_ptr(new ReconstructionPoleWidget()),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_file_state(view_state_.get_application_state().get_feature_collection_file_state()),
	d_file_io(view_state_.get_application_state().get_feature_collection_file_io())
{
	setupUi(this);
	
	QHBoxLayout *layout_pole_widget = new QHBoxLayout(widget_place_holder);
	layout_pole_widget->setSpacing(0);
	layout_pole_widget->setContentsMargins(0, 0, 0, 0);
	layout_pole_widget->addWidget(d_reconstruction_pole_widget_ptr);	
	
	QHBoxLayout *layout_table_widget = new QHBoxLayout(widget_table_place_holder);
	layout_table_widget->setSpacing(0);
	layout_table_widget->setContentsMargins(0, 0, 0, 0);
	layout_table_widget->addWidget(d_pole_sequence_table_widget_ptr);		
} 

void
GPlatesQtWidgets::InsertVGPReconstructionPoleDialog::setup(
	const GPlatesQtWidgets::ReconstructionPole &reconstruction_pole)
{
	d_reconstruction_pole = reconstruction_pole;
	d_reconstruction_pole_widget_ptr->set_fields(reconstruction_pole);
	
	// Check loaded reconstruction feature collections (if any)
	// Put appropriate entry in "feature collection" widget
	//update_collection_field
	
	QString feature_collection_name;
	
	// Should I use "reconstruction" or "file state" to access feature collections...?
	//
	// This is a tricky one...
	// "reconstruction" can now contain multiple reconstruction trees but it will only
	// contain those from "layer"s that have their input feature collection(s) enabled.
	// One possible solution is to get feature collections by asking the
	// reconstruction tree "layers" for their input feature collections.
	// But this is a bit difficult at the moment though - perhaps we can discuss when
	// this dialog is enabled - John.
#if 1
	feature_collection_name = QString("< Create a new feature collection >");
#else
	if (d_application_state_ptr->get_current_reconstruction().reconstruction_tree()
		.get_reconstruction_features().empty())
	{
		// We don't have any reconstruction feature collections, so we'll set up <Create a new feature collection>
		feature_collection_name = QString("< Create a new feature collection >");
	}
	else
	{
		GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range range = 
			d_file_state.get_active_reconstruction_files();
		if (range.begin != range.end)
		{
			feature_collection_name = range.begin->get_file_info().get_display_name(
							false /*use absolute file name =false */);
		}	
	}
#endif
	
	lineedit_collection->setText(feature_collection_name);


	// Find all the TRSes (total reconstruction sequences) whose moving ref-frame
	// plate ID matches our plate ID of interest.
	std::vector<PoleSequenceTableWidget::PoleSequenceInfo> sequence_choices;
	GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder trs_plate_id_finder;
	GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder trs_time_period_finder;

#if 0 // Needs to pass in a ReconstructionTree instead of a Reconstruction...
	find_trses(sequence_choices, trs_plate_id_finder, trs_time_period_finder, d_reconstruction_pole.d_moving_plate,
		d_application_state_ptr->get_current_reconstruction());	
#endif
	
}

