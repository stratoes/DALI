// Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DALI_CORE_GEOM_MAT_H_
#define DALI_CORE_GEOM_MAT_H_

#include <cmath>
#include "dali/core/host_dev.h"
#include "dali/core/util.h"
#include "dali/core/tuple_helpers.h"
#include "dali/core/geom/vec.h"

#ifndef MAT_LAYOUT_ROW_MAJOR
#define MAT_LAYOUT_ROW_MAJOR 0
#endif

namespace dali {

template <size_t rows_, size_t cols_, typename Element>
struct mat {
  using element_t = Element;
  using col_t = vec<rows_, Element>;
  using row_t = vec<cols_, Element>;

  static constexpr size_t rows = rows_;
  static constexpr size_t cols = cols_;
#if MAT_LAYOUT_ROW_MAJOR
  vec<cols, Element> m[rows];
#else
  vec<rows, Element> m[cols];
#endif

#if MAT_LAYOUT_ROW_MAJOR
  #define MAT_ELEMENT_LOOP(row_index, col_index)                \
    for (size_t row_index = 0; row_index < rows; row_index++)   \
      for (size_t col_index = 0; col_index < cols; col_index++)

  #define MAT_STORAGE_LOOP(m_index) \
    for (size_t m_index = 0; m_index < rows; m_index++)
#else
  #define MAT_ELEMENT_LOOP(row_index, col_index)                \
    for (size_t col_index = 0; col_index < cols; col_index++)   \
      for (size_t row_index = 0; row_index < rows; row_index++)

  #define MAT_STORAGE_LOOP(m_index) \
    for (size_t m_index = 0; m_index < cols; m_index++)
#endif

  static_assert(std::is_standard_layout<Element>::value,
                "Cannot create a matrix of a non-standard layout type");

  /// @brief Default constructor doesn't initialize the matrix for fundamental type.
  constexpr mat() = default;
  constexpr mat(const mat &) = default;
  constexpr mat(mat &&) = default;

  /// @brief Fills the diagonal with a scalar value
  DALI_HOST_DEV
  constexpr mat(Element scalar) : m{} {  // NOLINT
    size_t n = rows < cols ? rows : cols;
    for (size_t i = 0; i < n; i++)
      at(i, i) = scalar;
  }

  DALI_HOST_DEV
  constexpr mat(const Element(&values)[rows][cols]) : m{} {  // NOLINT
    MAT_ELEMENT_LOOP(i, j)
      at(i, j) = values[i][j];
  }

  template <typename U, size_t c = cols>
  DALI_HOST_DEV
  constexpr mat(const vec<rows, U> &v) : m{} {  // NOLINT
    static_assert(c == 1, "Only single-column matrices can be constructed from vectors");
    for (size_t i = 0; i < rows; i++)
      at(i, 0) = v[i];
  }

  template <typename U>
  DALI_HOST_DEV
  constexpr mat(const mat<rows, cols, U> &rhs) : mat(rhs.template cast<Element>()) {  // NOLINT
  }

  DALI_HOST_DEV
  constexpr auto row(int r) const {
  #if MAT_LAYOUT_ROW_MAJOR
    return m[r];
  #else
    vec<cols, Element> ret = {};
    for (size_t j = 0; j < cols; j++)
      ret[j] = at(r, j);
    return ret;
  #endif
  }

  DALI_HOST_DEV
  constexpr auto col(int c) const {
  #if MAT_LAYOUT_ROW_MAJOR
    vec<rows, Element> ret = {};
    for (size_t i = 0; i < rows; i++)
      ret[i] = at(i, c);
    return ret;
  #else
    return m[c];
  #endif
  }

  DALI_HOST_DEV
  void set_col(int c, const col_t &col) {
  #if MAT_LAYOUT_ROW_MAJOR
    for (size_t i = 0; i < rows; i++)
      m[i][c] = col[i];
  #else
    m[c] = col;
  #endif
  }

  DALI_HOST_DEV
  void set_row(int r, const row_t &row) {
  #if MAT_LAYOUT_ROW_MAJOR
    m[r] = row;
  #else
    for (size_t j = 0; j < cols; j++)
      m[j][r] = row[j];
  #endif
  }

  DALI_HOST_DEV
  constexpr Element &at(int r, int c) {
  #if MAT_LAYOUT_ROW_MAJOR
    return m[r][c];
  #else
    return m[c][r];
  #endif
  }
  DALI_HOST_DEV
  constexpr const Element &at(int r, int c) const {
  #if MAT_LAYOUT_ROW_MAJOR
    return m[r][c];
  #else
    return m[c][r];
  #endif
  }

  DALI_HOST_DEV
  constexpr Element &operator()(int r, int c) {
    return at(r, c);
  }
  DALI_HOST_DEV
  constexpr const Element &operator()(int r, int c) const {
    return at(r, c);
  }

  mat &operator=(const mat &) = default;

  DALI_HOST_DEV
  constexpr mat<cols, rows, Element> T() const {
    mat<cols, rows, Element> retval = {};
    MAT_ELEMENT_LOOP(i, j)
      retval(j, i) = at(i, j);
    return retval;
  }

  template <typename U>
  DALI_HOST_DEV
  constexpr mat<rows, cols, U> cast() const {
    mat<rows, cols, U> result = {};
    MAT_ELEMENT_LOOP(i, j)
      result(i, j) = static_cast<U>(at(i, j));
    return result;
  }

  DALI_HOST_DEV
  constexpr mat operator+() const {
    return *this;
  }

  DALI_HOST_DEV
  inline mat operator-() const {
    mat result;
    MAT_ELEMENT_LOOP(i, j)
      result(i, j) = -at(i, j);
    return result;
  }

  DALI_HOST_DEV
  inline mat operator~() const {
    mat result;
    MAT_ELEMENT_LOOP(i, j)
      result(i, j) = ~at(i, j);
  }

#define DEFINE_ASSIGN_MAT_OP(op)                                    \
  template <typename U>                                             \
  DALI_HOST_DEV mat &operator op(const mat<rows, cols, U> &other) { \
    MAT_STORAGE_LOOP(i)                                             \
      m[i] op other.m[i];                                           \
    return *this;                                                   \
  }

#define DEFINE_ASSIGN_MAT_SCALAR_OP(op)                             \
  template <typename U>                                             \
  DALI_HOST_DEV                                                     \
  std::enable_if_t<is_scalar<U>::value, mat<rows, cols, Element> &> \
  operator op(const U &other) {                                     \
    MAT_STORAGE_LOOP(i)                                             \
    m[i] op other;                                                  \
    return *this;                                                   \
  }

  DEFINE_ASSIGN_MAT_OP(=)
  DEFINE_ASSIGN_MAT_OP(+=)
  DEFINE_ASSIGN_MAT_OP(-=)
  DEFINE_ASSIGN_MAT_OP(&=)
  DEFINE_ASSIGN_MAT_OP(|=)
  DEFINE_ASSIGN_MAT_OP(^=)
  DEFINE_ASSIGN_MAT_OP(<<=)
  DEFINE_ASSIGN_MAT_OP(>>=)

  DEFINE_ASSIGN_MAT_SCALAR_OP(=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(+=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(-=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(*=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(/=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(&=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(|=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(^=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(<<=)
  DEFINE_ASSIGN_MAT_SCALAR_OP(>>=)

  #undef DEFINE_ASSIGN_MAT_OP
  #undef DEFINE_ASSIGN_MAT_SCALAR_OP

  template <size_t rhs_cols, typename U>
  DALI_HOST_DEV
  inline mat &operator*=(const mat<cols, rhs_cols, U> &m) {
    static_assert(rhs_cols == cols && rhs_cols == rows,
                  "Operator *= can only be applied to square matrices");
    *this = *this * m;
  }

  template <typename U, size_t rhs_cols>
  DALI_HOST_DEV
  inline auto operator*(const mat<cols, rhs_cols, U> &rhs) const {
    using R = promote_vec_t<Element, U>;
    mat<rows, rhs_cols, R> result = {};
    for (size_t j = 0; j < rhs_cols; j++) {
      for (size_t i = 0; i < rows; i++) {
        result(i, j) = dot(row(i), rhs.col(j));
      }
    }
    return result;
  }

  template <typename U>
  DALI_HOST_DEV
  inline auto operator*(const vec<cols, U> &v) const {
    using R = promote_vec_t<Element, U>;
  #if MAT_LAYOUT_ROW_MAJOR
    vec<rows, R> result;
    for (size_t i = 0; i < rows; i++)
      result[i] = dot(v, m[i]);
  #else
    vec<rows, R> result = v[0] * m[0];
    for (size_t i = 1; i < cols; i++)
      result += v[i] * m[i];
  #endif
    return result;
  }
};

template <size_t rows, size_t cols, typename T, typename U>
DALI_HOST_DEV
constexpr bool operator==(const mat<rows, cols, T> &a, const mat<rows, cols, U> &b) {
  MAT_ELEMENT_LOOP(i, j)
    if (a(i, j) != b(i, j))
      return false;
  return true;
}

template <size_t rows, size_t cols, typename T, typename U>
DALI_HOST_DEV
constexpr bool operator!=(const mat<rows, cols, T> &a, const mat<rows, cols, U> &b) {
  return !(a == b);
}

template <size_t sub_r, size_t sub_c, size_t rows, size_t cols, typename Element>
DALI_HOST_DEV
constexpr auto sub(const mat<rows, cols, Element> &m, int r, int c) {
  mat<sub_r, sub_c, Element> result = {};
  for (size_t j = 0; j < sub_c; j++)
    for (size_t i = 0; i < sub_r; i++)
      result(i, j) = m(i+r, j+c);
  return result;
}

#define DEFINE_ELEMENTWISE_MAT_MAT_BINOP(op)                            \
  template <size_t rows, size_t cols, typename T1, typename T2>         \
  DALI_HOST_DEV inline auto operator op(const mat<rows, cols, T1> &a,   \
                                        const mat<rows, cols, T2> &b) { \
    using R = promote_vec_t<T1, T2>;                                    \
    mat<rows, cols, R> result;                                          \
    MAT_ELEMENT_LOOP(i, j)                                              \
      result(i, j) = a(i, j) op b(i, j);                                \
    return result;                                                      \
  }

#define DEFINE_ELEMENTWISE_RHS_BINOP(op)                            \
  template <size_t rows, size_t cols, typename T1, typename T2,     \
            typename R = promote_vec_scalar_t<T1, T2>>              \
  DALI_HOST_DEV                                                     \
  inline std::enable_if_t<is_scalar<T2>::value, mat<rows, cols, R>> \
    operator op(const mat<rows, cols, T1> &a, const T2 &b) {        \
    mat<rows, cols, R> result;                                      \
    MAT_ELEMENT_LOOP(i, j)                                          \
      result(i, j) = a(i, j) op b;                                  \
    return result;                                                  \
  }

#define DEFINE_ELEMENTWISE_LHS_BINOP(op)                            \
  template <size_t rows, size_t cols, typename T1, typename T2,     \
            typename R = promote_vec_scalar_t<T2, T1>>              \
  DALI_HOST_DEV                                                     \
  inline std::enable_if_t<is_scalar<T1>::value, mat<rows, cols, R>> \
  operator op(const T1 &a, const mat<rows, cols, T2> &b) {          \
    mat<rows, cols, R> result;                                      \
    MAT_ELEMENT_LOOP(i, j)                                          \
      result(i, j) = a op b(i, j);                                  \
    return result;                                                  \
  }

#define DEFINE_ELEMENTWISE_MAT_BINOP(op)\
  DEFINE_ELEMENTWISE_MAT_MAT_BINOP(op)\
  DEFINE_ELEMENTWISE_RHS_BINOP(op)\
  DEFINE_ELEMENTWISE_LHS_BINOP(op)

DEFINE_ELEMENTWISE_MAT_BINOP(+)
DEFINE_ELEMENTWISE_MAT_BINOP(-)
DEFINE_ELEMENTWISE_MAT_BINOP(&)
DEFINE_ELEMENTWISE_MAT_BINOP(|)
DEFINE_ELEMENTWISE_MAT_BINOP(^)
DEFINE_ELEMENTWISE_MAT_BINOP(<<)
DEFINE_ELEMENTWISE_MAT_BINOP(>>)

DEFINE_ELEMENTWISE_LHS_BINOP(*)
DEFINE_ELEMENTWISE_RHS_BINOP(*)
DEFINE_ELEMENTWISE_RHS_BINOP(/)


template <size_t rows, size_t cols>
using imat = mat<rows, cols, int>;
template <size_t rows, size_t cols>
using dmat = mat<rows, cols, double>;

#define DEFINE_SQUARE_MAT_ALIASES(n)\
using mat##n = mat<n, n>;\
using imat##n = imat<n, n>;\
using dmat##n = dmat<n, n>;\

#define DEFINE_MAT_ALIASES(rows, cols)\
using mat##rows##x##cols = mat<rows, cols>;\
using imat##rows##x##cols = imat<rows, cols>;\
using dmat##rows##x##cols = dmat<rows, cols>;\

DEFINE_SQUARE_MAT_ALIASES(2)
DEFINE_SQUARE_MAT_ALIASES(3)
DEFINE_SQUARE_MAT_ALIASES(4)
DEFINE_SQUARE_MAT_ALIASES(5)
DEFINE_SQUARE_MAT_ALIASES(6)

DEFINE_MAT_ALIASES(1, 2)
DEFINE_MAT_ALIASES(2, 1)
DEFINE_MAT_ALIASES(2, 2)

DEFINE_MAT_ALIASES(1, 3)
DEFINE_MAT_ALIASES(3, 1)
DEFINE_MAT_ALIASES(2, 3)
DEFINE_MAT_ALIASES(3, 2)
DEFINE_MAT_ALIASES(3, 3)

DEFINE_MAT_ALIASES(1, 4)
DEFINE_MAT_ALIASES(4, 1)
DEFINE_MAT_ALIASES(2, 4)
DEFINE_MAT_ALIASES(4, 2)
DEFINE_MAT_ALIASES(3, 4)
DEFINE_MAT_ALIASES(4, 3)
DEFINE_MAT_ALIASES(4, 4)

template <typename T, size_t rows, size_t c1, size_t c2>
DALI_HOST_DEV
mat<rows, c1+c2, T> cat_cols(const mat<rows, c1, T> &a, const mat<rows, c2, T> &b) {
  mat<rows, c1+c2, T> ret;
#if MAT_LAYOUT_ROW_MAJOR
  for (size_t i = 0; i < rows; i++)
    ret.set_row(i, cat(a.row(i), b.row(i)));
#else
  for (size_t j = 0; j < c1; j++)
    ret.set_col(j, a.col(j));
  for (size_t j = 0; j < c2; j++)
    ret.set_col(j+c1, b.col(j));
#endif
  return ret;
}

template <typename T, size_t rows, size_t cols>
DALI_HOST_DEV
mat<rows, cols+1> cat_cols(const mat<rows, cols, T> &a, const vec<rows, T> &v) {
  mat<rows, cols+1, T> ret;
#if MAT_LAYOUT_ROW_MAJOR
  for (size_t i = 0; i < rows; i++)
    ret.set_row(i, cat(a.row(i), v[i]));
#else
  for (size_t j = 0; j < cols; j++)
    ret.set_col(j, a.col(j));
  ret.set_col(cols, v);
#endif
  return ret;
}

template <typename T, size_t rows, size_t cols>
DALI_HOST_DEV
mat<rows, cols+1> cat_cols(const vec<rows, T> &v, const mat<rows, cols, T> &a) {
  mat<rows, cols+1, T> ret;
#if MAT_LAYOUT_ROW_MAJOR
  for (size_t i = 0; i < rows; i++)
    ret.set_row(i, cat(v[i], a.row(i)));
#else
  ret.set_col(0, v);
  for (size_t j = 0; j < cols; j++)
    ret.set_col(j+1, a.col(j));
#endif
  return ret;
}

template <typename T, size_t rows>
DALI_HOST_DEV
mat<rows, 2> cat_cols(const vec<rows, T> &a, const vec<rows, T> &b) {
  mat<rows, 2, T> ret;
#if MAT_LAYOUT_ROW_MAJOR
  for (size_t i = 0; i < rows; i++)
    ret.set_row(i, vec<2, T>(a[i], b[i]));
#else
  ret.set_col(0, a);
  ret.set_col(1, b);
#endif
  return ret;
}

template <size_t r1, size_t r2, size_t cols, typename T>
DALI_HOST_DEV
mat<r1+r2, cols, T> cat_rows(const mat<r1, cols, T> &a, const mat<r2, cols, T> &b) {
  mat<r1+r2, cols, T> ret;
#if MAT_LAYOUT_ROW_MAJOR
  for (size_t i = 0; i < r1; i++)
    ret.set_row(i, a.row(i));
  for (size_t i = 0; i < r2; i++)
    ret.set_row(i+r1, b.row(i));
#else
  for (size_t j = 0; j < cols; j++)
    ret.set_col(j, cat(a.col(j), b.col(j)));
#endif
  return ret;
}

}  // namespace dali

#endif  // DALI_CORE_GEOM_MAT_H_