-- Discard peers that are older than 2 days
DELETE FROM dht_peers WHERE time < (CURRENT_TIMESTAMP - INTERVAL '1 day');

-- Discard logs after 30 days
DELETE FROM dht_crawl WHERE start < (CURRENT_TIMESTAMP - INTERVAL '30 days');
DELETE FROM dht_meta WHERE start < (CURRENT_TIMESTAMP - INTERVAL '30 days');

-- Discard subnets after a year
DELETE FROM dht_geoip WHERE time < (CURRENT_TIMESTAMP - INTERVAL '365 days');

-- Create temporary index for latest entries
DROP INDEX dht_torrents_latest;
SELECT (CURRENT_TIMESTAMP - INTERVAL '1 hour') AS lateststart
\gset
CREATE INDEX dht_torrents_latest ON dht_torrents(time) WHERE time > :'lateststart';
