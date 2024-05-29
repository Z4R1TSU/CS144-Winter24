#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  	return is_close_;
}

void Writer::push( string data )
{
  	buf_.push_back(data);
	bytePushed_ += data.size();
}

void Writer::close()
{
  	is_close_ = true;
}

uint64_t Writer::available_capacity() const
{
	uint64_t size = 0;
	for (const string& s : buf_) {
		size += s.size();
	}
  	return (capacity_ - size);
}

uint64_t Writer::bytes_pushed() const
{
  	return bytePushed_;
}

bool Reader::is_finished() const
{
  	return bytePushed_ == bytePoped_;
}

uint64_t Reader::bytes_popped() const
{
  	return bytePoped_;
}

string_view Reader::peek() const
{
  	return string_view(buf_[0]);
}

void Reader::pop( uint64_t len )
{
 	uint64_t cnt = 0;
	for (;;) {
		uint64_t size = buf_[0].size();
		if (cnt == len) {
			break;
		}
		if (cnt + size < len) {
			buf_.erase(buf_.begin());
		} else {
			buf_[0] = buf_[0].substr(len - cnt);
			break;
		}
	}
}

uint64_t Reader::bytes_buffered() const
{
	uint64_t size = 0;
    for (const string& s : buf_) {
		size += s.size();
	}
	return size;
}
