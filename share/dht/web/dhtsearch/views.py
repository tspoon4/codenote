from django.shortcuts import render
from django.http import HttpResponse
from django.template import loader
from django.db import connection

import binascii
from datetime import datetime
from datetime import timedelta


PEERS_DEFAULT = '00'
SEARCH_DEFAULT = 'Input at least 3 characters'

SEARCH_ORDER = ['count DESC', 'dht_torrents.hash ASC', 'dht_torrents.name ASC', 'dht_torrents.time DESC', 'dht_torrents.size DESC', 'dht_torrents.files DESC']
PEERS_ORDER = ['count DESC', 'dht_geoip.country ASC', 'dht_geoip.name ASC']

SQL_TOP24 = 'SELECT tmp.count, tmp.hash, name, time, size, files FROM (SELECT COUNT(*) AS count, hash FROM dht_peers GROUP BY hash ORDER BY {0} LIMIT 400) AS tmp JOIN dht_torrents ON tmp.hash = dht_torrents.hash LIMIT 100'
SQL_LATEST = "SELECT count, tmp2.hash, name, time, size, files FROM (SELECT COUNT(*) AS count, tmp.hash FROM (SELECT hash FROM dht_torrents WHERE time > %s ORDER BY time DESC LIMIT 100) AS tmp JOIN dht_peers ON tmp.hash = dht_peers.hash GROUP BY tmp.hash) AS tmp2 JOIN dht_torrents ON tmp2.hash = dht_torrents.hash ORDER BY {0}"
SQL_SEARCH = 'SELECT COUNT(*) AS count, dht_torrents.hash, name, dht_torrents.time, size, files FROM dht_torrents LEFT OUTER JOIN dht_peers ON dht_torrents.hash = dht_peers.hash WHERE name LIKE %s GROUP BY dht_torrents.hash ORDER BY {0} LIMIT 100 OFFSET %s'
SQL_SEARCH_HASH = 'SELECT COUNT(*) AS count, dht_torrents.hash, name, dht_torrents.time, size, files FROM dht_torrents LEFT OUTER JOIN dht_peers ON dht_torrents.hash = dht_peers.hash WHERE dht_torrents.hash = %s GROUP BY dht_torrents.hash'
SQL_PEERS = 'SELECT COUNT(*) AS count, country, name FROM dht_geoip JOIN dht_peers ON dht_geoip.subnet = SUBSTRING(dht_peers.peer FOR 3) WHERE dht_peers.hash = %s GROUP BY country, name ORDER BY {0} LIMIT 100 OFFSET %s'
SQL_STAT_CRAWL = 'SELECT SUM(total), SUM(timeout), SUM(sample_ext), SUM(peers_found) FROM dht_crawl WHERE start > (CURRENT_TIMESTAMP - INTERVAL %s)'
SQL_STAT_META = 'SELECT SUM(total), SUM(timeout), SUM(ut_metadata), SUM(torrents_found) FROM dht_meta WHERE start > (CURRENT_TIMESTAMP - INTERVAL %s)'


def index(request):
	template = loader.get_template('dhtsearch/index.html')
	context = {}
	return HttpResponse(template.render(context, request))


def stats(request):
	with connection.cursor() as cursor:
		cursor.execute(SQL_STAT_CRAWL, ['1 day'])
		crawl_day = cursor.fetchone()
		cursor.execute(SQL_STAT_CRAWL, ['1 week'])
		crawl_week = cursor.fetchone()
		cursor.execute(SQL_STAT_CRAWL, ['1 month'])
		crawl_month = cursor.fetchone()
		cursor.execute(SQL_STAT_META, ['1 day'])
		meta_day = cursor.fetchone()
		cursor.execute(SQL_STAT_META, ['1 week'])
		meta_week = cursor.fetchone()
		cursor.execute(SQL_STAT_META, ['1 month'])
		meta_month = cursor.fetchone()

	context = dict()
	context['dht_total'] = [crawl_day[0], crawl_week[0], crawl_month[0]]
	context['dht_timeout'] = [crawl_day[1], crawl_week[1], crawl_month[1]]
	context['dht_sample_ext'] = [crawl_day[2], crawl_week[2], crawl_month[2]]
	context['dht_peers'] = [crawl_day[3], crawl_week[3], crawl_month[3]]
	context['peers_total'] = [meta_day[0], meta_week[0], meta_month[0]]
	context['peers_timeout'] = [meta_day[1], meta_week[1], meta_month[1]]
	context['peers_metadata'] = [meta_day[2], meta_week[2], meta_month[2]]
	context['peers_torrents'] = [meta_day[3], meta_week[3], meta_month[3]]

	template = loader.get_template('dhtsearch/stats.html')
	return HttpResponse(template.render(context, request))


def top24(request):
	with connection.cursor() as cursor:
		cursor.execute(SQL_TOP24.format(SEARCH_ORDER[0]))
		results = cursor.fetchall()

	torrents = list()
	for row in results:
		row1 = binascii.hexlify(row[1]).decode()
		entry = (row[0], row1, row[2], row[3], row[4], row[5])
		torrents.append(entry)

	template = loader.get_template('dhtsearch/full.html')
	context = { 'torrents': torrents }
	return HttpResponse(template.render(context, request))


def latest(request):
	with connection.cursor() as cursor:
		onehourago = datetime.now() - timedelta(hours=1)
		cursor.execute(SQL_LATEST.format(SEARCH_ORDER[3]), [str(onehourago)])
		results = cursor.fetchall()

	torrents = list()
	for row in results:
		row1 = binascii.hexlify(row[1]).decode()
		entry = (row[0], row1, row[2], row[3], row[4], row[5])
		torrents.append(entry)

	template = loader.get_template('dhtsearch/full.html')
	context = { 'torrents': torrents }
	return HttpResponse(template.render(context, request))



def search(request):
	with connection.cursor() as cursor:
		query = request.GET.get('q', SEARCH_DEFAULT)
		page = int(request.GET.get('p', '0'))
		order = int(request.GET.get('o', '0'))
		
		results = list()
		if len(query) == 40:
			try:
				test = binascii.a2b_hex(query)
				hashinfo = "\\x" + query
				cursor.execute(SQL_SEARCH_HASH, [hashinfo])
				results = cursor.fetchall()
			except: pass
		
		if len(results) == 0:
			if len(query) < 3: query = SEARCH_DEFAULT
			if order >= len(SEARCH_ORDER): order = 0
			name = "%" + query + "%"
			offset = page * 100
			cursor.execute(SQL_SEARCH.format(SEARCH_ORDER[order]), [name, offset])
			results = cursor.fetchall()

	torrents = list()
	for row in results:
		row1 = binascii.hexlify(row[1]).decode()
		entry = (row[0], row1, row[2], row[3], row[4], row[5])
		torrents.append(entry)

	template = loader.get_template('dhtsearch/search.html')
	qstring = { 'query': query, 'page': page, 'order': order }
	context = { 'qstring': qstring, 'torrents': torrents, 'range': range(0, 20) }
	return HttpResponse(template.render(context, request))


def peers(request):
	with connection.cursor() as cursor:
		query = request.GET.get('q', PEERS_DEFAULT)
		page = int(request.GET.get('p', '0'))
		order = int(request.GET.get('o', '0'))

		try: binascii.unhexlify(query)
		except: query = PEERS_DEFAULT
		if order >= len(PEERS_ORDER): order = 0
		hashinfo = "\\x" + query
		offset = page * 100
		cursor.execute(SQL_PEERS.format(PEERS_ORDER[order]), [hashinfo, offset])
		results = cursor.fetchall()

	template = loader.get_template('dhtsearch/peers.html')
	qstring = { 'query': query, 'page': page, 'order': order }
	context = { 'qstring': qstring, 'peers': results, 'range': range(0, 20) }
	return HttpResponse(template.render(context, request))


def error_handler(request, exception=None):
	return HttpResponse('', status=403)


