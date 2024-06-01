#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return is_close_;
}

void Writer::push( string data )
{
  // Your code here.
  if (is_close_) {
    return;
  }

  uint64_t push_size = std::min(data.length(), available_capacity());
  
  // append buffer with data's form part
  buffer.append(data.substr(0, push_size));

  pushcnt_ += push_size;

  return;
}

void Writer::close()
{
  // Your code here.
  is_close_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return (capacity_ - buffer.size());
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return pushcnt_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return (is_close_ && pushcnt_ == popcnt_);
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return popcnt_;
}

string_view Reader::peek() const
{
  // Your code here.
  if (buffer.empty()) {
    return {};
  }

  return std::string_view(buffer);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t pop_size = std::min(len, buffer.size());

  // modify buffer to its last part (start at index pop_size)
  buffer = buffer.substr(pop_size);

  popcnt_ += pop_size;

  return;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer.size();
}