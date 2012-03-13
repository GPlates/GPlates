/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#ifndef GPLATESDATAMINING_COREGCONFIGURATIONTABLE_H
#define GPLATESDATAMINING_COREGCONFIGURATIONTABLE_H

#include <vector>
#include <map>
#include <boost/operators.hpp>

#include "CoRegFilter.h"
#include "Types.h"
#include "DataTable.h"

#include "app-logic/Layer.h"

#include "global/GPlatesException.h"

#include "model/FeatureHandle.h"


namespace GPlatesDataMining
{
	struct ConfigurationTableRow
	{
		ConfigurationTableRow();

		bool
		operator==(
				const ConfigurationTableRow &rhs) const;

		GPlatesAppLogic::Layer target_layer;
		boost::shared_ptr<CoRegFilter::Config> filter_cfg;
		QString attr_name;
		AttributeType attr_type;
		ReducerType reducer_type; //TODO: change to CoRegReducer::config
		unsigned int raster_level_of_detail; // Only used if target layer is a raster.
		bool raster_fill_polygons; // Cuurently only used if target layer is a raster.
		unsigned index;
	};

	class CoRegCfgTableOptimized : GPlatesGlobal::Exception
	{
	public:
		CoRegCfgTableOptimized(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::Exception(exception_source)
		{ }
	protected:
		const char *
		exception_name() const
		{
			return "CoRegCfgTableOptimized Exception";
		}

		void
		write_message(
				std::ostream &os) const
		{
			os << "The co-registration table has been optimized. The table is readonly now.";
		}
	};

	class CoRegConfigurationTable :
			public boost::equality_comparable<CoRegConfigurationTable>
	{
	public:
		typedef std::vector<ConfigurationTableRow>::iterator iterator;
		typedef std::vector<ConfigurationTableRow>::const_iterator const_iterator;
		
		iterator
		begin()
		{
			if(d_optimized)
				throw CoRegCfgTableOptimized(GPLATES_EXCEPTION_SOURCE);
			return d_rows.begin();
		}

		const_iterator
		begin() const
		{
			return d_rows.begin();
		}

		iterator
		end()
		{
			if(d_optimized)
				throw CoRegCfgTableOptimized(GPLATES_EXCEPTION_SOURCE);
			return d_rows.end();
		}

		const_iterator
		end() const
		{
			return d_rows.end();
		}
		
		ConfigurationTableRow &
		operator[](
				std::size_t index)
		{
			return d_rows[index];
		}
		
		const ConfigurationTableRow &
		operator[](
				std::size_t index) const
		{
			return d_rows[index];
		}

		/**
		 * 'operator!=()' provided by boost::equality_comparable.
		 */
		bool
		operator==(
				const CoRegConfigurationTable &rhs) const
		{
			return d_rows == rhs.d_rows;
		}

		void
		clear()
		{
			if(d_optimized)
				throw CoRegCfgTableOptimized(GPLATES_EXCEPTION_SOURCE);
			d_rows.clear();
		}

		void
		push_back(const ConfigurationTableRow& row)
		{
			if(d_optimized)
				throw CoRegCfgTableOptimized(GPLATES_EXCEPTION_SOURCE);
			d_rows.push_back(row);
		}

		void
		optimize();

		bool
		is_optimized()
		{
			return d_optimized;
		}

		CoRegConfigurationTable() :
			d_optimized(false)
		{ }

		std::size_t
		size() const
		{
			return d_rows.size();
		}

	protected:
		void
		group_and_sort();

	private:
		std::vector< ConfigurationTableRow > d_rows;
		bool d_optimized; // if the table has been optimized, the rows are readonly.
	};

	inline
	std::ostream &
	operator<<(
			std::ostream &o,
			const ConfigurationTableRow &g)
	{
		o << g.filter_cfg->to_string().toStdString() << "\t";
		o << g.attr_name.toStdString() <<  "\t";
		o << g.attr_type <<  "\t";
		o << g.reducer_type << "\t";
		return o;
	}
}
#endif //GPLATESDATAMINING_COREGCONFIGURATIONTABLE_H




