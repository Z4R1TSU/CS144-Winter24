#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <iostream>

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
	while ((wdsz_ == 0 ? 1 : wdsz_) > sequence_numbers_in_flight()) {
		if (is_fin_) {
			break;
		}

		auto msg = make_empty_message();
		if (!is_syn_) {
			msg.SYN = true;
			is_syn_ = true;
		}

		uint64_t remaining = (wdsz_ == 0 ? 1 : wdsz_) - sequence_numbers_in_flight();
		uint64_t len = min(TCPConfig::MAX_PAYLOAD_SIZE, remaining - msg.sequence_length());
		// TODO
		cout << "remain: " << remaining << " len: " << len << " msglen: " << msg.sequence_length() << endl;
		auto&& data = msg.payload;
		while (reader().bytes_buffered() && data.size() < len) {
			auto cur_data = reader().peek();
			cur_data = cur_data.substr(0, len - data.size());
			data += cur_data;
			input_.reader().pop(data.size());
		}

		if (!is_fin_ && remaining > msg.sequence_length() && reader().is_finished()) {
			msg.FIN = true;
			is_fin_ = true;
		}

		if (msg.sequence_length() == 0) {
			break;
		}

		// TODO
		cout << "msg len: " << msg.sequence_length() << endl;
		transmit(msg);
		if (!is_timer_on_) {
			is_timer_on_ = true;
			timer_ = 0;
		}
		send_cnt_ += msg.sequence_length();
		retx_queue_.emplace(msg);
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
	wdsz_ = msg.window_size;
	if (msg.RST) {
		input_.set_error();
		return;
	}
	if (msg.ackno.has_value()) {
		const uint64_t recv_ackno = msg.ackno.value().unwrap(isn_, ack_cnt_);
		if (recv_ackno > send_cnt_) {
			return;
		}
		while (!retx_queue_.empty()) {
			auto retx_msg = retx_queue_.front();
			if (recv_ackno < ack_cnt_ + retx_msg.sequence_length()) {
				break;
			}
			ack_cnt_ += retx_msg.sequence_length();
			retx_queue_.pop();
			retx_cnt_ = 0;
			cur_RTO_ms_ = initial_RTO_ms_;
			timer_ = 0;
			if (retx_queue_.empty()) {
				is_timer_on_ = false;
			}
		}
	}
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
	if (is_timer_on_) {
		timer_ += ms_since_last_tick;
	}
	if (timer_ >= cur_RTO_ms_) {
		while (!retx_queue_.empty()) {
			auto msg = retx_queue_.front();
			auto sendno = msg.seqno.unwrap(isn_, send_cnt_);
			if (sendno + msg.sequence_length() > ack_cnt_) {
				// if there are packets lost
				transmit(msg);
				if (wdsz_) {
					retx_cnt_ ++;
					cur_RTO_ms_ *= 2;
				}
				timer_ = 0;
				break;
			} else {
				retx_queue_.pop();
			}
		}
	}
}
