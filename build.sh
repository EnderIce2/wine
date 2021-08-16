set -e ## Exit if something goes wrong
clear
echo "EnderIce2's Wine builder"
echo "This process may take a while..."
mkdir -p ../wine_cache/build/wine-32
mkdir -p ../wine_cache/build/wine-64
mkdir -p $HOME/wine-mod
echo "============================================"
echo "                  Syncing"
rsync --progress -r -u ./* ../wine_cache/build/wine-32 ## Sync from wine-master to ./wine-main/wine-32
rsync --progress -r -u ./* ../wine_cache/build/wine-64 ## Sync from wine-master to ./wine-main/wine-64
cd ../wine_cache/build
echo "============================================"
echo "                Configuring"
echo "Configuring wine-32"
cd ./wine-32
./configure -q --prefix=$HOME/wine-mod
echo "Configuring wine-64"
cd ..
cd ./wine-64
./configure -q --prefix=$HOME/wine-mod --enable-win64
cd ..
echo "============================================"
echo "                  Making"
echo "Making wine-32"
cd ./wine-32
make -s -j12
echo "Making wine-64"
cd ..
cd ./wine-64
make -s -j12
cd ..
echo "============================================"
echo "                Installing"
echo "Installing wine-32"
cd ./wine-32
make install >/dev/null
echo "Installing wine-64"
cd ..
cd ./wine-64
make install >/dev/null
echo "============================================"
echo "             Complete Building"
cd ..
cd ..
cd ..
echo "============================================"
FILE32=$HOME/wine-mod/bin/wine
if [ -f "$FILE32" ]; then
    echo "WINE-32 exists."
    else
    echo "WINE-32 not found."
fi
FILE64=$HOME/wine-mod/bin/wine64
if [ -f "$FILE64" ]; then
    echo "WINE-64 exists."
    else
    echo "WINE-64 not found."
fi
echo "                    Done                    "
echo "============================================"
