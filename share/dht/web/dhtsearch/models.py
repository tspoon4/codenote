# This is an auto-generated Django model module.
# You'll have to do the following manually to clean this up:
#   * Rearrange models' order
#   * Make sure each model has one field with primary_key=True
#   * Make sure each ForeignKey and OneToOneField has `on_delete` set to the desired behavior
#   * Remove `managed = False` lines if you wish to allow Django to create, modify, and delete the table
# Feel free to rename the models, but don't rename db_table values or field names.
from django.db import models


class DhtCrawl(models.Model):
    start = models.DateTimeField()
    stop = models.DateTimeField()
    target = models.BinaryField()
    total = models.IntegerField()
    timeout = models.IntegerField()
    errors = models.IntegerField()
    sample_ext = models.IntegerField()
    peers_found = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'dht_crawl'


class DhtGeoip(models.Model):
    time = models.DateTimeField()
    subnet = models.BinaryField(primary_key=True)
    country = models.CharField(max_length=2)
    name = models.CharField(max_length=64)

    class Meta:
        managed = False
        db_table = 'dht_geoip'


class DhtMeta(models.Model):
    start = models.DateTimeField()
    stop = models.DateTimeField()
    total = models.IntegerField()
    timeout = models.IntegerField()
    errors = models.IntegerField()
    ut_metadata = models.IntegerField()
    torrents_found = models.IntegerField()

    class Meta:
        managed = False
        db_table = 'dht_meta'


class DhtPeers(models.Model):
    time = models.DateTimeField()
    peer = models.BinaryField()
    hash = models.BinaryField()

    class Meta:
        managed = False
        db_table = 'dht_peers'
        unique_together = (('hash', 'peer'),)


class DhtTorrents(models.Model):
    time = models.DateTimeField()
    hash = models.BinaryField(primary_key=True)
    name = models.CharField(max_length=256)
    files = models.IntegerField()
    size = models.BigIntegerField()

    class Meta:
        managed = False
        db_table = 'dht_torrents'
