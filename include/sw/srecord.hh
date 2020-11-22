/**
 * @file srecord.hh
 * @package de.atwillys.cc.swl
 * @license BSD (simplified)
 * @author Stefan Wilhelm (stfwi)
 * @ccflags
 * @ldflags
 * @platform linux, bsd, windows
 * @standard >= c++11
 * @version 1.1
 *
 * -----------------------------------------------------------------------------
 *
 * Motorolla SRECORD file parsing / modification / composition.
 *
 * -----------------------------------------------------------------------------
 * +++ BSD license header +++
 * Copyright (c) 2008-2020, Stefan Wilhelm <cerberos@atwillys.de>
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
#ifndef SW_SRECORD_HH
#define	SW_SRECORD_HH

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <deque>
#include <type_traits>

#ifdef WITH_SRECORD_DEBUG
  #undef SRECORD_DEBUG
  #define SRECORD_DEBUG(X) { std::cerr << X << std::endl; std::cerr.flush(); /* normally endl flushes, too */ }
#else
  #define SRECORD_DEBUG(X)
#endif

namespace sw { namespace detail {

template <typename ValueType, typename RandomAccessValueContainerType=std::vector<ValueType> >
class basic_srecord
{
public:

  /**
   * Binary value of each data word. Should be unsigned char
   * or char / uint8_t/int8_t.
   */
  using value_type = ValueType;

  /**
   * RandomAccessContainer of value_types, e.g.
   * vector or deque. Must support random access
   * iteration and index element access.
   */
  using data_type = RandomAccessValueContainerType;

  /**
   * Type for sizes.
   */
  using size_type = size_t;

  /**
   * The type for addresses.
   */
  using address_type = unsigned long long;

  /**
   * The type for storing the "type" of a record,
   * that is 1 for S1 (16bit address), 2 for S2 (24bit)
   * and 3 for S3 (32bit).
   */
  using record_type_type = enum {
    type_undefined = 0,
    type_s1_16bit  = 1,
    type_s2_24bit  = 2,
    type_s3_32bit  = 3,
  };

  /**
   * Error type.
   */
  typedef enum {
    e_ok = 0,
    e_parse_unacceptable_character,
    e_parse_line_not_starting_with_s,
    e_parse_invalid_line_length,
    e_parse_invalid_record_type,
    e_parse_chcksum_incorrect,
    e_parse_length_mismatch,
    e_parse_missing_s0,
    e_parse_s0_address_nonzero,
    e_parse_duplicate_data_count,
    e_parse_line_count_mismatch,
    e_parse_duplicate_start_address,
    e_parse_startaddress_vs_data_type_mismatch,
    e_parse_missing_data_lines,
    e_parse_mixed_data_line_types,
    e_compose_max_number_of_data_lines_exceeded,
    e_validate_record_type_to_small,
    e_validate_record_range_exceeded,
    e_validate_no_binary_data,
    e_validate_blocks_unordered,
    e_validate_overlapping_blocks,
    e_load_open_failed,
  } error_type;

  /**
   * Type for each connected data block. Blocks have
   * a start address and binary data with a defined
   * size. (note: this class is subclassed as it depends
   * on template parameters of basic_srecord.
   */
  class block_type
  {
  public:

    /**
     * c'tor
     */
    explicit block_type() : address_(0), bytes_()
    { }

    /**
     * c'tor
     */
    explicit block_type(address_type adr) : address_(adr), bytes_()
    { }

    /**
     * c'tor
     */
    explicit block_type(address_type adr, const data_type& data) : address_(adr), bytes_(data)
    { }

    /**
     * c'tor
     */
    explicit block_type(address_type adr, data_type&& data) : address_(adr), bytes_(std::move(data))
    { }

  public:

    /**
     * Returns the start address of this block.
     * @return address_type
     */
    inline address_type sadr() const
    { return address_; }

    /**
     * Sets the start address of this block.
     * @param address_type adr
     */
    inline void sadr(address_type adr)
    { address_ = adr; }

    /**
     * Returns the end of the block. That is NOT
     * the last address in the block, but the
     * first behind. (start address + size).
     *
     * @return address_type
     */
    inline address_type eadr() const
    { return address_ + (address_type)bytes_.size(); }

    /**
     * Returns the size of the block in bytes.
     * @return size_type
     */
    inline size_type size() const
    { return bytes_.size(); }

    /**
     * Returns true if the block has no data
     * bytes.
     * @return bool
     */
    inline bool empty() const
    { return bytes_.empty(); }

    /**
     * Returns a const reference to the block byte data.
     * @return const data_type&
     */
    inline const data_type& bytes() const
    { return bytes_; }

    /**
     * Returns a r/a reference to the block byte data.
     * If this block changes, the size changes implicitly,
     * too.
     * @return data_type&
     */
    inline data_type& bytes()
    { return bytes_; }

    /**
     * Sets new data. Implicitly changes the size().
     * @param const data_type& dat
     */
    inline void bytes(const data_type& dat)
    { bytes_ = dat; }

    /**
     * Sets new data (rvalue). Implicitly changes the size().
     * @param data_type&& dat
     */
    inline void bytes(data_type&& dat)
    { bytes_ = dat; }

  public:

    /**
     * Returns true if address and data are identical.
     * @param const block_type& blk
     * @return bool
     */
    inline bool operator==(const block_type& blk) const
    {
      if(blk.sadr() != sadr() || blk.size() != size()) return false;
      if(bytes().empty()) return true;
      typename data_type::const_iterator it1 = bytes().begin();
      typename data_type::const_iterator it2 = blk.bytes().begin();
      while(it1 != bytes().end()) if(*it1++ != *it2++) return false;
      return true;
    }

    /**
     * Returns true if either address or data are not identical.
     * @see bool operator==(const block_type& blk) const
     * @param const block_type& blk
     * @return bool
     */
    inline bool operator!=(const block_type& blk) const
    { return !(operator==(blk)); }

    /**
     * Address and content memory swap.
     */
    inline void swap(block_type& blk)
    {
      address_type a = blk.sadr();
      blk.sadr(sadr());
      sadr(a);
      bytes_.swap(blk.bytes_);
    }

    /**
     * Clears the instance, releases memory directly.
     * @return voir
     */
    inline void clear()
    { data_type().swap(bytes_); }

  public:

    /**
     * Returns a range of bytes/value_types in this block, defined
     * using the start address and the end address, as a new block.
     * The block data is a copy, not referenced. If the range exceeds
     * the address limits of this block, then the returned block
     * will encompass the matching part of the address range. T.m.
     * the returned start address end address of the block may differ
     * from the requested range from the method arguments.
     * --> Always check the returned block.
     *
     * Note: The end address is the start address + size, means one
     *       position behind the address range if the instance (as
     *       known from iterators, e.g. vector.end()).
     *
     * @param address_type start_address
     * @param address_type end_address
     * @return block_type
     */
    inline block_type get_range(address_type start_address, address_type end_address) const
    {
      block_type ret;
      if(start_address >= end_address) return ret;
      if(start_address < sadr()) start_address = sadr();
      ret.sadr(start_address);
      if(end_address > eadr()) end_address = eadr();
      if(start_address >= end_address) return ret;
      start_address -= sadr();
      end_address -= sadr();
      ret.bytes().reserve(end_address);
      while(start_address < end_address) ret.bytes().push_back(bytes_[start_address++]);
      return ret;
    }

    /**
     * Returns true if the block has data in the specified range, means
     * if it is at least partially in the specified range.
     * @param address_type start_address
     * @param address_type end_address
     */
    inline bool in_range(address_type start_address, address_type end_address)
    { return (end_address >= start_address) && (!(start_address >= eadr() || end_address <= sadr())); }

  public:

    /**
     * Bock dump stream out.
     * @param std::ostream&
     * @param unsigned align
     * @return void
     */
    void dump(std::ostream& os, unsigned align=16) const
    {
      align &= (~0x0001);
      if(align < 4) align = 4;
      address_type adr = sadr();
      if(!bytes_.size()) {
        os << "(empty block)";
        return;
      }
      address_type prefix = adr % align;
      if(!prefix) {
        os << "<" << numtohex(adr, 4) << "> ";
      } else {
        address_type eadr = adr;
        adr -= prefix;
        prefix = adr;
        os << "<" << numtohex(adr, 4) << "> ";
        while(adr < eadr) {
          os << "  ";
          if(!(adr & 0x01) && (adr != prefix)) os << ' ';
          ++adr;
        }
        if(!(adr & 0x01) && (adr != prefix)) os << ' ';
      }
      for(auto b: bytes_) {
        os << numtohex(b, 1);
        ++adr;
        if(!(adr & 0x01)) os << ' ';
        if(!(adr % align)) {
          if(adr != eadr()) {
            os << std::endl << "<" << numtohex(adr, 4) << "> ";
          }
        }
      }
      os << std::endl;
    }

  private:

    address_type address_;    ///< The start address.
    data_type bytes_;         ///< The buffer.
  };

  /**
   * Collection of blocks in this record
   */
  using block_container_type = std::vector<block_type>;

private:

  /**
   * Type for each connected data block.
   */
  struct line_type
  {
    explicit line_type() : type(type_undefined), address(0), bytes() {}
    void clear() { type=0; address=0; bytes.clear(); }
    record_type_type type;
    address_type address;
    data_type bytes;
  };

  /**
   * Type for all lines.
   */
  using line_container_type = std::deque<line_type>;

public:

  /**
   * c'tor (default)
   */
  explicit basic_srecord() : error_(e_ok), type_(type_undefined), start_address_(0),
          header_(), blocks_(), parser_line_(0), error_address_(0), default_value_(0x00),
          strict_parsing_(false)
  { }

  /**
   * c'tor, initialized via stream parsing
   */
  explicit inline basic_srecord(std::istream& is) : error_(e_ok), type_(type_undefined),
          start_address_(0), header_(), blocks_(), parser_line_(0), error_address_(0),
          default_value_(0x00), strict_parsing_(false)
  { parse(is); }

  /**
   * c'tor, initialized via string parsing
   */
  explicit inline basic_srecord(const std::string& s) : error_(e_ok), type_(type_undefined),
          start_address_(0), header_(), blocks_(), parser_line_(0), error_address_(0),
          default_value_(0x00), strict_parsing_(false)
  { parse(s); }

  /**
   * d'tor
   */
  ~basic_srecord()
  { clear(); }

public:

  /**
   * Clears all instance variables, resets the error.
   */
  inline void clear()
  {
    type_=type_undefined; start_address_=0; blocks_.clear(); header_.clear();
    error_ = e_ok; parser_line_ = 0; error_address_ = 0;
  }

  /**
   * Returns a copy of the current error message.
   * @return std::string
   */
  inline std::string error_message() const
  { return strerr(error_); }

  /**
   * Returns a the current error code.
   * @return error_type
   */
  inline error_type error() const
  { return error_; }

  /**
   * Returns true if the instance has no error.
   * @return bool
   */
  inline bool good() const
  { return error_ == e_ok; }

  /**
   * Returns the default value when ranges are
   * printed or checked that are not set in the
   * S-record. E.g. Zero initialized RAM would
   * read a default value of 0x00, erased Flash
   * ROM would read 0xff.
   * @return value_type
   */
  inline value_type default_value() const
  { return default_value_; }

  /**
   * Sets the default value when ranges are
   * printed or checked that are not set in the
   * S-record. E.g. Zero initialized RAM would
   * read a default value of 0x00, erased Flash
   * ROM would read 0xff.
   * @param value_type val
   */
  inline void default_value(value_type val)
  { default_value_ = val; }

  /**
   * Returns if the parser raises errors on incomplete data, e.g if the S-record
   * does not encompass complete information, e.g. if the S0 or S5/S6 is missing.
   * @return bool
   */
  inline bool strict_parsing() const noexcept
  { return strict_parsing_; }

  /**
   * Defines if the parser shall raise errors on incomplete data, e.g if the
   * S-record does not encompass complete information, e.g. if the S0 or S5/S6
   * is missing.
   * @param bool strict
   */
  inline void strict_parsing(bool strict) noexcept
  { strict_parsing_ = strict; }

  /**
   * Returns a random access reference to the record blocks.
   * @return block_container_type&
   */
  inline block_container_type& blocks()
  { return blocks_; }

  /**
   * Returns a const reference to the record blocks.
   * @return const block_container_type&
   */
  inline const block_container_type& blocks() const
  { return blocks_; }

  /**
   * Returns the first existing address in the whole srecord.
   * Note: This is an address_type, not an iterator. This
   * class does not provide iterators.
   * @return address_type
   */
  inline address_type sadr() const
  { return blocks_.empty() ? 0 : blocks_.front().sadr(); }

  /**
   * Returns the end address of the whole srecord. That is
   * the end() of the last block, t.m. it is one behind the
   * last existing byte in the whole srecord.
   * Note: This is an address_type, not an iterator. This
   * class does not provide iterators.
   * @return address_type
   */
  inline address_type eadr() const
  { return blocks_.empty() ? 0 : blocks_.back().eadr(); }

  /**
   * Returns the type (specifying the address width).
   * @return record_type_type
   */
  inline record_type_type type() const
  { return type_; }

  /**
   * Sets the type (specifying the address width).
   * @param record_type_type new_type
   * @return void
   */
  inline void type(record_type_type new_type)
  { type_ = ((new_type >= type_s1_16bit) && (new_type <= type_s3_32bit)) ? new_type : type_undefined; }

  /**
   * Sets the start address of the whole record (written
   * in S9/S8/S7 lines).
   * @param address_type new_address
   */
  inline void start_address_definition(address_type new_address)
  { start_address_ = new_address; }

  /**
   * Returns the start address of the whole record (S9/S8/S7 information).
   * @return address_type
   */
  inline address_type start_address_definition() const
  { return start_address_; }

  /**
   * Returns the S0 header data as bytes.
   * @return data_type
   */
  inline data_type header() const
  { return header_; }

  /**
   * Sets the S0 header data as bytes.
   * Note: If set, the size is set to a minimum of 10 bytes, as the S0 header is
   *       supposed to be: module name 10bytes, version 1byte, revision 1byte, description 0..18 bytes
   * @param data_type
   */
  inline void header(const data_type& s0data)
  { header_ = s0data; if(header_.size() < 10) header_.resize(10); }

  /**
   * Sets the S0 header as string.
   * @return std::string
   */
  inline std::string header_str() const
  {
    std::string s;
    for(auto c: header_) { if(!c) break; else s += (char) c; }
    while(!s.empty() && ::isspace(s[s.length()-1])) s.resize(s.length()-1);
    return s;
  }

  /**
   * Sets the S0 header from a string.
   * @param std::string
   */
  inline void header_str(std::string s)
  {
    header_.clear();
    if(s.length() > 25) s = s.substr(0, 25); // no string.resize()
    for(auto c: s) header_.push_back((value_type)c);
    if(header_.size() < 10) header_.resize(10);
  }

  /**
   * Returns the last processed parser line. Good to know if
   * the parser has an error.
   * @return unsigned long
   */
  inline unsigned long parser_line() const
  { return parser_line_; }

  /**
   * Returns address where validate() found an error, or 0
   * if no address is affected by an error.
   * @return address_type
   */
  inline address_type error_address() const
  { return error_address_; }

public:

  /**
   * Parse a string.
   * @param const std::string& data
   * @return bool
   */
  inline bool parse(const std::string& data)
  { std::istringstream ss(data); return parse(ss, true); }

  /**
   * Parse a string.
   * @param std::string&& data
   * @return bool
   */
  inline bool parse(std::string&& data)
  { std::istringstream ss(std::move(data)); return parse(ss, true); }

  /**
   * Parses an input stream.
   * By default stream reading is aborted when reading a non-empty
   * line not starting with "S". If `single_file_stream` is `true`,
   * all lines to the end of stream are forcefully parsed.
   *
   * @param std::istream& is
   * @bool single_file_stream=false
   */
  inline bool parse(std::istream& is, const bool single_file_stream=false)
  {
    clear();
    std::string line;
    bool found_s0 = false;
    line_container_type line_block;
    while(std::getline(is, line)) {
      ++parser_line_;
      std::string s;
      for(auto e:line) {
        if(!::isspace(e)) s += e;
      }
      if(s.empty()) continue;
      if((!single_file_stream) && (s[0] != 'S' && s[0] != 's')) {
        auto i = line.length();
        is.putback('\n');
        while(i) is.putback(line[--i]);
        break;
      }
      line_type rec;
      if(!parse_line(std::move(s), rec)) {
        break;
      } else if(rec.type == 0) {
        if(found_s0) {
          // That's already the next S0 --> put back for the next.
          auto i = line.length();
          is.putback('\n');
          while(i) is.putback(line[--i]);
          break;
        } else {
          line_block.push_back(rec);
          found_s0 = true;
        }
      } else {
        line_block.push_back(rec);
      }
    }
    parse_analyze_block(line_block);
    reorder(blocks());
    return good() && validate(strict_parsing());
  }

  /**
   * Recomposes a srec file. Returns success.
   *
   * @param std::ostream& os
   * @param size_type data_line_length
   * @return bool
   */
  bool compose(std::ostream& os, size_type line_length=0)
  {
    if(!good() || !validate()) return false;
    size_type data_line_length = 64;
    {
      const size_type frame_size = (2+2+(2*(1+type()))+2); // Sx+len+(adr)+(no data bytes)+cksum
      const size_type min_line_length = (frame_size+8);  // frame + min 4 data bytes.
      if(line_length == 0) line_length = frame_size + 64;
      else if(line_length > 92) line_length = 92;
      else if(line_length < min_line_length) line_length = min_line_length;
      data_line_length = (line_length-frame_size)/2;
    }
    // Header
    {
      std::deque<typename data_type::value_type> bin;
      for(auto e: header_) bin.push_back(e);
      while(bin.size() < 12) bin.push_back(0);
      bin.push_front(0);
      bin.push_front(0);
      if(type_ > 1) bin.push_front(0);
      if(type_ > 2) bin.push_front(0);
      bin.push_front(bin.size()+1);
      bin.push_back(cksum(bin));
      os << "S0" << tohex(bin) << std::endl;
    }
    // Data
    unsigned long line_data_count = 0;
    {
      record_type_type type = type_;
      for(const block_type& block: blocks_) {
        std::deque<typename data_type::value_type> bin;
        const data_type& bytes = block.bytes();
        typename data_type::size_type sz = bytes.size();
        address_type address = block.sadr();
        unsigned i = 0;
        while(i < sz) {
          // Get one line, either to max chars or until no bytes left in the block binary data.
          for(unsigned data_bytes = 0; (data_bytes < data_line_length) && (i < sz); ++data_bytes, ++i) {
            bin.push_back(bytes[i]);
          }
          // Prepend address, update address
          // Append checksum over address and binary data
          // Prepend count/length
          {
            unsigned line_sz = bin.size();
            bin.push_front((address>>0) & 0xff);
            bin.push_front((address>>8) & 0xff);
            if(type > 1) bin.push_front((address>>16) & 0xff);
            if(type > 2) bin.push_front((address>>24) & 0xff);
            address += line_sz;
            bin.push_front(bin.size()+1);
            bin.push_back(cksum(bin));
          }
          // Prepend type, out
          {
            std::string line = "S";
            line.reserve(2*data_line_length+18);
            line += ('0'+type);
            line += tohex(bin);
            os << line << std::endl;
            bin.clear();
            ++line_data_count;
          }
        }
      }
    }
    // Data line count
    {
      std::deque<typename data_type::value_type> bin;
      bin.push_front((line_data_count>>0) & 0xff);
      bin.push_front((line_data_count>>8) & 0xff);
      line_data_count >>= 16;
      if(line_data_count) bin.push_front(line_data_count & 0xff);
      line_data_count >>= 8;
      if(line_data_count) return error(e_compose_max_number_of_data_lines_exceeded);
      bin.push_front(bin.size()+1);
      bin.push_back(cksum(bin));
      std::string s = bin.size() == 4 ? "S5" : "S6";
      s += tohex(bin);
      os << s << std::endl;
    }
    // Start address (termination)
    {
      std::deque<typename data_type::value_type> bin;
      bin.push_front((start_address_>>0) & 0xff);
      bin.push_front((start_address_>>8) & 0xff);
      if(type_ > 1) bin.push_front((start_address_>>16) & 0xff);
      if(type_ > 2) bin.push_front((start_address_>>24) & 0xff);
      bin.push_front(bin.size()+1);
      bin.push_back(cksum(bin));
      std::string s = "S";
      s += ('0'+(10-type_));
      s += tohex(bin);
      os << s << std::endl;
    }
    return true;
  }

  /**
   * Recomposes a srec file to a string. Returns an empty
   * string on error.
   *
   * @param size_type data_line_length
   * @return std::string
   */
  std::string compose(size_type line_length=0)
  { std::stringstream ss; return compose(ss, line_length) ? (ss.str()) : (std::string()); }

  /**
   * Human readable dump to a defined ostream.
   * @param std::ostream&
   */
  inline void dump(std::ostream& os) const
  {
    os << "srec {" << std::endl;
    os << " data type: ";
    if(type() == 0) {
      os << "(auto/not set)";
    } else if(type() > 0 && type() < 4) {
      os << "S" << (int) type();
    } else {
      os << "(invalid)";
    }
    os << std::endl;
    os << " blocks: [" << std::endl;
    for(const block_type& e: blocks()) {
      std::string s;
      {
        std::ostringstream ss;
        e.dump(ss);
        s = ss.str();
      }
      std::istringstream ss(s);
      s.clear();
      while(std::getline(ss, s)) {
        if(s.empty()) continue;
        os << "    " << s << std::endl;
      }
      os << std::endl;
    }
    os << " ]" << std::endl;
    os << "}" << std::endl;
  }

  /**
   * Human readable string dump.
   * @return std::string
   */
  inline std::string dump() const
  { std::stringstream ss; dump(ss); return ss.str(); }

  /**
   * Loads an S-record file (.srec/.s19/.s28/.s37 ...).
   * Returns success, that is: The file path is not empty,
   * opening the file succeeded, parsing succeeded, and
   * file whole file was read (only contains S-Record data
   * and no comments or the like). If parsing fails, the
   * error is set in the instance, otherwise there was a
   * file input error.
   *
   * @param std::string file_path
   * @param basic_srecord& srec
   * @return bool
   */
  static inline bool load(std::string file_path, basic_srecord& srec)
  { return load(file_path.c_str(), srec); }

  /**
   * Loads an S-record file (.srec/.s19/.s28/.s37 ...).
   * Returns success, that is: The file path is not empty,
   * opening the file succeeded, parsing succeeded, and
   * file whole file was read (only contains S-Record data
   * and no comments or the like). If parsing fails, the
   * error is set in the instance, otherwise there was a
   * file input error.
   *
   * @param const char* file_path
   * @param basic_srecord& srec
   * @return bool
   */
  static inline bool load(const char* file_path, basic_srecord& srec)
  {
    srec.clear();
    if(!file_path || !file_path[0]) return false;
    std::ifstream fs(file_path);
    if((!fs.good()) || (!srec.parse(fs, true))) return false;
    return fs.eof();
  }

  /**
   * Loads an S-record file (.srec/.s19/.s28/.s37 ...).
   * Returns a basic_srecord object containing the parsed data.
   * On file stream error or parse error, the `error()` of the
   * instance is set accordingly.
   *
   * @param const char* file_path
   * @return basic_srecord
   */
  static inline basic_srecord load(const char* file_path)
  {
    basic_srecord srec;
    if(!file_path || !file_path[0]) { srec.error(e_load_open_failed); return srec; }
    std::ifstream fs(file_path);
    if(!fs.good()) { srec.error(e_load_open_failed); return srec; }
    srec.parse(fs, true);
    return srec;
  }

  /**
   * Loads an S-record file (.srec/.s19/.s28/.s37 ...).
   * Returns a basic_srecord object containing the parsed data.
   * On file stream error or parse error, the `error()` of the
   * instance is set accordingly.
   *
   * @param std::string file_path
   * @return basic_srecord
   */
  static inline basic_srecord load(std::string file_path)
  { return load(file_path.c_str()); }

  /**
   * Checks if the current "image" saved in the
   * instance is ok. If no address width type is set,
   * it sets this type implicitly to the minimum possible
   * address type (means S1/S2/S3).
   * @param bool strict =true
   * @return bool
   */
  bool validate(bool strict=true)
  {
    if(!good()) return false;
    // Check/set address type
    {
      record_type_type type = type_s1_16bit;
      for(block_type& block: blocks_) {
        if(block.eadr() > 0x100000000ull) { return error(e_validate_record_range_exceeded); }
        if(block.eadr() > 0x001000000ull) { type = type_s3_32bit; break; }
        if(block.eadr() > 0x000010000ull) { type = type_s2_24bit;        }
      }
      if(type_ == type_undefined) {
        type_ = type;
      } else if(type_ < type) {
        if(strict) {
          return error(e_validate_record_type_to_small);
        } else {
          type_ = type;
        }
      }
    }
    // Variable checks
    {
      if((type_ < type_s1_16bit) || (type_ > type_s3_32bit)) {
        return error(e_validate_record_type_to_small);
      }
      if(blocks_.empty()) {
        return error(e_validate_no_binary_data);
      }
    }
    // Block range check, note: blocks are ordered by address
    // when parsing or modifying. We only check that here.
    {
      for(size_type i=1; i<blocks_.size(); ++i) {
        if(blocks_[i].sadr() < blocks_[i-1].sadr()) {
          error_address_ = blocks_[i].sadr();
          return error(e_validate_blocks_unordered);
        } else if(blocks_[i-1].sadr() + blocks_[i-1].size() > blocks_[i].sadr()) {
          error_address_ = blocks_[i].sadr();
          return error(e_validate_overlapping_blocks);
        }
      }
    }
    return true;
  }

  /**
   * Returns an ordered container of blocks that are in the
   * specified range.
   *
   * @param address_type start_address
   * @param address_type end_address
   * @return block_container_type
   */
  inline block_container_type get_ranges(address_type start_address, address_type end_address) const
  {
    block_container_type blocks;
    if(start_address >= end_address) return blocks;
    for(auto& e: blocks_) {
      block_type blk = e.get_range(start_address, end_address);
      if(!blk.empty()) blocks.push_back(std::move(blk));
    }
    reorder(blocks);
    return blocks;
  }

  /**
   * Returns a range that starts at the address `start_address` and
   * ends just before `end_address`. Unassigned memory ranges are
   * filled with the value `fill_value`;
   * @param address_type start_address
   * @param address_type end_address
   * @param value_type fill_value
   * @return block_type
   */
  inline block_type get_range(address_type start_address, address_type end_address, value_type fill_value) const
  {
    block_type block;
    block_container_type blocks = get_ranges(start_address, end_address);
    block = connect(std::move(blocks), fill_value);
    block.sadr(start_address); // in case get_ranges() was empty
    extend(block, start_address, end_address, fill_value);
    return block;
  }

  /**
   * Returns a range that starts at the address `start_address` and
   * ends just before `end_address`. Unassigned memory ranges are
   * filled with the `default_value()`.
   * @param address_type start_address
   * @param address_type end_address
   * @return block_type
   */
  inline block_type get_range(address_type start_address, address_type end_address) const
  { return get_range(start_address, end_address, default_value()); }

  /**
   * Copies the contents of a given block to the appropriate
   * position. Extends the instance address range if needed,
   * overwrites existing blocks. Might merge and rearrange
   * blocks, means drop pointers or references to blocks
   * after using this method.
   * @param block_type&& block
   * @return basic_srecord&
   */
  basic_srecord& set_range(block_type&& block)
  {
    // The easy cases: No existing blocks are affected
    {
      bool affected = false;
      for(auto &e: blocks()) {
        if(e.in_range(block.sadr(), block.eadr())) {
          affected = true;
          break;
        }
      }
      if(!affected) {
        blocks().push_back(block);
        reorder(blocks());
        connect_adjacent_blocks();
        return *this;
      }
    }
    // The normal cases: Existing blocks are affected. Determmine first and last affected block.
    reorder(blocks());
    size_type i_first = 0;
    while(i_first < blocks().size() && !blocks()[i_first].in_range(block.sadr(), block.eadr()) ) {
      ++i_first;
    }
    size_type i_last = blocks().size()-1;
    while(i_last && !blocks()[i_last].in_range(block.sadr(), block.eadr()) ) {
      --i_last;
    }
    // All all blocks except the first and the last will be overwritten anyway.
    for(size_type i = i_first+1; i < i_last; ++i) {
      blocks()[i].bytes().clear();
    }
    block_type before = blocks()[i_first].get_range(blocks()[i_first].sadr(), block.sadr());
    block_type after  = blocks()[i_last].get_range(block.eadr(), blocks()[i_last].eadr());
    blocks()[i_first].bytes().clear();
    blocks()[i_last].bytes().clear();
    blocks().push_back(before);
    blocks().push_back(block);
    blocks().push_back(after);
    remove_empty_blocks();
    reorder(blocks());
    connect_adjacent_blocks();
    return *this;
  }

  /**
   * Copies the contents of a given block to the appropriate
   * position. Extends the instance address range if needed,
   * overwrites existing blocks. Might merge and re-arrange
   * blocks, means drop pointers or references to blocks
   * after using this method.
   * @param const block_type& block
   * @return basic_srecord&
   */
  inline basic_srecord& set_range(const block_type& block)
  { block_type blk = block; return set_range(std::move(blk)); }

  /**
   * Copies the contents of given byte data to the appropriate
   * address. Extends the instance address range if needed,
   * overwrites existing blocks. Might merge and re-arrange
   * blocks, means drop pointers or references to blocks
   * after using this method.
   * @param address_type address
   * @param const data_type& data
   * @return basic_srecord&
   */
  inline basic_srecord& set_range(address_type address, data_type&& data)
  { block_type blk; blk.sadr(address); blk.bytes(std::move(data)); return set_range(std::move(blk)); }

  /**
   * Copies the contents of given byte data to the appropriate
   * address. Extends the instance address range if needed,
   * overwrites existing blocks. Might merge and rearrange
   * blocks, means drop pointers or references to blocks
   * after using this method.
   * @param address_type address
   * @param const data_type& data
   * @return basic_srecord&
   */
  inline basic_srecord& set_range(address_type address, const data_type& data)
  { block_type blk(address, data); return set_range(blk); }

  /**
   * Copies the contents of given byte data to the appropriate
   * address. Extends the instance address range if needed,
   * overwrites existing blocks. Might merge and re-arrange
   * blocks, means drop pointers or references to blocks
   * after using this method.
   * @tparam typename Container
   * @param address_type address
   * @param const Container& data
   * @return basic_srecord&
   */
  template<typename Container>
  inline typename std::enable_if<std::is_same<typename Container::value_type, value_type>::value, basic_srecord&>
  ::type set_range(address_type address, const Container& data)
  {
    if(data.empty()) return *this;
    auto bytes = data_type();
    bytes.reserve(data.size());
    for(auto& e:data) bytes.push_back(e);
    set_range(std::move(bytes));
    return *this;
  }

  /**
   * Copies the contents of given byte data to the appropriate
   * address. Extends the instance address range if needed,
   * overwrites existing blocks. Might merge and re-arrange
   * blocks, means drop pointers or references to blocks
   * after using this method.
   * @tparam typename Container
   * @param address_type address
   * @param const Container& data
   */
  template<typename Container>
  inline typename std::enable_if<std::is_same<typename Container::value_type, value_type>::value,void>
  ::type set_range(address_type address, const Container& data)
  {
    if(data.empty()) return *this;
    auto bytes = data_type();
    bytes.reserve(data.size());
    for(auto& e:data) bytes.push_back(e);
    set_range(std::move(bytes));
    return *this;
  }

  /**
   * Removes an address range from the srecord. Might split and
   * rearrange blocks, means you should drop pointers or references
   * to blocks after using this method.
   * @param address_type start_address
   * @param address_type end_address
   * @return basic_srecord&
   */
  basic_srecord& remove_range(address_type start_address, address_type end_address)
  {
    if((start_address >= end_address) || blocks().empty()) return *this;
    size_type i_first = blocks().size();
    size_type i;
    for(i = 0; i < blocks().size(); ++i) {
      if(blocks()[i].in_range(start_address, end_address)) {
        i_first = i;
        break;
      }
    }
    if(i_first >= blocks().size()) return *this;
    size_type i_last = i_first;
    for(i = i_first; i < blocks().size(); ++i) {
      if(!blocks()[i].in_range(start_address, end_address)) break;
      i_last = i;
    }
    if(i_last >= blocks().size()) {
      i_last = blocks().size() - 1;
    }
    if(i_first == i_last) {
      i = i_first;
      if(blocks()[i].sadr() == start_address) {
        // Aligned to begin, just shrink the block
        shrink_to(blocks()[i], end_address, blocks()[i].eadr());
        if(blocks()[i].empty()) remove_empty_blocks();
      } else if(blocks()[i].eadr() == end_address) {
        // Aligned to end, also shrink
        shrink_to(blocks()[i], blocks()[i].sadr(), start_address);
        if(blocks()[i].empty()) remove_empty_blocks();
      } else {
        // otherwise split it.
        block_type blk;
        blk.swap(blocks()[i]);
        blocks()[i] = blk.get_range(blk.sadr(), start_address);
        blk = blk.get_range(end_address, blk.eadr());
        blocks().push_back(blk);
        remove_empty_blocks();
        reorder(blocks());
      }
    } else {
      i = i_first + 1;
      while(i < i_last) {
        blocks()[i++].clear();
      }
      {
        block_type& blk = blocks()[i_first];
        if(start_address == blk.sadr()) {
          blk.clear();
        } else {
          blk = blk.get_range(blk.sadr(), start_address);
        }
      }
      {
        block_type& blk = blocks()[i_last];
        if(end_address == blk.eadr()) {
          blk.clear();
        } else {
          blk = blk.get_range(end_address, blk.eadr());
        }
      }
      remove_empty_blocks();
      reorder(blocks());
    }
    return *this;
  }

  /**
   * Connects all blocks of this instance to one block, applying a `fill_value`
   * in unassigned ranges. Overlapping blocks implicitly overwrite, where the
   * overlapping contents of later blocks overwrite the overlapping ranges of
   * earlier blocks.
   * @param value_type fill_value
   * @return basic_srecord&
   */
  inline basic_srecord& merge(value_type fill_value)
  {
    block_container_type blks;
    blocks_.swap(blks);
    blocks_.push_back(connect(std::move(blks), fill_value));
    return *this;
  }

  /**
   * Connects all blocks of this instance to one block, applying the instance
   * `default_value()` to fill unassigned ranges. Overlapping blocks implicitly
   * overwrite, where the overlapping contents of later blocks overwrite the
   * overlapping ranges of earlier blocks.
   * @return basic_srecord&
   */
  inline basic_srecord& merge()
  { return merge(default_value()); }

  /**
   * Returns the address of the first byte where the `sequence`
   * was found, or `end()` if the sequence was not found at all.
   * @param const data_type& sequence
   * @param address_type start_address
   * @return address_type
   */
  address_type find(const data_type& sequence, address_type start_address=0) const
  {
    const size_type seq_size = sequence.size();
    if(sequence.empty() || blocks().empty() || ((start_address < sadr()) && ((start_address+seq_size) > eadr()))) {
      return eadr();
    }
    // As all exported methods connect blocks, it is assured that a
    // sequence cannot spread accross multiple blocks.
    size_type i_block = 0;
    while(i_block < blocks().size() && blocks()[i_block].sadr() < start_address) {
      ++i_block;
    }
    if(i_block >= blocks().size()) {
      return eadr();
    }
    // Initial in-block vector index.
    size_type i = (start_address > blocks()[i_block].sadr()) ? (start_address - blocks()[i_block].sadr()) : 0;
    // Block iteration
    while(i_block < blocks().size()) {
      // Analyse block ...
      const data_type& bytes = blocks()[i_block].bytes();
      size_type block_size = bytes.size();
      // Iterate bytes
      while(i < block_size) {
        // Find first byte match.
        while(i < block_size) {
          if(bytes[i] == sequence[0]) break;
          ++i;
        }
        if(i >= block_size) {
          // Not found here, next block
          break;
        } else if(seq_size <= 1) {
          // Special case: first match already enough.
          return blocks()[i_block].sadr() + i;
        } else {
          // Check next matches (byte compare from position i, and 1 in the sequence).
          size_type j = i;
          size_type match_size = 1;
          while(++j < block_size) {
            if(sequence[match_size] == bytes[j]) {
              if(++match_size >= seq_size) {
                // The complete seq starting at i matched --> return address.
                return blocks()[i_block].sadr() + i;
              }
            } else {
              break; // No match, retry with next i.
            }
          }
        }
        ++i;
      }
      ++i_block;
      i = 0;
    }
    return eadr(); // Not found
  }

private:

  /**
   * Merges blocks. Fills unassigned ranges in between with with `fill_value`.
   * @param block_container_type&& blocks
   * @return block_type
   */
  static block_type connect(block_container_type&& blocks, value_type fill_value)
  {
    block_type block;
    if(blocks.empty()) return block;
    if(blocks.size() < 2) return blocks.front();
    reorder(blocks);
    for(size_type i=1; i<blocks.size(); ++i) {
      block_type& a = blocks[i-1];
      block_type& b = blocks[i];
      if(a.eadr() == b.sadr()) {
        continue;
      } else {
        while(a.eadr() > b.sadr()) a.bytes().pop_back();
        while(a.eadr() < b.sadr()) a.bytes().push_back(fill_value);
      }
    }
    {
      size_type sz = 0;
      for(auto& e: blocks) {
        sz += e.size();
      }
      block.sadr(blocks.front().sadr());
      if(!sz) return block;
      block.bytes().swap(blocks.front().bytes());
      block.bytes().reserve(sz);
      for(auto& blk: blocks) {
        for(auto byte: blk.bytes()) {
          block.bytes().push_back(byte);
        }
        data_type().swap(blk.bytes());
      }
    }
    return block;
  }

  /**
   * Reorders blocks, so that the start addresses are sorted
   * ascending.
   * @param block_container_type& blocks
   */
  static inline void reorder(block_container_type& blocks)
  {
    if(blocks.size() < 2) return;
    std::sort(blocks.begin(), blocks.end(), [](const block_type& a, const block_type& b){
      return a.sadr() < b.sadr();
    });
  }

  /**
   * Extends a block to a given start/end address, filling the unassigned
   * bytes with a `fill_value`. Note: This function does not shrink the
   * block.
   * @param block_type& block
   * @param address_type start_address
   * @param address_type end_address
   * @param value_type fill_value
   * @return block_type
   */
  static void extend(block_type& block, address_type start_address, address_type end_address, value_type fill_value)
  {
    if(start_address >= end_address) return;
    data_type& bytes = block.bytes();
    if(start_address < block.sadr()) {
      size_type offset = block.sadr() - start_address;
      bytes.resize(bytes.size() + offset);
      size_type i = bytes.size()-offset, j = bytes.size();
      while(i) bytes[--j] = bytes[--i];
      while(j) bytes[--j] = fill_value;
      block.sadr(start_address);
    }
    while(end_address > block.eadr()) bytes.push_back(fill_value);
  }

  /**
   * Shrinks a block to a given start/end address
   * @param block_type& block
   * @param address_type start_address
   * @param address_type end_address
   * @return block_type
   */
  static void shrink_to(block_type& block, address_type start_address, address_type end_address)
  {
    if(start_address >= end_address) {
      block.sadr(block.eadr());
      block.bytes().clear();
      return;
    } else {
      data_type& bytes = block.bytes();
      const size_type size = ((end_address-start_address) <= bytes.size()) ? (end_address-start_address) : bytes.size();
      if(start_address > block.sadr()) {
        const size_type offset = start_address - block.sadr();
        size_type i = offset, j = 0;
        while(i < bytes.size()) bytes[j++] = bytes[i++];
        block.sadr(start_address);
      }
      bytes.resize(size);
    }
  }

  /**
   * Returns a c-string of an error code.
   *
   * @param error_type e
   * @return const char*
   */
  static const char* strerr(error_type e)
  {
    static const char* es[] = {
      "Ok",
      "[parse] Unacceptable character",
      "[parse] Line not starting with S",
      "[parse] Invalid line length",
      "[parse] Invalid record type",
      "[parse] Line checksum mismatch",
      "[parse] Line data length mismatch",
      "[parse] Missing record header (S0)",
      "[parse] S0 address field is nonzero",
      "[parse] Duplicate S5/S6 line found",
      "[parse] Number of data lines does not match the declaration (S5/S6)",
      "[parse] Duplicate start address specification (S7/S8/S9)",
      "[parse] Missing start address line (S7/S8/S9)",
      "[parse] Missing data lines (S1/S2/S3)",
      "[parse] Mixed data types in one record (S1/S2/S3)",
      "[compose] The output has too many data lines for the S5/S6 line data.",
      "[validate] The specified record type (S1/S2/S3) is to small for the needed data address range.",
      "[validate] The data range exceeds the greatest possible address of an s-record.",
      "[validate] No binary data blocks to write found in a record.",
      "[validate] Unordered data blocks detected",
      "[validate] Overlapping data blocks detected (address range collision)",
      "[load] Opening file failed",
      ""
    };
    return (e < sizeof(es)/sizeof(const char*)) ? es[e] : "unknown error";
  };

  /**
   * Parse a line
   *
   * @param std::string&& line
   * @param line_type& rec
   * @return bool
   */
  inline bool parse_line(std::string&& line, line_type& rec)
  {
    if(!good()) return false;
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
    if(line.find_first_not_of("0123456789ABCDEFS") != std::string::npos) {
      return error(e_parse_unacceptable_character);
    } else if(line[0] != 'S') {
      return error(e_parse_line_not_starting_with_s);
    } else if((line[1] < '0') || (line[1] > '9')) {
      return error(e_parse_invalid_record_type);
    } else if((line.length() & 0x01) || (line.length() < 10) || (line.length() > 514)) {
      // Line length not even, or less than minimum of 10 === "SxLLAAAACC", L=length bytes, A=address bytes, C=cksum
      return error(e_parse_invalid_line_length);
    } else {
      // HEX->blob
      std::deque<typename data_type::value_type> bin;
      bin.push_back(line[1]-'0'); // S0 to S9 --> value 0 to 9.
      for(size_type i=2; i < line.length(); i+=2) {
        bin.push_back(0
          | ((line[i+0] - (line[i+0] >= 'A' ? ('A'-10) : '0')) << 4)
          | ((line[i+1] - (line[i+1] >= 'A' ? ('A'-10) : '0')) << 0)
        );
      }
      // Record type check
      {
        record_type_type type = (record_type_type) bin[0];
        if((type > 9) || (type == 4)) {
          // S0 --> S9, S4 is reserved
          return error(e_parse_invalid_record_type);
        }
        rec.type = type;
      }
      // Checksum
      {
        unsigned cksum = 0;
        for(size_type i=1; i < bin.size()-1; ++i) cksum += bin[i];
        cksum = (~cksum) & 0xff;
        if(cksum != bin[bin.size()-1]) {
          return error(e_parse_chcksum_incorrect);
        }
      }
      // Byte count
      {
        unsigned byte_count = bin[1];
        if((byte_count < 3u) || (byte_count != bin.size()-2u)) {
          return error(e_parse_length_mismatch);
        }
      }
      // Pop type, byte count, checksum
      {
        bin.pop_front();
        bin.pop_front();
        bin.pop_back();
      }
      // Address
      {
        address_type adr = 0;
        switch((unsigned)rec.type) {
          case 0: // Block header, address must be zero
            if(bin[0] || bin[1]) {
              return error(e_parse_s0_address_nonzero);
            }
            bin.pop_front();
            bin.pop_front();
            break;
          case 1: // Data line, 16 bit address
          case 2: // Data line, 24 bit address
          case 3: // Data line, 32 bit address
            for(unsigned i=rec.type+1; i; --i) {
              adr <<= 8; adr |= bin.front(); bin.pop_front();
            }
            break;
          case 5: // Optional block line count: 16 bit value
          case 6: // Optional block line count: 24 bit value
            for(unsigned i=rec.type-3; i; --i) {
              adr <<= 8; adr |= bin.front(); bin.pop_front();
            }
            break;
          case 7: // Block termination for S3: Start Address 32bit address
          case 8: // Block termination for S2: Start Address 24bit address
          case 9: // Block termination for S1: Start Address 16bit address
            for(unsigned i=11-rec.type; i; --i) {
              adr <<= 8; adr |= bin.front(); bin.pop_front();
            }
            break;
          default:
            return error(e_parse_invalid_record_type);
        }
        rec.address = adr;
      }
      // Data
      {
        if(!bin.empty()) {
          rec.bytes.resize(bin.size());
          std::copy(bin.begin(), bin.end(), rec.bytes.begin());
        }
      }
      // Alright
      return true;
    }
  }

  /**
   * Analyse a parsed block, return success.
   *
   * @param line_container_type& lines
   * @return bool
   */
  inline bool parse_analyze_block(line_container_type& lines)
  {
    if(!good()) return false;
    // Collect and check line data
    if(lines.empty()) {
      return error(e_parse_missing_data_lines);
    } else if(lines.front().type != 0) {
      if(strict_parsing()) return error(e_parse_missing_s0);
    } else {
      header_ = lines.front().bytes; // Checked for multiple headers already in parse().
      lines.pop_front();
    }
    record_type_type type = type_undefined;  // S1/S2/S3
    bool have_start_address = false;  // S7/S8/S9
    long spec_count = 0; // S5/S6
    long count = 0; // Check for S5/S6
    for(auto& e: lines) {
      if(e.type < 4) {
        type = e.type;
        break;
      }
    }
    if(!type) {
      return error(e_parse_missing_data_lines);
    }
    type_ = type;
    for(auto e: lines) {
      if(e.type < 4) {
        // Data lines (header is out)
        if((e.type != type) && strict_parsing()) {
          return error(e_parse_mixed_data_line_types);
        }
        if((!blocks_.empty()) && (e.address == (blocks_.back().sadr() + blocks_.back().size()))) {
          for(auto ee: e.bytes) {
            blocks_.back().bytes().push_back(ee);
          }
        } else {
          block_type block;
          block.sadr(e.address);
          block.bytes() = e.bytes;
          if(blocks_.empty() || (block.sadr() >= blocks_.back().sadr())) {
            blocks_.push_back(block); // vector: push is fast.
          } else {
            typename block_container_type::iterator it = blocks_.begin();
            while(it != blocks_.end() && (block.sadr() >= it->sadr())) ++it;
            blocks_.insert(it, block);
          }
        }
        ++count;
      } else if(e.type < 7) {
        // Line count lines
        if(spec_count) {
          return error(e_parse_duplicate_data_count);
        }
        spec_count = (long) e.address;
      } else {
        // Start addresses S7/S8/S9
        // S<0, S4, S>9 already filtered out, S0/1/2/3 and S5/6 handled above --> S7/8/9 left.
        if(have_start_address && strict_parsing()) {
          return error(e_parse_duplicate_start_address);
        }
        have_start_address = true;
        if((e.type != 10-type) && strict_parsing()) return error(e_parse_startaddress_vs_data_type_mismatch);
        start_address_ = e.address;
      }
    }
    if(spec_count && (spec_count != count)) {
      return error(e_parse_line_count_mismatch);
    }
    // All ok
    return true;
  }

  /**
   * Error setter
   * @param error_type e
   * @return bool
   */
  inline bool error(error_type e)
  { error_ = e; return e == e_ok; }

  /**
   * Connects blocks where the condition
   * block[n].end() == block[n+1].begin()
   */
  inline void connect_adjacent_blocks()
  {
    if(blocks().size() < 2) return;
    size_type i=0, j=1;
    while(j < blocks().size() && blocks()[i].empty()) {
      ++i; ++j;
    }
    while(j < blocks().size()) {
      if((blocks()[i].eadr() != blocks()[j].sadr())) {
        i = j++;
      } else {
        block_type& a = blocks()[i];
        block_type b;
        b.swap(blocks()[j]);
        a.bytes().reserve(a.bytes().size()+b.bytes().size()); // append b to a
        for(auto e: b.bytes()) a.bytes().push_back(e);
        ++j;
      }
    }
    remove_empty_blocks();
  }

  /**
   * Erases blocks with the size 0 from the block container.
   */
  inline void remove_empty_blocks()
  {
    block_container_type blks;
    blks.swap(blocks_); // now the memory is in blks
    for(auto& e: blks) {
      if(!e.empty()) {
        blocks_.push_back(block_type()); // push fresh instance, and ...
        blocks_.back().swap(e);  // swap the contents.
      }
    }
  }

  /**
   * Number to hex
   *
   * @tparam typename T
   * @param T n
   * @return std::string
   */
  template <typename T>
  static inline std::string numtohex(T n)
  {
    static const char* hx = "0123456789ABCDEF";
    constexpr size_type num_chars = sizeof(T)*2;
    char s[num_chars+1];
    s[num_chars] = '\0';
    char *p = &(s[num_chars]);
    while(p > s) {
      *(--p) = hx[n & 0xf];
      n >>= 4;
    }
    return std::string(p);
  }

  /**
   * Number to hex, width specified in bytes
   *
   * @param unsigned long n
   * @param unsigned width
   * @return std::string
   */
  template <typename T>
  static inline std::string numtohex(T n, unsigned width)
  {
    static const char* hx = "0123456789ABCDEF";
    char s[33];
    if(width > 4) {
      width = 4; // limit width to 32bit
    } else if(!width) {
      width = 1; // limit width to 8bit
    }
    s[sizeof(s)-1] = '\0'; // terminate @ last char
    char *p = &(s[sizeof(s)-1]); // intentionally sizeof(s), as --p below.
    width <<= 1; // * 2 --> byte to hex chars
    while(width--) {
      *(--p) = hx[n & 0xf];
      n >>= 4;
    }
    return std::string(p);
  }

  /**
   * Container of numbers to hex
   *
   * @tparam typename ContainerType
   * @param const ContainerType bin
   * @return std::string
   */
  template <typename ContainerType>
  static inline std::string tohex(const ContainerType& bin) {
    std::string s;
    for(auto e:bin) s += numtohex(e);
    return s;
  }

  /**
   * Calculate the line checksum
   * @tparam ContainerType
   * @param const ContainerType& bin
   * @return value_type
   */
  template <typename ContainerType>
  static value_type cksum(const ContainerType& bin)
  {
    unsigned cksum = 0;
    for(auto e:bin) cksum += e;
    cksum = (~(cksum)) & 0xff;  // complement, only one byte.
    return (value_type) cksum;
  }

private:

  error_type error_;              ///< Current error
  record_type_type type_;         ///< Type of the data lines (S1,S2 or S3)
  address_type start_address_;    ///< Address offset from the S9/S8/S7 address field.
  data_type header_;              ///< Binary data of the S0 field (without address and checksum)
  block_container_type blocks_;   ///< Contains unconnected blocks of memory ranges.
  unsigned long parser_line_;     ///< Line counter of the parser
  address_type error_address_;    ///< The address where an error was found
  value_type default_value_;      ///< The value that is read in unset address ranges (e.g. RAM 0x00, FLASH 0xff).
  bool strict_parsing_;           ///< Raises errors if the S-record does not encompass complete information, e.g. if the S0 or S5/S6 is missing.

};
}}

/**
 * Stream out --> compose.
 * @param std::ostream&
 * @param const basic_srecord<V,C>&
 * @return std::ostream&
 */
template <typename V, typename C>
std::ostream& operator<<(std::ostream& os, sw::detail::basic_srecord<V,C>& rec)
{ rec.compose(os, 16); return os; }

/**
 * Stream in --> parse.
 * @param std::istream&
 * @param const basic_srecord<V,C>&
 * @return std::istream&
 */
template <typename V, typename C>
std::istream& operator>>(std::istream& is, sw::detail::basic_srecord<V,C>& rec)
{ rec.parse(is); return is; }

/**
 * Default specialisation.
 */
namespace sw {
  using srecord = detail::basic_srecord<unsigned char>;
}

#endif
