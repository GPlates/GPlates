/* $Id$ */

/**
 * \file 
 * Used to group a subset of @a RenderedGeometry objects.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYER_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYER_H

#include <boost/any.hpp>
#include <vector>
#include <QObject>

#include "RenderedGeometry.h"


namespace GPlatesViewOperations
{
	class ConstRenderedGeometryLayerVisitor;
	class RenderedGeometryLayerVisitor;

	class RenderedGeometryLayer :
		public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Typedef for arbitrary user-supplied data that will be returned when
		 * @a layer_was_updated signal is emitted.
		 */
		typedef boost::any user_data_type;

		/**
		 * Typedef for a @a RenderedGeometry index.
		 */
		typedef unsigned int RenderedGeometryIndex;

		/**
		 * Sets this layer as inactive.
		 *
		 * @param user_data arbitrary user-supplied data that will be returned
		 * when @a layer_was_updated signal is emitted.
		 */
		RenderedGeometryLayer(
				user_data_type user_data = user_data_type());

		void
		set_active(
				bool active = true);

		bool
		is_active() const
		{
			return d_is_active;
		}

		bool
		is_empty() const
		{
			return d_rendered_geom_seq.empty();
		}

		unsigned int
		get_num_rendered_geometries() const
		{
			return d_rendered_geom_seq.size();
		}

		RenderedGeometry
		get_rendered_geometry(
				RenderedGeometryIndex rendered_geom_index) const
		{
			return d_rendered_geom_seq[rendered_geom_index];
		}

		void
		add_rendered_geometry(
				RenderedGeometry);

		void
		clear_rendered_geometries();

		void
		accept_visitor(
				ConstRenderedGeometryLayerVisitor &) const;

		void
		accept_visitor(
				RenderedGeometryLayerVisitor &);

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Signal is emitted whenever this rendered geometry layer has been
		 * updated. Currently only @a RenderedGeometryCollection needs to listen to this.
		 *
		 * This includes:
		 * - any @a RenderedGeometry objects added.
		 * - clearing of @a RenderedGeometry objects.
		 * - changing active status.
		 */
		void
		layer_was_updated(
				GPlatesViewOperations::RenderedGeometryLayer &,
				GPlatesViewOperations::RenderedGeometryLayer::user_data_type user_data);

	private:
		typedef std::vector<RenderedGeometry> rendered_geom_seq_type;

		user_data_type d_user_data;
		rendered_geom_seq_type d_rendered_geom_seq;
		bool d_is_active;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYER_H
