/**
 * @file example.cc
 * @package de.atwillys.cc.swl
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 * @ccflags
 * @ldflags
 * @platform linux, bsd, windows
 * @standard >= c++11
 *
 * -----------------------------------------------------------------------------
 *
 * Motorolla SRECORD file parsing / modification / analysis.
 *
 * -----------------------------------------------------------------------------
 * +++ BSD license header +++
 * Copyright (c) 2008-2017, Stefan Wilhelm <cerberos@atwillys.de>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met: (1) Redistributions
 * of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer. (2) Redistributions in binary form must reproduce
 * the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * -----------------------------------------------------------------------------
 */
#define WITH_SRECORD_DEBUG
#include <sw/srecord.hh>
#include <iostream>
#include <fstream>
#include <string>

#define print_error(X) { std::cerr << "[error] " << X << std::endl; std::cerr.flush(); }
#define print_verbose(X) { if(verbosity > 0)  { std::cerr << "[verb1] " << X << std::endl; std::cerr.flush(); } }
#define print_verbose1(X) { if(verbosity > 1) { std::cerr << "[verb2] " << X << std::endl; std::cerr.flush(); } }

using namespace std;
using sw::srecord;
typedef sw::srecord::block_type block_type;
typedef sw::srecord::block_container_type block_container_type;
typedef sw::srecord::address_type address_type;
typedef sw::srecord::value_type value_type;
typedef sw::srecord::size_type size_type;
typedef sw::srecord::data_type data_type;

srecord srec;
vector<string> program_args;

#if(0)
Contents of file example.s19:

S00F000068656C6C6F212020202000003B
S11F00007C0802A6900100049421FFF07C6C1B787C8C23783C6000003863000026
S11F001C4BFFFFE5398000007D83637880010014382100107C0803A64E800020E9
S111003848656C6C6F20776F726C642E0A0042
S5030003F9
S9030000FC
#endif

using namespace std;
using sw::srecord;

/**
 * Example
 */
void example(string example_file)
{
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Parsing and getting record information.
  //////////////////////////////////////////////////////////////////////////////////////////////////

  // Parse from file:
  cout << "1. Loading a file ..." << endl;
  srecord srec;
  if(!srecord::load(example_file, srec)) {
    cout << "Failed to load S-record file '" << example_file << "': '"
         << srec.error_message() << "'. " << endl
         << "The problem is at line " << srec.parser_line() << endl
         ;
    return;
  }
  cout << "Loaded file. Dump is:" << endl;
  srec.dump(cout);
  // -->
  //  srec {
  //   data type: S1
  //   blocks: [
  //      <00000000> 7C08 02A6 9001 0004 9421 FFF0 7C6C 1B78
  //      <00000010> 7C8C 2378 3C60 0000 3863 0000 4BFF FFE5
  //      <00000020> 3980 0000 7D83 6378 8001 0014 3821 0010
  //      <00000030> 7C08 03A6 4E80 0020 4865 6C6C 6F20 776F
  //      <00000040> 726C 642E 0A00
  //   ]
  //  }

  cout << endl << "The S0 header as string is: '" << srec.header_str() << "'" << endl;
  // ---> "The S0 header as string is: 'hello!'"

  // Returns the type of the record, 1 for S1, 2 for S2, 3 for S3.
  cout << "The address type is: '" << (int)srec.type() << "'" << endl;
  // ---> "The address type is: '1'"

  // There is also an enumeration for setting and checking that:
  if(srec.type() == srecord::type_s1_16bit) {
    cout <<  "(The address type is 16 bit addresses)" << endl;
  } else {
    cout <<  "(The address type is not 16 bit addresses)" << endl;
  }

  // Returns the start address of the whole file. This does not include the S9/S8/S7
  // start address definition.
  cout << "The first address of the whole file is: 0x" << hex << srec.sadr() << endl;
  // ---> "The first address of the whole file is: 0x0"

  // Returns the end address of the last block. That is not the last valid byte, but one
  // beyond - means the usage is like iterators begin() and end() usage.
  cout << "The last address of the whole file is: 0x" << hex << srec.eadr() << endl;
  // ---> "The last address of the whole file is: 0x46"

  // As the information in the address lines do not need represent one big data block bigger and
  // smaller (unconnected) address ranges, the srecord instance deals with blocks. That is simply
  // a vector (or other STL container) of srecord::block_type:
  cout << endl << "The file is divided in " << srec.blocks().size() << " unconnected blocks" << endl;
  // ---> "The file is divided in 1 unconnected blocks"

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // File structure: Blocks.
  //////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // The file data is saved as an STL container (default: std::vector) if srecord::block_type
  // instances. These object basically have a start address and an container of bytes saved.
  // The remaining methods are convenience wrappers. When you use the template
  // sw::detail::basic_srecord<ValueType, ContainerType> you can set the byte type and the
  // container yourself, but the ValueType should be 8 bit and the container must be a
  // dynamic size random access container, such as deque<> or vector<>, (but not e.g. array<>).
  // However, normally the specialisation class sw::srecord, which is simply
  //   `namespace sw { typedef detail::basic_srecord<unsigned char> srecord; }`
  // should be already exactly what you want.
  //
  // Lets look into the first block of the file ..
  if(!srec.blocks().empty()) {
    srecord::block_type& first = srec.blocks().front();
    cout << "The first block start address is 0x" << hex << first.sadr() << "." << endl;
    // --> The first block start address is 0x0.

    cout << "The first block end address is 0x" << hex << first.eadr() << "." << endl;
    // --> The first block end address is 0x46.

    // As known from containers, the block_type supports for ease of programming swap(),
    // clear(), size(), and empty().
    cout << "The first block is " << (first.empty() ? "" : "not ") << "empty." << endl;

    // Returns the size of the byte vector.
    cout << "The first has " << dec << first.size() << " data bytes." << endl;
    // --> The first has 70 data bytes.

    // So, byte operations are simple std::vector access operations.
    first.bytes().resize(16);
    cout << "After resizing only " << dec << first.size() << " data bytes." << endl;

    // So, byte operations are simple std::vector access operations.
    cout << "These bytes are (hex): ";
    // or for(auto e: first.bytes()) { ... }
    for(size_t i=0; i < first.bytes().size(); ++i) {
      cout << hex << (int) first.bytes()[i] << " ";
    }
    cout << endl;
    // --> These bytes are (hex): 7c 8 2 a6 90 1 0 4 94 21 ff f0 7c 6c 1b

    // And we cah change the start address of this block:
    first.sadr(0x1000);

    cout << "The first block start address is now 0x" << hex << first.sadr() << "." << endl;
    // --> The first block start address is now 0x1000.

    cout << "The first block end address is now 0x" << hex << first.eadr() << "." << endl;
    // --> The first block end address is now 0x101a.

    // The address range of `srec` changes accordingly:
    cout << "The first address of the whole file is now: 0x" << hex << srec.sadr() << endl;
    cout << "The last address of the whole file is now: 0x" << hex << srec.eadr() << endl;
    // --> The first address of the whole file is now: 0x1000
    // --> The last address of the whole file is now: 0x101a

    // Additionally you can fetch a copy of block bytes by defining start and end address,
    // both are absolute, so if the block is not in range between these addresses, an empty
    // result will be returned. The result itself is again a block_type.
    {
      // That will be empty, as the block starts now at 0x1000.
      srecord::block_type block = first.get_range(0x100, 0x200);
      cout << "Fetched range form first block (0x100 --> 0x200) is: ";
      block.dump(cout);
      cout << endl;
      // ---> "Fetched range form first block (0x100 --> 0x200) is: (empty block)"
    }
    {
      // That will be the whole block
      srecord::block_type block = first.get_range(0x0000, 0x2000);
      cout << "Fetched range form first block (0x0 --> 0x2000) is: ";
      block.dump(cout);
      cout << endl;
      // --> "Fetched range form first block (0x0 --> 0x2000) is: <00001000> 7C08 02A6 9001 0004 9421 FFF0 7C6C 1B78"
    }
    {
      // That will be the whole block
      srecord::block_type block = first.get_range(0x1002, 0x1005);
      cout << "Fetched range form first block (0x1002 --> 0x1005) is: ";
      block.dump(cout);
      cout << endl;
      // --> "Fetched range form first block (0x1002 --> 0x1005) is: <00001000>      02A6 90"
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Fetching, removing and modifying address ranges
  //////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // The easiest way to set and get data from the srecord is to use address range based methods.
  // They get or return block copies, so that the integrity of the record ensured - that would be
  // more difficult with references.
  //

  // Adding a block, option 1: make one and add it:
  {
    srecord::block_type new_block;
    new_block.sadr(0x2100);
    new_block.bytes({0,1,2,3,4,5,6,7,8,9});
    srec.set_range(new_block);
  }
  // Adding a block, option 2: directly from start address and data:
  {
    srec.set_range(0x3000, {7,6,5,4,3,2,1});
  }

  cout << "srec after adding two blocks:" << endl;
  srec.dump(cout);
  cout << endl;
  // srec {
  // data type: S1
  // blocks: [
  //    <00001000> 7C08 02A6 9001 0004 9421 FFF0 7C6C 1B78
  //
  //    <00002100> 0001 0203 0405 0607 0809
  //
  //    <00003000> 0706 0504 0302 01
  //
  //  ]
  // }

  //
  // Modifying a block: Same as adding a block
  //
  {
    srec.set_range(0x3000, {0xff,0xfe,0xfd});
  }

  cout << "srec after modifying:" << endl;
  srec.dump(cout);
  cout << endl;
  // srec {
  //  data type: S1
  //  blocks: [
  //     <00001000> 7C08 02A6 9001 0004 9421 FFF0 7C6C 1B78
  //
  //     <00002100> 0001 0203 0405 0607 0809
  //
  //     <00003000> FFFE FD04 0302 01
  //
  //  ]
  // }

  //
  // Removing data:
  //
  srec.remove_range(0x2000, 0x3004);

  cout << "srec after removing data:" << endl;
  srec.dump(cout);
  cout << endl;
  //  srec {
  //   data type: S1
  //   blocks: [
  //      <00001000> 7C08 02A6 9001 0004 9421 FFF0 7C6C 1B78
  //
  //      <00003000>           0302 01
  //
  //   ]
  //  }

  //
  // Merging more blocks to one block, where the gaps are filled with
  // either a default value or a value specified as method argument:
  // (But first we move the second block up a bit as the dump() would be
  // quite large otherwise.). Now it depends on your application what
  // value you want to have there: For RAM these gaps maybe should be 0,
  // for FLASH it can be 0xff (erased state).
  srec.blocks().back().sadr(0x1035);
  srec.merge(0xff);
  cout << "srec after moving the second to 0x1035 and merging the two blocks:" << endl;
  srec.dump(cout);
  cout << endl;
  //  srec {
  //   data type: S1
  //   blocks: [
  //      <00001000> 7C08 02A6 9001 0004 9421 FFF0 7C6C 1B78
  //      <00001010> FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF
  //      <00001020> FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF
  //      <00001030> FFFF FFFF FF03 0201
  //
  //   ]
  //  }

  // If we remove a range, blocks might be split:
  srec.remove_range(0x1014, 0x1032);
  cout << "srec after removing range 0x1014 --> 0x1032:" << endl;
  srec.dump(cout);
  cout << endl;
  //  srec {
  //   data type: S1
  //   blocks: [
  //      <00001000> 7C08 02A6 9001 0004 9421 FFF0 7C6C 1B78
  //      <00001010> FFFF FFFF
  //
  //      <00001030>      FFFF FF03 0201
  //
  //   ]
  //  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Finding/searching data
  //////////////////////////////////////////////////////////////////////////////////////////////////

  // The method `find()` allows to search for a byte sequence in the record:
  // If the method did not find the sequence, it returns srec.eadr(), which is similar
  // to the STL container.end() convention.

  // If you have already a vector ...
  {
    std::vector<unsigned char> bytes{ 0xff, 0xff, 0x03 };
    unsigned long address = srec.find(bytes);
    if(address == srec.eadr()) {
      cout << "{ 0xff, 0xff, 0x03 } not found." << endl;
    } else {
      cout << "{ 0xff, 0xff, 0x03 } found at address 0x" << hex << address << endl;
    }
    // --> { 0xff, 0xff, 0x03 } found at address 0x1033
  }
  {
    // Or simply by rvalue. Note, always the first match is returned ....
    srecord::address_type first_match = srec.find({0xff, 0xff, 0xff});
    cout << "{ 0xff, 0xff, 0xff } first found at 0x" << hex << first_match << endl;
    // ---> { 0xff, 0xff, 0xff } first found at 0x1010

    // .... to search more matches specify a search start address:
    unsigned search_start_address = first_match + 3;
    auto next_match = srec.find({0xff, 0xff, 0xff}, search_start_address);
    cout << "{ 0xff, 0xff, 0xff } next_match found at 0x" << hex << next_match << endl;
    // ---> { 0xff, 0xff, 0xff } next_match found at 0x1032
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Composing
  //////////////////////////////////////////////////////////////////////////////////////////////////

  // Option 1: using compose()
  {
    std::stringstream ss;
    unsigned line_length = 16;
    if(!srec.compose(ss, line_length)) {
      cout << "Failed to compose: " << srec.error_message() << endl;
    }
  }

  // Option 2: ostream shifting. The line length is the set to a sensible default.
  {
    cout << endl << "Composed srec = " << endl;
    cout << srec << endl;
    if(!srec.good()) {
      cout << "Error composing: " << srec.error_message() << endl;
    }
    //  S00F000068656C6C6F212020202000003B
    //  S11510007C0802A6900100049421FFF07C6C1B78FFFFFC
    //  S1051012FFFFDA
    //  S1091032FFFFFF030201B1
    //  S5030003F9
    //  S9030000FC

    // We can also change the type to 32 bit addresses - just for fun here, no need to do that.
    srec.type(srecord::type_s3_32bit);
    cout << endl << "Composed srec with 32 bit addresses = " << endl;
    cout << srec << endl;
    //  S0110000000068656C6C6F2120202020000039
    //  S319000010007C0802A6900100049421FFF07C6C1B78FFFFFFFFFA
    //  S30B00001032FFFFFF030201AF
    //  S5030002FA
    //  S70500000000FA

    // and 24 bit addresses for having it complete:
    srec.type(srecord::type_s2_24bit);
    cout << endl << "Composed srec with 24 bit addresses = " << endl;
    cout << srec << endl;
    //  S01000000068656C6C6F212020202000003A
    //  S2180010007C0802A6900100049421FFF07C6C1B78FFFFFFFFFB
    //  S20A001032FFFFFF030201B0
    //  S5030002FA
    //  S804000000FB
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Alternative parsing how the class parses
  //////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // - The parser generally ignored whitespaces.
  // - The parser is generally case insensitive, so "S0" and "s0" are identical.
  // - It expects the stream or string to start with S0.
  // - It continues parsing until it finds the S7/S8/S9 end of block marker.
  // - Streams are left untouched after that.
  // - If any parse error occurs, the `error()` (an enum), the `error_message()` (string)
  //   and the `srec.parser_line()` help to determine where which problem was.

  // Option 1: parse from string:
  {
    srec.clear();
    if(!srec.parse(string(
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
    ))) {
      cerr << "Error " << dec << (long)srec.error() << " (" << srec.error_message()
           << ") at line " << srec.parser_line() << endl;
    } else {
      cout << endl << "String parsed record:" << endl;
      srec.dump(cout);
    }
  }

  // Option 2: parse stream:
  {
    std::stringstream ss(
      "S0110000000068656C6C6F2120202020000039\n"
      "S319000010007C0802A6900100049421FFF07C6C1B78FFFFFFFFFA\n"
      "S30B00001032FFFFFF030201AF\n"
      "S5030002FA\n"
      "S70500000000FA\n"
    );

    srec.parse(ss);

    // Possibility to check for errors: either check `srec.error() == srecord::e_ok` or simply
    // ask if it still feels good - like known from iostreams:
    // Let's also go through the error enumeration - which is normally not really interesting,
    // either the file or stream is correct or not. If, however, you want not just to show the
    // error message, which prints already the details what's wrong, but want to fix stuff
    // automatically etc, this can be interesting:
    if(!srec.good()) {
      switch(srec.error()) {
        //
        // Parser
        //
        case srecord::e_parse_chcksum_incorrect:
          // Each line has a checksum, not a good one but it's a checksum and the parser
          // complains if it does not match.
        case srecord::e_parse_duplicate_data_count:
          // More than one S5 line found.
        case srecord::e_parse_duplicate_start_address:
          // More than one S7/S8/S9 found.
        case srecord::e_parse_invalid_line_length:
          // That is a string pre-check error. A line cannot be correct if it has more
          // than 512 character, less than 10 characters or has no even number of characters,
          // as all values are bytes plus the beginning "S<number>".
        case srecord::e_parse_invalid_record_type:
          // Raised when the parser sees an "S4", which is not defined.
        case srecord::e_parse_length_mismatch:
          // Each line contains its length specification. This error is set when this length
          // does not match the real byte count of the line.
        case srecord::e_parse_line_count_mismatch:
          // S5 (optional) specifies how many data lines (S1/S2/S3) are in the block. If this
          // count does not match, you'll get this error.
        case srecord::e_parse_missing_data_lines:
          // This error is set if there are no data lines at all (S1/S2/S3).
        case srecord::e_parse_missing_s0:
          // Raised if there is no S0 block header. In the S-Record spec this is mandatory.
        case srecord::e_parse_s0_address_nonzero:
          // According to S-Record spec the address of the S0 header line has to be 0x0000.
        case srecord::e_parse_startaddress_vs_data_type_mismatch:
          // This error is set if e.g. the data lines are S1, but the termination line not S9,
          // but S8 or S7. Means, the termination line type must match the data line type.
        case srecord::e_parse_unacceptable_character:
          // Not a hex character and no starting "S"

        //
        // Validator
        //
        case srecord::e_validate_overlapping_blocks:
          // Set when the file contains somehow data that shall be written to the same
          // addresses. E.g. at address 0x0000 32 bytes and then at address 0x0010 e.g. 16 bytes.
          // That would mean that the range 0x0010 to 0x0020 is overlapping and is not ok.
        case srecord::e_validate_record_type_to_small:
          // Happens when the file says "S1", but an address to write is actually bigger than
          // 16 bit. Then S2 (24bit addresses) or S3 (32bit addresses) must be used.
        case srecord::e_validate_blocks_unordered:
          // Should not happen after parsing, as parse() sorts the blocks by start address.
          // However, before composing() or when you call srec.validate() you might get this
          // error if you made changes.
        case srecord::e_validate_no_binary_data:
          // There there are no data lines. Also more a problem when modifying.
        case srecord::e_ok:
          // That does not happen here because of the surrounding "if(!srec.good()) { ...",
          // and srec.good() is a wrapper for `srec.error() == e_ok`.

        default:
          cout << "Error parsing stream ..." << srec.error_message()
               << "@line " << srec.parser_line()
               << endl; // handling as above
      }
    } else {
      cout << endl << "Parsed srecord is: " << endl;
      srec.dump(cout);
      cout << endl;
    }
  }
}

/**
 * Main
 *
 * @param int argc
 * @param char *argv[]
 * @return int
 */
int main(int argc, char *argv[])
{
  if(argc < 2 || !argv[1] || !argv[1][0]) {
    cerr << "No file specified (first argument)." << endl;
  } else {
    try {
      example(string(argv[1]));
    } catch(const std::exception& e) {
      cerr << "Error: " << e.what() << endl;
      return 1;
    } catch(...) {
      cerr << "Error: Unspecified exception." << endl;
      return 1;
    }
    return 0;
  }
  return 1;
}
