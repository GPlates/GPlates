/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_PROJECTION_H
#define GPLATES_GUI_PROJECTION_H

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QObject>

#include "GlobeProjectionType.h"
#include "MapProjection.h"
#include "ViewportProjectionType.h"

#include "maths/MathsUtils.h"
#include "maths/types.h"


namespace GPlatesGui
{
	/**
	 * A central place to set both the globe/map projection, and the viewport projection (orthographic/perspective),
	 * and listens for changes.
	 */
	class Projection :
			public QObject
	{
		Q_OBJECT
		
	public:

		/**
		 * Helper class to handle globe/map projection (and their parameters).
		 */
		class GlobeMapProjection
		{
		public:
			//! Typedef for globe projection.
			typedef GlobeProjection::Type globe_projection_type;

			//! Typedef for a map projection.
			typedef MapProjection::Type map_projection_type;
			

			//! For globe projection.
			explicit
			GlobeMapProjection(
					globe_projection_type globe_projection_type_) :
				d_projection_type(globe_projection_type_),
				d_map_central_meridian(0)
			{  }

			//! For map projection.
			GlobeMapProjection(
					map_projection_type map_projection_type_,
					const double &map_central_meridian) :
				d_projection_type(map_projection_type_),
				d_map_central_meridian(map_central_meridian)
			{  }

			//! Default state is viewing the globe (and map central meridian is zero).
			GlobeMapProjection() :
				d_projection_type(GlobeProjection::GLOBE),
				d_map_central_meridian(0)
			{  }


			//! Sets viewing to the globe.
			void
			set_globe_projection_type(
					globe_projection_type globe_projection_type_)
			{
				d_projection_type = globe_projection_type_;
			}

			//! Sets viewing to the map.
			void
			set_map_projection_type(
					map_projection_type map_projection_type_)
			{
				d_projection_type = map_projection_type_;
			}

			void
			set_map_central_meridian(
					const double &map_central_meridian)
			{
				d_map_central_meridian = map_central_meridian;
			}


			//! Returns true if viewing globe (otherwise viewing map).
			bool
			is_viewing_globe_projection() const
			{
				return static_cast<bool>(boost::get<globe_projection_type>(&d_projection_type));
			}

			/**
			 * Returns globe projection (if viewing globe), otherwise throws boost::bad_get).
			 *
			 * Check @a is_viewing_globe_projection before calling.
			 */
			globe_projection_type
			get_globe_projection_type() const
			{
				return boost::get<globe_projection_type>(d_projection_type);
			}


			//! Returns true if viewing map (otherwise viewing globe).
			bool
			is_viewing_map_projection() const
			{
				return !is_viewing_globe_projection();
			}

			/**
			 * Returns map projection (if viewing map), otherwise throws boost::bad_get).
			 *
			 * Check @a is_viewing_globe_projection before calling.
			 */
			map_projection_type
			get_map_projection_type() const
			{
				return boost::get<map_projection_type>(d_projection_type);
			}

			double
			get_map_central_meridian() const
			{
				return d_map_central_meridian.dval();
			}


			bool
			operator==(
					const GlobeMapProjection &other) const
			{
				return d_projection_type == other.d_projection_type &&
						d_map_central_meridian == other.d_map_central_meridian;
			}

			bool
			operator!=(
					const GlobeMapProjection &other) const
			{
				return !operator==(other);
			}

		private:
			/**
			 * Typedef for globe or map projection.
			 *
			 * Either viewing globe projection or a map projection (not both).
			 */
			typedef boost::variant<globe_projection_type, map_projection_type> projection_type;

			projection_type d_projection_type;
			GPlatesMaths::real_t d_map_central_meridian;
		};

		//! Typedef for a globe/map projection.
		typedef GlobeMapProjection globe_map_projection_type;

		//! Typedef for a viewport projection (orthographic/perspective).
		typedef ViewportProjection::Type viewport_projection_type;


		/**
		 * Default state is orthographic viewing of the globe.
		 */
		Projection() :
			d_viewport_projection(ViewportProjection::ORTHOGRAPHIC)
		{  }


		//! Set globe/map projection and notify any listeners if it changed.
		void
		set_globe_map_projection(
				const globe_map_projection_type &globe_map_projection)
		{
			if (globe_map_projection != d_globe_map_projection)
			{
				Q_EMIT projection_about_to_change();
				Q_EMIT globe_map_projection_about_to_change();

				const globe_map_projection_type old_globe_map_projection = d_globe_map_projection;
				d_globe_map_projection = globe_map_projection;

				Q_EMIT globe_map_projection_changed(old_globe_map_projection, d_globe_map_projection);
				Q_EMIT projection_changed(
						old_globe_map_projection, d_viewport_projection,  // old
						d_globe_map_projection, d_viewport_projection);   // new
			}
		}


		//! Set viewport projection and notify any listeners if it changed.
		void
		set_viewport_projection(
				viewport_projection_type viewport_projection)
		{
			if (viewport_projection != d_viewport_projection)
			{
				Q_EMIT projection_about_to_change();
				Q_EMIT viewport_projection_about_to_change();

				const viewport_projection_type old_viewport_projection = d_viewport_projection;
				d_viewport_projection = viewport_projection;

				Q_EMIT viewport_projection_changed(old_viewport_projection, d_viewport_projection);
				Q_EMIT projection_changed(
						d_globe_map_projection, old_viewport_projection,  // old
						d_globe_map_projection, d_viewport_projection);   // new
			}
		}


		//! Set both the globe/map projection and viewport projection and notify any listeners (once) if either changed.
		void
		set_projection(
				const globe_map_projection_type &globe_map_projection,
				viewport_projection_type viewport_projection)
		{
			const bool has_globe_map_projection_changed = globe_map_projection != d_globe_map_projection;
			const bool has_viewport_projection_changed = viewport_projection != d_viewport_projection;

			if (has_globe_map_projection_changed || has_viewport_projection_changed)
			{
				if (has_globe_map_projection_changed && has_viewport_projection_changed)
				{
					Q_EMIT projection_about_to_change();
					Q_EMIT globe_map_projection_about_to_change();
					Q_EMIT viewport_projection_about_to_change();

					const globe_map_projection_type old_globe_map_projection = d_globe_map_projection;
					d_globe_map_projection = globe_map_projection;

					const viewport_projection_type old_viewport_projection = d_viewport_projection;
					d_viewport_projection = viewport_projection;

					Q_EMIT viewport_projection_changed(old_viewport_projection, d_viewport_projection);
					Q_EMIT globe_map_projection_changed(old_globe_map_projection, d_globe_map_projection);
					Q_EMIT projection_changed(
							old_globe_map_projection, old_viewport_projection,  // old
							d_globe_map_projection, d_viewport_projection);     // new
				}
				else if (has_globe_map_projection_changed)  // but not has_viewport_projection_changed
				{
					Q_EMIT projection_about_to_change();
					Q_EMIT globe_map_projection_about_to_change();

					const globe_map_projection_type old_globe_map_projection = d_globe_map_projection;
					d_globe_map_projection = globe_map_projection;

					Q_EMIT globe_map_projection_changed(old_globe_map_projection, d_globe_map_projection);
					Q_EMIT projection_changed(
							old_globe_map_projection, d_viewport_projection,  // old
							d_globe_map_projection, d_viewport_projection);     // new
				}
				else // has_viewport_projection_changed but not has_globe_map_projection_changed
				{
					Q_EMIT projection_about_to_change();
					Q_EMIT viewport_projection_about_to_change();

					const viewport_projection_type old_viewport_projection = d_viewport_projection;
					d_viewport_projection = viewport_projection;

					Q_EMIT viewport_projection_changed(old_viewport_projection, d_viewport_projection);
					Q_EMIT projection_changed(
							d_globe_map_projection, old_viewport_projection,  // old
							d_globe_map_projection, d_viewport_projection);   // new
				}
			}
		}


		/**
		 * Returns globe/map projection.
		 */
		const globe_map_projection_type &
		get_globe_map_projection() const
		{
			return d_globe_map_projection;
		}

		/**
		 * Returns viewport projection.
		 */
		viewport_projection_type
		get_viewport_projection() const
		{
			return d_viewport_projection;
		}

	Q_SIGNALS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		//! Anything about the projection (globe/map and/or viewport projection) is about to change.
		void
		projection_about_to_change();

		//! Anything about the projection (globe/map and/or viewport projection) just changed.
		void
		projection_changed(
				// Old...
				const GPlatesGui::Projection::globe_map_projection_type &old_globe_map_projection,
				GPlatesGui::Projection::viewport_projection_type old_viewport_projection,
				// New...
				const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection,
				GPlatesGui::Projection::viewport_projection_type viewport_projection);


		//! Globe/map projection is about to change.
		void
		globe_map_projection_about_to_change();

		//! Globe/map projection just changed.
		void
		globe_map_projection_changed(
				// Old...
				const GPlatesGui::Projection::globe_map_projection_type &old_globe_map_projection,
				// New...
				const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection);


		//! Viewport projection is about to change.
		void
		viewport_projection_about_to_change();

		//! Viewport projection just changed.
		void
		viewport_projection_changed(
				// Old...
				GPlatesGui::Projection::viewport_projection_type old_viewport_projection,
				// New...
				GPlatesGui::Projection::viewport_projection_type viewport_projection);

	private:
		globe_map_projection_type d_globe_map_projection;

		viewport_projection_type d_viewport_projection;
	};
}

#endif // GPLATES_GUI_PROJECTION_H
