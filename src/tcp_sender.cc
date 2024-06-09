#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
	return send_cnt_ - ack_cnt_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
	return retx_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
	if (wdsz_ > send_cnt_ - ack_cnt_) {
		if (is_fin_ || wdsz_ == 0) {
			// if the tx is fin or window size is 0, stop sending msg
			return;
		}
		
		if (!is_syn_) {
			auto msg = make_empty_message();
			msg.SYN = true;
			is_syn_ = true;
			send_cnt_ ++;
			transmit(msg);
		}

		uint64_t remaining = wdsz_ - (send_cnt_ - ack_cnt_);
		if (reader().bytes_buffered()) {
			auto data = reader().peek();
			uint64_t len = min(min(remaining, reader().bytes_buffered()), TCPConfig::MAX_PAYLOAD_SIZE);
			data = data.substr(0, len);
			TCPSenderMessage msg = {
				.seqno = Wrap32::wrap(send_cnt_, isn_),
				.SYN = !is_syn_,
				.payload = static_cast<string>(data),
				.FIN = false,
				.RST = input_.has_error()
			};
			input_.reader().pop(len);
			send_cnt_ += msg.sequence_length();
			transmit(msg);
		}

		if (!is_fin_ && wdsz_ > send_cnt_ - ack_cnt_ && reader().is_finished()) {
			auto msg = make_empty_message();
			msg.FIN = true;
			is_fin_ = true;
			send_cnt_ ++;
			transmit(msg);
		}
	}
}

TCPSenderMessage TCPSender::make_empty_message() const
{
	return { .seqno = Wrap32::wrap(send_cnt_, isn_), 
			 .SYN = false, 
			 .payload = {}, 
			 .FIN = false, 
			 .RST = input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
	if (msg.RST) {
		input_.set_error();
		return;
	}
	wdsz_ = msg.window_size;
	if (msg.ackno.has_value()) {
		uint64_t recv_ackno = msg.ackno.value().unwrap(isn_, ack_cnt_);
		if (recv_ackno > send_cnt_) {
			return;
		}
		if (recv_ackno > ack_cnt_) {
			ack_cnt_ = recv_ackno;
			cur_RTO_ms_ = initial_RTO_ms_;
			retx_cnt_ = 0;
			timer_ = 0;
		}
	}
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
	timer_ += ms_since_last_tick;
	if (timer_ >= cur_RTO_ms_) {
		while (!retx_queue_.empty()) {
			auto msg = retx_queue_.front();
			auto sendno = msg.seqno.unwrap(isn_, send_cnt_);
			if (sendno + msg.sequence_length() > ack_cnt_) {
				// if there are packets lost
				transmit(msg);
				if (wdsz_) {
					cur_RTO_ms_ = 2 * initial_RTO_ms_;
				}
				timer_ = 0;
				retx_cnt_ ++;
				break;
			} else {
				retx_queue_.pop();
			}
		}
	}
}
