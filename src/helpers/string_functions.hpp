#pragma once
#include <string>
#include "../snippets.hpp"

using namespace std;

/// \brief fillup a string to a certain length with a character
///
extern string fillup(string in, uint32 len, char with = ' ');

/// \brief replace all newlines with "\n\t"
///
extern string intab(string in, string with = "\n\t");

///
/// \brief insert a value into a string.
///
/// \param val value to be inserted
/// \param target target string to insert into (first "{}")
///
extern string insert(string val, string target);

///
/// \brief insert a value into a string.
///
/// \param val value to be inserted
/// \param target target string to insert into (last "{}")
///
extern string rinsert(string val, string target);
