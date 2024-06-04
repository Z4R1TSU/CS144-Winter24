#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
	uint32_t raw = (static_cast<uint32_t>(n) + zero_point.raw_value_) % (1UL << 32);
	return Wrap32 { raw };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
	uint64_t prefix = checkpoint & 0xFFFF'FFFF'0000'0000;
	uint64_t raw;
	if (this->raw_value_ >= zero_point.raw_value_) {
		raw = (static_cast<uint64_t>(this->raw_value_) - static_cast<uint64_t>(zero_point.raw_value_));
	} else {
		raw = (static_cast<uint64_t>(this->raw_value_) + (1UL << 32) - static_cast<uint64_t>(zero_point.raw_value_));
	}
	uint64_t probable_res2 = prefix + raw;
	uint64_t probable_res1 = probable_res2 > (1UL << 32) ? probable_res2 - (1UL << 32) : probable_res2;
	uint64_t probable_res3 = probable_res2 < (0xFFFF'FFFF'FFFF'FFFF) - (1UL << 32) ? probable_res2 + (1UL << 32) : probable_res2;

	if (checkpoint - probable_res1 < (1UL << 31)) {
		return probable_res1;
	} else if (probable_res3 - checkpoint < (1UL << 31)) {
		return probable_res3;
	} else {
		return probable_res2;
	}
}
