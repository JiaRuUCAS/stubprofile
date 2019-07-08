#include <stdio.h>
#include <sys/stat.h>

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_process.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_object.h"

#include "test.h"
#include "tracer.h"
#include "funcmap.h"
#include "util.h"

using namespace std;
using namespace Dyninst;

TracedFunc::TracedFunc(void) :
		func(NULL), index(UINT_MAX)
{
}

TracedFunc::TracedFunc(BPatch_function *f, unsigned int i) :
		func(f), index(i)
{
}

Test *TracerTest::construct(void)
{
	return new TracerTest();
}

TracerTest::TracerTest(void) :
		pid(-1), func_pattern(TRACER_PATTERN_ALL)
{
}

TracerTest::TracerTest(int pid, const char *pattern) :
		pid(pid)
{
	if (pattern == NULL || strlen(pattern) == 0)
		func_pattern = TRACER_PATTERN_ALL;
	else
		func_pattern = pattern;
}

TracerTest::~TracerTest(void)
{
}

bool TracerTest::init(void)
{
	BPatch_Vector<BPatch_object *> objs;

	LOG_INFO("Attaching to process %d", pid);
	proc = bpatch.processAttach(NULL, pid);
	if (proc == NULL) {
		LOG_ERROR("Failed to attach to process %d", pid);
		return false;
	}

	LOG_INFO("Get list of user ELFs");
	getUserObjects(objs);

	for (unsigned i = 0; i < objs.size(); i++) {
		FuncMap fmap(objs[i]);

		LOG_INFO("Load function map for %s",
						objs[i]->pathName().c_str());
		if (!fmap.load(false)) {
			LOG_ERROR("Failed to load function map for %s",
							objs[i]->pathName().c_str());
			goto detach_out;
		}

		getTraceFunctions(objs[i], &fmap);
	}
	if (trace_funcs.size() == 0) {
		LOG_ERROR("No function matched pattern %s",
						func_pattern.c_str());
		goto detach_out;
	}

	return true;

detach_out:
	LOG_INFO("Detaching process %d", pid);
	proc->detach(true);
	return false;
}

bool TracerTest::insertCount(void)
{
	unsigned int fun_id = 0;

	for (unsigned i = 0; i < trace_funcs.size(); i++) {
		TracedFunc *tf = &trace_funcs[i];
		vector<BPatch_point *> *pentry = NULL, *pexit = NULL;
		BPatchSnippetHandle *handle = NULL;

		// find entry point
		pentry = tf->func->findPoint(BPatch_entry);
		if (!pentry) {
			LOG_ERROR("Failed to find entry point of func %s",
							tf->func->getName().c_str());
			continue;
		}

		// find exit point
		pexit = tf->func->findPoint(BPatch_exit);
		if (!pexit) {
			LOG_ERROR("Failed to find exit point of func %s",
							tf->func->getName().c_str());
			continue;
		}

		// create snippet
		vector<BPatch_snippet *> cnt_arg;
		BPatch_constExpr id(tf->index);

		cnt_arg.push_back(&id);

		BPatch_funcCallExpr pre_expr(*pre_cnt, cnt_arg);
		BPatch_funcCallExpr post_expr(*post_cnt, cnt_arg);

		// insert counting functions
		LOG_INFO("Insert pre_cnt into %s",
						tf->func->getName().c_str());
		handle = proc->insertSnippet(pre_expr, *pentry, BPatch_callBefore);
		if (!handle) {
			LOG_ERROR("Failed to insert pre_cnt to %s",
							tf->func->getName().c_str());
			return false;
		}

		LOG_INFO("Insert post_cnt into %s",
						tf->func->getName().c_str());
		handle = proc->insertSnippet(post_expr, *pexit, BPatch_callAfter);
		if (!handle) {
			LOG_ERROR("Failed to insert post_cnt to %s",
							tf->func->getName().c_str());
			return false;
		}
	}

	return true;
}

bool TracerTest::process(void)
{
	BPatch_object *libcnt = NULL;

	// load libfunccnt.so
	LOG_INFO("Load libfunccnt.so");
	libcnt = proc->loadLibrary(TRACER_LIB);
	if (!libcnt) {
		LOG_ERROR("Failed to load %s", TRACER_LIB);
		return false;
	}

	// load functions
	LOG_INFO("Load counting functions");
	pre_cnt = findFunction(libcnt, "funcc_count_pre");
	post_cnt = findFunction(libcnt, "funcc_count_post");
	if (!pre_cnt || !post_cnt) {
		LOG_ERROR("Failed to load counting functions");
		return false;
	}

	// insert counting functions
	if (!insertCount()) {
		LOG_ERROR("Failed to insert functions");
		return false;
	}

	// continue the tracee
	if (!proc->continueExecution()) {
		LOG_ERROR("Failed to continue execution");
		return false;
	}

	// wait for termination of tracee
	LOG_INFO("Wait for termination");
	while (!proc->isTerminated()) {
		bpatch.waitForStatusChange();
	}

	return true;
}

void TracerTest::destroy(void)
{
	if (proc) {
		LOG_ERROR("Detach process %d", pid);
		proc->detach(true);
	}
}

void TracerTest::getTraceFunctions(BPatch_object *obj, FuncMap *fmap)
{
	BPatch_Vector<BPatch_function *> funcs;

	fmap->printAll();

	obj->findFunction(func_pattern.c_str(), funcs, false);
	for (unsigned j = 0; j < funcs.size(); j++) {
		unsigned int index = UINT_MAX;

		index = fmap->getFunctionID(funcs[j]->getName());
		if (index == UINT_MAX) {
			LOG_ERROR("Cannot find %s in map %s",
							funcs[j]->getName().c_str(),
							obj->name().c_str());
			continue;
		}

		trace_funcs.push_back(TracedFunc(funcs[j], index));
		LOG_INFO("Trace function %s:%s, ID %u",
						obj->name().c_str(),
						funcs[j]->getName().c_str(), index);
	}
}

#define OSLIBPATH_PREFIX "/lib/"
#define DYNINSTLIB_PREFIX "/home/jr/dyninst-master/build/lib/"
void TracerTest::getUserObjects(BPatch_Vector<BPatch_object *> &objs)
{
	BPatch_Vector<BPatch_object *> allobjs;
	BPatch_object *obj = NULL;
	string osprefix(OSLIBPATH_PREFIX);
	string diprefix(DYNINSTLIB_PREFIX);

	proc->getImage()->getObjects(allobjs);
	for (unsigned i = 0; i < allobjs.size(); i++) {
		std::string path;

		obj = allobjs[i];
		path = obj->pathName();

		if (!path.compare(0, osprefix.size(), osprefix) ||
						!path.compare(0, diprefix.size(), diprefix)) {
			LOG_INFO("\tSkip object %s", obj->name().c_str());
			continue;
		}
		objs.push_back(obj);
	}
}

void TracerTest::staticUsage(void)
{
	fprintf(stdout, "./dyninst-test %s -p <pid> [OPTIONS]\n",
					TRACER_CMD);
	fprintf(stdout, "  OPTIONS:\n"
			"    -f <function_pattern>  Define the matching pattern\n"
			"                           for traced function. Default\n"
			"                           is matching all functions.\n");
}

void TracerTest::usage(void)
{
	staticUsage();
}

bool TracerTest::parseArgs(int argc, char **argv)
{
	int c;
	char buf[64] = {'\0'};

	while ((c = getopt(argc, argv, "p:f:")) != -1) {
		switch(c) {
			case 'p':
				// check whether the process exists
				sprintf(buf, "/proc/%s", optarg);
				if (access(buf, F_OK) != 0) {
					LOG_ERROR("Process %s doesn't exist.", optarg);
					return false;
				}

				pid = atoi(optarg);
				break;

			case 'f':
				func_pattern = optarg;
				break;

			default:
				LOG_ERROR("Unknown option %c, usage:", c);
				usage();
				return false;
		}
	}

	if (pid == -1) {
		LOG_ERROR("No process specified, usage:");
		usage();
		return false;
	}

	return true;
}

BPatch_function *TracerTest::findFunction(BPatch_object *obj, string name)
{
	BPatch_Vector<BPatch_function *> funcs;

	obj->findFunction(name, funcs, false);
	if (funcs.size() == 0) {
		LOG_ERROR("Cannot find fucntion %s in %s",
						name.c_str(), obj->pathName().c_str());
		return NULL;
	}

	return funcs[0];
}
