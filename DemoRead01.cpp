/*
 * DemoRead01.cpp - simple example reader using E57 Foundation API.
 *
 * Copyright (C) 2009 Kevin Ackley (kackley@gwi.net)
 *
 * This file is part of the E57 Reference Implementation (E57RI).
 * 
 * E57RI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or at your option) any later version.
 * 
 * E57RI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with E57RI.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include "E57Foundation.h"
using namespace e57;
using namespace std;

void printSomePoints(ImageFile imf, CompressedVectorNode points);


void main(int argc, char** argv)
{
    try {
        /// Read file from disk
        ImageFile imf("foo.e57", "r");
        StructureNode root = imf.root();

        /// Make sure vector of scans is defined and of expected type.
        /// If "/images3D" wasn't defined, the call to root.get would raise an exception.
        if (!root.isDefined("/images3D")) {
            cout << "File doesn't contain 3D images" << endl;
            return;
        }
        Node n = root.get("/images3D");
        if (n.type() != VECTOR) {
            cout << "bad file" << endl;
            return;
        }

        /// The node is a vector so we can safely get a VectorNode handle to it.
        /// If n was not a VectorNode, this would raise an exception.
        VectorNode images3d(n);

        /// Print number of children of images3d.  This is the number of scans in file.
        int64_t scanCount = images3d.childCount();
        cout << "Number of scans in file:" << scanCount << endl;

        /// For each scan, print out first 4 points in either Cartesian or Spherical coordinates.
        for (int scanIndex = 0; scanIndex < scanCount; scanIndex++) {
            /// Get scan from "/images3d", assume its a Structure (else get exception)
            StructureNode scan(images3d.get(scanIndex));
            cout << "got:" << scan.pathName() << endl;

            /// Get "points" field in scan.  Should be a CompressedVectorNode.
            CompressedVectorNode points(scan.get("points"));
            cout << "got:" << points.pathName() << endl;

            /// Call subroutine in this file to print the points
            printSomePoints(imf, points);
        }

        imf.close();
    } catch (char* s) {
        cerr << "Got an char* exception: " << s << endl;
    } catch (const char* s) {
        cerr << "Got an const char* exception: " << s << endl;
    } catch (...) {
        cerr << "Got an exception" << endl;
    }
}


void printSomePoints(ImageFile imf, CompressedVectorNode points)
{
    /// Need to figure out if has Cartesian or spherical coordinate system.
    /// Interrogate the CompressedVector's prototype of its records.
    StructureNode proto(points.prototype());

    /// The prototype should have a field named either "cartesianX" or "sphericalRange".
    if (proto.isDefined("cartesianX")) {
        /// Make a list of buffers to receive the xyz values.
        vector<SourceDestBuffer> destBuffers;
        double x[4];     destBuffers.push_back(SourceDestBuffer(imf, "cartesianX", x, 4, true));
        double y[4];     destBuffers.push_back(SourceDestBuffer(imf, "cartesianY", y, 4, true));
        double z[4];     destBuffers.push_back(SourceDestBuffer(imf, "cartesianZ", z, 4, true));
        
        /// Create a reader of the points CompressedVector, try to read first block of 4 points
        /// Each call to reader.read() will fill the xyz buffers until the points are exhausted.
        CompressedVectorReader reader = points.reader(destBuffers);
        unsigned gotCount = reader.read();
        cout << "  got first " << gotCount << " points" << endl;

        /// Print the coordinates we got
        for (unsigned i=0; i < gotCount; i++)
            cout << "  " << i << ". x=" << x[i] << " y=" << y[i] << " z=" << z[i] << endl;
    } else if (proto.isDefined("sphericalRange")) {
        //??? not implemented yet
    } else
        cout << "Error: couldn't find either Cartesian or spherical points in scan" << endl;
}