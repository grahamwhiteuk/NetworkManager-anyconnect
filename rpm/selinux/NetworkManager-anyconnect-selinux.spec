# current policy created on 3.14.5-44

# Some helpful hints for maintenance...
#
# Generate a custom policy for the binary:
# $ sepolicy generate --init /usr/libexec/nm-anyconnect-service
#
# Rebuild the system policy with the new policy module using the setup script
# created by the previous command:
# $ ./nm_anyconnect_service.sh
#
# Display the denial message(s) for the binary:
# $ ausearch -m AVC -ts recent
#
# Get additional information also using the sealert tool:
# $ sealert
#
# Use the audit2allow tool to suggest changes:
# $ ausearch -m AVC -ts recent | audit2allow -R
#
# Carefully examine the suggested changes and merge with type enforcement file
#
# Reinstall the policy:
# $ ./nm_anyconnect_service.sh
#
# Loop throgh using the binary again, examine ausearch output, merge, etc...


%define relabel_files() \
restorecon -R /usr/libexec/nm-anyconnect-service; \

Name:     NetworkManager-anyconnect-selinux
Version:	1.0.0
Release:	1%{?dist}
Summary:	SELinux policy module for NetworkManager-anyconnect

Group:	  System Environment/Base
License:  GPLv2+
URL:      https://github.com/grahamwhiteuk/Networkmanager-anyconnect
Source0: nm_anyconnect_service.fc
Source1: nm_anyconnect_service.if
Source2: nm_anyconnect_service.te
BuildRequires: pkgconfig(systemd)
BuildRequires: selinux-policy
BuildRequires: selinux-policy-devel
Requires(post): selinux-policy-base >= 3.14.5-44.fc32
Requires(post): libselinux-utils
Requires(post): policycoreutils
Requires(post): policycoreutils-python-utils
BuildArch: noarch

%description
This package installs and sets up the SELinux policy security module for NetworkManager-anyconnect.  This package is
only required on systems that need to use the AnyConnect permissions workaronud script.

%prep
rm -rf %{_builddir}/%{name}-%{version}-%{release}
mkdir %{_builddir}/%{name}-%{version}-%{release}
cd %{_builddir}/%{name}-%{version}-%{release}
cp %{SOURCE0} %{SOURCE1} %{SOURCE2} .

%build
cd %{_builddir}/%{name}-%{version}-%{release}
make -f /usr/share/selinux/devel/Makefile nm_anyconnect_service.pp

%install
cd %{_builddir}/%{name}-%{version}-%{release}
install -d %{buildroot}%{_datadir}/selinux/packages
install -m 644 nm_anyconnect_service.pp %{buildroot}%{_datadir}/selinux/packages
install -d %{buildroot}%{_datadir}/selinux/devel/include/contrib
install -m 644 nm_anyconnect_service.if %{buildroot}%{_datadir}/selinux/devel/include/contrib/
install -d %{buildroot}/etc/selinux/targeted/contexts/users/


%post
semodule -n -i %{_datadir}/selinux/packages/nm_anyconnect_service.pp
if /usr/sbin/selinuxenabled ; then
    /usr/sbin/load_policy
    %relabel_files

fi;
exit 0

%postun
if [ $1 -eq 0 ]; then
    semodule -n -r nm_anyconnect_service
    if /usr/sbin/selinuxenabled ; then
       /usr/sbin/load_policy
       %relabel_files

    fi;
fi;
exit 0

%files
%attr(0600,root,root) %{_datadir}/selinux/packages/nm_anyconnect_service.pp
%{_datadir}/selinux/devel/include/contrib/nm_anyconnect_service.if


%changelog
* Fri Oct 16 2020 Graham White - 1.0.0-1
- initial version
