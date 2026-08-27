# ----------------------------------------------------------------------------
# File        : xilinx_env.sh
# Description : Set up shell environment for Xilinx tools (Vitis / Vivado /
#               xsim) installed under /opt/Xilinx. Bypasses the vendor
#               settings64.sh scripts which reference a stale
#               /home/ubuntu/Xilinx/... path from the original installer
#               user account.
#
#               Source, do not execute:
#                   . scripts/xilinx_env.sh          # bash / zsh
#                   source scripts/xilinx_env.sh     # equivalent
#
#               Override the install root by setting XILINX_INSTALL before
#               sourcing:
#                   XILINX_INSTALL=/some/other/path . scripts/xilinx_env.sh
#
# Author      : Gyusup LEE <gyu2910@waric.co.kr>
# Created     : 2026-08-20
# Copyright   : (c) 2026 Gyusup LEE. All rights reserved.
# ----------------------------------------------------------------------------

XILINX_INSTALL=${XILINX_INSTALL:-/opt/Xilinx/2025.2}

if [ ! -d "$XILINX_INSTALL" ]; then
    echo "xilinx_env: XILINX_INSTALL=$XILINX_INSTALL not found" >&2
    return 1 2>/dev/null || exit 1
fi

export XILINX_INSTALL
export XILINX_VITIS="$XILINX_INSTALL/Vitis"
export XILINX_HLS="$XILINX_INSTALL/Vitis"
export XILINX_VIVADO="$XILINX_INSTALL/Vivado"
export RDI_DATADIR="$XILINX_INSTALL/Vitis/data"
export TCL_LIBRARY="$XILINX_INSTALL/tps/tcl/tcl8.6"

# vitis_hls launcher is missing from the top-level bin/ in this install;
# expose the unwrapped binary path so callers can invoke it directly
# alongside a compatibility symlink to be created in scripts/.
export XILINX_HLS_BIN="$XILINX_VITIS/bin/unwrapped/lnx64.o/vitis_hls"

PATH="$XILINX_VITIS/bin:$XILINX_VIVADO/bin:$PATH"

# Build LD_LIBRARY_PATH using the vendor helper that appends the correct
# distro-specific subdirectory (Ubuntu/24, SuSE, Rhel, ...). Vendor
# ldlibpath.sh only knows Ubuntu 18/20/22/24; on newer releases (e.g.,
# 26.04) it drops the version subdir and vendor-shipped compat libs like
# libncurses.so.5 become invisible. XILINX_UBUNTU_FALLBACK forces the
# closest supported version subdir when needed. Override by setting it
# before sourcing this file.
: "${XILINX_UBUNTU_FALLBACK:=24}"

_ldlibpath_helper="$XILINX_VITIS/bin/ldlibpath.sh"
_vitis_libs="$XILINX_VITIS/lib/lnx64.o"
_vivado_libs="$XILINX_VIVADO/lib/lnx64.o"
if [ -x "$_ldlibpath_helper" ]; then
    _vitis_libs="$("$_ldlibpath_helper" "$_vitis_libs" 2>/dev/null)"
    _vivado_libs="$("$_ldlibpath_helper" "$_vivado_libs" 2>/dev/null)"
fi

# If we are on Ubuntu but the helper did not add a version subdir, splice
# in the fallback (e.g., Ubuntu/24) directly.
if [ -f /etc/os-release ] && grep -qi "^ID=ubuntu" /etc/os-release; then
    _fb_vitis="$XILINX_VITIS/lib/lnx64.o/Ubuntu/$XILINX_UBUNTU_FALLBACK"
    _fb_vivado="$XILINX_VIVADO/lib/lnx64.o/Ubuntu/$XILINX_UBUNTU_FALLBACK"
    if [ -d "$_fb_vitis"  ] && ! printf '%s' "$_vitis_libs"  | grep -q "/Ubuntu/[0-9]"; then
        _vitis_libs="$_fb_vitis:$_vitis_libs"
    fi
    if [ -d "$_fb_vivado" ] && ! printf '%s' "$_vivado_libs" | grep -q "/Ubuntu/[0-9]"; then
        _vivado_libs="$_fb_vivado:$_vivado_libs"
    fi
    unset _fb_vitis _fb_vivado
fi

LD_LIBRARY_PATH="$_vitis_libs:$_vivado_libs:${LD_LIBRARY_PATH:-}"
unset _ldlibpath_helper _vitis_libs _vivado_libs

export PATH LD_LIBRARY_PATH

echo "xilinx_env: XILINX_INSTALL = $XILINX_INSTALL"
echo "  vivado:    $(command -v vivado 2>/dev/null || echo MISSING)"
echo "  vitis:     $(command -v vitis  2>/dev/null || echo MISSING)"
echo "  xsim:      $(command -v xsim   2>/dev/null || echo MISSING)"
echo "  vitis_hls: $XILINX_HLS_BIN (invoke directly)"

if [ ! -e /home/ubuntu/Xilinx ]; then
    echo
    echo "  NOTE: vendor settings64.sh scripts hardcode /home/ubuntu/Xilinx."
    echo "        For a one-time system-wide fix (requires sudo):"
    echo
    echo "          sudo mkdir -p /home/ubuntu"
    echo "          sudo ln -s '$XILINX_INSTALL/..' /home/ubuntu/Xilinx"
    echo
    echo "        (Followed by: source $XILINX_VITIS/settings64.sh)"
fi
