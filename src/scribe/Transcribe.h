/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBE_H
#define GPLATES_SCRIBE_TRANSCRIBE_H

#include "TranscribeResult.h"


namespace GPlatesScribe
{
	// Forward declaration.
	class Scribe;

	// Forward declaration.
	template <typename ObjectType> class ConstructObject;


	/**
	 * Befriending this class enables the scribe system to privately access your client class.
	 */
	class Access;


	/**
	 * The function declaration for transcribing an object of type 'ObjectType'.
	 *
	 * Essentially there are two ways to transcribe an arbitrary class (or type):
	 *
	 *   (1) Non-intrusive approach: specialise or overload this 'transcribe()' non-member function.
	 *   (2) Intrusive approach: implement a 'transcribe()' method in your 'ObjectType' class.
	 *
	 * Method (1) is non-intrusive:
	 *
	 *   class A
	 *   {
	 *   public:
	 *		// NOTE: The non-intrusive approach only works for this class because a *reference*
	 *		// to the internal data is returned here...
	 *		const int & get_x() const { return x; }
	 *   private:
	 *		int x;
	 *   };
	 *
	 *   GPlatesScribe::TranscribeResult
	 *   transcribe(
	 *			GPlatesScribe::Scribe &scribe,
	 *			A &a,
	 *			bool transcribed_construct_data)
	 *   {
	 *		// Still loads/saves even though A::get_x() is 'const'...
	 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a.get_x(), "x", GPlatesScribe::TRACK))
	 *		{
	 *			return scribe.get_transcribe_result();
	 *		}
	 *
	 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *   }
	 *
	 * Method (2) is intrusive:
	 *
	 *   class A
	 *   {
	 *   public:
	 *		// NOTE: No *reference* to internal data is returned here so we need the intrusive approach...
	 *		int get_x() const { return x; }
	 *		
	 *   private:
	 *		int x;
	 *
	 *		//! Only the scribe system should call @a transcribe method.
	 *		friend class GPlatesScribe::Access;
	 *
	 *		//! Transcribe to/from serialization archives.
	 *		GPlatesScribe::TranscribeResult
	 *		transcribe(
	 *				GPlatesScribe::Scribe &scribe,
	 *				bool transcribed_construct_data)
	 *		{
	 *			if (!scribe.transcribe(TRANSCRIBE_SOURCE, x, "x", GPlatesScribe::TRACK))
	 *			{
	 *				return scribe.get_transcribe_result();
	 *			}
	 *
	 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *		}
	 *   };
	 *
	 *
	 * The parameter @a transcribed_construct_data indicates whether the function
	 * 'transcribe_construct_data()' has been called for @a object. This helps determine whether
	 * some data members of @a object have already been transcribed and hence do not need to be
	 * transcribed again.
	 *
	 * The following examples illustrates this:
	 *
	 *
	 *   class B
	 *   {
	 *   public:
	 *		B(const X &x) : d_x(x) {  }
	 *
	 *   private:
	 *		X d_x;
	 *		Y d_y;
	 *   
	 *		//! Only the scribe system should call @a transcribe and @a transcribe_construct_data methods.
	 *		friend class GPlatesScribe::Access;
	 *
	 *		GPlatesScribe::TranscribeResult
	 *		transcribe(
	 *				GPlatesScribe::Scribe &scribe,
	 *				bool transcribed_construct_data)
	 *		{
	 *			// If 'd_x' has not been transcribed in static method 'transcribe_construct_data()'
	 *			// then transcribe it here...
	 *			if (!transcribed_construct_data)
	 *			{
	 *				if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_x, "x", GPlatesScribe::TRACK))
	 *				{
	 *					return scribe.get_transcribe_result();
	 *				}
	 *			}
	 *
	 *			// Transcribe 'd_y' as normal (it's not a constructor parameter)...
	 *			if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_y, "y", GPlatesScribe::TRACK))
	 *			{
	 *				return scribe.get_transcribe_result();
	 *			}
	 *
	 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *		}
	 *
	 *		static
	 *		GPlatesScribe::TranscribeResult
	 *		transcribe_construct_data(
	 *				GPlatesScribe::Scribe &scribe,
	 *				GPlatesScribe::ConstructObject<B> &b)
	 *		{
	 *			if (scribe.is_saving())
	 *			{
	 *				scribe.save(TRANSCRIBE_SOURCE, b->d_x, "x", GPlatesScribe::TRACK);
	 *			}
	 *			else // loading
	 *			{
	 *				// Load 'x'.
	 *				GPlatesScribe::LoadRef<X> x = scribe.load<X>(TRANSCRIBE_SOURCE, "x", GPlatesScribe::TRACK);
	 *				if (!x.is_valid())
	 *				{
	 *					return scribe.get_transcribe_result();
	 *				}
	 *		
	 *				// Construct 'b' using 'x'.
	 *				// NOTE: Cannot dereference 'b' before here (since not yet constructed).
	 *				b.construct_object(x);
	 *		
	 *				// The transcribed 'x' now has a new address (inside 'b').
	 *				// NOTE: It's OK to dereference 'b' here (since is has been constructed above).
	 *				scribe.relocated(TRANSCRIBE_SOURCE, b->d_x, x);
	 *			}
	 *		
	 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *		}
	 *   };
	 *
	 * ...see function @a transcribe_construct_data (below) for more details.
	 *
	 *
	 * Note: Specialisation and overloads, of this function, for classes and types from external
	 * libraries such as Qt, boost and the C++ standard library are defined in separate source
	 * files named "Transcribe<library>.h".
	 *
	 * Note: Classes from the GPlates source code should handle transcribing by declaring their
	 * 'transcribe()' overload *in* that class's header file.
	 * They should not be declared/defined here.
	 *
	 *
	 * NOTE: If access to private data of 'ObjectType' is (likely) needed then the intrusive
	 * approach, method (2), is better and you'll also need the following friend declaration
	 * in your 'ObjectType' class:
	 *
	 *    friend class GPlatesScribe::Access;
	 *
	 */
	template <typename ObjectType>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			ObjectType &object,
			bool transcribed_construct_data);


	/**
	 * Used to transcribe constructor data for an 'ObjectType' with a *non-default* constructor.
	 *
	 * This function only needs to be implemented if there is no default constructor for type 'ObjectType'.
	 *
	 * Note that the rest of the object is still transcribed using @a transcribe which excludes
	 * any transcribed constructor parameters.
	 *
	 * An example scenario for a non-default constructor...
	 *
	 *   struct A
	 *   {
	 *		A(const X &x) : d_x(x) {  }
	 *		X d_x;
	 *   };
	 *
	 *   GPlatesScribe::TranscribeResult
	 *   transcribe_construct_data(
	 *			GPlatesScribe::Scribe &scribe,
	 *			GPlatesScribe::ConstructObject<A> &a)
	 *   {
	 *		if (scribe.is_saving())
	 *		{
	 *			scribe.save(TRANSCRIBE_SOURCE, a->d_x, "x", GPlatesScribe::TRACK);
	 *		}
	 *		else // loading
	 *		{
	 *			GPlatesScribe::LoadRef<X> x = scribe.load<X>(TRANSCRIBE_SOURCE, "x", GPlatesScribe::TRACK);
	 *			if (!x.is_valid())
	 *			{
	 *				return scribe.get_transcribe_result();
	 *			}
	 *
	 *			// NOTE: Cannot dereference 'a' before here (since not yet constructed).
	 *			a.construct_object(x);
	 *
	 *			// The transcribed 'x' now has a new address (inside 'a').
	 *			// NOTE: It's OK to dereference 'a' here (since is has been constructed above).
	 *			scribe.relocated(TRANSCRIBE_SOURCE, a->d_x, x);
	 *		}
	 *		
	 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *   }
	 *
	 *
	 * ...in the above example note that the *non-intrusive* method of transcribing constructor data
	 * was used. In other words the non-member function 'transcribe_construct_data()' was overloaded.
	 *
	 * The following example (for class B) uses the intrusive approach whereby a static method
	 * named 'transcribe_construct_data()' is implemented in the 'ObjectType' class which is used
	 * automatically by the scribe system if it exists (otherwise the non-member function is used).
	 *
	 *
	 *   class B
	 *   {
	 *   private:
	 *		B(X &x, const Y &y) : d_x(x), d_y(y) {  }
	 *		X &d_x;
	 *		Y d_y;
	 *   
	 *		//! Only the scribe system should call @a transcribe_construct_data method.
	 *		friend class GPlatesScribe::Access;
	 *
	 *		static
	 *		GPlatesScribe::TranscribeResult
	 *		transcribe_construct_data(
	 *				GPlatesScribe::Scribe &scribe,
	 *				GPlatesScribe::ConstructObject<B> &b)
	 *		{
	 *			if (scribe.is_saving())
	 *			{
	 *				scribe.save_reference(TRANSCRIBE_SOURCE, b->d_x, "x");
	 *				scribe.save(TRANSCRIBE_SOURCE, b->d_y, "y", GPlatesScribe::TRACK);
	 *			}
	 *			else // loading
	 *			{
	 *				// Load 'x'.
	 *				GPlatesScribe::LoadRef<X> x = scribe.load_reference<X>(TRANSCRIBE_SOURCE, "x");
	 *				if (!x.is_valid())
	 *				{
	 *					return scribe.get_transcribe_result();
	 *				}
	 *		
	 *				// Load 'y'.
	 *				GPlatesScribe::LoadRef<Y> y = scribe.load<Y>(TRANSCRIBE_SOURCE, "y", GPlatesScribe::TRACK);
	 *				if (!y.is_valid())
	 *				{
	 *					return scribe.get_transcribe_result();
	 *				}
	 *		
	 *				// Construct 'b' using 'x' and 'y'.
	 *				// NOTE: Cannot dereference 'b' before here (since not yet constructed).
	 *				b.construct_object(x, y);
	 *		
	 *				// The transcribed 'y' now has a new address (inside 'b').
	 *				// But we don't relocate *references* (ie, don't relocate 'x').
	 *				// NOTE: It's OK to dereference 'b' here (since is has been constructed above).
	 *				scribe.relocated(TRANSCRIBE_SOURCE, b->d_y, y);
	 *			}
	 *		
	 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *		}
	 *   };
	 *
	 *
	 * This function only gets called when a new object needs to be created. For example,
	 * when transcribing a shared pointer to a polymorphic object - the polymorphic object is
	 * loaded/created by the scribe system. This function is responsible for loading/transcribing
	 * the constructor parameters of 'ObjectType' and for constructs an instance of 'ObjectType'.
	 * After this function is called, the function @a transcribe is called to transcribe the remaining
	 * data members of 'ObjectType' that did not come from the transcribed constructor parameters.
	 *
	 * However when transcribing an existing object that does not first need to be created (such as
	 * a data member of an already constructed object) then only the function 'transcribe()' is called.
	 *
	 *
	 * NOTE: If access to private data of 'ObjectType' is (likely) needed then the intrusive
	 * approach (class B example) is better and you'll also need the following friend declaration
	 * in your 'ObjectType' class:
	 *
	 *    friend class GPlatesScribe::Access;
	 *
	 */
	template <typename ObjectType>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject<ObjectType> &object);


	/**
	 * Notification from the Scribe that a previously transcribed (loaded) object has been moved
	 * to a new memory location.
	 *
	 * This gives 'ObjectType' a chance to respond to the relocation of any of its data members
	 * that is not handled directly by the Scribe. Most data members of 'ObjectType' are already
	 * handled by the scribe system. If all data members are handled then nothing need be done and
	 * the default 'relocated()' function, which does nothing, will get used.
	 * It is really only pointer members that *own* their pointed-to object that need to be handled
	 * by 'ObjectType' explicitly - in which case this 'relocated()' function needs to be
	 * specialised or overloaded for 'ObjectType'. The alternative (which is even better) is add a
	 * static method to 'ObjectType' named 'relocated()' as the following example demonstrates...
	 *
	 *
	 *   class C
	 *   {
	 *   public:
	 *		C(int i) : c(new int(i)) { }
	 *		C(const C &rhs) : c(new int(*rhs.c)) { }
	 *		C& operator=(const C &rhs) { delete c; c = new int(*rhs.c); }
	 *		~C() { delete c; }
	 *
	 *   private:
	 *		int *c; // Pointer owns integer memory (allocated by operator new).
	 *
	 *		//! Only the scribe system should call @a relocated method.
	 *		friend class GPlatesScribe::Access;
	 *
	 *		static
	 *		void
	 *		relocated(
	 *				GPlatesScribe::Scribe &scribe,
	 *				const C &relocated_c,
	 *				const C &transcribed_c)
	 *		{
	 *			// Let the Scribe know that C's copy constructor relocated the integer '*C::c'.
	 *			scribe.relocated(TRANSCRIBE_SOURCE, *relocated_c.c, *transcribed_c.c);
	 *			
	 *			// We don't need to worry about relocating 'c' itself because the scribe system
	 *			// does this for us (because 'c' is contained within the 'C' object).
	 *			// The pointed-to object '*c' however is outside (which is why we handle it here).
	 *		}
	 *   };
	 *
	 *
	 * Note that we don't need to iterate over sub-objects that are *contained* inside 'transcribed_object'
	 * and relocate them - this is already handled by the scribe system.
	 * The Scribe is just notifying us in case 'ObjectType' has owning *pointers* to other outside
	 * objects that must be dealt with manually as seen in the above example.
	 *
	 * In the above example note that the *intrusive* method of relocating was used whereby a
	 * static method named 'relocated()' is implemented in the 'ObjectType' class which is used
	 * automatically by the scribe system if it exists (otherwise the non-member function is used).
	 *
	 * However the following examples use the *non-intrusive* approach whereby the non-member fuction
	 * 'relocated()' is overloaded.
	 *
	 * The meaning of *contained* is illustrated in the following example:
	 *
	 *   struct A { int a; int array[10]; };
	 *   struct B { int *b; };
	 *   struct C { int *c; C(int i) : c(new int(i)) {} C(const C &rhs) { c = new int(*rhs.c); } };
	 *
	 *   struct RefA { int *p; RefA(A &a) : p(&a.a) {} };
	 *   struct RefB { int *p; RefB(B &b) : p(b.b) {} };
	 *   struct RefC { int *p; RefC(C &c) : p(c.c) {} };
	 *
	 *   void relocated(GPlatesScribe::Scribe &scribe, const A &relocated_a, const A &transcribed_a)
	 *   {
	 *		// Nothing to do - don't need to create this overloaded function - it's just to demonstrate.
	 *   }
	 *   A transcribed_a;
	 *   RefA ref_a(transcribed_a);
	 *   ...
	 *   scribe.transcribe(TRANSCRIBE_SOURCE, transcribed_a, "a", GPlatesScribe::TRACK);
	 *   scribe.transcribe(TRANSCRIBE_SOURCE, ref_a, "ref_a", GPlatesScribe::TRACK);
	 *   assert(ref_a.p == &transcribed_a.a);
	 *   A relocated_a(transcribed_a);
	 *   assert(ref_a.p != &relocated_a.a);
	 *   // Scribe relocates any references to 'A::a'.
	 *   // It knows that 'A::a' is contained within struct A so we don't have to handle it.
	 *   scribe.relocated(TRANSCRIBE_SOURCE, relocated_a, transcribed_a);
	 *   assert(ref_a.p == &relocated_a.a);
	 *
	 *   void relocated(GPlatesScribe::Scribe &scribe, const B &relocated_b, const B &transcribed_b)
	 *   {
	 *		// Nothing to do - don't need to create this overloaded function - it's just to demonstrate.
	 *   }
	 *   B transcribed_b;
	 *   RefB ref_b(transcribed_b);
	 *   ...
	 *   scribe.transcribe(TRANSCRIBE_SOURCE, transcribed_b, "b", GPlatesScribe::TRACK);
	 *   scribe.transcribe(TRANSCRIBE_SOURCE, refb, "ref_b", GPlatesScribe::TRACK);
	 *   assert(ref_b.p == transcribed_b.b);
	 *   B relocated_b(transcribed_b);
	 *   assert(ref_b.p == relocated_b.b);
	 *   // Scribe has no references to relocate because nothing references the *pointer* 'B::b'.
	 *   // And 'relocated()', for B, doesn't need to do anything because the *integer* '*B::b' hasn't moved.
	 *   scribe.relocated(TRANSCRIBE_SOURCE, relocated_b, transcribed_b); // Essentially does nothing.
	 *   assert(ref_b.p == relocated_b.b); // Nothing has changed.
	 *
	 *   void relocated(GPlatesScribe::Scribe &scribe, const C &relocated_c, const C &transcribed_c)
	 *   {
	 *		// Let the Scribe know that C's copy constructor relocated the integer '*C::c'.
	 *		scribe.relocated(TRANSCRIBE_SOURCE, *relocated_c.c, *transcribed_c.c);
	 *   }
	 *   C transcribed_c;
	 *   RefC ref_c(transcribed_c);
	 *   ...
	 *   scribe.transcribe(TRANSCRIBE_SOURCE, transcribed_c, "c", GPlatesScribe::TRACK);
	 *   scribe.transcribe(TRANSCRIBE_SOURCE, ref_c, "ref_c", GPlatesScribe::TRACK);
	 *   assert(ref_c.p == transcribed_c.c);
	 *   C relocated_c(transcribed_c);
	 *   assert(ref_c.p != relocated_c.c);
	 *   // Scribe relocates any references to the *integer* '*C::c' by calling our 'relocated<C>()'.
	 *   scribe.relocated(TRANSCRIBE_SOURCE, relocated_c, transcribed_c);
	 *   assert(ref_c.p == relocated_c.c);
	 *
	 * Struct A requires no 'relocated()' because 'A::a' and 'A::array' are both contained
	 * wholly within A. And the Scribe can detect this by seeing that the address of 'A::a' is
	 * within the address range spanned by struct A (same applies to the array 'A::array').
	 * So any references to 'A::a' are automatically handled by the Scribe.
	 * Struct B also requires no 'relocated()' because the *integer*, that the *pointer* 'B::b'
	 * points to, does not move when an instance of B is moved.
	 * But struct C does require a 'relocated()' because the pointer 'C::c' *owns* the integer it
	 * points to and hence when an instance of C is moved to a new memory location we also get a
	 * new instance of the integer that 'C::c' points to (this did not happen with 'B::b' in struct B).
	 * And the Scribe needs to know about that in case any other object points to that same integer.
	 *
	 * Relocation enables tracked objects to continue to be tracked which is essential for
	 * resolving multiple pointers or references to the same object when loading an archive.
	 * NOTE: This is only called when loading from an archive (not when saving to an archive).
	 */
	template <typename ObjectType>
	void
	relocated(
			Scribe &scribe,
			const ObjectType &relocated_object,
			const ObjectType &transcribed_object);
}

#endif // GPLATES_SCRIBE_TRANSCRIBE_H
