/*
 * E57FoundationImpl.cpp - implementation of private functions of prototype XIF API.
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
#include <sstream>
//#include <memory> //??? needed?
#include <fstream> //??? needed?
#include <iomanip> //??? needed?
#include <cmath> //??? needed?
#include <float.h> //??? needed?

#include <sstream>
#include <io.h> //???needed?
#include <fcntl.h> //???needed?
#include <sys\stat.h>

#include "E57FoundationImpl.h"
using namespace e57;
using namespace std;
using namespace std::tr1;

///============================================================================================================
///============================================================================================================
///============================================================================================================

struct E57FileHeader {
    char        fileSignature[8];
    uint32_t    majorVersion;
    uint32_t    minorVersion;
    uint64_t    filePhysicalLength;
    uint64_t    xmlPhysicalOffset;
    uint64_t    xmlLogicalLength;
#ifdef E57_BIGENDIAN
    void        swab();
#else
    void        swab(){};
#endif
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

#ifdef E57_BIGENDIAN
void E57FileHeader::swab()
{
    /// Byte swap fields in-place, if CPU is BIG_ENDIAN
    SWAB(&majorVersion);
    SWAB(&minorVersion);
    SWAB(&filePhysicalLength);
    SWAB(&xmlPhysicalOffset);
    SWAB(&xmlLogicalLength);
};
#endif

#ifdef E57_DEBUG
void E57FileHeader::dump(int indent, std::ostream& os)
{
    os << space(indent) << "fileSignature:      "
       << fileSignature[0] << fileSignature[1] << fileSignature[2] << fileSignature[3]
       << fileSignature[4] << fileSignature[5] << fileSignature[6] << fileSignature[7] << endl;
    os << space(indent) << "majorVersion:       " << majorVersion << endl;
    os << space(indent) << "minorVersion:       " << minorVersion << endl;
    os << space(indent) << "filePhysicalLength: " << filePhysicalLength << endl;
    os << space(indent) << "xmlPhysicalOffset:  " << xmlPhysicalOffset << endl;
    os << space(indent) << "xmlLogicalLength:   " << xmlLogicalLength << endl;
}
#endif

struct BlobSectionHeader {
    uint8_t     sectionId;              // = E57_BLOB_SECTION
    uint8_t     reserved1[7];           // must be zero
    uint64_t    sectionLogicalLength;   // byte length of whole section
#ifdef E57_BIGENDIAN
    void        swab();
#else
    void        swab(){};
#endif
#ifdef E57_DEBUG
    void        dump(int indent = 0, std::ostream& os = std::cout);
#endif
};

#ifdef E57_BIGENDIAN
void BlobSectionHeader::swab()
{
    /// Byte swap fields in-place if CPU is BIG_ENDIAN
    SWAB(&sectionLogicalLength);
};
#endif

#ifdef E57_DEBUG
void BlobSectionHeader::dump(int indent, std::ostream& os)
{
    os << space(indent) << "sectionId:            " << sectionId << endl;
    os << space(indent) << "sectionLogicalLength: " << sectionLogicalLength << endl;
}
#endif

///================================================================
///================================================================
///================================================================

NodeImpl::NodeImpl(weak_ptr<ImageFileImpl> fileParent)
: fileParent_(fileParent)
{}

ustring NodeImpl::pathName(){
    if (isRoot())
        return("/");
    else {
        shared_ptr<NodeImpl> p(parent_);
        if (p->isRoot())
            return("/" + fieldName_);
        else
            return(p->pathName() + "/" + fieldName_);
    }
}

ustring NodeImpl::relativePathName(shared_ptr<NodeImpl> origin, ustring childPathName)
{
    if (origin == shared_from_this())
        return(childPathName);

    if (isRoot())
        /// Got to top and didn't find origin, must be error
        throw EXCEPTION("origin not found");
    else {
        /// Assemble relativePathName from right to left, recursively
        shared_ptr<NodeImpl> p(parent_);
        if (childPathName == "")
            return(p->relativePathName(origin, fieldName_));
        else
            return(p->relativePathName(origin, fieldName_ + "/" + childPathName));
    }
}

void NodeImpl::setParent(shared_ptr<NodeImpl> parent, const ustring& fieldName)
{
    parent_    = parent;
    fieldName_ = fieldName;
}

shared_ptr<NodeImpl> NodeImpl::getRoot()
{
    shared_ptr<NodeImpl> p(shared_from_this());
    while (!p->isRoot())
        p = shared_ptr<NodeImpl>(p->parent_);  //??? check if bad ptr?
    return(p);
}

//??? use visitor?
bool NodeImpl::isTypeConstrained()
{
    /// A node is type constrained if any of its parents is an homo VECTOR or COMPRESSED_VECTOR with more than one child
    shared_ptr<NodeImpl> p(shared_from_this());
    while (!p->isRoot()) {
        /// We have a parent since we are not root
        p = shared_ptr<NodeImpl>(p->parent_);  //??? check if bad ptr?

        switch (p->type()) {
            case E57_VECTOR:
                {
                    /// Downcast to shared_ptr<VectorNodeImpl>
                    shared_ptr<VectorNodeImpl> ai(dynamic_pointer_cast<VectorNodeImpl>(p));
                    if (!ai)
                        throw(EXCEPTION("bad downcast"));
                    if (!ai->allowHeteroChildren() && ai->childCount() > 1)
                        return(true);
                }
                break;
            case E57_COMPRESSED_VECTOR:
                /// Can't make any type changes to CompressedVector prototype.  ??? what if hasn't been written to yet
                return(true);
        }
    }
    /// Didn't find any constraining VECTORs or COMPRESSED_VECTORs in path above us, so our type is not constrained.
    return(false);
}

void NodeImpl::checkBuffers(const vector<SourceDestBuffer>& sdbufs, bool allowMissing)  //??? convert sdbufs to vector of shared_ptr
{
    std::set<ustring> pathNames;
    for (unsigned i = 0; i < sdbufs.size(); i++) {
        ustring pathName = sdbufs.at(i).impl()->pathName();

        /// Check that all buffers are same size
        if (sdbufs.at(i).impl()->capacity() != sdbufs.at(0).impl()->capacity())
            throw EXCEPTION("buffers not same size");

        /// Add each pathName to set, error if already in set (a duplicate pathName in sdbufs)
        if (!pathNames.insert(pathName).second)
            throw EXCEPTION("duplicate field name");

        /// Check no bad fields in sdbufs
        if (!isDefined(pathName))
            throw EXCEPTION("undefined field name");
    }

    if (!allowMissing) {
        /// Traverse tree recursively, checking that all nodes are listed in sdbufs
        checkLeavesInSet(pathNames, shared_from_this());
    }
}

bool NodeImpl::findTerminalPosition(shared_ptr<NodeImpl> target, uint64_t& countFromLeft)
{
    if (this == &*target) //??? ok?
        return(true);

    switch (type()) {
        case E57_STRUCTURE: {
                StructureNodeImpl* sni = dynamic_cast<StructureNodeImpl*>(this);

                /// Recursively visit child nodes
                uint64_t childCount = sni->childCount();
                for (uint64_t i = 0; i < childCount; i++) {
                    if (sni->get(i)->findTerminalPosition(target, countFromLeft))
                        return(true);
                }
            }
            break;
        case E57_VECTOR: {
                VectorNodeImpl* vni = dynamic_cast<VectorNodeImpl*>(this);

                /// Recursively visit child nodes
                uint64_t childCount = vni->childCount();
                for (uint64_t i = 0; i < childCount; i++) {
                    if (vni->get(i)->findTerminalPosition(target, countFromLeft))
                        return(true);
                }
            }
            break;
        case E57_COMPRESSED_VECTOR:
            break;  //??? for now, don't search into contents of compressed vector
        case E57_INTEGER:
        case E57_SCALED_INTEGER:
        case E57_FLOAT:
        case E57_STRING:
        case E57_BLOB:
            countFromLeft++;
            break;
    }

    return(false);
}


#if 0 //!!!================================================================

//??? use visitor?
bool NodeImpl::treeTerminalPosition(shared_ptr<NodeImpl> ni, uint64_t& countFromLeft)
{
    if (*this == *ni)
        return(true);


    switch (type_) {
        case E57_STRUCTURE:
            {
                /// Recursively visit child nodes
                uint64_t childCount = sni->childCount();
                for (uint64_t i = 0; i < childCount; i++) {
                    if (treeTerminalPosition(sni->get(i), countFromLeft))
                        return;
                }
            }
            break;
        case E57_VECTOR:
            ///
        case E57_COMPRESSED_VECTOR:
            /// termialCount * recordCount
        case E57_INTEGER:
        case E57_SCALED_INTEGER:
        case E57_FLOAT:
        case E57_STRING:
        case E57_BLOB:
    }


}

//================================================================

class Visitor {
public:
    virtual void visit(StructureNode s) = 0;
    virtual void visit(VectorNode v) = 0;
    virtual void visit(CompressedVectorNode cv) = 0;
    virtual void visit(IntegerNode i) = 0;
    virtual void visit(ScaledIntegerNode si) = 0;
    virtual void visit(FloatNode f) = 0;
    virtual void visit(StringNode s) = 0;
    virtual void visit(BlobNode b) = 0;
protected: //================
    void acceptAllChildren(StructureNode s);
    void acceptAllChildren(VectorNode s);
};

void Visitor::acceptAllChildren(StructureNode s)
{
    /// Visit child nodes of structure
    uint64_t childCount = s.childCount();
    for (uint64_t i = 0; i < childCount; i++)
        s.get(i).accept(*this);
}

void Visitor::acceptAllChildren(VectorNode v)
{
    /// Visit child nodes of structure
    uint64_t childCount = v.childCount();
    for (uint64_t i = 0; i < childCount; i++)
        v.get(i).accept(*this);
}

//================================================================

TerminalsVisit

AllNodesVisit

class CountTerminalsFromLeft : Visitor {
public::
    static unsigned count(Node n, Node target) {
        CountTerminalsFromLeft vistor(target);
        n.accept(visitor);
        return(terminalCount_);
    }

    virtual void visit(StructureNode s)         {terminalMatch();acceptChildren(s)};
    virtual void visit(VectorNode v)            {terminalMatch();acceptChildren(v)};
    virtual void visit(CompressedVectorNode cv) {terminalMatch()};
    virtual void visit(IntegerNode i)           {terminalMatch();};
    virtual void visit(ScaledIntegerNode i)     {terminalMatch();};
    virtual void visit(FloatNode f)             {terminalMatch();};
    virtual void visit(StringNode s)            {terminalMatch();};
    virtual void visit(BlobNode b)              {terminalMatch();};

protected: //================
    void terminalMatch(){
        if (gotMatch_)
            return;
        if (i == target_) {
            gotMatch_ = true;
            return;
        }
        terminalCount_++;
    };

    CountTerminalsFromLeft(Node target):gotMatch_(false),target_(target),terminalCount_(0){};

    bool     gotMatch_;
    Node     target_;
    unsigned terminalCount_;
};

//================================================================


class TotalTerminalsVisitor : Visitor {
public:
    TotalTerminalsVisitor():terminalCount_(0){};

    virtual void visit(StructureNode s);
    virtual void visit(VectorNode v);

    virtual void visit(IntegerNode i){terminalCount_++;};
    virtual void visit(ScaledIntegerNode si){terminalCount_++;};
    virtual void visit(FloatNode f){terminalCount_++;};
    virtual void visit(StringNode s){terminalCount_++;};
    virtual void visit(BlobNode b){terminalCount_++;};

    uint64_t terminalCount(){return(terminalCount_);};

protected: //================
    uint64_t terminalCount_;
};

void TotalTerminalsVisitor::visit(StructureNode s)
{
    uint64_t childCount = s.childCount();
    for (uint64_t i = 0; i < childCount; i++)
        s.get(i)->accept(*this);
}

#endif //!!!================================================================

//================================================================================================
StructureNodeImpl::StructureNodeImpl(weak_ptr<ImageFileImpl> fileParent)
: NodeImpl(fileParent)
{}

//??? use visitor?
bool StructureNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    /// Same node type?
    if (ni->type() != E57_STRUCTURE)
        return(false);

    /// Downcast to shared_ptr<StructureNodeImpl>, should succeed
    shared_ptr<StructureNodeImpl> si(dynamic_pointer_cast<StructureNodeImpl>(ni));
    if (!si)
        throw(EXCEPTION("bad downcast"));

    /// Same number of children?
    if (childCount() != si->childCount())
        return(false);

    /// Check each child is equivalent
    for (unsigned i = 0; i < childCount(); i++) {  //??? vector iterator?
        ustring myChildsFieldName = children_.at(i)->fieldName();
        /// Check if matching field name is in same position (to speed things up)
        if (myChildsFieldName == si->children_.at(i)->fieldName()) {
            if (!children_.at(i)->isTypeEquivalent(si->children_.at(i)))
                return(false);
        } else {
            /// Children in different order, so lookup by name and check if equal to our child
            if (!si->isDefined(myChildsFieldName))
                return(false);
            if (!children_.at(i)->isTypeEquivalent(si->lookup(myChildsFieldName)))
                return(false);
        }
    }

    /// Types match
    return(true);
}

bool StructureNodeImpl::isDefined(const ustring& pathName)
{
    shared_ptr<NodeImpl> ni(lookup(pathName));
    return(ni != 0);
}

shared_ptr<NodeImpl> StructureNodeImpl::get(int64_t index)
{
    if (index < 0 || index >= children_.size())
        throw EXCEPTION("index out of bounds");
    return(children_.at(static_cast<unsigned>(index)));
}


shared_ptr<NodeImpl> StructureNodeImpl::get(const ustring& pathName)
{
    shared_ptr<NodeImpl> ni(lookup(pathName));
    if (!ni)
        throw EXCEPTION("bad path");
    return(ni);
}

shared_ptr<NodeImpl> StructureNodeImpl::lookup(const ustring& pathName)
{
    //??? use lookup(fields, level) instead, for speed.
    bool isRelative;
    vector<ustring> fields;
    shared_ptr<ImageFileImpl> imf(fileParent_);
    imf->pathNameParse(pathName, isRelative, fields);

    if (isRelative || isRoot()) {
        if (fields.size() == 0)
            return(shared_ptr<NodeImpl>());  /// empty pointer
        else {
            /// Find child with fieldName that matches first field in path
            unsigned i;
            for (i = 0; i < children_.size(); i++) {
                if (fields.at(0) == children_.at(i)->fieldName())
                    break;
            }
            if (i == children_.size())
                return(shared_ptr<NodeImpl>());  /// empty pointer
            if (fields.size() == 1) {
                return(children_.at(i));
            } else {
                //??? use level here rather than unparse
                /// Remove first field in path
                fields.erase(fields.begin());

                /// Call lookup on child object with remaining fields in path name
                return(children_.at(i)->lookup(imf->pathNameUnparse(true, fields)));
            }
        }
    } else {  /// Absolute pathname and we aren't at the root
        /// Find root of the tree
        shared_ptr<NodeImpl> root(shared_from_this()->getRoot());

        /// Call lookup on root
        return(root->lookup(pathName));
    }
}

void StructureNodeImpl::set(int64_t index64, shared_ptr<NodeImpl> ni)
{
    //??? check index64 too large for implementation
    unsigned index = static_cast<unsigned>(index64);

    /// Allow index == current number of elements, interpret as append
    if (index < 0 || index > children_.size())
        throw EXCEPTION("index out of bounds");

    /// Field name is string version of index value, e.g. "14"
    stringstream fieldName;
    fieldName << index;

    /// Allow to set just off end of vector, interpret as append
    if (index == children_.size()) {
        /// If this struct is type constrained, can't add new child
        if (isTypeConstrained())
            throw EXCEPTION("type constraint violation");

        ni->setParent(shared_from_this(), fieldName.str());
        children_.push_back(ni);
    } else {
        /// If this struct is type constrained, can't change type of child
        if (isTypeConstrained() && !children_.at(index)->isTypeEquivalent(ni))
            throw EXCEPTION("type constraint violation");

        ni->setParent(shared_from_this(), fieldName.str());
        children_.at(index) = ni;
    }
}

void StructureNodeImpl::set(const ustring& pathName, shared_ptr<NodeImpl> ni, bool autoPathCreate)
{
    //??? parse pathName! throw if impossible, absolute and multi-level paths...
    //??? enforce type constraints on path (non-zero index types match zero index types for VECTOR, COMPRESSED_VECTOR

    bool isRelative;
    vector<ustring> fields;

    /// Path may be absolute or relative with several levels.  Break string into individual levels.
    shared_ptr<ImageFileImpl> imf(fileParent_);
    imf->pathNameParse(pathName, isRelative, fields);
    if (isRelative) {
        /// Relative path, starting from current object, e.g. "foo/17/bar"
        set(fields, 0, ni, autoPathCreate);
    } else {
        /// Absolute path (starting from root), e.g. "/foo/17/bar"
        getRoot()->set(fields, 0, ni, autoPathCreate);
    }
}

void StructureNodeImpl::set(const vector<ustring>& fields, int level, shared_ptr<NodeImpl> ni, bool autoPathCreate)
{
    //??? check if field is numeric string (e.g. "17"), verify number is same as index, else throw bad_path
#ifdef E57_MAX_DEBUG
//    cout << "StructureNodeImpl::set: level=" << level << endl;
//    for (unsigned i = 0; i < fields.size(); i++)
//        cout << "  field[" << i << "]: " << fields.at(i) << endl;
#endif

    /// Serial search for matching field name, replace if already defined
    for (unsigned i = 0; i < children_.size(); i++) {
        if (fields.at(level) == children_.at(i)->fieldName()) {
            if (level == fields.size()-1) {
                /// If this struct is type constrained, can't change type of child
                if (isTypeConstrained() && !children_.at(i)->isTypeEquivalent(ni))
                    throw EXCEPTION("type constraint violation");

                ni->setParent(shared_from_this(), fields.at(level));
                children_.at(i) = ni;
            } else {
                /// Recurse on child
                children_.at(i)->set(fields, level+1, ni);
            }
            return;
        }
    }
    /// Didn't find matching field name, so have a new child.

    /// If this struct is type constrained, can't add new child
    if (isTypeConstrained())
        throw EXCEPTION("type constraint violation");

    /// Check if we are at bottom level
    if (level == fields.size()-1){
        /// At bottom, so append node at end of children
        ni->setParent(shared_from_this(), fields.at(level));
        children_.push_back(ni);
    } else {
        /// Not at bottom level, if not autoPathCreate have an error
        if (!autoPathCreate)
            throw(EXCEPTION("bad path"));

        //??? what if extra fields are numbers?

        /// Do autoPathCreate: Create nested Struct objects for extra field names in path
        shared_ptr<NodeImpl> parent(shared_from_this());
        for (;level != fields.size()-1; level++) {
            shared_ptr<StructureNodeImpl> child(new StructureNodeImpl(fileParent_));
            parent->set(fields.at(level), child);
            parent = child;
        }
        parent->set(fields.at(level), ni);
    }
}

void StructureNodeImpl::append(shared_ptr<NodeImpl> ni)
{
    /// Create new node at end of list with integer field name
    set(childCount(), ni);
}

//??? use visitor?
void StructureNodeImpl::checkLeavesInSet(const std::set<ustring>& pathNames, shared_ptr<NodeImpl> origin)
{
    /// Not a leaf node, so check all our children
    for (unsigned i = 0; i < children_.size(); i++)
        children_.at(i)->checkLeavesInSet(pathNames, origin);
}

//??? use visitor?
void StructureNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    cf << space(indent) << "<" << fname << " type=\"Structure\"";

    /// If this struct is the root, add name space declarations
    if (isRoot()) {
        bool gotDefaultNamespace = false;
        for (int i = 0; i < imf->extensionsCount(); i++) {
            char* xmlnsExtension;
            if (imf->extensionsPrefix(i) == "") {
                gotDefaultNamespace = true;
                xmlnsExtension = "xmlns";
            } else
                xmlnsExtension = "xmlns:";
            cf << "\n" << space(indent+fname.length()+2) << xmlnsExtension << imf->extensionsPrefix(i) << "=\"" << imf->extensionsUri(i) << "\"";
        }

        /// If user didn't explicitly declare a default namespace, use the current E57 standard one.
        if (!gotDefaultNamespace)
            cf << "\n" << space(indent+fname.length()+2) << "xmlns=\"" << E57_V1_0_URI << "\"";
    }
    cf << ">\n";

    /// Write all children nested inside Structure element
    for (unsigned i = 0; i < children_.size(); i++)
        children_.at(i)->writeXml(imf, cf, indent+2);

    /// Write closing tag
    cf << space(indent) << "</" << fname << ">\n";
}

//??? use visitor?
#ifdef E57_DEBUG
void StructureNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:      Struct" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName: " << fieldName_ << endl;
    os << space(indent) << "path:      " << pathName() << endl;
    for (unsigned i = 0; i < children_.size(); i++) {
        os << space(indent) << "child[" << i << "]:" << endl;
        children_.at(i)->dump(indent+2, os);
    }
}
#endif

//=============================================================================
VectorNodeImpl::VectorNodeImpl(weak_ptr<ImageFileImpl> fileParent, bool allowHeteroChildren)
: StructureNodeImpl(fileParent),
  allowHeteroChildren_(allowHeteroChildren)
{}

bool VectorNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    /// Same node type?
    if (ni->type() != E57_VECTOR)
        return(false);

    /// Downcast to shared_ptr<VectorNodeImpl>
    shared_ptr<VectorNodeImpl> ai(dynamic_pointer_cast<VectorNodeImpl>(ni));
    if (!ai)
        throw(EXCEPTION("bad downcast"));

    /// allowHeteroChildren must match
    if (allowHeteroChildren_ != ai->allowHeteroChildren_)
        return(false);

    /// Same number of children?
    if (childCount() != ai->childCount())
        return(false);

    /// Check each child, must be in same order
    for (unsigned i = 0; i < childCount(); i++) {
        if (!children_.at(i)->isTypeEquivalent(ai->children_.at(i)))
            return(false);
    }

    /// Types match
    return(true);
}

void VectorNodeImpl::set(int64_t index64, shared_ptr<NodeImpl> ni)
{
    if (!allowHeteroChildren_) {
        /// New node type must match all existing children
        for (unsigned i = 0; i < children_.size(); i++) {
            if (!children_.at(i)->isTypeEquivalent(ni))
                throw EXCEPTION("type constraint violation");
        }
    }

    ///??? for now, use base implementation
    StructureNodeImpl::set(index64, ni);
}

void VectorNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    cf << space(indent) << "<" << fname << " type=\"Vector\" allowHeterogeneousChildren=\"" << static_cast<int64_t>(allowHeteroChildren_) << "\">\n";
    for (unsigned i = 0; i < children_.size(); i++)
        children_.at(i)->writeXml(imf, cf, indent+4, "vectorChild");
    cf << space(indent) << "</"<< fname << ">\n";
}

#ifdef E57_DEBUG
void VectorNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:      Vector" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName: " << fieldName_ << endl;
    os << space(indent) << "path:      " << pathName() << endl;
    os << space(indent) << "allowHeteroChildren: " << allowHeteroChildren() << endl;
    for (unsigned i = 0; i < children_.size(); i++) {
        os << space(indent) << "child[" << i << "]:" << endl;
        children_.at(i)->dump(indent+2, os);
    }
}
#endif

//=====================================================================================
SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, int8_t* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_INT8), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, uint8_t* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_UINT8), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, int16_t* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_INT16), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, uint16_t* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_UINT16), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, int32_t* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_INT32), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, uint32_t* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_UINT32), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, int64_t* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_INT64), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, bool* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_BOOL), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, float* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_REAL32), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, double* base, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_REAL64), base_(reinterpret_cast<char*>(base)), ustrings_(0),
  capacity_(capacity), doConversion_(doConversion), doScaling_(doScaling), stride_(stride), nextIndex_(0)
{}

SourceDestBufferImpl::SourceDestBufferImpl(weak_ptr<ImageFileImpl> fileParent, ustring pathName, vector<ustring>* b)
: fileParent_(fileParent), pathName_(pathName), elementType_(E57_USTRING), base_(0), ustrings_(b),
  capacity_(b->size()), doConversion_(false), doScaling_(false), stride_(0), nextIndex_(0)
{}

int64_t SourceDestBufferImpl::getNextInt64()
{
    /// Verify index is within bounds
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("index out of bounds");

    /// Fetch value from source buffer.
    /// Convert from non-integer formats if requested.
    char* p = &base_[nextIndex_*stride_];
    int64_t value;
    switch (elementType_) {
        case E57_INT8:
            value = static_cast<int64_t>(*reinterpret_cast<int8_t*>(p));
            break;
        case E57_UINT8:
            value = static_cast<int64_t>(*reinterpret_cast<uint8_t*>(p));
            break;
        case E57_INT16:
            value = static_cast<int64_t>(*reinterpret_cast<int16_t*>(p));
            break;
        case E57_UINT16:
            value = static_cast<int64_t>(*reinterpret_cast<uint16_t*>(p));
            break;
        case E57_INT32:
            value = static_cast<int64_t>(*reinterpret_cast<int32_t*>(p));
            break;
        case E57_UINT32:
            value = static_cast<int64_t>(*reinterpret_cast<uint32_t*>(p));
            break;
        case E57_INT64:
            value = *reinterpret_cast<int64_t*>(p);
            break;
        case E57_BOOL:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            /// Convert bool to 0/1, all non-zero values map to 1.0
            value = (*reinterpret_cast<bool*>(p)) ? 1 : 0;
            break;
        case E57_REAL32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            //??? fault if get special value: NaN, NegInf...
            value = static_cast<int64_t>(*reinterpret_cast<float*>(p));
            break;
        case E57_REAL64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            //??? fault if get special value: NaN, NegInf...
            value = static_cast<int64_t>(*reinterpret_cast<double*>(p));
            break;
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
        default:
            throw EXCEPTION("internal error");
    }
    nextIndex_++;
    return(value);
}

int64_t SourceDestBufferImpl::getNextInt64(double scale, double offset)
{
    /// Reverse scale (undo scaling) of a user's number to get raw value to put in file.

    /// Encorporating the scale is optional (requested by user when constructing the sdbuf).
    /// If the user did not request scaling, then we get raw values from user's buffer.
    if (!doScaling_) {
        /// Just return raw value.
        return(getNextInt64());
    }

    /// Double check non-zero scale.  Going to divide by it below.
    if (scale == 0)
        throw EXCEPTION("bad scale value");

    /// Verify index is within bounds
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("index out of bounds");

    /// Fetch value from source buffer.
    /// Convert from non-integer formats if requested
    char* p = &base_[nextIndex_*stride_];
    double doubleRawValue;
    switch (elementType_) {
        case E57_INT8:
            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<int8_t*>(p) - offset)/scale + 0.5);
            break;
        case E57_UINT8:
            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<uint8_t*>(p) - offset)/scale + 0.5);
            break;
        case E57_INT16:
            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<int16_t*>(p) - offset)/scale + 0.5);
            break;
        case E57_UINT16:
            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<uint16_t*>(p) - offset)/scale + 0.5);
            break;
        case E57_INT32:
            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<int32_t*>(p) - offset)/scale + 0.5);
            break;
        case E57_UINT32:
            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<uint32_t*>(p) - offset)/scale + 0.5);
            break;
        case E57_INT64:
            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<int64_t*>(p) - offset)/scale + 0.5);
            break;
        case E57_BOOL:
            throw EXCEPTION("type mismatch");
        case E57_REAL32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            //??? fault if get special value: NaN, NegInf...

            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<float*>(p) - offset)/scale + 0.5);
            break;
        case E57_REAL64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            //??? fault if get special value: NaN, NegInf...

            /// Calc (x-offset)/scale rounded to nearest integer, but keep in floating point until sure is in bounds
            doubleRawValue = floor((*reinterpret_cast<double*>(p) - offset)/scale + 0.5);
            break;
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
        default:
            throw EXCEPTION("internal error");
    }
    /// Make sure that value is representable in an int64_t
    if (doubleRawValue < INT64_MIN || INT64_MAX < doubleRawValue)
        throw EXCEPTION("raw value out of range");

    int64_t rawValue = static_cast<int64_t>(doubleRawValue);

    nextIndex_++;
    return(rawValue);
}

float SourceDestBufferImpl::getNextFloat()
{
    /// Verify index is within bounds
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("index out of bounds");  //??? underflow?

    /// Fetch value from source buffer.
    /// Convert from other formats to floating point if requested
    char* p = &base_[nextIndex_*stride_];
    float value;
    switch (elementType_) {
        case E57_INT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<float>(*reinterpret_cast<int8_t*>(p));
            break;
        case E57_UINT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<float>(*reinterpret_cast<uint8_t*>(p));
            break;
        case E57_INT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<float>(*reinterpret_cast<int16_t*>(p));
            break;
        case E57_UINT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<float>(*reinterpret_cast<uint16_t*>(p));
            break;
        case E57_INT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<float>(*reinterpret_cast<int32_t*>(p));
            break;
        case E57_UINT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<float>(*reinterpret_cast<uint32_t*>(p));
            break;
        case E57_INT64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<float>(*reinterpret_cast<int64_t*>(p));
            break;
        case E57_BOOL:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");

            /// Convert bool to 0/1, all non-zero values map to 1.0
            value = (*reinterpret_cast<bool*>(p)) ? 1.0F : 0.0F;
            break;
        case E57_REAL32:
            value = *reinterpret_cast<float*>(p);
            break;
        case E57_REAL64: {
            /// Check that exponent of user's value is not too large for single precision number in file.
            double d = *reinterpret_cast<double*>(p);

            ///??? silently limit here?
            if (d < DOUBLE_MIN || DOUBLE_MAX < d)
                throw EXCEPTION("value not representable");
            value = static_cast<float>(d);
            break;
        }
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
        default:
            throw EXCEPTION("internal error");
    }
    nextIndex_++;
    return(value);
}

double SourceDestBufferImpl::getNextDouble()
{
    /// Verify index is within bounds
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("index out of bounds");

    /// Fetch value from source buffer.
    /// Convert from other formats to floating point if requested
    char* p = &base_[nextIndex_*stride_];
    double value;
    switch (elementType_) {
        case E57_INT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<double>(*reinterpret_cast<int8_t*>(p));
            break;
        case E57_UINT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<double>(*reinterpret_cast<uint8_t*>(p));
            break;
        case E57_INT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<double>(*reinterpret_cast<int16_t*>(p));
            break;
        case E57_UINT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<double>(*reinterpret_cast<uint16_t*>(p));
            break;
        case E57_INT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<double>(*reinterpret_cast<int32_t*>(p));
            break;
        case E57_UINT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<double>(*reinterpret_cast<uint32_t*>(p));
            break;
        case E57_INT64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            value = static_cast<double>(*reinterpret_cast<int64_t*>(p));
            break;
        case E57_BOOL:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            /// Convert bool to 0/1, all non-zero values map to 1.0
            value = (*reinterpret_cast<bool*>(p)) ? 1.0 : 0.0;
        case E57_REAL32:
            value = static_cast<double>(*reinterpret_cast<float*>(p));
            break;
        case E57_REAL64:
            value = *reinterpret_cast<double*>(p);
            break;
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
        default:
            throw EXCEPTION("internal error");
    }
    nextIndex_++;
    return(value);
}

ustring SourceDestBufferImpl::getNextString()
{
    /// Verify index is within bounds
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("index out of bounds");

    ///??? need to handle reading null terminated char* as element type?  For reading only?

    /// Get ustring from buffer
    if (elementType_ != E57_USTRING)
        throw EXCEPTION("type mismatch");

    ustring* p = reinterpret_cast<ustring*>(&base_[nextIndex_*stride_]);
    nextIndex_++;
    return(*p);  /// A little risky here because copy constructor is going to be called!
}

void  SourceDestBufferImpl::setNextInt64(int64_t value)
{
    /// Verify have room
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("SourceDestBuffer overflow");

    /// Calc start of memory location, index into buffer using stride_ (the distance between elements).
    char* p = &base_[nextIndex_*stride_];

    switch (elementType_) {
        case E57_INT8:
            if (value < INT8_MIN || INT8_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int8_t*>(p) = static_cast<int8_t>(value);
            break;
        case E57_UINT8:
            if (value < UINT8_MIN || UINT8_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint8_t*>(p) = static_cast<uint8_t>(value);
            break;
        case E57_INT16:
            if (value < INT16_MIN || INT16_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int16_t*>(p) = static_cast<int16_t>(value);
            break;
        case E57_UINT16:
            if (value < UINT16_MIN || UINT16_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint16_t*>(p) = static_cast<uint16_t>(value);
            break;
        case E57_INT32:
            if (value < INT32_MIN || INT32_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int32_t*>(p) = static_cast<int32_t>(value);
            break;
        case E57_UINT32:
            if (value < UINT32_MIN || UINT32_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint32_t*>(p) = static_cast<uint32_t>(value);
            break;
        case E57_INT64:
            *reinterpret_cast<int64_t*>(p) = static_cast<int64_t>(value);
            break;
        case E57_BOOL:
            *reinterpret_cast<bool*>(p) = (value ? false : true);
            break;
        case E57_REAL32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            //??? very large integers may lose some lowest bits here. error?
            *reinterpret_cast<float*>(p) = static_cast<float>(value);
            break;
        case E57_REAL64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            *reinterpret_cast<double*>(p) = static_cast<double>(value);
            break;
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
    }

    nextIndex_++;
}

void  SourceDestBufferImpl::setNextInt64(int64_t value, double scale, double offset)
{
    /// Apply a scale and offset to numbers from file before puting in user's buffer.

    /// Encorporating the scale is optional (requested by user when constructing the sdbuf).
    /// If the user did not request scaling, then we send raw values to user's buffer.
    if (!doScaling_) {
        /// Use raw value routine, then bail out.
        setNextInt64(value);
        return;
    }

    /// Verify have room
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("SourceDestBuffer overflow");

    /// Calc start of memory location, index into buffer using stride_ (the distance between elements).
    char* p = &base_[nextIndex_*stride_];

    /// Calc x*scale+offset
    double scaledValue;
    if (elementType_ == E57_REAL32 || elementType_ == E57_REAL64) {
        /// Value will be stored in some floating point rep in user's buffer, so keep full resolution here.
        scaledValue = value*scale + offset;
    } else {
        /// Value will represented as some integer in user's buffer, so round to nearest integer here.
        /// But keep in floating point rep until we know that the value is representable in the user's buffer.
        scaledValue = floor(value*scale + offset + 0.5);
    }

    switch (elementType_) {
        case E57_INT8:
            if (scaledValue < INT8_MIN || INT8_MAX < scaledValue)
                throw EXCEPTION("scaled value not representable");
            *reinterpret_cast<int8_t*>(p) = static_cast<int8_t>(scaledValue);
            break;
        case E57_UINT8:
            if (scaledValue < UINT8_MIN || UINT8_MAX < scaledValue)
                throw EXCEPTION("scaled value not representable");
            *reinterpret_cast<uint8_t*>(p) = static_cast<uint8_t>(scaledValue);
            break;
        case E57_INT16:
            if (scaledValue < INT16_MIN || INT16_MAX < scaledValue)
                throw EXCEPTION("scaled value not representable");
            *reinterpret_cast<int16_t*>(p) = static_cast<int16_t>(scaledValue);
            break;
        case E57_UINT16:
            if (scaledValue < UINT16_MIN || UINT16_MAX < scaledValue)
                throw EXCEPTION("scaled value not representable");
            *reinterpret_cast<uint16_t*>(p) = static_cast<uint16_t>(scaledValue);
            break;
        case E57_INT32:
            if (scaledValue < INT32_MIN || INT32_MAX < scaledValue)
                throw EXCEPTION("scaled value not representable");
            *reinterpret_cast<int32_t*>(p) = static_cast<int32_t>(scaledValue);
            break;
        case E57_UINT32:
            if (scaledValue < UINT32_MIN || UINT32_MAX < scaledValue)
                throw EXCEPTION("scaled value not representable");
            *reinterpret_cast<uint32_t*>(p) = static_cast<uint32_t>(scaledValue);
            break;
        case E57_INT64:
            *reinterpret_cast<int64_t*>(p) = static_cast<int64_t>(scaledValue);
            break;
        case E57_BOOL:
            *reinterpret_cast<bool*>(p) = (scaledValue ? false : true);
            break;
        case E57_REAL32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            /// Check that exponent of result is not too big for single precision float
            if (scaledValue < DOUBLE_MIN || DOUBLE_MAX < scaledValue)
                throw EXCEPTION("scaled value not representable");
            *reinterpret_cast<float*>(p) = static_cast<float>(scaledValue);
            break;
        case E57_REAL64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            *reinterpret_cast<double*>(p) = scaledValue;
            break;
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
    }

    nextIndex_++;
}

void SourceDestBufferImpl::setNextFloat(float value)
{
    /// Verify have room
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("SourceDestBuffer overflow");

    /// Calc start of memory location, index into buffer using stride_ (the distance between elements).
    char* p = &base_[nextIndex_*stride_];

    switch (elementType_) {
        case E57_INT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            //??? fault if get special value: NaN, NegInf...  (all other ints below too)
            if (value < INT8_MIN || INT8_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int8_t*>(p) = static_cast<int8_t>(value);
            break;
        case E57_UINT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < UINT8_MIN || UINT8_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint8_t*>(p) = static_cast<uint8_t>(value);
            break;
        case E57_INT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < INT16_MIN || INT16_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int16_t*>(p) = static_cast<int16_t>(value);
            break;
        case E57_UINT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < UINT16_MIN || UINT16_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint16_t*>(p) = static_cast<uint16_t>(value);
            break;
        case E57_INT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < INT32_MIN || INT32_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int32_t*>(p) = static_cast<int32_t>(value);
            break;
        case E57_UINT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < UINT32_MIN || UINT32_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint32_t*>(p) = static_cast<uint32_t>(value);
            break;
        case E57_INT64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < INT64_MIN || INT64_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int64_t*>(p) = static_cast<int64_t>(value);
            break;
        case E57_BOOL:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            *reinterpret_cast<bool*>(p) = (value ? false : true);
            break;
        case E57_REAL32:
            *reinterpret_cast<float*>(p) = value;
            break;
        case E57_REAL64:
            //??? does this count as a conversion?
            *reinterpret_cast<double*>(p) = static_cast<double>(value);
            break;
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
    }

    nextIndex_++;
}

void SourceDestBufferImpl::setNextDouble(double value)
{
    /// Verify have room
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("SourceDestBuffer overflow");

    /// Calc start of memory location, index into buffer using stride_ (the distance between elements).
    char* p = &base_[nextIndex_*stride_];

    switch (elementType_) {
        case E57_INT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            //??? fault if get special value: NaN, NegInf...  (all other ints below too)
            if (value < INT8_MIN || INT8_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int8_t*>(p) = static_cast<int8_t>(value);
            break;
        case E57_UINT8:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < UINT8_MIN || UINT8_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint8_t*>(p) = static_cast<uint8_t>(value);
            break;
        case E57_INT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < INT16_MIN || INT16_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int16_t*>(p) = static_cast<int16_t>(value);
            break;
        case E57_UINT16:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < UINT16_MIN || UINT16_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint16_t*>(p) = static_cast<uint16_t>(value);
            break;
        case E57_INT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < INT32_MIN || INT32_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int32_t*>(p) = static_cast<int32_t>(value);
            break;
        case E57_UINT32:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < UINT32_MIN || UINT32_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<uint32_t*>(p) = static_cast<uint32_t>(value);
            break;
        case E57_INT64:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            if (value < INT64_MIN || INT64_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<int64_t*>(p) = static_cast<int64_t>(value);
            break;
        case E57_BOOL:
            if (!doConversion_)
                throw EXCEPTION("conversion not requested");
            *reinterpret_cast<bool*>(p) = (value ? false : true);
            break;
        case E57_REAL32:
            /// Does this count as conversion?  It loses information.
            /// Check for really large exponents that can't fit in a single precision
            if (value < DOUBLE_MIN || DOUBLE_MAX < value)
                throw EXCEPTION("value not representable");
            *reinterpret_cast<float*>(p) = static_cast<float>(value);
            break;
        case E57_REAL64:
            *reinterpret_cast<double*>(p) = value;
            break;
        case E57_USTRING:
            throw EXCEPTION("type mismatch");
    }

    nextIndex_++;
}

void SourceDestBufferImpl::setNextString(const ustring& value)
{
    /// Verify have room
    if (nextIndex_ >= capacity_)
        throw EXCEPTION("SourceDestBuffer overflow");

    if (elementType_ != E57_USTRING)
        throw EXCEPTION("type mismatch");

    /// Put ustring into buffer
    ustring* p = reinterpret_cast<ustring*>(&base_[nextIndex_*stride_]);
    *p = value;  /// A little risky here because assignment operator is going to be called!

    nextIndex_++;
}

#ifdef E57_DEBUG
void SourceDestBufferImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "pathName:       " << pathName_ << endl;
    os << space(indent) << "elementType:    ";
    switch (elementType_) {
        case E57_INT8:      os << "int8_t" << endl;    break;
        case E57_UINT8:     os << "uint8_t" << endl;   break;
        case E57_INT16:     os << "nt16_t" << endl;    break;
        case E57_UINT16:    os << "uint16_t" << endl;  break;
        case E57_INT32:     os << "int32_t" << endl;   break;
        case E57_UINT32:    os << "uint32_t" << endl;  break;
        case E57_INT64:     os << "int64_t" << endl;   break;
        case E57_BOOL:      os << "bool" << endl;      break;
        case E57_REAL32:    os << "float" << endl;     break;
        case E57_REAL64:    os << "double" << endl;    break;
        case E57_USTRING:   os << "ustring" << endl;   break;
        default:            os << "<unknown>" << endl; break;
    }
    os << space(indent) << "base:           " << reinterpret_cast<unsigned>(base_) << endl;
    os << space(indent) << "ustrings:       " << reinterpret_cast<unsigned>(ustrings_) << endl;
    os << space(indent) << "capacity:       " << capacity_ << endl;
    os << space(indent) << "doConversion:   " << doConversion_ << endl;
    os << space(indent) << "doScaling:      " << doScaling_ << endl;
    os << space(indent) << "stride:         " << stride_ << endl;
    os << space(indent) << "nextIndex:      " << nextIndex_ << endl;
}
#endif

//=============================================================================
CompressedVectorNodeImpl::CompressedVectorNodeImpl(weak_ptr<ImageFileImpl> fileParent)
: NodeImpl(fileParent)
{
    recordCount_                = 0;
    binarySectionLogicalStart_  = 0;
}

void CompressedVectorNodeImpl::setPrototype(shared_ptr<NodeImpl> prototype)
{
    //??? check ok for proto, no Blob CompressedVector, empty?

    //??? if resetting any pointer, do need to update old value's parent?
    prototype_ = prototype;

//!!! prototype not in the tree of values.
//!!!    prototype_->setParent(shared_from_this(), "prototype");
}

void CompressedVectorNodeImpl::setCodecs(shared_ptr<NodeImpl> codecs)
{
    //??? check ok for codecs, empty vector, or each element has "inputs" vector of strings, codec substruct

    //??? if resetting any pointer, do need to update old value's parent?
    codecs_ = codecs;

//!!! prototype not in the tree of values.
//!!! codecs_->setParent(shared_from_this(), "codecs");
}

bool CompressedVectorNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    //??? is this test a good idea?

    /// Same node type?
    if (ni->type() != E57_COMPRESSED_VECTOR)
        return(false);

    /// Downcast to shared_ptr<ScaledIntegerNodeImpl>
    shared_ptr<CompressedVectorNodeImpl> cvi(dynamic_pointer_cast<CompressedVectorNodeImpl>(ni));
    if (!cvi)
        throw(EXCEPTION("bad downcast"));

    /// recordCount must match
    if (recordCount_ != cvi->recordCount_)
        return(false);

    /// Prototypes and codecs must match ???
    if (!prototype_->isTypeEquivalent(cvi->prototype_))
        return(false);
    if (!codecs_->isTypeEquivalent(cvi->codecs_))
        return(false);

    return(true);
}

bool CompressedVectorNodeImpl::isDefined(const ustring& pathName)
{
    throw EXCEPTION("not implemented");  //!!! implement
    return(false);
}

int64_t CompressedVectorNodeImpl::childCount()
{
    return(recordCount_);
}

void CompressedVectorNodeImpl::checkLeavesInSet(const std::set<ustring>& pathNames, shared_ptr<NodeImpl> origin)
{
    throw EXCEPTION("not implemented");  //!!! implement
}

void CompressedVectorNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    uint64_t physicalStart = cf.logicalToPhysical(binarySectionLogicalStart_);

    cf << space(indent) << "<" << fname << " type=\"CompressedVector\"";
    cf << " fileOffset=\"" << physicalStart;
    cf << "\" recordCount=\"" << recordCount_ << "\">\n";

    if (prototype_)
        prototype_->writeXml(imf, cf, indent+4, "prototype");
    if (codecs_)
        codecs_->writeXml(imf, cf, indent+4, "codecs");
    cf << space(indent) << "</"<< fname << ">\n";
}

#ifdef E57_DEBUG
void CompressedVectorNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:      CompressedVectorNode" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName: " << fieldName_ << endl;
    os << space(indent) << "path:      " << pathName() << endl;
    if (prototype_) {
        os << space(indent) << "prototype:" << endl;
        prototype_->dump(indent+2, os);
    } else
        os << space(indent) << "prototype: <empty>" << endl;
    if (codecs_) {
        os << space(indent) << "codecs:" << endl;
        codecs_->dump(indent+2, os);
    } else
        os << space(indent) << "codecs: <empty>" << endl;
    os << space(indent) << "recordCount:                " << recordCount_ << endl;
    os << space(indent) << "binarySectionLogicalStart:  " << binarySectionLogicalStart_ << endl;
}
#endif

shared_ptr<CompressedVectorWriterImpl> CompressedVectorNodeImpl::writer(vector<SourceDestBuffer> sbufs)
{
    /// Get pointer to me (really shared_ptr<CompressedVectorNodeImpl>)
    shared_ptr<NodeImpl> ni(shared_from_this());

    /// Downcast pointer to right type
    shared_ptr<CompressedVectorNodeImpl> cai(dynamic_pointer_cast<CompressedVectorNodeImpl>(ni));
    if (!cai)
        throw(EXCEPTION("bad downcast"));

    /// Return a shared_ptr to new object
    shared_ptr<CompressedVectorWriterImpl> cvwi(new CompressedVectorWriterImpl(cai, sbufs));
    return(cvwi);
}

shared_ptr<CompressedVectorReaderImpl> CompressedVectorNodeImpl::reader(vector<SourceDestBuffer> dbufs)
{
    /// Get pointer to me (really shared_ptr<CompressedVectorNodeImpl>)
    shared_ptr<NodeImpl> ni(shared_from_this());
#ifdef E57_MAX_VERBOSE
    //cout<<"constructing CAReader, ni:"<<endl;
    //ni->dump(4);
#endif

    /// Downcast pointer to right type
    shared_ptr<CompressedVectorNodeImpl> cai(dynamic_pointer_cast<CompressedVectorNodeImpl>(ni));
    if (!cai)
        throw(EXCEPTION("bad downcast"));
#ifdef E57_MAX_VERBOSE
    //cout<<"constructing CAReader, cai:"<<endl;
    //cai->dump(4);
#endif
    /// Return a shared_ptr to new object
    shared_ptr<CompressedVectorReaderImpl> cvri(new CompressedVectorReaderImpl(cai, dbufs));
    return(cvri);
}

//=====================================================================
IntegerNodeImpl::IntegerNodeImpl(weak_ptr<ImageFileImpl> fileParent, int64_t value, int64_t minimum, int64_t maximum)
: NodeImpl(fileParent),
  value_(value),
  minimum_(minimum),
  maximum_(maximum)
{}

bool IntegerNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    /// Same node type?
    if (ni->type() != E57_INTEGER)
        return(false);

    /// Downcast to shared_ptr<IntegerNodeImpl>
    shared_ptr<IntegerNodeImpl> ii(dynamic_pointer_cast<IntegerNodeImpl>(ni));
    if (!ii)
        throw(EXCEPTION("bad downcast"));

    /// minimum must match
    if (minimum_ != ii->minimum_)
        return(false);

    /// maximum must match
    if (maximum_ != ii->maximum_)
        return(false);

    /// ignore value_, doesn't have to match

    /// Types match
    return(true);
}

bool IntegerNodeImpl::isDefined(const ustring& pathName)
{
    /// We have no sub-structure, so if path not empty return false
    return(pathName == "");
}

void IntegerNodeImpl::checkLeavesInSet(const std::set<ustring>& pathNames, shared_ptr<NodeImpl> origin)
{
    /// We are a leaf node, so verify that we are listed in set.
    if (pathNames.find(relativePathName(origin)) == pathNames.end())
        throw EXCEPTION("field name missing");
}

void IntegerNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    cf << space(indent) << "<" << fname << " type=\"Integer\"";
    cf << " value=\"" << value_ << "\" minimum=\"" << minimum_ << "\" maximum=\"" << maximum_ << "\"/>\n";
}

#ifdef E57_DEBUG
void IntegerNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:      Integer" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName: " << fieldName_ << endl;
    os << space(indent) << "path:      " << pathName() << endl;
    os << space(indent) << "minimum:   " << minimum_ << endl;
    os << space(indent) << "maximum:   " << maximum_ << endl;
    os << space(indent) << "value:     " << value_ << endl;
}
#endif

//=============================================================================
ScaledIntegerNodeImpl::ScaledIntegerNodeImpl(weak_ptr<ImageFileImpl> fileParent, int64_t value, int64_t minimum, int64_t maximum, double scale, double offset)
: NodeImpl(fileParent),
  value_(value),
  minimum_(minimum),
  maximum_(maximum),
  scale_(scale),
  offset_(offset)
{}

bool ScaledIntegerNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    /// Same node type?
    if (ni->type() != E57_SCALED_INTEGER)
        return(false);

    /// Downcast to shared_ptr<ScaledIntegerNodeImpl>
    shared_ptr<ScaledIntegerNodeImpl> ii(dynamic_pointer_cast<ScaledIntegerNodeImpl>(ni));
    if (!ii)
        throw(EXCEPTION("bad downcast"));

    /// minimum must match
    if (minimum_ != ii->minimum_)
        return(false);

    /// maximum must match
    if (maximum_ != ii->maximum_)
        return(false);

    /// scale must match
    if (scale_ != ii->scale_)
        return(false);

    /// offset must match
    if (offset_ != ii->offset_)
        return(false);

    /// ignore value_, doesn't have to match

    /// Types match
    return(true);
}

bool ScaledIntegerNodeImpl::isDefined(const ustring& pathName)
{
    /// We have no sub-structure, so if path not empty return false
    return(pathName == "");
}

void ScaledIntegerNodeImpl::checkLeavesInSet(const std::set<ustring>& pathNames, shared_ptr<NodeImpl> origin)
{
    /// We are a leaf node, so verify that we are listed in set.
    if (pathNames.find(relativePathName(origin)) == pathNames.end())
        throw EXCEPTION("field name missing");
}

void ScaledIntegerNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    //???TODO need to handle case where max is largest uint64
    //??? is 16 digits right number?, try to remove trailing zeros?
    cf << space(indent) << "<" << fname << " type=\"ScaledInteger\"";
    cf << " value=\"" << value_ << "\" minimum=\"" << minimum_ << "\" maximum=\"" << maximum_;
    cf << "\" scale=\"" << scale_ << "\" offset=\"" << offset_ << "\"/>\n";
}

#ifdef E57_DEBUG
void ScaledIntegerNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:      Integer" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName: " << fieldName_ << endl;
    os << space(indent) << "path:      " << pathName() << endl;
    os << space(indent) << "minimum:   " << minimum_ << endl;
    os << space(indent) << "maximum:   " << maximum_ << endl;
    os << space(indent) << "scale:     " << scale_ << endl;
    os << space(indent) << "offset:    " << offset_ << endl;
    os << space(indent) << "value:     " << value_ << endl;
}
#endif

//=============================================================================

FloatNodeImpl::FloatNodeImpl(weak_ptr<ImageFileImpl> fileParent, double value, FloatPrecision precision)
: NodeImpl(fileParent),
  value_(value),
  precision_(precision)
{}

bool FloatNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    /// Same node type?
    if (ni->type() != E57_FLOAT)
        return(false);

    /// Downcast to shared_ptr<FloatNodeImpl>
    shared_ptr<FloatNodeImpl> fi(dynamic_pointer_cast<FloatNodeImpl>(ni));
    if (!fi)
        throw(EXCEPTION("bad downcast"));

    /// precision must match
    if (precision_ != fi->precision_)
        return(false);

    /// ignore value_, doesn't have to match

    /// Types match
    return(true);
}

bool FloatNodeImpl::isDefined(const ustring& pathName)
{
    /// We have no sub-structure, so if path not empty return false
    return(pathName == "");
}

void FloatNodeImpl::checkLeavesInSet(const std::set<ustring>& pathNames, shared_ptr<NodeImpl> origin)
{
    /// We are a leaf node, so verify that we are listed in set.
    if (pathNames.find(relativePathName(origin)) == pathNames.end())
        throw EXCEPTION("field name missing");
}

void FloatNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    //??? is 16 digits right number?, try to remove trailing zeros?
    cf << space(indent) << "<" << fname << " type=\"Float\"";
    cf << " value=\"" << value_ << "\" ";
    if (precision_ == E57_SINGLE)
        cf << "precision=\"Single\"/>\n";
    else
        cf << "precision=\"Double\"/>\n";
}

#ifdef E57_DEBUG
void FloatNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:      Float" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName: " << fieldName_ << endl;
    os << space(indent) << "path:      " << pathName() << endl;
    os << space(indent) << "precision: ";
    if (precision() == E57_SINGLE)
        os << "single" << endl;
    else
        os << "double" << endl;

    /// Save old stream config
    const streamsize oldPrecision = os.precision();
    const ios_base::fmtflags oldFlags = os.flags();

    os << space(indent) << scientific << setprecision(16) << "value:     " << value_ << endl;

    /// Restore old stream config
    os.precision(oldPrecision);
    os.flags(oldFlags);
}
#endif

//=============================================================================

StringNodeImpl::StringNodeImpl(weak_ptr<ImageFileImpl> fileParent, ustring value)
: NodeImpl(fileParent),
  value_(value)
{}

bool StringNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    /// Same node type?
    if (ni->type() != E57_STRING)
        return(false);

    /// ignore value_, doesn't have to match

    /// Types match
    return(true);
}

bool StringNodeImpl::isDefined(const ustring& pathName)
{
    /// We have no sub-structure, so if path not empty return false
    return(pathName == "");
}

void StringNodeImpl::checkLeavesInSet(const std::set<ustring>& pathNames, shared_ptr<NodeImpl> origin)
{
    /// We are a leaf node, so verify that we are listed in set.
    if (pathNames.find(relativePathName(origin)) == pathNames.end())
        throw EXCEPTION("field name missing");
}

void StringNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    //??? need to escape some chars in value: double_quote
    cf << space(indent) << "<" << fname << " type=\"String\" value=\"" << value_ << "\"/>\n";
}

#ifdef E57_DEBUG
void StringNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:      String" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName: " << fieldName_ << endl;
    os << space(indent) << "path:      " << pathName() << endl;
    os << space(indent) << "value:     '" << value_ << "'" << endl;
}
#endif

//=============================================================================

BlobNodeImpl::BlobNodeImpl(weak_ptr<ImageFileImpl> fileParent, uint64_t byteCount)
: NodeImpl(fileParent)
{
    shared_ptr<ImageFileImpl> imf(fileParent);

    /// This what caller thinks blob length is
    blobLogicalLength_ = byteCount;

    /// Round segment length up to multiple of 4 bytes
    binarySectionLogicalLength_ = sizeof(BlobSectionHeader) + blobLogicalLength_;
    unsigned remainder = binarySectionLogicalLength_ % 4;
    if (remainder > 0)
        binarySectionLogicalLength_ += 4 - remainder;

    /// Reserve space for blob in file, extend with zeros since writes will happen at later time by caller
    binarySectionLogicalStart_ = imf->allocateSpace(binarySectionLogicalLength_, true);

    /// Prepare BlobSectionHeader
    BlobSectionHeader header;
    memset(&header, 0, sizeof(header));  /// need to init to zero, ok since no constructor
    header.sectionId = E57_BLOB_SECTION;
    header.sectionLogicalLength = binarySectionLogicalLength_;
#ifdef E57_MAX_VERBOSE
    header.dump(); //???
#endif
    header.swab();  /// swab if neccesary

    /// Write header at beginning of section
    imf->file_->seek(binarySectionLogicalStart_);
    imf->file_->write(reinterpret_cast<char*>(&header), sizeof(header));
}

BlobNodeImpl::BlobNodeImpl(weak_ptr<ImageFileImpl> fileParent, uint64_t fileOffset, uint64_t length)
: NodeImpl(fileParent)
{
    /// Init blob object that already exists in E57 file currently reading.

    shared_ptr<ImageFileImpl> imf(fileParent);

    /// Init state from values read from XML
    blobLogicalLength_ = length;
    binarySectionLogicalStart_ = imf->file_->physicalToLogical(fileOffset);
    binarySectionLogicalLength_ = sizeof(BlobSectionHeader) + blobLogicalLength_;
}

bool BlobNodeImpl::isTypeEquivalent(shared_ptr<NodeImpl> ni)
{
    /// Same node type?
    if (ni->type() != E57_BLOB)
        return(false);

    /// Downcast to shared_ptr<BlobNodeImpl>
    shared_ptr<BlobNodeImpl> bi(dynamic_pointer_cast<BlobNodeImpl>(ni));
    if (!bi)
        throw(EXCEPTION("bad downcast"));

    /// blob lengths must match
    if (blobLogicalLength_ != bi->blobLogicalLength_)
        return(false);

    /// ignore blob contents, doesn't have to match

    /// Types match
    return(true);
}

bool BlobNodeImpl::isDefined(const ustring& pathName)
{
    /// We have no sub-structure, so if path not empty return false
    return(pathName == "");
}

BlobNodeImpl::~BlobNodeImpl()
{}

int64_t BlobNodeImpl::byteCount()
{
    return(blobLogicalLength_);
}

void BlobNodeImpl::read(uint8_t* buf, uint64_t start, uint64_t count)
{
    if (start+count > blobLogicalLength_)
        throw EXCEPTION("bad indexes");
    if (count > SIZE_MAX)
        throw EXCEPTION("count too long");

    shared_ptr<ImageFileImpl> imf(fileParent_);
    imf->file_->seek(binarySectionLogicalStart_ + sizeof(BlobSectionHeader) + start);
    imf->file_->read(reinterpret_cast<char*>(buf), static_cast<size_t>(count));  //??? arg1 void* ?
}

void BlobNodeImpl::write(uint8_t* buf, uint64_t start, uint64_t count)
{
    if (start+count > blobLogicalLength_)
        throw(EXCEPTION("bad indexes"));
    if (count > SIZE_MAX)
        throw EXCEPTION("count too long");

    shared_ptr<ImageFileImpl> imf(fileParent_);
    imf->file_->seek(binarySectionLogicalStart_ + sizeof(BlobSectionHeader) + start);
    imf->file_->write(reinterpret_cast<char*>(buf), static_cast<size_t>(count));  //??? arg1 void* ?
}

void BlobNodeImpl::checkLeavesInSet(const std::set<ustring>& pathNames, shared_ptr<NodeImpl> origin)
{
    /// We are a leaf node, so verify that we are listed in set. ???true for blobs? what exception get if try blob in compressedvector?
    if (pathNames.find(relativePathName(origin)) == pathNames.end())
        throw EXCEPTION("field name missing");
}

void BlobNodeImpl::writeXml(std::tr1::shared_ptr<ImageFileImpl> imf, CheckedFile& cf, int indent, char* forcedFieldName)
{
    ustring fname;
    if (forcedFieldName != NULL)
        fname = forcedFieldName;
    else
        fname = fieldName_;

    //??? need to implement
    //??? Type --> type
    //??? need to have length?, check same as in section header?
    uint64_t physicalOffset = cf.logicalToPhysical(binarySectionLogicalStart_);
    cf << space(indent) << "<" << fname << " type=\"Blob\" fileOffset=\"" << physicalOffset << "\" length=\"" << blobLogicalLength_ << "\"/>\n";
}

#ifdef E57_DEBUG
void BlobNodeImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "type:                       Blob" << " (" << type() << ")" << endl;
    os << space(indent) << "fieldName:                  " << fieldName_ << endl;
    os << space(indent) << "path:                       " << pathName() << endl;
    os << space(indent) << "blobLogicalLength_:         " << blobLogicalLength_ << endl;
    os << space(indent) << "binarySectionLogicalStart:  " << binarySectionLogicalStart_ << endl;
    os << space(indent) << "binarySectionLogicalLength: " << binarySectionLogicalLength_ << endl;
    size_t i;
    for (i = 0; i < blobLogicalLength_ && i < 10; i++) {
        uint8_t b;
        read(&b, i, 1);
        os << space(indent) << "data[" << i << "]: "<< static_cast<int>(b) << endl;
    }
    if (i < blobLogicalLength_)
        os << space(indent) << "more data unprinted..." << endl;
}
#endif

///================================================================
///================================================================
///================================================================

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/util/BinInputStream.hpp>

/// Make shorthand for Xerces namespace???
XERCES_CPP_NAMESPACE_USE;

class E57FileInputStream : public BinInputStream
{
public :
                            E57FileInputStream(CheckedFile* cf, uint64_t logicalStart, uint64_t logicalLength);
    virtual                 ~E57FileInputStream() {};
    virtual XMLFilePos      curPos() const {return(logicalPosition_);};
    virtual XMLSize_t       readBytes(XMLByte* const toFill, const XMLSize_t maxToRead);
    virtual const XMLCh*    getContentType() const {return(0);};

private :
    ///  Unimplemented constructors and operators
    E57FileInputStream(const E57FileInputStream&);
    E57FileInputStream& operator=(const E57FileInputStream&);

    //??? lifetime of cf_ must be longer than this object!
    CheckedFile*    cf_;
    uint64_t        logicalStart_;
    uint64_t        logicalLength_;
    uint64_t        logicalPosition_;
};

E57FileInputStream::E57FileInputStream(CheckedFile* cf, uint64_t logicalStart, uint64_t logicalLength)
: cf_(cf),
  logicalStart_(logicalStart),
  logicalLength_(logicalLength),
  logicalPosition_(logicalStart)
{
}

XMLSize_t E57FileInputStream::readBytes(       XMLByte* const  toFill
                                       , const XMLSize_t       maxToRead)
{
    if (logicalPosition_ > logicalStart_ + logicalLength_)
        return(0);

    int64_t available = logicalStart_ + logicalLength_ - logicalPosition_;
    if (available <= 0)
        return(0);

    /// size_t and XMLSize_t should be compatible, should get compiler warning here if not
    size_t maxToRead_size = maxToRead;

    /// Be careful if size_t is smaller than int64_t
    size_t available_size = static_cast<size_t>(min(available, static_cast<int64_t>(SIZE_MAX)));

    size_t readCount = min(maxToRead_size, available_size);

    cf_->seek(logicalPosition_);
    cf_->read(reinterpret_cast<char*>(toFill), readCount);  //??? cast ok?
    logicalPosition_ += readCount;
    return(readCount);
}

///================================================================
class E57FileInputSource : public InputSource {
public :
    E57FileInputSource(CheckedFile* cf, uint64_t logicalStart, uint64_t logicalLength);
    ~E57FileInputSource(){};
    BinInputStream* makeStream() const;

private :
    ///  Unimplemented constructors and operators
    E57FileInputSource(const E57FileInputSource&);
    E57FileInputSource& operator=(const E57FileInputSource&);

    //??? lifetime of cf_ must be longer than this object!
    CheckedFile*    cf_;
    uint64_t        logicalStart_;
    uint64_t        logicalLength_;
};


E57FileInputSource::E57FileInputSource(CheckedFile* cf, uint64_t logicalStart, uint64_t logicalLength)
: InputSource("E57File", XMLPlatformUtils::fgMemoryManager),  //??? what if want to use our own memory manager?, what bufid is good?
  cf_(cf),
  logicalStart_(logicalStart),
  logicalLength_(logicalLength)
{
}

BinInputStream* E57FileInputSource::makeStream() const
{
    return new E57FileInputStream(cf_, logicalStart_, logicalLength_);
}

///============================================================================================================
///============================================================================================================
///============================================================================================================
#include <stdlib.h>

namespace e57 {

class E57XmlParser : public DefaultHandler
{
public:
    E57XmlParser(std::tr1::shared_ptr<ImageFileImpl> imf);
    ~E57XmlParser();

    /// SAX interface
    void startDocument();
    void endDocument();
    void startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname, const Attributes& attributes);
    void endElement( const XMLCh* const uri,
                     const XMLCh* const localname,
                     const XMLCh* const qname);
    void characters(const XMLCh* const chars, const XMLSize_t length);
    void processingInstruction(const XMLCh* const target, const XMLCh* const data);

    /// SAX error interface
    void warning(const SAXParseException& exc);
    void error(const SAXParseException& exc);
    void fatalError(const SAXParseException& exc);
private:
    ustring toUString(const XMLCh* const xml_str);
    ustring lookupAttribute(const Attributes& attributes, wchar_t* attribute_name);

    std::tr1::shared_ptr<ImageFileImpl> imf_;   /// Image file we are reading
    std::stack<shared_ptr<NodeImpl> >    stack_; /// Current path in tree we are reading
};

}; /// end namespace e57

E57XmlParser::E57XmlParser(std::tr1::shared_ptr<ImageFileImpl> imf)
: imf_(imf)
{
}

E57XmlParser::~E57XmlParser()
{
}


void E57XmlParser::startDocument()
{
#ifdef E57_MAX_VERBOSE
    //cout << "startDocument" << endl;
#endif
}


void E57XmlParser::endDocument()
{
#ifdef E57_MAX_VERBOSE
    //cout << "endDocument"<<endl;
#endif
}


void E57XmlParser::startElement(const   XMLCh* const    uri,
                             const   XMLCh* const    localname,
                             const   XMLCh* const    qname,
                             const   Attributes&     attributes)
{
#ifdef E57_MAX_VERBOSE
    cout << "startElement" << endl;
    cout << space(2) << "URI:       " << toUString(uri) << endl;
    cout << space(2) << "localName: " << toUString(localname) << endl;
    cout << space(2) << "qName:     " << toUString(qname) << endl;

    for (size_t i = 0; i < attributes.getLength(); i++) {
        cout << space(2) << "Attribute[" << i << "]" << endl;
        cout << space(4) << "URI:       " << toUString(attributes.getURI(i)) << endl;
        cout << space(4) << "localName: " << toUString(attributes.getLocalName(i)) << endl;
        cout << space(4) << "qName:     " << toUString(attributes.getQName(i)) << endl;
        cout << space(4) << "value:     " << toUString(attributes.getValue(i)) << endl;
    }
#endif
    /// Get Type attribute
    ustring node_type = lookupAttribute(attributes, L"type");

    //??? check to make sure not in primative type (can only nest inside compound types).

    if (node_type == "Integer") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a Integer" << endl;
#endif
        ustring value_str     = lookupAttribute(attributes, L"value");
        ustring minimum_str   = lookupAttribute(attributes, L"minimum");
        ustring maximum_str   = lookupAttribute(attributes, L"maximum");

        //??? check validity of numeric strings

        int64_t value   = _atoi64(value_str.c_str());    //??? not portable, MSonly
        int64_t min_val = _atoi64(minimum_str.c_str());  //??? not portable, MSonly
        int64_t max_val = _atoi64(maximum_str.c_str());  //??? not portable, MSonly

        /// Push onto stack
        shared_ptr<IntegerNodeImpl> ni(new IntegerNodeImpl(imf_, value, min_val, max_val));
        stack_.push(ni);
    } else if (node_type == "ScaledInteger") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a ScaledInteger" << endl;
#endif
        ustring value_str     = lookupAttribute(attributes, L"value");
        ustring minimum_str   = lookupAttribute(attributes, L"minimum");
        ustring maximum_str   = lookupAttribute(attributes, L"maximum");
        ustring scale_str     = lookupAttribute(attributes, L"scale");
        ustring offset_str    = lookupAttribute(attributes, L"offset");

        //??? check validity of numeric strings

        int64_t value   = _atoi64(value_str.c_str());    //??? not portable
        int64_t min_val = _atoi64(minimum_str.c_str());  //??? not portable
        int64_t max_val = _atoi64(maximum_str.c_str());  //??? not portable
        double scale    = atof(scale_str.c_str());
        double offset   = atof(offset_str.c_str());

        /// Push onto stack
        shared_ptr<ScaledIntegerNodeImpl> ni(new ScaledIntegerNodeImpl(imf_, value, min_val, max_val, scale, offset));
        stack_.push(ni);
    } else if (node_type == "Float") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a Float" << endl;
#endif
        ustring value_str     = lookupAttribute(attributes, L"value");
        ustring precision_str = lookupAttribute(attributes, L"precision");

        double value = atof(value_str.c_str());
        FloatPrecision fprec;
        if (precision_str == "Single")
            fprec = E57_SINGLE;
        else if (precision_str == "Double")
            fprec = E57_DOUBLE;
        else
            throw EXCEPTION("bad precision");

        /// Push onto stack
        shared_ptr<FloatNodeImpl> ni(new FloatNodeImpl(imf_, value, fprec));
        stack_.push(ni);
    } else if (node_type == "String") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a String" << endl;
#endif
        ustring value_str     = lookupAttribute(attributes, L"value");

        /// Push onto stack
        shared_ptr<StringNodeImpl> ni(new StringNodeImpl(imf_, value_str));
        stack_.push(ni);
    } else if (node_type == "Blob") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a Blob" << endl;
#endif
        ustring fileOffset_str = lookupAttribute(attributes, L"fileOffset");
        ustring length_str     = lookupAttribute(attributes, L"length");

        int64_t fileOffset     = _atoi64(fileOffset_str.c_str());    //??? not portable
        int64_t length         = _atoi64(length_str.c_str());  //??? not portable

        /// Push onto stack
        shared_ptr<BlobNodeImpl> ni(new BlobNodeImpl(imf_, fileOffset, length));
        stack_.push(ni);
    } else if (node_type == "Structure") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a Structure" << endl;
#endif
        //??? read name space decls

        /// Push onto stack
        shared_ptr<StructureNodeImpl> ni(new StructureNodeImpl(imf_));
        stack_.push(ni);
    } else if (node_type == "Vector") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a Vector" << endl;
#endif
        ustring allowHetero_str = lookupAttribute(attributes, L"allowHeterogeneousChildren");

        bool allowHetero = atoi(allowHetero_str.c_str()) != 0;

        /// Push onto stack
        shared_ptr<VectorNodeImpl> ni(new VectorNodeImpl(imf_, allowHetero));
        stack_.push(ni);
    } else if (node_type == "CompressedVector") {
#ifdef E57_MAX_VERBOSE
        //cout << "got a CompressedVector" << endl;
#endif
        ustring fileOffset_str  = lookupAttribute(attributes, L"fileOffset");
        ustring recordCount_str = lookupAttribute(attributes, L"recordCount");

        int64_t physicalOffset = _atoi64(fileOffset_str.c_str());    //??? not portable
        int64_t recordCount    = _atoi64(recordCount_str.c_str());   //??? not portable

        /// Construct a CompressedVector without prototype or codecs trees (don't have them yet).
        shared_ptr<CompressedVectorNodeImpl> cvi(new CompressedVectorNodeImpl(imf_));

        /// Set fileOffset and recordCount attributes of CompressedVector
        cvi->setRecordCount(recordCount);
        cvi->setBinarySectionLogicalStart(imf_->file_->physicalToLogical(physicalOffset));  //??? what if file_ is NULL?

        /// Push onto stack
        stack_.push(cvi);
    } else
        throw EXCEPTION("unknown node_type");
}

void E57XmlParser::endElement(const XMLCh* const uri,
                              const XMLCh* const localname,
                              const XMLCh* const qname)
{
#ifdef E57_MAX_VERBOSE
    //cout << "endElement" << endl;
#endif

    /// Pop the node that just ended
    shared_ptr<NodeImpl> ni = stack_.top();
    stack_.pop();

    /// If first node in file ended, we are all done
    if (stack_.empty()) {
        imf_->root_ = dynamic_pointer_cast<StructureNodeImpl>(ni);
        return;
    }

    shared_ptr<NodeImpl> container_ni = stack_.top();

    /// Add node to container at top of stack
    switch (container_ni->type()) {
        case E57_STRUCTURE: {
            shared_ptr<StructureNodeImpl> struct_ni = dynamic_pointer_cast<StructureNodeImpl>(container_ni);

            /// Add named child to structure
            struct_ni->set(toUString(qname), ni);
            }
            break;
        case E57_VECTOR: {
            shared_ptr<VectorNodeImpl> vector_ni = dynamic_pointer_cast<VectorNodeImpl>(container_ni);

            /// Add unnamed child to vector
            vector_ni->append(ni);
            }
            break;
        case E57_COMPRESSED_VECTOR: {
            shared_ptr<CompressedVectorNodeImpl> cv_ni = dynamic_pointer_cast<CompressedVectorNodeImpl>(container_ni);
                ustring uQname = toUString(qname);

                /// n can be either prototype or codecs
                if (uQname == "prototype")
                    cv_ni->setPrototype(ni);
                else if (uQname == "codecs")
                    cv_ni->setCodecs(ni);
                else
                    throw EXCEPTION("unknown XML child element of CompressedVector");
            }
            break;
        default:
            throw EXCEPTION("bad XML nesting");
    }
}


void E57XmlParser::processingInstruction(const XMLCh* const target,
                                      const XMLCh* const data)
{
#ifdef E57_MAX_VERBOSE
    //cout << "processingInstruction" << endl;
#endif
}


void E57XmlParser::characters(const   XMLCh* const    chars,
                        const   XMLSize_t    length)
{
    //???throw EXCEPTION("unexpected characters");
}

void E57XmlParser::error(const SAXParseException& e)
{
    //??? for now
    cerr << "\nError at file " << XMLString::transcode(e.getSystemId())
         << ", line " << e.getLineNumber()
         << ", char " << e.getColumnNumber()
         << "\n  Message: " << XMLString::transcode(e.getMessage()) << endl;
    throw EXCEPTION("XML E57XmlParser error");
}

void E57XmlParser::fatalError(const SAXParseException& e)
{
    //??? for now
    cerr << "\nFatal Error at file " << XMLString::transcode(e.getSystemId())
         << ", line " << e.getLineNumber()
         << ", char " << e.getColumnNumber()
         << "\n  Message: " << XMLString::transcode(e.getMessage()) << endl;
    throw EXCEPTION("XML E57XmlParser fatal error");
}

void E57XmlParser::warning(const SAXParseException& e)
{
    //??? for now
    cerr << "\nWarning at file " << XMLString::transcode(e.getSystemId())
         << ", line " << e.getLineNumber()
         << ", char " << e.getColumnNumber()
         << "\n  Message: " << XMLString::transcode(e.getMessage()) << endl;
    throw EXCEPTION("XML E57XmlParser warning");
}

ustring E57XmlParser::toUString(const XMLCh* const xml_str)
{
    char* cp_str = NULL;
    ustring u_str;

    try {
        /// Use Xerces to convert a XMLCh* to char* to ustring
        ///??? uses allocated buf, if small input, use buf on stack?, for speed
        cp_str = XMLString::transcode(xml_str);
        if (cp_str == NULL)
            throw EXCEPTION("bad unicode translation");
        u_str = ustring(cp_str);
    } catch (...) {
        if (cp_str != NULL)
            XMLString::release(&cp_str);
        throw;  // rethrow
    }
    XMLString::release(&cp_str);
    return(u_str);
}

ustring E57XmlParser::lookupAttribute(const Attributes& attributes, wchar_t* attribute_name)
{
    XMLSize_t attr_index;
    if (!attributes.getIndex(static_cast<XMLCh*>(attribute_name), attr_index))  //??? may not be portable static_cast
        throw EXCEPTION("missing attribute");
    return(toUString(attributes.getValue(attr_index)));
}

//=============================================================================
//=============================================================================
//=============================================================================

ImageFileImpl::ImageFileImpl()
{
    /// First phase of construction, can't do much until have the ImageFile object.
    /// See ImageFileImpl::construct2() for second phase.
}

void ImageFileImpl::construct2(const ustring& fname, const ustring& mode, const ustring& configuration)
{
    /// Second phase of construction, now we have a well-formed ImageFile object.

#ifdef E57_MAX_VERBOSE
    ///cout << "ImageFileImpl() called, fname=" << fname << " mode=" << mode << " configuration=" << configuration << endl;
#endif

    fname_ = fname;

    /// Get shared_ptr to this object
    shared_ptr<ImageFileImpl> imf=shared_from_this();
    shared_ptr<StructureNodeImpl> root(new StructureNodeImpl(imf));
    root_ = root; //??? ok?

    //??? allow "rw" or "a"?
    if (mode == "w")
        isWriter_ = true;
    else if (mode == "r")
        isWriter_ = false;
    else
        throw "unknown ImageFile mode";

    /// If mode is read, do it
    file_ = NULL;
    if (!isWriter_) {
        try {
            /// Open file for reading. !!!remember to close, try block here?
            file_ = new CheckedFile(fname_, CheckedFile::readOnly);

#ifdef E57_DEBUG
            /// Double check that compiler thinks sizeof header is what it is supposed to be
            if (sizeof(E57FileHeader) != 40)
                throw EXCEPTION("internal error");
#endif

            /// Fetch the file header
            E57FileHeader header;
            file_->read(reinterpret_cast<char*>(&header), sizeof(header));
#ifdef E57_MAX_VERBOSE
            header.dump(); //???
#endif
            header.swab();  /// swab if neccesary

            /// Check signature
            if (strncmp(header.fileSignature, "ASTM-E57", 8) != 0)
                throw EXCEPTION("bad file signature");

            /// Check file version compatibility ???
            if (header.majorVersion != E57_VERSION_MAJOR)
                throw EXCEPTION("bad file version");

            /// If is a prototype version (majorVersion==0), then minorVersion has to match too.
            /// In production versions (majorVersion>=1), should be able to handle any minor version.
            if (header.majorVersion == 0 && header.minorVersion != E57_VERSION_MINOR)
                throw EXCEPTION("bad file version");

            ///??? stash major,minor numbers for API?

            /// Check if file length matches actual physical length
            if (header.filePhysicalLength != file_->length(CheckedFile::physical))
                throw EXCEPTION("bad file length");

            xmlLogicalOffset_ = file_->physicalToLogical(header.xmlPhysicalOffset);
            xmlLogicalLength_ = header.xmlLogicalLength;
        } catch (...) {
            if (file_ != NULL) {
                delete file_;
                file_ = NULL;
            }
            throw;  // rethrow
        }

        SAX2XMLReader* xmlReader = NULL;

        // Initialize the XML4C2 system
        try {
             XMLPlatformUtils::Initialize();
        } catch (const XMLException& toCatch) {
             cerr << "Error during initialization! :\n"
                  << toCatch.getMessage() << XERCES_STD_QUALIFIER endl;
             throw EXCEPTION("xerces init failed");
        }

        xmlReader = XMLReaderFactory::createXMLReader(); //??? auto_ptr?

        //??? check these are right
        xmlReader->setFeature(XMLUni::fgSAX2CoreValidation,        true);
        xmlReader->setFeature(XMLUni::fgXercesDynamic,             true);
        xmlReader->setFeature(XMLUni::fgSAX2CoreNameSpaces,        true);
        xmlReader->setFeature(XMLUni::fgXercesSchema,              true);
        xmlReader->setFeature(XMLUni::fgXercesSchemaFullChecking,  true);
        xmlReader->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, true);

        try {
            /// Create parser state, attach its event handers to the SAX2 reader
            E57XmlParser parser(imf);
            xmlReader->setContentHandler(&parser);
            xmlReader->setErrorHandler(&parser);

            /// Create input source (XML section of E57 file turned into a stream).
            E57FileInputSource xmlSection(file_, xmlLogicalOffset_, xmlLogicalLength_);

            /// Do the parse, building up the node tree
            xmlReader->parse(xmlSection);
        } catch (...) {
            if (xmlReader != NULL)
                delete xmlReader;
            if (file_ != NULL) {
                delete file_;
                file_ = NULL;
            }
            throw;  // rethrow
        }
        delete xmlReader;
    } else { /// open for writing (start empty)
        /// Open file for writing, truncate if already exists. !!!remember to close
        file_ = new CheckedFile(fname_, CheckedFile::writeCreate);

        unusedLogicalStart_ = sizeof(E57FileHeader);
    }
}

shared_ptr<StructureNodeImpl> ImageFileImpl::root()
{
    return(root_);
}

void ImageFileImpl::close()
{
    //??? check if already closed
    //??? flush, close

    /// If file already closed, have nothing to do
    if (file_ == NULL)
        return;

    if (isWriter_) {
        /// Go to end of file, note physical position
        xmlLogicalOffset_ = unusedLogicalStart_;
        file_->seek(xmlLogicalOffset_, CheckedFile::logical);
        uint64_t xmlPhysicalOffset = file_->position(CheckedFile::physical);
        *file_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

        //??? need to add name space attributes to image3DFile
        root_->writeXml(shared_from_this(), *file_, 0, "image3DFile");

        /// Pad XML section so length is multiple of 4
        while ((file_->position(CheckedFile::logical) - xmlLogicalOffset_) % 4 != 0)
            *file_ << " ";

        /// Note logical length
        xmlLogicalLength_ = file_->position(CheckedFile::logical) - xmlLogicalOffset_;

        /// Init header contents
        E57FileHeader header;
        memset(&header, 0, sizeof(header));  /// need to init to zero, ok since no constructor
        memcpy(&header.fileSignature, "ASTM-E57", 8);
        header.majorVersion       = E57_VERSION_MAJOR;
        header.minorVersion       = E57_VERSION_MINOR;
        header.filePhysicalLength = file_->length(CheckedFile::physical);
        header.xmlPhysicalOffset  = xmlPhysicalOffset;
        header.xmlLogicalLength   = xmlLogicalLength_;
#ifdef E57_MAX_VERBOSE
        header.dump(); //???
#endif
        header.swab();  /// swab if neccesary

        /// Write header at beginning of file
        file_->seek(0);
        file_->write(reinterpret_cast<char*>(&header), sizeof(header));

        file_->close();
    }

    delete file_;
    file_ = NULL;
}

void ImageFileImpl::cancel()
{
    //??? flush, close
}

ImageFileImpl::~ImageFileImpl()
{
    //??? warning if not closed, if open, close(),

    /// Try to close if not already, but don't allow any exceptions to propogate to caller
    try {
        if (file_ != NULL) {
            close();
        }
    } catch (...) {};

    /// Just in case close failed without freeing file_, do free here.
    if (file_ != NULL) {
        delete file_;
        file_ = NULL;
    }
}

uint64_t ImageFileImpl::allocateSpace(uint64_t byteCount, bool doExtendNow)
{
    uint64_t oldLogicalStart = unusedLogicalStart_;

    /// Reserve space at end of file
    unusedLogicalStart_ += byteCount;

    /// If caller won't write to file immediately, it should request that the file be extended with zeros here
    if (doExtendNow)
        file_->extend(unusedLogicalStart_);

    return(oldLogicalStart);
}

void ImageFileImpl::extensionsAdd(const ustring& prefix, const ustring& uri)
{
    //??? check if prefix characters ok, check if uri has a double quote char (others?)

    /// Check to make sure that neither prefix or uri is already defined.
    ustring dummy;
    if (extensionsLookupPrefix(prefix, dummy))
        throw EXCEPTION("duplicate namespace prefix");
    if (extensionsLookupUri(uri, dummy))
        throw EXCEPTION("duplicate namespace uri");  //??? is this really an error?

    /// Append at end of list
    nameSpaces_.push_back(NameSpace(prefix, uri));
}

bool ImageFileImpl::extensionsLookupPrefix(const ustring& prefix, ustring& uri)
{
    vector<NameSpace>::iterator it;
    for (it = nameSpaces_.begin(); it < nameSpaces_.end(); ++it) {
        if (it->prefix == prefix) {
            uri = it->uri;
            return(true);
        }
    }
    return(false);
}

bool ImageFileImpl::extensionsLookupUri(const ustring& uri, ustring& prefix)
{
    vector<NameSpace>::iterator it;
    for (it = nameSpaces_.begin(); it < nameSpaces_.end(); ++it) {
        if (it->uri == uri) {
            prefix = it->prefix;
            return(true);
        }
    }
    return(false);
}

bool ImageFileImpl::fieldNameIsExtension(const ustring& fieldName)
{
    throw EXCEPTION("not implemented");  //!!! implement
    return(false);
}

void ImageFileImpl::fieldNameParse(const ustring& fieldName, ustring& prefix, ustring& base)
{
    throw EXCEPTION("not implemented");  //!!! implement
}

void ImageFileImpl::pathNameParse(const ustring& pathName, bool& isRelative, vector<ustring>& fields)
{
    /// Clear previous contents of fields vector
    fields.clear();

    if (pathName.size() == 0) {
        isRelative = true;
        return;
    }
    size_t start = 0;

    /// Check if absolute path
    if (pathName[start] == '/') {
        isRelative = false;
        start = 1;
    } else
        isRelative = true;

    /// Save strings in between each forward slash '/'
    /// Don't ignore whitespace
    while (start < pathName.size()) {
        size_t slash = pathName.find_first_of('/', start);
        fields.push_back(pathName.substr(start, slash-start));
        if (slash == string::npos)
            break;

        /// Handle case when pathname ends in /, e.g. "/foo/", add empty field at end of list
        if (slash == pathName.size()-1) {
            fields.push_back("");
            break;
        }

        /// Skip over the slash and keep going
        start = slash + 1;
    }
}

ustring ImageFileImpl::pathNameUnparse(bool isRelative, const vector<ustring>& fields)
{
    ustring path;

    if (!isRelative)
        path.push_back('/');
    for (unsigned i = 0; i < fields.size(); i++) {
        path.append(fields.at(i));
        if (i < fields.size()-1)
            path.push_back('/');
    }
    return(path);
}

ustring ImageFileImpl::fileNameExtension(const ustring& fileName)
{
    size_t last_dot = fileName.find_last_of('.');
    size_t last_slash = fileName.find_last_of('/');  //??? unix naming dependency here

    if (last_slash == string::npos || last_dot > last_slash)
        return(fileName.substr(last_dot+1));
    else
        return("");
}

void ImageFileImpl::fileNameParse(const ustring& fileName, bool& isRelative, ustring& volumeName, vector<ustring>& directories,
                                ustring& fileBase, ustring& extension)
{
    throw EXCEPTION("not implemented");  //!!! implement
}

ustring ImageFileImpl::fileNameUnparse(bool isRelative, const ustring& volumeName, const vector<ustring>& directories,
                                    const ustring& fileBase, const ustring& extension)
{
    throw EXCEPTION("not implemented");  //!!! implement
}

void ImageFileImpl::dump(int indent, ostream& os)
{
    os << space(indent) << "fname:     " << fname_ << endl;
    os << space(indent) << "isWriter:  " << isWriter_ << endl;
    for (int i=0; i < extensionsCount(); i++)
        os << space(indent) << "nameSpace[" << i << "]: prefix=" << extensionsPrefix(i) << " uri=" << extensionsUri(i) << endl;
    os << space(indent) << "root:      " << endl;
    root_->dump(indent+2, os);
}

unsigned ImageFileImpl::bitsNeeded(int64_t minimum, int64_t maximum)
{
    /// Relatively quick way to compute ceil(log2(maximum - minimum + 1)));
    /// Uses only integer operations and is machine independent (no assembly code).
    /// Find the bit position of the first 1 (from left) in the binary form of stateCountMinus1.

    uint64_t stateCountMinus1 = maximum - minimum;
    unsigned log2 = 0;
    if (stateCountMinus1 & 0xFFFFFFFF00000000LL) {
        stateCountMinus1 >>= 32;
        log2 += 32;
    }
    if (stateCountMinus1 & 0xFFFF0000LL) {
        stateCountMinus1 >>= 16;
        log2 += 16;
    }
    if (stateCountMinus1 & 0xFF00LL) {
        stateCountMinus1 >>= 8;
        log2 += 8;
    }
    if (stateCountMinus1 & 0xF0LL) {
        stateCountMinus1 >>= 4;
        log2 += 4;
    }
    if (stateCountMinus1 & 0xCLL) {
        stateCountMinus1 >>= 2;
        log2 += 2;
    }
    if (stateCountMinus1 & 0x2LL) {
        stateCountMinus1 >>= 1;
        log2 += 1;
    }
    if (stateCountMinus1 & 1LL)
        log2++;
    return(log2);
}

#ifdef BITSNEEDED_UNIT_TEST

void test1(int64_t minimum, int64_t maximum)
{
#ifdef E57_MAX_VERBOSE
    cout << "bitsNeeded(" << minimum << "," << maximum << ") = " << bitsNeeded(minimum, maximum) << endl;
#endif
}

void main()
{
    test1(0, 0);
    test1(0, 1);
    test1(0, 2);
    test1(0, 3);
    test1(0, 4);
    test1(0, 5);
    test1(0, 6);
    test1(0, 7);
    test1(0, 8);
    cout << endl;

    test1(1, 1);
    test1(1, 2);
    test1(-128, 127);
    cout << endl;

    test1(INT8_MIN, INT8_MAX);
    test1(INT16_MIN, INT16_MAX);
    test1(INT32_MIN, INT32_MAX);
    test1(INT64_MIN, INT64_MAX);
    cout << endl;

    for (int i=0; i < 64; i++) {
        if (bitsNeeded(0, 1LL<<i) != i+1) {
            cout << "OOPS: i=" << i << endl;
            exit(-1);
        }
    }

    cout << endl;
    for (int i=0; i < 64; i++)
        test1(0, 3LL<<i);
    cout << endl;
    for (int i=0; i < 64; i++)
        test1(0, 5LL<<i);
}

#endif

//================================================================

CheckedFile::CheckedFile(ustring fname, Mode mode)
: fd_(-1)
{
    switch (mode) {
        case readOnly:
            fd_ = open(fname.c_str()/*???*/, O_RDONLY|O_BINARY);
            if (fd_ < 0)
                throw EXCEPTION("open for reading failed");
            readOnly_ = true;
            logicalLength_ = physicalToLogical(length(physical));
            break;
        case writeCreate:
            /// File truncated to zero length if already exists
            fd_ = open(fname.c_str()/*???*/, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IWRITE|S_IREAD);
            if (fd_ < 0)
                throw EXCEPTION("open for writing failed");
            readOnly_ = false;
            logicalLength_ = 0;
            break;
        case writeExisting:
            fd_ = open(fname.c_str()/*???*/, O_RDWR|O_BINARY);
            if (fd_ < 0)
                throw EXCEPTION("open for writing failed");
            readOnly_ = false;
            logicalLength_ = physicalToLogical(length(physical)); //???
            break;
    }
}

CheckedFile::~CheckedFile()
{
    try {
        close();  ///??? what if already closed?
    } catch (...) {
            //??? report?
    }
}

void CheckedFile::read(char* buf, size_t nRead, size_t bufSize)
{
//??? what if read past logical end?, or physical end?
//??? need to keep track of logical length?

#ifdef SLOW_MODE
    uint64_t end = position(logical) + nRead;

    if (end > length(logical))
        throw EXCEPTION("read past end of file");

    uint64_t page;
    size_t   pageOffset;
    getCurrentPageAndOffset(page, pageOffset);

    size_t n = min(nRead, logicalPageSize - pageOffset);

    /// Allocate temp page buffer
    vector<char> page_buffer_v(physicalPageSize);
    char* page_buffer = &page_buffer_v[0];

    while (nRead > 0) {
        readPhysicalPage(page_buffer, page);
        memcpy(buf, page_buffer+pageOffset, n);

        buf += n;
        nRead -= n;
        pageOffset = 0;
        page++;
        n = min(nRead, logicalPageSize);
    }

    /// When done, leave cursor just past end of last byte read
    seek(end, logical);
#else
    ???
#endif

}

void CheckedFile::write(const char* buf, size_t nWrite)
{
#ifdef E57_MAX_VERBOSE
    // cout << "write nWrite=" << nWrite << " position()="<< position() << endl; //???
#endif
    if (readOnly_)
        throw EXCEPTION("can't write to read only file");

#ifdef SLOW_MODE
    uint64_t end = position(logical) + nWrite;

    uint64_t page;
    size_t   pageOffset;
    getCurrentPageAndOffset(page, pageOffset);

    size_t n = min(nWrite, logicalPageSize - pageOffset);

    /// Allocate temp page buffer
    vector<char> page_buffer_v(physicalPageSize);
    char* page_buffer = &page_buffer_v[0];

    while (nWrite > 0) {
        readPhysicalPage(page_buffer, page);
#ifdef E57_MAX_VERBOSE
        // cout << "  page_buffer[0] read: '" << page_buffer[0] << "'" << endl;
        // cout << "copy " << n << "bytes to page=" << page << " pageOffset=" << pageOffset << " buf='"; //???
        for (size_t i=0; i < n; i++)
            // cout << buf[i];
        // cout << "'" << endl;
#endif
        memcpy(page_buffer+pageOffset, buf, n);
        writePhysicalPage(page_buffer, page);
#ifdef E57_MAX_VERBOSE
        // cout << "  page_buffer[0] after write: '" << page_buffer[0] << "'" << endl; //???
#endif
        buf += n;
        nWrite -= n;
        pageOffset = 0;
        page++;
        n = min(nWrite, logicalPageSize);
    }

    if (end > logicalLength_)
        logicalLength_ = end;

    /// When done, leave cursor just past end of buf
    seek(end, logical);
#else
    ???
#endif
}

CheckedFile& CheckedFile::operator<<(const ustring& s)
{
    write(s.c_str(), s.length()); //??? should be times size of uchar?
    return(*this);
}

CheckedFile& CheckedFile::operator<<(int64_t i)
{
    stringstream ss;
    ss << i;
    return(*this << ss.str());
}

CheckedFile& CheckedFile::operator<<(uint64_t i)
{
    stringstream ss;
    ss << i;
    return(*this << ss.str());
}

CheckedFile& CheckedFile::operator<<(double d)
{
    stringstream ss;
    ss << scientific << setprecision(16) << d;
    return(*this << ss.str());
}

void CheckedFile::seek(uint64_t offset, OffsetMode omode)
{
#ifdef SLOW_MODE
    //??? check for seek beyond logicalLength_
    __int64 pos = static_cast<__int64>(omode==physical ? offset : logicalToPhysical(offset));

#ifdef E57_MAX_VERBOSE
    // cout << "seek offset=" << offset << " omode=" << omode << " pos=" << pos << endl; //???
#endif
    __int64 result = _lseeki64(fd_, pos, SEEK_SET);  //??? Microsoft dependency
    if (result < 0)
        throw EXCEPTION("lseek failed");
#else
    ???
#endif
}

uint64_t CheckedFile::position(OffsetMode omode)
{
#ifdef SLOW_MODE
    /// Get current file cursor position
    __int64 pos = _lseeki64(fd_, 0LL, SEEK_CUR);  //??? Microsoft dependency
    if (pos < 0)
        throw EXCEPTION("lseek failed");

    if (omode==physical)
        return(static_cast<uint64_t>(pos));
    else
        return(physicalToLogical(static_cast<uint64_t>(pos)));
#else
    ???
#endif
}

uint64_t CheckedFile::length(OffsetMode omode)
{
#ifdef SLOW_MODE
    if (omode==physical) {
        //??? is there a 64bit length call?
        /// Get current file cursor position
        __int64 original_pos = _lseeki64(fd_, 0LL, SEEK_CUR);  //??? Microsoft dependency
        if (original_pos < 0)
            throw EXCEPTION("lseek failed");

        /// Get current file cursor position
        __int64 end_pos = _lseeki64(fd_, 0LL, SEEK_END);  //??? Microsoft dependency
        if (end_pos < 0)
            throw EXCEPTION("lseek failed");

        /// Restore original position
        if (_lseeki64(fd_, original_pos, SEEK_SET) < 0)  //??? Microsoft dependency
            throw EXCEPTION("lseek failed");

        return(static_cast<uint64_t>(end_pos));
    } else
        return(logicalLength_);
#else
    ???
#endif
}

void CheckedFile::extend(uint64_t newLength, OffsetMode omode)
{
#ifdef E57_MAX_VERBOSE
    // cout << "extend newLength=" << newLength << " omode="<< omode << endl; //???
#endif
    if (readOnly_)
        throw EXCEPTION("can't write to read only file");

#ifdef SLOW_MODE
    uint64_t newLogicalLength;
    if (omode==physical)
        newLogicalLength = physicalToLogical(newLength);
    else
        newLogicalLength = newLength;

    uint64_t currentLogicalLength = length(logical);

    /// Make sure we are trying to make file longer
    if (newLogicalLength < currentLogicalLength)
        throw EXCEPTION("can't extend file to shorter length");

    /// Calc how may zero bytes we have to add to end
    uint64_t nWrite = newLogicalLength - currentLogicalLength;

    /// Seek to current end of file
    seek(currentLogicalLength, logical);

    uint64_t page;
    size_t   pageOffset;
    getCurrentPageAndOffset(page, pageOffset);

    /// Calc first write size (may be partial page)
    /// Watch out for different int sizes here.
    size_t n;
    if (nWrite < logicalPageSize - pageOffset)
        n = static_cast<size_t>(nWrite);
    else
        n = logicalPageSize - pageOffset;

    /// Allocate temp page buffer
    vector<char> page_buffer_v(physicalPageSize);
    char* page_buffer = &page_buffer_v[0];

    while (nWrite > 0) {
        readPhysicalPage(page_buffer, page);
#ifdef E57_MAX_VERBOSE
        // cout << "extend " << n << "bytes on page=" << page << " pageOffset=" << pageOffset << endl; //???
#endif
        memset(page_buffer+pageOffset, 0, n);
        writePhysicalPage(page_buffer, page);

        nWrite -= n;
        pageOffset = 0;
        page++;

        if (nWrite < logicalPageSize)
            n = static_cast<size_t>(nWrite);
        else
            n = logicalPageSize;
    }

    //??? what if loop above throws, logicalLength_ may be wrong
    logicalLength_ = newLogicalLength;

    /// When done, leave cursor at end of file
    seek(newLogicalLength, logical);
#else
    ???
#endif
}

void CheckedFile::flush()
{
#ifdef SLOW_MODE
    /// Nothing to do
#else
    ???
#endif
}

void CheckedFile::close()
{
    if (fd_ >= 0) {
#ifndef SLOW_MODE
        if (currentPageDirty_)
            finishPage();
#endif
        if (::close(fd_))
            throw EXCEPTION("close failed");
        fd_ = -1;
    }
}

size_t CheckedFile::efficientBufferSize(size_t logicalBytes)
{
#ifdef SLOW_MODE
    return(logicalBytes);
#else
    ???
#endif
}

uint32_t CheckedFile::checksum(char* buf, size_t size)
{
#ifdef SLOW_MODE
    ///??? for now
    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++)
        sum += static_cast<uint32_t>(buf[i]);
    return(sum+1);
#else
    ???
#endif
}

#ifdef SLOW_MODE

void CheckedFile::getCurrentPageAndOffset(uint64_t& page, size_t& pageOffset, OffsetMode omode)
{
    uint64_t pos = position(omode);
    if (omode == physical) {
        page = pos >> physicalPageSizeLog2;
        pageOffset = static_cast<size_t>(pos & physicalPageSizeMask);
    } else {
        page = pos / logicalPageSize;
        pageOffset = static_cast<size_t>(pos - page * logicalPageSize);
    }
}

void CheckedFile::readPhysicalPage(char* page_buffer, uint64_t page)
{
#ifdef E57_MAX_VERBOSE
    // cout << "readPhysicalPage, page:" << page << endl;//???
#endif

    if (page*physicalPageSize >= length(physical)) {
        /// If beyond end of file, just return blank buffer  ???sure isn't partially beyond end?
        memset(page_buffer, 0, physicalPageSize);
    } else {
        /// Seek to start of physical page
        seek(page*physicalPageSize, physical);

        if (::read(fd_, page_buffer, physicalPageSize) != physicalPageSize)
            throw EXCEPTION("readPage failed");

        uint32_t check_sum = checksum(page_buffer, logicalPageSize);
        if(*reinterpret_cast<uint32_t*>(&page_buffer[logicalPageSize]) != check_sum) {  //??? little endian dependency
#ifdef E57_MAX_VERBOSE
            // cout << "page:" << page << endl;//???
            // cout << "length():" << length() << endl;//???
#endif
            throw EXCEPTION("checksum mismatch");
        }
    }
}

void CheckedFile::writePhysicalPage(char* page_buffer, uint64_t page)
{
#ifdef E57_MAX_VERBOSE
    // cout << "writePhysicalPage, page:" << page << endl;//???
#endif

    /// Append checksum
    uint32_t check_sum = checksum(page_buffer, logicalPageSize);
    *reinterpret_cast<uint32_t*>(&page_buffer[logicalPageSize]) = check_sum;  //??? little endian dependency

    /// Seek to start of physical page
    seek(page*physicalPageSize, physical);

    if (::write(fd_, page_buffer, physicalPageSize) < 0)
        throw EXCEPTION("writePage failed");
}

#endif

//=============================================================
#ifdef UNIT_TEST

using namespace e57;

void printState(CheckedFile& cf)
{
    cout << "  Current logical file position:  " << cf.position(CheckedFile::logical) << endl;
    cout << "  Current physical file position: " << cf.position(CheckedFile::physical)<< endl;
    cout << "  Current logical file length:    " << cf.length(CheckedFile::logical) << endl;
    cout << "  Current physical file length:   " << cf.length(CheckedFile::physical) << endl;
}

int main()
{
    try {
        CheckedFile cf("checked_file_test.e57", CheckedFile::writeCreate);

        printState(cf);

        cout << "Writing 'hey'" << endl;
        cf.write("hey", 3);

        printState(cf);

        cout << "Writing ' you'" << endl;
        cf.write(" you", 4);

        printState(cf);

#if 0
        for (int i=0; i < 11; i++) {
            cout << "Writing ' yada yada...'" << endl;
            cf.write(" yada yada yada yada yada yada yada yada yada yada yada yada yada yada yada yada yada yada yada yada", 100);
        }

        printState(cf);

        uint64_t n = 2035;
        for (int i=0; i < 10; i++) {
            cout << "Extending to " << n << " bytes" << endl;
            cf.extend(n);
            printState(cf);
            n++;
        }
#else
        cout << "Extending to " << 1016 << " bytes" << endl;
        cf.extend(1016);

        for (int i=0; i < 6; i++) {
            cout << "Writing 'a'" << endl;
            char c = 'a' + i;
            cf.write(&c, 1);
            printState(cf);
        }
#endif
        cf.seek(0);
        char buf[8];
        cf.read(buf, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        cout << "Read: '" << buf << "'" << endl;
    } catch (char* s) {
        cerr << "Got an char* exception: " << s << endl;
    } catch (const char* s) {
        cerr << "Got an const char* exception: " << s << endl;
    } catch (...) {
        cerr << "Got an exception" << endl;
    }

    return(0);
}

#endif

//================================================================

CompressedVectorSectionHeader::CompressedVectorSectionHeader()
{
    /// Double check that header is correct length.  Watch out for RTTI increasing the size.
    if (sizeof(CompressedVectorSectionHeader) != 32)
        throw EXCEPTION("internal error");

    /// Now confident we have correct size, zero header.
    /// This guarantees that headers are always completely initialized to zero.
    memset(this, 0, sizeof(*this));
}

void CompressedVectorSectionHeader::verify(uint64_t filePhysicalSize)
{
    /// Verify that section is correct type
    if (sectionId != E57_COMPRESSED_VECTOR_SECTION)
        throw EXCEPTION("expected CompressedVector section id");

    /// Verify reserved fields are zero. ???  if fileversion==1.0 ???
    for (unsigned i=0; i < sizeof(reserved1); i++) {
        if (reserved1[i] != 0)
            throw EXCEPTION("reserved field not zero");
    }

    /// Check section length is multiple of 4
    if (sectionLogicalLength % 4)
        throw EXCEPTION("bad section length");

    /// Check sectionLogicalLength is in bounds
    if (filePhysicalSize > 0 && sectionLogicalLength >= filePhysicalSize)
        throw EXCEPTION("section length out of bounds");

    /// Check dataPhysicalOffset is in bounds
    if (filePhysicalSize > 0 && dataPhysicalOffset >= filePhysicalSize)
        throw EXCEPTION("data offset out of bounds");

    /// Check indexPhysicalOffset is in bounds
    if (filePhysicalSize > 0 && indexPhysicalOffset >= filePhysicalSize)
        throw EXCEPTION("index offset out of bounds");
}

#ifdef E57_BIGENDIAN
void CompressedVectorSectionHeader::swab()
{
    /// Byte swap fields in-place, if CPU is BIG_ENDIAN
    swab(&sectionLogicalLength);
    swab(&dataPhysicalOffset);
    swab(&indexPhysicalOffset);
};
#endif

#ifdef E57_DEBUG
void CompressedVectorSectionHeader::dump(int indent, std::ostream& os)
{
    os << space(indent) << "sectionId:            " << (unsigned)sectionId << endl;
    os << space(indent) << "sectionLogicalLength: " << sectionLogicalLength << endl;
    os << space(indent) << "dataPhysicalOffset:   " << dataPhysicalOffset << endl;
    os << space(indent) << "indexPhysicalOffset:  " << indexPhysicalOffset << endl;
}
#endif

///================================================================

DataPacketHeader::DataPacketHeader()
{
    /// Double check that packet struct is correct length.  Watch out for RTTI increasing the size.
    if (sizeof(*this) != 6)
        throw EXCEPTION("internal error");

    /// Now confident we have correct size, zero packet.
    /// This guarantees that data packet headers are always completely initialized to zero.
    memset(this, 0, sizeof(*this));
}

void DataPacketHeader::verify(unsigned bufferLength)
{
    /// Verify that packet is correct type
    if (packetType != E57_DATA_PACKET)
        throw EXCEPTION("packet not type not DATA");

    /// ??? check reserved flags zero?

    /// Check packetLength is at least large enough to hold header
    unsigned packetLength = packetLogicalLengthMinus1+1;
    if (packetLength < sizeof(*this))
        throw EXCEPTION("bad packet length");

    /// Check packet length is multiple of 4
    if (packetLength % 4)
        throw EXCEPTION("bad packet length");

    /// Check actual packet length is large enough.
    if (bufferLength > 0 && packetLength > bufferLength)
        throw EXCEPTION("bad packet length");

    /// Make sure there is at least one entry in packet  ??? 0 record cvect allowed?
    if (bytestreamCount == 0)
        throw EXCEPTION("data packet with no steams");

    /// Check packet is at least long enough to hold bytestreamBufferLength array
    if (sizeof(DataPacketHeader) + 2*bytestreamCount > packetLength)
        throw EXCEPTION("packet too short");
}

#ifdef E57_BIGENDIAN
void DataPacketHeader::swab()
{
    /// Byte swap fields in-place, if CPU is BIG_ENDIAN
    swab(&packetLogicalLengthMinus1);
    swab(&bytestreamCount);
};
#endif

#ifdef E57_DEBUG
void DataPacketHeader::dump(int indent, std::ostream& os)
{
    os << space(indent) << "packetType:                " << (unsigned)packetType << endl;
    os << space(indent) << "packetFlags:               " << (unsigned)packetFlags << endl;
    os << space(indent) << "packetLogicalLengthMinus1: " << packetLogicalLengthMinus1 << endl;
    os << space(indent) << "bytestreamCount:           " << bytestreamCount << endl;
}
#endif

//================================================================

DataPacket::DataPacket()
{
    /// Double check that packet struct is correct length.  Watch out for RTTI increasing the size.
    if (sizeof(*this) != 64*1024)
        throw EXCEPTION("internal error");

    /// Now confident we have correct size, zero packet.
    /// This guarantees that data packets are always completely initialized to zero.
    memset(this, 0, sizeof(*this));
}

void DataPacket::verify(unsigned bufferLength)
{
    //??? do all packets need versions?  how extend without breaking older checking?  need to check file version#?

    /// Verify header is good
    DataPacketHeader* hp = reinterpret_cast<DataPacketHeader*>(this);
    hp->verify(bufferLength);

    /// Calc sum of lengths of each bytestream buffer in this packet
    uint16_t* bsbLength = reinterpret_cast<uint16_t*>(&payload[0]);
    unsigned totalStreamByteCount = 0;
    for (unsigned i=0; i < bytestreamCount; i++)
        totalStreamByteCount += bsbLength[i];

    /// Calc size of packet needed
    unsigned packetLength = packetLogicalLengthMinus1+1;
    unsigned needed = sizeof(DataPacketHeader) + 2*bytestreamCount + totalStreamByteCount;
#ifdef E57_MAX_VERBOSE
    cout << "needed=" << needed << " actual=" << packetLength << endl; //???
#endif

    /// If needed is not with 3 bytes of actual packet size, have an error
    if (needed > packetLength || needed+3 < packetLength)
        throw EXCEPTION("bad packet length");

    /// Verify that padding at end of packet is zero
    for (unsigned i=needed; i < packetLength; i++) {
        if (reinterpret_cast<char*>(this)[i] != 0)
            throw EXCEPTION("non-zero padding");
    }
}

char* DataPacket::getBytestream(unsigned bytestreamNumber, unsigned& byteCount)
{
#ifdef E57_MAX_VERBOSE
    cout << "getBytestream called, bytestreamNumber=" << bytestreamNumber << endl;
#endif

    /// Verify that packet is correct type
    if (packetType != E57_DATA_PACKET)
        throw EXCEPTION("packet not type not DATA");

    /// Check bytestreamNumber in bounds
    if (bytestreamNumber >= bytestreamCount)
        throw EXCEPTION("stream index out of bounds");

    /// Calc positions in packet
    uint16_t* bsbLength = reinterpret_cast<uint16_t*>(&payload[0]);
    char* streamBase = reinterpret_cast<char*>(&bsbLength[bytestreamCount]);

    /// Sum size of preceeding stream buffers to get position
    unsigned totalPreceeding = 0;
    for (unsigned i=0; i < bytestreamNumber; i++)
        totalPreceeding += bsbLength[i];

    byteCount = bsbLength[bytestreamNumber];

    /// Double check buffer is completely within packet
    if (sizeof(DataPacketHeader) + 2*bytestreamCount + totalPreceeding + byteCount > packetLogicalLengthMinus1 + 1U)
        throw EXCEPTION("internal error");

    /// Return start of buffer
    return(&streamBase[totalPreceeding]);
}

unsigned DataPacket::getBytestreamBufferLength(unsigned bytestreamNumber)
{
    //??? for now:
    unsigned byteCount;
    (void) getBytestream(bytestreamNumber, byteCount);
    return(byteCount);
}

#ifdef E57_BIGENDIAN
DataPacket::swab(bool toLittleEndian)
{
    /// Be a little paranoid
    if (packetType != E57_INDEX_PACKET)
        throw EXCEPTION("packet not index type");

    swab(packetLogicalLengthMinus1);

    /// Need to watch out if packet starts out in natural CPU ordering or not
    unsigned goodEntryCount;
    if (toLittleEndian) {
        /// entryCount starts out in correct order, save it before trashing
        goodEntryCount = entryCount;
        swab(entryCount);
    } else {
        /// Have to fix entryCount before can use.
        swab(entryCount);
        goodEntryCount = entryCount;
    }

    /// Make sure we wont go off end of buffer (e.g. if we accidentally swab)
    if (goodEntryCount > MAX_ENTRIES)
        throw EXCEPTION("too many entries in index packet");

    for (unsigned i=0; i < goodEntryCount; i++) {
        swab(entries[i].chunkRecordNumber);
        swab(entries[i].chunkPhysicalOffset);
    }
}
#endif

#ifdef E57_DEBUG
void DataPacket::dump(int indent, std::ostream& os)
{
    if (packetType != E57_DATA_PACKET)
        throw EXCEPTION("internal error");
    reinterpret_cast<DataPacketHeader*>(this)->dump(indent, os);

    uint16_t* bsbLength = reinterpret_cast<uint16_t*>(&payload[0]);
    uint8_t* p = reinterpret_cast<uint8_t*>(&bsbLength[bytestreamCount]);
    for (unsigned i=0; i < bytestreamCount; i++) {
        os << space(indent) << "bytestream[" << i << "]:" << endl;
        os << space(indent+4) << "length: " << bsbLength[i] << endl;
/*!!!!
        unsigned j;
        for (j=0; j < bsbLength[i] && j < 10; j++)
            os << space(indent+4) << "byte[" << j << "]=" << (unsigned)p[j] << endl;
        if (j < bsbLength[i])
            os << space(indent+4) << bsbLength[i]-j << " more unprinted..." << endl;
!!!!*/
        p += bsbLength[i];
        if (p - reinterpret_cast<uint8_t*>(this) > E57_DATA_PACKET_MAX)
            throw EXCEPTION("internal error");
    }
}
#endif

//================================================================

IndexPacket::IndexPacket()
{
    /// Double check that packet struct is correct length.  Watch out for RTTI increasing the size.
    if (sizeof(*this) != 16+16*MAX_ENTRIES)
        throw EXCEPTION("internal error");

    /// Now confident we have correct size, zero packet.
    /// This guarantees that index packets are always completely initialized to zero.
    memset(this, 0, sizeof(*this));
}

void IndexPacket::verify(unsigned bufferLength, uint64_t totalRecordCount, uint64_t fileSize)
{
    //??? do all packets need versions?  how extend without breaking older checking?  need to check file version#?

    /// Verify that packet is correct type
    if (packetType != E57_INDEX_PACKET)
        throw EXCEPTION("packet not index type");

    /// Check packetLength is at least large enough to hold header
    unsigned packetLength = packetLogicalLengthMinus1+1;
    if (packetLength < sizeof(*this))
        throw EXCEPTION("bad packet length");

    /// Check packet length is multiple of 4
    if (packetLength % 4)
        throw EXCEPTION("bad packet length");

    /// Make sure there is at least one entry in packet  ??? 0 record cvect allowed?
    if (entryCount == 0)
        throw EXCEPTION("empty index packet");

    /// Have to have <= 2048 entries
    if (entryCount > MAX_ENTRIES)
        throw EXCEPTION("too many entries in index packet");

    /// Index level should be <= 5.  Because (5+1)* 11 bits = 66 bits, which will cover largest number of chunks possible.
    if (indexLevel > 5)
        throw EXCEPTION("index packet level too deep");

    /// Index packets above level 0 must have at least two entries (otherwise no point to existing).
    ///??? check that this is in spec
    if (indexLevel > 0 && entryCount < 2)
        throw EXCEPTION("unnecessary index packet");

    /// Verify reserved fields are zero. ???  if fileversion==1.0 ???
    for (unsigned i=0; i < sizeof(reserved1); i++) {
        if (reserved1[i] != 0)
            throw EXCEPTION("reserved field not zero");
    }

    /// Check actual packet length is large enough.
    if (bufferLength > 0 && packetLength > bufferLength)
        throw EXCEPTION("bad packet length");

    /// Check if entries will fit in space provided
    unsigned neededLength = 16 + 8*entryCount;
    if (packetLength < neededLength)
        throw EXCEPTION("bad packet length");

#ifdef E57_MAX_DEBUG
    /// Verify padding at end is zero.
    char* p = reinterpret_cast<char*>(this);
    for (unsigned i=neededLength; i < packetLength; i++) {
        if (p[i] != 0)
            throw EXCEPTION("non-zero padding");
    }

    /// Verify records and offsets are in sorted order
    for (unsigned i=0; i < entryCount; i++) {
        /// Check chunkRecordNumber is in bounds
        if (totalRecordCount > 0 && entries[i].chunkRecordNumber >= totalRecordCount)
            throw EXCEPTION("chunk record number out of bounds");  //??? mark exceptions as file content vs. program error

        /// Check record numbers are strictly increasing
        if (i > 0 && entries[i-1].chunkRecordNumber >= entries[i].chunkRecordNumber)
            throw EXCEPTION("bad record number ordering in index");

        /// Check chunkPhysicalOffset is in bounds
        if (fileSize > 0 && entries[i].chunkPhysicalOffset >= fileSize)
            throw EXCEPTION("chunk offset out of bounds");

        /// Check chunk offsets are strictly increasing
        if (i > 0 && entries[i-1].chunkPhysicalOffset >= entries[i].chunkPhysicalOffset)
            throw EXCEPTION("bad chunk offset ordering in index");
    }
#endif
}

#ifdef E57_BIGENDIAN
IndexPacket::swab(bool toLittleEndian)
{
    /// Be a little paranoid
    if (packetType != E57_INDEX_PACKET)
        throw EXCEPTION("packet not index type");

    swab(packetLogicalLengthMinus1);

    /// Need to watch out if packet starts out in natural CPU ordering or not
    unsigned goodEntryCount;
    if (toLittleEndian) {
        /// entryCount starts out in correct order, save it before trashing
        goodEntryCount = entryCount;
        swab(entryCount);
    } else {
        /// Have to fix entryCount before can use.
        swab(entryCount);
        goodEntryCount = entryCount;
    }

    /// Make sure we wont go off end of buffer (e.g. if we accidentally swab)
    if (goodEntryCount > MAX_ENTRIES)
        throw EXCEPTION("too many entries in index packet");

    for (unsigned i=0; i < goodEntryCount; i++) {
        swab(entries[i].chunkRecordNumber);
        swab(entries[i].chunkPhysicalOffset);
    }
}
#endif

#ifdef E57_DEBUG
void IndexPacket::dump(int indent, std::ostream& os)
{
    os << space(indent) << "packetType:                " << (unsigned)packetType << endl;
    os << space(indent) << "packetFlags:               " << (unsigned)packetFlags << endl;
    os << space(indent) << "packetLogicalLengthMinus1: " << packetLogicalLengthMinus1 << endl;
    os << space(indent) << "entryCount:                " << entryCount << endl;
    os << space(indent) << "indexLevel:                " << indexLevel << endl;
    unsigned i;
    for (i=0; i < entryCount && i < 10; i++) {
        os << space(indent) << "entry[" << i << "]:" << endl;
        os << space(indent+4) << "chunkRecordNumber:    " << entries[i].chunkRecordNumber << endl;
        os << space(indent+4) << "chunkPhysicalOffset:  " << entries[i].chunkPhysicalOffset << endl;
    }
    if (i < entryCount)
        os << space(indent) << entryCount-i << "more entries unprinted..." << endl;
}
#endif

///================================================================

EmptyPacketHeader::EmptyPacketHeader()
{
    /// Double check that packet struct is correct length.  Watch out for RTTI increasing the size.
    if (sizeof(*this) != 4)
        throw EXCEPTION("internal error");

    /// Now confident we have correct size, zero packet.
    /// This guarantees that EmptyPacket headers are always completely initialized to zero.
    memset(this, 0, sizeof(*this));
}

void EmptyPacketHeader::verify(unsigned bufferLength)
{
    /// Verify that packet is correct type
    if (packetType != E57_EMPTY_PACKET)
        throw EXCEPTION("packet not EMPTY type");

    /// Check packetLength is at least large enough to hold header
    unsigned packetLength = packetLogicalLengthMinus1+1;
    if (packetLength < sizeof(*this))
        throw EXCEPTION("bad packet length");

    /// Check packet length is multiple of 4
    if (packetLength % 4)
        throw EXCEPTION("bad packet length");

    /// Check actual packet length is large enough.
    if (bufferLength > 0 && packetLength > bufferLength)
        throw EXCEPTION("bad packet length");
}

#ifdef E57_BIGENDIAN
void EmptyPacketHeader::swab()
{
    /// Byte swap fields in-place, if CPU is BIG_ENDIAN
    SWAB(&packetLogicalLengthMinus1);
};
#endif

#ifdef E57_DEBUG
void EmptyPacketHeader::dump(int indent, std::ostream& os)
{
    os << space(indent) << "packetType:                " << (unsigned)packetType << endl;
    os << space(indent) << "packetLogicalLengthMinus1: " << packetLogicalLengthMinus1 << endl;
}
#endif

///================================================================

struct SortByBytestreamNumber {
    bool operator () (Encoder* lhs , Encoder* rhs) const {
        return(lhs->bytestreamNumber() < rhs->bytestreamNumber());
    }
};

CompressedVectorWriterImpl::CompressedVectorWriterImpl(shared_ptr<CompressedVectorNodeImpl> ni, vector<SourceDestBuffer>& sbufs)
: cVector_(ni),
  seekIndex_()      /// Init seek index for random access to beginning of chunks
{
    //???  check if cvector already been written (can't write twice)

    /// Get CompressedArray's prototype node (all array elements must match this type)
    proto_ = cVector_->getPrototype();

    /// Check sbufs well formed (matches proto exactly)
    setBuffers(sbufs); //??? copy code here?

    /// Zero dataPacket_ at start
    memset(&dataPacket_, 0, sizeof(dataPacket_));

    /// For each individual sbuf, create an appropriate Encoder based on the cVector_ attributes
    for (unsigned i=0; i < sbufs_.size(); i++) {
        /// Create vector of single sbuf  ??? for now, may have groups later
        vector<SourceDestBuffer> vTemp;
        vTemp.push_back(sbufs_.at(i));

        ustring codecPath = sbufs_.at(i).pathName();

        /// Calc which stream the given path belongs to.  This depends on position of the node in the proto tree.
        shared_ptr<NodeImpl> readNode = proto_->get(sbufs.at(i).pathName());
        uint64_t bytestreamNumber = 0;
        if (!proto_->findTerminalPosition(readNode, bytestreamNumber))
            throw EXCEPTION("internal error");

        /// EncoderFactory picks the appropriate encoder to match type declared in prototype
        bytestreams_.push_back(Encoder::EncoderFactory(static_cast<unsigned>(bytestreamNumber), cVector_, vTemp, codecPath));
    }

    /// The bytestreams_ vector must be ordered by bytestreamNumber, not by order called specified sbufs, so sort it.
    sort(bytestreams_.begin(), bytestreams_.end(), SortByBytestreamNumber());
#ifdef E57_MAX_DEBUG
    /// Double check that all bytestreams are specified
    for (unsigned i=0; i < bytestreams_.size(); i++) {
        if (bytestreams_.at(i)->bytestreamNumber() != i)
            throw EXCEPTION("internal error");
    }
#endif

    shared_ptr<ImageFileImpl> imf(ni->fileParent_);

    /// Reserve space for CompressedVector binary section header, record location so can save to when writer closes.
    /// Request that file be extended with zeros since we will write to it at a later time (when writer closes).
    sectionHeaderLogicalStart_ = imf->allocateSpace(sizeof(CompressedVectorSectionHeader), true);

    isOpen_                 = true;
    sectionLogicalLength_   = 0;
    dataPhysicalOffset_     = 0;
    topIndexPhysicalOffset_ = 0;
    recordCount_            = 0;
    dataPacketsCount_       = 0;
    indexPacketsCount_      = 0;
}

CompressedVectorWriterImpl::~CompressedVectorWriterImpl()
{
#ifdef E57_MAX_VERBOSE
    cout << "~CompressedVectorWriterImpl() called" << endl; //???
#endif

    try {
        if (isOpen_)
            close();
    } catch (...) {
        //??? report?
    }

    /// Free allocated channels
    for (unsigned i=0; i < bytestreams_.size(); i++)
        delete bytestreams_.at(i);
}

void CompressedVectorWriterImpl::close()
{
#ifdef E57_MAX_VERBOSE
    cout << "CompressedVectorWriterImpl::close() called" << endl; //???
#endif
    if (!isOpen_)
        return;

    /// Set closed before do anything, so if get fault and start unwinding, don't try to close again.
    isOpen_ = false;

    /// If have any data, write packet
    /// Write all remaining ioBuffers and internal encoder register cache into file.
    /// Know we are done when totalOutputAvailable() returns 0 after a flush().
    flush();
    while (totalOutputAvailable() > 0) {
        packetWrite();
        flush();
    }

    shared_ptr<ImageFileImpl> imf(cVector_->fileParent_);

    /// Compute length of whole section we just wrote (from section start to current start of free space).
    sectionLogicalLength_ = imf->unusedLogicalStart_ - sectionHeaderLogicalStart_;
#ifdef E57_MAX_VERBOSE
    cout << "  sectionLogicalLength_=" << sectionLogicalLength_ << endl; //???
#endif

    /// Prepare CompressedVectorSectionHeader
    CompressedVectorSectionHeader header;
    header.sectionId            = E57_COMPRESSED_VECTOR_SECTION;
    header.sectionLogicalLength = sectionLogicalLength_;
    header.dataPhysicalOffset   = dataPhysicalOffset_;   ///??? can be zero, if no data written ???not set yet
    header.indexPhysicalOffset  = topIndexPhysicalOffset_;  ///??? can be zero, if no data written ???not set yet
#ifdef E57_MAX_VERBOSE
    cout << "  CompressedVectorSectionHeader:" << endl;
    header.dump(4); //???
#endif
    header.swab();  /// swab if neccesary

    /// Write header at beginning of section, previously allocated
    imf->file_->seek(sectionHeaderLogicalStart_);
    imf->file_->write(reinterpret_cast<char*>(&header), sizeof(header));

    /// Set address and size of associated CompressedVector
    cVector_->setRecordCount(recordCount_);
    cVector_->setBinarySectionLogicalStart(sectionHeaderLogicalStart_);

#ifdef E57_MAX_VERBOSE
    cout << "  CompressedVectorWriter:" << endl;
    dump(4);
#endif
}

void CompressedVectorWriterImpl::setBuffers(vector<SourceDestBuffer>& sbufs)
{
    ///??? handle empty sbufs?

    /// Check sbufs well formed: no dups, no missing, no extra  ???matching number of entries
    /// For writing, all data fields in prototype must be presented for writing at same time.
    proto_->checkBuffers(sbufs, false);

    sbufs_ = sbufs;
}

void CompressedVectorWriterImpl::write(vector<SourceDestBuffer>& sbufs, unsigned requestedElementCount)
{
    ///??? check exactly compatible with existing sbufs, if any

    if (!isOpen_)
        throw EXCEPTION("not open");

    setBuffers(sbufs);
    write(requestedElementCount);
}

void CompressedVectorWriterImpl::write(unsigned requestedElementCount)
{
#ifdef E57_MAX_VERBOSE
    cout << "CompressedVectorWriterImpl::write() called" << endl; //???
#endif

    if (!isOpen_)
        throw EXCEPTION("not open");

    /// Check that requestedElementCount is not larger than the sbufs
    if (requestedElementCount > sbufs_.at(0).impl()->capacity())
        throw EXCEPTION("requestedElementCount too large");

    /// Rewind all sbufs so start reading from beginning
    for (unsigned i=0; i < sbufs_.size(); i++)
        sbufs_.at(i).impl()->rewind();

    /// Loop until all channels have completed requestedElementCount transfers
    uint64_t endRecordIndex = recordCount_ + requestedElementCount;
    for (;;) {
        /// Calc remaining record counts for all channels
        uint64_t totalElementCount = 0;
        for (unsigned i=0; i < bytestreams_.size(); i++)
            totalElementCount += endRecordIndex - bytestreams_.at(i)->currentRecordIndex();
#ifdef E57_MAX_VERBOSE
        cout << "  totalElementCount=" << totalElementCount << endl; //???
#endif

        /// We are done if have no more work, break out of loop
        if (totalElementCount == 0)
            break;

        /// Estimate how many records can write before have enough data to fill data packet to efficient length
        /// Efficient packet length is >= 75% of maximum packet length.
        /// It is OK if get too much data (more than one packet) in an iteration.
        /// Reader will be able to handle packets whose streams are not exactly synchronized to the record boundaries.
        /// But try to do a good job of keeping the stream synchronization "close enough" (so a reader that can cache only two packets is efficient).

#ifdef E57_MAX_VERBOSE
        cout << "  currentPacketSize()=" << currentPacketSize() << endl; //???
#endif

#if E57_WRITE_CRAZY_PACKET_MODE
///??? depends on number of streams
#  define E57_TARGET_PACKET_SIZE    500
#else
#  define E57_TARGET_PACKET_SIZE    (E57_DATA_PACKET_MAX*3/4)
#endif
        /// If have more than target fraction of packet, send it now
        if (currentPacketSize() >= E57_TARGET_PACKET_SIZE) {  //???
            packetWrite();
            continue;  /// restart loop so recalc statistics (packet size may not be zero after write, if have too much data)
        }

        ///??? useful?
        /// Get approximation of number of bytes per record of CompressedVector and total of bytes used
        float totalBitsPerRecord = 0;  // an estimate of future performance
        for (unsigned i=0; i < bytestreams_.size(); i++)
            totalBitsPerRecord += bytestreams_.at(i)->bitsPerRecord();
        float totalBytesPerRecord = max(totalBitsPerRecord/8, 0.1F); //??? trust

#ifdef E57_MAX_VERBOSE
        cout << "  totalBytesPerRecord=" << totalBytesPerRecord << endl; //???
#endif


//!!!        unsigned spaceRemaining = E57_DATA_PACKET_MAX - currentPacketSize();
//!!!        unsigned appoxRecordsNeeded = static_cast<unsigned>(floor(spaceRemaining / totalBytesPerRecord)); //??? divide by zero if all constants


        /// Don't allow straggler to get too far behind. ???
        /// Don't allow a single channel to get too far ahead ???
        /// Process channels that are furthest behind first. ???

        ///!!!! For now just process one record per loop until packet is full enough, or completed request
        for (unsigned i=0; i < bytestreams_.size(); i++) {
             if (bytestreams_.at(i)->currentRecordIndex() < endRecordIndex) {
#if 0
                bytestreams_.at(i)->processRecords(1);
#else
//!!! for now, process upto 50 records at a time
                uint64_t recordCount = endRecordIndex - bytestreams_.at(i)->currentRecordIndex();
                recordCount = min(recordCount, 50ULL);
                bytestreams_.at(i)->processRecords((unsigned)recordCount);
#endif
            }
        }
    }

    recordCount_ += requestedElementCount;

    /// When we leave this function, will likely still have data in channel ioBuffers as well as partial words in Encoder registers.
}

unsigned CompressedVectorWriterImpl::totalOutputAvailable()
{
    unsigned total = 0;
    for (unsigned i=0; i < bytestreams_.size(); i++) {
        total += bytestreams_.at(i)->outputAvailable();
    }
    return(total);
}

unsigned CompressedVectorWriterImpl::currentPacketSize()
{
    /// Calc current packet size
    return(sizeof(DataPacketHeader) + bytestreams_.size()*sizeof(uint16_t) + totalOutputAvailable());
}

uint64_t CompressedVectorWriterImpl::packetWrite()
{
#ifdef E57_MAX_VERBOSE
    cout << "CompressedVectorWriterImpl::packetWrite() called" << endl; //???
#endif

    /// Double check that we have work to do
    unsigned totalOutput = totalOutputAvailable();
    if (totalOutput == 0)
        return(0);
#ifdef E57_MAX_VERBOSE
    cout << "  totalOutput=" << totalOutput << endl; //???
#endif

    /// Calc maximum number of bytestream values can put in data packet.
    unsigned packetMaxPayloadBytes = E57_DATA_PACKET_MAX - sizeof(DataPacketHeader) - bytestreams_.size()*sizeof(uint16_t);
#ifdef E57_MAX_VERBOSE
    cout << "  packetMaxPayloadBytes=" << packetMaxPayloadBytes << endl; //???
#endif

    /// Allocate vector for number of bytes that each bytestream will write to file.
    vector<unsigned> count(bytestreams_.size());

    /// See if we can fit into a single data packet
    if (totalOutput < packetMaxPayloadBytes) {
        /// We can fit everything in one packet
        for (unsigned i=0; i < bytestreams_.size(); i++)
            count.at(i) = bytestreams_.at(i)->outputAvailable();
    } else {
        /// We have too much data for one packet.  Send proportional amounts from each bytestream.
        /// Adjust packetMaxPayloadBytes down by one so have a little slack for floating point weirdness.
        float fractionToSend =  (packetMaxPayloadBytes-1) / static_cast<float>(totalOutput);
        for (unsigned i=0; i < bytestreams_.size(); i++) {
            /// Round down here so sum <= packetMaxPayloadBytes
            count.at(i) = static_cast<unsigned>(floor(fractionToSend * bytestreams_.at(i)->outputAvailable()));
        }
    }
#ifdef E57_MAX_VERBOSE
    for (unsigned i=0; i < bytestreams_.size(); i++)
        cout << "  count[" << i << "]=" << count.at(i) << endl; //???
#endif

#ifdef E57_DEBUG
    /// Double check sum of count is <= packetMaxPayloadBytes
    unsigned totalByteCount = 0;
    for (unsigned i=0; i < count.size(); i++)
        totalByteCount += count.at(i);
    if (totalByteCount > packetMaxPayloadBytes)
        throw EXCEPTION("internal error");
#endif

    /// Get smart pointer to ImageFileImpl from associated CompressedVector
    shared_ptr<ImageFileImpl> imf(cVector_->fileParent_);

    /// Use temp buf in object (is 64KBytes long) instead of allocating each time here
    char* packet = reinterpret_cast<char*>(&dataPacket_);
#ifdef E57_MAX_VERBOSE
    cout << "  packet=" << (unsigned)packet << endl; //???
#endif

    /// To be safe, clear header part of packet
    memset(packet, 0, sizeof(DataPacketHeader));

    /// Write bytestreamBufferLength[bytestreamCount] after header, in dataPacket_
    uint16_t* bsbLength = reinterpret_cast<uint16_t*>(&packet[sizeof(DataPacketHeader)]);
#ifdef E57_MAX_VERBOSE
    cout << "  bsbLength=" << (unsigned)bsbLength << endl; //???
#endif
    for (unsigned i=0; i < bytestreams_.size(); i++) {
        bsbLength[i] = count.at(i);
#ifdef E57_MAX_VERBOSE
        cout << "  Writing " << bsbLength[i] << " bytes into bytestream " << i << endl; //???
#endif
    }

    /// Get pointer to end of data so far
    char* p = reinterpret_cast<char*>(&bsbLength[bytestreams_.size()]);
#ifdef E57_MAX_VERBOSE
    cout << "  after bsbLength, p=" << (unsigned)p << endl; //???
#endif

    /// Write contents of each bytestream in dataPacket_
    for (unsigned i=0; i < bytestreams_.size(); i++) {
        unsigned n = count.at(i);

#ifdef E57_DEBUG
        /// Double check we aren't accidentally going to write off end of vector<char>
        if (&p[n] > &packet[E57_DATA_PACKET_MAX])
            throw EXCEPTION("internal error");
#endif

        /// Read from encoder output into packet
        bytestreams_.at(i)->outputRead(p, n);

        /// Move pointer to end of current data
        p += n;
    }

    /// Length of packet is difference in beginning pointer and ending pointer
    unsigned packetLength = static_cast<unsigned>(p - packet);  ///??? pointer diff portable?
#ifdef E57_MAX_VERBOSE
    cout << "  packetLength=" << packetLength << endl; //???
#endif

#ifdef E57_DEBUG
    /// Double check that packetLength is what we expect
    if (packetLength != sizeof(DataPacketHeader) + bytestreams_.size()*sizeof(uint16_t) + totalByteCount)
        throw EXCEPTION("internal error");
#endif

    /// packetLength must be multiple of 4, if not, add some zero padding
    while (packetLength % 4) {
        /// Double check we aren't accidentally going to write off end of vector<char>
        if (p >= &packet[E57_DATA_PACKET_MAX-1])
            throw EXCEPTION("internal error");
        *p++ = 0;
        packetLength++;
#ifdef E57_MAX_VERBOSE
        cout << "  padding with zero byte, new packetLength=" << packetLength << endl; //???
#endif
    }

    /// Prepare header in dataPacket_, now that we are sure of packetLength
    dataPacket_.packetType = E57_DATA_PACKET;
    dataPacket_.packetFlags = 0;
    dataPacket_.packetLogicalLengthMinus1 = static_cast<uint16_t>(packetLength-1);
    dataPacket_.bytestreamCount = bytestreams_.size();

    /// Double check that data packet is well formed
    dataPacket_.verify(packetLength);

#ifdef E57_BIGENDIAN
    /// On bigendian CPUs, swab packet to little-endian byte order before writing.
    dataPacket_.swab(true);
#endif

    /// Write whole data packet at beginning of free space in file
    uint64_t packetLogicalOffset = imf->allocateSpace(packetLength, false);
    uint64_t packetPhysicalOffset = imf->file_->logicalToPhysical(packetLogicalOffset);
    imf->file_->seek(packetLogicalOffset);  //??? have seekLogical and seekPhysical instead? more explicit
    imf->file_->write(packet, packetLength);

//cout << "data packet:" << endl; //!!!!
//dataPacket_.dump(4); //!!!!

    /// If first data packet written for this CompressedVector binary section, save address to put in section header
    ///??? what if no data packets?
    ///??? what if have exceptions while write, what is state of file?  will close report file good/bad?
    if (dataPacketsCount_ == 0)
        dataPhysicalOffset_ = packetPhysicalOffset;
    dataPacketsCount_++;

    ///!!! update seekIndex here? if started new chunk?

    /// Return physical offset of data packet for potential use in seekIndex
    return(packetPhysicalOffset); //??? needed
}

void CompressedVectorWriterImpl::flush()
{
    for (unsigned i=0; i < bytestreams_.size(); i++)
        bytestreams_.at(i)->registerFlushToOutput();
}

void CompressedVectorWriterImpl::dump(int indent, std::ostream& os)
{
    for (unsigned i = 0; i < sbufs_.size(); i++) {
        os << space(indent) << "sbufs[" << i << "]:" << endl;
        sbufs_.at(i).dump(indent+4, os);
    }

    os << space(indent) << "cVector:" << endl;
    cVector_->dump(indent+4, os);

    os << space(indent) << "proto:" << endl;
    proto_->dump(indent+4, os);

    for (unsigned i = 0; i < bytestreams_.size(); i++) {
        os << space(indent) << "bytestreams[" << i << "]:" << endl;
        bytestreams_.at(i)->dump(indent+4, os);
    }

    os << space(indent) << "seekIndex:" << endl;
    seekIndex_.dump(indent+4, os);

    /// Don't call dump() for DataPacket, since it may contain junk when debugging.  Just print a few byte values.
    os << space(indent) << "dataPacket:" << endl;
    uint8_t* p = reinterpret_cast<uint8_t*>(&dataPacket_);
    unsigned i;
    for (i = 0; i < 40; i++)
        os << space(indent+4) << "dataPacket[" << i << "]: " << (unsigned)p[i] << endl;
    os << space(indent+4) << "more unprinted..." << endl;

    os << space(indent) << "sectionHeaderLogicalStart: " << sectionHeaderLogicalStart_ << endl;
    os << space(indent) << "sectionLogicalLength:      " << sectionLogicalLength_ << endl;
    os << space(indent) << "dataPhysicalOffset:        " << dataPhysicalOffset_ << endl;
    os << space(indent) << "topIndexPhysicalOffset:    " << topIndexPhysicalOffset_ << endl;
    os << space(indent) << "recordCount:               " << recordCount_ << endl;
    os << space(indent) << "dataPacketsCount:          " << dataPacketsCount_ << endl;
    os << space(indent) << "indexPacketsCount:         " << indexPacketsCount_ << endl;
}

///================================================================
///================================================================
///================================================================

CompressedVectorReaderImpl::CompressedVectorReaderImpl(shared_ptr<CompressedVectorNodeImpl> cvi, vector<SourceDestBuffer>& dbufs)
: cVector_(cvi)
{
#ifdef E57_MAX_VERBOSE
    cout << "CompressedVectorReaderImpl() called" << endl; //???
#endif

    /// Allow reading of a completed CompressedVector, whether file is being read or currently being written.
    ///??? what other situations need checking for?
    ///??? check if CV not yet written to?
    ///??? file in error state?

    /// Empty dbufs is an error
    if (dbufs.size() == 0)
        throw EXCEPTION("no destination buffers specified");

    /// Get CompressedArray's prototype node (all array elements must match this type)
    proto_ = cVector_->getPrototype();

    /// Check dbufs well formed (matches proto exactly)
    setBuffers(dbufs);

    /// For each dbuf, create an appropriate Decoder based on the cVector_ attributes
    for (unsigned i=0; i < dbufs_.size(); i++) {
        vector<SourceDestBuffer> theDbuf;
        theDbuf.push_back(dbufs.at(i));

        Decoder* decoder =  Decoder::DecoderFactory(i, cVector_, theDbuf, ustring());

        /// Calc which stream the given path belongs to.  This depends on position of the node in the proto tree.
        shared_ptr<NodeImpl> readNode = proto_->get(dbufs.at(i).pathName());
        uint64_t bytestreamNumber = 0;
        if (!proto_->findTerminalPosition(readNode, bytestreamNumber))
            throw EXCEPTION("internal error");

        channels_.push_back(DecodeChannel(dbufs.at(i), decoder, static_cast<unsigned>(bytestreamNumber), cVector_->childCount()));
    }

    recordCount_ = 0;

    /// Get how many records are actually defined
    maxRecordCount_ = cvi->childCount();

    shared_ptr<ImageFileImpl> imf(cVector_->fileParent_);

    //??? what if fault in this constructor?
    cache_ = new PacketReadCache(imf->file_, 4/*???*/);

    /// Read CompressedVector section header
    CompressedVectorSectionHeader sectionHeader;
    uint64_t sectionLogicalStart = cVector_->getBinarySectionLogicalStart();
    if (sectionLogicalStart == 0)
        throw EXCEPTION("internal error"); //??? should have caught this before got here, in XML read, get this if CV wasn't written to by writer.
    imf->file_->seek(sectionLogicalStart, CheckedFile::logical);
    imf->file_->read(reinterpret_cast<char*>(&sectionHeader), sizeof(sectionHeader));
    sectionHeader.swab();  /// swab if neccesary

    /// Pre-calc end of section, so can tell when we are out of packets.
    sectionEndLogicalOffset_ = sectionLogicalStart + sectionHeader.sectionLogicalLength;

    /// Convert physical offset to first data packet to logical
    uint64_t dataLogicalOffset = imf->file_->physicalToLogical(sectionHeader.dataPhysicalOffset);

    /// Verify that packet given by dataPhysicalOffset is actually a data packet, init channels
    {
        char* anyPacket = NULL;
        auto_ptr<PacketLock> packetLock = cache_->lock(dataLogicalOffset, anyPacket);

        DataPacket* dpkt = reinterpret_cast<DataPacket*>(anyPacket);

        /// Double check that have a data packet
        if (dpkt->packetType != E57_DATA_PACKET)
            throw EXCEPTION("bad first data packet");

        /// Have good packet, initialize channels
        for (unsigned i = 0; i < channels_.size(); i++) {
            DecodeChannel* chan = &channels_.at(i);
            chan->currentPacketLogicalOffset    = dataLogicalOffset;
            chan->currentBytestreamBufferIndex  = 0;
            chan->currentBytestreamBufferLength = dpkt->getBytestreamBufferLength(chan->bytestreamNumber);
        }
    }
}

CompressedVectorReaderImpl::~CompressedVectorReaderImpl()
{
#ifdef E57_MAX_VERBOSE
    cout << "~CompressedVectorReaderImpl() called" << endl; //???
#endif
#ifdef E57_MAX_VERBOSE
    //cout << "CompressedVectorWriterImpl:" << endl;
    //dump(4);
#endif
    /// Free allocated decoders
    for (unsigned i=0; i < channels_.size(); i++)
        delete channels_.at(i).decoder;

    delete cache_;
}

void CompressedVectorReaderImpl::setBuffers(vector<SourceDestBuffer>& dbufs)
{
    ///??? handle empty dbufs?

    /// Check dbufs well formed: no dups, no extra, missing is ok
    proto_->checkBuffers(dbufs, true);

    dbufs_ = dbufs;
}

unsigned CompressedVectorReaderImpl::read(vector<SourceDestBuffer>& dbufs)
{
    ///??? check exactly compatible with existing dbufs, if any

    setBuffers(dbufs);
    return(read());
}

unsigned CompressedVectorReaderImpl::read()
{
#ifdef E57_MAX_VERBOSE
    cout << "CompressedVectorReaderImpl::read() called" << endl; //???
#endif

    /// Rewind all dbufs so start writing to them at beginning
    for (unsigned i=0; i < dbufs_.size(); i++)
        dbufs_[i].impl()->rewind();

    /// Allow decoders to use data they already have in their queue to fill newly empty dbufs
    /// This helps to keep decoder input queues smaller, which reduces backtracking in the packet cache.
    for (unsigned i = 0; i < channels_.size(); i++)
        channels_[i].decoder->inputProcess(NULL, 0);

    /// Loop until every dbuf is full or we have reached end of the binary section.
    while (1) {
        /// Find the earliest packet position for channels that are still hungry
        /// It's important to call inputProcess of the decoders before this call, so current hungriness level is reflected.
        uint64_t earliestPacketLogicalOffset = earliestPacketNeededForInput();

        /// If nobody's hungry, we are done with the read
        if (earliestPacketLogicalOffset == UINT64_MAX)
            break;

        /// Feed packet to the hungry decoders
        feedPacketToDecoders(earliestPacketLogicalOffset);
    }

    /// Verify that each channel produced the same number of records
    unsigned outputCount = 0;
    for (unsigned i = 0; i < channels_.size(); i++) {
        DecodeChannel* chan = &channels_[i];
        if (i == 0)
            outputCount = chan->dbuf.impl()->nextIndex();
        else {
            if (outputCount != chan->dbuf.impl()->nextIndex())
                throw EXCEPTION("internal error");
        }
    }

    /// Return number of records transferred to each dbuf.
    return(outputCount);
}

uint64_t CompressedVectorReaderImpl::earliestPacketNeededForInput()
{
    uint64_t earliestPacketLogicalOffset = UINT64_MAX;
    unsigned earliestChannel = 0;
    for (unsigned i = 0; i < channels_.size(); i++) {
        DecodeChannel* chan = &channels_[i];

        /// Test if channel needs more input.
        /// Important to call inputProcess just before this, so these tests work.
        if (!chan->isOutputBlocked() && !chan->inputFinished) {
            /// Check if earliest so far
            if (chan->currentPacketLogicalOffset < earliestPacketLogicalOffset) {
                earliestPacketLogicalOffset = chan->currentPacketLogicalOffset;
                earliestChannel = i;
            }
        }
    }
#ifdef E57_MAX_VERBOSE
    if (earliestPacketLogicalOffset == UINT64_MAX)
        cout << "earliestPacketNeededForInput returning none found" << endl;
    else
        cout << "earliestPacketNeededForInput returning " << earliestPacketLogicalOffset << " for channel[" << earliestChannel << "]" << endl;
#endif
    return(earliestPacketLogicalOffset);
}

void CompressedVectorReaderImpl::feedPacketToDecoders(uint64_t currentPacketLogicalOffset)
{
    /// Read earliest packet into cache and send data to decoders with unblocked output
    bool channelHasExhaustedPacket = false;
    uint64_t nextPacketLogicalOffset = UINT64_MAX;
    {
        /// Get packet at currentPacketLogicalOffset into memory.
        char* anyPacket = NULL;
        auto_ptr<PacketLock> packetLock = cache_->lock(currentPacketLogicalOffset, anyPacket);
        DataPacket* dpkt = reinterpret_cast<DataPacket*>(anyPacket);

        /// Double check that have a data packet.  Should have already determined this.
        if (dpkt->packetType != E57_DATA_PACKET)
            throw EXCEPTION("internal error");//!!!

        /// Feed bytestreams to channels with unblocked output that are reading from this packet
        for (unsigned i = 0; i < channels_.size(); i++) {
            DecodeChannel* chan = &channels_[i];

            /// Skip channels that have already read this packet.
            if (chan->currentPacketLogicalOffset != currentPacketLogicalOffset || chan->isOutputBlocked())
                continue;

            /// Get bytestream buffer for this channel from packet
            unsigned bsbLength;
            char* bsbStart = dpkt->getBytestream(chan->bytestreamNumber, bsbLength);

            /// Calc where we are in the buffer
            char* uneatenStart = &bsbStart[chan->currentBytestreamBufferIndex];
            unsigned uneatenLength = bsbLength - chan->currentBytestreamBufferIndex;

            /// Double check we are not off end of buffer
            if (chan->currentBytestreamBufferIndex > bsbLength)
                throw EXCEPTION("internal error");
            if (&uneatenStart[uneatenLength] > &bsbStart[bsbLength])
                throw EXCEPTION("internal error");
#ifdef E57_VERBOSE
            cout << "  stream[" << chan->bytestreamNumber << "]: feeding decoder " << uneatenLength << " bytes" << endl;
            if (uneatenLength == 0)
                chan->dump(8); //!!!
#endif
            /// Feed into decoder
            unsigned bytesProcessed = chan->decoder->inputProcess(uneatenStart, uneatenLength);
#ifdef E57_VERBOSE
            cout << "  stream[" << chan->bytestreamNumber << "]: bytesProcessed=" << bytesProcessed << endl;
#endif
            /// Adjust counts of bytestream location
            chan->currentBytestreamBufferIndex += bytesProcessed;

            /// Check if this channel has exhausted its bytestream buffer in this packet
            if (chan->isInputBlocked()) {
#ifdef E57_VERBOSE
                cout << "  stream[" << chan->bytestreamNumber << "] has exhausted its input in current packet" << endl;
#endif
                channelHasExhaustedPacket = true;
                nextPacketLogicalOffset = currentPacketLogicalOffset + dpkt->packetLogicalLengthMinus1 + 1;
            }
        }

    }

    /// Skip over any index or empty packets to next data packet.
    nextPacketLogicalOffset = findNextDataPacket(nextPacketLogicalOffset);

    /// If some channel has exhausted this packet, find next data packet and update currentPacketLogicalOffset for all interested channels.
    if (channelHasExhaustedPacket) {
        if (nextPacketLogicalOffset < UINT64_MAX) {
            /// Get packet at nextPacketLogicalOffset into memory.
            char* anyPacket = NULL;
            auto_ptr<PacketLock> packetLock = cache_->lock(nextPacketLogicalOffset, anyPacket);
            DataPacket* dpkt = reinterpret_cast<DataPacket*>(anyPacket);

            /// Got a data packet, update the channels with exhausted input
            for (unsigned i = 0; i < channels_.size(); i++) {
                DecodeChannel* chan = &channels_[i];
                if (chan->currentPacketLogicalOffset == currentPacketLogicalOffset && chan->isInputBlocked()) {
                    chan->currentPacketLogicalOffset = nextPacketLogicalOffset;
                    chan->currentBytestreamBufferIndex = 0;

                    /// It is OK if the next packet doesn't contain any data for this channel, will skip packet on next iter of loop
                    chan->currentBytestreamBufferLength = dpkt->getBytestreamBufferLength(chan->bytestreamNumber);

#ifdef E57_MAX_VERBOSE
                    cout << "  set new stream buffer for channel[" << i << "], length=" << chan->currentBytestreamBufferLength << endl;
#endif
                    /// ??? perform flush if new packet flag set?
                }
            }
        } else {
            /// Reached end without finding data packet, mark exhausted channels as finished
#ifdef E57_MAX_VERBOSE
            cout << "  at end of data packets" << endl;
#endif
            if (nextPacketLogicalOffset >= sectionEndLogicalOffset_) {
                for (unsigned i = 0; i < channels_.size(); i++) {
                    DecodeChannel* chan = &channels_[i];
                    if (chan->currentPacketLogicalOffset == currentPacketLogicalOffset && chan->isInputBlocked()) {
#ifdef E57_MAX_VERBOSE
                        cout << "  Marking channel[" << i << "] as finished" << endl;
#endif
                        chan->inputFinished = true;
                    }
                }
            }
        }
    }
}

uint64_t CompressedVectorReaderImpl::findNextDataPacket(uint64_t nextPacketLogicalOffset)
{
#ifdef E57_MAX_VERBOSE
    cout << "  searching for next data packet, nextPacketLogicalOffset=" << nextPacketLogicalOffset
         << " sectionEndLogicalOffset=" << sectionEndLogicalOffset_ << endl;
#endif

    /// Starting at nextPacketLogicalOffset, search for next data packet until hit end of binary section.
    while (nextPacketLogicalOffset < sectionEndLogicalOffset_) {
        char* anyPacket = NULL;
        auto_ptr<PacketLock> packetLock = cache_->lock(nextPacketLogicalOffset, anyPacket);

        /// Guess it's a data packet, if not continue to next packet
        DataPacket* dpkt = reinterpret_cast<DataPacket*>(anyPacket);
        if (dpkt->packetType == E57_DATA_PACKET) {
#ifdef E57_MAX_VERBOSE
            cout << "  Found next data packet at nextPacketLogicalOffset=" << nextPacketLogicalOffset << endl;
#endif
            return(nextPacketLogicalOffset);
        }

        /// All packets have length in same place, so can use the field to skip to next packet.
        nextPacketLogicalOffset += dpkt->packetLogicalLengthMinus1 + 1;
    }

    /// Ran off end of section, so return failure code.
    return(UINT64_MAX);
}

void CompressedVectorReaderImpl::seek(uint64_t recordNumber)
{
    ///!!! implement
    throw EXCEPTION("not implemented yet");
}

void CompressedVectorReaderImpl::close()
{
    ///!!! implement
    throw EXCEPTION("not implemented yet");
}

void CompressedVectorReaderImpl::dump(int indent, std::ostream& os)
{
    for (unsigned i = 0; i < dbufs_.size(); i++) {
        os << space(indent) << "dbufs[" << i << "]:" << endl;
        dbufs_[i].dump(indent+4, os);
    }

    os << space(indent) << "cVector:" << endl;
    cVector_->dump(indent+4, os);

    os << space(indent) << "proto:" << endl;
    proto_->dump(indent+4, os);

    for (unsigned i = 0; i < channels_.size(); i++) {
        os << space(indent) << "channels[" << i << "]:" << endl;
        channels_[i].dump(indent+4, os);
    }

    os << space(indent) << "recordCount:             " << recordCount_ << endl;
    os << space(indent) << "maxRecordCount:          " << maxRecordCount_ << endl;
    os << space(indent) << "sectionEndLogicalOffset: " << sectionEndLogicalOffset_ << endl;
}

//================================================================
//================================================================
//================================================================

Encoder* Encoder::EncoderFactory(unsigned bytestreamNumber,
                                 shared_ptr<CompressedVectorNodeImpl> cVector,
                                 vector<SourceDestBuffer>& sbufs,
                                 ustring& codecPath)
{
    //??? For now, only handle one input
    if (sbufs.size() != 1)
        throw EXCEPTION("internal error");
    SourceDestBuffer sbuf = sbufs.at(0);

    /// Get node we are going to encode from the CompressedVector's prototype
    shared_ptr<NodeImpl> prototype = cVector->getPrototype();
    ustring path = sbuf.pathName();
    shared_ptr<NodeImpl> encodeNode = prototype->get(path);

#ifdef E57_MAX_VERBOSE
    cout << "Node to encode:" << endl; //???
    encodeNode->dump(2);
#endif
    switch (encodeNode->type()) {
        case E57_INTEGER: {
            shared_ptr<IntegerNodeImpl> ini = dynamic_pointer_cast<IntegerNodeImpl>(encodeNode);  // downcast to correct type

            /// Get pointer to parent ImageFileImpl, to call bitsNeeded()
            shared_ptr<ImageFileImpl> imf(encodeNode->fileParent_);  //??? should be function for this,  imf->parentFile()  --> ImageFile?

            unsigned bitsPerRecord = imf->bitsNeeded(ini->minimum(), ini->maximum());

            //!!! need to pick smarter channel buffer sizes, here and elsewhere
            /// Constuct Integer encoder with appropriate register size, based on number of bits stored.
            if (bitsPerRecord == 0) {
                return(new ConstantIntegerEncoder(bytestreamNumber, sbuf, ini->minimum()));
            } else if (bitsPerRecord <= 8) {
                return(new BitpackIntegerEncoder<uint8_t>(false, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                          ini->minimum(), ini->maximum(), 1.0, 0.0));
            } else if (bitsPerRecord <= 16) {
                return(new BitpackIntegerEncoder<uint16_t>(false, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                           ini->minimum(), ini->maximum(), 1.0, 0.0));
            } else if (bitsPerRecord <= 32) {
                return(new BitpackIntegerEncoder<uint32_t>(false, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                           ini->minimum(), ini->maximum(), 1.0, 0.0));
            } else {
                return(new BitpackIntegerEncoder<uint64_t>(false, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                           ini->minimum(), ini->maximum(), 1.0, 0.0));
            }
        }
        case E57_SCALED_INTEGER: {
            shared_ptr<ScaledIntegerNodeImpl> sini = dynamic_pointer_cast<ScaledIntegerNodeImpl>(encodeNode);  // downcast to correct type

            /// Get pointer to parent ImageFileImpl, to call bitsNeeded()
            shared_ptr<ImageFileImpl> imf(encodeNode->fileParent_);  //??? should be function for this,  imf->parentFile()  --> ImageFile?

            unsigned bitsPerRecord = imf->bitsNeeded(sini->minimum(), sini->maximum());

            //!!! need to pick smarter channel buffer sizes, here and elsewhere
            /// Constuct ScaledInteger encoder with appropriate register size, based on number of bits stored.
            if (bitsPerRecord == 0) {
                return(new ConstantIntegerEncoder(bytestreamNumber, sbuf, sini->minimum()));
            } else if (bitsPerRecord <= 8) {
                return(new BitpackIntegerEncoder<uint8_t>(true, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                          sini->minimum(), sini->maximum(), sini->scale(), sini->offset()));
            } else if (bitsPerRecord <= 16) {
                return(new BitpackIntegerEncoder<uint16_t>(true, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                           sini->minimum(), sini->maximum(), sini->scale(), sini->offset()));
            } else if (bitsPerRecord <= 32) {
                return(new BitpackIntegerEncoder<uint32_t>(true, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                           sini->minimum(), sini->maximum(), sini->scale(), sini->offset()));
            } else {
                return(new BitpackIntegerEncoder<uint64_t>(true, bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/,
                                                           sini->minimum(), sini->maximum(), sini->scale(), sini->offset()));
            }
        }
        case E57_FLOAT: {
            shared_ptr<FloatNodeImpl> fni = dynamic_pointer_cast<FloatNodeImpl>(encodeNode);  // downcast to correct type

            //!!! need to pick smarter channel buffer sizes, here and elsewhere
            return(new BitpackFloatEncoder(bytestreamNumber, sbuf, E57_DATA_PACKET_MAX/*!!!*/, fni->precision()));
        }
        case E57_STRING: {
            throw EXCEPTION("not implemented yet"); //???
        }
        default:
            throw EXCEPTION("bad type to encode");
    }
}

Encoder::Encoder(unsigned bytestreamNumber)
: bytestreamNumber_(bytestreamNumber)
{}

#ifdef E57_DEBUG
void Encoder::dump(int indent, std::ostream& os)
{
    os << space(indent) << "bytestreamNumber:       " << bytestreamNumber_ << endl;
}
#endif

///================

BitpackEncoder::BitpackEncoder(unsigned bytestreamNumber, SourceDestBuffer& sbuf, unsigned outputMaxSize, unsigned alignmentSize)
: Encoder(bytestreamNumber),
  sourceBuffer_(sbuf.impl()),
  outBuffer_(outputMaxSize),
  outBufferFirst_(0),
  outBufferEnd_(0),
  outBufferAlignmentSize_(alignmentSize),
  currentRecordIndex_(0)
{
}

unsigned BitpackEncoder::sourceBufferNextIndex()
{
    return(sourceBuffer_->nextIndex());
}

uint64_t BitpackEncoder::currentRecordIndex()
{
    return(currentRecordIndex_);
}

unsigned BitpackEncoder::outputAvailable()
{
    return(outBufferEnd_ - outBufferFirst_);
}

void BitpackEncoder::outputRead(char* dest, unsigned byteCount)
{
#ifdef E57_MAX_VERBOSE
    cout << "BitpackEncoder::outputRead() called, dest=" << (unsigned)dest << " byteCount="<< byteCount << endl; //???
#endif

    /// Check we have enough bytes in queue
    if (byteCount > outputAvailable())
        throw EXCEPTION("output underflow");

    /// Copy output bytes to caller
    memcpy(dest, &outBuffer_[outBufferFirst_], byteCount);

#ifdef E57_MAX_VERBOSE
    {
        unsigned i;
        for (i=0; i < byteCount && i < 20; i++)
            cout << "  outBuffer[" << outBufferFirst_+i << "]=" << static_cast<unsigned>(static_cast<unsigned char>(outBuffer_[outBufferFirst_+i])) << endl; //???
        if (i < byteCount)
            cout << "  " << byteCount-1 << " bytes unprinted..." << endl;
    }
#endif

    /// Advance head pointer.
    outBufferFirst_ += byteCount;

    /// Don't slide remaining data down now, wait until do some more processing (that's when data needs to be aligned).
}

void BitpackEncoder::outputClear()
{
    outBufferFirst_     = 0;
    outBufferEnd_       = 0;
}

void BitpackEncoder::sourceBufferSetNew(std::vector<SourceDestBuffer>& sbufs)
{
    /// Verify that this encoder only has single input buffer
    if (sbufs.size() != 1)
        throw EXCEPTION("internal error");

    sourceBuffer_ = sbufs.at(0).impl();
}

unsigned BitpackEncoder::outputGetMaxSize()
{
    /// Its size that matters here, not capacity
    return(outBuffer_.size());
}

void BitpackEncoder::outputSetMaxSize(unsigned byteCount)
{
    /// Ignore if trying to shrink buffer (queue might get messed up).
    if (byteCount > outBuffer_.size())
        outBuffer_.resize(byteCount);
}

void BitpackEncoder::outBufferShiftDown()
{
    /// Move data down closer to beginning of outBuffer_.
    /// But keep outBufferEnd_ as a multiple of outBufferAlignmentSize_.
    /// This ensures that writes into buffer can occur on natural boundaries.
    /// Otherwise some CPUs will fault.

    if (outBufferFirst_ == outBufferEnd_) {
        /// Buffer is empty, reset indices to 0
        outBufferFirst_ = 0;
        outBufferEnd_   = 0;
        return;
    }

    /// Round newEnd up to nearest multiple of outBufferAlignmentSize_.
    unsigned newEnd = outputAvailable();
    unsigned remainder = newEnd % outBufferAlignmentSize_;
    if (remainder > 0)
        newEnd += outBufferAlignmentSize_ - remainder;
    unsigned newFirst = outBufferFirst_ - (outBufferEnd_ - newEnd);
    unsigned byteCount = outBufferEnd_ - outBufferFirst_;

    /// Double check round up worked
    if (newEnd % outBufferAlignmentSize_)
        throw EXCEPTION("internal error");

    /// Be paranoid before memory copy
    if (newFirst+byteCount > outBuffer_.size())
        throw EXCEPTION("internal error");

    /// Move available data down closer to beginning of outBuffer_.  Overlapping regions ok with memmove().
    memmove(&outBuffer_[newFirst], &outBuffer_[outBufferFirst_], byteCount);

    /// Update indexes
    outBufferFirst_ = newFirst;
    outBufferEnd_  = newEnd;
}

#ifdef E57_DEBUG
void BitpackEncoder::dump(int indent, std::ostream& os)
{
    Encoder::dump(indent, os);
    os << space(indent) << "sourceBuffer:" << endl;
    sourceBuffer_->dump(indent+4, os);
    os << space(indent) << "outBuffer.size:           " << outBuffer_.size() << endl;
    os << space(indent) << "outBufferFirst:           " << outBufferFirst_ << endl;
    os << space(indent) << "outBufferEnd:             " << outBufferEnd_ << endl;
    os << space(indent) << "outBufferAlignmentSize:   " << outBufferAlignmentSize_ << endl;
    os << space(indent) << "currentRecordIndex:       " << currentRecordIndex_ << endl;
    os << space(indent) << "outBuffer:" << endl;
    unsigned i;
    for (i = 0; i < outBuffer_.size() && i < 20; i++)
        os << space(indent+4) << "outBuffer[" << i << "]: " << static_cast<unsigned>(static_cast<unsigned char>(outBuffer_.at(i))) << endl;
    if (i < outBuffer_.size())
        os << space(indent+4) << outBuffer_.size()-i << " more unprinted..." << endl;
}
#endif

//================

BitpackFloatEncoder::BitpackFloatEncoder(unsigned bytestreamNumber, SourceDestBuffer& sbuf, unsigned outputMaxSize, FloatPrecision precision)
: BitpackEncoder(bytestreamNumber, sbuf, outputMaxSize, (precision==E57_SINGLE) ? sizeof(float) : sizeof(double)),
  precision_(precision)
{
}

uint64_t BitpackFloatEncoder::processRecords(unsigned recordCount)
{
#ifdef E57_MAX_VERBOSE
    cout << "  BitpackFloatEncoder::processRecords() called, recordCount=" << recordCount << endl; //???
#endif

    /// Before we add any more, try to shift current contents of outBuffer_ down to beginning of buffer.
    /// This leaves outBufferEnd_ at a natural boundary.
    outBufferShiftDown();

    unsigned typeSize = (precision_ == E57_SINGLE) ? sizeof(float) : sizeof(double);

#ifdef E57_DEBUG
     /// Verify that outBufferEnd_ is multiple of typeSize (so transfers of floats are aligned naturally in memory).
     if (outBufferEnd_ % typeSize)
         throw EXCEPTION("internal error");  //??? verify all internal error checks are within E57_DEBUG statements
#endif

    /// Figure out how many records will fit in output.
    unsigned maxOutputRecords = (outBuffer_.size() - outBufferEnd_) / typeSize;

    /// Can't process more records than will safely fit in output stream
    if (recordCount > maxOutputRecords)
         recordCount = maxOutputRecords;

    if (precision_ == E57_SINGLE) {
        /// Form the starting address for next available location in outBuffer
        float* outp = reinterpret_cast<float*>(&outBuffer_[outBufferEnd_]);

        /// Copy floats from sourceBuffer_ to outBuffer_
        for (unsigned i=0; i < recordCount; i++) {
            outp[i] = sourceBuffer_->getNextFloat();
#ifdef E57_MAX_VERBOSE
            cout << "encoding float: " << outp[i] << endl;
#endif
            SWAB(&outp[i]);  /// swab if neccesary
        }
    } else {  /// E57_DOUBLE precision
        /// Form the starting address for next available location in outBuffer
        double* outp = reinterpret_cast<double*>(&outBuffer_[outBufferEnd_]);

        /// Copy doubles from sourceBuffer_ to outBuffer_
        for (unsigned i=0; i < recordCount; i++) {
            outp[i] = sourceBuffer_->getNextDouble();
#ifdef E57_MAX_VERBOSE
            cout << "encoding double: " << outp[i] << endl;
#endif
            SWAB(&outp[i]);  /// swab if neccesary
        }
    }

    /// Update end of outBuffer
    outBufferEnd_ += recordCount*typeSize;

    /// Update counts of records processed
    currentRecordIndex_ += recordCount;

    return(currentRecordIndex_);
}

bool BitpackFloatEncoder::registerFlushToOutput()
{
    /// Since have no registers in encoder, return success
    return(true);
}

float BitpackFloatEncoder::bitsPerRecord()
{
    return((precision_ == E57_SINGLE) ? 32.0F: 64.0F);
}

#ifdef E57_DEBUG
void BitpackFloatEncoder::dump(int indent, std::ostream& os)
{
    BitpackEncoder::dump(indent, os);
    if (precision_ == E57_SINGLE)
        os << space(indent) << "precision:                E57_SINGLE" << endl;
    else
        os << space(indent) << "precision:                E57_DOUBLE" << endl;
}
#endif

//================================================================

Decoder* Decoder::DecoderFactory(unsigned bytestreamNumber, //!!! name ok?
                                 shared_ptr<CompressedVectorNodeImpl> cVector,
                                 vector<SourceDestBuffer>& dbufs,
                                 ustring& codecPath)
{
    //!!! verify single dbuf


    /// Get node we are going to decode from the CompressedVector's prototype
    shared_ptr<NodeImpl> prototype = cVector->getPrototype();
    ustring path = dbufs.at(0).pathName();
    shared_ptr<NodeImpl> decodeNode = prototype->get(path);

#ifdef E57_MAX_VERBOSE
    cout << "Node to decode:" << endl; //???
    decodeNode->dump(2);
#endif

    uint64_t  maxRecordCount = cVector->childCount();

    switch (decodeNode->type()) {
        case E57_INTEGER: {
            shared_ptr<IntegerNodeImpl> ini = dynamic_pointer_cast<IntegerNodeImpl>(decodeNode);  // downcast to correct type

            /// Get pointer to parent ImageFileImpl, to call bitsNeeded()
            shared_ptr<ImageFileImpl> imf(decodeNode->fileParent_);  //??? should be function for this,  imf->parentFile()  --> ImageFile?

            unsigned bitsPerRecord = imf->bitsNeeded(ini->minimum(), ini->maximum());

            //!!! need to pick smarter channel buffer sizes, here and elsewhere
            /// Constuct Integer decoder with appropriate register size, based on number of bits stored.
            if (bitsPerRecord == 0)
                return(new ConstantIntegerDecoder(false, bytestreamNumber, dbufs.at(0), ini->minimum(), 1.0, 0.0, maxRecordCount));
            else if (bitsPerRecord <= 8)
                return(new BitpackIntegerDecoder<uint8_t>(false, bytestreamNumber, dbufs.at(0), ini->minimum(), ini->maximum(), 1.0, 0.0, maxRecordCount));
            else if (bitsPerRecord <= 16)
                return(new BitpackIntegerDecoder<uint16_t>(false, bytestreamNumber, dbufs.at(0), ini->minimum(), ini->maximum(), 1.0, 0.0, maxRecordCount));
            else if (bitsPerRecord <= 32)
                return(new BitpackIntegerDecoder<uint32_t>(false, bytestreamNumber, dbufs.at(0), ini->minimum(), ini->maximum(), 1.0, 0.0, maxRecordCount));
            else
                return(new BitpackIntegerDecoder<uint64_t>(false, bytestreamNumber, dbufs.at(0), ini->minimum(), ini->maximum(), 1.0, 0.0, maxRecordCount));

        }
        case E57_SCALED_INTEGER: {
            shared_ptr<ScaledIntegerNodeImpl> sini = dynamic_pointer_cast<ScaledIntegerNodeImpl>(decodeNode);  // downcast to correct type

            /// Get pointer to parent ImageFileImpl, to call bitsNeeded()
            shared_ptr<ImageFileImpl> imf(decodeNode->fileParent_);  //??? should be function for this,  imf->parentFile()  --> ImageFile?

            unsigned bitsPerRecord = imf->bitsNeeded(sini->minimum(), sini->maximum());

            //!!! need to pick smarter channel buffer sizes, here and elsewhere
            /// Constuct ScaledInteger dencoder with appropriate register size, based on number of bits stored.
            if (bitsPerRecord == 0)
                return(new ConstantIntegerDecoder(true, bytestreamNumber, dbufs.at(0), sini->minimum(), sini->scale(), sini->offset(), maxRecordCount));
            else if (bitsPerRecord <= 8)
                return(new BitpackIntegerDecoder<uint8_t>(true, bytestreamNumber, dbufs.at(0), sini->minimum(), sini->maximum(), sini->scale(), sini->offset(), maxRecordCount));
            else if (bitsPerRecord <= 16)
                return(new BitpackIntegerDecoder<uint16_t>(true, bytestreamNumber, dbufs.at(0), sini->minimum(), sini->maximum(), sini->scale(), sini->offset(), maxRecordCount));
            else if (bitsPerRecord <= 32)
                return(new BitpackIntegerDecoder<uint32_t>(true, bytestreamNumber, dbufs.at(0), sini->minimum(), sini->maximum(), sini->scale(), sini->offset(), maxRecordCount));
            else
                return(new BitpackIntegerDecoder<uint64_t>(true, bytestreamNumber, dbufs.at(0), sini->minimum(), sini->maximum(), sini->scale(), sini->offset(), maxRecordCount));

        }
        case E57_FLOAT: {
            shared_ptr<FloatNodeImpl> fni = dynamic_pointer_cast<FloatNodeImpl>(decodeNode);  // downcast to correct type

            return(new BitpackFloatDecoder(bytestreamNumber, dbufs.at(0), fni->precision(), maxRecordCount));
        }
        case E57_STRING: {
            throw EXCEPTION("not implemented yet"); //???
        }
        default:
            throw EXCEPTION("bad type to decode");
    }
}

//================================================================

Decoder::Decoder(unsigned bytestreamNumber)
: bytestreamNumber_(bytestreamNumber)
{}

//================================================================

BitpackDecoder::BitpackDecoder(unsigned bytestreamNumber, SourceDestBuffer& dbuf, unsigned alignmentSize, uint64_t maxRecordCount)
: Decoder(bytestreamNumber),
  inBuffer_(1024/*!!!*/),            //??? need to pick smarter channel buffer sizes
  destBuffer_(dbuf.impl())
{
    currentRecordIndex_     = 0;
    maxRecordCount_         = maxRecordCount;
    inBufferFirstBit_       = 0;
    inBufferEndByte_        = 0;
    inBufferAlignmentSize_  = alignmentSize;
    bitsPerWord_            = 8*alignmentSize;
    bytesPerWord_           = alignmentSize;
}

void BitpackDecoder::destBufferSetNew(vector<SourceDestBuffer>& dbufs)
{
    if (dbufs.size() != 1)
        throw EXCEPTION("internal error");
    destBuffer_ = dbufs.at(0).impl();
}

unsigned BitpackDecoder::inputProcess(const char* source, unsigned availableByteCount)
{
#ifdef E57_MAX_VERBOSE
    cout << "BitpackDecoder::inputprocess() called, source=" << (unsigned)source << " availableByteCount="<< availableByteCount << endl;
#endif
    unsigned bytesUnsaved = availableByteCount;
    unsigned bitsEaten = 0;
    do {
        unsigned byteCount = min(bytesUnsaved, inBuffer_.size() - inBufferEndByte_);

        /// Copy input bytes from caller, if any
        if (byteCount > 0) {
            memcpy(&inBuffer_[inBufferEndByte_], source, byteCount);

            /// Advance tail pointer.
            inBufferEndByte_ += byteCount;

            /// Update amount available from caller
            bytesUnsaved -= byteCount;
            source += byteCount;
        }
#ifdef E57_MAX_VERBOSE
        {
            unsigned i;
            unsigned firstByte = inBufferFirstBit_/8;
            for (i = 0; i < byteCount && i < 20; i++)
                cout << "  inBuffer[" << firstByte+i << "]=" << (unsigned)(unsigned char)(inBuffer_[firstByte+i]) << endl;
            if (i < byteCount)
                cout << "  " << byteCount-i << "source bytes unprinted..." << endl;
        }
#endif

        /// ??? fix doc for new bit interface
        /// Now that we have input stored in an aligned buffer, call derived class to try to eat some
        /// Note that end of filled buffer may not be at a natural boundary.
        /// The subclass may transfer this partial word in a full word transfer, but it must be carefull to only use the defined bits.
        /// inBuffer_ is a multiple of largest word size, so this full word transfer off the end will always be in defined memory.

        unsigned firstWord = inBufferFirstBit_ / bitsPerWord_;
        unsigned firstNaturalBit = firstWord * bitsPerWord_;
        unsigned endBit = inBufferEndByte_ * 8;
#ifdef E57_MAX_VERBOSE
    cout << "  feeding aligned decoder " << endBit - inBufferFirstBit_ << " bits." << endl;
#endif
        bitsEaten = inputProcessAligned(&inBuffer_[firstWord * bytesPerWord_], inBufferFirstBit_ - firstNaturalBit, endBit - firstNaturalBit);
#ifdef E57_MAX_VERBOSE
    cout << "  bitsEaten=" << bitsEaten << " firstWord=" << firstWord << " firstNaturalBit=" << firstNaturalBit << " endBit=" << endBit << endl;
#endif
#ifdef E57_DEBUG
        if (bitsEaten > endBit - inBufferFirstBit_)
            throw EXCEPTION("internal error");
#endif
        inBufferFirstBit_ += bitsEaten;

        /// Shift uneaten data to beginning of inBuffer_, keep on natural word boundaries.
        inBufferShiftDown();

        /// If the lower level processing didn't eat anything on this iteration, stop looping and tell caller how much we ate or stored.
    } while (bytesUnsaved > 0 && bitsEaten > 0);

    /// Return the number of bytes we ate/saved.
    return(availableByteCount - bytesUnsaved);
}

void BitpackDecoder::stateReset()
{
    inBufferFirstBit_ = 0;
    inBufferEndByte_  = 0;
}

void BitpackDecoder::inBufferShiftDown()
{
    /// Move uneaten data down to beginning of inBuffer_.
    /// Keep on natural boundaries.
    /// Moves all of word that contains inBufferFirstBit.
    unsigned firstWord          = inBufferFirstBit_ / bitsPerWord_;
    unsigned firstNaturalByte   = firstWord * bytesPerWord_;
#ifdef E57_DEBUG
    if (firstNaturalByte > inBufferEndByte_)
        throw EXCEPTION("internal error");
#endif
    unsigned byteCount          = inBufferEndByte_ - firstNaturalByte;
    if (byteCount > 0)
        memmove(&inBuffer_[0], &inBuffer_[firstNaturalByte], byteCount);  /// Overlapping regions ok with memmove().

    /// Update indexes
    inBufferEndByte_  = byteCount;
    inBufferFirstBit_ = inBufferFirstBit_ % bitsPerWord_;
}

#ifdef E57_DEBUG
void BitpackDecoder::dump(int indent, std::ostream& os)
{
    os << space(indent) << "bytestreamNumber:         " << bytestreamNumber_ << endl;
    os << space(indent) << "currentRecordIndex:       " << currentRecordIndex_ << endl;
    os << space(indent) << "maxRecordCount:           " << maxRecordCount_ << endl;
    os << space(indent) << "destBuffer:" << endl;
    destBuffer_->dump(indent+4, os);
    os << space(indent) << "inBufferFirstBit:        " << inBufferFirstBit_ << endl;
    os << space(indent) << "inBufferEndByte:         " << inBufferEndByte_ << endl;
    os << space(indent) << "inBufferAlignmentSize:   " << inBufferAlignmentSize_ << endl;
    os << space(indent) << "bitsPerWord:             " << bitsPerWord_ << endl;
    os << space(indent) << "bytesPerWord:            " << bytesPerWord_ << endl;
    os << space(indent) << "inBuffer:" << endl;
    unsigned i;
    for (i = 0; i < inBuffer_.size() && i < 20; i++)
        os << space(indent+4) << "inBuffer[" << i << "]: " << static_cast<unsigned>(static_cast<unsigned char>(inBuffer_.at(i))) << endl;
    if (i < inBuffer_.size())
        os << space(indent+4) << inBuffer_.size()-i << " more unprinted..." << endl;
}
#endif

//================================================================

BitpackFloatDecoder::BitpackFloatDecoder(unsigned bytestreamNumber, SourceDestBuffer& dbuf, FloatPrecision precision, uint64_t maxRecordCount)
: BitpackDecoder(bytestreamNumber, dbuf, (precision==E57_SINGLE) ? sizeof(float) : sizeof(double), maxRecordCount),
  precision_(precision)
{
}

unsigned BitpackFloatDecoder::inputProcessAligned(const char* inbuf, unsigned firstBit, unsigned endBit)
{
#ifdef E57_MAX_VERBOSE
    cout << "BitpackFloatDecoder::inputProcessAligned() called, inbuf=" << (unsigned)inbuf << " firstBit=" << firstBit << " endBit=" << endBit << endl;
#endif

    /// Read from inbuf, decode, store in destBuffer
    /// Repeat until have filled destBuffer, or completed all records

    unsigned n = destBuffer_->capacity() - destBuffer_->nextIndex();

    unsigned typeSize = (precision_ == E57_SINGLE) ? sizeof(float) : sizeof(double);

#ifdef E57_DEBUG
    /// Verify that inbuf is naturally aligned to correct boundary (4 or 8 bytes).  Base class should be doing this for us.
    if (((unsigned)inbuf) % typeSize)
        throw EXCEPTION("internal error");

    /// Verify first bit is zero
    if (firstBit != 0)
        throw EXCEPTION("internal error");
#endif

    /// Calc how many whole records worth of data we have in inbuf
    unsigned maxInputRecords = (endBit - firstBit) / (8*typeSize);

    /// Can't process more records than we have input data for.
    if (n > maxInputRecords)
        n = maxInputRecords;

    // Can't process more than defined in input file
    if (n > maxRecordCount_ - currentRecordIndex_)
        n = static_cast<unsigned>(maxRecordCount_ - currentRecordIndex_);

#ifdef E57_MAX_VERBOSE
    cout << "  n:" << n << endl; //???
#endif

    if (precision_ == E57_SINGLE) {
        /// Form the starting address for first data location in inBuffer
        const float* inp = reinterpret_cast<const float*>(inbuf);

        /// Copy floats from inbuf to destBuffer_
        for (unsigned i=0; i < n; i++) {
            float value = *inp;
            SWAB(&value);  /// swab if neccesary
#ifdef E57_MAX_VERBOSE
            cout << "  got float value=" << value << endl;
#endif
            destBuffer_->setNextFloat(value);
            inp++;
        }
    } else {  /// E57_DOUBLE precision
        /// Form the starting address for first data location in inBuffer
        const double* inp = reinterpret_cast<const double*>(inbuf);

        /// Copy doubles from inbuf to destBuffer_
        for (unsigned i=0; i < n; i++) {
            double value = *inp;
            SWAB(&value);  /// swab if neccesary
#ifdef E57_MAX_VERBOSE
            cout << "  got double value=" << value << endl;
#endif
            destBuffer_->setNextDouble(value);
            inp++;
        }
    }

    /// Update counts of records processed
    currentRecordIndex_ += n;

    /// Returned number of bits processed  (always a multiple of alignment size).
    return(n*8*typeSize);
}

#ifdef E57_DEBUG
void BitpackFloatDecoder::dump(int indent, std::ostream& os)
{
    BitpackDecoder::dump(indent, os);
    if (precision_ == E57_SINGLE)
        os << space(indent) << "precision:                E57_SINGLE" << endl;
    else
        os << space(indent) << "precision:                E57_DOUBLE" << endl;
}
#endif

//================================================================

ConstantIntegerDecoder::ConstantIntegerDecoder(bool isScaledInteger, unsigned bytestreamNumber, SourceDestBuffer& dbuf,
                                               int64_t minimum, double scale, double offset, uint64_t maxRecordCount)
: Decoder(bytestreamNumber),
  destBuffer_(dbuf.impl())
{
    currentRecordIndex_ = 0;
    maxRecordCount_     = maxRecordCount;
    isScaledInteger_    = isScaledInteger;
    minimum_            = minimum;
    scale_              = scale;
    offset_             = offset;
}

void ConstantIntegerDecoder::destBufferSetNew(vector<SourceDestBuffer>& dbufs)
{
    if (dbufs.size() != 1)
        throw EXCEPTION("internal error");
    destBuffer_ = dbufs.at(0).impl();
}

unsigned ConstantIntegerDecoder::inputProcess(const char* source, unsigned availableByteCount)
{
#ifdef E57_MAX_VERBOSE
    cout << "ConstantIntegerDecoder::inputprocess() called, source=" << (unsigned)source << " availableByteCount=" << availableByteCount << endl;
#endif

    /// We don't need any input bytes to produce output, so ignore source and availableByteCount.

    /// Fill dest buffer unless get to maxRecordCount
    unsigned count = destBuffer_->capacity() - destBuffer_->nextIndex();
    uint64_t remainingRecordCount = maxRecordCount_ - currentRecordIndex_;
    if (static_cast<uint64_t>(count) > remainingRecordCount)
        count = static_cast<unsigned>(remainingRecordCount);

    if (isScaledInteger_) {
        for (unsigned i = 0; i < count; i++)
            destBuffer_->setNextInt64(minimum_, scale_, offset_);
    } else {
        for (unsigned i = 0; i < count; i++)
            destBuffer_->setNextInt64(minimum_);
    }
    currentRecordIndex_ += count;
    return(count);
}

void ConstantIntegerDecoder::stateReset()
{
}

#ifdef E57_DEBUG
void ConstantIntegerDecoder::dump(int indent, std::ostream& os)
{
    os << space(indent) << "bytestreamNumber:   " << bytestreamNumber_ << endl;
    os << space(indent) << "currentRecordIndex: " << currentRecordIndex_ << endl;
    os << space(indent) << "maxRecordCount:     " << maxRecordCount_ << endl;
    os << space(indent) << "isScaledInteger:    " << isScaledInteger_ << endl;
    os << space(indent) << "minimum:            " << minimum_ << endl;
    os << space(indent) << "scale:              " << scale_ << endl;
    os << space(indent) << "offset:             " << offset_ << endl;
    os << space(indent) << "destBuffer:" << endl;
    destBuffer_->dump(indent+4, os);
}
#endif

//================================================================

PacketLock::PacketLock(PacketReadCache* cache, unsigned cacheIndex)
: cache_(cache),
  cacheIndex_(cacheIndex)
{
#ifdef E57_MAX_VERBOSE
    cout << "PacketLock() called" << endl;
#endif
}

PacketLock::~PacketLock()
{
#ifdef E57_MAX_VERBOSE
    cout << "~PacketLock() called" << endl;
#endif
    try {
        /// Note cache must live longer than lock, this is reasonable assumption.
        cache_->unlock(cacheIndex_);
    } catch (...) {
        //??? report?
    }
}

PacketReadCache::PacketReadCache(CheckedFile* cFile, unsigned packetCount)
: lockCount_(0),
  useCount_(0),
  cFile_(cFile),
  entries_(packetCount)
{
    if (packetCount == 0)
        throw EXCEPTION("bad cache size");

    /// Allocate requested number of maximum sized data packets buffers for holding data read from file
    for (unsigned i=0; i < entries_.size(); i++) {
        entries_.at(i).logicalOffset_ = 0;
        entries_.at(i).buffer_        = new char[E57_DATA_PACKET_MAX];
        entries_.at(i).lastUsed_      = 0;
    }
}

PacketReadCache::~PacketReadCache()
{
    /// Free allocated packet buffers
    for (unsigned i=0; i < entries_.size(); i++)
        delete [] entries_.at(i).buffer_;
}

auto_ptr<PacketLock> PacketReadCache::lock(uint64_t packetLogicalOffset, char* &pkt)
{
#ifdef E57_MAX_VERBOSE
    cout << "PacketReadCache::lock() called, packetLogicalOffset=" << packetLogicalOffset << endl;
#endif

    /// Only allow one locked packet at a time.
    if (lockCount_ > 0)
        throw EXCEPTION("cache already locked");

    /// Offset can't be 0
    if (packetLogicalOffset == 0)
        throw EXCEPTION("bad data packet offset");

    /// Linear scan for matching packet offset in cache
    for (unsigned i = 0; i < entries_.size(); i++) {
        if (packetLogicalOffset == entries_[i].logicalOffset_) {
            /// Found a match, so don't have to read anything
#ifdef E57_MAX_VERBOSE
            cout << "  Found matching cache entry, index=" << i << endl;
#endif
            /// Mark entry with current useCount (keeps track of age of entry).
            entries_[i].lastUsed_ = ++useCount_;

            /// Publish buffer address to caller
            pkt = entries_[i].buffer_;

            /// Create lock so we are sure that we will be unlocked when use is finished.
            auto_ptr<PacketLock> plock(new PacketLock(this, i));

            /// Increment cache lock just before return
            lockCount_++;
            return(plock);
        }
    }
    /// Get here if didn't find a match already in cache.

    /// Find least recently used (LRU) packet buffer
    unsigned oldestEntry = 0;
    unsigned oldestUsed = entries_.at(0).lastUsed_;
    for (unsigned i = 0; i < entries_.size(); i++) {
        if (entries_[i].lastUsed_ < oldestUsed) {
            oldestEntry = i;
            oldestUsed = entries_[i].lastUsed_;
        }
    }
#ifdef E57_MAX_VERBOSE
    cout << "  Oldest entry=" << oldestEntry << " lastUsed=" << oldestUsed << endl;
#endif

    readPacket(oldestEntry, packetLogicalOffset);

    /// Publish buffer address to caller
    pkt = entries_[oldestEntry].buffer_;

    /// Create lock so we are sure we will be unlocked when use is finished.
    auto_ptr<PacketLock> plock(new PacketLock(this, oldestEntry));

    /// Increment cache lock just before return
    lockCount_++;
    return(plock);
}

void PacketReadCache::markDiscarable(uint64_t packetLogicalOffset)
{
    /// The packet is probably not going to be used again, so mark it as really old.

    /// Linear scan for matching packet offset in cache
    for (unsigned i = 0; i < entries_.size(); i++) {
        if (packetLogicalOffset == entries_[i].logicalOffset_) {
            entries_[i].lastUsed_ = 0;
            return;
        }
    }
}

void PacketReadCache::unlock(unsigned lockedEntry)
{
#ifdef E57_MAX_VERBOSE
    cout << "PacketReadCache::unlock() called, lockedEntry=" << lockedEntry << endl;
#endif

    if (lockCount_ != 1)
        throw EXCEPTION("internal error");

    lockCount_--;
}

void PacketReadCache::readPacket(unsigned oldestEntry, uint64_t packetLogicalOffset)
{
#ifdef E57_MAX_VERBOSE
    cout << "PacketReadCache::readPacket() called, oldestEntry=" << oldestEntry << " packetLogicalOffset=" << packetLogicalOffset << endl;
#endif

    /// Read header of packet first to get length.  Use EmptyPacketHeader since it has the commom fields to all packets.
    EmptyPacketHeader header;
    cFile_->seek(packetLogicalOffset, CheckedFile::logical);
    cFile_->read(reinterpret_cast<char*>(&header), sizeof(header));
    header.swab();
    /// Can't verify packet header here, because it is not really an EmptyPacketHeader.
    unsigned packetLength = header.packetLogicalLengthMinus1+1;

    /// Be paranoid about packetLength before read
    if (packetLength > E57_DATA_PACKET_MAX)
        throw EXCEPTION("bad packet length");

    /// Now read in whole packet into preallocated buffer_.  Note buffer is
    cFile_->seek(packetLogicalOffset, CheckedFile::logical);
    cFile_->read(entries_.at(oldestEntry).buffer_, packetLength);

    /// Swab if necessary, then verify that packet is good.
    switch (header.packetType) {
        case E57_DATA_PACKET: {
                DataPacket* dpkt = reinterpret_cast<DataPacket*>(entries_.at(oldestEntry).buffer_);
#ifdef E57_BIGENDIAN
                dpkt->swab(false);
#endif
                dpkt->verify(packetLength);
#ifdef E57_MAX_VERBOSE
                cout << "  data packet:" << endl;
                dpkt->dump(4); //???
#endif
            }
            break;
        case E57_INDEX_PACKET: {
                IndexPacket* ipkt = reinterpret_cast<IndexPacket*>(entries_.at(oldestEntry).buffer_);
#ifdef E57_BIGENDIAN
                ipkt->swab(false);
#endif
                ipkt->verify(packetLength);
#ifdef E57_MAX_VERBOSE
                cout << "  index packet:" << endl;
                ipkt->dump(4); //???
#endif
            }
            break;
        case E57_EMPTY_PACKET: {
                EmptyPacketHeader* hp = reinterpret_cast<EmptyPacketHeader*>(entries_.at(oldestEntry).buffer_);
                hp->swab();
                hp->verify(packetLength);
#ifdef E57_MAX_VERBOSE
                cout << "  empty packet:" << endl;
                hp->dump(4); //???
#endif
            }
            break;
        default:
            throw EXCEPTION("unknown packet type");
    }

    entries_[oldestEntry].logicalOffset_ = packetLogicalOffset;

    /// Mark entry with current useCount (keeps track of age of entry).
    /// This is a cache, so a small hiccup when useCount_ overflows won't hurt.
    entries_[oldestEntry].lastUsed_ = ++useCount_;
}

#ifdef E57_DEBUG
void PacketReadCache::dump(int indent, std::ostream& os)
{
    os << space(indent) << "lockCount: " << lockCount_ << endl;
    os << space(indent) << "useCount:  " << useCount_ << endl;
    os << space(indent) << "entries:" << endl;
    for (unsigned i=0; i < entries_.size(); i++) {
        os << space(indent) << "entry[" << i << "]:" << endl;
        os << space(indent+4) << "logicalOffset:  " << entries_[i].logicalOffset_ << endl;
        os << space(indent+4) << "lastUsed:        " << entries_[i].lastUsed_ << endl;
        if (entries_[i].logicalOffset_ != 0) {
            os << space(indent+4) << "packet:" << endl;
            switch (reinterpret_cast<EmptyPacketHeader*>(entries_.at(i).buffer_)->packetType) {
                case E57_DATA_PACKET: {
                        DataPacket* dpkt = reinterpret_cast<DataPacket*>(entries_.at(i).buffer_);
                        dpkt->dump(indent+6, os);
                    }
                    break;
                case E57_INDEX_PACKET: {
                        IndexPacket* ipkt = reinterpret_cast<IndexPacket*>(entries_.at(i).buffer_);
                        ipkt->dump(indent+6, os);
                    }
                    break;
                case E57_EMPTY_PACKET: {
                        EmptyPacketHeader* hp = reinterpret_cast<EmptyPacketHeader*>(entries_.at(i).buffer_);
                        hp->dump(indent+6, os);
                    }
                    break;
                default:
                    throw EXCEPTION("unknown packet type");
            }
        }
    }
}
#endif

//================================================================

DecodeChannel::DecodeChannel(SourceDestBuffer dbuf_arg, Decoder* decoder_arg, unsigned bytestreamNumber_arg, uint64_t maxRecordCount_arg)
: dbuf(dbuf_arg),
  decoder(decoder_arg),
  bytestreamNumber(bytestreamNumber_arg)
{
    maxRecordCount = maxRecordCount_arg;
    currentPacketLogicalOffset = 0;
    currentBytestreamBufferIndex = 0;
    currentBytestreamBufferLength = 0;
    inputFinished = 0;
}

DecodeChannel::~DecodeChannel()
{
}


bool DecodeChannel::isOutputBlocked()
{
    /// If we have completed the entire vector, we are done
    if (decoder->totalRecordsCompleted() >= maxRecordCount)
        return(true);

    /// If we have filled the dest buffer, we are blocked
    return(dbuf.impl()->nextIndex() == dbuf.impl()->capacity());
}

bool DecodeChannel::isInputBlocked()
{
    /// If have read until the section end, we are done
    if (inputFinished)
        return(true);

    /// If have eaten all the input in the current packet, we are blocked.
    return(currentBytestreamBufferIndex == currentBytestreamBufferLength);
}


void DecodeChannel::dump(int indent, std::ostream& os)
{
    os << space(indent) << "dbuf" << endl;
    dbuf.dump(indent+4, os);

    os << space(indent) << "decoder:" << endl;
    decoder->dump(indent+4, os);

    os << space(indent) << "bytestreamNumber:              " << bytestreamNumber << endl;
    os << space(indent) << "maxRecordCount:                " << maxRecordCount << endl;
    os << space(indent) << "currentPacketLogicalOffset:    " << currentPacketLogicalOffset << endl;
    os << space(indent) << "currentBytestreamBufferIndex:  " << currentBytestreamBufferIndex << endl;
    os << space(indent) << "currentBytestreamBufferLength: " << currentBytestreamBufferLength << endl;
    os << space(indent) << "inputFinished:                 " << inputFinished << endl;
    os << space(indent) << "isInputBlocked():              " << isInputBlocked() << endl;
    os << space(indent) << "isOutputBlocked():             " << isOutputBlocked() << endl;
}

//================================================================

template <typename RegisterT>
BitpackIntegerEncoder<RegisterT>::BitpackIntegerEncoder(bool isScaledInteger, unsigned bytestreamNumber, SourceDestBuffer& sbuf,
                                                       unsigned outputMaxSize, int64_t minimum, int64_t maximum, double scale, double offset)
: BitpackEncoder(bytestreamNumber, sbuf, outputMaxSize, sizeof(RegisterT))
{
    /// Get pointer to parent ImageFileImpl
    shared_ptr<ImageFileImpl> imf(sbuf.impl()->fileParent_);  //??? should be function for this,  imf->parentFile()  --> ImageFile?

    isScaledInteger_    = isScaledInteger;
    minimum_            = minimum;
    maximum_            = maximum;
    scale_              = scale;
    offset_             = offset;
    bitsPerRecord_      = imf->bitsNeeded(minimum_, maximum_);
    sourceBitMask_      = (bitsPerRecord_==64) ? ~0 : (1ULL<<bitsPerRecord_)-1;
    registerBitsUsed_   = 0;
    register_           = 0;
}

template <typename RegisterT>
uint64_t BitpackIntegerEncoder<RegisterT>::processRecords(unsigned recordCount)
{
    //??? what are state guarantees if get an exception during transfer?
#ifdef E57_MAX_VERBOSE
    cout << "BitpackIntegerEncoder::processRecords() called, sizeof(RegisterT)=" << sizeof(RegisterT) << " recordCount=" << recordCount << endl;
    dump(4);
#endif
#ifdef E57_MAX_DEBUG
    /// Double check that register will hold at least one input records worth of bits
    if (8*sizeof(RegisterT) < bitsPerRecord_)
        throw EXCEPTION("internal error");
#endif

    /// Before we add any more, try to shift current contents of outBuffer_ down to beginning of buffer.
    /// This leaves outBufferEnd_ at a natural boundary.
    outBufferShiftDown();

#ifdef E57_DEBUG
     /// Verify that outBufferEnd_ is multiple of sizeof(RegisterT) (so transfers of RegisterT are aligned naturally in memory).
     if (outBufferEnd_ % sizeof(RegisterT))
         throw EXCEPTION("internal error");
     unsigned transferMax = (outBuffer_.size() - outBufferEnd_) / sizeof(RegisterT);
#endif

    /// Precalculate exact maximum number of records that will fit in output before overflow.
    unsigned outputWordCapacity = (outBuffer_.size() - outBufferEnd_) / sizeof(RegisterT);
    unsigned maxOutputRecords = (outputWordCapacity*8*sizeof(RegisterT) + 8*sizeof(RegisterT) - registerBitsUsed_ - 1) / bitsPerRecord_;

    /// Number of transfers is the smaller of what was requested and what will fit.
    recordCount = min(recordCount, maxOutputRecords);
#ifdef E57_MAX_VERBOSE
    cout << "  outputWordCapacity=" << outputWordCapacity << " maxOutputRecords=" << maxOutputRecords << " recordCount=" << recordCount << endl;
#endif

    /// Form the starting address for next available location in outBuffer
    RegisterT* outp = reinterpret_cast<RegisterT*>(&outBuffer_[outBufferEnd_]);
    unsigned outTransferred = 0;

    /// Copy bits from sourceBuffer_ to outBuffer_
    for (unsigned i=0; i < recordCount; i++) {
        int64_t rawValue;

        /// The parameter isScaledInteger_ determines which version of getNextInt64 gets called
        if (isScaledInteger_)
            rawValue = sourceBuffer_->getNextInt64(scale_, offset_);
        else
            rawValue = sourceBuffer_->getNextInt64();

        /// Enforce min/max specification on value
        if (rawValue < minimum_ || maximum_ < rawValue)
            throw EXCEPTION("value out of specified min/max bounds");

        uint64_t uValue = static_cast<uint64_t>(rawValue - minimum_);

#ifdef E57_MAX_VERBOSE
        cout << "encoding integer rawValue=" << binaryString(rawValue)  << " = " << hexString(rawValue)  << endl;
        cout << "                 uValue  =" << binaryString(uValue) << " = " << hexString(uValue) << endl;
#endif
#ifdef E57_DEBUG
        /// Double check that no bits outside of the mask are set
        if (uValue & ~static_cast<uint64_t>(sourceBitMask_))
            throw EXCEPTION("internal error");
#endif
        /// Mask off upper bits (just in case)
        uValue &= static_cast<uint64_t>(sourceBitMask_);

        /// See if uValue bits will fit in register
        unsigned newRegisterBitsUsed = registerBitsUsed_ + bitsPerRecord_;
#ifdef E57_MAX_VERBOSE
        cout << "  registerBitsUsed=" << registerBitsUsed_ << "  newRegisterBitsUsed=" << newRegisterBitsUsed << endl;
#endif
        if (newRegisterBitsUsed > 8*sizeof(RegisterT)) {
            /// Have more than one registers worth, fill register, transfer, then fill some more
            register_ |= static_cast<RegisterT>(uValue) << registerBitsUsed_;
#ifdef E57_DEBUG
            /// Before transfer, double check address within bounds
            if (outTransferred >= transferMax)
                throw EXCEPTION("internal error");
#endif
            outp[outTransferred] = register_;
            SWAB(&outp[outTransferred]);  /// swab if neccesary
            outTransferred++;

            register_ = static_cast<RegisterT>(uValue) >> (8*sizeof(RegisterT) - registerBitsUsed_);
            registerBitsUsed_ = newRegisterBitsUsed - 8*sizeof(RegisterT);
        } else if (newRegisterBitsUsed == 8*sizeof(RegisterT)) {
            /// Input will exactly fill register, insert value, then transfer
            register_ |= static_cast<RegisterT>(uValue) << registerBitsUsed_;
#ifdef E57_DEBUG
            /// Before transfer, double check address within bounds
            if (outTransferred >= transferMax)
                throw EXCEPTION("internal error");
#endif
            outp[outTransferred] = register_;
            SWAB(&outp[outTransferred]);  /// swab if neccesary
            outTransferred++;

            register_ = 0;
            registerBitsUsed_ = 0;
        } else {
            /// There is extra room in register, insert value, but don't do transfer yet
            register_ |= static_cast<RegisterT>(uValue) << registerBitsUsed_;
            registerBitsUsed_ = newRegisterBitsUsed;
        }
#ifdef E57_MAX_VERBOSE
        cout << "  After " << outTransferred << " transfers and " << i+1 << " records, encoder:" << endl;
        dump(4);
#endif
    }

    /// Update tail of output buffer
    outBufferEnd_ += outTransferred * sizeof(RegisterT);
#ifdef E57_DEBUG
    /// Double check end is ok
    if (outBufferEnd_ > outBuffer_.size())
        throw EXCEPTION("internal error");
#endif

    /// Update counts of records processed
    currentRecordIndex_ += recordCount;

    return(currentRecordIndex_);
}

template <typename RegisterT>
bool BitpackIntegerEncoder<RegisterT>::registerFlushToOutput()
{
#ifdef E57_MAX_VERBOSE
    cout << "BitpackIntegerEncoder::registerFlushToOutput() called, sizeof(RegisterT)=" << sizeof(RegisterT) << endl;
    dump(4);
#endif
    /// If have any used bits in register, transfer to output, padded in MSBits with zeros to RegisterT boundary
    if (registerBitsUsed_ > 0) {
        if (outBufferEnd_ < outBuffer_.size() - sizeof(RegisterT)) {
            RegisterT* outp = reinterpret_cast<RegisterT*>(&outBuffer_[outBufferEnd_]);
            *outp = register_;
            register_ = 0;
            registerBitsUsed_ = 0;
            outBufferEnd_ += sizeof(RegisterT);
            return(true);  // flush succeeded  ??? is this used? correctly?
        } else
            return(false);  // flush didn't complete (not enough room).
    } else
        return(true);
}

template <typename RegisterT>
float BitpackIntegerEncoder<RegisterT>::bitsPerRecord()
{
    return(static_cast<float>(bitsPerRecord_));
}

#ifdef E57_DEBUG
template <typename RegisterT>
void BitpackIntegerEncoder<RegisterT>::dump(int indent, std::ostream& os)
{
    BitpackEncoder::dump(indent, os);
    os << space(indent) << "isScaledInteger:  " << isScaledInteger_ << endl;
    os << space(indent) << "minimum:          " << minimum_ << endl;
    os << space(indent) << "maximum:          " << maximum_ << endl;
    os << space(indent) << "scale:            " << scale_ << endl;
    os << space(indent) << "offset:           " << offset_ << endl;
    os << space(indent) << "bitsPerRecord:    " << bitsPerRecord_ << endl;
    os << space(indent) << "sourceBitMask:    " << binaryString(sourceBitMask_) << " " << hexString(sourceBitMask_) << endl;
    os << space(indent) << "register:         " << binaryString(register_) << " " << hexString(register_) << endl;
    os << space(indent) << "registerBitsUsed: " << registerBitsUsed_ << endl;
}
#endif

//================================================================

ConstantIntegerEncoder::ConstantIntegerEncoder(unsigned bytestreamNumber, SourceDestBuffer& sbuf, int64_t minimum)
: Encoder(bytestreamNumber),
  sourceBuffer_(sbuf.impl()),
  currentRecordIndex_(0),
  minimum_(minimum)
{}

uint64_t ConstantIntegerEncoder::processRecords(unsigned recordCount)
{
#ifdef E57_MAX_VERBOSE
    cout << "ConstantIntegerEncoder::processRecords() called, recordCount=" << recordCount << endl;
    dump(4);
#endif

    /// Check that all source values are == minimum_
    for (unsigned i = 0; i < recordCount; i++) {
        if (sourceBuffer_->getNextInt64() != minimum_)
            throw EXCEPTION("value out of specified min/max bounds");
    }

    /// Update counts of records processed
    currentRecordIndex_ += recordCount;

    return(currentRecordIndex_);
}

unsigned ConstantIntegerEncoder::sourceBufferNextIndex()
{
    return(sourceBuffer_->nextIndex());
}

uint64_t ConstantIntegerEncoder::currentRecordIndex()
{
    return(currentRecordIndex_);
}

float ConstantIntegerEncoder::bitsPerRecord()
{
    /// We don't produce any output
    return(0.0);
}

bool ConstantIntegerEncoder::registerFlushToOutput()
{
    return(true);
}

unsigned ConstantIntegerEncoder::outputAvailable()
{
    /// We don't produce any output
    return(0);
}

void ConstantIntegerEncoder::outputRead(char* dest, unsigned byteCount)
{
    /// Should never request any output data
    if (byteCount > 0)
        throw EXCEPTION("internal error");
}

void ConstantIntegerEncoder::outputClear()
{}

void ConstantIntegerEncoder::sourceBufferSetNew(std::vector<SourceDestBuffer>& sbufs)
{
    /// Verify that this encoder only has single input buffer
    if (sbufs.size() != 1)
        throw EXCEPTION("internal error");

    sourceBuffer_ = sbufs.at(0).impl();
}

unsigned ConstantIntegerEncoder::outputGetMaxSize()
{
    /// We don't produce any output
    return(0);
}

void ConstantIntegerEncoder::outputSetMaxSize(unsigned byteCount)
{
    /// Ignore, since don't produce any output
}

#ifdef E57_DEBUG
void ConstantIntegerEncoder::dump(int indent, std::ostream& os)
{
    Encoder::dump(indent, os);
    os << space(indent) << "currentRecordIndex:  " << currentRecordIndex_ << endl;
    os << space(indent) << "minimum:             " << minimum_ << endl;
    os << space(indent) << "sourceBuffer:" << endl;
    sourceBuffer_->dump(indent+4, os);
}
#endif

//================================================================

template <typename RegisterT>
BitpackIntegerDecoder<RegisterT>::BitpackIntegerDecoder(bool isScaledInteger, unsigned bytestreamNumber, SourceDestBuffer& dbuf, int64_t minimum, int64_t maximum, double scale, double offset, uint64_t maxRecordCount)
: BitpackDecoder(bytestreamNumber, dbuf, sizeof(RegisterT), maxRecordCount)
{
    /// Get pointer to parent ImageFileImpl
    shared_ptr<ImageFileImpl> imf(dbuf.impl()->fileParent_);  //??? should be function for this,  imf->parentFile()  --> ImageFile?

    isScaledInteger_    = isScaledInteger;
    minimum_            = minimum;
    maximum_            = maximum;
    scale_              = scale;
    offset_             = offset;
    bitsPerRecord_      = imf->bitsNeeded(minimum_, maximum_);
    destBitMask_        = (bitsPerRecord_==64) ? ~0 : (1ULL<<bitsPerRecord_)-1;
}

template <typename RegisterT>
unsigned BitpackIntegerDecoder<RegisterT>::inputProcessAligned(const char* inbuf, unsigned firstBit, unsigned endBit)
{
#ifdef E57_MAX_VERBOSE
    cout << "BitpackIntegerDecoder::inputProcessAligned() called, inbuf=" << (unsigned)inbuf << " firstBit=" << firstBit << " endBit=" << endBit << endl;
#endif

    /// Read from inbuf, decode, store in destBuffer
    /// Repeat until have filled destBuffer, or completed all records

#ifdef E57_DEBUG
    /// Verify that inbuf is naturally aligned to RegisterT boundary (1, 2, 4,or 8 bytes).  Base class is doing this for us.
    if (((unsigned)inbuf) % sizeof(RegisterT))
        throw EXCEPTION("internal error");

    /// Verfiy first bit is in first word
    if (firstBit >= 8*sizeof(RegisterT))
        throw EXCEPTION("internal error");
#endif

    unsigned destRecords = destBuffer_->capacity() - destBuffer_->nextIndex();

    /// Precalculate exact number of full records that are in inbuf
    /// We can handle the case where don't have a full word at end of inbuf, but all the bits of the record are there;
    unsigned bitCount = endBit - firstBit;
    unsigned maxInputRecords = bitCount / bitsPerRecord_;

    /// Number of transfers is the smaller of what was requested and what is available in input.
    unsigned recordCount = min(destRecords, maxInputRecords);

    // Can't process more than defined in input file
    if (static_cast<uint64_t>(recordCount) > maxRecordCount_ - currentRecordIndex_)
        recordCount = static_cast<unsigned>(maxRecordCount_ - currentRecordIndex_);

#ifdef E57_MAX_VERBOSE
    cout << "  recordCount=" << recordCount << endl; //???
#endif

    const RegisterT* inp = reinterpret_cast<const RegisterT*>(inbuf);
    unsigned wordPosition = 0;      /// The index in inbuf of the word we are currently working on.

    ///  For example on little endian machine:
    ///  Assume: registerT=uint32_t, bitOffset=20, destBitMask=0x00007fff (for a 15 bit value).
    ///  inp[wordPosition]                    LLLLLLLL LLLLXXXX XXXXXXXX XXXXXXXX   Note LSB of value is at bit20
    ///  inp(wordPosition+1]                  XXXXXXXX XXXXXXXX XXXXXXXX XXXXXHHH   H=high bits of value, X=uninteresting bits
    ///  low = inp[i] >> bitOffset            00000000 00000000 0000LLLL LLLLLLLL   L=low bits of value, X=uninteresting bits
    ///  high = inp[i+1] << (32-bitOffset)    XXXXXXXX XXXXXXXX XHHH0000 00000000
    ///  w = high | low                       XXXXXXXX XXXXXXXX XHHHLLLL LLLLLLLL
    ///  destBitmask                          00000000 00000000 01111111 11111111
    ///  w & mask                             00000000 00000000 0HHHLLLL LLLLLLLL

    unsigned bitOffset = firstBit;

    for (unsigned i = 0; i < recordCount; i++) {
        /// Get lower word (contains at least the LSbit of the value),
        RegisterT low = inp[wordPosition];
        SWAB(&low);  // swab if necessary

        /// Get upper word (may or may not contain interesting bits),
        RegisterT high = inp[wordPosition+1];
        SWAB(&high);  // swab if necessary

        RegisterT w;
        if (bitOffset > 0) {
            /// Shift high to just above the lower bits, shift low LSBit to bit0, OR together.
            /// Note shifts are logical (not arithmetic) because using unsigned variables.
            w = (high << (8*sizeof(RegisterT) - bitOffset)) | (low >> bitOffset);
        } else {
            /// The left shift (used above) is not defined if shift is >= size of word
            w = low;
        }
#ifdef E57_MAX_VERBOSE
        cout << "  bitOffset: " << bitOffset << endl;
        cout << "  low: " << binaryString(low) << endl;
        cout << "  high:" << binaryString(high) << endl;
        cout << "  w:   " << binaryString(w) << endl;
#endif

        /// Mask off uninteresting bits
        w &= destBitMask_;

        /// Add minimum_ to value to get back what writer originally sent
        int64_t value = minimum_ + static_cast<uint64_t>(w);

#ifdef E57_MAX_VERBOSE
        cout << "  Storing value=" << value << endl;
#endif

        /// The parameter isScaledInteger_ determines which version of setNextInt64 gets called
        if (isScaledInteger_)
            destBuffer_->setNextInt64(value, scale_, offset_);
        else
            destBuffer_->setNextInt64(value);

        /// Store the result in next avaiable position in the user's dest buffer

        /// Calc next bit alignment and which word it starts in
        bitOffset += bitsPerRecord_;
        if (bitOffset >= 8*sizeof(RegisterT)) {
            bitOffset -= 8*sizeof(RegisterT);
            wordPosition++;
        }
#ifdef E57_MAX_VERBOSE
        cout << "  Processed " << i+1 << " records, wordPosition=" << wordPosition << " decoder:" << endl;
        dump(4);
#endif
    }

    /// Update counts of records processed
    currentRecordIndex_ += recordCount;

    /// Return number of bits processed.
    return(recordCount * bitsPerRecord_);
}

#ifdef E57_DEBUG
template <typename RegisterT>
void BitpackIntegerDecoder<RegisterT>::dump(int indent, std::ostream& os)
{
    BitpackDecoder::dump(indent, os);
    os << space(indent) << "isScaledInteger:  " << isScaledInteger_ << endl;
    os << space(indent) << "minimum:          " << minimum_ << endl;
    os << space(indent) << "maximum:          " << maximum_ << endl;
    os << space(indent) << "scale:            " << scale_ << endl;
    os << space(indent) << "offset:           " << offset_ << endl;
    os << space(indent) << "bitsPerRecord:    " << bitsPerRecord_ << endl;
    os << space(indent) << "destBitMask:      " << binaryString(destBitMask_) << " = " << hexString(destBitMask_) << endl;
}
#endif

