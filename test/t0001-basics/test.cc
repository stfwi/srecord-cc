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

#define srec_dump() { stringstream sss; srec.dump(sss); test_comment(sss.str()); }
#define range_dump(RNG) { stringstream sss; sss<<"Range(sadr:0x"<<std::hex << long(rng.sadr()) << ", size:" << std::dec << long(rng.size()) << "):\n"; (RNG).dump(sss); test_comment(sss.str()); }

using namespace std;
using sw::srecord;
typedef sw::srecord::block_type block_type;
typedef sw::srecord::block_container_type block_container_type;
typedef sw::srecord::address_type address_type;
typedef sw::srecord::value_type value_type;
typedef sw::srecord::size_type size_type;
typedef sw::srecord::data_type data_type;

srecord srec;

/**
 * Simple sequential sparse block data generator.
 */
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
 * @req: header_str() shall ignore tailing whitespaces
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
    test_expect(srec.blocks().front().eadr() == 0x0046);
    test_expect(srec.blocks().front().size() == 70);
    test_expect(srec.blocks().front().bytes().size()  == 70);
  }
}

/**
 * @req: Composing record into a output stream shall be possible.
 * @req: Composing record to a string shall be possible.
 */
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

/**
 * @req: Retrieving a connected data range from the record shall be possible (filled with value by argument or record default value).
 */
void test_range_get()
{
  test_info("Range get checks...");
  // Blocks: Independent, partially adjacent or sparse.
  {
    srec.clear();
    srec.blocks().push_back(mkblock_seq(0x001e, 0x1e, 18));
    srec.blocks().push_back(mkblock_seq(0x0040, 0x40, 32));
    srec.blocks().push_back(mkblock_seq(0x0060, 0x60, 16));
    srec.blocks().push_back(mkblock_seq(0x0075, 0x75, 30));
    srec_dump();
  }

  const auto check_range_against_srec = [&](const srecord::block_type& rng) -> size_t {
    #if(__cplusplus >= 201700L)
      auto n = size_t(0);
      auto adr = rng.sadr();
      for(const auto b:rng.bytes()) {
        ++n;
        test_expect_silent((srec.get<uint8_t>(adr++, srecord::endianess_type::big_endian).value_or(0)) == (b));
      }
      return n;
    #else
      (void)rng;
      test_note("Skipped range value checks (std>=c++17)");
      return 0;
    #endif
  };

  // Zero-filled at front and back.
  {
    test_note("Front and back filled range:");
    const auto rng = srec.get_range(0x000, 0x100);
    range_dump(rng);
    test_expect_eq(rng.sadr(), 0x000u);
    test_expect_eq(rng.eadr(), 0x100u);
    test_note(check_range_against_srec(rng) << " byte values checked.");
  }
  // Trimmed front, zero filled back.
  {
    test_note("Trimmed front, zero filled back:");
    const auto rng = srec.get_range(0x020, 0x100);
    range_dump(rng);
    test_expect_eq(rng.sadr(), 0x020u);
    test_expect_eq(rng.eadr(), 0x100u);
    test_note(check_range_against_srec(rng) << " byte values checked.");
  }
  // Zero-filled front, trimmed back.
  {
    test_note("Zero-filled front, trimmed back:");
    const auto rng = srec.get_range(0x000, 0x030);
    range_dump(rng);
    test_expect_eq(rng.sadr(), 0x000u);
    test_expect_eq(rng.eadr(), 0x030u);
    test_note(check_range_against_srec(rng) << " byte values checked.");
  }
  // Trimmed front and back.
  {
    test_note("Trimmed front and back:");
    const auto rng = srec.get_range(0x050, 0x060);
    range_dump(rng);
    test_expect_eq(rng.sadr(), 0x050u);
    test_expect_eq(rng.eadr(), 0x060u);
    test_note(check_range_against_srec(rng) << " byte values checked.");
  }
  // Out of range/empty, filled.
  {
    test_note("Out of range/empty, filled:");
    const auto rng = srec.get_range(0x100, 0x110);
    range_dump(rng);
    test_expect_eq(rng.sadr(), 0x100u);
    test_expect_eq(rng.eadr(), 0x110u);
    test_note(check_range_against_srec(rng) << " byte values checked.");
  }
}

/**
 * @req: Retrieving a sparse data range from the record shall be possible (with gaps, no gap filling).
 * @req: Altering record data using start address and byte container shall be possible.
 */
void test_range_get_set()
{
  test_info("Range get-set checks...");
  // Blocks: Independent, 0x20 apart, 0x10 long.
  {
    srec.clear();
    srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0060, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
    srec_dump();
    // Block data pre-check, all blocks have byte sequences 0..15.
    if(!test_expect_cond(srec.blocks().front().size() == 16u)) return;
    for(const auto& blk:srec.blocks()) {
      auto i = size_t(0);
      for(const auto b:blk.bytes()) {
        if(!test_expect_cond_silent(b == i++)) return;
      }
    }
  }
  // Block range return values:
  {
    test_info("Block range return values:");
    test_expect_eq(srec.get_ranges(0x00,0x00).size(), 0u);
    test_expect_eq(srec.get_ranges(0x00,0x20).size(), 0u);    // Note: end = sadr+size+1, this is not in range yet.
    test_expect_eq(srec.get_ranges(0x00,0x21).size(), 1u);    // This is.
    test_expect_eq(srec.get_ranges(0x00,0x30).size(), 1u);
    test_expect_eq(srec.get_ranges(0x00,0x40-1).size(), 1u);
    test_expect_eq(srec.get_ranges(0x00,0x40).size(), 1u);
    test_expect_eq(srec.get_ranges(0x00,0x40+1).size(), 2u);
    test_expect_eq(srec.get_ranges(0x20,0x20+1).size(), 1u);  // First byte of first block in range.
    test_expect_eq(srec.get_ranges(0x20,0x30).size(), 1u);
    test_expect_eq(srec.get_ranges(0x20,0x40-1).size(), 1u);
    test_expect_eq(srec.get_ranges(0x20,0x40).size(), 1u);
    test_expect_eq(srec.get_ranges(0x20,0x40+1).size(), 2u);
    test_expect_eq(srec.get_ranges(0x20,0x60).size(), 2u);
    test_expect_eq(srec.get_ranges(0x20,0x60+1).size(), 3u);
    test_expect_eq(srec.get_ranges(0x20,0x80).size(), 3u);
    test_expect_eq(srec.get_ranges(0x20,0x80+1).size(), 4u);
    test_expect_eq(srec.get_ranges(0x2f,0x80+1).size(), 4u);  // Last byte of first block yet in range ...
    test_expect_eq(srec.get_ranges(0x30,0x80+1).size(), 3u);  // ... that's behind the first block.
    test_expect_eq(srec.get_ranges(0x20,0x100).size(), 4u);
    test_expect_eq(srec.get_ranges(0x20,0x080).size(), 3u);
    test_expect_eq(srec.get_ranges(0x20,0x100).front().sadr(), 0x20u);
    test_expect_eq(srec.get_ranges(0x20,0x100).back().sadr(), 0x80u);
    test_expect_eq(srec.get_ranges(0x20,0x100).back().eadr(), 0x90u);
    test_expect_eq(srec.get_ranges(0x00,0x100).front().sadr(), 0x20u);
    test_expect_eq(srec.get_ranges(0x00,0x200).back().eadr(), 0x90u);
    test_expect_eq(srec.get_ranges(0x80,0x20).size(), 0u);    // sadr > eadr -> no match.
    test_expect_eq(srec.get_ranges(0x21,0x20).size(), 0u);    // sadr > eadr -> no match.
  }
  // Add data at the front, overlapping.
  {
    srec.set_range(0x0008, mkblock_seq(0x0, 0xaa, 29).bytes());
    srec_dump();
    test_expect_eq(srec.blocks().size(), 4u);  // Still 4 blocks, 1st one altered.
    test_expect_eq(srec.sadr(), 0x8u);
    test_expect_eq(srec.eadr(), 0x90u);
    test_expect_eq(srec.blocks().front().sadr(), 0x8u);
    test_expect_eq(srec.blocks().front().eadr(), 0x30u);
    for(size_t i=0; i<29; ++i) test_expect_silent(srec.blocks().front().bytes().at(i) == (0xaa+i));
    if(::sw::utest::test::num_fails() > 0) return;
  }
  // Alter existing range, which will overwrite all existing
  // ranges except the last.
  {
    srec.set_range(mkblock_seq(0x0018, 0x44, 0x60));
    srec_dump();
    test_expect_eq(srec.blocks().size(), 2u);
    test_expect_eq(srec.sadr(), 0x08u); // 0x0008..0x0018 still as before.
    test_expect_eq(srec.eadr(), 0x90u); // 0x0080..0x0090 still as before.
    test_expect_eq(srec.blocks().front().sadr(), 0x08u);
    test_expect_eq(srec.blocks().front().eadr(), 0x78u);
    for(size_t i=0; i<0x10; ++i) test_expect_silent(srec.get_range(0x8u+i, 0x8u+i+1).bytes().front() == 0xaa+i);
    for(size_t i=0; i<0x60; ++i) test_expect_silent(srec.get_range(0x18+i, 0x18+i+1).bytes().front() == 0x44+i);
    #if(__cplusplus >= 201700L)
      for(size_t i=0; i<0x60; ++i) test_expect_silent(srec.get<uint8_t>(0x18+i, srecord::endianess_type::big_endian).value_or(0) == 0x44+i);
      for(size_t i=0; i<0x60; ++i) test_expect_silent(srec.get<uint8_t>(0x18+i, srecord::endianess_type::little_endian).value_or(0) == 0x44+i);
    #endif
    if(::sw::utest::test::num_fails() > 0) return;
  }
  // Alter existing range, this time only one block should remain
  // due to implicit concatenation of adjacent blocks.
  {
    srec.set_range(mkblock_seq(0x0000, 0x00, 0x80));
    srec_dump();
    test_expect_eq(srec.blocks().size(), 1u);
    test_expect_eq(srec.sadr(), 0x00u);
    test_expect_eq(srec.eadr(), 0x90u);
    test_expect_eq(srec.blocks().front().sadr(), 0x00u);
    test_expect_eq(srec.blocks().front().eadr(), 0x90u);
    for(size_t i=0; i<0x80; ++i) test_expect_silent(srec.get_range(0x00u+i, 0x00u+i+1).bytes().front() == 0x00+i);
    for(size_t i=0; i<0x10; ++i) test_expect_silent(srec.get_range(0x80u+i, 0x80u+i+1).bytes().front() == 0x00+i);
  }
}

/**
 * @req: Merging sparse record data to one block with data gap filling shall be possible.
 * @req: Merging sparse record data fill bytes shall be specified as optional parameter, defaulting to the specified record instance default fill value (default:0).
 * @req: Merging sparse record data shall overwrite overlapping data based on ascending address order (t.m. not based on the ordering of the underlying sparse block data).
 */
void test_merge()
{
  const auto all_equal = [](srecord::data_type container, srecord::value_type value) {
    for(const auto c:container) { if(c != value) return false; }
    return true;
  };

  // By argument value gap fill
  {
    test_note("Merge checks, fill argument 0xfe ...");
    srec.clear();
    srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0030, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0050, 0, 16));
    srec_dump();
    srec.merge(0xfe);
    srec_dump();
    test_expect_eq(srec.blocks().size(), 1u);
    test_expect_eq(srec.sadr(), 0x0020u);
    test_expect_eq(srec.eadr(), 0x0050u+16);
    test_expect(all_equal(srec.get_range(0x40, 0x40+16).bytes(), 0xfe));
  }
  // By default value gap fill
  {
    test_note("Merge checks, fill with instance default 0xa5 ...");
    srec.clear();
    srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0030, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0050, 0, 16));
    srec_dump();
    srec.default_value(0xa5);
    srec.merge();
    srec.default_value(0);
    srec_dump();
    test_expect_eq(srec.blocks().size(), 1u);
    test_expect_eq(srec.sadr(), 0x0020u);
    test_expect_eq(srec.eadr(), 0x0050u+16);
    test_expect(all_equal(srec.get_range(0x40, 0x40+16).bytes(), 0xa5));
  }
  // Overwrite order
  {
    test_note("Merge checks, overwrite order ...");
    srec.clear();
    srec.blocks().push_back(mkblock_seq(0x0030, 0x70, 32));
    srec.blocks().push_back(mkblock_seq(0x0020, 0x00, 32));
    srec.blocks().push_back(mkblock_seq(0x0040, 0xa0, 16));
    srec_dump();
    srec.default_value(0xa5);
    srec.merge();
    srec.default_value(0);
    srec_dump();
    test_expect_eq(srec.sadr(), 0x0020u);
    test_expect_eq(srec.eadr(), 0x0040u+16);
    if(!test_expect_cond(srec.blocks().size() == 1)) return;
    // Using block access, srec.get<uint8_t>(adr) as of c++17.
    test_note("Block byte size: " << int(srec.blocks().front().bytes().size()));
    if(!test_expect_cond(srec.blocks().front().bytes().size() == 48)) return;
    test_expect_eq(int(srec.blocks().front().bytes()[0x00]), 0x00);
    test_expect_eq(int(srec.blocks().front().bytes()[0x10]), 0x70);
    test_expect_eq(int(srec.blocks().front().bytes()[0x20]), 0xa0);
    #if(__cplusplus >= 201700L)
      test_expect_eq(int(srec.get<uint8_t>(0x0020, srecord::endianess_type::big_endian).value_or(0xff)), 0x00);
      test_expect_eq(int(srec.get<uint8_t>(0x0030, srecord::endianess_type::big_endian).value_or(0xff)), 0x70);
      test_expect_eq(int(srec.get<uint8_t>(0x0040, srecord::endianess_type::big_endian).value_or(0xff)), 0xa0);
    #endif
  }
}

/**
 * @req: Searching the record for a byte sequence, optionally starting from a specified address, shall be possible.
 * @req: Searching the record for a byte sequence shall returns the start address of a found byte sequence, or the end address of the record if the sequence was not found.
 */
void test_find()
{
  test_note("Find byte sequence checks ...");
  srec.clear();
  srec.blocks().push_back(mkblock_seq(0x0020, 0x00,  8));
  srec.blocks().push_back(mkblock_seq(0x0080, 0xa0, 10));
  srec_dump();
  test_expect_eq( srec.find(data_type{0}), srec.sadr());
  test_expect_eq( srec.find(data_type{0x01, 0x02}), 0x0020u+1 );
  test_expect_eq( srec.find(data_type{0,1,2,3,4,5,6,7}), 0x0020u);
  test_expect_eq( srec.find(data_type{0,1,2,3,4,5,6,7,8}), srec.eadr());
  test_expect_eq( srec.find(data_type{0x01, 0x03}), srec.eadr() );
  test_expect_eq( srec.find(data_type{}), srec.eadr() );
  test_expect_eq( srec.find(data_type{0x01, 0x02}, 0x0020), 0x0020u+1 );
  test_expect_eq( srec.find(data_type{0x01, 0x02}, 0x0021), 0x0020u+1 );
  test_expect_eq( srec.find(data_type{0x01, 0x02}, 0x0022), srec.eadr() );
}

/**
 * @req: Removing ranges (defined by start and end address) from the record data shall be possible.
 */
void test_remove()
{
  test_note("Remove range checks ...");
  {
    srec.clear();
    srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
    srec.blocks().push_back(mkblock_seq(0x0060, 0xcc, 16));
    srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
    srec_dump();
    test_expect_eq(srec.blocks().size(), 4u);
    test_expect_eq(srec.sadr(), 0x0020u);
    test_expect_eq(srec.eadr(), 0x0090u);
  }
  const auto ref_block = srecord(srec).merge().blocks().front();
  test_expect_eq(ref_block.sadr(), 0x0020u);
  test_expect_eq(ref_block.eadr(), 0x0090u);
  test_expect_eq(ref_block.size(), 0x0070u);
  // Remove before begin / after end / empty range.
  {
    srec.remove_range(0x00, 0x20);
    test_expect_eq(srec.sadr(), ref_block.sadr());
    test_expect_eq(srec.eadr(), ref_block.eadr());
    srec.remove_range(0x90, 0x95);
    test_expect_eq(srec.sadr(), ref_block.sadr());
    test_expect_eq(srec.eadr(), ref_block.eadr());
    srec.remove_range(0x20, 0x20);
    test_expect_eq(srec.sadr(), ref_block.sadr());
    test_expect_eq(srec.eadr(), ref_block.eadr());
    srec.remove_range(0x30, 0x10);
    test_expect_eq(srec.sadr(), ref_block.sadr());
    test_expect_eq(srec.eadr(), ref_block.eadr());
  }
  // Remove from the middle of a block.
  {
    srec.remove_range(0x24, 0x28);
    srec_dump();
    test_expect_eq(srec.blocks().size(), 5u);
    test_expect_eq(srec.sadr(), 0x0020u);
    test_expect_eq(srec.eadr(), 0x0090u);
    test_expect_eq(srec.blocks().front().sadr(), 0x0020u);
    test_expect_eq(srec.blocks().front().eadr(), 0x0024u);
    test_expect_eq(srec.blocks().at(1).sadr(), 0x0028u);
    test_expect_eq(srec.blocks().at(1).eadr(), 0x0030u);
    test_expect_eq(srec.blocks().at(2).sadr(), 0x0040u);
    test_expect_eq(srec.blocks().at(2).eadr(), 0x0050u);
  }
  // Remove exact block range at the end.
  {
    srec.remove_range(0x80, 0xf0);
    srec_dump();
    test_expect_eq(srec.blocks().size(), 4u);
    test_expect_eq(srec.sadr(), 0x0020u);
    test_expect_eq(srec.eadr(), 0x0070u);
  }
  // Remove at the front.
  {
    srec.remove_range(0x20, 0x40);
    srec_dump();
    test_expect_eq(srec.blocks().size(), 2u);
    test_expect_eq(srec.sadr(), 0x0040u);
    test_expect_eq(srec.eadr(), 0x0070u);
  }
}

/**
 * @req: Direct read/write access to the sparse block data shall be possible.
 */
void test_block_data_access()
{
  test_info("Direct block access checks ...");
  srec.clear();
  srec.blocks().push_back(mkblock_seq(0x0020, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0040, 0, 16));
  srec.blocks().push_back(mkblock_seq(0x0060, 0xcc, 16));
  srec.blocks().push_back(mkblock_seq(0x0080, 0, 16));
  srec_dump();
  if(!test_expect_cond(srec.blocks().size() == 4u)) return;
  test_expect_eq(srec.blocks()[0].sadr(), 0x0020u);
  test_expect_eq(srec.blocks().at(1).sadr(), 0x0040u);
  test_expect_eq(srec.blocks()[2].sadr(), 0x0060u);
  test_expect_eq(srec.blocks().at(3).sadr(), 0x0080u);
}

/**
 * @req: parse() with strict parsing shall fail if no (S0) header is specified.
 * @req: parse() with strict parsing shall fail if data ranges are overlapping.
 * @req: parse() with strict parsing shall fail if the specified line count (S5) does not match the read line count.
 * @req: parse() with strict parsing shall fail if invalid characters are present in the S-record.
 * @req: parse() with strict parsing shall fail if a line checksum is invalid.
 * @req: parse() with strict parsing shall fail if a record type definition ("S?" at the line start) is invalid or unknown (no comments etc allowed).
 * @req: parse() with strict parsing shall fail if a line does not start with a record tag ("S").
 * @req: parse() with strict parsing shall fail if a line length of the S-record is invalid.
 * @req: parse() with strict parsing shall fail if a line is longer than 514 characters (aka, exceeds the value range of the S-record line format).
 * @req: parse() with strict parsing shall fail if a start address duplication is detected (multiple S9).
 * @req: parse() with strict parsing shall fail if a data size duplication is detected (multiple S5).
 * @req: parse() with strict parsing shall fail if a multiple address size formats are used (aka only one of S1/S2/S3 allowed).
 */
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

/**
 * @req: Parsing strings or streams shall be stopped successfully at blank lines (to enable parsing multiple records in one stream).
 */
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

void test(const vector<string>& args)
{
  (void)args;
  test_expect_noexcept( test_parse_example_s19() );
  test_expect_noexcept( test_compose() );
  test_expect_noexcept( test_range_get() );
  test_expect_noexcept( test_range_get_set() );
  test_expect_noexcept( test_merge() );
  test_expect_noexcept( test_remove() );
  test_expect_noexcept( test_find() );
  test_expect_noexcept( test_block_data_access() );
  test_expect_noexcept( test_strict_parsing() );
  test_expect_noexcept( test_multi_file_stream() );
}
