Summary: GreenSQL open source database firewall solution.
Name: greensql-fw
Vendor: GreenSQL
Version: 1.3.0
Release: 1
License: GPL
Group: Applications/Databases
URL: http://www.greensql.net/
Source: http://easynews.dl.sourceforge.net/sourceforge/greensql/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildRequires: postgresql-devel, pcre-devel, flex, bison, gcc-c++
%if "%{_vendor}" == "redhat"
BuildRequires: libevent-devel
%endif

%if "%{_vendor}" == "suse"
BuildRequires: libmysqlclient-devel
%else
BuildRequires: mysql-devel
%endif

%if "%{_vendor}" == "MandrakeSoft" || "%{_vendor}" == "Mandrakesoft" || "%{_vendor}" == "Mandriva"  || "%{_vendor}" == "mandriva"
BuildRequires: libevent-devel
%else
BuildRequires: libevent
%endif

%if %{_vendor} == "suse"
%if %{suse_version} >= 1110
BuildRequires: libevent-devel
%endif
%endif

%description
GreenSQL is an Open Source database firewall used to protect
databases from SQL injection attacks. GreenSQL works in a 
proxy mode and has built in support for MySQL. The logic is 
based on evaluation of SQL commands using a risk scoring 
matrix as well as blocking known db administrative commands
(DROP, CREATE, etc).

%prep
%setup -q

%build
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/greensql
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
mkdir -p $RPM_BUILD_ROOT/usr/sbin
mkdir -p $RPM_BUILD_ROOT/usr/bin

%ifarch i686 || x86_64
  mkdir -p $RPM_BUILD_ROOT/usr/lib64
%else
  mkdir -p $RPM_BUILD_ROOT/usr/lib
%endif

mkdir -p $RPM_BUILD_ROOT/usr/share/greensql-fw

install -s -m 0755 greensql-fw $RPM_BUILD_ROOT/usr/sbin/greensql-fw
install -m 0755 scripts/greensql-create-db.sh $RPM_BUILD_ROOT/usr/sbin/greensql-create-db
install -m 0644 conf/greensql.conf $RPM_BUILD_ROOT/etc/greensql/greensql.conf
install -m 0644 conf/mysql.conf $RPM_BUILD_ROOT/etc/greensql/mysql.conf
install -m 0644 conf/pgsql.conf $RPM_BUILD_ROOT/etc/greensql/pgsql.conf
install -m 0644 conf/greensql-apache.conf $RPM_BUILD_ROOT/etc/greensql/greensql-apache.conf
install -m 0644 scripts/greensql.rotate $RPM_BUILD_ROOT/etc/logrotate.d/greensql
install -m 0755 rpm/greensql-config $RPM_BUILD_ROOT/usr/sbin/

%ifarch i686 || x86_64
  install -m 0644 src/lib/libgsql-mysql.so.1 $RPM_BUILD_ROOT/usr/lib64/libgsql-mysql.so.1
  install -m 0644 src/lib/libgsql-pgsql.so.1 $RPM_BUILD_ROOT/usr/lib64/libgsql-pgsql.so.1
%else
  install -m 0644 src/lib/libgsql-mysql.so.1 $RPM_BUILD_ROOT/usr/lib/libgsql-mysql.so.1
  install -m 0644 src/lib/libgsql-pgsql.so.1 $RPM_BUILD_ROOT/usr/lib/libgsql-pgsql.so.1
%endif

cp -R greensql-console/* $RPM_BUILD_ROOT/usr/share/greensql-fw/
chmod 755 $RPM_BUILD_ROOT/usr/share/greensql-fw/

%if "%{_vendor}" == "redhat"
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d/
install -m 0755 rpm/greensql-fw.redhat.init $RPM_BUILD_ROOT/etc/rc.d/init.d/greensql-fw
%endif

mkdir -p $RPM_BUILD_ROOT/etc/init.d/
install -m 0755 rpm/greensql-fw.suse.init $RPM_BUILD_ROOT/etc/init.d/greensql-fw

#make DESTDIR=$RPM_BUILD_ROOT

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc license.txt readme.txt docs/greensql-mysql-db.txt docs/greensql-postgresql-db.txt docs/tautology.txt conf/greensql.conf conf/mysql.conf conf/pgsql.conf conf/greensql-apache.conf greensql-console/config.php
%config /etc/greensql/greensql-apache.conf
%config /etc/greensql/greensql.conf
%config /etc/greensql/mysql.conf
%config /etc/greensql/pgsql.conf
%config /etc/logrotate.d/greensql

/usr/sbin/greensql-fw
/usr/sbin/greensql-create-db
/usr/share/greensql-fw*
/usr/sbin/greensql-config

%ifarch i686 || x86_64
  /usr/lib64/libgsql-mysql.so.1
  /usr/lib64/libgsql-pgsql.so.1
%else
  /usr/lib/libgsql-mysql.so.1
  /usr/lib/libgsql-pgsql.so.1
%endif

%if "%{_vendor}" == "redhat"
/etc/rc.d/init.d/greensql-fw
%endif

/etc/init.d/greensql-fw

%dir %attr(0700, root, root) /etc/greensql/

%post
echo ""
echo "run /usr/sbin/greensql-config for setting up configuration"
echo ""

ln -s /usr/sbin/greensql-create-db /usr/bin/greensql-create-db.sh > /dev/null 2>&1 || true

%ifarch i686 || x86_64
  ln -s /usr/lib64/libgsql-mysql.so.1 /usr/lib64/libgsql-mysql.so > /dev/null 2>&1 || true
  ln -s /usr/lib64/libgsql-pgsql.so.1 /usr/lib64/libgsql-pgsql.so > /dev/null 2>&1 || true

  ln -s /usr/lib64/libgsql-mysql.so.1 /usr/lib/libgsql-mysql.so > /dev/null 2>&1 || true
  ln -s /usr/lib64/libgsql-pgsql.so.1 /usr/lib/libgsql-pgsql.so > /dev/null 2>&1 || true
%else
  ln -s /usr/lib/libgsql-mysql.so.1 /usr/lib/libgsql-mysql.so > /dev/null 2>&1 || true
  ln -s /usr/lib/libgsql-pgsql.so.1 /usr/lib/libgsql-pgsql.so > /dev/null 2>&1 || true
%endif

/sbin/chkconfig --add greensql-fw  > /dev/null 2>&1 || true
/sbin/chkconfig greensql-fw on || true

groupadd greensql > /dev/null 2>&1 || true
if ! /usr/bin/id greensql > /dev/null 2>&1 ; then
    useradd -g greensql -s /dev/null greensql > /dev/null 2>&1 || true
fi
touch /var/log/greensql.log || true
chown greensql:greensql /var/log/greensql.log  > /dev/null 2>&1 || true
chown greensql:greensql -R /etc/greensql  > /dev/null 2>&1 || true
#echo
#echo "Now, you need to create database used to store GreenSQL configuration.

ldconfig
%preun
/sbin/chkconfig --del greensql-fw  > /dev/null 2>&1 || true
rm -rf /usr/bin/greensql-create-db.sh

%ifarch i686 || x86_64
  rm -rf /usr/lib64/libgsql-mysql.so
  rm -rf /usr/lib64/libgsql-pgsql.so
  rm -rf /usr/lib/libgsql-mysql.so
  rm -rf /usr/lib/libgsql-pgsql.so
%else
  rm -rf /usr/lib/libgsql-mysql.so
  rm -rf /usr/lib/libgsql-pgsql.so
%endif

%postun
chmod 0600 /var/log/greensql.log > /dev/null 2>&1 || true
chown root:root /var/log/greensql.log > /dev/null 2>&1 || true
chmod 0700 /etc/greensql > /dev/null 2>&1 || true
chown root:root /etc/greensql > /dev/null 2>&1 || true
/usr/sbin/userdel -f greensql > /dev/null 2>&1 || true
/usr/sbin/groupdel greensql > /dev/null 2>&1 || true
ldconfig || true
true

%changelog
