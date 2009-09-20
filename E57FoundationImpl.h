/*
 * E57FoundationImpl.h - private implementation header of E57 Foundation API.
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
#ifndef E57FOUNDATIONIMPL_H_INCLUDED
#define E57FOUNDATIONIMPL_H_INCLUDED

#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "stdint.h"

/// Define the following symbol adds some functions to the API for implementation purposes.
/// These functions are not available to a normal API user.
#define E57_INTERNAL_IMPLEMENTATION_ENABLE 1

#ifndef E57FOUNDATION_H_INCLUDED
#  include "E57Foundation.h"
#endif

/// Uncomment the lines below to enable various levels of cross checking and verification in the code.
/// The extra code does not change the file contents.
/// Recomment that E57_DEBUG remain defined even for production versions.
#define E57_DEBUG       1
#define E57_MAX_DEBUG   1

/// Uncomment the lines below to enable various levels of printing to the console of what is going on in the code.
//#define E57_VERBOSE     1
//#define E57_MAX_VERBOSE 1

/// Uncomment the line below to enable writing packets that are correct but will stress the reader.
//#define E57_WRITE_CRAZY_PACKET_MODE 1

/// Disable MSVC warning: warning C4224: nonstandard extension used : formal parameter 'locale' was previously defined as a type
#pragma warning( disable : 4224)

#include <stack>

/// Turn off DLL input/export mechanism for Xerces library (usually done by defining in compile command line).
//#define XERCES_STATIC_LIBRARY 1

/// The XML parser headers
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>

/// Make shorthand for Xerces namespace???
XERCES_CPP_NAMESPACE_USE;

namespace e57 {

/// Disable MSVC warning:warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning( disable : 4996)

//??? for development:
static char* exception_string(char* e_name, char* file_name, int line_no) {
    static char buf[256];
    sprintf(buf, "%s at %s:%d", e_name, file_name, line_no); //?? strstream?
    return(buf);
}
#define EXCEPTION(e_name) (exception_string((e_name), __FILE__, __LINE__))

/// Create whitespace of given length, for indenting printouts in dump() functions
static std::string space(int n) {return(std::string(n,' '));};

/// Convert number to hexadecimal and binary strings  (Note hex strings don't have leading zeros).
static std::string hexString(uint64_t x) {std::ostringstream ss; ss << "0x" << std::hex << std::setw(16)<< std::setfill('0') << x; return(ss.str());};
static std::string hexString(uint32_t x) {std::ostringstream ss; ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << x; return(ss.str());};
static std::string hexString(uint16_t x) {std::ostringstream ss; ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << x; return(ss.str());};
static std::string hexString(uint8_t x)  {std::ostringstream ss; ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(x); return(ss.str());};
static std::string binaryString(uint64_t x) {std::ostringstream ss;for(int i=63;i>=0;i--){ss<<((x&(1LL<<i))?1:0);if(i>0&&i%8==0)ss<<" ";} return(ss.str());};
static std::string binaryString(uint32_t x) {std::ostringstream ss;for(int i=31;i>=0;i--){ss<<((x&(1LL<<i))?1:0);if(i>0&&i%8==0)ss<<" ";} return(ss.str());};
static std::string binaryString(uint16_t x) {std::ostringstream ss;for(int i=15;i>=0;i--){ss<<((x&(1LL<<i))?1:0);if(i>0&&i%8==0)ss<<" ";} return(ss.str());};
static std::string binaryString(uint8_t x) {std::ostringstream ss;for(int i=7;i>=0;i--){ss<<((x&(1LL<<i))?1:0);if(i>0&&i%8==0)ss<<" ";} return(ss.str());};
static std::string hexString(int64_t x) {return(hexString(static_cast<uint64_t>(x)));};
static std::string hexString(int32_t x) {return(hexString(static_cast<uint32_t>(x)));};
static std::string hexString(int16_t x) {return(hexString(static_cast<uint16_t>(x)));};
static std::string hexString(int8_t x)  {return(hexString(static_cast<uint8_t>(x)));};
static std::string binaryString(int64_t x) {return(binaryString(static_cast<uint64_t>(x)));};
static std::string binaryString(int32_t x) {return(binaryString(static_cast<uint32_t>(x)));};
static std::string binaryString(int16_t x) {return(binaryString(static_cast<uint16_t>(x)));};
static std::string binaryString(int8_t x)  {return(binaryString(static_cast<uint8_t>(x)));};

/// Forward reference
template <typename RegisterT> class BitpackIntegerEncoder;
template <typename RegisterT> class BitpackIntegerDecoder;
class E57XmlParser;
class Encoder;

enum MemoryRep {
    E57_INT8,
    E57_UINT8,
    E57_INT16,
    E57_UINT16,
    E57_INT32,
    E57_UINT32,
    E57_INT64,
    E57_BOOL,
    E57_REAL32,
    E57_REAL64,
    E57_USTRING
};

const uint32_t E57_VERSION_MAJOR = 0; //??? should be here?
const uint32_t E57_VERSION_MINOR = 2; //??? should be here?

/// Section types:
#define E57_BLOB_SECTION                1
#define E57_COMPRESSED_VECTOR_SECTION   2

/// Packet types (in a compressed vector section)
#define E57_DATA_PACKET                 1
#define E57_INDEX_PACKET                2
#define E57_EMPTY_PACKET                3

#ifdef E57_BIGENDIAN
#  define  SWAB(p)  swab(p)
#else
#  define  SWAB(p)
#endif

//================================================================
#define SLOW_MODE 1 //??? CHECKEDFILE_SLOW_MODE?

class CheckedFile {
public:
    enum Mode {readOnly, writeCreate, writeExisting};
    enum OffsetMode {logical, physical};
    static const size_t   physicalPageSizeLog2 = 10;
    static const size_t   physicalPageSize = 1 << physicalPageSizeLog2;
    static const uint64_t physicalPageSizeMask = physicalPageSize-1;
    static const size_t   logicalPageSize = physicalPageSize - 4; //??? rename

                    CheckedFile(ustring fname, Mode mode);
                    ~CheckedFile();

    void            read(char* buf, size_t nRead, size_t bufSize = 0);
    //???void       write(char* buf, size_t nWrite, size_t bufSize = 0);
    void            write(const char* buf, size_t nWrite);
    CheckedFile&    operator<<(const ustring& s);
    CheckedFile&    operator<<(int64_t i);
    CheckedFile&    operator<<(uint64_t i);
    CheckedFile&    operator<<(double d);
    void            seek(uint64_t offset, OffsetMode omode = logical);
    uint64_t        position(OffsetMode omode = logical);
    uint64_t        length(OffsetMode omode = logical);
    void            extend(uint64_t length, OffsetMode omode = logical);
    void            flush();
    void            close();

    static size_t   efficientBufferSize(size_t logicalSize);  //??? needed?

    static inline uint64_t logicalToPhysical(uint64_t logicalOffset);
    static inline uint64_t physicalToLogical(uint64_t physicalOffset);
private:
    uint32_t        checksum(char* buf, size_t size);

    int             fd_;
    bool            readOnly_;
    uint64_t        logicalLength_;

#ifdef SLOW_MODE
    void getCurrentPageAndOffset(uint64_t& page, size_t& pageOffset, OffsetMode omode = logical);
    void readPhysicalPage(char* page_buffer, uint64_t page);
    void writePhysicalPage(char* page_buffer, uint64_t page);
#else
    ???
    void        finishPage();

    uint32_t    pageChecksum_;
    bool        currentPageDirty_;
    char*       temp_page;
#endif
};

inline uint64_t CheckedFile::logicalToPhysical(uint64_t logicalOffset)
{
    uint64_t page = logicalOffset / logicalPageSize;
    uint64_t remainder = logicalOffset - page*logicalPageSize;
    return(page*physicalPageSize + remainder);
}

inline uint64_t CheckedFile::physicalToLogical(uint64_t physicalOffset)
{
    uint64_t page = physicalOffset >> physicalPageSizeLog2;
    size_t remainder = static_cast<size_t> (physicalOffset & physicalPageSizeMask);

    return(page*logicalPageSize + std::min(remainder, logicalPageSize));
}

//================================================================

class NodeImpl : public std::tr1::enable_shared_from_this<NodeImpl> {
public:
    virtual NodeType        type() = 0;
    virtual bool            isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni) = 0;
    bool                    isRoot() {return(parent_.expired());};
    std::tr1::shared_ptr<NodeImpl> parent() {std::tr1::shared_ptr<NodeImpl>myParent(parent_);return(myParent);};  //!!! fails for root
    ustring                 pathName();
    ustring                 relativePathName(std::tr1::shared_ptr<NodeImpl> origin, ustring childPathName = ustring());
    ustring                 fieldName() {return(fieldName_);};
    virtual bool            isDefined(const ustring& pathName) = 0; 

    void                    setParent(std::tr1::shared_ptr<NodeImpl> parent, const ustring& fieldName);
    bool                    isTypeConstrained();

    virtual std::tr1::shared_ptr<NodeImpl> get(const ustring& pathName) {throw EXCEPTION("bad operation");};
    virtual void            set(const ustring& pathName, std::tr1::shared_ptr<NodeImpl> ni, bool autoPathCreate = false) {throw EXCEPTION("bad operation");};
    virtual void            set(const std::vector<ustring>& fields, int level, std::tr1::shared_ptr<NodeImpl> ni, bool autoPathCreate = false)  {throw EXCEPTION("bad operation");};

    virtual void            checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin) = 0;
    void                    checkBuffers(const std::vector<SourceDestBuffer>& sdbufs, bool allowMissing);
    bool                    findTerminalPosition(std::tr1::shared_ptr<NodeImpl> ni, uint64_t& countFromLeft);

    virtual void            writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL) = 0;

    virtual                 ~NodeImpl() {};

#ifdef E57_DEBUG
    virtual void            dump(int indent = 0, std::ostream& os = std::cout) = 0;
#endif

protected: //=================
    //??? owned by image file?
    friend StructureNodeImpl;
    friend CompressedVectorWriterImpl;
    friend Decoder; //???
    friend Encoder; //???

                                            NodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent);
    NodeImpl&                               operator=(NodeImpl& n);
    virtual std::tr1::shared_ptr<NodeImpl>  lookup(const ustring& pathName) {return(std::tr1::shared_ptr<NodeImpl>());}; //???
    std::tr1::shared_ptr<NodeImpl>          getRoot();

    std::tr1::weak_ptr<ImageFileImpl>       fileParent_;
    std::tr1::weak_ptr<NodeImpl>            parent_;
    ustring                                 fieldName_;
};

class StructureNodeImpl : public NodeImpl {
public:
                        StructureNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent);
    virtual             ~StructureNodeImpl() {};

    virtual NodeType    type() {return(E57_STRUCTURE);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    virtual bool        isDefined(const ustring& pathName); 
    virtual int64_t     childCount() {return(children_.size());};
    virtual std::tr1::shared_ptr<NodeImpl> get(int64_t index);
    virtual std::tr1::shared_ptr<NodeImpl> get(const ustring& pathName);
    virtual void        set(int64_t index, std::tr1::shared_ptr<NodeImpl> ni);
    virtual void        set(const ustring& pathName, std::tr1::shared_ptr<NodeImpl> ni, bool autoPathCreate = false);
    virtual void        set(const std::vector<ustring>& fields, int level, std::tr1::shared_ptr<NodeImpl> ni, bool autoPathCreate = false);
    virtual void        append(std::tr1::shared_ptr<NodeImpl> ni);

    virtual void        checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
    friend CompressedVectorReaderImpl;
    virtual std::tr1::shared_ptr<NodeImpl> lookup(const ustring& pathName);

    std::vector<std::tr1::shared_ptr<NodeImpl>> children_;
};

class VectorNodeImpl : public StructureNodeImpl {
public:
    explicit            VectorNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, bool allowHeteroChildren);
    virtual             ~VectorNodeImpl() {};

    virtual NodeType    type() {return(E57_VECTOR);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    bool                allowHeteroChildren() {return(allowHeteroChildren_);};

    //???virtual Node   get(int64_t index);
    //???virtual Node   get(const ustring& pathName);
    virtual void        set(int64_t index, std::tr1::shared_ptr<NodeImpl> ni);
    //???virtual void   set(const ustring& pathName, std::tr1::shared_ptr<NodeImpl> ni);
    //???virtual void   append(std::tr1::shared_ptr<NodeImpl> ni);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //=================
    bool allowHeteroChildren_;
};

class SourceDestBufferImpl : public std::tr1::enable_shared_from_this<SourceDestBufferImpl> {
public:
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, int8_t* b,   unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(int8_t));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, uint8_t* b,  unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(uint8_t));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, int16_t* b,  unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(int16_t));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, uint16_t* b, unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(uint16_t));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, int32_t* b,  unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(int32_t));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, uint32_t* b, unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(uint32_t));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, int64_t* b,  unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(int64_t));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, bool* b,     unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(bool));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, float* b,    unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(float));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, double* b,   unsigned capacity, bool doConversion = false, 
                         bool doScaling = false, size_t stride = sizeof(double));
    SourceDestBufferImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring pathName, std::vector<ustring>* b);
    
    ustring                 pathName()      {return(pathName_);}
    MemoryRep               elementType()   {return(elementType_);};
    void*                   base()          {return(base_);}
    std::vector<ustring>*   ustrings()      {return(ustrings_);}
    bool                    doConversion()  {return(doConversion_);}
    bool                    doScaling()     {return(doScaling_);}
    size_t                  stride()        {return(stride_);}
    unsigned                capacity()      {return(capacity_);}
    unsigned                nextIndex()     {return(nextIndex_);};
    void                    rewind()        {nextIndex_=0;};

    /// Get/set values:
    int64_t         getNextInt64();
    int64_t         getNextInt64(double scale, double offset);
    float           getNextFloat();
    double          getNextDouble();
    ustring         getNextString();
    void            setNextInt64(int64_t value);
    void            setNextInt64(int64_t value, double scale, double offset);
    void            setNextFloat(float value);
    void            setNextDouble(double value);
    void            setNextString(const ustring& value);

#ifdef E57_DEBUG
    void            dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
friend BitpackIntegerEncoder<uint8_t>;   //??? needed?
friend BitpackIntegerEncoder<uint16_t>;  //??? needed?
friend BitpackIntegerEncoder<uint32_t>;  //??? needed?
friend BitpackIntegerEncoder<uint64_t>;  //??? needed?
friend BitpackIntegerDecoder<uint8_t> ;  //??? needed?
friend BitpackIntegerDecoder<uint16_t>;  //??? needed?
friend BitpackIntegerDecoder<uint32_t>;  //??? needed?
friend BitpackIntegerDecoder<uint64_t>;  //??? needed?

    //??? verify alignment
    std::tr1::weak_ptr<ImageFileImpl> fileParent_;
    ustring                 pathName_;      /// Pathname from CompressedVectorNode to source/dest object, e.g. "Indices/0"
    MemoryRep               elementType_;   /// Type of element (e.g. E57_INT8, E57_UINT64, DOUBLE...)
    char*                   base_;          /// Address of first element, for non-ustring buffers
    std::vector<ustring>*   ustrings_;      /// Optional array of ustrings (used if elementType_==E57_USTRING) ???ownership
    unsigned                capacity_;      /// Total number of elements in array
    bool                    doConversion_;  /// Convert memory representation to/from disk representation
    bool                    doScaling_;     /// Apply scale factor for integer type
    size_t                  stride_;        /// Distance between each element (different than size_ if elements not contiguous)
    unsigned                nextIndex_;     /// Number of elements that have been set (dest buffer) or read (source buffer) since rewind().
};

//================================================================

class CompressedVectorNodeImpl : public NodeImpl {
public:
                        CompressedVectorNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent);
    virtual             ~CompressedVectorNodeImpl() {};

    virtual NodeType    type() {return(E57_COMPRESSED_VECTOR);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    virtual bool        isDefined(const ustring& pathName); 

    void                setPrototype(std::tr1::shared_ptr<NodeImpl> prototype);
    std::tr1::shared_ptr<NodeImpl> getPrototype() {return(prototype_);};  //??? check defined
    void                setCodecs(std::tr1::shared_ptr<NodeImpl> codecs);
    std::tr1::shared_ptr<NodeImpl> getCodecs() {return(codecs_);};  //??? check defined

    virtual int64_t     childCount();

    virtual void        checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);

    /// Iterator constructors
    std::tr1::shared_ptr<CompressedVectorWriterImpl> writer(std::vector<SourceDestBuffer> sbufs);
    std::tr1::shared_ptr<CompressedVectorReaderImpl> reader(std::vector<SourceDestBuffer> dbufs);

    uint64_t            getRecordCount()                        {return(recordCount_);};
    uint64_t            getBinarySectionLogicalStart()          {return(binarySectionLogicalStart_);};
    void                setRecordCount(uint64_t recordCount)    {recordCount_ = recordCount;};
    void                setBinarySectionLogicalStart(uint64_t binarySectionLogicalStart)
                                                                {binarySectionLogicalStart_ = binarySectionLogicalStart;};

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //=================
    friend CompressedVectorReaderImpl; //???

    std::tr1::shared_ptr<NodeImpl>  prototype_;
    std::tr1::shared_ptr<NodeImpl>  codecs_;

//???    bool                            writeCompleted_;
    uint64_t                        recordCount_;
    uint64_t                        binarySectionLogicalStart_;
};

class IntegerNodeImpl : public NodeImpl {
public:
                        IntegerNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, int64_t value = 0, int64_t minimum = 0, int64_t maximum = 0); //??? need default min/max, must specify?
    virtual             ~IntegerNodeImpl() {};

    virtual NodeType    type()    {return(E57_INTEGER);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    virtual bool        isDefined(const ustring& pathName); 
    int64_t             value()   {return(value_);};
    int64_t             minimum() {return(minimum_);};
    int64_t             maximum() {return(maximum_);};

    virtual void        checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
    int64_t             value_;
    int64_t             minimum_;
    int64_t             maximum_;
};

class ScaledIntegerNodeImpl : public NodeImpl {
public:
                        ScaledIntegerNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, int64_t value = 0, int64_t minimum = 0, int64_t maximum = 0,
                                              double scale = 1.0, double offset = 0.0);
    virtual             ~ScaledIntegerNodeImpl() {};

    virtual NodeType    type()          {return(E57_SCALED_INTEGER);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    virtual bool        isDefined(const ustring& pathName); 
    int64_t             rawValue()      {return(value_);};
    double              scaledValue()   {return(value_ * scale_ + offset_);};
    int64_t             minimum()       {return(minimum_);};
    int64_t             maximum()       {return(maximum_);};
    double              scale()         {return(scale_);};
    double              offset()        {return(offset_);};

    virtual void        checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);


#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
    int64_t             value_;
    int64_t             minimum_;
    int64_t             maximum_;
    double              scale_;
    double              offset_;
};

class FloatNodeImpl : public NodeImpl {
public:
                        FloatNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, double value = 0, FloatPrecision precision = E57_DOUBLE);
    virtual             ~FloatNodeImpl() {};

    virtual NodeType    type() {return(E57_FLOAT);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    virtual bool        isDefined(const ustring& pathName); 
    double              value() {return(value_);};
    FloatPrecision      precision() {return(precision_);};

    virtual void        checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
    double              value_;
    FloatPrecision      precision_;
};

class StringNodeImpl : public NodeImpl {
public:
    explicit            StringNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, ustring value = "");
    virtual             ~StringNodeImpl() {};

    virtual NodeType    type() {return(E57_STRING);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    virtual bool        isDefined(const ustring& pathName); 
    ustring             value() {return(value_);};

    virtual void        checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
    ustring             value_;
};

class BlobNodeImpl : public NodeImpl {
public:
                        BlobNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, uint64_t byteCount);
                        BlobNodeImpl(std::tr1::weak_ptr<ImageFileImpl> fileParent, uint64_t fileOffset, uint64_t length);
    virtual             ~BlobNodeImpl();

    virtual NodeType    type() {return(E57_BLOB);};
    virtual bool        isTypeEquivalent(std::tr1::shared_ptr<NodeImpl> ni);
    virtual bool        isDefined(const ustring& pathName); 
    int64_t             byteCount();
    void                read(uint8_t* buf, uint64_t start, uint64_t count);
    void                write(uint8_t* buf, uint64_t start, uint64_t count);

    virtual void        checkLeavesInSet(const std::set<ustring>& pathNames, std::tr1::shared_ptr<NodeImpl> origin);

    virtual void        writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName=NULL);

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
    uint64_t            blobLogicalLength_;
    uint64_t            binarySectionLogicalStart_;
    uint64_t            binarySectionLogicalLength_;
};

class ImageFileImpl : public std::tr1::enable_shared_from_this<ImageFileImpl> {
public:
					ImageFileImpl();
	void			construct2(const ustring& fname, const ustring& mode, const ustring& configuration);
    std::tr1::shared_ptr<StructureNodeImpl> root();
    void            close();
    void            cancel();
                    ~ImageFileImpl();

    uint64_t        allocateSpace(uint64_t byteCount, bool doExtendNow);
    CheckedFile*    file() {return(file_);};

    /// Manipulate registered extensions in the file
    void            extensionsAdd(const ustring& prefix, const ustring& uri);
    bool            extensionsLookupPrefix(const ustring& prefix, ustring& uri);
    bool            extensionsLookupUri(const ustring& uri, ustring& prefix);
    int             extensionsCount()           {return(nameSpaces_.size());};
    ustring         extensionsPrefix(int index) {return(nameSpaces_[index].prefix);};
    ustring         extensionsUri(int index)    {return(nameSpaces_[index].uri);};

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
    unsigned        bitsNeeded(int64_t minimum, int64_t maximum);

    /// Diagnostic functions:
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif

protected: //=================
    friend E57XmlParser;
    friend BlobNodeImpl;
    friend CompressedVectorWriterImpl;
    friend CompressedVectorReaderImpl; //??? add file() instead of accessing file_, others friends too

    struct NameSpace {
        ustring     prefix;
        ustring     uri;
                    NameSpace(ustring prefix0, ustring uri0) : prefix(prefix0),uri(uri0) {};
    };

    //??? copy, default ctor, assign

    //!!! dump all members
    ustring                 fname_;
    bool                    isWriter_;
    CheckedFile*            file_;

    /// Read file attributes
    uint64_t                xmlLogicalOffset_;
    uint64_t                xmlLogicalLength_;

    /// Write file attributes
    uint64_t                unusedLogicalStart_;

    /// Bidirectional map from namespace prefix to uri
    std::vector<NameSpace>  nameSpaces_; 

    /// Smart pointer to metadata tree
    std::tr1::shared_ptr<StructureNodeImpl> root_;

    bool                    isWriteIterActive_; //???
};

//================================================================


class SeekIndex {
public:
    ///!!! implement
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout) {/*???*/};
#endif
};

//================================================================

struct CompressedVectorSectionHeader {
    uint8_t     sectionId;              // = E57_COMPRESSED_VECTOR_SECTION
    uint8_t     reserved1[7];           // must be zero
    uint64_t    sectionLogicalLength;   // byte length of whole section
    uint64_t    dataPhysicalOffset;     // offset of first data packet
    uint64_t    indexPhysicalOffset;    // offset of first index packet

                CompressedVectorSectionHeader();
    void        verify(uint64_t filePhysicalSize=0); //???use
#ifdef E57_BIGENDIAN
    void        swab();
#else
    void        swab(){};
#endif
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

//================================================================

#define E57_DATA_PACKET_MAX (64*1024)  /// maximum size of CompressedVector binary data packet   ??? where put this


struct DataPacketHeader {  ///??? where put this
    uint8_t     packetType;         // = E57_DATA_PACKET
    uint8_t     packetFlags;
    uint16_t    packetLogicalLengthMinus1;
    uint16_t    bytestreamCount;

                DataPacketHeader();
    void        verify(unsigned bufferLength=0); //???use
#ifdef E57_BIGENDIAN
    void        swab();
#else
    void        swab(){};
#endif
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

//================================================================

struct DataPacket {  /// Note this is full sized packet, not just header
    uint8_t     packetType;         // = E57_DATA_PACKET
    uint8_t     packetFlags;
    uint16_t    packetLogicalLengthMinus1;
    uint16_t    bytestreamCount;
    uint8_t     payload[64*1024-6]; // pad packet to full length, can't spec layout because depends bytestream data

                DataPacket();
    void        verify(unsigned bufferLength=0);
    char*       getBytestream(unsigned bytestreamNumber, unsigned& bufferLength);
    unsigned    getBytestreamBufferLength(unsigned bytestreamNumber);

#ifdef E57_BIGENDIAN
    void        swab(bool toLittleEndian);    //??? change to swabIfBigEndian() and elsewhere
#else
    void        swab(bool toLittleEndian){};
#endif
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

//================================================================

struct IndexPacket {  /// Note this is whole packet, not just header
    static const unsigned MAX_ENTRIES = 2048;

    uint8_t     packetType;     // = E57_INDEX_PACKET
    uint8_t     packetFlags;    // flag bitfields
    uint16_t    packetLogicalLengthMinus1;
    uint16_t    entryCount;
    uint8_t     indexLevel;
    uint8_t     reserved1[9];   // must be zero
    struct IndexPacketEntry {
        uint64_t    chunkRecordNumber;
        uint64_t    chunkPhysicalOffset;
    } entries[MAX_ENTRIES];

                IndexPacket();
    void        verify(unsigned bufferLength=0, uint64_t totalRecordCount=0, uint64_t fileSize=0);
#ifdef E57_BIGENDIAN
    void        swab(bool toLittleEndian);
#else
    void        swab(bool toLittleEndian) {};
#endif
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

//================================================================

struct EmptyPacketHeader {
    uint8_t     packetType;    // = E57_EMPTY_PACKET
    uint8_t     reserved1;     // must be zero
    uint16_t    packetLogicalLengthMinus1;

                EmptyPacketHeader();
    void        verify(unsigned bufferLength=0); //???use
#ifdef E57_BIGENDIAN
    void        swab();
#else
    void        swab(){};
#endif
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

//================================================================

struct DecodeChannel {
    SourceDestBuffer    dbuf; //??? for now, one input per channel
    Decoder*            decoder;
    unsigned            bytestreamNumber;
    uint64_t            maxRecordCount;
    uint64_t            currentPacketLogicalOffset;
    unsigned            currentBytestreamBufferIndex;
    unsigned            currentBytestreamBufferLength;
    bool                inputFinished;

                        DecodeChannel(SourceDestBuffer dbuf_arg, Decoder* decoder_arg, unsigned bytestreamNumber_arg, uint64_t maxRecordCount_arg);
                        ~DecodeChannel();
    bool                isOutputBlocked();
    bool                isInputBlocked();   /// has exhausted data in the current packet
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

//================================================================

class PacketReadCache;

class CompressedVectorReaderImpl {
public:
                CompressedVectorReaderImpl(std::tr1::shared_ptr<CompressedVectorNodeImpl> ni, std::vector<SourceDestBuffer>& dbufs);
                ~CompressedVectorReaderImpl();
    unsigned    read();
    unsigned    read(std::vector<SourceDestBuffer>& dbufs);
    void        seek(uint64_t recordNumber);
    void        close();

#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //=================
    void        setBuffers(std::vector<SourceDestBuffer>& dbufs); //???needed?
    uint64_t    earliestPacketNeededForInput();
    void        feedPacketToDecoders(uint64_t currentPacketLogicalOffset);
    uint64_t    findNextDataPacket(uint64_t nextPacketLogicalOffset);

    //??? no default ctor, copy, assignment?

    std::vector<SourceDestBuffer>                   dbufs_;
    std::tr1::shared_ptr<CompressedVectorNodeImpl>  cVector_;
    std::tr1::shared_ptr<NodeImpl>                  proto_;
    std::vector<DecodeChannel>                      channels_;
    PacketReadCache*                                cache_;

    uint64_t    recordCount_;                   /// number of records written so far
    uint64_t    maxRecordCount_;
    uint64_t    sectionEndLogicalOffset_;
};

//================================================================

class CompressedVectorWriterImpl {
public:
                CompressedVectorWriterImpl(std::tr1::shared_ptr<CompressedVectorNodeImpl> ni, std::vector<SourceDestBuffer>& sbufs);
                ~CompressedVectorWriterImpl();
    void        close();
    void        write(unsigned requestedElementCount);
    void        write(std::vector<SourceDestBuffer>& sbufs, unsigned requestedElementCount);

#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //=================
    void        setBuffers(std::vector<SourceDestBuffer>& sbufs); //???needed?
    unsigned    totalOutputAvailable();
    unsigned    currentPacketSize();
    uint64_t    packetWrite();
    void        flush();

    //??? no default ctor, copy, assignment?

    std::vector<SourceDestBuffer>                   sbufs_;
    std::tr1::shared_ptr<CompressedVectorNodeImpl>  cVector_;
    std::tr1::shared_ptr<NodeImpl>                  proto_;

    std::vector<Encoder*>   bytestreams_;
    SeekIndex               seekIndex_;
    DataPacket              dataPacket_;

    bool                    isOpen_;
    uint64_t                sectionHeaderLogicalStart_;     /// start of CompressedVector binary section
    uint64_t                sectionLogicalLength_;          /// total length of CompressedVector binary section
    uint64_t                dataPhysicalOffset_;            /// start of first data packet
    uint64_t                topIndexPhysicalOffset_;        /// top level index packet
    uint64_t                recordCount_;                   /// number of records written so far
    uint64_t                dataPacketsCount_;              /// number of data packets written so far
    uint64_t                indexPacketsCount_;             /// number of index packets written so far
};

//================================================================

class Encoder {
public:
    static Encoder*     EncoderFactory(unsigned bytestreamNumber,
                                       std::tr1::shared_ptr<CompressedVectorNodeImpl> cVector,
                                       std::vector<SourceDestBuffer>& sbuf,
                                       ustring& codecPath);

    virtual             ~Encoder(){};

    virtual uint64_t    processRecords(unsigned recordCount) = 0;
    virtual unsigned    sourceBufferNextIndex() = 0;
    virtual uint64_t    currentRecordIndex() = 0;
    virtual float       bitsPerRecord() = 0;
    virtual bool        registerFlushToOutput() = 0;

    virtual unsigned    outputAvailable() = 0;                                /// number of bytes that can be read
    virtual void        outputRead(char* dest, unsigned byteCount) = 0;       /// get data from encoder
    virtual void        outputClear() = 0;

    virtual void        sourceBufferSetNew(std::vector<SourceDestBuffer>& sbufs) = 0;
    virtual unsigned    outputGetMaxSize() = 0;
    virtual void        outputSetMaxSize(unsigned byteCount) = 0;

    unsigned            bytestreamNumber() {return(bytestreamNumber_);};

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
                        Encoder(unsigned bytestreamNumber);

    unsigned            bytestreamNumber_;
};

//================================================================

class BitpackEncoder : public Encoder {
public:
    virtual uint64_t    processRecords(unsigned recordCount) = 0;
    virtual unsigned    sourceBufferNextIndex();
    virtual uint64_t    currentRecordIndex();
    virtual float       bitsPerRecord() = 0;
    virtual bool        registerFlushToOutput() = 0;

    virtual unsigned    outputAvailable();                                /// number of bytes that can be read
    virtual void        outputRead(char* dest, unsigned byteCount);       /// get data from encoder
    virtual void        outputClear();

    virtual void        sourceBufferSetNew(std::vector<SourceDestBuffer>& sbufs);
    virtual unsigned    outputGetMaxSize();
    virtual void        outputSetMaxSize(unsigned byteCount);

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
                        BitpackEncoder(unsigned bytestreamNumber, SourceDestBuffer& sbuf, unsigned outputMaxSize, unsigned alignmentSize);

    void                outBufferShiftDown();

    std::tr1::shared_ptr<SourceDestBufferImpl>  sourceBuffer_;

    std::vector<char>   outBuffer_;
    unsigned            outBufferFirst_;
    unsigned            outBufferEnd_;
    unsigned            outBufferAlignmentSize_;

    uint64_t            currentRecordIndex_;
};

//================================================================

class BitpackFloatEncoder : public BitpackEncoder {
public:
                        BitpackFloatEncoder(unsigned bytestreamNumber, SourceDestBuffer& sbuf, unsigned outputMaxSize, FloatPrecision precision);

    virtual uint64_t    processRecords(unsigned recordCount);
    virtual bool        registerFlushToOutput();
    virtual float       bitsPerRecord();

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
    FloatPrecision      precision_;    
};

//================================================================

template <typename RegisterT>
class BitpackIntegerEncoder : public BitpackEncoder {
public:
                        BitpackIntegerEncoder(bool isScaledInteger, unsigned bytestreamNumber, SourceDestBuffer& sbuf,
                                              unsigned outputMaxSize, int64_t minimum, int64_t maximum, double scale, double offset);

    virtual uint64_t    processRecords(unsigned recordCount);
    virtual bool        registerFlushToOutput();
    virtual float       bitsPerRecord();

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
    bool            isScaledInteger_;
    int64_t         minimum_;
    int64_t         maximum_;
    double          scale_;
    double          offset_;
    unsigned        bitsPerRecord_;
    uint64_t        sourceBitMask_;
    unsigned        registerBitsUsed_;
    RegisterT       register_;
};

//================================================================

class ConstantIntegerEncoder : public Encoder {
public:
                        ConstantIntegerEncoder(unsigned bytestreamNumber, SourceDestBuffer& sbuf, int64_t minimum);
    virtual uint64_t    processRecords(unsigned recordCount);
    virtual unsigned    sourceBufferNextIndex();
    virtual uint64_t    currentRecordIndex();
    virtual float       bitsPerRecord();
    virtual bool        registerFlushToOutput();

    virtual unsigned    outputAvailable();                                /// number of bytes that can be read
    virtual void        outputRead(char* dest, unsigned byteCount);       /// get data from encoder
    virtual void        outputClear();

    virtual void        sourceBufferSetNew(std::vector<SourceDestBuffer>& sbufs);
    virtual unsigned    outputGetMaxSize();
    virtual void        outputSetMaxSize(unsigned byteCount);

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
    std::tr1::shared_ptr<SourceDestBufferImpl>  sourceBuffer_;
    uint64_t            currentRecordIndex_;
    int64_t             minimum_;
};

//================================================================

class Decoder {
public:
    static Decoder*     DecoderFactory(unsigned bytestreamNumber,
                                       std::tr1::shared_ptr<CompressedVectorNodeImpl> cVector,
                                       std::vector<SourceDestBuffer>& dbufs,
                                       ustring& codecPath);
    virtual             ~Decoder() {};

    virtual void        destBufferSetNew(std::vector<SourceDestBuffer>& dbufs) = 0;
    virtual uint64_t    totalRecordsCompleted() = 0;
    virtual unsigned    inputProcess(const char* source, unsigned count) = 0;
    virtual void        stateReset() = 0;
    unsigned            bytestreamNumber() {return(bytestreamNumber_);};
#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout) = 0;
#endif
protected: //================
                        Decoder(unsigned bytestreamNumber);

    unsigned            bytestreamNumber_;
};

//??? into stateReset body
    /// discard any input queued
    /// doesn't change dbuf pointers

//================================================================

class BitpackDecoder : public Decoder {
public:
    virtual void        destBufferSetNew(std::vector<SourceDestBuffer>& dbufs);

    virtual uint64_t    totalRecordsCompleted() {return(currentRecordIndex_);};

    virtual unsigned    inputProcess(const char* source, unsigned byteCount);
    virtual unsigned    inputProcessAligned(const char* inbuf, unsigned firstBit, unsigned endBit) = 0;

    virtual void        stateReset();

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
                        BitpackDecoder(unsigned bytestreamNumber, SourceDestBuffer& dbuf, unsigned alignmentSize, uint64_t maxRecordCount);

    void                inBufferShiftDown();

    uint64_t            currentRecordIndex_;
    uint64_t            maxRecordCount_;

    std::tr1::shared_ptr<SourceDestBufferImpl> destBuffer_;

    std::vector<char>   inBuffer_;
    unsigned            inBufferFirstBit_;
    unsigned            inBufferEndByte_;
    unsigned            inBufferAlignmentSize_;
    unsigned            bitsPerWord_;
    unsigned            bytesPerWord_;
};

//================================================================

class BitpackFloatDecoder : public BitpackDecoder {
public:
                        BitpackFloatDecoder(unsigned bytestreamNumber, SourceDestBuffer& dbuf, FloatPrecision precision, uint64_t maxRecordCount);

    virtual unsigned    inputProcessAligned(const char* inbuf, unsigned firstBit, unsigned endBit);

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
    FloatPrecision      precision_;
};

//================================================================

template <typename RegisterT>
class BitpackIntegerDecoder : public BitpackDecoder {
public:
                        BitpackIntegerDecoder(bool isScaledInteger, unsigned bytestreamNumber, SourceDestBuffer& dbuf, 
                                              int64_t minimum, int64_t maximum, double scale, double offset, uint64_t maxRecordCount);

    virtual unsigned    inputProcessAligned(const char* inbuf, unsigned firstBit, unsigned endBit);

#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
    bool        isScaledInteger_;
    int64_t     minimum_;
    int64_t     maximum_;
    double      scale_;
    double      offset_;
    unsigned    bitsPerRecord_;
    RegisterT   destBitMask_;
};

//================================================================

class ConstantIntegerDecoder : public Decoder {
public:
                        ConstantIntegerDecoder(bool isScaledInteger, unsigned bytestreamNumber, SourceDestBuffer& dbuf, 
                                              int64_t minimum, double scale, double offset, uint64_t maxRecordCount);
    virtual void        destBufferSetNew(std::vector<SourceDestBuffer>& dbufs);
    virtual uint64_t    totalRecordsCompleted() {return(currentRecordIndex_);};
    virtual unsigned    inputProcess(const char* source, unsigned byteCount);
    virtual void        stateReset();
#ifdef E57_DEBUG
    virtual void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
    uint64_t            currentRecordIndex_;
    uint64_t            maxRecordCount_;

    std::tr1::shared_ptr<SourceDestBufferImpl> destBuffer_;

    bool                isScaledInteger_;
    int64_t             minimum_;
    double              scale_;
    double              offset_;
};

//================================================================

class PacketLock {
public:
                    ~PacketLock();

private: //================
    /// Can't be copied or assigned
                    PacketLock(const PacketLock& plock);
    PacketLock&     operator=(const PacketLock& plock);

protected: //================
    friend class PacketReadCache;
    /// Only PacketReadCache can construct
                     PacketLock(PacketReadCache* cache, unsigned cacheIndex);

    PacketReadCache* cache_;
    unsigned         cacheIndex_;
};

//================================================================

class PacketReadCache {
public:
                        PacketReadCache(CheckedFile* cFile, unsigned packetCount);
                        ~PacketReadCache();

    std::auto_ptr<PacketLock> lock(uint64_t packetLogicalOffset, char* &pkt);  //??? pkt could be const
    void                 markDiscarable(uint64_t packetLogicalOffset);

#ifdef E57_DEBUG
    void                dump(int indent = 0, std::ostream& os = std::cout);
#endif
protected: //================
    /// Only PacketLock can unlock the cache
    friend class PacketLock;
    void                unlock(unsigned cacheIndex);

    void                readPacket(unsigned oldestEntry, uint64_t packetLogicalOffset);

    struct CacheEntry {
        uint64_t    logicalOffset_;
        char*       buffer_;  //??? could be const?
        unsigned    lastUsed_;
    };

    unsigned            lockCount_;
    unsigned            useCount_;
    CheckedFile*        cFile_;
    std::vector<CacheEntry>  entries_;
};

}; /// end namespace e57

#endif // E57FOUNDATIONIMPL_H_INCLUDED
