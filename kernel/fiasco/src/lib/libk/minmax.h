#pragma once

/** @brief Equivalent to std::min. */
template<typename T>
const T&
min(const T& a, const T& b)
{ return (a < b) ? a : b; }

/** @brief Equivalent to std::max. */
template<typename T>
const T&
max(const T& a, const T& b)
{ return (a > b) ? a : b; }

