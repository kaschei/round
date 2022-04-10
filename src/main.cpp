#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <random>
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

const size_t tileHeight = 6;
//const size_t tileWidth = 10;

using TileText = std::array<const char*, tileHeight>;

constexpr const TileText blank {
  R"(          )",
  R"(          )",
  R"(          )",
  R"(          )",
  R"(          )",
  R"(         .)" };

constexpr const TileText t1 {
  R"(   |  |   )",
  R"(_   \  \__)",
  R"( \   \    )",
  R"(_/   _|___)",
  R"(    / |   )",
  R"(   |  |  .)", };

const TileText t2 {
  R"(   |  |   )",
  R"(___|__|___)",
  R"(   |  |   )",
  R"(___|__|___)",
  R"(   |  |   )",
  R"(   |  |  .)" };

const TileText t3 {
  R"(   |  |   )",
  R"(__/  _|___)",
  R"(    /  \  )",
  R"(___|____|_)",
  R"(   |   /  )",
  R"(   |  |  .)" };

const TileText t4 {
  R"(  _|__|_  )",
  R"(_/ |  | \_)",
  R"(    \/    )",
  R"(_   /\   _)",
  R"( \_|__|_/ )",
  R"(   |  |  .)" };

const TileText t5 {
  R"(   |  |   )",
  R"(___|_  \__)",
  R"(   | \    )",
  R"(_  | _|___)",
  R"( \_|/ |   )",
  R"(   |  |  .)" };

const TileText t6 {
  R"(   |  |   )",
  R"(_   \  \__)",
  R"( \   \__  )",
  R"(__|__   \_)",
  R"(  |  \    )",
  R"(   \  |  .)" };

const TileText t7 {
  R"(   |  |   )",
  R"(__  \/   _)",
  R"(  \_/\  / )",
  R"(__    \_|_)",
  R"(  \    /  )",
  R"(   |  |  .)" };

const TileText t8 {
  R"(   |  |   )",
  R"(__/  / ___)",
  R"(    / /   )",
  R"(__  \/   _)",
  R"(  \    /  )",
  R"(   |  |  .)" };

const TileText t9 {
  R"(   |  |   )",
  R"(__  \/ ___)",
  R"(  \   /   )",
  R"(___|_/ ___)",
  R"(   |  /   )",
  R"(   |  |  .)" };

const TileText t10 {
  R"(   |  |   )",
  R"(___|__/  _)",
  R"(   |    / )",
  R"(__  \   \_)",
  R"(  \  \    )",
  R"(   |  |  .)" };

const TileText t11 {
  R"(   |  |   )",
  R"(___|_  \__)",
  R"(   | \    )",
  R"(___|__|___)",
  R"(   |  |   )",
  R"(   |  |  .)" };

const TileText t12 {
  R"(   |  |   )",
  R"(_  \   \__)",
  R"( \_/ ___  )",
  R"(____/   \_)",
  R"(    __    )",
  R"(   /  \  .)" };


const TileText t13 {
  R"(* _|__|_ *)",
  R"(_/ |  | \_)",
  R"(    \/    )",
  R"(_   /\   _)",
  R"( \_|__|_/ )",
  R"(*  |  |  *)" };

const std::array<TileText, 14> gTileBook { blank, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13 };



Element
MakeTile(const TileText& tt)
{
  Elements tileLines;
  std::transform(tt.begin(), tt.end(), std::back_inserter(tileLines),
                 [](const char* t){ return text(t); } );
  return vbox(tileLines);
}


const TileText&
LuckyTile(Rando& r)
{
  const auto i = r();
  return gTileBook.at(i);
}

Element
MakeLuckyTile(Rando& r)
{
  const auto i = r();
  Element tile = MakeTile(gTileBook.at(i));
  if (i == gTileBook.size() - 1) {
    tile = tile | inverted;
  }
  return tile;
  //return MakeTile(LuckyTile(r));
}


std::vector<Elements>
MakeBoard(Rando& r)
{
  std::vector<Elements> board;
  for (size_t y = 0; y < boardHeight ; ++y) {
    Elements row;
    for (size_t x = 0; x < boardWidth; ++x) {
      row.push_back(MakeLuckyTile(r));
    }
    board.push_back(row);
  }
  return board;
}



void round_game()
{
  //auto screen = ftxui::ScreenInteractive::TerminalOutput();
  //auto b1 = Button(t1, screen.ExitLoopClosure());
  // Component b2 = Button(&t2, screen.ExitLoopClosure());
  // Component b3 = Button(&t3, screen.ExitLoopClosure());
  // Component b4 = Button(&t4, screen.ExitLoopClosure());
  // auto document = txui::gridbox({
  //     { b1, b2 },
  //     { b3, b4 }
  //   });


  auto dice = Rando(1, gTileBook.size());
  auto board = MakeBoard(dice);
  auto grid = gridbox(board) | border;

  auto next = vbox({
      MakeLuckyTile(dice) | inverted | border,
      filler()
    });
  auto page = hbox({next, grid});

  auto screen = Screen::Create(
    Dimension::Fit(page),
    Dimension::Fit(page));

  Render(screen, page);
  //screen.Loop(doc);
  screen.Print();
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
