Summary: GreenSQL open source database firewall solution.
Name: greensql-fw
Version: 0.8.2
Release: 1
License: GPL
Group: Applications/Databases
URL: http://www.greensql.net/
Source: http://easynews.dl.sourceforge.net/sourceforge/greensql/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildRequires: mysql-devel, pcre-devel, libevent, flex, bison

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
mkdir -p $RPM_BUILD_ROOT/etc/init.d
mkdir -p $RPM_BUILD_ROOT/usr/sbin

install -s -m 755 greensql-fw $RPM_BUILD_ROOT/usr/sbin/greensql-fw
install -m 0644 conf/greensql.conf $RPM_BUILD_ROOT/etc/greensql/greensql.conf
install -m 0644 conf/mysql.conf $RPM_BUILD_ROOT/etc/greensql/mysql.conf
install -m 0644 scripts/greensql.rotate $RPM_BUILD_ROOT/etc/logrotate.d/greensql
install -m 0755 rpm/greensql-fw.init $RPM_BUILD_ROOT/etc/init.d/greensql-fw
#make DESTDIR=$RPM_BUILD_ROOT

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc license.txt readme.txt docs/greensql-mysql-db.txt docs/tautology.txt
%config /etc/greensql/greensql.conf
%config /etc/greensql/mysql.conf
%config /etc/logrotate.d/greensql

/usr/sbin/greensql-fw
/etc/init.d/greensql-fw

%post
groupadd greensql > /dev/null 2>&1
if ! /usr/bin/id greensql > /dev/null 2>&1 ; then
    useradd -g greensql -s /dev/null greensql
fi
touch /var/log/greensql.log
chown greensql:greensql /var/log/greensql.log
chown greensql:greensql -R /etc/greensql
#echo
#echo "Now, you need to create database used to store GreenSQL configuration.

%postun
chmod 0600 /var/log/greensql.log > /dev/null 2>&1
chown root:root /var/log/greensql.log > /dev/null 2>&1
chmod 0700 /etc/greensql > /dev/null 2>&1
chown root:root /etc/greensql > /dev/null 2>&1
userdel -f greensql > /dev/null 2>&1
groupdel greensql > /dev/null 2>&1
%changelog
