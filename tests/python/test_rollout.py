import proxddp

from proxnlp.manifolds import MultibodyPhaseSpace
from proxddp.dynamics import MultibodyFreeFwdDynamics, IntegratorRK2, IntegratorMidpoint

import numpy as np
import pinocchio as pin
import example_robot_data as erd
import pytest
import sys
import matplotlib.pyplot as plt

robot = erd.load("ur5")
rmodel = robot.model
rdata = robot.data
space = MultibodyPhaseSpace(rmodel)
actuation_matrix = np.eye(rmodel.nv)
ode_dynamics = MultibodyFreeFwdDynamics(space, actuation_matrix)
x0 = space.rand()
nsteps = 500
dt = 0.01
times_ = np.linspace(0.0, dt * nsteps, nsteps + 1)

DISPLAY = False


if DISPLAY:
    from pinocchio.visualize import MeshcatVisualizer
    import meshcat_utils as msu

    vizer = MeshcatVisualizer(
        rmodel, robot.collision_model, robot.visual_model, data=rdata
    )
    vizer.initViewer(open=True, loadModel=True)
    viz_util = msu.VizUtil(vizer)
    q0 = pin.neutral(rmodel)
    vizer.display(q0)


def display(xs, us, dt):
    import time

    time.sleep(1)
    for i in range(3):
        viz_util.play_trajectory(xs, us, timestep=dt)


def computeMechanicalEnergy(rmodel, rdata, xs):
    n = len(xs)
    nq = rmodel.nq
    nv = rmodel.nv
    energ = []
    for i in range(n):
        q = xs[i][:nq]
        v = xs[i][nq : nq + nv]
        e = pin.computeMechanicalEnergy(rmodel, rdata, q, v)
        energ.append(e)
    return np.array(energ)


@pytest.fixture(scope="session")
def setup_fig():
    fig = plt.figure()
    plt.figure(dpi=240)
    plt.title("Mechanical energy")
    print("FIGURE:", fig)
    yield fig
    plt.legend()
    plt.savefig("assets/ur5_rollout_energy.png")


def test_rk2(setup_fig):
    discrete_dyn = IntegratorRK2(ode_dynamics, dt)
    u0 = np.zeros(discrete_dyn.nu)
    us = [u0] * nsteps
    xs = proxddp.rollout(discrete_dyn, x0, us).tolist()
    if DISPLAY:
        display(xs, us, dt)
    e = computeMechanicalEnergy(rmodel, rdata, xs)
    plt.plot(times_, e, label="RK2")


def test_midpoint(setup_fig):
    discrete_dyn = IntegratorMidpoint(ode_dynamics, dt)
    u0 = np.zeros(discrete_dyn.nu)
    us = [u0] * nsteps
    xs = proxddp.rollout_implicit(space, discrete_dyn, x0, us).tolist()
    if DISPLAY:
        display(xs, us, dt)
    e = computeMechanicalEnergy(rmodel, rdata, xs)
    plt.plot(times_, e, label="midpoint")


if __name__ == "__main__":
    retcode = pytest.main(sys.argv)