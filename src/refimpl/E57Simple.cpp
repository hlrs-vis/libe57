//////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 2010 ASTM E57.04 3D Imaging System File Format Committee
//	All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person or organization
//  obtaining a copy of the software and accompanying documentation covered by
//  this license (the "Software") to use, reproduce, display, distribute,
//  execute, and transmit the Software, and to prepare derivative works of the
//  Software, and to permit third-parties to whom the Software is furnished to
//  do so, all subject to the following:
// 
//  The copyright notices in the Software and this entire statement, including
//  the above license grant, this restriction and the following disclaimer,
//  must be included in all copies of the Software, in whole or in part, and
//  all derivative works of the Software, unless such copies or derivative
//  works are solely in the form of machine-executable object code generated by
//  a source language processor.
// 
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
//  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
//  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//
//	The Boost license Vestion 1.0 - August 17th, 2003 is discussed in
//	http://www.boost.org/users/license.html.
//
//  This source code is only intended as a supplement to promote the
//	ASTM E57.04 3D Imaging System File Format standard for interoperability
//	of Lidar Data.  See http://www.libe57.org.
//
//////////////////////////////////////////////////////////////////////////
//
//	New E57Simple.cpp
//	V1		May 26, 2010	Stan Coleby		scoleby@intelisum.com
//
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

using namespace e57;
using namespace std;
using namespace boost;

////////////////////////////////////////////////////////////////////
//
//	e57::E57Root
//
	E57Root::E57Root(void)
{
};

	E57Root::~E57Root(void)
{
};
////////////////////////////////////////////////////////////////////
//
//	e57::Data3D
//
	Data3D::Data3D(void)
{
	originalGuids.clear();
	temperature = 0.;
	relativeHumidity = 0.;
	atmosphericPressure = 0.;

	acquisitionStart.dateTimeValue = 0.;
	acquisitionStart.isGpsReferenced = 0;
	acquisitionEnd.dateTimeValue = 0.;
	acquisitionEnd.isGpsReferenced = 0;

	pose.rotation.w = 1.;
	pose.rotation.x = 0.;
	pose.rotation.y = 0.;
	pose.rotation.z = 0.;
	pose.translation.x = 0.;
	pose.translation.y = 0.;
	pose.translation.z = 0.;

	cartesianBounds.xMaximum = E57_DOUBLE_MAX;
	cartesianBounds.xMinimum = -E57_DOUBLE_MAX;
	cartesianBounds.yMaximum = E57_DOUBLE_MAX;
	cartesianBounds.yMinimum = -E57_DOUBLE_MAX;
	cartesianBounds.zMaximum = E57_DOUBLE_MAX;
	cartesianBounds.zMinimum = -E57_DOUBLE_MAX;

	sphericalBounds.rangeMinimum = 0.;
	sphericalBounds.rangeMaximum = E57_DOUBLE_MAX;
	sphericalBounds.azimuthStart = 0.;
	sphericalBounds.azimuthEnd = 2 * PI;
	sphericalBounds.elevationMinimum = -PI/2.;
	sphericalBounds.elevationMaximum = PI/2.;

	pointGroupingSchemes.groupingByLine.groupsSize = 0;
	pointGroupingSchemes.groupingByLine.idElementName = "ColumnIndex";

	pointFields.azimuth = false;
	pointFields.colorBlue = false;
	pointFields.colorGreen = false;
	pointFields.colorRed = false;
	pointFields.columnIndex = false;
	pointFields.elevation = false;
	pointFields.intensity = false;
	pointFields.isValid = false;
	pointFields.range = false;
	pointFields.returnCount = false;
	pointFields.returnIndex = false;
	pointFields.rowIndex = false;
	pointFields.timestamp = false;
	pointFields.x = false;
	pointFields.y = false;
	pointFields.z = false;

	row = 0;
	column = 0;
	pointsSize = 0;
};

	Data3D::~Data3D(void)
{
};
////////////////////////////////////////////////////////////////////
//
//	e57::CameraImage
//
	CameraImage::CameraImage(void)
{
	acquisitionDateTime.dateTimeValue = 0.;
	acquisitionDateTime.isGpsReferenced = 0;

	pose.rotation.w = 1.;
	pose.rotation.x = 0.;
	pose.rotation.y = 0.;
	pose.rotation.z = 0.;
	pose.translation.x = 0.;
	pose.translation.y = 0.;
	pose.translation.z = 0.;

//	pinholeRepresentation;
//	sphericalRepresentation;
//	cylindricalRepresentation;
};

	CameraImage::~CameraImage(void)
{
};
////////////////////////////////////////////////////////////////////
//
//	e57::Reader
//
	Reader::Reader(
		const ustring & filePath)
	: m_imf(filePath,"r")
	, m_root(m_imf.root())
	, m_data3D(m_root.get("/data3D"))
	, m_cameraImages(m_root.get("/cameraImages"))
{
	try
	{

	}
	catch (...)
	{

	};
};

	Reader::~Reader(void)
{
	if(IsOpen())
		Close();
};

e57::Reader* Reader::CreateReader(
		const ustring & filePath)
{
	e57::Reader* ptr = NULL;
	try
	{
 		ptr = new Reader(filePath);
	}
	catch (...)
	{
		if (ptr != NULL)
			delete ptr;
		ptr = NULL;
    }
	return ptr;
};

//! This function returns true if the file is open
bool	Reader :: IsOpen(void)
{
	if( m_imf.isOpen())
		return true;
	return false;
};

//! This function closes the file
bool	Reader :: Close(void)
{
	if(IsOpen())
	{
		m_imf.close();
		return true;
	}
	return false;
};

////////////////////////////////////////////////////////////////////
//
//	File information
//
//! This function returns the file header information
bool	Reader :: GetE57Root(
	E57Root * fileHeader)	//!< This is the main header information
{
	try
	{
		fileHeader->formatName = StringNode(m_root.get("formatName")).value();
		fileHeader->versionMajor = IntegerNode(m_root.get("versionMajor")).value();
		fileHeader->versionMinor = IntegerNode(m_root.get("versionMinor")).value();
		fileHeader->guid = StringNode(m_root.get("guid")).value();
		fileHeader->coordinateMetadata = StringNode(m_root.get("coordinateMetadata")).value();

		StructureNode creationDateTime(m_root.get("creationDateTime"));
		fileHeader->creationDateTime.dateTimeValue = FloatNode(creationDateTime.get("dateTimeValue")).value();
		fileHeader->creationDateTime.isGpsReferenced = IntegerNode(creationDateTime.get("isGpsReferenced")).value();

		fileHeader->data3DSize = m_data3D.childCount();
		fileHeader->cameraImagesSize = m_cameraImages.childCount();
		return true;

	} catch(E57Exception& ex) {
        ex.report(__FILE__, __LINE__, __FUNCTION__);
    } catch (std::exception& ex) {
        cerr << "Got an std::exception, what=" << ex.what() << endl;
    } catch (...) {
        cerr << "Got an unknown exception" << endl;
    }
	return false;
};

////////////////////////////////////////////////////////////////////
//
//	Camera Image picture data
//
//! This function returns the total number of Picture Blocks
int32_t	Reader :: GetCameraImageCount( void)
{
	return m_cameraImages.childCount();
};

//! This function returns the cameraImages header and positions the cursor
bool	Reader :: GetCameraImage( 
	int32_t			imageIndex,		//!< This in the index into the cameraImages vector
	CameraImage *	cameraImageHeader	//!< pointer to the CameraImage structure to receive the picture information
	)						//!< /return Returns true if sucessful
{
	return false;
};

//! This function reads the block
int64_t	Reader :: ReadCameraImageData(
	int32_t		imageIndex,		//!< picture block index
	void *		pBuffer,	//!< pointer the buffer
	int64_t		start,		//!< position in the block to start reading
	int64_t		count		//!< size of desired chuck or buffer size
	)						//!< /return Returns the number of bytes transferred.
{
	return 0;
};

////////////////////////////////////////////////////////////////////
//
//	Scanner Image 3d data
//
//! This function returns the total number of Image Blocks
int32_t	Reader :: GetData3DCount( void)
{
	return m_data3D.childCount();
};

//! This function returns the Data3D header and positions the cursor
bool	Reader :: GetData3D( 
	int32_t		dataIndex,	//!< This in the index into the images3D vector
	Data3D *	data3DHeader //!< pointer to the Data3D structure to receive the image information
	)	//!< /return Returns true if sucessful
{
	return false;
};

//! This function returns the size of the point data
void	Reader :: GetData3DPointSize(
	int32_t		dataIndex,	//!< image block index
	int32_t *	row,		//!< image row size
	int32_t *	column		//!< image column size
	)
{

};

//! This function returns the number of point groups
int32_t	Reader :: GetData3DGroupSize(
	int32_t		dataIndex		//!< image block index
	)
{
	return 0;
};

//! This function returns the active fields available
bool	Reader :: GetData3DStandardizedFieldsAvailable(
	int32_t		dataIndex,		//!< This in the index into the images3D vector
	PointStandardizedFieldsAvailable * pointFields //!< pointer to the Data3D structure to receive the PointStandardizedFieldsAvailable information
	)
{
	return false;
};

//! This function returns the point data fields fetched in single call
//* All the non-NULL buffers in the call below have number of elements = count */

int64_t	Reader :: GetData3DStandardPoints(
	int32_t		dataIndex,
	int64_t		startPointIndex,
	int64_t		count,
	bool*		isValid,
	int32_t*	rowIndex,
	int32_t*	columnIndex,
	int32_t*	returnIndex,
	int32_t*	returnCount,
	double*		x,
	double*		y,
	double*		z,
	double*		range,
	double*		azimuth,
	double*		elevation,
	double*		intensity,
	double*		colorRed,
	double*		colorGreen,
	double*		colorBlue,
	double*		timestamp
	)
{
	return 0;
};

//! This function interrogate what fields (standardized and extensions) are available
bool	Reader :: GetData3DGeneralFieldsAvailable(
	int32_t					dataIndex,
	std::vector<ustring>&	fieldsAvailable)
{
	return false;
};

int64_t	Reader :: GetData3DGeneralPoints(
	int32_t				dataIndex,
	int64_t				startPointIndex,
	int64_t				pointCount,
	bool*				isValid,
	vector<ustring>&	numericFieldNames,
	vector<double*>&	numericBuffers,
	vector<ustring>&	stringFieldNames,
	vector<ustring*>&	stringBuffers)
{
	return 0;
};
////////////////////////////////////////////////////////////////////
//
//	e57::Writer
//
//! This function is the constructor for the writer class
	Writer::Writer(
		const ustring & filePath,
		const ustring & coordinateMetadata)
	: m_imf(filePath,"w")
	, m_root(m_imf.root())
	, m_data3D(m_imf,true)
	, m_cameraImages(m_imf,true)
{
	m_idElementValue.clear();
	m_startPointIndex.clear();
	m_pointCount.clear();

	try
	{
// Set per-file properties.
/// Path names: "/formatName", "/majorVersion", "/minorVersion", "/coordinateMetadata"
		m_root.set("formatName", StringNode(m_imf, "ASTM E57 3D Imaging Data File"));

#if defined(_MSC_VER)
		GUID		guid;
		CoCreateGuid((GUID*)&guid);

		OLECHAR wbuffer[64];
		StringFromGUID2(guid,&wbuffer[0],64);

		char	fileGuid[64];
		wcstombs(fileGuid,wbuffer,64);
#else
		char	fileGuid[] = "{4179C162-49A8-4fba-ADC6-527543D26D86}";
#endif
		m_root.set("guid", StringNode(m_imf, fileGuid));

// Get ASTM version number supported by library, so can write it into file
		int astmMajor;
		int astmMinor;
		ustring libraryId;
		E57Utilities().getVersions(astmMajor, astmMinor, libraryId);

		m_root.set("versionMajor", IntegerNode(m_imf, astmMajor));
		m_root.set("versionMinor", IntegerNode(m_imf, astmMinor));

// Save a dummy string for coordinate system.
/// Really should be a valid WKT string identifying the coordinate reference system (CRS).
        m_root.set("coordinateMetadata", StringNode(m_imf, coordinateMetadata));

// Create creationDateTime structure
/// Path name: "/creationDateTime
        StructureNode creationDateTime = StructureNode(m_imf);
		creationDateTime.set("dateTimeValue", FloatNode(m_imf, 1234567890.)); //!!! convert time() to GPStime
		creationDateTime.set("isGpsReferenced", IntegerNode(m_imf,0));
        m_root.set("creationDateTime", creationDateTime);

		m_root.set("data3D", m_data3D);
		m_root.set("cameraImages", m_cameraImages);

	} catch(E57Exception& ex) {
        ex.report(__FILE__, __LINE__, __FUNCTION__);
    } catch (std::exception& ex) {
        cerr << "Got an std::exception, what=" << ex.what() << endl;
    } catch (...) {
        cerr << "Got an unknown exception" << endl;
    }
};
//! This function is the destructor for the writer class
	Writer::~Writer(void)
{
	m_idElementValue.clear();
	m_startPointIndex.clear();
	m_pointCount.clear();

	if(IsOpen())
		Close();
};
//! This function returns an E57::Writer pointer and opens the file
e57::Writer* Writer::CreateWriter(
	const ustring & filePath,			//!< file path string
	const ustring & coordinateMetadata	//!< Information describing the Coordinate Reference System to be used for the file
	)									//!< /return This returns a e57::Writer object which should be deleted when finish
{
	e57::Writer* ptr = NULL;
	try
	{
 		ptr = new Writer(filePath, coordinateMetadata);	
	}
	catch (...)
	{
		if (ptr != NULL)
			delete ptr;
		ptr = NULL;
    }
	return ptr;
};
//! This function returns true if the file is open
bool	Writer :: IsOpen(void)
{
	if(m_imf.isOpen())
		return true;
	return false;
};

//! This function closes the file
bool	Writer :: Close(void)
{
	if(IsOpen())
	{
		m_idElementValue.clear();
		m_startPointIndex.clear();
		m_pointCount.clear();

		m_imf.close();
		return true;
	}
	return false;
};

////////////////////////////////////////////////////////////////////
//
//	Camera Image picture data
//

//! This function sets up the cameraImages header and positions the cursor
//* The user needs to config a CameraImage structure with all the camera information before making this call. */
int32_t	Writer :: NewCameraImage( 
	CameraImage &	cameraImageHeader	//!< pointer to the CameraImage structure to receive the picture information
	)						//!< /return Returns the cameraImage index
{
	return 0;
};

//! This function writes the block
int64_t	Writer :: WriteCameraImage(
	int32_t		imageIndex,	//!< picture block index
	void *		pBuffer,	//!< pointer the buffer
	int64_t		start,		//!< position in the block to start writing
	int64_t		count		//!< size of desired chuck or buffer size
	)						//!< /return Returns the number of bytes written
{
	return 0;
};
//! This function closes the CameraImage block
bool	Writer :: CloseCameraImage(
	int32_t		imageIndex	//!< picture block index given by the NewCameraImage
		)					//!< /return Returns true if successful, false otherwise
{
	return false;
}
//! This function sets up the Data3D header and positions the cursor for the binary data
//* The user needs to config a Data3D structure with all the scanning information before making this call. */

int32_t	Writer :: NewData3D( 
	Data3D &	data3DHeader //!< pointer to the Data3D structure to receive the image information
	)	//!< /return Returns the index of the new scan.
{
	int32_t pos = -1;
	try
	{
		m_data3DHeader = data3DHeader;		//Make a copy

		int row = data3DHeader.row;
		int col = data3DHeader.column;

		StructureNode scan = StructureNode(m_imf);
		m_data3D.append(scan);
		pos = m_data3D.childCount() - 1;

		scan.set("guid", StringNode(m_imf, data3DHeader.guid));
		scan.set("name", StringNode(m_imf, data3DHeader.name));
		scan.set("description", StringNode(m_imf, data3DHeader.description));

// Add various sensor and version strings to scan.
/// Path names: "/data3D/0/sensorVendor", etc...
		scan.set("sensorVendor",           StringNode(m_imf, data3DHeader.sensorVendor));
		scan.set("sensorModel",            StringNode(m_imf, data3DHeader.sensorModel));
		scan.set("sensorSerialNumber",     StringNode(m_imf, data3DHeader.sensorSerialNumber));
		scan.set("sensorHardwareVersion",  StringNode(m_imf, data3DHeader.sensorHardwareVersion));
		scan.set("sensorSoftwareVersion",  StringNode(m_imf, data3DHeader.sensorSoftwareVersion));
		scan.set("sensorFirmwareVersion",  StringNode(m_imf, data3DHeader.sensorFirmwareVersion));

// Add temp/humidity to scan.
/// Path names: "/data3D/0/temperature", etc...
		scan.set("temperature",      FloatNode(m_imf, data3DHeader.temperature));
		scan.set("relativeHumidity", FloatNode(m_imf, data3DHeader.relativeHumidity));
		scan.set("atmosphericPressure", FloatNode(m_imf, data3DHeader.atmosphericPressure));

// Add Cartesian bounding box to scan.
/// Path names: "/data3D/0/cartesianBounds/xMinimum", etc...
        StructureNode bbox = StructureNode(m_imf);
		bbox.set("xMinimum", FloatNode(m_imf, data3DHeader.cartesianBounds.xMinimum));
		bbox.set("xMaximum", FloatNode(m_imf, data3DHeader.cartesianBounds.xMaximum));
		bbox.set("yMinimum", FloatNode(m_imf, data3DHeader.cartesianBounds.yMinimum));
		bbox.set("yMaximum", FloatNode(m_imf, data3DHeader.cartesianBounds.yMaximum));
		bbox.set("zMinimum", FloatNode(m_imf, data3DHeader.cartesianBounds.zMinimum));
		bbox.set("zMaximum", FloatNode(m_imf, data3DHeader.cartesianBounds.zMaximum));
        scan.set("cartesianBounds", bbox);

// Create pose structure for scan.
/// Path names: "/data3D/0/pose/rotation/w", etc...
///             "/data3D/0/pose/translation/x", etc...
        StructureNode pose = StructureNode(m_imf);
        scan.set("pose", pose);
        StructureNode rotation = StructureNode(m_imf);
        pose.set("rotation", rotation);
		rotation.set("w", FloatNode(m_imf, data3DHeader.pose.rotation.w));
        rotation.set("x", FloatNode(m_imf, data3DHeader.pose.rotation.x));
        rotation.set("y", FloatNode(m_imf, data3DHeader.pose.rotation.y));
        rotation.set("z", FloatNode(m_imf, data3DHeader.pose.rotation.z));
        StructureNode translation = StructureNode(m_imf);
        pose.set("translation", translation);
		translation.set("x", FloatNode(m_imf, data3DHeader.pose.translation.x));
        translation.set("y", FloatNode(m_imf, data3DHeader.pose.translation.y));
        translation.set("z", FloatNode(m_imf, data3DHeader.pose.translation.z));

// Add start/stop acquisition times to scan.
/// Path names: "/data3D/0/acquisitionStart/dateTimeValue",
///             "/data3D/0/acquisitionEnd/dateTimeValue"
        StructureNode acquisitionStart = StructureNode(m_imf);
        scan.set("acquisitionStart", acquisitionStart);
		acquisitionStart.set("dateTimeValue",
			FloatNode(m_imf, data3DHeader.acquisitionStart.dateTimeValue));
		acquisitionStart.set("isAtomicClockReferenced",
			IntegerNode(m_imf, data3DHeader.acquisitionStart.isGpsReferenced));

        StructureNode acquisitionEnd = StructureNode(m_imf);
        scan.set("acquisitionEnd", acquisitionEnd);
		acquisitionEnd.set("dateTimeValue",
			FloatNode(m_imf, data3DHeader.acquisitionEnd.dateTimeValue));
		acquisitionEnd.set("isAtomicClockReferenced",
			IntegerNode(m_imf, data3DHeader.acquisitionEnd.isGpsReferenced));

// Add grouping scheme area
        /// Path name: "/data3D/0/pointGroupingSchemes"
        StructureNode pointGroupingSchemes = StructureNode(m_imf);
        scan.set("pointGroupingSchemes", pointGroupingSchemes);

        /// Add a line grouping scheme
        /// Path name: "/data3D/0/pointGroupingSchemes/groupingByLine"
        StructureNode groupingByLine = StructureNode(m_imf);
        pointGroupingSchemes.set("groupingByLine", groupingByLine);

        /// Add idElementName to groupingByLine, specify a line is column oriented
        /// Path name: "/data3D/0/pointGroupingSchemes/groupingByLine/idElementName"
		groupingByLine.set("idElementName", StringNode(m_imf, "columnIndex"));
		///			data3DHeader.pointGroupingSchemes.groupingByLine.idElementName));

// Make a prototype of datatypes that will be stored in LineGroupRecord.
        /// This prototype will be used in creating the groups CompressedVector.
        /// Will define path names like:
        ///     "/data3D/0/pointGroupingSchemes/groupingByLine/groups/0/idElementValue"
        StructureNode lineGroupProto = StructureNode(m_imf);
        lineGroupProto.set("idElementValue",    IntegerNode(m_imf, 0, 0, col));
        lineGroupProto.set("startPointIndex",   IntegerNode(m_imf, 0, 0, row*col));
        lineGroupProto.set("pointCount",        IntegerNode(m_imf, 0, 0, row));

       /// Make empty codecs vector for use in creating groups CompressedVector.
        /// If this vector is empty, it is assumed that all fields will use the BitPack codec.
        VectorNode lineGroupCodecs = VectorNode(m_imf, true);

        /// Create CompressedVector for storing groups.  
        /// Path Name: "/data3D/0/pointGroupingSchemes/groupingByLine/groups".
        /// We use the prototype and empty codecs tree from above.
        /// The CompressedVector will be filled by code below.
        CompressedVectorNode groups = CompressedVectorNode(m_imf, lineGroupProto, lineGroupCodecs);
        groupingByLine.set("groups", groups);

// Make a prototype of datatypes that will be stored in points record.
        /// This prototype will be used in creating the points CompressedVector.
        /// Using this proto in a CompressedVector will define path names like:
        ///      "/data3D/0/points/0/cartesianX"
        StructureNode proto = StructureNode(m_imf);

		if(data3DHeader.pointFields.isValid)
			proto.set("valid",       IntegerNode(m_imf, 0, 0, 1));

		if(data3DHeader.pointFields.x)
			proto.set("cartesianX",  FloatNode(m_imf, 0., E57_SINGLE, E57_FLOAT_MIN, E57_FLOAT_MAX));
//			proto.set("cartesianX",  ScaledIntegerNode(m_imf, 0, E57_INT16_MIN, E57_INT16_MAX, 0.001, 0));
		if(data3DHeader.pointFields.y)
			proto.set("cartesianY",  FloatNode(m_imf, 0., E57_SINGLE, E57_FLOAT_MIN, E57_FLOAT_MAX));
//			proto.set("cartesianY",  ScaledIntegerNode(m_imf, 0, E57_INT16_MIN, E57_INT16_MAX, 0.001, 0));
		if(data3DHeader.pointFields.z)
			proto.set("cartesianZ",  FloatNode(m_imf, 0., E57_SINGLE, E57_FLOAT_MIN, E57_FLOAT_MAX));
//			proto.set("cartesianZ",  ScaledIntegerNode(m_imf, 0, E57_INT16_MIN, E57_INT16_MAX, 0.001, 0));

		if(data3DHeader.pointFields.range)
			proto.set("sphericalRange",  ScaledIntegerNode(m_imf, 0, E57_INT16_MIN, E57_INT16_MAX, 0.001, 0));
//			proto.set("sphericalRange",  FloatNode(m_imf, 0., E57_SINGLE, E57_FLOAT_MIN, E57_FLOAT_MAX));
		if(data3DHeader.pointFields.azimuth)
			proto.set("spherialAzimuth",  ScaledIntegerNode(m_imf, 0, E57_INT16_MIN, E57_INT16_MAX, 0.001, 0));
//			proto.set("spherialAzimuth",  FloatNode(m_imf, 0., E57_SINGLE, E57_FLOAT_MIN, E57_FLOAT_MAX));
		if(data3DHeader.pointFields.elevation)
			proto.set("sphericalElevation",  ScaledIntegerNode(m_imf, 0, E57_INT16_MIN, E57_INT16_MAX, 0.001, 0));
//			proto.set("sphericalElevation",  FloatNode(m_imf, 0., E57_SINGLE, E57_FLOAT_MIN, E57_FLOAT_MAX));

		if(data3DHeader.pointFields.rowIndex)
			proto.set("rowIndex",    IntegerNode(m_imf, 0, 0, data3DHeader.row));
		if(data3DHeader.pointFields.columnIndex)
			proto.set("columnIndex", IntegerNode(m_imf, 0, 0, data3DHeader.column));

		if(data3DHeader.pointFields.returnIndex)
			proto.set("returnIndex", IntegerNode(m_imf, 0, 0, 0));
	    if(data3DHeader.pointFields.returnCount)
			proto.set("returnCount", IntegerNode(m_imf, 1, 1, 1));
		if(data3DHeader.pointFields.timestamp)
			proto.set("timeStamp",   FloatNode(m_imf, 0.0, E57_DOUBLE));

		if(data3DHeader.pointFields.intensity)
			proto.set("intensity",   FloatNode(m_imf, 0.0, E57_SINGLE, 0.0, 1.0));
//			proto.set("intensity",   IntegerNode(m_imf, 0, 0, 255));
		if(data3DHeader.pointFields.colorRed)
			proto.set("colorRed",    FloatNode(m_imf, 0.0, E57_SINGLE, 0.0, 1.0));
//			proto.set("colorRed",   IntegerNode(m_imf, 0, 0, 255));
		if(data3DHeader.pointFields.colorGreen)
			proto.set("colorGreen",  FloatNode(m_imf, 0.0, E57_SINGLE, 0.0, 1.0));
//			proto.set("colorGreen",   IntegerNode(m_imf, 0, 0, 255));
		if(data3DHeader.pointFields.colorBlue)
			proto.set("colorBlue",   FloatNode(m_imf, 0.0, E57_SINGLE, 0.0, 1.0));
//			proto.set("colorBlue",   IntegerNode(m_imf, 0, 0, 255));

//        proto.set("demo:extra2", StringNode(m_imf));

// Make empty codecs vector for use in creating points CompressedVector.
        /// If this vector is empty, it is assumed that all fields will use the BitPack codec.
        VectorNode codecs = VectorNode(m_imf, true);

// Create CompressedVector for storing points.  Path Name: "/data3D/0/points".
        /// We use the prototype and empty codecs tree from above.
        /// The CompressedVector will be filled by code below.
        CompressedVectorNode points = CompressedVectorNode(m_imf, proto, codecs);
        scan.set("points", points);
		return pos;

	} catch(E57Exception& ex) {
        ex.report(__FILE__, __LINE__, __FUNCTION__);
    } catch (std::exception& ex) {
        cerr << "Got an std::exception, what=" << ex.what() << endl;
    } catch (...) {
        cerr << "Got an unknown exception" << endl;
    }
	return -1;
};

int64_t	Writer :: WriteData3DStandardPoints(
	int32_t		dataIndex,
	int64_t		idElementValue,
	int64_t		startPointIndex,
	int64_t		count,
	bool*		isValid,
	int32_t*	rowIndex,
	int32_t*	columnIndex,
	int32_t*	returnIndex,
	int32_t*	returnCount,
	double*		x,
	double*		y,
	double*		z,
	double*		range,
	double*		azimuth,
	double*		elevation,
	double*		intensity,
	double*		colorRed,
	double*		colorGreen,
	double*		colorBlue,
	double*		timestamp
	)
{
	try
	{
		if( (dataIndex < 0) || (dataIndex >= m_data3D.childCount()))
			return 0;
///////////  This is a problem because we have to do this for every call
		StructureNode scan(m_data3D.get(dataIndex));
		CompressedVectorNode points(scan.get("points"));

		vector<SourceDestBuffer> sourceBuffers;
		if(m_data3DHeader.pointFields.x)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "cartesianX",  x,  count, true, true));
		if(m_data3DHeader.pointFields.y)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "cartesianY",  y,  count, true, true));
		if(m_data3DHeader.pointFields.z)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "cartesianZ",  z,  count, true, true));
		if(m_data3DHeader.pointFields.range)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "sphericalRange",  range,  count, true, true));
		if(m_data3DHeader.pointFields.azimuth)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "spherialAzimuth",  azimuth,  count, true, true));
		if(m_data3DHeader.pointFields.elevation)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "sphericalElevation",  elevation,  count, true, true));
		if(m_data3DHeader.pointFields.isValid)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "valid",       isValid,       count, true));
		if(m_data3DHeader.pointFields.rowIndex)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "rowIndex",    rowIndex,    count, true));
		if(m_data3DHeader.pointFields.columnIndex)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "columnIndex", columnIndex, count, true));
		if(m_data3DHeader.pointFields.returnIndex)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "returnIndex", returnIndex, count, true));
		if(m_data3DHeader.pointFields.returnCount)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "returnCount", returnCount, count, true));
		if(m_data3DHeader.pointFields.timestamp)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "timeStamp",   timestamp,   count, true));
		if(m_data3DHeader.pointFields.intensity)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "intensity",   intensity,   count, true));
		if(m_data3DHeader.pointFields.colorRed)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "colorRed",    colorRed,    count, true));
		if(m_data3DHeader.pointFields.colorGreen)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "colorGreen",  colorGreen,  count, true));
		if(m_data3DHeader.pointFields.colorBlue)
			sourceBuffers.push_back(SourceDestBuffer(m_imf, "colorBlue",   colorBlue,   count, true));

		CompressedVectorWriter writer = points.writer(sourceBuffers);
		writer.write(count);
		writer.close();

		m_idElementValue.push_back(idElementValue);
		m_startPointIndex.push_back(startPointIndex);
		m_pointCount.push_back(count);

		return count;

	} catch(E57Exception& ex) {
        ex.report(__FILE__, __LINE__, __FUNCTION__);
    } catch (std::exception& ex) {
        cerr << "Got an std::exception, what=" << ex.what() << endl;
    } catch (...) {
        cerr << "Got an unknown exception" << endl;
    }
	return 0;
};

//! This function closes the data3D block
bool	Writer :: CloseData3D(
	int32_t		dataIndex	//!< data block index given by the NewData3D
	){;						//!< /return Returns true if sucessful, false otherwise
	if( (dataIndex < 0) || (dataIndex >= m_data3D.childCount()))
		return 0;
	try	{
		StructureNode scan(m_data3D.get(dataIndex));
		StructureNode pointGroupingSchemes(scan.get("pointGroupingSchemes"));
		StructureNode groupingByLine(pointGroupingSchemes.get("groupingByLine"));
		CompressedVectorNode groups(groupingByLine.get("groups"));

		int NG = m_idElementValue.size();

		vector<SourceDestBuffer> groupSDBuffers;
        groupSDBuffers.push_back(SourceDestBuffer(m_imf, "idElementValue",  (int32_t*) &m_idElementValue[0],   NG, true));
        groupSDBuffers.push_back(SourceDestBuffer(m_imf, "startPointIndex", (int32_t*) &m_startPointIndex[0],  NG, true));
        groupSDBuffers.push_back(SourceDestBuffer(m_imf, "pointCount",      (int32_t*) &m_pointCount[0],       NG, true));

		CompressedVectorWriter writer = groups.writer(groupSDBuffers);
        writer.write(NG);
        writer.close();
 
		return true;
	
	} catch(E57Exception& ex) {
        ex.report(__FILE__, __LINE__, __FUNCTION__);
    } catch (std::exception& ex) {
        cerr << "Got an std::exception, what=" << ex.what() << endl;
    } catch (...) {
        cerr << "Got an unknown exception" << endl;
    }

	return false;
};

//! This function sets the extensions field that will be available
bool	Writer :: SetData3DGeneralFieldsAvailable(
	int32_t					dataIndex,
	std::vector<ustring>&	fieldsAvailable)
{
	return false;
};

int64_t	Writer :: WriteData3DGeneralPoints(
	int32_t				dataIndex,
	int64_t				startPointIndex,
	int64_t				pointCount,
	bool*				isValid,
	vector<ustring>&	numericFieldNames,
	vector<double*>&	numericBuffers,
	vector<ustring>&	stringFieldNames,
	vector<ustring*>&	stringBuffers)
{
	return 0;
};

