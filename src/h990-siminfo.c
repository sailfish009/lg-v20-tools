/* **********************************************************************
* Copyright (C) 2017 Elliott Mitchell					*
*									*
*	This program is free software: you can redistribute it and/or	*
*	modify it under the terms of the GNU General Public License as	*
*	published by the Free Software Foundation, either version 3 of	*
*	the License, or (at your option) any later version.		*
*									*
*	This program is distributed in the hope that it will be useful,	*
*	but WITHOUT ANY WARRANTY; without even the implied warranty of	*
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*
*	GNU General Public License for more details.			*
*									*
*	You should have received a copy of the GNU General Public	*
*	License along with this program.  If not, see			*
*	<http://www.gnu.org/licenses/>.					*
*************************************************************************
*$Id$			*
************************************************************************/


#define _FILE_OFFSET_BITS 64

#include <linux/fs.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <endian.h>


struct sim_info {
	char magic[16];  // must be set to "LGE ONE BINARY" to be valid.
	uint32_t reserved;
	uint16_t hwinfo_major;
	uint16_t hwinfo_minor;
	char lg_model_name[20];
	uint32_t sim_num;
};

const char magic[]="LGE ONE BINARY\0";


char model_name[23];



int main(int argc, char **argv)
{
	FILE *out=NULL;
	int fd;
	uint32_t blksz;
	char *map;
	const struct sim_info *info;
	off_t misclen;
	uint32_t cur;
	int opt;
	unsigned char cmdsim=0;
	char *outname=NULL;
	char buf[4];
	uint64_t cnt;
	off_t head;

	while((opt=getopt(argc, argv, "h?o:12"))>=0) {
		switch(opt) {
		case 'o':
			outname=optarg;
			break;
		case '1':
			if(cmdsim) {
				fprintf(stderr,
"-1 specified after expected SIM count already specified\n");
				return 1;
			}
			cmdsim=1;
			break;
		case '2':
			if(cmdsim) {
				fprintf(stderr,
"-2 specified after expected SIM count already specified\n");
				return 1;
			}
			cmdsim=2;
			break;
		default:
		case 'h':
		case '?':
			printf("Usage: %s [-1 | -2] [-o <filename>]\n", argv[0]);
			return 0;
		}
	}


	if((fd=open("/dev/block/bootdevice/by-name/misc", O_RDONLY|O_LARGEFILE))<0) {
		fprintf(stderr, "Failed to open misc area: %s\n", strerror(errno));
		return 1;
	}

	if((misclen=lseek(fd, 0, SEEK_END))<0) {
		fprintf(stderr, "Failed to get size of misc area: %s\n",
strerror(errno));
		return 1;
	} else if(misclen>=((off_t)1<<32)) {
		fprintf(stderr, "Misc area too big for 32-bit numbers\n");
		return 1;
	}

	if(ioctl(fd, BLKSSZGET, &blksz)<0) {
		fprintf(stderr, "Failed to get device block size: %s\n", strerror(errno));
		return 1;
	}

	/* our minimum standard */
	if(blksz<2048) blksz=2048;

	if((map=mmap(NULL, misclen, PROT_READ, MAP_SHARED, fd, 0))==MAP_FAILED) {
		fprintf(stderr, "Failed to mmap() misc area: %s\n",
strerror(errno));
		return 1;
	}

	if(close(fd)) {
		fprintf(stderr, "Close failed: %s\n", strerror(errno));
		return 1;
	}


	info=(struct sim_info *)(map+blksz*103);

	if(memcmp(info->magic, magic, sizeof(magic)))
		printf("SIM/Modem hardware information area missing\n");
	else {
		printf("LGE model information format version %hd.%hd\n", info->hwinfo_major, info->hwinfo_minor);

		printf("Misc area: Model Name: \"%s\" SIM count=%d DSDS mode is: %s\n",
info->lg_model_name, info->sim_num, info->sim_num==1?"none":"dsds");

		if(cmdsim&&(info->sim_num!=cmdsim))
			printf("SIM count found in misc area disagrees with command-line (hmm)\n");
	}


	if(outname) {
		if(!(out=fopen(outname, "wb"))) {
			fprintf(stderr,
"Failed to open \"%s\" for output, exiting.\n", outname);
			return 1;
		}
		if(fprintf(out, "SIM-MATCH-%c?", cmdsim+'0')<0) {
			fprintf(stderr, "Failed writing magic number to \"%s\"",
outname);
			return 1;
		}
	}


	if(out&&fwrite("BYTE....", 1, 8, out)!=8) {
		fprintf(stderr, "Failed writing byte match header to \"%s\"\n",
outname);
		return 1;
	}
	head=16; /* offset of match count */

	printf("Searching misc for byte 0x%02X...\n", cmdsim);

	for(cur=0, cnt=0; cur<misclen; ++cur) {
		if(map[cur]!=cmdsim) continue;

		++cnt;
		if(out) {
			*((uint32_t *)buf)=htobe32(cur);
			fwrite(buf, 1, 4, out);
		}
	}

	printf("Found %lu matches.\n", cnt);

	if(out) {
		if(fseek(out, head, SEEK_SET)) {
			fprintf(stderr, "Failed while seeking \"%s\"!\n",
outname);
			return 1;
		}
		*((uint32_t *)buf)=htobe32(cnt);
		if(fwrite(buf, 1, 4, out)!=4) {
			fprintf(stderr, "Failed while writing \"%s\"!\n",
outname);
			return 1;
		}


		if(fseek(out, 0, SEEK_END)) {
			fprintf(stderr, "Failed while seeking \"%s\"!\n",
outname);
			return 1;
		}


		if(fwrite("CHAR....", 1, 8, out)!=8) {
			fprintf(stderr,
"Failed writing character match header to \"%s\"\n", outname);
			return 1;
		}

		if((head=ftell(out)-4)<0) {
			fprintf(stderr,
"Failed to retrieve current file position on \"%s\"\n", outname);
			return 1;
		}
	}


	cmdsim+='0';
	printf("Searching misc for character '%c'...\n", cmdsim);

	for(cur=0, cnt=0; cur<misclen; ++cur) {
		if(map[cur]!=cmdsim) continue;

		++cnt;
		if(out) {
			*((uint32_t *)buf)=htobe32(cur);
			fwrite(buf, 1, 4, out);
		}
	}

	printf("Found %lu matches.\n", cnt);


	if(out) {
		if(fseek(out, head, SEEK_SET)) {
			fprintf(stderr, "Failed while seeking \"%s\"!\n",
outname);
			return 1;
		}
		*((uint32_t *)buf)=htobe32(cnt);
		if(fwrite(buf, 1, 4, out)!=4) {
			fprintf(stderr, "Failed while writing \"%s\"!\n",
outname);
			return 1;
		}

		if(fclose(out))
			fprintf(stderr,
"Failed while closing \"%s\", data recording may be incomplete\n", outname);
	}

	munmap(map, misclen);

	return 0;
}

