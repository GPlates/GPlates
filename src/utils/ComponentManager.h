/* $Id: CommandLineParser.h 7795 2010-03-16 04:26:54Z jcannon $ */

/**
* \file
* File specific comments.
*
* Most recent change:
*   $Date: 2010-03-16 15:26:54 +1100 (Tue, 16 Mar 2010) $
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

#ifndef GPLATES_UTILS_COMPONENT_MANAGER_H
#define GPLATES_UTILS_COMPONENT_MANAGER_H

#include <bitset>

namespace GPlatesUtils
{
	class ComponentManager
	{
		enum ComponentTypes
		{
			DATA_MINING = 0,
			PYTHON,
			SYMBOLOGY,
                        HELLINGER,

                        NUM_COMPONENTS
		};

	public:
		class Component
		{
		public:
			static
			Component
			data_mining()
			{
				return Component(ComponentManager::DATA_MINING);
			}

			static
			Component
			python()
			{
				return Component(ComponentManager::PYTHON);
			}

			static
			Component
			symbology()
			{
				return Component(ComponentManager::SYMBOLOGY);
			}

                        static
                        Component
                        hellinger()
                        {
                            return Component(ComponentManager::HELLINGER);
                        }

			operator 
			ComponentManager::ComponentTypes()
			{
				return d_type;
			}

			operator
			std::size_t()
			{
				return d_type;
			}

		private:
			Component(ComponentManager::ComponentTypes type) : d_type(type){}
			ComponentManager::ComponentTypes d_type;
		};

		friend class Component;
		void
		enable(Component t)
		{
			d_switchs.set(t);
		}

		void 
		disable(Component t)
		{
			d_switchs.reset(t);
		}

		bool
		is_enabled(Component t)
		{
			return d_switchs.test(t);
		}

		static
		ComponentManager&
		instance()
		{
			static ComponentManager instance;
			return instance;
		}

	private:
		ComponentManager() :
			d_switchs() //by default, all 0s. set default value here.
		{
			enable(Component::python()); // enable python by default
		}

		ComponentManager(const ComponentManager&);
		ComponentManager& operator=(const ComponentManager&);
		
                std::bitset<NUM_COMPONENTS> d_switchs;
	};

	
}
#endif    //GPLATES_UTILS_COMPONENT_MANAGER_H
