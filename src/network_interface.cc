#include <iostream>
#include <system_error>

#include "address.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "exception.hh"
#include "ipv4_datagram.hh"
#include "parser.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
    : name_( name )
    , port_( notnull( "OutputPort", move( port ) ) )
    , ethernet_address_( ethernet_address )
    , ip_address_( ip_address )
{
    cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
        << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
    const uint32_t ip = next_hop.ipv4_numeric();
    if (auto it = arp_map_.find(ip); it == arp_map_.end()) {
        if (boardcast_waitlist_.contains(ip)) {
            return;
        }
        ARPMessage arp_request = {
            .opcode = ARPMessage::OPCODE_REQUEST,
            .sender_ethernet_address = this->ethernet_address_,
            .sender_ip_address = this->ip_address_.ipv4_numeric(),
            .target_ethernet_address = ETHERNET_REQUEST_ADDRESS,
            .target_ip_address = ip
        };
        EthernetHeader eth_header = {
            .dst = ETHERNET_BROADCAST,
            .src = this->ethernet_address_,
            .type = EthernetHeader::TYPE_ARP
        };
        transmit( {
            .header = eth_header,
            .payload = serialize(arp_request)
        } );
        boardcast_waitlist_[ip].first.emplace_back(dgram);
    } else {
        EthernetHeader eth_header = {
            .dst = it->second.first,
            .src = this->ethernet_address_,
            .type = EthernetHeader::TYPE_IPv4
        };
        transmit( {
            .header = eth_header,
            .payload = serialize(dgram)
        } );
    }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
    const auto eth_header = frame.header;
    if (eth_header.dst != ETHERNET_BROADCAST && eth_header.dst != this->ethernet_address_) {
        return;
    }

    if (eth_header.type == EthernetHeader::TYPE_IPv4) {
        IPv4Datagram ip_dgram;
        if (parse(ip_dgram, frame.payload)) {
            datagrams_received_.push(ip_dgram);
        }
    } else if (eth_header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_msg;
        if (parse(arp_msg, frame.payload)) {
            const auto sender_ip = arp_msg.sender_ip_address;
            const auto sender_mac = arp_msg.sender_ethernet_address;
            arp_map_[sender_ip] = {sender_mac, 0};

            if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST && arp_msg.target_ip_address == this->ip_address_.ipv4_numeric()) {
                ARPMessage arp_reply = {
                    .opcode = ARPMessage::OPCODE_REPLY,
                    .sender_ethernet_address = this->ethernet_address_,
                    .sender_ip_address = this->ip_address_.ipv4_numeric(),
                    .target_ethernet_address = sender_mac,
                    .target_ip_address = sender_ip,
                };
                EthernetHeader arp_eth_header = {
                    .dst = sender_mac,
                    .src = this->ethernet_address_,
                    .type = EthernetHeader::TYPE_ARP
                };
                transmit( {
                    .header = arp_eth_header,
                    .payload = serialize(arp_reply)
                } );
            } else {
                if (boardcast_waitlist_.contains(sender_ip)) {
                    for (auto dgram : boardcast_waitlist_[sender_ip].first) {
                        EthernetHeader dgram_eth_header = {
                            .dst = sender_mac,
                            .src = this->ethernet_address_,
                            .type = EthernetHeader::TYPE_IPv4
                        };
                        transmit( {
                            .header = dgram_eth_header,
                            .payload = serialize(dgram)
                        } );
                    }
                    boardcast_waitlist_.erase(sender_ip);
                }
            }
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
    for (auto it = arp_map_.begin(); it != arp_map_.end(); ) {
        it->second.second += ms_since_last_tick;
        if (it->second.second >= ARP_MAP_TTL) {
            it = arp_map_.erase(it);
        } else {
            it = next(it);
        }
    }

    for (auto it = boardcast_waitlist_.begin(); it != boardcast_waitlist_.end(); ) {
        it->second.second += ms_since_last_tick;
        if (it->second.second >= ARP_RETX_PERIOD) {
            it = boardcast_waitlist_.erase(it);
        } else {
            it = next(it);
        }
    }
}
