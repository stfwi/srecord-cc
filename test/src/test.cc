/**
 * @file test.cc
 * @package de.atwillys.cc.swl
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 * -----------------------------------------------------------------------------
 * @sw: todo: extend tests, add fuzzy.
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

void test_strict_parsing()
{
  // No header
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string("S309FFFFFFFC0200E0FF1C");
    test_expect( (!srec.parse(data)) && (srec.error() == srecord::e_parse_missing_s0) );
    if(!srec.good() && (srec.error() != srecord::e_parse_missing_s0)) test_note( srec.error_message() );
  }
  // Overlapping data
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
      "S11F001C4BFFFFE5398000007D83637880010014382100107C0803A64E800020E9\n"
      "S111003A48656C6C6F20776F726C642E0A0040\n"
      "S111003848656C6C6F20776F726C642E0A0042\n" // overlapping line, only error when parsing strict.
      "S9030000FC\n"
    );
    test_expect ((!srec.parse(data)) && (srec.error() == srecord::e_validate_overlapping_blocks));
    if(!srec.good() && (srec.error() != srecord::e_validate_overlapping_blocks)) test_note( srec.error_message() );
  }
  // Line count mismatch if S5 is given
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
      "S11F001C4BFFFFE5398000007D83637880010014382100107C0803A64E800020E9\n"
      "S111003A48656C6C6F20776F726C642E0A0040\n"
      "S5030007F5\n" // incorrect line count
    );
    test_expect ((!srec.parse(data)) && (srec.error() == srecord::e_parse_line_count_mismatch));
    if(!srec.good() && (srec.error() != srecord::e_parse_line_count_mismatch)) test_note( srec.error_message() );
  }
  // Unacceptable char
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000W026\n" // <-- W in the line data range is invalid
    );
    test_expect ((!srec.parse(data)) && (srec.error() == srecord::e_parse_unacceptable_character));
    if(!srec.good() && (srec.error() != srecord::e_parse_unacceptable_character)) test_note( srec.error_message() );
  }
  // Checksum
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000027\n" // 0x26->0x27
    );
    test_expect ((!srec.parse(data)) && (srec.error() == srecord::e_parse_chcksum_incorrect));
    if((!srec.good()) && (srec.error() != srecord::e_parse_chcksum_incorrect)) test_note( srec.error_message() );
  }
  // Invalid record type
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "SC1F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n" // SC invalid
    );
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_invalid_record_type));
    if((!srec.good()) && (srec.error() != srecord::e_parse_invalid_record_type)) test_note( srec.error_message() );
  }
  // Not starting with S
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "A11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
    );
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_line_not_starting_with_s));
    if((!srec.good()) && (srec.error() != srecord::e_parse_line_not_starting_with_s)) test_note( srec.error_message() );
  }
  // Line length odd
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string("S00F000068656C6C6F21202020200000F3B\n");
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_invalid_line_length));
    if((!srec.good()) && (srec.error() != srecord::e_parse_invalid_line_length)) test_note( srec.error_message() );
  }
  // Line too short
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string("S00F0\n");
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_invalid_line_length));
    if((!srec.good()) && (srec.error() != srecord::e_parse_invalid_line_length)) test_note( srec.error_message() );
  }
  // Line too long
  {
    srecord srec;
    srec.strict_parsing(true);
    string data = string("S0");
    for(auto i=0; i<513; ++i) data += "0";
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_invalid_line_length));
    if((!srec.good()) && (srec.error() != srecord::e_parse_invalid_line_length)) test_note( srec.error_message() );
  }
  // Duplicate start address
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
      "S9030000FC\n"
      "S9030000FC\n"
    );
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_duplicate_start_address));
    if((!srec.good()) && (srec.error() != srecord::e_parse_duplicate_start_address)) test_note( srec.error_message() );
  }
  // Duplicate line count spec
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
      "S5030003F9\n"
      "S5030003F9\n"
    );
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_duplicate_data_count));
    if((!srec.good()) && (srec.error() != srecord::e_parse_duplicate_data_count)) test_note( srec.error_message() );
  }
  // Mixed S1/S2/S3
  {
    srecord srec;
    srec.strict_parsing(true);
    const string data = string(
      "S00F000068656C6C6F212020202000003B\n"
      "S2080010007C0802A6BB\n"
      "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
    );
    test_expect((!srec.parse(data)) && (srec.error() == srecord::e_parse_mixed_data_line_types));
    if((!srec.good()) && (srec.error() != srecord::e_parse_mixed_data_line_types)) test_note( srec.error_message() );
  }
}

void test_multi_file_stream()
{
  // Test implicit stream abort to support usages like "cat *.s19 *.s28 *.s37 | my-srec-merge > merged.srec"
  const string content = string(
    "S00F000068656C6C6F212020202000003B\n"
    "S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026\n"
    "S11F001C4BFFFFE5398000007D83637880010014382100107C0803A64E800020E9\n"
    "S111003A48656C6C6F20776F726C642E0A0040\n"
  );
  stringstream ss(content + "\n" + content + "\n\n"); // two srec files in the same stream
  vector<srecord> records;
  while(ss.good()) records.push_back(srecord(ss));
  test_expect( ss.eof() );
  test_expect( records.size() == 2 );
  if(records.size() == 2) {
    test_expect( records.front().dump() == records.back().dump() );
    test_expect( !records.front().error() );
    test_expect( !records.back().error() );
  }
}

void test()
{
  test_expect_noexcept( test_parse_example_s19() );
  test_expect_noexcept( test_compose() );
  test_expect_noexcept( test_range_get_set() );
  test_expect_noexcept( test_merge() );
  test_expect_noexcept( test_push_block() );
  test_expect_noexcept( test_load_file() );
  test_expect_noexcept( test_strict_parsing() );
  test_expect_noexcept( test_multi_file_stream() );
}
