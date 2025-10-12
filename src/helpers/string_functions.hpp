#pragma once
#include <string>
#include "../snippets.hpp"

using namespace std;

/// \brief fillup a string to a certain length with a character
///
extern string fillup(string in, uint32 len, char with = ' ');

/// \brief replace all newlines with "\n\t"
///
extern string intab(string in, string with="\n\t");
