#include "aligator/python/fwd.hpp"
#include "aligator/python/utils.hpp"

#include "aligator/solvers/proxddp/solver-proxddp.hpp"

namespace aligator {
namespace python {

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(prox_run_overloads, run, 1, 4)

void exposeProxDDP() {
  using context::ConstVectorRef;
  using context::Results;
  using context::Scalar;
  using context::TrajOptProblem;
  using context::VectorRef;
  using context::Workspace;

  eigenpy::register_symbolic_link_to_registered_type<
      Linesearch<Scalar>::Options>();
  eigenpy::register_symbolic_link_to_registered_type<LinesearchStrategy>();
  eigenpy::register_symbolic_link_to_registered_type<
      proxsuite::nlp::LSInterpolation>();
  eigenpy::register_symbolic_link_to_registered_type<context::BCLParams>();

  using ProxScaler = ConstraintProximalScalerTpl<Scalar>;
  bp::class_<ProxScaler, boost::noncopyable>("ProxScaler", bp::no_init)
      .def(
          "set_weight",
          +[](ProxScaler &s, Scalar v, std::size_t j) {
            if (j >= s.size()) {
              PyErr_SetString(PyExc_IndexError, "Index out of bounds.");
              bp::throw_error_already_set();
            }
            s.setWeight(v, j);
          },
          ("self"_a, "value", "j"))
      .add_property("size", &ProxScaler::size,
                    "Get the number of constraint blocks.")
      .def(
          "setWeights",
          +[](ProxScaler &s, const ConstVectorRef &w) {
            if (s.size() != std::size_t(w.size())) {
              PyErr_SetString(PyExc_ValueError, "Input has wrong dimension.");
            }
            s.setWeights(w);
          },
          "Vector of weights for each constraint in the stack.")
      .add_property(
          "matrix", +[](ProxScaler &sc) -> ConstVectorRef {
            return sc.diagMatrix().toDenseMatrix();
          });

  bp::def("applyDefaultScalingStrategy", applyDefaultScalingStrategy<Scalar>,
          "scaler"_a, "Apply the default strategy for scaling constraints.");

  bp::class_<Workspace, bp::bases<WorkspaceBaseTpl<Scalar>>,
             boost::noncopyable>(
      "Workspace", "Workspace for ProxDDP.",
      bp::init<const TrajOptProblem &>(("self"_a, "problem")))
      .def(
          "getConstraintScaler",
          +[](const Workspace &ws, std::size_t j) -> const ProxScaler & {
            if (j >= ws.cstr_scalers.size()) {
              PyErr_SetString(PyExc_IndexError, "Index out of bounds.");
              bp::throw_error_already_set();
            }
            return ws.cstr_scalers[j];
          },
          ("self"_a, "j"), bp::return_internal_reference<>(),
          "Scalers of the constraints in the proximal algorithm.")
      .def_readonly("lqr_problem", &Workspace::lqr_problem,
                    "Buffers for the LQ subproblem.")
      .def_readonly("Lxs", &Workspace::Lxs_)
      .def_readonly("Lus", &Workspace::Lus_)
      .def_readonly("Lds", &Workspace::Lds_)
      .def_readonly("Lvs", &Workspace::Lvs_)
      .def_readonly("dxs", &Workspace::dxs)
      .def_readonly("dus", &Workspace::dus)
      .def_readonly("dvs", &Workspace::dvs)
      .def_readonly("dlams", &Workspace::dlams)
      .def_readonly("trial_vs", &Workspace::trial_vs)
      .def_readonly("trial_lams", &Workspace::trial_lams)
      .def_readonly("lams_plus", &Workspace::lams_plus)
      .def_readonly("lams_pdal", &Workspace::lams_pdal)
      .def_readonly("vs_plus", &Workspace::vs_plus)
      .def_readonly("vs_pdal", &Workspace::vs_pdal)
      .def_readonly("shifted_constraints", &Workspace::shifted_constraints)
      .def_readonly("constraintProjJacobians",
                    &Workspace::constraintProjJacobians)
      .def_readonly("inner_crit", &Workspace::inner_criterion)
      .def_readonly("active_constraints", &Workspace::active_constraints)
      .def_readonly("prev_xs", &Workspace::prev_xs)
      .def_readonly("prev_us", &Workspace::prev_us)
      .def_readonly("prev_lams", &Workspace::prev_lams)
      .def_readonly("prev_vs", &Workspace::prev_vs)
      .def_readonly("stage_infeasibilities", &Workspace::stage_infeasibilities)
      .def_readonly("state_dual_infeas", &Workspace::state_dual_infeas)
      .def_readonly("control_dual_infeas", &Workspace::control_dual_infeas)
      .def(PrintableVisitor<Workspace>());

  bp::class_<Results, bp::bases<ResultsBaseTpl<Scalar>>>(
      "Results", "Results struct for proxDDP.",
      bp::init<const TrajOptProblem &>(("self"_a, "problem")))
      .def_readonly("al_iter", &Results::al_iter)
      .def_readonly("lams", &Results::lams)
      .def(PrintableVisitor<Results>());

  using SolverType = SolverProxDDPTpl<Scalar>;

  bp::class_<SolverType, boost::noncopyable>(
      "SolverProxDDP",
      "A proximal, augmented Lagrangian solver, using a DDP-type scheme to "
      "compute "
      "search directions and feedforward, feedback gains."
      " The solver instance initializes both a Workspace and a Results struct.",
      bp::init<Scalar, Scalar, Scalar, std::size_t, VerboseLevel,
               HessianApprox>(("self"_a, "tol", "mu_init"_a = 1e-2,
                               "rho_init"_a = 0., "max_iters"_a = 1000,
                               "verbose"_a = VerboseLevel::QUIET,
                               "hess_approx"_a = HessianApprox::GAUSS_NEWTON)))
      .def_readwrite("bcl_params", &SolverType::bcl_params, "BCL parameters.")
      .def_readwrite("max_refinement_steps", &SolverType::maxRefinementSteps_)
      .def_readwrite("refinement_threshold", &SolverType::refinementThreshold_)
      .def_readwrite("multiplier_update_mode",
                     &SolverType::multiplier_update_mode)
      .def_readwrite("mu_init", &SolverType::mu_init,
                     "Initial AL penalty parameter.")
      .def_readwrite("rho_init", &SolverType::rho_init,
                     "Initial proximal regularization.")
      .def_readwrite("mu_min", &SolverType::mu_lower_bound,
                     "Lower bound on the AL penalty parameter.")
      .def_readwrite(
          "rollout_max_iters", &SolverType::rollout_max_iters,
          "Maximum number of iterations when solving the forward dynamics.")
      .def_readwrite("max_al_iters", &SolverType::max_al_iters,
                     "Maximum number of AL iterations.")
      .def_readwrite("ls_mode", &SolverType::ls_mode, "Linesearch mode.")
      .def_readwrite("rollout_type", &SolverType::rollout_type_,
                     "Rollout type.")
      .def_readwrite("dual_weight", &SolverType::dual_weight,
                     "Dual penalty weight.")
      .def_readwrite("reg_min", &SolverType::reg_min,
                     "Minimum regularization value.")
      .def_readwrite("reg_max", &SolverType::reg_max,
                     "Maximum regularization value.")
      .def("updateLQSubproblem", &SolverType::updateLQSubproblem, "self"_a)
      .def("computeCriterion", &SolverType::computeCriterion, "self"_a,
           "Compute problem stationarity.")
      .def("computeInfeasibilities", &SolverType::computeInfeasibilities,
           ("self"_a, "problem"), "Compute problem infeasibilities.")
      .def(SolverVisitor<SolverType>())
      .def("run", &SolverType::run,
           prox_run_overloads(
               ("self"_a, "problem", "xs_init", "us_init", "lams_init"),
               "Run the algorithm. Can receive initial guess for "
               "multiplier trajectory."));
}

} // namespace python
} // namespace aligator
