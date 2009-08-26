/*
 * E57Foundation.cpp - implementation of public functions of the E57 Foundation API.
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
#include "E57FoundationImpl.h"
using namespace e57;
using namespace std;
using namespace std::tr1;

//=====================================================================================
NodeType Node::type()                       {return(impl_->type());}
bool Node::isRoot()                         {return(impl_->isRoot());}
Node Node::parent()                         {return(Node(impl_->parent()));}
ustring Node::pathName()                    {return(impl_->pathName());}
ustring Node::fieldName()                   {return(impl_->fieldName());}
#ifdef E57_DEBUG
void Node::dump(int indent, ostream& os)    {impl_->dump(indent, os);}
#else
void Node::dump(int indent, ostream& os)    {}
#endif

Node::Node(shared_ptr<NodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
StructureNode::StructureNode(ImageFile imf)
: impl_(new StructureNodeImpl(imf.impl()))
{}

StructureNode::StructureNode(std::tr1::weak_ptr<ImageFileImpl> fileParent)
: impl_(new StructureNodeImpl(fileParent))
{}

NodeType StructureNode::type()                              {return(impl_->type());}
bool StructureNode::isRoot()                                {return(impl_->isRoot());}
Node StructureNode::parent()                                {return(Node(impl_->parent()));}
ustring StructureNode::pathName()                           {return(impl_->pathName());}
ustring StructureNode::fieldName()                          {return(impl_->fieldName());}
int64_t StructureNode::childCount()                         {return(impl_->childCount());}
bool StructureNode::isDefined(const ustring& pathName)      {return(impl_->isDefined(pathName));}
Node StructureNode::get(int64_t index)                      {return(Node(impl_->get(index)));}
Node StructureNode::get(const ustring& pathName)            {return(Node(impl_->get(pathName)));}
void StructureNode::set(int64_t index, Node n)              {impl_->set(index, n.impl());}
void StructureNode::set(const ustring& pathName, Node n, bool autoPathCreate)   {impl_->set(pathName, n.impl(), autoPathCreate);}
void StructureNode::append(Node n)                          {impl_->append(n.impl());}
#ifdef E57_DEBUG
void StructureNode::dump(int indent, ostream& os)           {impl_->dump(indent, os);}
#else
void StructureNode::dump(int indent, ostream& os)           {}
#endif

StructureNode::operator Node()
{
    /// Implicitly upcast from shared_ptr<StructureNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

StructureNode::StructureNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<StructureNodeImpl>
    shared_ptr<StructureNodeImpl> ni(dynamic_pointer_cast<StructureNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

StructureNode::StructureNode(shared_ptr<StructureNodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
VectorNode::VectorNode(ImageFile imf, bool allowHeteroChildren)
: impl_(new VectorNodeImpl(imf.impl(), allowHeteroChildren))
{}

NodeType VectorNode::type()                             {return(impl_->type());}
bool VectorNode::isRoot()                               {return(impl_->isRoot());}
Node VectorNode::parent()                               {return(Node(impl_->parent()));}
ustring VectorNode::pathName()                          {return(impl_->pathName());}
ustring VectorNode::fieldName()                         {return(impl_->fieldName());}
bool VectorNode::allowHeteroChildren()                  {return(impl_->allowHeteroChildren());}
int64_t VectorNode::childCount()                        {return(impl_->childCount());}
bool VectorNode::isDefined(const ustring& pathName)     {return(impl_->isDefined(pathName));}
Node VectorNode::get(int64_t index)                     {return(Node(impl_->get(index)));}
Node VectorNode::get(const ustring& pathName)           {return(Node(impl_->get(pathName)));}
void VectorNode::set(int64_t index, Node n)             {impl_->set(index, n.impl());}
void VectorNode::set(const ustring& pathName, Node n, bool autoPathCreate)  {/*???impl_->set(pathName, n.impl());*/} //!!! huh?
void VectorNode::append(Node n)                         {impl_->append(n.impl());}
#ifdef E57_DEBUG
void VectorNode::dump(int indent, ostream& os)          {impl_->dump(indent, os);}
#else
void VectorNode::dump(int indent, ostream& os)          {}
#endif

VectorNode::operator Node()
{
    /// Implicitly upcast from shared_ptr<VectorNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

VectorNode::VectorNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<VectorNodeImpl>
    shared_ptr<VectorNodeImpl> ni(dynamic_pointer_cast<VectorNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

VectorNode::VectorNode(shared_ptr<VectorNodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, int8_t* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, uint8_t* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, int16_t* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, uint16_t* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, int32_t* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, uint32_t* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, int64_t* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, bool* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, float* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, double* b, unsigned capacity, bool doConversion, bool doScaling, size_t stride)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b, capacity, doConversion, doScaling, stride))
{}

SourceDestBuffer::SourceDestBuffer(ImageFile imf, ustring pathName, vector<ustring>* b)
: impl_(new SourceDestBufferImpl(imf.impl(), pathName, b))
{}

ustring     SourceDestBuffer::pathName()        {return(impl_->pathName());}
MemoryRep   SourceDestBuffer::elementType()     {return(impl_->elementType());};
unsigned    SourceDestBuffer::capacity()        {return(impl_->capacity());}
bool        SourceDestBuffer::doConversion()    {return(impl_->doConversion());}
bool        SourceDestBuffer::doScaling()       {return(impl_->doScaling());}
size_t      SourceDestBuffer::stride()          {return(impl_->stride());}

#ifdef E57_DEBUG
void SourceDestBuffer::dump(int indent, ostream& os)            {impl_->dump(indent, os);}
#else
void SourceDestBuffer::dump(int indent, ostream& os)            {}
#endif

//=====================================================================================
CompressedVectorReader::CompressedVectorReader(shared_ptr<CompressedVectorReaderImpl> ni)
: impl_(ni)
{}

unsigned CompressedVectorReader::read()
{
    return(impl_->read());
}

unsigned CompressedVectorReader::read(vector<SourceDestBuffer>& dbufs)
{
    return(impl_->read(dbufs));
}

void CompressedVectorReader::seek(uint64_t recordNumber)
{
    impl_->seek(recordNumber);
}

void CompressedVectorReader::close()
{
    impl_->close();
}

#ifdef E57_DEBUG
void CompressedVectorReader::dump(int indent, ostream& os)      {impl_->dump(indent, os);}
#else
void CompressedVectorReader::dump(int indent, ostream& os)      {}
#endif

//=====================================================================================
CompressedVectorWriter::CompressedVectorWriter(shared_ptr<CompressedVectorWriterImpl> ni)
: impl_(ni)
{}

void CompressedVectorWriter::write(unsigned elementCount)
{
    impl_->write(elementCount);
}

void CompressedVectorWriter::write(vector<SourceDestBuffer>& sbufs, unsigned elementCount)
{
    impl_->write(sbufs, elementCount);
}

void CompressedVectorWriter::close()
{
    impl_->close();
}

#ifdef E57_DEBUG
void CompressedVectorWriter::dump(int indent, ostream& os)      {impl_->dump(indent, os);}
#else
void CompressedVectorWriter::dump(int indent, ostream& os)      {}
#endif

//=====================================================================================

CompressedVectorNode::CompressedVectorNode(ImageFile imf, Node prototype, Node codecs)
: impl_(new CompressedVectorNodeImpl(imf.impl()))
{
    /// Because of shared_ptr quirks, can't set prototype,codecs in CompressedVectorNodeImpl(), so set it afterwards ???true?
    impl_->setPrototype(prototype.impl());
    impl_->setCodecs(codecs.impl());
}

NodeType CompressedVectorNode::type()                           {return(impl_->type());}
bool CompressedVectorNode::isRoot()                             {return(impl_->isRoot());}
Node CompressedVectorNode::parent()                             {return(Node(impl_->parent()));}
ustring CompressedVectorNode::pathName()                        {return(impl_->pathName());}
ustring CompressedVectorNode::fieldName()                       {return(impl_->fieldName());}
int64_t CompressedVectorNode::childCount()                      {return(impl_->childCount());}
Node CompressedVectorNode::prototype()                          {return(Node(impl_->getPrototype()));}
bool CompressedVectorNode::isDefined(const ustring& pathName)   {return(impl_->isDefined(pathName));}
#ifdef E57_DEBUG
void CompressedVectorNode::dump(int indent, ostream& os)        {impl_->dump(indent, os);}
#else
void CompressedVectorNode::dump(int indent, ostream& os)        {}
#endif

CompressedVectorNode::operator Node()
{
    /// Implicitly upcast from shared_ptr<CompressedVectorNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

CompressedVectorNode::CompressedVectorNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<CompressedVectorNodeImpl>
    shared_ptr<CompressedVectorNodeImpl> ni(dynamic_pointer_cast<CompressedVectorNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

CompressedVectorNode::CompressedVectorNode(shared_ptr<CompressedVectorNodeImpl> ni)
: impl_(ni)
{}

CompressedVectorWriter CompressedVectorNode::writer(vector<SourceDestBuffer>& sbufs)
{
    return(CompressedVectorWriter(impl_->writer(sbufs)));
}

CompressedVectorReader CompressedVectorNode::reader(vector<SourceDestBuffer>& dbufs)
{
    return(CompressedVectorReader(impl_->reader(dbufs)));
}

//=====================================================================================
IntegerNode::IntegerNode(ImageFile imf, int8_t   value, int8_t   minimum, int8_t  maximum)
: impl_(new IntegerNodeImpl(imf.impl(), static_cast<int64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum)))
{}

IntegerNode::IntegerNode(ImageFile imf, int16_t  value, int16_t  minimum, int16_t  maximum)
: impl_(new IntegerNodeImpl(imf.impl(), static_cast<int64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum)))
{}

IntegerNode::IntegerNode(ImageFile imf, int32_t  value, int32_t  minimum, int32_t  maximum)
: impl_(new IntegerNodeImpl(imf.impl(), static_cast<int64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum)))
{}

IntegerNode::IntegerNode(ImageFile imf, int64_t  value, int64_t  minimum, int64_t  maximum)
: impl_(new IntegerNodeImpl(imf.impl(), value, minimum, maximum))
{}

IntegerNode::IntegerNode(ImageFile imf, uint8_t  value, uint8_t  minimum, uint8_t  maximum)
: impl_(new IntegerNodeImpl(imf.impl(), static_cast<int64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum)))
{}

IntegerNode::IntegerNode(ImageFile imf, uint16_t value, uint16_t minimum, uint16_t maximum)
: impl_(new IntegerNodeImpl(imf.impl(), static_cast<int64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum)))
{}

IntegerNode::IntegerNode(ImageFile imf, uint32_t value, uint32_t minimum, uint32_t maximum)
: impl_(new IntegerNodeImpl(imf.impl(), static_cast<int64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum)))
{}

NodeType IntegerNode::type()        {return(impl_->type());}
bool IntegerNode::isRoot()          {return(impl_->isRoot());}
Node IntegerNode::parent()          {return(Node(impl_->parent()));}
ustring IntegerNode::pathName()     {return(impl_->pathName());}
ustring IntegerNode::fieldName()    {return(impl_->fieldName());}
int64_t IntegerNode::value()        {return(impl_->value());}
int64_t IntegerNode::minimum()      {return(impl_->minimum());}
int64_t IntegerNode::maximum()      {return(impl_->maximum());}

#ifdef E57_DEBUG
void IntegerNode::dump(int indent, ostream& os) {impl_->dump(indent, os);}
#else
void IntegerNode::dump(int indent, ostream& os) {}
#endif

IntegerNode::operator Node()
{
    /// Upcast from shared_ptr<IntegerNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

IntegerNode::IntegerNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<IntegerNodeImpl>
    shared_ptr<IntegerNodeImpl> ni(dynamic_pointer_cast<IntegerNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

IntegerNode::IntegerNode(shared_ptr<IntegerNodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
ScaledIntegerNode::ScaledIntegerNode(ImageFile imf, int8_t   value, int8_t   minimum, int8_t  maximum, double scale, double offset)
: impl_(new ScaledIntegerNodeImpl(imf.impl(), static_cast<uint64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum), scale, offset))
{}

ScaledIntegerNode::ScaledIntegerNode(ImageFile imf, int16_t  value, int16_t  minimum, int16_t  maximum, double scale, double offset)
: impl_(new ScaledIntegerNodeImpl(imf.impl(), static_cast<uint64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum), scale, offset))
{}

ScaledIntegerNode::ScaledIntegerNode(ImageFile imf, int32_t  value, int32_t  minimum, int32_t  maximum, double scale, double offset)
: impl_(new ScaledIntegerNodeImpl(imf.impl(), static_cast<uint64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum), scale, offset))
{}

ScaledIntegerNode::ScaledIntegerNode(ImageFile imf, int64_t  value, int64_t  minimum, int64_t  maximum, double scale, double offset)
: impl_(new ScaledIntegerNodeImpl(imf.impl(), static_cast<uint64_t>(value), minimum, maximum, scale, offset))
{}

ScaledIntegerNode::ScaledIntegerNode(ImageFile imf, uint8_t  value, uint8_t  minimum, uint8_t  maximum, double scale, double offset)
: impl_(new ScaledIntegerNodeImpl(imf.impl(), static_cast<uint64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum), scale, offset))
{}

ScaledIntegerNode::ScaledIntegerNode(ImageFile imf, uint16_t value, uint16_t minimum, uint16_t maximum, double scale, double offset)
: impl_(new ScaledIntegerNodeImpl(imf.impl(), static_cast<uint64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum), scale, offset))
{}

ScaledIntegerNode::ScaledIntegerNode(ImageFile imf, uint32_t value, uint32_t minimum, uint32_t maximum, double scale, double offset)
: impl_(new ScaledIntegerNodeImpl(imf.impl(), static_cast<uint64_t>(value), static_cast<int64_t>(minimum), static_cast<int64_t>(maximum), scale, offset))
{}

NodeType ScaledIntegerNode::type()      {return(impl_->type());}
bool ScaledIntegerNode::isRoot()        {return(impl_->isRoot());}
Node ScaledIntegerNode::parent()        {return(Node(impl_->parent()));}
ustring ScaledIntegerNode::pathName()   {return(impl_->pathName());}
ustring ScaledIntegerNode::fieldName()  {return(impl_->fieldName());}
int64_t ScaledIntegerNode::rawValue()   {return(impl_->rawValue());}
double  ScaledIntegerNode::scaledValue(){return(impl_->scaledValue());}
int64_t ScaledIntegerNode::minimum()    {return(impl_->minimum());}
int64_t ScaledIntegerNode::maximum()    {return(impl_->maximum());}
double  ScaledIntegerNode::scale()      {return(impl_->scale());}
double  ScaledIntegerNode::offset()     {return(impl_->offset());}

#ifdef E57_DEBUG
void ScaledIntegerNode::dump(int indent, ostream& os)   {impl_->dump(indent, os);}
#else
void ScaledIntegerNode::dump(int indent, ostream& os)   {}
#endif

ScaledIntegerNode::operator Node()
{
    /// Upcast from shared_ptr<ScaledIntegerNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

ScaledIntegerNode::ScaledIntegerNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<ScaledIntegerNodeImpl>
    shared_ptr<ScaledIntegerNodeImpl> ni(dynamic_pointer_cast<ScaledIntegerNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

ScaledIntegerNode::ScaledIntegerNode(shared_ptr<ScaledIntegerNodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
FloatNode::FloatNode(ImageFile imf, float value, FloatPrecision precision)
: impl_(new FloatNodeImpl(imf.impl(), value, precision))
{}

FloatNode::FloatNode(ImageFile imf, double value, FloatPrecision precision)
: impl_(new FloatNodeImpl(imf.impl(), value, precision))
{}

NodeType FloatNode::type()              {return(impl_->type());}
bool FloatNode::isRoot()                {return(impl_->isRoot());}
Node FloatNode::parent()                {return(Node(impl_->parent()));}
ustring FloatNode::pathName()           {return(impl_->pathName());}
ustring FloatNode::fieldName()          {return(impl_->fieldName());}
double FloatNode::value()               {return(impl_->value());}
FloatPrecision FloatNode::precision()   {return(impl_->precision());}

#ifdef E57_DEBUG
void FloatNode::dump(int indent, ostream& os)   {impl_->dump(indent, os);}
#else
void FloatNode::dump(int indent, ostream& os)   {}
#endif

FloatNode::operator Node()
{
    /// Upcast from shared_ptr<FloatNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

FloatNode::FloatNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<FloatNodeImpl>
    shared_ptr<FloatNodeImpl> ni(dynamic_pointer_cast<FloatNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

FloatNode::FloatNode(shared_ptr<FloatNodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
StringNode::StringNode(ImageFile imf, ustring value)
: impl_(new StringNodeImpl(imf.impl(), value))
{}

NodeType StringNode::type()     {return(impl_->type());}
bool StringNode::isRoot()       {return(impl_->isRoot());}
Node StringNode::parent()       {return(Node(impl_->parent()));}
ustring StringNode::pathName()  {return(impl_->pathName());}
ustring StringNode::fieldName() {return(impl_->fieldName());}
ustring StringNode::value()     {return(impl_->value());}

#ifdef E57_DEBUG
void StringNode::dump(int indent, ostream& os)  {impl_->dump(indent, os);}
#else
void StringNode::dump(int indent, ostream& os)  {}
#endif

StringNode::operator Node()
{
    /// Upcast from shared_ptr<StringNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

StringNode::StringNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<StringNodeImpl>
    shared_ptr<StringNodeImpl> ni(dynamic_pointer_cast<StringNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

StringNode::StringNode(shared_ptr<StringNodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
BlobNode::BlobNode(ImageFile imf, uint64_t byteCount)
: impl_(new BlobNodeImpl(imf.impl(), byteCount))
{}

BlobNode::BlobNode(ImageFile imf, uint64_t fileOffset, uint64_t length)
: impl_(new BlobNodeImpl(imf.impl(), fileOffset, length))
{}

NodeType BlobNode::type()       {return(impl_->type());}
bool BlobNode::isRoot()         {return(impl_->isRoot());}
Node BlobNode::parent()         {return(Node(impl_->parent()));}
ustring BlobNode::pathName()    {return(impl_->pathName());}
ustring BlobNode::fieldName()   {return(impl_->fieldName());}
int64_t BlobNode::byteCount()   {return(impl_->byteCount());}

void BlobNode::read(uint8_t* buf, uint64_t start, uint64_t count)
{
    impl_->read(buf, start, count);
}

void BlobNode::write(uint8_t* buf, uint64_t start, uint64_t count)
{
    impl_->write(buf, start, count);
}


#ifdef E57_DEBUG
void BlobNode::dump(int indent, ostream& os)    {impl_->dump(indent, os);}
#else
void BlobNode::dump(int indent, ostream& os)    {}
#endif

BlobNode::operator Node()
{
    /// Upcast from shared_ptr<StringNodeImpl> to shared_ptr<NodeImpl> and construct a Node object
    return(Node(impl_));
}

BlobNode::BlobNode(Node& n)
{
    /// Downcast from shared_ptr<NodeImpl> to shared_ptr<BlobNodeImpl>
    shared_ptr<BlobNodeImpl> ni(dynamic_pointer_cast<BlobNodeImpl>(n.impl()));
    if (!ni)
        throw(EXCEPTION("bad downcast"));

    /// Set our shared_ptr to the downcast shared_ptr
    impl_ = ni;
}

BlobNode::BlobNode(shared_ptr<BlobNodeImpl> ni)
: impl_(ni)
{}

//=====================================================================================
ImageFile::ImageFile(const ustring& fname, const ustring& mode, const ustring& configuration)
: impl_(new ImageFileImpl())
{
    /// Do second phase of construction, now that ImageFile object is complete.
    impl_->construct2(fname, mode, configuration);
}


StructureNode   ImageFile::root()           {return(StructureNode(impl_->root()));}
void            ImageFile::close()          {impl_->close();}
void            ImageFile::cancel()         {impl_->cancel();}

void    ImageFile::extensionsAdd(const ustring& prefix, const ustring& uri)     {return(impl_->extensionsAdd(prefix, uri));}
bool    ImageFile::extensionsLookupPrefix(const ustring& prefix, ustring& uri)  {return(impl_->extensionsLookupPrefix(prefix, uri));}
bool    ImageFile::extensionsLookupUri(const ustring& uri, ustring& prefix)     {return(impl_->extensionsLookupUri(uri, prefix));}
int     ImageFile::extensionsCount()                                            {return(impl_->extensionsCount());}
ustring ImageFile::extensionsPrefix(int index)                                  {return(impl_->extensionsPrefix(index));}
ustring ImageFile::extensionsUri(int index)                                     {return(impl_->extensionsUri(index));}

bool ImageFile::fieldNameIsExtension(const ustring& fieldName)
{
    return(impl_->fieldNameIsExtension(fieldName));
}

void ImageFile::fieldNameParse(const ustring& fieldName, ustring& prefix, ustring& base)
{
    return(impl_->fieldNameParse(fieldName, prefix, base));
}

void ImageFile::pathNameParse(const ustring& pathName, bool& isRelative, std::vector<ustring>& fields)
{
    return(impl_->pathNameParse(pathName, isRelative, fields));
}

ustring ImageFile::pathNameUnparse(bool isRelative, const std::vector<ustring>& fields)
{
    return(impl_->pathNameUnparse(isRelative, fields));
}

ustring ImageFile::fileNameExtension(const ustring& fileName)
{
    return(impl_->fileNameExtension(fileName));
}

void ImageFile::fileNameParse(const ustring& fileName, bool& isRelative, ustring& volumeName, std::vector<ustring>& directories, 
                                 ustring& fileBase, ustring& extension)
{
    return(impl_->fileNameParse(fileName, isRelative, volumeName, directories, fileBase, extension));
}

ustring ImageFile::fileNameUnparse(bool isRelative, const ustring& volumeName, const std::vector<ustring>& directories,
                                   const ustring& fileBase, const ustring& extension)
{
    return(impl_->fileNameUnparse(isRelative, volumeName, directories, fileBase, extension));
}


#ifdef E57_DEBUG
void ImageFile::dump(int indent, ostream& os)   {impl_->dump(indent, os);}
#else
void ImageFile::dump(int indent, ostream& os)   {}
#endif
