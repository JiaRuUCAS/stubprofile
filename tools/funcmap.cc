#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <cstring>
#include <string>
#include <set>

#include "BPatch.h"
#include "BPatch_object.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_function.h"
#include "BPatch_module.h"

#include "util.h"
#include "funcmap.h"

using namespace std;
using namespace Dyninst;

static set<string> skiplist;

static void initSkipList(void)
{
	if (skiplist.size() > 0)
		return;

	skiplist.insert("_init");
	skiplist.insert("_fini");
	skiplist.insert("__popcountdi2");
	skiplist.insert("__do_global_dtors_aux");
}

FuncMap::FuncMap(std::string name, std::string path) :
		obj(NULL)
{
	elf_path = path;
	elf_name = name;

	if (skiplist.size() == 0)
		initSkipList();
}

FuncMap::FuncMap(string path) :
		elf_path(path),
		obj(NULL)
{
	size_t pos = 0;

	pos = elf_path.find_last_of('/');
	if (pos == (size_t)-1)
		elf_name = elf_path;
	else
		elf_name = elf_path.substr(pos + 1);
}

FuncMap::FuncMap(BPatch_object *target) :
		obj(target)
{
	elf_path = obj->pathName();
	elf_name = obj->name();

	if (skiplist.size() == 0)
		initSkipList();
}

uint8_t FuncMap::checkState(void)
{
	time_t last_update;
	struct stat fileinfo;
	int ret = 0;
	string cachefile(FUNCMAP_DIR);

	// check elf file
	ret = stat(elf_path.c_str(), &fileinfo);
	if (ret < 0)
		return FUNCMAP_STATE_ENOELF;
	last_update = fileinfo.st_mtime;

	// check cache file
	cachefile += elf_name;
	ret = stat(cachefile.c_str(), &fileinfo);
	if (ret < 0) {
		// if cache doesn't exist, create(update) it.
		if (errno == ENOENT)
			return FUNCMAP_STATE_UPDATE;
		// Otherwise, error
		return FUNCMAP_STATE_ECACHE;
	}
	// if cache exists, check its last-updated time
	else {
		// if cache is out-of-date, update
		if (difftime(last_update, fileinfo.st_mtime) > 0)
			return FUNCMAP_STATE_UPDATE;
		// if cache is empty, update
		else if (fileinfo.st_size == 0)
			return FUNCMAP_STATE_UPDATE;
		else
			return FUNCMAP_STATE_CACHED;
	}
}

void FuncMap::addFunction(const char *func)
{
	string funcname(func);
	unsigned index = 0;

	// check if exists
	if (func_indices.count(funcname))
		return;

	// filter
	if (skiplist.count(func))
		return;

	index = funcs.size();
	funcs.push_back(funcname);
	func_indices.insert(pair<string, unsigned int>(funcname, index));

	LOG_DEBUG("[%s] Function %u: %s",
					elf_path.c_str(), index, func);
}

bool FuncMap::loadFromCache(void)
{
	FILE *file = NULL;
	string cachefile(FUNCMAP_DIR);
	char str[FUNC_NAME_MAX] = {'\0'};
	unsigned len = 0;

	cachefile += elf_name;
	file = fopen(cachefile.c_str(), "r");
	if (file == NULL) {
		LOG_ERROR("Failed to open cache file %s, err %d",
						cachefile.c_str(), errno);
		return false;
	}

	while (!feof(file)) {
		fgets(str, FUNC_NAME_MAX, file);
		len = strlen(str);
		if (str[len - 1] == '\n')
			str[len - 1] = '\0';
		addFunction(str);
	}
	return true;
}

bool FuncMap::buildDir(void)
{
	string path(FUNCMAP_DIR);
	ssize_t pos = 0;

	while (1) {
		pos = path.find_first_of('/', pos);
		if (pos == 0) {
			pos++;
			continue;
		}
		else if (pos == (ssize_t)-1)
			break;

		string subpath = path.substr(0, pos);

		// check if exists
		if (access(subpath.c_str(), F_OK) != 0) {
			if (mkdir(subpath.c_str(), 0755) != 0) {
				LOG_ERROR("Failed to create directory %s, err %d",
								subpath.c_str(), errno);
				return false;
			}
		}
		pos++;
	}
	return true;
}

bool FuncMap::updateCache(void)
{
	FILE *file = NULL;
	string cachefile(FUNCMAP_DIR);

	// check and create the dictories
	if (!buildDir())
		return false;

	cachefile += elf_name;
	file = fopen(cachefile.c_str(), "w");
	if (file == NULL) {
		LOG_ERROR("Failed to open cache file %s",
						cachefile.c_str());
		return false;
	}

	for (unsigned i = 0; i < funcs.size(); i++)
		fprintf(file, "%s\n", funcs[i].c_str());
	return true;
}

bool FuncMap::loadFromELF(void)
{
	/* if there is no existing open BPatch_object for this ELF, open
	 * this ELF by BPatch_binaryEdit.
	 */
	if (obj == NULL) {
		BPatch_Vector<BPatch_function *> funclist;
		BPatch_binaryEdit *binary = NULL;

		binary = bpatch.openBinary(elf_path.c_str(), false);
		if (!binary) {
			LOG_ERROR("Failed to open ELF file %s",
							elf_path.c_str());
			return false;
		}

		if (!(binary->getImage()->getProcedures(funclist, false))) {
			LOG_ERROR("Failed to get function list from "
					  "BPatch_binaryEdit(%s)", elf_path.c_str());
			return false;
		}

		// generate function map		
		for (unsigned i = 0; i < funclist.size(); i++)
			addFunction(funclist[i]->getName().c_str());
	}
	// get the function list directly from BPatch_object
	else {
		BPatch_Vector<BPatch_module *> mods;

		obj->modules(mods);
		for (unsigned i = 0; i < mods.size(); i++) {
			BPatch_Vector<BPatch_function *> funcs;

			mods[i]->getProcedures(funcs, false);
			for (unsigned j = 0; j < funcs.size(); j++) {
				addFunction(funcs[j]->getName().c_str());
			}
		}
	}

	// update cache
	if (!updateCache()) {
		LOG_ERROR("Failed to update cache");
		return false;
	}
	return true;
}

bool FuncMap::load(bool force_update)
{
	uint8_t state;

	if (force_update) {
		LOG_INFO("Force updating. Load function map from ELF file.");
		return loadFromELF();
	}

	// check the state of cache
	state = checkState();
	switch (state) {
		case FUNCMAP_STATE_CACHED:
			LOG_INFO("Load function map from cache");
			return loadFromCache();
		case FUNCMAP_STATE_UPDATE:
			LOG_INFO("Load function map from ELF file");
			return loadFromELF();
		default:
			LOG_ERROR("Error state: %d", state);
			return false;
	}
}

unsigned int FuncMap::getFunctionID(string func)
{
	map<string, unsigned>::iterator iter;

	iter = func_indices.find(func);
	if (iter == func_indices.end())
		return UINT_MAX;
	return iter->second;
}

unsigned FuncMap::getFunctionID(const char *func)
{
	return getFunctionID(string(func));
}

void FuncMap::printAll(void)
{
	map<string, unsigned>::iterator iter;

	for (iter = func_indices.begin(); iter != func_indices.end(); iter++)
		LOG_INFO("FUNC[%u]: %s", iter->second, iter->first.c_str());
	LOG_INFO("Total %lu functions", func_indices.size());
}
