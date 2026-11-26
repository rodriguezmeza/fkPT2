#!/bin/sh
set -eu
cd "$(dirname "$0")"
cp fkpt2.m fkpt2.1
mandoc -T html fkpt2.1 > fkpt2.html.tmp
mv fkpt2.html.tmp fkpt2.html
