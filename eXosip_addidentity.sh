#!/bin/sh

#  eXosip - This is the eXtended osip library.
#  Copyright (C) 2002, 2003  Aymeric MOIZARD  - jack@atosc.org
#  
#  eXosip is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  eXosip is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

afile=$1
identity=$2
registrar=$3
realm=$4
userid=$5
pwd=$6

if !(test -w $1); then
 mkdir -p `dirname $1`
 touch $1
 if !(test -w $1); then
  # no access permissions
  exit
 fi
fi



# Action 1: remove the entry if it already exist!
#
# if the nick name exist, replace the entry
tmp=`grep -i "ENTRY: $identity|" $afile`
if test "$tmp"; then
  #find a match!
  id=`echo $tmp | sed 's/^\(.*\) ENTRY:\(.*\)/\1/'`
  grep -v "^"$id $afile > $afile-tmp
  cp $afile-tmp $afile
  # user entry are removed :/
fi

declare num;
num=`grep "ENTRY:" $afile | wc -l`


# file
if !(test "$identity"); then
  exit
fi

if !(test "$registrar"); then
  exit
fi

if !(test "$realm"); then
  if !(test "$userid"); then

    if !(test "$pwd"); then
      realm="*empty*"
      userid="*empty*"
      pwd="*empty*"
    else
      exit
    fi
  fi
  if test "$userid"; then
    if test "$pwd"; then
    ole=""
    else
      exit
    fi
  else
    exit
  fi
fi

echo  $num "ENTRY:" $identity"|"$registrar"|"$realm"|"$userid"|"$pwd >> $afile

