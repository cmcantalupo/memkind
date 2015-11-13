#!/bin/bash
echo '#####     SC15 MCDRAM Tutorials Bootstrap Script      #####'
echo '##### Report issues to: sc15tutorial.mcdram@gmail.com #####'
set -x
# Install KDE
yum -y groupinstall 'KDE Plasma Workspaces'

# Install EPEL
yum -y install epel-release

# Add OpenHPC yum repository
ohpc_repo=OpenHPC:1.0.repo
ohpc_url=http://build.openhpc.community/OpenHPC:/1.0/CentOS_7.1/$ohpc_repo
if [ ! -f /etc/yum.repos.d/$ohpc_repo ]; then
    wget -P /etc/yum.repos.d $ohpc_url
fi

# Install OpenHPC base groups
yum -y groupinstall ohpc-base ohpc-autotools

# Install Intel compilers and VTune
yum -y install gnu-compilers-ohpc \
               intel-compilers-devel-ohpc \
               intel-vtune-ohpc \
               intel-mpi-ohpc \
               intel-mpi-devel-ohpc

# Install memkind 0.3.0
if [ ! -f /etc/yum.repos.d/home:cmcantalupo.repo ]; then
    wget -P /etc/yum.repos.d http://download.opensuse.org/repositories/home:/cmcantalupo/CentOS_7/home:cmcantalupo.repo
fi
yum install -y memkind-devel

# Install VBoxGuestAdditions (requires VBoxGuestAdditions.iso as /dev/cdrom)
yum install -y gcc kernel-devel-`uname -r` dkms
mount /dev/cdrom /mnt/cdrom
if [ -f /mnt/cdrom/VBoxLinuxAdditions.run ]; then
    pushd /mnt/cdrom
    sh ./VBoxLinuxAdditions.run
    popd
fi

# Install memkind build requirements
yum install -y git numactl-devel automake libtool rpmbuild

# Set default runlevel to 5
systemctl set-default graphical.target

# Reboot
shutdown -r now
