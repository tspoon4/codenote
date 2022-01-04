#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import os
import sys
import json
import argparse
import datetime
import subprocess


# In[ ]:


CONFIG = '/mnt/data/script/config.json'
DUMP = 'pg_dump -O -f {0}'
RESTORE = 'psql -f {0}'
GZIP = 'gzip -f {0}'
GUNZIP = 'gzip -df {0}'
COPY = 'cp -f {0} {1}'


# In[ ]:


def __build_path(param: dict) -> str:
    timenow = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    return os.path.join(param['folder'], param['name'].format(timenow))


# In[ ]:


def __list_backups(param: dict) -> list:
    backups = list()
    entries = os.listdir(param['folder'])
    entries.sort()
    for file in reversed(entries):
        if file.endswith('.gz'):
            backups.append(os.path.join(param['folder'], file))
    return backups


# In[ ]:


def main():
    result = 1;
    parser = argparse.ArgumentParser(description='Utility to dump/restore postgres database')
    parser.add_argument('--dump', action='store_true', help='Dumps the database')
    parser.add_argument('--restore', action='store_true', help='Restores the database')
    arguments = parser.parse_args()
    
    with open(CONFIG, 'r') as f:
        config = json.load(f)
        param = config['backup']
        
    if arguments.dump:
        path = __build_path(param)
        subprocess.run(DUMP.format(path).split(), check=True)
        subprocess.run(GZIP.format(path).split(), check=True)
        backups = __list_backups(param)
        history = param['history']
        if len(backups) > history:
            delete = backups[history:]
            for file in delete: os.remove(file)
        result = 0
                
    elif arguments.restore:
        backups = __list_backups(param)
        if len(backups) > 0:
            for item in backups:            
                try:
                    subprocess.run(COPY.format(item, '/tmp/tmp.sql.gz').split(), check=True)
                    subprocess.run(GUNZIP.format('/tmp/tmp.sql.gz').split(), check=True)
                    subprocess.run(RESTORE.format('/tmp/tmp.sql').split(), check=True)
                    result = 0
                    break
                except: pass
                
    else:
        parser.print_help()
        
    return result


# In[ ]:


if __name__ == '__main__':
    sys.exit(main())

