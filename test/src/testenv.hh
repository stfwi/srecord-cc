/**
 * @package de.atwillys.cc.test
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 *
 * -----------------------------------------------------------------------------
 *
 * Testing wrapper for swlib-cc.
 *
 * -----------------------------------------------------------------------------
 */
#ifndef UTEST_HH
#define	UTEST_HH
#include "microtest.hh"
#include <typeinfo>
#include <iostream>
#include <fstream>
#include <deque>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

#ifndef DIRECTORY_SEPARATOR
  #if defined(_MSCVER) || defined(_WINDOWS_) || defined(__WIN32) || defined(__WIN64)
    #define DIRECTORY_SEPARATOR "\\"
  #else
    #define DIRECTORY_SEPARATOR "/"
  #endif
#endif

using namespace std;

string mn_program_path;
vector<string> program_arguments_;
map<string, string> program_environment_;

std::string program_path() { return mn_program_path; }
vector<string>& program_arguments() noexcept { return program_arguments_; }
map<string, string>& program_environment() noexcept { return program_environment_; }

void test();

int main(int argc, char* argv[], char** envv)
{
  #ifndef UTEST_NOFRAME
  test_buildinfo();
  try {
    ::setlocale(LC_ALL, "en_US.UTF-8");
    #ifdef __GNUC__
    std::setlocale(LC_ALL, "en_US.UTF-8");
    #endif
    mn_program_path = argv[0] ? argv[0] : "";
    for(int i=1; (i<argc) && (argv[i]); ++i) {
      program_arguments_.push_back(argv[i]);
    }
    for(size_t i=0; envv[i] && envv[i+1]; i+=2) {
      if(envv[i][0]) {
        program_environment_[envv[i]] = envv[i+1];
      }
    }
    test();
  }
  #define addex(WHAT) catch(const WHAT & e) { ::sw::utest::test::fail(); cout << "Unhandled " \
                << #WHAT << ": " << e.what() << endl;  }
  addex(std::bad_cast)
  addex(std::bad_typeid)
  addex(std::invalid_argument)
  addex(std::length_error)
  addex(std::logic_error)
  addex(std::bad_exception)
  addex(std::exception)
  #undef addex
  catch(const char* e) { ::sw::utest::test::fail(); cout << e << endl; }
  catch(...) { ::sw::utest::test::fail(); cout << "Unhandled exception" << endl; }
  return ::sw::utest::test::summary();
  #else
  test();
  return 0;
  #endif
}
#endif
