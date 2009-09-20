/*
 * DemoWrite01.cpp - simple example writer using E57 Foundation API.
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
#include "E57Foundation.h"
using namespace e57;
using namespace std;

int main(int argc, char** argv)
{
    try {
        /// Open new file for writing, get the initialized root node (a Structure).
        /// Path name: "/"
        ImageFile imf("foo.e57", "w");
        StructureNode root = imf.root();

        /// Set per-file properties.
        /// Path names: "/formatName", "/majorVersion", "/minorVersion", "/creationDateTime"
        root.set("formatName", StringNode(imf, "ASTM E57 3D Image File"));
        root.set("majorVersion", IntegerNode(imf, 0)); //!!! versionMajor
        root.set("minorVersion", IntegerNode(imf, 2));
        root.set("creationDateTime", FloatNode(imf, 123.456));

        /// Create 3D image area.
        /// Path name: "/images3D"
        VectorNode images3d = VectorNode(imf, true);
        root.set("images3D", images3d);

        /// Create coordinate metadata area.
        /// Path name: "/coordinateMetadata"
        StructureNode coordinateMetadata = StructureNode(imf);
        root.set("coordinateMetadata", coordinateMetadata);

        /// Add first 3D image to area.
        /// Path name: "/images3D/0"
        StructureNode scan0 = StructureNode(imf);
        images3d.append(scan0);

        /// Add guid to scan0.
        /// Path name: "/images3D/0/guid".
        scan0.set("guid", StringNode(imf, "3F2504E0-4F89-11D3-9A0C-0305E82C3302"));

        /// Make a prototype datatypes that will be stored in points record.
        /// This prototype will be used in creating the points CompressedVector.
        /// The prototype is a flat structure containing each field.
        /// Using this proto in a CompressedVector will form path names like: "/images3D/0/points/0/cartesianX".
        StructureNode proto = StructureNode(imf);
        proto.set("cartesianX",  FloatNode(imf, 0.0, E57_SINGLE));
        proto.set("cartesianY",  FloatNode(imf, 0.0, E57_SINGLE));
        proto.set("cartesianZ",  FloatNode(imf, 0.0, E57_SINGLE));
        proto.set("valid",       IntegerNode(imf, 0, 0, 1));
        proto.set("rowIndex",    IntegerNode(imf, 0));
        proto.set("columnIndex", IntegerNode(imf, 0));
        proto.set("returnIndex", IntegerNode(imf, 0));
        proto.set("returnCount", IntegerNode(imf, 0));
        proto.set("timeStamp",   FloatNode(imf, 0.0, E57_DOUBLE));
        proto.set("intensity",   IntegerNode(imf, 0));
        proto.set("colorRed",    FloatNode(imf, 0.0, E57_SINGLE));
        proto.set("colorGreen",  FloatNode(imf, 0.0, E57_SINGLE));
        proto.set("colorBlue",   FloatNode(imf, 0.0, E57_SINGLE));

        /// Make empty codecs vector for use in creating points CompressedVector.
        /// If this vector is empty, it is assumed that all fields will use the BitPack codec.
        VectorNode codecs = VectorNode(imf, true);

        /// Create CompressedVector for storing points.  Path Name: "/images3D/0/points".
        /// We use the prototype and empty codecs tree from above.
        CompressedVectorNode points = CompressedVectorNode(imf, proto, codecs);
        scan0.set("points", points);

        /// Create pose structure for scan.
        /// Path names: "/images3D/0/pose/rotation/w", etc...
        ///             "/images3D/0/pose/translation/x", etc...
        StructureNode pose = StructureNode(imf);
        scan0.set("pose", pose);
        StructureNode rotation = StructureNode(imf);
        pose.set("rotation", rotation);
        rotation.set("w", FloatNode(imf, 1.0));
        rotation.set("x", FloatNode(imf, 0.0));
        rotation.set("y", FloatNode(imf, 0.0));
        rotation.set("z", FloatNode(imf, 0.0));
        StructureNode translation = StructureNode(imf);
        pose.set("translation", translation);
        translation.set("x", FloatNode(imf, 0.0));
        translation.set("y", FloatNode(imf, 0.0));
        translation.set("z", FloatNode(imf, 0.0));

        /// Add name and description to scan
        /// Path names: "/images3D/0/name", "/images3D/0/description".
        scan0.set("name", StringNode(imf, "Station 3F"));
        scan0.set("description", StringNode(imf, "An example image"));

        /// Add Cartesian bounding box to scan.
        /// Path names: "/images3D/0/cartesianBounds/xMinimum", etc...
        StructureNode bbox = StructureNode(imf);
        bbox.set("xMinimum", FloatNode(imf, 0.0));
        bbox.set("xMaximum", FloatNode(imf, 1.0));
        bbox.set("yMinimum", FloatNode(imf, 0.0));
        bbox.set("yMaximum", FloatNode(imf, 1.0));
        bbox.set("zMinimum", FloatNode(imf, 0.0));
        bbox.set("zMaximum", FloatNode(imf, 1.0));
        scan0.set("cartesianBounds", bbox);

        /// Add start/stop collection times to scan.
        /// Path names: "/images3D/0/collectionStartDateTime",
        ///             "/images3D/0/collectionEndDateTime"
        scan0.set("collectionStartDateTime", FloatNode(imf, 1234.0, E57_DOUBLE));
        scan0.set("collectionEndDateTime", FloatNode(imf, 1235.0, E57_DOUBLE));

        /// Add various sensor and version strings to scan.
        /// Path names: "/images3D/0/sensorVendor", etc...
        scan0.set("sensorVendor",    StringNode(imf, "Scan Co"));
        scan0.set("sensorModel",     StringNode(imf, "Scanmatic 2000"));
        scan0.set("serialNumber",    StringNode(imf, "123-321"));
        scan0.set("hardwareVersion", StringNode(imf, "3.3.0"));
        scan0.set("softwareVersion", StringNode(imf, "27.0.3"));
        scan0.set("firmwareVersion", StringNode(imf, "27.0.3"));

        /// Add temp/humidity to scan.
        /// Path names: "/images3D/0/temperature", etc...
        scan0.set("temperature",      FloatNode(imf, 20.0));
        scan0.set("relativeHumidity", FloatNode(imf, 40.0));

        ///================
        /// Create 2D image (camera pictures) area.
        /// Path name: "/images2D"
        VectorNode images2d = VectorNode(imf, true);
        root.set("images2D", images2d);

        /// Add a first picture area to the list.
        /// Path name: "/images2D/0"
        StructureNode picture0 = StructureNode(imf);
        images2d.append(picture0);

        /// Attach a guid to first picture.
        /// Path name: "/images2D/0/guid"
        scan0.set("guid", StringNode(imf, "3F2504E0-4F89-11D3-9A0C-0305E82C3303"));

        /// Create a toy 10 byte blob (instead of copying a big .jpg into .e57 file)
        /// Path name: "/images2D/0/jpegImage"
        BlobNode jpegImage = BlobNode(imf, 10);
        picture0.set("jpegImage", jpegImage);

        /// Save some fake data in the blob
        uint8_t fakeBlobData[10] = {1,2,3,4,5,6,7,8,9,10};
        jpegImage.write(fakeBlobData, 0, 10);

        /// Attach a pose to first picture
        /// Path names: "/images2D/0/pose/rotation/w", etc...
        ///             "/images2D/0/pose/translation/x", etc...
        StructureNode picturePose = StructureNode(imf);
        picture0.set("pose", picturePose);
        StructureNode pictureRotation = StructureNode(imf);
        picturePose.set("rotation", pictureRotation);
        pictureRotation.set("w", FloatNode(imf, 1.0));
        pictureRotation.set("x", FloatNode(imf, 0.0));
        pictureRotation.set("y", FloatNode(imf, 0.0));
        pictureRotation.set("z", FloatNode(imf, 0.0));
        StructureNode pictureTranslation = StructureNode(imf);
        picturePose.set("translation", pictureTranslation);
        pictureTranslation.set("x", FloatNode(imf, 0.0));
        pictureTranslation.set("y", FloatNode(imf, 0.0));
        pictureTranslation.set("z", FloatNode(imf, 0.0));

        /// Make an area holding pinhole projection parameters for first picture.
        /// Path names: "/images2D/0/basicPinholeProjection/imageWidth", etc...
        StructureNode pinhole = StructureNode(imf);
        picture0.set("basicPinholeProjection", pinhole);
        pinhole.set("imageWidth",      IntegerNode(imf, 1024));
        pinhole.set("imageHeight",     IntegerNode(imf, 1024));
        pinhole.set("focalLength",     FloatNode(imf, 1.0));
        pinhole.set("pixelWidth",      FloatNode(imf, 1e-3));
        pinhole.set("pixelHeight"  ,   FloatNode(imf, 1e-3));
        pinhole.set("principalPointX", FloatNode(imf, 512.0));
        pinhole.set("principalPointY", FloatNode(imf, 512.0));

        /// Add name, description, and time to first picture.
        /// Path names: "/images2D/0/name", etc...
        picture0.set("name",             StringNode(imf, "pic123"));
        picture0.set("description"  ,    StringNode(imf, "trial picture"));
        picture0.set("creationDateTime", FloatNode(imf, 321.123));

        /// Prepare vector of source buffers for writing in the CompressedVector of points
        const int N = 10;
        double cartesianX[N]  = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
        double cartesianY[N]  = {1.1, 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 8.1, 9.1, 10.1};
        double cartesianZ[N]  = {1.2, 2.2, 3.2, 4.2, 5.2, 6.2, 7.2, 8.2, 9.1, 10.2};
        int    valid[N]       = {1,   1,   1,   1,   1,   1,   1,   1,   1,   1};
        int    rowIndex[N]    = {0,   1,   0,   1,   0,   1,   0,   1,   0,   1};
        int    columnIndex[N] = {0,   0,   1,   1,   2,   2,   3,   3,   4,   4};
        int    returnIndex[N] = {0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
        int    returnCount[N] = {1,   1,   1,   1,   1,   1,   1,   1,   1,   1};
        double timeStamp[N]   = {.1,  .2,  .3,  .4,  .5,  .6,  .7,  .8,  .9,  1.0};
        int    intensity[N]   = {1,   2,   3,   2,   1,   1,   2,   3,   2,   1};
        double colorRed[N]    = {.1,  .2,  .3,  .4,  .5,  6.,  .7,  .8,  .9,  1.0};
        double colorGreen[N]  = {.5,  .5,  .5,  .5,  .5,  .5,  .5,  .5,  .5,  .5};
        double colorBlue[N]   = {1.0, .9,  .8,  .7,  .6,  .5,  .4,  .3,  .2,  .1};
        vector<SourceDestBuffer> sourceBuffers;
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianX",  cartesianX, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianY",  cartesianY, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianZ",  cartesianZ, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "valid",       valid, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "rowIndex",    rowIndex, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "columnIndex", columnIndex, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "returnIndex", returnIndex, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "returnCount", returnCount, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "timeStamp",   timeStamp, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "intensity",   intensity, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorRed",    colorRed, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorGreen",  colorGreen, N, true));
        sourceBuffers.push_back(SourceDestBuffer(imf, "colorBlue",   colorBlue, N, true));

        /// Write source buffers into CompressedVector
        {
            CompressedVectorWriter writer = points.writer(sourceBuffers);
            writer.write(N);
        }

        imf.close();
    } catch (char* s) {
        cerr << "Got an char* exception: " << s << endl;
    } catch (const char* s) {
        cerr << "Got an const char* exception: " << s << endl;
    } catch (...) {
        cerr << "Got an exception" << endl;
    }
    return(0);
}
