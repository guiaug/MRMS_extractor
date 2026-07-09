/*
 ----------------------------------
		extract_data_from_mrms by Guillaume Auger @ RPI
		aug.gui@gmail.com
		
		This piece of code aims at qpe data from MRMS grib2 files and convert
		everything into a netcdf.
		
 ----------------------------------
 */


#include <iostream> // Grib2 needs the file type
#include <vector> // To get the data
#include <eccodes.h> // Library to handle grib2 files

using namespace std;

int convert_grib2_to_netcdf(string filename_str, double latmin, double latmax, double lonmin, double lonmax);//, const char *lakename, double *latminmax, double *lonminmax);
void create_coordinate_array(double *coord, double coord_start, double coord_step, long length, long ScanNegatively);
void get_mapping_indices(double *raw_coord, long length, double bb_m, double bb_M, long *index, long ScanNegatively);

int main() {
	
	// Setting up the filename
	// 				NOTE: In the future the main block will get a list of files to handle
	//				At the moment, we deal with only 1 file
	string filename = "../RAW_MRMS/MRMS_MultiSensor_QPE_01H_Pass2_00.00_20210101-010000.grib2";
	
	// Error handle
	int error;
	
	error = convert_grib2_to_netcdf(filename, 42.02524,42.34468, -79.5843 + 360.0, -79.1575 + 360.0);

	
	return 0;
}



int convert_grib2_to_netcdf(string filename_str, double latmin, double latmax, double lonmin, double lonmax)//, const char *lakename, double *latminmax, double *lonminmax)
{
	// Creatiof on a pointer to get all the qpe values from the grib2 file
	double *qpe_values;
	double *longitude;
	double *latitude;
	long lat_index[2];
	long lon_index[2];
	
	// Apparently, the function FILE requires a const char*
	// That way, the filename can be modified in the main block.
	const char * filename = filename_str.c_str();
	
	// Variable to handle errors from grib2 library
	int err = 0; //
	
	// This will be the handle to the grib file
	codes_handle *h = nullptr;
	
	// Handle to Cpp file
	FILE* f;
	
	// Information related to the variable in the grib file
	long Nlon; // Number of longitude data points
	long Nlat; // Number of latitude data points
	size_t values_len; // Total number of datapoints. It is equals to Nlon * Nlat
	double nor_wes_lat; // Latitude of north west grid point
	double nor_wes_lon; // Longitude of north west grid point
	double lat_delta; // heigt of grid cell
	double lon_delta; // width of grid cell
	long iScansNegatively; // Useful to know where the first point is located
	long jScansNegatively; // Useful to know where the first point is located
	
	
	// 1. Open the GRIB2 file
	f = fopen(filename, "rb");
	if (!f) {
		cerr << "  Error: Could not open file " << filename << endl;
		return 1;
	}
	
	// 2. Load the first GRIB message from the file
	h = codes_handle_new_from_file(nullptr, f, PRODUCT_GRIB, &err);
	if (h == nullptr) {
		cerr << "  Error: Unable to read GRIB handle. Code: " << err << endl;
		fclose(f);
		return 1;
	}
	
	// 3. Extract Grid Metadata (Dimensions)
	CODES_CHECK(codes_get_long(h, "Ni", &Nlon), 0);
	CODES_CHECK(codes_get_long(h, "Nj", &Nlat), 0);
	CODES_CHECK(codes_get_long(h, "iScansNegatively", &iScansNegatively), 0);
	CODES_CHECK(codes_get_long(h, "jScansNegatively", &jScansNegatively), 0);
	
	CODES_CHECK(codes_get_size(h, "values", &values_len), 0);
	CODES_CHECK(codes_get_double(h, "latitudeOfFirstGridPointInDegrees", &nor_wes_lat), 0);
	CODES_CHECK(codes_get_double(h, "longitudeOfFirstGridPointInDegrees", &nor_wes_lon), 0);
	CODES_CHECK(codes_get_double(h, "jDirectionIncrementInDegrees", &lat_delta), 0);
	CODES_CHECK(codes_get_double(h, "iDirectionIncrementInDegrees", &lon_delta), 0);
	
	// ======================= Begin creating the coordinate variables
	// First we do latitude
	latitude = new double[Nlat];
	cout << "Creating latitude array" << endl;
	create_coordinate_array(latitude, nor_wes_lat, lat_delta, Nlat, jScansNegatively);
	
	// Then longitude
	longitude = new double[Nlon];
	cout << "Creating longitude array" << endl;
	create_coordinate_array(longitude, nor_wes_lon, lon_delta, Nlon, iScansNegatively);
	
	// ======================= Finish creating the coordinate variables
	
	
	
	// ======================= Begin creating the QPE variable and subsample the data
	// 4. Extract Data Array Size and Values
	qpe_values = new double[values_len];
	CODES_CHECK(codes_get_double_array(h, "values", qpe_values, &values_len), 0);
	
	get_mapping_indices(latitude, Nlat, latmin, latmax, lat_index, jScansNegatively);
	get_mapping_indices(longitude, Nlon, lonmin, lonmax, lon_index, iScansNegatively);
	
	// Get the indices that fall into the bounding box set up by the user
	// The way it is done will depend on orientation of the coordinates
	
	
	// ======================= Finish creating the QPE variable and subsample the data
	
	
	// 7. Clean up memory and close file
	codes_handle_delete(h);
	delete [] qpe_values;
	delete [] longitude;
	delete [] latitude;
	fclose(f);
	
	cout << "\n Successfully processed MRMS file." << endl;
	
	return 0;
}

// This function creates the longitude and latitude array, based on the metadata gathered from the
void create_coordinate_array(double *coord, double coord_start, double coord_step, long length, long ScanNegatively)
{
	int ni;
	
	// If the longitude goes from east to west
	if(ScanNegatively)
	{
		cout << "Negative Scan" << endl;
		for(int ni=0; ni< length; ni++)
		{
			coord[ni] = coord_start - ni * coord_step;
		}
	}else{
		cout << "Positive Scan" << endl;
		for(int ni=0; ni< length; ni++)
		{
			coord[ni] = coord_start + ni * coord_step;
		}
	}
}

// This function creates a mapping between the future 2D array and the 1D array from MRMS.
// That way, we don't need to create another two full 2D array with the coordinates, and
// check with the bounding box. It ought to be faster and less memory hungry that way.
void get_mapping_indices(double *raw_coord, long length, double bb_m, double bb_M, long *index, long ScanNegatively)
{
	long ni;
	
	// If it scans negatively, i.e. it goes through the coordinate in a decreasing order.
	if(ScanNegatively)
	{
		for(ni=0; ni<length; ni++)
		{
			if(raw_coord[ni] >= bb_M)
			{
				index[0] = ni;
			}

			if(raw_coord[ni] >= bb_m)
			{
				index[1] = ni;
			}
		}
		
		cout << "bbox min: " << bb_m << " bbox_max: " << bb_M << endl;
		cout << "data min: " << raw_coord[index[1] ] << " data max: " << raw_coord[index[0]]<< endl;
		
	}else{
		for(ni=0; ni<length; ni++)
		{
			if(raw_coord[ni] <= bb_m)
			{
				index[0] = ni;
			}

			if(raw_coord[ni] <= bb_M)
			{
				index[1] = ni;
			}
		}
		
		cout << "bbox min: " << bb_m << " bbox_max: " << bb_M << endl;
		cout << "data min: " << raw_coord[index[0] ] << " data max: " << raw_coord[index[1]]<< endl;
		
	}
	
	
}
