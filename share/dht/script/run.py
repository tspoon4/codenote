#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import gc
import sys
import json
import time
import signal
import subprocess


# In[ ]:


CONFIG = '/mnt/data/script/config.json'
DHT = '/mnt/data/script/dht_crawl.py'
META = '/mnt/data/script/dht_metadata.py'
GEOIP = '/mnt/data/script/dht_geoloc.py'


# In[ ]:


dht = list()
meta = list()
geoip = list() 


# In[ ]:


def __tick_list(processes, script, count):
    for p in list(processes):
        ret = p.poll()
        if ret is not None:
            processes.remove(p)            
        
    spawncount = count - len(processes)
    for i in range(0, spawncount):
        p = subprocess.Popen([sys.executable, script])
        processes.append(p)


# In[ ]:


def __kill_list(processes):
    for p in processes: p.terminate()
    for p in processes: p.wait()    


# In[ ]:


def __term_handler(signum, frame):
    __kill_list(dht)
    __kill_list(meta)
    __kill_list(geoip)
    sys.exit(0)


# In[ ]:


def main():
    signal.signal(signal.SIGTERM, __term_handler)    
    with open(CONFIG, 'r') as f:
        config = json.load(f)
        crawlers = config['crawlers']       
    
    while True:
        __tick_list(dht, DHT, crawlers['dht'])
        __tick_list(meta, META, crawlers['metadata'])
        __tick_list(geoip, GEOIP, crawlers['geoip'])
        time.sleep(crawlers['polltime'])
        gc.collect()
        
    return 1


# In[ ]:


if __name__ == '__main__':
    sys.exit(main())

