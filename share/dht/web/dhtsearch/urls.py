
from django.urls import path
from . import views

urlpatterns = [
	path('', views.index, name='index'),
	path('search', views.search, name='search'),
	path('peers', views.peers, name='peers'),
	path('top24', views.top24, name='top24'),
	path('latest', views.latest, name='latest'),
	path('stats', views.stats, name='stats'),
]

