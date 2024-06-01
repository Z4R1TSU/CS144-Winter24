#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
    // Your code here.
    uint64_t wd_start = nxt_expected_idx_;
    uint64_t wd_end = wd_start + output_.writer().available_capacity();
    uint64_t cur_start = first_index;
    uint64_t cur_end = cur_start + data.size();

    // TODO
    cout<<"insert:"<<wd_start<<' '<<wd_end<<' '<<cur_start<<' '<<cur_end<<endl;
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
    // TODO
    cout<<"index:"<<start_idx<<' '<<end_idx<<" len:"<<len<<endl;
    
    buf_.insert({start_idx, end_idx, data.substr(start_idx - first_index, len)});
   
    std::vector<Interval> merged;
    auto it = buf_.begin();
    Interval current = *it;
    it ++;

    while (it != buf_.end()) {
        if (current.end >= it->start - 1) {
            current.end = std::max(current.end, it->end);
            current.data = current.data.substr(0, it->start - current.start) + it->data;
        } else {
            merged.push_back(current);
            current = *it;
        }
        it ++;
    }
    merged.push_back(current);

    buf_.clear();
    for (const auto& interval : merged) {
        buf_.insert(interval);
    }

    // TODO
    for (it=buf_.begin(); it!=buf_.end(); it++)
        {cout<<it->start<<' '<<it->end<<endl;}

    it = buf_.begin();
    while (it->start == nxt_expected_idx_) {
        output_.writer().push(it->data);
        nxt_expected_idx_ = it->end;
        it = buf_.erase(it);
    }
    // TODO
    cout << nxt_expected_idx_ << endl;
    if (nxt_expected_idx_ == eof_idx_) {
        output_.writer().close();
    }
}

uint64_t Reassembler::bytes_pending() const
{
    // Your code here.
    // return byte_pend_;
    uint64_t pendcnt = 0;
    for (const auto& interval : buf_) {
        pendcnt += interval.end - interval.start;
    }
    return pendcnt;
}
