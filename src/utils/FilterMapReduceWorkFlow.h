/* $Id$ */

/**
 * \file validate filename template.
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
#ifndef GPLATES_UTILS_FILTERMAPREDUCEWORKFLOW_H
#define GPLATES_UTILS_FILTERMAPREDUCEWORKFLOW_H

#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/is_same.hpp>

#include <QDebug>

namespace GPlatesUtils
{
	/*
	* FilterMapReduceWorkFlow creates a filter, map reduce objects chain.
	* There always only one reduce object at the end of the chain.
	* The chain can have multiple filter and map objects. 
	* The client code should guarantee the types of input and output compatible. 
	* boost::mpl::reverse_fold has been used here to make the code concise and clear.
	* The client can use this work flow easily, for example
	*		data = 
				FilterMapReduceWorkFlow<TypeList,
						CoRegReducer,
						InputSequence::const_iterator,
						OpaqueData
						>::exec(work_units, *d_reducer, begin, end);

	* Above function call will return the reduced data.
	*/
	template< 
			class FilterMapList,
			class ReducerType,
			class InputIterator,
			class OutputDataType >
	struct FilterMapReduceWorkFlow
	{
		typedef typename ReducerType::InputIteratorType ReturnType;

		//The caller needs to make the unit_list doesn't contain any invalid item( null pointer ).
		static
		OutputDataType 
		exec(
				std::vector< boost::any >& unit_list,
				ReducerType& reducer,
				InputIterator input_begin,
				InputIterator input_end)
		{
			typedef typename create_workflow< FilterMapList >::type workflow;
			typename ReducerType::InputIteratorType begin, end;
			boost::tie(begin, end) = workflow::exec(unit_list.begin(), input_begin, input_end);
			return reducer(begin,end);
		}
	
		struct NullProcessUnit
		{
			template< class Iterator >
			static
			boost::tuple< Iterator, Iterator >
			exec(
					std::vector< boost::any >::iterator,
					Iterator begin,
					Iterator end)
			{ 
				#ifdef _DEBUG
				qDebug()<< "reached the end of work flow." ;
				#endif
				return boost::make_tuple(begin, end); 
			}
		};

		template<class Current, class Next>
		struct ProcessUnit
		{	
			static
			boost::tuple< 
					ReturnType, 
					ReturnType >
			exec(
					std::vector< boost::any >::iterator unit_iter,
					typename Current::InputIteratorType input_begin,
					typename Current::InputIteratorType input_end)
			{ 
				try
				{
					#ifdef _DEBUG
					qDebug()<<"Processing...." ;
					#endif
					boost::shared_ptr<Current> process_unit = 
						boost::any_cast<boost::shared_ptr<Current> >(*unit_iter);
					typename Current::OutputIteratorType result_begin;
					typename Current::OutputIteratorType result_end;

					boost::tuples::tie(result_begin, result_end) = 
						(*process_unit)(
								input_begin, 
								input_end);
					unit_iter++;
					#ifdef _DEBUG
					qDebug() << "about to move to next stage....";
					#endif
					return Next::exec(unit_iter, result_begin, result_end);
				}
				catch(const boost::bad_any_cast &)
				{
					#ifdef _DEBUG
					qDebug() << "unexpected process unit encountered.";
					#endif
					throw;
				}
			}
		};

		template< class TypeList >
		struct create_workflow : 
			boost::mpl::reverse_fold<
					TypeList, 
					NullProcessUnit,
					ProcessUnit<boost::mpl::placeholders::_2,boost::mpl::placeholders::_1> 
					 >
			{ };
	};
}

#endif
