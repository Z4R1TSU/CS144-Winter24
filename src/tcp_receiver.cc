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
	// the parameters should be translated from seqno to absolute seqno
	if (!isn_.has_value()) {
		if (!message.SYN) {
			return;
		}
		isn_.emplace(message.seqno);
	}
	Wrap32 zero_point = isn_.value();
	uint64_t checkpoint = writer().bytes_pushed() + static_cast<uint32_t>(message.SYN);
	uint64_t abs_seqno = message.seqno.unwrap(zero_point, checkpoint);
	uint64_t stream_index = abs_seqno + static_cast<uint64_t>(message.SYN) - 1;
	reassembler_.insert(stream_index, move(message.payload), message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
	uint16_t wdsz = static_cast<uint16_t>(min(writer().available_capacity(), static_cast<uint64_t>(UINT16_MAX)));
	bool reset = writer().has_error();
	if (isn_.has_value()) {
		Wrap32 ackno = Wrap32::wrap(writer().bytes_pushed() + static_cast<uint64_t>(writer().is_closed()), isn_.value()) + 1;
		return { ackno, wdsz, reset };
	} else {
		return { nullopt, wdsz, reset };
	}
}
