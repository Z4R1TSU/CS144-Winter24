#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
	bool syn = message.SYN;
	bool fin = message.FIN;
	bool reset = message.RST;
	if (reset) {
		// TODO: reset, connection should be aborted
		return;
	}
	// void insert( uint64_t first_index, std::string data, bool is_last_substring );
	// the parameters should be translated from seqno to absolute seqno
	Wrap32 zero_point = 
	uint64_t checkpoint = writer().bytes_pushed();
	uint64_t abs_seqno = message.seqno.unwrap();
	this->reassembler().insert()
}

TCPReceiverMessage TCPReceiver::send() const
{
	// Your code here.
	return {};
}
