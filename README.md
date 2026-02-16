Program to convert the EN daq file to ROOT file
====

## Description

## Requirement
[ROOT](https://root.cern.ch/) is required. <br>
You need to install RIBF DAQ software "babirl".
It can be downloaded from https://ribf.riken.jp/RIBFDAQ/index.php?DAQ%2FDownload#fe96a553 <br>
After compile babirl, modify Makefile
~~~
LDFLAGS = -L/home/furuno/opt/babirl/babirl250503/lib
~~~
to the correct directory of the babirl library.

## Installation
Just type `$> make`

## Usage
- ./ana run****.ridf run****.root [Options]

- Options
  - -online: Online mode. When the program reaches at the eond of file,
  it will stand-by and wait for another data.
  - -web [port num]: Enable THttp server. http://aino-1:[port num]
  - -last: Analyze from the last block.
  - -refresh: Auto refresh of the http histograms in every 3 mins.

- Examples
  - Offline analysis: `$> ./ana data/run7777.ridf run7777.root`
  - Online analysis:  `$> ./ana data/run7777.ridf out.root -online -last -web 5920`
  - Useful script for online monitor: `./online.sh`
    It analyzes the latest .bld data with -web 5920 and -last options.