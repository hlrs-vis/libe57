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


#ifndef WRITER_HPP
#define WRITER_HPP

#include "fields.hpp"

#define TIXML_USE_STL 
#include "detail/tinyxml.h"

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdio>

namespace e57 {

class wrecord
{
    friend class wstream;
    std::vector<char> buffer;
    
    template<class F>
    class proxy
    {
        const F& field;
        char* begin;
        
    public:
        proxy(const F& f, char* b)
            : field(f)
            , begin(b)
        {}
        
        proxy& operator=(const typename F::value_type& x)
        {
            field.put_at(begin, x);
            return *this;
        }
    };
    
public:

    wrecord(unsigned size)
        : buffer(size)
    {}
    
    template<class F>
    proxy<F> operator[](const F& f)
    {
        return proxy<F>(f, &buffer[0]);
    }
};

class writer;

class wstream
{
    std::auto_ptr<std::streambuf> sb;
    
    // wstream must not be copied
    // (wstream is moveable only via auto_ptr)
    wstream(const wstream&);
    wstream& operator = (const wstream&);
    
    // only writer may create instances of wstream
    friend class writer;
    wstream(std::auto_ptr<std::streambuf>& sb_)
    {
        sb = sb_;
    }
    
public:

    void write(const wrecord& r)
    {
        sb->sputn(&(r.buffer[0]), r.buffer.size());
    }
};

class writer
{
    std::string name;
    TiXmlDocument xml;
    std::ofstream xmls;
    TiXmlElement* images_root;
    std::map<std::string, unsigned> images;
    
public:
    writer()
        : images_root(0)
    {}
    
    writer(const std::string& s)
        : images_root(0)
    { open(s); }
    
    ~writer()
    {
        // maintain invariant, i.e. remove fragments if file has not
        // been created entirely
        if (xmls.is_open()) {
            std::remove((name+".xml").c_str());
        }
    }
    
    void open(const std::string& s)
    {
        name = s;
        xmls.open((name+".xml").c_str());
        xml.InsertEndChild(TiXmlDeclaration( "1.0", "US-ASCII", "yes" ));
        xml.InsertEndChild(TiXmlElement("e57:threeDImageFile"));
        xml.RootElement()->SetAttribute("xmlns:e57","http://astm.org/e57/v0.1");
        images_root = new TiXmlElement("e57:threeDImages");
        xml.RootElement()->LinkEndChild(images_root);
    }

    void close()
    {
        xmls << xml;
        xml.Clear();
        images_root = 0;
        xmls.close();
    }
       
    std::auto_ptr<wstream> create_stream(
        const std::string& s
        , const std::map<std::string, basic_field_descriptor>& schema
        , unsigned size
    )
    {
        if (images.end() != images.find(s))
            throw(std::runtime_error("image names must be unique."));
        
        images.insert(std::make_pair(s, images.size()));
        
        //write the schema definition to the xml directory
        TiXmlElement* e57image = new TiXmlElement("e57:threeDImage");
        images_root->LinkEndChild(e57image);
        // the name of the image is an attribute
        e57image->SetAttribute("name", s);
        e57image->SetAttribute("index", images[s]);
        TiXmlElement* e57schema = new TiXmlElement("e57:schema");
        // the size attribute is the number of octetts in the record
        e57schema->SetAttribute("size", size);
        e57image->LinkEndChild(e57schema);
        // iterate over map of field descriptors
        std::map<std::string, basic_field_descriptor>::const_iterator it;
        for (it=schema.begin(); it!=schema.end(); ++it) {
            TiXmlElement* fd = new TiXmlElement("e57:field");
            // set name, type and offset as attributes
            fd->SetAttribute("name", it->first);
            fd->SetAttribute("type", it->second.type);
            fd->SetAttribute("offset", it->second.offset);
            e57schema->LinkEndChild(fd);
        }
        
        //create the stream
        std::ostringstream sbname;
        sbname << name << "." << images[s];
        std::auto_ptr<std::filebuf> fb(new std::filebuf());
        fb->open(
            sbname.str().c_str()
            , std::ios_base::out|std::ios_base::binary
        );
        std::auto_ptr<std::streambuf> sb(fb.release());
        return std::auto_ptr<wstream>(new wstream(sb));
    }
    
};

} // namespace e57

#endif // WRITER_HPP
