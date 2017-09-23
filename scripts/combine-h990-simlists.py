#!/usr/bin/env python

"""
Copyright (C) 2017 Elliott Mitchell <ehem+lg-v20-toolsd@m5p.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

from __future__ import print_function

import argparse
import io
import sys
import os


magic = {
	b"SIM-MATCH-X?": -1,
	b"SIM-MATCH-0?": 0,
	b"SIM-MATCH-1?": 1,
	b"SIM-MATCH-2?": 2,
}


byte_val = (((((ord('B')<<8)|ord('Y'))<<8)|ord('T'))<<8)|ord('E')
char_val = (((((ord('C')<<8)|ord('H'))<<8)|ord('A'))<<8)|ord('R')


def startfile(name):
	try:
		file = io.open(name, "rb")
	except:
		print('Failed opening file "{:s}"'.format(name))
		sys.exit(1)

	head = file.read(12)

	if head not in magic:
		print('Magic number missing from file "{:s}"'.format(name))
		sys.exit(1)

	return file


def readval(file):
	str = file.read(4)
	return (((((ord(str[0])<<8)|ord(str[1]))<<8)|ord(str[2]))<<8)|ord(str[3])


def writeval(file, val):
	vals = [0,0,0,0]
	vals[3] = val & 0xFF
	val >>= 8
	vals[2] = val & 0xFF
	val >>= 8
	vals[1] = val & 0xFF
	val >>= 8
	vals[0] = val & 0xFF
	file.write(chr(vals[0])+chr(vals[1])+chr(vals[2])+chr(vals[3]))


# continue work...
if __name__ == "__main__":
	if len(sys.argv) != 4:
		print("{:s} <in SIM-match-list> <in SIM-match-list> <out SIM-match-list".format(sys.argv[0]))
		sys.exit(1)

	name0 = sys.argv[1]
	name1 = sys.argv[2]

	in0 = startfile(name0)
	in1 = startfile(name1)

	try:
		out = io.open(sys.argv[3], "wb")
	except:
		print('failed opening file "{:s}"'.format(sys.argv[0], sys.argv[3]))
		sys.exit(1)

	out.write(b"SIM-MATCH-X?")

	writeval(out, byte_val)

	poso = out.tell()

	out.write(b"\x00\x00\x00\x00")

	cnt = 0

	if readval(in0) != byte_val:
		print('BYTE header missing from "{:s}"'.format(name0))
		sys.exit(1)

	cnt0 = readval(in0)

	if readval(in1) != byte_val:
		print('BYTE header missing from "{:s}"'.format(name1))
		sys.exit(1)

	cnt1 = readval(in1)

	if cnt0>0 and cnt1>0:
		val0=readval(in0)
		cnt0-=1
		val1=readval(in1)
		cnt1-=1

		while cnt0>0 and cnt1>0:
			if val0 == val1:
				writeval(out, val0)
				cnt+=1
			if val0 <= val1:
				val0=readval(in0)
				cnt0-=1
			if val0 >= val1:
				val1=readval(in1)
				cnt1-=1
		else:
			if val0 == val1:
				writeval(out, val0)
				cnt+=1

		if cnt0>0:
			in0.seek(cnt0<<2, io.SEEK_CUR)
		else:
			in1.seek(cnt1<<2, io.SEEK_CUR)

	out.seek(poso, io.SEEK_SET)
	writeval(out, cnt)
	out.seek(0, io.SEEK_END)

	print("Found {:d} common byte matches".format(cnt))



	writeval(out, char_val)

	poso = out.tell()

	out.write(b"\x00\x00\x00\x00")

	cnt = 0

	if readval(in0) != char_val:
		print('CHAR header missing from "{:s}"'.format(name0))
		sys.exit(1)

	cnt0 = readval(in0)

	if readval(in1) != char_val:
		print('CHAR header missing from "{:s}"'.format(name1))
		sys.exit(1)

	cnt1 = readval(in1)

	if cnt0>0 and cnt1>0:
		val0=readval(in0)
		cnt0-=1
		val1=readval(in1)
		cnt1-=1

		while cnt0>0 and cnt1>0:
			if val0 == val1:
				writeval(out, val0)
				cnt+=1
			if val0 <= val1:
				val0=readval(in0)
				cnt0-=1
			if val0 >= val1:
				val1=readval(in1)
				cnt1-=1
		else:
			if val0 == val1:
				writeval(out, val0)
				cnt+=1

	out.seek(poso, io.SEEK_SET)
	writeval(out, cnt)

	print("Found {:d} common char matches".format(cnt))

	print("Done.")

