#pragma once

#include "aligator/core/function-abstract.hpp"
#include "aligator/core/unary-function.hpp"
#include <proxsuite-nlp/third-party/polymorphic_cxx14.hpp>

namespace aligator {

namespace detail {

template <typename _FunType> struct linear_func_composition_impl : _FunType {
  using FunType = _FunType;
  using Scalar = typename FunType::Scalar;
  ALIGATOR_DYNAMIC_TYPEDEFS(Scalar);
  using BaseData = StageFunctionDataTpl<Scalar>;

  xyz::polymorphic<FunType> func;
  MatrixXs A;
  VectorXs b;

  struct Data : BaseData {
    shared_ptr<BaseData> sub_data;
    Data(const linear_func_composition_impl &ptr)
        : BaseData(ptr.ndx1, ptr.nu, ptr.ndx2, ptr.nr),
          sub_data(ptr.func->createData()) {}
  };

  linear_func_composition_impl(xyz::polymorphic<FunType> func,
                               const ConstMatrixRef A, const ConstVectorRef b)
      : FunType(func->ndx1, func->nu, func->ndx2, (int)A.rows()), func(func),
        A(A), b(b) {
    // if (func == 0) {
    //   ALIGATOR_RUNTIME_ERROR("Underlying function cannot be nullptr.");
    // }
    if (A.rows() != b.rows()) {
      ALIGATOR_RUNTIME_ERROR("Incompatible dimensions: A.rows() != b.rows()");
    }
    if (A.cols() != func->nr) {
      ALIGATOR_RUNTIME_ERROR("Incompatible dimensions: A.cols() != func.nr");
    }
  }

  linear_func_composition_impl(xyz::polymorphic<FunType> func,
                               const ConstMatrixRef A)
      : linear_func_composition_impl(func, A, VectorXs::Zero(A.rows())) {}

  shared_ptr<BaseData> createData() const {
    return std::make_shared<Data>(*this);
  }
};
} // namespace detail

template <typename _Scalar>
struct LinearFunctionCompositionTpl
    : detail::linear_func_composition_impl<StageFunctionTpl<_Scalar>> {
  using Scalar = _Scalar;
  ALIGATOR_DYNAMIC_TYPEDEFS(Scalar);
  using Impl = detail::linear_func_composition_impl<StageFunctionTpl<Scalar>>;
  using Base = typename Impl::FunType;
  using Data = typename Impl::Data;
  using BaseData = StageFunctionDataTpl<Scalar>;

  using Impl::A;
  using Impl::b;
  using Impl::createData;
  using Impl::func;
  using Impl::Impl;

  void evaluate(const ConstVectorRef &x, const ConstVectorRef &u,
                const ConstVectorRef &y, BaseData &data) const;

  void computeJacobians(const ConstVectorRef &x, const ConstVectorRef &u,
                        const ConstVectorRef &y, BaseData &data) const;
};

template <typename _Scalar>
struct LinearUnaryFunctionCompositionTpl
    : detail::linear_func_composition_impl<UnaryFunctionTpl<_Scalar>> {
  using Scalar = _Scalar;
  ALIGATOR_DYNAMIC_TYPEDEFS(Scalar);
  ALIGATOR_UNARY_FUNCTION_INTERFACE(Scalar);
  using Impl = detail::linear_func_composition_impl<Base>;
  using Data = typename Impl::Data;
  using BaseData = StageFunctionDataTpl<Scalar>;

  using Impl::A;
  using Impl::b;
  using Impl::createData;
  using Impl::func;
  using Impl::Impl;

  void evaluate(const ConstVectorRef &x, BaseData &data) const {
    Data &d = static_cast<Data &>(data);

    func->evaluate(x, *d.sub_data);
    data.value_ = b;
    data.value_.noalias() += A * d.sub_data->value_;
  }

  void computeJacobians(const ConstVectorRef &x, BaseData &data) const {
    Data &d = static_cast<Data &>(data);

    func->computeJacobians(x, *d.sub_data);
    data.jac_buffer_.noalias() = A * d.sub_data->jac_buffer_;
  }
};

/// @brief Create a linear composition of the input function @p func.
template <typename Scalar>
auto linear_compose(xyz::polymorphic<StageFunctionTpl<Scalar>> func,
                    const typename math_types<Scalar>::ConstMatrixRef A,
                    const typename math_types<Scalar>::ConstVectorRef b) {
  return LinearFunctionCompositionTpl<Scalar>(func, A, b);
}

/// @copybrief linear_compose This will return a UnaryFunctionTpl.
template <typename Scalar>
auto linear_compose(xyz::polymorphic<UnaryFunctionTpl<Scalar>> func,
                    const typename math_types<Scalar>::ConstMatrixRef A,
                    const typename math_types<Scalar>::ConstVectorRef b) {
  return LinearUnaryFunctionCompositionTpl<Scalar>(func, A, b);
}

} // namespace aligator

#include "aligator/modelling/linear-function-composition.hxx"
