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
#include "IteratableChecksumValidator.h"
#include "Util.h"
#include "message.h"

#define BUFSIZE 16*1024

void IteratableChecksumValidator::validateChunk()
{
  if(!finished()) {
    
    unsigned char data[BUFSIZE];

    int32_t size = _diskWriter->readData(data, sizeof(data), _currentOffset);

    _ctx->digestUpdate(data, size);
    _currentOffset += sizeof(data);    

    if(finished()) {
      unsigned char* digest = new unsigned char[_ctx->digestLength()];
      try {
	_ctx->digestFinal(digest);
	if(_checksum->getMessageDigest() != Util::toHex(digest, _ctx->digestLength())) {
	  _bitfield->clearAllBit();
	}
	delete [] digest;
      } catch(...) {
	delete [] digest;
	throw;
      }
    }
  }
}

bool IteratableChecksumValidator::canValidate() const
{
  // We assume file is already opened using DiskWriter::open or openExistingFile.
  return !_checksum.isNull() && !_checksum->isEmpty();
}

void IteratableChecksumValidator::init()
{
  _bitfield->setAllBit();
  _currentOffset = 0;

  _ctx = new MessageDigestContext();
  _ctx->trySetAlgo(_checksum->getAlgo());
  _ctx->digestInit();
}
