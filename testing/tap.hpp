#pragma once

#include <nall/function.hpp>
#include <nall/string.hpp>
#include <nall/vector.hpp>

namespace nall::tap {

enum class result : uint { unknown, pass, fail, skip, todo };

struct test_result {
  result result;
  string description;
  string comment;
};

class emitter {
public:
  function<void (string)> out;

  auto version() -> void {
    // TAP v12 is implicit.
  }

  auto plan(int num_tests) -> void {
    out({"1..", num_tests, "\n"});
  }

  auto test(int n, result result, string description, string comment) {
    string directive = "", testno = "";
    if (n != 0) testno = {n, " "};
    switch(result) {
    case result::skip:
      directive = {" # SKIP ", comment};
    case result::pass:
      out({"ok ", testno, description, directive, "\n"});
      break;
    case result::todo:
      directive = {" # TODO ", comment};
    case result::fail:
      out({"not ok ", testno, description, directive, "\n"});
      break;
    }
  }

  auto diagnostic(string diagnostic) -> void {
    out({"# ", diagnostic});
  }

  auto pass(int n, string description) -> void {
    test(n, result::pass, description, "");
  }

  auto fail(int n, string description) -> void {
    test(n, result::fail, description, "");
  }

  auto skip(int n, string description, string comment) -> void {
    test(n, result::skip, description, comment);
  }

  auto todo(int n, string description, string comment) -> void {
    test(n, result::todo, description, comment);
  }

  auto bail(string reason) {
    out({"Bail out! ", reason, "\n");
  }
}

class parser {
public:
  int version = 12;
  int num_tests = 0;
  bool bailed = false;
  string bail_out_reason;
  vector<test_result> results;

  auto in(string text) -> void {
    _buffer.append(text);
    while(true) {
      auto lf = _buffer.find("\n");
      if (lf == nothing) return;
      auto line = _buffer.slice(0, *lf);
      _buffer = _buffer.slice(*lf + 1);
      parse(line);
    }
  }

  auto parse(string line) -> void {
    // version (note: current TAP shouldn't do this)
    if (line.ibeginsWith("TAP version ")) {
      version = line.slice(12).integer();
      return;
    }

    // test plan
    if (line.ibeginsWith("1..")) {
      num_tests = line.slice(3).integer();
      results.reserve(num_tests);
      return;
    }

    // diagnostic line
    if (line.ibeginsWith("#")) {
      return;
    }

    // early bail
    if (line.ibeginsWith("Bail out!")) {
      bailed = true;
      bail_out_reason = line.slice(9).stripLeft();
      return;
    }

    // test line
    result r;
    if (line.ibeginsWith("ok")) {
      r = result::pass;
      line = line.slice(2).stripLeft();
    } else if (line.ibeginsWith("not ok")) {
      r = result::fail;
      line = line.slice(6).stripLeft();
    } else {
      // unknown line
      return;
    }

    // (maybe) test number
    int n = 0;
    if (line.size() > 0 && line[0] >= '0' && line[0] <= '9') {
      n = line.integer();
      auto ws = line.find(" ");
      if (ws == nothing) {
        line = "";
      } else {
        line = line.slice(*ws + 1).stripLeft();
      }
    } else {
      n = ++_counter;
    }

    // (maybe) description
    string description;
    if (line.size() > 0) {
      auto hash = line.find("#");
      if (hash == nothing) {
        description = line;
        line = "";
      } else {
        description = line.slice(0, *hash).stripRight();
        line = line.slice(*hash)
      }
    }

    // (maybe) directive
    string comment;
    if (line.size() > 0 && line[0] == '#') {
      line = line.slice(1).trimLeft();
      if (line.ibeginsWith("TODO")) {
        r = result::todo;
      } else if (line.ibeginsWith("SKIP")) {
        r = result::skip;
      } else {
        // unknown directive
      }
      auto ws = line.find(" ");
      if (ws != nothing) {
        comment = line.slice(*ws + 1).stripLeft();
      }
    }

    if (results.size() < n + 1) {
      results.resize(n + 1);
    }

    results[n].result = r;
    results[n].description = description;
    results[n].comment = comment;
  }

private:
  string _buffer;
  int _counter = 0;
}

}
