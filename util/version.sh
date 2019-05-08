#!/bin/sh

# This script produces a version number for the current git branch,
# based on the number of commits since the last-tagged version, the
# current branch, and any uncommitted changes.

if [ -n "$TRAVIS_PULL_REQUEST" ] || [ -n "$TRAVIS_BRANCH" ]; then
    if [ "$TRAVIS_PULL_REQUEST" = "false" ]; then
        BRANCH="$TRAVIS_BRANCH"
    else
        BRANCH="$TRAVIS_PULL_REQUEST_BRANCH"
    fi
else
    BRANCH="$(git branch | grep \* | sed 's,[() *],,g')"
fi

LASTTAG="$(git tag -l | sed 's/\(.*\)/\1z/' | sort -rn | sed 's/z//' | perl -nle 'print $_  if m/^\d+\.\d+(\.\d+)*(rc)?$/' | head -n 1)"

if [ "$LASTTAG"x = x ]; then
    LASTTAG="0"
    INCREMENT="-0"
else
    INCREMENT=-$(git rev-list "$LASTTAG"..HEAD | wc -l | awk '{print $1}')
    if [ "$INCREMENT"x = "-0"x ]; then
        INCREMENT=""
    fi
fi

BRANCH=-"$BRANCH"
if [ "$BRANCH"x = -masterx ]; then
    BRANCH=""
fi

DIFF=""
if [ "$(git diff | head -n 5)"x != x ]; then

    if false; then
    # determine checksum program to use
    SUM=$(which sha1sum | awk '{print $1}')
    if [ "$SUM"x = x -o "$SUM"x = nox ]; then
		SUM=$(which md5sum | awk '{print $1}')
    fi
    if [ "$SUM"x = x -o "$SUM"x = nox ]; then
		SUM=$(which md5 | awk '{print $1}')
    fi
    fi

    # checksum differences between index and HEAD
    if [ "$SUM"x = x -o "$SUM" = nox ]; then
        DIFF="-dirty"
    else
        DIFF=-"$(git diff | $SUM | awk '{print substr($1,0,8)}')"
    fi

fi

echo $LASTTAG$INCREMENT$BRANCH$DIFF
