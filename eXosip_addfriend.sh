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
nickname=$2
home=$3
work=$4
email=$5
e164=$6


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
tmp=`grep -i "ENTRY: $nickname|" $afile`
if test "$tmp"; then
  #find a match!
  id=`echo $tmp | sed 's/^\(.*\) ENTRY:\(.*\)/\1/'`
  grep -v "^"$id $afile > $afile-tmp
  cp $afile-tmp $afile
  # user entry are removed :/
fi

declare num;
num=`grep "ENTRY:" $afile | wc -l`


#echo "Jmsip'user is adding a contact "
# file
if !(test "$nickname"); then
  exit
fi

if !(test "$home"); then
  home="*empty*"
fi

if !(test "$work"); then
  work="*empty*"
fi

if !(test "$email"); then
  email="*empty*"
fi

if !(test "$e164"); then
  e164="*e164*"
fi

echo  $num "ENTRY:" $nickname"|"$home"|"$work"|"$email"|"$e164 >> $afile

