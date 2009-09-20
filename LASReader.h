#ifndef LASREADER_INCLUDED
#define LASREADER_INCLUDED

#include <fstream>
#include <iostream>
#include <vector>
#ifndef E57FOUNDATIONIMPL_H_INCLUDED
#  include "E57FoundationImpl.h"
#endif

namespace e57 {

struct LASPublicHeaderBlock {//                   file format#:  1.0 1.1 1.2 1.3
    char      fileSignature[4];                     // required   4   4   4   4
    uint16_t  fileSourceId;                         // required       2   2   2
    uint8_t   gpsTimeType;                          // required           1b  1b
    bool      waveformDataPacketsInternal;          // required               1b
    bool      waveformDataPacketsExternal;          // required               1b
    bool      returnNumbersSyntheticallyGenerated;  // required               1b
    uint32_t  projectIdGuidData1;                   // optional   4   4   4   4
    uint16_t  projectIdGuidData2;                   // optional   2   2   2   2
    uint16_t  projectIdGuidData3;                   // optional   2   2   2   2
    uint8_t   projectIdGuidData4[8];                // optional   8   8   8   8
    uint8_t   versionMajor;                         // required   1   1   1   1
    uint8_t   versionMinor;                         // required   1   1   1   1
    char      systemIdentifier[32];                 // required  32  32  32  32
    char      generatingSoftware[32];               // required  32  32  32  32
    uint16_t  fileCreationDayOfYear;                // *=reqd     2   2   2   2*
    uint16_t  fileCreationYear;                     // *=reqd     2   2   2   2*
    uint16_t  headerSize;                           // required   2   2   2   2
    uint32_t  offsetToPointData;                    // required   4   4   4   4
    uint32_t  numberOfVariableLengthRecords;        // required   4   4   4   4
    uint8_t   pointDataFormatId;                    // required   1   1   1   1
    uint16_t  pointDataRecordLength;                // required   2   2   2   2
    uint32_t  numberOfPointRecords;                 // required   4   4   4   4
    uint32_t  numberOfPointsByReturn[7];            // required  20  20  20  28
    double    xScaleFactor;                         // required   8   8   8   8
    double    yScaleFactor;                         // required   8   8   8   8
    double    zScaleFactor;                         // required   8   8   8   8
    double    xOffset;                              // required   8   8   8   8
    double    yOffset;                              // required   8   8   8   8
    double    zOffset;                              // required   8   8   8   8
    double    maxX;                                 // required   8   8   8   8
    double    minX;                                 // required   8   8   8   8
    double    maxY;                                 // required   8   8   8   8
    double    minY;                                 // required   8   8   8   8
    double    maxZ;                                 // required   8   8   8   8
    double    minZ;                                 // required   8   8   8   8
    uint64_t  startOfWaveformDataPacketRecord;      // required               8

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

                LASPublicHeaderBlock(){memset(this, 0, sizeof(*this));};
};

                           //              file format#:  1.0 1.1 1.0 1.1 
struct LASPointDataRecord {//              point format#: pf0 pf0 pf1 pf1 pf2 pf3 pf4 pf5
    int32_t   x;                             // required, 32  32  32  32  32  32  32  32  
    int32_t   y;                             // required, 32  32  32  32  32  32  32  32  
    int32_t   z;                             // required, 32  32  32  32  32  32  32  32  
    uint16_t  intensity;                     // optional, 16  16  16  16  16  16  16  16
    uint8_t   returnNumber;                  // required,  3   3   3   3   3   3   3   3
    uint8_t   numberOfReturns;               // required,  3   3   3   3   3   3   3   3
    bool      scanDirectionFlag;             // required,  1   1   1   1   1   1   1   1
    bool      edgeOfFlightLine;              // required,  1   1   1   1   1   1   1   1
    uint8_t   classification;                // required,  8   5   8   5   5   5   5   5
    bool      synthetic;                     // required,      1       1   1   1   1   1
    bool      keyPoint;                      // required,      1       1   1   1   1   1
    bool      withheld;                      // required,      1       1   1   1   1   1
    int8_t    scanAngleRank;                 // required,  8   8   8   8   8   8   8   8
    uint8_t   fileMarker;                    // optional,  8       8                    
    uint16_t  userBitField;                  // optional  16      16                    
    uint8_t   userData;                      // optional       8       8   8   8   8   8
    uint16_t  pointSourceId;                 // required      16      16  16  16  16  16
    double    gpsTime;                       // required          64  64      64  64  64
    uint16_t  red;                           // required                  16  16      16
    uint16_t  green;                         // required                  16  16      16
    uint16_t  blue;                          // required                  16  16      16
    uint8_t   wavePacketDescriptorIndex;     // required                           8   8
    uint64_t  byteOffsetToWaveformData;      // required                          64  64
    uint32_t  waveformPacketSizeInBytes;     // required                          32  32
    float     returnPointWaveformLocation;   // required                          32  32
    float     xT;                            // required                          32  32
    float     yT;                            // required                          32  32
    float     zT;                            // required                          32  32

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

                LASPointDataRecord(){memset(this, 0, sizeof(*this));};
};

struct LASVariableRecordLengthHeader { //!!! not needed
    uint16_t  reserved;                      // 2 bytes  optional
    char      userId[16];                    // 16 bytes required
    uint16_t  recordId;                      // 2 bytes  required
    uint16_t  recordLengthAfterHeader;       // 2 bytes  required
    char      description[32];               // 32 bytes optional

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

                LASVariableRecordLengthHeader(){memset(this, 0, sizeof(*this));};
};

struct LASVariableLengthRecord {
    ustring     userId;
    uint16_t    recordId;
    uint16_t    recordLengthAfterHeader;
    ustring     description;
    std::vector<uint8_t> wholeRecordData;

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);
};

struct LASGeoKeyHeader {
    uint16_t keyDirectoryVersion;
    uint16_t keyRevision;
    uint16_t minorRevision;
    uint16_t numberOfKeys;

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

                LASGeoKeyHeader(){memset(this, 0, sizeof(*this));};
};

struct LASGeoKeyEntry {
    uint16_t keyId;
    uint16_t tiffTagLocation;
    uint16_t count;
    uint16_t valueOffset;

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

                LASGeoKeyEntry(){memset(this, 0, sizeof(*this));};

};

class LASReader {
public:
                LASReader(ustring fname);
    void        operator~();
    void        close(){/*???*/};  //???TODO implement

    void        getHeader(LASPublicHeaderBlock& h);
    std::vector<uint8_t> getRawHeader();
    int         readPoints(LASPointDataRecord* points, uint32_t start, uint32_t count);

    void        rewindVLR();
    bool        readNextVLR(LASVariableLengthRecord& vlrInfo);

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

/*!!!
    int readEVLR
    read raw
    key header, key contents
*/

protected: //================
    std::ifstream           fs_;
    LASPublicHeaderBlock    hdr_;
    unsigned                nextVLROffset_;
    unsigned                readVLRCount_;

    //TODO this doesn't work in MSVC6.0:
    //static const int POINT_DATA_RECORD_MAX_BYTES = 63;  // version dependency!, change if point record gets longer
#define POINT_DATA_RECORD_MAX_BYTES 63

    void readPoint(LASPointDataRecord& point);
};

}; // end namspace e57

#endif
