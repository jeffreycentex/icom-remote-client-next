// ----------------------------------------------------------------------------
// misc.cxx  --  Miscellaneous helper functions
//
// Copyright (C) 2006-2007
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  These filters were adapted from code contained
// in the gmfsk source code distribution.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "misc.h"
unsigned char grayencode(unsigned char data)
//unsigned char graydecode(unsigned char data)
{
	unsigned char bits = data;

	bits ^= data >> 1;
	bits ^= data >> 2;
	bits ^= data >> 3;
	bits ^= data >> 4;
	bits ^= data >> 5;
	bits ^= data >> 6;
	bits ^= data >> 7;

	return bits;
}

unsigned char graydecode(unsigned char data)
//unsigned char grayencode(unsigned char data)
{
	return data ^ (data >> 1);
}

