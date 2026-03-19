#ifndef PUNCH_TRANSPOSITION_TABLE_H_
#define PUNCH_TRANSPOSITION_TABLE_H_

#include <vector>

#include "chess/types.h"

namespace punch {

enum class Bound : uint8_t { kNone, kExact, kLowerBound, kUpperBound };

struct TtEntry {
  Key key;
  Move move;
  int16_t depth;
  int16_t score;
  Bound bound;
  uint8_t age;
};

inline Value ValueFromTt(Value value, int ply) {
  if (IsMateValue(value)) {
    return value > 0 ? value - ply : value + ply;
  }
  return value;
}

inline Value ValueToTt(Value value, int ply) {
  if (IsMateValue(value)) {
    return value > 0 ? value + ply : value - ply;
  }
  return value;
}

class TranspositionTable {
 public:
  TranspositionTable() = default;
  TranspositionTable(size_t mb);

  void Resize(size_t mb);
  void Clear();

  inline void NewSearch() { age_++; }

  inline TtEntry* Probe(Key key) { return table_.data() + (key & (size_ - 1)); }

  inline void Store(Key key, Move move, int depth, Value score, Bound bound,
                    int ply) {
    TtEntry* entry = Probe(key);

    bool is_old = (age_ - entry->age) > 2;

    if (entry->key != key || depth >= entry->depth || is_old) {
      entry->key = key;
      entry->move = move;
      entry->depth = static_cast<int16_t>(depth);
      entry->score = static_cast<int16_t>(ValueToTt(score, ply));
      entry->bound = bound;
      entry->age = age_;
    }
  }

  int Hashfull() const;
  inline size_t Size() const { return size_; }

 private:
  std::vector<TtEntry> table_;
  size_t size_ = 0;
  uint8_t age_ = 0;
};

}  // namespace punch

#endif  // PUNCH_TRANSPOSITION_TABLE_H_
