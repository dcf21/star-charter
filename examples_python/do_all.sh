#!/bin/bash
# do_all.sh

mkdir -p output
rm -f output/*

mkdir -p output_config
rm -f output_config/*

for all in do_*.py
do
  echo "[`date`] Working on <${all}>"
  ./${all} &
done

wait
