#
#  Copyright (C) 2014, 2015 Intel Corporation.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright notice(s),
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice(s),
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
#  EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
#  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# targets for building rpm
version ?= 0.0.0
release ?= 1
arch = $(shell uname -p)
ifeq ($(jemalloc_installed),true)
	rpmbuild_flags += -E '%define jemalloc_installed true'
endif

rpm: memkind-$(version).tar.gz
	rpmbuild $(rpmbuild_flags) $^ -ta

memkind-$(version).spec:
	@echo "$$memkind_spec" > $@
	cat ChangeLog >> $@

.PHONY: rpm

define memkind_spec
Summary: Extention to libnuma for kinds of memory
Name: memkind
Version: $(version)
Release: $(release)
License: See COPYING
Group: System Environment/Libraries
Vendor: Intel Corporation
URL: https://github.com/memkind/memkind
Source0: memkind-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:  gcc-c++
%if %{defined suse_version}
BuildRequires: libnuma-devel
%else
BuildRequires: numactl-devel
%endif
Prefix: %{_prefix}
Prefix: %{_initddir}

%if %{defined suse_version}
%define docdir %{_defaultdocdir}/memkind
%else
%define docdir %{_defaultdocdir}/memkind-%{version}
%endif

%description
The memkind library is a user extensible heap manager built on top of
jemalloc which enables control of memory characteristics and a
partitioning of the heap between kinds of memory.  The kinds of memory
implemented within the library enable the control of NUMA and page
size features.  The jemalloc heap manager has been extended to include
callbacks which modify the parameters for the mmap() system call and
provide a wrapper around the mbind() system call.  The callbacks are
managed by the memkind library which provides both a heap manager and
functions to create new user defined kinds of memory.

%prep

%setup

%package devel
Summary: Extention to libnuma for kinds of memory - development
Group: Development/Libraries

%description devel
The memkind library is a user extensible heap manager built on top of
jemalloc which enables control of memory characteristics and a
partitioning of the heap between kinds of memory.  The kinds of memory
implemented within the library enable the control of NUMA and page
size features.  The jemalloc heap manager has been extended to include
callbacks which modify the parameters for the mmap() system call and
provide a wrapper around the mbind() system call.  The callbacks are
managed by the memkind library which provides both a heap manager and
functions to create new user defined kinds of memory.  The devel 
package installs header files.

%build
test -f configure || ./autogen.sh

pushd jemalloc
test -f configure || %{__autoconf}
%{__mkdir_p} obj
pushd obj
%if %{defined suse_version}
../configure --enable-autogen --with-jemalloc-prefix=je_ --enable-memkind \
             --enable-safe --enable-cc-silence \
             --prefix=%{_prefix} --includedir=%{_includedir} --libdir=%{_libdir} \
             --bindir=%{_bindir} --docdir=%{_docdir} --mandir=%{_mandir} \
             --with-xslroot=/usr/share/xml/docbook/stylesheet/nwalsh/current
%else
../configure --enable-autogen --with-jemalloc-prefix=je_ --enable-memkind \
             --enable-safe --enable-cc-silence \
             --prefix=%{_prefix} --includedir=%{_includedir} --libdir=%{_libdir} \
             --bindir=%{_bindir} --docdir=%{_docdir} --mandir=%{_mandir}
%endif
$(make_prefix)%{__make} $(make_postfix)
%{__make} build_doc
popd
popd

./configure --enable-tls --prefix=%{_prefix} --libdir=%{_libdir} \
    --includedir=%{_includedir} --sbindir=%{_sbindir} \
    --mandir=%{_mandir} --docdir=%{docdir}
$(make_prefix)%{__make} $(make_postfix)

%install
%{__make} DESTDIR=%{buildroot} install
%{__install} -d %{buildroot}/%{_initddir}
%{__install} init.d/memkind %{buildroot}/%{_initddir}/memkind
rm -f %{buildroot}/%{_libdir}/libmemkind.a
rm -f %{buildroot}/%{_libdir}/libmemkind.la
$(extra_install)

%clean

%post devel
/sbin/ldconfig
if [ -x /usr/lib/lsb/install_initd ]; then
    /usr/lib/lsb/install_initd %{_initddir}/memkind
elif [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --add memkind
else
    for i in 3 4 5; do
        ln -sf %{_initddir}/memkind /etc/rc.d/rc${i}.d/S90memkind
    done
    for i in 0 1 2 6; do
        ln -sf %{_initddir}/memkind /etc/rc.d/rc${i}.d/K10memkind
    done
fi
%{_initddir}/memkind restart >/dev/null 2>&1 || :

%preun devel
if [ -z "$1" ] || [ "$1" == 0 ]; then
    %{_initddir}/memkind stop >/dev/null 2>&1 || :
    if [ -x /usr/lib/lsb/remove_initd ]; then
        /usr/lib/lsb/remove_initd %{_initddir}/memkind
    elif [ -x /sbin/chkconfig ]; then
        /sbin/chkconfig --del memkind
    else
        rm -f /etc/rc.d/rc?.d/???memkind
    fi
fi

%postun devel
/sbin/ldconfig

%files devel
%defattr(-,root,root,-)
%{_includedir}/memkind.h
%{_includedir}/hbwmalloc.h
%{_includedir}/memkind_arena.h
%{_includedir}/memkind_default.h
%{_includedir}/memkind_gbtlb.h
%{_includedir}/memkind_hbw.h
%{_includedir}/memkind_hugetlb.h
%{_libdir}/libmemkind.so.0.0.1
%{_libdir}/libmemkind.so.0
%{_libdir}/libmemkind.so
%{_sbindir}/memkind-pmtt
%{_initddir}/memkind
%dir %{docdir}
%doc %{docdir}/README
%doc %{docdir}/COPYING
%doc %{docdir}/VERSION
%doc %{_mandir}/man3/hbwmalloc.3.gz
%doc %{_mandir}/man3/memkind.3.gz
%doc %{_mandir}/man3/memkind_arena.3.gz
%doc %{_mandir}/man3/memkind_default.3.gz
%doc %{_mandir}/man3/memkind_gbtlb.3.gz
%doc %{_mandir}/man3/memkind_hbw.3.gz
%doc %{_mandir}/man3/memkind_hugetlb.3.gz
$(extra_files)

%changelog
endef

export memkind_spec

