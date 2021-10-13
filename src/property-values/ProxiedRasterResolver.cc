/* $Id$ */

/**
 * \file 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include "ProxiedRasterResolver.h"


namespace
{
	using namespace GPlatesPropertyValues;

	class CreateProxiedRasterResolverVisitorImpl
	{
	public:

		typedef boost::optional<ProxiedRasterResolver::non_null_ptr_type> result_type;

		result_type
		get_result() const
		{
			return d_result;
		}

	private:

		template<class RawRasterType, bool has_proxied_data>
		class Create
		{
		public:

			static
			result_type
			create(
					RawRasterType &raster)
			{
				// No work to do; has_proxied_data = false case handled here.
				return boost::none;
			}
		};

		template<class RawRasterType>
		class Create<RawRasterType, true>
		{
		public:

			static
			result_type
			create(
					RawRasterType &raster)
			{
				boost::optional<typename ProxiedRasterResolverImpl<RawRasterType>::non_null_ptr_type> result =
					ProxiedRasterResolverImpl<RawRasterType>::create(&raster);
				if (result)
				{
					return result_type(*result);
				}
				else
				{
					return boost::none;
				}
			}
		};

		template<class RawRasterType>
		void
		do_visit(
				RawRasterType &raster)
		{
			d_result = Create<RawRasterType, RawRasterType::has_proxied_data>::create(raster);
		}

		friend class TemplatedRawRasterVisitor<CreateProxiedRasterResolverVisitorImpl>;

		result_type d_result;
	};

	typedef TemplatedRawRasterVisitor<CreateProxiedRasterResolverVisitorImpl>
		CreateProxiedRasterResolverVisitor;
}


GPlatesPropertyValues::ProxiedRasterResolver::ProxiedRasterResolver()
{  }


GPlatesPropertyValues::ProxiedRasterResolver::~ProxiedRasterResolver()
{  }


boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type>
GPlatesPropertyValues::ProxiedRasterResolver::create(
		const RawRaster::non_null_ptr_type &raster)
{
	CreateProxiedRasterResolverVisitor visitor;
	raster->accept_visitor(visitor);

	return visitor.get_result();
}
