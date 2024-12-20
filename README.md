# Powerlifted Planner

Powerlifted is a domain-independent classical planner that uses only lifted
representations.

The planner supports the STRIPS formalism, but extended with inequalities (e.g.,
`(not (= ?x ?y))`), types (e.g., `?b - block`), and object creation (e.g., `:new
?b - block`).

(See [References](#references) for more details.)

## Usage

The `powerlifted.py` script solves a PDDL task provided as input. It also builds
the planner if the `--build` parameter is passed. To run a single search, you
can use the following algorithms:

```$ ./powerlifted.py [-d DOMAIN] -i INSTANCE -s SEARCH -e EVALUATOR -g GENERATOR [ADDITIONAL OPTIONS] [--build]```

The options for each parameter are described below. If you do not pass any value
for `SEARCH`, `EVALUATOR`, and `GENERATOR`, the planner will use the best
(known) configuration for _satisficing_ planning (i.e., no optimality
guaranteed). (See next section for more details.)

It is also possible to perform multiple search algorithms on the same task
iteratively. See the section "Multiple Search Algorithms" below.

You can either use the `build.py` script to build the planner first, or pass the
`--build` flag to build the planner prior to the search execution.

### Best Configuration for Satisficing Planning

Currently, the best configuration for satisficing planning (with respect to
total coverage) is the following:

```$ ./powerlifted.py [-d DOMAIN] -i INSTANCE -s alt-bfws1 -e ff -g yannakakis [ADDITIONAL OPTIONS] --only-effects-novelty-check```

These are also the default values for `-s`, `-e`, and `-g`. To maximize
coverage, we also recommend adding `--unit-cost` (see below) to the `ADDITIONAL
OPTIONS`.

### Available Options for `SEARCH`:
- `alt-bfws1` and `alt-bfws2`: [R_x, h] with w=1 and w=2, respectively. The choice of h is
  given the `EVALUATOR` option. (See Corrêa and Seipp 2022.)
- `astar`: A* Search
- `bfs`: Breadth-First Search (This option was previously called `naive`. You
  can still use `naive` with the `powerlifted.py` script but the planner will internally
  use the new keyword `bfs`.)
- `bfws1` and `bfws2`: Best-First Width Search with w=1 and w=2, respectively.
- `bfws1-rx` and `bfws2-rx`: BFWS(R_x) with w=1 and w=2, respectively. (See Corrêa and Seipp 2022.)
- `dq-bfws1-rx` and `dq-bfws2-rx`: DQ(R_x) with w=1 and w=2, respectively. (See Corrêa and Seipp 2022.)
- `gbfs`: Greedy Best-First Search
- `iw1` and `iw2`: Iterated Width Search (with w=1 and w=2, respectively)
- `lazy`: Lazy Best-First Search
- `lazy-po`: Lazy Best-First Search with Boosted Dual-Queue
- `lazy-prune`: Lazy Best-First Search with pruning of states generated by
non-preferred operators

### Available Options for `EVALUATOR`:
- `add`: The additive heuristic
- `blind`: No Heuristic
- `ff`: The FF heuristic
- `goalcount`: The goal-count/STRIPS heuristic
- `hmax`: The hmax heuristic
- `rff`: The rule-based FF heuristic

### Available Options for `GENERATOR`:
- `join`: Join program using the predicate order given in the PDDL file
- `random_join`: Randomly ordered join program
- `ordered_join`: Join program ordered by the arity of the predicates
- `full_reducer`: Generate successor for acyclic schemas using the full
  reducer method; for cyclic schemas it uses a partial reducer and a join
  program.
- `yannakakis`: Same as above but replaces the final join of the full
      reducer method by the Yannakakis' project-join program.

### Available `ADDITIONAL OPTIONS`:
- `[--novelty-early-stop]`: Flag if the novelty evaluation of a state should
  stop as soon as the return value is defined. (See Corrêa and Seipp 2022.)
- `[--only-effects-novelty-check]`: Flag if the novelty evaluation of a state
  should only consider atoms in the applied action effect. *Warning*: for
  state-of-the-art performance, you must use this option when running BFWS-based
  search engines. (See Corrêa and Seipp 2022.)
- `[--plan-file]`: Name of the file used to output the plan(s) found. If you are
  using sequential iterations, the plan found at the x-th iteration will have
  `.x` appended to its name.
- `[--preprocess-task]`: Preprocess domain and problem PDDL files so it is
  translated into a fragment supported by Powerlifted. You need to have
  [CPDDL](https://gitlab.com/danfis/cpddl) installed, and an environment
  variable `CPDDL_BIN` pointing to the `cpddl/bin` directory after it is locally
  compiled. *Note*: This is still not fully functional. CPDDL still leaves
  negated static precondition on the PDDL file, which Powerlifted does not
  support. Also, it produces two new intermediate files, which now have
  hardcoded names.
- `[--seed RANDOM SEED]`: Random seed for the random number generator.
- `[--translator-output-file TRANSLATOR_FILE]`: Output of the intermediate
  representation to be parsed by the search component will be saved into
  `TRANSLATOR_FILE`. (Default: `output.lifted`)
- `[--unit-cost]`: Use unit cost  (i.e., all costs are equal to 1) instead of
  the costs specified in the domain file.
- `[--validate]`: Runs VAL after a plan is found to validate it. This requires
  [VAL](https://github.com/KCL-Planning/VAL) to be added as `validate` to the `PATH`.

## Multiple Search Algorithms

You can use the flag `--iteration` to specify one single search iteration for
the planner. You can pass as many `--iteration` arguments as you wish, and each
argument will execute a different search.

The syntax to specify a search iteration is the following:

```$ ./powerlifted.py -i INSTANCE --iteration S,E,G```

where `S` is a search algorithm, `E` is an evaluator, and `G` a successor generator. For example, to execute Greedy Best-First Search with FF followed by a Lazy Best-First Search with the additive heuristic (and both using the Yannakakis' algorithm for successor generation), you should run

```$ ./powerlifted.py -i INSTANCE --iteration gbfs,ff,yannakakis --iteration lazy,add,yannakakis```

The plan founds are then numbered based on its iterations. If the first iteration finds a plan, it will be called `plan.1`; the second will be called `plan.2; etc.

Unfortunately, the planner has the limitation that additional options are set
_for all the iterations_.


## Object Creation

Powerlifted supports **object creation** as part of object effects. The precise
semantics are described in [5]. Currently, Powerlifted only supports a
STRIPS-like fragment of the problem where action effects can have the following
form:

```
:effect (:new (?o - some-type) (CONJ-EFFECT))
```

where `CONJ-EFFECT` is a conjunctive PDDL effect (e.g., `(and (p ?x ?y) (p ?y
?o))`) where `?o` occurs. The basic semantics is that variable `?o` must be
instantiated with a fresh object that does not exist on the state the the
(ground) action is applied.

Unfortunately, not all features of the planner support object creation. Here is
a list of features that do support it:
- search engines:
- evaluators:
- successor generators:

See [this repository](https://github.com/abcorrea/object-creation-benchmarks)
for domain examples.

## Running Powerlifted as a Apptainer container

You can also build an Apptainer image to run the planner. This might be useful
in the case where you are not able to compile the planner locally, for
example. You can run the following command to create the planner image:


```apptainer build powerlifted.sif Apptainer.powerlifted```

Be aware that this might take a while. Once the image `powerlifted.sif` is
created, you can run it with the same parameters as the `powerlifted.py`
script.

Note that VAL and CPDDL are not installed in the container, so it is not
possible to use the `--validate` flag with the image, or to use the CPDDL
preprocessor (`--preprocess-task`). However, you can run VAL with the `plan`
file created by the planner after the execution, and use CPDDL as a standalone
to preprocess the task beforehand.

The following command is a usage example on how to run the planner with the
Apptainer image:

```./powerlifted.sif -i /path/to/instance.pddl```

## Requirements
 - A C++17-compliant compiler
 - CMake 3.9+
 - Python 3.5+
 - Boost

## Limitations
 - **Axioms**: not supported
 - **Conditional effects**: not supported
 - **Negated preconditions**: only inequality
 - **Quantifiers**: not supported

 ## References

 1. Corrêa, A. B.; Pommerening, F.; Helmert, M.; and Francès, G. 2020. Lifted Successor Generation using Query Optimization Techniques.
    In Proc. ICAPS 2020, pp. 80-89. [[pdf]](https://abcorrea.github.io/assets/pdf/correa-et-al-icaps2020.pdf)
 2. Corrêa, A. B.; Francès, G.; Pommerening, F.; and Helmert, M. 2021. Delete-Relaxation Heuristics for Lifted Classical Planning.
    In Proc. ICAPS 2021, pp. 94-102. [[pdf]](https://abcorrea.github.io/assets/pdf/correa-et-al-icaps2021.pdf)
 3. Corrêa, A. B.; Pommerening, F.; Helmert, M.; and Francès, G. 2022. The
    FF Heuristic for Lifted Classical Planning.
    In Proc. AAAI 2022. [[pdf]](https://abcorrea.github.io/assets/pdf/correa-et-al-aaai2022.pdf)
 4. Corrêa, A. B.; and Seipp, J. 2022. Best-First Width Search for Lifted
    Classical Planning. In Proc. ICAPS 2022. [[pdf]](https://abcorrea.github.io/assets/pdf/correa-seipp-icaps2022.pdf)
 5. Corrêa, A. B.; De Giacomo, G.; Helmert, M.; and Rubin, S. 2024. Planning with Object Creation.
    In Proc. ICAPS 2024. [[pdf]](https://abcorrea.github.io/assets/pdf/correa-et-al-icaps2024.pdf)
