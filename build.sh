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
  debuild -us -uc
  rm -rf debian/greensql-fw
  cd ../../
  echo
  echo "package created ../ directory"
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
  cp ../scripts/greensql-create-db.sh $VIRTDIR/sbin/
  mkdir -p $VIRTDIR/etc/greensql
  cp ../conf/*.conf                $VIRTDIR/etc/greensql/
  mkdir -p $VIRTDIR/etc/rc.d
  cp greensql-fw.sh                $VIRTDIR/etc/rc.d/greensql-fw
  mkdir -p $VIRTDIR/share/doc/greensql-fw
  cp ../docs/greensql-mysql-db.txt $VIRTDIR/share/doc/greensql-fw/
  cp ../docs/tautology.txt         $VIRTDIR/share/doc/greensql-fw/
  cp ../install.txt                $VIRTDIR/share/doc/greensql-fw/
  cp ../license.txt                $VIRTDIR/share/doc/greensql-fw/
#  mkdir -p $VIRTDIR/www/greensql-fw/
#  cp -R ../../greensql-console/* $VIRTDIR/www/greensql-fw/

  pkg_create -v -d pkg-descr -f pkg-plist -i pkg-install -S $CURDIR -p /usr/local greensql-fw.tbz
  rm -rf ./usr
  mv greensql-fw.tbz ../../
  cd ../../
  echo
  echo "Package created in ../ directory !!!"
  echo "Package file name is ../greensql-fw.tbz"
  exit
}

build_rpm()
{
  if `which rpmbuild >/dev/null 2>&1`
  then
    echo "rpmbuild ok"
  else
    echo "rpmbuild not found. This application is used during package creation."
    exit
  fi

  GREEN_VER=`grep Version rpm/greensql-fw.spec | sed -e "s/^[a-zA-Z]*:\s*//"`
  if [ -d "../greensql-console" ] && [ ! -d "greensql-console" ]; then
    cp -R ../greensql-console .
  fi

  if [ ! -d "../greensql-fw-$GREEN_VER" ]; then
    mkdir ../greensql-fw-$GREEN_VER
  fi

  cp -r ./ ../greensql-fw-$GREEN_VER

  cd ..
  rm -rf greensql-fw-$GREEN_VER.tar.gz 
  tar -czf greensql-fw-$GREEN_VER.tar.gz greensql-fw-$GREEN_VER/
  rpmbuild -ta greensql-fw-$GREEN_VER.tar.gz
  echo ""
  #echo "Look for packages in the following directory /usr/src/packages"
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

if `grep -i -E "suse|fedora|redhat|centos" /etc/issue >/dev/null 2>&1`
then
  echo "Building rpm package (for SuSe/Fedora/Redhat/Centos)"
  build_rpm
fi

if `grep -i -E "suse|fedora|redhat|centos" /etc/rpm/platform >/dev/null 2>&1`
then
  echo "Building rpm package (for SuSe/Fedora/Redhat/Centos)"
  build_rpm
fi

if `uname | grep -i freebsd >/dev/null 2>&1`
then
  echo "Building FreeBSD package"
  build_bsd
fi


echo "This script could be used to build greensql-fw package for:"
echo "Debian/Ubuntu/FreeBSD/RedHat/CentOS/Fedora/Suse"
echo "For other systems you have to do some hacking"
echo
echo "You can start by running: make"
echo
exit

