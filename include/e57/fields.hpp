// $Id$

//////////////////////////////////////////////////////////////////////////
//      Copyright (c) ASTM E57.04 3D Coomittee, Inc. 2008
//      Copyright (c) Roland Schwarz at Riegl GmbH
//      All Rights reserved.
// E57.04 3D Imaging System Software is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. Permission is
// granted to anyone to use this software for any purpose, including
// commercial applications, and to alter it and redistribute it freely.
// The copyright will be held by the E57.04 individuals involved in the
// open source project as a group.
//
// This source code is only intended as a supplement to promote the
// E57.04 3D Imaging System File Format standard for interoperability
// of Lidar Data.
//////////////////////////////////////////////////////////////////////////

#ifndef FIELDS_HPP
#define FIELDS_HPP
#include <iostream>

#include <limits>
#include <stdexcept>

namespace e57 {

// The following typedefs are for demonstration only.
// (We need unaligned little endian types here.)
// A real implementation could use e.g. the endian class
// from Beman Dawes (see boost library)
// The types are for representation on disk.
typedef unsigned char       ulittle8_t;
typedef unsigned short int  ulittle16_t;
typedef unsigned int        ulittle32_t;

typedef signed char         little8_t;
typedef short int           little16_t;
typedef int                 little32_t;      

typedef float               flittle32_t;
typedef double              flittle64_t; 

class basic_field_descriptor
{
public:    
    enum type_t {
        undefined
        , uint8
        , int8
        , uint16
        , int16
        , uint32
        , int32
        , float32
        , float64
        
    } type;

    unsigned offset;

    basic_field_descriptor(unsigned o = 0, type_t t = undefined)
        : type(t)
        , offset(o)
    {}
};

template<class F>
class field_descriptor;

template<>
class field_descriptor<ulittle8_t>
    : public basic_field_descriptor
{
public:
    typedef ulittle8_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::uint8) 
    {}

    mutable value_type u;
};

template<>
class field_descriptor<little8_t>
    : public basic_field_descriptor
{
public:
    typedef little8_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::int8) 
    {}

    mutable value_type u;
};

template<>
class field_descriptor<ulittle16_t>
    : public basic_field_descriptor
{
public:
    typedef ulittle16_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::uint16) 
    {}

    mutable value_type u;
};

template<>
class field_descriptor<little16_t>
    : public basic_field_descriptor
{
public:
    typedef little16_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::int16) 
    {}

    mutable value_type u;
};

template<>
class field_descriptor<ulittle32_t>
    : public basic_field_descriptor
{
public:
    typedef ulittle32_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::uint32) 
    {}

    mutable value_type u;
};

template<>
class field_descriptor<little32_t>
    : public basic_field_descriptor
{
public:
    typedef little32_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::int32) 
    {}

    mutable value_type u;
};

template<>
class field_descriptor<flittle32_t>
    : public basic_field_descriptor
{
public:
    typedef flittle32_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::float32) 
    {}

    mutable value_type u;
};

template<>
class field_descriptor<flittle64_t>
    : public basic_field_descriptor
{
public:
    typedef flittle64_t value_type;
    field_descriptor(unsigned o)
        : basic_field_descriptor(o, basic_field_descriptor::float64) 
    {}

    mutable value_type u;
};

template<class T, class F = void>
class field
    : public basic_field_descriptor
{
    typedef field_descriptor<F> field_descriptor_type;
    typedef F persistent_value_type;
public:
    typedef T value_type;
    typedef T& reference_type;

    // this ctor will be used by writers
    field(unsigned o)
        : basic_field_descriptor(field_descriptor<F>(o))
    {}
    
    // this ctor will be used by readers
    field(const basic_field_descriptor& fd)
        : basic_field_descriptor(fd)
    {
        //todo: test if the type T is capable enough for 
        // basic_field_descriptor.type .
        // this inevitably has to be done at runtime
    }
    
    T get_from(const char* b) const
    {
        // todo: the following code will not work on all platforms
        // hint: unaligned access
        switch(type) {
            case basic_field_descriptor::uint8:
                return static_cast<T>(
                    *reinterpret_cast<const ulittle8_t*>(b+offset)
                );
            case basic_field_descriptor::uint16:
                return static_cast<T>(
                    *reinterpret_cast<const ulittle16_t*>(b+offset)
                );
            case basic_field_descriptor::uint32:
                return static_cast<T>(
                    *reinterpret_cast<const ulittle32_t*>(b+offset)
                );
            case basic_field_descriptor::int8:
                return static_cast<T>(
                    *reinterpret_cast<const little8_t*>(b+offset)
                );
            case basic_field_descriptor::int16:
                return static_cast<T>(
                    *reinterpret_cast<const little16_t*>(b+offset)
                );
            case basic_field_descriptor::int32:
                return static_cast<T>(
                    *reinterpret_cast<const little32_t*>(b+offset)
                );
            case basic_field_descriptor::float32:
                return static_cast<T>(
                    *reinterpret_cast<const flittle32_t*>(b+offset)
                );
            case basic_field_descriptor::float64:
                return static_cast<T>(
                    *reinterpret_cast<const flittle64_t*>(b+offset)
                );
            case basic_field_descriptor::undefined:
                throw(std::invalid_argument("field: undefined source type"));
            default:
                throw(std::invalid_argument("field: unknown source type"));
        }
        return T();
    }
    
    void put_at(char* b, const T& x) const
    {
        //todo: similar as in get_from; reinterpret_cast is not as safe way
        // for unaligned access
        *reinterpret_cast<persistent_value_type*>(b+offset) = 
            static_cast<persistent_value_type>(x);
    }
    
    operator bool() const
    {
        return type != basic_field_descriptor::undefined;
    }

};

} // namespace e57

#endif //FIELDS_HPP
