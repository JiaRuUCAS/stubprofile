#ifndef __COUNT_H__
#define __COUNT_H__

#include <vector>
#include <map>
#include <string>

#include "BPatch_Vector.h"


//#include "test.h"

class BPatch_addressSpace;
class BPatch_function;
class BPatch_object;
class BPatch_snippet;

class FuncMap;

#define USE_FUNCCNT

#define LIBCNT "/home/jr/stubprofile/build/lib/libprobe.so"

#ifdef USE_FUNCCNT
#define FUNC_PRE "funcc_count_pre"
#define FUNC_POST "funcc_count_post"
#define FUNC_INIT "funcc_count_init"
#define FUNC_EXIT "funcc_count_exit"
#define FUNC_TEXIT "funcc_count_thread_exit"
#define FUNCC_ARG "f:"
#else
#define FUNC_PRE "prof_count_pre"
#define FUNC_POST "prof_count_post"
#define FUNC_INIT "prof_init"
#define FUNC_EXIT "prof_exit"
#define FUNC_TEXIT "prof_thread_exit"
#define FUNCC_ARG "f:e:l:F:o:"
#endif /* ifdef USE_FUNCCNT */

#define PATTERN_ALL "(.*)"

class TargetFunc{
	public:
		BPatch_function *func;
		unsigned int index;

		TargetFunc(void);
		TargetFunc(BPatch_function *, unsigned int);
};

struct range {
	unsigned int min;
	unsigned int max;
};

class CountUtil {
	private:
		/* Command-line arguments */
		std::string pattern;
#ifndef USE_FUNCCNT
		std::string output;
#define OUTPUT_DEF "profile.data"
#define EVLIST_DEF "cpu-cycles"
		std::string evlist;
#define LOGFILE_DEF "profile.log"
		std::string logfile;
#define FREQ_DEF (0U)
		unsigned int freq;
#endif

		BPatch_addressSpace *as;

		BPatch_function *func_pre, *func_post;
		BPatch_function *func_init;
		BPatch_function *func_exit, *func_texit;

		struct range func_id_range;

		std::vector<TargetFunc> target_funcs;

	public:
		/* Get option string for parsing */
		static std::string getOptStr(void);
		/* Get usage string */
		static std::string getUsageStr(void);

		CountUtil(void) : pattern(PATTERN_ALL) {};

		/* Parse command-line options */
		bool parseOption(int opt, char *optarg);

		/* Set address space (for convenience) */
		void setAS(BPatch_addressSpace *addr) { as = addr; };

		/* Get function pattern */
		const std::string getPattern(void) { return pattern; };
		/* Get init function */
		BPatch_function *getInit(void) { return func_init; };

		/* Load all functions */
		bool loadFunctions(void);

		/* Get the list of target functions. */
		bool getTargetFuncs(void);
		/* Add target function to the list */
		void addTargetFunc(BPatch_function *func, unsigned idx);

		/* Insert counting functions into target functions */
		bool insertCount(void);

		/* Insert exit functions into target program */
		bool insertExit(std::string filter);

		/* Construct the argument list for init function */
		void buildInitArgs(std::vector<BPatch_snippet *> &args);
	private:
		/* Get all objects and filter out system libraries and
		 * dyninst libraries */
		void getUserObjs(BPatch_Vector<BPatch_object *> &objs);

		/* Match the 'pattern' and the functions in 'fmap'. */
		void matchFuncs(BPatch_object *obj, FuncMap *fmap,
						std::string pattern);

		/* Find a function in 'obj' based on its 'name' */
		BPatch_function *findFunction(BPatch_object *obj,
						std::string name);

		/* Calculate range of target function indices */
		void calculateRange(void);
};


#endif // __COUNT_H__
