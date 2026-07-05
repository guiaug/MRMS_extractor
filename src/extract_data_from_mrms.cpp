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

int convert_grib2_to_netcdf(string filename_str);//, const char *lakename, double *latminmax, double *lonminmax);

int main() {
	
	// Setting up the filename
	// 				NOTE: In the future the main block will get a list of files to handle
	//				At the moment, we deal with only 1 file
	string filename = "../RAW_MRMS/MRMS_MultiSensor_QPE_01H_Pass2_00.00_20210101-010000.grib2";
	
	// Error handle
	int error;
	
	error = convert_grib2_to_netcdf(filename);

	
	return 0;
}



int convert_grib2_to_netcdf(string filename_str)//, const char *lakename, double *latminmax, double *lonminmax)
{
	// Creatiof of a pointer to get all the qpe values from the grib2 file
	double *qpe_values;
	double *longitude;
	double *latitude;
	
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
	double top_right_lat; // Latitude of top right grid point
	double top_right_lon; // Longitude of top right grid point
	double lat_delta; // heigt of grid cell
	double lon_delta; // width of grid cell
	
	
	/*
	latitudeOfFirstGridPointInDegrees       54.995
	longitudeOfFirstGridPointInDegrees      230.005
	latitudeOfLastGridPointInDegrees        20.005
	longitudeOfLastGridPointInDegrees       299.995
	iDirectionIncrementInDegrees            0.01
	jDirectionIncrementInDegrees            0.01
	*/
	
	
	
	
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
	CODES_CHECK(codes_get_size(h, "values", &values_len), 0);
	CODES_CHECK(codes_get_double(h, "latitudeOfFirstGridPointInDegrees", &top_right_lat), 0);
	CODES_CHECK(codes_get_double(h, "longitudeOfFirstGridPointInDegrees", &top_right_lon), 0);
	CODES_CHECK(codes_get_double(h, "jDirectionIncrementInDegrees", &lat_delta), 0);
	CODES_CHECK(codes_get_double(h, "iDirectionIncrementInDegrees", &lon_delta), 0);

	
	cout << "   Grid Dimensions: " << Nlon << " x " << Nlat << " (Total points: " << (Nlon * Nlat) << ")" << endl;
	cout << values_len << endl;
	cout << "First latitude: " << top_right_lat << " First longitude: " << top_right_lon << endl;
	cout << "lat_delta: " << lat_delta << " lon_delta: " << lon_delta << endl;
	
	// Create the coordinate variables
	// First we do latitude
	latitude = new double[Nlat];
	for(int nj=0; nj< Nlat; nj++)
	{
		// Because the starting point is top right, we have to deduct the lat step
		latitude[nj] = top_right_lat - nj * lat_delta;
	}
	// Then longitude
	longitude = new double[Nlon];
	for(int ni=0; ni< Nlon; ni++)
	{
		longitude[ni] = top_right_lon + ni * lon_delta;
	}
	
	
	cout << "Last latitude point: " << latitude[Nlat-1] << endl;
	cout << "Last longitude point: " << longitude[Nlon-1] << endl;
	
	// 4. Extract Data Array Size and Values
	qpe_values = new double[values_len];
	CODES_CHECK(codes_get_double_array(h, "values", qpe_values, &values_len), 0);
	
	
	/*
	// 5. Print Metadata details
	char shortName[256];
	size_t name_len = sizeof(shortName);
	codes_get_string(h, "shortName", shortName, &name_len);
	cout << " Parameter Short Name: " << shortName << endl;
	
	// 6. Print sample values (First 10 non-negative data points)
	cout << "\n Sample Precip Data (mm):" << endl;
	int printed_count = 0;
	for (size_t i = 0; i < values_len && printed_count < 10; ++i) {
		// MRMS uses specific negative values (like -99) for missing data/no coverage
		if (qpe_values[i] >= 0) {
			cout << "Index [" << i << "]: " << qpe_values[i] << " mm" << endl;
			printed_count++;
		}
	}
	 */
	
	// 7. Clean up memory and close file
	codes_handle_delete(h);
	delete [] qpe_values;
	delete [] longitude;
	delete [] latitude;
	fclose(f);
	
	cout << "\n Successfully processed MRMS file." << endl;
	
	return 0;
}
