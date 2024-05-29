#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) 
	: capacity_( capacity )
	, is_close_(false)
	, bytePushed_(0)
	, bytePoped_(0)
	, buf_()
	, peek_()
{}

bool Writer::is_closed() const
{
  	return is_close_;
}

void Writer::push( string data )
{
	int push_size = min(available_capacity(), data.size());
	for (int i = 0; i < push_size; i ++) {
		buf_.push_back(data[i]);
	}
	bytePushed_ += push_size;
}

void Writer::close()
{
  	is_close_ = true;
}

uint64_t Writer::available_capacity() const
{
  	return (capacity_ - buf_.size());
}

uint64_t Writer::bytes_pushed() const
{
  	return bytePushed_;
}

bool Reader::is_finished() const
{
  	return (is_close_ && bytePushed_ == bytePoped_);
}

uint64_t Reader::bytes_popped() const
{
  	return bytePoped_;
}

string_view Reader::peek() const
{
	// if (buf_.empty()) {
	// 	return string_view();
	// }
	// return string_view(&buf_.front(), buf_.size());
    peek_.clear();

    for (const auto& ch : buf_) {
        peek_.push_back(ch);
    }

    return peek_;
}

void Reader::pop( uint64_t len )
{
	int pop_size = min(len, buf_.size());
	for (int i = 0; i < pop_size; i ++) {
		buf_.pop_front();
	}
	bytePoped_ += pop_size;
}

uint64_t Reader::bytes_buffered() const
{
	return buf_.size();
}
