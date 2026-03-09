#!/bin/bash

# Define the patterns to look for
PATTERNS=(".venv" "__pycache__" ".pytest_cache", ".ruff_cache", ".mypy_cache")
FOUND_ITEMS=()

# Find items matching patterns
for pattern in "${PATTERNS[@]}"; do
    while IFS= read -r line; do
        FOUND_ITEMS+=("$line")
    done < <(find . -name "$pattern" -type d ! -path "./.venv/*" 2>/dev/null)
done

# Check if anything was found
if [ ${#FOUND_ITEMS[@]} -eq 0 ]; then
    echo "No generated Python files found."
    exit 0
fi

# List what was found
echo "The following items will be deleted:"
for item in "${FOUND_ITEMS[@]}"; do
    echo "  $item"
done

# Confirmation prompt
read -p "Are you sure you want to delete these? (y/N): " confirm

if [[ "$confirm" =~ ^[Yy]$ ]]; then
    for item in "${FOUND_ITEMS[@]}"; do
        rm -rf "$item"
        echo "Deleted: $item"
    done
else
    echo "Cleanup cancelled."
fi
