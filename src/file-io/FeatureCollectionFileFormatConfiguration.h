/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCONFIGURATION_H
#define GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCONFIGURATION_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "FeatureCollectionFileFormat.h"

#include "utils/CopyConst.h"


namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		/**
		 * Base class for specifying configuration options (such as for reading and/or writing a
		 * feature collection from/to a file).
		 *
		 * If a file format requires specialised options then create a derived
		 * @a Configuration class for it and register that with @a Registry.
		 */
		class Configuration
		{
		public:
			//! Typedef for a shared pointer to const @a Configuration.
			typedef boost::shared_ptr<const Configuration> shared_ptr_to_const_type;
			//! Typedef for a shared pointer to @a Configuration.
			typedef boost::shared_ptr<Configuration> shared_ptr_type;


			virtual
			~Configuration()
			{  }
		};


		/**
		 * A convenience function to dynamic cast a shared pointer to a @a Configuration
		 * into a shared pointer to a derived type 'ConfigurationDerivedType'.
		 *
		 * Const usage:
		 *   Configuration::shared_ptr_to_const_type configuration;
		 *   boost::optional<DerivedConfiguration::shared_ptr_to_const_type> derived_configuration =
		 *       dynamic_cast_configuration<const DerivedConfiguration>(configuration);
		 *
		 * Non-const usage:
		 *   Configuration::shared_ptr_type configuration;
		 *   boost::optional<DerivedConfiguration::shared_ptr_type> derived_configuration =
		 *       dynamic_cast_configuration<DerivedConfiguration>(configuration);
		 *
		 * Returns boost::none if type of @a configuration cannot be cast to 'ConfigurationDerivedType'.
		 */
		template <class ConfigurationDerivedType>
		boost::optional<boost::shared_ptr<ConfigurationDerivedType> >
		dynamic_cast_configuration(
				const boost::shared_ptr<typename GPlatesUtils::CopyConst<ConfigurationDerivedType, Configuration>::type> &configuration)
		{
			boost::shared_ptr<ConfigurationDerivedType>
					derived_configuration =
							boost::dynamic_pointer_cast<
									ConfigurationDerivedType>(
											configuration);
			if (!derived_configuration)
			{
				return boost::none;
			}

			return derived_configuration;
		}


		/**
		 * Similar to the other overload of @a dynamic_cast_configuration but accepts a boost::optional.
		 */
		template <class ConfigurationDerivedType>
		boost::optional<boost::shared_ptr<ConfigurationDerivedType> >
		dynamic_cast_configuration(
				const boost::optional<boost::shared_ptr<
						typename GPlatesUtils::CopyConst<ConfigurationDerivedType, Configuration>::type> > &configuration)
		{
			if (!configuration)
			{
				return boost::none;
			}

			return dynamic_cast_configuration<ConfigurationDerivedType>(configuration.get());
		}


		/**
		 * A convenience function to dynamic cast a shared pointer to a @a Configuration
		 * into a shared pointer to a derived type 'ConfigurationDerivedType' and
		 * then return a *copy* of that (using the copy constructor of 'DerivedConfiguration').
		 *
		 * Usage:
		 *   Configuration::shared_ptr_to_const_type configuration;
		 *   boost::optional<DerivedConfiguration::shared_ptr_type> derived_configuration =
		 *       copy_cast_configuration<DerivedConfiguration>(configuration);
		 *
		 * Returns boost::none if type of @a configuration cannot be cast to 'ConfigurationDerivedType'.
		 */
		template <class ConfigurationDerivedType>
		boost::optional<boost::shared_ptr<ConfigurationDerivedType> >
		copy_cast_configuration(
				const Configuration::shared_ptr_to_const_type &configuration)
		{
			typedef typename boost::add_const<ConfigurationDerivedType>::type const_configuration_derived_type;
			typedef typename boost::remove_const<ConfigurationDerivedType>::type non_const_configuration_derived_type;

			boost::optional<boost::shared_ptr<const_configuration_derived_type> > derived_configuration =
					dynamic_cast_configuration<const_configuration_derived_type>(configuration);
			if (!derived_configuration)
			{
				return boost::none;
			}

			return boost::shared_ptr<ConfigurationDerivedType>(
					// Invoke copy constructor of derived configuration class...
					new non_const_configuration_derived_type(*derived_configuration.get()));
		}


		/**
		 * Similar to the other overload of @a copy_cast_configuration but accepts a boost::optional.
		 */
		template <class ConfigurationDerivedType>
		boost::optional<boost::shared_ptr<ConfigurationDerivedType> >
		copy_cast_configuration(
				const boost::optional<Configuration::shared_ptr_to_const_type> &configuration)
		{
			if (!configuration)
			{
				return boost::none;
			}

			return copy_cast_configuration<ConfigurationDerivedType>(configuration.get());
		}
	}
}

#endif // GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCONFIGURATION_H
