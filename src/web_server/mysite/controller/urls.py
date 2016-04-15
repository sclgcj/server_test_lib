from django.conf.urls import url

from . import views

app_name = 'controller'
urlpatterns = [
  url(r'^get_cur_run_serv_list/$', views.ctrl_get_cur_run_serv_list, name='get_cur_run_servers'),
  url(r'^get_servers/$', views.ctrl_get_serv_list, name="get_servers"),
  url(r'^get_test_list/$', views.ctrl_get_test_list, name="get_test_list"),
  #url(r'^detail/ip(\w+)port(\w+)id(.+)/$', views.ctrl_get_detail, name="detail"),
  url(r'^detail/(.+)/$', views.ctrl_get_detail, name="detail"),
]
