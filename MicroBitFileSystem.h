#ifndef MICROBIT_FILE_SYSTEM_H
#define MICROBIT_FILE_SYSTEM_H

#include "MicroBitConfig.h"
#include "MicroBitFlash.h"


// Configuration options.
#define MBFS_FILENAME_LENGTH		16		
#define MBFS_MAGIC					"MICROBIT_FS_1_0"

// open() flags.
#define MB_READ 0x01
#define MB_WRITE 0x02
#define MB_CREAT 0x04
#define MB_APPEND 0x08

// seek() flags.
#define MB_SEEK_SET 0x01
#define MB_SEEK_END 0x02
#define MB_SEEK_CUR 0x04

// Status flags
#define MBFS_STATUS_INITIALISED		0x01

// FileTable codes
#define MBFS_UNUSED			0xFFFF
#define MBFS_EOF			0xEFFF
#define MBFS_DELETED		0x0000

// DirectorEntry flags
#define MBFS_DIRECTORY_ENTRY_FREE		0x8000
#define MBFS_DIRECTORY_ENTRY_VALID		0x4000
#define MBFS_DIRECTORY_ENTRY_DIRECTORY	0x2000
#define MBFS_DIRECTORY_ENTRY_NEW		0xffff
#define MBFS_DIRECTORY_ENTRY_DELETED	0x0000

// Enumeration of BLOCK YPES
#define MBFS_BLOCK_TYPE_FILE			1
#define MBFS_BLOCK_TYPE_DIRECTORY		2
#define MBFS_BLOCK_TYPE_FILETABLE		3



//
// Every file in the file system has a file descriptor.
// These are held in directory entries.
//
struct DirectoryEntry
{
	char file_name[MBFS_FILENAME_LENGTH];		// Name of the file 
	uint16_t first_block;						// Logical block address of the start of the file.
	uint16_t flags;								// Status of the file.
	uint32_t length;							// Length of the file in bytes.
};

struct Directory
{
	DirectoryEntry entry[0];
};

//
// A FileDescriptor holds contextual information needed for each open file.
//
struct FileDescriptor
{
    // read/write/creat flags.
    uint16_t flags;

	// FileDescriptor id
	uint16_t id;

    // current file position, in bytes.
    uint32_t seek;

    // the current file size. n.b. this may be different to that stored in the DirectoryEntry.
    uint32_t length;

	// the directory entry of this file. 
	DirectoryEntry *dirent;

	// the directory entry of our parent directory. 
	DirectoryEntry *directory;

	// We maintain a chain of open file descriptors. Reference to the next FileDescriptor in the chain.
	FileDescriptor *next;
};

/**
  * @brief Class definition for the MicroBit File system
  *
  * Microbit file system class. Presents a POSIX-like interface consisting of:
  * - open()
  * - close()
  * - read()
  * - write()
  * - seek()
  * - remove()
  *
  * Only a single instance shoud exist at any given time.
  */
class MicroBitFileSystem
{
    private:

	// Status flags
	uint32_t status;

    // The instance of MicroBitFlash - the interface used for all flash writes/erasures
    MicroBitFlash flash;

	// Total Number of logical pages available for file data (including the file table)
	int	fileSystemSize;

	// Memory address of the start of the file system.
	uint16_t* fileSystemTable = NULL;

	// Size of the file table
	uint16_t fileSystemTableSize;

	// Cache of the last block allocated.
	uint16_t lastBlockAllocated = 0;

	// Reference to the root directory of the file system.
	DirectoryEntry *rootDirectory = NULL;

	// Chain of open files.
	FileDescriptor *openFiles = NULL;
    /**
      * Initialize the flash storage system
      *
      * The file system is located dynamically, based on where the program code
      * and code data finishes. This avoids having to allocate a fixed flash
      * region for builds even without MicroBitFileSystem. 
	  *	  
      * This method checks if the file system already exists, and loads it it. 
	  * If not, it will determines the optimal size of the file system, if necessary, and format the space
      *
      * @return MICROBIT_OK on success, or an error code.
      */
    int init(uint32_t flashStart, int flashPages);

	/**
 	  * Attempts to detect and load an exisitng file file system.
	  *
	  * @return MICROBIT_OK on success, or MICROBIT_NO_DATA if the file system could not be found.
	  */
	int load();

    /**
      * Allocate a free logical block.
	  * This is chosen at random from the blocks available, to even out the wear on the physical device.
      * @return NULL on error, page address on success
      */
    uint16_t getFreeBlock();

	/**
	* Allocates a free physical block.
	* This is chosen at random from the blocks available, to even out the wear on the physical device.
	* @return NULL on error, page address on success
	*/
	uint32_t* getFreePage();

	/**
	* Retrieve the DirectoryEntry assoiated with the given file's DIRECTORY (not the file itself).
	*
	* @param filename A fully qualified filename, from the root.
	*
	* @return A pointer to the DirectoryEntry for the given file's directory, or NULL if no entry is found.
	*/
	DirectoryEntry* getDirectoryOf(char const * filename);

	/**
	* Retrieve the DirectoryEntry for the given filename.
	*
	* @param filename A fully or partially qualified filename.
	* @param directory The directory to search. If ommitted, the root directory will be used.
	*
	* @return A pointer to the DirectoryEntry for the given file, or NULL if no entry is found.
	*/
	DirectoryEntry* getDirectoryEntry(char const * filename, const DirectoryEntry *directory = NULL);
	
	/**
	* Create a new DirectoryEntry with the given filename and flags.
	*
	* @param filename A fully or partially qualified filename.
	* @param directory The directory in which to create the entry
	* @param isDirectory true if the entry being created is itself a directory
	*
	* @return A pointer to the new DirectoryEntry for the given file, or NULL if it was not possible to allocated resources.
	*/
	DirectoryEntry* createFile(char const * filename, DirectoryEntry *directory, bool isDirectory);

	/**
	* Allocate a free DiretoryEntry in the given directory, extending and refreshing the directory block if necessary.
	*
	* @param directory The directory to add a DirectoryEntry to
	* @return A pointer to the new DirectoryEntry for the given file, or NULL if it was not possible to allocated resources.
	*/
	DirectoryEntry* createDirectoryEntry(DirectoryEntry *directory);

	/**
	* Refresh the physical page associated with the given block.
	* Any logical blocks marked for deletion on that page are recycled.
	*
	* @param block the block to recycle.
	* @param isDirectory set to tru of the block contains a directory to enable deep DirectoryEntry recycling.
	* @return MICROBIT_OK on success.
	*/
	int recycleBlock(uint16_t block, int type = MBFS_BLOCK_TYPE_FILE);

	/**
	* Refresh the physical pages associated with the file table.
	* Any logical blocks marked for deletion on those pages are recycled back to UNUSED.
	**
	* @return MICROBIT_OK on success.
	*/
	int recycleFileTable();

	/**
	* Retrieve a memory pointer for the start of the physical memory page containing the given block.
	*
	* @param block A valid block in a file.
	*
	* @return A pointer to the physical page in FLASH memory holding the given block.
	*/
	uint32_t *getPage(uint16_t block);

	/**
	* Retrieve a memory pointer for the start of the given block.
	*
	* @param block A valid block in a file.
	*
	* @return A pointer to the FLASH memory associated with the given block.
	*/
	uint32_t *getBlock(uint16_t block);

	/**
	* Retrieve the next block in a chain.
	*
	* @param block A valid block in a file.
	*
	* @return The block number of the next block in the file.
	*/
	uint16_t getNextFileBlock(uint16_t block);

	/**
	* Determine the logical block that contains the given address.
	*
	* @param address A valid memory location within the file system space.
	*
	* @return The block number containing the given address.
	*/
	uint16_t getBlockNumber(void *address);

	/**
	* Determine the number of logical blocks required to hold the file table.
	*
	* @return The number of logical blocks required to hold the file table.
	*/
	uint16_t calculateFileTableSize();

	/*
	* Update a file table entry to a given value.
	*
	* @param block The block to update.
	* @param value The value to store in the file table.
	* @return MICROBIT_OK on success.
	*/
	int fileTableWrite(uint16_t block, uint16_t value);

	/**
	* Searches the list of open files for one with the given identifier.
	*
	* @param fd A previsouly opened file identifier, as returned by open().
	* @param remove Remove the file descriptor from the list if true.
	* @return A FileDescriptor matching the given ID, or NULL if the file is not open.
	*/
	FileDescriptor* getFileDescriptor(int fd, bool remove = false);

	/**
	* Initialises a new file system
	*
	* @return MICROBIT_OK on success, or an error code..
	*/
	int format();

    public:

    static MicroBitFileSystem *defaultFileSystem;

	/**
      * Constructor. Creates an instance of a MicroBitFileSystem.
      */
    MicroBitFileSystem(uint32_t flashStart = 0, int flashPages = 0);

    /**
      * Open a new file, and obtain a new file handle (int) to
      * read/write/seek the file. The flags are:
      *  - MB_READ : read from the file.
      *  - MB_WRITE : write to the file.
      *  - MB_CREAT : create a new file, if it doesn't already exist.
      *
      * If a file is opened that doesn't exist, and MB_CREAT isn't passed,
      * an error is returned, otherwise the file is created.
      *
      * @todo The same file can only be opened by a single handle at once.
      * @todo Add MB_APPEND flag.
      *
      * @param filename name of the file to open, must be null terminated.
      * @param flags
      * @return return the file handle,MICROBIT_NOT_SUPPORTED if the file system has
      *         not been initialised MICROBIT_INVALID_PARAMETER if the filename is
      *         too large, MICROBIT_NO_RESOURCES if the file system is full.
      *
      * @code
      * MicroBitFileSystem f();
      * int fd = f.open("test.txt", MB_WRITE|MB_CREAT);
      * if(fd<0)
      *    print("file open error");
      * @endcode
      */
    int open(char const * filename, uint32_t flags);

    /**
      * Close the specified file handle.
      * File handle resources are then made available for future open() calls.
      *
      * close() must be called at some point to ensure the filesize in the
      * FT is synced with the cached value in the FD.
      *
      * @warning if close() is not called, the FT may not be correct,
      * leading to data loss.
      *
      * @param fd file descriptor - obtained with open().
      * @return non-zero on success, MICROBIT_NOT_SUPPORTED if the file system has not
      *         been initialised, MICROBIT_INVALID_PARAMETER if the given file handle
      *         is invalid.
      *
      * @code
      * MicroBitFileSystem f();
      * int fd = f.open("test.txt", MB_READ);
      * if(!f.close(fd))
      *    print("error closing file.");
      * @endcode
      */
    int close(int fd);

    /**
      * Move the current position of a file handle, to be used for
      * subsequent read/write calls.
      *
      * The offset modifier can be:
      *  - MB_SEEK_SET set the absolute seek position.
      *  - MB_SEEK_CUR set the seek position based on the current offset.
      *  - MB_SEEK_END set the seek position from the end of the file.
      * E.g. to seek to 2nd-to-last byte, use offset=-1.
      *
      * @param fd file handle, obtained with open()
      * @param offset new offset, can be positive/negative.
      * @param flags
      * @return new offset position on success, MICROBIT_NOT_SUPPORTED if the file system
      *         is not intiialised, MICROBIT_INVALID_PARAMETER if the flag given is invalid
      *         or the file handle is invalid.
      *
      * @code
      * MicroBitFileSystem f;
      * int fd = f.open("test.txt", MB_READ);
      * f.seek(fd, -100, MB_SEEK_END); //100 bytes before end of file.
      * @endcode
      */
    int seek(int fd, int offset, uint8_t flags);

    /**
      * Write data to the file.
      *
      * Write from buffer, len bytes to the current seek position.
      * On each invocation to write, the seek position of the file handle
      * is incremented atomically, by the number of bytes returned.
      *
      * The cached filesize in the FD is updated on this call. Also, the
      * FT file size is updated only if a new page(s) has been written too,
      * to reduce the number of FT writes.
      *
      * @param fd File handle
      * @param buffer the buffer from which to write data
      * @param len number of bytes to write
      * @return number of bytes written on success, MICROBIT_NO_RESOURCES if data did
      *         not get written to flash or the file system has not been initialised,
      *         or this file was not opened with the MB_WRITE flag set, MICROBIT_INVALID_PARAMETER
      *         if the given file handle is invalid.
      *
      * @code
      * MicroBitFileSystem f();
      * int fd = f.open("test.txt", MB_WRITE);
      * if(f.write(fd, "hello!", 7) != 7)
      *    print("error writing");
      * @endcode
      */
    int write(int fd, uint8_t* buffer, int len);

    /**
      * Read data from the file.
      *
      * Read len bytes from the current seek position in the file, into the
      * buffer. On each invocation to read, the seek position of the file
      * handle is incremented atomically, by the number of bytes returned.
      *
      * @param fd File handle, obtained with open()
      * @param buffer to store data
      * @param len number of bytes to read
      * @return number of bytes read on success, MICROBIT_NOT_SUPPORTED if the file
      *         system is not initialised, or this file was not opened with the
      *         MB_READ flag set, MICROBIT_INVALID_PARAMETER if the given file handle
      *         is invalid.
      *
      * @code
      * MicroBitFileSystem f;
      * int fd = f.open("read.txt", MB_READ);
      * if(f.read(fd, buffer, 100) != 100)
      *    print("read error");
      * @endcode
      */
    int read(int fd, uint8_t* buffer, int len);

    /**
      * Remove a file from the system, and free allocated assets
      * (including assigned blocks which are returned for use by other files).
      *
      * @todo the file must not already have an open file handle.
      *
      * @param filename name of the file to remove.
      * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the given filename
      *         does not exist, MICROBIT_CANCELLED if something went wrong
      *
      * @code
      * MicroBitFileSystem f;
      * if(!f.remove("file.txt"))
      *     print("file could not be removed")
      * @endcode
      */
    int remove(char const * filename);

	/**
	* Creates a new directory with the given name and location
	*
	* @param name The fully qualified name of the new directory.
	* @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the path is invalid, or MICROBT_NO_RESOURCES if the FileSystem is full.
	*/
	int createDirectory(char const *name);

	void debugFAT();
	void debugRootDirectory();
	int debugDirectory(char *name);
	void debugDirectory(DirectoryEntry *directory);
};

#endif
