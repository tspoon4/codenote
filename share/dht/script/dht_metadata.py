#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import os
import sys
import json
import time
import bencode
import socket
import binascii
import psycopg2
import hashlib
from datetime import datetime


# In[ ]:


CONFIG = '/mnt/data/script/config.json'

EXT_HANDSHAKE = 0
EXT_METADATA = 2
META_REQUEST = 0
META_DATA = 1
META_REJECT = 2


# In[ ]:


def btr_build_handshake(sha: bytearray, selfid: bytearray) -> bytearray:
    # Fixed size header 68    
    msg = bytearray()
    msg += b'\x13BitTorrent protocol'
    msg += b'\x00\x00\x00\x00\x00\x10\x00\x00'
    msg += sha
    msg += selfid
    return msg

def btr_parse_handshake(data):
    sha, sid, ext = b'', b'', 0
    if data[0:20] == b'\x13BitTorrent protocol':
        ext = data[25]
        sha = data[28:48]
        sid = data[48:68]
    return sha, sid, ext


# In[ ]:


def btr_build_ext_message(msgid: int, body: dict) -> bytearray:
    msg = bytearray()
    msg += b'\x14'
    msg += msgid.to_bytes(1, 'big') 
    msg += bencode.bencode(body)
    return len(msg).to_bytes(4, 'big') + msg

def btr_parse_ext_message(data):
    ext, body = 0, dict()
    if data[0] == 0x14:
        ext = data[1]
        body = bencode.bdecode(data[2:])
    return ext, body


# In[ ]:


def btr_connect(sock, peer, retry):
    result = False
    while retry > 0:
        try:
            sock.connect(peer)
            retry = 0
            result = True
        except: retry -= 1
    return result


# In[ ]:


def btr_select_peers(connection):
    peers = list()
    try:
        cursor = connection.cursor()
        select = """SELECT peer, hash 
                 	FROM dht_peers TABLESAMPLE BERNOULLI(0.2) WHERE NOT EXISTS 
                 		(SELECT NULL FROM dht_torrents 
                 		WHERE dht_torrents.hash = dht_peers.hash)
                 	LIMIT 400"""
        cursor.execute(select)
        result = cursor.fetchall()
        for row in result: 
            peer = bytes(row[0])
            bip = peer[:4]
            bport = peer[4:]
            ip = "{0}.{1}.{2}.{3}".format(bip[0], bip[1], bip[2], bip[3])
            port = int.from_bytes(bport, 'big')
            tup = ((ip, port), bytes(row[1]))
            peers.append(tup)
        connection.commit()
    except: connection.rollback()
    cursor.close()
    return peers


# In[ ]:


def btr_log_hash(connection, sha, infodict):    
    total = 0
    files = 1
    name = str()
    if 'name' in infodict: name = infodict['name']
    if 'length' in infodict: total = infodict['length']
    if 'files' in infodict:
        files = len(infodict['files'])
        for file in infodict['files']: total += file['length']
    try:
        cursor = connection.cursor()
        insert = """INSERT INTO dht_torrents VALUES(CURRENT_TIMESTAMP, %s, %s, %s, %s)"""
        values = (sha, name, files, total)
        cursor.execute(insert, values)
        connection.commit()
        #print(sha, name, files, total)
    except: connection.rollback()
    cursor.close()
    return

def btr_log_crawl(connection, stats):
    try:
        cursor = connection.cursor()
        insert = """INSERT INTO dht_meta VALUES(%s, CURRENT_TIMESTAMP, %s, %s, %s, %s, %s)"""
        values = (stats['start'], stats['total'], stats['timeout'], 
                  stats['errors'], stats['ut_metadata'], stats['torrents_found'])
        cursor.execute(insert, values)
        connection.commit()
    except: connection.rollback()
    cursor.close()
    return


# In[ ]:


def btr_recv_size(sock, size):
    msg = b''
    while size > 0:
        data = sock.recv(size)
        if len(data) == 0: break
        size -= len(data) 
        msg += data        
    return msg

def btr_recv_ext_message(sock):
    data = b''
    while True:
        blen = btr_recv_size(sock, 4)
        length = int.from_bytes(blen, 'big')
        data = btr_recv_size(sock, length)
        #print(length, data)
        if data[0] == 0x14: break
    return data


# In[ ]:


def btr_crawl_peers(connection, selfid, stats):
    peers = btr_select_peers(connection)
    for item in peers:        
        stats['total'] += 1
        peer, sha = item[0], item[1]
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            try:
                sock.settimeout(1)
                if not btr_connect(sock, peer, 2):
                    stats['timeout'] += 1
                    continue
                
                # Exchange handshake                
                sock.settimeout(10)
                msg = btr_build_handshake(sha, selfid)
                sock.send(msg)
                data = btr_recv_size(sock, 68)
                rsha, otherid, ext = btr_parse_handshake(data)
                if sha != rsha or ext & 0x10 == 0: raise

                # Retrieve metadata size
                #print(sha, peer)
                body = {'m': {'ut_metadata': EXT_METADATA}, 'metadata_size': 0}
                msg = btr_build_ext_message(EXT_HANDSHAKE, body)
                sock.send(msg)                
                data = btr_recv_ext_message(sock)
                ext, body = btr_parse_ext_message(data)
                if ext != EXT_HANDSHAKE or 'metadata_size' not in body: raise                                    

                # Request metadata pieces
                #print(body)
                piece = 0
                metadata = bytearray()                
                metasize = body['metadata_size']
                ext_metadata = body['m']['ut_metadata']
                stats['ut_metadata'] += 1
                while metasize > 0:
                    if metasize < 16*1024: piecesize = metasize
                    else: piecesize = 16*1024
                    body = {'msg_type': META_REQUEST, 'piece': piece}
                    msg = btr_build_ext_message(ext_metadata, body)
                    sock.send(msg)
                    data = btr_recv_ext_message(sock)
                    ext, body = btr_parse_ext_message(data[:len(data)-piecesize])                    
                    if body['msg_type'] == META_DATA:
                        metadata += data[len(data)-piecesize:]
                        metasize -= piecesize
                        piece += 1
                    else: break
                if metasize > 0: raise

                # Validate info dictionnary with torrent hash
                tsha = hashlib.sha1(metadata).digest()
                if tsha != sha: raise

                infodict = bencode.bdecode(bytes(metadata))                    
                btr_log_hash(connection, sha, infodict)
                stats['torrents_found'] += 1
            except Exception as e:
                # Unknown errors during retreival
                stats['errors'] += 1            


# In[ ]:


def btr_crawl(connection):    
    stats = {'start': datetime.now(), 'total': 0, 'timeout': 0, 'errors': 0,
             'ut_metadata': 0, 'torrents_found': 0}
    
    btr_crawl_peers(connection, os.urandom(20), stats)        
    btr_log_crawl(connection, stats)


# In[ ]:


def main():    
    with open(CONFIG, 'r') as f:
        config = json.load(f)
        db = config['postgres']

    connection = psycopg2.connect(user=db['user'], password=db['password'], host=db['host'], port=db['port'], database=db['database']) 
    btr_crawl(connection)
    connection.close()
        
    return 0


# In[ ]:


if __name__ == '__main__':
    sys.exit(main())

