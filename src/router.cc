#include "router.hh"
#include "address.hh"
#include "network_interface.hh"

#include <iostream>
#include <limits>
#include <memory>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
        << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
        << " on interface " << interface_num << "\n";

    router_map_.emplace_back(route_prefix, prefix_length, next_hop, interface_num);
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
    for (const auto& interface: _interfaces) {
        auto dgrams = interface->datagrams_received();
        while (!dgrams.empty()) {
            auto dgram = dgrams.front();
            dgrams.pop();
            auto dst_ip = dgram.header.dst;
            if (dgram.header.ttl -- <= 1) {
                continue;
            }
            // given the destination ip address, get the next interface through _interfaces
            auto cur_best_match = router_map_.end();
            for (auto it = router_map_.begin(); it != router_map_.end(); it = next(it)) {
                auto netmask = 0xFFFF'FFFF << (32 - it->netmask);
                auto cur_net_addr = it->ipv4 & netmask;
                auto dst_net_addr = dst_ip & netmask;
                if (cur_net_addr == dst_net_addr && it->netmask > cur_best_match->netmask) {
                    cur_best_match = it;
                }
            }
            if (cur_best_match == router_map_.end()) {
                continue;
            }
            if (cur_best_match->next_hop.has_value()) {
                _interfaces[cur_best_match->interface_idx]->send_datagram(dgram, cur_best_match->next_hop.value());
            } else {
                _interfaces[cur_best_match->interface_idx]->send_datagram(dgram, Address::from_ipv4_numeric(dst_ip));
            }
        }
    }
}
