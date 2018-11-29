import subprocess
import math
import os

def read_seeds():
    s = []
    seed_file = open('seeds', 'r')
    for seed in seed_file:
        s.append(seed[:seed.find('\n')])
    return s

def execute_sims(type, events):
    seeds = read_seeds()
    for seed in seeds:
        output_file = 'out.' + type + '.' + events + '.' + seed
        if os.path.isfile(output_file):
            os.remove(output_file)
        paramsfile = "Params" + type + ".txt"
	eventsfile = events + '.txt'
	subprocess.call(["../../../carbmetsim", "Food.txt", "Exercise.txt", paramsfile, eventsfile, seed, output_file])

execute_sims("Diab","Events1")
execute_sims("Diab","Events2")
execute_sims("Diab","Events3")
execute_sims("Diab","Events4")

