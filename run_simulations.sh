#!/bin/bash

# Arguments to pass in
#
# Forwarding strategy
# Frequency of request rate
# ZipfMandelbrot  S and Q (popularity)
# Size of cache (max size)


for protocol in 'Betweeness' 'Probcache' 'CEE'
do
  for s in '0.5' '1.2'
  do
    for freq in '20' '40' '60' '80' '100'
    do
      ./waf --run "scratch/probcache --frequency=$freq --mandelbrot=$s --protocol=$protocol"
    done

    for cachesize in '5' '10' '15' '20' '25'
    do
      ./waf --run "scratch/probcache --cachesize=$cachesize --mandelbrot=$s --protocol=$protocol"
    done

    for contents in '50 100 150 200 250'
    do
      ./waf --run "scratch/probcache --contents=$contents --mandelbrot=$s --protocol=$protocol"
    done
  done
done