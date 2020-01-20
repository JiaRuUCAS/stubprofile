#ifndef __EDIT_H__
#define __EDIT_H__

#include <vector>
#include <string>

#include "BPatch_Vector.h"
#include "count.h"
#include "test.h"

class BPatch_binaryEdit;

#define EDIT_CMD "edit"

class EditTest: public Test {
	private:
		std::string file;
		std::string output;
		BPatch_binaryEdit *editor;

		CountUtil count;

	public:
		EditTest(void);
		~EditTest(void);

		static void staticUsage(void);
		static Test *construct(void);
		bool parseArgs(int argc, char **argv);
		bool init(void);
		bool process(void);
		void destroy(void) {};

	private:
		bool insertInit(void);
};


#endif /* __EDIT_H__ */
