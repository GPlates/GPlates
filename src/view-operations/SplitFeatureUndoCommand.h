/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_SPLITFEATUREUNDOCOMMAND_H
#define GPLATES_VIEWOPERATIONS_SPLITFEATUREUNDOCOMMAND_H

#include <QUndoCommand>

#include "UndoRedo.h"
#include "GeometryBuilder.h"
#include "RenderedGeometryCollection.h"

#include "app-logic/ApplicationState.h"

#include "feature-visitors/GeometryFinder.h"
#include "feature-visitors/GeometryTypeFinder.h"

#include "global/InvalidFeatureCollectionException.h"
#include "global/InvalidParametersException.h"

#include "gui/FeatureFocus.h"
#include "gui/ChooseCanvasToolUndoCommand.h"

#include "maths/PointOnSphere.h"

#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/TopLevelProperty.h"

#include "presentation/ViewState.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesViewOperations
{

	/**
	 * Command to split a feature.
	 */
	
	class SplitFeatureUndoCommand :
		public QUndoCommand
	{
	public:
		SplitFeatureUndoCommand(
				GPlatesGui::FeatureFocus &feature_focus,
				const GPlatesModel::Gpgim &gpgim,
				GPlatesModel::ModelInterface model_interface,
				GeometryBuilder::PointIndex point_index_to_insert_at,
				boost::optional<const GPlatesMaths::PointOnSphere> &oriented_pos_on_globe,
				QUndoCommand *parent = 0) :
			QUndoCommand(parent),
			d_feature_focus(&feature_focus),
			d_gpgim(gpgim),
			d_model_interface(model_interface),
			d_point_index_to_insert_at(point_index_to_insert_at),
			d_oriented_pos_on_globe(oriented_pos_on_globe),
			d_nothing_has_been_done(false)
		{
			setText(QObject::tr("split feature"));
		}

		virtual
		void
		redo();		

		virtual
		void
		undo();
		
	private:
		GPlatesGui::FeatureFocus *d_feature_focus;
		const GPlatesModel::Gpgim &d_gpgim;
		GPlatesModel::ModelInterface d_model_interface;
		GeometryBuilder::PointIndex d_point_index_to_insert_at;
		boost::optional<GPlatesMaths::PointOnSphere> d_oriented_pos_on_globe;
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> d_old_geometry_property;
		GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection_ref;
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_new_feature;
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_old_feature;
		bool d_nothing_has_been_done;
	};
}

#endif	// GPLATES_VIEWOPERATIONS_SPLITFEATUREUNDOCOMMAND_H
