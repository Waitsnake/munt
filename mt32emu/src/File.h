/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_FILE_H
#define MT32EMU_FILE_H

#include <cstddef>

namespace MT32Emu {

class File {
public:
	enum OpenMode {
		OpenMode_read  = 0,
		// DEPRECATED: Only read operations are necessary.
		OpenMode_write = 1
	};
	virtual ~File() {}
	virtual size_t getSize() = 0;
	virtual unsigned char *getData() = 0;
	virtual unsigned char *getSHA1() = 0;

	virtual void close() = 0;

	// DEPRECATED: Unused
	virtual size_t read(void * /*in*/, size_t) {return 0;}
	// DEPRECATED: Unused
	virtual bool readBit8u(Bit8u * /*in*/) {return false;}
	// DEPRECATED: Unused
	virtual bool readBit16u(Bit16u * /*in*/) {return false;}
	// DEPRECATED: Unused
	virtual bool readBit32u(Bit32u * /*in*/) {return false;}
	// DEPRECATED: Unused
	virtual bool isEOF() {return false;}
	// DEPRECATED: Unused
	virtual bool readLine(char * /*in*/, size_t /*size*/) {return false;}
	// DEPRECATED: Only read operations need to be implemented.
	virtual size_t write(const void * /*out*/, size_t /*size*/) {return 0;}
	// DEPRECATED: Only read operations need to be implemented.
	virtual bool writeBit8u(Bit8u /*out*/) {return false;}
	// DEPRECATED: Only read operations need to be implemented.
	virtual bool writeBit16u(Bit16u /*out*/) {return false;}
	// DEPRECATED: Only read operations need to be implemented.
	virtual bool writeBit32u(Bit32u /*out*/) {return false;}
};

}

#endif
