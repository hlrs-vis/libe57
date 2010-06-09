//////////////////////////////////////////////////////////////////////////
//
//  E57Simple.h - public header of E57 Simple API for reading/writing .e57 files.
//
//	Copyright (c) 2010 Stan Coleby (scoleby@intelisum.com)
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
//	V5		May 12, 2010	Stan Coleby		scoleby@intelisum.com
//	V6		June 8, 2010	Stan Coleby		scoleby@intelisum.com
//
//////////////////////////////////////////////////////////////////////////

//! @file E57Simple.h

#ifndef E57SIMPLE_H_INCLUDED
#define E57SIMPLE_H_INCLUDED

#ifndef E57FOUNDATION_H_INCLUDED
#include "E57Foundation.h"
#endif

using namespace std;
using namespace boost;

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

namespace e57 {

class ReaderImpl;
class WriterImpl;

////////////////////////////////////////////////////////////////////
//
//	e57::Point
//

//! @brief The e57::Translation defines a rigid body translation in Cartesian coordinates.
class Translation {
public:
	double		x;	//!< The X coordinate of the translation (in meters)
	double		y;	//!< The Y coordinate of the translation (in meters)
	double		z;	//!< The Z coordinate of the translation (in meters)
};
////////////////////////////////////////////////////////////////////
//
//	e57::Quaternion
//
//! @brief The e57::Quaternion is a quaternion which represents a rigid body rotation.

class Quaternion {
public:
	double		w;	//!< The real part of the quaternion. Shall be nonnegative
	double		x;	//!< The i coefficient of the quaternion
	double		y;	//!< The j coefficient of the quaternion
	double		z;	//!< The k coefficient of the quaternion
};

////////////////////////////////////////////////////////////////////
//
//	e57::RigidBodyTransform
//
//! @brief The e57::RigidBodyTransform is a structure that defines a rigid body transform in cartesian coordinates.

class RigidBodyTransform {
public:
	e57::Quaternion		rotation;		//!< A unit quaternion representing the rotation, R, of the transform
	e57::Translation	translation;	//!< The translation point vector, t, of the transform
};

////////////////////////////////////////////////////////////////////
//
//	e57::CartesianBounds
//
//! @brief The e57::CartesianBounds structure specifies an axis-aligned box in local cartesian coordinates.

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
//! @brief The e57::SphericalBounds structure stores the bounds of some data in spherical coordinates.

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
//	e57::IndexBounds
//
//! @brief The e57::IndexBounds structure stores the minimum and maximum 669 of rowIndex, columnIndex, and returnIndex fields for a set of points.

class IndexBounds {
public:
	int64_t		rowMinimum;		//!< The minimum rowIndex value of any point represented by this IndexBounds object.
	int64_t		rowMaximum;		//!< The maximum rowIndex value of any point represented by this IndexBounds object.
	int64_t		columnMinimum;	//!< The minimum columnIndex value of any point represented by this IndexBounds object.
	int64_t		columnMaximum;	//!< The maximum columnIndex value of any point represented by this IndexBounds object.
	int64_t		returnMinimum;	//!< The minimum returnIndex value of any point represented by this IndexBounds object.
	int64_t		returnMaximum;	//!< The maximum returnIndex value of any point represented by this IndexBounds object.
};
////////////////////////////////////////////////////////////////////
//
//	e57::DateTime
//
//! @brief The e57::DateTime is a structure for encoding date and time. 
/*! @details The date and time is encoded using a single
	562 floating point number, stored as an E57 Float element which is based on the Global Positioning
	563 System (GPS) time scale. */

class DateTime {
public:
	double		dateTimeValue;		//!< The time, in seconds, since GPS time was zero. This time specification may include fractions of a second.
	int32_t		isAtomicClockReferenced;	//!< This element should be present, and its value set to 1 if, and only if, the time stored in the dateTimeValue element is obtained from an atomic clock time source. Shall be either 0 or 1.
};

////////////////////////////////////////////////////////////////////
//
//	e57::E57Root
//

//! @brief The e57::E57Root is a structure that stores the top-level information for the XML section of the file.

class E57Root {
public:
//! @brief This function is the constructor for the images3D class
					E57Root(void);
//! @brief This function is the destructor for the reader class
					~E57Root(void);

	ustring			formatName;			//!< Contains the string �ASTM E57 3D Image File�
	ustring			guid;				//!< A globally unique identification string for the current version of the file
	uint32_t		versionMajor;		//!< Major version number, should be 1
	uint32_t		versionMinor;		//!< Minor version number, should be 0
	ustring			e57libraryVersion;	//!< The version identifier for the E57 file format library that wrote the file.
	e57::DateTime	creationDateTime;	//!< Date/time that the file was created
	int32_t			data3DSize;			//!< Size of the Data3D vector for storing 3D imaging data
	int32_t			image2DSize;		//!< Size of the A heterogeneous Vector of Image2D Structures for storing 2D images from a camera or similar device.
	ustring			coordinateMetadata;	//!< Information describing the Coordinate Reference System to be used for the file
};

////////////////////////////////////////////////////////////////////
//
//	e57::LineGroupRecord
//
//! @brief The e57::LineGroupRecord is a structure that stores information about a single group of points in a row or column

class LineGroupRecord {
public:
	int64_t					idElementValue;		//!< The value of the identifying element of all members in this group. Shall be in the interval [0, 2^63).
	int64_t					startPointIndex;	//!< The record number of the first point in the continuous interval. Shall be in the interval [0, 2^63).
	int64_t					pointCount;			//!< The number of PointRecords in the group. Shall be in the interval [1, 2^63). May be zero.
	e57::CartesianBounds	cartesianBounds;	//!< The bounding box (in Cartesian coordinates) of all points in the group (in the local coordinate system of the points).
	e57::SphericalBounds	sphericalBounds;	//!< The bounding region (in spherical coordinates) of all the points in the group (in the local coordinate system of the points).
};

////////////////////////////////////////////////////////////////////
//
//	e57::GroupingByLine
//
//! @brief The e57::GroupingByLine is a structure that stores a set of point groups organized by the rowIndex or columnIndex attribute of the PointRecord

class GroupingByLine {
public:
	ustring		idElementName;		//!< The name of the PointRecord element that identifies which group the point is in. The value of this string must be �rowIndex� or �columnIndex�
	int64_t		groupsSize;			//!< Size of the groups compressedVector of LineGroupRecord structures
};

////////////////////////////////////////////////////////////////////
//
//	e57::PointGroupingSchemes
//
//! @brief The e57::PointGroupingSchemes structure structure supports the division of points within an Data3D into logical groupings

class PointGroupingSchemes {
public:
	e57::GroupingByLine	groupingByLine;	//!< Grouping information by row or column index
};

////////////////////////////////////////////////////////////////////
//
//	e57::PointStandardizedFieldsAvailable
//
//! @brief The e57::PointStandardizedFieldsAvailable is a structure use to interrogate if standardized fields are available

class PointStandardizedFieldsAvailable {
public:
	bool	valid;				//!< indicates that the PointRecord valid field is active
	bool	x;					//!< indicates that the PointRecord cartesianX field is active
	bool	y;					//!< indicates that the PointRecord cartesianY field is active
	bool	z;					//!< indicates that the PointRecord cartesianZ field is active
	bool	range;				//!< indicates that the PointRecord sphericalRange field is active
	bool	azimuth;			//!< indicates that the PointRecord sphericalAzimuth field is active
	bool	elevation;			//!< indicates that the PointRecord sphericalElevation field is active
	bool	rowIndex;			//!< indicates that the PointRecord rowIndex field is active
	bool	columnIndex;		//!< indicates that the PointRecord columnIndex field is active
	bool	returnIndex;		//!< indicates that the PointRecord returnIndex field is active
	bool	returnCount;		//!< indicates that the PointRecord returnCount field is active
	bool	timeStamp;			//!< indicates that the PointRecord timeStamp field is active
	bool	intensity;			//!< indicates that the PointRecord intensity field is active
	bool	colorRed;			//!< indicates that the PointRecord colorRed field is active
	bool	colorGreen;			//!< indicates that the PointRecord colorGreen field is active
	bool	colorBlue;			//!< indicates that the PointRecord colorBlue field is active
};

////////////////////////////////////////////////////////////////////
//
//	e57::PointRecord
//
//! @brief The e57::PointRecord is a structure that stores the information for an individual 3D imaging system point measurement.
/*! @details This structure is not actually used by is here for completeness.
*/

class PointRecord {
public:
	double		cartesianX;		//!< The X coordinate (in meters) of the point in Cartesian coordinates
	double		cartesianY;		//!< The Y coordinate (in meters) of the point in Cartesian coordinates
	double		cartesianZ;		//!< The Z coordinate (in meters) of the point in Cartesian coordinates
	double		sphericalRange;	//!< The range (in meters) of points in spherical coordinates. Shall be non-negative
	double		sphericalAzimuth; //!< Azimuth angle (in radians) of point in spherical coordinates
	double		sphericalElevation;	//!< Elevation angle (in radians) of point in spherical coordinates
	bool		valid;			//!< Value = 1 if the point is considered valid, 0 otherwise
	int32_t		rowIndex;		//!< The row number of point (zero based). This is useful for data that is stored in a regular grid.Shall be in the interval [0, 2^63).
	int32_t		columnIndex;	//!< The column number of point (zero based). This is useful for data that is stored in a regular grid. Shall be in the interval [0, 2^63)
	int32_t		returnIndex;	//!< Only for multi-return sensors. The number of this return (zero based). That is, 0 is the first return, 1 is the second, and so on. Shall be in the interval [0, returnCount).
	int32_t		returnCount;	//!< Only for multi-return sensors. The total number of returns for the pulse that this corresponds to. Shall be in the interval (0, 2^63).
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

//! @brief The e57::Data3D is a structure that stores the top-level information for a single lidar scan
class Data3D {
public:
//! @brief This function is the constructor for the Data3D class
					Data3D(void);
//! @brief This function is the destructor for the Data3D class
					~Data3D(void);

	ustring			name;					//!< A user-defined name for the Data3D.
	ustring			guid;					//!< A globally unique identification string for the current version of the Data3D object
	std::vector<ustring>	originalGuids;			//!< A vector of globally unique identification Strings from which the points in this Data3D originated.
	ustring			description;			//!< A user-defined description of the Image

	ustring			sensorVendor;			//!< The name of the manufacturer for the sensor used to collect the points in this Data3D.
	ustring			sensorModel;			//!< The model name or number for the sensor.
	ustring			sensorSerialNumber;		//!< The serial number for the sensor.
	ustring			sensorHardwareVersion;	//!< The version number for the sensor hardware at the time of data collection.
	ustring			sensorSoftwareVersion;	//!< The version number for the software used for the data collection.
	ustring			sensorFirmwareVersion;	//!< The version number for the firmware installed in the sensor at the time of data collection.

	float			temperature;			//!< The ambient temperature, measured at the sensor, at the time of data collection (in degrees Celsius). Shall be ? ?273.15� (absolute zero).
	float			relativeHumidity;		//!< The percentage relative humidity, measured at the sensor, at the time of data collection. Shall be in the interval [0, 100].
	float			atmosphericPressure;	//!< The atmospheric pressure, measured at the sensor, at the time of data collection (in Pascals). Shall be positive.

	e57::DateTime	acquisitionStart;		//!< The start date and time that the data was acquired.
	e57::DateTime	acquisitionEnd;			//!< The end date and time that the data was acquired.

	e57::RigidBodyTransform		pose;		//!< A rigid body transform that describes the coordinate frame of the 3D imaging system origin in the file-level coordinate system.
	e57::IndexBounds			indexBounds;	//!< The bounds of the row, column, and return number of all the points in this Data3D.
	e57::CartesianBounds		cartesianBounds; //!< The bounding region (in cartesian coordinates) of all the points in this Data3D (in the local coordinate system of the points).
	e57::SphericalBounds		sphericalBounds; //!< The bounding region (in spherical coordinates) of all the points in this Data3D (in the local coordinate system of the points).

	e57::PointGroupingSchemes	pointGroupingSchemes;	//!< The defined schemes that group points in different ways
	e57::PointStandardizedFieldsAvailable pointFields;	//!< This defines the active fields used in the WritePoints function.

	int64_t			pointsSize;				//!< Total size of the compressed vector of PointRecord structures referring to the binary data that actually stores the point data
};

////////////////////////////////////////////////////////////////////
//
//	e57::VisualReferenceRepresentation
//

//! @brief The e57::VisualReferenceRepresentation is a structure that stores an image that is to be used only as a visual reference.

class VisualReferenceRepresentation
{
public:
	int64_t			jpegImage;		//!< Size of JPEG format image data in BlobNode.
	int64_t			pngImage;		//!< Size of PNG format image data in BlobNode.
	int64_t			imageMask;		//!< Size of PNG format image mask in BlobNode.
	int32_t			imageWidth;		//!< The image width (in pixels). Shall be positive
	int32_t			imageHeight;	//!< The image height (in pixels). Shall be positive
};

////////////////////////////////////////////////////////////////////
//
//	e57::PinholeRepresentation
//

//! @brief The e57::PinholeRepresentation is a structure that stores an image that is mapped from 3D using the pinhole camera projection model.

class PinholeRepresentation
{
public:
	int64_t			jpegImage;		//!< Size of JPEG format image data in BlobNode.
	int64_t			pngImage;		//!< Size of PNG format image data in BlobNode.
	int64_t			imageMask;		//!< Size of PNG format image mask in BlobNode.
	int32_t			imageWidth;		//!< The image width (in pixels). Shall be positive
	int32_t			imageHeight;	//!< The image height (in pixels). Shall be positive
	double			focalLength;	//!< The camera's focal length (in meters). Shall be positive
	double			pixelWidth;		//!< The width of the pixels in the camera (in meters). Shall be positive
	double			pixelHeight;	//!< The height of the pixels in the camera (in meters). Shall be positive
	double			principalPointX;//!< The X coordinate in the image of the principal point, (in pixels). The principal point is the intersection of the z axis of the camera coordinate frame with the image plane.
	double			principalPointY;//!< The Y coordinate in the image of the principal point (in pixels).
};
////////////////////////////////////////////////////////////////////
//
//	e57::SphericalRepresentation
//

//! @brief The e57::SphericalRepresentation is a structure that stores an image that is mapped from 3D using a spherical projection model

class SphericalRepresentation
{
public:
	int64_t			jpegImage;		//!< Size of JPEG format image data in BlobNode.
	int64_t			pngImage;		//!< Size of PNG format image data in BlobNode.
	int64_t			imageMask;		//!< Size of PNG format image mask in BlobNode.
	int32_t			imageWidth;		//!< The image width (in pixels). Shall be positive
	int32_t			imageHeight;	//!< The image height (in pixels). Shall be positive
	double			pixelWidth;		//!< The width of a pixel in the image (in radians). Shall be positive
	double			pixelHeight;	//!< The height of a pixel in the image (in radians). Shall be positive.
};

////////////////////////////////////////////////////////////////////
//
//	e57::CylindricalRepresentation
//

//! @brief The e57::CylindricalRepresentation is a structure that stores an image that is mapped from 3D using a cylindrical projection model.

class CylindricalRepresentation
{
public:
	int64_t			jpegImage;		//!< Size of JPEG format image data in Blob.
	int64_t			pngImage;		//!< Size of PNG format image data in Blob.
	int64_t			imageMask;		//!< Size of PNG format image mask in Blob.
	int32_t			imageWidth;		//!< The image width (in pixels). Shall be positive
	int32_t			imageHeight;	//!< The image height (in pixels). Shall be positive
	double			pixelWidth;		//!< The width of a pixel in the image (in radians). Shall be positive
	double			pixelHeight;	//!< The height of a pixel in the image (in meters). Shall be positive
	double			radius;			//!< The closest distance from the cylindrical image surface to the center of projection (that is, the radius of the cylinder) (in meters). Shall be non-negative
	double			principalPointY;//!< The Y coordinate in the image of the principal point (in pixels). This is the intersection of the z = 0 plane with the image
};

////////////////////////////////////////////////////////////////////
//
//	e57::Image2D
//

//! @brief The e57::Image2D is a structure that stores an image from a camera
class Image2D {
public:
//! @brief This function is the constructor for the Image2D class
					Image2D(void);
//! @brief This function is the destructor for the Image2D class
					~Image2D(void);

	ustring			name;					//!< A user-defined name for the Image2D.
	ustring			guid;					//!< A globally unique identification string for the current version of the Image2D object
	ustring			description;			//!< A user-defined description of the Image2D
	e57::DateTime	acquisitionDateTime;	//!< The date and time that the image was taken

	ustring			associatedData3DGuid;	//!< The globally unique identification string (guid element) for the Data3D that was being acquired when the picture was taken

	ustring			sensorVendor;			//!< The name of the manufacturer for the sensor used to collect the points in this Data3D.
	ustring			sensorModel;			//!< The model name or number for the sensor.
	ustring			sensorSerialNumber;		//!< The serial number for the sensor.

	e57::RigidBodyTransform				pose;	//!< A rigid body transform that describes the coordinate frame of the camera in the file-level coordinate system
	
	e57::VisualReferenceRepresentation	visualReferenceRepresentation;  //!< Representation for an image that does not define any camera projection model. The image is to be used for visual reference only
	e57::PinholeRepresentation			pinholeRepresentation;			//!< Representation for an image using the pinhole camera projection model
	e57::SphericalRepresentation		sphericalRepresentation;		//!< Representation for an image using the spherical camera projection model.
	e57::CylindricalRepresentation		cylindricalRepresentation;		//!< Representation for an image using the cylindrical camera projection model
};

////////////////////////////////////////////////////////////////////
//
//	e57::Image2DType
//
//! @brief The e57::Image2DType identifies the format representation for the image data
enum Image2DType {
	E57_NO_IMAGE = 0,		//!< No image data
    E57_JPEG_IMAGE = 1,		//!< JPEG format image data.
    E57_PNG_IMAGE = 2,		//!< PNG format image data.
	E57_PNG_IMAGE_MASK = 3	//!< PNG format image mask.
};

////////////////////////////////////////////////////////////////////
//
//	e57::Image2DProjection
//
//! @brief The e57::Image2DProjection identifies the representation for the image data
enum Image2DProjection {
	E57_NO_PROJECTION = 0,	//!< No representation for the image data is present
    E57_VISUAL = 1,			//!< VisualReferenceRepresentation for the image data
    E57_PINHOLE = 2,		//!< PinholeRepresentation for the image data
	E57_SPHERICAL = 3,		//!< SphericalRepresentation for the image data
	E57_CYLINDRICAL = 4		//!< CylindricalRepresentation for the image data
};

////////////////////////////////////////////////////////////////////
//
//	e57::Reader
//

//! @brief This is the E57 Reader class

class	Reader {
public:

//! @brief This function is the constructor for the reader class
				Reader(
					const ustring & filePath		//!< file path string
					);

//! @brief This function returns true if the file is open
	bool		IsOpen(void) const;

//! @brief This function closes the file
	bool		Close(void) const;

////////////////////////////////////////////////////////////////////
//
//	File information
//
//! @brief This function returns the file header information
	bool		GetE57Root(
						E57Root & fileHeader	//!< This is the main header information
					    ) const;	//!< @return Returns true if sucessful

////////////////////////////////////////////////////////////////////
//
//	Camera Image 2D picture data
//
//! @brief This function returns the total number of Picture Blocks
	int32_t		GetImage2DCount( void) const;	//!< @return Returns the number of Image2D blocks

//! @brief This function returns the image2D header and positions the cursor
	bool		ReadImage2D( 
						int32_t		imageIndex,		//!< This in the index into the image2D vector
						Image2D &	image2DHeader	//!< pointer to the Image2D structure to receive the picture information
						) const;					//!< @return Returns true if sucessful

//! @brief This function returns the size of the image data
/*! @details The e57::Image2DType identifies the format representation for the image data
<tt><PRE>
enum Image2DType {
	E57_NO_IMAGE = 0,	//!< No image data
	E57_JPEG_IMAGE = 1,	//!< JPEG format image data.
	E57_PNG_IMAGE = 2,	//!< PNG format image data.
	E57_PNG_IMAGE_MASK = 3	//!< PNG format image mask.
};
</PRE></tt>
*/
/*! The e57::Image2DProjection identifies the representation for the image data
<tt><PRE>
enum Image2DProjection {
	E57_NO_PROJECTION = 0,	//!< No representation for the image data is present
    E57_VISUAL = 1,		//!< VisualReferenceRepresentation for the image data
    E57_PINHOLE = 2,	//!< PinholeRepresentation for the image data
	E57_SPHERICAL = 3,	//!< SphericalRepresentation for the image data
	E57_CYLINDRICAL = 4	//!< CylindricalRepresentation for the image data
};
</PRE></tt>
*/
	bool		GetImage2DSizes(
						int32_t					imageIndex,		//!< This in the index into the image2D vector
						e57::Image2DProjection&	imageProjection,//!< identifies the projection in the image2D.
						e57::Image2DType &		imageType,		//!< identifies the image format of the projection.
						int64_t &				imageWidth,		//!< The image width (in pixels).
						int64_t &				imageHeight,	//!< The image height (in pixels).
						int64_t &				imageSize,		//!< This is the total number of bytes for the image blob.
						e57::Image2DType &		imageMaskType,	//!< This is E57_PNG_IMAGE_MASK if "imageMask" is defined in the projection
						e57::Image2DType &		imageVisualType	//!< This is image type of the VisualReferenceRepresentation if given.
						) const;								//!< @return Returns true if sucessful

//! @brief This function reads the block
	int64_t		ReadImage2DData(
						int32_t					imageIndex,		//!< picture block index
						e57::Image2DProjection	imageProjection,//!< identifies the projection desired.
						e57::Image2DType		imageType,		//!< identifies the image format desired.
						void *					pBuffer,		//!< pointer the raw image buffer
						int64_t					start,			//!< position in the block to start reading
						int64_t					count			//!< size of desired chuck or buffer size
						) const;								//!< @return Returns the number of bytes transferred.

////////////////////////////////////////////////////////////////////
//
//	Scanner 3d data
//
//! @brief This function returns the total number of Data3D Blocks
	int32_t		GetData3DCount( void) const; //!< @return Returns number of Data3D blocks.

//! @brief This function returns the Data3D header and positions the cursor
	bool		ReadData3D( 
						int32_t		dataIndex,	//!< This in the index into the images3D vector
						Data3D &	data3DHeader //!< pointer to the Data3D structure to receive the image information
						) const;	//!< @return Returns true if sucessful

//! @brief This function returns the size of the point data
	bool		GetData3DSizes(
						int32_t		dataIndex,	//!< This in the index into the images3D vector
						int64_t &	rowMax,		//!< This is the maximum row size
						int64_t &	columnMax,	//!< This is the maximum column size
						int64_t &	pointsSize,	//!< This is the total number of point records
						int64_t &	groupsSize	//!< This is the total number of group reocrds
						) const;				//!< @return Return true if sucessful, false otherwise

//! @brief This funtion writes out the group data.
	bool		ReadData3DGroupsData(
						int32_t		dataIndex,			//!< data block index given by the NewData3D
						int64_t		groupCount,			//!< size of each of the buffers given
						int64_t*	idElementValue,		//!< index for this group
						int64_t*	startPointIndex,	//!< Starting index in to the "points" data vector for the groups
						int64_t*	pointCount			//!< size of the groups given
						) const;						//!< @return Return true if sucessful, false otherwise

//! @brief This function sets up the point data fields 
/*! @details All the non-NULL buffers in the call below have number of elements = pointCount.
Call the CompressedVectorReader::read() until all data is read.
*/
	CompressedVectorReader	SetUpData3DPointsData(
						int32_t		dataIndex,			//!< data block index given by the NewData3D
						int64_t		pointCount,			//!< size of each element buffer.
						int32_t*	valid,				//!< Value = 1 if the point is considered valid, 0 otherwise
						double*		cartesianX,			//!< pointer to a buffer with the X coordinate (in meters) of the point in Cartesian coordinates
						double*		cartesianY,			//!< pointer to a buffer with the Y coordinate (in meters) of the point in Cartesian coordinates
						double*		cartesianZ,			//!< pointer to a buffer with the Z coordinate (in meters) of the point in Cartesian coordinates
						double*		intensity = NULL,	//!< pointer to a buffer with the Point response intensity. Unit is unspecified
						double*		colorRed = NULL,	//!< pointer to a buffer with the Red color coefficient. Unit is unspecified
						double*		colorGreen = NULL,	//!< pointer to a buffer with the Green color coefficient. Unit is unspecified
						double*		colorBlue = NULL,	//!< pointer to a buffer with the Blue color coefficient. Unit is unspecified
						double*		sphericalRange = NULL,		//!< pointer to a buffer with the range (in meters) of points in spherical coordinates. Shall be non-negative
						double*		sphericalAzimuth = NULL,	//!< pointer to a buffer with the Azimuth angle (in radians) of point in spherical coordinates
						double*		sphericalElevation = NULL,	//!< pointer to a buffer with the Elevation angle (in radians) of point in spherical coordinates
						int64_t*	rowIndex = NULL,	//!< pointer to a buffer with the row number of point (zero based). This is useful for data that is stored in a regular grid.Shall be in the interval (0, 2^63).
						int64_t*	columnIndex = NULL,	//!< pointer to a buffer with the column number of point (zero based). This is useful for data that is stored in a regular grid. Shall be in the interval (0, 2^63).
						int64_t*	returnIndex = NULL,	//!< pointer to a buffer with the number of this return (zero based). That is, 0 is the first return, 1 is the second, and so on. Shall be in the interval (0, returnCount). Only for multi-return sensors. 
						int64_t*	returnCount = NULL,	//!< pointer to a buffer with the total number of returns for the pulse that this corresponds to. Shall be in the interval (0, 2^63). Only for multi-return sensors. 
						double*		timeStamp = NULL	//!< pointer to a buffer with the time (in seconds) since the start time for the data, which is given by acquisitionStart in the parent Data3D Structure. Shall be non-negative
						) const;					//!< @return Return true if sucessful, false otherwise

////////////////////////////////////////////////////////////////////
//
//	Raw File information
//
//! @brief This function returns the file raw E57Root Structure Node
	StructureNode		GetRawE57Root(void);	//!< @return Returns the E57Root StructureNode
//! @brief This function returns the raw Data3D Vector Node
	VectorNode			GetRawData3D(void);		//!< @return Returns the raw Data3D VectorNode
//! @brief This function returns the raw Image2D Vector Node
	VectorNode			GetRawImage2D(void);	//!< @return Returns the raw Image2D VectorNode

private:   //=================
					Reader();                 // No default constructor is defined for Node
protected: //=================
    friend class	ReaderImpl;

    E57_OBJECT_IMPLEMENTATION(Reader)  // Internal implementation details, not part of API, must be last in object

}; //end Reader class


////////////////////////////////////////////////////////////////////
//
//	e57::Writer
//

//! @brief This is the E57 Writer class

class	Writer {
public:

//! @brief This function is the constructor for the writer class
				Writer(
					const ustring & filePath,		//!< file path string
					const ustring & coordinateMetaData	//!< Information describing the Coordinate Reference System to be used for the file
					);

//! @brief This function returns true if the file is open
	bool		IsOpen(void) const;	//!< @return Returns true if the file is open and ready.

//! @brief This function closes the file
	bool		Close(void) const;

////////////////////////////////////////////////////////////////////
//
//	Camera Image picture data
//

//! @brief This function sets up the image2D header and positions the cursor
//* @details The user needs to config a Image2D structure with all the camera information before making this call. */
	int32_t		NewImage2D( 
						Image2D &	image2DHeader	//!< pointer to the Image2D structure to receive the picture information
						) const;						//!< @return Returns the image2D index

//! @brief This function writes the image block of data
	int64_t		WriteImage2DData(
						int32_t					imageIndex,	//!< picture block index given by the NewImage2D
						e57::Image2DType		imageType,		//!< identifies the image format desired.
						e57::Image2DProjection	imageProjection,//!< identifies the projection desired.
						void *					pBuffer,	//!< pointer the buffer
						int64_t					start,		//!< position in the block to start writing
						int64_t					count		//!< size of desired chuck or buffer size
						) const;						//!< @return Returns the number of bytes written

//! @brief This function closes the Image2D block
	bool		CloseImage2D(
						int32_t		imageIndex	//!< picture block index given by the NewImage2D
						) const ;				//!< @return Returns true if successful, false otherwise

//! @brief This function sets up the Data3D header and positions the cursor for the binary data
//* @details The user needs to config a Data3D structure with all the scanning information before making this call. */

	int32_t		NewData3D( 
						Data3D &	data3DHeader	//!< pointer to the Data3D structure to receive the image information
						) const;							//!< @return Returns the index of the new scan's data3D block.

//! @brief This function writes out blocks of point data
	CompressedVectorWriter	SetUpData3DPointsData(
						int32_t		dataIndex,			//!< data block index given by the NewData3D
						int64_t		pointCount,			//!< size of each of the buffers given
						int32_t*	valid,				//!< Value = 1 if the point is considered valid, 0 otherwise
						double*		cartesianX,			//!< pointer to a buffer with the X coordinate (in meters) of the point in Cartesian coordinates
						double*		cartesianY,			//!< pointer to a buffer with the Y coordinate (in meters) of the point in Cartesian coordinates
						double*		cartesianZ,			//!< pointer to a buffer with the Z coordinate (in meters) of the point in Cartesian coordinates
						double*		intensity = NULL,	//!< pointer to a buffer with the Point response intensity. Unit is unspecified
						double*		colorRed = NULL,	//!< pointer to a buffer with the Red color coefficient. Unit is unspecified
						double*		colorGreen = NULL,	//!< pointer to a buffer with the Green color coefficient. Unit is unspecified
						double*		colorBlue = NULL,	//!< pointer to a buffer with the Blue color coefficient. Unit is unspecified
						double*		sphericalRange = NULL,		//!< pointer to a buffer with the range (in meters) of points in spherical coordinates. Shall be non-negative
						double*		sphericalAzimuth = NULL,	//!< pointer to a buffer with the Azimuth angle (in radians) of point in spherical coordinates
						double*		sphericalElevation = NULL,	//!< pointer to a buffer with the Elevation angle (in radians) of point in spherical coordinates
						int64_t*	rowIndex = NULL,	//!< pointer to a buffer with the row number of point (zero based). This is useful for data that is stored in a regular grid.Shall be in the interval (0, 2^63).
						int64_t*	columnIndex = NULL,	//!< pointer to a buffer with the column number of point (zero based). This is useful for data that is stored in a regular grid. Shall be in the interval (0, 2^63).
						int64_t*	returnIndex = NULL,	//!< pointer to a buffer with the number of this return (zero based). That is, 0 is the first return, 1 is the second, and so on. Shall be in the interval (0, returnCount). Only for multi-return sensors. 
						int64_t*	returnCount = NULL,	//!< pointer to a buffer with the total number of returns for the pulse that this corresponds to. Shall be in the interval (0, 2^63). Only for multi-return sensors. 
						double*		timeStamp = NULL	//!< pointer to a buffer with the time (in seconds) since the start time for the data, which is given by acquisitionStart in the parent Data3D Structure. Shall be non-negative
						) const ;		//!< @return Return true if sucessful, false otherwise


//! @brief This funtion writes out the group data
	bool		WriteData3DGroupsData(
						int32_t		dataIndex,			//!< data block index given by the NewData3D
						int64_t*	idElementValue,		//!< index for this group
						int64_t*	startPointIndex,	//!< Starting index in to the "points" data vector for the groups
						int64_t*	pointCount,			//!< size of the groups given
						int32_t		count				//!< size of each of the buffers given
						) const;						//!< @return Return true if sucessful, false otherwise

////////////////////////////////////////////////////////////////////
//
//	Raw File information
//
//! @brief This function returns the file raw E57Root Structure Node
	StructureNode		GetRawE57Root(void);	//!< @return Returns the E57Root StructureNode
//! @brief This function returns the raw Data3D Vector Node
	VectorNode			GetRawData3D(void);		//!< @return Returns the raw Data3D VectorNode
//! @brief This function returns the raw Image2D Vector Node
	VectorNode			GetRawImage2D(void);	//!< @return Returns the raw Image2D VectorNode

private:   //=================
					Writer();                 // No default constructor is defined for Node
protected: //=================
    friend class	WriterImpl;

    E57_OBJECT_IMPLEMENTATION(Writer)  // Internal implementation details, not part of API, must be last in object

}; //end Writer class


}; //end namespace
#endif
