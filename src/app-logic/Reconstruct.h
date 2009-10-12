/* $Id$ */

/**
 * \file Reconstructs feature geometry(s) from present day to the past.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCT_H
#define GPLATES_APP_LOGIC_RECONSTRUCT_H

#include <utility>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/types.h"

#include "feature-visitors/TopologyResolver.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileState;


	namespace ReconstructUtils
	{
		/**
		 * Create and return a reconstruction tree for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const GPlatesModel::ReconstructionTree::non_null_ptr_type
		create_reconstruction_tree(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				const double &time,
				GPlatesModel::integer_plate_id_type root);


		/**
		 * Create and return a reconstruction for the reconstruction time @a time, with
		 * root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 *
		 * TopologyResolver is currently referenced by ComputationalMeshSolver
		 * so we return it to the caller. Later it may be divided into two parts
		 * and not need to be returned here.
		 */
		std::pair<
				const GPlatesModel::Reconstruction::non_null_ptr_type,
				boost::shared_ptr<GPlatesFeatureVisitors::TopologyResolver> >
		create_reconstruction(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				const double &time,
				GPlatesModel::integer_plate_id_type root);


		/**
		 * Create and return an empty reconstruction for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The reconstruction tree contained within the reconstruction will also be empty.
		 *
		 * FIXME:  Remove this function once it is possible to create empty reconstructions
		 * by simply passing empty lists of feature-collections into the prev function.
		 */
		const GPlatesModel::Reconstruction::non_null_ptr_type
		create_empty_reconstruction(
				const double &time,
				GPlatesModel::integer_plate_id_type root);
	}


	/**
	 * Handles reconstruction generation, storage and notification.
	 */
	class Reconstruct :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		/**
		 * Interface class for calling client-defined code before and after each reconstruction.
		 */
		class Hook
		{
		public:
			virtual
			~Hook()
			{  }


			/**
			 * Called before a reconstruction is created.
			 */
			virtual
			void
			begin_reconstruction(
					GPlatesModel::ModelInterface &model,
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id)
			{  }


			/**
			 * Called after a reconstruction is created.
			 *
			 * The created reconstruction is passed as @a reconstruction.
			 *
			 * FIXME: When TopologyResolver is divided into two parts (see comment inside
			 * Reconstruct::create_reconstruction) remove it from argument list.
			 */
			virtual
			void
			end_reconstruction(
					GPlatesModel::ModelInterface &model,
					GPlatesModel::Reconstruction &reconstruction,
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
					const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
							reconstructable_features_collection,
					const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
							reconstruction_features_collection,
					GPlatesFeatureVisitors::TopologyResolver &topology_resolver)
			{  }
		};


		/**
		 * Constructor.
		 *
		 * The default @a reconstruction_hook does nothing.
		 */
		Reconstruct(
				GPlatesModel::ModelInterface &model,
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				double reconstruction_time = 0,
				GPlatesModel::integer_plate_id_type anchored_plate_id = 0,
				Hook *reconstruction_hook = NULL);


		/**
		 * Sets the reconstruction hook to be called when a reconstruction is next done.
		 *
		 * Default for @a reconstruction_hook means do nothing.
		 */
		void
		set_reconstruction_hook(
				Hook *reconstruction_hook = NULL)
		{
			d_reconstruction_hook = reconstruction_hook;
		}


		const double &
		get_current_reconstruction_time() const
		{
			return d_reconstruction_time;
		}


		GPlatesModel::integer_plate_id_type
		get_current_anchored_plate_id() const
		{
			return d_anchored_plate_id;
		}


		/**
		 * Returns the reconstruction generated by the last call to
		 * @a reconstruct, @a reconstruct_to_time, @a reconstruct_with_anchor,
		 * @a reconstruct_to_time_with_anchor or if none have been called so far then
		 * returns an empty reconstruction.
		 */
		GPlatesModel::Reconstruction &
		get_current_reconstruction()
		{
			return *d_reconstruction;
		}


	public slots:
		/**
		 * Create a reconstruction for the current reconstruction time and anchored plate id
		 * using the active reconstructable/reconstruction feature collections in
		 * @a FeatureCollectionFileState.
		 *
		 * This method is useful if @a FeatureCollectionFileState has changed.
		 */
		void
		reconstruct();


		/**
		 * Create a reconstruction for the current plate id and a reconstruction time
		 * of @a reconstruction_time using the active reconstructable/reconstruction
		 * feature collections in @a FeatureCollectionFileState..
		 *
		 * This also sets the current reconstruction time.
		 */
		void
		reconstruct_to_time(
				double reconstruction_time);


		/**
		 * Create a reconstruction with the current reconstruction time and an
		 * anchor plate id of @a anchor_plate_id using the active
		 * reconstructable/reconstruction feature collections in @a FeatureCollectionFileState..
		 *
		 * This also sets the current anchor plate id.
		 */
		void
		reconstruct_with_anchor(
				unsigned long anchor_plate_id);


		/**
		 * Create a reconstruction with a reconstruction time of @a reconstruction_time
		 * and an anchor plate id of @a anchor_plate_id using the active
		 * reconstructable/reconstruction feature collections in @a FeatureCollectionFileState..
		 *
		 * This also sets the current reconstruction time and anchor plate id.
		 */
		void
		reconstruct_to_time_with_anchor(
				double reconstruction_time,
				unsigned long anchor_plate_id);

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		reconstructed(
				GPlatesAppLogic::Reconstruct &reconstructer,
				bool reconstruction_time_changed,
				bool anchor_plate_id_changed);

	private:
		GPlatesModel::ModelInterface d_model;
		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;
		double d_reconstruction_time;
		GPlatesModel::integer_plate_id_type d_anchored_plate_id;

		GPlatesModel::Reconstruction::non_null_ptr_type d_reconstruction;

		Hook *d_reconstruction_hook;


		void
		reconstruct_application_state();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCT_H
