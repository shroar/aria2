/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#ifndef _D_MULTI_DISK_ADAPTOR_H_
#define _D_MULTI_DISK_ADAPTOR_H_

#include "DiskAdaptor.h"
#include "Option.h"
#include "DiskWriter.h"
#include "File.h"
#include "DlAbortEx.h"

class DiskWriterEntry {
private:
  FileEntryHandle fileEntry;
  DiskWriterHandle diskWriter;
public:
  DiskWriterEntry(const FileEntryHandle& fileEntry):
    fileEntry(fileEntry),
    diskWriter(0) {}

  ~DiskWriterEntry() {}

  string getFilePath(const string& topDir) const
  {
    return topDir+"/"+fileEntry->getPath();
  }

  void initAndOpenFile(const string& topDir)
    throw(DlAbortEx*)
  {
    diskWriter->initAndOpenFile(getFilePath(topDir), fileEntry->getLength());
  }

  void openFile(const string& topDir)
    throw(DlAbortEx*)
  {
    diskWriter->openFile(getFilePath(topDir), fileEntry->getLength());
  }

  void openExistingFile(const string& topDir)
    throw(DlAbortEx*)
  {
    diskWriter->openExistingFile(getFilePath(topDir), fileEntry->getLength());
  }

  void closeFile()
  {
    diskWriter->closeFile();
  }

  bool fileExists(const string& topDir)
  {
    return File(getFilePath(topDir)).exists();
  }

  int64_t size() const
    throw(DlAbortEx*)
  {
    return diskWriter->size();
  }

  FileEntryHandle getFileEntry() const {
    return fileEntry;
  }

  void setDiskWriter(const DiskWriterHandle& diskWriter) {
    this->diskWriter = diskWriter;
  }

  DiskWriterHandle getDiskWriter() const {
    return diskWriter;
  }
};

typedef SharedHandle<DiskWriterEntry> DiskWriterEntryHandle;

typedef deque<DiskWriterEntryHandle> DiskWriterEntries;

class MultiDiskAdaptor : public DiskAdaptor {
private:
  string topDir;
  int32_t pieceLength;
  DiskWriterEntries diskWriterEntries;
  const Option* option;

  void resetDiskWriterEntries();

  void mkdir() const throw(DlAbortEx*);

  bool isInRange(const DiskWriterEntryHandle entry, int64_t offset) const;

  int32_t calculateLength(const DiskWriterEntryHandle entry,
			  int64_t fileOffset,
			  int32_t rem) const;

  string getTopDirPath() const;
public:
  MultiDiskAdaptor():pieceLength(0),
		     option(0)
  {}

  virtual ~MultiDiskAdaptor() {}

  virtual void initAndOpenFile() throw(DlAbortEx*);

  virtual void openFile() throw(DlAbortEx*);

  virtual void openExistingFile() throw(DlAbortEx*);

  virtual void closeFile();

  virtual void onDownloadComplete() throw(DlAbortEx*);

  virtual void writeData(const unsigned char* data, int32_t len,
			 int64_t offset) throw(DlAbortEx*);

  virtual int32_t readData(unsigned char* data, int32_t len, int64_t offset) throw(DlAbortEx*);

  virtual bool fileExists();

  virtual string getFilePath()
  {
    return getTopDirPath();
  }

  virtual int64_t size() const throw(DlAbortEx*);

  virtual FileAllocationIteratorHandle fileAllocationIterator();

  void setTopDir(const string& topDir) {
    this->topDir = topDir;
  }

  const string& getTopDir() const {
    return topDir;
  }

  void setPieceLength(int32_t pieceLength) {
    this->pieceLength = pieceLength;
  }

  int32_t getPieceLength() const {
    return pieceLength;
  }

  void setOption(const Option* option) {
    this->option = option;
  }

  const Option* getOption() const {
    return option;
  }
};

typedef SharedHandle<MultiDiskAdaptor> MultiDiskAdaptorHandle;

#endif // _D_MULTI_DISK_ADAPTOR_H_
