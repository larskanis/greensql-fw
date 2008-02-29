#/bin/sh

if `which flex >/dev/null 2>&1`
then
  echo "flex ok"
else
  echo "flex not found. This application is used during compilation."
  exit
fi

if `which bison >/dev/null 2>&1`
then
  echo "bison ok"
else
  echo "bison not found. This application is used during compilation."
  exit
fi

build_deb()
{
  make clean >/dev/null 2>&1
  cd debian
  debuild --linda -us -uc
  rm -rf debian/greensql-fw
  cd ../../
  exit
}

build_bsd()
{
  make clean >/dev/null 2>&1
  make
  cd freebsd
  VIRTDIR=./usr/local
  CURDIR=`pwd`
  rm -rf $VIRTDIR
  mkdir -p $VIRTDIR/sbin
  cp ../greensql-fw                $VIRTDIR/sbin/
  mkdir -p $VIRTDIR/etc/greensql
  cp ../conf/*.conf                $VIRTDIR/etc/greensql/
  mkdir -p $VIRTDIR/etc/rc.d
  cp greensql-fw.sh                $VIRTDIR/etc/rc.d/greensql-fw
  mkdir -p $VIRTDIR/share/doc/greensql-fw
  cp ../docs/greensql-mysql-db.txt $VIRTDIR/share/doc/greensql-fw/
  cp ../docs/tautology.txt         $VIRTDIR/share/doc/greensql-fw/
  cp ../install.txt                $VIRTDIR/share/doc/greensql-fw/
  cp ../license.txt                $VIRTDIR/share/doc/greensql-fw/

  pkg_create -v -c ./COMMENT -d ./DESC -f ./CONTENTS -i ./INSTALL -S $CURDIR -p /usr/local greensql-fw.tbz
  rm -rf ./usr
  mv greensql-fw.tbz ../../
  cd ../../
  echo
  echo "package created ../greensql-fw.tbz"
  exit
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

if `uname | grep -i freebsd >/dev/null 2>&1`
then
  echo "Building FreeBSD package"
  build_bsd
fi

echo "This script could be used to build GreenSQL for Debian/Ubuntu/FreeBSD"
echo "For other OSes you will have to do some hacking"
echo
echo "You can start by running \"make\""
echo
exit;

