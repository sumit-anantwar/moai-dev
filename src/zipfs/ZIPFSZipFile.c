// Copyright (c) 2010-2011 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "pch.h"
#include <zipfs/zipfs_util.h>
#include <zipfs/ZIPFSZipFile.h>

#define SCAN_BUFFER_SIZE 256

#define ARCHIVE_HEADER_SIGNATURE  0x06054b50
#define ENTRY_HEADER_SIGNATURE  0x02014b50
#define FILE_HEADER_SIGNATURE  0x04034b50

//================================================================//
// ArchiveHeader
//================================================================//
typedef struct ArchiveHeader {

	unsigned long	mSignature;			// 4 End of central directory signature = 0x06054b50
	unsigned short	mDiskNumber;		// 2 Number of this disk
	unsigned short	mStartDisk;			// 2 Disk where central directory starts
	unsigned short	mTotalDiskEntries;	// 2 Total number of entries on disk
	unsigned short	mTotalEntries;		// 2 Total number of central in archive
	unsigned long	mCDSize;			// 4 Size of central directory in bytes
	unsigned long	mCDAddr;			// 4 Offset of start of central directory, relative to start of archive
	unsigned short	mCommentLength;		// 2 ZIP file comment length

} ArchiveHeader;

//================================================================//
// EntryHeader
//================================================================//
typedef struct EntryHeader {

	unsigned long	mSignature;				// 4 Central directory file header signature = 0x02014b50
	unsigned short	mByVersion;				// 2 Version made by
	unsigned short	mVersionNeeded;			// 2 Version needed to extract (minimum)
	unsigned short	mFlag;					// 2 General purpose bit flag
	unsigned short	mCompression;			// 2 Compression method
	unsigned short	mLastModTime;			// 2 File last modification time
	unsigned short	mLastModDate;			// 2 File last modification date
	unsigned long	mCrc32;					// 4 CRC-32
	unsigned long	mCompressedSize;		// 4 Compressed size
	unsigned long	mUncompressedSize;		// 4 Uncompressed size
	unsigned short	mNameLength;			// 2 File name length (n)
	unsigned short	mExtraFieldLength;		// 2 Extra field length (m)
	unsigned short	mCommentLength;			// 2 File comment length (k)
	unsigned short	mDiskNumber;			// 2 Disk number where file starts
	unsigned short	mInternalAttributes;	// 2 Internal file attributes
	unsigned long	mExternalAttributes;	// 4 External file attributes
	unsigned long	mFileHeaderAddr;		// 4 Relative offset of file header

} EntryHeader;

//================================================================//
// FileHeader
//================================================================//
typedef struct FileHeader {

	unsigned long	mSignature;				// 4	Local file header signature = 0x04034b50 (read as a little-endian number)
	unsigned short	mVersionNeeded;			// 2	Version needed to extract (minimum)
	unsigned short	mFlag;					// 2	General purpose bit flag
	unsigned short	mCompression;			// 2	Compression method
	unsigned short	mLastModTime;			// 2	File last modification time
	unsigned short	mLastModDate;			// 2	File last modification date
	unsigned long	mCrc32;					// 4	CRC-32
	unsigned long	mCompressedSize;		// 4	Compressed size
	unsigned long	mUncompressedSize;		// 4	Uncompressed size
	unsigned short	mNameLength;			// 2	File name length
	unsigned short	mExtraFieldLength;		// 2	Extra field length

} FileHeader;

//================================================================//
// local
//================================================================//

//----------------------------------------------------------------//
static int read_archive_header ( FILE* file, ArchiveHeader* header ) {

	size_t filelen;
	size_t cursor;
	char buffer [ SCAN_BUFFER_SIZE ];
	size_t stepsize;
	size_t scansize;
	int i;
	
	if ( !file ) return -1;

	fseek ( file, 0, SEEK_END );
	filelen = ftell ( file );
	
	cursor = filelen;
	stepsize = SCAN_BUFFER_SIZE - 4;
    while ( cursor ) {
		
		cursor = ( cursor > stepsize ) ? cursor - stepsize : 0;
		scansize = (( cursor + SCAN_BUFFER_SIZE ) > filelen ) ? filelen - cursor : SCAN_BUFFER_SIZE;
		
		fseek ( file, cursor, SEEK_SET );
		fread ( buffer, scansize, 1, file );

        for ( i = scansize - 4; i > 0; --i ) {
			
			// maybe found it
			if ( *( unsigned long* )&buffer [ i ] == ARCHIVE_HEADER_SIGNATURE ) {

				fseek ( file, cursor + i, SEEK_SET );
				
				fread ( &header->mSignature, 4, 1, file );
				fread ( &header->mDiskNumber, 2, 1, file );
				fread ( &header->mStartDisk, 2, 1, file );
				fread ( &header->mTotalDiskEntries, 2, 1, file );
				fread ( &header->mTotalEntries, 2, 1, file );
				fread ( &header->mCDSize, 4, 1, file );
				fread ( &header->mCDAddr, 4, 1, file );
				fread ( &header->mCommentLength, 2, 1, file );
				
				return 0;
			}
		}
    }
	return -1;
}

//----------------------------------------------------------------//
static int read_entry_header ( FILE* file, EntryHeader* header ) {
	
	fread ( &header->mSignature, 4, 1, file );
	
	if ( header->mSignature != ENTRY_HEADER_SIGNATURE ) return -1;
	
	fread ( &header->mByVersion, 2, 1, file );
	fread ( &header->mVersionNeeded, 2, 1, file );
	fread ( &header->mFlag, 2, 1, file );
	fread ( &header->mCompression, 2, 1, file );
	fread ( &header->mLastModTime, 2, 1, file );
	fread ( &header->mLastModDate, 2, 1, file );
	fread ( &header->mCrc32, 4, 1, file );
	fread ( &header->mCompressedSize, 4, 1, file );
	fread ( &header->mUncompressedSize, 4, 1, file );
	fread ( &header->mNameLength, 2, 1, file );
	fread ( &header->mExtraFieldLength, 2, 1, file );
	fread ( &header->mCommentLength, 2, 1, file );
	fread ( &header->mDiskNumber, 2, 1, file );
	fread ( &header->mInternalAttributes, 2, 1, file );
	fread ( &header->mExternalAttributes, 4, 1, file );
	fread ( &header->mFileHeaderAddr, 4, 1, file );
	
	return 0;
}

//----------------------------------------------------------------//
static int read_file_header ( FILE* file, FileHeader* header ) {
	
	fread ( &header->mSignature, 4, 1, file );
	
	if ( header->mSignature != FILE_HEADER_SIGNATURE ) return -1;
	
	fread ( &header->mVersionNeeded, 2, 1, file );
	fread ( &header->mFlag, 2, 1, file );
	fread ( &header->mCompression, 2, 1, file );
	fread ( &header->mLastModTime, 2, 1, file );
	fread ( &header->mLastModDate, 2, 1, file );
	fread ( &header->mCrc32, 4, 1, file );
	fread ( &header->mCompressedSize, 4, 1, file );
	fread ( &header->mUncompressedSize, 4, 1, file );
	fread ( &header->mNameLength, 2, 1, file );
	fread ( &header->mExtraFieldLength, 2, 1, file );
	
	return 0;
}

//================================================================//
// ZIPFSZipFileEntry
//================================================================//

//----------------------------------------------------------------//
void ZIPFSZipFileEntry_Delete ( ZIPFSZipFileEntry* self ) {

	if ( !self ) return;

	clear_string ( self->mName );
	memset ( self, 0, sizeof ( ZIPFSZipFileEntry ));
	free ( self );
}

//================================================================//
// ZIPFSZipFileDir private
//================================================================//

ZIPFSZipFileDir*	ZIPFSZipFileDir_AffirmSubDir	( ZIPFSZipFileDir* self, const char* name, size_t len );

//----------------------------------------------------------------//
ZIPFSZipFileDir* ZIPFSZipFileDir_AffirmSubDir ( ZIPFSZipFileDir* self, const char* path, size_t len ) {

	ZIPFSZipFileDir* dir = self->mChildDirs;
	
	for ( ; dir; dir = dir->mNext ) {
		if ( count_same_nocase ( dir->mName, path ) == len ) return dir;
	}
	
	dir = ( ZIPFSZipFileDir* )calloc ( 1, sizeof ( ZIPFSZipFileDir ));
	
	dir->mNext = self->mChildDirs;
	self->mChildDirs = dir;
	
	dir->mName = ( char* )malloc ( len + 1 );
	memcpy ( dir->mName, path, len );
	dir->mName [ len ] = 0;
	
	return dir;
}

//================================================================//
// ZIPFSZipFileDir
//================================================================//

//----------------------------------------------------------------//
void ZIPFSZipFileDir_Delete ( ZIPFSZipFileDir* self ) {

	ZIPFSZipFileDir* dirCursor;
	ZIPFSZipFileEntry* entryCursor;

	if ( !self ) return;

	dirCursor = self->mChildDirs;
	while ( dirCursor ) {
		ZIPFSZipFileDir* dir = dirCursor;
		dirCursor = dirCursor->mNext;
		
		ZIPFSZipFileDir_Delete ( dir );
	}

	entryCursor = self->mChildFiles;
	while ( entryCursor ) {
		ZIPFSZipFileEntry* entry = entryCursor;
		entryCursor = entryCursor->mNext;
		
		ZIPFSZipFileEntry_Delete ( entry );
	}

	clear_string ( self->mName );
	memset ( self, 0, sizeof ( ZIPFSZipFileEntry ));
	free ( self );
}

//================================================================//
// ZIPFSZipFile private
//================================================================//

void ZIPFSZipFile_AddEntry ( ZIPFSZipFile* self, EntryHeader* header, const char* name );

//----------------------------------------------------------------//
void ZIPFSZipFile_AddEntry ( ZIPFSZipFile* self, EntryHeader* header, const char* name ) {

	int i;
	const char* path = name;
	ZIPFSZipFileDir* dir = self->mRoot;
	
	// gobble the leading '/' (if any)
	if ( path [ 0 ] == '/' ) {
		path = &path [ 1 ];
	}
	
	// build out directories
	for ( i = 0; path [ i ]; ) {
		if ( path [ i ] == '/' ) {
			dir = ZIPFSZipFileDir_AffirmSubDir ( dir, path, i );
			path = &path [ i + 1 ];
			i = 0;
			continue;
		}
		i++;
	}
	
	if ( path [ 0 ]) {
		
		ZIPFSZipFileEntry* entry = ( ZIPFSZipFileEntry* )calloc ( 1, sizeof ( ZIPFSZipFileEntry ));
		
		entry->mFileHeaderAddr		= header->mFileHeaderAddr;
		entry->mCrc32				= header->mCrc32;
		entry->mCompression			= header->mCompression;
		entry->mCompressedSize		= header->mCompressedSize;
		entry->mUncompressedSize	= header->mUncompressedSize;
	
		entry->mName = copy_string ( path );
		
		entry->mNext = dir->mChildFiles;
		dir->mChildFiles = entry;
	}
}

//================================================================//
// ZIPFSZipFile
//================================================================//

//----------------------------------------------------------------//
void ZIPFSZipFile_Delete ( ZIPFSZipFile* self ) {

	if ( !self ) return;
	
	clear_string ( self->mFilename );
	
	if ( self->mRoot ) {
		ZIPFSZipFileDir_Delete ( self->mRoot );
	}
	
	memset ( self, 0, sizeof ( ZIPFSZipFile ));
	free ( self );
}

//----------------------------------------------------------------//
ZIPFSZipFileDir* ZIPFSZipFile_FindDir ( ZIPFSZipFile* self, char const* path ) {

	size_t i;
	ZIPFSZipFileDir* dir;
	
	if ( !self->mRoot ) return 0;
	if ( !path ) return 0;
	
	// gobble the leading '/' (if any)
	if ( path [ 0 ] == '/' ) {
		path = &path [ 1 ];
	}
	
	dir = self->mRoot;
	
	for ( i = 0; path [ i ]; ) {
		if ( path [ i ] == '/' ) {
			
			ZIPFSZipFileDir* cursor = dir->mChildDirs;
			
			for ( ; cursor; cursor = cursor->mNext ) {
				if ( count_same_nocase ( cursor->mName, path ) == i ) {
					dir = cursor;
					break;
				}
			}
			
			// no match found; can't resolve directory
			if ( !cursor ) return 0;
			
			path = &path [ i + 1 ];
			i = 0;
			continue;
		}
		i++;
	}
	
	return dir;
}

//----------------------------------------------------------------//
ZIPFSZipFileEntry* ZIPFSZipFile_FindEntry ( ZIPFSZipFile* self, char const* filename ) {

	ZIPFSZipFileDir* dir;
	ZIPFSZipFileEntry* entry;
	size_t i;
	
	i = strlen ( filename ) - 1;
	if ( filename [ i ] == '/' ) return 0;
	
	dir = ZIPFSZipFile_FindDir ( self, filename );
	if ( !dir ) return 0;
	
	for ( ; i >= 0; --i ) {
		if ( filename [ i ] == '/' ) {
			filename = &filename [ i + 1 ];
			break;
		}
	}
	
	entry = dir->mChildFiles;
	for ( ; entry; entry = entry->mNext ) {
		if ( stricmp ( entry->mName, filename ) == 0 ) break;
	}

	return entry;
}

//----------------------------------------------------------------//
ZIPFSZipFile* ZIPFSZipFile_New ( const char* filename ) {

	ZIPFSZipFile* self = 0;
	ArchiveHeader header;
	EntryHeader entryHeader;
	char* nameBuffer = 0;
	int nameBufferSize = 0;
	int result;
	int i;

	FILE* file = fopen ( filename, "rb" );
	if ( !file ) goto error;
 
	result = read_archive_header ( file, &header );
	if ( result ) goto error;
	
	if ( header.mDiskNumber != 0 ) goto error; // unsupported
	if ( header.mStartDisk != 0 ) goto error; // unsupported
	if ( header.mTotalDiskEntries != header.mTotalEntries ) goto error; // unsupported
	
	// seek to top of central directory
	fseek ( file, header.mCDAddr, SEEK_SET );
	
	// create the info
	self = ( ZIPFSZipFile* )calloc ( 1, sizeof ( ZIPFSZipFile ));
	
	self->mFilename = copy_string ( filename );
	self->mRoot = ( ZIPFSZipFileDir* )calloc ( 1, sizeof ( ZIPFSZipFileDir ));
	
	// parse in the entries
	for ( i = 0; i < header.mTotalEntries; ++i ) {
	
		result = read_entry_header ( file, &entryHeader );
		if ( result ) goto error;
		
		if (( entryHeader.mNameLength + 1 ) > nameBufferSize ) {
			nameBufferSize += SCAN_BUFFER_SIZE;
			nameBuffer = ( char* )realloc ( nameBuffer, nameBufferSize );
		}
		
		fread ( nameBuffer, entryHeader.mNameLength, 1, file );
		nameBuffer [ entryHeader.mNameLength ] = 0;
		
		ZIPFSZipFile_AddEntry ( self, &entryHeader, nameBuffer );
	}
	
	goto finish;
	
error:

	if ( self ) {
		ZIPFSZipFile_Delete ( self );
		self = 0;
	}

finish:

	if ( nameBuffer ) {
		free ( nameBuffer );
	}

	if ( file ) {
		fclose ( file );
	}

	return self;
}

//================================================================//
// ZIPFSZipStream
//================================================================//

//----------------------------------------------------------------//
int ZIPFSZipStream_Close ( ZIPFSZipStream* self ) {

	if ( !self ) return 0;

	if ( self->mFile ) {
		fclose ( self->mFile );
		inflateEnd ( &self->mStream );
	}
	
	if ( self->mBuffer ) {
		free ( self->mBuffer );
	}
	return 0;
}

//----------------------------------------------------------------//
ZIPFSZipStream* ZIPFSZipStream_Open ( ZIPFSZipFile* archive, const char* entryname ) {

	int result;
	FILE* file = 0;
	ZIPFSZipStream* self = 0;
	ZIPFSZipFileEntry* entry;
	FileHeader fileHeader;
	
	entry = ZIPFSZipFile_FindEntry ( archive, entryname );
	if ( !entry ) goto error;

	file = fopen ( archive->mFilename, "rb" );
	if ( !file ) goto error;

    self = ( ZIPFSZipStream* )calloc ( 1, sizeof ( ZIPFSZipStream ));

	self->mFile = file;
	self->mEntry = entry;
	// finfo->entry = (( entry->symlink != NULL ) ? entry->symlink : entry );

	self->mBufferSize = entry->mCompressedSize < ZIP_STREAM_BUFFER_MAX ? entry->mCompressedSize : ZIP_STREAM_BUFFER_MAX;
	self->mBuffer = malloc ( self->mBufferSize );

	result = fseek ( file, entry->mFileHeaderAddr, SEEK_SET );
	if ( result ) goto error;

	// read local header
	result = read_file_header ( file, &fileHeader );
	if ( result ) goto error;
	
	// sanity check
	if ( fileHeader.mCrc32 != entry->mCrc32 ) goto error;

	result = fseek ( file, fileHeader.mNameLength + fileHeader.mExtraFieldLength, SEEK_CUR );
	if ( result ) goto error;

	self->mBaseAddr = ftell ( file );

    if ( entry->mCompression ) {
		result = inflateInit2 ( &self->mStream, -MAX_WBITS );
		if ( result != Z_OK ) goto error;
    }
	return self;

error:

	if ( self ) {
		ZIPFSZipStream_Close ( self );
	}
	return 0;
}

//----------------------------------------------------------------//
size_t ZIPFSZipStream_Read ( ZIPFSZipStream* self, void* buffer, size_t size ) {

	int result;
	FILE* file = self->mFile;
	z_stream* stream = &self->mStream;
    ZIPFSZipFileEntry* entry = self->mEntry;
    size_t totalRead = 0;
    size_t totalOut = 0;
    size_t remaining = entry->mUncompressedSize - self->mUncompressedCursor;

	if ( !entry->mCompression ) {
		return fread ( buffer, size, 1, file );
	}

	if ( !file ) return 0;
	if ( !stream ) return 0;
	if ( !size ) return 0;
	if ( !remaining ) return 0;

	if ( remaining < size ) {
		size = remaining;
    }
    
	stream->next_out = buffer;
	stream->avail_out = size;

    while ( totalRead < size ) {
		
		if ( stream->avail_in == 0 ) {
			
			size_t cacheSize = entry->mCompressedSize - self->mCompressedCursor;
			
			if ( cacheSize > 0 ) {
				if ( cacheSize > self->mBufferSize ) {
					cacheSize = self->mBufferSize;
				}

				cacheSize = fread ( self->mBuffer, 1, cacheSize, file );
				if ( cacheSize <= 0 ) break;

				self->mCompressedCursor += cacheSize;
				stream->next_in = ( Bytef* )self->mBuffer;
				stream->avail_in = cacheSize;
			}
		}

		totalOut = stream->total_out;
		result = inflate ( stream, Z_SYNC_FLUSH );
		totalRead += ( stream->total_out - totalOut );
		
		if ( result != Z_OK ) break;
	}

	if ( totalRead > 0 ) {
		self->mUncompressedCursor += totalRead;
	}

    return totalRead;
}

//----------------------------------------------------------------//
int ZIPFSZipStream_Seek ( ZIPFSZipStream* self, long int offset, int origin ) {

	int result;
	size_t absOffset = 0;
	FILE* file = self->mFile;
	z_stream* stream = &self->mStream;
    ZIPFSZipFileEntry* entry = self->mEntry;

	switch ( origin ) {
		case SEEK_CUR: {
			absOffset = self->mUncompressedCursor + offset;
			break;
		}
		case SEEK_END: {
			absOffset = entry->mUncompressedSize;
			break;
		}
		case SEEK_SET: {
			absOffset = offset;
			break;
		}
	}

	if ( absOffset > entry->mUncompressedSize ) return -1;

	if ( !entry->mCompression ) {
		return fseek ( file, offset, SEEK_SET );
	}

    if ( absOffset < self->mUncompressedCursor ) {
		
		z_stream newStream;
		memset ( &newStream, 0, sizeof ( z_stream ));
		
		result = fseek ( file, self->mBaseAddr, SEEK_SET );
		if ( result ) return -1;
		
		result = inflateInit2 ( &newStream, -MAX_WBITS );
		if ( result != Z_OK ) return -1;

		inflateEnd ( stream );
		( *stream ) = newStream;

		self->mCompressedCursor = 0;
		self->mUncompressedCursor = 0;
    }

    while ( self->mUncompressedCursor < absOffset ) {
    
		char buffer [ SCAN_BUFFER_SIZE ];
		size_t size;
		size_t read;

		size = offset - self->mUncompressedCursor;
		if ( size > SCAN_BUFFER_SIZE ) {
			size = SCAN_BUFFER_SIZE;
		}
		
		read = ZIPFSZipStream_Read ( self, buffer, size );
		if ( read != size ) return -1;
	}

    return 0;
}

//----------------------------------------------------------------//
size_t ZIPFSZipStream_Tell ( ZIPFSZipStream* self ) {

	return self->mUncompressedCursor;
}
