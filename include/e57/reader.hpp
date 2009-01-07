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

#ifndef READER_HPP
#define READER_HPP

#include "fields.hpp"

#define TIXML_USE_STL 
#include "detail/tinyxml.h"

#include <string>
#include <map>
#include <utility>
#include <stdexcept>
#include <vector>
#include <tr1/memory>
#include <fstream>
#include <sstream>

namespace e57 {

class rrecord
{
    friend class rstream;
    std::vector<char> buffer;
        
public:
    template<class F>
    typename F::value_type operator[](const F& f)
    {
        return f.get_from(&buffer[0]);
    }
};

class rstream
{
    std::tr1::shared_ptr<std::streambuf> sb;

    friend class reader;
    rstream(std::tr1::shared_ptr<std::streambuf> sb_)
    {
        sb = sb_;
    }

public:

    std::map<std::string, basic_field_descriptor> schema;
    unsigned size; // the record size
    
    bool read(rrecord& r)
    {
        if (size != r.buffer.size())
            r.buffer.resize(size);
        // read in the next record. return false if no more records available.
		sb->sgetn(&(r.buffer[0]), static_cast<std::streamsize>(r.buffer.size()));
        return std::streambuf::traits_type::eof() != sb->sgetc();
    }
     
};


class reader
{
    std::string name;
    TiXmlDocument xml;
    TiXmlElement* images_root;

public:
    reader()
    {}
    
    reader(const std::string& s)
    { open(s); }
    
    void open(const std::string& s)
    {
        name = s;
        xml.LoadFile(name+".xml");
        TiXmlHandle doc(&xml);
        images_root = doc
            . FirstChild("e57:threeDImageFile")
            . FirstChild("e57:threeDImages")
            . ToElement()
            ;
        if (0==images_root)
            throw(std::runtime_error("cannot open xml information"));
            
        TiXmlElement* e57image;
        for (
            e57image = images_root->FirstChildElement("e57:threeDImage");
            e57image !=0;
            e57image = e57image->NextSiblingElement("e57:threeDImage")
        ) {
            TiXmlElement* e57schema 
                = e57image->FirstChildElement("e57:schema");
            if (0 != e57schema) {
                std::tr1::shared_ptr<std::filebuf> fb(new std::filebuf);
                rstream r(std::tr1::dynamic_pointer_cast<std::streambuf>(fb));
                TiXmlElement* e57field;
                for (
                    e57field = e57schema->FirstChildElement("e57:field");
                    e57field != 0;
                    e57field = e57field->NextSiblingElement("e57:field")
                ) {
                    unsigned offset;
                    int type;
                    std::string name;
                    name = e57field->Attribute("name");
                    e57field->QueryValueAttribute("type", &type);
                    e57field->QueryValueAttribute("offset", &offset);
                    r.schema.insert(
                        std::make_pair(
                            name
                            , basic_field_descriptor(
                                offset
                                , basic_field_descriptor::type_t(type)
                            )
                        )
                    );
                }
                std::ostringstream fbname;
                fbname << name << "." << e57image->Attribute("index");
                std::cout << fbname.str() << std::endl;
                fb->open(
                    fbname.str().c_str()
                    , std::ios_base::in|std::ios_base::binary
                );
                e57schema->QueryValueAttribute("size", &(r.size));
                std::string iname = e57image->Attribute("name");
                streams.insert(std::make_pair(iname,r));
                std::cout << iname << std::endl;
            }
        }
    }

    void close()
    {}
       
    // public member variables
    std::map<std::string, rstream> streams;
    
};

} // namespace e57

#endif // READER_HPP
