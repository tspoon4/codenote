#!/bin/bash
# Check log in /var/log/cloud-init-output.log

# Install required packages
apt -y update
apt -y install python3-pip
apt -y install postgresql

# Install python pacakges for ubuntu user
su ubuntu -c 'pip3 install bencode.py'
su ubuntu -c 'pip3 install requests'
su ubuntu -c 'pip3 install psycopg2-binary'
su ubuntu -c 'pip3 install Django'
su ubuntu -c 'pip3 install django-sslserver'

# Mount external disk
mkdir /mnt/data
mount /dev/nvme1n1 /mnt/data

# Copy systemd files
cp /mnt/data/boot/mountebs.service /etc/systemd/system
cp /mnt/data/boot/crawling.service /etc/systemd/system
cp /mnt/data/boot/django.service /etc/systemd/system

# Restore database and setup cron jobs for postgres user
su postgres -c 'psql -f /mnt/data/script/alter.sql'
su postgres -c 'python3 /mnt/data/script/backup.py --restore'
su postgres -c 'crontab /mnt/data/boot/cronjobs'

umount /mnt/data

# Start daemons
systemctl daemon-reload
systemctl start mountebs
systemctl enable mountebs
systemctl start crawling
systemctl enable crawling
systemctl start django
systemctl enable django

