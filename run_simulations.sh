#!/bin/bash

# Arguments to pass in
#
# Forwarding strategy
# Frequency of request rate
# ZipfMandelbrot  S and Q (popularity)
# Size of cache (max size)


for protocol in 'Betweeness' 'Probcache' 'CEE'
do
  for s in '0.5' '3'
  do
    for freq in '100' '500' '1000' '2000'
    do
      ./waf --run "scratch/probcache --frequency=$freq --mandelbrot=$s --protocol=$protocol"
    done

    for cachesize in '10' '20' '30' '40'
    do
      ./waf --run "scratch/probcache --cachesize=$cachesize --mandelbrot=$s --protocol=$protocol"
    done
  done
done