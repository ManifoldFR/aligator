/// @file
/// @copyright Copyright (C) 2022-2024 LAAS-CNRS, INRIA
#pragma once

#include "aligator/core/stage-model.hpp"
#include "aligator/modelling/state-error.hpp"

namespace aligator {

/**
 * @brief    Trajectory optimization problem.
 * @tparam   Scalar the scalar type.
 *
 * @details  The problem can be written as a nonlinear program:
 * \f[
 *   \begin{aligned}
 *     \min_{\bmx,\bmu}~& \sum_{i=0}^{N-1} \ell_i(x_i, u_i) + \ell_N(x_N)  \\
 *     \subjectto & \varphi(x_i, u_i, x_{i+1}) = 0, \ 0 \leq i < N \\
 *                & g(x_i, u_i) \in \calC_i
 *   \end{aligned}
 * \f]
 */
template <typename _Scalar> struct TrajOptProblemTpl {
  using Scalar = _Scalar;
  using StageModel = StageModelTpl<Scalar>;
  using StageFunction = StageFunctionTpl<Scalar>;
  using UnaryFunction = UnaryFunctionTpl<Scalar>;
  using Data = TrajOptDataTpl<Scalar>;
  using Manifold = ManifoldAbstractTpl<Scalar>;
  using CostAbstract = CostAbstractTpl<Scalar>;
  using ConstraintSet = ConstraintSetTpl<Scalar>;
  using StateErrorResidual = StateErrorResidualTpl<Scalar>;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  using StageConstraint = StageConstraintTpl<Scalar>;
#pragma GCC diagnostic pop

  /**
   * @page trajoptproblem Trajectory optimization problems
   * @tableofcontents
   *
   * # Trajectory optimization
   *
   * The objective of this library is to model and solve optimal control
   * problems (OCPs) of the form
   *
   * \begin{align}
   *     \min_{x,u}~& \int_0^T \ell(x, u)\, dt + \ell_\mathrm{f}(x(T)) \\\\
   *     \subjectto  & \\dot x (t) = f(x(t), u(t)) \\\\
   *                 & g(x(t), u(t)) = 0 \\\\
   *                 & h(x(t), u(t)) \leq 0
   * \end{align}
   *
   * ## Transcription
   * A _transcription_ translates the continuous-time OCP to a discrete-time,
   * finite-dimensional nonlinear program. Aligator allows us to consider
   * transcriptions with implicit discrete dynamics: \begin{aligned}
   *     \min_{\bmx,\bmu}~& J(\bmx, \bmu) = \sum_{i=0}^{N-1} \ell_i(x_i, u_i) +
   * \ell_N(x_N) \\\\
   *     \subjectto  & f(x_i, u_i, x_{i+1}) = 0 \\\\
   *                 & g(x_i, u_i) = 0 \\\\
   *                 & h(x_i, u_i) \leq 0
   * \end{aligned}
   *
   * In aligator, trajectory optimization problems are described using the class
   * TrajOptProblemTpl. Each TrajOptProblemTpl is described by a succession of
   * stages (StageModelTpl) which encompass the set of constraints and the cost
   * function (class CostAbstractTpl) for this stage.
   *
   * Additionally, a TrajOptProblemTpl must provide an initial condition @f$ x_0
   * = \bar{x} @f$, a terminal cost
   * $$
   *    \ell_{\mathrm{f}}(x_N)
   * $$
   * on the terminal state @f$x_N @f$; optionally, a terminal constraint
   * @f$g(x_N) = 0, h(x_N) \leq 0 @f$ on this state may be added.
   *
   * # Stage models
   * A stage model (StageModelTpl) describes a node in the discrete-time optimal
   * control problem: it consists in a running cost function, and a vector of
   * constraints (StageConstraintTpl), the first of which @b must describe
   * system dynamics (through a DynamicsModelTpl).
   *
   * # Example
   *
   * Define and solve an LQR (Python API):
   *
   * @include{lineno} lqr.py
   *
   */

  ALIGATOR_DYNAMIC_TYPEDEFS(Scalar);

  /// Initial condition
  xyz::polymorphic<UnaryFunction> init_constraint_;
  /// Stages of the control problem.
  std::vector<xyz::polymorphic<StageModel>> stages_;
  /// Terminal cost.
  xyz::polymorphic<CostAbstract> term_cost_;
  /// Terminal constraints.
  ConstraintStackTpl<Scalar> term_cstrs_;
  /// Dummy, "neutral" control value.
  VectorXs unone_;

  /// @defgroup ctor1 Constructors with pre-allocated stages

  /// @ingroup ctor1
  TrajOptProblemTpl(xyz::polymorphic<UnaryFunction> init_constraint,
                    const std::vector<xyz::polymorphic<StageModel>> &stages,
                    xyz::polymorphic<CostAbstract> term_cost);

  /// @ingroup ctor1
  /// @brief Constructor for an initial value problem.
  TrajOptProblemTpl(const ConstVectorRef &x0,
                    const std::vector<xyz::polymorphic<StageModel>> &stages,
                    xyz::polymorphic<CostAbstract> term_cost);

  /// @defgroup ctor2 Constructors without pre-allocated stages

  /// @ingroup ctor2
  TrajOptProblemTpl(xyz::polymorphic<UnaryFunction> init_constraint,
                    xyz::polymorphic<CostAbstract> term_cost);

  /// @ingroup ctor2
  /// @brief Constructor for an initial value problem.
  TrajOptProblemTpl(const ConstVectorRef &x0, const int nu,
                    xyz::polymorphic<Manifold> space,
                    xyz::polymorphic<CostAbstract> term_cost);

  bool initCondIsStateError() const { return init_state_error_ != nullptr; }

  /// @brief Add a stage to the control problem.
  void addStage(const xyz::polymorphic<StageModel> &stage);

  /// @brief Get initial state constraint.
  ConstVectorRef getInitState() const {
    if (!initCondIsStateError()) {
      ALIGATOR_RUNTIME_ERROR(
          "Initial condition is not a StateErrorResidual.\n");
    }
    return init_state_error_->target_;
  }

  /// @brief Set initial state constraint.
  void setInitState(const ConstVectorRef &x0) {
    if (!initCondIsStateError()) {
      ALIGATOR_RUNTIME_ERROR(
          "Initial condition is not a StateErrorResidual.\n");
    }
    init_state_error_->target_ = x0;
  }

  /// @brief Add a terminal constraint for the model.
  ALIGATOR_DEPRECATED void addTerminalConstraint(const StageConstraint &cstr);
  void addTerminalConstraint(const xyz::polymorphic<StageFunction> &func,
                             const xyz::polymorphic<ConstraintSet> &set) {
    this->term_cstrs_.pushBack(func, set);
  }
  /// @brief Remove all terminal constraints.
  void removeTerminalConstraints() { term_cstrs_.clear(); }

  std::size_t numSteps() const;

  /// @brief Rollout the problem costs, constraints, dynamics, stage per stage.
  Scalar evaluate(const std::vector<VectorXs> &xs,
                  const std::vector<VectorXs> &us, Data &prob_data,
                  std::size_t num_threads = 1) const;

  /**
   * @brief Rollout the problem derivatives, stage per stage.
   *
   * @param xs State sequence
   * @param us Control sequence
   * @param prob_data Problem data
   * @param num_threads Number of threads to use
   * @param compute_second_order Whether to compute second-order derivatives
   */
  void computeDerivatives(const std::vector<VectorXs> &xs,
                          const std::vector<VectorXs> &us, Data &prob_data,
                          std::size_t num_threads = 1,
                          bool compute_second_order = true) const;

  /// @brief Pop out the first StageModel and replace by the supplied one;
  /// updates the supplied problem data (TrajOptDataTpl) object.
  void replaceStageCircular(const xyz::polymorphic<StageModel> &model);

  bool checkIntegrity() const;

protected:
  /// Pointer to underlying state error residual
  StateErrorResidual *init_state_error_;
};

namespace internal {
template <typename Scalar>
auto problem_last_state_space_helper(const TrajOptProblemTpl<Scalar> &problem) {
  return problem.term_cost_->space;
}

/// Get dimension of problem's last stage/cost function.
template <typename Scalar>
int problem_last_ndx_helper(const TrajOptProblemTpl<Scalar> &problem) {
  return problem_last_state_space_helper(problem)->ndx();
}
} // namespace internal

} // namespace aligator

#ifdef ALIGATOR_ENABLE_TEMPLATE_INSTANTIATION
#include "aligator/core/traj-opt-problem.txx"
#endif
