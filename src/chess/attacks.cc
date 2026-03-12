#include "chess/attacks.h"

#include <array>
#include <bit>
#include <span>

#include "chess/bitboard.h"
#include "chess/types.h"

namespace punch::attacks {

namespace internal {

Bitboard rook_attacks[0x19000] = {};
Bitboard bishop_attacks[0x1480] = {};
Magic rook_magics[64] = {};
Magic bishop_magics[64] = {};

}  // namespace internal

namespace {

template <PieceType Pt>
concept MagicSlider = (Pt == PieceType::kRook || Pt == PieceType::kBishop);

inline constexpr uint64_t kRookMagicMultipliers[64] = {
    0x8a80104000800020ULL, 0x140002000100040ULL,  0x2801880a0017001ULL,
    0x100081001000420ULL,  0x200020010080420ULL,  0x3001c0002010008ULL,
    0x8480008002000100ULL, 0x2080088004402900ULL, 0x800098204000ULL,
    0x2024401000200040ULL, 0x100802000801000ULL,  0x120800800801000ULL,
    0x208808088000400ULL,  0x2802200800400ULL,    0x2200800100020080ULL,
    0x801000060821100ULL,  0x80044006422000ULL,   0x100808020004000ULL,
    0x12108a0010204200ULL, 0x140848010000802ULL,  0x481828014002800ULL,
    0x8094004002004100ULL, 0x4010040010010802ULL, 0x20008806104ULL,
    0x100400080208000ULL,  0x2040002120081000ULL, 0x21200680100081ULL,
    0x20100080080080ULL,   0x2000a00200410ULL,    0x20080800400ULL,
    0x80088400100102ULL,   0x80004600042881ULL,   0x4040008040800020ULL,
    0x440003000200801ULL,  0x4200011004500ULL,    0x188020010100100ULL,
    0x14800401802800ULL,   0x2080040080800200ULL, 0x124080204001001ULL,
    0x200046502000484ULL,  0x480400080088020ULL,  0x1000422010034000ULL,
    0x30200100110040ULL,   0x100021010009ULL,     0x2002080100110004ULL,
    0x202008004008002ULL,  0x20020004010100ULL,   0x2048440040820001ULL,
    0x101002200408200ULL,  0x40802000401080ULL,   0x4008142004410100ULL,
    0x2060820c0120200ULL,  0x1001004080100ULL,    0x20c020080040080ULL,
    0x2935610830022400ULL, 0x44440041009200ULL,   0x280001040802101ULL,
    0x2100190040002085ULL, 0x80c0084100102001ULL, 0x4024081001000421ULL,
    0x20030a0244872ULL,    0x12001008414402ULL,   0x2006104900a0804ULL,
    0x1004081002402ULL};

inline constexpr uint64_t kBishopMagicMultipliers[64] = {
    0x40040844404084ULL,   0x2004208a004208ULL,   0x10190041080202ULL,
    0x108060845042010ULL,  0x581104180800210ULL,  0x2112080446200010ULL,
    0x1080820820060210ULL, 0x3c0808410220200ULL,  0x4050404440404ULL,
    0x21001420088ULL,      0x24d0080801082102ULL, 0x1020a0a020400ULL,
    0x40308200402ULL,      0x4011002100800ULL,    0x401484104104005ULL,
    0x801010402020200ULL,  0x400210c3880100ULL,   0x404022024108200ULL,
    0x810018200204102ULL,  0x4002801a02003ULL,    0x85040820080400ULL,
    0x810102c808880400ULL, 0xe900410884800ULL,    0x8002020480840102ULL,
    0x220200865090201ULL,  0x2010100a02021202ULL, 0x152048408022401ULL,
    0x20080002081110ULL,   0x4001001021004000ULL, 0x800040400a011002ULL,
    0xe4004081011002ULL,   0x1c004001012080ULL,   0x8004200962a00220ULL,
    0x8422100208500202ULL, 0x2000402200300c08ULL, 0x8646020080080080ULL,
    0x80020a0200100808ULL, 0x2010004880111000ULL, 0x623000a080011400ULL,
    0x42008c0340209202ULL, 0x209188240001000ULL,  0x400408a884001800ULL,
    0x110400a6080400ULL,   0x1840060a44020800ULL, 0x90080104000041ULL,
    0x201011000808101ULL,  0x1a2208080504f080ULL, 0x8012020600211212ULL,
    0x500861011240000ULL,  0x180806108200800ULL,  0x4000020e01040044ULL,
    0x300000261044000aULL, 0x802241102020002ULL,  0x20906061210001ULL,
    0x5a84841004010310ULL, 0x4010801011c04ULL,    0xa010109502200ULL,
    0x4a02012000ULL,       0x500201010098b028ULL, 0x8040002811040900ULL,
    0x28000010020204ULL,   0x6000020202d0240ULL,  0x8918844842082200ULL,
    0x4010011029020020ULL};

// Helper to prevent rays from wrapping around the board
constexpr bool IsSafeStep(Square s, Direction d) {
  File f = FileOf(s);
  Rank r = RankOf(s);
  if (d == Direction::kNorth) {
    return r < Rank::kRank8;
  }
  if (d == Direction::kSouth) {
    return r > Rank::kRank1;
  }
  if (d == Direction::kEast) {
    return f < File::kFileH;
  }
  if (d == Direction::kWest) {
    return f > File::kFileA;
  }
  if (d == Direction::kNorthEast) {
    return r < Rank::kRank8 && f < File::kFileH;
  }
  if (d == Direction::kNorthWest) {
    return r < Rank::kRank8 && f > File::kFileA;
  }
  if (d == Direction::kSouthEast) {
    return r > Rank::kRank1 && f < File::kFileH;
  }
  if (d == Direction::kSouthWest) {
    return r > Rank::kRank1 && f > File::kFileA;
  }
  return false;
}

// Generate attacks on the empty board, but excluding the outer edges.
// This is required to form the correct 'mask' for Magic Bitboards.
template <PieceType Pt>
  requires MagicSlider<Pt>
Bitboard MaskSliderAttacks(Square sq) {
  constexpr std::array<Direction, 4> dirs =
      (Pt == PieceType::kRook)
          ? std::array<Direction, 4>{Direction::kNorth, Direction::kSouth,
                                     Direction::kEast, Direction::kWest}
          : std::array<Direction, 4>{
                Direction::kNorthEast, Direction::kSouthEast,
                Direction::kSouthWest, Direction::kNorthWest};

  Bitboard mask = 0ULL;

  for (Direction d : dirs) {
    Square s = sq;
    while (IsSafeStep(s, d)) {
      s += d;
      // If the next step is NOT safe, it means 's' is on the outer edge.
      // We don't include outer edge squares in the Magic mask.
      if (!IsSafeStep(s, d)) {
        break;
      }
      SetBit(mask, s);
    }
  }
  return mask;
}

// Generate real attacks given a specific occupancy configuration
template <PieceType Pt>
  requires MagicSlider<Pt>
Bitboard SliderAttacksOnTheFly(Square sq, Bitboard occ) {
  constexpr std::array<Direction, 4> dirs =
      (Pt == PieceType::kRook)
          ? std::array<Direction, 4>{Direction::kNorth, Direction::kSouth,
                                     Direction::kEast, Direction::kWest}
          : std::array<Direction, 4>{
                Direction::kNorthEast, Direction::kSouthEast,
                Direction::kSouthWest, Direction::kNorthWest};

  Bitboard attacks = 0ULL;

  for (Direction d : dirs) {
    Square s = sq;
    while (IsSafeStep(s, d)) {
      s += d;
      attacks |= (1ULL << s);
      // Stop the ray if we hit a blocking piece
      if (occ & (1ULL << s)) {
        break;
      }
    }
  }
  return attacks;
}

// Maps an index (0 to 2^bits - 1) to a specific blocker configuration
Bitboard SetOccupancy(int index, int bits_in_mask, Bitboard attack_mask) {
  Bitboard occupancy = 0ULL;
  for (int i = 0; i < bits_in_mask; i++) {
    Square square = PopLsb(attack_mask);
    if (index & (1 << i)) {
      SetBit(occupancy, square);
    }
  }
  return occupancy;
}

// Initialize slider tables
template <PieceType Pt>
  requires MagicSlider<Pt>
void InitSlider(std::span<internal::Magic> magics, std::span<Bitboard> table,
                std::span<const uint64_t> multipliers) {
  int offset = 0;
  for (int i = 0; i < Square::kSquareNb; ++i) {
    Square sq = static_cast<Square>(i);
    Bitboard mask = MaskSliderAttacks<Pt>(sq);
    int bits = PopCount(mask);

    magics[sq].mask = mask;
    magics[sq].magic = multipliers[sq];
    magics[sq].shift = 64 - bits;
    magics[sq].attacks = table.data() + offset;

    int occupancy_indices = 1 << bits;
    for (int j = 0; j < occupancy_indices; ++j) {
      Bitboard occ = SetOccupancy(j, bits, mask);
      int magic_index =
          static_cast<int>((occ * multipliers[sq]) >> (64 - bits));
      magics[sq].attacks[magic_index] = SliderAttacksOnTheFly<Pt>(sq, occ);
    }
    offset += occupancy_indices;
  }
}

}  // namespace

void Initialize() {
  InitSlider<PieceType::kBishop>(internal::bishop_magics,
                                 internal::bishop_attacks,
                                 kBishopMagicMultipliers);
  InitSlider<PieceType::kRook>(internal::rook_magics, internal::rook_attacks,
                               kRookMagicMultipliers);
}

}  // namespace punch::attacks
