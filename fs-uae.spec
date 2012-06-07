Name:           fs-uae
Version:        0.9.7
Release:        1%{?dist}
Summary:        Amiga emulator for games

Group:          Applications/Emulators
License:        GPLv2+
URL:            http://fengestad.no/fs-uae/
Source0:        http://fengestad.no/fs-uae/files/%{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gcc-c++ openal-devel SDL-devel mesa-libGL-devel
BuildRequires:  mesa-libGLU-devel glib2-devel libpng-devel

Autoprov:       0

%description
fs-uae is a multi-platform Amiga emulator based on UAE/WinUAE, with a focus
on emulating floppy-disk and CD-ROM based games.

An important feature of the emulator is that it is fully controllable with
a gamepad from your couch, with an on-screen GUI, which means that you can
easily swap floppies and load save states with your gamepad. fs-uae is well
suited to be started from an emulator frontend running on a HTPC, for
instance.

The emulator has less configuration options than other UAE variants, but
focuses more on default settings which just works. It requires a moderately
fast computer with accelerated graphics (OpenGL) to work.

%prep
%setup -q


%build
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_bindir}/*
%{_libdir}/%{name}/
%{_datadir}/%{name}/
%{_datadir}/doc/%{name}/

%doc



%changelog
