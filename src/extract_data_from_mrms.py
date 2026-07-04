import xarray
import glob
from netCDF4 import Dataset, date2num
from datetime import datetime
import numpy as np

# Where the MRMS data is located
folder_to_data = "../RAW_MRMS/"

# A dictionnary that contains lakes to focus on
watersheds = {}
# Lake George
lakename = "LakeGeorge"
watersheds[lakename] = {}
watersheds[lakename] = np.array([-79.5843 + 360.0, -79.1575 + 360.0, 42.02524, 42.34468])



# Bounding box for the data to be sampled
lon_west = -79.5843 + 360.0
lon_east = -79.1575 + 360.0

lat_south = 42.02524
lat_north = 42.34468


# Here we have to get all the files in the directory named :folder_to_data
# and sort them chronologically
file_list = glob.glob(folder_to_data + "*.grib2")

# Sort them from the timestamps in the name of the file
sorted_files = sorted(file_list, key = lambda x: datetime.strptime(x, '../RAW_MRMS/MRMS_MultiSensor_QPE_01H_Pass2_00.00_%Y%m%d-%H0000.grib2'))


# Now that we have the file list in order, we can loop through the list and tet the data

basetime = "seconds since 2020-01-01 00:00:00"


for filename in sorted_files:

    print("Doing: ", filename)

    # First we extract the time stamp
    timestamp = datetime.strptime(filename, '../RAW_MRMS/MRMS_MultiSensor_QPE_01H_Pass2_00.00_%Y%m%d-%H0000.grib2')
    # We open the grib2 file
    data = xarray.open_dataset(filename, engine="cfgrib", decode_timedelta=True)

    # We get longitude and latitude
    longitude = data["longitude"].values
    latitude = data["latitude"].values


    # Now let's loop through the lakes we want to focus on
    for lakename in watersheds:
        print(lakename)
        lon_west = watersheds[lakename][0]
        lon_east = watersheds[lakename][1]
        lat_south = watersheds[lakename][2]
        lat_north = watersheds[lakename][3]



    # Look for the data point that falls into the bounding box
    loc_lon_west = np.squeeze(np.where(longitude >= lon_west))[0]
    loc_lon_east = np.squeeze(np.where(longitude >= lon_east))[0]

    # Confusing to high heavens, the vertical axis is reversed. array[0] is the northermost value
    # array[-1] is the southernmost of the latitude
    loc_lat_south = np.squeeze(np.where(latitude >= lat_south))[-1]
    loc_lat_north = np.squeeze(np.where(latitude >= lat_north))[-1]

    # This array would contain all the QPE for the bounding box setup earlier in the code
    data_to_save = data["unknown"].values[loc_lat_north:loc_lat_south, loc_lon_west:loc_lon_east]


    longitude_out = longitude[loc_lon_west:loc_lon_east]
    latitude_out = latitude[loc_lat_north:loc_lat_south]


    ## Create the netcdf file for that time step
    nc_out = Dataset(timestamp.strftime('DATA_NETCDF/MRMS_MultiSensor_QPE_01H_Pass2_00.00_%Y%m%d-%H0000.nc'), "w")

    nc_out.createDimension("Nt", 1)
    nc_out.createDimension("Nlat", len(latitude_out))
    nc_out.createDimension("Nlon", len(longitude_out))

    # Create the variables for the netcdf
    Var_lon = nc_out.createVariable("longitude", "f8", ("Nlon",))
    Var_lon.long_name = "Longitude of data point"
    Var_lon.units = "Degree East"
    Var_lon[:] = longitude_out.copy()

    Var_lat = nc_out.createVariable("latitude", "f8", ("Nlat",))
    Var_lat.long_name = "Latitude of data point"
    Var_lat.units = "Degree North"
    Var_lat[:] = latitude_out.copy()

    Var_qpe = nc_out.createVariable("QPE", "f8", ("Nt", "Nlat","Nlon"))
    Var_qpe.long_name = "Hourly QPE data from MRMS pass2"
    Var_qpe.units = "QPE [mm]"
    Var_qpe[:] = data_to_save.copy()

    Var_time = nc_out.createVariable("time", "f8", ("Nt",))
    Var_time.units = basetime
    Var_time[:] = date2num(timestamp, units=basetime)

    nc_out.close()
    
