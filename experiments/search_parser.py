#! /usr/bin/env python

from lab.parser import Parser


def process_unsolvable(content, props):
    props["unsolvable"] = int("exhausted" in props)

def process_invalid(content, props):
    props["invalid"] = int("invalid" in props)

def add_coverage(content, props):
    if "cost" in props or props.get("unsolvable", 0):
        props["coverage"] = 1
    else:
        props["coverage"] = 0

def compute_total_time(content, props):
    # total_time is translation_time + search_time
    if "translation_time" in props and "search_time" in props:
        props["total_time"] = props["translation_time"] + props["search_time"]

def translate_search_time_to_ms(content, props):
    if "search_time" in props:
        props["search_time"] = int(1000 * props["search_time"])

def translate_total_time_to_ms(content, props):
    if "total_time" in props:
        props["total_time"] = int(1000 * props["total_time"])

def ensure_minimum_times(content, props):
    for attr in ["search_time", "total_time"]:
        time = props.get(attr, None)
        if time is not None:
            props[attr] = max(time, 1)


class SearchParser(Parser):
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
        self.add_pattern("translation_time", r"Total translation time: (.+)s", type=float)
        self.add_pattern("search_time", r"Total time: (.+)", type=float)  # search_time is total time in powerlifted
        self.add_pattern("num_expanded", r"Expanded (\d+) state\(s\).", type=int)
        self.add_pattern("num_generated", r"Generated (\d+) state\(s\).", type=int)
        self.add_pattern("num_expanded_until_last_g_layer", r"Expanded until last jump: (\d+) state\(s\).", type=int)
        self.add_pattern("num_generated_until_last_g_layer", r"Generated until last jump: (\d+) state\(s\).", type=int) # ok
        self.add_pattern("cost", r"Total plan cost: (\d+)", type=int)
        self.add_pattern("length", r"Plan length: (\d+) step\(s\).", type=int)
        self.add_pattern("invalid", r"(Plan invalid)", type=str)

        self.add_function(process_unsolvable)
        self.add_function(process_invalid)
        self.add_function(add_coverage)
        
        self.add_function(compute_total_time) # has to come before translating search_time to ms
        self.add_function(translate_search_time_to_ms)
        self.add_function(translate_total_time_to_ms)
        self.add_function(ensure_minimum_times)