#ifndef __TRACER_H__
#define __TRACER_H__

#include <vector>
#include <map>
#include <string>

#include "BPatch_Vector.h"

#include "test.h"

class BPatch_process;
class BPatch_function;
class BPatch_object;

class FuncMap;

#define TRACER_CMD "tracer"
#define TRACER_LIB "/home/jr/stubprofile/build/lib/libprofile.so"

class TracedFunc{
	public:
		BPatch_function *func;
		unsigned int index;

		TracedFunc(void);
		TracedFunc(BPatch_function *, unsigned int);
};

class TracerTest: public Test {

	private:
		int pid;
#define TRACER_PATTERN_ALL "(.*)"
		std::string func_pattern;

		BPatch_process *proc;
		std::vector<TracedFunc> trace_funcs;

		BPatch_function *pre_cnt;
		BPatch_function *post_cnt;

		void getTraceFunctions(BPatch_object *obj, FuncMap *fmap);
		void getUserObjects(BPatch_Vector<BPatch_object *> &objs);
		BPatch_function *findFunction(
						BPatch_object *obj, std::string name);
		bool callInit(BPatch_object *lib);
		bool insertExit(BPatch_object *lib);
		bool insertCount(BPatch_object *lib);

		void clearSnippets(void);

	public:
		TracerTest(void);
		TracerTest(int pid, const char *pattern);
		~TracerTest(void);

		static void staticUsage(void);
		static Test *construct(void);

		void usage(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void);
};






#endif // __TRACER_H__
