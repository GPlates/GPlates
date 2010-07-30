/* $Id$ */

/**
 * @file 
 * Contains the definition of the VGPRenderSettings class.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_APP_VGPRENDERSETTINGS_H
#define GPLATES_APP_VGPRENDERSETTINGS_H

#include <boost/optional.hpp>

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{

	/**
	 *  Stores render settings for VirtualGeomagneticPole features.                                                                     
	 */
	class VGPRenderSettings
	{
	public:

		static const double INITIAL_VGP_DELTA_T;

		enum VGPVisibilitySetting
		{
			ALWAYS_VISIBLE,
			TIME_WINDOW,
			DELTA_T_AROUND_AGE
		};		

		static
		inline
		VGPRenderSettings*
		instance()
		{
			if(!d_instance)
			{
				d_instance = new VGPRenderSettings();
			}
			return d_instance;
		}

		bool
		should_draw_vgp(
				double current_time,
				boost::optional<double> age) const;

		VGPVisibilitySetting 
		get_vgp_visibility_setting() const
		{	
			return d_vgp_visibility_setting;
		}

		void
		set_vgp_visibility_setting(
		VGPVisibilitySetting setting)
		{
			d_vgp_visibility_setting = setting;
		}

		double
		get_vgp_delta_t() const
		{
			return d_vgp_delta_t;
		}

		void
		set_vgp_delta_t(
				const double &vgp_delta_t)
		{	
			d_vgp_delta_t = vgp_delta_t;
		}

		const 
		GPlatesPropertyValues::GeoTimeInstant &
		get_vgp_earliest_time() 
		{
			return d_vgp_earliest_time;	
		};

		const 
		GPlatesPropertyValues::GeoTimeInstant &
		get_vgp_latest_time() 
		{
			return d_vgp_latest_time;	
		};

		void
		set_vgp_earliest_time(
			const GPlatesPropertyValues::GeoTimeInstant &earliest_time)
		{	
			d_vgp_earliest_time = GPlatesPropertyValues::GeoTimeInstant(earliest_time);
		}

		void
		set_vgp_latest_time(
			const GPlatesPropertyValues::GeoTimeInstant &latest_time)
		{	
			d_vgp_latest_time = GPlatesPropertyValues::GeoTimeInstant(latest_time);
		};		

		inline
		bool
		should_draw_circular_error() const
		{
			return d_should_draw_circular_error;
		}

		void
		set_should_draw_circular_error(
			bool should_draw_circular_error_)
		{
			d_should_draw_circular_error = should_draw_circular_error_;
		}

	private:
		VGPRenderSettings();
		VGPRenderSettings(
				const VGPRenderSettings& );

		/**
		 *  enum indicating what sort of VGP visibility we have, one of:
		 *		ALWAYS_VISIBLE		- all vgps are displayed at all times
		 *		TIME_WINDOW			- all vgps are displayed between a specified time interval
		 *		DELTA_T_AROUND_AGE  - vgps are displayed if the reconstruction time is within a time window 
		 *							  around the VGP's age.                                                                   
		 */
		VGPVisibilitySetting d_vgp_visibility_setting;

		/**
		 *  Delta used for time window around VGP age.                                                                     
		 */
		double d_vgp_delta_t;

		/**
		 *  Begin time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
		 */
		GPlatesPropertyValues::GeoTimeInstant d_vgp_earliest_time;

		/**
		 *  End time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
		 */
		GPlatesPropertyValues::GeoTimeInstant d_vgp_latest_time;

		/**
		 * Whether or not we should draw pole errors as circles around the pole location.
		 * 
		 * If true, we draw circles (circle size defined by the A95 property).
		 * If false, we draw ellipses. (ellipse size defined by yet-to-be-calculated properties).                                                                     
		 */
		bool d_should_draw_circular_error;
		
		static VGPRenderSettings* d_instance;

	};	
}

#endif  // GPLATES_APP_VGPRENDERSETTINGS_H
