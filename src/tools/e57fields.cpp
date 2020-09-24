/*
 * e57fields.cpp - print summary statistics of field use in an E57 file.
 *
 * Copyright 2009 - 2010 Kevin Ackley (kackley@gwi.net)
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <iostream>
#include <sstream>
#include <iomanip>
#include <float.h>
#include <map>
#if defined(_MSC_VER)
#   include <memory>
#elif defined(__APPLE__)
#   include <memory>
#else
#   include <tr1/memory>
using namespace std::tr1;
#endif
#include "E57Foundation.h"
#include "E57FoundationImpl.h" //??? for exceptions, should be in separate file

#include <boost/math/special_functions/fpclassify.hpp>
using boost::math::fpclassify;

using namespace e57;
using namespace std;

//!!! prologue, file name, date, version#, total # elements
//!!! doc
//!!! -noScaling

/// Uncomment the line below to enable debug printing
//#define VERBOSE 1

ustring decimalString(uint64_t x) {std::ostringstream ss; ss << x; return(ss.str());};

struct CommandLineOptions {
    ustring startPath;
    ustring inputFileName;

            CommandLineOptions(){};
    void    parse(int argc, char** argv);
};


struct StatEntry {
    uint64_t    count;      // number of matching occurrences
    int64_t     ivalue;     // value of occurrence with integer values (or lengths)
    double      dvalue;     // value of occurrence with floating point values
    ustring     firstPath;  // path name of first matching occurrence

    StatEntry():count(0),ivalue(0),dvalue(0.0){};
};


struct OccurrenceStats {
    uint64_t    count;
    const char*       measure;
    ustring     firstPath;
    double      sum;

    /// Extrema
    StatEntry   minimum;
    StatEntry   maximum;

    /// Keep track occurrences of special floating point values
    StatEntry   zero;
    StatEntry   quietNan;
    StatEntry   signalingNan;
    StatEntry   infinity;
    StatEntry   denormalized;
    StatEntry   negativeZero;

    OccurrenceStats():count(0),measure("unknown"),sum(0.0){};
};


struct Statistics {
    map<ustring, OccurrenceStats> typeStats;
    map<pair<ustring,ustring>, OccurrenceStats> fieldTypeStats;
    map<pair<ustring,ustring>, OccurrenceStats> fieldDistinctTypeStats;

    Statistics(){};
};

/// Forward declarations for local functions
void gatherStats(Node n, Statistics& stats, ImageFile imf);
void printStats(CommandLineOptions& options, Statistics& stats, ImageFile imf);

int main(int argc, char** argv)
{
	/// Catch any exceptions thrown.
	try {
        CommandLineOptions options;
        options.parse(argc, argv);

        /// Read file from disk
        ImageFile imf(options.inputFileName, "r");
        StructureNode root = imf.root();

        Statistics stats;
        if (options.startPath == "")
            gatherStats(root, stats, imf);
        else {
            ///??? handle case where startPath is a compressed node (inside a CompressedVector).
            if (!root.isDefined(options.startPath)) {
                cerr << "Error: E57 path " << options.startPath << " not defined." << endl;
                return 0;
            }
            gatherStats(root.get(options.startPath), stats, imf);
        }

        printStats(options, stats, imf);
    } catch(E57Exception& ex) {
        ex.report(__FILE__, __LINE__, __FUNCTION__);
        return -1;
    } catch (std::exception& ex) {
        cerr << "Got an std::exception, what=" << ex.what() << endl;
        return -1;
    } catch (...) {
        cerr << "Got an unknown exception" << endl;
        return -1;
    }
    return 0;
}

//================================================================

void usage(ustring msg)
{
    cerr << "ERROR: " << msg << endl;
    cerr << "Usage:" << endl;
    cerr << "    e57fields <e57_file> [start_path]" << endl;
    cerr << "    For example:" << endl;
    cerr << "        e57fields scan0001.e57" << endl;
    cerr << "        e57fields scan0001.e57 /data3D/0/points" << endl;
    cerr << endl;
    exit(-1); //??? consistent with other utilities?
}


void CommandLineOptions::parse(int argc, char** argv)
{
    /// Skip program name
    argc--; argv++;

//??? no options yet
//    for (; argc > 0 && *argv[0] == '-'; argc--,argv++) {
//        if (strcmp(argv[0], "-unusedNotOptional") == 0)
//            unusedNotOptional = true;
//        else {
//            usage(ustring("unknown option: ") + argv[0]);
//            throw EXCEPTION("unknown option"); //??? UsageException
//        }
//    }

	if (argc != 1 && argc != 2)
        usage("wrong number of command line arguments");

    inputFileName = argv[0];
    if (argc > 1)
        startPath = argv[1];
}
//================================================================

ustring typeString(NodeType t)
{
    switch (t) {
        case E57_STRUCTURE:             return("Structure");
        case E57_VECTOR:                return("Vector");
        case E57_COMPRESSED_VECTOR:     return("CompressedVector");
        case E57_INTEGER:               return("Integer");
        case E57_SCALED_INTEGER:        return("ScaledInteger");
        case E57_FLOAT:                 return("Float");
        case E57_STRING:                return("String");
        case E57_BLOB:                  return("Blob");
        default:                    return("<unknown>");
    }
}

//================================================================
ustring minMaxString(int64_t minimum, int64_t maximum)
{
    /// Print min,max to string.  Detect if interval is equivalent to an even number of bits, and print shorthand instead.

    if (minimum == E57_INT64_MIN && maximum == E57_INT64_MAX)
        return("int64");
    ostringstream ss;
    for (unsigned i = 1; i < 62; i++) {
        if (minimum == 0 && maximum == (1LL<<i)-1) {
            ss << "uint" << i;
            return(ss.str());
        }
        if (minimum == -(1LL<<i) && maximum == (1LL<<i)-1) {
            ss << "int" << i+1;
            return(ss.str());
        }
    }

    /// Print special format if field requires zero bits (a constant value).
    if (minimum == maximum) {
        ss << "constant=" << minimum;
        return(ss.str());
    }

    ss << "min=" << minimum << ",max=" << maximum;
    return(ss.str());
}

//================================================================

ustring distinctTypeString(Node n)
{
    switch (n.type()) {
        case E57_STRUCTURE: {
            //StructureNode s(n);
            return("Structure");
        }
        break;
        case E57_VECTOR: {
            VectorNode v(n);
            if (v.allowHeteroChildren())
                return("Vector(heterogeneous)");
            else
                return("Vector(homogeneous)");
        }
        break;
        case E57_COMPRESSED_VECTOR: {
            //CompressedVectorNode cv(n);
            return("CompressedVector");
        }
        break;
        case E57_INTEGER: {
            IntegerNode i(n);
            ostringstream ss;
            ss << "Integer(" << minMaxString(i.minimum(), i.maximum()) << ")";
            return(ss.str());
        }
        break;
        case E57_SCALED_INTEGER: {
            ScaledIntegerNode si(n);
            ostringstream ss;
            ss.flags(ios::scientific);
            ss.precision(4);
            ss << "ScaledInteger(" << minMaxString(si.minimum(), si.maximum()) << ",scale=" << si.scale() << ",offset=" << si.offset() << ")";
            return(ss.str());
        }
        break;
        case E57_FLOAT: {
            FloatNode f(n);
            if (f.precision() == E57_SINGLE)
                return("Float(precision=single)");
            else
                return("Float(precision=double)");
        }
        break;
        case E57_STRING: {
            //StringNode s(n);
            return("String");
        }
        break;
        case E57_BLOB: {
            BlobNode b(n);
            return("Blob");
        }
        break;
        default:
            return("<unknown>");
    }
}

//================================================================

/// A class to delay the work of creating the pathname of a CompressedVector element until it is actually needed.
class GetPathFunctor {
public:
    GetPathFunctor(ustring path1, uint64_t recordNumber=0, ustring path2=""):path1_(path1),recordNumber_(recordNumber),path2_(path2){};

    ustring getPath();
    void    setRecordNumber(uint64_t recordNumber){recordNumber_ = recordNumber;};
protected: //================
    ustring     path1_;
    uint64_t    recordNumber_;
    ustring     path2_;
};

ustring GetPathFunctor::getPath()
{
    if (path2_ != "")
        return(path1_ + "/" + decimalString(recordNumber_) + path2_);   // path2_ already has leading "/"
    else
        return(path1_);
}

//================================================================


void calcInt64Stats(OccurrenceStats* occur, const char* measure, int64_t value, GetPathFunctor& getPathFunctor)
{
#ifdef VERBOSE
    cout << "calcInt64Stats() called, measure=" << measure << " value=" << value << " path=" << getPathFunctor.getPath() << endl;
#endif

    if (++(occur->count) == 1) {
        occur->measure = measure;
        occur->firstPath = getPathFunctor.getPath();
    }

    occur->sum += value;

    /// Compute minimum value
    if (occur->minimum.count == 0 || value < occur->minimum.ivalue) {
#ifdef VERBOSE
        cout << "  found new min:" << value << " old:" << occur->minimum.ivalue << endl;
#endif
        occur->minimum.ivalue = value;
        occur->minimum.count = 1;
        occur->minimum.firstPath = getPathFunctor.getPath();
    } else if (value == occur->minimum.ivalue)
        occur->minimum.count++;

    /// Compute maximum value
    if (occur->maximum.count == 0 || value > occur->maximum.ivalue) {
#ifdef VERBOSE
        cout << "  found new max:" << value << " old:" << occur->maximum.ivalue << endl;
#endif
        occur->maximum.ivalue = value;
        occur->maximum.count = 1;
        occur->maximum.firstPath = getPathFunctor.getPath();
    } else if (value == occur->maximum.ivalue)
        occur->maximum.count++;

    /// Record occurrences of zeros
    if (value == 0) {
        if (occur->zero.count == 0) {
            occur->zero.ivalue = value;
            occur->zero.firstPath = getPathFunctor.getPath();
        }
        occur->zero.count++;
    }
}

//================================================================

void calcDoubleStats(OccurrenceStats* occur, const char* measure, double value, GetPathFunctor& getPathFunctor)
{
#ifdef VERBOSE
//    cout << "calcDoubleStats() called, measure=" << measure << " value=" << value << " pathName=" << getPathFunctor.getPath() << endl;
#endif
    if (++(occur->count) == 1) {
        occur->measure = measure;
        occur->firstPath = getPathFunctor.getPath();
    }

    occur->sum += value;

    /// Compute minimum value
    if (occur->minimum.count == 0 || value < occur->minimum.dvalue) {
        occur->minimum.dvalue = value;
        occur->minimum.count = 1;
        occur->minimum.firstPath = getPathFunctor.getPath();
    } else if (value == occur->minimum.dvalue)
        occur->minimum.count++;

    /// Compute maximum value
    if (occur->maximum.count == 0 || value > occur->maximum.dvalue) {
        occur->maximum.dvalue = value;
        occur->maximum.count = 1;
        occur->maximum.firstPath = getPathFunctor.getPath();
    } else if (value == occur->maximum.dvalue)
        occur->maximum.count++;

    /// Record occurrences of zeros
    if (value == 0) {
        if (occur->zero.count == 0) {
            occur->zero.dvalue = value;
            occur->zero.firstPath = getPathFunctor.getPath();
        }
        occur->zero.count++;
    }

    switch (boost::math::fpclassify(value)) {
        case FP_NAN: // Quiet NaN
             if (occur->quietNan.count == 0) {
                occur->quietNan.dvalue = value;
                occur->quietNan.firstPath = getPathFunctor.getPath();
            }
            occur->quietNan.count++;
            break;
        case FP_INFINITE: // Negative or positive infinity
             if (occur->infinity.count == 0) {
                occur->infinity.dvalue = value;
                occur->infinity.firstPath = getPathFunctor.getPath();
            }
            occur->infinity.count++;
            break;
        case FP_SUBNORMAL:   // Negative or positive denormalized
             if (occur->denormalized.count == 0) {
                occur->denormalized.dvalue = value;
                occur->denormalized.firstPath = getPathFunctor.getPath();
            }
            occur->denormalized.count++;
            break;
        case FP_ZERO:   // Negative zero (-0)
            if (value < 0.0 && occur->negativeZero.count == 0) {
                occur->negativeZero.dvalue = value;
                occur->negativeZero.firstPath = getPathFunctor.getPath();
            }
            occur->negativeZero.count++;
            break;
        default:
            break;
    }
}

//================================================================

struct CVElementInfo {
    static const int BUFFER_ELEMENT_COUNT = 10*1024;

    CVElementInfo(Node cv_arg, Node protoNode_arg);

    Node            protoNode;

    /// The buffers that a CompressedVector element is read into.
    /// Only one is used, depending on the type of the E57 element.
    /// One of these three should be resized to BUFFER_ELEMENT_COUNT.
    /// These are smart pointers to avoid the copying (and the moving) when put on the cvElements list.
    boost::shared_ptr<vector<int64_t> > iBuffer;
    boost::shared_ptr<vector<double> >  dBuffer;
    boost::shared_ptr<vector<string> >  sBuffer;

    /// The precalculated parts of the element path name.
    /// The only part that is missing is the record number which goes in between.
    /// For example:  "/data3D/0/points" + "/" + recordNumber + "/" + "cartesianX"
    /// We wait until we actually need the path string to construct it, for performance reasons.

    GetPathFunctor  getPathFunctor; //???doc

    /// The predetermined stats areas for this element
    OccurrenceStats* typeOccur;
    OccurrenceStats* fieldTypeOccur;
    OccurrenceStats* fieldDistinctTypeOccur;
};

CVElementInfo::CVElementInfo(Node cv_arg, Node protoNode_arg)
: protoNode(protoNode_arg),
  iBuffer(new vector<int64_t>(0)),
  dBuffer(new vector<double>(0)),
  sBuffer(new vector<ustring>(0)),
  getPathFunctor(cv_arg.pathName(), 0, protoNode_arg.pathName())
{
}

//================================================================

void findAllNodes(Node n, vector<Node>& allNodes)
{
    /// Visit all nodes in a tree (including branch nodes) and put in a list.

    allNodes.push_back(n);

    switch (n.type()) {
        case E57_STRUCTURE: {
            StructureNode s(n);

            /// Recursively visit child nodes
            uint64_t childCount = s.childCount();
            for (uint64_t i = 0; i < childCount; i++)
                findAllNodes(s.get(i), allNodes);
        }
        break;
        case E57_VECTOR: {
            VectorNode v(n);

            /// Recursively visit child nodes
            uint64_t childCount = v.childCount();
            for (uint64_t i = 0; i < childCount; i++)
                findAllNodes(v.get(i), allNodes);
        }
        break;
    }
}

void makeCVInfo(CompressedVectorNode cv, vector<CVElementInfo>& cvElements,
                ImageFile imf, Statistics& stats, vector<SourceDestBuffer>& destBuffers)
{
    /// Get all nodes in prototype (branch and terminal) into a list.
    vector<Node> allNodes;
    findAllNodes(cv.prototype(), allNodes);

    /// For performance reasons, precompute a bunch of stuff about each node.
    /// Allocate an appropriate block transfer buffer (if needed) and put in the destBuffers list.
    for (unsigned i = 0; i < allNodes.size(); i++) {
        Node protoNode = allNodes.at(i);
        ustring protoPathName = protoNode.pathName();

        CVElementInfo einfo(cv, protoNode);

        ustring elementName;
        if (protoNode.isRoot() || protoNode.parent().type() == E57_VECTOR) {
            /// Use special string for field names of vector children (the real field names are string versions of integers like "0").
            elementName = "<0,1,2...>";
        } else
            elementName = protoNode.elementName();

        /// Pre-lookup the appropriate OccurrenceStats for this element, save in einfo
        ustring typeName = typeString(protoNode.type());
        ustring distinctTypeName = distinctTypeString(protoNode);
        pair<ustring,ustring> fieldAndType(elementName, typeName);
        pair<ustring,ustring> fieldAndDistinctType(elementName, distinctTypeName);
        einfo.typeOccur              = &stats.typeStats[typeName];
        einfo.fieldTypeOccur         = &stats.fieldTypeStats[fieldAndType];
        einfo.fieldDistinctTypeOccur = &stats.fieldDistinctTypeStats[fieldAndDistinctType];

        switch (protoNode.type()) {
            case E57_STRUCTURE:
            case E57_VECTOR:
                /// Don't need to record any more info for these type.
                break;
            case E57_INTEGER:
                /// Resize int64_t buffer and add it to destBuffers
                einfo.iBuffer->resize(CVElementInfo::BUFFER_ELEMENT_COUNT);
                destBuffers.push_back(SourceDestBuffer(imf, protoPathName, &(*(einfo.iBuffer))[0], einfo.iBuffer->size(), true, true));
                break;
            case E57_SCALED_INTEGER:
            case E57_FLOAT:
                /// Resize double buffer and add it to destBuffers
                /// Note we request scaling when create SourceDestBuffer, so scaled values will be transferred to buffer.
                einfo.dBuffer->resize(CVElementInfo::BUFFER_ELEMENT_COUNT);
                destBuffers.push_back(SourceDestBuffer(imf, protoPathName, &(*(einfo.dBuffer))[0], einfo.dBuffer->size(), true, true));
                break;
            case E57_STRING:
                /// Resize string buffer and add it to destBuffers
                einfo.sBuffer->resize(CVElementInfo::BUFFER_ELEMENT_COUNT);
                destBuffers.push_back(SourceDestBuffer(imf, protoPathName, &(*einfo.sBuffer))); //!!! change to buffer ref
                break;
            case E57_COMPRESSED_VECTOR:
            case E57_BLOB:
                /// These types shouldn't occur in prototype of a CompressedVector.
                break;
        }

        /// Add einfo to list
        cvElements.push_back(einfo);
    }
}

void gatherCompressedVectorStats(CompressedVectorNode cv, Statistics& stats, ImageFile imf)
{
#ifdef VERBOSE
    cout << "gathering stats on CompressedVector: " << cv.pathName() << endl;
#endif

    /// Make a list of all elements in prototype and allocate appropriate transfer buffers
    vector<CVElementInfo> cvElements;
    vector<SourceDestBuffer> destBuffers;
    makeCVInfo(cv, cvElements, imf, stats, destBuffers);

    CompressedVectorReader reader = cv.reader(destBuffers);

    unsigned gotCount = 0;
    uint64_t recordNumber = 0;
    do {
        /// Read a block of data into the buffers stored in each CVElementInfo
        gotCount = reader.read();

        for (unsigned elem = 0; elem < cvElements.size(); elem++) {
            CVElementInfo* cvElement = &cvElements.at(elem);
            Node protoNode = cvElement->protoNode;

            /// Update the getPathFunctor with the current recordNumber
            /// If the pathName of a CV element is needed (e.g. when new minimum found) this recordNumber will be used.
//???
            switch (protoNode.type()) {
                case E57_STRUCTURE: {
                    StructureNode s(protoNode);
                    int64_t value = s.childCount();  /// value is same for each record in CompressedVector
                    for (unsigned i = 0; i < gotCount; i++) {
                        cvElement->getPathFunctor.setRecordNumber(recordNumber+i);
                        calcInt64Stats(cvElement->typeOccur,              "childCount", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldTypeOccur,         "childCount", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldDistinctTypeOccur, "childCount", value, cvElement->getPathFunctor);
                    }
                }
                break;
                case E57_VECTOR: {
                    VectorNode v(protoNode);
                    int64_t value = v.childCount();  /// value is same for each record in CompressedVector
                    for (unsigned i = 0; i < gotCount; i++) {
                        cvElement->getPathFunctor.setRecordNumber(recordNumber+i);
                        calcInt64Stats(cvElement->typeOccur,              "childCount", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldTypeOccur,         "childCount", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldDistinctTypeOccur, "childCount", value, cvElement->getPathFunctor);
                    }
                }
                break;
                case E57_INTEGER: {
                    int64_t* p = &cvElement->iBuffer->at(0);
                    for (unsigned i = 0; i < gotCount; i++,p++) {
                        int64_t value = *p;
                        cvElement->getPathFunctor.setRecordNumber(recordNumber+i);
                        calcInt64Stats(cvElement->typeOccur,              "value", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldTypeOccur,         "value", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldDistinctTypeOccur, "value", value, cvElement->getPathFunctor);
                    }
                }
                break;
                case E57_SCALED_INTEGER:
                case E57_FLOAT: {
                    double* p = &cvElement->dBuffer->at(0);
                    for (unsigned i = 0; i < gotCount; i++,p++) {
                        double value = *p;
                        cvElement->getPathFunctor.setRecordNumber(recordNumber+i);
                        calcDoubleStats(cvElement->typeOccur,              "value", value, cvElement->getPathFunctor);
                        calcDoubleStats(cvElement->fieldTypeOccur,         "value", value, cvElement->getPathFunctor);
                        calcDoubleStats(cvElement->fieldDistinctTypeOccur, "value", value, cvElement->getPathFunctor);
                    }
                }
                break;
                case E57_STRING: {
                    for (unsigned i = 0; i < gotCount; i++) {
                        int64_t value = cvElement->sBuffer->at(i).length();  // Get length of next string in transfer buffer
                        cvElement->getPathFunctor.setRecordNumber(recordNumber+i);
                        calcInt64Stats(cvElement->typeOccur,              "length", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldTypeOccur,         "length", value, cvElement->getPathFunctor);
                        calcInt64Stats(cvElement->fieldDistinctTypeOccur, "length", value, cvElement->getPathFunctor);
                    }
                }
                break;
                case E57_BLOB:
                case E57_COMPRESSED_VECTOR:
                    /// These shouldn't happen in a CompressedVector
                    break;
            }
        }
        recordNumber += gotCount;
    } while (gotCount > 0);
    reader.close();
}


//================================================================

void gatherStats(Node n, Statistics& stats, ImageFile imf)
{
#ifdef VERBOSE
    cout << "gathering stats on node " << n.pathName() << endl;
#endif

    /// Use special string for field names of vector children (the real field names are string versions of integers like "0").
    ustring elementName;
    if (!n.isRoot() && n.parent().type() == E57_VECTOR)
        elementName = "<0,1,2...>";
    else
        elementName = n.elementName();

    /// Handle the field name of the root node
    if (elementName == "")
        elementName = "/";

    ustring typeName = typeString(n.type());
    ustring distinctTypeName = distinctTypeString(n);

    /// Get OccurrenceStats for E57 element type (e.g. "Integer");
    OccurrenceStats* typeOccur = &stats.typeStats[typeName];

    /// Get OccurrenceStats for combination of elementName and type (e.g. "cartesianX" and "Integer")
    pair<ustring,ustring> fieldAndType(elementName, typeName);
    OccurrenceStats* fieldTypeOccur = &stats.fieldTypeStats[fieldAndType];

    /// Get OccurrenceStats for combination of elementName and distinct type (e.g. "cartesianX" and "Integer(minimum=0,maximum=1023)")
    pair<ustring,ustring> fieldAndDistinctType(elementName, distinctTypeName);
    OccurrenceStats* fieldDistinctTypeOccur = &stats.fieldDistinctTypeStats[fieldAndDistinctType];

    /// A class to delay work of making a pathname.  Really needed in CompressedVector elements case, not here.
    GetPathFunctor getPathFunctor(n.pathName());

    switch (n.type()) {
        case E57_STRUCTURE: {
            StructureNode s(n);
            calcInt64Stats(typeOccur, "childCount", s.childCount(), getPathFunctor);
            calcInt64Stats(fieldTypeOccur, "childCount", s.childCount(), getPathFunctor);
            calcInt64Stats(fieldDistinctTypeOccur, "childCount", s.childCount(), getPathFunctor);

            /// Gather stats on all of children
            uint64_t childCount = s.childCount();
            for (uint64_t i = 0; i < childCount; i++)
                gatherStats(s.get(i), stats, imf);
        }
        break;
        case E57_VECTOR: {
            VectorNode v(n);
            calcInt64Stats(typeOccur, "childCount", v.childCount(), getPathFunctor);
            calcInt64Stats(fieldTypeOccur, "childCount", v.childCount(), getPathFunctor);
            calcInt64Stats(fieldDistinctTypeOccur, "childCount", v.childCount(), getPathFunctor);

            /// Gather stats on all of children
            uint64_t childCount = v.childCount();
            for (uint64_t i = 0; i < childCount; i++)
                gatherStats(v.get(i), stats, imf);
        }
        break;
        case E57_COMPRESSED_VECTOR: {
            CompressedVectorNode cv(n);
            calcInt64Stats(typeOccur, "childCount", cv.childCount(), getPathFunctor);
            calcInt64Stats(fieldTypeOccur, "childCount", cv.childCount(), getPathFunctor);
            calcInt64Stats(fieldDistinctTypeOccur, "childCount", cv.childCount(), getPathFunctor);

            gatherCompressedVectorStats(cv, stats, imf);
        }
        break;
        case E57_INTEGER: {
            IntegerNode i(n);
            calcInt64Stats(typeOccur, "value", i.value(), getPathFunctor);
            calcInt64Stats(fieldTypeOccur, "value", i.value(), getPathFunctor);
            calcInt64Stats(fieldDistinctTypeOccur, "value", i.value(), getPathFunctor);
        }
        break;
        case E57_SCALED_INTEGER: {
            ScaledIntegerNode si(n);
            calcDoubleStats(typeOccur, "value", si.scaledValue(), getPathFunctor);
            calcDoubleStats(fieldTypeOccur, "value", si.scaledValue(), getPathFunctor);
            calcDoubleStats(fieldDistinctTypeOccur, "value", si.scaledValue(), getPathFunctor);
        }
        break;
        case E57_FLOAT: {
            FloatNode f(n);
            calcDoubleStats(typeOccur, "value", f.value(), getPathFunctor);
            calcDoubleStats(fieldTypeOccur, "value", f.value(), getPathFunctor);
            calcDoubleStats(fieldDistinctTypeOccur, "value", f.value(), getPathFunctor);
        }
        break;
        case E57_STRING: {
            StringNode s(n);
            calcInt64Stats(typeOccur, "length", s.value().length(), getPathFunctor);
            calcInt64Stats(fieldTypeOccur, "length", s.value().length(), getPathFunctor);
            calcInt64Stats(fieldDistinctTypeOccur, "length", s.value().length(), getPathFunctor);
        }
        break;
        case E57_BLOB: {
            BlobNode b(n);
            calcInt64Stats(typeOccur, "length", b.byteCount(), getPathFunctor);
            calcInt64Stats(fieldTypeOccur, "length", b.byteCount(), getPathFunctor);
            calcInt64Stats(fieldDistinctTypeOccur, "length", b.byteCount(), getPathFunctor);
        }
        break;
    }
}

//================================================================

void printExtrema(const StatEntry* stats, ustring elementName, ustring typeName, ustring statName, ustring measure)
{
    if (stats->count == 0)
        return;

    ostringstream ss;
    ss <<        setw(10) << right << stats->count;
    ss << " " << setw(27) << left  << elementName;
    ss << " " << setw(16) << left  << typeName;
    ss << " " << setw(15) << left  << statName + ustring(" ") + measure;
    if (stats->ivalue == 0)
        ss << " " << setw(12) << right << setprecision(5) << stats->dvalue;
    else
        ss << " " << setw(12) << right << setprecision(5) << stats->ivalue;
    ss << " " << setw(0)  << left  << stats->firstPath;

    /// Send formatted string to standard out
    cout << ss.str() << endl;
}

//================================================================

void printStats(CommandLineOptions& options, Statistics& stats, ImageFile imf)
{
    //??? other info here?
    cout << "Summary of file: " << options.inputFileName << endl;
    cout << endl;

    cout << "Field usage:" << endl;
    cout << "============" << endl;
    cout << "                                                                                                          minima     maxima" << endl;
    cout << "count      field name                  element type     measure    minimum      mean         maximum      count      count      first occurrence" << endl;
    cout << "========== =========================== ================ ========== ============ ============ ============ ========== ========== =================================" << endl;

    uint64_t totalElementCount = 0;
    map<pair<ustring,ustring>, OccurrenceStats>::const_iterator iter2;
    for (iter2=stats.fieldTypeStats.begin(); iter2 != stats.fieldTypeStats.end(); ++iter2) {
        ustring elementName          = iter2->first.first;
        ustring typeName             = iter2->first.second;
        const OccurrenceStats* occur = &iter2->second;

        ostringstream ss;
        ss <<        setw(10) << right << occur->count;
        ss << " " << setw(27) << left  << elementName;
        ss << " " << setw(16) << left  << typeName;
        ss << " " << setw(10) << left  << occur->measure;
        if (occur->minimum.ivalue == 0)
            ss << " " << setw(12) << right << setprecision(5) << occur->minimum.dvalue;
        else
            ss << " " << setw(12) << right << setprecision(5) << occur->minimum.ivalue;
        ss << " " << setw(12) << right << setprecision(5) << occur->sum/occur->count;
        if (occur->maximum.ivalue == 0)
            ss << " " << setw(12) << right << setprecision(5) << occur->maximum.dvalue;
        else
            ss << " " << setw(12) << right << setprecision(5) << occur->maximum.ivalue;
        ss << " " << setw(10) << right << occur->minimum.count;
        ss << " " << setw(10) << right << occur->maximum.count;
        ss << " " << setw(0)  << left  << occur->firstPath;

        /// Send formatted string to standard out
        cout << ss.str() << endl;

        totalElementCount += occur->count;
    }
    cout << "==========" << endl;
    {
        ostringstream ss;
        ss <<  setw(10) << right << totalElementCount;
        cout << ss.str() << " Total" << endl;
    }
    cout << endl;
    cout << endl;

#if 0 //!!! not yet
    cout << "Extensions:" << endl;
    cout << "===========" << endl;
    cout << "prefix     URI" << endl;
    cout << "========== ==================================" << endl;
    for (int i = 0; i < imf.extensionsCount(); i++) {
        ostringstream ss;
        ss <<        setw(10) << left << imf.extensionsPrefix(i);
        ss << " " << setw(0)  << left << imf.extensionsUri(i);
        cout << ss.str() << endl;
    }
    cout << endl;
    cout << endl;
#endif //!!!

    cout << "E57 type usage:" << endl;
    cout << "===============" << endl;
    cout << "                                                                              minima     maxima" << endl;
    cout << "count      element type     measure    minimum      mean         maximum      count      count" << endl;
    cout << "========== ================ ========== ============ ============ ============ ========== ==========" << endl;

    totalElementCount = 0;
    map<ustring, OccurrenceStats>::const_iterator iter;
    for (iter=stats.typeStats.begin(); iter != stats.typeStats.end(); ++iter) {
        ustring typeName             = iter->first;
        const OccurrenceStats* occur = &iter->second;

        ostringstream ss;
        ss <<        setw(10) << right << occur->count;
        ss << " " << setw(16) << left  << typeName;
        ss << " " << setw(10) << left  << occur->measure;
        if (occur->minimum.ivalue == 0)
            ss << " " << setw(12) << right << setprecision(5) << occur->minimum.dvalue;
        else
            ss << " " << setw(12) << right << setprecision(5) << occur->minimum.ivalue;
        ss << " " << setw(12) << right << occur->sum/occur->count;
        if (occur->maximum.ivalue == 0)
            ss << " " << setw(12) << right << setprecision(5) << occur->maximum.dvalue;
        else
            ss << " " << setw(12) << right << setprecision(5) << occur->maximum.ivalue;
        ss << " " << setw(10) << right << occur->minimum.count;
        ss << " " << setw(10) << right << occur->maximum.count;

        /// Send formatted string to standard out
        cout << ss.str() << endl;

        totalElementCount += occur->count;
    }
    cout << "==========" << endl;
    {
        ostringstream ss;
        ss <<  setw(10) << right << totalElementCount;
        cout << ss.str() << " Total" << endl;
    }
    cout << endl;
    cout << endl;

    cout << "Field and distinct type usage:" << endl;
    cout << "==============================" << endl;
    cout << "count      field name                  distinct type                       first occurrence" << endl;
    cout << "========== =========================== =================================== =======================================" << endl;
    totalElementCount = 0;
    map<pair<ustring,ustring>, OccurrenceStats>::const_iterator iter3;
    for (iter3=stats.fieldDistinctTypeStats.begin(); iter3 != stats.fieldDistinctTypeStats.end(); ++iter3) {
        ustring elementName          = iter3->first.first;
        ustring distinctTypeName     = iter3->first.second;
        const OccurrenceStats* occur = &iter3->second;

        ostringstream ss;
        ss <<        setw(10) << right << occur->count;
        ss << " " << setw(27) << left  << elementName;
        ss << " " << setw(35) << left  << distinctTypeName;
        ss << " " << setw(0)  << left  << occur->firstPath;

        /// Send formatted string to standard out
        cout << ss.str() << endl;

        totalElementCount += occur->count;
    }
    cout << "==========" << endl;
    {
        ostringstream ss;
        ss <<  setw(10) << right << totalElementCount;
        cout << ss.str() << " Total" << endl;
    }
    cout << endl;
    cout << endl;

    cout << "Extremum locations:" << endl;
    cout << "===================" << endl;
    cout << "count      field name                  element type     extremum        value        first occurrence" << endl;
    cout << "========== =========================== ================ =============== ============ =======================================" << endl;
    map<pair<ustring,ustring>, OccurrenceStats>::const_iterator iter4;
    for (iter4=stats.fieldTypeStats.begin(); iter4 != stats.fieldTypeStats.end(); ++iter4) {
        ustring elementName          = iter4->first.first;
        ustring typeName             = iter4->first.second;
        const OccurrenceStats* occur = &iter4->second;

        printExtrema(&occur->minimum, elementName, typeName, "min", occur->measure);
        printExtrema(&occur->maximum, elementName, typeName, "max", occur->measure);
        printExtrema(&occur->zero, elementName, typeName, "zero", occur->measure);
        printExtrema(&occur->quietNan, elementName, typeName, "qNan", occur->measure);
        printExtrema(&occur->signalingNan, elementName, typeName, "sNan", occur->measure);
        printExtrema(&occur->infinity, elementName, typeName, "inf", occur->measure);
        printExtrema(&occur->denormalized, elementName, typeName, "denormal", occur->measure);
        printExtrema(&occur->negativeZero, elementName, typeName, "-0", occur->measure);
    }
    cout << endl;
}
