/// @file solver-base.hpp
/// @brief Common definitions for all solvers.
#pragma once

#include "proxddp/fwd.hpp"
#include "proxddp/core/traj-opt-problem.hpp"
#include "proxddp/core/solver-results.hpp"
#include "proxddp/utils/exceptions.hpp"

namespace proxddp {

template <typename Scalar>
static const typename math_types<Scalar>::VectorOfVectors DEFAULT_VECTOR;

/// @brief Default-intialize a trajectory to the neutral states for each state
/// space at each stage.
template <typename Scalar>
void xs_default_init(const TrajOptProblemTpl<Scalar> &problem,
                     std::vector<typename math_types<Scalar>::VectorXs> &xs) {
  using Manifold = ManifoldAbstractTpl<Scalar>;
  const std::size_t nsteps = problem.numSteps();
  xs.resize(nsteps + 1);
  for (std::size_t i = 0; i < nsteps; i++) {
    const StageModelTpl<Scalar> &sm = *problem.stages_[i];
    xs[i] = sm.xspace().neutral();
  }
  const Manifold &space = problem.stages_.back()->xspace_next();
  xs[nsteps] = space.neutral();
}

/// @brief Default-initialize a controls trajectory from the neutral element of
/// each control space.
template <typename Scalar>
void us_default_init(const TrajOptProblemTpl<Scalar> &problem,
                     std::vector<typename math_types<Scalar>::VectorXs> &us) {
  using Manifold = ManifoldAbstractTpl<Scalar>;
  const std::size_t nsteps = problem.numSteps();
  us.resize(nsteps);
  for (std::size_t i = 0; i < nsteps; i++) {
    const StageModelTpl<Scalar> &sm = *problem.stages_[i];
    us[i] = sm.uspace().neutral();
  }
}

/// @brief Check the input state-control trajectory is a consistent warm-start
/// for the output.
template <typename Scalar>
void checkTrajectoryAndAssign(
    const TrajOptProblemTpl<Scalar> &problem,
    const typename math_types<Scalar>::VectorOfVectors &xs_init,
    const typename math_types<Scalar>::VectorOfVectors &us_init,
    typename math_types<Scalar>::VectorOfVectors &xs_out,
    typename math_types<Scalar>::VectorOfVectors &us_out) {
  using StageModel = StageModelTpl<Scalar>;
  const std::size_t nsteps = problem.numSteps();
  xs_out.resize(nsteps + 1);
  us_out.resize(nsteps);
  if (xs_init.size() == 0) {
    xs_default_init(problem, xs_out);
  } else {
    if (xs_init.size() != (nsteps + 1)) {
      proxddp_runtime_error("warm-start for xs has wrong size!")
    }
    xs_out = xs_init;
  }
  if (us_init.size() == 0) {
    us_default_init(problem, us_out);
  } else {
    if (us_init.size() != nsteps) {
      proxddp_runtime_error("warm-start for us has wrong size!")
    }
    us_out = us_init;
  }
}

} // namespace proxddp
