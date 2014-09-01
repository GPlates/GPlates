/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_EXPORTRASTERANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTRASTERANIMATIONSTRATEGY_H

#include <QString>

#include "ExportAnimationStrategy.h"

#include "property-values/Georeferencing.h"

#include "qt-widgets/GlobeAndMapWidget.h"

#include "utils/ReferenceCount.h"


namespace GPlatesGui
{
	// Forward declaration to avoid spaghetti
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for saving (colour or numerical)
	 * raster data (unwrapped to latitude/longitude) to a file at each timestep.
	 * 
	 * This class serves as the concrete Strategy role as
	 * described in Gamma et al. p315. It is used by ExportAnimationContext.
	 */
	class ExportRasterAnimationStrategy:
			public QObject,
			public GPlatesGui::ExportAnimationStrategy
	{
		Q_OBJECT

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ExportRasterAnimationStrategy>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportRasterAnimationStrategy> non_null_ptr_type;
		

		/**
		 * Configuration options.
		 */
		class Configuration :
				public ExportAnimationStrategy::ConfigurationBase
		{
		public:

			enum RasterType
			{
				NUMERICAL,
				COLOUR
			};


			Configuration(
					const QString& filename_template_,
					RasterType raster_type_,
					const double &resolution_in_degrees_,
					const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type &lat_lon_extents_) :
				ConfigurationBase(filename_template_),
				raster_type(raster_type_),
				resolution_in_degrees(resolution_in_degrees_),
				lat_lon_extents(lat_lon_extents_)
			{  }

			virtual
			configuration_base_ptr
			clone() const
			{
				return configuration_base_ptr(new Configuration(*this));
			}

			RasterType raster_type;
			double resolution_in_degrees;
			GPlatesPropertyValues::Georeferencing::lat_lon_extents_type lat_lon_extents;
		};

		//! Typedef for a shared pointer to const @a Configuration.
		typedef boost::shared_ptr<const Configuration> const_configuration_ptr;


		static
		const non_null_ptr_type
		create(
				ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration)
		{
			return non_null_ptr_type(
					new ExportRasterAnimationStrategy(
							export_animation_context,
							export_configuration));
		}

		virtual
		~ExportRasterAnimationStrategy()
		{  }

		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		ExportRasterAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const const_configuration_ptr &export_configuration);

	private:
		//! Export configuration parameters.
		const_configuration_ptr d_configuration;

	};
}

#endif // GPLATES_GUI_EXPORTRASTERANIMATIONSTRATEGY_H
