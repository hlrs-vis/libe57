/*
 * las2e57.cpp - convert LAS format file to E57 format.
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
#include "LASReader.h"
#include "time_conversion.h"  // code from Essential GNSS Project

#include <math.h>
#include <fstream>  // std::ifstream
#include <iostream>
#include <iomanip>
#include <map>

#include <Rpc.h>  // for UuidCreate
#pragma comment(lib, "Rpcrt4.lib")

using namespace std;
using namespace e57;

/// Define the following symbols to get various amounts of debug printing:
//#define E57_VERBOSE 1
//#define E57_MAX_VERBOSE 1

/// Declare local functions:
ustring guidUnparse(uint32_t data1, uint16_t data2, uint16_t data3, uint8_t data4[8]);
ustring gpsTimeUnparse(double gpsTime);

//================================================================

void usage(ustring msg)
{
    cerr << "Usage: " << msg << endl;
    cerr << "    las2e57 [options] <las_file> <e57_file>" << endl;
    cerr << "    options:" << endl;
    cerr << "        -unusedNotOptional         completely zero fields are forced to be included in output file even though they could be discarded" <<endl;
//??? uncomment following line when waveform support is implemented
//???    cerr << "        -stripWaveforms            don't include waveforms in output file (included by default)." << endl;
    cerr << endl;
    cerr << "    For example:" << endl;
    cerr << "        las2e57 scan0001.las scan0001.e57" << endl;
    cerr << endl;
}

//================================================================

struct CommandLineOptions {
    ustring inputFileName;
    ustring outputFileName;
    bool    unusedNotOptional;
    bool    stripWaveforms;

            CommandLineOptions():unusedNotOptional(false),stripWaveforms(false){};
    void    parse(int argc, char** argv);
};


void CommandLineOptions::parse(int argc, char** argv)
{
    /// Skip program name
    argc--; argv++;

    for (; argc > 0 && *argv[0] == '-'; argc--,argv++) {
        if (strcmp(argv[0], "-unusedNotOptional") == 0)
            unusedNotOptional = true;
        else if (strcmp(argv[0], "-stripWaveforms") == 0)
            stripWaveforms = true;
        else {
            usage(ustring("unknown option: ") + argv[0]);
            throw EXCEPTION("unknown option"); //??? UsageException
        }
    }

	if (argc != 2) {
        usage("wrong number of command line arguments");
        throw EXCEPTION("wrong number of command line arguments"); //??? UsageException
    }
    
    inputFileName = argv[0];
    outputFileName = argv[1];
}

//================================================================

/// Information about position of waveforms needed for writing point records.
struct WaveformDatabase {

};

//================================================================

struct BoundingBox {
    bool    notEmpty;
    double  minimum[3];
    double  maximum[3];

            BoundingBox();
    void    addPoint(double coords[3]);
    void    dump(int indent = 0, std::ostream& os = std::cout);
};

BoundingBox::BoundingBox()
{
    notEmpty = false;
    for (unsigned i = 0; i < 3; i++) {
        minimum[i] = 0.0;
        maximum[i] = 0.0;
    }
}

void BoundingBox::addPoint(double coords[3])
{
    if (notEmpty) {
        for (unsigned i = 0; i < 3; i++) {
            if (coords[i] < minimum[i])
                minimum[i] = coords[i];
            if (coords[i] > maximum[i])
                maximum[i] = coords[i];
        }
    } else {
        notEmpty = true;
        for (unsigned i = 0; i < 3; i++) {
            minimum[i] = coords[i];
            maximum[i] = coords[i];
        }
    }
}
        
void BoundingBox::dump(int indent, std::ostream& os)
{
    os << space(indent) << "notEmpty:     " << notEmpty << endl;
    for (unsigned i = 0; i < 3; i++)
        os << space(indent) << "coords[" << i << "]: minimum=" << minimum[i] << " maximum=" << maximum[i] << endl;
}

//================================================================

struct UseInfo {
    bool    gotPoint;
    bool    intensityUsed;
    bool    returnNumberUsed;
    bool    numberOfReturnsUsed;
    bool    scanDirectionFlagUsed;
    bool    edgeOfFlightLineUsed;
    bool    classificationUsed;
    bool    syntheticUsed;
    bool    keyPointUsed;
    bool    withheldUsed;
    bool    scanAngleRankUsed;
    bool    fileMarkerUsed;
    bool    userBitFieldUsed;
    bool    userDataUsed;
    bool    pointSourceIdUsed;
    bool    gpsTimeUsed;
    bool    redUsed;
    bool    greenUsed;
    bool    blueUsed;
    bool    wavePacketDescriptorIndexUsed;
    bool    byteOffsetToWaveformDataUsed;
    bool    waveformPacketSizeInBytesUsed;
    bool    returnPointWaveformLocationUsed;
    bool    xTUsed;
    bool    yTUsed;
    bool    zTUsed;

    int64_t maximumColumnIndex;
    int64_t maximumReturnIndex;
    int64_t maximumReturnCount;
    int64_t minimumClassification;
    int64_t maximumClassification;
    int64_t minimumScanAngleRank;
    int64_t maximumScanAngleRank;
    double  minimumGpsTime;
    double  maximumGpsTime;
    int64_t minimumX;
    int64_t maximumX;
    int64_t minimumY;
    int64_t maximumY;
    int64_t minimumZ;
    int64_t maximumZ;


            UseInfo();
    void    processPoint(LASPublicHeaderBlock& hdr, LASPointDataRecord& point, int64_t columnIndex);
    void    processPoint(LASPublicHeaderBlock& hdr, LASPointDataRecord& point);
    void    dump(int indent = 0, std::ostream& os = std::cout);
};

UseInfo::UseInfo()
{
    gotPoint = false;
    intensityUsed = false;
    returnNumberUsed = false;
    numberOfReturnsUsed = false;
    scanDirectionFlagUsed = false;
    edgeOfFlightLineUsed = false;
    classificationUsed = false;
    syntheticUsed = false;
    keyPointUsed = false;
    withheldUsed = false;
    scanAngleRankUsed = false;
    fileMarkerUsed = false;
    userBitFieldUsed = false;
    userDataUsed = false;
    pointSourceIdUsed = false;
    gpsTimeUsed = false;
    redUsed = false;
    greenUsed = false;
    blueUsed = false;
    wavePacketDescriptorIndexUsed = false;
    byteOffsetToWaveformDataUsed = false;
    waveformPacketSizeInBytesUsed = false;
    returnPointWaveformLocationUsed = false;
    xTUsed = false;
    yTUsed = false;
    zTUsed = false;
    maximumColumnIndex = 0;
    maximumReturnIndex = 0;
    maximumReturnCount = 0;
    minimumClassification = INT64_MAX;
    maximumClassification = 0;
    minimumScanAngleRank = INT64_MAX;
    maximumScanAngleRank = INT64_MIN;
    minimumGpsTime = DOUBLE_MAX;
    maximumGpsTime = -DOUBLE_MAX;
    minimumX = INT64_MAX;
    maximumX = INT64_MIN;
    minimumY = INT64_MAX;
    maximumY = INT64_MIN;
    minimumZ = INT64_MAX;
    maximumZ = INT64_MIN;
}

void UseInfo::processPoint(LASPublicHeaderBlock& hdr, LASPointDataRecord& point, int64_t columnIndex)
{
    gotPoint = true;
    if (point.intensity != 0)
        intensityUsed = true;
    if (point.returnNumber != 0)
        returnNumberUsed = true;
    if (point.numberOfReturns != 0)
        numberOfReturnsUsed = true;
    if (point.scanDirectionFlag != 0)
        scanDirectionFlagUsed = true;
    if (point.edgeOfFlightLine != 0)
        edgeOfFlightLineUsed = true;
    if (point.classification != 0)
        classificationUsed = true;
    if (point.synthetic != 0)
        syntheticUsed = true;
    if (point.keyPoint != 0)
        keyPointUsed = true;
    if (point.withheld != 0)
        withheldUsed = true;
    if (point.scanAngleRank != 0)
        scanAngleRankUsed = true;
    if (point.fileMarker != 0)
        fileMarkerUsed = true;
    if (point.userBitField != 0)
        userBitFieldUsed = true;
    if (point.userData != 0)
        userDataUsed = true;
    if (point.pointSourceId != 0)
        pointSourceIdUsed = true;
    if (point.gpsTime != 0)
        gpsTimeUsed = true;
    if (point.red != 0)
        redUsed = true;
    if (point.green != 0)
        greenUsed = true;
    if (point.blue != 0)
        blueUsed = true;
    if (point.wavePacketDescriptorIndex != 0)
        wavePacketDescriptorIndexUsed = true;
    if (point.byteOffsetToWaveformData != 0)
        byteOffsetToWaveformDataUsed = true;
    if (point.waveformPacketSizeInBytes != 0)
        waveformPacketSizeInBytesUsed = true;
    if (point.returnPointWaveformLocation != 0)
        returnPointWaveformLocationUsed = true;
    if (point.xT != 0)
        xTUsed = true;
    if (point.yT != 0)
        yTUsed = true;
    if (point.zT != 0)
        zTUsed = true;

    if (columnIndex > maximumColumnIndex)
        maximumColumnIndex = columnIndex;
    if (point.returnNumber > maximumReturnIndex)
        maximumReturnIndex = point.returnNumber;
    if (point.numberOfReturns > maximumReturnCount)
        maximumReturnCount = point.numberOfReturns;
    if (point.classification < minimumClassification)
        minimumClassification = point.classification;
    if (point.classification > maximumClassification)
        maximumClassification = point.classification;
    if (point.scanAngleRank < minimumScanAngleRank)
        minimumScanAngleRank = point.scanAngleRank;
    if (point.scanAngleRank > maximumScanAngleRank)
        maximumScanAngleRank = point.scanAngleRank;
    if (point.gpsTime < minimumGpsTime)
        minimumGpsTime = point.gpsTime;
    if (point.gpsTime > maximumGpsTime)
        maximumGpsTime = point.gpsTime;
    if (point.x < minimumX)
        minimumX = point.x;
    if (point.x > maximumX)
        maximumX = point.x;
    if (point.y < minimumY)
        minimumY = point.y;
    if (point.y > maximumY)
        maximumY = point.y;
    if (point.z < minimumZ)
        minimumZ = point.z;
    if (point.z > maximumZ)
        maximumZ = point.z;
}

void UseInfo::dump(int indent, std::ostream& os)
{
    os << space(indent) << "gotPoint:               " << gotPoint << endl;
    os << space(indent) << "intensityUsed:          " << intensityUsed << endl;
    os << space(indent) << "returnNumberUsed:       " << returnNumberUsed << endl;
    os << space(indent) << "numberOfReturnsUsed:    " << numberOfReturnsUsed << endl;
    os << space(indent) << "scanDirectionFlagUsed:  " << scanDirectionFlagUsed << endl;
    os << space(indent) << "edgeOfFlightLineUsed:   " << edgeOfFlightLineUsed << endl;
    os << space(indent) << "classificationUsed:     " << classificationUsed << endl;
    os << space(indent) << "syntheticUsed:          " << syntheticUsed << endl;
    os << space(indent) << "keyPointUsed:           " << keyPointUsed << endl;
    os << space(indent) << "withheldUsed:           " << withheldUsed << endl;
    os << space(indent) << "scanAngleRankUsed:      " << scanAngleRankUsed << endl;
    os << space(indent) << "fileMarkerUsed:         " << fileMarkerUsed << endl;
    os << space(indent) << "userBitFieldUsed:       " << userBitFieldUsed << endl;
    os << space(indent) << "userDataUsed:           " << userDataUsed << endl;
    os << space(indent) << "pointSourceIdUsed:      " << pointSourceIdUsed << endl;
    os << space(indent) << "gpsTimeUsed:            " << gpsTimeUsed << endl;
    os << space(indent) << "redUsed:                " << redUsed << endl;
    os << space(indent) << "greenUsed:              " << greenUsed << endl;
    os << space(indent) << "blueUsed:               " << blueUsed << endl;
    os << space(indent) << "wavePacketDescriptorIndexUsed:   " << wavePacketDescriptorIndexUsed << endl;
    os << space(indent) << "byteOffsetToWaveformDataUsed:    " << byteOffsetToWaveformDataUsed << endl;
    os << space(indent) << "waveformPacketSizeInBytesUsed:   " << waveformPacketSizeInBytesUsed << endl;
    os << space(indent) << "returnPointWaveformLocationUsed: " << returnPointWaveformLocationUsed << endl;
    os << space(indent) << "xTUsed:                 " << xTUsed << endl;
    os << space(indent) << "yTUsed:                 " << yTUsed << endl;
    os << space(indent) << "zTUsed:                 " << zTUsed << endl;
    os << space(indent) << "maximumColumnIndex:     " << maximumColumnIndex << endl;
    os << space(indent) << "maximumReturnIndex:     " << maximumReturnIndex << endl;
    os << space(indent) << "maximumReturnCount:     " << maximumReturnCount << endl;
    os << space(indent) << "minimumClassification:  " << minimumClassification << endl;
    os << space(indent) << "maximumClassification:  " << maximumClassification << endl;
    os << space(indent) << "minimumScanAngleRank:   " << minimumScanAngleRank << endl;
    os << space(indent) << "maximumScanAngleRank:   " << maximumScanAngleRank << endl;
    os << space(indent) << "minimumGpsTime:         " << minimumGpsTime << " (" << gpsTimeUnparse(minimumGpsTime) << ")" << endl;
    os << space(indent) << "maximumGpsTime:         " << maximumGpsTime << " (" << gpsTimeUnparse(maximumGpsTime) << ")" << endl;
    os << space(indent) << "minimumX:               " << minimumX << endl;
    os << space(indent) << "maximumX:               " << maximumX << endl;
    os << space(indent) << "minimumY:               " << minimumY << endl;
    os << space(indent) << "maximumY:               " << maximumY << endl;
    os << space(indent) << "minimumZ:               " << minimumZ << endl;
    os << space(indent) << "maximumZ:               " << maximumZ << endl;
}

//================================================================

struct GroupRecord {
    int64_t     id;
    int64_t     count;
    int64_t     startIndex;
    BoundingBox bbox;

                GroupRecord(int64_t id);
    void        addMember(double coords[3], int64_t recordIndex);
    void        dump(int indent = 0, std::ostream& os = std::cout);
};

GroupRecord::GroupRecord(int64_t id_arg = 0)
: id(id_arg),
  count(0),
  startIndex(0),
  bbox()
{
}

void GroupRecord::addMember(double coords[3], int64_t recordIndex)
{
    if (count == 0) {
        count = 1;
        startIndex = recordIndex;
        bbox.addPoint(coords);
    } else {
        count++;
        if (recordIndex < startIndex)
            startIndex = recordIndex;
        bbox.addPoint(coords);
    }
}

void GroupRecord::dump(int indent, std::ostream& os)
{
    os << space(indent) << "id:         " << id << endl;
    os << space(indent) << "count:      " << count << endl;
    os << space(indent) << "startIndex: " << startIndex << endl;
    os << space(indent) << "bbox:" << endl;
    bbox.dump(indent+4, os);    
}

//================================================================

struct DenseGroupingScheme {
    bool    isFixedSize;
    bool    areMembersContiguous;
    int64_t minimumId;
    int64_t maximumId;
    vector<GroupRecord> groups;

            DenseGroupingScheme(int64_t minimumId, int64_t maximumId, bool isFixedSize = true, bool areMembersContiguous = false);
    void    addMember(int64_t id, double coords[3], int64_t recordIndex);
    void    write(ImageFile imf, StructureNode groupingSchemesNode, ustring idElementName, ustring description);
    void    dump(int indent = 0, std::ostream& os = std::cout);
};

DenseGroupingScheme::DenseGroupingScheme(int64_t minimumId_arg, int64_t maximumId_arg, bool isFixedSize_arg, bool areMembersContiguous_arg)
: minimumId(minimumId_arg),
  maximumId(maximumId_arg),
  isFixedSize(isFixedSize_arg),
  areMembersContiguous(areMembersContiguous_arg),
  groups(0)
{
    if (isFixedSize) {
        groups.resize(static_cast<size_t>(maximumId - minimumId + 1));
        for (unsigned i = 0; i < groups.size(); i++)
            groups[i].id = minimumId + i;
    }
}

void DenseGroupingScheme::addMember(int64_t id, double coords[3], int64_t recordIndex)
{
    if (id < minimumId || maximumId < id)
        throw EXCEPTION("group identifier out of bounds");
    if (!isFixedSize && id - minimumId + 1 > groups.size()) {
        unsigned oldSize = groups.size();
        groups.resize(static_cast<size_t>(id - minimumId + 1));
        for (unsigned i = oldSize; i < groups.size(); i++)
            groups[i].id = minimumId + i;
    }
    groups[static_cast<size_t>(id - minimumId)].addMember(coords, recordIndex);
}

void DenseGroupingScheme::write(ImageFile imf, StructureNode groupingSchemeNode, ustring idElementName, ustring description)
{
    groupingSchemeNode.set("idElementName", StringNode(imf, idElementName));
    groupingSchemeNode.set("description", StringNode(imf, description));
    
    StructureNode proto = StructureNode(imf);
    vector<SourceDestBuffer> sourceBuffers;

    /// If identifiers don't start at zero, add them to write, pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/identifier"
    if (minimumId != 0) {
        /// Prepare prototype and source buffers for pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/identifier"
        proto.set("identifier",  IntegerNode(imf, 0, minimumId, maximumId));
        sourceBuffers.push_back(SourceDestBuffer(imf, "identifier", &(groups[0].id), groups.size(), false, false, sizeof(GroupRecord)));
    }

    /// Find the maximum point count and startPointIndex in all the groups (will save a few bytes in the file).
    int64_t pointCountMax = 0;
    int64_t startPointIndexMax = 0;
    for (unsigned i = 0; i < groups.size(); i++) {
        if (groups[i].count > pointCountMax)
            pointCountMax = groups[i].count;
        if (groups[i].startIndex > startPointIndexMax)
            startPointIndexMax = groups[i].startIndex;
    }

    /// Add pointCount, pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/pointCount"
    proto.set("pointCount",  IntegerNode(imf, 0, 0, pointCountMax));
    sourceBuffers.push_back(SourceDestBuffer(imf, "pointCount", &(groups[0].count), groups.size(), false, false, sizeof(GroupRecord)));

    /// If group members are contiguous, add startPointIndex, pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/startPointIndex"
    if (areMembersContiguous) {
        proto.set("startPointIndex",  IntegerNode(imf, 0, 0, startPointIndexMax));
        sourceBuffers.push_back(SourceDestBuffer(imf, "startPointIndex", &(groups[0].startIndex), groups.size(), false, false, sizeof(GroupRecord)));
    }

    /// Add bounding box, pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/boundingBox/xMinimum" etc...
    StructureNode bbox = StructureNode(imf);
    proto.set("boundingBox", bbox);
    bbox.set("xMinimum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/xMinimum", &(groups[0].bbox.minimum[0]), groups.size(), false, false, sizeof(GroupRecord)));
    bbox.set("xMaximum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/xMaximum", &(groups[0].bbox.maximum[0]), groups.size(), false, false, sizeof(GroupRecord)));
    bbox.set("yMinimum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/yMinimum", &(groups[0].bbox.minimum[1]), groups.size(), false, false, sizeof(GroupRecord)));
    bbox.set("yMaximum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/yMaximum", &(groups[0].bbox.maximum[1]), groups.size(), false, false, sizeof(GroupRecord)));
    bbox.set("zMinimum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/zMinimum", &(groups[0].bbox.minimum[2]), groups.size(), false, false, sizeof(GroupRecord)));
    bbox.set("zMaximum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/zMaximum", &(groups[0].bbox.maximum[2]), groups.size(), false, false, sizeof(GroupRecord)));

    /// Make empty codecs vector for use in creating groups CompressedVector.
    /// If this vector is empty, it is assumed that all fields will use the BitPack codec.
    VectorNode codecs = VectorNode(imf, true);

    /// Create CompressedVector for storing groups.  Path Name: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups"
    /// We use the prototype and empty codecs tree from above.
    CompressedVectorNode groupsNode = CompressedVectorNode(imf, proto, codecs);
    groupingSchemeNode.set("groups", groupsNode);

    /// Write source buffers into CompressedVector
    {
        CompressedVectorWriter writer = groupsNode.writer(sourceBuffers);
        writer.write(groups.size());
        writer.close();
    }        
}

void DenseGroupingScheme::dump(int indent, std::ostream& os)
{
    os << space(indent) << "isFixedSize:           " << isFixedSize << endl;
    os << space(indent) << "areMembersContiguous:  " << areMembersContiguous << endl;
    os << space(indent) << "minimumId:             " << minimumId << endl;
    os << space(indent) << "maximumId:             " << maximumId << endl;
    unsigned i;
    for (i = 0; i < groups.size() && i < 200; i++) {
        os << space(indent) << "groups[" << i << "]:" << endl;
        groups[i].dump(indent+4, os);
    }
    if (i < groups.size())
        os << space(indent) << groups.size()-i << " groups unprinted..." << endl;
}

//================================================================

struct SparseGroupingScheme {
    bool    areMembersContiguous;
    map<int64_t, GroupRecord> groups;

            SparseGroupingScheme(bool areMembersContiguous = false);
    void    addMember(int64_t id, double coords[3], int64_t recordIndex);
    void    write(ImageFile imf, StructureNode groupingSchemesNode, ustring idElementName, ustring description);
    void    dump(int indent = 0, std::ostream& os = std::cout);
};

SparseGroupingScheme::SparseGroupingScheme(bool areMembersContiguous_arg)
: areMembersContiguous(areMembersContiguous_arg)
{
}

void SparseGroupingScheme::addMember(int64_t id, double coords[3], int64_t recordIndex)
{
    groups[id].addMember(coords, recordIndex);
}

void SparseGroupingScheme::write(ImageFile imf, StructureNode groupingSchemeNode, ustring idElementName, ustring description)
{
    groupingSchemeNode.set("idElementName", StringNode(imf, idElementName));
    groupingSchemeNode.set("description", StringNode(imf, description));
    
    StructureNode proto = StructureNode(imf);
    vector<SourceDestBuffer> sourceBuffers;

    /// Gather all groups into a vector so can write attributes as a block
    vector<GroupRecord> groupVector;
    map<int64_t, GroupRecord>::iterator iter;
    for (iter=groups.begin(); iter != groups.end(); ++iter)
        groupVector.push_back(iter->second);

    /// Find the maximum point count and startPointIndex in all the groups (will save a few bytes in the file).
    int64_t pointCountMax = 0;
    int64_t startPointIndexMax = 0;
    int64_t minimumId = INT64_MIN;
    int64_t maximumId = INT64_MAX;
    for (unsigned i = 0; i < groupVector.size(); i++) {
        if (groupVector[i].count > pointCountMax)
            pointCountMax = groupVector[i].count;
        if (groupVector[i].startIndex > startPointIndexMax)
            startPointIndexMax = groupVector[i].startIndex;
        if (i == 0 || groupVector[i].id < minimumId)
            minimumId = groupVector[i].id;
        if (i == 0 || groupVector[i].id > maximumId)
            maximumId = groupVector[i].id;
    }

    /// Prepare prototype and source buffers for pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/identifier"
    proto.set("identifier",  IntegerNode(imf, 0, minimumId, maximumId));
    sourceBuffers.push_back(SourceDestBuffer(imf, "identifier", &(groupVector[0].id), groupVector.size(), false, false, sizeof(GroupRecord)));

    /// Add pointCount, pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/pointCount"
    proto.set("pointCount",  IntegerNode(imf, 0, 0, pointCountMax));
    sourceBuffers.push_back(SourceDestBuffer(imf, "pointCount", &(groupVector[0].count), groupVector.size(), false, false, sizeof(GroupRecord)));

    /// If group members are contiguous, add startPointIndex, pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/startPointIndex"
    if (areMembersContiguous) {
        proto.set("startPointIndex",  IntegerNode(imf, 0, 0, startPointIndexMax));
        sourceBuffers.push_back(SourceDestBuffer(imf, "startPointIndex", &(groupVector[0].startIndex), groupVector.size(), false, false, sizeof(GroupRecord)));
    }

    /// Add bounding box, pathname: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups/0/boundingBox/xMinimum" etc...
    StructureNode bbox = StructureNode(imf);
    proto.set("boundingBox", bbox);
    bbox.set("xMinimum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/xMinimum", &(groupVector[0].bbox.minimum[0]), groupVector.size(), false, false, sizeof(GroupRecord)));
    bbox.set("xMaximum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/xMaximum", &(groupVector[0].bbox.maximum[0]), groupVector.size(), false, false, sizeof(GroupRecord)));
    bbox.set("yMinimum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/yMinimum", &(groupVector[0].bbox.minimum[1]), groupVector.size(), false, false, sizeof(GroupRecord)));
    bbox.set("yMaximum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/yMaximum", &(groupVector[0].bbox.maximum[1]), groupVector.size(), false, false, sizeof(GroupRecord)));
    bbox.set("zMinimum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/zMinimum", &(groupVector[0].bbox.minimum[2]), groupVector.size(), false, false, sizeof(GroupRecord)));
    bbox.set("zMaximum", FloatNode(imf, 0.0, E57_DOUBLE));
    sourceBuffers.push_back(SourceDestBuffer(imf, "boundingBox/zMaximum", &(groupVector[0].bbox.maximum[2]), groupVector.size(), false, false, sizeof(GroupRecord)));

    /// Make empty codecs vector for use in creating groups CompressedVector.
    /// If this vector is empty, it is assumed that all fields will use the BitPack codec.
    VectorNode codecs = VectorNode(imf, true);

    /// Create CompressedVector for storing groups.  Path Name: "/images3D/0/groupingSchemes/<groupingSchemeName>/groups"
    /// We use the prototype and empty codecs tree from above.
    CompressedVectorNode groupsNode = CompressedVectorNode(imf, proto, codecs);
    groupingSchemeNode.set("groups", groupsNode);

    /// Write source buffers into CompressedVector
    {
        CompressedVectorWriter writer = groupsNode.writer(sourceBuffers);
        writer.write(groupVector.size());
        writer.close();
    }        
}

void SparseGroupingScheme::dump(int indent, std::ostream& os)
{
    os << space(indent) << "areMembersContiguous:  " << areMembersContiguous << endl;
    map<int64_t, GroupRecord>::iterator iter;
    for (iter=groups.begin(); iter != groups.end(); ++iter) {
        os << space(indent) << "groups[" << iter->first << "]:" << endl;
        iter->second.dump(indent+4, os);
    }
}

//================================================================

class GroupingSchemes {
public:
            GroupingSchemes(UseInfo useInfo);
    void    addMember(LASPublicHeaderBlock& hdr, LASPointDataRecord& point, int64_t recordIndex, int64_t columnIndex);
    void    write(ImageFile imf);
    void    dump(int indent = 0, std::ostream& os = std::cout);

protected:
    UseInfo              useInfo_;
    DenseGroupingScheme  groupByColumn_;              // fieldName: columnIndex,          ids: >=0, contiguous members
    DenseGroupingScheme  groupByReturnIndex_;         // fieldName: returnIndex,          ids: 0 to 7       //??? in base std, not las extension?
    DenseGroupingScheme  groupByScanDirectionFlag_;   // fieldName: las:scanDirectionFlag,ids: 0 to 1
    DenseGroupingScheme  groupByEdgeOfFlightLine_;    // fieldName: las:edgeOfFlightLine, ids: 0 to 1
    DenseGroupingScheme  groupByClassification_;      // fieldName: las:classification,   ids: 0 to 1
    DenseGroupingScheme  groupBySynthetic_;           // fieldName: las:synthetic,        ids: 0 to 1
    DenseGroupingScheme  groupByKeyPoint_;            // fieldName: las:keyPoint,         ids: 0 to 1
    DenseGroupingScheme  groupByWithheld_;            // fieldName: las:withheld,         ids: 0 to 1
    DenseGroupingScheme  groupByScanAngleRank_;       // fieldName: las:scanAngleRank,    ids: -90 to 90
    SparseGroupingScheme groupByPointSourceId_;       // fieldName: las:pointSourceId,    ids: 0 to 64K, sparse ids
};

GroupingSchemes::GroupingSchemes(UseInfo useInfo)
                         : useInfo_(useInfo),
  groupByColumn_(0, useInfo.maximumColumnIndex, false, true),
  groupByReturnIndex_(0, useInfo.maximumReturnIndex),
  groupByScanDirectionFlag_(0, 1),
  groupByEdgeOfFlightLine_(0, 1),
  groupByClassification_(useInfo.minimumClassification, useInfo.maximumClassification),
  groupBySynthetic_(0, 1),
  groupByKeyPoint_(0, 1),
  groupByWithheld_(0, 1),
  groupByScanAngleRank_(useInfo.minimumScanAngleRank, useInfo.maximumScanAngleRank),
  groupByPointSourceId_()
{}

void GroupingSchemes::addMember(LASPublicHeaderBlock& hdr, LASPointDataRecord& point, int64_t recordIndex,int64_t columnIndex)
{
    double coords[3];
    coords[0] = hdr.xScaleFactor * point.x + hdr.xOffset;
    coords[1] = hdr.yScaleFactor * point.y + hdr.yOffset;
    coords[2] = hdr.zScaleFactor * point.z + hdr.zOffset;
    
    if (useInfo_.maximumColumnIndex > 0)
        groupByColumn_.addMember(columnIndex, coords, recordIndex);
    if (useInfo_.returnNumberUsed)
        groupByReturnIndex_.addMember(point.returnNumber, coords, recordIndex);
    if (useInfo_.scanDirectionFlagUsed)
        groupByScanDirectionFlag_.addMember(point.scanDirectionFlag, coords, recordIndex);
    if (useInfo_.edgeOfFlightLineUsed)
        groupByEdgeOfFlightLine_.addMember(point.edgeOfFlightLine, coords, recordIndex);
    if (useInfo_.classificationUsed)
        groupByClassification_.addMember(point.classification, coords, recordIndex);
    if (useInfo_.syntheticUsed)
        groupBySynthetic_.addMember(point.synthetic, coords, recordIndex);
    if (useInfo_.keyPointUsed)
        groupByKeyPoint_.addMember(point.keyPoint, coords, recordIndex);
    if (useInfo_.withheldUsed)
        groupByWithheld_.addMember(point.withheld, coords, recordIndex);
    if (useInfo_.scanAngleRankUsed)
        groupByScanAngleRank_.addMember(point.scanAngleRank, coords, recordIndex);
    if (useInfo_.pointSourceIdUsed)
        groupByPointSourceId_.addMember(point.pointSourceId, coords, recordIndex);
}

void GroupingSchemes::write(ImageFile imf)
{
    /// Add groupingSchemes structure in per-scan area, pathname: "/images3D/0/groupingSchemes"
    StructureNode scan0 = StructureNode(imf.root().get("/images3D/0"));
    StructureNode groupingSchemesNode = StructureNode(imf);
    scan0.set("groupingSchemes", groupingSchemesNode);

    if (useInfo_.maximumColumnIndex > 0) {
        /// Add groupByColumn scheme, pathname: "/images3D/0/groupingSchemes/groupByColumn"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("groupByColumn", groupingSchemeNode);
        groupByColumn_.write(imf, groupingSchemeNode, "columnIndex", 
                             "Points are grouped into scanlines or columns.  Columns are delimited by a change in scanDirection.");  //??? handle startIndex
    }
    if (useInfo_.returnNumberUsed) {
        /// Add groupByReturnIndex scheme, pathname: "/images3D/0/groupingSchemes/las:groupByReturnIndex"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByReturnIndex", groupingSchemeNode);
        groupByReturnIndex_.write(imf, groupingSchemeNode, "returnIndex", 
                                  "Points are grouped into return pulse positions. returnIndex=0 is first return.");
    }
    if (useInfo_.scanDirectionFlagUsed) {
        /// Add groupByScanDirectionFlag scheme, pathname: "/images3D/0/groupingSchemes/las:groupByScanDirectionFlag"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByScanDirectionFlag", groupingSchemeNode);
        groupByScanDirectionFlag_.write(imf, groupingSchemeNode, "las:scanDirectionFlag", 
                                        "Points are grouped into positive (dir=1) or negative (dir=0) scan direction.  Positive scan direction moves from left side of in-track direction to right side.");
    }
    if (useInfo_.edgeOfFlightLineUsed) {
        /// Add groupByEdgeOfFlightLine scheme, pathname: "/images3D/0/groupingSchemes/las:groupByEdgeOfFlightLine"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByEdgeOfFlightLine", groupingSchemeNode);
        groupByEdgeOfFlightLine_.write(imf, groupingSchemeNode, "las:edgeOfFlightLine", 
                                       "Points are grouped edge and non-edge points in the scan lines.");
    }
    if (useInfo_.classificationUsed) {
        /// Add groupByClassification scheme, pathname: "/images3D/0/groupingSchemes/las:groupByClassification"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByClassification", groupingSchemeNode);
        groupByClassification_.write(imf, groupingSchemeNode, "las:classification", 
                                     "Points are grouped into classes of surface that the beam illuminated.  Point classes are standardized by ASPRS.");
    }
    if (useInfo_.syntheticUsed) {
        /// Add groupBySynthetic scheme, pathname: "/images3D/0/groupingSchemes/las:groupBySynthetic"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupBySynthetic", groupingSchemeNode);
        groupBySynthetic_.write(imf, groupingSchemeNode, "las:synthetic", 
                                "Points are grouped into synthetic and non-synthetic.  Synthetic points are created by a technique other than LIDAR collection.");
    }
    if (useInfo_.keyPointUsed) {
        /// Add groupByKeyPoint scheme, pathname: "/images3D/0/groupingSchemes/las:groupByKeyPoint"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByKeyPoint", groupingSchemeNode);
        groupByKeyPoint_.write(imf, groupingSchemeNode, "las:keyPoint",
                               "Points are grouped into key and non-key points.  A key point should generally not be withheld in a thinning algorithm.");
    }
    if (useInfo_.withheldUsed) {
        /// Add groupByWithheld scheme, pathname: "/images3D/0/groupingSchemes/las:groupByWithheld"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByWithheld", groupingSchemeNode);
        groupByWithheld_.write(imf, groupingSchemeNode, "las:withheld",
                               "Points are grouped withheld and non-withheld.  Withheld points should not be included in processing (synonymous with Deleted)");
    }
    if (useInfo_.scanAngleRankUsed) {
        /// Add groupByScanAngleRank scheme, pathname: "/images3D/0/groupingSchemes/las:groupByScanAngleRank"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByScanAngleRank", groupingSchemeNode);
        groupByScanAngleRank_.write(imf, groupingSchemeNode, "las:scanAngleRank",
                                    "Points are grouped by scan angle rank, which is the angle in degrees from nadir of the beam.  Negative angles being to the left side of the aircraft.");
    }
    if (useInfo_.pointSourceIdUsed) {
        /// Add groupByPointSourceId scheme, pathname: "/images3D/0/groupingSchemes/las:groupByPointSourceId"
        StructureNode groupingSchemeNode = StructureNode(imf);
        groupingSchemesNode.set("las:groupByPointSourceId", groupingSchemeNode);
        groupByPointSourceId_.write(imf, groupingSchemeNode, "las:pointSourceId",
                                    "Points grouped by which file they originated from.  Point source ids are integers that uniquely identify a file within a project.");
    }
}

void GroupingSchemes::dump(int indent, std::ostream& os)
{
    os << space(indent) << "useInfo:" << endl;
    useInfo_.dump(indent+4, os);
    if (useInfo_.maximumColumnIndex > 0) {
        os << space(indent) << "groupByColumn:" << endl;
        groupByColumn_.dump(indent+4, os);
    }
    if (useInfo_.returnNumberUsed) {
        os << space(indent) << "groupByReturnIndex:" << endl;
        groupByReturnIndex_.dump(indent+4, os);
    }
    if (useInfo_.scanDirectionFlagUsed) {
        os << space(indent) << "groupByScanDirectionFlag:" << endl;
        groupByScanDirectionFlag_.dump(indent+4, os);
    }
    if (useInfo_.edgeOfFlightLineUsed) {
        os << space(indent) << "groupByEdgeOfFlightLine:" << endl;
        groupByEdgeOfFlightLine_.dump(indent+4, os);
    }
    if (useInfo_.classificationUsed) {
        os << space(indent) << "groupByClassification:" << endl;
        groupByClassification_.dump(indent+4, os);
    }
    if (useInfo_.syntheticUsed) {
        os << space(indent) << "groupBySynthetic:" << endl;
        groupBySynthetic_.dump(indent+4, os);
    }
    if (useInfo_.keyPointUsed) {
        os << space(indent) << "groupByKeyPoint:" << endl;
        groupByKeyPoint_.dump(indent+4, os);
    }
    if (useInfo_.withheldUsed) {
        os << space(indent) << "groupByWithheld:" << endl;
        groupByWithheld_.dump(indent+4, os);
    }
    if (useInfo_.scanAngleRankUsed) {
        os << space(indent) << "groupByScanAngleRank:" << endl;
        groupByScanAngleRank_.dump(indent+4, os);
    }
    if (useInfo_.pointSourceIdUsed) {
        os << space(indent) << "groupByPointSourceId:" << endl;
        groupByPointSourceId_.dump(indent+4, os);
    }
}

//================================================================

/// Declare local functions:
void findUnusedFields(CommandLineOptions& options, LASReader& lasf, ImageFile imf, UseInfo& useInfo);
void copyPerScanData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, UseInfo& useInfo);
void copyVariableLengthRecords(CommandLineOptions& options, LASReader& lasf, ImageFile imf, WaveformDatabase& waveDb);
void copyWaveformData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, WaveformDatabase& waveDb);
void copyPerPointData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, 
                      UseInfo& useInfo, GroupingSchemes& groupings, WaveformDatabase& waveDb);
void copyPerFileData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, UseInfo& useInfo);
ustring generateUuidString();

void main(int argc, char** argv)
{
#ifdef E57_DEBUG_MEMORY
    /// Turn on leak-checking bits.
    int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    tmpFlag |= _CRTDBG_ALLOC_MEM_DF    |
               _CRTDBG_CHECK_ALWAYS_DF |
               _CRTDBG_CHECK_CRT_DF    |
               _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag( tmpFlag );

    /// Send error messages to stdout
    _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);

    _ASSERTE(_CrtCheckMemory()); //??? check heap ok
#endif

	/// Catch any exceptions thrown.
	try {
        /// Parse the command line into useful structure
        CommandLineOptions options;
        options.parse(argc, argv);

        /// Open existing LAS file for input
        LASReader lasf(options.inputFileName);
        LASPublicHeaderBlock hdr;
        lasf.getHeader(hdr);
#ifdef E57_VERBOSE
        hdr.dump(4);
#endif

        /// Check LAS file is version that this conversion utility supports
        if (hdr.versionMajor != 1 || hdr.versionMinor > 2) {
            cout << "ERROR: can't convert LAS version " << hdr.versionMajor << "." << hdr.versionMinor << " files, aborting." << endl;
            exit(-1);
        }

		/// Create empty E57 file for output
        ImageFile imf(options.outputFileName, "w");

        /// We are using the E57 v1.0 data format standard fieldnames.
        /// The standard fieldnames are used without an extension prefix (in the default namespace).
        /// We explicitly register it for completeness (the reference implementaion would do it for us, if we didn't).
        imf.extensionsAdd("", E57_V1_0_URI);

        /// We will use las extensions to the e57 standard field names.
        /// So register the appropriate URI with the "las" prefix.
        /// Choosing the "las" prefix is a convention, not a requirement.
        /// Any prefix could be used, as long as it is associated with the correct URI.
        imf.extensionsAdd("las", LAS_V1_0_URI);

        WaveformDatabase waveDb;
        UseInfo useInfo;

        /// Do a first pass through the LAS points to see what fields are actually used.
        /// In practice, many "required" fields in an LAS file are filled with zeroes.
        findUnusedFields(options, lasf, imf, useInfo);
#ifdef E57_VERBOSE
        useInfo.dump();
#endif

		copyPerScanData(options, lasf, imf, useInfo);
		copyVariableLengthRecords(options, lasf, imf, waveDb);
        copyWaveformData(options, lasf, imf, waveDb);
        GroupingSchemes groupings(useInfo);
		copyPerPointData(options, lasf, imf, useInfo, groupings, waveDb);
		copyPerFileData(options, lasf, imf, useInfo);
    	groupings.write(imf);

#ifdef E57_MAX_VERBOSE
        groupings.dump();
        imf.dump();
#endif
        lasf.close();
		imf.close();
    } catch (char* s) { //??? use E57Exceptions when implemented
        cout << "ERROR: " << s << endl;  //??? to cerr?
    } catch (const char* s) {
        cout << "ERROR: " << s << endl;
    } catch (...) {
        cout << "ERROR: Got an exception" << endl;
    }

#ifdef E57_DEBUG_MEMORY
    _ASSERTE(_CrtCheckMemory()); //??? check heap ok
#endif
}

//================================================================

void findUnusedFields(CommandLineOptions& options, LASReader& lasf, ImageFile imf, UseInfo& useInfo)
{
    LASPublicHeaderBlock hdr;
    lasf.getHeader(hdr);

    const unsigned N = 10*1024;
    vector<LASPointDataRecord> pointBuffer(N);

    unsigned    totalPointCount = hdr.numberOfPointRecords;
    unsigned    recordCount = 0;
    bool        lastScanDirectionFlag = false;
    int64_t     columnIndex = 0;
    for (unsigned currentRecord = 0; currentRecord < totalPointCount; currentRecord += recordCount) {
        recordCount = min(N, totalPointCount - currentRecord);
#ifdef E57_MAX_VERBOSE
        cerr << "reading " << recordCount << endl;
#endif
        lasf.readPoints(&pointBuffer[0], currentRecord, recordCount);

        /// Process each point and note fields that are used (have non-zero values).
        for (unsigned i = 0; i < recordCount; i++) {
            /// Look for reverses in scan direction to delimit columns
            if (currentRecord != 0 || i != 0) {
                if (lastScanDirectionFlag != pointBuffer[i].scanDirectionFlag)
                    columnIndex++;
            }
            lastScanDirectionFlag = pointBuffer[i].scanDirectionFlag;

            useInfo.processPoint(hdr, pointBuffer[i], columnIndex);
        }
     }

#ifdef E57_VERBOSE
    cout << "searched " << totalPointCount << " points" << endl;
#endif
}

//================================================================

void copyPerScanData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, UseInfo& useInfo)
{
    LASPublicHeaderBlock hdr;
    lasf.getHeader(hdr);

    StructureNode root = imf.root();

    /// Create 3D image area, if not already defined.
    /// Path name: "/images3D"
    if (!root.isDefined("/images3D"))
        root.set("images3D", VectorNode(imf, true));
    VectorNode images3d = VectorNode(root.get("/images3D"));

    /// Create first scan area, if not already defined.
    /// Path name: "/images3D/0"
    if (!images3d.isDefined("0"))
        images3d.set(0, StructureNode(imf));
    StructureNode scan0 = StructureNode(images3d.get(0));

    /// Save fileSourceId
    /// Path name: "/images3D/0/las:fileSourceId"
    scan0.set("las:fileSourceId", IntegerNode(imf, hdr.fileSourceId, 0, UINT16_MAX));

    /// gpsTimeType is not stored in E57 file.  If gpsTimeType==true, timeStamp uses offset = +1e9.

    /// Waveform location flags: hdr.waveformDataPacketsInternal and hdr.waveformDataPacketsExternal not stored in E57 file.
    /// They are used to find the waveforms, which are always stored inside the .e57 file.

    /// Save returnNumbersSyntheticallyGenerated
    /// Path name: "/images3D/0/las:returnNumbersSynthetic"
    scan0.set("las:returnNumbersSynthetic", IntegerNode(imf, hdr.returnNumbersSyntheticallyGenerated, 0, 1));

    /// Save projectID in guid format.  Example string format: "3F2504E0-4F89-11D3-9A0C-0305E82C3301"
    /// Path name: "/images3D/0/las:projectId"
    scan0.set("las:projectId", StringNode(imf, guidUnparse(hdr.projectIdGuidData1, hdr.projectIdGuidData2, hdr.projectIdGuidData3, hdr.projectIdGuidData4)));

    /// Save versionMajor and version minor
    /// Path names: "/images3D/0/las:versionMajor", "/images3D/0/las:versionMinor"
    scan0.set("las:versionMajor", IntegerNode(imf, hdr.versionMajor, 0, UINT8_MAX));
    scan0.set("las:versionMinor", IntegerNode(imf, hdr.versionMinor, 0, UINT8_MAX));

    /// Save systemIdentifier as sensorModel
    /// Assuming descriptor of aquisition hardware, but might be name of processing software (can't tell which).
    /// Path name: "/images3D/0/sensorModel"
    if (hdr.systemIdentifier[0] != '\0')
        scan0.set("sensorModel", StringNode(imf, hdr.systemIdentifier));

    /// Save generatingSoftware as softwareVersion
    /// Path name: "/images3D/0/softwareVersion"
    if (hdr.generatingSoftware[0] != '\0')
        scan0.set("softwareVersion", StringNode(imf, hdr.generatingSoftware));

    /// Convert year and day to GPS time, store as file creationDateTime
    /// Use routine from GNSS function library to do conversion.
    /// The LAS fields fileCreationDayOfYear and fileCreationDayOfYear are documented differently
    ///   in various LAS versions.  Assuming this is when the data was acquired.
    /// Might be when file was written by processing software (can't tell).
    /// Path name: "/images3D/0/creationDateTimeStart"
    double creationStartGpsTime = 0.0;
    if (hdr.fileCreationDayOfYear > 0 && hdr.fileCreationYear > 0) {
        unsigned short year = static_cast<unsigned short>(hdr.fileCreationYear);
        unsigned short dayOfYear = static_cast<unsigned short>(hdr.fileCreationDayOfYear);
        unsigned short gpsWeek = 0;
        double gpsTOW = 0.0;
        if (!TIMECONV_GetGPSTimeFromYearAndDayOfYear(year, dayOfYear, &gpsWeek, &gpsTOW))
            throw EXCEPTION("bad year,day");
        double creationStartGpsTime = gpsWeek * SECONDS_IN_WEEK + gpsTOW;
        scan0.set("creationDateTimeStart", FloatNode(imf, creationStartGpsTime, E57_DOUBLE));
    } else {
        switch (hdr.pointDataFormatId) {
            case 1:
            case 3:
            case 4:
            case 5:
                /// Can only use point gpsTime times to deduce acquisition start time if they are absolute.
                /// If don't have gpsTime or if is gps week time, then we don't know when data was acquired.
                if (hdr.gpsTimeType == 1) {
                    /// gpsTime is defined, but sometimes some of the values are junk, so check if reasonable
                    if (useInfo.minimumGpsTime < -1e10 || 1e10 < useInfo.minimumGpsTime) {
                        cout << "***Warning***: range of point gps time is suspicious" << endl;
                        break;
                    }

                    if (options.unusedNotOptional || useInfo.gpsTimeUsed) {
                        /// Use smallest absolute gps time from points as aquisition start.
                        /// Add back in the offset that the LAS standard subtracts out.
                        /// We don't lose resolution in E57 file because timeStamps are always relative to creationDateTimeStart.
                        scan0.set("creationDateTimeStart", FloatNode(imf, useInfo.minimumGpsTime + 1e9, E57_DOUBLE));
                    }
                }
                break;
        }
    }

    /// Since an LAS file writer can legally add stuff to end of public header block, we save whole binary copy in a blob.
    /// This happens when headerSize field is larger than required by standardized fields, but we make copy even if it isn't.
    /// Path name: "/images3D/0/las:publicHeaderBlockData"
    vector<uint8_t> rawHeader = lasf.getRawHeader();
    BlobNode rawHeaderBlob = BlobNode(imf, rawHeader.size());
    scan0.set("las:publicHeaderBlockData", rawHeaderBlob);
    rawHeaderBlob.write(&rawHeader[0], 0LL, static_cast<uint64_t>(rawHeader.size()));

    /// Add bounding box, pathname: "/images3D/0/boundingBox/xMinimum" etc...
    /// Don't use bounding box in LAS header, use the one computed by this program ("guaranteed" to be right).
    StructureNode bbox = StructureNode(imf);
    scan0.set("boundingBox", bbox);
    bbox.set("xMinimum", FloatNode(imf, hdr.xScaleFactor * useInfo.minimumX + hdr.xOffset, E57_DOUBLE));
    bbox.set("xMaximum", FloatNode(imf, hdr.xScaleFactor * useInfo.maximumX + hdr.xOffset, E57_DOUBLE));
    bbox.set("yMinimum", FloatNode(imf, hdr.yScaleFactor * useInfo.minimumY + hdr.yOffset, E57_DOUBLE));
    bbox.set("yMaximum", FloatNode(imf, hdr.yScaleFactor * useInfo.maximumY + hdr.yOffset, E57_DOUBLE));
    bbox.set("zMinimum", FloatNode(imf, hdr.zScaleFactor * useInfo.minimumZ + hdr.zOffset, E57_DOUBLE));
    bbox.set("zMaximum", FloatNode(imf, hdr.zScaleFactor * useInfo.maximumZ + hdr.zOffset, E57_DOUBLE));
}

//================================================================

void copyVariableLengthRecords(CommandLineOptions& options, LASReader& lasf, ImageFile imf, WaveformDatabase& waveDb)
{
    LASPublicHeaderBlock hdr;
    lasf.getHeader(hdr);

    StructureNode root = imf.root();

    /// Create 3D image area, if not already defined.
    /// Path name: "/images3D"
    if (!root.isDefined("/images3D"))
        root.set("images3D", VectorNode(imf, true));
    VectorNode images3d = VectorNode(root.get("/images3D"));

    /// Create first scan area, if not already defined.
    /// Path name: "/images3D/0"
    if (!images3d.isDefined("0"))
        images3d.set(0, StructureNode(imf));
    StructureNode scan0 = StructureNode(images3d.get(0));

    /// Create vector for holding VLRs
    /// Path name: /images3d/0/las:variableLengthRecords
    VectorNode vlrs = VectorNode(imf, true);
    scan0.set("las:variableLengthRecords", vlrs);

    /// Start at beginning of variable length records section
    lasf.rewindVLR();

    LASVariableLengthRecord vlrInfo;
    while (lasf.readNextVLR(vlrInfo)) {
#ifdef E57_MAX_VERBOSE
        vlrInfo.dump(4);
#endif
        /// Create new structure entry to append to vector las:variableLengthRecords
        /// Path name: /images3d/0/las:variableLengthRecords/N
        StructureNode vlr = StructureNode(imf);
        vlrs.append(vlr);

        /// Add userId to vlr
        /// Path name: /images3d/0/las:variableLengthRecords/N/las:userId
        vlr.set("las:userId", StringNode(imf, vlrInfo.userId));

        /// Add recordId to vlr
        /// Path name: /images3d/0/las:variableLengthRecords/N/las:recordId
        vlr.set("las:recordId", IntegerNode(imf, vlrInfo.recordId, UINT16_MIN, UINT16_MAX));

        /// Add recordLengthAfterHeader to vlr
        /// Path name: /images3d/0/las:variableLengthRecords/N/las:recordLengthAfterHeader
        vlr.set("las:recordLengthAfterHeader", IntegerNode(imf, vlrInfo.recordLengthAfterHeader, UINT16_MIN, UINT16_MAX));

        /// Save whole binary record (including header) as a blob
        uint64_t recordSize = static_cast<uint64_t>(vlrInfo.wholeRecordData.size());
        BlobNode recordData = BlobNode(imf, recordSize);
        vlr.set("las:recordData", recordData);
        recordData.write(&vlrInfo.wholeRecordData[0], 0LL, recordSize);

        /// If vlr is waveform packet descriptor, save some more named fields, and record it in waveDb
        if (vlrInfo.userId == "LASF_Spec" && 100 <= vlrInfo.recordId && vlrInfo.recordId < 356) {
            //!!! implement waveform
        }
    }
}

//================================================================

void copyWaveformData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, WaveformDatabase& waveDb)
{
    LASPublicHeaderBlock hdr;
    lasf.getHeader(hdr);

    StructureNode root = imf.root();

    /// Create 3D image area, if not already defined.
    /// Path name: "/images3D"
    if (!root.isDefined("/images3D"))
        root.set("images3D", VectorNode(imf, true));
    VectorNode images3d = VectorNode(root.get("/images3D"));

    /// Create first scan area, if not already defined.
    /// Path name: "/images3D/0"
    if (!images3d.isDefined("0"))
        images3d.set(0, StructureNode(imf));
    StructureNode scan0 = StructureNode(images3d.get(0));

    //??? waveforms not implemented yet
}

//================================================================

void copyPerPointData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, 
                      UseInfo& useInfo, GroupingSchemes& groupings, WaveformDatabase& waveDb)
{
    LASPublicHeaderBlock hdr;
    lasf.getHeader(hdr);

    StructureNode root = imf.root();

    /// Create 3D image area, if not already defined.
    /// Path name: "/images3D"
    if (!root.isDefined("/images3D"))
        root.set("images3D", VectorNode(imf, true));
    VectorNode images3d = VectorNode(root.get("/images3D"));

    /// Create first scan area, if not already defined.
    /// Path name: "/images3D/0"
    if (!images3d.isDefined("0"))
        images3d.set(0, StructureNode(imf));
    StructureNode scan0 = StructureNode(images3d.get(0));

    /// Make empty codecs vector for use in creating points CompressedVector.
    /// If this vector is empty, it is assumed that all fields will use the BitPack codec.
    VectorNode codecs = VectorNode(imf, true);

    /// Make a prototype datatypes that will be stored in points record.
    /// This prototype will be used in creating the points CompressedVector.
    /// The prototype is a flat structure containing each field.
    /// Using this proto in a CompressedVector will form path names like: "/images3D/0/points/0/cartesianX".
    StructureNode proto = StructureNode(imf);
    const unsigned N = 10*1024;
    vector<SourceDestBuffer> sourceBuffers;

    vector<LASPointDataRecord> pointBuffer(N);
    vector<int64_t> columnIndices(N);
    bool writingTimeStamp = false;
    double lasTimeOffset = 0.0;  // amount to add to gpsTime in LAS point record to get absolute GPS time.
    double e57TimeOffset = 0.0;  // amount to add to timeStamp in E57 point record to get absolute GPS time.

    sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianX", &pointBuffer[0].x, N, true, false, sizeof(LASPointDataRecord)));
    proto.set("cartesianX",  ScaledIntegerNode(imf, 0, useInfo.minimumX, useInfo.maximumX, hdr.xScaleFactor, hdr.xOffset));

    sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianY", &pointBuffer[0].y, N, true, false, sizeof(LASPointDataRecord)));
    proto.set("cartesianY",  ScaledIntegerNode(imf, 0, useInfo.minimumY, useInfo.maximumY, hdr.yScaleFactor, hdr.yOffset));

    sourceBuffers.push_back(SourceDestBuffer(imf, "cartesianZ", &pointBuffer[0].z, N, true, false, sizeof(LASPointDataRecord)));
    proto.set("cartesianZ",  ScaledIntegerNode(imf, 0, useInfo.minimumZ, useInfo.maximumZ, hdr.zScaleFactor, hdr.zOffset));

    if (options.unusedNotOptional || useInfo.intensityUsed) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "intensity", &pointBuffer[0].intensity, N, true, false, sizeof(LASPointDataRecord)));
        proto.set("intensity",  IntegerNode(imf, 0, UINT16_MIN, UINT16_MAX)); //!!! get rid of overloads!
    }

    if (useInfo.maximumColumnIndex > 0) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "columnIndex", &columnIndices[0], N, true, false));
        proto.set("columnIndex",  IntegerNode(imf, 0, 0, useInfo.maximumColumnIndex));
    }

    if (options.unusedNotOptional || useInfo.returnNumberUsed) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "returnIndex", &pointBuffer[0].returnNumber, N, true, false, sizeof(LASPointDataRecord)));
        proto.set("returnIndex",  IntegerNode(imf, 0, 0, useInfo.maximumReturnIndex));
    }

    if (options.unusedNotOptional || useInfo.numberOfReturnsUsed) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "returnCount", &pointBuffer[0].numberOfReturns, N, true, false, sizeof(LASPointDataRecord)));
        proto.set("returnCount",  IntegerNode(imf, 0, 0, max(useInfo.maximumReturnCount, useInfo.maximumReturnIndex)));
    }
    if (options.unusedNotOptional || useInfo.scanDirectionFlagUsed) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "las:scanDirectionFlag", &pointBuffer[0].scanDirectionFlag, N, true, false, sizeof(LASPointDataRecord)));
        proto.set("las:scanDirectionFlag",  IntegerNode(imf, 0, 0, 1));
    }

    if (options.unusedNotOptional || useInfo.edgeOfFlightLineUsed) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "las:edgeOfFlightLine", &pointBuffer[0].edgeOfFlightLine, N, true, false, sizeof(LASPointDataRecord)));
        proto.set("las:edgeOfFlightLine",  IntegerNode(imf, 0, 0, 1));
    }

    /// LAS v1.0 had 8 bit classification field, >v1.0 has 5 bit field.
    /// Specifying actual min and max used, so don't care about size in LAS file.
    if (options.unusedNotOptional || useInfo.classificationUsed) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "las:classification", &pointBuffer[0].classification, N, true, false, sizeof(LASPointDataRecord)));
        proto.set("las:classification",  IntegerNode(imf, 0, useInfo.minimumClassification, useInfo.maximumClassification));
    }

    if (hdr.versionMajor == 1 && hdr.versionMinor > 0) {
        if (options.unusedNotOptional || useInfo.syntheticUsed) {
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:synthetic", &pointBuffer[0].synthetic, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:synthetic",  IntegerNode(imf, 0, 0, 1));
        }

        if (options.unusedNotOptional || useInfo.keyPointUsed) {
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:keyPoint", &pointBuffer[0].keyPoint, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:keyPoint",  IntegerNode(imf, 0, 0, 1));
        }

        if (options.unusedNotOptional || useInfo.withheldUsed) {
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:withheld", &pointBuffer[0].withheld, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:withheld",  IntegerNode(imf, 0, 0, 1));
        }
    }
    if (options.unusedNotOptional || useInfo.scanAngleRankUsed) {
        sourceBuffers.push_back(SourceDestBuffer(imf, "las:scanAngleRank", &pointBuffer[0].scanAngleRank, N, true, false, sizeof(LASPointDataRecord)));
        proto.set("las:scanAngleRank",  IntegerNode(imf, 0, useInfo.minimumScanAngleRank, useInfo.maximumScanAngleRank));
    }

    if (hdr.versionMajor == 1 && hdr.versionMinor == 0) {
        /// LAS v1.0 had file marker and user bit fields
        if (options.unusedNotOptional || useInfo.fileMarkerUsed) {
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:fileMarker", &pointBuffer[0].fileMarker, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:fileMarker",  IntegerNode(imf, 0, UINT8_MIN, UINT8_MAX));
        }

        if (options.unusedNotOptional || useInfo.userBitFieldUsed) {
            /// Reusing las:userData field name rather than create new name.  Means that may be either 16 bits or 8 bits.
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:userData", &pointBuffer[0].userBitField, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:userData",  IntegerNode(imf, 0, UINT16_MIN, UINT16_MAX));
        }
    } else {
        /// LAS v1.1+ has user data and point source id
        if (options.unusedNotOptional || useInfo.userDataUsed) {
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:userData", &pointBuffer[0].userData, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:userData",  IntegerNode(imf, 0, UINT8_MIN, UINT8_MAX));
        }

        /// Note a zero value of pointSourceId should be interpreted as the fileSourceId of this file.
        /// So if this field isn't defined (because all zero), this should be interpreted as all pointSourcIds set to fileSourceId.
        if (options.unusedNotOptional || useInfo.pointSourceIdUsed) {
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:pointSourceId", &pointBuffer[0].pointSourceId, N, true, false,sizeof(LASPointDataRecord)));
            proto.set("las:pointSourceId",  IntegerNode(imf, 0, UINT16_MIN, UINT16_MAX));
        }
    }
    
    switch (hdr.pointDataFormatId) {
        case 1:
        case 3:
        case 4:
        case 5:
            if (options.unusedNotOptional || useInfo.gpsTimeUsed) {
                writingTimeStamp = true;
                sourceBuffers.push_back(SourceDestBuffer(imf, "timeStamp", &pointBuffer[0].gpsTime, N, true, false, sizeof(LASPointDataRecord)));
                proto.set("timeStamp", FloatNode(imf, 0.0, E57_DOUBLE));

                /// Calc lasTimeOffset, the amount that is added to gpsTime in LAS point record to get absolute GPS time.
                /// The gpsTime values are relative to either the start of the GPS week, or 1e9 seconds, depending on gpsTimeType
                if (hdr.gpsTimeType == 1) {
                    /// gpsTime is in "Adjusted Standard GPS Time" (offset from 1e9)
                    lasTimeOffset = 1e9;
                } else {
                    /// gpsTime is in "GPS Week Time" (offset from start of GPS week).
                    /// We are out of luck if duration of scan crosses a week boundary.
                    /// We assume that the gps week is the one that contains the file creation day.
                    if (hdr.fileCreationDayOfYear > 0 && hdr.fileCreationYear > 0) {
                        unsigned short gpsWeek = 0;
                        double gpsTOW = 0.0;
                        if (TIMECONV_GetGPSTimeFromYearAndDayOfYear(hdr.fileCreationYear, hdr.fileCreationDayOfYear, &gpsWeek, &gpsTOW))
                            lasTimeOffset = gpsWeek * SECONDS_IN_WEEK;
                        else
                            lasTimeOffset = 0.0;
                    } else {
                        /// Don't know which GPS week, so just copy LAS time-of-week as-is.
                        lasTimeOffset = 0.0;
                    }
                }

                /// Calc e57TimeOffset, the amount that is added to timestamp in E57 point record to get absolute GPS time.
                if (scan0.isDefined("creationDateTimeStart")) {
                    FloatNode creationDateTimeStart = FloatNode(scan0.get("creationDateTimeStart"));
                    e57TimeOffset = creationDateTimeStart.value();
                } else {
                    /// Don't have a start time, so go with absolute GPS time
                    e57TimeOffset = 0.0;
                }
            }
            break;
    }

    switch (hdr.pointDataFormatId) {
        case 2:
        case 3:
        case 5:
            if (options.unusedNotOptional || useInfo.redUsed || useInfo.blueUsed || useInfo.greenUsed) {
                sourceBuffers.push_back(SourceDestBuffer(imf, "colorRed", &pointBuffer[0].red, N, true, false, sizeof(LASPointDataRecord)));
                proto.set("colorRed", IntegerNode(imf, 0, UINT16_MIN, UINT16_MAX));

                sourceBuffers.push_back(SourceDestBuffer(imf, "colorGreen", &pointBuffer[0].green, N, true, false, sizeof(LASPointDataRecord)));
                proto.set("colorGreen", IntegerNode(imf, 0, UINT16_MIN, UINT16_MAX));

                sourceBuffers.push_back(SourceDestBuffer(imf, "colorBlue", &pointBuffer[0].blue, N, true, false, sizeof(LASPointDataRecord)));
                proto.set("colorBlue", IntegerNode(imf, 0, UINT16_MIN, UINT16_MAX));
            }
            break;
    }

    switch (hdr.pointDataFormatId) {
        case 4:
        case 5:
            sourceBuffers.push_back(SourceDestBuffer(imf, "las:wavePacketDescriptorIndex", &pointBuffer[0].wavePacketDescriptorIndex, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:wavePacketDescriptorIndex",  IntegerNode(imf, 0, UINT8_MIN, UINT8_MAX));

            sourceBuffers.push_back(SourceDestBuffer(imf, "las:byteOffsetToWaveformData", reinterpret_cast<int64_t*>(&pointBuffer[0].byteOffsetToWaveformData), N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:byteOffsetToWaveformData",  IntegerNode(imf, 0LL, 0LL, INT64_MAX));

            sourceBuffers.push_back(SourceDestBuffer(imf, "las:waveformPacketSizeInBytes", &pointBuffer[0].waveformPacketSizeInBytes, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:waveformPacketSizeInBytes",  IntegerNode(imf, 0LU, UINT32_MIN, UINT32_MAX));

            sourceBuffers.push_back(SourceDestBuffer(imf, "las:returnPointWaveformLocation", &pointBuffer[0].returnPointWaveformLocation, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:returnPointWaveformLocation",  FloatNode(imf, 0.0, E57_SINGLE));

            sourceBuffers.push_back(SourceDestBuffer(imf, "las:xT", &pointBuffer[0].xT, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:xT",  FloatNode(imf, 0.0, E57_SINGLE));

            sourceBuffers.push_back(SourceDestBuffer(imf, "las:yT", &pointBuffer[0].yT, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:yT",  FloatNode(imf, 0.0, E57_SINGLE));

            sourceBuffers.push_back(SourceDestBuffer(imf, "las:zT", &pointBuffer[0].zT, N, true, false, sizeof(LASPointDataRecord)));
            proto.set("las:zT",  FloatNode(imf, 0.0, E57_SINGLE));
            break;
    }

    /// Create CompressedVector for storing points.  Path Name: "/images3D/0/points".
    /// We use the prototype and empty codecs tree from above.
    CompressedVectorNode points = CompressedVectorNode(imf, proto, codecs);
    scan0.set("points", points);

    unsigned totalPointCount = hdr.numberOfPointRecords;
    int64_t columnIndex = 0;
    bool lastScanDirectionFlag = false;
    {
        CompressedVectorWriter writer = points.writer(sourceBuffers);

        unsigned recordCount = 0;
        for (unsigned currentRecord = 0; currentRecord < totalPointCount; currentRecord += recordCount) {
            recordCount = min(N, totalPointCount - currentRecord);
#ifdef E57_MAX_VERBOSE
            cerr << "reading " << recordCount << endl;
#endif
            lasf.readPoints(&pointBuffer[0], currentRecord, recordCount);

            /// If we can compute columnIndex, do so for each point we read
            if (useInfo.maximumColumnIndex > 0) {
                for (unsigned i = 0; i < recordCount; i++) {
                    /// Look for reverses in scan direction to delimit columns
                    if (currentRecord != 0 || i != 0) {
                        if (lastScanDirectionFlag != (pointBuffer[i].scanDirectionFlag != 0))
                            columnIndex++;
                    }
                    lastScanDirectionFlag = (pointBuffer[i].scanDirectionFlag != 0);

                    /// Save current column index in buffer for writing
                    columnIndices[i] = columnIndex;
                }
            }

            /// Adjust time stamp for new offset
            if (writingTimeStamp) {
                /// Want timeStamp+e57TimeOffset to be as close as possible to gpsTime+lasTimeOffset
                /// So timeStamp = gpsTime+lasTimeOffset - e57TimeOffset
                /// Be careful about order of addition/subtraction so as not to lose resolution.
                /// Subtract two larger numbers first.  It is OK to trash old value of gpsTime here.
                long double netOffset = lasTimeOffset - e57TimeOffset;
                for (unsigned i = 0; i < recordCount; i++)
                    pointBuffer[i].gpsTime += netOffset;
            }
#ifdef E57_MAX_VERBOSE
            cerr << "writing " << recordCount << endl;
#endif
            writer.write(recordCount);

            /// Add each point into various grouping schemes
            for (unsigned i = 0; i < recordCount; i++)
                groupings.addMember(hdr, pointBuffer[i], currentRecord+i, columnIndices[i]);
        }

        writer.close();
    }        
    cout << "converted " << totalPointCount << " points" << endl;
}

//================================================================

void copyPerFileData(CommandLineOptions& options, LASReader& lasf, ImageFile imf, UseInfo& useInfo)
{
    StructureNode root = imf.root();

    /// Write required format name string
    /// Path name: "/formatName"
    root.set("formatName", StringNode(imf, "ASTM E57 3D Image File")); //??? 3D data file?

    /// Write E57 version numbers
    /// Path names: "/versionMajor", "/versionMinor"
    root.set("versionMajor", IntegerNode(imf, E57_VERSION_MAJOR, 0, UINT8_MAX));
    root.set("versionMinor", IntegerNode(imf, E57_VERSION_MINOR, 0, UINT8_MAX));

    /// Mark file with current GPS time (the time the file was opened for writing).
    /// Path name: "/creationDateTime"
    unsigned short  utc_year;     // Universal Time Coordinated    [year]
    unsigned char   utc_month;    // Universal Time Coordinated    [1-12 months] 
    unsigned char   utc_day;      // Universal Time Coordinated    [1-31 days]
    unsigned char   utc_hour;     // Universal Time Coordinated    [hours]
    unsigned char   utc_minute;   // Universal Time Coordinated    [minutes]
    float           utc_seconds;  // Universal Time Coordinated    [s]
    unsigned char   utc_offset;   // Integer seconds that GPS is ahead of UTC time; always positive             [s], obtained from a look up table
    double          julian_date;  // Number of days since noon Universal Time Jan 1, 4713 BCE (Julian calendar) [days]
    unsigned short  gps_week;     // GPS week (0-1024+)            [week]
    double          gps_tow;      // GPS time of week (0-604800.0) [s]
    if (!TIMECONV_GetSystemTime(&utc_year, &utc_month, &utc_day, &utc_hour, &utc_minute, &utc_seconds, &utc_offset, &julian_date, &gps_week, &gps_tow))
        throw EXCEPTION("get system time failed");
    double gpsTime = gps_week * SECONDS_IN_WEEK + gps_tow;
    root.set("creationDateTime", FloatNode(imf, gpsTime, E57_DOUBLE));
#ifdef E57_MAX_VERBOSE
    cout << "utc_year=" << utc_year << " utc_month=" << static_cast<unsigned>(utc_month) << " utc_day=" << static_cast<unsigned>(utc_day)
         << " utc_hour=" << static_cast<unsigned>(utc_hour) << " utc_minute=" << static_cast<unsigned>(utc_minute) 
         << " utc_seconds=" << utc_seconds << " utc_offset=" << static_cast<unsigned>(utc_offset) << endl;
    cout << "julian_date=" << julian_date << " gps_week=" << gps_week << " gps_tow=" << gps_tow << " gpsTime=" << gpsTime << endl;
#endif

    //??? this right?
    /// There is no guid that uniquely ids the file, so generate one.
    /// The guid in the LAS file uniquely ids the project, not the file.
    /// The combination of the projectID and fileSourceId uniquely ids the original source, but this file might contain processed data.
    /// So generate a guid when the file is converted to e57 format
    ustring fileGuid = generateUuidString();
    root.set("las:projectId", StringNode(imf, fileGuid));
}

//================================================================

ustring generateUuidString()
{
    UUID uuid;
    memset(&uuid, 0, sizeof(UUID));

    // Create uuid or load from a string by UuidFromString() function
    ::UuidCreate(&uuid); //!!! has return?

#ifdef E57_VERBOSE
    cout << "generateUuidString: Data1=" << hexString(static_cast<uint32_t>(uuid.Data1)) 
         << " Data2=" << hexString(static_cast<uint16_t>(uuid.Data2))
         << " Data3=" << hexString(static_cast<uint16_t>(uuid.Data3))
         << " Data4=";
    for (int i=0;i<8;i++)
        cout << hexString(static_cast<uint8_t>(uuid.Data4[i])) << ' ';
    cout << endl;
#endif
    return(guidUnparse(uuid.Data1, uuid.Data2, uuid.Data3, uuid.Data4));
}

ustring guidUnparse(uint32_t data1, uint16_t data2, uint16_t data3, uint8_t data4[8]) {
    ostringstream guid;
    guid << hex << setw(8) << setfill('0') << data1 << "-";
    guid << hex << setw(4) << setfill('0') << data2 << "-";
    guid << hex << setw(4) << setfill('0') << data3 << "-";
    guid << hex << setw(2) << setfill('0') << static_cast<unsigned>(data4[0]);
    guid << hex << setw(2) << setfill('0') << static_cast<unsigned>(data4[1]) << "-";
    for (int i=2; i < 8; i++)
        guid << hex << setw(2) << setfill('0') << static_cast<unsigned>(data4[i]);
    return(guid.str());
}

ustring gpsTimeUnparse(double gpsTime)
{
    unsigned short gpsWeek = static_cast<unsigned short>(floor(gpsTime/SECONDS_IN_WEEK));
    double gpsTOW = gpsTime - gpsWeek * SECONDS_IN_WEEK;

    unsigned short     utc_year;     //!< Universal Time Coordinated    [year]
    unsigned char      utc_month;    //!< Universal Time Coordinated    [1-12 months] 
    unsigned char      utc_day;      //!< Universal Time Coordinated    [1-31 days]
    unsigned char      utc_hour;     //!< Universal Time Coordinated    [hours]
    unsigned char      utc_minute;   //!< Universal Time Coordinated    [minutes]
    float              utc_seconds;  //!< Universal Time Coordinated    [s]
    ostringstream ss;
    if (TIMECONV_GetUTCTimeFromGPSTime(gpsWeek, gpsTOW, &utc_year, &utc_month, &utc_day, &utc_hour, &utc_minute, &utc_seconds)) {
        ss << utc_year << "-" << (unsigned)utc_month << "-" << (unsigned)utc_day << " " << (unsigned)utc_hour << ":" << (unsigned)utc_minute << ":" << utc_seconds;
        return(ss.str());
    } else
        return("<unknown>");
}