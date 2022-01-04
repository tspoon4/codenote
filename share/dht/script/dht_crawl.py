#!/usr/bin/env python
# coding: utf-8

# In[1]:


import os
import sys
import json
import time
import bencode
import socket
import psycopg2
from datetime import datetime


# In[ ]:


CONFIG = '/mnt/data/script/config.json'


# In[2]:


def dht_build_find_node(selfid, target):
    query = {"t":"wl", "y":"q", "q":"find_node", "a": {"id":selfid, "target":target}}
    return bencode.bencode(query)

def dht_parse_nodes(data):
    nodes = list()
    res = bencode.bdecode(data)
    bnodes = res['r']['nodes']
    for i in range(0, len(bnodes), 26):
        sha = bnodes[i:i+20]
        bip = bnodes[i+20:i+24]
        ip = "{0}.{1}.{2}.{3}".format(bip[0], bip[1], bip[2], bip[3])
        bport = bnodes[i+24:i+26]
        port = int.from_bytes(bport, 'big')
        nodes.append((sha, ip, port))
    return nodes


# In[3]:


def dht_build_sample_infohash(selfid, target):
    query = {"t":"wl", "y":"q", "q":"sample_infohashes", "a": {"id":selfid, "target":target}}
    return bencode.bencode(query)

def dht_parse_infohash(data):
    infohash = list()
    res = bencode.bdecode(data)
    bsamples = res['r']['samples']
    for i in range(0, len(bsamples), 20):
        sha = bsamples[i:i+20]    
        infohash.append(sha)
    return infohash


# In[4]:


def dht_build_get_peers(selfid, infohash):
    query = {"t":"wl", "y":"q", "q":"get_peers", "a": {"id":selfid, "info_hash":infohash}}
    return bencode.bencode(query)

def dht_parse_peers(data):
    peers = list()
    res = bencode.bdecode(data)
    for peer in res['r']['values']:
        peers.append(peer)
    return peers


# In[5]:


def dht_query(sock, peer, msg, retry):
    data = None
    while retry > 0:
        try:
            sock.sendto(msg, peer)
            data, address = sock.recvfrom(1500)
            retry = 0
        except: retry -= 1
    return data


# In[6]:


def dht_log_peer(connection, peer, info, stats):
    try:
        cursor = connection.cursor()
        insert = """INSERT INTO dht_peers VALUES(CURRENT_TIMESTAMP, %s, %s)"""
        values = (peer, info)
        cursor.execute(insert, values)
        connection.commit()
        stats['peers_found'] += 1
    except: connection.rollback()
    cursor.close()    
    
def dht_log_crawl(connection, stats):
    try:
        cursor = connection.cursor()
        insert = """INSERT INTO dht_crawl VALUES(%s, CURRENT_TIMESTAMP, %s, %s, %s, %s, %s, %s)"""
        values = (stats['start'], stats['target'], stats['total'], stats['timeout'], 
                  stats['errors'], stats['sample_ext'], stats['peers_found'])
        cursor.execute(insert, values)
        connection.commit()
    except: connection.rollback()
    cursor.close()


# In[7]:


def dht_crawl(sock, connection, selfid, target, visits):
    clist = {}
    olist = {'bootstrap': ('87.98.162.88', 6881)} #dht.transmissionbt.com
    stats = {'start': datetime.now(), 'target': target, 'timeout': 0, 'errors': 0,
             'sample_ext': 0, 'peers_found': 0}
    
    while len(olist) > 0 and len(clist) < visits:
        count = len(olist)
        sha = list(olist)[0]
        peer = olist[sha]
        olist.pop(sha)
        clist[sha] = peer
        retry = 2 + (10 // count)    
        
        msg = dht_build_find_node(selfid, target)
        data = dht_query(sock, peer, msg, retry)
        if data is None: stats['timeout'] += 1
        else:
            nodes = list()
            try: nodes = dht_parse_nodes(data)
            except: stats['errors'] += 1
            for n in nodes:
                if n[0] not in clist and n[0] not in olist:
                    tmp = {n[0]: (n[1], n[2])}
                    tmp.update(olist)
                    olist = tmp 
            msg = dht_build_sample_infohash(selfid, target)
            data = dht_query(sock, peer, msg, 2)
            if data is not None:
                infohash = list()
                try:
                    infohash = dht_parse_infohash(data)
                    stats['sample_ext'] += 1
                except: pass
                for info in infohash:
                    msg = dht_build_get_peers(selfid, info)
                    data = dht_query(sock, peer, msg, 2)
                    if data is None: break
                    else:
                        peers = list()
                        try: peers = dht_parse_peers(data)
                        except: pass
                        for p in peers:
                            dht_log_peer(connection, p, info, stats)
                            
    stats['total'] = len(clist)
    dht_log_crawl(connection, stats)


# In[8]:


def main():
    with open(CONFIG, 'r') as f:
        config = json.load(f)
        db = config['postgres']
        
    connection = psycopg2.connect(user=db['user'], password=db['password'], host=db['host'], port=db['port'], database=db['database']) 
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(1)
    dht_crawl(sock, connection, os.urandom(20), os.urandom(20), 1000)    
    sock.close()    
    connection.close()
        
    return 0


# In[ ]:


if __name__ == '__main__':
    sys.exit(main())

