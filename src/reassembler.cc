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
   
    auto it = buf_.begin();
    auto next_it = std::next(it);
    uint64_t last_st = it->start;
    uint64_t last_en = it->end;
    std::string last_data = it->data;

    buf_.erase(it);
    
    while (next_it != buf_.end()) {
        if (next_it->start > last_en + 1) {
            // 将上一个区间插入新的集合
            buf_.insert({last_st, last_en, last_data});
            last_st = next_it->start;
            last_en = next_it->end;
            last_data = next_it->data;
        } else { 
            // 合并当前区间
            last_en = max(last_en, next_it->end);
            last_data += next_it->data;
        }
        next_it = buf_.erase(next_it);
    }

    // 插入最后一个区间
    buf_.insert({last_st, last_en, last_data});

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
