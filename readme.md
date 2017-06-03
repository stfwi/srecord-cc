# C++ S-Record parser/composer/modification class (`srecord-cc`)

The class `sw::srecord` allows to read, write, dump, and alter `S19`/`S28`/`S37`
formatted memory images. Shrinking the S-record file format down to the baseline,
there are lines with binary data and the memory addresses where these data are
located (the rest is header and meta data for integrity check). The address ranges
of the data lines are mostly connected to another, but sometimes there are gaps
in between. This results effectively in one or more *blocks* of data ranges. This
is how the class represents a parsed file, stream or string:

- There is a main object of type `srecord`, which
  - allows to get and set settings of the parser/composer,
  - allows to get and set meta data of the s-record,
  - provides access methods for accessibility and faster usage,
  - and has a container of blocks, where each block
    - has a start address
    - has a `std::vector` of the data bytes
    - provides access methods

Because the memory range of the target devices is limited, you load from a file or
stream into the RAM, work there, and save it or send the data directly to the target.

Features of the class are:

  - parse from file or `std::istream`
  - compose to `std::ostream`
  - strict or non-strict validation
  - block direct access (STL containers)
  - block structure independent memory range getters/setters
  - block merging with gap filling
  - default memory reset values (depends on target ROM type)

For usage please take a look at the [example](test/src/example.cc) and the [test](test/src/test.cc),
or as a brief overview the following code:

```c++
#include <sw/srecord.hh>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

int main(int argc, const char** argv)
{
  sw::srecord srec;

  #ifdef TESTFILE
  // Load and parse from file (convenience wrapper for `parse()` for
  // file streams).
  sw::srecord::load(TESTFILE, srec);
  #else
  // Parse from stream
  srec.parse(cin);
  #endif

  // Errors are indicated using boolean returns, `error()`
  // and `error_message()` - not exceptions. You can throw
  // if you like.
  if(srec.error()) {
    cout << "Failed to load/parse: "
         << srec.error_message() << " (at line "
         << srec.parser_line() << ")" << endl;
  } else {

    // Human readable dump
    srec.dump(cout);

    // List independent memory ranges ("blocks")
    for(auto block:srec.blocks()) {
      cout << "Memory range from address 0x" << hex << block.sadr()
           << " to 0x" << hex << block.eadr() << endl;
    }

    // Get a range from-to; that is also a block according to the
    // data type, and it is a copy, not a reference. If the range
    // exceeds the existing blocks, then the unknown regions are
    // filled with a default value that you can set (e.g. 0xff or
    // 0x00 depending on the flash type of your target controller).
    auto range1 = srec.get_range(0x100, 0x200);

    // The byte data in a range is simply a std::vector of bytes,
    // you can change, shrink, expand it as you need it.
    auto& data1 = range1.bytes();

    // E.g. copy the block we got above to another address.
    srec.set_range(0x300, range1.bytes());

    std::vector<uint8_t> mydata{0,1,2,3,4,5};
    srec.set_range(0x200, mydata);

    // Direct data access, e.g. the bytes of the fist data
    // range.
    unsigned sum = 0;
    for(auto byte:srec.blocks().front().bytes()) sum += byte;

    // Make all independent block to one big block. The gaps
    // are filled with the default value.
    // Merging is also good if you simply like to `push_bach()`
    // blocks and then finally make one memory image, where the
    // later blocks overwrite the earlier blocks.
    srec.default_value(0xff);
    srec.merge();

    // Compose is used to save, e.g. into a std::ofstream
    srec.compose(cout);
  }
  return 0;
}
```
