#!/bin/bash

usage()
{
    echo "usage: `basename $0` git_add_files \"git_commit_message\""
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

# git add
# for file in "$@"
# do
#     echo "git add $file"
#     git add $file
# done
while [ $# -gt 1 ]
do
    echo "git add $1"
    git add $1
    shift
done

# git commit
echo "git commit -m \"$(eval echo \$$#)\""
git commit -m "$(eval echo \$$#)"

# git push
echo "git push github master"
git push github master
