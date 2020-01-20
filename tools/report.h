#ifndef __REPORT_H__
#define __REPORT_H__

#include "test.h"

#define REPORT_CMD "report"

class ReportTest: public Test {
	private:

	public:
		ReportTest(void);
		~ReportTest(void);

		static void staticUsage(void);
		static Test *construct(void);

		void usage(void);
		bool parseArgs(int argc, char **argv);
		bool init(void) {};
		bool process(void);
		void destroy(void) {};
};



#endif /* __REPORT_H__ */
