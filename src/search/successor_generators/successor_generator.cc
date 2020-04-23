#include "successor_generator.h"

#include "../action.h"

#include <cassert>
#include <vector>

using namespace std;

/**
 * Generate successors states for a given state
 *
 * @details For each action schema, we first check if the nullary preconditions
 * are satisfied in the current state. If they are, we instantiate them using
 * the successor generator passed as command line parameter. Then we check if
 * there is any instantiation of the action schema in the given state. If there
 * is none, then two cases are possible:
 *    1. The action schema is not applicable. In this case, we just proceed to
 *    instantiate the next schema; or
 *    2. The action schema is ground. In this case, we simply proceed to check
 *    if the preconditions are satisfied and, if so, apply the ground action. We
 *    need to check applicability here because, if there is no parameter, then
 *    the join in the successor generator was never performed.
 * If there are instantiations, then we simply apply the action effects, since
 * we know the actions are applicable.
 *
 * @attention A lot of duplication in this code. :-)
 *
 * @param actions: list of actions
 * @param state: state being evaluated
 * @param staticInformation: static information of the task
 * @return vector of pairs <State, Action> where state is the successor state and
 * action is the ground action generating it from the current state
 */
const std::vector<std::pair<State, Action>> &
SuccessorGenerator::generate_successors(const std::vector<ActionSchema> &actions,
                                        const State &state,
                                        const StaticInformation &staticInformation)
{

    // TODO Apply nullary effects only once
    successors.clear();
    // Duplicate code from generic join implementation
    for (const ActionSchema &action : actions) {
        bool trivially_inapplicable = false;
        for (size_t i = 0;
             i < action.get_positive_nullary_precond().size() and !trivially_inapplicable;
             ++i) {
            if ((action.get_positive_nullary_precond()[i] and !state.nullary_atoms[i]) or
                (action.get_negative_nullary_precond()[i] and state.nullary_atoms[i])) {
                trivially_inapplicable = true;
            }
        }
        if (trivially_inapplicable) {
            continue;
        }

        Table instantiations = instantiate(action, state, staticInformation);

        if (instantiations.tuples.empty()) {
            // Or there is no applicable instantiation, or the action is ground
            if (action.get_parameters().empty()) {
                // Action is ground
                bool applicable = is_ground_action_applicable(action, state, staticInformation);
                if (!applicable)
                    continue;
                vector<bool> new_nullary_atoms(state.nullary_atoms);
                apply_nullary_effects(action, new_nullary_atoms);
                vector<Relation> new_relation(state.relations);
                apply_ground_action_effects(action, new_relation);
                successors.emplace_back(State(move(new_relation), move(new_nullary_atoms)),
                                        Action(action.get_index(), vector<int>()));
            }
            else {
                // Action not applicable
                continue;
            }
        }
        else {
            vector<bool> new_nullary_atoms(state.nullary_atoms);
            apply_nullary_effects(action, new_nullary_atoms);
            for (const vector<int> &tuple_with_const : instantiations.tuples) {
                // First, order tuple of indices and then apply effects
                vector<int> tuple;
                vector<int> indices;
                for (size_t j = 0; j < instantiations.tuple_index.size(); ++j) {
                    if (instantiations.tuple_index[j] >= 0) {
                        indices.push_back(instantiations.tuple_index[j]);
                        tuple.push_back(tuple_with_const[j]);
                    }
                }
                vector<int> ordered_tuple(tuple.size());
                assert(ordered_tuple.size() == indices.size());
                for (size_t i = 0; i < indices.size(); ++i) {
                    ordered_tuple[indices[i]] = tuple[i];
                }
                vector<Relation> new_relation(state.relations);
                apply_lifted_action_effects(action, tuple, indices, new_relation);
                successors.emplace_back(State(move(new_relation), vector<bool>(new_nullary_atoms)),
                                        Action(action.get_index(), move(ordered_tuple)));
            }
        }
    }
    return successors;
}

void SuccessorGenerator::apply_nullary_effects(const ActionSchema &action,
                                               vector<bool> &new_nullary_atoms) const
{
    /*
     * Loop over positive and negative nullary effects and apply them accordingly
     * to the state.
     */
    for (size_t i = 0; i < action.get_negative_nullary_effects().size(); ++i) {
        if (action.get_negative_nullary_effects()[i])
            new_nullary_atoms[i] = false;
    }
    for (size_t i = 0; i < action.get_positive_nullary_effects().size(); ++i) {
        if (action.get_positive_nullary_effects()[i])
            new_nullary_atoms[i] = true;
    }
}

void SuccessorGenerator::apply_ground_action_effects(const ActionSchema &action,
                                                     vector<Relation> &new_relation) const
{
    for (const Atom &eff : action.get_effects()) {
        GroundAtom ga;
        for (const Argument &a : eff.arguments) {
            // Create ground atom for each effect given the instantiation
            assert(a.constant);
            ga.push_back(a.index);
        }
        assert(eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
        if (eff.negated) {
            // If ground effect is negated, remove it from relation
            new_relation[eff.predicate_symbol].tuples.erase(ga);
        }
        else {
            // If ground effect is not in the state, we add it
            new_relation[eff.predicate_symbol].tuples.insert(ga);
        }
    }
}

void SuccessorGenerator::apply_lifted_action_effects(const ActionSchema &action,
                                                     const vector<int> &tuple,
                                                     const vector<int> &indices,
                                                     vector<Relation> &new_relation)
{
    for (const Atom &eff : action.get_effects()) {
        const GroundAtom &ga = tuple_to_atom(tuple, indices, eff);
        assert(eff.predicate_symbol == new_relation[eff.predicate_symbol].predicate_symbol);
        if (eff.negated) {
            // Remove from relation
            new_relation[eff.predicate_symbol].tuples.erase(ga);
        }
        else {
            if (find(new_relation[eff.predicate_symbol].tuples.begin(),
                     new_relation[eff.predicate_symbol].tuples.end(),
                     ga) == new_relation[eff.predicate_symbol].tuples.end()) {
                // If ground atom is not in the state, we add it
                new_relation[eff.predicate_symbol].tuples.insert(ga);
            }
        }
    }
}

/**
 *    This action generates the ground atom produced by an atomic effect given an instantiation of
 *    its parameters.
 *
 *    @details First, we rearrange the indices. Then, we create the atom based on whether the
 * argument is a constant or not. If it is, then we simply pass the constant value; otherwise we use
 *    the instantiation that we found.
 */
const GroundAtom &SuccessorGenerator::tuple_to_atom(const vector<int> &tuple,
                                                    const vector<int> &indices,
                                                    const Atom &eff)
{

    vector<int> ordered_tuple(tuple.size(), -1);
    assert(tuple.size() == indices.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        ordered_tuple[indices[i]] = tuple[i];
    }

    ground_atom.clear();
    ground_atom.reserve(eff.arguments.size());
    for (size_t i = 0; i < eff.arguments.size(); i++) {
        if (!eff.arguments[i].constant)
            ground_atom.push_back(ordered_tuple[eff.arguments[i].index]);
        else
            ground_atom.push_back(eff.arguments[i].index);
    }

    // Sanity check: check that all positions of the tuple were initialized
    assert(find(ground_atom.begin(), ground_atom.end(), -1) == ground_atom.end());

    return ground_atom;
}

/*
 * Check the applicability of an already ground action (given grounded in the
 * PDDL). We just need to check applicability for completely ground actions
 * because the successor generations find only applicable actions for lifted
 * ones.
 *
 * In this case, the parameter type is slightly misleading, but the parameter
 * 'action' is a ground action here.
 */
bool SuccessorGenerator::is_ground_action_applicable(const ActionSchema &action,
                                                     const State &state,
                                                     const StaticInformation &staticInformation)
{
    for (const Atom &precond : action.get_precondition()) {
        int index = precond.predicate_symbol;
        vector<int> tuple;
        tuple.reserve(precond.arguments.size());
        for (const Argument &arg : precond.arguments) {
            assert(arg.constant);
            tuple.push_back(arg.index);  // Index of a constant is the obj index
        }
        if (!state.relations[index].tuples.empty()) {
            if (precond.negated) {
                if (state.relations[index].tuples.find(tuple) !=
                    state.relations[index].tuples.end())
                    return false;
            }
            else {
                if (state.relations[index].tuples.find(tuple) ==
                    state.relations[index].tuples.end())
                    return false;
            }
        }
        else if (!staticInformation.relations[index].tuples.empty()) {
            if (precond.negated) {
                if (staticInformation.relations[index].tuples.find(tuple) !=
                    staticInformation.relations[index].tuples.end())
                    return false;
            }
            else {
                if (staticInformation.relations[index].tuples.find(tuple) ==
                    staticInformation.relations[index].tuples.end())
                    return false;
            }
        }
        else {
            return false;
        }
    }
    return true;
}
