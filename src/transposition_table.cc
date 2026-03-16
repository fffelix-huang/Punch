#include "transposition_table.h"

#include <algorithm>

#include "chess/types.h"

namespace punch {

TranspositionTable::TranspositionTable(size_t mb) { Resize(mb); }

void TranspositionTable::Resize(size_t mb) {
  size_t bytes = mb * 1024 * 1024;
  size_t count = bytes / sizeof(TtEntry);

  size_t new_size = 1;
  while (new_size * 2 <= count) {
    new_size *= 2;
  }

  if (new_size != size_) {
    size_ = new_size;
    table_.assign(size_, TtEntry{});
  }
}

void TranspositionTable::Clear() { std::ranges::fill(table_, TtEntry{}); }

int TranspositionTable::Hashfull() const {
  if (size_ == 0) {
    return 0;
  }

  int occupied = 0;
  size_t sample_size = std::min<size_t>(size_, 1000);

  for (size_t i = 0; i < sample_size; ++i) {
    occupied += (table_[i].key != Key(0) && table_[i].age == age_);
  }

  return (occupied * 1000) / sample_size;
}

}  // namespace punch
