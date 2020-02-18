%define soname  1
Name:           nanonl
Version:        1.0
Release:        1
Summary:        Small library for dealing with netlink
License:        BSD-2-Clause
Group:          Productivity/Networking/Other
Url:            https://github.com/SonarWireless/nanonl
Source0:        https://github.com/SonarWireless/nanonl/archive/v%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  gcc
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
This library implements a small set of helper functions for constructing
netlink messages, and looking up netlink attributes (NLAs) within nmessages.

%package     -n %{name}%{soname}
Summary:        Small library for dealing with netlink
Group:          System/Libraries

%description -n %{name}%{soname}
This library implements a small set of helper functions for constructing
netlink messages, and looking up netlink attributes (NLAs) within nmessages.

%package        devel
Summary:        Development files for nanonl
Group:          Development/Libraries/C and C++
Requires:       %{name}%{soname} = %{version}

%description    devel
This package contains header files, and libraries needed to develop
application that use nanonl.

%prep
%setup -q

%build
%cmake -DENABLE_ALL=ON -DCMAKE_INSTALL_LIBDIR=%{_libdir}
%make_build

%install
%make_install

%files -n %{name}%{soname}
%defattr(-,root,root)
%doc LICENSE README.md
%{_libdir}/lib%{name}.so

%files devel
%defattr(-,root,root)
%{_includedir}/nanonl
%{_libdir}/lib%{name}.a
%{_datadir}/pkgconfig/%{name}.pc

%changelog
* Tue Feb  18 2020 me@ys.lc
- Initial release
