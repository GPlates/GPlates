/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Scopeguard, by Andrei Alexandrescu and Petru Marginean, December 2000.
 * Modified by Joshua Lehrer, FactSet Research Systems, November 2005.
 */
 
#ifndef GPLATES_UTILS_SCOPEGUARD_H
#define GPLATES_UTILS_SCOPEGUARD_H

namespace GPlatesUtils
{
	//
	// The source code in this file was obtained from
	// http://www.zete.org/people/jlehrer/scopeguard.html
	//
	// Also a more in-depth description can be found at
	// http://www.drdobbs.com by searching for 'scopeguard' and
	// selecting the Dec 01, 2000 article by Andrei Alexandrescu.
	// 
	// The following example gives Vector::foo() commit-or-rollback functionality
	// in the presence of exceptions:
	//
	//    struct Vector
	//    {
	//       void foo();
	//
	//       typedef std::vector<int> vector_type;
	//       vector_type d_vec;
	//    };
	//
	//    void Vector::foo()
	//    {
	//       // 'push_back' can throw
	//       d_vec.push_back(1);
	//       // 'pop_back' does not throw
	//       GPlatesUtils::ScopeGuard guard1 = GPlatesUtils::make_guard(
	//           &vector_type::pop_back, vec);
	//
	//       // 'push_back' can throw
	//       d_vec.push_back(2);
	//       // 'pop_back' does not throw
	//       GPlatesUtils::ScopeGuard guard2 = GPlatesUtils::make_guard(
	//           &vector_type::pop_back, vec);
	//
	//       // 'push_back' can throw
	//       d_vec.push_back(3);
	//       // No need for guard on last atomic operation.
	//
	//       // We got this far without any exceptions so dismiss all undos.
	//       guard1.dismiss();
	//       guard2.dismiss();
	//    }
	//

	template <class T>
	class RefHolder
	{
		T& ref_;
	public:
		RefHolder(T& ref) : ref_(ref) {}
		operator T& () const 
		{
			return ref_;
		}
	private:
		// Disable assignment - not implemented
		RefHolder& operator=(const RefHolder&);
	};

	template <class T>
	inline RefHolder<T> ByRef(T& t)
	{
		return RefHolder<T>(t);
	}

	class ScopeGuardImplBase
	{
		ScopeGuardImplBase& operator =(const ScopeGuardImplBase&);
	protected:
		~ScopeGuardImplBase()
		{
		}
		ScopeGuardImplBase(const ScopeGuardImplBase& other)
			: dismissed_(other.dismissed_)
		{
			other.dismiss();
		}
		template <typename J>
		static void safe_execute(J& j)
		{
			if (!j.dismissed_)
				try
				{
					j.execute();
				}
				catch(...)
				{
				}
		}
		
		mutable bool dismissed_;
	public:
		ScopeGuardImplBase() : dismissed_(false) 
		{
		}
		void dismiss() const
		{
			dismissed_ = true;
		}
	};

	typedef const ScopeGuardImplBase& ScopeGuard;

	template <typename F>
	class ScopeGuardImpl0 : public ScopeGuardImplBase
	{
	public:
		static ScopeGuardImpl0<F> make_guard(F fun)
		{
			return ScopeGuardImpl0<F>(fun);
		}
		~ScopeGuardImpl0()
		{
			safe_execute(*this);
		}
		void execute() 
		{
			fun_();
		}
	protected:
		ScopeGuardImpl0(F fun) : fun_(fun) 
		{
		}
		F fun_;
	};

	template <typename F> 
	inline ScopeGuardImpl0<F> make_guard(F fun)
	{
		return ScopeGuardImpl0<F>::make_guard(fun);
	}

	template <typename F, typename P1>
	class ScopeGuardImpl1 : public ScopeGuardImplBase
	{
	public:
		static ScopeGuardImpl1<F, P1> make_guard(F fun, P1 p1)
		{
			return ScopeGuardImpl1<F, P1>(fun, p1);
		}
		~ScopeGuardImpl1()
		{
			safe_execute(*this);
		}
		void execute()
		{
			fun_(p1_);
		}
	protected:
		ScopeGuardImpl1(F fun, P1 p1) : fun_(fun), p1_(p1) 
		{
		}
		F fun_;
		const P1 p1_;
	};

	template <typename F, typename P1> 
	inline ScopeGuardImpl1<F, P1> make_guard(F fun, P1 p1)
	{
		return ScopeGuardImpl1<F, P1>::make_guard(fun, p1);
	}

	template <typename F, typename P1, typename P2>
	class ScopeGuardImpl2: public ScopeGuardImplBase
	{
	public:
		static ScopeGuardImpl2<F, P1, P2> make_guard(F fun, P1 p1, P2 p2)
		{
			return ScopeGuardImpl2<F, P1, P2>(fun, p1, p2);
		}
		~ScopeGuardImpl2()
		{
			safe_execute(*this);
		}
		void execute()
		{
			fun_(p1_, p2_);
		}
	protected:
		ScopeGuardImpl2(F fun, P1 p1, P2 p2) : fun_(fun), p1_(p1), p2_(p2) 
		{
		}
		F fun_;
		const P1 p1_;
		const P2 p2_;
	};

	template <typename F, typename P1, typename P2>
	inline ScopeGuardImpl2<F, P1, P2> make_guard(F fun, P1 p1, P2 p2)
	{
		return ScopeGuardImpl2<F, P1, P2>::make_guard(fun, p1, p2);
	}

	template <typename F, typename P1, typename P2, typename P3>
	class ScopeGuardImpl3 : public ScopeGuardImplBase
	{
	public:
		static ScopeGuardImpl3<F, P1, P2, P3> make_guard(F fun, P1 p1, P2 p2, P3 p3)
		{
			return ScopeGuardImpl3<F, P1, P2, P3>(fun, p1, p2, p3);
		}
		~ScopeGuardImpl3()
		{
			safe_execute(*this);
		}
		void execute()
		{
			fun_(p1_, p2_, p3_);
		}
	protected:
		ScopeGuardImpl3(F fun, P1 p1, P2 p2, P3 p3) : fun_(fun), p1_(p1), p2_(p2), p3_(p3) 
		{
		}
		F fun_;
		const P1 p1_;
		const P2 p2_;
		const P3 p3_;
	};

	template <typename F, typename P1, typename P2, typename P3>
	inline ScopeGuardImpl3<F, P1, P2, P3> make_guard(F fun, P1 p1, P2 p2, P3 p3)
	{
		return ScopeGuardImpl3<F, P1, P2, P3>::make_guard(fun, p1, p2, p3);
	}

	//************************************************************

	template <class Obj, typename MemFun>
	class ObjScopeGuardImpl0 : public ScopeGuardImplBase
	{
	public:
		static ObjScopeGuardImpl0<Obj, MemFun> make_obj_guard(Obj& obj, MemFun memFun)
		{
			return ObjScopeGuardImpl0<Obj, MemFun>(obj, memFun);
		}
		~ObjScopeGuardImpl0()
		{
			safe_execute(*this);
		}
		void execute() 
		{
			(obj_.*memFun_)();
		}
	protected:
		ObjScopeGuardImpl0(Obj& obj, MemFun memFun) 
			: obj_(obj), memFun_(memFun) {}
		Obj& obj_;
		MemFun memFun_;
	};

	template <class Obj, typename MemFun>
	inline ObjScopeGuardImpl0<Obj, MemFun> make_obj_guard(Obj& obj, MemFun memFun)
	{
		return ObjScopeGuardImpl0<Obj, MemFun>::make_obj_guard(obj, memFun);
	}

	template <typename Ret, class Obj1, class Obj2>
	inline ObjScopeGuardImpl0<Obj1,Ret(Obj2::*)()> make_guard(Ret(Obj2::*memFun)(), Obj1 &obj) {
	  return ObjScopeGuardImpl0<Obj1,Ret(Obj2::*)()>::make_obj_guard(obj,memFun);
	}

	template <typename Ret, class Obj1, class Obj2>
	inline ObjScopeGuardImpl0<Obj1,Ret(Obj2::*)()> make_guard(Ret(Obj2::*memFun)(), Obj1 *obj) {
	  return ObjScopeGuardImpl0<Obj1,Ret(Obj2::*)()>::make_obj_guard(*obj,memFun);
	}

	template <class Obj, typename MemFun, typename P1>
	class ObjScopeGuardImpl1 : public ScopeGuardImplBase
	{
	public:
		static ObjScopeGuardImpl1<Obj, MemFun, P1> make_obj_guard(Obj& obj, MemFun memFun, P1 p1)
		{
			return ObjScopeGuardImpl1<Obj, MemFun, P1>(obj, memFun, p1);
		}
		~ObjScopeGuardImpl1()
		{
			safe_execute(*this);
		}
		void execute() 
		{
			(obj_.*memFun_)(p1_);
		}
	protected:
		ObjScopeGuardImpl1(Obj& obj, MemFun memFun, P1 p1) 
			: obj_(obj), memFun_(memFun), p1_(p1) {}
		Obj& obj_;
		MemFun memFun_;
		const P1 p1_;
	};

	template <class Obj, typename MemFun, typename P1>
	inline ObjScopeGuardImpl1<Obj, MemFun, P1> make_obj_guard(Obj& obj, MemFun memFun, P1 p1)
	{
		return ObjScopeGuardImpl1<Obj, MemFun, P1>::make_obj_guard(obj, memFun, p1);
	}

	template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b>
	inline ObjScopeGuardImpl1<Obj1,Ret(Obj2::*)(P1a),P1b> make_guard(Ret(Obj2::*memFun)(P1a), Obj1 &obj, P1b p1) {
	  return ObjScopeGuardImpl1<Obj1,Ret(Obj2::*)(P1a),P1b>::make_obj_guard(obj,memFun,p1);
	}

	template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b>
	inline ObjScopeGuardImpl1<Obj1,Ret(Obj2::*)(P1a),P1b> make_guard(Ret(Obj2::*memFun)(P1a), Obj1 *obj, P1b p1) {
	  return ObjScopeGuardImpl1<Obj1,Ret(Obj2::*)(P1a),P1b>::make_obj_guard(*obj,memFun,p1);
	}

	template <class Obj, typename MemFun, typename P1, typename P2>
	class ObjScopeGuardImpl2 : public ScopeGuardImplBase
	{
	public:
		static ObjScopeGuardImpl2<Obj, MemFun, P1, P2> make_obj_guard(Obj& obj, MemFun memFun, P1 p1, P2 p2)
		{
			return ObjScopeGuardImpl2<Obj, MemFun, P1, P2>(obj, memFun, p1, p2);
		}
		~ObjScopeGuardImpl2()
		{
			safe_execute(*this);
		}
		void execute() 
		{
			(obj_.*memFun_)(p1_, p2_);
		}
	protected:
		ObjScopeGuardImpl2(Obj& obj, MemFun memFun, P1 p1, P2 p2) 
			: obj_(obj), memFun_(memFun), p1_(p1), p2_(p2) {}
		Obj& obj_;
		MemFun memFun_;
		const P1 p1_;
		const P2 p2_;
	};

	template <class Obj, typename MemFun, typename P1, typename P2>
	inline ObjScopeGuardImpl2<Obj, MemFun, P1, P2> make_obj_guard(Obj& obj, MemFun memFun, P1 p1, P2 p2)
	{
		return ObjScopeGuardImpl2<Obj, MemFun, P1, P2>::make_obj_guard(obj, memFun, p1, p2);
	}

	template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a, typename P2b>
	inline ObjScopeGuardImpl2<Obj1,Ret(Obj2::*)(P1a,P2a),P1b,P2b> make_guard(Ret(Obj2::*memFun)(P1a,P2a), Obj1 &obj, P1b p1, P2b p2) {
	  return ObjScopeGuardImpl2<Obj1,Ret(Obj2::*)(P1a,P2a),P1b,P2b>::make_obj_guard(obj,memFun,p1,p2);
	}

	template <typename Ret, class Obj1, class Obj2, typename P1a, typename P1b, typename P2a, typename P2b>
	inline ObjScopeGuardImpl2<Obj1,Ret(Obj2::*)(P1a,P2a),P1b,P2b> make_guard(Ret(Obj2::*memFun)(P1a,P2a), Obj1 *obj, P1b p1, P2b p2) {
	  return ObjScopeGuardImpl2<Obj1,Ret(Obj2::*)(P1a,P2a),P1b,P2b>::make_obj_guard(*obj,memFun,p1,p2);
	}
}


#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)

#define ON_BLOCK_EXIT GPlatesUtils::ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = GPlatesUtils::make_guard
#define ON_BLOCK_EXIT_OBJ GPlatesUtils::ScopeGuard ANONYMOUS_VARIABLE(scopeGuard) = GPlatesUtils::make_obj_guard

#endif // GPLATES_UTILS_SCOPEGUARD_H
