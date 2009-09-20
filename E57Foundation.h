/*
 * E57Foundation.h - public header of E57 Foundation API.
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
#ifndef E57FOUNDATION_H_INCLUDED
#define E57FOUNDATION_H_INCLUDED

/// Undefine the following symbol to enable heap corruption and memory leakage debugging:
//#define E57_DEBUG_MEMORY 1
#if E57_DEBUG_MEMORY
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
#endif

#include <vector>
#include <string>
#include <iostream>
#include "stdint.h"

namespace e57 {

/// Shorthand for unicode string
typedef std::string ustring;

enum NodeType {
    E57_STRUCTURE         = 1,
    E57_VECTOR            = 2,
    E57_COMPRESSED_VECTOR = 3,
    E57_INTEGER           = 4,
    E57_SCALED_INTEGER    = 5,
    E57_FLOAT             = 6,
    E57_STRING            = 7,
    E57_BLOB              = 8
};

enum FloatPrecision {
    E57_SINGLE = 0,
    E57_DOUBLE = 1
};

/// The URI of E57 v1.0 standard XML namespace
/// Used to identify the standard field names and the grammar that relates them.
/// Will typically be associated with the default namespace in an E57 file.
/// Field names in the default namespace have no prefix (e.g. "cartesianX" as opposed to "las:edgeOfFlightLine").
#define E57_V1_0_URI "www.astm.org/E57/2009/E57/v0.0" //??? change to v1.0 before final release

/// The URI of the LAS extension.    ??? should not be in E57Foundation.h, should be in separate file with names of fields
/// Used to identify the extended field names for encoding data from LAS files (LAS versions 1.0 to 1.3).
/// By convention, will typically be used with prefix "las".
#define LAS_V1_0_URI "www.astm.org/E57/2009/LAS/v0.0" //??? change to v1.0 before final release

/// Forward references to classes in this header
class Node;
class StructureNode;
class VectorNode;
class SourceDestBuffer;
class CompressedVectorReader;
class CompressedVectorWriter;
class CompressedVectorNode;
class IntegerNode;
class ScaledIntegerNode;
class FloatNode;
class StringNode;
class BlobNode;
class ImageFile;

//???doc
//??? Can define operator-> that will make implementation more readable
/// Internal implementation files should include e57FoundationImpl.h first which defines symbol E57_INTERNAL_IMPLEMENTATION_ENABLE.
/// Normal API users should not define this symbol.
#ifdef E57_INTERNAL_IMPLEMENTATION_ENABLE
#  define E57_OBJECT_IMPLEMENTATION(T)                              \
public:                                                             \
    std::tr1::shared_ptr<T##Impl> impl() const {return(impl_);};    \
protected:                                                          \
    std::tr1::shared_ptr<T##Impl> impl_;
#else
#  define E57_OBJECT_IMPLEMENTATION(T)                              \
protected:                                                          \
    std::tr1::shared_ptr<T##Impl> impl_;
#endif

#if 1 //!!!
/// Forward references to implementation in other headers (so don't have to include E57FoundationImpl.h)
//??? test all needed?
class NodeImpl;
class StructureNodeImpl;
class VectorNodeImpl;
class SourceDestBufferImpl;
class CompressedVectorReaderImpl;
class CompressedVectorWriterImpl;
class CompressedVectorNodeImpl;
class IntegerNodeImpl;
class ScaledIntegerNodeImpl;
class FloatNodeImpl;
class StringNodeImpl;
class BlobNodeImpl;
class ImageFileImpl;
class E57XmlParser;  //??? needed?
class Encoder;  //??? needed?
class Decoder;  //??? needed?
template <typename RegisterT> class BitpackIntegerEncoder;  //??? needed?
#endif

class Node {
public:
    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();
    void        dump(int indent = 0, std::ostream& os = std::cout);

#ifdef E57_INTERNAL_IMPLEMENTATION_ENABLE
    explicit    Node(std::tr1::shared_ptr<NodeImpl>);  // internal use only
#endif
private:   //=================
                Node();                 // Not defined, can't default construct Node, see StructureNode(), IntegerNode()...

protected: //=================
    friend NodeImpl;

    E57_OBJECT_IMPLEMENTATION(Node)  /// Internal implementation details, not part of API, must be last in object
};

class StructureNode {
public:
                StructureNode(ImageFile imf);

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    int64_t     childCount();
    bool        isDefined(const ustring& pathName);
    Node        get(int64_t index);
    Node        get(const ustring& pathName);
    void        set(int64_t index, Node n);
    void        set(const ustring& pathName, Node n, bool autoPathCreate = false);
    void        append(Node n);

    /// Up/Down cast conversion //???split into two comments
                operator Node();
    explicit    StructureNode(Node& n);

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    friend ImageFile;

                StructureNode(std::tr1::shared_ptr<StructureNodeImpl> ni);    // internal use only
                StructureNode(std::tr1::weak_ptr<ImageFileImpl> fileParent);  // internal use only

    E57_OBJECT_IMPLEMENTATION(StructureNode)  /// Internal implementation details, not part of API, must be last in object
};


class VectorNode {
public:
    explicit    VectorNode(ImageFile imf, bool allowHeteroChildren = false);

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    bool        allowHeteroChildren();

    int64_t     childCount();
    bool        isDefined(const ustring& pathName);
    Node        get(int64_t index);  //??? allow Node get(const ustring& pathName); and set...
    void        set(int64_t index, Node n);
    void        append(Node n);

    /// Up/Down cast conversion
                operator Node();
    explicit    VectorNode(Node& n);
            
    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    Node        get(const ustring& pathName);           // not available ???allow if numeric string
    void        set(const ustring& pathName, Node n, bool autoPathCreate = false);  // not available ???allow if numeric string

                VectorNode(std::tr1::shared_ptr<VectorNodeImpl> ni);  // internal use only

    E57_OBJECT_IMPLEMENTATION(VectorNode)  /// Internal implementation details, not part of API, must be last in object
};

class SourceDestBuffer {
public:
    SourceDestBuffer(ImageFile imf, ustring pathName, int8_t* b,   unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(int8_t));
    SourceDestBuffer(ImageFile imf, ustring pathName, uint8_t* b,  unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(uint8_t));
    SourceDestBuffer(ImageFile imf, ustring pathName, int16_t* b,  unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(int16_t));
    SourceDestBuffer(ImageFile imf, ustring pathName, uint16_t* b, unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(uint16_t));
    SourceDestBuffer(ImageFile imf, ustring pathName, int32_t* b,  unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(int32_t));
    SourceDestBuffer(ImageFile imf, ustring pathName, uint32_t* b, unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(uint32_t));
    SourceDestBuffer(ImageFile imf, ustring pathName, int64_t* b,  unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(int64_t));
    SourceDestBuffer(ImageFile imf, ustring pathName, bool* b,     unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(bool));
    SourceDestBuffer(ImageFile imf, ustring pathName, float* b,    unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(float));
    SourceDestBuffer(ImageFile imf, ustring pathName, double* b,   unsigned capacity, bool doConversion = false, bool doScaling = false, 
                     size_t stride = sizeof(double));
    SourceDestBuffer(ImageFile imf, ustring pathName, std::vector<ustring>* b);  //??? should be pointer or ref?

    ustring         pathName();
    enum MemoryRep  elementType();
    unsigned        capacity();
    bool            doConversion();
    bool            doScaling();
    size_t          stride();

    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================

    E57_OBJECT_IMPLEMENTATION(SourceDestBuffer)  /// Internal implementation details, not part of API, must be last in object
};

class CompressedVectorReader {
public:
    unsigned    read();
    unsigned    read(std::vector<SourceDestBuffer>& dbufs);
    void        seek(uint64_t recordNumber); //!!! not implemented yet
    void        close();

    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    //??? no default ctor, copy
    friend CompressedVectorNode;

                CompressedVectorReader(std::tr1::shared_ptr<CompressedVectorReaderImpl> ni);

    E57_OBJECT_IMPLEMENTATION(CompressedVectorReader)  /// Internal implementation details, not part of API, must be last in object
};

class CompressedVectorWriter {
public:
    void        write(unsigned requestedElementCount);
    void        write(std::vector<SourceDestBuffer>& sbufs, unsigned requestedElementCount);
    void        close();

    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    //??? no default ctor, copy
    friend CompressedVectorNode;

                CompressedVectorWriter(std::tr1::shared_ptr<CompressedVectorWriterImpl> ni);

    E57_OBJECT_IMPLEMENTATION(CompressedVectorWriter)  /// Internal implementation details, not part of API, must be last in object
};

class CompressedVectorNode {
public:
    explicit    CompressedVectorNode(ImageFile imf, Node prototype, Node codecs);

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    int64_t     childCount();
    bool        isDefined(const ustring& pathName); //??? needed?
    Node        prototype();

    /// Iterators
    CompressedVectorWriter writer(std::vector<SourceDestBuffer>& sbufs);  //??? totalRecordCount?
    CompressedVectorReader reader(std::vector<SourceDestBuffer>& dbufs);  //??? totalRecordCount?

    /// Up/Down cast conversion
                operator Node();
    explicit    CompressedVectorNode(Node& n);
            
    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    friend E57XmlParser;

                CompressedVectorNode(std::tr1::shared_ptr<CompressedVectorNodeImpl> ni);  // internal use only

    E57_OBJECT_IMPLEMENTATION(CompressedVectorNode)  /// Internal implementation details, not part of API, must be last in object
};

class IntegerNode {
public:
    explicit    IntegerNode(ImageFile imf, int64_t  value, int64_t  minimum = INT64_MIN, int64_t  maximum = INT64_MAX);

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    int64_t     value();
    int64_t     minimum();
    int64_t     maximum();

    /// Up/Down cast conversion
                operator Node();
    explicit    IntegerNode(Node& n);
            
    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================

                IntegerNode(std::tr1::shared_ptr<IntegerNodeImpl> ni);  // internal use only

    E57_OBJECT_IMPLEMENTATION(IntegerNode)  /// Internal implementation details, not part of API, must be last in object
};

class ScaledIntegerNode {
public:
//    explicit    ScaledIntegerNode(ImageFile imf, int8_t   value, int8_t   minimum = INT8_MIN,  int8_t   maximum = INT8_MAX,   
//                                  double scale = 1.0, double offset = 0.0);
//    explicit    ScaledIntegerNode(ImageFile imf, int16_t  value, int16_t  minimum = INT16_MIN, int16_t  maximum = INT16_MAX,  
//                                  double scale = 1.0, double offset = 0.0);
//    explicit    ScaledIntegerNode(ImageFile imf, int32_t  value, int32_t  minimum = INT32_MIN, int32_t  maximum = INT32_MAX,  
//                                  double scale = 1.0, double offset = 0.0);
    explicit    ScaledIntegerNode(ImageFile imf, int64_t  value, int64_t  minimum = INT64_MIN, int64_t  maximum = INT64_MAX,  
                                  double scale = 1.0, double offset = 0.0);
//    explicit    ScaledIntegerNode(ImageFile imf, uint8_t  value, uint8_t  minimum = 0,         uint8_t  maximum = UINT8_MAX,  
//                                  double scale = 1.0, double offset = 0.0);
//    explicit    ScaledIntegerNode(ImageFile imf, uint16_t value, uint16_t minimum = 0,         uint16_t maximum = UINT16_MAX, 
//                                  double scale = 1.0, double offset = 0.0);
//    explicit    ScaledIntegerNode(ImageFile imf, uint32_t value, uint32_t minimum = 0,         uint32_t maximum = UINT32_MAX, 
//                                  double scale = 1.0, double offset = 0.0);

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    int64_t     rawValue();
    double      scaledValue();
    int64_t     minimum();
    int64_t     maximum();
    double      scale();
    double      offset();

    /// Up/Down cast conversion
                operator Node();
    explicit    ScaledIntegerNode(Node& n);
            
    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================

                ScaledIntegerNode(std::tr1::shared_ptr<ScaledIntegerNodeImpl> ni);  // internal use only

    E57_OBJECT_IMPLEMENTATION(ScaledIntegerNode)  /// Internal implementation details, not part of API, must be last in object
};

class FloatNode {
public:
    explicit    FloatNode(ImageFile imf, float value, FloatPrecision precision = E57_SINGLE);
    explicit    FloatNode(ImageFile imf, double value, FloatPrecision precision = E57_DOUBLE);

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    double      value();
    FloatPrecision precision();

    /// Up/Down cast conversion
                operator Node();
    explicit    FloatNode(Node& n);
            
    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================

                FloatNode(std::tr1::shared_ptr<FloatNodeImpl> ni);  // internal use only

    E57_OBJECT_IMPLEMENTATION(FloatNode)  /// Internal implementation details, not part of API, must be last in object
};

class StringNode {
public:
    explicit    StringNode(ImageFile imf, ustring value = ""); //??? explicit?, need default ""?

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    ustring     value();

    /// Up/Down cast conversion
                operator Node();
    explicit    StringNode(Node& n);
            
    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    friend StringNodeImpl;
                StringNode(std::tr1::shared_ptr<StringNodeImpl> ni);  // internal use only

    E57_OBJECT_IMPLEMENTATION(StringNode)  /// Internal implementation details, not part of API, must be last in object
};

class BlobNode {
public:
    explicit    BlobNode(ImageFile imf, uint64_t byteCount);

    NodeType    type();
    bool        isRoot();
    Node        parent();
    ustring     pathName();
    ustring     fieldName();

    int64_t     byteCount();
    void        read(uint8_t* buf, uint64_t start, uint64_t byteCount);
    void        write(uint8_t* buf, uint64_t start, uint64_t byteCount);

    /// Up/Down cast conversion
                operator Node();
    explicit    BlobNode(Node& n);
            
    /// Diagnostic functions:
    void        dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    friend E57XmlParser;

                BlobNode(std::tr1::shared_ptr<BlobNodeImpl> ni);       // internal use only

                /// Internal use only, create blob already in a file
                BlobNode(ImageFile imf, uint64_t fileOffset, uint64_t length);

    E57_OBJECT_IMPLEMENTATION(BlobNode)  /// Internal implementation details, not part of API, must be last in object
};

class ImageFile {
public:
                    ImageFile(const ustring& fname, const ustring& mode, const ustring& configuration = "");
    StructureNode   root();
    void            close();
    void            cancel();

    /// Manipulate registered extensions in the file
    void            extensionsAdd(const ustring& prefix, const ustring& uri);
    bool            extensionsLookupPrefix(const ustring& prefix, ustring& uri);
    bool            extensionsLookupUri(const ustring& uri, ustring& prefix);
    int             extensionsCount();
    ustring         extensionsPrefix(int index);
    ustring         extensionsUri(int index);

    /// Utility functions:!!!
    bool            fieldNameIsExtension(const ustring& fieldName);
    void            fieldNameParse(const ustring& fieldName, ustring& prefix, ustring& base);
    void            pathNameParse(const ustring& pathName, bool& isRelative, std::vector<ustring>& fields);
    ustring         pathNameUnparse(bool isRelative, const std::vector<ustring>& fields);
    ustring         fileNameExtension(const ustring& fileName);
    void            fileNameParse(const ustring& fileName, bool& isRelative, ustring& volumeName, std::vector<ustring>& directories, 
                                  ustring& fileBase, ustring& extension);
    ustring         fileNameUnparse(bool isRelative, const ustring& volumeName, const std::vector<ustring>& directories,
                                    const ustring& fileBase, const ustring& extension);

    /// Diagnostic functions:
    void            dump(int indent = 0, std::ostream& os = std::cout);

protected: //=================
    //??? copy, default ctor, assign

    ImageFile(std::tr1::shared_ptr<ImageFileImpl> imfi);  // internal use only

    E57_OBJECT_IMPLEMENTATION(ImageFile)  /// Internal implementation details, not part of API, must be last in object
};



#if 0 //================================================================
//
//!!! These property identifiers are out of data, update to latest names
//
const ustring       PICTURE_NAME         = "Name";          //Type_String
const ustring       PICTURE_UID          = "Uid";           //Type_String
const ustring       PICTURE_DECODER_NAME = "DecoderName";   //Type_String
const ustring       PICTURE_STATUS       = "Status";        //Type_Int64
const ustring       PICTURE_DATETIME     = "DateTime";      //Type_DateTime
    
//Loc_Image Name (Optional)
//This is a name of the scan, if the picture was taken during a scan.

const ustring       PICTURE_IMAGE_NAME   = "ImageName";     //Type_String

//Loc_Picture Pose
//This is the camera position.

const ustring       PICTURE_POSITION      = "Position";         //Type_Position
const ustring       PICTURE_ROTATION      = "Rotation";         //Type_Quaternion
const ustring       PICTURE_FIELD_OF_VIEW = "FieldOfView";      //Type_Position
const ustring       PICTURE_FOCAL_LENGTH  = "FocalLength";      //Type_Real64

//////////////////////////////////////////////////////////////////////////
//  Standard Loc_Image Property List
//
//  ustring name = GetPropertyString(Loc_Image, IMAGE_NAME);
//
const ustring       IMAGE_NAME             = "Name";            //Type_String
const ustring       IMAGE_UID              = "Uid";             //Type_String
const ustring       IMAGE_COMPRESSION_NAME = "CompressionName"; //Type_String
const ustring       IMAGE_STATUS           = "Status";          //Type_Int64
const ustring       IMAGE_DATETIME         = "DateTime";        //Type_DateTime

//Loc_Image Description (Optional)

const ustring       IMAGE_DESCRIPTION  = "Description";         //Type_String
const ustring       IMAGE_USER_NAME    = "UserName";            //Type_String
const ustring       IMAGE_LOCATION     = "Location";            //Type_String
const ustring       IMAGE_POINT_NUMBER = "PointNumber";         //Type_String
//
//  Orientation
//
const ustring       IMAGE_POSITION = "Position";                //Type_Position
const ustring       IMAGE_ROTATION = "Rotation";                //Type_Quaternion

// Scan Bounding Box
// An axis aligned bounding box describing the extent of the data in the scan along
//  the x, y, and z axes in the scanner local coordinate system.  The bounding box
//  will be specified by the minimum and maximum coordinate along each of the x, y, and z axes.

const ustring       IMAGE_STATISTIC_RAE_MIN = "RAEMinimum";         //Type_Position
const ustring       IMAGE_STATISTIC_RAE_MAX = "RAEMaximum";         //Type_Position
const ustring       IMAGE_STATISTIC_XYZ_MIN = "XYZMinimum";         //Type_Position
const ustring       IMAGE_STATISTIC_XYZ_MAX = "XYZMaximum";         //Type_Position
const ustring       IMAGE_STATISTIC_INT_MIN = "IntensityMinimum";   //Type_Real64
const ustring       IMAGE_STATISTIC_INT_MAX = "IntensityMaximum";   //Type_Real64
//
//  Scanner Information
//
const ustring       IMAGE_SCANNER_VENDOR_NAME      = "VendorName";      //Type_String
const ustring       IMAGE_SCANNER_MODEL_NAME       = "ModelName";       //Type_String
const ustring       IMAGE_SCANNER_SERIAL_NUMBER    = "SerialNumber";    //Type_String
const ustring       IMAGE_SCANNER_SOFTWARE_VERSION = "SoftwareVersion"; //Type_String
const ustring       IMAGE_SCANNER_SUPPLY_VOLTAGE   = "SupplyVoltage";   //Type_Real64

// Scanner Environmental (Optional)  [Original document]
const ustring       IMAGE_SCANNER_TEMPERATURE  = "Temperature"; //Type_Real64
const ustring       IMAGE_SCANNER_AIR_PRESSURE = "AirPressure"; //Type_Real64

//
//  Locations (Optional)
//
const ustring       IMAGE_GCS_POSITION = "LatitudeLongitude";   //Type_Position
const ustring       IMAGE_GCS_SCALER   = "Scaler";              //Type_Real64
const ustring       IMAGE_GCS_ZONE     = "Zone";                //Type_Int64
const ustring       IMAGE_GCS_DATETIME = "DateTime";            //Type_DataTime

//Scanner Settings (Optional)
const ustring       IMAGE_HIGH_AZIMUTH    = "HighAzimuth";      //Type_Real64
const ustring       IMAGE_LOW_AZIMUTH     = "LowAzimuth";       //Type_Real64
const ustring       IMAGE_DELTA_AZIMUTH   = "DeltaAzimuth";     //Type_Real64
const ustring       IMAGE_HIGH_ELEVATION  = "HighElevation";    //Type_Real64
const ustring       IMAGE_LOW_ELEVATION   = "LowElevation";     //Type_Real64
const ustring       IMAGE_DELTA_ELEVATION = "DeltaElevation";   //Type_Real64
#endif //!!!================================================================

};  // end namespace e57

#endif // E57FOUNDATION_H_INCLUDED
