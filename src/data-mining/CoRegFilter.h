/* $Id: Filter.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file 
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
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
#ifndef GPLATESDATAMINING_COREGFILTER_H
#define GPLATESDATAMINING_COREGFILTER_H

#include <vector>
#include <bitset>
#include <QDebug>
#include <boost/variant.hpp>
#include <boost/variant/get.hpp>

#include "app-logic/ReconstructContext.h"

#include "global/LogException.h" 


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	class CoRegFilter
	{
	public:
		typedef std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature>
				reconstructed_feature_vector_type;
		
		class Config
		{
		public:
			virtual
			CoRegFilter*
			create_filter(
					const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature) = 0;
			
			virtual
			bool
			is_same_type(
					const Config* other) = 0;

			virtual
			const QString
			to_string(){ return QString("The derived class doesn't override this function.");}

			virtual 
			const QString
			filter_name() = 0;

			virtual
			std::vector<QString>
			get_parameters_as_strings() const 
			{
				qDebug() << "Using get_parameters_as_strings() in base class.";
				return std::vector<QString>();
			}

			virtual
			bool
			operator< (const CoRegFilter::Config&) = 0;

			virtual
			bool
			operator==(const CoRegFilter::Config&) = 0;

			virtual
			~Config() { }
		};

		virtual
		void
		process(
				reconstructed_feature_vector_type::const_iterator first,
				reconstructed_feature_vector_type::const_iterator last,
				reconstructed_feature_vector_type& output
		) = 0;

		virtual
		~CoRegFilter() {}
	};

	class DummyFilter : public CoRegFilter
	{
	public:
		class Config : public CoRegFilter::Config
		{
		public:
			CoRegFilter*
			create_filter(
					const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature) 
			{
				return new DummyFilter();
			}

			bool
			is_same_type(const CoRegFilter::Config* other)
			{
				return dynamic_cast<const DummyFilter*>(other);
			}

			const QString
			filter_name()
			{
				return "Dummy";
			}

			bool
			operator< (const CoRegFilter::Config& other)
			{
				throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"not implement yet.");
				return false;
			}

			bool
			operator==(const CoRegFilter::Config&) 
			{
				throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"not implement yet.");
				return false;
			}

			~Config(){}
		};

		void
		process(
				CoRegFilter::reconstructed_feature_vector_type::const_iterator first,
				CoRegFilter::reconstructed_feature_vector_type::const_iterator last,
				CoRegFilter::reconstructed_feature_vector_type& output) 
		{ }

		~DummyFilter(){ }
	};
}
#endif //GPLATESDATAMINING_COREGFILTER_H





