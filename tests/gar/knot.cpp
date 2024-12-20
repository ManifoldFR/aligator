#include "aligator/gar/lqr-problem.hpp"
#include <proxsuite-nlp/fmt-eigen.hpp>

#include <boost/test/unit_test.hpp>

#include "test_util.hpp"

using namespace aligator::gar;
using Eigen::MatrixXd;
using Eigen::VectorXd;

struct knot_fixture {
  uint nx = 2;
  uint nu = 2;
  knot_t knot;
  knot_fixture() : knot(nx, nu, 0) {
    knot.Q.setRandom();
    knot.R.setRandom();
    knot.q.setRandom();
    knot.r.setRandom();

    knot.A.setZero();
    knot.B.setZero();
    knot.E.setZero();
    knot.f.setZero();
  }
};

BOOST_FIXTURE_TEST_CASE(move, knot_fixture) {

  MatrixXd Q = knot.Q;
  MatrixXd R = knot.R;

  knot_t knot_moved{std::move(knot)};
  BOOST_CHECK_EQUAL(knot_moved.nx, nx);
  BOOST_CHECK_EQUAL(knot_moved.nu, nu);
  BOOST_CHECK(Q == knot_moved.Q);
  BOOST_CHECK(R == knot_moved.R);

  // copy ctor
  knot_t knot_move2 = std::move(knot_moved);
}

BOOST_FIXTURE_TEST_CASE(copy, knot_fixture) {

  knot_t knot2{knot};
  BOOST_CHECK_EQUAL(knot, knot2);
}

BOOST_FIXTURE_TEST_CASE(swap, knot_fixture) {
  using std::swap;
  knot_t knot2 = knot;
  knot2.Q.setIdentity();

  fmt::println("knot.Q:\n{}", knot.Q);
  fmt::println("knot2.Q:\n{}", knot2.Q);

  swap(knot, knot2);

  fmt::println("knot2.Q:\n{}", knot2.Q);
}

BOOST_FIXTURE_TEST_CASE(gen_knot, knot_fixture) {
  knot_t knot2 = generate_knot(nx, nu, 0);
  this->knot = knot2;
}

BOOST_AUTO_TEST_CASE(knot_vec) {
  uint nx = 4;
  uint nu = 2;
  std::vector<knot_t> v;
  v.reserve(10);
  for (int i = 0; i < 10; i++) {
    v.push_back(generate_knot(nx, nu, 0));
  }

  for (size_t i = 0; i < 10; i++) {
    fmt::println("v [{:d}].q = {}", i, v[i].q.transpose());
  }

  std::vector<knot_t> v2 = std::move(v);
  for (size_t i = 0; i < 10; i++) {
    fmt::println("v2[{:d}].q = {}", i, v2[i].q.transpose());
  }

  std::vector<knot_t> vc{v2};
  for (size_t i = 0; i < 10; i++) {
    fmt::println("vc[{:d}].q = {}", i, vc[i].q.transpose());
    BOOST_CHECK_EQUAL(v2[i], vc[i]);
  }
}

auto make_problem() -> problem_t {
  uint nx = 4;
  uint nu = 2;
  std::vector<knot_t> v;
  v.reserve(10);
  for (int i = 0; i < 10; i++) {
    v.push_back(generate_knot(nx, nu, 0));
  }
  problem_t prob{v, nx};
  return prob;
}

BOOST_AUTO_TEST_CASE(problem) {
  BOOST_TEST_MESSAGE("problem");
  auto prob = make_problem();
  fmt::print("Q[0] = \n{}\n", prob.stages[0].Q);

  problem_t prob_copy{prob};

  for (size_t i = 0; i < 10; i++) {
    BOOST_CHECK_EQUAL(prob.stages[i], prob_copy.stages[i]);
  }

  prob_copy.addParameterization(1);
  for (size_t i = 0; i < 10; i++) {
    BOOST_CHECK_EQUAL(prob.stages[i].Q, prob_copy.stages[i].Q);
  }
}
