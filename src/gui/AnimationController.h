/* $Id$ */

/**
 * \file 
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
 
#ifndef GPLATES_GUI_ANIMATIONCONTROLLER_H
#define GPLATES_GUI_ANIMATIONCONTROLLER_H

#include <QTimer>

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	/**
	 * The behind-the-scenes logic for the AnimateDialog and AnimateControlWidget.
	 *
	 * This class probably belongs to the Presentation tier, due to it relying on
	 * QObject signals and slots (requires 'moc' processing, but not an actual
	 * qt widget).
	 */
	class AnimationController : 
			public QObject
	{
		Q_OBJECT
		
	public:
		/**
		 * Typedef for frame index numbers used by @a set_view_frame() etc.
		 */
		typedef long frame_index_type;
	
		explicit
		AnimationController(
				GPlatesAppLogic::ApplicationState &application_state);

		virtual
		~AnimationController()
		{  }

		/**
		 * Returns the current reconstruction time the View is looking at.
		 * Naturally, you could go straight to the view for this, but
		 * accessing it from here may be more convenient - and also reduces
		 * dependencies on ViewState, which is A Good Thing.
		 */
		const double &
		view_time() const;

		/**
		 * The time that the animation should begin at. This may be before
		 * or after the @a end_time() - the increment will be adjusted
		 * automatically.
		 *
		 * The desired start time may be set with @a set_set_time().
		 */
		const double &
		start_time() const;

		/**
		 * The time that the animation should end at. This may be before
		 * or after the @a end_time() - the increment will be adjusted
		 * automatically.
		 *
		 * The desired end time may be set with @a set_end_time().
		 *
		 * Note that if @a should_finish_exactly_on_end_time() is false,
		 * then the actual ending frame may be earlier than the desired
		 * ending frame - see the @a ending_frame_time() accessor.
		 */
		const double &
		end_time() const;

		/**
		 * Returns the user-friendly 'increment' value,
		 * which will always be a positive number.
		 *
		 * See also @a set_time_increment().
		 */
		double
		time_increment() const;

		/**
		 * Returns the actual 'increment' value which needs to be applied
		 * to move from @a start_time() to @a end_time(). This may be
		 * a positive or negative number - don't show the users this one,
		 * it would blow their minds.
		 */
		double
		raw_time_increment() const;
		
		bool
		is_playing() const;

		const double &
		frames_per_second() const;

		/**
		 * Returns the number of frames between @a start_time() and @a end_time().
		 * This assumes we start at the beginning, and end at the end, taking into
		 * account if we @a should_finish_exactly_on_end_time().
		 */
		frame_index_type
		duration_in_frames() const;

		/**
		 * Returns the distance between @a start_time() and whatever time we would
		 * finish on if we counted @a duration_in_frame from the start.
		 * Always a non-negative number.
		 */
		double
		duration_in_ma() const;

		/**
		 * Returns the time that the first frame of animation will use.
		 * This should always be identical to @a start_time().
		 */
		double
		starting_frame_time() const;

		/**
		 * Returns the time that the last frame of animation will use.
		 * This @em may be different to @a end_time().
		 *
		 * Specifically, if the desired range supplied by the user is not
		 * an integer multiple of the increment, there will be a short
		 * frame left over - whether this frame gets played or not is up
		 * to the @a should_finish_exactly_on_end_time() setting.
		 */
		double
		ending_frame_time() const;
		
		/**
		 * Given the currently-configured range and increment, plus a target
		 * frame number, calculates what reconstruction time will correspond
		 * to the given @a frame.
		 *
		 * if we @a should_finish_exactly_on_end_time() and the animation duration
		 * does not divide cleanly by the increment, the last frame will be the
		 * @a end_time(); otherwise, the last frame will be whatever multiple of
		 * the increment would be closest to the end time but still fit inside the
		 * animation range.
		 */
		double
		calculate_time_for_frame(
				GPlatesGui::AnimationController::frame_index_type frame) const;

		bool
		should_finish_exactly_on_end_time() const;
		
		bool
		should_loop() const;

		bool
		should_adjust_bounds_to_contain_current_time() const;

		static
		inline
		double
		min_reconstruction_time()
		{
			// This value denotes the present-day.
			return 0.0;
		}

		static
		inline
		double
		max_reconstruction_time()
		{
			// This value denotes a time 10000 million years ago.
			return 10000.0;
		}

		static
		bool
		is_valid_reconstruction_time(
				const double &time);

	public slots:

		/**
		 * Initiates the animation. If the animation is already playing,
		 * this will do nothing. If the animation is unplayable (for
		 * instance, a total time range smaller than the increment), this
		 * will do nothing.
		 *
		 * If the animation is already at the end and the 'loop' option
		 * is set, the animation will be rewound and played from the
		 * beginning.
		 */
		void
		play();
		
		/**
		 * Ceases animation. The current view time will be left as-is,
		 * not reset to the beginning.
		 */
		void
		pause();

		/**
		 * Convenience function to call play() or pause() depending on bool.
		 * Useful if you need to connect to a signal that offers the same.
		 */
		void
		set_play_or_pause(
				bool lets_play);

		/**
		 * Increments or decrements the view time so as to progress
		 * forwards through the animation by one @a time_increment().
		 */
		void
		step_forward();

		/**
		 * Increments or decrements the view time so as to progress
		 * backwards through the animation by one @a time_increment().
		 */
		void
		step_back();

		/**
		 * Moves the view time to match the animation's start time.
		 */
		void
		seek_beginning();
		
		/**
		 * Moves the view time to match the animation's end time.
		 */
		void
		seek_end();
		
		/**
		 * Modifies the view time as requested by a dialog's widget such as
		 * a slider or part of the animation process and ensures signals are
		 * emitted to the Qt dialogs and widgets accordingly.
		 */
		void
		set_view_time(
				const double new_time);

		/**
		 * Modifies the view time to correspond to the given frame of animation;
		 * frame 0 is the same as @a start_time(), and subsequent frame numbers
		 * are incremented to approach @a end_time().
		 *
		 * if we @a should_finish_exactly_on_end_time() and the animation duration
		 * does not divide cleanly by the increment, the last frame will set the
		 * view time to @a end_time(); otherwise, the last frame will set the view
		 * time to whatever multiple of the increment would be closest to the end
		 * time but still fit inside the animation range.
		 */
		void
		set_view_frame(
				GPlatesGui::AnimationController::frame_index_type frame);

		void
		set_start_time(
				const double new_time);

		void
		set_end_time(
				const double new_time);
		
		/**
		 * Sets the geological time increment between frames.
		 * This sets the user-friendly version of the increment, which will
		 * always be a positive number. d_time_increment is set to a positive
		 * or negative number depending on the start and end range.
		 */
		void
		set_time_increment(
				const double new_abs_increment);

		void
		set_frames_per_second(
				const double fps);
		
		void
		set_should_finish_exactly_on_end_time(
				bool finish_exactly);
		
		void
		set_should_loop(
				bool loop);

		void
		set_should_adjust_bounds_to_contain_current_time(
				bool adjust_bounds);


		// FIXME: Should this really be public and dialog-called, or should it be
		// private and inscrutable?
		/**
		 * Modify the current time, if necessary, to ensure that it lies within the
		 * [closed, closed] range of the boundary times.
		 */
		void
		ensure_current_time_lies_within_bounds();

		/**
		 * Modify the boundary times, if necessary, to ensure that they contain the current
		 * time.
		 */
		void
		ensure_bounds_contain_current_time();
		
		void
		swap_start_and_end_times();

	signals:
	
		void
		view_time_changed(
				double new_time);

		void
		start_time_changed(
				double new_time);

		void
		end_time_changed(
				double new_time);

		void
		time_increment_changed(
				double new_increment);

		void
		frames_per_second_changed(
				double fps);
		
		void
		finish_exactly_on_end_time_changed(
				bool finish_exactly_on_end_time);
		
		void
		should_loop_changed(
				bool should_loop);

		void
		should_adjust_bounds_to_contain_current_time_changed(
				bool adjust_bounds);

		void
		animation_started();

		void
		animation_paused();

		/**
		 * Convenience signal which is emitted at the same time that
		 * @a animation_started() and @a animation_paused() are,
		 * to aid signal/slot connections that would ideally like
		 * a bool.
		 */
		void
		animation_state_changed(
				bool is_playing);

	private slots:

		/**
		 * Triggered whenever the internal QTimer ticks.
		 */
		void
		react_animation_playback_step();

		/**
		 * Triggered whenever the view time changes, either by our animation
		 * or by the user from the time-control buttons. This is used to
		 * check the current time against the animation bounds.
		 */
		void
		react_view_time_changed(
				GPlatesAppLogic::ApplicationState &application_state);

	private:
		/**
		 * This performs the reconstructions and is used to query and modify the current
		 * reconstruction time.
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;

		/**
		 * This QTimer instance triggers the frame updates during animation playback.
		 */
		QTimer d_timer;
		
		/**
		 * This is the starting time of the animation.
		 */
		double d_start_time;

		/**
		 * This is the ending time of the animation. Note that the animation may not
		 * stop exactly on the end time if the "Finish animation exactly at end time"
		 * option is not enabled.
		 */
		double d_end_time;

		/**
		 * This is the increment applied to the current time in successive frames of the
		 * animation.
		 *
		 * This value is either greater than zero or less than zero.
		 *
		 * The user specifies the absolute value of this time increment in the "time
		 * increment" widget in the AnimateDialog.  The value in the "time increment"
		 * widget is constrained to be greater than zero. The @a recalculate_increment
		 * function examines the value in the "time increment" dialog, and determines
		 * whether the value of this datum must be greater than zero or less than zero
		 * in order to successively increment the current-time from the start-time to
		 * the end-time.
		 */
		double d_time_increment;
		
		/**
		 * This is the number of frames to display per second. This value is used to
		 * calculate the number of milliseconds of delay between each animation step.
		 */
		double d_frames_per_second;

		/**
		 * This option controls whether animations whose duration is not an exact
		 * multiple of the increment should end their animation on the last valid
		 * time-step, or jump directly to the specified end time at the conclusion
		 * of the animation.
		 */
		bool d_finish_exactly_on_end_time;
		
		/**
		 * This option controls whether animations should loop or simply stop
		 * once they reach the end time.
		 */
		bool d_loop;
		
		/**
		 * This option controls whether start and end times should be adjusted
		 * to contain the current time whenever the current time lies outside
		 * the bounds.
		 */
		bool d_adjust_bounds_to_contain_current_time;
		
		/**
		 * Does the work of configuring and starting the timer, beginning the
		 * animation and emitting an appropriate signal.
		 */
		void
		start_animation_timer();
		
		/**
		 * Stops the timer, pausing the animation and emitting an appropriate signal.
		 */
		void
		stop_animation_timer();


		/**
		 * Double-checks the value of the member datum @a d_time_increment.
		 *
		 * This function examines the current time range and determines whether
		 * the value of this datum must be greater than zero or less than zero in
		 * order to successively increment the current-time from the start-time to the
		 * end-time.
		 */
		void
		recalculate_increment();

	};
}

#endif
