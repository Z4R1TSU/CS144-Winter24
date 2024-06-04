#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
	if (writer().has_error()) {
		return;
	}
	if (message.RST) {
		reader().set_error();
		return;
	}
	// void insert( uint64_t first_index, std::string data, bool is_last_substring );
	// the parameters should be translated from seqno to absolute seqno
	if (!this->isn_.has_value()) {
		isn_.emplace(message.seqno + static_cast<uint32_t>(message.SYN));
	}
	Wrap32 zero_point = isn_.value();
	uint64_t checkpoint = writer().bytes_pushed();
	uint64_t abs_seqno = message.seqno.unwrap(zero_point, checkpoint);
	uint64_t stream_index = abs_seqno - static_cast<uint64_t>(message.SYN);
	reassembler_.insert(stream_index, move(message.payload), message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
	uint16_t wdsz = static_cast<uint16_t>(min(writer().available_capacity(), static_cast<uint64_t>(UINT16_MAX)));
	bool reset = writer().has_error();
	if (isn_.has_value()) {
		return { isn_.value(), wdsz, reset };
	} else {
		return { nullopt, wdsz, reset };
	}
}
