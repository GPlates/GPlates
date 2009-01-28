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
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/operators.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/bind.hpp>
#include <stack>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
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

		ticks_t
		get_self_ticks() const
		{
			return d_self_ticks;
		}

		ticks_t
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
				ProfileNode *parent,
				ProfileNode *child);

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

		ticks_t
		get_ticks_in_child() const
		{
			return d_ticks_in_child;
		}

		ticks_t
		get_ticks_in_childs_children() const
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
			ProfileNode *parent,
			ProfileNode *child)
	{
		ProfileLink *link_mem = s_profile_link_pool.malloc();
		if (link_mem == NULL)
		{
			throw std::bad_alloc();
		}

		return pointer_type(
				new (link_mem) ProfileLink(parent, child),
				boost::bind(
						&profile_link_pool_type::destroy,
						boost::ref(s_profile_link_pool),
						_1));
	}

	inline
	ticks_t
	get_ticks(
			const ProfileLink *profile_link)
	{
		return profile_link->get_ticks_in_child() + profile_link->get_ticks_in_childs_children();
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

			const ProfileLink *
			get() const
			{
				return d_profile_link_map_const_iter->second.get();
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
			d_self_ticks(0),
			d_most_recent_parent(NULL)
		{ }

		/**
		 * Updates this profile node with profile counts in @a run and
		 * updates link to parent node.
		 */
		void
		update(
				const ProfileRun &run,
				ProfileNode &parent);

		const std::string &
		get_name() const
		{
			return d_name;
		}

		//! The number of ticks counted - not including children.
		ticks_t
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

		profile_count_const_iterator
		child_profiles_begin() const
		{
			return profile_count_const_iterator(d_child_profiles, d_child_profiles.begin());
		}

		profile_count_const_iterator
		child_profiles_end() const
		{
			return profile_count_const_iterator(d_child_profiles, d_child_profiles.end());
		}

	private:
		std::string d_name;
		ticks_t d_self_ticks;

		profile_link_map_type d_parent_profiles;
		profile_link_map_type d_child_profiles;

		//! Used for speed optimisation purposes to try and avoid searching @a d_parent_profiles.
		ProfileNode *d_most_recent_parent;
		//! Used for speed optimisation purposes to try and avoid searching @a d_parent_profiles.
		ProfileLink *d_most_recent_parent_link;

		//! Returns reference to parent link corresponding to @a parent_node.
		ProfileLink *
		get_parent_link(
				ProfileNode *parent_node);

		/**
		 * Creates a ProfileLink and connects it between 'parent' and 'child'.
		 * There must not already exist such a link.
		 */
		static
		void
		create_call_graph_link(
				ProfileNode *parent,
				ProfileNode *child)
		{
			ProfileLink::pointer_type link = ProfileLink::create_profile_link(parent, child);
			child->d_parent_profiles[parent] = link;
			parent->d_child_profiles[child] = link;
		}
	};

	inline
	ProfileLink *
	ProfileNode::get_parent_link(
			ProfileNode *parent_node)
	{
		// An optimisation is to keep track of the most recent parent as that's the most
		// likely scenario and avoids having to search our parent mappings. This is
		// effective when 'this' profile node is in a tight loop that calls very many times
		// because its parent will always be the same while in that loop. And these tight
		// loops are exactly the place we want to optimise for speed so that our profiling
		// code doesn't slow down program running time too much over non-profiled running time.
		if (d_most_recent_parent != parent_node)
		{
			// We haven't got a cached version of the parent link (or it might not even
			// exist yet) so search our mappings.
			profile_link_map_type::iterator p = d_parent_profiles.find(parent_node);
			if ( p == d_parent_profiles.end() )
			{
				// Create a ProfileLink between parent_node and us.
				create_call_graph_link(parent_node, this);

				// The previous call should've added a ProfileLink to our parent profiles.
				p = d_parent_profiles.find(parent_node);
				GPlatesGlobal::Assert(p != d_parent_profiles.end(),
						GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
			}

			d_most_recent_parent_link = p->second.get();
			d_most_recent_parent = parent_node;
		}

		return d_most_recent_parent_link;
	}

	void
	ProfileNode::update(
		const ProfileRun &run,
		ProfileNode &parent_node)
	{
		// Update how much time gets allocated to us.
		d_self_ticks += run.get_self_ticks();

		// Get the call graph ProfileLink to parent_node.
		// This will create one if it doesn't exist.
		ProfileLink *parent_link = get_parent_link(&parent_node);

		// Get this parent_link to update itself from info in 'run'.
		parent_link->update(run);
	}

	calls_t
	calc_total_calls_from_parents(
			const ProfileNode *profile_node)
	{
		calls_t calls = 0;
		ProfileNode::profile_count_const_iterator parent_begin = profile_node->parent_profiles_begin();
		ProfileNode::profile_count_const_iterator parent_end = profile_node->parent_profiles_end();
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
			const ProfileNode *profile_node)
	{
		ticks_t ticks = 0;
		ProfileNode::profile_count_const_iterator child_begin = profile_node->child_profiles_begin();
		ProfileNode::profile_count_const_iterator child_end = profile_node->child_profiles_end();
		for (
			ProfileNode::profile_count_const_iterator child_iter = child_begin;
			child_iter != child_end;
			++child_iter)
		{
			ticks += child_iter->get_ticks_in_child() + child_iter->get_ticks_in_childs_children();
		}
		return ticks;
	}

	inline
	ticks_t
	calc_ticks_in_profile_node_and_all_its_children(
			const ProfileNode *profile_node)
	{
		return profile_node->get_self_ticks() + calc_ticks_in_all_children(profile_node);
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


	//! The call graph of profile nodes.
	class ProfileGraph
	{
	public:
		//! Sequence of @a ProfileNode objects.
		typedef std::vector<const ProfileNode *> profile_node_seq_type;

		/**
		 * Returns a ProfileNode object for 'profile_name' - creates one if necessary.
		 */
		ProfileNode &
		get_or_create_profile_node_by_name(
				const std::string &profile_name);

		/**
		 * Returns the sequence of all @a ProfileNode objects in the call graph.
		 */
		profile_node_seq_type
		get_call_graph() const;

	private:
		//! Maps profile name to ProfileNode object.
		typedef std::map<std::string, ProfileNode> profile_node_map_type;

		profile_node_map_type d_profile_node_map;
	};

	ProfileNode &
	ProfileGraph::get_or_create_profile_node_by_name(
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

	ProfileGraph::profile_node_seq_type
	ProfileGraph::get_call_graph() const
	{
		// Sequence of all ProfileNode objects.
		profile_node_seq_type profile_nodes;

		// If there are no recorded profiles then there is no
		// reporting to be done.
		if ( !d_profile_node_map.empty() )
		{
			// Copy ProfileNodes from map to vector.
			profile_node_map_type::const_iterator profile_node_map_iter;
			for (
				profile_node_map_iter = d_profile_node_map.begin();
				profile_node_map_iter != d_profile_node_map.end();
				++profile_node_map_iter)
			{
				const ProfileNode *profile_node = &profile_node_map_iter->second;
				profile_nodes.push_back(profile_node);
			}
		}

		return profile_nodes;
	}


	//! Keeps track of profiles on function call stack.
	class ProfileManager
	{
	public:
		//! Return the sole instance of this class.
		static ProfileManager &instance()
		{
			static boost::scoped_ptr<ProfileManager> s_instance(new ProfileManager());
			return *s_instance;
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
		 * the ticks count of the current profile run.
		 * The caller is required to assign to the returned reference
		 * the value of @a get_ticks().
		*/
		ticks_t &
		stop_profile(
				const ticks_t &suspend_profile_time);

		/**
		 * Returns true if all profile runs have finished.
		 */
		bool
		have_all_profile_runs_finished() const
		{
			// If only the root profile run exists then it means all
			// user added profiles have finished.
			return d_profile_run_stack.size() == 1;
		}

		/**
		 * Returns generated profile graph.
		 */
		const ProfileGraph &
		get_profile_graph() const
		{
			return d_profile_graph;
		}

	private:
		ProfileManager() :
			d_root_profile_node("<root>")
		{
			// The root profile run will always exist on the stack.
			// It is used only to test for mismatching profile begin/end calls.
			d_profile_run_stack.push( ProfileRun(d_root_profile_node) );
		}

		/**
		 * Root profile node.
		 * The only node that doesn't live in the @a ProfileGraph.
		 * Used mainly as an aid to testing for mismatching profile begin/end calls.
		 */
		ProfileNode d_root_profile_node;

		//! Contains profile call graph.
		ProfileGraph d_profile_graph;

		//! Stack of profile runs that are currently following the call stack.
		std::stack<ProfileRun> d_profile_run_stack;
	};

	void *
	ProfileManager::get_profile_cache(
			const char *profile_name)
	{
		ProfileNode &profile_node = d_profile_graph.get_or_create_profile_node_by_name(profile_name);

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

	ticks_t &
	ProfileManager::stop_profile(
			const ticks_t &suspend_profile_time)
	{
		// Pop the current profile run off the stack - it should correspond
		// to 'profile_node'.
		GPlatesGlobal::Assert(!d_profile_run_stack.empty(),
			GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
		ProfileRun current_run = d_profile_run_stack.top();
		d_profile_run_stack.pop();

		// The stack should always have the root profile run that never
		// gets popped off the stack. This is how we check for mismatched
		// profile begin/end calls. Here we've got too many profile end calls.
		if (d_profile_run_stack.empty())
		{
			std::cerr << "Profiler encountered too many PROFILE_END calls - "
					"number of PROFILE_BEGIN and PROFILE_END calls must match." << std::endl;
			throw GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__);
		}

		// Stop the current profile run.
		current_run.stop_profile(suspend_profile_time);

		// Get the next profile run and reset its last clock.
		ProfileRun &parent_run = d_profile_run_stack.top();

		// Get the current profile run to transfer its information to the
		// ProfileNode object that it's associated with.
		current_run.finished_profiling(parent_run);

		// Caller will need to set the returned time when profiling of
		// the get_parent profile actually resumes again.
		return parent_run.start_profile();
	}

	//
	// Printing of profiling statistics
	//

	void
	print_accurate_time(double seconds, std::ostream &output_stream, int field_width)
	{
		// The most accurate timer is QueryPerformanceCounter and its overhead is
		// about 0.5usec so no point printing more accurate than this.
		// Also the linux timers are at best 1usec accurate so this figure seems
		// a reasonable one.
		const double accuracy = 1e-6;
		seconds = accuracy * static_cast<boost::uint64_t>(seconds / accuracy  + 0.5);

		const char *time_suffix;
		double time;

		if (seconds >= 1)
		{
			time = seconds;
			time_suffix = "s";
			field_width -= 1;
		}
		else if (seconds >= 1e-3)
		{
			time = 1e+3 * seconds;
			time_suffix = "ms";
			field_width -= 2;
		}
		else
		{
			time = 1e+6 * seconds;
			time_suffix = "us";
			field_width -= 2;
		}

		output_stream
			<< std::resetiosflags(std::ios::fixed)
			<< std::setprecision(3)
			<< std::setw(field_width)
			<< std::right
			<< time
			<< time_suffix;
	}

	void
	report_flat_profile(
			std::ostream &output_stream,
			const ProfileGraph &profile_graph,
			const ticks_t total_ticks)
	{
		// Get sequence of ProfileNode objects representing the call graph.
		ProfileGraph::profile_node_seq_type profile_node_seq = profile_graph.get_call_graph();

		// Copy sequence of ProfileNode objects.
		ProfileGraph::profile_node_seq_type sorted_profile_node_seq(
				profile_node_seq.begin(), profile_node_seq.end());

		// Sort sequence of ProfileNode objects according to time spent
		// in each profile only (ie, not counting time spent in their children).
		std::sort(
				sorted_profile_node_seq.begin(),
				sorted_profile_node_seq.end(),
				boost::bind(&ProfileNode::get_self_ticks, _1)
						> boost::bind(&ProfileNode::get_self_ticks, _2));
	
		output_stream << "Flat Profile" << std::endl;
		output_stream << "------------" << std::endl;
		output_stream << std::endl;
		
		// Print header rows.
		output_stream << "  %    cumulative    self               self        total       " << std::endl;
		output_stream << " time    seconds    seconds   calls   time/call   time/call  name" << std::endl;

		// Cumulative time.
		ticks_t cumulative_ticks = 0;

		// Print out the flat profile in order of time taken.
		ProfileGraph::profile_node_seq_type::iterator sorted_profile_node_seq_iter;
		for (
			sorted_profile_node_seq_iter = sorted_profile_node_seq.begin();
			sorted_profile_node_seq_iter != sorted_profile_node_seq.end();
			++sorted_profile_node_seq_iter)
		{
			const ProfileNode *profile_node = *sorted_profile_node_seq_iter;
			const ticks_t self_ticks = profile_node->get_self_ticks();
			const double self_seconds = convert_ticks_to_seconds(self_ticks);
			const ticks_t children_ticks = calc_ticks_in_all_children(profile_node);
			const double children_seconds = convert_ticks_to_seconds(children_ticks);
			// If self_ticks == total_ticks then we want to  print 100% and
			// not 99.9999997% so use exact arithmetic until final divide by 100.0.
			const double percent = (self_ticks * 100 * 100 / total_ticks) / 100.0;
			const calls_t calls = calc_total_calls_from_parents(profile_node);
			const double self_seconds_per_call = (calls > 0) ? (self_seconds / calls) : 0;
			const double self_plus_children_seconds_per_call =
					(calls > 0) ? ((self_seconds + children_seconds) / calls) : 0;
			cumulative_ticks += self_ticks;
			const double cumulative_seconds = convert_ticks_to_seconds(cumulative_ticks);

			output_stream
				<< std::fixed << std::setprecision(2) << std::setw(6) << std::right
				<< percent
				<< std::fixed << std::setprecision(2) << std::setw(10) << std::right
				<< cumulative_seconds
				<< std::fixed << std::setprecision(3) << std::setw(10) << std::right
				<< self_seconds
				<< std::setw(9) << std::right
				<< calls;

			print_accurate_time(self_seconds_per_call, output_stream, 12/*field_width*/);
			print_accurate_time(self_plus_children_seconds_per_call, output_stream, 12/*field_width*/);

			output_stream
				<< std::setw(0) << std::left
				<< "  "
				<< profile_node->get_name().c_str()
				<< std::endl;
		}
		output_stream << std::endl;

		output_stream << "--------------" << std::endl;
		output_stream << "--------------" << std::endl;
	}

	void
	report_call_graph_profile(
			std::ostream &output_stream,
			const ProfileGraph &profile_graph,
			const ticks_t total_ticks)
	{
		// Get sequence of ProfileNode objects representing the call graph.
		ProfileGraph::profile_node_seq_type profile_node_seq = profile_graph.get_call_graph();

		// Copy sequence of ProfileNode objects.
		ProfileGraph::profile_node_seq_type sorted_profile_node_seq(
				profile_node_seq.begin(), profile_node_seq.end());

		// Sort sequence of ProfileNode objects according to time spent
		// in each profile AND time spent in their children.
		std::sort(
				sorted_profile_node_seq.begin(),
				sorted_profile_node_seq.end(),
				boost::bind(&calc_ticks_in_profile_node_and_all_its_children, _1)
					> boost::bind(&calc_ticks_in_profile_node_and_all_its_children, _2));

		output_stream << "Call Graph Profile" << std::endl;
		output_stream << "------------------" << std::endl;
		output_stream << std::endl;
		
		// Print header rows.
		output_stream << "index % time     self   children      called      name" << std::endl;
		output_stream << "               seconds   seconds                      " << std::endl;

		// Print out the call graph profile in order of time taken.
		ProfileGraph::profile_node_seq_type::iterator sorted_profile_node_seq_iter;
		for (
			sorted_profile_node_seq_iter = sorted_profile_node_seq.begin();
			sorted_profile_node_seq_iter != sorted_profile_node_seq.end();
			++sorted_profile_node_seq_iter)
		{
			const ProfileNode *profile_node = *sorted_profile_node_seq_iter;

			// Index into sorted profile node sequence.
			const int node_index = std::distance(
					sorted_profile_node_seq.begin(), sorted_profile_node_seq_iter);


			//
			// Print out parents of current node
			//


			// Sort the parent links according to time spent time spent passing through each link.
			typedef std::vector<const ProfileLink *> parent_profile_link_seq_type;
			parent_profile_link_seq_type sorted_parent_links;

			// Copy parent profile links into vector for sorting.
			ProfileNode::profile_count_const_iterator parent_begin = profile_node->parent_profiles_begin();
			ProfileNode::profile_count_const_iterator parent_end = profile_node->parent_profiles_end();
			for (
				ProfileNode::profile_count_const_iterator parent_iter = parent_begin;
				parent_iter != parent_end;
				++parent_iter)
			{
				sorted_parent_links.push_back(parent_iter.get());
			}

			// Sort in ascending order.
			std::sort(
					sorted_parent_links.begin(),
					sorted_parent_links.end(),
					boost::bind(&get_ticks, _1)
						< boost::bind(&get_ticks, _2));

			// Iterate through the sorted sequence of parent links and print them out.
			parent_profile_link_seq_type::const_iterator sorted_parent_iter;
			for (
				sorted_parent_iter = sorted_parent_links.begin();
				sorted_parent_iter != sorted_parent_links.end();
				++sorted_parent_iter)
			{
				const ProfileLink *parent_link = *sorted_parent_iter;

				// Find parent node in the sorted profile node sequence and if it exists
				// in that sequence (note: root profile node does not) then its index
				// is its distance from beginning of sequence.
				ProfileGraph::profile_node_seq_type::iterator parent_index_iter = std::find(
						sorted_profile_node_seq.begin(),
						sorted_profile_node_seq.end(),
						parent_link->get_parent());
				const int parent_node_index = (parent_index_iter != sorted_profile_node_seq.end())
						? std::distance(sorted_profile_node_seq.begin(), parent_index_iter)
						: -1;

				output_stream
					<< std::fixed << std::setprecision(3) << std::setw(22) << std::right
					<< convert_ticks_to_seconds(parent_link->get_ticks_in_child())
					<< std::fixed << std::setprecision(3) << std::setw(10) << std::right
					<< convert_ticks_to_seconds(parent_link->get_ticks_in_childs_children())
					<< std::setw(9) << std::right
					<< parent_link->get_calls()
					<< std::setw(0) << std::left
					<< '/'
					<< std::setw(12) << std::left
					<< calc_total_calls_from_parents(profile_node)
					<< std::setw(0) << std::left
					<< parent_link->get_parent()->get_name().c_str()
					<< std::setw(0) << std::left
					<< " [" << parent_node_index + 1 << ']'
					<< std::endl;
			}


			//
			// Print out current node
			//


			const ticks_t self_ticks = profile_node->get_self_ticks();
			const ticks_t children_ticks = calc_ticks_in_all_children(profile_node);
			const double self_seconds = convert_ticks_to_seconds(self_ticks);
			const double children_seconds = convert_ticks_to_seconds(children_ticks);
			// If (self_ticks + children_ticks) == total_ticks then we want to  print 100% and
			// not 99.9999997% so use exact arithmetic until final divide by 100.0.
			const double percent = ((self_ticks + children_ticks) * 100 * 100 / total_ticks) / 100.0;
			const calls_t calls = calc_total_calls_from_parents(profile_node);

			std::ostringstream index_field;
			index_field << '[' << node_index + 1 << ']';

			output_stream
				<< std::setw(6) << std::left
				<< index_field.str().c_str()
				<< std::fixed << std::setprecision(1) << std::setw(6) << std::right
				<< percent
				<< std::fixed << std::setprecision(3) << std::setw(10) << std::right
				<< self_seconds
				<< std::fixed << std::setprecision(3) << std::setw(10) << std::right
				<< children_seconds
				<< std::setw(10) << std::right
				<< calls
				<< std::setw(0) << std::left
				<< "        "
				<< profile_node->get_name().c_str()
				<< std::endl;


			//
			// Print out children of current node
			//


			// Sort the child links according to time spent time spent passing through each link.
			typedef std::vector<const ProfileLink *> child_profile_link_seq_type;
			child_profile_link_seq_type sorted_child_links;

			// Copy child profile links into vector for sorting.
			ProfileNode::profile_count_const_iterator child_begin = profile_node->child_profiles_begin();
			ProfileNode::profile_count_const_iterator child_end = profile_node->child_profiles_end();
			for (
				ProfileNode::profile_count_const_iterator child_iter = child_begin;
				child_iter != child_end;
				++child_iter)
			{
				sorted_child_links.push_back(child_iter.get());
			}

			// Sort in descending order.
			std::sort(
					sorted_child_links.begin(),
					sorted_child_links.end(),
					boost::bind(&get_ticks, _1)
						> boost::bind(&get_ticks, _2));

			// Iterate through the sorted sequence of child links and print them out.
			child_profile_link_seq_type::const_iterator sorted_child_iter;
			for (
				sorted_child_iter = sorted_child_links.begin();
				sorted_child_iter != sorted_child_links.end();
				++sorted_child_iter)
			{
				const ProfileLink *child_link = *sorted_child_iter;

				// Find child node in the sorted profile node sequence and if it exists
				// in that sequence (which it should since root node will not be child
				// of any nodes) then its index is its distance from beginning of sequence.
				ProfileGraph::profile_node_seq_type::iterator child_index_iter = std::find(
						sorted_profile_node_seq.begin(),
						sorted_profile_node_seq.end(),
						child_link->get_child());
				const int child_node_index = (child_index_iter != sorted_profile_node_seq.end())
						? std::distance(sorted_profile_node_seq.begin(), child_index_iter)
						: -1;

				output_stream
					<< std::fixed << std::setprecision(3) << std::setw(22) << std::right
					<< convert_ticks_to_seconds(child_link->get_ticks_in_child())
					<< std::fixed << std::setprecision(3) << std::setw(10) << std::right
					<< convert_ticks_to_seconds(child_link->get_ticks_in_childs_children())
					<< std::setw(9) << std::right
					<< child_link->get_calls()
					<< std::setw(0) << std::left
					<< '/'
					<< std::setw(12) << std::left
					<< calc_total_calls_from_parents(child_link->get_child())
					<< std::setw(0) << std::left
					<< child_link->get_child()->get_name().c_str()
					<< std::setw(0) << std::left
					<< " [" << child_node_index + 1 << ']'
					<< std::endl;
			}

			output_stream << "------------------" << std::endl;
		}
		output_stream << std::endl;

		output_stream << "------------------" << std::endl;
		output_stream << "------------------" << std::endl;
	}

	/**
	 * Prints out a report of this call graph to @a output_stream
	 * (if any profiling has been done).
	 */
	void
	report(
			const ProfileGraph &profile_graph,
			std::ostream &output_stream)
	{
		// Get sequence of ProfileNode objects representing the call graph.
		ProfileGraph::profile_node_seq_type profile_nodes = profile_graph.get_call_graph();

		// Get the total number of ticks spent profiling.
		const ticks_t total_ticks = std::accumulate(
				profile_nodes.begin(),
				profile_nodes.end(),
				ticks_t(0),
				boost::bind(std::plus<ticks_t>(),
						_1,
						boost::bind(&ProfileNode::get_self_ticks, _2)));
		
		const double total_seconds = convert_ticks_to_seconds(total_ticks);
		
		output_stream << std::endl;
		output_stream << "Profile Report" << std::endl;
		output_stream << "--------------" << std::endl;
		output_stream << "--------------" << std::endl;
		output_stream << std::endl;

		output_stream
			<< "Total profiled time: "
			<< std::fixed << std::setprecision(2) << std::setw(0) << std::left
			<< total_seconds
			<< " seconds"
			<< std::endl;
		output_stream << std::endl;

		report_flat_profile(output_stream, profile_graph, total_ticks);

		output_stream << std::endl;
		output_stream << std::endl;

		output_stream
			<< "Total profiled time: "
			<< std::fixed << std::setprecision(2) << std::setw(0) << std::left
			<< total_seconds
			<< " seconds"
			<< std::endl;
		output_stream << std::endl;

		report_call_graph_profile(output_stream, profile_graph, total_ticks);
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
		ticks_t suspend_profiling_ticks = get_ticks();

		ticks_t &resume_profiling_ticks = ProfileManager::instance().start_profile(
				profile_cache, suspend_profiling_ticks);

		// We could call 'get_ticks()' at beginning of this function and then again at
		// the end to remove the time spent in the profiling code itself.
		// However 90% of the time spent in profiling code is due to API calls like
		// QueryPerformanceCount(), which lives in 'get_ticks()', so having two calls
		// to 'get_ticks()' hardly increases our profiling accuracy and would just
		// make the profiling code twice as slow.
		resume_profiling_ticks = suspend_profiling_ticks;
	}

	void
	profile_end()
	{
		ticks_t suspend_profiling_ticks = get_ticks();

		ticks_t &resume_profiling_ticks = ProfileManager::instance().stop_profile(suspend_profiling_ticks);
		
		// We could call 'get_ticks()' at beginning of this function and then again at
		// the end to remove the time spent in the profiling code itself.
		// However 90% of the time spent in profiling code is due to API calls like
		// QueryPerformanceCount(), which lives in 'get_ticks()', so having two calls
		// to 'get_ticks()' hardly increases our profiling accuracy and would just
		// make the profiling code twice as slow.
		resume_profiling_ticks = suspend_profiling_ticks;
	}

	void
	profile_report_to_ostream(
			std::ostream &output_stream)
	{
		if (!ProfileManager::instance().have_all_profile_runs_finished())
		{
			std::cerr << "Profiler encountered too many PROFILE_BEGIN calls - "
					"number of PROFILE_BEGIN and PROFILE_END calls must match." << std::endl;
			std::cerr << "...Or PROFILE_REPORT called when profiles are still running." << std::endl;
			throw GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__);
		}

		const ProfileGraph &profile_graph = ProfileManager::instance().get_profile_graph();

		report(profile_graph, output_stream);
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
