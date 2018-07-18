#!/bin/bash
FILES=`find . -name '*.cc' -or -name '*.h'`
echo "Source files: `echo "$FILES" | wc -l`"
echo "Lines & Chars: `wc -lm $FILES | tail -n 1`"
