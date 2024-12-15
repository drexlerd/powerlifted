#! /usr/bin/env python

from lab.parser import Parser


def coverage(content, props):
    props["coverage"] = int("cost" in props)

def unsolvable(content, props):
    props["unsolvable"] = int("exhausted" in props)

def invalid_plan_reported(content, props):
    props["invalid_plan_reported"] = int("val_plan_invalid" in props)

class BrfsParser(Parser):
    """
    Goal found at: 0.00365
    Total time: 0.003652
    Total plan cost: 11
    Plan length: 11 step(s).
    Expanded 238 state(s).
    Reopened 0 state(s).
    Evaluated 0 state(s).
    Evaluations: 0 state(s).
    Generated 1070 state(s).
    Dead ends: 0 state(s).
    Pruned: 0 state(s).
    Expanded until last jump: 234 state(s).
    Reopened until last jump: 0 state(s).
    Evaluated until last jump: 0 state(s).
    Generated until last jump: 1052 state(s).
    Peak memory usage: 7856 kB
    Number of registered states: 253
    Int hash set load factor: 253/256 = 0.988281
    Int hash set resizes: 8
    Solution found.
    Iteration finished correctly.
    """
    def __init__(self):
        super().__init__()
        self.add_pattern("search_time", r"Total time: (.+)", type=float)
        self.add_pattern("num_expanded", r"Expanded (\d+) state\(s\).", type=int)
        self.add_pattern("num_generated", r"Generated (\d+) state\(s\).", type=int)
        self.add_pattern("num_expanded_until_last_g_layer", r"Expanded until last jump: (\d+) state\(s\).", type=int)  
        self.add_pattern("num_generated_until_last_g_layer", r"Generated until last jump: (\d+) state\(s\).", type=int) # ok
        self.add_pattern("cost", r"Total plan cost: (\d+)", type=int)
        self.add_pattern("length", r"Plan length: (\d+) step\(s\).", type=int)
        self.add_pattern("val_plan_invalid", r"(Plan invalid)", type=str)

        self.add_function(coverage)
        self.add_function(unsolvable)
        self.add_function(invalid_plan_reported)
