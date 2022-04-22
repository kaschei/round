#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <random>
#include <string_view>
#include <vector>

#include <docopt/docopt.h>
#include <ftxui/component/captured_mouse.hpp>// for ftxui
#include <ftxui/component/component.hpp>// for Slider
#include <ftxui/component/screen_interactive.hpp>// for ScreenInteractive
#include <spdlog/spdlog.h>

#include <cstdlib>

// This file will be generated automatically when you run the CMake
// configuration step. It creates a namespace called `round`. You can modify
// the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>


using namespace ftxui;
using namespace std::literals;


class Rando {
 public:
  explicit Rando(size_t begin, size_t end)
    : gen(rd()),
      rng(begin, end - 1)
    {}

  size_t operator() () { return rng(gen); }

 private:
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<size_t> rng;
};



const size_t boardHeight = 6;
const size_t boardWidth = 6;

const size_t tileHeight = 5;
const size_t tileWidth = 10;

// Trying the box shapes

// 10 x 5 boxes
// Eight entrances at:
//            n1     n2
//           (3,0), (6,4)
// w1  (0,1)              (9,1)  e1
// w2  (0,3)              (9,3)  e2
//           (3,4), (6,4)
//            s1     s2

// Each entrance has an in-direction. We also have a z0 Entrance which
// is used for empty tiles.

// This gives four paths across the tile Each path is given as the
// start and end entrances, and a sequence of steps.
//
// Steps are given as the direction to move in.

// Twists will be calculated from the sequence Twists are named by the
// two exit directions from the pixel.  Normally there are only two
// exits from a cell, but a tile can also be empty (zz) or have two
// paths crossing (xx).

// Paths are repesented by the steps to take form start to finish
// (inclusive). There'll be one less step than the number of tiles in
// path).

// From can be reversed to go from finish back to start.

// To calculate the Twist of a tile, we need to combine two Steps.
// For the Twist of the first tile, we need the Step direction of the
// Entrance itself.


enum class Step : char { e, n, s, w };

enum class Twist {
   zz,
   en, es, ew, ns, nw, sw,
   xx
};

enum class Entrance { z0, e1, e2, n1, n2, s1, s2, w1, w2 };

struct Coord { int x {}; int y {}; };
struct Delta { int dx {}; int dy {}; };

// Basic step deltas
constexpr const Delta d_e { +1, 0 };
constexpr const Delta d_n { 0, -1 };
constexpr const Delta d_s { 0, +1 };
constexpr const Delta d_w { -1, 0 };
constexpr const Delta d_z { 0, 0 };


constexpr Coord& operator+=(Coord& pt, Delta v)
{
  pt.x += v.dx;
  pt.y += v.dy;
  return pt;
}


constexpr Coord operator+(Coord pt, Delta v)
{
  return pt += v;
}

constexpr Coord& operator-=(Coord& pt, Delta v)
{
  pt.x -= v.dx;
  pt.y -= v.dy;
  return pt;
}


constexpr Coord operator-(Coord pt, Delta v)
{
  return pt -= v;
}


constexpr Delta operator-(Coord a, Coord b)
{
  return Delta{ b.x - a.x, b.y - a.y};
}


constexpr Step rev(Step step)
{
  using enum Step;
  switch (step) {
    case e: return w;
    case n: return s;
    case s: return n;
    case w: return e;
  }
  return e; // arbitrary
}


constexpr Coord entranceLoc(Entrance e)
{
  // edge ordinates:
  const int x_w = 0;
  const int x_e = 9;
  const int y_n = 0;
  const int y_s = 4;

  // entrance ordinates:
  const int y_1 = 1;
  const int y_2 = 3;
  const int x_1 = 3;
  const int x_2 = 6;

  using enum Entrance;
  switch (e) {
    case z0: return Coord { 0, 0 };
    case e1: return Coord { x_e, y_1 };
    case e2: return Coord { x_e, y_2 };
    case n1: return Coord { x_1, y_n };
    case n2: return Coord { x_2, y_n };
    case s1: return Coord { x_1, y_s };
    case s2: return Coord { x_2, y_s };
    case w1: return Coord { x_w, y_1 };
    case w2: return Coord { x_w, y_2 };
  }
  return Coord{0,0};
}


constexpr Step entranceDir(Entrance entrance)
{
  using enum Entrance;
  using enum Step;
  switch (entrance) {
    case z0: return e; // arbitrary
    case e1: case e2: return w;
    case n1: case n2: return s;
    case s1: case s2: return n;
    case w1: case w2: return e;
  }
  return e;
}

constexpr Step exitDir(Entrance entrance)
{
  return rev(entranceDir(entrance));
}


constexpr Delta move(Step step)
{
  using enum Step;
  switch (step) {
    case e: return d_e;
    case n: return d_n;
    case s: return d_s;
    case w: return d_w;
  }
  return d_z;
}

constexpr Coord& operator+=(Coord& pt, Step step) { pt += move(step); return pt; }
constexpr Coord operator+(Coord pt, Step step) { return pt += step;}

constexpr Coord& operator-=(Coord& pt, Step step) { pt -= move(step); return pt; }
constexpr Coord operator-(Coord pt, Step step) { return pt -= step; }



using Path = std::vector<Step>;


struct TileTrack {
  Entrance start {};
  Entrance finish {};
  Path route;
};




struct TileMap {
   std::array<TileTrack, 4> tracks;
};

using TileBook = std::vector<TileMap>;



constexpr Twist makeTwist(Step in, Step out) // NOLINT(bugprone-easily-swappable-parameters)
{
  // Depends very much on enum Step values
  Step rin = rev(in);
  const size_t n = static_cast<size_t>(rin) * 4 + static_cast<size_t>(out);
  using enum Twist;
  constexpr std::array<Twist, 16> twister = {
    zz, en, es, ew,
    en, zz, ns, nw,
    es, ns, zz, sw,
    ew, nw, sw, zz
  };

  return twister.at(n);
}


// Convert a path twist into the graphic character of the map

std::string makeTwistPix(Twist t)
{
  using enum Twist;
  switch (t) {
    case zz: return " "s;
    case en: return "╰"s;
    case es: return "╭"s;
    case ew: return "─"s;
    case ns: return "│"s;
    case nw: return "╯"s;
    case sw: return "╮"s;
    case xx: return "┼"s;
  }
}




//   0123456789  //
//  0   |  |     //
//  1-        -  //
//  2            //
//  3-        -  //
//  4   |  |     //


  //     |  |
  //  ..........
  // -..........-
  //  ..........
  // -..........-
  //  ..........
  //     |  |


std::vector<TileMap> makeTileMaps() {
  using enum Step;
  using enum Entrance;

  //     |  |
  //  ..........
  // -..........-
  //  ..........
  // -..........-
  //  ..........
  //     |  |
  const TileMap t0 {
     TileTrack{ n1, n2, Path{}},
     TileTrack{ e1, e2, Path{}},
     TileTrack{ w1, w2, Path{}},
     TileTrack{ s1, e2, Path{}}
  };

  //     |  |
  //  ...1..222.
  // -33.11...22-
  //  .3..111...
  // -33.444+444-
  //  ...4..1...
  //     |  |
  const TileMap t1 {
     TileTrack{ n1, s2, Path{ s, e, s, e, e, s, s } },
     TileTrack{ n2, e1, Path{ e, e, s, e } },
     TileTrack{ w1, w2, Path{ e, s, s, w } },
     TileTrack{ s1, e2, Path{ n, e, e, e, e, e, e } }
  };

  //     |  |
  //  ...1..2...
  // -333+33+333-
  //  ...1..2...
  // -444+44+444-
  //  ...1..2...
  //     |  |
  const TileMap t2 {
     TileTrack{ n1, s1, Path{ s, s, s, s }},
     TileTrack{ n2, s2, Path{ s, s, s, s }},
     TileTrack{ w1, e1, Path{ e, e, e, e, e, e, e, e, e}},
     TileTrack{ w2, e2, Path{ e, e, e, e, e, e, e, e, e}}
  };


  //     |  |
  //  ...1..22..
  // -1111.44+44-
  //  ...444.22.
  // -3..43333+3-
  //  333+3.222.
  //     |  |
  const TileMap t3 {
     TileTrack{ n1, w1, Path{ s, w, w, w }},
     TileTrack{ n2, s2, Path{ e, s, s, e, s, s, w, w }},
     TileTrack{ w2, e2, Path{ s, e, e, e, e, n, e, e, e, e, e}},
     TileTrack{ s1, e1, Path{ n, n, e, e, n, e, e, e, e}},
  };

  //     |  |
  //  .22+22+2..
  // -22.3344222-
  //  ....3+3...
  // -11.4443.11-
  //  .11+11+11.
  //     |  |
  const TileMap t4 {
     TileTrack{ w2, e2, Path{ e, s, e, e, e, e, e, e, e, n, e }},
     TileTrack{ w1, e1, Path{ e, n, e, e, e, e, e, e, s, e, e }},
     TileTrack{ n1, s2, Path{ s, e, s, e, e, s, s }},
     TileTrack{ n2, s1, Path{ s, w, s, s, w, w, s }}
  };

  //     |  |
  //  ...3331...
  // -22221+1444-
  //  ...2133+3.
  // -111+1..433-
  //  ...2..44..
  //     |  |
  const TileMap t5 {
    TileTrack{ w2, n2, Path{e, e, e, e, n, n, e, e, n}},
    TileTrack{ w1, s1, Path{e, e, e, s, s, s}},
    TileTrack{ n1, e2, Path{e, e, s, s, e, e, e, s, e}},
    TileTrack{ s2, w1, Path{e, n, n, n, e, e, e}}
  };

  return std::vector<TileMap>{ t0, t1, t2, t3, t4, t5 };
};





class City {
 public:
  City(size_t width, size_t height);

  [[nodiscard]] bool is_vacant(size_t x, size_t y) const;
  [[nodiscard]] size_t at(size_t x, size_t y) const;
  [[nodiscard]] size_t& at(size_t x, size_t y);

 private:
  std::vector<std::vector<size_t>> tileCodes;
};


City::City(size_t width, size_t height)
  : tileCodes(height, std::vector<size_t>(width, 0))
{}


size_t
City::at(size_t x, size_t y) const
{
  return tileCodes.at(y).at(x);
}

size_t&
City::at(size_t x, size_t y)
{
  return tileCodes.at(y).at(x);
}

bool
City::is_vacant(size_t x, size_t y) const
{
  return at(x, y) == size_t{0};
}


class Hand {
 public:
  explicit Hand(size_t code = 0) : code_{code} {}

  void reset(size_t code) { *this = Hand{code}; }

  [[nodiscard]] size_t code() const {return code_;}

  [[nodiscard]]
  bool is_here(size_t x, size_t y) const {
    return onboard_ && x_ == x && y_ == y;
  }

  [[nodiscard]] bool is_onboard() const {return onboard_;}

  [[nodiscard]] size_t x() const { return x_; }
  [[nodiscard]] size_t y() const { return y_; }

  void move_left() {
    if (!onboard_) {
      return;
    }
    if (x_ == 0) {
      onboard_ = false;
      return;
    }
    --x_;
  }

  void move_right() {
    if (!onboard_) {
      onboard_ = true;
      return;
    }
    if (x_ == boardWidth - 1) {
      return;
    }
    ++x_;
  }

  void move_up() {
    if (!onboard_) {
      return;
    }
    if (y_ == 0) {
      return;
    }
    --y_;
  }

  void move_down() {
    if (!onboard_) {
      return;
    }
    if (y_ == boardHeight - 1) {
      return;
    }
    ++y_;
  }

 private:
  size_t code_ {};
  bool onboard_ {};
  size_t x_ {};
  size_t y_ {};
};



struct Crumb {
  Coord pt_;
  std::string shape_;
};


using Trail = std::vector<Crumb>;


class Stamp {
 public:
  Stamp(int width, int height) // NOLINT
    : width_(width), height_(height),
      pix_(static_cast<size_t>(width * height)) {}

  void blank() {
    std::fill(pix_.begin(), pix_.end(), " "s);
  }

  [[nodiscard]] size_t loc(int x, int y) const {
    int n = y * width_ + x;
    return static_cast<size_t>(n);
  }
  [[nodiscard]] size_t loc(Coord pt) const { return loc(pt.x, pt.y); }

  std::string& at(Coord pt) { return pix_.at(loc(pt)); }
  [[nodiscard]] const std::string& at(Coord pt) const { return pix_.at(loc(pt)); }


  void render(Screen& screen, Delta offset) {
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        screen.at(x + offset.dx, y + offset.dy) = pix_.at(loc(x,y));
      }
    }
  }

  void render_inverted(Screen& screen, Delta offset) {
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        Pixel& p = screen.PixelAt(x + offset.dx, y + offset.dy);
        p.character = pix_.at(loc(x,y));
        p.inverted = true;
      }
    }
  }

 private:
  int width_;
  int height_;
  std::vector<std::string> pix_;
};


void
AddTrackStamp(Stamp& s, const TileTrack& track)
{
  Coord pt = entranceLoc(track.start);
  Step din = entranceDir(track.start);
  for (Step dout : track.route) {
    const Twist t = makeTwist(din, dout);
    s.at(pt) = makeTwistPix(t);
    pt += dout;
    din = dout;
  }
  const Step dout = exitDir(track.finish);
  const Twist t = makeTwist(din, dout);

  std::string& ch = s.at(pt);
  const bool is_empty = (ch == " "s);
  ch = is_empty ? makeTwistPix(t) : makeTwistPix(Twist::xx);
}



// class TileNode : public Node {
//  public:
//   TileNode(const TileMap& m);

//  private:
// };


Stamp
MakeTileStamp(const TileMap& m, size_t code)
{
  Stamp s(tileWidth, tileHeight);
  s.blank(); // opaque stamp
  for (const TileTrack& t : m.tracks) {
    AddTrackStamp(s, t);
  }
  s.at(Coord{0,0}) = std::to_string(code);
  return s;
}


// const TileMap&
// LuckyTile(Rando& r)
// {
//   const auto i = r();
//   return gTileBook.at(i);
// }

// Element
// MakeLuckyTile(Rando& r)
// {
//   const auto i = r();
//   Element tile = MakeTile(gTileBook.at(i));
//   if (i == gTileBook.size() - 1) {
//     tile = tile | inverted;
//   }
//   return tile;
//   //return MakeTile(LuckyTile(r));
// }

class BoardNode : public ftxui::Node {
 public:
  BoardNode(const TileBook& book, const City& city, const Hand& n)
    : book_(&book), city_(&city), hand_(&n) {}

  void ComputeRequirement() override {
    requirement_.min_x = boardWidth * tileWidth;
    requirement_.min_y = boardHeight * tileHeight;
  }

  void RenderTiles(Screen& screen) {
    const int x = box_.x_min;
    const int y = box_.y_min;
    for (size_t i = 0; i < boardWidth; ++i) {
      for (size_t j = 0; j < boardHeight; ++j) {
        const size_t tileCode = city_->at(i, j);
        if (tileCode == 0) {
          continue;
        }
        const Delta offset { static_cast<int>(i * tileWidth) + x, static_cast<int>(j * tileHeight) + y };
        Stamp s = MakeTileStamp(book_->at(tileCode), tileCode);
        s.render(screen, offset);
      }
    }
  }

  void RenderHand(Screen& screen) {
    if (hand_->is_onboard()) {
      const int x = box_.x_min;
      const int y = box_.y_min;
      const size_t tileCode = hand_->code();
      const size_t i = hand_->x();
      const size_t j = hand_->y();
      const Delta offset { static_cast<int>(i * tileWidth) + x, static_cast<int>(j * tileHeight) + y };
      Stamp s = MakeTileStamp(book_->at(tileCode), tileCode);
      s.render_inverted(screen, offset);
    }
  }

  void Render(Screen& screen) override {
    RenderTiles(screen);
    RenderHand(screen);
  }

 private:
  const TileBook* book_;
  const City* city_;
  const Hand* hand_;
};


class HandNode : public ftxui::Node {
 public:
  HandNode(const TileBook& book, const Hand& n)
    : book_(&book), hand_(&n) {}

  void ComputeRequirement() override {
    requirement_.min_x = tileWidth;
    requirement_.min_y = tileHeight;
  }

  void Render(Screen& screen) override {
    if (not hand_->is_onboard()) {
      const int x = box_.x_min;
      const int y = box_.y_min;
      const size_t tileCode = hand_->code();
      const Delta offset { x, y };
      Stamp s = MakeTileStamp(book_->at(tileCode), tileCode);
      s.render(screen, offset);
    }
  }

 private:
  const TileBook* book_;
  const Hand* hand_;
};


Element
MakeBoard(const TileBook& book, const City& city, const Hand& n)
{
  return std::make_shared<BoardNode>(book, city, n);
}

Element
MakeHandElem(const TileBook& book, const Hand& n)
{
  return std::make_shared<HandNode>(book, n);
}



void round_game()
{
  auto screen = ftxui::ScreenInteractive::TerminalOutput();
  const auto book = makeTileMaps();

  auto dice = Rando(1, book.size());
  auto city = City(boardWidth, boardHeight);
  auto hand = Hand{};
  std::string keys;

  hand.reset(dice());

  auto component = Renderer([&] {
    auto board = MakeBoard(book, city, hand) | border;
    auto handy = MakeHandElem(book, hand) | border;
    // auto grid = gridbox(board) | border;
    // auto handElem = hand.is_onboard()
    //   ? MakeTile(gTileBook.at(0)) | border
    //   : MakeTile(gTileBook.at(hand.code())) | inverted | border;
    auto next = vbox({
        handy,
        text(keys) | border,
        filler()
      });
    auto page = hbox({next, board});
    return page;
  });

  auto c2 = CatchEvent(component, [&](Event event){ // NOLINT
    keys = event.character();
    if (event == Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == Event::ArrowLeft) {
      hand.move_left();
      return true;
    }
    if (event == Event::ArrowRight) {
      hand.move_right();
      return true;
    }
    if (event == Event::ArrowUp) {
      hand.move_up();
      return true;
    }
    if (event == Event::ArrowDown) {
      hand.move_down();
      return true;
    }
    if (event == Event::Return || event == Event::Character(' ')) {
      if (hand.is_onboard() && city.is_vacant(hand.x(), hand.y())) {
        city.at(hand.x(), hand.y()) = hand.code();
        hand.reset(dice());
      }
      return true;
    }
    if (event == Event::Character('n') || event == Event::Character('?')) {
      hand.reset(dice());
      return true;
    }
    return false;
  });

  screen.Loop(c2);
  // screen.Print();
}




int main(int argc, const char **argv)  // NOLINT(bugprone-exception-escape)
{
  static constexpr auto USAGE =
    R"(round
Try to get a round.

Usage:
  round [-n]
  round (-h | --help)
  round --version


Options:
  -h --help     Show this screen.
  --version     Show version.
)";

  std::map<std::string, docopt::value> args = docopt::docopt(
    USAGE,
    { std::next(argv), std::next(argv, argc) },
    true,// show help if requested
    fmt::format("{} {}",
                round::cmake::project_name,
                round::cmake::project_version));// version string, acquired
  // from config.hpp via CMake

  if (args["-n"].asBool()) {
    std::cout << "n\n";
  }

  round_game();
}
