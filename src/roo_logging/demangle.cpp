// Copyright (c) 2024, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Satoru Takabayashi
//
// For reference check out:
// http://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling
//
// Note that we only have partial C++0x support yet.

#include "demangle.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

// #include "utilities.h"

#if defined(HAVE___CXA_DEMANGLE)
#include <cxxabi.h>
#endif

#if defined(GLOG_OS_WINDOWS)
#include <dbghelp.h>
#endif

namespace roo_logging {

#if !defined(GLOG_OS_WINDOWS) && !defined(HAVE___CXA_DEMANGLE)
namespace {
struct AbbrevPair {
  const char* const abbrev;
  const char* const real_name;
};

// List of operators from Itanium C++ ABI.
const AbbrevPair kOperatorList[] = {
    {"nw", "new"},    {"na", "new[]"},    {"dl", "delete"}, {"da", "delete[]"},
    {"ps", "+"},      {"ng", "-"},        {"ad", "&"},      {"de", "*"},
    {"co", "~"},      {"pl", "+"},        {"mi", "-"},      {"ml", "*"},
    {"dv", "/"},      {"rm", "%"},        {"an", "&"},      {"or", "|"},
    {"eo", "^"},      {"aS", "="},        {"pL", "+="},     {"mI", "-="},
    {"mL", "*="},     {"dV", "/="},       {"rM", "%="},     {"aN", "&="},
    {"oR", "|="},     {"eO", "^="},       {"ls", "<<"},     {"rs", ">>"},
    {"lS", "<<="},    {"rS", ">>="},      {"eq", "=="},     {"ne", "!="},
    {"lt", "<"},      {"gt", ">"},        {"le", "<="},     {"ge", ">="},
    {"nt", "!"},      {"aa", "&&"},       {"oo", "||"},     {"pp", "++"},
    {"mm", "--"},     {"cm", ","},        {"pm", "->*"},    {"pt", "->"},
    {"cl", "()"},     {"ix", "[]"},       {"qu", "?"},      {"st", "sizeof"},
    {"sz", "sizeof"}, {nullptr, nullptr},
};

// List of builtin types from Itanium C++ ABI.
const AbbrevPair kBuiltinTypeList[] = {
    {"v", "void"},        {"w", "wchar_t"},
    {"b", "bool"},        {"c", "char"},
    {"a", "signed char"}, {"h", "unsigned char"},
    {"s", "short"},       {"t", "unsigned short"},
    {"i", "int"},         {"j", "unsigned int"},
    {"l", "long"},        {"m", "unsigned long"},
    {"x", "long long"},   {"y", "unsigned long long"},
    {"n", "__int128"},    {"o", "unsigned __int128"},
    {"f", "float"},       {"d", "double"},
    {"e", "long double"}, {"g", "__float128"},
    {"z", "ellipsis"},    {"Dn", "decltype(nullptr)"},
    {nullptr, nullptr}};

// List of substitutions Itanium C++ ABI.
const AbbrevPair kSubstitutionList[] = {
    {"St", ""},
    {"Sa", "allocator"},
    {"Sb", "basic_string"},
    // std::basic_string<char, std::char_traits<char>,std::allocator<char> >
    {"Ss", "string"},
    // std::basic_istream<char, std::char_traits<char> >
    {"Si", "istream"},
    // std::basic_ostream<char, std::char_traits<char> >
    {"So", "ostream"},
    // std::basic_iostream<char, std::char_traits<char> >
    {"Sd", "iostream"},
    {nullptr, nullptr}};

// State needed for demangling.
struct State {
  const char* mangled_cur;   // Cursor of mangled name.
  char* out_cur;             // Cursor of output string.
  const char* out_begin;     // Beginning of output string.
  const char* out_end;       // End of output string.
  const char* prev_name;     // For constructors/destructors.
  ssize_t prev_name_length;  // For constructors/destructors.
  short nest_level;          // For nested names.
  bool append;               // Append flag.
  bool overflowed;           // True if output gets overflowed.
  uint32_t local_level;
  uint32_t expr_level;
  uint32_t arg_level;
};

// We don't use strlen() in libc since it's not guaranteed to be async
// signal safe.
size_t StrLen(const char* str) {
  size_t len = 0;
  while (*str != '\0') {
    ++str;
    ++len;
  }
  return len;
}

// Returns true if "str" has at least "n" characters remaining.
bool AtLeastNumCharsRemaining(const char* str, ssize_t n) {
  for (ssize_t i = 0; i < n; ++i) {
    if (str[i] == '\0') {
      return false;
    }
  }
  return true;
}

// Returns true if "str" has "prefix" as a prefix.
bool StrPrefix(const char* str, const char* prefix) {
  size_t i = 0;
  while (str[i] != '\0' && prefix[i] != '\0' && str[i] == prefix[i]) {
    ++i;
  }
  return prefix[i] == '\0';  // Consumed everything in "prefix".
}

void InitState(State* state, const char* mangled, char* out, size_t out_size) {
  state->mangled_cur = mangled;
  state->out_cur = out;
  state->out_begin = out;
  state->out_end = out + out_size;
  state->prev_name = nullptr;
  state->prev_name_length = -1;
  state->nest_level = -1;
  state->append = true;
  state->overflowed = false;
  state->local_level = 0;
  state->expr_level = 0;
  state->arg_level = 0;
}

// Returns true and advances "mangled_cur" if we find "one_char_token"
// at "mangled_cur" position.  It is assumed that "one_char_token" does
// not contain '\0'.
bool ParseOneCharToken(State* state, const char one_char_token) {
  if (state->mangled_cur[0] == one_char_token) {
    ++state->mangled_cur;
    return true;
  }
  return false;
}

// Returns true and advances "mangled_cur" if we find "two_char_token"
// at "mangled_cur" position.  It is assumed that "two_char_token" does
// not contain '\0'.
bool ParseTwoCharToken(State* state, const char* two_char_token) {
  if (state->mangled_cur[0] == two_char_token[0] &&
      state->mangled_cur[1] == two_char_token[1]) {
    state->mangled_cur += 2;
    return true;
  }
  return false;
}

// Returns true and advances "mangled_cur" if we find any character in
// "char_class" at "mangled_cur" position.
bool ParseCharClass(State* state, const char* char_class) {
  const char* p = char_class;
  for (; *p != '\0'; ++p) {
    if (state->mangled_cur[0] == *p) {
      ++state->mangled_cur;
      return true;
    }
  }
  return false;
}

// This function is used for handling an optional non-terminal.
bool Optional(bool) { return true; }

// This function is used for handling <non-terminal>+ syntax.
using ParseFunc = bool (*)(State*);
bool OneOrMore(ParseFunc parse_func, State* state) {
  if (parse_func(state)) {
    while (parse_func(state)) {
    }
    return true;
  }
  return false;
}

// This function is used for handling <non-terminal>* syntax. The function
// always returns true and must be followed by a termination token or a
// terminating sequence not handled by parse_func (e.g.
// ParseOneCharToken(state, 'E')).
bool ZeroOrMore(ParseFunc parse_func, State* state) {
  while (parse_func(state)) {
  }
  return true;
}

// Append "str" at "out_cur".  If there is an overflow, "overflowed"
// is set to true for later use.  The output string is ensured to
// always terminate with '\0' as long as there is no overflow.
void Append(State* state, const char* const str, ssize_t length) {
  if (state->out_cur == nullptr) {
    state->overflowed = true;
    return;
  }
  for (ssize_t i = 0; i < length; ++i) {
    if (state->out_cur + 1 < state->out_end) {  // +1 for '\0'
      *state->out_cur = str[i];
      ++state->out_cur;
    } else {
      state->overflowed = true;
      break;
    }
  }
  if (!state->overflowed) {
    *state->out_cur = '\0';  // Terminate it with '\0'
  }
}

// We don't use equivalents in libc to avoid locale issues.
bool IsLower(char c) { return c >= 'a' && c <= 'z'; }

bool IsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsDigit(char c) { return c >= '0' && c <= '9'; }

// Returns true if "str" is a function clone suffix.  These suffixes are used
// by GCC 4.5.x and later versions to indicate functions which have been
// cloned during optimization.  We treat any sequence (.<alpha>+.<digit>+)+ as
// a function clone suffix.
bool IsFunctionCloneSuffix(const char* str) {
  size_t i = 0;
  while (str[i] != '\0') {
    // Consume a single .<alpha>+.<digit>+ sequence.
    if (str[i] != '.' || !IsAlpha(str[i + 1])) {
      return false;
    }
    i += 2;
    while (IsAlpha(str[i])) {
      ++i;
    }
    if (str[i] != '.' || !IsDigit(str[i + 1])) {
      return false;
    }
    i += 2;
    while (IsDigit(str[i])) {
      ++i;
    }
  }
  return true;  // Consumed everything in "str".
}

// Append "str" with some tweaks, iff "append" state is true.
// Returns true so that it can be placed in "if" conditions.
void MaybeAppendWithLength(State* state, const char* const str,
                           ssize_t length) {
  if (state->append && length > 0) {
    // Append a space if the output buffer ends with '<' and "str"
    // starts with '<' to avoid <<<.
    if (str[0] == '<' && state->out_begin < state->out_cur &&
        state->out_cur[-1] == '<') {
      Append(state, " ", 1);
    }
    // Remember the last identifier name for ctors/dtors.
    if (IsAlpha(str[0]) || str[0] == '_') {
      state->prev_name = state->out_cur;
      state->prev_name_length = length;
    }
    Append(state, str, length);
  }
}

// A convenient wrapper around MaybeAppendWithLength().
bool MaybeAppend(State* state, const char* const str) {
  if (state->append) {
    size_t length = StrLen(str);
    MaybeAppendWithLength(state, str, static_cast<ssize_t>(length));
  }
  return true;
}

// This function is used for handling nested names.
bool EnterNestedName(State* state) {
  state->nest_level = 0;
  return true;
}

// This function is used for handling nested names.
bool LeaveNestedName(State* state, short prev_value) {
  state->nest_level = prev_value;
  return true;
}

// Disable the append mode not to print function parameters, etc.
bool DisableAppend(State* state) {
  state->append = false;
  return true;
}

// Restore the append mode to the previous state.
bool RestoreAppend(State* state, bool prev_value) {
  state->append = prev_value;
  return true;
}

// Increase the nest level for nested names.
void MaybeIncreaseNestLevel(State* state) {
  if (state->nest_level > -1) {
    ++state->nest_level;
  }
}

// Appends :: for nested names if necessary.
void MaybeAppendSeparator(State* state) {
  if (state->nest_level >= 1) {
    MaybeAppend(state, "::");
  }
}

// Cancel the last separator if necessary.
void MaybeCancelLastSeparator(State* state) {
  if (state->nest_level >= 1 && state->append &&
      state->out_begin <= state->out_cur - 2) {
    state->out_cur -= 2;
    *state->out_cur = '\0';
  }
}

// Returns true if the identifier of the given length pointed to by
// "mangled_cur" is anonymous namespace.
bool IdentifierIsAnonymousNamespace(State* state, ssize_t length) {
  const char anon_prefix[] = "_GLOBAL__N_";
  return (length > static_cast<ssize_t>(sizeof(anon_prefix)) -
                       1 &&  // Should be longer.
          StrPrefix(state->mangled_cur, anon_prefix));
}

// Forward declarations of our parsing functions.
bool ParseMangledName(State* state);
bool ParseEncoding(State* state);
bool ParseName(State* state);
bool ParseUnscopedName(State* state);
bool ParseUnscopedTemplateName(State* state);
bool ParseNestedName(State* state);
bool ParsePrefix(State* state);
bool ParseUnqualifiedName(State* state);
bool ParseSourceName(State* state);
bool ParseLocalSourceName(State* state);
bool ParseNumber(State* state, int* number_out);
bool ParseFloatNumber(State* state);
bool ParseSeqId(State* state);
bool ParseIdentifier(State* state, ssize_t length);
bool ParseAbiTags(State* state);
bool ParseAbiTag(State* state);
bool ParseOperatorName(State* state);
bool ParseSpecialName(State* state);
bool ParseCallOffset(State* state);
bool ParseNVOffset(State* state);
bool ParseVOffset(State* state);
bool ParseCtorDtorName(State* state);
bool ParseType(State* state);
bool ParseCVQualifiers(State* state);
bool ParseBuiltinType(State* state);
bool ParseFunctionType(State* state);
bool ParseBareFunctionType(State* state);
bool ParseClassEnumType(State* state);
bool ParseArrayType(State* state);
bool ParsePointerToMemberType(State* state);
bool ParseTemplateParam(State* state);
bool ParseTemplateTemplateParam(State* state);
bool ParseTemplateArgs(State* state);
bool ParseTemplateArg(State* state);
bool ParseExpression(State* state);
bool ParseExprPrimary(State* state);
bool ParseLocalName(State* state);
bool ParseDiscriminator(State* state);
bool ParseSubstitution(State* state);

// Implementation note: the following code is a straightforward
// translation of the Itanium C++ ABI defined in BNF with a couple of
// exceptions.
//
// - Support GNU extensions not defined in the Itanium C++ ABI
// - <prefix> and <template-prefix> are combined to avoid infinite loop
// - Reorder patterns to shorten the code
// - Reorder patterns to give greedier functions precedence
//   We'll mark "Less greedy than" for these cases in the code
//
// Each parsing function changes the state and returns true on
// success.  Otherwise, don't change the state and returns false.  To
// ensure that the state isn't changed in the latter case, we save the
// original state before we call more than one parsing functions
// consecutively with &&, and restore the state if unsuccessful.  See
// ParseEncoding() as an example of this convention.  We follow the
// convention throughout the code.
//
// Originally we tried to do demangling without following the full ABI
// syntax but it turned out we needed to follow the full syntax to
// parse complicated cases like nested template arguments.  Note that
// implementing a full-fledged demangler isn't trivial (libiberty's
// cp-demangle.c has +4300 lines).
//
// Note that (foo) in <(foo) ...> is a modifier to be ignored.
//
// Reference:
// - Itanium C++ ABI
//   <http://www.codesourcery.com/cxx-abi/abi.html#mangling>

// <mangled-name> ::= _Z <encoding>
bool ParseMangledName(State* state) {
  return ParseTwoCharToken(state, "_Z") && ParseEncoding(state);
}

// <encoding> ::= <(function) name> <bare-function-type>
//            ::= <(data) name>
//            ::= <special-name>
bool ParseEncoding(State* state) {
  State copy = *state;
  if (ParseName(state) && ParseBareFunctionType(state)) {
    return true;
  }
  *state = copy;

  if (ParseName(state) || ParseSpecialName(state)) {
    return true;
  }
  return false;
}

// <name> ::= <nested-name>
//        ::= <unscoped-template-name> <template-args>
//        ::= <unscoped-name>
//        ::= <local-name>
bool ParseName(State* state) {
  if (ParseNestedName(state) || ParseLocalName(state)) {
    return true;
  }

  State copy = *state;
  if (ParseUnscopedTemplateName(state) && ParseTemplateArgs(state)) {
    return true;
  }
  *state = copy;

  // Less greedy than <unscoped-template-name> <template-args>.
  if (ParseUnscopedName(state)) {
    return true;
  }
  return false;
}

// <unscoped-name> ::= <unqualified-name>
//                 ::= St <unqualified-name>
bool ParseUnscopedName(State* state) {
  if (ParseUnqualifiedName(state)) {
    return true;
  }

  State copy = *state;
  if (ParseTwoCharToken(state, "St") && MaybeAppend(state, "std::") &&
      ParseUnqualifiedName(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <unscoped-template-name> ::= <unscoped-name>
//                          ::= <substitution>
bool ParseUnscopedTemplateName(State* state) {
  return ParseUnscopedName(state) || ParseSubstitution(state);
}

// <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
//               ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
bool ParseNestedName(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'N') && EnterNestedName(state) &&
      Optional(ParseCVQualifiers(state)) && ParsePrefix(state) &&
      LeaveNestedName(state, copy.nest_level) &&
      ParseOneCharToken(state, 'E')) {
    return true;
  }
  *state = copy;
  return false;
}

// This part is tricky.  If we literally translate them to code, we'll
// end up infinite loop.  Hence we merge them to avoid the case.
//
// <prefix> ::= <prefix> <unqualified-name>
//          ::= <template-prefix> <template-args>
//          ::= <template-param>
//          ::= <substitution>
//          ::= # empty
// <template-prefix> ::= <prefix> <(template) unqualified-name>
//                   ::= <template-param>
//                   ::= <substitution>
bool ParsePrefix(State* state) {
  bool has_something = false;
  while (true) {
    MaybeAppendSeparator(state);
    if (ParseTemplateParam(state) || ParseSubstitution(state) ||
        ParseUnscopedName(state)) {
      has_something = true;
      MaybeIncreaseNestLevel(state);
      continue;
    }
    MaybeCancelLastSeparator(state);
    if (has_something && ParseTemplateArgs(state)) {
      return ParsePrefix(state);
    } else {
      break;
    }
  }
  return true;
}

// <unqualified-name> ::= <operator-name>
//                    ::= <ctor-dtor-name>
//                    ::= <source-name> [<abi-tags>]
//                    ::= <local-source-name> [<abi-tags>]
bool ParseUnqualifiedName(State* state) {
  return (ParseOperatorName(state) || ParseCtorDtorName(state) ||
          (ParseSourceName(state) && Optional(ParseAbiTags(state))) ||
          (ParseLocalSourceName(state) && Optional(ParseAbiTags(state))));
}

// <source-name> ::= <positive length number> <identifier>
bool ParseSourceName(State* state) {
  State copy = *state;
  int length = -1;
  if (ParseNumber(state, &length) && ParseIdentifier(state, length)) {
    return true;
  }
  *state = copy;
  return false;
}

// <local-source-name> ::= L <source-name> [<discriminator>]
//
// References:
//   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=31775
//   http://gcc.gnu.org/viewcvs?view=rev&revision=124467
bool ParseLocalSourceName(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'L') && ParseSourceName(state) &&
      Optional(ParseDiscriminator(state))) {
    return true;
  }
  *state = copy;
  return false;
}

// <number> ::= [n] <non-negative decimal integer>
// If "number_out" is non-null, then *number_out is set to the value of the
// parsed number on success.
bool ParseNumber(State* state, int* number_out) {
  int sign = 1;
  if (ParseOneCharToken(state, 'n')) {
    sign = -1;
  }
  const char* p = state->mangled_cur;
  int number = 0;
  constexpr int int_max_by_10 = std::numeric_limits<int>::max() / 10;
  for (; *p != '\0'; ++p) {
    if (IsDigit(*p)) {
      // Prevent signed integer overflow when multiplying
      if (number > int_max_by_10) {
        return false;
      }

      const int digit = *p - '0';
      const int shifted = number * 10;

      // Prevent signed integer overflow when summing
      if (digit > std::numeric_limits<int>::max() - shifted) {
        return false;
      }

      number = shifted + digit;
    } else {
      break;
    }
  }
  if (p != state->mangled_cur) {  // Conversion succeeded.
    state->mangled_cur = p;
    if (number_out != nullptr) {
      *number_out = number * sign;
    }
    return true;
  }
  return false;
}

// Floating-point literals are encoded using a fixed-length lowercase
// hexadecimal string.
bool ParseFloatNumber(State* state) {
  const char* p = state->mangled_cur;
  for (; *p != '\0'; ++p) {
    if (!IsDigit(*p) && !(*p >= 'a' && *p <= 'f')) {
      break;
    }
  }
  if (p != state->mangled_cur) {  // Conversion succeeded.
    state->mangled_cur = p;
    return true;
  }
  return false;
}

// The <seq-id> is a sequence number in base 36,
// using digits and upper case letters
bool ParseSeqId(State* state) {
  const char* p = state->mangled_cur;
  for (; *p != '\0'; ++p) {
    if (!IsDigit(*p) && !(*p >= 'A' && *p <= 'Z')) {
      break;
    }
  }
  if (p != state->mangled_cur) {  // Conversion succeeded.
    state->mangled_cur = p;
    return true;
  }
  return false;
}

// <identifier> ::= <unqualified source code identifier> (of given length)
bool ParseIdentifier(State* state, ssize_t length) {
  if (length == -1 || !AtLeastNumCharsRemaining(state->mangled_cur, length)) {
    return false;
  }
  if (IdentifierIsAnonymousNamespace(state, length)) {
    MaybeAppend(state, "(anonymous namespace)");
  } else {
    MaybeAppendWithLength(state, state->mangled_cur, length);
  }
  if (length < 0 ||
      static_cast<std::size_t>(length) > StrLen(state->mangled_cur)) {
    return false;
  }
  state->mangled_cur += length;
  return true;
}

// <abi-tags> ::= <abi-tag> [<abi-tags>]
bool ParseAbiTags(State* state) {
  State copy = *state;
  DisableAppend(state);
  if (OneOrMore(ParseAbiTag, state)) {
    RestoreAppend(state, copy.append);
    return true;
  }
  *state = copy;
  return false;
}

// <abi-tag> ::= B <source-name>
bool ParseAbiTag(State* state) {
  return ParseOneCharToken(state, 'B') && ParseSourceName(state);
}

// <operator-name> ::= nw, and other two letters cases
//                 ::= cv <type>  # (cast)
//                 ::= v  <digit> <source-name> # vendor extended operator
bool ParseOperatorName(State* state) {
  if (!AtLeastNumCharsRemaining(state->mangled_cur, 2)) {
    return false;
  }
  // First check with "cv" (cast) case.
  State copy = *state;
  if (ParseTwoCharToken(state, "cv") && MaybeAppend(state, "operator ") &&
      EnterNestedName(state) && ParseType(state) &&
      LeaveNestedName(state, copy.nest_level)) {
    return true;
  }
  *state = copy;

  // Then vendor extended operators.
  if (ParseOneCharToken(state, 'v') && ParseCharClass(state, "0123456789") &&
      ParseSourceName(state)) {
    return true;
  }
  *state = copy;

  // Other operator names should start with a lower alphabet followed
  // by a lower/upper alphabet.
  if (!(IsLower(state->mangled_cur[0]) && IsAlpha(state->mangled_cur[1]))) {
    return false;
  }
  // We may want to perform a binary search if we really need speed.
  const AbbrevPair* p;
  for (p = kOperatorList; p->abbrev != nullptr; ++p) {
    if (state->mangled_cur[0] == p->abbrev[0] &&
        state->mangled_cur[1] == p->abbrev[1]) {
      MaybeAppend(state, "operator");
      if (IsLower(*p->real_name)) {  // new, delete, etc.
        MaybeAppend(state, " ");
      }
      MaybeAppend(state, p->real_name);
      state->mangled_cur += 2;
      return true;
    }
  }
  return false;
}

// <special-name> ::= TV <type>
//                ::= TT <type>
//                ::= TI <type>
//                ::= TS <type>
//                ::= Tc <call-offset> <call-offset> <(base) encoding>
//                ::= GV <(object) name>
//                ::= T <call-offset> <(base) encoding>
// G++ extensions:
//                ::= TC <type> <(offset) number> _ <(base) type>
//                ::= TF <type>
//                ::= TJ <type>
//                ::= GR <name>
//                ::= GA <encoding>
//                ::= Th <call-offset> <(base) encoding>
//                ::= Tv <call-offset> <(base) encoding>
//
// Note: we don't care much about them since they don't appear in
// stack traces.  The are special data.
bool ParseSpecialName(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'T') && ParseCharClass(state, "VTIS") &&
      ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "Tc") && ParseCallOffset(state) &&
      ParseCallOffset(state) && ParseEncoding(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "GV") && ParseName(state)) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'T') && ParseCallOffset(state) &&
      ParseEncoding(state)) {
    return true;
  }
  *state = copy;

  // G++ extensions
  if (ParseTwoCharToken(state, "TC") && ParseType(state) &&
      ParseNumber(state, nullptr) && ParseOneCharToken(state, '_') &&
      DisableAppend(state) && ParseType(state)) {
    RestoreAppend(state, copy.append);
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'T') && ParseCharClass(state, "FJ") &&
      ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "GR") && ParseName(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "GA") && ParseEncoding(state)) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'T') && ParseCharClass(state, "hv") &&
      ParseCallOffset(state) && ParseEncoding(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <call-offset> ::= h <nv-offset> _
//               ::= v <v-offset> _
bool ParseCallOffset(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'h') && ParseNVOffset(state) &&
      ParseOneCharToken(state, '_')) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'v') && ParseVOffset(state) &&
      ParseOneCharToken(state, '_')) {
    return true;
  }
  *state = copy;

  return false;
}

// <nv-offset> ::= <(offset) number>
bool ParseNVOffset(State* state) { return ParseNumber(state, nullptr); }

// <v-offset>  ::= <(offset) number> _ <(virtual offset) number>
bool ParseVOffset(State* state) {
  State copy = *state;
  if (ParseNumber(state, nullptr) && ParseOneCharToken(state, '_') &&
      ParseNumber(state, nullptr)) {
    return true;
  }
  *state = copy;
  return false;
}

// <ctor-dtor-name> ::= C1 | C2 | C3
//                  ::= D0 | D1 | D2
bool ParseCtorDtorName(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'C') && ParseCharClass(state, "123")) {
    const char* const prev_name = state->prev_name;
    const ssize_t prev_name_length = state->prev_name_length;
    MaybeAppendWithLength(state, prev_name, prev_name_length);
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'D') && ParseCharClass(state, "012")) {
    const char* const prev_name = state->prev_name;
    const ssize_t prev_name_length = state->prev_name_length;
    MaybeAppend(state, "~");
    MaybeAppendWithLength(state, prev_name, prev_name_length);
    return true;
  }
  *state = copy;
  return false;
}

// <type> ::= <CV-qualifiers> <type>
//        ::= P <type>   # pointer-to
//        ::= R <type>   # reference-to
//        ::= O <type>   # rvalue reference-to (C++0x)
//        ::= C <type>   # complex pair (C 2000)
//        ::= G <type>   # imaginary (C 2000)
//        ::= U <source-name> <type>  # vendor extended type qualifier
//        ::= <builtin-type>
//        ::= <function-type>
//        ::= <class-enum-type>
//        ::= <array-type>
//        ::= <pointer-to-member-type>
//        ::= <template-template-param> <template-args>
//        ::= <template-param>
//        ::= <substitution>
//        ::= Dp <type>          # pack expansion of (C++0x)
//        ::= Dt <expression> E  # decltype of an id-expression or class
//                               # member access (C++0x)
//        ::= DT <expression> E  # decltype of an expression (C++0x)
//
bool ParseType(State* state) {
  // We should check CV-qualifers, and PRGC things first.
  State copy = *state;
  if (ParseCVQualifiers(state) && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseCharClass(state, "OPRCG") && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "Dp") && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'D') && ParseCharClass(state, "tT") &&
      ParseExpression(state) && ParseOneCharToken(state, 'E')) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'U') && ParseSourceName(state) &&
      ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseBuiltinType(state) || ParseFunctionType(state) ||
      ParseClassEnumType(state) || ParseArrayType(state) ||
      ParsePointerToMemberType(state) || ParseSubstitution(state)) {
    return true;
  }

  if (ParseTemplateTemplateParam(state) && ParseTemplateArgs(state)) {
    return true;
  }
  *state = copy;

  // Less greedy than <template-template-param> <template-args>.
  if (ParseTemplateParam(state)) {
    return true;
  }

  return false;
}

// <CV-qualifiers> ::= [r] [V] [K]
// We don't allow empty <CV-qualifiers> to avoid infinite loop in
// ParseType().
bool ParseCVQualifiers(State* state) {
  int num_cv_qualifiers = 0;
  num_cv_qualifiers += ParseOneCharToken(state, 'r');
  num_cv_qualifiers += ParseOneCharToken(state, 'V');
  num_cv_qualifiers += ParseOneCharToken(state, 'K');
  return num_cv_qualifiers > 0;
}

// <builtin-type> ::= v, etc.
//                ::= u <source-name>
bool ParseBuiltinType(State* state) {
  const AbbrevPair* p;
  for (p = kBuiltinTypeList; p->abbrev != nullptr; ++p) {
    if (state->mangled_cur[0] == p->abbrev[0]) {
      MaybeAppend(state, p->real_name);
      ++state->mangled_cur;
      return true;
    }
  }

  State copy = *state;
  if (ParseOneCharToken(state, 'u') && ParseSourceName(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <function-type> ::= F [Y] <bare-function-type> E
bool ParseFunctionType(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'F') &&
      Optional(ParseOneCharToken(state, 'Y')) && ParseBareFunctionType(state) &&
      ParseOneCharToken(state, 'E')) {
    return true;
  }
  *state = copy;
  return false;
}

// <bare-function-type> ::= <(signature) type>+
bool ParseBareFunctionType(State* state) {
  State copy = *state;
  DisableAppend(state);
  if (OneOrMore(ParseType, state)) {
    RestoreAppend(state, copy.append);
    MaybeAppend(state, "()");
    return true;
  }
  *state = copy;
  return false;
}

// <class-enum-type> ::= <name>
bool ParseClassEnumType(State* state) { return ParseName(state); }

// <array-type> ::= A <(positive dimension) number> _ <(element) type>
//              ::= A [<(dimension) expression>] _ <(element) type>
bool ParseArrayType(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'A') && ParseNumber(state, nullptr) &&
      ParseOneCharToken(state, '_') && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'A') && Optional(ParseExpression(state)) &&
      ParseOneCharToken(state, '_') && ParseType(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <pointer-to-member-type> ::= M <(class) type> <(member) type>
bool ParsePointerToMemberType(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'M') && ParseType(state) && ParseType(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <template-param> ::= T_
//                  ::= T <parameter-2 non-negative number> _
bool ParseTemplateParam(State* state) {
  if (ParseTwoCharToken(state, "T_")) {
    MaybeAppend(state, "?");  // We don't support template substitutions.
    return true;
  }

  State copy = *state;
  if (ParseOneCharToken(state, 'T') && ParseNumber(state, nullptr) &&
      ParseOneCharToken(state, '_')) {
    MaybeAppend(state, "?");  // We don't support template substitutions.
    return true;
  }
  *state = copy;
  return false;
}

// <template-template-param> ::= <template-param>
//                           ::= <substitution>
bool ParseTemplateTemplateParam(State* state) {
  return (ParseTemplateParam(state) || ParseSubstitution(state));
}

// <template-args> ::= I <template-arg>+ E
bool ParseTemplateArgs(State* state) {
  State copy = *state;
  DisableAppend(state);
  if (ParseOneCharToken(state, 'I') && OneOrMore(ParseTemplateArg, state) &&
      ParseOneCharToken(state, 'E')) {
    RestoreAppend(state, copy.append);
    MaybeAppend(state, "<>");
    return true;
  }
  *state = copy;
  return false;
}

// <template-arg>  ::= <type>
//                 ::= <expr-primary>
//                 ::= I <template-arg>* E        # argument pack
//                 ::= J <template-arg>* E        # argument pack
//                 ::= X <expression> E
bool ParseTemplateArg(State* state) {
  // Avoid recursion above max_levels
  constexpr uint32_t max_levels = 6;

  if (state->arg_level > max_levels) {
    return false;
  }
  ++state->arg_level;

  State copy = *state;
  if ((ParseOneCharToken(state, 'I') || ParseOneCharToken(state, 'J')) &&
      ZeroOrMore(ParseTemplateArg, state) && ParseOneCharToken(state, 'E')) {
    --state->arg_level;
    return true;
  }
  *state = copy;

  if (ParseType(state) || ParseExprPrimary(state)) {
    --state->arg_level;
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'X') && ParseExpression(state) &&
      ParseOneCharToken(state, 'E')) {
    --state->arg_level;
    return true;
  }
  *state = copy;
  return false;
}

// <expression> ::= <template-param>
//              ::= <expr-primary>
//              ::= <unary operator-name> <expression>
//              ::= <binary operator-name> <expression> <expression>
//              ::= <trinary operator-name> <expression> <expression>
//                  <expression>
//              ::= st <type>
//              ::= sr <type> <unqualified-name> <template-args>
//              ::= sr <type> <unqualified-name>
bool ParseExpression(State* state) {
  if (ParseTemplateParam(state) || ParseExprPrimary(state)) {
    return true;
  }

  // Avoid recursion above max_levels
  constexpr uint32_t max_levels = 5;

  if (state->expr_level > max_levels) {
    return false;
  }
  ++state->expr_level;

  State copy = *state;
  if (ParseOperatorName(state) && ParseExpression(state) &&
      ParseExpression(state) && ParseExpression(state)) {
    --state->expr_level;
    return true;
  }
  *state = copy;

  if (ParseOperatorName(state) && ParseExpression(state) &&
      ParseExpression(state)) {
    --state->expr_level;
    return true;
  }
  *state = copy;

  if (ParseOperatorName(state) && ParseExpression(state)) {
    --state->expr_level;
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "st") && ParseType(state)) {
    return true;
    --state->expr_level;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "sr") && ParseType(state) &&
      ParseUnqualifiedName(state) && ParseTemplateArgs(state)) {
    --state->expr_level;
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "sr") && ParseType(state) &&
      ParseUnqualifiedName(state)) {
    --state->expr_level;
    return true;
  }
  *state = copy;

  // Pack expansion
  if (ParseTwoCharToken(state, "sp") && ParseType(state)) {
    --state->expr_level;
    return true;
  }
  *state = copy;

  return false;
}

// <expr-primary> ::= L <type> <(value) number> E
//                ::= L <type> <(value) float> E
//                ::= L <mangled-name> E
//                // A bug in g++'s C++ ABI version 2 (-fabi-version=2).
//                ::= LZ <encoding> E
bool ParseExprPrimary(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, 'L') && ParseType(state) &&
      ParseNumber(state, nullptr) && ParseOneCharToken(state, 'E')) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'L') && ParseType(state) &&
      ParseFloatNumber(state) && ParseOneCharToken(state, 'E')) {
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'L') && ParseMangledName(state) &&
      ParseOneCharToken(state, 'E')) {
    return true;
  }
  *state = copy;

  if (ParseTwoCharToken(state, "LZ") && ParseEncoding(state) &&
      ParseOneCharToken(state, 'E')) {
    return true;
  }
  *state = copy;

  return false;
}

// <local-name> := Z <(function) encoding> E <(entity) name>
//                 [<discriminator>]
//              := Z <(function) encoding> E s [<discriminator>]
bool ParseLocalName(State* state) {
  // Avoid recursion above max_levels
  constexpr uint32_t max_levels = 5;
  if (state->local_level > max_levels) {
    return false;
  }
  ++state->local_level;

  State copy = *state;
  if (ParseOneCharToken(state, 'Z') && ParseEncoding(state) &&
      ParseOneCharToken(state, 'E') && MaybeAppend(state, "::") &&
      ParseName(state) && Optional(ParseDiscriminator(state))) {
    --state->local_level;
    return true;
  }
  *state = copy;

  if (ParseOneCharToken(state, 'Z') && ParseEncoding(state) &&
      ParseTwoCharToken(state, "Es") && Optional(ParseDiscriminator(state))) {
    --state->local_level;
    return true;
  }
  *state = copy;
  return false;
}

// <discriminator> := _ <(non-negative) number>
bool ParseDiscriminator(State* state) {
  State copy = *state;
  if (ParseOneCharToken(state, '_') && ParseNumber(state, nullptr)) {
    return true;
  }
  *state = copy;
  return false;
}

// <substitution> ::= S_
//                ::= S <seq-id> _
//                ::= St, etc.
bool ParseSubstitution(State* state) {
  if (ParseTwoCharToken(state, "S_")) {
    MaybeAppend(state, "?");  // We don't support substitutions.
    return true;
  }

  State copy = *state;
  if (ParseOneCharToken(state, 'S') && ParseSeqId(state) &&
      ParseOneCharToken(state, '_')) {
    MaybeAppend(state, "?");  // We don't support substitutions.
    return true;
  }
  *state = copy;

  // Expand abbreviations like "St" => "std".
  if (ParseOneCharToken(state, 'S')) {
    const AbbrevPair* p;
    for (p = kSubstitutionList; p->abbrev != nullptr; ++p) {
      if (state->mangled_cur[0] == p->abbrev[1]) {
        MaybeAppend(state, "std");
        if (p->real_name[0] != '\0') {
          MaybeAppend(state, "::");
          MaybeAppend(state, p->real_name);
        }
        ++state->mangled_cur;
        return true;
      }
    }
  }
  *state = copy;
  return false;
}

// Parse <mangled-name>, optionally followed by either a function-clone suffix
// or version suffix.  Returns true only if all of "mangled_cur" was consumed.
bool ParseTopLevelMangledName(State* state) {
  if (ParseMangledName(state)) {
    if (state->mangled_cur[0] != '\0') {
      // Drop trailing function clone suffix, if any.
      if (IsFunctionCloneSuffix(state->mangled_cur)) {
        return true;
      }
      // Append trailing version suffix if any.
      // ex. _Z3foo@@GLIBCXX_3.4
      if (state->mangled_cur[0] == '@') {
        MaybeAppend(state, state->mangled_cur);
        return true;
      }
      return ParseName(state);
    }
    return true;
  }
  return false;
}
}  // namespace
#endif

// The demangler entry point.
bool Demangle(const char* mangled, char* out, size_t out_size) {
#if defined(GLOG_OS_WINDOWS)
#if defined(HAVE_DBGHELP)
  // When built with incremental linking, the Windows debugger
  // library provides a more complicated `Symbol->Name` with the
  // Incremental Linking Table offset, which looks like
  // `@ILT+1105(?func@Foo@@SAXH@Z)`. However, the demangler expects
  // only the mangled symbol, `?func@Foo@@SAXH@Z`. Fortunately, the
  // mangled symbol is guaranteed not to have parentheses,
  // so we search for `(` and extract up to `)`.
  //
  // Since we may be in a signal handler here, we cannot use `std::string`.
  char buffer[1024];  // Big enough for a sane symbol.
  const char* lparen = strchr(mangled, '(');
  if (lparen) {
    // Extract the string `(?...)`
    const char* rparen = strchr(lparen, ')');
    size_t length = static_cast<size_t>(rparen - lparen) - 1;
    strncpy(buffer, lparen + 1, length);
    buffer[length] = '\0';
    mangled = buffer;
  }  // Else the symbol wasn't inside a set of parentheses
  // We use the ANSI version to ensure the string type is always `char *`.
  return UnDecorateSymbolName(mangled, out, out_size, UNDNAME_COMPLETE);
#else
  (void)mangled;
  (void)out;
  (void)out_size;
  return false;
#endif
#elif defined(HAVE___CXA_DEMANGLE)
  int status = -1;
  std::size_t n = 0;
  std::unique_ptr<char, decltype(&std::free)> unmangled{
      abi::__cxa_demangle(mangled, nullptr, &n, &status), &std::free};

  if (!unmangled) {
    return false;
  }

  std::copy_n(unmangled.get(), std::min(n, out_size), out);
  return status == 0;
#else
  State state;
  InitState(&state, mangled, out, out_size);
  return ParseTopLevelMangledName(&state) && !state.overflowed;
#endif
}

}  // namespace roo_logging