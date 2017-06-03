/**
 * @file test.cc
 * @package de.atwillys.cc.swl
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 * -----------------------------------------------------------------------------
 * @sw: todo: extend tests.
 */
#include "test.hh"
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
typedef sw::srecord::block_type block_type;
typedef sw::srecord::block_container_type block_container_type;
typedef sw::srecord::address_type address_type;
typedef sw::srecord::value_type value_type;
typedef sw::srecord::size_type size_type;
typedef sw::srecord::data_type data_type;

srecord srec;

block_type mkblock_seq(address_type adr, value_type startval, size_type size)
{
  block_type blk(adr);
  while(size--) blk.bytes().push_back(startval++);
  return blk;
}

/**
 * @req: parse() shall ignore whitespaces
 * @req: parse() shall ignore character case
 * @req: parse() shall ignore empty lines
 * @req; header_str() shall ignore tailing whitespaces
 */
void test_parse_example_s19()
{
  string test_record = string(
    "\n\r"
    "\r"
    "s0 0f 0000 68656c6c6f21202020200000  3b\n"
    "\n"
    "S1 1F00007C0802A69001000\t49421FFF07C6C1B787C8C23783C6000003863000026\r\n"
    "S1 1F001C4BFFFFE5398000007D83637880010014382100107C0803A64E800020E9\n"
    "\r\n"
    "S1 11 0038 48656C6C6F20776F726C642E0A00 42\n"
    "S5  030003F9\n"
    "S9\t030000FC\n"
    "\n"
  );

  test_expect(srec.parse(std::move(test_record)));
  test_expect(srec.header_str() == "hello!");
  if(test_expect_cond(srec.blocks().size() == 1)) {
    test_expect(srec.good());
    test_expect(!srec.blocks().front().empty());
    test_expect(srec.blocks().front().sadr() == 0x0000);
    test_expect(srec.blocks().front().eadr()   == 0x0046);
    test_expect(srec.blocks().front().size()  == 70);
    test_expect(srec.blocks().front().bytes().size()  == 70);
  }
}

void test_compose()
{
  srec.clear();
  srec.blocks().push_back(srecord::block_type(0x0010, { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 }));
  if(!test_expect_cond(srec.good() && !srec.blocks().empty())) return;
  std::stringstream ss;
  test_expect(srec.compose(ss));
  test_comment(string("Composed:\n") + ss.str());

  srecord srec2;
  srec2.parse(ss.str());
  test_expect(srec2.good());
  test_expect(srec2.header_str() == srec.header_str());
  if(test_expect_cond(srec2.blocks().size() == srec.blocks().size())) {
    test_expect(srec2.blocks().front() == srec2.blocks().front());
  }
}

void test_range_get_set()
{
  srec.clear();
  srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0060, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
  srec_dump();
  srec.set_range(0x0008, mkblock_seq(0x0, 0xaa, 29).bytes());
  srec_dump();
  srec.set_range(mkblock_seq(0x0018, 0x44, 0x60));
  srec_dump();
  srec.set_range(mkblock_seq(0x0000, 0x00, 0x80));
  srec_dump();
}

void test_merge()
{
  srec.clear();
  srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0030, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
  srec_dump();
  srec.merge(0xfe);
  srec_dump();
}

void test_find()
{
  srec.clear();
  srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0030, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
  srec_dump();
  data_type seq{125};
  address_type adr = srec.find(seq, 0x70);
  if(adr == srec.eadr()) {
    cout << "Not found." << endl;
  } else {
    cout << "Found at: " << hex << adr << endl;
  }
}

void test_remove()
{
  srec.clear();
  srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0060, 0xcc, 16));
  srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
  srec_dump();
  srec.remove_range(0x24, 0x28);
  srec_dump();
}

void test_push_block()
{
  srec.clear();
  srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0060, 0xcc, 16));
  srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
  srec_dump();
}

void test_load_file()
{
  srec.clear();
  test_expect( srecord::load(RESOURCE_DIRECTORY TEST_FILE, srec) );
  test_expect( !srec.blocks().empty() );
  test_expect( srec.good() );
  srec_dump();
}


void test()
{
  test_expect_noexcept( test_parse_example_s19() );
  test_expect_noexcept( test_compose() );
  test_expect_noexcept( test_range_get_set() );
  test_expect_noexcept( test_merge() );
  test_expect_noexcept( test_push_block() );
  test_expect_noexcept( test_load_file() );
}
