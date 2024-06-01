#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
    uint64_t wd_start = nxt_expected_idx_;
    uint64_t wd_end = wd_start + output_.writer().available_capacity();
    uint64_t cur_start = first_index;
    uint64_t cur_end = cur_start + data.size();

    // set the eof index of this reassembling
    if (is_last_substring) {
        eof_idx_ = cur_end;
    }

    if (cur_start >= wd_end) {
        return;
    }

    uint64_t start_idx = max(wd_start, cur_start);
    uint64_t end_idx = min(wd_end, cur_end);
    if (start_idx >= end_idx) {
        if (nxt_expected_idx_ == eof_idx_) {
            output_.writer().close();
        }
        return;
    }
    uint64_t len = end_idx - start_idx;
    
    // insert the current data
    buf_.insert({start_idx, end_idx, data.substr(start_idx - first_index, len)});
   
    // handle the overlapping of intervals
    std::vector<Interval> merged;
    auto it = buf_.begin();
    Interval last = *it;
    it ++;

    while (it != buf_.end()) {
        if (it->start <= last.end) {
            if (last.end < it->end) {
                last.end = it->end;
                last.data = last.data.substr(0, it->start - last.start) + it->data;
            }
        } else {
            merged.push_back(last);
            last = *it;
        }
        it ++;
    }
    merged.push_back(last);

    buf_.clear();
    for (const auto& interval : merged) {
        buf_.insert(interval);
    }

    // push when it ready
    it = buf_.begin();
    while (it->start == nxt_expected_idx_) {
        output_.writer().push(it->data);
        nxt_expected_idx_ = it->end;
        it = buf_.erase(it);
    }

    // close when all bytes are pushed
    if (nxt_expected_idx_ == eof_idx_) {
        output_.writer().close();
    }
}

uint64_t Reassembler::bytes_pending() const
{
    uint64_t pendcnt = 0;
    for (const auto& interval : buf_) {
        pendcnt += interval.end - interval.start;
    }
    return pendcnt;
}
