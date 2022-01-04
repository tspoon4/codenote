#!/bin/bash
python3 /mnt/data/script/backup.py --dump
rm /var/log/postgresql/postgresql-12-main.log.*

