#/bin/sh

build_deb()
{
  make clean >/dev/null 2>&1
  cd debian
  debuild --linda -us -uc
  rm -rf debian/greensql-fw
  cd ../../
}

if `grep -i ubuntu /etc/issue >/dev/null 2>&1`
then
  echo "Building Ubuntu package"
  build_deb
fi

if `grep -i debian /etc/issue >/dev/null 2>&1`
then
  echo "Building Debian package"
  build_deb
fi

echo "This script could be used to build GreenSQL for Debian/Ubuntu/FreeBSD"
echo "For other OSes you will have to do some hacking"
echo "You can start by running \"make\""

exit;

