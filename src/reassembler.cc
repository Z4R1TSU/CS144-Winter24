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
        eof_idx_ = min(wd_end, cur_end);
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
    
    buf_.insert({start_idx, end_idx, data.substr(start_idx - first_index, len)});
   
    auto it = buf_.begin();
    uint64_t last_st = it->start;
    uint64_t last_en = it->end;
    string last_data = it->data;
    
    it ++;

    while (it != buf_.end()) {
        if (it->start > last_en + 1) {
            // 将上一个区间插入新的集合
            buf_.insert({last_st, last_en, last_data});
            last_st = it->start;
            last_en = it->end;
            last_data = it->data;
        } else {
            // 合并当前区间
            last_en = std::max(last_en, it->end);
            last_data += it->data.substr(0, it->end - it->start);
        }
        it = buf_.erase(it);
    }

    // 插入最后一个区间
    buf_.insert({last_st, last_en, last_data});

    it = buf_.begin();
    if (it->start == nxt_expected_idx_) {
        output_.writer().push(it->data);
        nxt_expected_idx_ = it->end;
        it = buf_.erase(it);
    }
    cout << nxt_expected_idx_ << endl;
    if (nxt_expected_idx_ == eof_idx_) {
        output_.writer().close();
    }

    // uint64_t last_st, last_en;
    // for (auto it = buf_.begin(); it != buf_.end(); ) {
    //     if (it == buf_.begin()) {
    //         last_st = it->start; last_en = it->end;
    //         it ++;
    //         continue;
    //     }
    //     if (it->start >= last_en) {
    //         last_st = it->start;
    //         last_en = it->end;
    //         it ++;
    //     } else {
    //         uint64_t cur_st = it->start, cur_en = it->end;
    //         string cur_data = it->data;
    //         it --;
    //         it->end = cur_end;
    //         it->data += cur_data.substr(last_en - cur_st, cur_en - last_en));
    //         it ++;
    //         it = buf_.erase(it);
    //         last_en = cur_en;
    //     }
    // } 
}

uint64_t Reassembler::bytes_pending() const
{
    // Your code here.
    // return byte_pend_;
    uint64_t pendcnt = 0;
    for (const auto& interval : buf_) {
        pendcnt += interval.end - interval.start + 1;
    }
    return pendcnt;
}
