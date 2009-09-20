#include "LASReader.h"
#include "e57FoundationImpl.h"

#include <algorithm>

using namespace std;
using namespace e57;

LASReader::LASReader(string fname)
: fs_(fname.c_str(), ios::in|ios::binary)   /// Open the file for binary read
{
    if (!fs_.is_open())
        throw EXCEPTION("open failed"); //??? TODO pick standard exception

    /// Initialize all fields in header to zero
    memset(&hdr_, 0, sizeof(hdr_));

    /// Read each header field.
    /// Note file is little endian, so if running on big endian machine need to 
    ///   reverse byte order for fields larger than 1 byte.  See below.
    fs_.read(reinterpret_cast<char*>(hdr_.fileSignature), sizeof(hdr_.fileSignature));
    if (fs_.fail())
        throw EXCEPTION("file read failed");

    if (hdr_.fileSignature[0] != 'L' || hdr_.fileSignature[1] != 'A' || 
        hdr_.fileSignature[2] != 'S' || hdr_.fileSignature[3] != 'F')
        throw EXCEPTION("not an LAS file");

    /// wait until have version number to decode fileSourceId
    uint16_t fileSourceId;
    fs_.read(reinterpret_cast<char*>(&fileSourceId), sizeof(hdr_.fileSourceId));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(fileSourceId);

    /// wait until have version number to decode globalEncoding
    uint16_t globalEncoding;
    fs_.read(reinterpret_cast<char*>(&globalEncoding), sizeof(globalEncoding));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(globalEncoding);

    fs_.read(reinterpret_cast<char*>(&hdr_.projectIdGuidData1), sizeof(hdr_.projectIdGuidData1));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.projectIdGuidData1);
    fs_.read(reinterpret_cast<char*>(&hdr_.projectIdGuidData2), sizeof(hdr_.projectIdGuidData2));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.projectIdGuidData2);
    fs_.read(reinterpret_cast<char*>(&hdr_.projectIdGuidData3), sizeof(hdr_.projectIdGuidData3));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.projectIdGuidData3);
    fs_.read(reinterpret_cast<char*>(hdr_.projectIdGuidData4), sizeof(hdr_.projectIdGuidData4));
    if (fs_.fail())
        throw EXCEPTION("file read failed");

    fs_.read(reinterpret_cast<char*>(&hdr_.versionMajor), sizeof(hdr_.versionMajor));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    fs_.read(reinterpret_cast<char*>(&hdr_.versionMinor), sizeof(hdr_.versionMinor));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    /// We understand versions 1.0, 1.1, 1.2, 1.3
    if (hdr_.versionMajor != 1 || hdr_.versionMinor > 3)
        throw EXCEPTION("unknown LAS version");

    /// Decode fileSourceId field, now that we known the version
    if (hdr_.versionMajor >= 1 ||  hdr_.versionMinor >= 1)
        hdr_.fileSourceId = fileSourceId;

    /// Decode globalEncoding field, now that we known the version
    if (hdr_.versionMajor >= 1 ||  hdr_.versionMinor >= 2)
        hdr_.gpsTimeType = (globalEncoding & (1<<0)) ? true : false;
    if (hdr_.versionMajor >= 1 ||  hdr_.versionMinor >= 3) {
        hdr_.waveformDataPacketsInternal         = (globalEncoding & (1<<1)) ? true : false;
        hdr_.waveformDataPacketsExternal         = (globalEncoding & (1<<2)) ? true : false;
        hdr_.returnNumbersSyntheticallyGenerated = (globalEncoding & (1<<3)) ? true : false;
    }

    fs_.read(reinterpret_cast<char*>(hdr_.systemIdentifier), sizeof(hdr_.systemIdentifier));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    hdr_.systemIdentifier[sizeof(hdr_.systemIdentifier)-1] = '\0'; // make sure null terminated
    fs_.read(reinterpret_cast<char*>(hdr_.generatingSoftware), sizeof(hdr_.generatingSoftware));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    hdr_.generatingSoftware[sizeof(hdr_.generatingSoftware)-1] = '\0'; // make sure null terminated
    fs_.read(reinterpret_cast<char*>(&hdr_.fileCreationDayOfYear), sizeof(hdr_.fileCreationDayOfYear));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.fileCreationDayOfYear);
    fs_.read(reinterpret_cast<char*>(&hdr_.fileCreationYear), sizeof(hdr_.fileCreationYear));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.fileCreationYear);
    fs_.read(reinterpret_cast<char*>(&hdr_.headerSize), sizeof(hdr_.headerSize));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.headerSize);
    fs_.read(reinterpret_cast<char*>(&hdr_.offsetToPointData), sizeof(hdr_.offsetToPointData));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.offsetToPointData);
    fs_.read(reinterpret_cast<char*>(&hdr_.numberOfVariableLengthRecords), sizeof(hdr_.numberOfVariableLengthRecords));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.numberOfVariableLengthRecords);
    fs_.read(reinterpret_cast<char*>(&hdr_.pointDataFormatId), sizeof(hdr_.pointDataFormatId));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    fs_.read(reinterpret_cast<char*>(&hdr_.pointDataRecordLength), sizeof(hdr_.pointDataRecordLength));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.pointDataRecordLength);
    fs_.read(reinterpret_cast<char*>(&hdr_.numberOfPointRecords), sizeof(hdr_.numberOfPointRecords));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.numberOfPointRecords);
    if (hdr_.versionMajor == 1 && hdr_.versionMinor <= 2) {
        /// Earlier versions of LAS (1.0, 1.1, 1.2) only had provision for upto 5 returns per pulse
        fs_.read(reinterpret_cast<char*>(hdr_.numberOfPointsByReturn), 5*sizeof(uint32_t));
        if (fs_.fail())
            throw EXCEPTION("file read failed");
        for (int i=0; i<5; i++)
            SWAB(hdr_.numberOfPointsByReturn[i]);
    } else {
        fs_.read(reinterpret_cast<char*>(hdr_.numberOfPointsByReturn), 7*sizeof(uint32_t));
        if (fs_.fail())
            throw EXCEPTION("file read failed");
        for (int i=0; i<7; i++)
            SWAB(hdr_.numberOfPointsByReturn[i]);
    }
    fs_.read(reinterpret_cast<char*>(&hdr_.xScaleFactor), sizeof(hdr_.xScaleFactor));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.xScaleFactor);
    fs_.read(reinterpret_cast<char*>(&hdr_.yScaleFactor), sizeof(hdr_.yScaleFactor));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.yScaleFactor);
    fs_.read(reinterpret_cast<char*>(&hdr_.zScaleFactor), sizeof(hdr_.zScaleFactor));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.zScaleFactor);
    fs_.read(reinterpret_cast<char*>(&hdr_.xOffset), sizeof(hdr_.xOffset));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.xOffset);
    fs_.read(reinterpret_cast<char*>(&hdr_.yOffset), sizeof(hdr_.yOffset));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.yOffset);
    fs_.read(reinterpret_cast<char*>(&hdr_.zOffset), sizeof(hdr_.zOffset));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.zOffset);
    fs_.read(reinterpret_cast<char*>(&hdr_.maxX), sizeof(hdr_.maxX));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.maxX);
    fs_.read(reinterpret_cast<char*>(&hdr_.minX), sizeof(hdr_.minX));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.minX);
    fs_.read(reinterpret_cast<char*>(&hdr_.maxY), sizeof(hdr_.maxY));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.maxY);
    fs_.read(reinterpret_cast<char*>(&hdr_.minY), sizeof(hdr_.minY));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.minY);
    fs_.read(reinterpret_cast<char*>(&hdr_.maxZ), sizeof(hdr_.maxZ));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.maxZ);
    fs_.read(reinterpret_cast<char*>(&hdr_.minZ), sizeof(hdr_.minZ));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(hdr_.minZ);
    
    if (hdr_.versionMajor >= 1 && hdr_.versionMinor >= 3) {
        fs_.read(reinterpret_cast<char*>(&hdr_.startOfWaveformDataPacketRecord), sizeof(hdr_.startOfWaveformDataPacketRecord));
        if (fs_.fail())
            throw EXCEPTION("file read failed");
        SWAB(hdr_.startOfWaveformDataPacketRecord);
    }

//???TODO check error returns
    if (fs_.fail())
        throw EXCEPTION("file read failed");

    nextVLROffset_ = hdr_.headerSize;
    readVLRCount_ = 0;
}

void LASReader::getHeader(LASPublicHeaderBlock& h)
{
    h = hdr_;
}

int LASReader::readPoints(LASPointDataRecord* points, unsigned start, unsigned count)
{
    if (!fs_.is_open())
        throw EXCEPTION("file not open"); //??? TODO pick standard exception
    
    /// Calculate location of first point and how many we can fetch
    unsigned long start_offset = hdr_.offsetToPointData + start * hdr_.pointDataRecordLength;
    count = (count < hdr_.numberOfPointRecords - start) ? count : hdr_.numberOfPointRecords - start;

    /// Seek to start of first record
    fs_.seekg(start_offset, ios::beg);
    if (fs_.fail())
        throw EXCEPTION("file seek failed");

    /// Read points one at a time.  No need to seek to following point.
    for (unsigned i = 0; i < count; i++)
        readPoint(points[i]);
    
    return(count);
}

void LASReader::readPoint(LASPointDataRecord& point)
{
    if (!fs_.is_open())
        throw EXCEPTION("file not open"); //??? TODO pick standard exception
    
    /// Initialize all fields in output point record to zero
    memset(&point, 0, sizeof(LASPointDataRecord));

    /// A buffer on the stack big enough to hold a whole point record from file
    unsigned char buf[POINT_DATA_RECORD_MAX_BYTES];

    /// Verify that we have enough room
    if (hdr_.pointDataRecordLength > POINT_DATA_RECORD_MAX_BYTES)
        throw EXCEPTION("internal failure"); //??? TODO pick standard exception

    /// Read whole record into character buffer
    /// Reading whole record at a time for speed (as opposed to calling ifstream::read() 8 to 10 times
    fs_.read(reinterpret_cast<char*>(buf), hdr_.pointDataRecordLength);
    if (fs_.fail())
        throw EXCEPTION("file read failed");

    /// Copy fields from buffer to structure.  Use memcpy to avoid risk of word alignment problems
    memcpy(&point.x, &buf[0], sizeof(point.x));
    SWAB(point.x);
    memcpy(&point.y, &buf[4], sizeof(point.y));
    SWAB(point.y);
    memcpy(&point.z, &buf[8], sizeof(point.z));
    SWAB(point.z);
    memcpy(&point.intensity, &buf[12], sizeof (point.intensity));
    SWAB(point.intensity);
    point.returnNumber      = buf[14]      & 0x3;
    point.numberOfReturns   = (buf[14]>>3) & 0x3;
    point.scanDirectionFlag = (buf[14] & (1<<6)) ? true : false;
    point.edgeOfFlightLine  = (buf[14] & (1<<7)) ? true : false;
    if (hdr_.versionMajor == 1 && hdr_.versionMinor == 0) {
        point.classification = buf[15];
    } else {
        point.classification = buf[15] & 0x1f;
        point.synthetic         = (buf[15] & (1<<5)) ? true : false;
        point.withheld          = (buf[15] & (1<<6)) ? true : false;
        point.edgeOfFlightLine  = (buf[15] & (1<<7)) ? true : false;
    }
    point.scanAngleRank = buf[16];
    if (hdr_.versionMajor == 1 && hdr_.versionMinor == 0) {
        point.fileMarker = buf[17];
        memcpy(&point.userBitField, &buf[18], sizeof (point.userBitField));
        SWAB(point.userBitField);
    } else {
        point.userData = buf[17];
        memcpy(&point.pointSourceId, &buf[18], sizeof (point.pointSourceId));
        SWAB(point.pointSourceId);
    }
    switch (hdr_.pointDataFormatId) {
        case 0:
            if (hdr_.pointDataRecordLength != 20)
                throw EXCEPTION("bad packet length"); //??? TODO pick standard exception
            break;
        case 1:
            if (hdr_.pointDataRecordLength != 28)
                throw EXCEPTION("bad packet length"); //??? TODO pick standard exception
            memcpy(&point.gpsTime, &buf[20], sizeof(point.gpsTime));
            SWAB(point.gpsTime);
            break;
        case 2:
            if (hdr_.pointDataRecordLength != 26)
                throw EXCEPTION("bad packet length"); //??? TODO pick standard exception
            memcpy(&point.red, &buf[20], sizeof(point.red));
            SWAB(point.red);
            memcpy(&point.green, &buf[22], sizeof(point.green));
            SWAB(point.green);
            memcpy(&point.blue, &buf[24], sizeof(point.blue));
            SWAB(point.blue);
            break;
        case 3:
            if (hdr_.pointDataRecordLength != 34)
                throw EXCEPTION("bad packet length"); //??? TODO pick standard exception
            memcpy(&point.gpsTime, &buf[20], sizeof(point.gpsTime));
            SWAB(point.gpsTime);
            memcpy(&point.red, &buf[28], sizeof(point.red));
            SWAB(point.red);
            memcpy(&point.green, &buf[30], sizeof(point.green));
            SWAB(point.green);
            memcpy(&point.blue, &buf[32], sizeof(point.blue));
            SWAB(point.blue);
            break;
        case 4:
            if (hdr_.pointDataRecordLength != 57)
                throw EXCEPTION("bad packet length"); //??? TODO pick standard exception
            memcpy(&point.gpsTime, &buf[20], sizeof(point.gpsTime));
            SWAB(point.gpsTime);
            point.wavePacketDescriptorIndex = buf[28];
            memcpy(&point.byteOffsetToWaveformData, &buf[29], sizeof(point.byteOffsetToWaveformData));
            SWAB(point.byteOffsetToWaveformData);
            memcpy(&point.waveformPacketSizeInBytes, &buf[37], sizeof(point.waveformPacketSizeInBytes));
            SWAB(point.waveformPacketSizeInBytes);
            memcpy(&point.returnPointWaveformLocation, &buf[41], sizeof(point.returnPointWaveformLocation));
            SWAB(point.returnPointWaveformLocation);
            memcpy(&point.xT, &buf[45], sizeof(point.xT));
            SWAB(point.xT);
            memcpy(&point.yT, &buf[49], sizeof(point.yT));
            SWAB(point.yT);
            memcpy(&point.zT, &buf[53], sizeof(point.zT));
            SWAB(point.zT);
            break;
        case 5:
            if (hdr_.pointDataRecordLength != 63)
                throw EXCEPTION("bad packet length"); //??? TODO pick standard exception
            memcpy(&point.gpsTime, &buf[20], sizeof(point.gpsTime));
            SWAB(point.gpsTime);
            memcpy(&point.red, &buf[28], sizeof(point.red));
            SWAB(point.red);
            memcpy(&point.green, &buf[30], sizeof(point.green));
            SWAB(point.green);
            memcpy(&point.blue, &buf[32], sizeof(point.blue));
            SWAB(point.blue);
            point.wavePacketDescriptorIndex = buf[34];
            memcpy(&point.byteOffsetToWaveformData, &buf[35], sizeof(point.byteOffsetToWaveformData));
            SWAB(point.byteOffsetToWaveformData);
            memcpy(&point.waveformPacketSizeInBytes, &buf[43], sizeof(point.waveformPacketSizeInBytes));
            SWAB(point.waveformPacketSizeInBytes);
            memcpy(&point.returnPointWaveformLocation, &buf[47], sizeof(point.returnPointWaveformLocation));
            SWAB(point.returnPointWaveformLocation);
            memcpy(&point.xT, &buf[51], sizeof(point.xT));
            SWAB(point.xT);
            memcpy(&point.yT, &buf[55], sizeof(point.yT));
            SWAB(point.yT);
            memcpy(&point.zT, &buf[59], sizeof(point.zT));
            SWAB(point.zT);
            break;
        default:
            throw EXCEPTION("unknown LAS packet format"); //??? TODO pick standard exception
    }
}

vector<uint8_t> LASReader::getRawHeader()
{
    vector<uint8_t> rawHeader(hdr_.headerSize);
    fs_.seekg(0, ios::beg);
    if (fs_.fail())
        throw EXCEPTION("file seek failed");
    fs_.read(reinterpret_cast<char*>(&rawHeader[0]), hdr_.headerSize);
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    return(rawHeader);
}

void LASReader::rewindVLR()
{
    nextVLROffset_ = hdr_.headerSize;
    readVLRCount_ = 0;
}

bool LASReader::readNextVLR(LASVariableLengthRecord& vlr)
{
    /// Check if done reading
    if (readVLRCount_ >= hdr_.numberOfVariableLengthRecords)
        return(false);

    /// Seek to start of next variable length record
    /// Other calls may have moved file pointer since last VLR read.
    fs_.seekg(nextVLROffset_, ios::beg);
    if (fs_.fail())
        throw EXCEPTION("file seek failed");

    /// Skip over reserved 2 bytes
    uint16_t reserved;
    fs_.read(reinterpret_cast<char*>(&reserved), sizeof(reserved));
    if (fs_.fail())
        throw EXCEPTION("file read failed");

    char userId[16+1]; // extra char to make sure terminated
    fs_.read(userId, 16);
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    userId[16] = '\0';
    vlr.userId = ustring(userId);

    fs_.read(reinterpret_cast<char*>(&vlr.recordId), sizeof(vlr.recordId));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(vlr.recordId);

    fs_.read(reinterpret_cast<char*>(&vlr.recordLengthAfterHeader), sizeof(vlr.recordLengthAfterHeader));
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    SWAB(vlr.recordLengthAfterHeader);

    char description[32+1]; // extra char to make sure terminated
    fs_.read(description, 32);
    if (fs_.fail())
        throw EXCEPTION("file read failed");
    description[32] = '\0';
    vlr.description = ustring(description);

    /// Make binary copy of whole record including header
    unsigned recordSize = 54 + vlr.recordLengthAfterHeader;
    vlr.wholeRecordData.resize(recordSize);
    fs_.seekg(nextVLROffset_, ios::beg);
    if (fs_.fail())
        throw EXCEPTION("file seek failed");
    fs_.read(reinterpret_cast<char*>(&vlr.wholeRecordData[0]), recordSize);
    if (fs_.fail())
        throw EXCEPTION("file read failed");

    /// Move nextVLROffset_ to next packet
    nextVLROffset_ += recordSize;
    readVLRCount_++;
    return(true);
}

void LASPublicHeaderBlock::dump(int indent, std::ostream& os)
{
    os << space(indent) << "fileSignature:             " << fileSignature[0] << fileSignature[1] 
                                                         << fileSignature[2] << fileSignature[3] << endl;
    os << space(indent) << "fileSourceId:              " << fileSourceId << endl;
    os << space(indent) << "gpsTimeType:               " << static_cast<unsigned>(gpsTimeType) << endl;
    os << space(indent) << "waveformDataPacketsInternal:" << static_cast<unsigned>(waveformDataPacketsInternal) << endl;
    os << space(indent) << "waveformDataPacketsExternal:" << static_cast<unsigned>(waveformDataPacketsExternal) << endl;
    os << space(indent) << "returnNumbersSyntheticallyGenerated:" << static_cast<unsigned>(returnNumbersSyntheticallyGenerated) << endl;
    os << space(indent) << "projectIdGuidData1:        " << hexString(projectIdGuidData1) << endl;
    os << space(indent) << "projectIdGuidData2:        " << hexString(projectIdGuidData2) << endl;
    os << space(indent) << "projectIdGuidData3:        " << hexString(projectIdGuidData3) << endl;
    os << space(indent) << "projectIdGuidData4:        " << hexString(projectIdGuidData4[0]) << " "
                                                         << hexString(projectIdGuidData4[1]) << " "
                                                         << hexString(projectIdGuidData4[2]) << " "
                                                         << hexString(projectIdGuidData4[3]) << " "
                                                         << hexString(projectIdGuidData4[4]) << " "
                                                         << hexString(projectIdGuidData4[5]) << " "
                                                         << hexString(projectIdGuidData4[6]) << " "
                                                         << hexString(projectIdGuidData4[7]) << endl;

    os << space(indent) << "versionMajor:              " << static_cast<unsigned>(versionMajor) << endl;
    os << space(indent) << "versionMinor:              " << static_cast<unsigned>(versionMinor) << endl;
    os << space(indent) << "systemIdentifier:          " << systemIdentifier << endl;
    os << space(indent) << "generatingSoftware:        " << generatingSoftware << endl;
    os << space(indent) << "fileCreationDayOfYear:     " << fileCreationDayOfYear << endl;
    os << space(indent) << "fileCreationYear:          " << fileCreationYear << endl;
    os << space(indent) << "headerSize:                " << headerSize << endl;
    os << space(indent) << "offsetToPointData:         " << offsetToPointData << endl;
    os << space(indent) << "numberOfVariableLengthRecords: " << numberOfVariableLengthRecords << endl;
    os << space(indent) << "pointDataFormatId:         " << static_cast<unsigned>(pointDataFormatId) << endl;
    os << space(indent) << "pointDataRecordLength:     " << pointDataRecordLength << endl;
    os << space(indent) << "numberOfPointRecords:      " << numberOfPointRecords << endl;
    os << space(indent) << "numberOfPointsByReturn[0]: " << numberOfPointsByReturn[0] << endl;
    os << space(indent) << "numberOfPointsByReturn[1]: " << numberOfPointsByReturn[1] << endl;
    os << space(indent) << "numberOfPointsByReturn[2]: " << numberOfPointsByReturn[2] << endl;
    os << space(indent) << "numberOfPointsByReturn[3]: " << numberOfPointsByReturn[3] << endl;
    os << space(indent) << "numberOfPointsByReturn[4]: " << numberOfPointsByReturn[4] << endl;
    os << space(indent) << "numberOfPointsByReturn[5]: " << numberOfPointsByReturn[5] << endl;
    os << space(indent) << "numberOfPointsByReturn[6]: " << numberOfPointsByReturn[6] << endl;
    os << space(indent) << "xScaleFactor:              " << xScaleFactor << endl;
    os << space(indent) << "yScaleFactor:              " << yScaleFactor << endl;
    os << space(indent) << "zScaleFactor:              " << zScaleFactor << endl;
    os << space(indent) << "xOffset:                   " << xOffset << endl;
    os << space(indent) << "yOffset:                   " << yOffset << endl;
    os << space(indent) << "zOffset:                   " << zOffset << endl;
    os << space(indent) << "maxX:                      " << maxX << endl;
    os << space(indent) << "minX:                      " << minX << endl;
    os << space(indent) << "maxY:                      " << maxY << endl;
    os << space(indent) << "minY:                      " << minY << endl;
    os << space(indent) << "maxZ:                      " << maxZ << endl;
    os << space(indent) << "minZ:                      " << minZ << endl;
    os << space(indent) << "startOfWaveformDataPacketRecord:" << startOfWaveformDataPacketRecord << endl;
}

void LASPointDataRecord::dump(int indent, std::ostream& os)
{
    os << space(indent) << "x:                 " << x << endl;
    os << space(indent) << "y:                 " << y << endl;
    os << space(indent) << "z:                 " << z << endl;
    os << space(indent) << "intensity:         " << intensity << endl;
    os << space(indent) << "returnNumber:      " << static_cast<unsigned>(returnNumber) << endl;
    os << space(indent) << "numberOfReturns:   " << static_cast<unsigned>(numberOfReturns) << endl;
    os << space(indent) << "scanDirectionFlag: " << static_cast<unsigned>(scanDirectionFlag) << endl;
    os << space(indent) << "edgeOfFlightLine:  " << static_cast<unsigned>(edgeOfFlightLine) << endl;
    os << space(indent) << "classification:    " << static_cast<unsigned>(classification) << endl;
    os << space(indent) << "synthetic:         " << static_cast<unsigned>(synthetic) << endl;
    os << space(indent) << "keyPoint:          " << static_cast<unsigned>(keyPoint) << endl;
    os << space(indent) << "withheld:          " << static_cast<unsigned>(withheld) << endl;
    os << space(indent) << "scanAngleRank:     " << static_cast<unsigned>(scanAngleRank) << endl;
    os << space(indent) << "fileMarker:        " << static_cast<unsigned>(fileMarker) << endl;
    os << space(indent) << "userBitField:      " << userBitField << endl;
    os << space(indent) << "userData:          " << static_cast<unsigned>(userData) << endl;
    os << space(indent) << "pointSourceId:     " << pointSourceId << endl;
    os << space(indent) << "gpsTime:           " << gpsTime << endl;
    os << space(indent) << "red:               " << red << endl;
    os << space(indent) << "green:             " << green << endl;
    os << space(indent) << "blue:              " << blue << endl;
    os << space(indent) << "wavePacketDescriptorIndex:  " << static_cast<unsigned>(wavePacketDescriptorIndex) << endl;
    os << space(indent) << "byteOffsetToWaveformData:   " << byteOffsetToWaveformData << endl;
    os << space(indent) << "waveformPacketSizeInBytes:  " << waveformPacketSizeInBytes << endl;
    os << space(indent) << "returnPointWaveformLocation:" << returnPointWaveformLocation << endl;
    os << space(indent) << "xT                          " << xT << endl;
    os << space(indent) << "yT                          " << yT << endl;
    os << space(indent) << "zT                          " << zT << endl;
}

void LASVariableLengthRecord::dump(int indent, std::ostream& os)
{
    os << space(indent) << "userId:                  " << userId << endl;
    os << space(indent) << "recordId:                " << recordId << endl;
    os << space(indent) << "recordLengthAfterHeader: " << recordLengthAfterHeader << endl;
    os << space(indent) << "description:             " << description << endl;
    unsigned i;
    for (i = 0; i < wholeRecordData.size() && i < 4; i++)
        os << space(indent) << "wholeRecordData[" << i << "]: " << static_cast<unsigned>(wholeRecordData[i]) << endl;
    if (i < wholeRecordData.size())
        os << space(indent) << wholeRecordData.size() - i << " bytes unprinted..." << endl;

}

void LASReader::dump(int indent, std::ostream& os)
{
    hdr_.dump(0, os);
    os << space(indent) << "readVLRCount:      " << readVLRCount_ << endl;
    os << space(indent) << "nextVLROffset:     " << nextVLROffset_ << endl;

    unsigned i;
    for (i = 0; i < hdr_.numberOfPointRecords && i < 4; i++) {
        LASPointDataRecord pt;
        readPoints(&pt, i, 1);
        os << space(indent) << "Point[" << i << "]:" << endl;
        pt.dump(4, os);
    }
    if (i < hdr_.numberOfPointRecords)
        os << space(indent) << hdr_.numberOfPointRecords - i << " points unprinted..." << endl;
}

#if 0 //!!!
int LASReader::readGeoKeys(GeoKeyEntry* keys, unsigned start, unsigned count)
{
    //??? implement
    return(0);
}
#endif
