# Khiops Native Interface (KNI) - Python Package

Python bindings for the Khiops Native Interface (KNI), enabling direct deployment of Khiops models from Python without temporary files.

## Installation

```bash
pip install khiops-kni
```

## Quick Start

```python
from kni import KNI

# Initialize KNI wrapper
kni = KNI()

# Open a stream for recoding
stream_handle = kni.open_stream(
    dictionary_file_name="model.kdic",
    dictionary_name="MyDictionary",
    header_line="feature1\tfeature2\tfeature3",
    field_separator="\t"
)

# Recode records
input_record = "value1\tvalue2\tvalue3"
ret_code, output_record = kni.recode_stream_record(stream_handle, input_record)

if ret_code == KNI.KNI_OK:
    print(f"Output: {output_record}")
else:
    print(f"Error: {kni.get_error_message(ret_code)}")

# Close the stream
kni.close_stream(stream_handle)
```

## Features

- **Cross-platform**: Works on Windows, Linux, and macOS
- **Easy to use**: Pythonic API wrapping the C library
- **High performance**: Direct C library calls via ctypes
- **Multi-table support**: Handle complex multi-table schemas
- **Comprehensive error handling**: Clear error messages for debugging

## API Overview

### KNI Class

Main class for interacting with the Khiops Native Interface.

#### Methods

- `get_version()` - Get KNI version as integer
- `get_full_version()` - Get full version string
- `set_log_file_name(log_file_name)` - Set error log file
- `open_stream(dictionary_file_name, dictionary_name, header_line, field_separator)` - Open a stream
- `close_stream(stream_handle)` - Close a stream
- `recode_stream_record(stream_handle, input_record)` - Recode a record
- `set_secondary_header_line(stream_handle, data_path, header_line)` - Set secondary table header (multi-table)
- `set_external_table(stream_handle, data_root, data_path, data_table_file_name)` - Set external table (multi-table)
- `finish_opening_stream(stream_handle)` - Finish opening multi-table stream
- `set_secondary_input_record(stream_handle, data_path, input_record)` - Set secondary record (multi-table)
- `get_stream_max_memory()` - Get max memory for stream
- `set_stream_max_memory(max_mb)` - Set max memory for stream
- `get_error_message(error_code)` - Get human-readable error message

## Multi-Table Example

```python
from kni import KNI

kni = KNI()

# Open stream with main table header
stream_handle = kni.open_stream(
    "model.kdic", "MainDict", "id\tname\tvalue", "\t"
)

# Set secondary table header
kni.set_secondary_header_line(
    stream_handle, "Details", "detail_id\tmain_id\tinfo"
)

# Set external table
kni.set_external_table(
    stream_handle, "RefData", "", "reference.txt"
)

# Finish opening for multi-table
kni.finish_opening_stream(stream_handle)

# Set secondary records before recoding main record
kni.set_secondary_input_record(stream_handle, "Details", "1\t100\tinfo1")
kni.set_secondary_input_record(stream_handle, "Details", "2\t100\tinfo2")

# Recode main record
ret_code, output = kni.recode_stream_record(stream_handle, "100\tJohn\t42")

kni.close_stream(stream_handle)
```

## Requirements

- Python 3.9 or later
- The KhiopsNativeInterface C library is bundled with this package

## License

BSD-3-Clause-Clear

## Links

- [Khiops Website](https://khiops.org)
- [Documentation](https://github.com/KhiopsML/KNI-tutorial)
- [GitHub Repository](https://github.com/KhiopsML/khiops)
- [Issue Tracker](https://github.com/KhiopsML/khiops/issues)

## Support

For questions or issues, please visit the [issue tracker](https://github.com/KhiopsML/khiops/issues) or contact khiops.team@orange.com.
