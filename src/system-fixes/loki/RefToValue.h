////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2006 Richard Sposato
// Copyright (c) 2006 Peter K�mmel
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The authors make no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////
#ifndef LOKI_REFTOVALUE_H
#define LOKI_REFTOVALUE_H

namespace Loki
{

    ////////////////////////////////////////////////////////////////////////////////
    ///  \class RefToValue
    ///
    ///  \ingroup SmartPointerGroup 
    ///  Transports a reference as a value
    ///  Serves to implement the Colvin/Gibbons trick for SmartPtr/ScopeGuard
    ////////////////////////////////////////////////////////////////////////////////

    template <class T>
    class RefToValue
    {   
    public:
    
        RefToValue(T& ref) : ref_(ref) 
        {}

        RefToValue(const RefToValue& rhs) : ref_(rhs.ref_)
        {}

        operator T& () const 
        {
            return ref_;
        }

    private:
        // Disable - not implemented
        RefToValue();
        RefToValue& operator=(const RefToValue&);
        
        T& ref_;
    };


    ////////////////////////////////////////////////////////////////////////////////
    ///  \ingroup ExceptionGroup 
    ///  RefToValue creator.
    ////////////////////////////////////////////////////////////////////////////////

    template <class T>
    inline RefToValue<T> ByRef(T& t)
    {
        return RefToValue<T>(t);
    }    
    
}



#endif //LOKI_REFTOVALUE_H

