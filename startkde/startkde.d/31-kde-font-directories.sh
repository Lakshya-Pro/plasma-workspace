# Activate the kde font directories.
#
# There are 4 directories that may be used for supplying fonts for KDE.
#
# There are two system directories. These belong to the administrator.
# There are two user directories, where the user may add her own fonts.
#
# The 'override' versions are for fonts that should come first in the list,
# i.e. if you have a font in your 'override' directory, it will be used in
# preference to any other.
#
# The preference order looks like this:
# user override, system override, X, user, system
#
# Where X is the original font database that was set up before this script
# runs.

usr_odir=$HOME/.fonts/kde-override
usr_fdir=$HOME/.fonts

if test -n "$KDEDIRS"; then
  kdedirs_first=`echo "$KDEDIRS"|sed -e 's/:.*//'`
  sys_odir=$kdedirs_first/share/fonts/override
  sys_fdir=$kdedirs_first/share/fonts
else
  sys_odir=$KDEDIR/share/fonts/override
  sys_fdir=$KDEDIR/share/fonts
fi

# We run mkfontdir on the user's font dirs (if we have permission) to pick
# up any new fonts they may have installed. If mkfontdir fails, we still
# add the user's dirs to the font path, as they might simply have been made
# read-only by the administrator, for whatever reason.

test -d "$sys_odir" && xset +fp "$sys_odir"
test -d "$usr_odir" && (mkfontdir "$usr_odir" ; xset +fp "$usr_odir")
test -d "$usr_fdir" && (mkfontdir "$usr_fdir" ; xset fp+ "$usr_fdir")
test -d "$sys_fdir" && xset fp+ "$sys_fdir"

# Ask X11 to rebuild its font list.
xset fp rehash
