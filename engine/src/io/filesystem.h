#pragma once

#include "defines.h"

// Holds a handle to a file.
typedef struct FILE_HANDLE {
    // Opaque handle to internal file handle.
    void* handle;
    b8 is_valid;
} FILE_HANDLE;

typedef enum E_FILE_MODES {
    FILE_MODE_READ = 0x1,
    FILE_MODE_WRITE = 0x2
} E_FILE_MODES;

/**
 * Checks if a file with the given path exists.
 * @param path The path of the file to be checked.
 * @returns True if exists; otherwise false.
 */
YAPI b8 filesystem_exists(const char* path);

/** 
 * Attempt to open file located at path.
 * @param path The path of the file to be opened.
 * @param mode Mode flags for the file when opened (read/write). See file_modes enum in filesystem.h.
 * @param binary Indicates if the file should be opened in binary mode.
 * @param out_handle A pointer to a file_handle structure which holds the handle information.
 * @returns True if opened successfully; otherwise false.
 */
YAPI b8 filesystem_open(const char* path, E_FILE_MODES mode, b8 binary, FILE_HANDLE* out_handle);

/** 
 * Closes the provided handle to a file.
 * @param handle A pointer to a file_handle structure which holds the handle to be closed.
 */
YAPI void filesystem_close(FILE_HANDLE* handle);

/**
 * @brief Attempts to read the size of the file to which handle is attached.
 *
 * @param handle The file handle.
 * @param out_size A pointer to hold the file size.
 * @return
 */
YAPI b8 filesystem_size(FILE_HANDLE* handle, u64* out_size);

/** 
 * Reads up to a newline or EOF.
 * @param handle A pointer to a file_handle structure.
 * @param max_length The maximum length to be read from the line.
 * @param line_buf A pointer to a character array populated by this method. Must already be allocated.
 * @param out_line_length A pointer to hold the line length read from the file.
 * @returns True if successful; otherwise false.
 */
YAPI b8 filesystem_read_line(FILE_HANDLE* handle, u64 max_length, char** line_buf, u64* out_line_length);

/** 
 * Writes text to the provided file, appending a '\n' afterward.
 * @param handle A pointer to a file_handle structure.
 * @param text The text to be written.
 * @returns True if successful; otherwise false.
 */
YAPI b8 filesystem_write_line(FILE_HANDLE* handle, const char* text);

/** 
 * Reads up to data_size bytes of data into out_bytes_read. 
 * Allocates *out_data, which must be freed by the caller.
 * @param handle A pointer to a file_handle structure.
 * @param data_size The number of bytes to read.
 * @param out_data A pointer to a block of memory to be populated by this method.
 * @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
YAPI b8 filesystem_read(FILE_HANDLE* handle, u64 data_size, void* out_data, u64* out_bytes_read);

/** 
 * Reads up to data_size bytes of data into out_bytes_read. 
 * Allocates *out_bytes, which must be freed by the caller.
 * @param handle A pointer to a file_handle structure.
 * @param out_bytes A pointer to a byte array which will be allocated and populated by this method.
 * @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
YAPI b8 filesystem_read_all_bytes(FILE_HANDLE* handle, u8** out_bytes, u64* out_bytes_read);

/**
 * @brief Reads all characters of data into out_text.
 * @param handle A pointer to a file_handle structure.
 * @param out_text A character array which will be populated by this method.
 * @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
YAPI b8 filesystem_read_all_text(FILE_HANDLE* handle, char* out_text, u64* out_bytes_read);

/** 
 * Writes provided data to the file.
 * @param handle A pointer to a file_handle structure.
 * @param data_size The size of the data in bytes.
 * @param data The data to be written.
 * @param out_bytes_written A pointer to a number which will be populated with the number of bytes actually written to the file.
 * @returns True if successful; otherwise false.
 */
YAPI b8 filesystem_write(FILE_HANDLE* handle, u64 data_size, const void* data, u64* out_bytes_written);