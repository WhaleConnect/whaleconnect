
# Script by David Corbett, 2017.
# License: GPL 2+
#
NR == FNR { a[$1] = $2 }
NR >  FNR { if ($1 in a) a[$1] = $2 }
END { for (cp in a) print cp":"a[cp] }
