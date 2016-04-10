from django.http import HttpResponse

import control_unix_socket

import json

# Create your views here.

def ctrl_get_cur_run_serv_list(request):

  data = control_unix_socket.ctrl_get_server_data("get_cur_run_servers")

  return HttpResponse(data)

def ctrl_get_serv_list(request):
  data = control_unix_socket.ctrl_get_server_data("get_servers")

  return HttpResponse(data)

def ctrl_get_test_list(request):
  data = control_unix_socket.ctrl_get_server_data("get_test_list")

  return HttpResponse(data)

def ctrl_get_detail(request, ip, port, sid):
  data = control_unix_socket.ctrl_get_server_detail_data("get_detail", ip, port, sid)

  return HttpResponse(data);
