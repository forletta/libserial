#!/bin/bash
find . -not -path "*/lib/*" \( -name "*.mk" -o -name "Makefile" \) -exec .venv/bin/mbake format --config .bake.toml {} \;
