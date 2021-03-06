#ifndef __TEST_H__
#define __TEST_H__

#include "BPatch.h"

enum {
	TEST_MODE_ATTACH = 0,
	TEST_MODE_FUNCMAP,
	TEST_MODE_EDIT,
	TEST_MODE_HELP,
	TEST_MODE_NUM,
};

class Test {

	public:
		virtual ~Test(void) = default;

		virtual bool parseArgs(int argc, char **argv) = 0;
		virtual bool init(void) = 0;
		virtual bool process(void) = 0;
		virtual void destroy(void) = 0;
};

extern BPatch bpatch;

#endif // __TEST_H__
