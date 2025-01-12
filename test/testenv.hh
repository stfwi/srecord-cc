#ifndef TESTING_ENVIRONMENT_HH
#define TESTING_ENVIRONMENT_HH
#define WITH_MICROTEST_MAIN       /* opt-in: int main(){} generation */
// #define WITH_MICROTEST_GENERATORS /* opt-in: sequence and container generation functions */
#define WITH_MICROTEST_ANSI_COLORS  /* opt-in: ANSI coloring for console/TTY out streams */
// #define WITHOUT_MICROTEST_RANDOM    /* opt-out: No utest::random() functions */
// #define WITH_MICROTEST_TMPDIR       /* opt-in: !experimental! Temporary directory creation and handling */
// #define WITH_MICROTEST_TMPFILE      /* opt-in: !experimental1 Temporary file creation and handling */
#include <test/microtest/include/microtest.hh>
#endif
