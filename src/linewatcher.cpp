#include "linewatcher.hpp"
#include <deque>
#include <iostream>
#include <iterator>
#include <termios.h>
#include <unistd.h>

bool Linewatcher::isEOF() { return isEOF_; }

static void printBuf(const std::deque<char> &buf) {
  std::flush(std::cout);
  for (auto c : buf) {
    std::cout << c;
  }
  std::flush(std::cout);
}
enum KEYS {
  ESC,
  ARW_UP,
  ARW_DOWN,
  ARW_LEFT,
  ARW_RIGHT,
};

static KEYS parseSeq() {
  char seq[2];
  if (read(STDIN_FILENO, &seq[0], 1) != 1) {
    return ESC;
  }
  if (read(STDIN_FILENO, &seq[1], 1) != 1) {
    return ESC;
  }
  if (seq[1] == 'A') {
    return ARW_UP;
  }
  if (seq[1] == 'B') {
    return ARW_DOWN;
  }
  if (seq[1] == 'C') {
    return ARW_RIGHT;
  }
  if (seq[1] == 'D') {
    return ARW_LEFT;
  }
  return ESC;
}
inline void print(std::string_view text) { std::cout << text << std::flush; }
inline void rePrint(std::string_view text) {
  std::cout << "\x1b[1K\r";
  std::cout << text;
  std::flush(std::cout);
}
void moveCursor(std::deque<char>::iterator &cursor,
                std::deque<char>::iterator begin,
                std::deque<char>::iterator end, bool toLeft) {
  if (toLeft) {
    if (cursor == begin) return;
    std::advance(cursor, -1);
    print("\x1b[D");
    return;
  }
  print("hell");
  if (cursor == end) return;
  std::advance(cursor, 1);
  print("\x1b[C");
}
std::string Linewatcher::getline(const std::string &prompt) {
  std::deque<char> buf{};
  std::deque<char> bufZero{};
  std::deque<char>::iterator cursor = buf.end();
  isEOF_ = false;
  rePrint(prompt);
  char c = '\0';
  while (true) {
    int nread = read(STDIN_FILENO, &c, 1);
    if (nread == 0) continue;
    bool reprint = false;
    if (c == '\n') {
      std::cout << std::endl;
      break;
    }
    if (c == 4) {
      isEOF_ = true;
      break;
    }
    switch (c) {
    case 127:
      if (!buf.empty()) {
        history_.resetIndex();
        buf.erase(cursor - 1);
        moveCursor(cursor, buf.end(), buf.begin(), false);
        print("\x1b 7");
        reprint = true;
      }
      break;
    case '\x1b': {
      KEYS key = parseSeq();
      if (key == ARW_UP) {
        if (history_.index() == -1) {
          bufZero = buf;
        }
        std::deque<char> histBuf = history_.prev();
        if (!histBuf.empty()) {
          buf = histBuf;
          cursor = buf.end();
        }
        reprint = true;
      }
      if (key == ARW_DOWN) {
        std::deque<char> histBuf = history_.next();
        if (histBuf.empty()) {
          buf = bufZero;
        } else {
          buf = histBuf;
        }
        cursor = buf.end();
        reprint = true;
      }
      if (key == ARW_LEFT) {
        moveCursor(cursor, buf.end(), buf.begin(), true);
      }
      if (key == ARW_RIGHT) {
        moveCursor(cursor, buf.end(), buf.begin(), false);
      }
      break;
    }
    default:
      cursor = buf.insert(cursor, c);
      moveCursor(cursor, buf.end(), buf.begin(), false);
      print("\x1b 7");
      reprint = true;
    }
    if (reprint) {
      rePrint(prompt);
      printBuf(buf);
      print("\x1b 8");
    }
  }
  std::string str{};
  if (!isEOF_) {
    str = {buf.begin(), buf.end()};
  }
  if (buf.size()) {
    history_.push(buf);
  }
  return str;
}
