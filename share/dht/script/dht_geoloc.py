#!/usr/bin/env python
# coding: utf-8

# In[1]:


import sys
import time
import json
import requests
import psycopg2


# In[ ]:


CONFIG = '/mnt/data/script/config.json'


# In[2]:


def dht_select_subnets(connection) -> list:
    subnets = list()
    try:
        cursor = connection.cursor()
        select = """SELECT COUNT(*) as count, SUBSTRING(peer FOR 3) as subnet
        			FROM dht_peers TABLESAMPLE BERNOULLI(0.2) WHERE NOT EXISTS
        				(SELECT NULL FROM dht_geoip
        				WHERE SUBSTRING(dht_peers.peer FOR 3) = dht_geoip.subnet)
        			GROUP BY subnet ORDER BY count DESC LIMIT 200"""
        cursor.execute(select)
        result = cursor.fetchall()
        for row in result: subnets.append(bytes(row[1]))
        connection.commit()
    except: connection.rollback()
    
    cursor.close()
    return subnets


# In[3]:


def dht_insert_subnet(connection, subnet, country, name):
    try:
        cursor = connection.cursor()
        insert = """INSERT INTO dht_geoip VALUES(CURRENT_TIMESTAMP, %s, %s, %s)"""
        values = (subnet, country, name)
        cursor.execute(insert, values)
        connection.commit()
    except: connection.rollback()
        
    cursor.close()


# In[6]:


def dht_geoloc(connection):
   subnets = dht_select_subnets(connection)
   for sub in subnets:
       url = "https://rdap.apnic.net/ip/{0}.{1}.{2}.{3}".format(sub[0], sub[1], sub[2], 0)        
       try:
           response = requests.get(url, allow_redirects=True)        
           if response.status_code == 200:
               document = response.json()
               country = 'WW'
               name = 'Unavailable'
               if 'country' in document: country = document['country']
               if 'name' in document: name = document['name']
               dht_insert_subnet(connection, sub, country, name)
       except Exception as error:
           print(error)            
       # Don't spam APNIC, wait before sending a new request
       time.sleep(60)


# In[8]:


def main():
    with open(CONFIG, 'r') as f:
        config = json.load(f)
        db = config['postgres']

    connection = psycopg2.connect(user=db['user'], password=db['password'], host=db['host'],port=db['port'],database=db['database'])
    dht_geoloc(connection)
    connection.close()
        
    return 0


# In[ ]:


if __name__ == '__main__':
    sys.exit(main())

