#ifndef CONSTRAINT_SOLVER_H
#define CONSTRAINT_SOLVER_H

#include "ConstraintProblem.h"
#include "ConstraintSolution.h"
#include "core/object/ref_counted.h"

// Generic finite-domain constraint solver for procedural generation.
//
// Based on the forward-checking + backtracking design from GameAIPro2 Ch.26
// ("Rolling Your Own Finite-Domain Constraint Solver"), generalized so the
// variable set can grow during search (TOPOLOGY_GROW) in addition to the classic
// fixed-variable case (TOPOLOGY_FIXED).
//
// solve() is synchronous and holds no global/engine state, so it is safe to call
// from a background thread (e.g. via WorkerThreadPool). A step/time budget on the
// ConstraintProblem guarantees it returns instead of hanging on hard problems, and
// a recursion-depth cap turns pathological inputs into a clean failure rather than
// a stack overflow.
//
// One caveat to the background-thread guarantee: ConstraintCallback rules invoke a
// GDScript Callable synchronously on the solving thread, so those callbacks must be
// thread-safe when solve() runs off the main thread. See ConstraintCallback.
class ConstraintSolver : public RefCounted {
	GDCLASS(ConstraintSolver, RefCounted)

protected:
	static void _bind_methods();

public:
	// Solve p_problem. p_seed_override >= 0 replaces the problem's own seed,
	// letting the caller re-run the same problem for different results.
	Ref<ConstraintSolution> solve(const Ref<ConstraintProblem> &p_problem, int p_seed_override = -1);
};

#endif // CONSTRAINT_SOLVER_H
