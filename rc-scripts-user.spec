
Summary:	Run user scripts
Summary(pl.UTF-8):	Uruchom skrypty użytkownika
Name:		rc-scripts-user
Version:	1.1
Release:	1
License:	GPL v2+
Group:		Applications/System
Source0:	userscripts
URL:		http://www.pld-linux.org/Packages/UserScripts
BuildRequires:	rpmbuild(macros) >= 1.228
Requires(post,preun):	/sbin/chkconfig
Requires:	rc-scripts
BuildArch:	noarch
BuildRoot:	%{tmpdir}/%{name}-%{version}-root-%(id -u -n)

%description
Run user-defined scripts on system startup and shutdown.

%description -l pl.UTF-8
Uruchom skrypty użytkownika przy starcie oraz wyłączaniu systemu.

%prep
awk '/Version/ { v=$4 } END { exit v != "%{version}" }' < %{SOURCE0}

%install
rm -rf $RPM_BUILD_ROOT
install -d $RPM_BUILD_ROOT/etc/{sysconfig,rc.d/init.d}

install %{SOURCE0} $RPM_BUILD_ROOT/etc/rc.d/init.d/userscripts

cat <<'EOF' > $RPM_BUILD_ROOT/etc/sysconfig/userscripts
# If ALLOWED list is not empty, only users from this list will be able
# to run their scripts. Otherwise anyone with valid login shell will be
# allowed.
ALLOWED=""

# List of users which are never allowed to run scripts:
# BANNED="root guest"
BANNED=""

# userscripts does not wait for user actions to finish, how much time,
# in seconds, should it wait before continuing system shutdown.
STOP_WAIT_TIME=5

# Script priority
NICE=15

EOF

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/chkconfig --add userscripts

%preun
if [ "$1" = "0" ]; then
	%service -q userscripts stop
	/sbin/chkconfig --del userscripts
fi

%files
%defattr(644,root,root,755)
%attr(754,root,root) /etc/rc.d/init.d/userscripts
%config(noreplace) %verify(not md5 mtime size) /etc/sysconfig/userscripts
