#! /bin/sh

# exit when any command fails
set -e

#-------------------------------------------------------------------------------
# Shell script to update the copyright notice of all oomph-lib-owned files.
#-------------------------------------------------------------------------------

# --------[ IDENTIFY COPYRIGHTED FILES ]----------------------------------------

# Get the current filename; we need to remember to ignore it from the
# grep/ripgrep search rule
current_filename=$(basename "$0")

printf "Searching for ripgrep... "
if command -v rg &>/dev/null; then
    # Ripgrep is found so use that (it's much quicker than grep)
    printf "found!\n"
    echo "Searching for files"
    rg -l -g '!'"$current_filename" -e "Copyright (C) 2006-" -e " Matthias Heil and Andrew Hazel"
    echo "Searching for files"
    oomph_lib_copyrighted_files=$(rg -l -g '!'"$current_filename" -e "Copyright (C) 2006-" -e " Matthias Heil and Andrew Hazel")
else
    printf "not found!\n"
    oomph_lib_copyrighted_files=$(grep . -R -l --exclude="$current_filename" -e "Copyright (C) 2006-" -e " Matthias Heil and Andrew Hazel")
fi

echo "files found"

# --------[ UPDATE COPYRIGHT YEAR ]---------------------------------------------

# The end-year to update the notice to
current_year=$(date +"%Y")

# Update the licence header for each file
for file in $oomph_lib_copyrighted_files; do
    echo "Updating copyright notice for file:" $file
    sed "s/Copyright (C) 2006-.* Matthias Heil and Andrew Hazel/Copyright (C) 2006-$current_year Matthias Heil and Andrew Hazel/g" $file >$file.updated
    mv $file.updated $file
done

# ------------------------------------------------------------------------------
