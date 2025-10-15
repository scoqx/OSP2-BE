#!/bin/sh

HELPMSG="osp2-be updater
  --help, -h      this menu
  --silent, -s    launch the game without printing to the ingame console"

if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
  echo $HELPMSG
  exit 1
fi

# game launch arguments. change them to your liking.
ARGS="+set sv_pure 0
      +set fs_game osp
      +set ui_cdkeychecked 1
      +set com_introplayed 1
      +set r_ingamevideo 0
      +set com_skipidlogo 1"

CURL='/usr/bin/curl'
url="https://scoqx.github.io/assets/zz-osp-pak8be.pk3"
url_version="https://scoqx.github.io/version.json?v="
url_changelog="https://scoqx.github.io/assets/changelog_be.txt"

SCRPATH=$(dirname "$(realpath $0)")

if [ ! -d "$SCRPATH/osp" ]; then
  echo "osp folder not found, creating one..."
  mkdir "$SCRPATH/osp"
fi

osp_path="$SCRPATH/osp/zz-osp-pak8be.pk3"
version_path="$SCRPATH/.ospver"

version_cmd="$CURL -s $url_version"
changelog_cmd="$CURL -s $url_changelog"
osp_cmd="$CURL -# -o $osp_path $url"

echo -n "last version: "

version=$($version_cmd | jq -r '.version')
echo -e "\033[1m$version\033[0m"

if [ "$version" == "$(cat "$version_path")" ] && [ -e "$osp_path" ]; then
  echo "you're on the latest version"
else
  echo "a new version found, downloading..."
  $osp_cmd

  echo "$version" >> "$SCRPATH/.ospver"

  if [ "$1" != "--silent" ] && [ "$1" != "-s" ]; then
    version=$(echo "$version" | sed -r 's/-/ v/g')
    changelog=$(echo "$($changelog_cmd)" | awk "/$version/,0" | sed -e 's/^/+echo "/g' | sed 's/$/"/g')

    ARGS="${ARGS}
      +echo ^z------------------
      +echo ^z- ^2osp2-be update^z -
      +echo ^z------------------
      $changelog
      +toggleconsole"
  fi
fi

echo -e "\033[1m-- launching the game --\033[0m"
# $SCRPATH/XQ3E_Vulkan.x64 $ARGS
$SCRPATH/quake3e-vulkan.x64 $ARGS
