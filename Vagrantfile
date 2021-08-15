# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/xenial32"
  config.vm.box_version = ">=20210804.0.0"

  config.vm.box_check_update = false
  config.vbguest.auto_update = false

  config.vm.network "forwarded_port", guest: 23, host: 23

  config.vm.provider "virtualbox" do |vb|
    vb.gui = false
    vb.memory = "512"
  end

  config.vm.provision "shell", inline: <<-SHELL
    /bin/bash /vagrant/setup_bbs_ubuntu.sh
  SHELL
end
