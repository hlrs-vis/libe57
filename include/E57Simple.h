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
//  Old ReaderE57.h
//	V1		Aug 4, 2008		Stan Coleby		scoleby@intelisum.com
//	V2		Aug 19, 2008	Stan Coleby		scoleby@intelisum.com
//  V3		Oct 4, 2008		Stan Coleby		scoleby@intelisum.com
//  V4		Oct 29, 2008	Stan Coleby		scoleby@intelisum.com
//
//	New E57Simple.h
//	V5		May 22, 2010	Stan Coleby		scoleby@intelisum.com
//
//////////////////////////////////////////////////////////////////////////

#ifndef E57SIMPLE_H_INCLUDED
#define E57SIMPLE_H_INCLUDED

#ifndef E57FOUNDATION_H_INCLUDED
#include "E57Foundation.h"
#endif

namespace e57 {

////////////////////////////////////////////////////////////////////
//
//	e57::Point
//
/*
//! e57::Point is a location of a lidar point in 3D space.
class Point {
public:
	double		x;	//!< position on the X axis as a double
	double		y;	//!< position on the Y axis as a double
	double		z;	//!< position on the Z axis as a double
};

//! e57::PointF is a location of a lidar point in 3D space using a float real
class PointF {
public:
	float		x;	//!< position on the X axis as a float
	float		y;	//!< position on the Y axis as a float
	float		z;	//!< position on the Z axis as a float
};

//! e57::range is a location of a lidar point in 3D space using polar coordinates.
class Range {
public:
	double		r;	//!< range the distance to the point as a double
	double		e;	//!< elevation angle from the horizontal plane as a double
	double		a;	//!< aximuth angle from the X axis as a double
};
*/

//! e57::Translation is a direction vector from the origin to a point.
class Translation {
public:
	double		x;	//!< position on the X axis as a double
	double		y;	//!< position on the Y axis as a double
	double		z;	//!< position on the Z axis as a double
};
////////////////////////////////////////////////////////////////////
//
//	e57::Quaternion
//
//! e57::Quaternion is a quaternion which represents a rigid body rotation.

class Quaternion {
public:
	double		w;	//!< The real part as a double
	double		x;	//!< The i component of the quaternion as a double
	double		y;	//!< The j component of the quaternion as a double
	double		z;	//!< The k component of the quaternion as a double
};

////////////////////////////////////////////////////////////////////
//
//	e57::RigidBodyTransform
//
//! e57::RigidBodyTransform is a structure that defines a rigid body transform in cartesian coordinates.

class RigidBodyTransform {
public:
	e57::Quaternion		rotation;		//!< A unit quaternion representing the rotation, R, of the transform
	e57::Translation	translation;	//!< The translation point vector, t, of the transform
};

////////////////////////////////////////////////////////////////////
//
//	e57::CartesianBounds
//
//! e57::CartesianBounds structure specifies an axis-aligned box in local cartesian coordinates.

class CartesianBounds {
public:
	double		xMinimum;	//!< The minimum extent of the bounding box in the X direction
	double		xMaximum;	//!< The maximum extent of the bounding box in the X direction
	double		yMinimum;	//!< The minimum extent of the bounding box in the Y direction
	double		yMaximum;	//!< The maximum extent of the bounding box in the Y direction
	double		zMinimum;	//!< The minimum extent of the bounding box in the Z direction
	double		zMaximum;	//!< The maximum extent of the bounding box in the Z direction
};

////////////////////////////////////////////////////////////////////
//
//	e57::SphericalBounds
//
//! e57::SphericalBounds structure stores teh bounds of some data in spherical coordinates.

class SphericalBounds {
public:
	double		rangeMinimum;		//!< The minimum extent of the bounding region in the r direction
	double		rangeMaximum;		//!< The maximum extent of the bounding region in the r direction
	double		elevationMinimum;	//!< The minimum extent of the bounding region from the horizontal plane
	double		elevationMaximum;	//!< The maximum extent of the bounding region from the horizontal plane
	double		azimuthStart;		//!< The starting azimuth angle defining the extent of the bounding region around the z axis
	double		azimuthEnd;			//!< The ending azimuth angle defining the extent of the bounding region around the z axix
};

////////////////////////////////////////////////////////////////////
//
//	e57::DateTime
//
//! e57::DateTime is a structure for encoding date and time. 
/*! The date and time is encoded using a single
	562 floating point number, stored as an E57 Float element which is based on the Global Positioning
	563 System (GPS) time scale. */

class DateTime {
public:
	double		dateTimeValue;		//!< The time, in seconds, since GPS time was zero. This time specification may include fractions of a second
	int			isGpsReferenced;	//!< This element should be present, and its value set to 1 if, and only if, the time stored in the dateTimeValue element is truly referenced to GPS time
};

////////////////////////////////////////////////////////////////////
//
//	e57::E57Root
//

//! The e57::E57Root is a structure that stores the top-level information for the XML section of the file.

class E57Root {
public:
//! This function is the constructor for the images3D class
					E57Root(void);
//! This function is the destructor for the reader class
					~E57Root(void);

	ustring			formatName;			//!< Contains the string �ASTM E57 3D Image File�
	ustring			guid;				//!< A globally unique identification string for the current version of the file
	uint32_t		versionMajor;		//!< Major version number, should be 1
	uint32_t		versionMinor;		//!< Minor version number, should be 0
	e57::DateTime	creationDateTime;	//!< Date/time that the file was created
	int				data3DSize;			//!< Size of the vector of data3D structures for storing 3D imaging data
	int				cameraImagesSize;	//!< Size of the vector of cameraImage structures for storing camera images.
	ustring			coordinateMetadata;	//!< Information describing the Coordinate Reference System to be used for the file
};

////////////////////////////////////////////////////////////////////
//
//	e57::LineGroupRecord
//
//! e57::LineGroupRecord is a structure that stores information about a single group of points in a row or column

class LineGroupRecord {
public:
	int						idElementValue;		//!< A user-defined identifier for this group
	int						startPointIndex;	//!< The record number of the first point in the continuous interval
	int						pointCount;			//!< The number of PointRecords in the group. May be zero
	e57::CartesianBounds	cartesianBounds;	//!< The bounding box (in Cartesian coordinates) of all points in the group (in the local coordinate system of the points.)
	e57::SphericalBounds	sphericalBounds;	//!< The bounding region (in spherical coordinates) of all the points in the group (in the local coordinate system of the points.)
};

////////////////////////////////////////////////////////////////////
//
//	e57::GroupingByLine
//
//! e57::GroupingByLine is a structure that stores a set of point groups organized by the rowIndex or columnIndex attribute of the PointRecord

class GroupingByLine {
public:
	ustring		idElementName;		//!< The name of the PointRecord element that identifies which group the point is in. The value of this string must be �rowIndex� or �columnIndex�
	int			groupsSize;			//!< Size of the compressedVector of LineGroupRecord structures
};

////////////////////////////////////////////////////////////////////
//
//	e57::PointGroupingSchemes
//
//! e57::PointGroupingSchemes structure structure supports the division of points within an Data3D into logical groupings

class PointGroupingSchemes {
public:
	GroupingByLine	groupingByLine;	//!< Grouping information by row or column index
};

////////////////////////////////////////////////////////////////////
//
//	e57::PointStandardizedFieldsAvailable
//
//! The e57::PointStandardizedFieldAvailable is a structure use to interrogate if standardized fields are available

class PointStandardizedFieldsAvailable {
public:
	bool	isValid;			//!< indicates that the PointRecord valid field is active
	bool	rowIndex;			//!< indicates that the PointRecord rowIndex field is active
	bool	columnIndex;		//!< indicates that the PointRecord columnIndex field is active
	bool	returnIndex;		//!< indicates that the PointRecord returnIndex field is active
	bool	returnCount;		//!< indicates that the PointRecord returnCount field is active
	bool	x;					//!< indicates that the PointRecord cartesianX field is active
	bool	y;					//!< indicates that the PointRecord cartesianY field is active
	bool	z;					//!< indicates that the PointRecord cartesianZ field is active
	bool	range;				//!< indicates that the PointRecord sphericalRange field is active
	bool	azimuth;			//!< indicates that the PointRecord sphericalAzimuth field is active
	bool	elevation;			//!< indicates that the PointRecord sphericalElevation field is active
	bool	intensity;			//!< indicates that the PointRecord intensity field is active
	bool	colorRed;			//!< indicates that the PointRecord colorRed field is active
	bool	colorGreen;			//!< indicates that the PointRecord colorGreen field is active
	bool	colorBlue;			//!< indicates that the PointRecord colorBlue field is active
	bool	timestamp;			//!< indicates that the PointRecord timeStamp field is active
};

////////////////////////////////////////////////////////////////////
//
//	e57::PointRecord
//
//! e57::PointRecord is a structure that stores the information for an individual 3D imaging system point measurement.

class PointRecord {
public:
	double		cartesianX;		//!< The X coordinate (in meters) of the point in Cartesian coordinates
	double		cartesianY;		//!< The Y coordinate (in meters) of the point in Cartesian coordinates
	double		cartesianZ;		//!< The Z coordinate (in meters) of the point in Cartesian coordinates
	double		sphericalRange;	//!< The range (in meters) of points in spherical coordinates. Shall be non-negative
	double		sphericalAzimuth; //!< Azimuth angle (in radians) of point in spherical coordinates
	double		sphericalElevation;	//!< Elevation angle (in radians) of point in spherical coordinates
	bool		valid;			//!< Value = 1 if the point is considered valid, 0 otherwise
	int			rowIndex;		//!< The row number of point (zero based). This is useful for data that is stored in a regular grid.Shall be in the interval [0, 2^63).
	int			columnIndex;	//!< The column number of point (zero based). This is useful for data that is stored in a regular grid. Shall be in the interval [0, 2^63)
	int			returnIndex;	//!< Only for multi-return sensors. The number of this return (zero based). That is, 0 is the first return, 1 is the second, and so on. Shall be in the interval [0, returnCount).
	int			returnCount;	//!< Only for multi-return sensors. The total number of returns for the pulse that this corresponds to. Shall be in the interval (0, 2^63).
	double		timeStamp;		//!< The time (in seconds) since the start time for the data, which is given by acquisitionStart in the parent Data3D Structure. Shall be non-negative
	double		intensity;		//!< Point response intensity. Unit is unspecified
	uint8_t		colorRed;		//!< Red color coefficient. Unit is unspecified.
	uint8_t		colorGreen;		//!< Green color coefficient. Unit is unspecified
	uint8_t		colorBlue;		//!< Blue color coefficient. Unit is unspecified
};

////////////////////////////////////////////////////////////////////
//
//	e57::Data3D
//

//! The e57::Data3D is a structure that stores the top-level information for a single lidar scan
class Data3D {
public:
//! This function is the constructor for the Data3D class
					Data3D(void);
//! This function is the destructor for the Data3D class
					~Data3D(void);

	ustring			name;					//!< A user-defined name for the Data3D.
	ustring			guid;					//!< A globally unique identification string for the current version of the Data3D object
	vector<ustring>	originalGuids;			//!< A vector of globally unique identification Strings from which the points in this Data3D originated.
	ustring			description;			//!< A user-defined description of the Image

	ustring			sensorVendor;			//!< The name of the manufacturer for the sensor used to collect the points in this Data3D.
	ustring			sensorModel;			//!< The model name or number for the sensor.
	ustring			sensorSerialNumber;		//!< The serial number for the sensor.
	ustring			sensorHardwareVersion;	//!< The version number for the sensor hardware at the time of data collection.
	ustring			sensorSoftwareVersion;	//!< The version number for the software used for the data collection.
	ustring			sensorFirmwareVersion;	//!< The version number for the firmware installed in the sensor at the time of data collection.

	float			temperature;			//!< The ambient temperature at the time of data collection.
	float			relativeHumidity;		//!< The relative humidity at the time of data collection.
	float			atmosphericPressure;	//!< The air pressure at the time of data collection.

	e57::DateTime	acquisitionStart;		//!< The start date and time that the data was recorded
	e57::DateTime	acquisitionEnd;			//!< The end date and time that the data was recorded

	e57::RigidBodyTransform		pose;		//!< A rigid body transform that describes the coordinate frame of the 3D imaging system origin in the file-level coordinate system.
	e57::CartesianBounds		cartesianBounds; //!< The bounding box (in Cartesian coordinates) of all the points in this Data3D (in the local coordinate system of the points).
	e57::SphericalBounds		sphericalBounds; //!< The bounding region (in spherical coordinates) of all the points in this Data3D (in the local coordinate system of the points)

	e57::PointGroupingSchemes	pointGroupingSchemes;	//!< The defined schemes that group points in different ways
	e57::PointStandardizedFieldsAvailable pointFields;	//!< This defines the active fields used in the WritePoints function.

	int64_t						row;			//!< data3D row size
	int64_t						column;			//!< data3D column size
	int64_t						pointsSize;		//!< Total size of the compressed vector of PointRecord structures referring to the binary data that actually stores the point data
};


////////////////////////////////////////////////////////////////////
//
//	e57::PinholeProjection
//

//! The e57::PinholeProjection is a structure that stores an image that is mapped from 3D using the pinhole camera projection model.

class PinholeProjection
{
	int				imageWidth;		//!< The image width (in pixels). Shall be positive
	int				imageHeight;	//!< The image height (in pixels). Shall be positive
	double			focalLength;	//!< The camera�s focal length (in meters). Shall be positive
	double			pixelWidth;		//!< The width of the pixels in the camera (in meters). Shall be positive
	double			pixelHeight;	//!< The height of the pixels in the camera (in meters). Shall be positive
	double			principalPointX;//!< The X coordinate in the image of the principal point, (in pixels). The principal point is the intersection of the z axis of the camera coordinate frame with the image plane.
	double			principalPointY;//!< The Y coordinate in the image of the principal point (in pixels).
};
////////////////////////////////////////////////////////////////////
//
//	e57::SphericalProjection
//

//! The e57::SphericalProjection is a structure that stores an image that is mapped from 3D using a spherical projection model

class SphericalProjection
{
	int				imageWidth;		//!< The image width (in pixels). Shall be positive
	int				imageHeight;	//!< The image height (in pixels). Shall be positive
	double			pixelWidth;		//!< The width of the pixels in the camera (in meters). Shall be positive
	double			pixelHeight;	//!< The height of the pixels in the camera (in meters). Shall be positive
	double			azimuthStart;	//!< The azimuth angle (in radians) of the left side of the image. Shall be in the interval [-PI,PI].
	double			elevationStart; //!< The elevation angle (in radians) of the top of the image. Shall be in the interval [-PI/2, PI/2].
};

////////////////////////////////////////////////////////////////////
//
//	e57::CylindricalProjection
//

//! The e57::CylindricalProjection is a structure that stores an image that is mapped from 3D using a cylindrical projection model.

class CylindricalProjection
{
	int				imageWidth;		//!< The image width (in pixels). Shall be positive
	int				imageHeight;	//!< The image height (in pixels). Shall be positive
	double			pixelWidth;		//!< The width of the pixels in the camera (in meters). Shall be positive
	double			pixelHeight;	//!< The height of the pixels in the camera (in meters). Shall be positive
	double			radius;			//!< The closest distance from the cylindrical image surface to the center of projection (that is, the radius of the cylinder) (in meters). Shall be non-negative
	double			azimuthStart;	//!< The azimuth angle (in radians) of the left side of the image. Shall be in the interval [PI,PI].
	double			principalPointY;//!< The Y coordinate in the image of the principal point (in pixels). This is the intersection of the z = 0 plane with the image
};

////////////////////////////////////////////////////////////////////
//
//	e57::CameraImage
//

//! The e57::CameraImage is a structure that stores an image from a camera
class CameraImage {
public:
//! This function is the constructor for the CameraImage class
					CameraImage(void);
//! This function is the destructor for the CameraImage class
					~CameraImage(void);

	ustring			name;					//!< A user-defined name for the CameraImage.
	ustring			guid;					//!< A globally unique identification string for the current version of the CameraImage object
	ustring			description;			//!< A user-defined description of the CameraImage
	e57::DateTime	acquisitionDateTime;	//!< The date and time that the image was taken

	ustring			associatedData3DGuid;	//!< The globally unique identification string (guid element) for the Data3D that was being acquired when the picture was taken

	ustring			sensorVendor;			//!< The name of the manufacturer for the sensor used to collect the points in this Data3D.
	ustring			sensorModel;			//!< The model name or number for the sensor.
	ustring			sensorSerialNumber;		//!< The serial number for the sensor.

	e57::RigidBodyTransform				pose;	//!< A rigid body transform that describes the coordinate frame of the camera in the file-level coordinate system
	
//	e57::VisualReferenceRepresentation	visualReferenceRepresentation;  //!< Representation for an image that does not define any camera projection model. The image is to be used for visual reference only
	e57::PinholeProjection				pinholeRepresentation;			//!< Representation for an image using the pinhole camera projection model
	e57::SphericalProjection			sphericalRepresentation;		//!< Representation for an image using the spherical camera projection model.
	e57::CylindricalProjection			cylindricalRepresentation;		//!< Representation for an image using the cylindrical camera projection model
};

////////////////////////////////////////////////////////////////////
//
//	e57::Reader
//

//! This is the E57 Reader class

class	Reader {

private:

	ImageFile		m_imf;
	StructureNode	m_root;

	VectorNode		m_data3D;
	VectorNode		m_cameraImages;

public:

//! This function returns an E57::Reader pointer and opens the file
static e57::Reader* CreateReader(
						const ustring & filePath	//!< file path string
						);	//!< /return This returns a e57::Reader object which should be deleted when finish

//! This function is the constructor for the reader class
					Reader(
						const ustring & filePath		//!< file path string
						);

//! This function is the destructor for the reader class
virtual				~Reader(void);

//! This function returns true if the file is open
virtual	bool		IsOpen(void);

//! This function closes the file
virtual	bool		Close(void);

////////////////////////////////////////////////////////////////////
//
//	File information
//
//! This function returns the file header information
virtual bool		GetE57Root(
						E57Root * fileHeader	//!< This is the main header information
					    );	//!< /return Returns true if sucessful

////////////////////////////////////////////////////////////////////
//
//	Camera Image picture data
//
//! This function returns the total number of Picture Blocks
virtual	int32_t		GetCameraImageCount( void);

//! This function returns the cameraImages header and positions the cursor
virtual bool		GetCameraImage( 
						int32_t			imageIndex,		//!< This in the index into the cameraImages vector
						CameraImage *	cameraImageHeader	//!< pointer to the CameraImage structure to receive the picture information
						);						//!< /return Returns true if sucessful

//! This function reads the block
virtual	int64_t		ReadCameraImageData(
						int32_t		imageIndex,		//!< picture block index
						void *		pBuffer,	//!< pointer the buffer
						int64_t		start,		//!< position in the block to start reading
						int64_t		count		//!< size of desired chuck or buffer size
						);						//!< /return Returns the number of bytes transferred.

////////////////////////////////////////////////////////////////////
//
//	Scanner Image 3d data
//
//! This function returns the total number of Image Blocks
virtual	int32_t		GetData3DCount( void);

//! This function returns the Data3D header and positions the cursor
virtual bool		GetData3D( 
						int32_t		dataIndex,	//!< This in the index into the images3D vector
						Data3D *	data3DHeader //!< pointer to the Data3D structure to receive the image information
						);	//!< /return Returns true if sucessful

//! This function returns the size of the point data
virtual	void		GetData3DPointSize(
						int32_t		dataIndex,	//!< image block index
						int32_t *	row,		//!< image row size
						int32_t *	column		//!< image column size
						);

//! This function returns the number of point groups
virtual int32_t		GetData3DGroupSize(
						int32_t		dataIndex		//!< image block index
						);

//! This function returns the active fields available
virtual	bool		GetData3DStandardizedFieldsAvailable(
						int32_t		dataIndex,		//!< This in the index into the images3D vector
						PointStandardizedFieldsAvailable * pointFields //!< pointer to the Data3D structure to receive the PointStandardizedFieldsAvailable information
						);

//! This function returns the point data fields fetched in single call
//* All the non-NULL buffers in the call below have number of elements = count */

virtual int64_t		GetData3DStandardPoints(
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
						);

//! This function interrogate what fields (standardized and extensions) are available
virtual bool		GetData3DGeneralFieldsAvailable(
						int32_t					dataIndex,
						std::vector<ustring>&	fieldsAvailable);

virtual int64_t		GetData3DGeneralPoints(
						int32_t				dataIndex,
						int64_t				startPointIndex,
						int64_t				pointCount,
						bool*				isValid,
						vector<ustring>&	numericFieldNames,
						vector<double*>&	numericBuffers,
						vector<ustring>&	stringFieldNames,
						vector<ustring*>&	stringBuffers);

}; //end Reader class


////////////////////////////////////////////////////////////////////
//
//	e57::Writer
//

//! This is the E57 Writer class

class	Writer{

private:
	ImageFile				m_imf;
	StructureNode			m_root;

	VectorNode				m_data3D;

	Data3D					m_data3DHeader;
    vector<int32_t>			m_idElementValue;
    vector<int32_t>			m_startPointIndex;
    vector<int32_t>			m_pointCount;

	VectorNode				m_cameraImages;

public:

//! This function returns an E57::Writer pointer and opens the file
static e57::Writer* CreateWriter(
						const ustring & filePath,	//!< file path string
						const ustring & coordinateMetaData	//!< Information describing the Coordinate Reference System to be used for the file
						);	//!< /return This returns a e57::Writer object which should be deleted when finish

//! This function is the constructor for the writer class
					Writer(
						const ustring & filePath,		//!< file path string
						const ustring & coordinateMetaData	//!< Information describing the Coordinate Reference System to be used for the file
						);

//! This function is the destructor for the writer class
virtual				~Writer(void);

//! This function returns true if the file is open
virtual	bool		IsOpen(void);

//! This function closes the file
virtual	bool		Close(void);

////////////////////////////////////////////////////////////////////
//
//	Camera Image picture data
//

//! This function sets up the cameraImages header and positions the cursor
//* The user needs to config a CameraImage structure with all the camera information before making this call. */
virtual int32_t		NewCameraImage( 
						CameraImage &	cameraImageHeader	//!< pointer to the CameraImage structure to receive the picture information
						);						//!< /return Returns the cameraImage index

//! This function writes the block
virtual	int64_t		WriteCameraImage(
						int32_t		imageIndex,	//!< picture block index given by the NewCameraImage
						void *		pBuffer,	//!< pointer the buffer
						int64_t		start,		//!< position in the block to start writing
						int64_t		count		//!< size of desired chuck or buffer size
						);						//!< /return Returns the number of bytes written

//! This function closes the CameraImage block
virtual bool		CloseCameraImage(
						int32_t		imageIndex	//!< picture block index given by the NewCameraImage
)						;						//!< /return Returns true if successful, false otherwise

//! This function sets up the Data3D header and positions the cursor for the binary data
//* The user needs to config a Data3D structure with all the scanning information before making this call. */

virtual int32_t		NewData3D( 
						Data3D &	data3DHeader //!< pointer to the Data3D structure to receive the image information
						);						//!< /return Returns the index of the new scan's data3D block.

//! This function writes out blocks of point data
virtual int64_t		WriteData3DStandardPoints(
						int32_t		dataIndex,			//!< data block index given by the NewData3D
						int64_t		idElementValue,		//!< index for this group
						int64_t		startPointIndex,	//!< Starting index in to the "points" data vector
						int64_t		count,				//!< size of each of the buffers given
						bool*		isValid,			//!< pointer to a buffer with the valid indication
						int32_t*	rowIndex,			//!< pointer to a buffer with the row index
						int32_t*	columnIndex,		//!< pointer to a buffer with the column index
						int32_t*	returnIndex,		//!< pointer to a buffer with the return index
						int32_t*	returnCount,		//!< pointer to a buffer with the return count data
						double*		x,					//!< pointer to a buffer with the x data
						double*		y,					//!< pointer to a buffer with the y data
						double*		z,					//!< pointer to a buffer with the z data
						double*		range,				//!< pointer to a buffer with the range data
						double*		azimuth,			//!< pointer to a buffer with the azimuth angle
						double*		elevation,			//!< pointer to a buffer with the elevation angle
						double*		intensity,			//!< pointer to a buffer with the lidar return intesity
						double*		colorRed,			//!< pointer to a buffer with the color red data
						double*		colorGreen,			//!< pointer to a buffer with the color green data
						double*		colorBlue,			//!< pointer to a buffer with the color blue data
						double*		timestamp			//!< pointer to a buffer with the time stamp data
						);

//! This function sets the extensions field that will be available
virtual bool		SetData3DGeneralFieldsAvailable(
						int32_t					dataIndex,	//!< data block index given by the NewData3D
						std::vector<ustring>&	fieldsAvailable);

//! This function writes the General data point information
virtual int64_t		WriteData3DGeneralPoints(
						int32_t				dataIndex,	//!< data block index given by the NewData3D
						int64_t				startPointIndex,
						int64_t				pointCount,
						bool*				isValid,
						vector<ustring>&	numericFieldNames,
						vector<double*>&	numericBuffers,
						vector<ustring>&	stringFieldNames,
						vector<ustring*>&	stringBuffers);

//! This function closes the data3D block
virtual bool		CloseData3D(
						int32_t		dataIndex	//!< data block index given by the NewData3D
						);						//!< /return Returns true if sucessful, false otherwise

}; //end Writer class

}; //end namespace
#endif
