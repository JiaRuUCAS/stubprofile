#include <stdio.h>
#include <sys/stat.h>

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_object.h"

#include "test.h"
#include "edit.h"
#include "funcmap.h"
#include "count.h"
#include "util.h"

using namespace std;
using namespace Dyninst;

Test *EditTest::construct(void)
{
	return new EditTest();
}

EditTest::EditTest(void)
{
}

EditTest::~EditTest(void)
{
}

bool EditTest::init(void)
{
	/* load ELF and construct addressSpace */
	editor = bpatch.openBinary(file.c_str(), true);
	if (!editor) {
		LOG_ERROR("Failed to open file %s", file.c_str());
		return false;
	}

	/* init CountUtil */
	count.setAS(editor);

	/* find target functions */
	if (!count.getTargetFuncs()) {
		LOG_ERROR("Failed to find target functions (%s)",
						count.getPattern().c_str());
		return false;
	}
	return true;
}

void EditTest::staticUsage(void)
{
	fprintf(stdout, "stubprofile %s -h\t\tShow the usage.\n",
					EDIT_CMD);
	fprintf(stdout, "stubprofile %s -i <input_ELF> [OPTIONS]\n"
					"%s  OPTIONS:\n\t-o <output>\n"
					"\t\tDefine the name and path of the output file\n"
					"\t\tto store the new ELF. Default is '<input_ELD>_new'.\n",
					EDIT_CMD, CountUtil::getUsageStr().c_str());
}

bool EditTest::parseArgs(int argc, char **argv)
{
	int c;
	char buf[64] = {'\0'};
	string optstr = CountUtil::getOptStr() + "i:o:h";

	while ((c = getopt(argc, argv, optstr.c_str())) != -1) {
		switch(c) {
			case 'i':
				// check whether the file exists
				if (access(optarg, F_OK) != 0) {
					LOG_ERROR("File %s doesn't exist.", optarg);
					return false;
				}

				file = optarg;
				break;

			case 'o':
				output = optarg;
				break;

			case 'h':
				staticUsage();
				exit(0);

			default:
				if (!count.parseOption(c, optarg)) {
					LOG_ERROR("Unknown option %c, usage:", c);
					staticUsage();
					return false;
				}
				break;
		}
	}

	if (file.size() == 0) {
		LOG_ERROR("No ELF file specific.");
		return false;
	}

	if (output.size() == 0) {
		output = file + "_new";
	}

	return true;
}

bool EditTest::insertInit(void)
{
	BPatch_Vector<BPatch_function *> funcs;
	BPatch_Vector<BPatch_snippet *> init_arg;
	BPatch_Vector<BPatch_point *> *init_point = NULL;
	BPatchSnippetHandle *handle = NULL;
	bool ret = true;
	string target_elf;

	LOG_DEBUG("Insert init function");

	count.buildInitArgs(init_arg);

	BPatch_funcCallExpr init_expr(*(count.getInit()),
					init_arg);

	editor->getImage()->findFunction("^_init", funcs);
	if (funcs.size() == 0) {
		LOG_ERROR("No _init function is found.");
		ret = false;
		goto free_arg;
	}

	target_elf = file.substr(file.find_last_of('/') + 1);
	LOG_DEBUG("Target ELF %s", target_elf.c_str());

	for (unsigned int i = 0; i < funcs.size(); i++) {
		BPatch_module *mod = funcs[i]->getModule();

		if (target_elf.compare(mod->getObject()->name()))
			continue;

		LOG_INFO("Insert init function to %s",
						mod->getObject()->name().c_str());
		handle = mod->getObject()->insertInitCallback(init_expr);
		if (!handle) {
			LOG_ERROR("Failed to insert init function to %s",
							mod->getObject()->name().c_str());
			ret = false;
			goto free_arg;
		}
		count.addTargetFunc(funcs[i], UINT_MAX);
	}

free_arg:
	for (unsigned int i = 0; i < init_arg.size(); i++) {
		delete init_arg[i];
	}
	return ret;
}

bool EditTest::process(void)
{
	// load counting functions
	if (!count.loadFunctions()) {
		LOG_ERROR("Failed to load counting functions");
		return false;
	}

	// insert counter
	if (!count.insertCount()) {
		LOG_ERROR("Failed to insert counting functions");
		return false;
	}

	// insert init function
	if (!insertInit()) {
		LOG_ERROR("Failed to insert init function");
		return false;
	}

	// insert exit function
	if (!count.insertExit(file.substr(
								file.find_last_of('/') + 1))) {
		LOG_ERROR("Failed to insert exit function");
		return false;
	}

	// write to file
	if (!editor->writeFile(output.c_str())) {
		LOG_ERROR("Failed to write new file %s",
						output.c_str());
		return false;
	}
	return true;
}
