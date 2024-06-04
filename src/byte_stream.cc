#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
	return is_close_;
}

void Writer::push( string data )
{
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
	is_close_ = true;
}

uint64_t Writer::available_capacity() const
{
	return (capacity_ - buffer.size());
}

uint64_t Writer::bytes_pushed() const
{
	return pushcnt_;
}

bool Reader::is_finished() const
{
	return (is_close_ && pushcnt_ == popcnt_);
}

uint64_t Reader::bytes_popped() const
{
	return popcnt_;
}

string_view Reader::peek() const
{
	if (buffer.empty()) {
		return {};
	}

	return std::string_view(buffer);
}

void Reader::pop( uint64_t len )
{
	uint64_t pop_size = std::min(len, buffer.size());

	buffer = buffer.substr(pop_size);

	popcnt_ += pop_size;

	return;
}

uint64_t Reader::bytes_buffered() const
{
	return buffer.size();
}