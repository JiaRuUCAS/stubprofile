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

TracerTest::~TracerTest(void)
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

struct range {
	unsigned min, max;
};

static void __getIndexRange(vector<TracedFunc> &list, struct range *r)
{
	unsigned int i = 0, max = 0, min = UINT_MAX;

	for (i = 0; i < list.size(); i++) {
		if (list[i].index == UINT_MAX)
			continue;
		if (max < list[i].index)
			max = list[i].index;
		if (min > list[i].index)
			min = list[i].index;
	}
	r->min = min;
	r->max = max;
}

bool TracerTest::callInit(BPatch_object *lib)
{
	BPatch_function *init_func = NULL;
	void *init_ret = NULL;
	bool err;
	struct range range;

	LOG_INFO("Load init function");
	init_func = findFunction(lib, "prof_init");
	if (!init_func) {
		LOG_ERROR("Failed to load init function");
		return false;
	}

	LOG_INFO("Insert init function");
	vector<BPatch_snippet *> init_arg;
	BPatch_constExpr evlist("cpu-cycles");
	BPatch_constExpr logfile("");

	__getIndexRange(trace_funcs, &range);
	BPatch_constExpr min_id(range.min);
	BPatch_constExpr max_id(range.max);
	BPatch_constExpr freq((unsigned)0);

	init_arg.push_back(&evlist);
	init_arg.push_back(&logfile);
	init_arg.push_back(&min_id);
	init_arg.push_back(&max_id);
	init_arg.push_back(&freq);

	BPatch_funcCallExpr init_expr(*init_func, init_arg);

	LOG_INFO("Execute init function");
	init_ret = proc->oneTimeCode(init_expr, &err);
	if (err) {
		LOG_ERROR("Failed to execute init function");
		return false;
	}
	else if (init_ret == (void *)-1) {
		LOG_ERROR("Init function returns error, %p", init_ret);
		return false;
	}

	return true;
}

bool TracerTest::insertCount(BPatch_object *lib)
{
	LOG_INFO("Load counting functions");
	pre_cnt = findFunction(lib, "prof_count_pre");
	post_cnt = findFunction(lib, "prof_count_post");
	if (!pre_cnt || !post_cnt) {
		LOG_ERROR("Failed to load counting functions");
		return false;
	}

	LOG_INFO("Insert counting functions");
	for (unsigned i = 0; i < trace_funcs.size(); i++) {
		TracedFunc *tf = &trace_funcs[i];
		vector<BPatch_point *> *pentry = NULL, *pexit = NULL;
		BPatchSnippetHandle *handle = NULL;

		if (tf->index == UINT_MAX)
			continue;

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

bool TracerTest::insertExit(BPatch_object *lib)
{
	BPatch_function *fexit = NULL;
	BPatch_Vector<BPatch_snippet *> exit_arg;
	BPatch_Vector<BPatch_function *> funcs;
	BPatch_Vector<BPatch_point *> *exit_point = NULL;
	BPatchSnippetHandle *handle = NULL;

	// insert global exit function into _fini
	fexit = findFunction(lib, "prof_exit");
	if (!fexit) {
		LOG_ERROR("Failed to insert exit function");
		return false;
	}

	BPatch_funcCallExpr exit_expr(*fexit, exit_arg);

	proc->getImage()->findFunction("_fini", funcs);
	for (unsigned int i = 0; i < funcs.size(); i++) {
		BPatch_module *mod = funcs[i]->getModule();

		if (!mod->isSharedLib()) {
			LOG_INFO("Insert exit function to %s",
							mod->getObject()->name().c_str());
			mod->getObject()->insertFiniCallback(exit_expr);

			trace_funcs.push_back(TracedFunc(funcs[i], UINT_MAX));
		}
	}

	// insert thread exit function into pthread_exit
	// check if pthread_exit exists
	funcs.clear();
	proc->getImage()->findFunction("pthread_exit", funcs);
	if (funcs.size() == 0) {
		LOG_INFO("No pthread_exit found");
		return true;
	}

	exit_point = funcs[0]->findPoint(BPatch_entry);
	if (!exit_point) {
		LOG_ERROR("Failed to find exit point for pthread_exit");
		return true;
	}

	fexit = findFunction(lib, "prof_thread_exit");
	if (!fexit) {
		LOG_ERROR("Failed to insert thread exit function");
		return false;
	}

	BPatch_funcCallExpr thread_exit_expr(*fexit, exit_arg);

	handle = proc->insertSnippet(thread_exit_expr,
					*exit_point, BPatch_callBefore);
	if (!handle) {
		LOG_ERROR("Failed to insert thread exit function");
		return false;
	}

	return true;
}

bool TracerTest::process(void)
{
	BPatch_object *lib = NULL;

	// load libfunccnt.so
	LOG_INFO("Load libprofile.so");
	lib = proc->loadLibrary(TRACER_LIB);
	if (!lib) {
		LOG_ERROR("Failed to load %s", TRACER_LIB);
		return false;
	}

	// insert init function
	if (!callInit(lib)) {
		LOG_ERROR("Failed to call init function");
		return false;
	}

	if (!insertExit(lib)) {
		LOG_ERROR("Failed to insert exit function");
		return false;		
	}

	// insert counting functions
	if (!insertCount(lib)) {
		LOG_ERROR("Failed to insert counting functions");
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

void TracerTest::clearSnippets(void)
{
	if (proc->isTerminated())
		return;

	while (!proc->isStopped()) {
		LOG_INFO("Try to stop the tracee");
		proc->stopExecution();
	}

	LOG_INFO("Clear all snippets");
	for (unsigned int i = 0; i < trace_funcs.size(); i++)
		trace_funcs[i].func->removeInstrumentation(false);
}

void TracerTest::destroy(void)
{
	if (!proc)
		return;

	clearSnippets();

	LOG_INFO("Detach process %d", pid);
	proc->detach(true);
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

void TracerTest::getUserObjects(BPatch_Vector<BPatch_object *> &objs)
{
	BPatch_Vector<BPatch_object *> allobjs;
	BPatch_object *obj = NULL;
	string osprefix(OSLIBPATH_PREFIX);

	proc->getImage()->getObjects(allobjs);
	for (unsigned i = 0; i < allobjs.size(); i++) {
		std::string path;

		obj = allobjs[i];
		path = obj->pathName();

		if (!path.compare(0, osprefix.size(), osprefix) ||
						obj->name().find("dyninst") != (size_t)-1) {
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
