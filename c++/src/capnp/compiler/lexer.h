// Copyright (c) 2013-2014 Sandstorm Development Group, Inc. and contributors
// Licensed under the MIT License:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#if defined(__GNUC__) && !defined(CAPNP_HEADER_WARNINGS)
#pragma GCC system_header
#endif

#include <capnp/compiler/lexer.capnp.h>
#include <kj/parse/common.h>
#include <kj/arena.h>
#include <kj/debug.h>
#include "error-reporter.h"

namespace capnp {
namespace compiler {

bool lex(kj::ArrayPtr<const char> input, LexedStatements::Builder result,
         ErrorReporter& errorReporter);
bool lex(kj::ArrayPtr<const char> input, LexedTokens::Builder result, ErrorReporter& errorReporter);
// Lex the given source code, placing the results in `result`.  Returns true if there
// were no errors, false if there were.  Even when errors are present, the file may have partial
// content which can be fed into later stages of parsing in order to find more errors.
//
// There are two versions, one that parses a list of statements, and one which just parses tokens
// that might form a part of one statement.  In other words, in the later case, the input should
// not contain semicolons or curly braces, unless they are in string literals of course.

class Lexer {
  // Advanced lexer interface.  This interface exposes the inner parsers so that you can embed them
  // into your own parsers.

public:
  Lexer(Orphanage orphanage, ErrorReporter& errorReporter);
  // `orphanage` is used to allocate Cap'n Proto message objects in the result.  `inputStart` is
  // a pointer to the beginning of the input, used to compute byte offsets.

  ~Lexer() noexcept(false);

  class ParserInput: public kj::parse::IteratorInput<char, const char*> {
    // Like IteratorInput<char, const char*> except that positions are measured as byte offsets
    // rather than pointers.

  public:
    ParserInput(const char* begin, const char* end, Lexer &lexer)
      : IteratorInput<char, const char*>(begin, end), begin(begin), lexer(lexer) {}
    explicit ParserInput(ParserInput& parent)
      : IteratorInput<char, const char*>(parent), begin(parent.begin), lexer(parent.lexer) {}

    inline uint32_t getBest() {
      return IteratorInput<char, const char*>::getBest() - begin;
    }
    inline uint32_t getPosition() {
      return IteratorInput<char, const char*>::getPosition() - begin;
    }             
    template <typename... Params>
    inline void setReport(Params&&... params) {
      lexer.report = kj::str(kj::fwd<Params>(params)...);
    }
    inline void setReport(ErrorReport report) {
      lexer.report = kj::mv(report);
    }
    inline void setReport(kj::String&& str) {
      auto report = ErrorReport(kj::mv(str));
      report.setLocation(getPosition());
      lexer.report = kj::mv(report);
    }
    inline void clearReport() {
      lexer.report = nullptr;
    }

  private:
    const char* begin;
    Lexer &lexer;
  };

  template <typename Output>
  using Parser = kj::parse::ParserRef<ParserInput, Output>;

  struct Parsers {
    Parser<kj::Tuple<>> emptySpace;
    Parser<Orphan<Token>> token;
    Parser<kj::Array<Orphan<Token>>> tokenSequence;
    Parser<Orphan<Statement>> statement;
    Parser<kj::Array<Orphan<Statement>>> statementSequence;
  };

  const Parsers& getParsers() { return parsers; }

  kj::Maybe<ErrorReport> takeErrorReport() { return kj::mv(report); }

private:
  Orphanage orphanage;
  kj::Arena arena;
  Parsers parsers;
  kj::Maybe<ErrorReport> report;
};

// -------------------------------------------------------------------
// error indicators

template <typename Output, bool succeed, typename... Params>
class IfError_ {
public:
  explicit constexpr IfError_(Params&&... params) : params(kj::fwd<Params>(params)...) {}
  template <typename T>
  inline kj::Maybe<Output> operator()(T &input) const {
    input.setReport(kj::apply(kj::str<Params...>, params));
    if (succeed) return kj::instance<Output>();
    return nullptr;
  }

private:
  kj::Tuple<Params...> params;
};

template <typename Output, typename... Params>
IfError_<Output, false, Params...> ifError(Params&&... params) {
  return IfError_<Output, false, Params...>(kj::fwd<Params>(params)...);
};

template <typename Output> auto noError() {
  return [](auto &input) -> kj::Maybe<Output>{
    input.clearReport();
    return kj::instance<Output>();
  };
}

template <typename SubParser>
class NoError_ {
  template <typename Input, typename Output = kj::parse::OutputType<SubParser, Input>>
  struct Impl;
public:
  explicit constexpr NoError_(SubParser&& subParser)
      : subParser(kj::fwd<SubParser>(subParser)){}

  template <typename Input>
  auto operator()(Input& input) const
  -> decltype(kj::apply(kj::instance<const SubParser&>(), input)) {
    input.clearReport();
    return kj::apply(subParser, input);
  }

private:
  SubParser subParser;
};

template <typename SubParser>
auto noError(SubParser&& subParser) {
  return NoError_<SubParser>(kj::fwd<SubParser>(subParser));
}

}  // namespace compiler
}  // namespace capnp
