/* $Id$ */

/**
* \file 
* Source code profiling.
* $Revision$
* $Date$
* 
* Copyright (C) 2009 The University of Sydney, Australia
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

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/operators.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/bind.hpp>
#include <stack>
#include <vector>
#include <string>
#include <map>
#include <iterator>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <functional>

#include "Profile.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

namespace
{
	/**
	 * Stores platform-dependent tick count.
	 * Time unit varies across platforms.
	 */
	typedef boost::uint64_t ticks_t;

	/**
	 * Stores number of get_calls to a profiled section of code.
	 */
	typedef boost::uint64_t calls_t;

	/**
	 * Returns current time in units of @a ticks_t.
	 */
	ticks_t
			get_ticks();

	/**
	 * Converts ticks to seconds.
	 */
	double
	convert_ticks_to_seconds(
			ticks_t);


	class ProfileNode;

	/**
	 * Responsible for profiling a running segment of code.
	 */
	class ProfileRun
	{
	public:
		ProfileRun(
				ProfileNode &profile_node) :
			d_profile_node(&profile_node),
			d_self_ticks(0),
			d_children_ticks(0),
			d_last_ticks(0)
		{ }

		ProfileRun(
				ProfileNode &profile_node,
				const ticks_t &start_ticks) :
			d_profile_node(&profile_node),
			d_last_ticks(start_ticks)
		{ }

		/**
		 * The return value should be assigned the value of @a get_ticks()
		 * when the profiling actually starts (ie the time when we start
		 * executing regular code again.
		 */
		ticks_t &
		start_profile()
		{
			return d_last_ticks;
		}

		/**
		 * Update the self ticks between now and when the currently
		 * profiled object started profiling.
		 */
		void
		stop_profile(
				const ticks_t &stop_ticks)
		{
			if ( d_last_ticks < stop_ticks )
			{
				d_self_ticks += stop_ticks - d_last_ticks;
			}
		}

		/**
		 * Transfer information to the ProfileNode that we're
		 * referencing - a parent ProfileRun is passed in if it exists.
		 */
		void
		finished_profiling(
				ProfileRun& parent_run);

		void
		finished_profiling();

		const ticks_t &
		get_self_ticks() const
		{
			return d_self_ticks;
		}

		const ticks_t &
		get_children_ticks() const
		{
			return d_children_ticks;
		}

		//! Returns node in call graph associated with this profile run.
		const ProfileNode *
		get_profile_node() const
		{
			return d_profile_node;
		}

	private:
		ProfileNode *d_profile_node;
		ticks_t d_self_ticks;   //!< Cycles spent in our run (excluding child runs).
		ticks_t d_children_ticks;  //!< Cycles spent in children ProfileRuns.
		ticks_t d_last_ticks;
	};


	/**
	 * Links between @a ProfileNode objects in the call graph.
	 */
	class ProfileLink
	{
	public:
		/**
		 * Typedef for pool allocator used to allocate @a ProfileLink objects.
		 */
		typedef boost::object_pool<ProfileLink> profile_link_pool_type;

		/**
		 * Shared pointer to @a ProfileLink object.
		 */
		typedef boost::shared_ptr<ProfileLink> pointer_type;

		/**
		 * Creates a ProfileLink and connects it between 'parent' and 'child'.
		 */
		static
		pointer_type
		create_profile_link(
				ProfileNode &parent,
				ProfileNode &child);

		//! Update with info from a get_child ProfileRun.
		void
		update(
				const ProfileRun &child_run)
		{
			++d_calls;
			d_ticks_in_child += child_run.get_self_ticks();
			d_ticks_in_childs_children += child_run.get_children_ticks();
		}

		calls_t
		get_calls() const
		{
			return d_calls;
		}

		const ProfileNode *
		get_child() const
		{
			return d_child;
		}

		const ProfileNode *
		get_parent() const
		{
			return d_parent;
		}

		const ticks_t &
		get_ticks_in_child() const
		{
			return d_ticks_in_child;
		}

		const ticks_t &
		get_ticks_in_child_children() const
		{
			return d_ticks_in_childs_children;
		}

	private:
		ProfileLink(
				const ProfileNode *parent,
				const ProfileNode *child) :
			d_child(child),
			d_parent(parent),
			d_ticks_in_child(0),
			d_ticks_in_childs_children(0),
			d_calls(0)
		{ }

		const ProfileNode *d_child;
		const ProfileNode *d_parent;
		ticks_t d_ticks_in_child;
		ticks_t d_ticks_in_childs_children;
		calls_t d_calls;

		//! Used to efficiently allocate memory for @a ProfileLink objects.
		static profile_link_pool_type s_profile_link_pool;
	};

	ProfileLink::profile_link_pool_type ProfileLink::s_profile_link_pool;

	ProfileLink::pointer_type
	ProfileLink::create_profile_link(
			ProfileNode &parent,
			ProfileNode &child)
	{
		ProfileLink *link_mem = s_profile_link_pool.malloc();
		if (link_mem == NULL)
		{
			throw std::bad_alloc();
		}

		return pointer_type(
				new (link_mem) ProfileLink(&parent, &child),
				boost::bind(
						&profile_link_pool_type::destroy,
						boost::ref(s_profile_link_pool),
						_1));
	}


	/**
	 * A node in the call graph that keeps track of time spent in
	 * code segments profiled with the same profile name.
	 */
	class ProfileNode
	{
	public:
		//! Typedef for a sequence of ProfileNode objects.
		typedef std::map<const ProfileNode *, ProfileLink::pointer_type> profile_link_map_type;

		//! Typedef for a const iterator to a sequence of ProfileNode objects.
		typedef profile_link_map_type::const_iterator profile_link_map_const_iterator;

		/**
		 * Iterator links in the call graph eminating from a @a ProfileNode object.
		 * Links are either the child links or the parent links.
		 */
		class ProfileLinkIterator :
				public std::iterator<
						std::iterator_traits<profile_link_map_const_iterator>::iterator_category,
						ProfileLink>,
				public boost::equality_comparable<ProfileLinkIterator>
		{
		public:
			ProfileLinkIterator(
					const profile_link_map_type &profile_link_map,
					const profile_link_map_const_iterator &profile_link_map_const_iter) :
				d_profile_link_map(&profile_link_map),
				d_profile_link_map_const_iter(profile_link_map_const_iter)
			{ }

			bool
			operator==(
					const ProfileLinkIterator &rhs) const
			{
				return d_profile_link_map == rhs.d_profile_link_map &&
					d_profile_link_map_const_iter == rhs.d_profile_link_map_const_iter;
			}

			const ProfileLink &
			operator*() const
			{
				return *d_profile_link_map_const_iter->second;
			}

			const ProfileLink *
			operator->() const
			{
				return d_profile_link_map_const_iter->second.get();
			}

			//! Pre-increment.
			ProfileLinkIterator &
			operator++()
			{
				++d_profile_link_map_const_iter;
				return *this;
			}

			//! Post-increment.
			const ProfileLinkIterator
			operator++(int)
			{
				ProfileLinkIterator old = *this;
				++d_profile_link_map_const_iter;
				return old;
			}

			//! Pre-decrement this iterator.
			ProfileLinkIterator &
			operator--()
			{
				--d_profile_link_map_const_iter;
				return *this;
			}

			//! Post-decrement this iterator.
			const ProfileLinkIterator
			operator--(int)
			{
				ProfileLinkIterator old = *this;
				--d_profile_link_map_const_iter;
				return old;
			}

		private:
			const profile_link_map_type *d_profile_link_map;
			profile_link_map_const_iterator d_profile_link_map_const_iter;
		};

		//! Iterator over sequence of ProfileNode objects.
		typedef ProfileLinkIterator profile_count_const_iterator;

		ProfileNode(
				const std::string &profileName) :
			d_name(profileName),
			d_self_ticks(0)
		{ }

		//! Update from info in a ProfileRun - the 'parent' is given if there is one.
		void
		update(
				const ProfileRun &run,
				ProfileNode &parent);

		void
		update(
				const ProfileRun &run)
		{
			d_self_ticks += run.get_self_ticks();
		}

		const std::string &
		get_name() const
		{
			return d_name;
		}

		//! The number of ticks counted - not including children.
		const ticks_t &
		get_self_ticks() const
		{
			return d_self_ticks;
		}

		profile_count_const_iterator
		parent_profiles_begin() const
		{
			return profile_count_const_iterator(d_parent_profiles, d_parent_profiles.begin());
		}

		profile_count_const_iterator
		parent_profiles_end() const
		{
			return profile_count_const_iterator(d_parent_profiles, d_parent_profiles.end());
		}

		profile_count_const_iterator child_profiles_begin() const
		{
			return profile_count_const_iterator(d_child_profiles, d_parent_profiles.begin());
		}

		profile_count_const_iterator child_profiles_end() const
		{
			return profile_count_const_iterator(d_child_profiles, d_parent_profiles.end());
		}

	private:
		std::string d_name;
		ticks_t d_self_ticks;

		profile_link_map_type d_parent_profiles;
		profile_link_map_type d_child_profiles;

		/**
		 * Creates a ProfileLink and connects it between 'parent' and 'child'.
		 * There must not already exist such a link.
		 */
		static
		void
		create_call_graph_link(
				ProfileNode &parent,
				ProfileNode &child)
		{
			ProfileLink::pointer_type link = ProfileLink::create_profile_link(parent, child);
			child.d_parent_profiles[&parent] = link;
			parent.d_child_profiles[&child] = link;
		}
	};

	void
	ProfileNode::update(
		const ProfileRun &run,
		ProfileNode &parent_node)
	{
		// Get the call graph ProfileLink to parent_node.
		profile_link_map_type::iterator p = d_parent_profiles.find(&parent_node);
		if ( p == d_parent_profiles.end() )
		{
			// Create a ProfileLink between parent_node and us.
			create_call_graph_link(parent_node, *this);

			// The previous call should've added a ProfileLink to our parent profiles.
			p = d_parent_profiles.find(&parent_node);
			GPlatesGlobal::Assert(p != d_parent_profiles.end(),
					GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
		}

		// Get this link to update itself from info in 'run'.
		ProfileLink::pointer_type &parentLink = p->second;
		parentLink->update(run);

		update(run);
	}

	calls_t
	calc_total_calls_from_parents(
			const ProfileNode &profile_node)
	{
		calls_t calls = 0;
		ProfileNode::profile_count_const_iterator parent_begin = profile_node.parent_profiles_begin();
		ProfileNode::profile_count_const_iterator parent_end = profile_node.parent_profiles_end();
		for (
			ProfileNode::profile_count_const_iterator parent_iter = parent_begin;
			parent_iter != parent_end;
			++parent_iter)
		{
			calls += parent_iter->get_calls();
		}

		// If there were no parents then we set the calls to one.
		return (calls > 0) ? calls : 1;
	}

	ticks_t
	calc_ticks_in_all_children(
			const ProfileNode &profile_node)
	{
		ticks_t ticks = 0;
		ProfileNode::profile_count_const_iterator child_begin = profile_node.child_profiles_begin();
		ProfileNode::profile_count_const_iterator child_end = profile_node.child_profiles_end();
		for (
			ProfileNode::profile_count_const_iterator child_iter = child_begin;
			child_iter != child_end;
			++child_iter)
		{
			ticks += child_iter->get_ticks_in_child() + child_iter->get_ticks_in_child_children();
		}
		return ticks;
	}


	//
	// This is here since ProfileNode needs to be defined before this method.
	//
	void
	ProfileRun::finished_profiling(
			ProfileRun &parent_run)
	{
		parent_run.d_children_ticks += d_self_ticks + d_children_ticks;
		d_profile_node->update(*this, *parent_run.d_profile_node);
	}

	//
	// This is here since ProfileNode needs to be defined before this method.
	//
	void
	ProfileRun::finished_profiling()
	{
		d_profile_node->update(*this);
	}


	//! Keeps track of profiles on function call stack.
	class ProfileManager
	{
	public:
		//! Return the sole instance of this class.
		static ProfileManager &instance()
		{
			return s_instance;
		}

		/**
		 * An optimisation to avoid repeated lookups of the @a profile_name
		 * string to find the @a ProfileNode each time the same segment of
		 * source code is profiled.
		 * The returned cache is subsequently passed to @a start_profile().
		 */
		void *
		get_profile_cache(
				const char *profile_name);

		/**
		 * Called when starting a profile run for 'profile_cache'.
		 * @a suspend_profile_time is the time just
		 * when profile is first started. This value is used to update
		 * the ticks count of the previous profile run. The caller
		 * is required to assign to the returned reference the value
		 * of @a get_ticks() when it returns to the code being profiled.
		*/
		ticks_t &
		start_profile(
				void *profile_cache,
				const ticks_t &suspend_profile_time);

		/**
		 * Called when stopping a profile run.
		 * @a suspend_profile_time is the time just
		 * when profile is first stopped. This value is used to update
		 * the ticks count of the current profile run. The caller
		 * is required to assign to the returned reference the value
		 * of @a get_ticks() when it returns to the code being profiled
		 * only if this method returns true.
		*/
		bool
		stop_profile(
				const ticks_t &suspend_profile_time,
				ticks_t **resume_profile_time);

		/**
		 * Prints out a report of all profiles to @a output_stream
		 * (if any profiling has been done).
		 */
		void
		report(
				std::ostream &output_stream);


	private:
		ProfileManager() { }

		static ProfileManager s_instance;

		//! Returns a ProfileNode object for 'profile_name' - creates one if necessary.
		ProfileNode &
		get_or_create_profile_node_by_name(
				const std::string& profile_name);

		//! Maps profile name to ProfileNode object.
		typedef std::map<std::string, ProfileNode> profile_node_map_type;

		profile_node_map_type d_profile_node_map;

		//! Stack of profile runs that are currently following the call stack.
		std::stack<ProfileRun> d_profile_run_stack;
	};

	ProfileManager ProfileManager::s_instance;

	ProfileNode &
	ProfileManager::get_or_create_profile_node_by_name(
			const std::string &profile_name)
	{
		// A default ProfileNode object is created for 'profile_name' if
		// that name is not in the map.
		profile_node_map_type::iterator profile_node = d_profile_node_map.find(profile_name);
		if ( profile_node == d_profile_node_map.end() )
		{
			// Create a ProfileNode for 'profile_name'.
			std::pair<profile_node_map_type::iterator,bool> p = d_profile_node_map.insert(
				profile_node_map_type::value_type(profile_name, ProfileNode(profile_name)) );
			GPlatesGlobal::Assert(p.second,
				GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
			profile_node = p.first;
		}
		return profile_node->second;
	}

	void *
	ProfileManager::get_profile_cache(
			const char *profile_name)
	{
		ProfileNode &profile_node = get_or_create_profile_node_by_name(profile_name);

		return &profile_node;
	}

	ticks_t &
	ProfileManager::start_profile(
			void *profile_cache,
			const ticks_t &suspend_profile_time)
	{
		ProfileNode &profile_node = *reinterpret_cast<ProfileNode *>(profile_cache);

		// Get information about the current ProfileRun if there is one -
		// it'll be the get_parent of the new ProfileRun that we'll push on the
		// stack below.
		if ( ! d_profile_run_stack.empty() )
		{
			ProfileRun &parentRun = d_profile_run_stack.top();

			// Stop current profile run so we can start a new one.
			parentRun.stop_profile(suspend_profile_time);
		}

		// The currently profiled object is now 'profile_node'. Push a
		// reference to it and the current clock onto the stack.
		// We add to the stack first and then set the time later so that we don't include
		// the time it takes to push onto the stack.
		d_profile_run_stack.push( ProfileRun(profile_node) );
		ProfileRun &run = d_profile_run_stack.top();  // Get reference to pushed item.

		// The caller is required to assign to the ticks_t& returned.
		return run.start_profile();
	}

	bool
	ProfileManager::stop_profile(
			const ticks_t &suspend_profile_time,
			ticks_t **resume_profile_time)
	{
		// Pop the current profile run off the stack - it should correspond
		// to 'profile_node'.
		GPlatesGlobal::Assert(!d_profile_run_stack.empty(),
			GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
		ProfileRun current_run = d_profile_run_stack.top();
		d_profile_run_stack.pop();

		// Stop the current profile run.
		current_run.stop_profile(suspend_profile_time);

		// Get the next profile run and reset its last clock.
		if ( ! d_profile_run_stack.empty() )
		{
			ProfileRun &parent_run = d_profile_run_stack.top();

			// Get the current profile run to transfer its information to the
			// ProfileNode object that it's associated with.
			current_run.finished_profiling(parent_run);

			// Caller will need to set the returned time when profiling of
			// the get_parent profile actually resumes again.
			*resume_profile_time = &parent_run.start_profile();

			return true;
		}
		else
		{
			// Get the current profile run to transfer its information to the
			// ProfileNode object that it's associated with.
			current_run.finished_profiling();

			return false;
		}
	}

	void
	ProfileManager::report(
			std::ostream &output_stream)
	{
		// If there are no recorded profiles then there is no
		// reporting to be done.
		if ( d_profile_node_map.empty() )
		{
			return;
		}

		// Sequence of all ProfileNode objects sorted on tick count.
		typedef std::vector<const ProfileNode *> sorted_profile_node_seq_type;
		sorted_profile_node_seq_type sorted_profile_nodes;

		// Copy ProfileNodes from map to vector.
		profile_node_map_type::const_iterator profile_node_map_iter;
		for (
			profile_node_map_iter = d_profile_node_map.begin();
			profile_node_map_iter != d_profile_node_map.end();
			++profile_node_map_iter)
		{
			const ProfileNode *profile_node = &profile_node_map_iter->second;
			sorted_profile_nodes.push_back(profile_node);
		}

		std::sort(
				sorted_profile_nodes.begin(),
				sorted_profile_nodes.end(),
				boost::bind(&ProfileNode::get_self_ticks, _1)
						< boost::bind(&ProfileNode::get_self_ticks, _2));

		// Get the total number of ticks spent profiling.
		const ticks_t total_ticks = std::accumulate(
				sorted_profile_nodes.begin(),
				sorted_profile_nodes.end(),
				ticks_t(0),
				boost::bind(std::plus<ticks_t>(),
						_1,
						boost::bind(&ProfileNode::get_self_ticks, _2)));
		
		const double total_seconds = convert_ticks_to_seconds(total_ticks);
		
		output_stream << std::endl;
		output_stream << "Profile Report" << std::endl;
		output_stream << "--------------" << std::endl;
		output_stream << "--------------" << std::endl << std::endl;
		
		output_stream << "Flat Profile" << std::endl;
		output_stream << "------------" << std::endl << std::endl;
		
		output_stream.setf(std::ios::fixed);

		// Print out the flat profile in order of time taken.
		sorted_profile_node_seq_type::reverse_iterator sorted_profile_node_seq_iter;
		for (
			sorted_profile_node_seq_iter = sorted_profile_nodes.rbegin();
			sorted_profile_node_seq_iter != sorted_profile_nodes.rend();
			++sorted_profile_node_seq_iter)
		{
			const ProfileNode *profile_node = *sorted_profile_node_seq_iter;
			const double self_seconds = convert_ticks_to_seconds(profile_node->get_self_ticks());
			const double percent = self_seconds / total_seconds * 100;
			const calls_t calls = calc_total_calls_from_parents(*profile_node);
			const double seconds_per_call = (calls > 0) ? (self_seconds / calls) : 0;

			output_stream
				<< std::setprecision(2) << percent
				<< "% \""
				<< profile_node->get_name().c_str()
				<< "\" : "
				<< calls
				<< " get_calls : "
				<< std::setprecision(6) << seconds_per_call
				<< " seconds/call." << std::endl;
		}
		output_stream << std::endl;

		output_stream << "--------------" << std::endl;
		output_stream << "--------------" << std::endl;
	}

	//
	// Timing functions.
	//

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

	inline
	ticks_t
	get_ticks()
	{
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return counter.QuadPart;
	}

	double
	get_seconds_per_tick()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return 1.0 / frequency.QuadPart;
	}

#else // if defined(_WIN32) ...

#include <sys/time.h>

	inline
	ticks_t
	get_ticks()
	{
		timeval time;
		gettimeofday(&time, NULL);

		return time.tv_sec * 1000000 + time.tv_usec;
	}

	double
	get_seconds_per_tick()
	{
		return 1e-6;
	}

#endif // if defined(_WIN32) ... else ...

	/**
	 * Converts ticks to seconds.
	 */
	double
	convert_ticks_to_seconds(
			ticks_t ticks)
	{
		static double seconds_per_tick = get_seconds_per_tick();
		return ticks * seconds_per_tick;
	}
}

namespace GPlatesUtils
{
	void *
	profile_get_cache(
			const char *profile_name)
	{
		return ProfileManager::instance().get_profile_cache(profile_name);
	}

	void
	profile_begin(
			void *profile_cache)
	{
		// The first and last things we do are call 'get_ticks()'
		// that way we can take as long as we like in between and it doesn't get counted.
		ticks_t start_ticks = get_ticks();

		ticks_t &end_ticks = ProfileManager::instance().start_profile(profile_cache, start_ticks);

		end_ticks = get_ticks();
	}

	void
	profile_end()
	{
		// The first and last things we do are call 'get_ticks()'
		// that way we can take as long as we like in between and it doesn't get counted.
		ticks_t start_ticks = get_ticks();

		ticks_t *end_ticks;
		if (ProfileManager::instance().stop_profile(start_ticks, &end_ticks))
		{
			*end_ticks = get_ticks();
		}
	}

	void
	profile_report_to_ostream(
			std::ostream &output_stream)
	{
		ProfileManager::instance().report(output_stream);
	}

	void
	profile_report_to_file(
			const std::string &filename)
	{
		std::ofstream output_file(filename.c_str());
		if (!output_file)
		{
			std::cerr << "Failed to open file '" << filename << "' for writing." << std::endl;
			return;
		}

		profile_report_to_ostream(output_file);
	}
}
