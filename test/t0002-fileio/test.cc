/**
 * @file test.cc
 * @package de.atwillys.cc.swl
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 * -----------------------------------------------------------------------------
 * @sw: todo: extend tests, add fuzzy.
 */
#include "testenv.hh"
#include <sw/srecord.hh>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#ifndef RESOURCE_DIRECTORY
#define RESOURCE_DIRECTORY "res/"
#endif

#ifndef TEST_FILE
#define TEST_FILE "test0.s19"
#endif

#define srec_dump() { stringstream sss; srec.dump(sss); test_comment(sss.str()); }

using namespace std;
using sw::srecord;

srecord srec;

void test_load_file()
{
  {
    srec.clear();
    test_expect( srecord::load(RESOURCE_DIRECTORY TEST_FILE, srec) );
    test_expect( !srec.blocks().empty() );
    test_expect( srec.good() );
    srec_dump();
  }
  {
    srecord raii = srecord::load(RESOURCE_DIRECTORY TEST_FILE);
    test_expect( !raii.blocks().empty() );
  }
  {
    srecord raii_fail = srecord::load(RESOURCE_DIRECTORY TEST_FILE ".nonexisting");
    test_expect( raii_fail.blocks().empty() );
    test_expect( raii_fail.error() == srecord::e_load_open_failed );
  }
}


void test(const vector<string>& args)
{
  (void)args;
  test_expect_noexcept( test_load_file() );
}
