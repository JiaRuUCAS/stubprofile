/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.	We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "common/MappedFile.h"
#include "common/pathName.h"
#include <iostream>
using namespace std;

dyn_hash_map<std::string, MappedFile *> MappedFile::mapped_files;

MappedFile *MappedFile::createMappedFile(std::string fullpath_)
{
	if (mapped_files.find(fullpath_) != mapped_files.end()) {
		MappedFile *ret = mapped_files[fullpath_];
		if (ret->can_share) {
			ret->refCount++;
			return ret;
		}
	}

	bool ok = false;
	MappedFile *mf = new MappedFile(fullpath_, ok);

	if (!mf)
		return NULL;

	if (!ok) {
		delete mf;
		return NULL;
	}

	mapped_files[fullpath_] = mf;

	return mf;
}

MappedFile::MappedFile(std::string fullpath_, bool &ok) :
		fullpath(fullpath_),
		map_addr(NULL),
		fd(-1),
		remote_file(false),
		did_mmap(false),
		did_open(false),
		can_share(true),
		refCount(1)
{
	ok = check_path(fullpath);
	if (!ok)
		return;

	ok = open_file();
	if (!ok)
		return;

	ok = map_file();
}

MappedFile *MappedFile::createMappedFile(
				void *loc, unsigned long size_, const std::string &name)
{
	bool ok = false;
	MappedFile *mf = new MappedFile(loc, size_, name, ok);

	if (!mf || !ok) {
		if (mf)
			delete mf;
		return NULL;
	}

	return mf;
}

MappedFile::MappedFile(void *loc, unsigned long size_,
				const std::string &name, bool &ok) :
		fullpath(name),
		map_addr(NULL),
		remote_file(false),
		did_mmap(false),
		did_open(false),
		can_share(true),
		refCount(1)
{
	ok = open_file(loc, size_);
}

bool MappedFile::open_file(void *loc, unsigned long size_)
{
	map_addr = loc;
	file_size = size_;
	did_open = false;
	fd = -1;
	return true;
}

void MappedFile::closeMappedFile(MappedFile *&mf)
{
	if (!mf) {
		fprintf(stderr, "%s[%d]:	BAD NEWS:	called closeMappedFile(NULL)\n",
						FILE__, __LINE__);
		return;
	}

	mf->refCount--;

	if (mf->refCount <= 0) {
		dyn_hash_map<std::string, MappedFile *>::iterator iter;

		iter = mapped_files.find(mf->pathname());
		if (iter != mapped_files.end()) {
			mapped_files.erase(iter);
		}

		delete mf;
		mf = NULL;
	}
}

bool MappedFile::clean_up()
{
	if (did_mmap) {
		if (!unmap_file()) goto err;
	}
	if (did_open) {
		if (!close_file()) goto err;
	}
	return true;

err:
	fprintf(stderr, "%s[%d]:	error unmapping file %s\n", 
				FILE__, __LINE__, fullpath.c_str() );
	return false;
}

MappedFile::~MappedFile()
{
	//	warning, destructor should not allowed to throw exceptions
	if (did_mmap)
		unmap_file();
	if (did_open) 
		close_file();
}

bool MappedFile::check_path(std::string &filename)
{
	struct stat statbuf;

	if (0 != stat(filename.c_str(), &statbuf)) {
		char ebuf[1024];
		sprintf(ebuf, "stat: %s", strerror(errno));
		goto err;
	}

	file_size = statbuf.st_size;

	return true;

err:
	return false;
}

bool MappedFile::open_file()
{
	fd = open(fullpath.c_str(), O_RDONLY);
	if (-1 == fd) {
		char ebuf[1024];

		sprintf(ebuf, "open(%s) failed: %s", fullpath.c_str(), strerror(errno));
		goto err;
	}

	did_open = true;
	return true;

err:
	fprintf(stderr, "%s[%d]: failed to open file\n", FILE__, __LINE__);
	return false;
}

bool MappedFile::map_file()
{
	char ebuf[1024];
	int mmap_prot	= PROT_READ;
	int mmap_flags = MAP_SHARED;

	map_addr = mmap(0, file_size, mmap_prot, mmap_flags, fd, 0);
	if (MAP_FAILED == map_addr) {
		sprintf(ebuf, "mmap(0, %lu, prot=0x%x, flags=0x%x, %d, 0): %s", 
					file_size, mmap_prot, mmap_flags, fd, strerror(errno));
		goto err;
	}

	did_mmap = true;
	return true;

err:
	return false;
}

bool MappedFile::unmap_file()
{
	if (remote_file) {
		return true;
	}

	if (0 != munmap(map_addr, file_size)) {
		fprintf(stderr, "%s[%d]: failed to unmap file\n", FILE__, __LINE__);
		return false;
	}

	map_addr = NULL;
	return true;
}

bool MappedFile::close_file()
{
	if (remote_file) {
		return true;
	}

	if (-1 == close(fd)) {
		fprintf(stderr, "%s[%d]: failed to close file\n", FILE__, __LINE__);
		return false;
	}

	return true;
}

std::string MappedFile::pathname() 
{
	return fullpath;
}

std::string MappedFile::filename() 
{
	return extract_pathname_tail(fullpath);
}

void MappedFile::setSharing(bool s)
{
	can_share = s;
}

bool MappedFile::canBeShared()
{
	return can_share;
}
