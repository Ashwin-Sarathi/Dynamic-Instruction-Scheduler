# dynamic_scheduler
Simulation of an out of order, 9-stage dynamic instruction scheduler

This project was a part of course ECE 563 at NC State University, under Dr. Eric Rotenburg, for the fall semester of 2023. The goal was to reinforce learnings on out of order execution.
This technique implements in order frontend execution and out of order back end execution.
This project also tested the superscalar nature of the pipeline, having up to 16 instruction wide execution lanes tested.


Instructions to run the project:
1. Type "make" to build.  (Type "make clean" first if you already compiled and want to recompile from scratch.)

2. Run trace reader:

   To run without throttling output:
   ./sim 256 32 4 gcc_trace.txt

   To run with throttling (via "less"):
   ./sim 256 32 4 gcc_trace.txt | less
