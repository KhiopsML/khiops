# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

"""
Khiops Native Interface (KNI) Python wrapper using ctypes.

This module provides a Python interface to the Khiops Native Interface (KNI) C library,
allowing direct deployment of Khiops models without temporary files.
"""

import ctypes
import os
import platform
import sys
import sysconfig
from pathlib import Path


class KNIError(Exception):
    """Exception raised for KNI errors."""

    def __init__(self, message, error_code=None):
        super().__init__(message)
        self.error_code = error_code


class KNI:
    """Python wrapper for Khiops Native Interface using ctypes."""

    # Error codes
    KNI_OK = 0
    KNI_ErrorRunningFunction = -1
    KNI_ErrorDictionaryFileName = -2
    KNI_ErrorDictionaryMissingFile = -3
    KNI_ErrorDictionaryFileFormat = -4
    KNI_ErrorDictionaryName = -5
    KNI_ErrorMissingDictionary = -6
    KNI_ErrorTooManyStreams = -7
    KNI_ErrorStreamHeaderLine = -8
    KNI_ErrorFieldSeparator = -9
    KNI_ErrorStreamHandle = -10
    KNI_ErrorStreamOpened = -11
    KNI_ErrorStreamNotOpened = -12
    KNI_ErrorStreamInputRecord = -13
    KNI_ErrorStreamInputRead = -14
    KNI_ErrorStreamOutputRecord = -15
    KNI_ErrorMissingSecondaryHeader = -16
    KNI_ErrorMissingExternalTable = -17
    KNI_ErrorDataRoot = -18
    KNI_ErrorDataPath = -19
    KNI_ErrorDataTableFile = -20
    KNI_ErrorLoadDataTable = -21
    KNI_ErrorMemoryOverflow = -22
    KNI_ErrorStreamOpening = -23
    KNI_ErrorStreamOpeningNotFinished = -24
    KNI_ErrorLogFile = -25

    # Constants
    KNI_MaxStreamNumber = 512
    KNI_DefaultMaxStreamMemory = 100
    KNI_MaxPathNameLength = 1024
    KNI_MaxDictionaryNameLength = 128
    KNI_MaxRecordLength = 8 * 1024 * 1024  # 8 MB

    def __init__(self, library_path: str | bytes | Path | None = None):
        """
        Initialize the KNI wrapper.

        Args:
            library_path: Optional path to the KNI shared library (str, bytes, or
                         pathlib.Path). If None, attempts to locate it automatically.
        """
        self._lib = self._load_library(library_path)
        self._setup_function_signatures()

    def _load_library(self, library_path: str | bytes | Path | None):
        """Load the KNI shared library."""
        if library_path:
            return ctypes.CDLL(os.fsdecode(library_path))

        # Try to locate library automatically
        system = platform.system()
        if system == "Windows":
            lib_name = "KhiopsNativeInterface.dll"
        elif system == "Linux":
            lib_name = "libKhiopsNativeInterface.so"
        elif system == "Darwin":  # macOS
            lib_name = "libKhiopsNativeInterface.dylib"
        else:
            raise RuntimeError(f"Unsupported platform: {system}")

        # When installed via pip, the shared library is placed in the Python
        # scripts directory (bin/ on Unix, Scripts/ on Windows)
        scripts_dir = sysconfig.get_path("scripts")
        if scripts_dir:
            lib_path = Path(scripts_dir) / lib_name
            if lib_path.exists():
                return ctypes.CDLL(str(lib_path))

        raise RuntimeError(
            f"Could not find {lib_name}. Reinstall the khiops-kni package."
        )

    def _setup_function_signatures(self):
        """Setup function signatures for KNI library functions."""
        # KNIGetVersion
        self._lib.KNIGetVersion.argtypes = []
        self._lib.KNIGetVersion.restype = ctypes.c_int

        # KNIGetFullVersion
        self._lib.KNIGetFullVersion.argtypes = []
        self._lib.KNIGetFullVersion.restype = ctypes.c_char_p

        # KNISetLogFileName
        self._lib.KNISetLogFileName.argtypes = [ctypes.c_char_p]
        self._lib.KNISetLogFileName.restype = ctypes.c_int

        # KNIOpenStream
        self._lib.KNIOpenStream.argtypes = [
            ctypes.c_char_p,  # sDictionaryFileName
            ctypes.c_char_p,  # sDictionaryName
            ctypes.c_char_p,  # sStreamHeaderLine
            ctypes.c_char,  # cFieldSeparator
        ]
        self._lib.KNIOpenStream.restype = ctypes.c_int

        # KNICloseStream
        self._lib.KNICloseStream.argtypes = [ctypes.c_int]
        self._lib.KNICloseStream.restype = ctypes.c_int

        # KNIRecodeStreamRecord
        self._lib.KNIRecodeStreamRecord.argtypes = [
            ctypes.c_int,  # hStream
            ctypes.c_char_p,  # sInputRecord
            ctypes.c_char_p,  # sOutputRecord
            ctypes.c_int,  # nOutputMaxLength
        ]
        self._lib.KNIRecodeStreamRecord.restype = ctypes.c_int

        # Multi-table functions
        # KNISetSecondaryHeaderLine
        self._lib.KNISetSecondaryHeaderLine.argtypes = [
            ctypes.c_int,  # hStream
            ctypes.c_char_p,  # sDataPath
            ctypes.c_char_p,  # sStreamSecondaryHeaderLine
        ]
        self._lib.KNISetSecondaryHeaderLine.restype = ctypes.c_int

        # KNISetExternalTable
        self._lib.KNISetExternalTable.argtypes = [
            ctypes.c_int,  # hStream
            ctypes.c_char_p,  # sDataRoot
            ctypes.c_char_p,  # sDataPath
            ctypes.c_char_p,  # sDataTableFileName
        ]
        self._lib.KNISetExternalTable.restype = ctypes.c_int

        # KNIFinishOpeningStream
        self._lib.KNIFinishOpeningStream.argtypes = [ctypes.c_int]
        self._lib.KNIFinishOpeningStream.restype = ctypes.c_int

        # KNISetSecondaryInputRecord
        self._lib.KNISetSecondaryInputRecord.argtypes = [
            ctypes.c_int,  # hStream
            ctypes.c_char_p,  # sDataPath
            ctypes.c_char_p,  # sStreamSecondaryInputRecord
        ]
        self._lib.KNISetSecondaryInputRecord.restype = ctypes.c_int

        # Advanced parameters
        # KNIGetStreamMaxMemory
        self._lib.KNIGetStreamMaxMemory.argtypes = []
        self._lib.KNIGetStreamMaxMemory.restype = ctypes.c_int

        # KNISetStreamMaxMemory
        self._lib.KNISetStreamMaxMemory.argtypes = [ctypes.c_int]
        self._lib.KNISetStreamMaxMemory.restype = ctypes.c_int

    @staticmethod
    def _to_bytes(value: str | bytes, param_name: str) -> bytes:
        """Encode a str or bytes parameter to UTF-8 bytes."""
        if isinstance(value, str):
            return value.encode("utf-8")
        if isinstance(value, bytes):
            return value
        raise TypeError(
            f"{param_name} must be str or bytes, not {type(value).__name__}"
        )

    def get_version(self) -> int:
        """Get KNI version as integer (10*major + minor)."""
        return self._lib.KNIGetVersion()

    def get_full_version(self) -> str:
        """Get KNI full version string."""
        return self._lib.KNIGetFullVersion().decode("utf-8")

    def set_log_file_name(self, log_file_name: str | bytes) -> None:
        """
        Set the log file name for error messages.

        Args:
            log_file_name: Path to log file (str or bytes, empty string for no logging)

        Raises:
            KNIError: If setting log file fails
            TypeError: If log_file_name is not str or bytes
        """
        log_file_name_bytes = self._to_bytes(log_file_name, "log_file_name")
        ret_code = self._lib.KNISetLogFileName(log_file_name_bytes)
        if ret_code != self.KNI_OK:
            raise KNIError(
                f"Failed to set log file: {self.get_error_message(ret_code)}",
                ret_code,
            )

    def open_stream(
        self,
        dictionary_file_path: str | bytes | Path,
        dictionary_name: str | bytes,
        header_line: str | bytes,
        field_separator: str | bytes = "\t",
    ) -> int:
        """
        Open a KNI stream for recoding.

        Args:
            dictionary_file_path: Path to the dictionary file (str, bytes, or pathlib.Path)
            dictionary_name: Name of the dictionary to use (str or bytes)
            header_line: Header line with field names (str or bytes)
            field_separator: Character used to separate fields (str or bytes, default: tab)

        Returns:
            Stream handle (positive integer)

        Raises:
            KNIError: If opening stream fails
            TypeError: If arguments have invalid types
        """
        # Type checking and conversion
        if isinstance(dictionary_file_path, Path):
            dictionary_file_path = str(dictionary_file_path)
        dictionary_file_path_bytes = self._to_bytes(
            dictionary_file_path, "dictionary_file_path"
        )
        dictionary_name_bytes = self._to_bytes(dictionary_name, "dictionary_name")
        header_line_bytes = self._to_bytes(header_line, "header_line")
        if len(field_separator) != 1:
            raise ValueError("field_separator must be a single character")
        field_separator_bytes = self._to_bytes(field_separator, "field_separator")
        field_separator_byte = field_separator_bytes[0]

        stream_handle = self._lib.KNIOpenStream(
            dictionary_file_path_bytes,
            dictionary_name_bytes,
            header_line_bytes,
            field_separator_byte,
        )
        if stream_handle < 0:
            raise KNIError(
                f"Failed to open stream: {self.get_error_message(stream_handle)}",
                stream_handle,
            )
        return stream_handle

    def close_stream(self, stream_handle: int) -> None:
        """
        Close a KNI stream.

        Args:
            stream_handle: Handle returned by open_stream (int)

        Raises:
            KNIError: If closing stream fails
            TypeError: If stream_handle is not int
        """
        if not isinstance(stream_handle, int):
            raise TypeError(
                f"stream_handle must be int, not {type(stream_handle).__name__}"
            )
        ret_code = self._lib.KNICloseStream(stream_handle)
        if ret_code != self.KNI_OK:
            raise KNIError(
                f"Failed to close stream: {self.get_error_message(ret_code)}",
                ret_code,
            )

    def recode_stream_record(
        self,
        stream_handle: int,
        input_record: str | bytes,
        max_output_length: int | None = None,
    ) -> str:
        """
        Recode an input record using the stream's dictionary.

        Args:
            stream_handle: Handle returned by open_stream (int)
            input_record: Input record (str or bytes)
            max_output_length: Maximum output buffer size (int, default: KNI_MaxRecordLength)

        Returns:
            Recoded output string

        Raises:
            KNIError: If recoding fails
            TypeError: If arguments have invalid types
        """
        if not isinstance(stream_handle, int):
            raise TypeError(
                f"stream_handle must be int, not {type(stream_handle).__name__}"
            )
        input_record_bytes = self._to_bytes(input_record, "input_record")
        if max_output_length is None:
            max_output_length = self.KNI_MaxRecordLength
        elif not isinstance(max_output_length, int):
            raise TypeError(
                f"max_output_length must be int or None, not {type(max_output_length).__name__}"
            )

        output_buffer = ctypes.create_string_buffer(max_output_length)
        ret_code = self._lib.KNIRecodeStreamRecord(
            stream_handle,
            input_record_bytes,
            output_buffer,
            max_output_length,
        )

        if ret_code != self.KNI_OK:
            raise KNIError(
                f"Failed to recode record: {self.get_error_message(ret_code)}",
                ret_code,
            )
        return output_buffer.value.decode("utf-8")

    def set_secondary_header_line(
        self, stream_handle: int, data_path: str | bytes, header_line: str | bytes
    ) -> None:
        """
        Set the header line of a secondary table (multi-table only).

        Args:
            stream_handle: Handle returned by open_stream (int)
            data_path: Data path identifying the secondary table (str or bytes)
            header_line: Header line with field names (str or bytes)

        Raises:
            KNIError: If setting secondary header fails
            TypeError: If arguments have invalid types
        """
        if not isinstance(stream_handle, int):
            raise TypeError(
                f"stream_handle must be int, not {type(stream_handle).__name__}"
            )
        data_path_bytes = self._to_bytes(data_path, "data_path")
        header_line_bytes = self._to_bytes(header_line, "header_line")
        ret_code = self._lib.KNISetSecondaryHeaderLine(
            stream_handle, data_path_bytes, header_line_bytes
        )
        if ret_code != self.KNI_OK:
            raise KNIError(
                f"Failed to set secondary header line: {self.get_error_message(ret_code)}",
                ret_code,
            )

    def set_external_table(
        self,
        stream_handle: int,
        data_root: str | bytes,
        data_path: str | bytes,
        data_table_file_name: str | bytes,
    ) -> None:
        """
        Set the name of a data file for an external table (multi-table only).

        Args:
            stream_handle: Handle returned by open_stream (int)
            data_root: Root dictionary of the external table (str or bytes)
            data_path: Data path for secondary external tables (str or bytes, empty for root)
            data_table_file_name: Path to the external table data file (str or bytes)

        Raises:
            KNIError: If setting external table fails
            TypeError: If arguments have invalid types
        """
        if not isinstance(stream_handle, int):
            raise TypeError(
                f"stream_handle must be int, not {type(stream_handle).__name__}"
            )
        data_root_bytes = self._to_bytes(data_root, "data_root")
        data_path_bytes = self._to_bytes(data_path, "data_path")
        data_table_file_name_bytes = self._to_bytes(
            data_table_file_name, "data_table_file_name"
        )
        ret_code = self._lib.KNISetExternalTable(
            stream_handle,
            data_root_bytes,
            data_path_bytes,
            data_table_file_name_bytes,
        )
        if ret_code != self.KNI_OK:
            raise KNIError(
                f"Failed to set external table: {self.get_error_message(ret_code)}",
                ret_code,
            )

    def finish_opening_stream(self, stream_handle: int) -> None:
        """
        Finish opening a stream (multi-table only).

        Must be called after all secondary headers and external tables are set.

        Args:
            stream_handle: Handle returned by open_stream (int)

        Raises:
            KNIError: If finishing opening stream fails
            TypeError: If stream_handle is not int
        """
        if not isinstance(stream_handle, int):
            raise TypeError(
                f"stream_handle must be int, not {type(stream_handle).__name__}"
            )
        ret_code = self._lib.KNIFinishOpeningStream(stream_handle)
        if ret_code != self.KNI_OK:
            raise KNIError(
                f"Failed to finish opening stream: {self.get_error_message(ret_code)}",
                ret_code,
            )

    def set_secondary_input_record(
        self, stream_handle: int, data_path: str | bytes, input_record: str | bytes
    ) -> None:
        """
        Set a secondary input record for multi-table recoding.

        All secondary records must be set before recoding the primary record.

        Args:
            stream_handle: Handle returned by open_stream (int)
            data_path: Data path identifying the secondary table (str or bytes)
            input_record: Secondary input record (str or bytes)

        Raises:
            KNIError: If setting secondary input record fails
            TypeError: If arguments have invalid types
        """
        if not isinstance(stream_handle, int):
            raise TypeError(
                f"stream_handle must be int, not {type(stream_handle).__name__}"
            )
        data_path_bytes = self._to_bytes(data_path, "data_path")
        input_record_bytes = self._to_bytes(input_record, "input_record")
        ret_code = self._lib.KNISetSecondaryInputRecord(
            stream_handle, data_path_bytes, input_record_bytes
        )
        if ret_code != self.KNI_OK:
            raise KNIError(
                f"Failed to set secondary input record: {self.get_error_message(ret_code)}",
                ret_code,
            )

    def get_stream_max_memory(self) -> int:
        """
        Get the maximum amount of memory (in MB) for stream opening.

        Returns:
            Maximum memory in MB
        """
        return self._lib.KNIGetStreamMaxMemory()

    def set_stream_max_memory(self, max_mb: int) -> int:
        """
        Set the maximum amount of memory (in MB) for stream opening.

        Args:
            max_mb: Maximum memory in MB (int)

        Returns:
            Accepted value (bounded by system limits)
        """
        if not isinstance(max_mb, int):
            raise TypeError(f"max_mb must be int, not {type(max_mb).__name__}")
        return self._lib.KNISetStreamMaxMemory(max_mb)

    @staticmethod
    def get_error_message(error_code: int) -> str:
        """
        Get a human-readable error message for an error code.

        Args:
            error_code: KNI error code

        Returns:
            Error message string
        """
        if not isinstance(error_code, int):
            raise TypeError(f"error_code must be int, not {type(error_code).__name__}")
        error_messages = {
            KNI.KNI_OK: "Success",
            KNI.KNI_ErrorRunningFunction: "Another KNI function is currently running",
            KNI.KNI_ErrorDictionaryFileName: "Bad dictionary file name",
            KNI.KNI_ErrorDictionaryMissingFile: "Dictionary file does not exist",
            KNI.KNI_ErrorDictionaryFileFormat: "Bad dictionary format",
            KNI.KNI_ErrorDictionaryName: "Bad dictionary name",
            KNI.KNI_ErrorMissingDictionary: "Dictionary not found in dictionary file",
            KNI.KNI_ErrorTooManyStreams: "Too many streams opened",
            KNI.KNI_ErrorStreamHeaderLine: "Bad stream header line",
            KNI.KNI_ErrorFieldSeparator: "Bad field separator",
            KNI.KNI_ErrorStreamHandle: "Bad stream handle",
            KNI.KNI_ErrorStreamOpened: "Stream already opened",
            KNI.KNI_ErrorStreamNotOpened: "Stream not opened",
            KNI.KNI_ErrorStreamInputRecord: "Bad input record",
            KNI.KNI_ErrorStreamInputRead: "Problem reading input record",
            KNI.KNI_ErrorStreamOutputRecord: "Output record too long",
            KNI.KNI_ErrorMissingSecondaryHeader: "Missing secondary table header",
            KNI.KNI_ErrorMissingExternalTable: "Missing external table",
            KNI.KNI_ErrorDataRoot: "Bad data root",
            KNI.KNI_ErrorDataPath: "Bad data path",
            KNI.KNI_ErrorDataTableFile: "Bad data table file",
            KNI.KNI_ErrorLoadDataTable: "Problem loading external data tables",
            KNI.KNI_ErrorMemoryOverflow: "Memory overflow",
            KNI.KNI_ErrorStreamOpening: "Stream could not be opened",
            KNI.KNI_ErrorStreamOpeningNotFinished: "Multi-table stream opening not finished",
            KNI.KNI_ErrorLogFile: "Bad error file",
        }
        return error_messages.get(error_code, f"Unknown error code: {error_code}")
