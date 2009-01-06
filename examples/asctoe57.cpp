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


#include "e57/writer.hpp"

#include <iostream>
#include <exception>
#include <map>
#include <string>
#include <sstream>
#include <utility>

using namespace std;
using namespace e57;

int main(int argc, char* argv[])
{
    try {
        // create the target file
        writer outfile(argv[1]);
        
        // specify bindings between source types and types on disk,
        // specify the offsets of the fields
        field<unsigned, ulittle32_t> x(0);
        field<unsigned, ulittle32_t> y(4);
        field<unsigned, ulittle32_t> z(8);
        
        // define the schema by coupling the fields through a map
        map<string, basic_field_descriptor> schema;
        schema.insert(make_pair("x", x));
        schema.insert(make_pair("y", y));
        schema.insert(make_pair("z", z));

        // write 3 datasets
        for (int n=0; n<3; ++n) {
            ostringstream sname;
            sname << "example-" << n; // create unique set names
            
            // create a new stream in the file, give it a name and record size
            std::auto_ptr<wstream> img=outfile.create_stream(
                sname.str(), schema, 12
            );
            
            // the write record buffer
            wrecord w(12);
            
            // write 1000 points
            for (unsigned n=0; n<1000; ++n) {
                w[x] = n+1;
                w[y] = n+2;
                w[z] = n+3;
                
                img->write(w);
            }
        }
        
        outfile.close();
        
    }
    catch(exception& e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}
