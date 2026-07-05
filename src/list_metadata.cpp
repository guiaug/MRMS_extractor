#include <iostream>
#include <iomanip>
#include <string>
#include <eccodes.h>

using namespace std;

int main() {
    const char* filename = "../RAW_MRMS/MRMS_MultiSensor_QPE_01H_Pass2_00.00_20210101-010000.grib2";

    // 1. Open the GRIB file
    FILE* f = fopen(filename, "rb");
    if (!f) {
        cerr << "❌ Error: Could not open file " << filename << endl;
        return 1;
    }

    int err = 0;
    codes_handle* h = nullptr;

    // 2. Load the first GRIB message
    h = codes_handle_new_from_file(nullptr, f, PRODUCT_GRIB, &err);
    if (!h) {
        cerr << "❌ Error: Unable to read GRIB handle. Code: " << err << endl;
        fclose(f);
        return 1;
    }

    // 3. Create a keys iterator to scan the metadata header
    // CODES_KEYS_ITERATOR_ALL_KEYS loops through every metadata key
    codes_keys_iterator* kiter = codes_keys_iterator_new(h, CODES_KEYS_ITERATOR_ALL_KEYS, nullptr);
    if (!kiter) {
        cerr << "❌ Error: Unable to create keys iterator." << endl;
        codes_handle_delete(h);
        fclose(f);
        return 1;
    }

    cout << left << setw(40) << "🔑 KEY NAME" << "📄 VALUE" << endl;
    cout << string(80, '-') << endl;

    // 4. Step through each key in the metadata
    while (codes_keys_iterator_next(kiter)) {
        // Get the key name
        const char* name = codes_keys_iterator_get_name(kiter);
        
        // Allocate a buffer to extract the value as a string representation
        char value[1024];
        size_t vlen = sizeof(value);
        
        // Safely extract the value as text
        int get_err = codes_get_string(h, const_cast<char*>(name), value, &vlen);
        
        if (get_err == CODES_SUCCESS) {
            cout << left << setw(40) << name << value << endl;
        } else {
            // Some internal/structural keys might fail to print cleanly as a string
            cout << left << setw(40) << name << "[Unable to read value]" << endl;
        }
    }

    // 5. Clean up allocations and pointers
    codes_keys_iterator_delete(kiter);
    codes_handle_delete(h);
    fclose(f);

    cout << string(80, '-') << endl;
    cout << "✅ Finished listing all metadata keys." << endl;
    return 0;
}
