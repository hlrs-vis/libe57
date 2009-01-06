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

#include "e57/reader.hpp"

#include <iostream>
#include <exception>
#include <map>

using namespace std;
using namespace e57;

int main(int argc, char* argv[])
{
    try {
        
        // The reader object establishes the connection to the external file
        reader infile(argv[1]);
        
        // The reader object has a map from image names to stream objects
        map<string, rstream>::iterator it;
        
        // The map can be iterated (or looked up by name)
        for (it = infile.streams.begin(); it != infile.streams.end(); ++it) {
            
            cout << "3D image: " << it->first << endl;
            
            // Define a reference to current stream.
            rstream& img(it->second);

            // A record is a reference to a point
            rrecord r;
            
            // Define various field extractor function objects.
            // img.schema is a map of objects that describe the offset,
            // type (float or integer) and bitsize of the attribute.
            // This information may be taken from the embeded XML description.
            // If a field is not in the file, attrib will return an empty
            // description that can be handled appropriately by the extractor.
            field<double> x(img.schema["x"]);
            field<double> y(img.schema["y"]);
            field<double> z(img.schema["z"]);
            //field<float>  a(img.schema["a"]);
            
            // The next field is a vendor extension:
            field<unsigned> e(img.schema["riegl:e"]);
            
            // The pointcloud is read sequentially in this example
            while (img.read(r)) {
                // and the extractors are used to access the record.
                cout << r[x] << ", " << r[y] << ", " << r[z];
                // Conditionally process extension field.
                if (e) cout << ", " << r[e];
                cout << endl;
            }
        }
    }
    catch(exception& e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}
